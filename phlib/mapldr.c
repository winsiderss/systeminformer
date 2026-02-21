/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2024
 *
 */

#include <ph.h>
#include <apiimport.h>
#include <mapimg.h>
#include <mapldr.h>

/**
 * Locates a loader entry in the current process.
 *
 * \param[in,opt] DllBase The base address of the DLL. Specify NULL if this is not a search criteria.
 * \param[in,opt] FullDllName The full name of the DLL. Specify NULL if this is not a search criteria.
 * \param[in,opt] BaseDllName The base name of the DLL. Specify NULL if this is not a search criteria.
 * \remarks This function must be called with the loader lock acquired. The first entry matching all
 * of the specified values is returned.
 */
PLDR_DATA_TABLE_ENTRY PhFindLoaderEntry(
    _In_opt_ PVOID DllBase,
    _In_opt_ PCPH_STRINGREF FullDllName,
    _In_opt_ PCPH_STRINGREF BaseDllName
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

/**
 * Locates a loader entry by address in the current process.
 *
 * \param[in] Address An address within the module's address range.
 * \return A pointer to the loader entry, or NULL if not found.
 */
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

/**
 * Locates a loader entry by base name hash in the current process.
 *
 * \param[in] BaseNameHash The hash value of the DLL base name.
 * \return A pointer to the loader entry, or NULL if not found.
 */
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

/**
 * Loads a DLL into the current process with safe search path behavior.
 *
 * \param[in] FileName The file name of the DLL to load.
 * \return The base address of the loaded DLL, or NULL if loading failed.
 * \remarks This function attempts to load the DLL with safe search paths, prioritizing
 * System32 and the application directory. On Windows 7 without KB2533623, it falls back
 * to the default LoadLibraryEx behavior for compatibility.
 */
PVOID PhLoadLibrary(
    _In_ PCWSTR FileName
    )
{
    PVOID baseAddress;

    // Force System32 as the default search path.
    // - If the file name is not qualified this will fail.
    // - If the file name qualified this will succeed.
    // Note: Directories in the standard search path are not searched.

    if (baseAddress = LoadLibraryEx(FileName, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32))
        return baseAddress;

    // Force the current executable directory as the default search path.
    // - If the file name is not qualified this will fail.
    // - If the file name qualified this will succeed.
    // Note: Directories in the standard search path are not searched.

    if (baseAddress = LoadLibraryEx(FileName, NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR))
        return baseAddress;

    // Windows 7, Windows 8, Windows Server 2008 R2, and Windows Server 2008
    // don't support the above flags prior to installing KB2533623. (dmex)

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

/**
 * Loads a library with specific flags.
 *
 * \param[out] BaseAddress Receives the base address of the loaded library.
 * \param[in] FileName The file name of the library to load.
 * \param[in] Flags Flags to control the loading behavior.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhLoadLibraryEx(
    _Out_ PVOID* BaseAddress,
    _In_ PCWSTR FileName,
    _In_ ULONG Flags
    )
{
    PVOID baseAddress;

    if (baseAddress = LoadLibraryEx(FileName, NULL, Flags))
    {
        *BaseAddress = baseAddress;
        return STATUS_SUCCESS;
    }

    *BaseAddress = NULL;
    return PhGetLastWin32ErrorAsNtStatus();
}

/**
 * Frees a loaded library.
 *
 * \param[in] BaseAddress The base address of the library to free.
 * \return TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhFreeLibrary(
    _In_ PVOID BaseAddress
    )
{
    return !!FreeLibrary(BaseAddress);
}

#ifdef DEBUG
// rev from BasepLoadLibraryAsDataFileInternal (dmex)
/**
 * Temporarily suppresses debugger symbol loading notifications for resource loading.
 *
 * \param [in] SuppressDebugMessage TRUE to suppress debug messages, FALSE to restore them.
 *
 * \remarks This function is based on BasepLoadLibraryAsDataFileInternal from kernelbase.dll.
 * On Windows 7/8, the Visual Studio debugger attempts to load symbols for non-executable resources,
 * causing unnecessary overhead. This function temporarily sets the TEB's SuppressDebugMsg flag to
 * prevent symbol loading during resource-only image mapping. On Windows 10+, SEC_IMAGE_NO_EXECUTE
 * prevents symbol loading, making this suppression unnecessary. Only enabled in DEBUG builds.
 */
FORCEINLINE
VOID
NtSuppressDebugMessage(
    _In_ BOOLEAN SuppressDebugMessage
    )
{
    // Note: The Visual Studio debugger on Windows 7/8 attempts to load symbols
    // for non-executable resources. The kernelbase BasepLoadLibraryAsDataFileInternal
    // function temporarily suppresses symbols while loading resources (both release and debug)
    // as a QOL optimization but we only suppress debug messages for debug builds. This issue was fixed
    // by Microsoft on Windows 10 and above with SEC_IMAGE_NO_EXECUTE and suppression is not required (dmex)

    if (WindowsVersion < WINDOWS_10)
    {
        NtCurrentTeb()->SuppressDebugMsg = SuppressDebugMessage;
    }
}
#else
#define NtSuppressDebugMessage(SuppressDebugMessage)
#endif

/**
 * Loads a DLL as a read-only resource from a file handle.
 *
 * \param[in] FileHandle A handle to the file containing the DLL image.
 * \param[out,opt] BaseAddress An optional pointer that receives the base address with the image mapping flag.
 * \return NTSTATUS Successful or errant status.
 * \remarks This function creates a read-only SEC_IMAGE section and maps it into the current
 * process. On Windows 8+, SEC_IMAGE_NO_EXECUTE is used. The returned address has the
 * LDR_IS_IMAGEMAPPING flag set (bitwise OR 2). Debug messages are suppressed during mapping
 * on Windows versions prior to Windows 10.
 */
NTSTATUS PhLoadLibraryAsResource(
    _In_ HANDLE FileHandle,
    _Out_opt_ PVOID *BaseAddress
    )
{
    NTSTATUS status;
    HANDLE sectionHandle;
    PVOID imageBaseAddress;
    SIZE_T imageBaseSize;

    status = PhCreateSection(
        &sectionHandle,
        SECTION_QUERY | SECTION_MAP_READ,
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

    status = PhMapViewOfSection(
        sectionHandle,
        NtCurrentProcess(),
        &imageBaseAddress,
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

/**
 * Loads a DLL as a read-only image resource from a file path.
 *
 * \param[in] FileName The file name of the DLL as a string reference.
 * \param[in] NativeFileName TRUE if FileName is in NT native format, FALSE for Win32 format.
 * \param[out,opt] BaseAddress An optional pointer that receives the base address with the image mapping flag.
 * \return NTSTATUS Successful or errant status.
 * \remarks This function opens the file and calls PhLoadLibraryAsResource(). The file is
 * opened with read-only access and share permissions.
 */
NTSTATUS PhLoadLibraryAsImageResource(
    _In_ PCPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName,
    _Out_opt_ PVOID *BaseAddress
    )
{
    NTSTATUS status;
    HANDLE fileHandle;

    if (NativeFileName)
    {
        status = PhCreateFile(
            &fileHandle,
            FileName,
            FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );
    }
    else
    {
        status = PhCreateFileWin32(
            &fileHandle,
            FileName->Buffer,
            FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );
    }

    if (!NT_SUCCESS(status))
        return status;

    status = PhLoadLibraryAsResource(fileHandle, BaseAddress);
    NtClose(fileHandle);

    return status;
}

/**
 * Releases a DLL loaded as an image resource.
 *
 * \param[in] BaseAddress The base address returned by PhLoadLibraryAsImageResource(), with the image mapping flag.
 * \return NTSTATUS Successful or errant status.
 * \remarks This function unmaps the view created by PhLoadLibraryAsResource(). The
 * LDR_IS_IMAGEMAPPING flag is removed before unmapping. Debug messages are suppressed
 * during unmapping on Windows versions prior to Windows 10.
 */
NTSTATUS PhFreeLibraryAsImageResource(
    _In_ PVOID BaseAddress
    )
{
    NTSTATUS status;

    NtSuppressDebugMessage(TRUE);

    status = PhUnmapViewOfSection(
        NtCurrentProcess(),
        LDR_IMAGEMAPPING_TO_MAPPEDVIEW(BaseAddress)
        );

    NtSuppressDebugMessage(FALSE);

    return status;
}

/**
 * Retrieves the base address of a loaded DLL by name.
 *
 * \param[in] DllName The base name of the DLL (e.g., "kernel32.dll").
 * \return The base address of the DLL, or NULL if not found.
 * \remarks On builds with PHNT_NATIVE_LDR, this uses LdrGetDllHandle(). Otherwise, it
 * searches the loader data table manually.
 */
PVOID PhGetDllHandle(
    _In_ PCWSTR DllName
    )
{
#if defined(PHNT_NATIVE_LDR)
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

/**
 * Retrieves the address of an exported function from a loaded module by name.
 *
 * \param[in] ModuleName The name of the module (e.g., "ntdll.dll").
 * \param[in,opt] ProcedureName The name of the exported procedure, or an ordinal value cast to PCSTR.
 * \return The address of the procedure, or NULL if not found.
 * \remarks This function first locates the module, then retrieves the procedure address.
 * Ordinal values should be passed using the IS_INTRESOURCE macro.
 */
PVOID PhGetModuleProcAddress(
    _In_ PCWSTR ModuleName,
    _In_opt_ PCSTR ProcedureName
    )
{
#if defined(PHNT_NATIVE_LDR)
    PVOID module;

    module = PhGetDllHandle(ModuleName);

    if (module)
        return PhGetProcedureAddress(module, ProcedureName, 0);
    else
        return NULL;
#else
    PH_STRINGREF baseDllName;

    PhInitializeStringRefLongHint(&baseDllName, ModuleName);

    if (IS_INTRESOURCE(ProcedureName))
        return PhGetDllProcedureAddress(&baseDllName, NULL, PtrToUshort(ProcedureName));

    return PhGetDllProcedureAddress(&baseDllName, ProcedureName, 0);
#endif
}

/**
 * Retrieves the address of an exported function from a loaded DLL.
 *
 * \param[in] DllHandle The base address of the DLL.
 * \param[in,opt] ProcedureName The name of the exported procedure. Specify NULL to use ProcedureNumber.
 * \param[in,opt] ProcedureNumber The ordinal of the exported procedure. Specify 0 to use ProcedureName.
 * \return The address of the procedure, or NULL if not found.
 * \remarks On builds with PHNT_NATIVE_LDR, this uses LdrGetProcedureAddress(). Otherwise,
 * it calls PhGetDllBaseProcedureAddress() which handles export suppression and forwarding.
 */
PVOID PhGetProcedureAddress(
    _In_ PVOID DllHandle,
    _In_opt_ PCSTR ProcedureName,
    _In_opt_ USHORT ProcedureNumber
    )
{
#if defined(PHNT_NATIVE_LDR)
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
    return PhGetDllBaseProcedureAddress(DllHandle, ProcedureName, ProcedureNumber);
#endif
}

typedef struct _PH_PROCEDURE_ADDRESS_REMOTE_CONTEXT
{
    PVOID DllBase;
    PPH_STRING FileName;
} PH_PROCEDURE_ADDRESS_REMOTE_CONTEXT, *PPH_PROCEDURE_ADDRESS_REMOTE_CONTEXT;

/**
 * Callback function for limited module enumeration during remote procedure address lookup.
 *
 * \param [in] ProcessHandle A handle to the target process.
 * \param [in] VirtualAddress The virtual address of the module.
 * \param [in] ImageBase The base address of the module.
 * \param [in] ImageSize The size of the module image in bytes.
 * \param [in] FileName The file name of the module.
 * \param [in] Context A pointer to PH_PROCEDURE_ADDRESS_REMOTE_CONTEXT containing search criteria.
 * \return STATUS_SUCCESS to continue enumeration, or STATUS_NO_MORE_ENTRIES to stop when a match is found.
 * \remarks This callback is used with limited process module enumeration (PROCESS_QUERY_LIMITED_INFORMATION)
 * to locate a DLL by name in a remote process. When the file name matches the search criteria, it stores
 * the base address in the context and returns STATUS_NO_MORE_ENTRIES to terminate enumeration.
 */
_Function_class_(PH_ENUM_PROCESS_MODULES_LIMITED_CALLBACK)
static NTSTATUS NTAPI PhpGetProcedureAddressRemoteLimitedCallback(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress,
    _In_ PVOID ImageBase,
    _In_ SIZE_T ImageSize,
    _In_ PPH_STRING FileName,
    _In_ PPH_PROCEDURE_ADDRESS_REMOTE_CONTEXT Context
    )
{
    if (PhEqualString(FileName, Context->FileName, TRUE))
    {
        Context->DllBase = ImageBase;
        return STATUS_NO_MORE_ENTRIES;
    }

    return STATUS_SUCCESS;
}

/**
 * Callback function for full module enumeration during remote procedure address lookup.
 *
 * \param [in] Module A pointer to the loader data table entry for the module.
 * \param [in] Context A pointer to PH_PROCEDURE_ADDRESS_REMOTE_CONTEXT containing search criteria.
 * \return TRUE to continue enumeration, FALSE to stop when a match is found.
 * \remarks This callback is used with full process module enumeration (requiring PROCESS_VM_READ)
 * to locate a DLL by its full path in a remote process. When the full DLL name matches the search
 * criteria, it stores the base address in the context and returns FALSE to terminate enumeration.
 */
_Function_class_(PH_ENUM_PROCESS_MODULES_CALLBACK)
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
 * \param[in] ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param[in] FileName The file name of the DLL containing the procedure.
 * \param[in] ProcedureName The name or ordinal of the procedure.
 * \param[out] ProcedureAddress A variable which receives the address of the procedure in the address
 * space of the process.
 * \param[out,opt] DllBase A variable which receives the base address of the DLL containing the procedure.
 */
NTSTATUS PhGetProcedureAddressRemote(
    _In_ HANDLE ProcessHandle,
    _In_ PCPH_STRINGREF FileName,
    _In_ PCSTR ProcedureName,
    _Out_ PVOID *ProcedureAddress,
    _Out_opt_ PVOID *DllBase
    )
{
    NTSTATUS status;
    PPH_STRING fileName = NULL;
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

    status = PhEnumProcessModulesLimited(
        ProcessHandle,
        PhpGetProcedureAddressRemoteLimitedCallback,
        &context
        );

    if (!context.DllBase)
    {
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
        default:
            status = STATUS_NOT_SUPPORTED;
            break;
        }

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    if (!context.DllBase)
    {
        status = STATUS_DLL_NOT_FOUND;
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
            exportsFlags |= PH_GET_IMAGE_EXPORTS_ARM64X;
    }

    status = PhGetMappedImageExportsEx(&exports, &mappedImage, exportsFlags);
#else
    status = PhGetMappedImageExports(&exports, &mappedImage);
#endif

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (IS_INTRESOURCE(ProcedureName))
    {
        status = PhGetMappedImageExportFunctionRemote(
            &exports,
            NULL,
            PtrToUshort(ProcedureName),
            context.DllBase,
            ProcedureAddress
            );
    }
    else
    {
        status = PhGetMappedImageExportFunctionRemote(
            &exports,
            ProcedureName,
            0,
            context.DllBase,
            ProcedureAddress
            );
    }

    if (NT_SUCCESS(status))
    {
        if (DllBase)
            *DllBase = context.DllBase;
    }

CleanupExit:
    PhUnloadMappedImage(&mappedImage);
    PhClearReference(&fileName);

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
 * \param[in] DllBase The base address of the image containing the resource.
 * \param[in] Name The name of the resource or the integer identifier.
 * \param[in] Type The type of resource or the integer identifier.
 * \param[out,opt] ResourceLength A variable which will receive the length of the resource block.
 * \param[out,opt] ResourceBuffer A variable which receives the address of the resource data.
 * \return TRUE if the resource was found, FALSE if an error occurred.
 * \remarks Use this function instead of LoadResource() because no memory allocation is required.
 * This function returns the address of the resource from the read-only section of the image
 * and does not need to be allocated or deallocated. This function cannot be used when the
 * image will be unloaded since the validity of the address is tied to the lifetime of the image.
 */
NTSTATUS PhLoadResource(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _Out_opt_ ULONG *ResourceLength,
    _Out_opt_ PVOID *ResourceBuffer
    )
{
    NTSTATUS status;
    PIMAGE_RESOURCE_DATA_ENTRY resourceData = NULL;
    PVOID resourceBuffer = NULL;
    ULONG resourceLength;
    ULONG_PTR resourcePath[] = {
        (ULONG_PTR)Type,
        (ULONG_PTR)Name,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)
    };

    __try
    {
        status = LdrFindResource_U(DllBase, resourcePath, RTL_NUMBER_OF(resourcePath), &resourceData);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    if (!NT_SUCCESS(status))
        return status;

    __try
    {
        status = LdrAccessResource(DllBase, resourceData, &resourceBuffer, &resourceLength);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    if (!NT_SUCCESS(status))
        return status;

    if (ResourceLength)
        *ResourceLength = resourceLength;
    if (ResourceBuffer)
        *ResourceBuffer = resourceBuffer;

    return status;
}

 /**
  * Finds and returns a copy of the resource.
  *
  * \param[in] DllBase The base address of the image containing the resource.
  * \param[in] Name The name of the resource or the integer identifier.
  * \param[in] Type The type of resource or the integer identifier.
  * \param[out,opt] ResourceLength A variable which will receive the length of the resource block.
  * \param[out,opt] ResourceBuffer A variable which receives the address of the resource data.
  * \return TRUE if the resource was found, FALSE if an error occurred.
  * \remarks This function returns a copy of a resource from heap memory
  * and must be deallocated. Use this function when the image will be unloaded.
  */
NTSTATUS PhLoadResourceCopy(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _Out_opt_ ULONG *ResourceLength,
    _Out_opt_ PVOID *ResourceBuffer
    )
{
    NTSTATUS status;
    ULONG resourceLength;
    PVOID resourceBuffer;

    status = PhLoadResource(
        DllBase,
        Name,
        Type,
        &resourceLength,
        &resourceBuffer
        );

    if (NT_SUCCESS(status))
    {
        if (ResourceLength)
            *ResourceLength = resourceLength;
        if (ResourceBuffer)
            *ResourceBuffer = PhAllocateCopy(resourceBuffer, resourceLength);
    }

    return status;
}

// rev from LoadString (dmex)
/**
 * Loads a string resource from the image into a buffer.
 *
 * \param[in] DllBase The base address of the image containing the resource.
 * \param[in] ResourceId The identifier of the string to be loaded.
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

    if (!NT_SUCCESS(PhLoadResource(
        DllBase,
        MAKEINTRESOURCE(resourceId),
        RT_STRING,
        &resourceLength,
        &resourceBuffer
        )))
    {
        return NULL;
    }

    if (!resourceBuffer)
        return NULL;

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
            stringBuffer->Length * sizeof(WCHAR) - sizeof(UNICODE_NULL)
            );
    }

    return string;
}

// rev from SHLoadIndirectString (dmex)
/**
 * Extracts a specified text resource when given that resource in the form of an indirect string (a string that begins with the '@' symbol).
 * \param[in] SourceString The indirect string from which the resource will be retrieved.
 */
PPH_STRING PhLoadIndirectString(
    _In_ PCPH_STRINGREF SourceString
    )
{
    PPH_STRING indirectString = NULL;

    if (!SourceString || !SourceString->Buffer)
        return NULL;

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

        if (NT_SUCCESS(PhLoadLibraryAsImageResource(&libraryString->sr, FALSE, &libraryModule)))
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
 * \param[in] DllBase The base address of the DLL.
 * \param[out,opt] IndexOfFileName A variable which receives the index of the base name of the DLL in the
 * returned string.
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

    PhAcquireLoaderLock();

    entry = PhFindLoaderEntry(DllBase, NULL, NULL);

    if (entry)
        fileName = PhCreateStringFromUnicodeString(&entry->FullDllName);
    else
        fileName = NULL;

    PhReleaseLoaderLock();

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

/**
 * Retrieves information about a loaded DLL by its base name.
 *
 * \param[in] BaseDllName The base name of the DLL as a string reference (e.g., "kernel32.dll").
 * \param[out,opt] DllBase An optional pointer that receives the base address of the DLL.
 * \param[out,opt] SizeOfImage An optional pointer that receives the size of the image in bytes.
 * \param[out,opt] FullName An optional pointer that receives the full path to the DLL.
 * \return TRUE if the DLL was found, FALSE otherwise.
 * \remarks This function searches the loader data table with the loader lock held.
 */
_Success_(return)
BOOLEAN PhGetLoaderEntryData(
    _In_ PCPH_STRINGREF BaseDllName,
    _Out_opt_ PVOID* DllBase,
    _Out_opt_ ULONG* SizeOfImage,
    _Out_opt_ PPH_STRING* FullName
    )
{
    BOOLEAN result = FALSE;
    PLDR_DATA_TABLE_ENTRY entry;

    PhAcquireLoaderLock();

    entry = PhFindLoaderEntry(NULL, NULL, BaseDllName);

    if (entry)
    {
        if (DllBase)
        {
            *DllBase = entry->DllBase;
            result = TRUE;
        }

        if (SizeOfImage)
        {
            *SizeOfImage = entry->SizeOfImage;
            result = TRUE;
        }

        if (FullName)
        {
            PH_STRINGREF fullName;
            PPH_STRING fileName;

            PhUnicodeStringToStringRef(&entry->FullDllName, &fullName);

            if (fileName = PhDosPathNameToNtPathName(&fullName))
            {
                *FullName = fileName;
                result = TRUE;
            }
            else
            {
                result = FALSE;
            }
        }
    }

    PhReleaseLoaderLock();

    return result;
}

/**
 * Retrieves the base address of a DLL containing the specified address.
 *
 * \param[in] Address An address within the DLL's address space.
 * \return The base address of the DLL, or NULL if not found.
 * \remarks This function searches the loader data table with the loader lock held.
 */
PVOID PhGetLoaderEntryAddressDllBase(
    _In_ PVOID Address
    )
{
    PLDR_DATA_TABLE_ENTRY entry;
    PVOID baseAddress;

    PhAcquireLoaderLock();

    entry = PhFindLoaderEntryAddress(Address);

    if (entry)
        baseAddress = entry->DllBase;
    else
        baseAddress = NULL;

    PhReleaseLoaderLock();

    return baseAddress;
}

/**
 * Retrieves the base address of a DLL by name.
 *
 * \param[in,opt] FullDllName The full path to the DLL. Specify NULL if not used.
 * \param[in,opt] BaseDllName The base name of the DLL (e.g., "kernel32.dll"). Specify NULL if not used.
 * \return The base address of the DLL, or NULL if not found.
 * \remarks On Windows 8 and later, when only BaseDllName is specified, this function uses
 * hash-based lookup for improved performance. Otherwise it performs a linear search.
 */
PVOID PhGetLoaderEntryDllBase(
    _In_opt_ PCPH_STRINGREF FullDllName,
    _In_opt_ PCPH_STRINGREF BaseDllName
    )
{
    PLDR_DATA_TABLE_ENTRY entry;
    PVOID baseAddress;

    PhAcquireLoaderLock();

    if (WindowsVersion >= WINDOWS_8 && !FullDllName && BaseDllName)
    {
        ULONG baseNameHash = PhHashStringRefEx(BaseDllName, TRUE, PH_STRING_HASH_X65599);

        entry = PhFindLoaderEntryNameHash(baseNameHash);
    }
    else
    {
        entry = PhFindLoaderEntry(NULL, FullDllName, BaseDllName);
    }

    if (entry)
        baseAddress = entry->DllBase;
    else
        baseAddress = NULL;

    PhReleaseLoaderLock();

    return baseAddress;
}

/**
 * Retrieves the address of an exported function from a loaded DLL.
 *
 * \param[in] DllBase The base address of the DLL.
 * \param[in,opt] ProcedureName The name of the exported procedure. Specify NULL to use ProcedureNumber.
 * \param[in,opt] ProcedureNumber The ordinal number of the exported procedure. Specify 0 to use ProcedureName.
 * \return The address of the exported function, or NULL if the function could not be found.
 * \remarks This function supports Control Flow Guard (CFG) export suppression on Windows 10 RS2 and later.
 * It also handles forwarded exports by recursively resolving them through the forwarding chain.
 */
PVOID PhGetDllBaseProcedureAddress(
    _In_ PVOID DllBase,
    _In_opt_ PCSTR ProcedureName,
    _In_opt_ USHORT ProcedureNumber
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOLEAN exportSuppressionEnabled = FALSE;
    PVOID exportAddress;
    PIMAGE_NT_HEADERS imageNtHeader;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_EXPORT_DIRECTORY exportDirectory;
    PROCESS_MITIGATION_POLICY_INFORMATION mitigation;

    if (PhBeginInitOnce(&initOnce))
    {
        if ((WindowsVersion >= WINDOWS_10_RS2) &&
            NT_SUCCESS(PhGetProcessMitigationPolicy(NtCurrentProcess(), ProcessControlFlowGuardPolicy, &mitigation)) &&
            mitigation.ControlFlowGuardPolicy.EnableExportSuppression)
        {
            exportSuppressionEnabled = TRUE;
        }

        PhEndInitOnce(&initOnce);
    }

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

    exportAddress = PhGetLoaderEntryImageExportFunction(
        DllBase,
        imageNtHeader,
        dataDirectory,
        exportDirectory,
        ProcedureName,
        ProcedureNumber
        );

    if (exportAddress && exportSuppressionEnabled)
    {
        if (PhLoaderEntryImageExportSupressionPresent(DllBase, imageNtHeader))
        {
            PhLoaderEntryGrantSuppressedCall(exportAddress);
        }
    }

    return exportAddress;
}

/**
 * Retrieves the address of an exported function from a loaded DLL.
 *
 * \param[in] BaseAddress The base address of the DLL.
 * \param[in,opt] ProcedureName The name of the exported procedure, or an ordinal value cast to a pointer.
 * \return The address of the exported function, or NULL if the function could not be found.
 * \remarks This is a convenience wrapper around PhGetDllBaseProcedureAddress that automatically
 * determines whether ProcedureName is a name or an ordinal based on IS_INTRESOURCE.
 */
PVOID PhGetDllBaseProcAddress(
    _In_ PVOID BaseAddress,
    _In_opt_ PCSTR ProcedureName
    )
{
    if (IS_INTRESOURCE(ProcedureName))
        return PhGetDllBaseProcedureAddress(BaseAddress, NULL, PtrToUshort(ProcedureName));

    return PhGetDllBaseProcedureAddress(BaseAddress, ProcedureName, 0);
}

/**
 * Retrieves the address of an exported function from a DLL by name.
 *
 * \param[in] DllName The name of the DLL.
 * \param[in,opt] ProcedureName The name of the exported procedure. Specify NULL to use ProcedureNumber.
 * \param[in,opt] ProcedureNumber The ordinal number of the exported procedure. Specify 0 to use ProcedureName.
 * \return The address of the exported function, or NULL if the DLL or function could not be found.
 * \remarks This function first locates the DLL in the loader data, then resolves the export address.
 */
PVOID PhGetDllProcedureAddress(
    _In_ PCPH_STRINGREF DllName,
    _In_opt_ PCSTR ProcedureName,
    _In_opt_ USHORT ProcedureNumber
    )
{
    PVOID baseAddress;

    if (!(baseAddress = PhGetLoaderEntryDllBase(NULL, DllName)))
        return NULL;

    return PhGetDllBaseProcedureAddress(
        baseAddress,
        ProcedureName,
        ProcedureNumber
        );
}

/**
 * Retrieves the NT headers of a loaded image.
 *
 * \param[in] BaseAddress The base address of the image.
 * \param[out] ImageNtHeaders A variable which receives a pointer to the NT headers.
 * \return An NTSTATUS code indicating success or failure.
 * \retval STATUS_INVALID_IMAGE_NOT_MZ The DOS signature is invalid.
 * \retval STATUS_INVALID_IMAGE_FORMAT The NT headers offset or signature is invalid.
 * \remarks This function validates the DOS and NT signatures before returning the NT headers pointer.
 */
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

    if (imageNtHeadersOffset == 0 || imageNtHeadersOffset >= RTL_IMAGE_MAX_DOS_HEADER)
        return STATUS_INVALID_IMAGE_FORMAT;

    imageNtHeaders = PTR_ADD_OFFSET(BaseAddress, imageNtHeadersOffset);

    if (imageNtHeaders->Signature != IMAGE_NT_SIGNATURE)
        return STATUS_INVALID_IMAGE_FORMAT;

    *ImageNtHeaders = imageNtHeaders;
    return STATUS_SUCCESS;
}

/**
 * Retrieves the entry point address of an image.
 *
 * \param[in] BaseAddress The base address of the image.
 * \param[in] ImageNtHeader The NT headers of the image.
 * \param[out] ImageEntryPoint A variable which receives the entry point address.
 * \return An NTSTATUS code indicating success or failure.
 * \retval STATUS_ENTRYPOINT_NOT_FOUND The image has no entry point.
 * \remarks Some images, such as resource-only DLLs, may not have an entry point.
 */
NTSTATUS PhGetLoaderEntryImageEntryPoint(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_NT_HEADERS ImageNtHeader,
    _Out_ PDLL_INIT_ROUTINE *ImageEntryPoint
    )
{
    if (ImageNtHeader->OptionalHeader.AddressOfEntryPoint == 0)
        return STATUS_ENTRYPOINT_NOT_FOUND;

    *ImageEntryPoint = PTR_ADD_OFFSET(BaseAddress, ImageNtHeader->OptionalHeader.AddressOfEntryPoint);
    return STATUS_SUCCESS;
}

/**
 * Retrieves a data directory entry from an image.
 *
 * \param[in] BaseAddress The base address of the image.
 * \param[in] ImageNtHeader The NT headers of the image.
 * \param[in] ImageDirectoryIndex The index of the data directory (e.g., IMAGE_DIRECTORY_ENTRY_EXPORT).
 * \param[out] ImageDataDirectoryEntry A variable which receives a pointer to the data directory entry.
 * \param[out] ImageDirectoryEntry A variable which receives the virtual address of the directory data.
 * \param[out,opt] ImageDirectoryLength A variable which optionally receives the size of the directory.
 * \return An NTSTATUS code indicating success or failure.
 * \retval STATUS_INVALID_FILE_FOR_SECTION The directory index is invalid or the directory is empty.
 */
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

/**
 * Locates the section containing a given virtual address.
 *
 * \param[in] BaseAddress The base address of the image.
 * \param[in] ImageNtHeader The NT headers of the image.
 * \param[in] ImageDirectoryAddress The virtual address to locate.
 * \param[out] ImageSectionAddress A pointer that receives the base address of the section.
 * \param[out] ImageSectionLength A pointer that receives the size of the section.
 * \return NTSTATUS Successful or errant status.
 * \retval STATUS_SECTION_NOT_IMAGE The address is not within any section.
 * \remarks This function iterates through all sections to find which one contains the
 * specified address. Section size is calculated as the maximum of VirtualSize and SizeOfRawData.
 */
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
    PVOID imageSectionStart;
    ULONG imageSectionSize;
    PVOID imageSectionEnd;

    section = IMAGE_FIRST_SECTION(ImageNtHeader);

    for (USHORT i = 0; i < ImageNtHeader->FileHeader.NumberOfSections; i++)
    {
        sectionHeader = PTR_ADD_OFFSET(section, UInt32x32To64(IMAGE_SIZEOF_SECTION_HEADER, i));

        // Note: VirtualSize is used by the loader, SizeOfRawData is used for file on disk.
        // A .bss section in a PE file might have SizeOfRawData = 0 (since it is not stored in the file) 
        // and VirtualSize = 4096 (the amount of memory to allocate and zero-fill).
        // The section length must be the maximum of the two values.

        imageSectionStart = PTR_ADD_OFFSET(BaseAddress, sectionHeader->VirtualAddress);
        imageSectionSize = max(sectionHeader->Misc.VirtualSize, sectionHeader->SizeOfRawData);
        imageSectionEnd = PTR_ADD_OFFSET(imageSectionStart, imageSectionSize);

        if (
            ((ULONG_PTR)ImageDirectoryAddress >= (ULONG_PTR)imageSectionStart) &&
            ((ULONG_PTR)ImageDirectoryAddress < (ULONG_PTR)imageSectionEnd)
            )
        {
            directorySectionLength = imageSectionSize;
            directorySectionAddress = imageSectionStart;
            break;
        }
    }

    if (directorySectionAddress && directorySectionLength)
    {
        *ImageSectionAddress = directorySectionAddress;
        *ImageSectionLength = directorySectionLength;
        return STATUS_SUCCESS;
    }

    *ImageSectionAddress = NULL;
    *ImageSectionLength = 0;
    return STATUS_SECTION_NOT_IMAGE;
}

/**
 * Converts an RVA to a file offset within an image.
 *
 * \param[in] ImageNtHeader The NT headers of the image.
 * \param[in] Rva The relative virtual address to convert.
 * \param[out] Offset A pointer that receives the file offset.
 * \return NTSTATUS Successful or errant status.
 * \retval STATUS_NOT_FOUND The RVA is not within any section's raw data range.
 * \remarks This function calculates the file offset by finding the section containing
 * the RVA and adjusting for the section's PointerToRawData.
 */
NTSTATUS PhLoaderEntryImageRvaToFileOffset(
    _In_ PIMAGE_NT_HEADERS ImageNtHeader,
    _In_ ULONG Rva,
    _Out_ PULONG Offset
    )
{
    PIMAGE_SECTION_HEADER section;
    PIMAGE_SECTION_HEADER sectionHeader;

    section = IMAGE_FIRST_SECTION(ImageNtHeader);

    for (USHORT i = 0; i < ImageNtHeader->FileHeader.NumberOfSections; i++)
    {
        sectionHeader = PTR_ADD_OFFSET(section, UInt32x32To64(IMAGE_SIZEOF_SECTION_HEADER, i));

        if (
            Rva >= sectionHeader->VirtualAddress &&
            Rva < sectionHeader->VirtualAddress + sectionHeader->SizeOfRawData
            )
        {
            *Offset = sectionHeader->PointerToRawData + (Rva - sectionHeader->VirtualAddress);
            return STATUS_SUCCESS;
        }
    }

    *Offset = 0;
    return STATUS_NOT_FOUND;
}

/**
 * Locates the section header containing a given RVA.
 *
 * \param[in] ImageNtHeader The NT headers of the image.
 * \param[in] Rva The relative virtual address to locate.
 * \param[out] ImageSection A pointer that receives the section header.
 * \param[out] ImageSectionLength A pointer that receives the section size.
 * \return NTSTATUS Successful or errant status.
 * \retval STATUS_SECTION_NOT_IMAGE The RVA is not within any section.
 * \remarks Section size is calculated as the maximum of VirtualSize and SizeOfRawData
 * to handle both memory-mapped and file-mapped scenarios correctly.
 */
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
    ULONG imageSectionAddress;
    SIZE_T imageSectionLength;
    PVOID imageSectionMaximum;

    section = IMAGE_FIRST_SECTION(ImageNtHeader);

    for (USHORT i = 0; i < ImageNtHeader->FileHeader.NumberOfSections; i++)
    {
        sectionHeader = PTR_ADD_OFFSET(section, UInt32x32To64(IMAGE_SIZEOF_SECTION_HEADER, i));

        imageSectionAddress = sectionHeader->VirtualAddress;
        imageSectionLength = __max(sectionHeader->Misc.VirtualSize, sectionHeader->SizeOfRawData);
        imageSectionMaximum = PTR_ADD_OFFSET(imageSectionAddress, imageSectionLength);

        if (
            ((ULONG_PTR)Rva >= (ULONG_PTR)imageSectionAddress) &&
            ((ULONG_PTR)Rva < (ULONG_PTR)imageSectionMaximum)
            )
        {
            directorySectionLength = imageSectionLength;
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

    *ImageSection = NULL;
    *ImageSectionLength = 0;
    return STATUS_SECTION_NOT_IMAGE;
}

/**
 * Converts an RVA to a virtual address within a loaded image.
 *
 * \param[in] BaseAddress The base address of the image.
 * \param[in] Rva The relative virtual address to convert.
 * \param[out] Va A pointer that receives the virtual address.
 * \return NTSTATUS Successful or errant status.
 * \retval STATUS_SECTION_NOT_IMAGE The RVA is not within any section.
 * \remarks This function finds the section containing the RVA and calculates the
 * virtual address by adding the base address and adjusting for section alignment.
 */
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

/**
 * Determines whether an image has Control Flow Guard export suppression information.
 *
 * \param[in] BaseAddress The base address of the image.
 * \param[in] ImageNtHeader The NT headers of the image.
 * \return TRUE if export suppression information is present; otherwise, FALSE.
 * \remarks This function checks the IMAGE_GUARD_CF_EXPORT_SUPPRESSION_INFO_PRESENT flag
 * in the load configuration directory. Export suppression is a CFG feature on Windows 10 RS2
 * and later that restricts which exported functions can be called.
 */
BOOLEAN PhLoaderEntryImageExportSupressionPresent(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_NT_HEADERS ImageNtHeader
    )
{
    PIMAGE_LOAD_CONFIG_DIRECTORY configDirectory;
    PIMAGE_DATA_DIRECTORY dataDirectory;

    if (NT_SUCCESS(PhGetLoaderEntryImageDirectory(
        BaseAddress,
        ImageNtHeader,
        IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
        &dataDirectory,
        &configDirectory,
        NULL
        )))
    {
        if (RTL_CONTAINS_FIELD(configDirectory, configDirectory->Size, GuardFlags))
        {
            if (BooleanFlagOn(configDirectory->GuardFlags, IMAGE_GUARD_CF_EXPORT_SUPPRESSION_INFO_PRESENT))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

/**
 * Grants CFG-suppressed call access to an export address.
 *
 * \param[in] ExportAddress The address of the suppressed export to grant access to.
 * \remarks This function maintains a cache of export addresses that have been granted access
 * to avoid making redundant system calls. On Windows 10 RS2 and later with CFG export
 * suppression enabled, attempting to call a suppressed export will fail unless access is
 * explicitly granted via PhGuardGrantSuppressedCallAccess.
 */
VOID PhLoaderEntryGrantSuppressedCall(
    _In_ PVOID ExportAddress
    )
{
    static PPH_HASHTABLE PhLoaderEntryCacheHashtable = NULL;

    if (!PhLoaderEntryCacheHashtable)
    {
        PhLoaderEntryCacheHashtable = PhCreateSimpleHashtable(10);
    }

    if (PhLoaderEntryCacheHashtable && !PhFindItemSimpleHashtable(PhLoaderEntryCacheHashtable, ExportAddress))
    {
        if (NT_SUCCESS(PhGuardGrantSuppressedCallAccess(NtCurrentProcess(), ExportAddress)))
        {
            PhAddItemSimpleHashtable(PhLoaderEntryCacheHashtable, ExportAddress, UlongToPtr(TRUE));
        }
    }
}

/**
 * Performs a binary search to find the index of an exported function by name.
 *
 * \param [in] BaseAddress The base address of the image.
 * \param [in] ExportDirectory The export directory structure.
 * \param [in] ExportNameTable The export name table (array of RVAs to name strings).
 * \param [in] ExportName The name of the exported function to search for.
 *
 * \return The index into the export name table, or ULONG_MAX if not found.
 *
 * \remarks This function performs a binary search on the export name table, which is sorted
 * alphabetically by the Windows linker. This provides O(log n) lookup performance compared to
 * linear search. The returned index can be used with the export ordinal table to locate the
 * function address in the export address table.
 */
static ULONG PhpLookupLoaderEntryImageExportFunctionIndex(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    _In_ PULONG ExportNameTable,
    _In_ PCSTR ExportName
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

/**
 * Validates export table RVAs to prevent out-of-bounds memory access.
 *
 * \param [in] ImageNtHeader The NT headers of the image.
 * \param [in] ExportDirectory The export directory structure to validate.
 * \return TRUE if all export table RVAs are valid and within image bounds, FALSE otherwise.
 */
static BOOLEAN PhpValidateExportTableRvas(
    _In_ PIMAGE_NT_HEADERS ImageNtHeader,
    _In_ PIMAGE_EXPORT_DIRECTORY ExportDirectory
    )
{
    ULONG imageSize;
    ULONG addressTableSize;
    ULONG nameTableSize;
    ULONG ordinalTableSize;
    ULONG addressTableEnd;
    ULONG nameTableEnd;
    ULONG ordinalTableEnd;

    imageSize = ImageNtHeader->OptionalHeader.SizeOfImage;

    if (ExportDirectory->NumberOfFunctions > 0)
    {
        if (ExportDirectory->AddressOfFunctions == 0)
            return FALSE;
        if (!NT_SUCCESS(RtlULongMult(ExportDirectory->NumberOfFunctions, sizeof(ULONG), &addressTableSize)))
            return FALSE;
        if (!NT_SUCCESS(RtlULongAdd(ExportDirectory->AddressOfFunctions, addressTableSize, &addressTableEnd)))
            return FALSE;
        if (addressTableEnd > imageSize)
            return FALSE;
    }

    if (ExportDirectory->NumberOfNames > 0)
    {
        if (ExportDirectory->AddressOfNames == 0)
            return FALSE;
        if (!NT_SUCCESS(RtlULongMult(ExportDirectory->NumberOfNames, sizeof(ULONG), &nameTableSize)))
            return FALSE;
        if (!NT_SUCCESS(RtlULongAdd(ExportDirectory->AddressOfNames, nameTableSize, &nameTableEnd)))
            return FALSE;
        if (nameTableEnd > imageSize)
            return FALSE;
    }

    if (ExportDirectory->NumberOfNames > 0)
    {
        if (ExportDirectory->AddressOfNameOrdinals == 0)
            return FALSE;
        if (!NT_SUCCESS(RtlULongMult(ExportDirectory->NumberOfNames, sizeof(USHORT), &ordinalTableSize)))
            return FALSE;
        if (!NT_SUCCESS(RtlULongAdd(ExportDirectory->AddressOfNameOrdinals, ordinalTableSize, &ordinalTableEnd)))
            return FALSE;
        if (ordinalTableEnd > imageSize)
            return FALSE;
    }

    return TRUE;
}

/**
 * Retrieves the address of an exported function from an image's export directory.
 *
 * \param [in] BaseAddress The base address of the image.
 * \param [in] ImageNtHeader The NT headers of the image.
 * \param [in] DataDirectory The data directory entry for the export table.
 * \param [in] ExportDirectory The export directory.
 * \param [in,opt] ExportName The name of the exported function. Specify NULL to use ExportOrdinal.
 * \param [in,opt] ExportOrdinal The ordinal number of the exported function. Specify 0 to use ExportName.
 * \return The address of the exported function, or NULL if the function could not be found or validation failed.
 * \remarks This function validates export table bounds before accessing export data to prevent
 * malformed DLLs from causing out-of-bounds reads. It uses safe integer arithmetic to prevent
 * overflow attacks. The function also handles forwarded exports by recursively calling
 * PhGetDllBaseProcedureAddress to resolve the forwarding chain.
 */
PVOID PhGetLoaderEntryImageExportFunction(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_NT_HEADERS ImageNtHeader,
    _In_ PIMAGE_DATA_DIRECTORY DataDirectory,
    _In_ PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    _In_opt_ PCSTR ExportName,
    _In_opt_ USHORT ExportOrdinal
    )
{
    PVOID exportAddress = NULL;
    PULONG exportAddressTable;
    PULONG exportNameTable;
    PUSHORT exportOrdinalTable;

    if (!PhpValidateExportTableRvas(ImageNtHeader, ExportDirectory))
        return NULL;

    exportAddressTable = PTR_ADD_OFFSET(BaseAddress, ExportDirectory->AddressOfFunctions);
    exportNameTable = PTR_ADD_OFFSET(BaseAddress, ExportDirectory->AddressOfNames);
    exportOrdinalTable = PTR_ADD_OFFSET(BaseAddress, ExportDirectory->AddressOfNameOrdinals);

    if (ExportOrdinal)
    {
        ULONG maxOrdinal;

        if (!NT_SUCCESS(RtlULongAdd(ExportDirectory->Base, ExportDirectory->NumberOfFunctions, &maxOrdinal)))
            return NULL;

        if (ExportOrdinal > maxOrdinal)
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
        SIZE_T dllForwarderLength;
        PH_STRINGREF dllNameRef;
        PH_STRINGREF dllForwarderRef;
        PH_STRINGREF dllProcedureRef;
        WCHAR dllForwarderName[DOS_MAX_PATH_LENGTH] = L"";

        // This is a forwarder RVA.

        dllForwarderLength = PhCountBytesZ((PCSTR)exportAddress);
        PhZeroExtendToUtf16Buffer((PCSTR)exportAddress, dllForwarderLength, dllForwarderName);
        dllForwarderRef.Length = dllForwarderLength * sizeof(WCHAR);
        dllForwarderRef.Buffer = dllForwarderName;

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

                if (!NT_SUCCESS(PhConvertUtf16ToUtf8Buffer(
                    libraryFunctionName,
                    sizeof(libraryFunctionName),
                    NULL,
                    dllProcedureRef.Buffer,
                    dllProcedureRef.Length
                    )))
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

/**
 * Retrieves the address of an exported function using a hint for optimization.
 *
 * \param[in] BaseAddress The base address of the image.
 * \param[in] ProcedureName The name of the exported procedure.
 * \param[in] ProcedureHint A hint index into the export name table (typically from import descriptor).
 * \return The address of the exported function, or NULL if not found.
 * \remarks The hint is an optimization that allows the loader to check if the export at
 * the hint index matches the desired name before performing a binary search. If the hint
 * matches, the lookup is O(1). If not, the function falls back to PhGetDllBaseProcedureAddress().
 * This function also handles forwarded exports.
 */
PVOID PhGetDllBaseProcedureAddressWithHint(
    _In_ PVOID BaseAddress,
    _In_ PCSTR ProcedureName,
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
                SIZE_T dllForwarderLength;
                PH_STRINGREF dllNameRef;
                PH_STRINGREF dllForwarderRef;
                PH_STRINGREF dllProcedureRef;
                WCHAR dllForwarderName[DOS_MAX_PATH_LENGTH] = L"";

                // This is a forwarder RVA.

                dllForwarderLength = PhCountBytesZ((PCSTR)exportAddress);
                PhZeroExtendToUtf16Buffer((PCSTR)exportAddress, dllForwarderLength, dllForwarderName);
                dllForwarderRef.Length = dllForwarderLength * sizeof(WCHAR);
                dllForwarderRef.Buffer = dllForwarderName;

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

                        if (!NT_SUCCESS(PhConvertUtf16ToUtf8Buffer(
                            libraryFunctionName,
                            sizeof(libraryFunctionName),
                            NULL,
                            dllProcedureRef.Buffer,
                            dllProcedureRef.Length
                            )))
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

/**
 * Detours (replaces) an imported function in a loaded module's Import Address Table (IAT).
 *
 * \param[in] BaseAddress The base address of the module whose import to detour.
 * \param[in] ImportName The name of the DLL that exports the function (e.g., "ntdll.dll").
 * \param[in] ProcedureName The name of the imported function to detour (e.g., "NtCreateFile").
 * \param[in] FunctionAddress The address of the replacement function.
 * \param[out,opt] OriginalAddress An optional pointer that receives the original function address.
 * \return NTSTATUS Successful or errant status.
 * \retval STATUS_INVALID_PARAMETER BaseAddress, ImportName, or ProcedureName is NULL.
 * \retval STATUS_UNSUCCESSFUL The import or procedure was not found.
 * \remarks This function modifies the Import Address Table (IAT) to redirect calls to the
 * specified import to a new function address. The memory is temporarily made writable, the
 * thunk is updated, and protection is restored. On ARM64, the instruction cache is flushed.
 * This is commonly used for API hooking and function interception. The caller should save
 * the original address if they need to call the original function.
 */
NTSTATUS PhLoaderEntryDetourImportProcedure(
    _In_ PVOID BaseAddress,
    _In_ PCSTR ImportName,
    _In_ PCSTR ProcedureName,
    _In_ PVOID FunctionAddress,
    _Out_opt_ PVOID* OriginalAddress
    )
{
    NTSTATUS status;
    PIMAGE_NT_HEADERS imageNtHeaders;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_IMPORT_DESCRIPTOR importDirectory;

    if (!BaseAddress || !ImportName || !ProcedureName)
        return STATUS_INVALID_PARAMETER;

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
        PCSTR importName;
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

                if (NT_SUCCESS(status = PhProtectVirtualMemory(
                    NtCurrentProcess(),
                    importThunkAddress,
                    importThunkSize,
                    PAGE_READWRITE,
                    &importThunkProtect
                    )))
                {
                    importThunk->u1.Function = (ULONG_PTR)FunctionAddress;

                    PhProtectVirtualMemory(
                        NtCurrentProcess(),
                        importThunkAddress,
                        importThunkSize,
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

/**
 * Displays an error message when an import cannot be resolved during plugin loading.
 *
 * \param [in] BaseAddress The base address of the image that failed to load.
 * \param [in] ImportName The name of the DLL containing the unresolved import.
 * \param [in] OriginalThunk The thunk data for the unresolved import.
 * \remarks This function shows a user-friendly error dialog indicating which function
 * or ordinal could not be resolved from which module. This is typically called when
 * plugin loading fails due to missing dependencies.
 */
VOID PhLoaderEntrySnapShowErrorMessage(
    _In_ PVOID BaseAddress,
    _In_ PCSTR ImportName,
    _In_ PIMAGE_THUNK_DATA OriginalThunk
    )
{
    PPH_STRING fileName;

    if (!BaseAddress || !ImportName || !OriginalThunk)
        return;

    if (NT_SUCCESS(PhGetProcessMappedFileName(NtCurrentProcess(), BaseAddress, &fileName)))
    {
        PhMoveReference(&fileName, PhGetFileName(fileName));
        PhMoveReference(&fileName, PhGetBaseName(fileName));

        if (IMAGE_SNAP_BY_ORDINAL(OriginalThunk->u1.Ordinal))
        {
            PhShowError2(
                NULL,
                L"Unable to load plugin.",
                L"Name: %s\r\nOrdinal: %u\r\nModule: %hs",
                PhGetStringOrEmpty(fileName),
                IMAGE_ORDINAL(OriginalThunk->u1.Ordinal),
                ImportName
                );
        }
        else
        {
            PIMAGE_IMPORT_BY_NAME importByName;

            importByName = PTR_ADD_OFFSET(BaseAddress, OriginalThunk->u1.AddressOfData);

            PhShowError2(
                NULL,
                L"Unable to load plugin.",
                L"Name: %s\r\nFunction: %hs\r\nModule: %hs",
                PhGetStringOrEmpty(fileName),
                importByName->Name,
                ImportName
                );
        }

        PhDereferenceObject(fileName);
    }
}

/**
 * Resolves a single import thunk by filling it with the actual function address.
 *
 * \param [in] BaseAddress The base address of the importing image.
 * \param [in] ImportBaseAddress The base address of the DLL containing the export.
 * \param [in] OriginalThunk The original thunk data containing the import information.
 * \param [in] ImportThunk The import thunk to be filled with the resolved address.
 * \return NTSTATUS Successful or errant status.
 * \retval STATUS_INVALID_PARAMETER One of the required parameters is NULL.
 * \retval STATUS_ORDINAL_NOT_FOUND The import could not be resolved.
 *
 * \remarks This function handles both ordinal and name-based imports. For ordinal imports,
 * it uses the ordinal number directly. For name-based imports, it uses the hint value
 * for optimization before falling back to a full lookup. This is the core function for
 * binding imports during dynamic loading.
 */
NTSTATUS PhLoaderEntrySnapImportThunk(
    _In_ PVOID BaseAddress,
    _In_ PVOID ImportBaseAddress,
    _In_ PIMAGE_THUNK_DATA OriginalThunk,
    _In_ PIMAGE_THUNK_DATA ImportThunk
    )
{
    if (!BaseAddress || !ImportBaseAddress || !OriginalThunk || !ImportThunk)
        return STATUS_INVALID_PARAMETER;

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

#if defined(PH_NATIVE_LOADER_WORKQUEUE)
typedef struct _PH_LOADER_IMPORT_THUNK_WORKQUEUE_CONTEXT
{
    PVOID BaseAddress;
    PVOID ImportBaseAddress;
    PIMAGE_THUNK_DATA OriginalThunkData;
    PIMAGE_THUNK_DATA ImportThunkData;
} PH_LOADER_IMPORT_THUNK_WORKQUEUE_CONTEXT, *PPH_LOADER_IMPORT_THUNK_WORKQUEUE_CONTEXT;

/**
 * Thread pool callback function that resolves import thunks asynchronously.
 *
 * \param Instance The callback instance.
 * \param Context A pointer to the PH_LOADER_IMPORT_THUNK_WORKQUEUE_CONTEXT structure.
 * \param Work The work item.
 * \remarks This function is called by the thread pool to resolve imports in parallel when
 * PH_NATIVE_LOADER_WORKQUEUE is defined. It calls PhLoaderEntrySnapImportThunk() and raises
 * an exception on failure. The context is freed after processing.
 */
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

/**
 * Resolves all imports from a specific DLL for a single import descriptor.
 *
 * \param [in] BaseAddress The base address of the importing image.
 * \param [in] ImportDirectory The import descriptor for the DLL.
 * \param [in] ImportDllName The name of the DLL to resolve imports from.
 * \return NTSTATUS Successful or errant status.
 * \retval STATUS_INVALID_PARAMETER One of the required parameters is NULL.
 * \remarks This function loads the specified DLL (if not already loaded), then iterates
 * through all import thunks for that DLL and resolves them to actual function addresses.
 * Special handling exists for self-imports (SystemInformer.exe importing from itself).
 * When PH_NATIVE_LOADER_WORKQUEUE is enabled, imports are resolved in parallel using a
 * thread pool for better performance.
 */
NTSTATUS PhLoaderEntrySnapImportDirectory(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_IMPORT_DESCRIPTOR ImportDirectory,
    _In_ PCSTR ImportDllName
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PCSTR importName;
    PIMAGE_THUNK_DATA importThunk;
    PIMAGE_THUNK_DATA originalThunk;
    PVOID importBaseAddress;

    if (!BaseAddress || !ImportDirectory || !ImportDllName)
        return STATUS_INVALID_PARAMETER;

    importName = PTR_ADD_OFFSET(BaseAddress, ImportDirectory->Name);
    importThunk = PTR_ADD_OFFSET(BaseAddress, ImportDirectory->FirstThunk);
    originalThunk = PTR_ADD_OFFSET(BaseAddress, ImportDirectory->OriginalFirstThunk);

    if (PhEqualBytesZ(importName, ImportDllName, FALSE))
    {
        importBaseAddress = NtCurrentImageBase();
    }
    else
    {
        WCHAR dllImportName[DOS_MAX_PATH_LENGTH] = L"";

        PhZeroExtendToUtf16Buffer(importName, PhCountBytesZ(importName), dllImportName);

        importBaseAddress = PhLoadLibrary(dllImportName);
    }

    if (!importBaseAddress)
    {
        return STATUS_DLL_NOT_FOUND;
    }

#if defined(PH_NATIVE_LOADER_WORKQUEUE)
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
    PCSTR ImportName;
} PH_LOADER_IMPORTS_WORKQUEUE_CONTEXT, *PPH_LOADER_IMPORTS_WORKQUEUE_CONTEXT;

/**
 * Thread pool callback function that resolves import directories asynchronously.
 *
 * \param [in,out] Instance The callback instance.
 * \param [in,out,opt] Context A pointer to the PH_LOADER_IMPORTS_WORKQUEUE_CONTEXT structure.
 * \param [in,out] Work The work item.
 * \remarks This function is called by the thread pool to resolve import directories in parallel
 * when PH_NATIVE_LOADER_WORKQUEUE is defined. It calls PhLoaderEntrySnapImportDirectory() and
 * raises an exception on failure. The context is freed after processing.
 */
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
        context->ImportDirectory,
        context->ImportName
        );

    if (!NT_SUCCESS(status))
    {
        PhRaiseStatus(status);
    }

    PhFree(context);
}
#endif

/**
 * Resolves all standard imports for a loaded image from a specific DLL.
 *
 * \param [in] BaseAddress The base address of the image.
 * \param [in] ImageNtHeader The NT headers of the image.
 * \param [in] ImportDllName The name of the DLL to resolve imports from.
 * \return NTSTATUS Successful or errant status.
 * \remarks This function processes the import directory table, locating all import descriptors
 * and resolving their thunks. Memory protection is temporarily changed to PAGE_READWRITE to
 * allow updating the IAT, then restored. On ARM64, the instruction cache is flushed after
 * updates. When PH_NATIVE_LOADER_WORKQUEUE is enabled, import directories are processed
 * in parallel using a thread pool.
 */
static NTSTATUS PhpFixupLoaderEntryImageImports(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_NT_HEADERS ImageNtHeader,
    _In_ PCSTR ImportDllName
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

    status = PhProtectVirtualMemory(
        NtCurrentProcess(),
        importDirectorySectionAddress,
        importDirectorySectionSize,
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
        context->BaseAddress = BaseAddress;
        context->ImportDirectory = importDirectory;
        context->ImportName = ImportDllName;

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
            importDirectory,
            ImportDllName
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

    status = PhProtectVirtualMemory(
        NtCurrentProcess(),
        importDirectorySectionAddress,
        importDirectorySectionSize,
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

/**
 * Resolves all delay-load imports for a loaded image from a specific DLL.
 *
 * \param [in] BaseAddress The base address of the image.
 * \param [in] ImageNtHeaders The NT headers of the image.
 * \param [in] ImportDllName The name of the DLL to resolve delay-load imports from.
 * \return NTSTATUS Successful or errant status.
 * \retval STATUS_SUCCESS All delay-load imports were resolved or no delay-load imports exist.
 * \remarks Delay-load imports are resolved on-demand rather than at load time. This function
 * processes the delay-load import directory, loading required DLLs and binding their imports.
 * Special handling exists for self-imports and cached module handles. If the import DLL is already
 * cached in the module handle, it is reused to avoid redundant loading.
 */
static NTSTATUS PhpFixupLoaderEntryImageDelayImports(
    _In_ PVOID BaseAddress,
    _In_ PIMAGE_NT_HEADERS ImageNtHeaders,
    _In_ PCSTR ImportDllName
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

    status = PhProtectVirtualMemory(
        NtCurrentProcess(),
        importDirectorySectionAddress,
        importDirectorySectionSize,
        PAGE_READWRITE,
        &importDirectoryProtect
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    while (delayImportDirectory->ImportAddressTableRVA && delayImportDirectory->ImportNameTableRVA)
    {
        PCSTR importName;
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
                importBaseAddress = NtCurrentImageBase();
                status = STATUS_SUCCESS;
            }
            else if (ReadPointerAcquire(importHandle))
            {
                importBaseAddress = ReadPointerAcquire(importHandle);
                status = STATUS_SUCCESS;
            }
            else
            {
                WCHAR dllImportName[DOS_MAX_PATH_LENGTH] = L"";

                PhZeroExtendToUtf16Buffer(importName, PhCountBytesZ(importName), dllImportName);

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

    status = PhProtectVirtualMemory(
        NtCurrentProcess(),
        importDirectorySectionAddress,
        importDirectorySectionSize,
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

/**
 * Validates that a section handle represents a valid executable DLL image.
 *
 * \param [in] SectionHandle A handle to the section to validate.
 * \return NTSTATUS Successful or errant status.
 * \retval STATUS_INVALID_IMPORT_OF_NON_DLL The section is not a valid DLL or executable.
 * \retval STATUS_IMAGE_MACHINE_TYPE_MISMATCH The image architecture doesn't match the process.
 * \remarks This function queries section information to verify it's a Windows GUI subsystem
 * image with DLL and executable characteristics. It also validates the machine type matches
 * the current process architecture (x64 for 64-bit processes, x86 for 32-bit processes).
 * ARM64 images are also accepted on 64-bit processes.
 */
NTSTATUS PhLoaderEntryIsSectionImageExecutable(
    _In_ HANDLE SectionHandle
    )
{
    NTSTATUS status;
    SECTION_IMAGE_INFORMATION sectionInfo;

    status = NtQuerySection(
        SectionHandle,
        SectionImageInformation,
        &sectionInfo,
        sizeof(SECTION_IMAGE_INFORMATION),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (sectionInfo.SubSystemType != IMAGE_SUBSYSTEM_WINDOWS_GUI)
        return STATUS_INVALID_IMPORT_OF_NON_DLL;
    if (!FlagOn(sectionInfo.ImageCharacteristics, IMAGE_FILE_DLL | IMAGE_FILE_EXECUTABLE_IMAGE))
        return STATUS_INVALID_IMPORT_OF_NON_DLL;

#ifdef _WIN64
    if (sectionInfo.Machine != IMAGE_FILE_MACHINE_AMD64 && sectionInfo.Machine != IMAGE_FILE_MACHINE_ARM64)
        return STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
#else
    if (sectionInfo.Machine != IMAGE_FILE_MACHINE_I386)
        return STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
#endif

    return STATUS_SUCCESS;
}

/**
 * Applies base relocations to an image loaded at a different address than its preferred base.
 *
 * \param [in] BaseAddress The base address where the image is loaded.
 * \return NTSTATUS Successful or errant status.
 * \remarks This function is essential when loading DLLs at non-preferred base addresses due to
 * ASLR or address conflicts. It calculates the relocation delta (actual base - preferred base),
 * then iterates through all relocation blocks applying fixups to adjust pointers.
 */
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

    for (USHORT i = 0; i < imageNtHeader->FileHeader.NumberOfSections; i++)
    {
        PIMAGE_SECTION_HEADER sectionHeader;
        PVOID sectionHeaderAddress;
        SIZE_T sectionHeaderSize;
        ULONG sectionProtectionJunk = 0;

        sectionHeader = PTR_ADD_OFFSET(IMAGE_FIRST_SECTION(imageNtHeader), UInt32x32To64(IMAGE_SIZEOF_SECTION_HEADER, i));
        sectionHeaderAddress = PTR_ADD_OFFSET(BaseAddress, sectionHeader->VirtualAddress);
        sectionHeaderSize = sectionHeader->SizeOfRawData;

        status = PhProtectVirtualMemory(
            NtCurrentProcess(),
            sectionHeaderAddress,
            sectionHeaderSize,
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

    for (USHORT i = 0; i < imageNtHeader->FileHeader.NumberOfSections; i++)
    {
        PIMAGE_SECTION_HEADER sectionHeader;
        PVOID sectionHeaderAddress;
        SIZE_T sectionHeaderSize;
        ULONG sectionProtection = 0;
        ULONG sectionProtectionJunk = 0;

        sectionHeader = PTR_ADD_OFFSET(IMAGE_FIRST_SECTION(imageNtHeader), UInt32x32To64(IMAGE_SIZEOF_SECTION_HEADER, i));
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

        status = PhProtectVirtualMemory(
            NtCurrentProcess(),
            sectionHeaderAddress,
            sectionHeaderSize,
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

/**
 * Retrieves the export name for a given ordinal from a DLL.
 *
 * \param [in] DllBase The base address of the DLL.
 * \param [in,opt] ProcedureNumber The ordinal number of the exported function.
 * \return A string containing the export name, or NULL if not found.
 * \remarks This function searches the export name table for an entry matching the specified
 * ordinal. If found and not a forwarder, it returns the function name. If the export is a
 * forwarder (RVA within the export directory), it returns the forwarder string. The returned
 * string must be freed by the caller using PhDereferenceObject().
 */
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
                return PhZeroExtendToUtf16((PCSTR)baseAddress);
            }
            else
            {
                return PhZeroExtendToUtf16(PTR_ADD_OFFSET(DllBase, exportNameTable[i]));
            }
        }
    }

    return NULL;
}

/**
 * Loads a DLL from a file into the current process using native loader functionality.
 *
 * \param [in] FileName The path to the DLL file as a string reference.
 * \param [in,opt] RootDirectory An optional handle to a root directory for relative paths.
 * \param [out] BaseAddress A pointer that receives the base address of the loaded DLL.
 * \return NTSTATUS Successful or errant status.
 * \remarks This is only built to support plugins.
 */
NTSTATUS PhLoaderEntryLoadDll(
    _In_ PCPH_STRINGREF FileName,
    _In_opt_ HANDLE RootDirectory,
    _Out_ PVOID* BaseAddress
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    HANDLE sectionHandle;
    PVOID imageBaseAddress;
    SIZE_T imageBaseSize;

    status = PhCreateFileEx(
        &fileHandle,
        FileName,
        FILE_READ_DATA | FILE_EXECUTE | SYNCHRONIZE,
        RootDirectory,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhCreateSection(
        &sectionHandle,
        SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_EXECUTE,
        NULL,
        PAGE_EXECUTE,
        SEC_IMAGE,
        fileHandle
        );

    NtClose(fileHandle);

    if (!NT_SUCCESS(status))
        return status;

    status = PhLoaderEntryIsSectionImageExecutable(
        sectionHandle
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(sectionHandle);
        return status;
    }

    imageBaseAddress = NULL;
    imageBaseSize = 0;

    status = PhMapViewOfSection(
        sectionHandle,
        NtCurrentProcess(),
        &imageBaseAddress,
        0,
        NULL,
        &imageBaseSize,
        ViewUnmap,
        0,
        PAGE_EXECUTE
        );

    NtClose(sectionHandle);

    if (status == STATUS_IMAGE_NOT_AT_BASE)
    {
        status = PhLoaderEntryRelocateImage(imageBaseAddress);
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
        PhUnmapViewOfSection(NtCurrentProcess(), imageBaseAddress);
    }

    return status;
}

/**
 * Unloads a manually-loaded DLL from the current process.
 *
 * \param [in] BaseAddress The base address of the DLL to unload.
 * \return NTSTATUS Successful or errant status.
 * \remarks This function calls the DLL's entry point with DLL_PROCESS_DETACH to allow it to
 * perform cleanup operations, then unmaps the image from the process address space. If the
 * DLL entry point returns FALSE during detachment, STATUS_DLL_INIT_FAILED is returned.
 * This is the cleanup counterpart to PhLoaderEntryLoadDll().
 */
NTSTATUS PhLoaderEntryUnloadDll(
    _In_ PVOID BaseAddress
    )
{
    NTSTATUS status;
    PIMAGE_NT_HEADERS imageNtHeaders;
    PDLL_INIT_ROUTINE imageEntryRoutine;

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

    PhUnmapViewOfSection(NtCurrentProcess(), BaseAddress);

    return status;
}

/**
 * Resolves all delay-load imports for a specific DLL within a loaded image.
 *
 * \param [in] BaseAddress The base address of the image to process.
 * \param [in] ImportDllName The name of the DLL whose imports should be resolved (e.g., "SystemInformer.exe").
 * \return NTSTATUS Successful or errant status.
 * \remarks This function resolves delay-load imports from the specified DLL by calling
 * PhpFixupLoaderEntryImageDelayImports(). Delay-load imports are not resolved at load time
 * but are instead resolved on first use. This function allows preemptive resolution, which
 * is useful for plugins that need to ensure all imports from the host application are
 * available before proceeding.
 */
NTSTATUS PhLoaderEntryLoadAllImportsForDll(
    _In_ PVOID BaseAddress,
    _In_ PCSTR ImportDllName
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

/**
 * Resolves all delay-load imports for a specific DLL within a loaded image identified by name.
 *
 * \param [in] TargetDllName The name of the target DLL whose imports should be resolved.
 * \param [in] ImportDllName The name of the DLL to import from (e.g., "SystemInformer.exe").
 * \return NTSTATUS Successful or errant status.
 * \retval STATUS_INVALID_PARAMETER The target DLL was not found in the process.
 * \remarks This is a convenience wrapper around PhLoaderEntryLoadAllImportsForDll() that
 * looks up the DLL by name in the loader data structures. It's useful when you know the
 * DLL name but not its base address.
 */
NTSTATUS PhLoadAllImportsForDll(
    _In_ PCWSTR TargetDllName,
    _In_ PCSTR ImportDllName
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

/**
 * Loads a System Informer plugin image with full initialization.
 *
 * \param FileName The path to the plugin DLL as a string reference.
 * \param RootDirectory An optional handle to a root directory for relative paths.
 * \param BaseAddress An optional pointer that receives the base address of the loaded plugin.
 * \return NTSTATUS Successful or errant status.
 * \retval STATUS_DLL_INIT_FAILED The plugin's DllMain returned FALSE during initialization.
 * \remarks This function performs a complete plugin load sequence:
 * 1. Loads the plugin using PhLoaderEntryLoadDll() or LdrLoadDll() (if PH_NATIVE_PLUGIN_IMAGE_LOAD is defined)
 * 2. Resolves standard imports from SystemInformer.exe (this is required since the executable might be renamed)
 * 3. Resolves delay-load imports from SystemInformer.exe (this is required since the executable might be renamed)
 * 4. Calls the plugin's DllMain with DLL_PROCESS_ATTACH (optionally including an extra internal context)
 * The loader lock is held during the operation unless PH_NATIVE_PLUGIN_IMAGE_LOAD is defined.
 * On failure, the plugin is unloaded before returning.
 */
NTSTATUS PhLoadPluginImage(
    _In_ PCPH_STRINGREF FileName,
    _In_opt_ HANDLE RootDirectory,
    _Out_opt_ PVOID *BaseAddress
    )
{
    NTSTATUS status;
    PVOID imageBaseAddress;
    PIMAGE_NT_HEADERS imageNtHeaders;
    PDLL_INIT_ROUTINE imageEntryRoutine;

#if defined(PH_NATIVE_PLUGIN_IMAGE_LOAD)
    UNICODE_STRING imageFileName;
    ULONG imageType;

    if (!PhStringRefToUnicodeString(FileName, &imageFileName))
        return STATUS_NAME_TOO_LONG;

    imageType = IMAGE_FILE_EXECUTABLE_IMAGE;

    status = LdrLoadDll(
        NULL,
        &imageType,
        &imageFileName,
        &imageBaseAddress
        );
#else
    PhAcquireLoaderLock();

    status = PhLoaderEntryLoadDll(
        FileName,
        RootDirectory,
        &imageBaseAddress
        );
#endif

    if (!NT_SUCCESS(status))
    {
#if !defined(PH_NATIVE_PLUGIN_IMAGE_LOAD)
        PhReleaseLoaderLock();
#endif
        return status;
    }

    status = PhGetLoaderEntryImageNtHeaders(
        imageBaseAddress,
        &imageNtHeaders
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhpFixupLoaderEntryImageImports(
        imageBaseAddress,
        imageNtHeaders,
        "SystemInformer.exe"
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
#if defined(PH_NATIVE_PLUGIN_IMAGE_LOAD)
        LdrUnloadDll(imageBaseAddress);
#else
        PhLoaderEntryUnloadDll(imageBaseAddress);
#endif
    }

#if !defined(PH_NATIVE_PLUGIN_IMAGE_LOAD)
    PhReleaseLoaderLock();
#endif

    return status;
}

/**
 * Determines the binary type of an executable file (32-bit, 64-bit, or DOS).
 *
 * \param FileName The path to the executable file.
 * \param BinaryType A pointer that receives the binary type constant (SCS_32BIT_BINARY, SCS_64BIT_BINARY, or SCS_DOS_BINARY).
 * \return NTSTATUS Successful or errant status.
 * \remarks This function mimics the behavior of GetBinaryTypeW by creating a SEC_IMAGE section
 * and querying its machine type. It returns:
 * - SCS_32BIT_BINARY for x86 images or STATUS_INVALID_IMAGE_WIN_32
 * - SCS_64BIT_BINARY for x64/IA64 images or STATUS_INVALID_IMAGE_WIN_64
 * - SCS_DOS_BINARY for DOS executables (STATUS_INVALID_IMAGE_PROTECT) or .COM/.PIF/.EXE files without valid PE headers
 * This is useful for determining compatibility before attempting to load an image.
 */
// based on GetBinaryTypeW (dmex)
NTSTATUS PhGetFileBinaryTypeWin32(
    _In_ PCWSTR FileName,
    _Out_ PULONG BinaryType
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    HANDLE sectionHandle;
    UNICODE_STRING fileName;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    SECTION_IMAGE_INFORMATION section;

    RtlZeroMemory(&section, sizeof(SECTION_IMAGE_INFORMATION));

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

    status = PhCreateSection(
        &sectionHandle,
        SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_EXECUTE,
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

/**
 * Converts a system DLL init block from the current OS version layout to the latest layout.
 *
 * \param[in] Source A buffer returned or copied from LdrSystemDllInitBlock.
 * \param[out] Destination A buffer for storing a version-independent copy of the structure.
 */
NTSTATUS PhCaptureSystemDllInitBlock(
    _In_ PVOID Source,
    _Out_ PPS_SYSTEM_DLL_INIT_BLOCK Destination
    )
{
    ULONG sourceSize;

    RtlZeroMemory(Destination, sizeof(PS_SYSTEM_DLL_INIT_BLOCK));

    if (WindowsVersion >= WINDOWS_10_20H1)
    {
        PPS_SYSTEM_DLL_INIT_BLOCK_V3 source = (PPS_SYSTEM_DLL_INIT_BLOCK_V3)Source;

        sourceSize = source->Size;

        if (sourceSize > sizeof(PS_SYSTEM_DLL_INIT_BLOCK_V3))
            sourceSize = sizeof(PS_SYSTEM_DLL_INIT_BLOCK_V3);

        // No adjustments necessary for the latest layout
        RtlCopyMemory(Destination, source, sourceSize);
        Destination->Size = sourceSize;

        return STATUS_SUCCESS;
    }
    else if (WindowsVersion >= WINDOWS_10_RS2)
    {
        PPS_SYSTEM_DLL_INIT_BLOCK_V2 source = (PPS_SYSTEM_DLL_INIT_BLOCK_V2)Source;

        sourceSize = source->Size;

        if (sourceSize > sizeof(PS_SYSTEM_DLL_INIT_BLOCK_V2))
            sourceSize = sizeof(PS_SYSTEM_DLL_INIT_BLOCK_V2);

        // Everything up to and including Flags uses the same layout
        if (RTL_CONTAINS_FIELD(source, sourceSize, Flags))
        {
            RtlCopyMemory(
                Destination,
                source,
                RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK_V2, Flags)
                );

            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, Flags);
        }

        // Mitigation options include 2 out of 3 values
        if (RTL_CONTAINS_FIELD(source, sourceSize, MitigationOptionsMap))
        {
            Destination->MitigationOptionsMap.Map[0] = source->MitigationOptionsMap.Map[0];
            Destination->MitigationOptionsMap.Map[1] = source->MitigationOptionsMap.Map[1];
            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, MitigationOptionsMap.Map[1]);
        }

        // The subsequent fields are shifted
        if (RTL_CONTAINS_FIELD(source, sourceSize, CfgBitMap))
        {
            Destination->CfgBitMap = source->CfgBitMap;
            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, CfgBitMap);
        }

        if (RTL_CONTAINS_FIELD(source, sourceSize, CfgBitMapSize))
        {
            Destination->CfgBitMapSize = source->CfgBitMapSize;
            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, CfgBitMapSize);
        }

        if (RTL_CONTAINS_FIELD(source, sourceSize, Wow64CfgBitMap))
        {
            Destination->Wow64CfgBitMap = source->Wow64CfgBitMap;
            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, Wow64CfgBitMap);
        }

        if (RTL_CONTAINS_FIELD(source, sourceSize, Wow64CfgBitMapSize))
        {
            Destination->Wow64CfgBitMapSize = source->Wow64CfgBitMapSize;
            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, Wow64CfgBitMapSize);
        }

        // Mitigation audit options include 2 out of 3 values
        if (RTL_CONTAINS_FIELD(source, sourceSize, MitigationAuditOptionsMap))
        {
            Destination->MitigationAuditOptionsMap.Map[0] = source->MitigationAuditOptionsMap.Map[0];
            Destination->MitigationAuditOptionsMap.Map[1] = source->MitigationAuditOptionsMap.Map[1];
            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, MitigationAuditOptionsMap.Map[1]);
        }

        return STATUS_SUCCESS;
    }
    else if (WindowsVersion >= PHNT_WINDOWS_8)
    {
        PPS_SYSTEM_DLL_INIT_BLOCK_V1 source = (PPS_SYSTEM_DLL_INIT_BLOCK_V1)Source;

        sourceSize = source->Size;

        if (sourceSize > sizeof(PS_SYSTEM_DLL_INIT_BLOCK_V1))
            sourceSize = sizeof(PS_SYSTEM_DLL_INIT_BLOCK_V1);

        // All fields are shifted
        if (RTL_CONTAINS_FIELD(source, sourceSize, SystemDllWowRelocation))
        {
            Destination->SystemDllWowRelocation = source->SystemDllWowRelocation;
            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, SystemDllWowRelocation);
        }

        if (RTL_CONTAINS_FIELD(source, sourceSize, SystemDllNativeRelocation))
        {
            Destination->SystemDllNativeRelocation = source->SystemDllNativeRelocation;
            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, SystemDllNativeRelocation);
        }

        if (RTL_CONTAINS_FIELD(source, sourceSize, Wow64SharedInformation))
        {
            RtlCopyMemory(
                &Destination->Wow64SharedInformation,
                &source->Wow64SharedInformation,
                RTL_FIELD_SIZE(PS_SYSTEM_DLL_INIT_BLOCK_V1, Wow64SharedInformation)
                );

            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, Wow64SharedInformation);
        }

        if (RTL_CONTAINS_FIELD(source, sourceSize, RngData))
        {
            Destination->RngData = source->RngData;
            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, RngData);
        }

        if (RTL_CONTAINS_FIELD(source, sourceSize, Flags))
        {
            Destination->Flags = source->Flags;
            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, Flags);
        }

        // Mitigation options include 1 out of 3 values
        if (RTL_CONTAINS_FIELD(source, sourceSize, MitigationOptions))
        {
            Destination->MitigationOptionsMap.Map[0] = source->MitigationOptions;
            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, MitigationOptionsMap.Map[0]);
        }

        if (RTL_CONTAINS_FIELD(source, sourceSize, CfgBitMap))
        {
            Destination->CfgBitMap = source->CfgBitMap;
            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, CfgBitMap);
        }

        if (RTL_CONTAINS_FIELD(source, sourceSize, CfgBitMapSize))
        {
            Destination->CfgBitMapSize = source->CfgBitMapSize;
            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, CfgBitMapSize);
        }

        if (RTL_CONTAINS_FIELD(source, sourceSize, Wow64CfgBitMap))
        {
            Destination->Wow64CfgBitMap = source->Wow64CfgBitMap;
            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, Wow64CfgBitMap);
        }

        if (RTL_CONTAINS_FIELD(source, sourceSize, Wow64CfgBitMapSize))
        {
            Destination->Wow64CfgBitMapSize = source->Wow64CfgBitMapSize;
            Destination->Size = RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, Wow64CfgBitMapSize);
        }

        return STATUS_SUCCESS;
    }

    return STATUS_NOT_SUPPORTED;
}

/**
 * Retrieves a copy of the system DLL init block of the current process.
 *
 * \param[out] SystemDllInitBlock A buffer for storing a version-independent copy of LdrSystemDllInitBlock.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetSystemDllInitBlock(
    _Out_ PPS_SYSTEM_DLL_INIT_BLOCK SystemDllInitBlock
    )
{
    PVOID systemDllInitBlock;

    if (!LdrSystemDllInitBlock_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    systemDllInitBlock = LdrSystemDllInitBlock_Import();

    if (!systemDllInitBlock)
        return STATUS_NOT_SUPPORTED;

    return PhCaptureSystemDllInitBlock(systemDllInitBlock, SystemDllInitBlock);
}
