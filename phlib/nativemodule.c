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

_Function_class_(PHP_ENUM_PROCESS_MODULES_CALLBACK)
typedef BOOLEAN (NTAPI PHP_ENUM_PROCESS_MODULES_CALLBACK)(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_DATA_TABLE_ENTRY Entry,
    _In_ PVOID AddressOfEntry,
    _In_opt_ PVOID Context1,
    _In_opt_ PVOID Context2
    );
typedef PHP_ENUM_PROCESS_MODULES_CALLBACK* PPHP_ENUM_PROCESS_MODULES_CALLBACK;

_Function_class_(PHP_ENUM_PROCESS_MODULES32_CALLBACK)
typedef BOOLEAN (NTAPI PHP_ENUM_PROCESS_MODULES32_CALLBACK)(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_DATA_TABLE_ENTRY32 Entry,
    _In_ ULONG AddressOfEntry,
    _In_opt_ PVOID Context1,
    _In_opt_ PVOID Context2
    );
typedef PHP_ENUM_PROCESS_MODULES32_CALLBACK* PPHP_ENUM_PROCESS_MODULES32_CALLBACK;


/**
 * Enumerates the modules loaded by the kernel.
 *
 * \param Modules A variable which receives a pointer to a structure containing information about
 * the kernel modules. You must free the structure using PhFree() when you no longer need it.
 */
NTSTATUS PhEnumKernelModules(
    _Out_ PRTL_PROCESS_MODULES *Modules
    )
{
    static ULONG initialBufferSize = 0x1000;
    NTSTATUS status;
    PRTL_PROCESS_MODULES buffer;
    ULONG bufferSize;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemModuleInformation,
        buffer,
        bufferSize,
        &bufferSize
        );

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformation(
            SystemModuleInformation,
            buffer,
            bufferSize,
            &bufferSize
            );
    }

    if (!NT_SUCCESS(status))
        return status;

    if (bufferSize <= 0x100000) initialBufferSize = bufferSize;
    *Modules = buffer;

    return status;
}

/**
 * Enumerates the modules loaded by the kernel.
 *
 * \param Modules A variable which receives a pointer to a structure containing information about
 * the kernel modules. You must free the structure using PhFree() when you no longer need it.
 */
NTSTATUS PhEnumKernelModulesEx(
    _Out_ PRTL_PROCESS_MODULE_INFORMATION_EX *Modules
    )
{
    static ULONG initialBufferSize = 0x1000;
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemModuleInformationEx,
        buffer,
        bufferSize,
        &bufferSize
        );

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformation(
            SystemModuleInformationEx,
            buffer,
            bufferSize,
            &bufferSize
            );
    }

    if (!NT_SUCCESS(status))
        return status;

    if (bufferSize <= 0x100000) initialBufferSize = bufferSize;
    *Modules = buffer;

    return status;
}

/**
 * Gets the file name of the kernel image.
 *
 * \return A pointer to a string containing the kernel image file name. You must free the string
 * using PhDereferenceObject() when you no longer need it.
 */
PPH_STRING PhGetKernelFileName(
    VOID
    )
{
    NTSTATUS status;
    UCHAR buffer[FIELD_OFFSET(RTL_PROCESS_MODULES, Modules) + sizeof(RTL_PROCESS_MODULE_INFORMATION)] = { 0 };
    PRTL_PROCESS_MODULES modules;
    ULONG modulesLength;

    modules = (PRTL_PROCESS_MODULES)buffer;
    modulesLength = sizeof(buffer);

    status = NtQuerySystemInformation(
        SystemModuleInformation,
        modules,
        modulesLength,
        &modulesLength
        );

    if (status != STATUS_SUCCESS && status != STATUS_INFO_LENGTH_MISMATCH)
        return NULL;
    if (status == STATUS_SUCCESS || modules->NumberOfModules < 1)
        return NULL;

    return PhConvertUtf8ToUtf16(modules->Modules[0].FullPathName);
}

/**
 * Gets the file name of the kernel image without the SystemModuleInformation and string conversion overhead.
 *
 * \return A pointer to a string containing the kernel image file name. You must free the string
 * using PhDereferenceObject() when you no longer need it.
 */
PPH_STRING PhGetKernelFileName2(
    VOID
    )
{
    if (WindowsVersion >= WINDOWS_10)
    {
        static PH_STRINGREF kernelFileName = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntoskrnl.exe");

        return PhCreateString2(&kernelFileName);
    }

    return PhGetKernelFileName();
}

/**
 * Gets the file name, base address and size of the kernel image.
 *
 * \return A pointer to a string containing the kernel image file name. You must free the string
 * using PhDereferenceObject() when you no longer need it.
 */
NTSTATUS PhGetKernelFileNameEx(
    _Out_opt_ PPH_STRING* FileName,
    _Out_ PVOID* ImageBase,
    _Out_ ULONG* ImageSize
    )
{
    NTSTATUS status;
    UCHAR buffer[FIELD_OFFSET(RTL_PROCESS_MODULES, Modules) + sizeof(RTL_PROCESS_MODULE_INFORMATION)] = { 0 };
    PRTL_PROCESS_MODULES modules;
    ULONG modulesLength;

    modules = (PRTL_PROCESS_MODULES)buffer;
    modulesLength = sizeof(buffer);

    status = NtQuerySystemInformation(
        SystemModuleInformation,
        modules,
        modulesLength,
        &modulesLength
        );

    if (status != STATUS_SUCCESS && status != STATUS_INFO_LENGTH_MISMATCH)
        return status;
    if (status == STATUS_SUCCESS || modules->NumberOfModules < 1)
        return STATUS_UNSUCCESSFUL;

    if (FileName)
    {
        //if (WindowsVersion >= WINDOWS_10)
        //{
        //    static PH_STRINGREF kernelFileName = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntoskrnl.exe");
        //    *FileName = PhCreateString2(&kernelFileName);
        //}

        *FileName = PhConvertUtf8ToUtf16(modules->Modules[0].FullPathName);
    }

    if (WindowsVersion >= WINDOWS_10_22H2)
    {
        if (modules->Modules[0].ImageBase == NULL)
        {
            modules->Modules[0].ImageBase = (PVOID)(ULONG64_MAX - 1);
        }
    }

    *ImageBase = modules->Modules[0].ImageBase;
    *ImageSize = modules->Modules[0].ImageSize;

    return STATUS_SUCCESS;
}

PPH_STRING PhGetSecureKernelFileName(
    VOID
    )
{
    static PH_STRINGREF secureKernelFileName = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\securekernel.exe");
    static PH_STRINGREF secureKernelPathPart = PH_STRINGREF_INIT(L"\\securekernel.exe");
    PPH_STRING fileName = NULL;
    PPH_STRING kernelFileName;

    if (kernelFileName = PhGetKernelFileName())
    {
        PH_STRINGREF baseName;

        if (PhGetBasePath(&kernelFileName->sr, &baseName, NULL))
        {
            fileName = PhConcatStringRef2(&baseName, &secureKernelPathPart);
        }

        PhDereferenceObject(kernelFileName);
    }

    if (PhIsNullOrEmptyString(fileName))
    {
        fileName = PhCreateString2(&secureKernelFileName);
    }

    return fileName;
}

