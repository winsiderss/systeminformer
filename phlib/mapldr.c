/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2023
 *
 */

#include <ph.h>
#include <mapimg.h>
#include <mapldr.h>

/**
 * Locates a loader entry in the current process.
 *
 * \param DllBase The base address of the DLL. Specify NULL if this is not a search criteria.
 * \param FullDllName The full name of the DLL. Specify NULL if this is not a search criteria.
 * \param BaseDllName The base name of the DLL. Specify NULL if this is not a search criteria.
 *
 * \remarks This function must be called with the loader lock acquired. The first entry matching all
 * of the specified values is returned.
 */
PLDR_DATA_TABLE_ENTRY PhFindLoaderEntry(
    _In_opt_ PVOID DllBase,
    _In_opt_ PPH_STRINGREF FullDllName,
    _In_opt_ PPH_STRINGREF BaseDllName
    )
{
    PLDR_DATA_TABLE_ENTRY result = NULL;
    PLDR_DATA_TABLE_ENTRY entry;
    PH_STRINGREF fullDllName;
    PH_STRINGREF baseDllName;
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;

    listHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    listEntry = listHead->Flink;

    while (listEntry != listHead)
    {
        entry = CONTAINING_RECORD(listEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        PhUnicodeStringToStringRef(&entry->FullDllName, &fullDllName);
        PhUnicodeStringToStringRef(&entry->BaseDllName, &baseDllName);

        if (
            (!DllBase || entry->DllBase == DllBase) &&
            (!FullDllName || PhStartsWithStringRef(&fullDllName, FullDllName, TRUE)) &&
            (!BaseDllName || PhStartsWithStringRef(&baseDllName, BaseDllName, TRUE))
            )
        {
            result = entry;
            break;
        }

        listEntry = listEntry->Flink;
    }

    return result;
}

PLDR_DATA_TABLE_ENTRY PhFindLoaderEntryAddress(
    _In_ PVOID Address
    )
{
    PLDR_DATA_TABLE_ENTRY result = NULL;
    PLDR_DATA_TABLE_ENTRY entry;
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;

    listHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    listEntry = listHead->Flink;

    while (listEntry != listHead)
    {
        entry = CONTAINING_RECORD(listEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if (
            (ULONG_PTR)Address >= (ULONG_PTR)entry->DllBase &&
            (ULONG_PTR)Address < (ULONG_PTR)PTR_ADD_OFFSET(entry->DllBase, entry->SizeOfImage)
            )
        {
            result = entry;
            break;
        }

        listEntry = listEntry->Flink;
    }

    return result;
}

PLDR_DATA_TABLE_ENTRY PhFindLoaderEntryNameHash(
    _In_ ULONG BaseNameHash
    )
{
    PLDR_DATA_TABLE_ENTRY entry;
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;

    listHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    listEntry = listHead->Flink;

    while (listEntry != listHead)
    {
        entry = CONTAINING_RECORD(listEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if (entry->BaseNameHashValue == BaseNameHash)
            return entry;

        listEntry = listEntry->Flink;
    }

    return NULL;
}

/*!
    @brief PhLoadLibrary prevents the loader from searching in an unsafe
     order by first requiring the loader try to load and resolve through
     System32. Then upping the loading flags until the library is loaded.

    @param[in] FileName - The file name of the library to load.

    @return HMODULE to the library on success, null on failure.
*/
PVOID PhLoadLibrary(
    _In_ PCWSTR FileName
    )
{
    PVOID baseAddress;

    // Force LOAD_LIBRARY_SEARCH_SYSTEM32. If the library file name is a fully
    // qualified path this will succeed.

    if (baseAddress = LoadLibraryEx(
        FileName,
        NULL,
        LOAD_LIBRARY_SEARCH_SYSTEM32
        ))
    {
        return baseAddress;
    }

    // Include the application directory now.

    if (baseAddress = LoadLibraryEx(
        FileName,
        NULL,
        LOAD_LIBRARY_SEARCH_APPLICATION_DIR
        ))
    {
        return baseAddress;
    }

    if (WindowsVersion < WINDOWS_8)
    {
        // Note: This case is required for Windows 7 without KB2533623 (dmex)

        if (baseAddress = LoadLibraryEx(FileName, NULL, 0))
        {
            return baseAddress;
        }
    }

    return NULL;
}

BOOLEAN PhFreeLibrary(
    _In_ PVOID BaseAddress
    )
{
    return !!FreeLibrary(BaseAddress);
}

#ifdef DEBUG
// rev from BasepLoadLibraryAsDataFileInternal (dmex)
FORCEINLINE
VOID
NtSuppressDebugMessage(
    _In_ BOOLEAN SuppressDebugMessage
    )
{
    // Note: The Visual Studio debugger on Windows 7/8 attempts to load symbols
    // for non-executable resources. The kernelbase BasepLoadLibraryAsDataFileInternal
    // function temporarily suppresses symbols while loading resources (both release and debug)
    // as a QOL optimization but we only suppress debug messages for debug builds. This issue
    // was fixed on Windows 10 with SEC_IMAGE_NO_EXECUTE and suppression is not required (dmex)

    if (WindowsVersion < WINDOWS_10)
    {
        NtCurrentTeb()->SuppressDebugMsg = SuppressDebugMessage;
    }
}
#else
#define NtSuppressDebugMessage(SuppressDebugMessage)
#endif

// rev from LoadLibraryEx and LOAD_LIBRARY_AS_IMAGE_RESOURCE
// with some extra changes for a read-only page/section. (dmex)
NTSTATUS PhLoadLibraryAsResource(
    _In_ HANDLE FileHandle,
    _Out_opt_ PVOID *BaseAddress
    )
{
    NTSTATUS status;
    HANDLE sectionHandle;
    PVOID imageBaseAddress;
    SIZE_T imageBaseSize;

    status = NtCreateSection(
        &sectionHandle,
        SECTION_QUERY | SECTION_MAP_READ,
        NULL,
        NULL,
        PAGE_READONLY,
        WindowsVersion < WINDOWS_8 ? SEC_IMAGE : SEC_IMAGE_NO_EXECUTE,
        FileHandle
        );

    if (!NT_SUCCESS(status))
        return status;

    imageBaseAddress = NULL;
    imageBaseSize = 0;

    NtSuppressDebugMessage(TRUE);

    status = NtMapViewOfSection(
        sectionHandle,
        NtCurrentProcess(),
        &imageBaseAddress,
        0,
        0,
        NULL,
        &imageBaseSize,
        ViewUnmap,
        WindowsVersion < WINDOWS_10_RS2 ? 0 : MEM_MAPPED,
        PAGE_READONLY
        );

    NtSuppressDebugMessage(FALSE);

    if (status == STATUS_IMAGE_NOT_AT_BASE)
    {
        status = STATUS_SUCCESS;
    }

    NtClose(sectionHandle);

    if (NT_SUCCESS(status))
    {
        // Windows returns the address with bitwise OR|2 for use with LDR_IS_IMAGEMAPPING (dmex)
        if (BaseAddress)
        {
            *BaseAddress = LDR_MAPPEDVIEW_TO_IMAGEMAPPING(imageBaseAddress);
        }
    }

    return status;
}

NTSTATUS PhLoadLibraryAsImageResource(
    _In_ PPH_STRINGREF FileName,
    _Out_opt_ PVOID *BaseAddress
    )
{
    NTSTATUS status;
    HANDLE fileHandle;

    status = PhCreateFile(
        &fileHandle,
        FileName,
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhLoadLibraryAsResource(fileHandle, BaseAddress);
    NtClose(fileHandle);

    return status;
}

NTSTATUS PhLoadLibraryAsImageResourceWin32(
    _In_ PPH_STRINGREF FileName,
    _Out_opt_ PVOID *BaseAddress
    )
{
    NTSTATUS status;
    HANDLE fileHandle;

    status = PhCreateFileWin32(
        &fileHandle,
        FileName->Buffer,
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhLoadLibraryAsResource(fileHandle, BaseAddress);
    NtClose(fileHandle);

    return status;
}

NTSTATUS PhFreeLibraryAsImageResource(
    _In_ PVOID BaseAddress
    )
{
    NTSTATUS status;

    NtSuppressDebugMessage(TRUE);

    status = NtUnmapViewOfSection(
        NtCurrentProcess(),
        LDR_IMAGEMAPPING_TO_MAPPEDVIEW(BaseAddress)
        );

    NtSuppressDebugMessage(FALSE);

    return status;
}

PVOID PhGetDllHandle(
    _In_ PWSTR DllName
    )
{
#if (PHNT_NATIVE_LDR)
    UNICODE_STRING dllName;
    PVOID dllHandle;

    RtlInitUnicodeString(&dllName, DllName);

    if (NT_SUCCESS(LdrGetDllHandle(NULL, NULL, &dllName, &dllHandle)))
        return dllHandle;
    else
        return NULL;
#else
    PH_STRINGREF baseDllName;

    PhInitializeStringRefLongHint(&baseDllName, DllName);

    return PhGetLoaderEntryDllBase(NULL, &baseDllName);
#endif
}

PVOID PhGetModuleProcAddress(
    _In_ PWSTR ModuleName,
    _In_opt_ PSTR ProcedureName
    )
{
#if (PHNT_NATIVE_LDR)
    PVOID module;

    module = PhGetDllHandle(ModuleName);

    if (module)
        return PhGetProcedureAddress(module, ProcedureName, 0);
    else
        return NULL;
#else
    if (IS_INTRESOURCE(ProcedureName))
        return PhGetDllProcedureAddress(ModuleName, NULL, PtrToUshort(ProcedureName));

    return PhGetDllProcedureAddress(ModuleName, ProcedureName, 0);
#endif
}

PVOID PhGetProcedureAddress(
    _In_ PVOID DllHandle,
    _In_opt_ PSTR ProcedureName,
    _In_opt_ ULONG ProcedureNumber
    )
{
#if (PHNT_NATIVE_LDR)
    NTSTATUS status;
    ANSI_STRING procedureName;
    PVOID procedureAddress;

    if (ProcedureName)
    {
        RtlInitAnsiString(&procedureName, ProcedureName);
        status = LdrGetProcedureAddress(
            DllHandle,
            &procedureName,
            0,
            &procedureAddress
            );
    }
    else
    {
        status = LdrGetProcedureAddress(
            DllHandle,
            NULL,
            ProcedureNumber,
            &procedureAddress
            );
    }

    if (!NT_SUCCESS(status))
        return NULL;

    return procedureAddress;
#else
    return PhGetDllBaseProcedureAddress(DllHandle, ProcedureName, (USHORT)ProcedureNumber);
#endif
}

typedef struct _PH_PROCEDURE_ADDRESS_REMOTE_CONTEXT
{
    PVOID DllBase;
    PPH_STRING FileName;
} PH_PROCEDURE_ADDRESS_REMOTE_CONTEXT, *PPH_PROCEDURE_ADDRESS_REMOTE_CONTEXT;

static BOOLEAN PhpGetProcedureAddressRemoteCallback(
    _In_ PLDR_DATA_TABLE_ENTRY Module,
    _In_ PPH_PROCEDURE_ADDRESS_REMOTE_CONTEXT Context
    )
{
    PH_STRINGREF fullDllName;

    PhUnicodeStringToStringRef(&Module->FullDllName, &fullDllName);

    if (PhEqualStringRef(&fullDllName, &Context->FileName->sr, TRUE))
    {
        Context->DllBase = Module->DllBase;
        return FALSE;
    }

    return TRUE;
}

/**
 * Gets the address of a procedure in a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param FileName The file name of the DLL containing the procedure.
 * \param ProcedureName The name of the procedure.
 * \param ProcedureNumber The ordinal of the procedure.
 * \param ProcedureAddress A variable which receives the address of the procedure in the address
 * space of the process.
 * \param DllBase A variable which receives the base address of the DLL containing the procedure.
 */
NTSTATUS PhGetProcedureAddressRemote(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_STRINGREF FileName,
    _In_opt_ PSTR ProcedureName,
    _In_opt_ USHORT ProcedureNumber,
    _Out_ PVOID *ProcedureAddress,
    _Out_opt_ PVOID *DllBase
    )
{
    NTSTATUS status;
    PPH_STRING fileName;
    PH_MAPPED_IMAGE mappedImage;
    PH_MAPPED_IMAGE_EXPORTS exports;
    PH_PROCEDURE_ADDRESS_REMOTE_CONTEXT context;
    PH_ENUM_PROCESS_MODULES_PARAMETERS parameters;
#ifdef _M_ARM64
    USHORT processArchitecture;
    ULONG exportsFlags;
#endif

    status = PhLoadMappedImageEx(
        FileName,
        NULL,
        &mappedImage
        );

    if (!NT_SUCCESS(status))
        return status;

    fileName = PhDosPathNameToNtPathName(FileName);

    if (PhIsNullOrEmptyString(fileName))
    {
        status = PhGetProcessMappedFileName(
            NtCurrentProcess(),
            mappedImage.ViewBase,
            &fileName
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    memset(&context, 0, sizeof(PH_PROCEDURE_ADDRESS_REMOTE_CONTEXT));
    context.FileName = fileName;

    memset(&parameters, 0, sizeof(PH_ENUM_PROCESS_MODULES_PARAMETERS));
    parameters.Callback = PhpGetProcedureAddressRemoteCallback;
    parameters.Context = &context;
    parameters.Flags = PH_ENUM_PROCESS_MODULES_TRY_MAPPED_FILE_NAME;

    switch (mappedImage.Magic)
    {
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        status = PhEnumProcessModules32Ex(ProcessHandle, &parameters);
        break;
    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
        status = PhEnumProcessModulesEx(ProcessHandle, &parameters);
        break;
    }

    PhDereferenceObject(fileName);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (!context.DllBase)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

#ifdef _M_ARM64
    exportsFlags = 0;
    if (mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
        mappedImage.NtHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_ARM64)
    {
        status = PhGetProcessArchitecture(ProcessHandle, &processArchitecture);

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        if (processArchitecture == IMAGE_FILE_MACHINE_AMD64)
            exportsFlags |= PH_GET_IMAGE_EXPORTS_ARM64EC;
    }

    status = PhGetMappedImageExportsEx(&exports, &mappedImage, exportsFlags);
#else
    status = PhGetMappedImageExports(&exports, &mappedImage);
#endif

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetMappedImageExportFunctionRemote(
        &exports,
        ProcedureName,
        ProcedureNumber,
        context.DllBase,
        ProcedureAddress
        );

    if (NT_SUCCESS(status))
    {
        if (DllBase)
            *DllBase = context.DllBase;
    }

CleanupExit:
    PhUnloadMappedImage(&mappedImage);

    return status;
}

// rev from LdrAccessResource (dmex)
//NTSTATUS PhAccessResource(
//    _In_ PVOID DllBase,
//    _In_ PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
//    _Out_opt_ PVOID *ResourceBuffer,
//    _Out_opt_ ULONG *ResourceLength
//    )
//{
//    PVOID baseAddress;
//
//    if (LDR_IS_DATAFILE(DllBase))
//        baseAddress = LDR_DATAFILE_TO_MAPPEDVIEW(DllBase);
//    else if (LDR_IS_IMAGEMAPPING(DllBase))
//        baseAddress = LDR_IMAGEMAPPING_TO_MAPPEDVIEW(DllBase);
//    else
//        baseAddress = DllBase;
//
//    if (ResourceLength)
//    {
//        *ResourceLength = ResourceDataEntry->Size;
//    }
//
//    if (ResourceBuffer)
//    {
//        if (LDR_IS_DATAFILE(DllBase))
//        {
//            NTSTATUS status;
//
//            status = PhLoaderEntryImageRvaToVa(
//                baseAddress,
//                ResourceDataEntry->OffsetToData,
//                ResourceBuffer
//                );
//
//            return status;
//        }
//        else
//        {
//            *ResourceBuffer = PTR_ADD_OFFSET(
//                baseAddress,
//                ResourceDataEntry->OffsetToData
//                );
//
//            return STATUS_SUCCESS;
//        }
//    }
//
//    return STATUS_SUCCESS;
//}

/**
 * Finds and returns the address of a resource.
 *
 * \param DllBase The base address of the image containing the resource.
 * \param Name The name of the resource or the integer identifier.
 * \param Type The type of resource or the integer identifier.
 * \param ResourceLength A variable which will receive the length of the resource block.
 * \param ResourceBuffer A variable which receives the address of the resource data.
 *
 * \return TRUE if the resource was found, FALSE if an error occurred.
 *
 * \remarks Use this function instead of LoadResource() because no memory allocation is required.
 * This function returns the address of the resource from the read-only section of the image
 * and does not need to be allocated or deallocated. This function cannot be used when the
 * image will be unloaded since the validity of the address is tied to the lifetime of the image.
 */
_Success_(return)
BOOLEAN PhLoadResource(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _Out_opt_ ULONG *ResourceLength,
    _Out_opt_ PVOID *ResourceBuffer
    )
{
    LDR_RESOURCE_INFO resourceInfo;
    PIMAGE_RESOURCE_DATA_ENTRY resourceData;
    ULONG resourceLength;
    PVOID resourceBuffer;

    resourceInfo.Type = (ULONG_PTR)Type;
    resourceInfo.Name = (ULONG_PTR)Name;
    resourceInfo.Language = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

    if (!NT_SUCCESS(LdrFindResource_U(DllBase, &resourceInfo, RESOURCE_DATA_LEVEL, &resourceData)))
        return FALSE;

    if (!NT_SUCCESS(LdrAccessResource(DllBase, resourceData, &resourceBuffer, &resourceLength)))
        return FALSE;

    if (ResourceLength)
        *ResourceLength = resourceLength;
    if (ResourceBuffer)
        *ResourceBuffer = resourceBuffer;

    return TRUE;
}

 /**
  * Finds and returns a copy of the resource.
  *
  * \param DllBase The base address of the image containing the resource.
  * \param Name The name of the resource or the integer identifier.
  * \param Type The type of resource or the integer identifier.
  * \param ResourceLength A variable which will receive the length of the resource block.
  * \param ResourceBuffer A variable which receives the address of the resource data.
  *
  * \return TRUE if the resource was found, FALSE if an error occurred.
  *
  * \remarks This function returns a copy of a resource from heap memory
  * and must be deallocated. Use this function when the image will be unloaded.
  */
_Success_(return)
BOOLEAN PhLoadResourceCopy(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _Out_opt_ ULONG *ResourceLength,
    _Out_opt_ PVOID *ResourceBuffer
    )
{
    ULONG resourceLength;
    PVOID resourceBuffer;

    if (!PhLoadResource(DllBase, Name, Type, &resourceLength, &resourceBuffer))
        return FALSE;

    if (ResourceLength)
        *ResourceLength = resourceLength;
    if (ResourceBuffer)
        *ResourceBuffer = PhAllocateCopy(resourceBuffer, resourceLength);

    return TRUE;
}

// rev from LoadString (dmex)
/**
 * Loads a string resource from the image into a buffer.
 *
 * \param DllBase The base address of the image containing the resource.
 * \param ResourceId The identifier of the string to be loaded.
 *
 * \return A string containing a copy of the string resource.
 */
PPH_STRING PhLoadString(
    _In_ PVOID DllBase,
    _In_ ULONG ResourceId
    )
{
    ULONG resourceId = (LOWORD(ResourceId) >> 4) + 1;
    PIMAGE_RESOURCE_DIR_STRING_U stringBuffer;
    PPH_STRING string = NULL;
    ULONG resourceLength;
    PVOID resourceBuffer;
    ULONG stringIndex;

    if (!PhLoadResource(
        DllBase,
        MAKEINTRESOURCE(resourceId),
        RT_STRING,
        &resourceLength,
        &resourceBuffer
        ))
    {
        return NULL;
    }

    stringBuffer = resourceBuffer;
    stringIndex = ResourceId & 0x000F;

    for (ULONG i = 0; i < stringIndex; i++)
    {
        stringBuffer = PTR_ADD_OFFSET(stringBuffer, (stringBuffer->Length + sizeof(ANSI_NULL)) * sizeof(WCHAR)); // ANSI_NULL required (dmex)
    }

    if (
        stringBuffer->Length > 0 && // sizeof(UNICODE_NULL) || resourceLength
        stringBuffer->Length < UNICODE_STRING_MAX_BYTES
        )
    {
        string = PhCreateStringEx(
            stringBuffer->NameString,
            stringBuffer->Length * sizeof(WCHAR)
            );
    }

    return string;
}

// rev from SHLoadIndirectString (dmex)
/**
 * Extracts a specified text resource when given that resource in the form of an indirect string (a string that begins with the '@' symbol).
 *
 * \param SourceString The indirect string from which the resource will be retrieved.
 */
PPH_STRING PhLoadIndirectString(
    _In_ PPH_STRINGREF SourceString
    )
{
    PPH_STRING indirectString = NULL;

    if (SourceString->Buffer[0] == L'@')
    {
        PPH_STRING libraryString;
        PVOID libraryModule;
        PH_STRINGREF sourceRef;
        PH_STRINGREF dllNameRef;
        PH_STRINGREF dllIndexRef;
        LONG64 index64;
        LONG index;

        sourceRef.Length = SourceString->Length;
        sourceRef.Buffer = SourceString->Buffer;
        PhSkipStringRef(&sourceRef, sizeof(WCHAR)); // Skip the @ character.

        if (!PhSplitStringRefAtChar(&sourceRef, L',', &dllNameRef, &dllIndexRef))
            return NULL;

        if (!PhStringToInteger64(&dllIndexRef, 10, &index64))
        {
            // HACK: Services.exe includes custom logic for indirect Service description strings by reading descriptions from inf files,
            // these strings use the following format: "@FileName.inf,%SectionKeyName%;DefaultString".
            // Return the last token of the service string instead of locating and parsing the inf file with GetPrivateProfileString.
            if (PhSplitStringRefAtChar(&sourceRef, L';', &dllNameRef, &dllIndexRef)) // dllIndexRef.Buffer[0] == L'%'
            {
                return PhCreateString2(&dllIndexRef);
            }

            return NULL;
        }

        libraryString = PhCreateString2(&dllNameRef);
        index = (LONG)index64;

        if (libraryString->Buffer[0] == L'%')
        {
            PPH_STRING expandedString;

            if (expandedString = PhExpandEnvironmentStrings(&libraryString->sr))
                PhMoveReference(&libraryString, expandedString);
        }

        if (PhDetermineDosPathNameType(&libraryString->sr) == RtlPathTypeRelative)
        {
            PPH_STRING librarySearchPathString;

            if (librarySearchPathString = PhSearchFilePath(libraryString->Buffer, L".dll"))
            {
                PhMoveReference(&libraryString, librarySearchPathString);
            }
        }

        if (NT_SUCCESS(PhLoadLibraryAsImageResourceWin32(&libraryString->sr, &libraryModule)))
        {
            indirectString = PhLoadString(libraryModule, -index);
            PhFreeLibraryAsImageResource(libraryModule);
        }

        PhDereferenceObject(libraryString);
    }

    return indirectString;
}

/**
 * Retrieves the file name of a DLL loaded by the current process.
 *
 * \param DllBase The base address of the DLL.
 * \param IndexOfFileName A variable which receives the index of the base name of the DLL in the
 * returned string.
 *
 * \return The file name of the DLL, or NULL if the DLL could not be found.
 */
_Success_(return != NULL)
PPH_STRING PhGetDllFileName(
    _In_ PVOID DllBase,
    _Out_opt_ PULONG IndexOfFileName
    )
{
    PLDR_DATA_TABLE_ENTRY entry;
    PPH_STRING fileName;
    ULONG_PTR indexOfFileName;

    RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);

    entry = PhFindLoaderEntry(DllBase, NULL, NULL);

    if (entry)
        fileName = PhCreateStringFromUnicodeString(&entry->FullDllName);
    else
        fileName = NULL;

    RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);

    if (!fileName)
        return NULL;

    PhMoveReference(&fileName, PhGetFileName(fileName));

    if (IndexOfFileName)
    {
        indexOfFileName = PhFindLastCharInString(fileName, 0, OBJ_NAME_PATH_SEPARATOR);

        if (indexOfFileName != SIZE_MAX)
            indexOfFileName++;
        else
            indexOfFileName = 0;

        *IndexOfFileName = (ULONG)indexOfFileName;
    }

    return fileName;
}

PVOID PhGetLoaderEntryAddressDllBase(
    _In_ PVOID Address
    )
{
    PLDR_DATA_TABLE_ENTRY ldrEntry;
    PPEB peb = NtCurrentPeb();

    RtlEnterCriticalSection(peb->LoaderLock);
    ldrEntry = PhFindLoaderEntryAddress(Address);
    RtlLeaveCriticalSection(peb->LoaderLock);

    if (ldrEntry)
        return ldrEntry->DllBase;
    else
        return NULL;
}

PVOID PhGetLoaderEntryDllBase(
    _In_opt_ PPH_STRINGREF FullDllName,
    _In_opt_ PPH_STRINGREF BaseDllName
    )
{
    PLDR_DATA_TABLE_ENTRY ldrEntry;
    PPEB peb = NtCurrentPeb();

    RtlEnterCriticalSection(peb->LoaderLock);

    if (WindowsVersion >= WINDOWS_8 && !FullDllName && BaseDllName)
    {
        ULONG baseNameHash = PhHashStringRefEx(BaseDllName, TRUE, PH_STRING_HASH_X65599);

        ldrEntry = PhFindLoaderEntryNameHash(baseNameHash);
    }
    else
    {
        ldrEntry = PhFindLoaderEntry(NULL, FullDllName, BaseDllName);
    }

    RtlLeaveCriticalSection(peb->LoaderLock);

    if (ldrEntry)
        return ldrEntry->DllBase;
    else
        return NULL;
}

PVOID PhGetDllBaseProcedureAddress(
    _In_ PVOID DllBase,
    _In_opt_ PSTR ProcedureName,
    _In_opt_ USHORT ProcedureNumber
    )
{
    PIMAGE_NT_HEADERS imageNtHeader;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_EXPORT_DIRECTORY exportDirectory;

    if (!NT_SUCCESS(PhGetLoaderEntryImageNtHeaders(DllBase, &imageNtHeader)))
        return NULL;

    if (!NT_SUCCESS(PhGetLoaderEntryImageDirectory(
        DllBase,
        imageNtHeader,
        IMAGE_DIRECTORY_ENTRY_EXPORT,
        &dataDirectory,
        &exportDirectory,
        NULL
        )))
        return NULL;

    return PhGetLoaderEntryImageExportFunction(
        DllBase,
        dataDirectory,
        exportDirectory,
        ProcedureName,
        ProcedureNumber
        );
}

PVOID PhGetDllBaseProcAddress(
    _In_ PVOID BaseAddress,
    _In_opt_ PSTR ProcedureName
    )
{
    if (IS_INTRESOURCE(ProcedureName))
        return PhGetDllBaseProcedureAddress(BaseAddress, NULL, PtrToUshort(ProcedureName));

    return PhGetDllBaseProcedureAddress(BaseAddress, ProcedureName, 0);
}

PVOID PhGetDllProcedureAddress(
    _In_ PWSTR DllName,
    _In_opt_ PSTR ProcedureName,
    _In_opt_ USHORT ProcedureNumber
    )
{
    PVOID baseAddress;
    PH_STRINGREF baseDllName;

    PhInitializeStringRefLongHint(&baseDllName, DllName);

    if (!(baseAddress = PhGetLoaderEntryDllBase(NULL, &baseDllName)))
        return NULL;

    //if (!(baseAddress = PhGetLoaderEntryDllBaseZ(DllName)))
    //    return NULL;

    return PhGetDllBaseProcedureAddress(
        baseAddress,
        ProcedureName,
        ProcedureNumber
        );
}

NTSTATUS PhGetLoaderEntryImageNtHeaders(
    _In_ PVOID BaseAddress,
    _Out_ PIMAGE_NT_HEADERS *ImageNtHeaders
    )
{
    PIMAGE_DOS_HEADER imageDosHeader;
    PIMAGE_NT_HEADERS imageNtHeaders;
    ULONG imageNtHeadersOffset;

    imageDosHeader = PTR_ADD_OFFSET(BaseAddress, 0);

    if (imageDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return STATUS_INVALID_IMAGE_NOT_MZ;

    imageNtHeadersOffset = (ULONG)imageDosHeader->e_lfanew;

    if (imageNtHeadersOffset == 0 || imageNtHeadersOffset >= LONG_MAX)
        return STATUS_INVALID_IMAGE_FORMAT;

    imageNtHeaders = PTR_ADD_OFFSET(BaseAddress, imageNtHeadersOffset);

    if (imageNtHeaders->Signature != IMAGE_NT_SIGNATURE)
        return STATUS_INVALID_IMAGE_FORMAT;

    *ImageNtHeaders = imageNtHeaders;
    return STATUS_SUCCESS;
}

NTSTATUS PhGetLoaderEntryImageEntryPoint(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_NT_HEADERS ImageNtHeader,
    _Out_ PLDR_INIT_ROUTINE *ImageEntryPoint
    )
{
    if (ImageNtHeader->OptionalHeader.AddressOfEntryPoint == 0)
        return STATUS_ENTRYPOINT_NOT_FOUND;

    *ImageEntryPoint = PTR_ADD_OFFSET(BaseAddress, ImageNtHeader->OptionalHeader.AddressOfEntryPoint);
    return STATUS_SUCCESS;
}

NTSTATUS PhGetLoaderEntryImageDirectory(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_NT_HEADERS ImageNtHeader,
    _In_ ULONG ImageDirectoryIndex,
    _Out_ PIMAGE_DATA_DIRECTORY *ImageDataDirectoryEntry,
    _Out_ PVOID *ImageDirectoryEntry,
    _Out_opt_ SIZE_T *ImageDirectoryLength
    )
{
    PIMAGE_DATA_DIRECTORY directory;

    directory = &ImageNtHeader->OptionalHeader.DataDirectory[ImageDirectoryIndex];

    //if (ImageNtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
    //{
    //    PIMAGE_OPTIONAL_HEADER32 optionalHeader;
    //
    //    optionalHeader = &((PIMAGE_NT_HEADERS32)ImageNtHeader)->OptionalHeader;
    //
    //    if (ImageDirectoryIndex >= optionalHeader->NumberOfRvaAndSizes)
    //        return STATUS_INVALID_FILE_FOR_SECTION;
    //
    //    directory = &optionalHeader->DataDirectory[ImageDirectoryIndex];
    //}
    //else if (ImageNtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
    //{
    //    PIMAGE_OPTIONAL_HEADER64 optionalHeader;
    //
    //    optionalHeader = &((PIMAGE_NT_HEADERS64)ImageNtHeader)->OptionalHeader;
    //
    //    if (ImageDirectoryIndex >= optionalHeader->NumberOfRvaAndSizes)
    //        return STATUS_INVALID_FILE_FOR_SECTION;
    //
    //    directory = &optionalHeader->DataDirectory[ImageDirectoryIndex];
    //}
    //else
    //{
    //    return STATUS_INVALID_FILE_FOR_SECTION;
    //}

    if (directory->VirtualAddress == 0 || directory->Size == 0)
        return STATUS_INVALID_FILE_FOR_SECTION;

    *ImageDataDirectoryEntry = directory;
    *ImageDirectoryEntry = PTR_ADD_OFFSET(BaseAddress, directory->VirtualAddress);
    if (ImageDirectoryLength) *ImageDirectoryLength = directory->Size;

    return STATUS_SUCCESS;
}

NTSTATUS PhGetLoaderEntryImageVaToSection(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_NT_HEADERS ImageNtHeader,
    _In_ PVOID ImageDirectoryAddress,
    _Out_ PVOID *ImageSectionAddress,
    _Out_ SIZE_T *ImageSectionLength
    )
{
    SIZE_T directorySectionLength = 0;
    PIMAGE_SECTION_HEADER section;
    PIMAGE_SECTION_HEADER sectionHeader;
    PVOID directorySectionAddress = NULL;

    section = IMAGE_FIRST_SECTION(ImageNtHeader);

    for (ULONG i = 0; i < ImageNtHeader->FileHeader.NumberOfSections; i++)
    {
        sectionHeader = PTR_ADD_OFFSET(section, UInt32x32To64(sizeof(IMAGE_SECTION_HEADER), i));

        if (
            ((ULONG_PTR)ImageDirectoryAddress >= (ULONG_PTR)PTR_ADD_OFFSET(BaseAddress, sectionHeader->VirtualAddress)) &&
            ((ULONG_PTR)ImageDirectoryAddress < (ULONG_PTR)PTR_ADD_OFFSET(PTR_ADD_OFFSET(BaseAddress, sectionHeader->VirtualAddress), sectionHeader->SizeOfRawData))
            )
        {
            directorySectionLength = sectionHeader->Misc.VirtualSize;
            directorySectionAddress = PTR_ADD_OFFSET(BaseAddress, sectionHeader->VirtualAddress);
            break;
        }
    }

    if (directorySectionAddress && directorySectionLength)
    {
        *ImageSectionAddress = directorySectionAddress;
        *ImageSectionLength = directorySectionLength;
        return STATUS_SUCCESS;
    }

    return STATUS_SECTION_NOT_IMAGE;
}

NTSTATUS PhLoaderEntryImageRvaToSection(
    _In_ PIMAGE_NT_HEADERS ImageNtHeader,
    _In_ ULONG Rva,
    _Out_ PIMAGE_SECTION_HEADER *ImageSection,
    _Out_ SIZE_T *ImageSectionLength
    )
{
    SIZE_T directorySectionLength = 0;
    PIMAGE_SECTION_HEADER section;
    PIMAGE_SECTION_HEADER sectionHeader;
    PIMAGE_SECTION_HEADER directorySectionHeader = NULL;

    section = IMAGE_FIRST_SECTION(ImageNtHeader);

    for (ULONG i = 0; i < ImageNtHeader->FileHeader.NumberOfSections; i++)
    {
        sectionHeader = PTR_ADD_OFFSET(section, UInt32x32To64(sizeof(IMAGE_SECTION_HEADER), i));

        if (
            ((ULONG_PTR)Rva >= (ULONG_PTR)sectionHeader->VirtualAddress) &&
            ((ULONG_PTR)Rva < (ULONG_PTR)PTR_ADD_OFFSET(sectionHeader->VirtualAddress, sectionHeader->SizeOfRawData))
            )
        {
            directorySectionLength = sectionHeader->Misc.VirtualSize;
            directorySectionHeader = sectionHeader;
            break;
        }
    }

    if (directorySectionHeader && directorySectionLength)
    {
        *ImageSection = directorySectionHeader;
        *ImageSectionLength = directorySectionLength;
        return STATUS_SUCCESS;
    }

    return STATUS_SECTION_NOT_IMAGE;
}

NTSTATUS PhLoaderEntryImageRvaToVa(
    _In_ PVOID BaseAddress,
    _In_ ULONG Rva,
    _Out_ PVOID *Va
    )
{
    NTSTATUS status;
    SIZE_T imageSectionSize;
    PIMAGE_SECTION_HEADER imageSection;
    PIMAGE_NT_HEADERS imageNtHeader;

    status = PhGetLoaderEntryImageNtHeaders(
        BaseAddress,
        &imageNtHeader
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhLoaderEntryImageRvaToSection(
        imageNtHeader,
        Rva,
        &imageSection,
        &imageSectionSize
        );

    if (!NT_SUCCESS(status))
        return status;

    *Va = PTR_ADD_OFFSET(BaseAddress, PTR_ADD_OFFSET(
        PTR_SUB_OFFSET(Rva, imageSection->VirtualAddress),
        imageSection->PointerToRawData
        ));

    return STATUS_SUCCESS;
}

VOID PhLoaderEntryGrantSuppressedCall(
    _In_ PVOID ExportAddress
    )
{
#if (PH_NATIVE_LOADER_FLOWGUARD)
    static BOOLEAN PhLoaderEntryCacheInitialized = FALSE;
    static BOOLEAN (NTAPI* LdrControlFlowGuardEnforced_I)(VOID) = NULL;
    static PPH_HASHTABLE PhLoaderEntryCacheHashtable = NULL;

    if (!PhLoaderEntryCacheInitialized && PhInstanceHandle) // delay initialize (dmex)
    {
        PhLoaderEntryCacheInitialized = TRUE;

        if (!LdrControlFlowGuardEnforced_I && WindowsVersion >= WINDOWS_10)
        {
            LdrControlFlowGuardEnforced_I = PhGetDllProcedureAddress(L"ntdll.dll", "LdrControlFlowGuardEnforced", 0);
        }

        if (LdrControlFlowGuardEnforced_I && LdrControlFlowGuardEnforced_I())
        {
            PhGuardGrantSuppressedCallAccess(NtCurrentProcess(), (PVOID)SIZE_T_MAX); // initialize imports (dmex)

            PhLoaderEntryCacheHashtable = PhCreateSimpleHashtable(10);
        }

        return;
    }

    if (PhLoaderEntryCacheHashtable && !PhFindItemSimpleHashtable(PhLoaderEntryCacheHashtable, ExportAddress))
    {
        if (NT_SUCCESS(PhGuardGrantSuppressedCallAccess(NtCurrentProcess(), ExportAddress)))
        {
            PhAddItemSimpleHashtable(PhLoaderEntryCacheHashtable, ExportAddress, UlongToPtr(TRUE));
        }
    }
#endif
}

static ULONG PhpLookupLoaderEntryImageExportFunctionIndex(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    _In_ PULONG ExportNameTable,
    _In_ PSTR ExportName
    )
{
    LONG low;
    LONG high;
    LONG i;

    if (ExportDirectory->NumberOfNames == 0)
        return ULONG_MAX;

    low = 0;
    high = ExportDirectory->NumberOfNames - 1;

    do
    {
        PSTR name;
        INT comparison;

        i = (low + high) / 2;
        name = PTR_ADD_OFFSET(BaseAddress, ExportNameTable[i]);

        if (!name)
            return ULONG_MAX;

        comparison = strcmp(ExportName, name);

        if (comparison == 0)
            return i;
        else if (comparison < 0)
            high = i - 1;
        else
            low = i + 1;
    } while (low <= high);

    return ULONG_MAX;
}

PVOID PhGetLoaderEntryImageExportFunction(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_DATA_DIRECTORY DataDirectory,
    _In_ PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    _In_opt_ PSTR ExportName,
    _In_opt_ USHORT ExportOrdinal
    )
{
    PVOID exportAddress = NULL;
    PULONG exportAddressTable;
    PULONG exportNameTable;
    PUSHORT exportOrdinalTable;

    exportAddressTable = PTR_ADD_OFFSET(BaseAddress, ExportDirectory->AddressOfFunctions);
    exportNameTable = PTR_ADD_OFFSET(BaseAddress, ExportDirectory->AddressOfNames);
    exportOrdinalTable = PTR_ADD_OFFSET(BaseAddress, ExportDirectory->AddressOfNameOrdinals);

    if (ExportOrdinal)
    {
        if (ExportOrdinal > ExportDirectory->Base + ExportDirectory->NumberOfFunctions)
            return NULL;

        exportAddress = PTR_ADD_OFFSET(BaseAddress, exportAddressTable[ExportOrdinal - ExportDirectory->Base]);
    }
    else if (ExportName)
    {
        ULONG exportIndex;

        exportIndex = PhpLookupLoaderEntryImageExportFunctionIndex(
            BaseAddress,
            ExportDirectory,
            exportNameTable,
            ExportName
            );

        if (exportIndex == ULONG_MAX)
            return NULL;

        exportAddress = PTR_ADD_OFFSET(BaseAddress, exportAddressTable[exportOrdinalTable[exportIndex]]);

        //for (exportIndex = 0; exportIndex < ExportDirectory->NumberOfNames; exportIndex++)
        //{
        //    if (PhEqualBytesZ(ExportName, PTR_ADD_OFFSET(BaseAddress, exportNameTable[exportIndex]), FALSE))
        //    {
        //        exportAddress = PTR_ADD_OFFSET(BaseAddress, exportAddressTable[exportOrdinalTable[exportIndex]]);
        //        break;
        //    }
        //}
    }

    if (!exportAddress)
        return NULL;

    if (
        ((ULONG_PTR)exportAddress >= (ULONG_PTR)ExportDirectory) &&
        ((ULONG_PTR)exportAddress < (ULONG_PTR)PTR_ADD_OFFSET(ExportDirectory, DataDirectory->Size))
        )
    {
        WCHAR dllForwarderName[DOS_MAX_PATH_LENGTH] = L"";
        PH_STRINGREF dllForwarderRef;
        PH_STRINGREF dllNameRef;
        PH_STRINGREF dllProcedureRef;

        // This is a forwarder RVA.

        PhZeroExtendToUtf16Buffer((PSTR)exportAddress, strlen((PSTR)exportAddress), dllForwarderName);
        PhInitializeStringRefLongHint(&dllForwarderRef, dllForwarderName);

        if (PhSplitStringRefAtChar(&dllForwarderRef, L'.', &dllNameRef, &dllProcedureRef))
        {
            PVOID libraryDllBase;
            WCHAR libraryName[DOS_MAX_PATH_LENGTH] = L"";

            if (memcpy_s(libraryName, sizeof(libraryName), dllNameRef.Buffer, dllNameRef.Length))
            {
                return NULL;
            }

            libraryDllBase = PhLoadLibrary(libraryName);

            if (libraryDllBase)
            {
                CHAR libraryFunctionName[DOS_MAX_PATH_LENGTH] = "";

                if (!PhConvertUtf16ToUtf8Buffer(
                    libraryFunctionName,
                    sizeof(libraryFunctionName),
                    NULL,
                    dllProcedureRef.Buffer,
                    dllProcedureRef.Length
                    ))
                {
                    return NULL;
                }

                if (libraryFunctionName[0] == '#') // This is a forwarder RVA with an ordinal import.
                {
                    LONG64 importOrdinal;

                    PhSkipStringRef(&dllProcedureRef, sizeof(L'#'));

                    if (PhStringToInteger64(&dllProcedureRef, 10, &importOrdinal))
                        exportAddress = PhGetDllBaseProcedureAddress(libraryDllBase, NULL, (USHORT)importOrdinal);
                    else
                        exportAddress = PhGetDllBaseProcedureAddress(libraryDllBase, libraryFunctionName, 0);
                }
                else
                {
                    exportAddress = PhGetDllBaseProcedureAddress(libraryDllBase, libraryFunctionName, 0);
                }
            }
        }
    }

    PhLoaderEntryGrantSuppressedCall(exportAddress);

    return exportAddress;
}

PVOID PhGetDllBaseProcedureAddressWithHint(
    _In_ PVOID BaseAddress,
    _In_ PSTR ProcedureName,
    _In_ USHORT ProcedureHint
    )
{
    PIMAGE_NT_HEADERS imageNtHeader;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_EXPORT_DIRECTORY exportDirectory;

    if (!NT_SUCCESS(PhGetLoaderEntryImageNtHeaders(BaseAddress, &imageNtHeader)))
        return NULL;

    if (!NT_SUCCESS(PhGetLoaderEntryImageDirectory(
        BaseAddress,
        imageNtHeader,
        IMAGE_DIRECTORY_ENTRY_EXPORT,
        &dataDirectory,
        &exportDirectory,
        NULL
        )))
    {
        return NULL;
    }

    if (ProcedureHint < exportDirectory->NumberOfNames)
    {
        PULONG exportAddressTable = PTR_ADD_OFFSET(BaseAddress, exportDirectory->AddressOfFunctions);
        PULONG exportNameTable = PTR_ADD_OFFSET(BaseAddress, exportDirectory->AddressOfNames);
        PUSHORT exportOrdinalTable = PTR_ADD_OFFSET(BaseAddress, exportDirectory->AddressOfNameOrdinals);

        // If the import hint matches the export name then return the address.
        if (PhEqualBytesZ(ProcedureName, PTR_ADD_OFFSET(BaseAddress, exportNameTable[ProcedureHint]), FALSE))
        {
            PVOID exportAddress = PTR_ADD_OFFSET(BaseAddress, exportAddressTable[exportOrdinalTable[ProcedureHint]]);

            if (
                ((ULONG_PTR)exportAddress >= (ULONG_PTR)exportDirectory) &&
                ((ULONG_PTR)exportAddress < (ULONG_PTR)PTR_ADD_OFFSET(exportDirectory, dataDirectory->Size))
                )
            {
                WCHAR dllForwarderName[DOS_MAX_PATH_LENGTH] = L"";
                PH_STRINGREF dllForwarderRef;
                PH_STRINGREF dllNameRef;
                PH_STRINGREF dllProcedureRef;

                // This is a forwarder RVA.

                PhZeroExtendToUtf16Buffer((PSTR)exportAddress, strlen((PSTR)exportAddress), dllForwarderName);
                PhInitializeStringRefLongHint(&dllForwarderRef, dllForwarderName);

                if (PhSplitStringRefAtChar(&dllForwarderRef, L'.', &dllNameRef, &dllProcedureRef))
                {
                    PVOID libraryDllBase;
                    WCHAR libraryName[DOS_MAX_PATH_LENGTH] = L"";

                    if (memcpy_s(libraryName, sizeof(libraryName), dllNameRef.Buffer, dllNameRef.Length))
                    {
                        return NULL;
                    }

                    libraryDllBase = PhLoadLibrary(libraryName);

                    if (libraryDllBase)
                    {
                        CHAR libraryFunctionName[DOS_MAX_PATH_LENGTH] = "";

                        if (!PhConvertUtf16ToUtf8Buffer(
                            libraryFunctionName,
                            sizeof(libraryFunctionName),
                            NULL,
                            dllProcedureRef.Buffer,
                            dllProcedureRef.Length
                            ))
                        {
                            return NULL;
                        }

                        if (libraryFunctionName[0] == '#') // This is a forwarder RVA with an ordinal import.
                        {
                            LONG64 importOrdinal;

                            PhSkipStringRef(&dllProcedureRef, sizeof(L'#'));

                            if (PhStringToInteger64(&dllProcedureRef, 10, &importOrdinal))
                                exportAddress = PhGetDllBaseProcedureAddress(libraryDllBase, NULL, (USHORT)importOrdinal);
                            else
                                exportAddress = PhGetDllBaseProcedureAddress(libraryDllBase, libraryFunctionName, 0);
                        }
                        else
                        {
                            exportAddress = PhGetDllBaseProcedureAddress(libraryDllBase, libraryFunctionName, 0);
                        }
                    }
                }
            }

            return exportAddress;
        }
    }

    return PhGetDllBaseProcedureAddress(BaseAddress, ProcedureName, 0);
}

NTSTATUS PhLoaderEntryDetourImportProcedure(
    _In_ PVOID BaseAddress,
    _In_ PSTR ImportName,
    _In_ PSTR ProcedureName,
    _In_ PVOID FunctionAddress,
    _Out_opt_ PVOID* OriginalAddress
    )
{
    NTSTATUS status;
    PIMAGE_NT_HEADERS imageNtHeaders;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_IMPORT_DESCRIPTOR importDirectory;

    status = PhGetLoaderEntryImageNtHeaders(
        BaseAddress,
        &imageNtHeaders
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetLoaderEntryImageDirectory(
        BaseAddress,
        imageNtHeaders,
        IMAGE_DIRECTORY_ENTRY_IMPORT,
        &dataDirectory,
        &importDirectory,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = STATUS_UNSUCCESSFUL;

    while (importDirectory->Name && importDirectory->OriginalFirstThunk)
    {
        PSTR importName;
        PIMAGE_THUNK_DATA importThunk;
        PIMAGE_THUNK_DATA originalThunk;
        PIMAGE_IMPORT_BY_NAME importByName;

        importName = PTR_ADD_OFFSET(BaseAddress, importDirectory->Name);
        importThunk = PTR_ADD_OFFSET(BaseAddress, importDirectory->FirstThunk);
        originalThunk = PTR_ADD_OFFSET(BaseAddress, importDirectory->OriginalFirstThunk);

        if (PhEqualBytesZ(importName, ImportName, TRUE))
        {
            for (; originalThunk->u1.AddressOfData; originalThunk++, importThunk++) // while (originalThunk->u1.AddressOfData)
            {
                SIZE_T importThunkSize = sizeof(IMAGE_THUNK_DATA);
                PVOID importThunkAddress = importThunk;
                ULONG importThunkProtect = 0;

                if (IMAGE_SNAP_BY_ORDINAL(originalThunk->u1.Ordinal))
                    continue;

                importByName = PTR_ADD_OFFSET(BaseAddress, originalThunk->u1.AddressOfData);

                if (!PhEqualBytesZ(importByName->Name, ProcedureName, FALSE))
                    continue;

                if (OriginalAddress)
                    *OriginalAddress = (PVOID)importThunk->u1.Function;

                if (NT_SUCCESS(status = NtProtectVirtualMemory(
                    NtCurrentProcess(),
                    &importThunkAddress,
                    &importThunkSize,
                    PAGE_READWRITE,
                    &importThunkProtect
                    )))
                {
                    importThunk->u1.Function = (ULONG_PTR)FunctionAddress;

                    NtProtectVirtualMemory(
                        NtCurrentProcess(),
                        &importThunkAddress,
                        &importThunkSize,
                        importThunkProtect,
                        &importThunkProtect
                        );
#ifdef _M_ARM64
                    NtFlushInstructionCache(
                        NtCurrentProcess(),
                        importThunkAddress,
                        importThunkSize
                        );
#endif
                }
                break;
            }
        }

        importDirectory++;
    }

    return status;
}

VOID PhLoaderEntrySnapShowErrorMessage(
    _In_ PVOID BaseAddress,
    _In_ PSTR ImportName,
    _In_ PIMAGE_THUNK_DATA OriginalThunk
    )
{
    PPH_STRING fileName;

    if (NT_SUCCESS(PhGetProcessMappedFileName(NtCurrentProcess(), BaseAddress, &fileName)))
    {
        PhMoveReference(&fileName, PhGetFileName(fileName));
        PhMoveReference(&fileName, PhGetBaseName(fileName));

        if (IMAGE_SNAP_BY_ORDINAL(OriginalThunk->u1.Ordinal))
        {
            PhShowError(
                NULL,
                L"Unable to load plugin.\r\nName: %s\r\nOrdinal: %u\r\nModule: %hs",
                PhGetStringOrEmpty(fileName),
                IMAGE_ORDINAL(OriginalThunk->u1.Ordinal),
                ImportName
                );
        }
        else
        {
            PIMAGE_IMPORT_BY_NAME importByName;

            importByName = PTR_ADD_OFFSET(BaseAddress, OriginalThunk->u1.AddressOfData);

            PhShowError(
                NULL,
                L"Unable to load plugin.\r\nName: %s\r\nFunction: %hs\r\nModule: %hs",
                PhGetStringOrEmpty(fileName),
                importByName->Name,
                ImportName
                );
        }

        PhDereferenceObject(fileName);
    }
}

NTSTATUS PhLoaderEntrySnapImportThunk(
    _In_ PVOID BaseAddress,
    _In_ PVOID ImportBaseAddress,
    _In_ PIMAGE_THUNK_DATA OriginalThunk,
    _In_ PIMAGE_THUNK_DATA ImportThunk
    )
{
    if (IMAGE_SNAP_BY_ORDINAL(OriginalThunk->u1.Ordinal))
    {
        USHORT procedureOrdinal;
        PVOID procedureAddress;

        procedureOrdinal = IMAGE_ORDINAL(OriginalThunk->u1.Ordinal);
        procedureAddress = PhGetDllBaseProcedureAddress(ImportBaseAddress, NULL, procedureOrdinal);

        if (procedureAddress)
        {
            ImportThunk->u1.Function = (ULONG_PTR)procedureAddress;
            return STATUS_SUCCESS;
        }
    }
    else
    {
        PIMAGE_IMPORT_BY_NAME importByName;
        PVOID procedureAddress;

        importByName = PTR_ADD_OFFSET(BaseAddress, OriginalThunk->u1.AddressOfData);
        procedureAddress = PhGetDllBaseProcedureAddressWithHint(ImportBaseAddress, importByName->Name, importByName->Hint);

        if (procedureAddress)
        {
            ImportThunk->u1.Function = (ULONG_PTR)procedureAddress;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_ORDINAL_NOT_FOUND;
}

#if (PH_NATIVE_LOADER_WORKQUEUE)
typedef struct _PH_LOADER_IMPORT_THUNK_WORKQUEUE_CONTEXT
{
    PVOID BaseAddress;
    PVOID ImportBaseAddress;
    PIMAGE_THUNK_DATA OriginalThunkData;
    PIMAGE_THUNK_DATA ImportThunkData;
} PH_LOADER_IMPORT_THUNK_WORKQUEUE_CONTEXT, *PPH_LOADER_IMPORT_THUNK_WORKQUEUE_CONTEXT;

VOID CALLBACK LoaderEntryImageImportThunkWorkQueueCallback(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _Inout_ PTP_WORK Work
    )
{
    PPH_LOADER_IMPORT_THUNK_WORKQUEUE_CONTEXT context = (PPH_LOADER_IMPORT_THUNK_WORKQUEUE_CONTEXT)Context;
    NTSTATUS status;

    status = PhLoaderEntrySnapImportThunk(
        context->BaseAddress,
        context->ImportBaseAddress,
        context->OriginalThunkData,
        context->ImportThunkData
        );

    if (!NT_SUCCESS(status))
    {
#ifdef DEBUG
        PhLoaderEntrySnapShowErrorMessage(context->BaseAddress, "ThunkWorkQueue", context->OriginalThunkData);
#endif
        PhRaiseStatus(status);
    }

    PhFree(context);
}
#endif

NTSTATUS PhLoaderEntrySnapImportDirectory(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_IMPORT_DESCRIPTOR ImportDirectory
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PSTR importName;
    PIMAGE_THUNK_DATA importThunk;
    PIMAGE_THUNK_DATA originalThunk;
    PVOID importBaseAddress;

    importName = PTR_ADD_OFFSET(BaseAddress, ImportDirectory->Name);
    importThunk = PTR_ADD_OFFSET(BaseAddress, ImportDirectory->FirstThunk);
    originalThunk = PTR_ADD_OFFSET(BaseAddress, ImportDirectory->OriginalFirstThunk);

    if (PhEqualBytesZ(importName, "SystemInformer.exe", FALSE))
    {
        importBaseAddress = PhInstanceHandle;
    }
    else
    {
        WCHAR dllImportName[DOS_MAX_PATH_LENGTH] = L"";

        PhZeroExtendToUtf16Buffer(importName, strlen(importName), dllImportName);

        importBaseAddress = PhLoadLibrary(dllImportName);
    }

    if (!importBaseAddress)
    {
        return STATUS_DLL_NOT_FOUND;
    }

#if (PH_NATIVE_LOADER_WORKQUEUE)
    PTP_POOL loaderThreadpool;
    TP_CALLBACK_ENVIRON loaderThreadpoolEnvironment;
    PTP_CLEANUP_GROUP loaderThreadpoolCleanupGroup;

    status = TpAllocPool(&loaderThreadpool, NULL);

    if (!NT_SUCCESS(status))
        return status;

    status = TpAllocCleanupGroup(&loaderThreadpoolCleanupGroup);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = TpSetPoolMinThreads(loaderThreadpool, 1); // NtCurrentPeb()->ProcessParameters->LoaderThreads

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    TpSetPoolMaxThreads(loaderThreadpool, PhGetActiveProcessorCount(ALL_PROCESSOR_GROUPS));

    TpInitializeCallbackEnviron(&loaderThreadpoolEnvironment);
    TpSetCallbackThreadpool(&loaderThreadpoolEnvironment, loaderThreadpool);
    TpSetCallbackCleanupGroup(&loaderThreadpoolEnvironment, loaderThreadpoolCleanupGroup, NULL);
#endif

    while (originalThunk->u1.AddressOfData)
    {
#if (PH_NATIVE_LOADER_WORKQUEUE)
        PPH_LOADER_IMPORT_THUNK_WORKQUEUE_CONTEXT context;
        PTP_WORK loaderThreadpoolWork;

        context = PhAllocateZero(sizeof(PH_LOADER_IMPORT_THUNK_WORKQUEUE_CONTEXT));
        context->BaseAddress = BaseAddress;
        context->ImportBaseAddress = importBaseAddress;
        context->OriginalThunkData = originalThunk;
        context->ImportThunkData = importThunk;

        status = TpAllocWork(
            &loaderThreadpoolWork,
            LoaderEntryImageImportThunkWorkQueueCallback,
            context,
            &loaderThreadpoolEnvironment
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        TpPostWork(loaderThreadpoolWork);
#else
        status = PhLoaderEntrySnapImportThunk(
            BaseAddress,
            importBaseAddress,
            originalThunk,
            importThunk
            );

        if (!NT_SUCCESS(status))
        {
#ifdef DEBUG
            PhLoaderEntrySnapShowErrorMessage(BaseAddress, importName, originalThunk);
#endif
            break;
        }
#endif
        originalThunk++;
        importThunk++;
    }

#if (PH_NATIVE_LOADER_WORKQUEUE)
CleanupExit:
    TpReleaseCleanupGroupMembers(loaderThreadpoolCleanupGroup, FALSE, NULL);
    TpReleaseCleanupGroup(loaderThreadpoolCleanupGroup);
    TpReleasePool(loaderThreadpool);
#endif

    return status;
}

#if (PH_NATIVE_LOADER_WORKQUEUE)
typedef struct _PH_LOADER_IMPORTS_WORKQUEUE_CONTEXT
{
    PVOID BaseAddress;
    PIMAGE_IMPORT_DESCRIPTOR ImportDirectory;
} PH_LOADER_IMPORTS_WORKQUEUE_CONTEXT, *PPH_LOADER_IMPORTS_WORKQUEUE_CONTEXT;

VOID CALLBACK LoaderEntryImageImportsWorkQueueCallback(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _Inout_ PTP_WORK Work
    )
{
    PPH_LOADER_IMPORTS_WORKQUEUE_CONTEXT context = (PPH_LOADER_IMPORTS_WORKQUEUE_CONTEXT)Context;
    NTSTATUS status;

    status = PhLoaderEntrySnapImportDirectory(
        context->BaseAddress,
        context->ImportDirectory
        );

    if (!NT_SUCCESS(status))
    {
        PhRaiseStatus(status);
    }

    PhFree(context);
}
#endif

static NTSTATUS PhpFixupLoaderEntryImageImports(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_NT_HEADERS ImageNtHeader
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_IMPORT_DESCRIPTOR importDirectory;
    PVOID importDirectorySectionAddress;
    SIZE_T importDirectorySectionSize;
    ULONG importDirectoryProtect;

    status = PhGetLoaderEntryImageDirectory(
        BaseAddress,
        ImageNtHeader,
        IMAGE_DIRECTORY_ENTRY_IMPORT,
        &dataDirectory,
        &importDirectory,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetLoaderEntryImageVaToSection(
        BaseAddress,
        ImageNtHeader,
        importDirectory,
        &importDirectorySectionAddress,
        &importDirectorySectionSize
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtProtectVirtualMemory(
        NtCurrentProcess(),
        &importDirectorySectionAddress,
        &importDirectorySectionSize,
        PAGE_READWRITE,
        &importDirectoryProtect
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

#if (PH_NATIVE_LOADER_WORKQUEUE)
    PTP_POOL loaderThreadpool;
    TP_CALLBACK_ENVIRON loaderThreadpoolEnvironment;
    PTP_CLEANUP_GROUP loaderThreadpoolCleanupGroup;

    status = TpAllocPool(&loaderThreadpool, NULL);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = TpAllocCleanupGroup(&loaderThreadpoolCleanupGroup);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = TpSetPoolMinThreads(loaderThreadpool, 8);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    TpSetPoolMaxThreads(loaderThreadpool, 8);

    TpInitializeCallbackEnviron(&loaderThreadpoolEnvironment);
    TpSetCallbackThreadpool(&loaderThreadpoolEnvironment, loaderThreadpool);
    TpSetCallbackCleanupGroup(&loaderThreadpoolEnvironment, loaderThreadpoolCleanupGroup, NULL);
#endif

    while (importDirectory->Name && importDirectory->OriginalFirstThunk)
    {
#if (PH_NATIVE_LOADER_WORKQUEUE)
        PPH_LOADER_IMPORTS_WORKQUEUE_CONTEXT context;
        PTP_WORK loaderThreadpoolWork;

        context = PhAllocateZero(sizeof(PH_LOADER_IMPORTS_WORKQUEUE_CONTEXT));
        context->ImportDirectory = importDirectory;
        context->BaseAddress = BaseAddress;

        status = TpAllocWork(
            &loaderThreadpoolWork,
            LoaderEntryImageImportsWorkQueueCallback,
            context,
            &loaderThreadpoolEnvironment
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        TpPostWork(loaderThreadpoolWork);
#else
        status = PhLoaderEntrySnapImportDirectory(
            BaseAddress,
            importDirectory
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
#endif
        importDirectory++;
    }

#if (PH_NATIVE_LOADER_WORKQUEUE)
    // Wait for all callbacks to finish.
    TpReleaseCleanupGroupMembers(loaderThreadpoolCleanupGroup, FALSE, NULL);
    // Clean up the pool.
    TpReleaseCleanupGroup(loaderThreadpoolCleanupGroup);
    TpReleasePool(loaderThreadpool);
#endif

    status = NtProtectVirtualMemory(
        NtCurrentProcess(),
        &importDirectorySectionAddress,
        &importDirectorySectionSize,
        importDirectoryProtect,
        &importDirectoryProtect
        );

CleanupExit:

#ifdef _M_ARM64
    NtFlushInstructionCache(
        NtCurrentProcess(),
        importDirectorySectionAddress,
        importDirectorySectionSize
        );
#endif

    return status;
}

static NTSTATUS PhpFixupLoaderEntryImageDelayImports(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_NT_HEADERS ImageNtHeaders,
    _In_ PSTR ImportDllName
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_DELAYLOAD_DESCRIPTOR delayImportDirectory;
    PVOID importDirectorySectionAddress;
    SIZE_T importDirectorySectionSize;
    ULONG importDirectoryProtect;

    status = PhGetLoaderEntryImageDirectory(
        BaseAddress,
        ImageNtHeaders,
        IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT,
        &dataDirectory,
        &delayImportDirectory,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        if (status == STATUS_INVALID_FILE_FOR_SECTION)
            status = STATUS_SUCCESS;

        return status;
    }

    status = PhGetLoaderEntryImageVaToSection(
        BaseAddress,
        ImageNtHeaders,
        PTR_ADD_OFFSET(BaseAddress, delayImportDirectory->ImportAddressTableRVA),
        &importDirectorySectionAddress,
        &importDirectorySectionSize
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtProtectVirtualMemory(
        NtCurrentProcess(),
        &importDirectorySectionAddress,
        &importDirectorySectionSize,
        PAGE_READWRITE,
        &importDirectoryProtect
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    while (delayImportDirectory->ImportAddressTableRVA && delayImportDirectory->ImportNameTableRVA)
    {
        PSTR importName;
        PVOID* importHandle;
        PIMAGE_THUNK_DATA importAddressTable;
        PIMAGE_THUNK_DATA importNameTable;
        PVOID importBaseAddress;
        BOOLEAN importNeedsFree = FALSE;

        importName = PTR_ADD_OFFSET(BaseAddress, delayImportDirectory->DllNameRVA);
        importHandle = PTR_ADD_OFFSET(BaseAddress, delayImportDirectory->ModuleHandleRVA);
        importAddressTable = PTR_ADD_OFFSET(BaseAddress, delayImportDirectory->ImportAddressTableRVA);
        importNameTable = PTR_ADD_OFFSET(BaseAddress, delayImportDirectory->ImportNameTableRVA);

        if (PhEqualBytesZ(importName, ImportDllName, TRUE))
        {
            if (PhEqualBytesZ(importName, "SystemInformer.exe", FALSE))
            {
                importBaseAddress = PhInstanceHandle;
                status = STATUS_SUCCESS;
            }
            else if (*importHandle)
            {
                importBaseAddress = *importHandle;
                status = STATUS_SUCCESS;
            }
            else
            {
                WCHAR dllImportName[DOS_MAX_PATH_LENGTH] = L"";

                PhZeroExtendToUtf16Buffer(importName, strlen(importName), dllImportName);

                if (importBaseAddress = PhLoadLibrary(dllImportName))
                {
                    importNeedsFree = TRUE;
                    status = STATUS_SUCCESS;
                }
                else
                {
                    status = PhGetLastWin32ErrorAsNtStatus();
                }
            }

            if (!NT_SUCCESS(status))
                break;

            if (!importBaseAddress)
            {
                status = STATUS_UNSUCCESSFUL;
                break;
            }

            while (importNameTable->u1.AddressOfData)
            {
                status = PhLoaderEntrySnapImportThunk(
                    BaseAddress,
                    importBaseAddress,
                    importNameTable,
                    importAddressTable
                    );

                if (!NT_SUCCESS(status))
                    break;

                importAddressTable++;
                importNameTable++;
            }

            if ((InterlockedExchangePointer(importHandle, importBaseAddress) == importBaseAddress) && importNeedsFree)
            {
                PhFreeLibrary(importBaseAddress); // A different thread has already updated the cache.
            }
        }

        delayImportDirectory++;
    }

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtProtectVirtualMemory(
        NtCurrentProcess(),
        &importDirectorySectionAddress,
        &importDirectorySectionSize,
        importDirectoryProtect,
        &importDirectoryProtect
        );

CleanupExit:

#ifdef _M_ARM64
    NtFlushInstructionCache(
        NtCurrentProcess(),
        importDirectorySectionAddress,
        importDirectorySectionSize
        );
#endif

    return status;
}

NTSTATUS PhpLoaderEntryQuerySectionInformation(
    _In_ HANDLE SectionHandle
    )
{
    SECTION_IMAGE_INFORMATION section;
    NTSTATUS status;

    memset(&section, 0, sizeof(SECTION_IMAGE_INFORMATION));

    status = NtQuerySection(
        SectionHandle,
        SectionImageInformation,
        &section,
        sizeof(SECTION_IMAGE_INFORMATION),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (section.SubSystemType != IMAGE_SUBSYSTEM_WINDOWS_GUI)
        return STATUS_INVALID_IMPORT_OF_NON_DLL;
    if (!FlagOn(section.ImageCharacteristics, IMAGE_FILE_DLL | IMAGE_FILE_EXECUTABLE_IMAGE))
        return STATUS_INVALID_IMPORT_OF_NON_DLL;

#ifdef _WIN64
    if (section.Machine != IMAGE_FILE_MACHINE_AMD64 && section.Machine != IMAGE_FILE_MACHINE_ARM64)
        return STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
#else
    if (section.Machine != IMAGE_FILE_MACHINE_I386)
        return STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
#endif

    return status;
}

NTSTATUS PhLoaderEntryRelocateImage(
    _In_ PVOID BaseAddress)
{
    NTSTATUS status;
    PIMAGE_NT_HEADERS imageNtHeader;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_BASE_RELOCATION relocationDirectory;
    PVOID relocationDirectoryEnd;
    ULONG_PTR relocationDelta;

    status = PhGetLoaderEntryImageNtHeaders(
        BaseAddress,
        &imageNtHeader
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetLoaderEntryImageDirectory(
        BaseAddress,
        imageNtHeader,
        IMAGE_DIRECTORY_ENTRY_BASERELOC,
        &dataDirectory,
        &relocationDirectory,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (FlagOn(imageNtHeader->FileHeader.Characteristics, IMAGE_FILE_RELOCS_STRIPPED))
        return STATUS_SUCCESS;

    for (ULONG i = 0; i < imageNtHeader->FileHeader.NumberOfSections; i++)
    {
        PIMAGE_SECTION_HEADER sectionHeader;
        PVOID sectionHeaderAddress;
        SIZE_T sectionHeaderSize;
        ULONG sectionProtectionJunk = 0;

        sectionHeader = PTR_ADD_OFFSET(IMAGE_FIRST_SECTION(imageNtHeader), UInt32x32To64(sizeof(IMAGE_SECTION_HEADER), i));
        sectionHeaderAddress = PTR_ADD_OFFSET(BaseAddress, sectionHeader->VirtualAddress);
        sectionHeaderSize = sectionHeader->SizeOfRawData;

        status = NtProtectVirtualMemory(
            NtCurrentProcess(),
            &sectionHeaderAddress,
            &sectionHeaderSize,
            PAGE_READWRITE,
            &sectionProtectionJunk
            );

        if (!NT_SUCCESS(status))
            break;
    }

    if (!NT_SUCCESS(status))
        return status;

    relocationDirectoryEnd = PTR_ADD_OFFSET(relocationDirectory, dataDirectory->Size);
    relocationDelta = (ULONG_PTR)PTR_SUB_OFFSET(BaseAddress, imageNtHeader->OptionalHeader.ImageBase);

    while ((ULONG_PTR)relocationDirectory < (ULONG_PTR)relocationDirectoryEnd)
    {
        ULONG relocationCount;
        PVOID relocationAddress;
        PIMAGE_RELOCATION_RECORD relocations;

        relocationCount = (relocationDirectory->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(IMAGE_RELOCATION_RECORD);
        relocationAddress = PTR_ADD_OFFSET(BaseAddress, relocationDirectory->VirtualAddress);
        relocations = PTR_ADD_OFFSET(relocationDirectory, RTL_SIZEOF_THROUGH_FIELD(IMAGE_BASE_RELOCATION, SizeOfBlock));

        for (ULONG i = 0; i < relocationCount; i++)
        {
            switch (relocations[i].Type)
            {
            case IMAGE_REL_BASED_LOW:
                *(PUSHORT)PTR_ADD_OFFSET(relocationAddress, relocations[i].Offset) += (USHORT)relocationDelta;
                break;
            case IMAGE_REL_BASED_HIGHLOW:
                *(PULONG)PTR_ADD_OFFSET(relocationAddress, relocations[i].Offset) += (ULONG)relocationDelta;
                break;
            case IMAGE_REL_BASED_DIR64:
                *(PULONGLONG)PTR_ADD_OFFSET(relocationAddress, relocations[i].Offset) += (ULONGLONG)relocationDelta;
                break;
            }
        }

        relocationDirectory = PTR_ADD_OFFSET(relocationDirectory, relocationDirectory->SizeOfBlock);
    }

    for (ULONG i = 0; i < imageNtHeader->FileHeader.NumberOfSections; i++)
    {
        PIMAGE_SECTION_HEADER sectionHeader;
        PVOID sectionHeaderAddress;
        SIZE_T sectionHeaderSize;
        ULONG sectionProtection = 0;
        ULONG sectionProtectionJunk = 0;

        sectionHeader = PTR_ADD_OFFSET(IMAGE_FIRST_SECTION(imageNtHeader), UInt32x32To64(sizeof(IMAGE_SECTION_HEADER), i));
        sectionHeaderAddress = PTR_ADD_OFFSET(BaseAddress, sectionHeader->VirtualAddress);
        sectionHeaderSize = sectionHeader->SizeOfRawData;

        if (FlagOn(sectionHeader->Characteristics, IMAGE_SCN_MEM_READ))
            sectionProtection = PAGE_READONLY;
        if (FlagOn(sectionHeader->Characteristics, IMAGE_SCN_MEM_WRITE))
            sectionProtection = PAGE_WRITECOPY;
        if (FlagOn(sectionHeader->Characteristics, IMAGE_SCN_MEM_EXECUTE))
            sectionProtection = PAGE_EXECUTE;
        if (FlagOn(sectionHeader->Characteristics, IMAGE_SCN_MEM_READ) && FlagOn(sectionHeader->Characteristics, IMAGE_SCN_MEM_EXECUTE))
            sectionProtection = PAGE_EXECUTE_READ;
        if (FlagOn(sectionHeader->Characteristics, IMAGE_SCN_MEM_WRITE) && FlagOn(sectionHeader->Characteristics, IMAGE_SCN_MEM_EXECUTE))
            sectionProtection = PAGE_EXECUTE_READWRITE;

        status = NtProtectVirtualMemory(
            NtCurrentProcess(),
            &sectionHeaderAddress,
            &sectionHeaderSize,
            sectionProtection,
            &sectionProtectionJunk
            );

#ifdef _M_ARM64
        NtFlushInstructionCache(
            NtCurrentProcess(),
            sectionHeaderAddress,
            sectionHeaderSize
            );
#endif
        if (!NT_SUCCESS(status))
            break;
    }

    return status;
}

PPH_STRING PhGetExportNameFromOrdinal(
    _In_ PVOID DllBase,
    _In_opt_ USHORT ProcedureNumber
    )
{
    PIMAGE_NT_HEADERS imageNtHeader;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_EXPORT_DIRECTORY exportDirectory;
    PULONG exportAddressTable;
    PULONG exportNameTable;
    PUSHORT exportOrdinalTable;
    ULONG i;

    if (!NT_SUCCESS(PhGetLoaderEntryImageNtHeaders(DllBase, &imageNtHeader)))
        return NULL;

    if (!NT_SUCCESS(PhGetLoaderEntryImageDirectory(
        DllBase,
        imageNtHeader,
        IMAGE_DIRECTORY_ENTRY_EXPORT,
        &dataDirectory,
        &exportDirectory,
        NULL
        )))
        return NULL;

    exportAddressTable = PTR_ADD_OFFSET(DllBase, exportDirectory->AddressOfFunctions);
    exportNameTable = PTR_ADD_OFFSET(DllBase, exportDirectory->AddressOfNames);
    exportOrdinalTable = PTR_ADD_OFFSET(DllBase, exportDirectory->AddressOfNameOrdinals);

    for (i = 0; i < exportDirectory->NumberOfNames; i++)
    {
        if ((exportOrdinalTable[i] + exportDirectory->Base) == ProcedureNumber)
        {
            PVOID baseAddress = PTR_ADD_OFFSET(DllBase, exportAddressTable[exportOrdinalTable[i]]);

            if (
                ((ULONG_PTR)baseAddress >= (ULONG_PTR)exportDirectory) &&
                ((ULONG_PTR)baseAddress < (ULONG_PTR)PTR_ADD_OFFSET(exportDirectory, dataDirectory->Size))
                )
            {
                // This is a forwarder RVA.
                return PhZeroExtendToUtf16((PSTR)baseAddress);
            }
            else
            {
                return PhZeroExtendToUtf16(PTR_ADD_OFFSET(DllBase, exportNameTable[i]));
            }
        }
    }

    return NULL;
}

NTSTATUS PhLoaderEntryLoadDll(
    _In_ PPH_STRINGREF FileName,
    _Out_ PVOID* BaseAddress
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    HANDLE sectionHandle;
    PVOID imageBaseAddress;
    SIZE_T imageBaseOffset;

    status = PhCreateFile(
        &fileHandle,
        FileName,
        FILE_READ_DATA | FILE_EXECUTE | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtCreateSection(
        &sectionHandle,
        SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_EXECUTE,
        NULL,
        NULL,
        PAGE_EXECUTE,
        SEC_IMAGE,
        fileHandle
        );

    NtClose(fileHandle);

    if (!NT_SUCCESS(status))
        return status;

    status = PhpLoaderEntryQuerySectionInformation(
        sectionHandle
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(sectionHandle);
        return status;
    }

    imageBaseAddress = NULL;
    imageBaseOffset = 0;

    status = NtMapViewOfSection(
        sectionHandle,
        NtCurrentProcess(),
        &imageBaseAddress,
        0,
        0,
        NULL,
        &imageBaseOffset,
        ViewUnmap,
        0,
        PAGE_EXECUTE
        );

    NtClose(sectionHandle);

    if (status == STATUS_IMAGE_NOT_AT_BASE)
    {
        status = PhLoaderEntryRelocateImage(
            imageBaseAddress
            );
    }

    if (NT_SUCCESS(status))
    {
        if (BaseAddress)
        {
            *BaseAddress = imageBaseAddress;
        }
    }
    else
    {
        NtUnmapViewOfSection(
            NtCurrentProcess(),
            imageBaseAddress
            );
    }

    return status;
}

NTSTATUS PhLoaderEntryUnloadDll(
    _In_ PVOID BaseAddress
    )
{
    NTSTATUS status;
    PIMAGE_NT_HEADERS imageNtHeaders;
    PLDR_INIT_ROUTINE imageEntryRoutine;

    status = PhGetLoaderEntryImageNtHeaders(
        BaseAddress,
        &imageNtHeaders
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetLoaderEntryImageEntryPoint(
        BaseAddress,
        imageNtHeaders,
        &imageEntryRoutine
        );

    if (!NT_SUCCESS(status))
        return status;

    if (!imageEntryRoutine(BaseAddress, DLL_PROCESS_DETACH, NULL))
        status = STATUS_DLL_INIT_FAILED;

    NtUnmapViewOfSection(NtCurrentProcess(), BaseAddress);

    return status;
}

NTSTATUS PhLoaderEntryLoadAllImportsForDll(
    _In_ PVOID BaseAddress,
    _In_ PSTR ImportDllName
    )
{
    NTSTATUS status;
    PIMAGE_NT_HEADERS imageNtHeaders;

    status = PhGetLoaderEntryImageNtHeaders(
        BaseAddress,
        &imageNtHeaders
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhpFixupLoaderEntryImageDelayImports(
        BaseAddress,
        imageNtHeaders,
        ImportDllName
        );

    return status;
}

NTSTATUS PhLoadAllImportsForDll(
    _In_ PWSTR TargetDllName,
    _In_ PSTR ImportDllName
    )
{
    PVOID imageBaseAddress;

    if (!(imageBaseAddress = PhGetLoaderEntryDllBaseZ(TargetDllName)))
        return STATUS_INVALID_PARAMETER;

    return PhLoaderEntryLoadAllImportsForDll(
        imageBaseAddress,
        ImportDllName
        );
}

NTSTATUS PhLoadPluginImage(
    _In_ PPH_STRINGREF FileName,
    _Out_opt_ PVOID *BaseAddress
    )
{
    NTSTATUS status;
    PVOID imageBaseAddress;
    PIMAGE_NT_HEADERS imageNtHeaders;
    PLDR_INIT_ROUTINE imageEntryRoutine;

#if (PH_NATIVE_PLUGIN_IMAGE_LOAD)
    UNICODE_STRING imageFileName;
    ULONG imageType;

    imageType = IMAGE_FILE_EXECUTABLE_IMAGE;
    PhStringRefToUnicodeString(FileName, &imageFileName);

    status = LdrLoadDll(
        NULL,
        &imageType,
        &imageFileName,
        &imageBaseAddress
        );
#else
    status = PhLoaderEntryLoadDll(
        FileName,
        &imageBaseAddress
        );
#endif

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetLoaderEntryImageNtHeaders(
        imageBaseAddress,
        &imageNtHeaders
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhpFixupLoaderEntryImageImports(
        imageBaseAddress,
        imageNtHeaders
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    //status = PhLoaderEntryLoadAllImportsForDll(imageBaseAddress, "SystemInformer.exe");
    status = PhpFixupLoaderEntryImageDelayImports(
        imageBaseAddress,
        imageNtHeaders,
        "SystemInformer.exe"
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetLoaderEntryImageEntryPoint(
        imageBaseAddress,
        imageNtHeaders,
        &imageEntryRoutine
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (!imageEntryRoutine(imageBaseAddress, DLL_PROCESS_ATTACH, NULL))
        status = STATUS_DLL_INIT_FAILED;

CleanupExit:

    if (NT_SUCCESS(status))
    {
        if (BaseAddress)
        {
            *BaseAddress = imageBaseAddress;
        }
    }
    else
    {
#if (PH_NATIVE_PLUGIN_IMAGE_LOAD)
        LdrUnloadDll(imageBaseAddress);
#else
        PhLoaderEntryUnloadDll(imageBaseAddress);
#endif
    }

    return status;
}

// based on GetBinaryTypeW (dmex)
NTSTATUS PhGetFileBinaryTypeWin32(
    _In_ PWSTR FileName,
    _Out_ PULONG BinaryType
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    HANDLE sectionHandle;
    UNICODE_STRING fileName;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    SECTION_IMAGE_INFORMATION section = { 0 };

    status = PhDosLongPathNameToNtPathNameWithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
        &fileHandle,
        FILE_READ_DATA | FILE_EXECUTE | SYNCHRONIZE,
        &objectAttributes,
        &ioStatusBlock,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtCreateSection(
        &sectionHandle,
        SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_EXECUTE,
        NULL,
        NULL,
        PAGE_EXECUTE,
        SEC_IMAGE,
        fileHandle
        );

    NtClose(fileHandle);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtQuerySection(
        sectionHandle,
        SectionImageInformation,
        &section,
        sizeof(SECTION_IMAGE_INFORMATION),
        NULL
        );

    NtClose(sectionHandle);

CleanupExit:
    RtlFreeUnicodeString(&fileName);

    if (NT_SUCCESS(status))
    {
        switch (section.Machine)
        {
        case IMAGE_FILE_MACHINE_I386:
            {
                *BinaryType = SCS_32BIT_BINARY;
            }
            return STATUS_SUCCESS;
        case IMAGE_FILE_MACHINE_IA64:
        case IMAGE_FILE_MACHINE_AMD64:
            {
                *BinaryType = SCS_64BIT_BINARY;
            }
            return STATUS_SUCCESS;
        }

        return STATUS_UNSUCCESSFUL;
    }
    else
    {
        switch (status)
        {
        case STATUS_INVALID_IMAGE_PROTECT:
            {
                *BinaryType = SCS_DOS_BINARY;
            }
            return STATUS_SUCCESS;
        case STATUS_INVALID_IMAGE_NOT_MZ:
            {
                static PH_STRINGREF extensionCOM = PH_STRINGREF_INIT(L".COM");
                static PH_STRINGREF extensionPIF = PH_STRINGREF_INIT(L".PIF");
                static PH_STRINGREF extensionEXE = PH_STRINGREF_INIT(L".EXE");
                PH_STRINGREF fileNameSr;

                PhInitializeStringRefLongHint(&fileNameSr, FileName);

                if (
                    PhEndsWithStringRef(&fileNameSr, &extensionCOM, TRUE) ||
                    PhEndsWithStringRef(&fileNameSr, &extensionPIF, TRUE) ||
                    PhEndsWithStringRef(&fileNameSr, &extensionEXE, TRUE)
                    )
                {
                    *BinaryType = SCS_DOS_BINARY;
                    return STATUS_SUCCESS;
                }
            }
            break;
        case STATUS_INVALID_IMAGE_WIN_32:
            {
                *BinaryType = SCS_32BIT_BINARY;
            }
            return STATUS_SUCCESS;
        case STATUS_INVALID_IMAGE_WIN_64:
            {
                *BinaryType = SCS_64BIT_BINARY;
            }
            return STATUS_SUCCESS;
        }
    }

    return status;
}
