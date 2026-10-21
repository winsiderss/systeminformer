/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2019-2026
 *
 */

#include <phapp.h>
#include <mapimg.h>

#define NTOS_SERVICE_INDEX 0
#define WIN32K_SERVICE_INDEX 1

typedef union _NT_SYSTEMCALL_NUMBER
{
    USHORT SystemCallNumber;
    struct
    {
        USHORT SystemCallIndex : 12;
        USHORT SystemServiceIndex : 4;
    };
} NT_SYSTEMCALL_NUMBER, *PNT_SYSTEMCALL_NUMBER;

//
// System call dispatch value (the value loaded into EAX before the syscall
// instruction).
//
// The normal kernel (ntoskrnl!KiSystemCall64) only consumes bits 0-15: the
// service index (bits 0-11) and the service table selector (bits 12-15, where
// bit 12 chooses ntoskrnl vs win32k). Bits 16-31 are masked off and ignored.
//
// SecureKernelCall (bit 27, 0x08000000) is consumed by the secure kernel's own
// dispatcher (securekernel!KiSystemCall64): when set it selects
// SkiSecureServiceTable (a secure-kernel service), when clear it selects
// IumSyscallDispatchTable (a normal NT syscall proxied for a trustlet). The
// stubs that set it live in the IUM trustlet libraries shipped in System32
// (e.g. iumdll.dll: "mov eax, 0x0800000N; syscall") and execute in VTL1 user
// mode. The ntdll/win32u syscall numbers decoded by this file come from a
// normal VTL0 process and never set this bit, so it is always zero here.
//
typedef union _NT_SYSCALL_DISPATCH
{
    ULONG EAX;
    struct
    {
        USHORT SystemCallIndex : 12;    // Bits 0-11: service index within the selected table
        USHORT SystemServiceIndex : 4;  // Bits 12-15: service table selector (ntoskrnl vs win32k)
        USHORT Reserved : 11;           // Bits 16-26
        USHORT SecureKernelCall : 1;    // Bit 27: secure-kernel service selector (secure kernel only)
        USHORT Reserved2 : 4;           // Bits 28-31
    };
} NT_SYSCALL_DISPATCH, *PNT_SYSCALL_DISPATCH;

/**
 * Extracts the individual fields from a raw system call dispatch value.
 *
 * \param Dispatch The raw dispatch value (the contents of EAX at the syscall instruction).
 * \param ServiceIndex Optionally receives the service table selector (bits 12-15).
 * \param SecureKernelCall Optionally receives the secure-kernel service selector (bit 27).
 *
 * \return The service index within the selected table (bits 0-11).
 */
FORCEINLINE
USHORT
PhExtractSystemCallDispatch(
    _In_ ULONG Dispatch,
    _Out_opt_ PUSHORT ServiceIndex,
    _Out_opt_ PBOOLEAN SecureKernelCall
    )
{
    NT_SYSCALL_DISPATCH dispatch;

    dispatch.EAX = Dispatch;

    if (ServiceIndex)
        *ServiceIndex = dispatch.SystemServiceIndex;
    if (SecureKernelCall)
        *SecureKernelCall = !!dispatch.SecureKernelCall;

    return dispatch.SystemCallIndex;
}