NTSTATUS PhpEnumProcessModules(
    _In_ HANDLE ProcessHandle,
    _In_ PPHP_ENUM_PROCESS_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context1,
    _In_opt_ PVOID Context2
    )
{
    NTSTATUS status;
    PPEB peb;
    PPEB_LDR_DATA ldr;
    PEB_LDR_DATA pebLdrData;
    PLIST_ENTRY startLink;
    PLIST_ENTRY currentLink;
    ULONG dataTableEntrySize;
    LDR_DATA_TABLE_ENTRY currentEntry;
    ULONG i;

    // Get the PEB address.
    status = PhGetProcessPeb(ProcessHandle, &peb);

    if (!NT_SUCCESS(status))
        return status;

    // Read the address of the loader data.
    status = NtReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(peb, FIELD_OFFSET(PEB, Ldr)),
        &ldr,
        sizeof(PVOID),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // Check the process has initialized (dmex)
    if (!ldr)
        return STATUS_UNSUCCESSFUL;

    // Read the loader data.
    status = NtReadVirtualMemory(
        ProcessHandle,
        ldr,
        &pebLdrData,
        sizeof(PEB_LDR_DATA),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (!pebLdrData.Initialized)
        return STATUS_UNSUCCESSFUL;

    if (WindowsVersion >= WINDOWS_11)
        dataTableEntrySize = LDR_DATA_TABLE_ENTRY_SIZE_WIN11;
    else if (WindowsVersion >= WINDOWS_8)
        dataTableEntrySize = LDR_DATA_TABLE_ENTRY_SIZE_WIN8;
    else
        dataTableEntrySize = LDR_DATA_TABLE_ENTRY_SIZE_WIN7;

    // Traverse the linked list (in load order).

    i = 0;
    startLink = PTR_ADD_OFFSET(ldr, FIELD_OFFSET(PEB_LDR_DATA, InLoadOrderModuleList));
    currentLink = pebLdrData.InLoadOrderModuleList.Flink;

    while (
        currentLink != startLink &&
        i <= PH_ENUM_PROCESS_MODULES_LIMIT
        )
    {
        PVOID addressOfEntry;

        addressOfEntry = CONTAINING_RECORD(currentLink, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        status = NtReadVirtualMemory(
            ProcessHandle,
            addressOfEntry,
            &currentEntry,
            dataTableEntrySize,
            NULL
            );

        if (!NT_SUCCESS(status))
            return status;

        // Make sure the entry is valid.
        if (currentEntry.DllBase)
        {
            // Execute the callback.
            if (!Callback(
                ProcessHandle,
                &currentEntry,
                addressOfEntry,
                Context1,
                Context2
                ))
                break;
        }

        currentLink = currentEntry.InLoadOrderLinks.Flink;
        i++;
    }

    return status;
}

BOOLEAN NTAPI PhpEnumProcessModulesCallback(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_DATA_TABLE_ENTRY Entry,
    _In_ PVOID AddressOfEntry,
    _In_ PVOID Context1,
    _In_ PVOID Context2
    )
{
    PPH_ENUM_PROCESS_MODULES_PARAMETERS parameters = Context1;
    NTSTATUS status;
    BOOLEAN result;
    PPH_STRING mappedFileName = NULL;
    PWSTR fullDllNameOriginal;
    PWSTR fullDllNameBuffer = NULL;
    PWSTR baseDllNameOriginal;
    PWSTR baseDllNameBuffer = NULL;

    if (parameters->Flags & PH_ENUM_PROCESS_MODULES_TRY_MAPPED_FILE_NAME)
    {
        PhGetProcessMappedFileName(ProcessHandle, Entry->DllBase, &mappedFileName);
    }

    if (mappedFileName)
    {
        ULONG_PTR indexOfLastBackslash;

        PhStringRefToUnicodeString(&mappedFileName->sr, &Entry->FullDllName);
        indexOfLastBackslash = PhFindLastCharInString(mappedFileName, 0, OBJ_NAME_PATH_SEPARATOR);

        if (indexOfLastBackslash != SIZE_MAX)
        {
            Entry->BaseDllName.Buffer = Entry->FullDllName.Buffer + indexOfLastBackslash + 1;
            Entry->BaseDllName.Length = Entry->FullDllName.Length - (USHORT)indexOfLastBackslash * sizeof(WCHAR) - sizeof(UNICODE_NULL);
            Entry->BaseDllName.MaximumLength = Entry->BaseDllName.Length + sizeof(UNICODE_NULL);
        }
        else
        {
            Entry->BaseDllName = Entry->FullDllName;
        }
    }
    else
    {
        // Read the full DLL name string and add a null terminator.

        fullDllNameOriginal = Entry->FullDllName.Buffer;
        fullDllNameBuffer = PhAllocate(Entry->FullDllName.Length + sizeof(UNICODE_NULL));
        Entry->FullDllName.Buffer = fullDllNameBuffer;

        if (NT_SUCCESS(status = NtReadVirtualMemory(
            ProcessHandle,
            fullDllNameOriginal,
            fullDllNameBuffer,
            Entry->FullDllName.Length,
            NULL
            )))
        {
            fullDllNameBuffer[Entry->FullDllName.Length / sizeof(WCHAR)] = UNICODE_NULL;
        }
        else
        {
            fullDllNameBuffer[0] = UNICODE_NULL;
            Entry->FullDllName.Length = 0;
        }

        baseDllNameOriginal = Entry->BaseDllName.Buffer;

        // Try to use the buffer we just read in.
        if (
            NT_SUCCESS(status) &&
            (ULONG_PTR)baseDllNameOriginal >= (ULONG_PTR)fullDllNameOriginal &&
            (ULONG_PTR)PTR_ADD_OFFSET(baseDllNameOriginal, Entry->BaseDllName.Length) >= (ULONG_PTR)baseDllNameOriginal &&
            (ULONG_PTR)PTR_ADD_OFFSET(baseDllNameOriginal, Entry->BaseDllName.Length) <= (ULONG_PTR)PTR_ADD_OFFSET(fullDllNameOriginal, Entry->FullDllName.Length)
            )
        {
            baseDllNameBuffer = NULL;

            Entry->BaseDllName.Buffer = PTR_ADD_OFFSET(Entry->FullDllName.Buffer, (baseDllNameOriginal - fullDllNameOriginal));
        }
        else
        {
            // Read the base DLL name string and add a null terminator.

            baseDllNameBuffer = PhAllocate(Entry->BaseDllName.Length + sizeof(UNICODE_NULL));
            Entry->BaseDllName.Buffer = baseDllNameBuffer;

            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                baseDllNameOriginal,
                baseDllNameBuffer,
                Entry->BaseDllName.Length,
                NULL
                )))
            {
                baseDllNameBuffer[Entry->BaseDllName.Length / sizeof(WCHAR)] = UNICODE_NULL;
            }
            else
            {
                baseDllNameBuffer[0] = UNICODE_NULL;
                Entry->BaseDllName.Length = 0;
            }
        }
    }

    if (WindowsVersion >= WINDOWS_8 && Entry->DdagNode)
    {
        LDR_DDAG_NODE ldrDagNode;

        memset(&ldrDagNode, 0, sizeof(LDR_DDAG_NODE));

        if (NT_SUCCESS(NtReadVirtualMemory(
            ProcessHandle,
            Entry->DdagNode,
            &ldrDagNode,
            sizeof(LDR_DDAG_NODE),
            NULL
            )))
        {
            // Fixup the module load count. (dmex)
            Entry->ObsoleteLoadCount = (USHORT)ldrDagNode.LoadCount;
        }
    }

    // Execute the callback.
    result = parameters->Callback(Entry, parameters->Context);

    if (mappedFileName)
    {
        PhDereferenceObject(mappedFileName);
    }
    else
    {
        if (fullDllNameBuffer)
            PhFree(fullDllNameBuffer);
        if (baseDllNameBuffer)
            PhFree(baseDllNameBuffer);
    }

    return result;
}

