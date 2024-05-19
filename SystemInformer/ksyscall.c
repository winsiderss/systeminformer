/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2019-2023
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

VOID PhGenerateSyscallLists(
    _Out_ PPH_LIST* NtdllSystemCallList,
    _Out_ PPH_LIST* Win32kSystemCallList
    )
{
    static PH_STRINGREF ntdllFileName = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntdll.dll");
    static PH_STRINGREF win32kFileName = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\win32k.sys");
    static PH_STRINGREF win32uFileName = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\win32u.dll");
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

    if (WindowsVersion >= WINDOWS_10_20H1)
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
                            if (WindowsVersion >= WINDOWS_10_21H1)
                            {
                                PBYTE exportAddress;

                                if (exportAddress = PhMappedImageRvaToVa(&mappedImage, PtrToUlong(exportFunction.Function), NULL))
                                {
                                    PNT_SYSTEMCALL_NUMBER systemCallEntry = NULL;

                                #ifdef _WIN64
                                    BYTE prologue[4] = { 0x4C, 0x8B, 0xD1, 0xB8 };

                                    if (RtlEqualMemory(exportAddress, prologue, sizeof(prologue)))
                                    {
                                        systemCallEntry = PTR_ADD_OFFSET(exportAddress, sizeof(prologue));
                                    }
                                #else
                                    BYTE prologue[1] = { 0xB8 };

                                    if (RtlEqualMemory(exportAddress, prologue, sizeof(prologue)))
                                    {
                                        systemCallEntry = PTR_ADD_OFFSET(exportAddress, sizeof(prologue));
                                    }
                                #endif

                                    if (systemCallEntry && systemCallEntry->SystemServiceIndex == WIN32K_SERVICE_INDEX)
                                    {
                                        entry->Address = systemCallEntry->SystemCallIndex;
                                        PhAddItemList(list, entry);
                                    }
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

PPH_STRING PhGetSystemCallNumberName(
    _In_ USHORT SystemCallNumber
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PPH_LIST ntdllSystemCallList = NULL;
    static PPH_LIST win32kSystemCallList = NULL;
    PNT_SYSTEMCALL_NUMBER systemCallEntry = (PNT_SYSTEMCALL_NUMBER)&SystemCallNumber;

    if (PhBeginInitOnce(&initOnce))
    {
        PhGenerateSyscallLists(&ntdllSystemCallList, &win32kSystemCallList);
        PhEndInitOnce(&initOnce);
    }

    switch (systemCallEntry->SystemServiceIndex)
    {
    case NTOS_SERVICE_INDEX:
        {
            if (ntdllSystemCallList && systemCallEntry->SystemCallIndex <= ntdllSystemCallList->Count)
            {
                PPH_STRING entry = ntdllSystemCallList->Items[systemCallEntry->SystemCallIndex];

                return PhReferenceObject(entry);
            }
        }
        break;
    case WIN32K_SERVICE_INDEX:
        {
            if (win32kSystemCallList && systemCallEntry->SystemCallIndex <= win32kSystemCallList->Count)
            {
                PPH_STRING entry = win32kSystemCallList->Items[systemCallEntry->SystemCallIndex];

                return PhReferenceObject(entry);
            }
        }
        break;
    }

    return NULL;
}