//
// IUM secure-service names, indexed by the secure service index (bits 0-11 of
// the dispatch value when SecureKernelCall is set). This is the service surface
// exposed by the trustlet stub library iumdll.dll (one "mov eax, 0x0800000N;
// syscall" stub per export). It is used only as a fallback when iumdll.dll
// cannot be parsed at runtime (see PhpGenerateSecureServiceList); the table is
// build-specific and indices can change between Windows versions.
//
static CONST PWSTR PhpSecureServiceNames[] =
{
    L"IumAssignMemoryToSocDomain",         // 0
    L"IumAwaitSmc",                        // 1
    L"IumCreateSecureDevice",              // 2
    L"IumCreateSecureSection",             // 3
    L"IumCreateSecureSectionSpecifyPages", // 4
    L"IumCrypto",                          // 5
    L"IumDmaMapMemory",                    // 6
    L"IumEmitSmc",                         // 7
    L"IumFlushSecureSectionBuffers",       // 8
    L"IumGetDmaEnabler",                   // 9
    L"IumGetExposedSecureSection",         // 10
    L"IumGetIdk",                          // 11
    L"IumMapSecureIo",                     // 12
    L"IumOpenCurrentExtension",            // 13
    L"IumOpenSecureSection",               // 14
    L"IumPostMailbox",                     // 15
    L"IumProtectSecureIo",                 // 16
    L"IumQuerySecureDeviceInformation",    // 17
    L"IumSecureStorageGet",                // 18
    L"IumSecureStoragePut",                // 19
    L"IumSetDmaTargetProperties",          // 20
    L"IumSetPolicyExtension",              // 21
    L"IumUnmapSecureIo",                   // 22
    L"IumUpdateSecureDeviceState",         // 23
    L"IumEnclaveUsesAttestedKeys",         // 24
    L"IumCopyEnclaveMemory",               // 25
};

typedef struct _PH_SYSCALL_NAME_ENTRY
{
    ULONG_PTR Address;
    PPH_STRING Name;
} PH_SYSCALL_NAME_ENTRY, *PPH_SYSCALL_NAME_ENTRY;

static int __cdecl PhpSystemCallListIndexSort(
    _In_ void* Context,
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PPH_SYSCALL_NAME_ENTRY entry1 = *(PPH_SYSCALL_NAME_ENTRY*)elem1;
    PPH_SYSCALL_NAME_ENTRY entry2 = *(PPH_SYSCALL_NAME_ENTRY*)elem2;

    return uintptrcmp(entry1->Address, entry2->Address);
}

/**
 * Decodes the system call number from a win32u.dll export stub.
 *
 * \param MappedImage The mapped win32u.dll image.
 * \param ExportAddress The mapped address of the export.
 * \param SystemCallNumber A variable which receives the system call number.
 *
 * \return TRUE if the prologue matched a system call stub for the architecture
 * of the image, otherwise FALSE.
 *
 * \remarks The architecture is taken from the image rather than the host since the
 * file is opened by path and is not subject to WOW64 or emulation redirection. (dmex)
 */