/**
 * Enumerates the modules loaded by a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param Callback A callback function which is executed for each process module.
 * \param Context A user-defined value to pass to the callback function.
 */
NTSTATUS PhEnumProcessModules(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PH_ENUM_PROCESS_MODULES_PARAMETERS parameters;

    parameters.Callback = Callback;
    parameters.Context = Context;
    parameters.Flags = 0;

    return PhEnumProcessModulesEx(ProcessHandle, &parameters);
}

/**
 * Enumerates the modules loaded by a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access. If
 * \c PH_ENUM_PROCESS_MODULES_TRY_MAPPED_FILE_NAME is specified in \a Parameters, the handle should
 * have PROCESS_QUERY_INFORMATION access.
 * \param Parameters The enumeration parameters.
 */
NTSTATUS PhEnumProcessModulesEx(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_PARAMETERS Parameters
    )
{
    return PhpEnumProcessModules(
        ProcessHandle,
        PhpEnumProcessModulesCallback,
        Parameters,
        NULL
        );
}

typedef struct _SET_PROCESS_MODULE_LOAD_COUNT_CONTEXT
{
    NTSTATUS Status;
    PVOID BaseAddress;
    ULONG LoadCount;
} SET_PROCESS_MODULE_LOAD_COUNT_CONTEXT, *PSET_PROCESS_MODULE_LOAD_COUNT_CONTEXT;

BOOLEAN NTAPI PhpSetProcessModuleLoadCountCallback(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_DATA_TABLE_ENTRY Entry,
    _In_ PVOID AddressOfEntry,
    _In_ PVOID Context1,
    _In_opt_ PVOID Context2
    )
{
    PSET_PROCESS_MODULE_LOAD_COUNT_CONTEXT context = Context1;

    if (Entry->DllBase == context->BaseAddress)
    {
        context->Status = NtWriteVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(AddressOfEntry, FIELD_OFFSET(LDR_DATA_TABLE_ENTRY, ObsoleteLoadCount)),
            &context->LoadCount,
            sizeof(USHORT),
            NULL
            );

        return FALSE;
    }

    return TRUE;
}

/**
 * Sets the load count of a process module.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_VM_READ and PROCESS_VM_WRITE access.
 * \param BaseAddress The base address of a module.
 * \param LoadCount The new load count of the module.
 *
 * \retval STATUS_DLL_NOT_FOUND The module was not found.
 */
NTSTATUS PhSetProcessModuleLoadCount(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ ULONG LoadCount
    )
{
    NTSTATUS status;
    SET_PROCESS_MODULE_LOAD_COUNT_CONTEXT context;

    context.Status = STATUS_DLL_NOT_FOUND;
    context.BaseAddress = BaseAddress;
    context.LoadCount = LoadCount;

    status = PhpEnumProcessModules(
        ProcessHandle,
        PhpSetProcessModuleLoadCountCallback,
        &context,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    return context.Status;
}

NTSTATUS PhpEnumProcessModules32(
    _In_ HANDLE ProcessHandle,
    _In_ PPHP_ENUM_PROCESS_MODULES32_CALLBACK Callback,
    _In_opt_ PVOID Context1,
    _In_opt_ PVOID Context2
    )
{
    NTSTATUS status;
    PPEB32 peb;
    ULONG ldr; // PEB_LDR_DATA32 *32
    PEB_LDR_DATA32 pebLdrData;
    ULONG startLink; // LIST_ENTRY32 *32
    ULONG currentLink; // LIST_ENTRY32 *32
    ULONG dataTableEntrySize;
    LDR_DATA_TABLE_ENTRY32 currentEntry;
    ULONG i;

    // Get the 32-bit PEB address.
    status = PhGetProcessPeb32(ProcessHandle, &peb);

    if (!NT_SUCCESS(status))
        return status;

    // Read the address of the loader data.
    status = NtReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(peb, FIELD_OFFSET(PEB32, Ldr)),
        &ldr,
        sizeof(ULONG),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // Check the process has initialized (dmex)
    if (!ldr)
        return STATUS_UNSUCCESSFUL;

    // Read the loader data.
    status = NtReadVirtualMemory(
        ProcessHandle,
        UlongToPtr(ldr),
        &pebLdrData,
        sizeof(PEB_LDR_DATA32),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (!pebLdrData.Initialized)
        return STATUS_UNSUCCESSFUL;

    if (WindowsVersion >= WINDOWS_11)
        dataTableEntrySize = LDR_DATA_TABLE_ENTRY_SIZE_WIN11_32;
    else if (WindowsVersion >= WINDOWS_8)
        dataTableEntrySize = LDR_DATA_TABLE_ENTRY_SIZE_WIN8_32;
    else
        dataTableEntrySize = LDR_DATA_TABLE_ENTRY_SIZE_WIN7_32;

    // Traverse the linked list (in load order).

    i = 0;
    startLink = ldr + UFIELD_OFFSET(PEB_LDR_DATA32, InLoadOrderModuleList);
    currentLink = pebLdrData.InLoadOrderModuleList.Flink;

    while (
        currentLink != startLink &&
        i <= PH_ENUM_PROCESS_MODULES_LIMIT
        )
    {
        ULONG addressOfEntry;

        addressOfEntry = PtrToUlong(CONTAINING_RECORD(UlongToPtr(currentLink), LDR_DATA_TABLE_ENTRY32, InLoadOrderLinks));
        status = NtReadVirtualMemory(
            ProcessHandle,
            UlongToPtr(addressOfEntry),
            &currentEntry,
            dataTableEntrySize,
            NULL
            );

        if (!NT_SUCCESS(status))
            return status;

        // Make sure the entry is valid.
        if (currentEntry.DllBase)
        {
            // Execute the callback.
            if (!Callback(
                ProcessHandle,
                &currentEntry,
                addressOfEntry,
                Context1,
                Context2
                ))
                break;
        }

        currentLink = currentEntry.InLoadOrderLinks.Flink;
        i++;
    }

    return status;
}

BOOLEAN NTAPI PhpEnumProcessModules32Callback(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_DATA_TABLE_ENTRY32 Entry,
    _In_ ULONG AddressOfEntry,
    _In_ PVOID Context1,
    _In_opt_ PVOID Context2
    )
{
    static PH_STRINGREF system32String = PH_STRINGREF_INIT(L"\\System32\\");
    PPH_ENUM_PROCESS_MODULES_PARAMETERS parameters = Context1;
    BOOLEAN cont;
    LDR_DATA_TABLE_ENTRY nativeEntry;
    PPH_STRING mappedFileName;
    PWSTR baseDllNameBuffer = NULL;
    PWSTR fullDllNameBuffer = NULL;
    PH_STRINGREF fullDllName;
    PH_STRINGREF systemRootString;

    // Convert the 32-bit entry to a native-sized entry.

    memset(&nativeEntry, 0, sizeof(LDR_DATA_TABLE_ENTRY));
    nativeEntry.DllBase = UlongToPtr(Entry->DllBase);
    nativeEntry.EntryPoint = UlongToPtr(Entry->EntryPoint);
    nativeEntry.SizeOfImage = Entry->SizeOfImage;
    UStr32ToUStr(&nativeEntry.FullDllName, &Entry->FullDllName);
    UStr32ToUStr(&nativeEntry.BaseDllName, &Entry->BaseDllName);
    nativeEntry.Flags = Entry->Flags;
    nativeEntry.ObsoleteLoadCount = Entry->ObsoleteLoadCount;
    nativeEntry.TlsIndex = Entry->TlsIndex;
    nativeEntry.TimeDateStamp = Entry->TimeDateStamp;
    nativeEntry.OriginalBase = Entry->OriginalBase;
    nativeEntry.LoadTime = Entry->LoadTime;
    nativeEntry.BaseNameHashValue = Entry->BaseNameHashValue;
    nativeEntry.LoadReason = Entry->LoadReason;
    nativeEntry.ParentDllBase = UlongToPtr(Entry->ParentDllBase);

    mappedFileName = NULL;

    if (parameters->Flags & PH_ENUM_PROCESS_MODULES_TRY_MAPPED_FILE_NAME)
    {
        PhGetProcessMappedFileName(ProcessHandle, nativeEntry.DllBase, &mappedFileName);
    }

    if (mappedFileName)
    {
        ULONG_PTR indexOfLastBackslash;

        PhStringRefToUnicodeString(&mappedFileName->sr, &nativeEntry.FullDllName);
        indexOfLastBackslash = PhFindLastCharInString(mappedFileName, 0, OBJ_NAME_PATH_SEPARATOR);

        if (indexOfLastBackslash != SIZE_MAX)
        {
            nativeEntry.BaseDllName.Buffer = PTR_ADD_OFFSET(nativeEntry.FullDllName.Buffer, PTR_ADD_OFFSET(indexOfLastBackslash * sizeof(WCHAR), sizeof(WCHAR)));
            nativeEntry.BaseDllName.Length = nativeEntry.FullDllName.Length - (USHORT)indexOfLastBackslash * sizeof(WCHAR) - sizeof(WCHAR);
            nativeEntry.BaseDllName.MaximumLength = nativeEntry.BaseDllName.Length;
        }
        else
        {
            nativeEntry.BaseDllName = nativeEntry.FullDllName;
        }
    }
    else
    {
        // Read the base DLL name string and add a null terminator.

        baseDllNameBuffer = PhAllocate(nativeEntry.BaseDllName.Length + sizeof(UNICODE_NULL));

        if (NT_SUCCESS(NtReadVirtualMemory(
            ProcessHandle,
            nativeEntry.BaseDllName.Buffer,
            baseDllNameBuffer,
            nativeEntry.BaseDllName.Length,
            NULL
            )))
        {
            baseDllNameBuffer[nativeEntry.BaseDllName.Length / sizeof(WCHAR)] = UNICODE_NULL;
        }
        else
        {
            baseDllNameBuffer[0] = UNICODE_NULL;
            nativeEntry.BaseDllName.Length = 0;
        }

        nativeEntry.BaseDllName.Buffer = baseDllNameBuffer;

        // Read the full DLL name string and add a null terminator.

        fullDllNameBuffer = PhAllocate(nativeEntry.FullDllName.Length + sizeof(UNICODE_NULL));

        if (NT_SUCCESS(NtReadVirtualMemory(
            ProcessHandle,
            nativeEntry.FullDllName.Buffer,
            fullDllNameBuffer,
            nativeEntry.FullDllName.Length,
            NULL
            )))
        {
            fullDllNameBuffer[nativeEntry.FullDllName.Length / sizeof(WCHAR)] = UNICODE_NULL;

            if (!(parameters->Flags & PH_ENUM_PROCESS_MODULES_DONT_RESOLVE_WOW64_FS))
            {
                // WOW64 file system redirection - convert "system32" to "SysWOW64" or "SysArm32".
                if (!(nativeEntry.FullDllName.Length & 1)) // validate the string length
                {
#ifdef _M_ARM64
                    USHORT arch;
                    if (NT_SUCCESS(PhGetProcessArchitecture(ProcessHandle, &arch)))
                    {
#endif
                        fullDllName.Buffer = fullDllNameBuffer;
                        fullDllName.Length = nativeEntry.FullDllName.Length;

                        PhGetSystemRoot(&systemRootString);

                        if (PhStartsWithStringRef(&fullDllName, &systemRootString, TRUE))
                        {
                            PhSkipStringRef(&fullDllName, systemRootString.Length);

                            if (PhStartsWithStringRef(&fullDllName, &system32String, TRUE))
                            {
#ifdef _M_ARM64
                                if (arch == IMAGE_FILE_MACHINE_ARMNT)
                                {
                                    fullDllName.Buffer[1] = L'S';
                                    fullDllName.Buffer[2] = L'y';
                                    fullDllName.Buffer[3] = L's';
                                    fullDllName.Buffer[4] = L'A';
                                    fullDllName.Buffer[5] = L'r';
                                    fullDllName.Buffer[6] = L'm';
                                    fullDllName.Buffer[7] = L'3';
                                    fullDllName.Buffer[8] = L'2';
                                }
                                else
#endif
                                {
                                    fullDllName.Buffer[1] = L'S';
                                    fullDllName.Buffer[4] = L'W';
                                    fullDllName.Buffer[5] = L'O';
                                    fullDllName.Buffer[6] = L'W';
                                    fullDllName.Buffer[7] = L'6';
                                    fullDllName.Buffer[8] = L'4';
                                }
                            }
                        }
#ifdef _M_ARM64
                    }
                    else
                    {
                        fullDllNameBuffer[0] = UNICODE_NULL;
                        nativeEntry.FullDllName.Length = 0;
                    }
#endif
                }
            }
        }
        else
        {
            fullDllNameBuffer[0] = UNICODE_NULL;
            nativeEntry.FullDllName.Length = 0;
        }

        nativeEntry.FullDllName.Buffer = fullDllNameBuffer;
    }

    if (WindowsVersion >= WINDOWS_8 && Entry->DdagNode)
    {
        LDR_DDAG_NODE32 ldrDagNode32 = { 0 };

        if (NT_SUCCESS(NtReadVirtualMemory(
            ProcessHandle,
            UlongToPtr(Entry->DdagNode),
            &ldrDagNode32,
            sizeof(LDR_DDAG_NODE32),
            NULL
            )))
        {
            // Fixup the module load count. (dmex)
            nativeEntry.ObsoleteLoadCount = (USHORT)ldrDagNode32.LoadCount;
        }
    }

    // Execute the callback.
    cont = parameters->Callback(&nativeEntry, parameters->Context);

    if (mappedFileName)
    {
        PhDereferenceObject(mappedFileName);
    }
    else
    {
        if (baseDllNameBuffer)
            PhFree(baseDllNameBuffer);
        if (fullDllNameBuffer)
            PhFree(fullDllNameBuffer);
    }

    return cont;
}

/**
 * Enumerates the 32-bit modules loaded by a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param Callback A callback function which is executed for each process module.
 * \param Context A user-defined value to pass to the callback function.
 *
 * \retval STATUS_NOT_SUPPORTED The process is not running under WOW64.
 *
 * \remarks Do not use this function under a 32-bit environment.
 */
NTSTATUS PhEnumProcessModules32(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PH_ENUM_PROCESS_MODULES_PARAMETERS parameters;

    parameters.Callback = Callback;
    parameters.Context = Context;
    parameters.Flags = 0;

    return PhEnumProcessModules32Ex(ProcessHandle, &parameters);
}

/**
 * Enumerates the 32-bit modules loaded by a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access. If
 * \c PH_ENUM_PROCESS_MODULES_TRY_MAPPED_FILE_NAME is specified in \a Parameters, the handle should
 * have PROCESS_QUERY_INFORMATION access.
 * \param Parameters The enumeration parameters.
 *
 * \retval STATUS_NOT_SUPPORTED The process is not running under WOW64.
 *
 * \remarks Do not use this function under a 32-bit environment.
 */
NTSTATUS PhEnumProcessModules32Ex(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_PARAMETERS Parameters
    )
{
    return PhpEnumProcessModules32(
        ProcessHandle,
        PhpEnumProcessModules32Callback,
        Parameters,
        NULL
        );
}

BOOLEAN NTAPI PhpSetProcessModuleLoadCount32Callback(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_DATA_TABLE_ENTRY32 Entry,
    _In_ ULONG AddressOfEntry,
    _In_ PVOID Context1,
    _In_opt_ PVOID Context2
    )
{
    PSET_PROCESS_MODULE_LOAD_COUNT_CONTEXT context = Context1;

    if (UlongToPtr(Entry->DllBase) == context->BaseAddress)
    {
        context->Status = NtWriteVirtualMemory(
            ProcessHandle,
            UlongToPtr(AddressOfEntry + FIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, ObsoleteLoadCount)),
            &context->LoadCount,
            sizeof(USHORT),
            NULL
            );

        return FALSE;
    }

    return TRUE;
}

/**
 * Sets the load count of a 32-bit process module.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_VM_READ and PROCESS_VM_WRITE access.
 * \param BaseAddress The base address of a module.
 * \param LoadCount The new load count of the module.
 *
 * \retval STATUS_DLL_NOT_FOUND The module was not found.
 * \retval STATUS_NOT_SUPPORTED The process is not running under WOW64.
 *
 * \remarks Do not use this function under a 32-bit environment.
 */
NTSTATUS PhSetProcessModuleLoadCount32(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ ULONG LoadCount
    )
{
    NTSTATUS status;
    SET_PROCESS_MODULE_LOAD_COUNT_CONTEXT context;

    context.Status = STATUS_DLL_NOT_FOUND;
    context.BaseAddress = BaseAddress;
    context.LoadCount = LoadCount;

    status = PhpEnumProcessModules32(
        ProcessHandle,
        PhpSetProcessModuleLoadCount32Callback,
        &context,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    return context.Status;
}

typedef struct _ENUM_GENERIC_PROCESS_MODULES_CONTEXT
{
    PPH_HASHTABLE BaseAddressHashtable;
    PPH_ENUM_GENERIC_MODULES_CALLBACK Callback;
    PVOID Context;
    ULONG Type;
    ULONG LoadOrderIndex;
} ENUM_GENERIC_PROCESS_MODULES_CONTEXT, *PENUM_GENERIC_PROCESS_MODULES_CONTEXT;

ULONG PhGetRtlModuleType(
    _In_ PVOID ImageBase
    )
{
    if ((ULONG_PTR)ImageBase > (ULONG_PTR)PhSystemBasicInformation.MaximumUserModeAddress)
        return PH_MODULE_TYPE_KERNEL_MODULE;

    return PH_MODULE_TYPE_MODULE;
}

PVOID PhGetRtlModuleBase(
    _In_ PVOID DllBase,
    _In_ ULONG Value
    )
{
    // Note: 24H2 and above return zero for Dllbase resulting in hash collisions.
    // Create a pseudo Dllbase based on the index with an invalid memory region
    // greater than MaximumUserModeAddress or less than 0x1000. (dmex)

    if (WindowsVersion >= WINDOWS_11_24H2 && !DllBase)
    {
        return PTR_SUB_OFFSET(ULONG64_MAX, Value);
    }

    return DllBase;
}

static BOOLEAN EnumGenericProcessModulesCallback(
    _In_ PLDR_DATA_TABLE_ENTRY Module,
    _In_ PENUM_GENERIC_PROCESS_MODULES_CONTEXT Context
    )
{
    PH_MODULE_INFO moduleInfo;
    PVOID baseAddress;
    BOOLEAN result;

    // Get the current module base address.
    baseAddress = PhGetRtlModuleBase(Module->DllBase, Context->LoadOrderIndex);

    // Check if we have a duplicate base address.
    if (PhFindEntryHashtable(Context->BaseAddressHashtable, &baseAddress))
        return TRUE;

    PhAddEntryHashtable(Context->BaseAddressHashtable, &baseAddress);

    RtlZeroMemory(&moduleInfo, sizeof(PH_MODULE_INFO));
    moduleInfo.Type = Context->Type;
    moduleInfo.BaseAddress = baseAddress;
    moduleInfo.Size = Module->SizeOfImage;
    moduleInfo.EntryPoint = Module->EntryPoint;
    moduleInfo.Flags = Module->Flags;
    moduleInfo.LoadOrderIndex = (USHORT)(Context->LoadOrderIndex++);
    moduleInfo.LoadCount = Module->ObsoleteLoadCount;
    moduleInfo.Name = PhCreateStringFromUnicodeString(&Module->BaseDllName);
    moduleInfo.FileName = PhCreateStringFromUnicodeString(&Module->FullDllName);

    if (WindowsVersion >= WINDOWS_8)
    {
        moduleInfo.ParentBaseAddress = Module->ParentDllBase;
        moduleInfo.OriginalBaseAddress = (PVOID)Module->OriginalBase;
        moduleInfo.LoadReason = (USHORT)Module->LoadReason;
        moduleInfo.LoadTime = Module->LoadTime;
    }
    else
    {
        moduleInfo.ParentBaseAddress = NULL;
        moduleInfo.OriginalBaseAddress = NULL;
        moduleInfo.LoadReason = USHRT_MAX;
        moduleInfo.LoadTime.QuadPart = 0;
    }

    result = Context->Callback(&moduleInfo, Context->Context);

    PhDereferenceObject(moduleInfo.Name);
    PhDereferenceObject(moduleInfo.FileName);

    return result;
}

VOID PhpRtlModulesToGenericModules(
    _In_ PRTL_PROCESS_MODULES Modules,
    _In_ PPH_ENUM_GENERIC_MODULES_CALLBACK Callback,
    _In_ PPH_HASHTABLE BaseAddressHashtable,
    _In_opt_ PVOID Context
    )
{
    PRTL_PROCESS_MODULE_INFORMATION module;
    PVOID baseAddress;
    PH_MODULE_INFO moduleInfo;
    BOOLEAN result;

    for (ULONG i = 0; i < Modules->NumberOfModules; i++)
    {
        module = &Modules->Modules[i];

        // Get the current module base address.
        baseAddress = PhGetRtlModuleBase(module->ImageBase, i);

        // Check if we have a duplicate base address.
        if (PhFindEntryHashtable(BaseAddressHashtable, &baseAddress))
            continue;

        PhAddEntryHashtable(BaseAddressHashtable, &baseAddress);

        RtlZeroMemory(&moduleInfo, sizeof(PH_MODULE_INFO));
        moduleInfo.Type = PhGetRtlModuleType(baseAddress);
        moduleInfo.BaseAddress = baseAddress;
        moduleInfo.Size = module->ImageSize;
        moduleInfo.EntryPoint = NULL;
        moduleInfo.Flags = module->Flags;
        moduleInfo.LoadOrderIndex = module->LoadOrderIndex;
        moduleInfo.LoadCount = module->LoadCount;
        moduleInfo.LoadReason = USHRT_MAX;
        moduleInfo.LoadTime.QuadPart = 0;
        moduleInfo.ParentBaseAddress = NULL;
        moduleInfo.OriginalBaseAddress = NULL;

        if (module->OffsetToFileName == 0)
        {
            static PH_STRINGREF driversString = PH_STRINGREF_INIT(L"\\System32\\Drivers\\");
            PH_STRINGREF systemRoot;

            // We only have the file name, without a path. The driver must be in the default drivers
            // directory.
            PhGetNtSystemRoot(&systemRoot);
            moduleInfo.Name = PhConvertUtf8ToUtf16(module->FullPathName);
            moduleInfo.FileName = PhConcatStringRef3(&systemRoot, &driversString, &moduleInfo.Name->sr);
        }
        else
        {
            moduleInfo.Name = PhConvertUtf8ToUtf16(&module->FullPathName[module->OffsetToFileName]);
            moduleInfo.FileName = PhConvertUtf8ToUtf16(module->FullPathName);
        }

        result = Callback(&moduleInfo, Context);

        PhDereferenceObject(moduleInfo.Name);
        PhDereferenceObject(moduleInfo.FileName);

        if (!result)
            break;
    }
}

VOID PhpRtlModulesExToGenericModules(
    _In_ PRTL_PROCESS_MODULE_INFORMATION_EX Modules,
    _In_ PPH_ENUM_GENERIC_MODULES_CALLBACK Callback,
    _In_ PPH_HASHTABLE BaseAddressHashtable,
    _In_opt_ PVOID Context
    )
{
    PRTL_PROCESS_MODULE_INFORMATION_EX module;
    PH_MODULE_INFO moduleInfo;
    PVOID baseAddress;
    BOOLEAN result;

    for (module = Modules; module->NextOffset; module = PTR_ADD_OFFSET(module, module->NextOffset))
    {
        // Get the current module base address.
        baseAddress = PhGetRtlModuleBase(module->ImageBase, module->LoadOrderIndex);

        // Check if we have a duplicate base address.
        if (PhFindEntryHashtable(BaseAddressHashtable, &baseAddress))
            continue;

        PhAddEntryHashtable(BaseAddressHashtable, &baseAddress);
        
        RtlZeroMemory(&moduleInfo, sizeof(PH_MODULE_INFO));
        moduleInfo.Type = PhGetRtlModuleType(baseAddress);
        moduleInfo.BaseAddress = baseAddress;
        moduleInfo.Size = module->ImageSize;
        moduleInfo.EntryPoint = NULL;
        moduleInfo.Flags = module->Flags;
        moduleInfo.LoadOrderIndex = module->LoadOrderIndex;
        moduleInfo.LoadCount = module->LoadCount;
        moduleInfo.LoadReason = USHRT_MAX;
        moduleInfo.LoadTime.QuadPart = 0;
        moduleInfo.ParentBaseAddress = NULL;
        moduleInfo.OriginalBaseAddress = NULL;

        if (module->OffsetToFileName == 0)
        {
            static PH_STRINGREF driversString = PH_STRINGREF_INIT(L"\\System32\\Drivers\\");
            PH_STRINGREF systemRoot;

            // We only have the file name, without a path. The driver must be in the default drivers
            // directory.
            PhGetNtSystemRoot(&systemRoot);
            moduleInfo.Name = PhConvertUtf8ToUtf16((PSTR)module->FullPathName);
            moduleInfo.FileName = PhConcatStringRef3(&systemRoot, &driversString, &moduleInfo.Name->sr);
        }
        else
        {
            moduleInfo.Name = PhConvertUtf8ToUtf16(&module->FullPathName[module->OffsetToFileName]);
            moduleInfo.FileName = PhConvertUtf8ToUtf16((PSTR)module->FullPathName);
        }

        result = Callback(&moduleInfo, Context);

        PhDereferenceObject(moduleInfo.Name);
        PhDereferenceObject(moduleInfo.FileName);

        if (!result)
            break;
    }
}

BOOLEAN PhpCallbackMappedFileOrImage(
    _In_ PVOID AllocationBase,
    _In_ SIZE_T AllocationSize,
    _In_ ULONG Type,
    _In_ PPH_STRING FileName,
    _In_ PPH_ENUM_GENERIC_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PH_MODULE_INFO moduleInfo;
    BOOLEAN result;

    RtlZeroMemory(&moduleInfo, sizeof(PH_MODULE_INFO));
    moduleInfo.Type = Type;
    moduleInfo.BaseAddress = AllocationBase;
    moduleInfo.Size = (ULONG)AllocationSize;
    moduleInfo.EntryPoint = NULL;
    moduleInfo.Flags = 0;
    moduleInfo.FileName = FileName;
    moduleInfo.Name = PhGetBaseName(moduleInfo.FileName);
    moduleInfo.LoadOrderIndex = USHRT_MAX;
    moduleInfo.LoadCount = USHRT_MAX;
    moduleInfo.LoadReason = USHRT_MAX;
    moduleInfo.LoadTime.QuadPart = 0;
    moduleInfo.ParentBaseAddress = NULL;
    moduleInfo.OriginalBaseAddress = NULL;

    result = Callback(&moduleInfo, Context);

    PhDereferenceObject(moduleInfo.FileName);
    PhDereferenceObject(moduleInfo.Name);

    return result;
}

typedef struct _PH_ENUM_MAPPED_MODULES_PARAMETERS
{
    PPH_ENUM_GENERIC_MODULES_CALLBACK Callback;
    PVOID Context;
    BOOLEAN TrackingAllocationBase;
    PVOID LastAllocationBase;
    SIZE_T AllocationSize;
    PPH_HASHTABLE BaseAddressHashtable;
} PH_ENUM_MAPPED_MODULES_PARAMETERS, *PPH_ENUM_MAPPED_MODULES_PARAMETERS;

NTSTATUS NTAPI PhpEnumGenericMappedFilesAndImagesBulk(
    _In_ HANDLE ProcessHandle,
    _In_ PMEMORY_BASIC_INFORMATION MemoryBasicInfo,
    _In_ SIZE_T Count,
    _In_ PPH_ENUM_MAPPED_MODULES_PARAMETERS Parameters
    )
{
    ULONG type;
    PPH_STRING fileName;

    for (SIZE_T i = 0; i < Count; i++)
    {
        PMEMORY_BASIC_INFORMATION basicInfo = &MemoryBasicInfo[i];

        if (
            basicInfo->Type == MEM_MAPPED ||
            basicInfo->Type == MEM_IMAGE ||
            basicInfo->Type == SEC_PROTECTED_IMAGE ||
            basicInfo->Type == (MEM_MAPPED | MEM_IMAGE | SEC_PROTECTED_IMAGE))
        {
            if (basicInfo->Type == MEM_MAPPED)
                type = PH_MODULE_TYPE_MAPPED_FILE;
            else
                type = PH_MODULE_TYPE_MAPPED_IMAGE;

            if (Parameters->LastAllocationBase == basicInfo->AllocationBase)
            {
                Parameters->TrackingAllocationBase = TRUE;
                Parameters->AllocationSize += basicInfo->RegionSize;
            }
            else
            {
                if (Parameters->TrackingAllocationBase)
                {
                    Parameters->TrackingAllocationBase = FALSE;

                    if (!PhFindEntryHashtable(Parameters->BaseAddressHashtable, &Parameters->LastAllocationBase))
                    {
                        PhAddEntryHashtable(Parameters->BaseAddressHashtable, &Parameters->LastAllocationBase);

                        if (NT_SUCCESS(PhGetProcessMappedFileName(ProcessHandle, Parameters->LastAllocationBase, &fileName)))
                        {
                            if (!PhpCallbackMappedFileOrImage(
                                Parameters->LastAllocationBase,
                                Parameters->AllocationSize,
                                type,
                                fileName,
                                Parameters->Callback,
                                Parameters->Context
                                ))
                            {
                                break;
                            }
                        }
                    }
                }

                Parameters->AllocationSize = basicInfo->RegionSize;
                Parameters->LastAllocationBase = basicInfo->AllocationBase;
            }
        }
    }

    return STATUS_SUCCESS;
}

VOID PhpEnumGenericMappedFilesAndImages(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Flags,
    _In_ PPH_ENUM_GENERIC_MODULES_CALLBACK Callback,
    _In_ PPH_HASHTABLE BaseAddressHashtable,
    _In_opt_ PVOID Context
    )
{
    BOOLEAN querySucceeded;
    PVOID baseAddress;
    MEMORY_BASIC_INFORMATION basicInfo;
    PH_ENUM_MAPPED_MODULES_PARAMETERS enumParameters;

    memset(&enumParameters, 0, sizeof(PH_ENUM_MAPPED_MODULES_PARAMETERS));
    enumParameters.BaseAddressHashtable = BaseAddressHashtable;
    enumParameters.Callback = Callback;
    enumParameters.Context = Context;

    baseAddress = (PVOID)0;

    if (NT_SUCCESS(PhEnumVirtualMemoryBulk(
        ProcessHandle,
        baseAddress,
        FALSE,
        PhpEnumGenericMappedFilesAndImagesBulk,
        &enumParameters
        )))
    {
        return;
    }

    if (!NT_SUCCESS(NtQueryVirtualMemory(
        ProcessHandle,
        baseAddress,
        MemoryBasicInformation,
        &basicInfo,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        )))
    {
        return;
    }

    querySucceeded = TRUE;

    while (querySucceeded)
    {
        PVOID allocationBase;
        SIZE_T allocationSize;
        ULONG type;
        PPH_STRING fileName;
        BOOLEAN result;

        if (basicInfo.Type == MEM_MAPPED || basicInfo.Type == MEM_IMAGE)
        {
            if (basicInfo.Type == MEM_MAPPED)
                type = PH_MODULE_TYPE_MAPPED_FILE;
            else
                type = PH_MODULE_TYPE_MAPPED_IMAGE;

            // Find the total allocation size.

            allocationBase = basicInfo.AllocationBase;
            allocationSize = 0;

            do
            {
                baseAddress = PTR_ADD_OFFSET(baseAddress, basicInfo.RegionSize);
                allocationSize += basicInfo.RegionSize;

                if (!NT_SUCCESS(NtQueryVirtualMemory(
                    ProcessHandle,
                    baseAddress,
                    MemoryBasicInformation,
                    &basicInfo,
                    sizeof(MEMORY_BASIC_INFORMATION),
                    NULL
                    )))
                {
                    querySucceeded = FALSE;
                    break;
                }
            } while (basicInfo.AllocationBase == allocationBase);

            if ((type == PH_MODULE_TYPE_MAPPED_FILE && !(Flags & PH_ENUM_GENERIC_MAPPED_FILES)) ||
                (type == PH_MODULE_TYPE_MAPPED_IMAGE && !(Flags & PH_ENUM_GENERIC_MAPPED_IMAGES)))
            {
                // The user doesn't want this type of entry.
                continue;
            }

            // Check if we have a duplicate base address.
            if (PhFindEntryHashtable(BaseAddressHashtable, &allocationBase))
            {
                continue;
            }

            if (!NT_SUCCESS(PhGetProcessMappedFileName(
                ProcessHandle,
                allocationBase,
                &fileName
                )))
            {
                continue;
            }

            PhAddEntryHashtable(BaseAddressHashtable, &allocationBase);

            result = PhpCallbackMappedFileOrImage(
                allocationBase,
                allocationSize,
                type,
                fileName,
                Callback,
                Context
                );

            if (!result)
                break;
        }
        else
        {
            baseAddress = PTR_ADD_OFFSET(baseAddress, basicInfo.RegionSize);

            if (!NT_SUCCESS(NtQueryVirtualMemory(
                ProcessHandle,
                baseAddress,
                MemoryBasicInformation,
                &basicInfo,
                sizeof(MEMORY_BASIC_INFORMATION),
                NULL
                )))
            {
                querySucceeded = FALSE;
            }
        }
    }
}

static BOOLEAN NTAPI PhpBaseAddressHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    return *(PVOID *)Entry1 == *(PVOID *)Entry2;
}

static ULONG NTAPI PhpBaseAddressHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashIntPtr((ULONG_PTR)*(PVOID *)Entry);
}