static BOOLEAN PhpGetWin32uSystemCallNumber(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PVOID ExportAddress,
    _Out_ PNT_SYSTEMCALL_NUMBER SystemCallNumber
    )
{
    NT_SYSTEMCALL_NUMBER systemCallNumber = { 0 };
    BOOLEAN decoded = FALSE;

    __try
    {
        switch (MappedImage->NtHeaders->FileHeader.Machine)
        {
        case IMAGE_FILE_MACHINE_AMD64:
            {
                static CONST BYTE prologue[4] = { 0x4C, 0x8B, 0xD1, 0xB8 }; // mov r10, rcx ; mov eax, imm32

                PhMappedImageProbe(MappedImage, ExportAddress, sizeof(prologue) + sizeof(USHORT));

                if (RtlEqualMemory(ExportAddress, prologue, sizeof(prologue)))
                {
                    systemCallNumber.SystemCallNumber = *(USHORT UNALIGNED*)PTR_ADD_OFFSET(ExportAddress, sizeof(prologue));
                    decoded = TRUE;
                }
            }
            break;
        case IMAGE_FILE_MACHINE_I386:
            {
                static CONST BYTE prologue[1] = { 0xB8 }; // mov eax, imm32

                PhMappedImageProbe(MappedImage, ExportAddress, sizeof(prologue) + sizeof(USHORT));

                if (RtlEqualMemory(ExportAddress, prologue, sizeof(prologue)))
                {
                    systemCallNumber.SystemCallNumber = *(USHORT UNALIGNED*)PTR_ADD_OFFSET(ExportAddress, sizeof(prologue));
                    decoded = TRUE;
                }
            }
            break;
        case IMAGE_FILE_MACHINE_ARM64:
            {
                CONST ULONG UNALIGNED* instruction = ExportAddress;

                PhMappedImageProbe(MappedImage, ExportAddress, sizeof(ULONG) * 2);

                // mov w8, #imm16 (or mov x8, #imm16) ; svc #0
                // The MOVZ sf bit is masked since either width encodes the same immediate. (dmex)
                if ((instruction[0] & 0x7FE0001F) == 0x52800008 && instruction[1] == 0xD4000001)
                {
                    systemCallNumber.SystemCallNumber = (USHORT)((instruction[0] >> 5) & 0xFFFF);
                    decoded = TRUE;
                }
            }
            break;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        decoded = FALSE;
    }

    *SystemCallNumber = systemCallNumber;

    return decoded;
}

VOID PhGenerateSyscallLists(
    _Out_ PPH_LIST* NtdllSystemCallList,
    _Out_ PPH_LIST* Win32kSystemCallList
    )
{
    static CONST PH_STRINGREF ntdllFileName = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntdll.dll");
    static CONST PH_STRINGREF win32kFileName = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\win32k.sys");
    static CONST PH_STRINGREF win32uFileName = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\win32u.dll");
    PPH_LIST ntdllSystemCallList = NULL;
    PPH_LIST win32kSystemCallList = NULL;
    PH_MAPPED_IMAGE mappedImage;
    PH_MAPPED_IMAGE_EXPORTS exports;
    PH_MAPPED_IMAGE_EXPORT_ENTRY exportEntry;
    PH_MAPPED_IMAGE_EXPORT_FUNCTION exportFunction;

    if (NT_SUCCESS(PhLoadMappedImageEx(&ntdllFileName, NULL, &mappedImage)))
    {
        if (NT_SUCCESS(PhGetMappedImageExports(&exports, &mappedImage)))
        {
            PPH_LIST list = PhCreateList(exports.NumberOfEntries);

            for (ULONG i = 0; i < exports.NumberOfEntries; i++)
            {
                if (NT_SUCCESS(PhGetMappedImageExportEntry(&exports, i, &exportEntry)))
                {
                    if (!(exportEntry.Name && strncmp(exportEntry.Name, "Zw", 2) == 0))
                        continue;

                    if (NT_SUCCESS(PhGetMappedImageExportFunction(&exports, NULL, exportEntry.Ordinal, &exportFunction)))
                    {
                        PPH_SYSCALL_NAME_ENTRY entry;

                        entry = PhAllocate(sizeof(PH_SYSCALL_NAME_ENTRY));
                        entry->Address = (ULONG_PTR)exportFunction.Function;
                        entry->Name = PhZeroExtendToUtf16(exportEntry.Name);
                        entry->Name->Buffer[0] = L'N';
                        entry->Name->Buffer[1] = L't';

                        PhAddItemList(list, entry);
                    }
                }
            }

            qsort_s(list->Items, list->Count, sizeof(PVOID), PhpSystemCallListIndexSort, NULL);

            if (list->Count)
            {
                ntdllSystemCallList = PhCreateList(list->Count);

                for (ULONG i = 0; i < list->Count; i++)
                {
                    PPH_SYSCALL_NAME_ENTRY entry = list->Items[i];
                    PhAddItemList(ntdllSystemCallList, entry->Name);
                    PhFree(entry);
                }
            }

            PhDereferenceObject(list);
        }

        PhUnloadMappedImage(&mappedImage);
    }

    if (WindowsVersion >= WINDOWS_10_21H1)
    {
        if (NT_SUCCESS(PhLoadMappedImageEx(&win32kFileName, NULL, &mappedImage)))
        {
            if (NT_SUCCESS(PhGetMappedImageExports(&exports, &mappedImage)))
            {
                PPH_LIST list = PhCreateList(exports.NumberOfEntries);

                for (ULONG i = 0; i < exports.NumberOfEntries; i++)
                {
                    if (NT_SUCCESS(PhGetMappedImageExportEntry(&exports, i, &exportEntry)))
                    {
                        if (!(exportEntry.Name && strncmp(exportEntry.Name, "__win32kstub_", 13) == 0))
                            continue;

                        if (NT_SUCCESS(PhGetMappedImageExportFunction(&exports, NULL, exportEntry.Ordinal, &exportFunction)))
                        {
                            PPH_SYSCALL_NAME_ENTRY entry;

                            entry = PhAllocate(sizeof(PH_SYSCALL_NAME_ENTRY));
                            entry->Address = (ULONG_PTR)exportFunction.Function;
                            entry->Name = PhZeroExtendToUtf16(exportEntry.Name);
                            PhSkipStringRef(&entry->Name->sr, 13 * sizeof(WCHAR));

                            PhAddItemList(list, entry);
                        }
                    }
                }

                qsort_s(list->Items, list->Count, sizeof(PVOID), PhpSystemCallListIndexSort, NULL);

                if (list->Count)
                {
                    win32kSystemCallList = PhCreateList(list->Count);

                    for (ULONG i = 0; i < list->Count; i++)
                    {
                        PPH_SYSCALL_NAME_ENTRY entry = list->Items[i];
                        PhAddItemList(win32kSystemCallList, entry->Name);
                        PhFree(entry);
                    }
                }

                PhDereferenceObject(list);
            }

            PhUnloadMappedImage(&mappedImage);
        }
    }
    else
    {
        if (NT_SUCCESS(PhLoadMappedImageEx(&win32uFileName, NULL, &mappedImage)))
        {
            if (NT_SUCCESS(PhGetMappedImageExports(&exports, &mappedImage)))
            {
                PPH_LIST list = PhCreateList(exports.NumberOfEntries);

                for (ULONG i = 0; i < exports.NumberOfEntries; i++)
                {
                    if (NT_SUCCESS(PhGetMappedImageExportEntry(&exports, i, &exportEntry)))
                    {
                        if (!(exportEntry.Name && strncmp(exportEntry.Name, "Nt", 2) == 0))
                            continue;

                        if (NT_SUCCESS(PhGetMappedImageExportFunction(&exports, NULL, exportEntry.Ordinal, &exportFunction)))
                        {
                            PPH_SYSCALL_NAME_ENTRY entry;

                            entry = PhAllocate(sizeof(PH_SYSCALL_NAME_ENTRY));
                            entry->Address = (ULONG_PTR)exportFunction.Function;
                            entry->Name = PhZeroExtendToUtf16(exportEntry.Name);

                            // NOTE: When system calls are deleted from win32k.sys the win32u.dll interface gets
                            // replaced with stubs that call RaiseFailFastException. This breaks the sorting via
                            // RVA based on exports since these exports still exist so we have to check the prologue. (dmex)
                            if (WindowsVersion >= WINDOWS_10_20H1)
                            {
                                PBYTE exportAddress;
                                BOOLEAN added = FALSE;

                                if (NT_SUCCESS(PhMappedImageRvaToVa(&mappedImage, PtrToUlong(exportFunction.Function), (PVOID *)&exportAddress)))
                                {
                                    NT_SYSTEMCALL_NUMBER systemCallNumber;

                                    if (PhpGetWin32uSystemCallNumber(&mappedImage, exportAddress, &systemCallNumber) &&
                                        systemCallNumber.SystemServiceIndex == WIN32K_SERVICE_INDEX)
                                    {
                                        entry->Address = systemCallNumber.SystemCallIndex;
                                        PhAddItemList(list, entry);
                                        added = TRUE;
                                    }
                                }

                                if (!added)
                                {
                                    PhDereferenceObject(entry->Name);
                                    PhFree(entry);
                                }
                            }
                            else
                            {
                                PhAddItemList(list, entry);
                            }
                        }
                    }
                }

                qsort_s(list->Items, list->Count, sizeof(PVOID), PhpSystemCallListIndexSort, NULL);

                if (list->Count)
                {
                    win32kSystemCallList = PhCreateList(list->Count);

                    for (ULONG i = 0; i < list->Count; i++)
                    {
                        PPH_SYSCALL_NAME_ENTRY entry = list->Items[i];
                        PhAddItemList(win32kSystemCallList, entry->Name);
                        PhFree(entry);
                    }
                }

                PhDereferenceObject(list);
            }

            PhUnloadMappedImage(&mappedImage);
        }
    }

    *NtdllSystemCallList = ntdllSystemCallList;
    *Win32kSystemCallList = win32kSystemCallList;
}

/**
 * Decodes the secure-service dispatch value from an iumdll.dll export stub.
 *
 * The IUM trustlet stubs differ from the Nt/Zw stubs: there is no "mov r10, rcx"
 * prologue and the immediate is a full 32-bit dispatch value with the
 * SecureKernelCall bit set (e.g. "mov eax, 0x0800000N ; syscall ; ret").
 *
 * \param MappedImage The mapped iumdll.dll image.
 * \param ExportAddress The virtual address of the export stub within the mapping.
 * \param Dispatch Receives the decoded 32-bit dispatch value.
 *
 * \return TRUE if the stub was recognized and decoded.
 */
static BOOLEAN PhpGetSecureServiceNumber(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PVOID ExportAddress,
    _Out_ PULONG Dispatch
    )
{
    ULONG dispatch = 0;
    BOOLEAN decoded = FALSE;

    __try
    {
        switch (MappedImage->NtHeaders->FileHeader.Machine)
        {
        case IMAGE_FILE_MACHINE_AMD64:
        case IMAGE_FILE_MACHINE_I386:
            {
                static CONST BYTE prologue[1] = { 0xB8 }; // mov eax, imm32

                PhMappedImageProbe(MappedImage, ExportAddress, sizeof(prologue) + sizeof(ULONG));

                if (RtlEqualMemory(ExportAddress, prologue, sizeof(prologue)))
                {
                    dispatch = *(ULONG UNALIGNED*)PTR_ADD_OFFSET(ExportAddress, sizeof(prologue));
                    decoded = TRUE;
                }
            }
            break;
        case IMAGE_FILE_MACHINE_ARM64:
            {
                CONST ULONG UNALIGNED* instruction = ExportAddress;

                PhMappedImageProbe(MappedImage, ExportAddress, sizeof(ULONG) * 3);

                // movz w8, #imm16 ; movk w8, #imm16, lsl #16 ; svc #0
                // The secure dispatch value does not fit a single MOVZ, so the high
                // half is supplied by a MOVK before the SVC. (dmex)
                if ((instruction[0] & 0x7FE0001F) == 0x52800008 &&
                    (instruction[1] & 0x7FE0001F) == 0x72A00008 &&
                    instruction[2] == 0xD4000001)
                {
                    dispatch = ((instruction[0] >> 5) & 0xFFFF) |
                        (((instruction[1] >> 5) & 0xFFFF) << 16);
                    decoded = TRUE;
                }
            }
            break;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        decoded = FALSE;
    }

    *Dispatch = dispatch;

    return decoded;
}

/**
 * Builds the IUM secure-service name list by parsing the trustlet stub library.
 *
 * Each iumdll.dll export whose stub carries the SecureKernelCall bit is placed
 * at its secure-service index (bits 0-11), mirroring how the ntdll/win32k lists
 * are generated. Returns NULL if the image cannot be mapped or parsed, in which
 * case callers fall back to the embedded PhpSecureServiceNames table.
 */
static PPH_LIST PhpGenerateSecureServiceList(
    VOID
    )
{
    static CONST PH_STRINGREF iumdllFileName = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\iumdll.dll");
    PPH_LIST secureServiceList = NULL;
    PH_MAPPED_IMAGE mappedImage;
    PH_MAPPED_IMAGE_EXPORTS exports;
    PH_MAPPED_IMAGE_EXPORT_ENTRY exportEntry;
    PH_MAPPED_IMAGE_EXPORT_FUNCTION exportFunction;

    if (!NT_SUCCESS(PhLoadMappedImageEx(&iumdllFileName, NULL, &mappedImage)))
        return NULL;

    if (NT_SUCCESS(PhGetMappedImageExports(&exports, &mappedImage)))
    {
        PPH_LIST list = PhCreateList(exports.NumberOfEntries);

        for (ULONG i = 0; i < exports.NumberOfEntries; i++)
        {
            if (!NT_SUCCESS(PhGetMappedImageExportEntry(&exports, i, &exportEntry)))
                continue;

            if (!(exportEntry.Name && strncmp(exportEntry.Name, "Ium", 3) == 0))
                continue;

            if (NT_SUCCESS(PhGetMappedImageExportFunction(&exports, NULL, exportEntry.Ordinal, &exportFunction)))
            {
                PBYTE exportAddress;
                ULONG dispatch;
                BOOLEAN secureKernelCall;
                USHORT serviceIndex;

                if (!NT_SUCCESS(PhMappedImageRvaToVa(&mappedImage, PtrToUlong(exportFunction.Function), (PVOID*)&exportAddress)))
                    continue;

                if (!PhpGetSecureServiceNumber(&mappedImage, exportAddress, &dispatch))
                    continue;

                serviceIndex = PhExtractSystemCallDispatch(dispatch, NULL, &secureKernelCall);

                if (secureKernelCall)
                {
                    PPH_SYSCALL_NAME_ENTRY entry;

                    entry = PhAllocate(sizeof(PH_SYSCALL_NAME_ENTRY));
                    entry->Address = serviceIndex;
                    entry->Name = PhZeroExtendToUtf16(exportEntry.Name);

                    PhAddItemList(list, entry);
                }
            }
        }

        qsort_s(list->Items, list->Count, sizeof(PVOID), PhpSystemCallListIndexSort, NULL);

        if (list->Count)
        {
            secureServiceList = PhCreateList(list->Count);

            for (ULONG i = 0; i < list->Count; i++)
            {
                PPH_SYSCALL_NAME_ENTRY entry = list->Items[i];
                PhAddItemList(secureServiceList, entry->Name);
                PhFree(entry);
            }
        }

        PhDereferenceObject(list);
    }

    PhUnloadMappedImage(&mappedImage);

    return secureServiceList;
}

/**
 * Resolves the name of an IUM secure-kernel service from its service index.
 *
 * The list is extracted from iumdll.dll at first use; if that image cannot be
 * parsed the embedded PhpSecureServiceNames table is used instead.
 *
 * \param ServiceIndex The secure service index (bits 0-11 of the dispatch value).
 *
 * \return A referenced string naming the secure service, or NULL if the index
 * is out of range.
 */
PPH_STRING PhGetSecureServiceName(
    _In_ USHORT ServiceIndex
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PPH_LIST secureServiceList = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        secureServiceList = PhpGenerateSecureServiceList();
        PhEndInitOnce(&initOnce);
    }

    if (secureServiceList)
    {
        if (ServiceIndex < secureServiceList->Count)
            return PhReferenceObject(secureServiceList->Items[ServiceIndex]);

        return NULL;
    }

    // Fallback: the embedded table (iumdll.dll could not be parsed).
    if (ServiceIndex < RTL_NUMBER_OF(PhpSecureServiceNames))
        return PhCreateString(PhpSecureServiceNames[ServiceIndex]);

    return NULL;
}

PPH_STRING PhGetSystemCallNumberName(
    _In_ USHORT SystemCallNumber
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PPH_LIST ntdllSystemCallList = NULL;
    static PPH_LIST win32kSystemCallList = NULL;
    USHORT serviceIndex;
    USHORT systemCallIndex;

    if (PhBeginInitOnce(&initOnce))
    {
        PhGenerateSyscallLists(&ntdllSystemCallList, &win32kSystemCallList);
        PhEndInitOnce(&initOnce);
    }

    systemCallIndex = PhExtractSystemCallDispatch(SystemCallNumber, &serviceIndex, NULL);

    switch (serviceIndex)
    {
    case NTOS_SERVICE_INDEX:
        {
            if (ntdllSystemCallList && systemCallIndex < ntdllSystemCallList->Count)
            {
                PPH_STRING entry = ntdllSystemCallList->Items[systemCallIndex];

                return PhReferenceObject(entry);
            }
        }
        break;
    case WIN32K_SERVICE_INDEX:
        {
            if (win32kSystemCallList && systemCallIndex < win32kSystemCallList->Count)
            {
                PPH_STRING entry = win32kSystemCallList->Items[systemCallIndex];

                return PhReferenceObject(entry);
            }
        }
        break;
    }

    return NULL;
}