/**
 * Enumerates the modules loaded by a process.
 *
 * \param ProcessId The ID of a process. If \ref SYSTEM_PROCESS_ID is specified the function
 * enumerates the kernel modules.
 * \param ProcessHandle A handle to the process.
 * \param Flags Flags controlling the information to retrieve.
 * \li \c PH_ENUM_GENERIC_MAPPED_FILES Enumerate mapped files.
 * \li \c PH_ENUM_GENERIC_MAPPED_IMAGES Enumerate mapped images (those which are not mapped by the
 * loader).
 * \param Callback A callback function which is executed for each module.
 * \param Context A user-defined value to pass to the callback function.
 */
NTSTATUS PhEnumGenericModules(
    _In_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _In_ ULONG Flags,
    _In_ PPH_ENUM_GENERIC_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PPH_HASHTABLE baseAddressHashtable;

    baseAddressHashtable = PhCreateHashtable(
        sizeof(PVOID),
        PhpBaseAddressHashtableEqualFunction,
        PhpBaseAddressHashtableHashFunction,
        100
        );

    if (ProcessId == SYSTEM_PROCESS_ID)
    {
        // Kernel modules

        PVOID modules;

        status = PhEnumKernelModulesEx((PRTL_PROCESS_MODULE_INFORMATION_EX*)&modules);

        if (NT_SUCCESS(status))
        {
            PhpRtlModulesExToGenericModules(
                modules,
                Callback,
                baseAddressHashtable,
                Context
                );
            PhFree(modules);
        }
        else
        {
            status = PhEnumKernelModules((PRTL_PROCESS_MODULES*)&modules);

            if (NT_SUCCESS(status))
            {
                PhpRtlModulesToGenericModules(
                    modules,
                    Callback,
                    baseAddressHashtable,
                    Context
                    );
                PhFree(modules);
            }
        }
    }
    else
    {
        // Process modules

        BOOLEAN opened = FALSE;
#ifdef _WIN64
        BOOLEAN isWow64 = FALSE;
#endif
        ENUM_GENERIC_PROCESS_MODULES_CONTEXT context;
        PH_ENUM_PROCESS_MODULES_PARAMETERS parameters;

        if (!ProcessHandle)
        {
            if (!NT_SUCCESS(status = PhOpenProcess(
                &ProcessHandle,
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, // needed for enumerating mapped files
                ProcessId
                )))
            {
                if (!NT_SUCCESS(status = PhOpenProcess(
                    &ProcessHandle,
                    PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                    ProcessId
                    )))
                {
                    goto CleanupExit;
                }
            }

            opened = TRUE;
        }

        context.Callback = Callback;
        context.Context = Context;
        context.Type = PH_MODULE_TYPE_MODULE;
        context.BaseAddressHashtable = baseAddressHashtable;
        context.LoadOrderIndex = 0;

        parameters.Callback = EnumGenericProcessModulesCallback;
        parameters.Context = &context;
        parameters.Flags = PH_ENUM_PROCESS_MODULES_TRY_MAPPED_FILE_NAME;

        status = PhEnumProcessModulesEx(
            ProcessHandle,
            &parameters
            );

#ifdef _WIN64
        PhGetProcessIsWow64(ProcessHandle, &isWow64);

        // 32-bit process modules
        if (isWow64)
        {
            context.Callback = Callback;
            context.Context = Context;
            context.Type = PH_MODULE_TYPE_WOW64_MODULE;
            context.BaseAddressHashtable = baseAddressHashtable;
            context.LoadOrderIndex = 0;

            status = PhEnumProcessModules32Ex(
                ProcessHandle,
                &parameters
                );
        }
#endif

        // Mapped files and mapped images
        // This is done last because it provides the least amount of information.

        if (Flags & (PH_ENUM_GENERIC_MAPPED_FILES | PH_ENUM_GENERIC_MAPPED_IMAGES))
        {
            PhpEnumGenericMappedFilesAndImages(
                ProcessHandle,
                Flags,
                Callback,
                baseAddressHashtable,
                Context
                );
        }

        if (opened)
            NtClose(ProcessHandle);
    }

CleanupExit:
    PhDereferenceObject(baseAddressHashtable);

    return status;
}

typedef struct _PH_ENUM_PROCESS_MODULES_LIMITED_PARAMETERS
{
    PPH_ENUM_PROCESS_MODULES_LIMITED_CALLBACK Callback;
    PPH_HASHTABLE BaseAddressHashtable;
    PVOID Context;
} PH_ENUM_PROCESS_MODULES_LIMITED_PARAMETERS, *PPH_ENUM_PROCESS_MODULES_LIMITED_PARAMETERS;

NTSTATUS NTAPI PhEnumProcessModulesLimitedCallback(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG_PTR NumberOfEntries,
    _In_ PMEMORY_WORKING_SET_BLOCK WorkingSetBlock,
    _In_ PPH_ENUM_PROCESS_MODULES_LIMITED_PARAMETERS Parameters
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    MEMORY_IMAGE_INFORMATION imageInformation;
    PVOID baseAddress = NULL;
    PPH_STRING fileName;

    for (ULONG_PTR i = 0; i < NumberOfEntries; i++)
    {
        PMEMORY_WORKING_SET_BLOCK workingSetBlock = &WorkingSetBlock[i];
        PVOID virtualAddress = (PVOID)(workingSetBlock->VirtualPage << PAGE_SHIFT);

        if (virtualAddress < baseAddress)
            continue;

        status = PhGetProcessMappedImageInformation(
            ProcessHandle,
            virtualAddress,
            &imageInformation
            );

        if (
            !NT_SUCCESS(status) ||
            !imageInformation.ImageBase ||
            imageInformation.ImageNotExecutable ||
            imageInformation.ImagePartialMap
            )
        {
            continue;
        }

        if (PhFindEntryHashtable(Parameters->BaseAddressHashtable, &imageInformation.ImageBase))
            continue;

        PhAddEntryHashtable(Parameters->BaseAddressHashtable, &imageInformation.ImageBase);

        status = PhGetProcessMappedFileName(
            ProcessHandle,
            imageInformation.ImageBase,
            &fileName
            );

        if (!NT_SUCCESS(status))
            continue;

        status = Parameters->Callback(
            ProcessHandle,
            virtualAddress,
            imageInformation.ImageBase,
            imageInformation.SizeOfImage,
            fileName,
            Parameters->Context
            );

        PhDereferenceObject(fileName);

        if (!NT_SUCCESS(status))
            break;

        baseAddress = PTR_ADD_OFFSET(imageInformation.ImageBase, imageInformation.SizeOfImage);
    }

    if (status == STATUS_NO_MORE_ENTRIES)
    {
        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS PhEnumProcessModulesLimited(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_PROCESS_MODULES_LIMITED_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PH_ENUM_PROCESS_MODULES_LIMITED_PARAMETERS limitedParameters;
    PPH_HASHTABLE baseAddressHashtable;

    baseAddressHashtable = PhCreateHashtable(
        sizeof(PVOID),
        PhpBaseAddressHashtableEqualFunction,
        PhpBaseAddressHashtableHashFunction,
        100
        );

    memset(&limitedParameters, 0, sizeof(PH_ENUM_PROCESS_MODULES_LIMITED_PARAMETERS));
    limitedParameters.BaseAddressHashtable = baseAddressHashtable;
    limitedParameters.Callback = Callback;
    limitedParameters.Context = Context;

    status = PhEnumVirtualMemoryPages(
        ProcessHandle,
        PhEnumProcessModulesLimitedCallback,
        &limitedParameters
        );

    //if (!NT_SUCCESS(status))
    //{
    //    PH_ENUM_MAPPED_MODULES_PARAMETERS mappedParameters;
    //
    //    memset(&mappedParameters, 0, sizeof(PH_ENUM_MAPPED_MODULES_PARAMETERS));
    //    mappedParameters.BaseAddressHashtable = baseAddressHashtable;
    //    mappedParameters.Callback = Callback;
    //    mappedParameters.Context = Context;
    //
    //    status = PhEnumVirtualMemoryBulk(
    //        ProcessHandle,
    //        nullptr,
    //        FALSE,
    //        PhpEnumGenericMappedFilesAndImagesBulk,
    //        &mappedParameters
    //        );
    //}

    PhDereferenceObject(baseAddressHashtable);

    return status;
}

NTSTATUS PhEnumProcessEnclaveModules(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID EnclaveAddress,
    _In_ PLDR_SOFTWARE_ENCLAVE Enclave,
    _In_ PPH_ENUM_PROCESS_ENCLAVE_MODULES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PVOID listHead;
    LDR_DATA_TABLE_ENTRY entry;

    status = STATUS_SUCCESS;

    listHead = PTR_ADD_OFFSET(EnclaveAddress, FIELD_OFFSET(LDR_SOFTWARE_ENCLAVE, Modules));

    for (PLIST_ENTRY link = Enclave->Modules.Flink;
         link != listHead;
         link = entry.InLoadOrderLinks.Flink)
    {
        PVOID entryAddress;

        entryAddress = CONTAINING_RECORD(link, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        status = NtReadVirtualMemory(
            ProcessHandle,
            entryAddress,
            &entry,
            sizeof(entry),
            NULL
            );
        if (!NT_SUCCESS(status))
            return status;

        if (!Callback(ProcessHandle, Enclave, entryAddress, &entry, Context))
            break;
    }

    return status;
}

NTSTATUS PhGetProcessLdrTableEntryNames(
    _In_ HANDLE ProcessHandle,
    _In_ PLDR_DATA_TABLE_ENTRY Entry,
    _Out_ PPH_STRING* Name,
    _Out_ PPH_STRING* FileName
    )
{
    NTSTATUS status;
    PPH_STRING name;
    PPH_STRING fileName;
    PWSTR fullDllName;
    ULONG_PTR index;

    *Name = NULL;
    *FileName = NULL;

    name = NULL;
    fileName = NULL;
    fullDllName = NULL;

    if (Entry->DllBase)
    {
        PhGetProcessMappedFileName(ProcessHandle, Entry->DllBase, &fileName);
    }

    if (PhIsNullOrEmptyString(fileName))
    {
        fullDllName = PhAllocate(Entry->FullDllName.Length);

        status = NtReadVirtualMemory(
            ProcessHandle,
            Entry->FullDllName.Buffer,
            fullDllName,
            Entry->FullDllName.Length,
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        if (!(Entry->FullDllName.Length & 1)) // validate the string length
        {
            fileName = PhCreateStringEx(fullDllName, Entry->FullDllName.Length);
        }
        else
        {
            goto CleanupExit;
        }
    }

    index = PhFindLastCharInStringRef(
        &fileName->sr,
        OBJ_NAME_PATH_SEPARATOR,
        FALSE
        );

    if (index != SIZE_MAX)
    {
        name = PhCreateStringEx(
            &fileName->Buffer[index + 1],
            fileName->Length - (index * sizeof(WCHAR))
            );
    }
    else
    {
        name = PhReferenceObject(fileName);
    }

    *Name = name;
    *FileName = fileName;

    name = NULL;
    fileName = NULL;

CleanupExit:

    if (fullDllName)
        PhFree(fullDllName);

    PhClearReference(&name);
    PhClearReference(&fileName);

    return STATUS_SUCCESS;
}
