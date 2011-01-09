/*
 * Process Hacker Extended Tools - 
 *   working set watch
 * 
 * Copyright (C) 2011 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "exttools.h"
#include "resource.h"

typedef struct _WS_WATCH_CONTEXT
{
    LONG RefCount;

    PPH_PROCESS_ITEM ProcessItem;
    HWND WindowHandle;
    HWND ListViewHandle;
    BOOLEAN Enabled;
    BOOLEAN Destroying;
    PPH_HASHTABLE Hashtable;
    HANDLE ProcessHandle;
    PVOID Buffer;
    ULONG BufferSize;

    PPH_SYMBOL_PROVIDER SymbolProvider;
    HANDLE LoadingSymbolsForProcessId;
    SINGLE_LIST_ENTRY ResultListHead;
    PH_QUEUED_LOCK ResultListLock;
} WS_WATCH_CONTEXT, *PWS_WATCH_CONTEXT;

typedef struct _SYMBOL_LOOKUP_RESULT
{
    SINGLE_LIST_ENTRY ListEntry;
    PWS_WATCH_CONTEXT Context;
    PVOID Address;
    PPH_STRING Symbol;
} SYMBOL_LOOKUP_RESULT, *PSYMBOL_LOOKUP_RESULT;

VOID EtpReferenceWsWatchContext(
    __inout PWS_WATCH_CONTEXT Context
    );

VOID EtpDereferenceWsWatchContext(
    __inout PWS_WATCH_CONTEXT Context
    );

INT_PTR CALLBACK EtpWsWatchDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID EtShowWsWatchDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PWS_WATCH_CONTEXT context;

    context = PhAllocate(sizeof(WS_WATCH_CONTEXT));
    memset(context, 0, sizeof(WS_WATCH_CONTEXT));
    context->RefCount = 1;
    context->ProcessItem = ProcessItem;

    DialogBoxParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_WSWATCH),
        ParentWindowHandle,
        EtpWsWatchDlgProc,
        (LPARAM)context
        );
    EtpDereferenceWsWatchContext(context);
}

static VOID EtpReferenceWsWatchContext(
    __inout PWS_WATCH_CONTEXT Context
    )
{
    _InterlockedIncrement(&Context->RefCount);
}

static VOID EtpDereferenceWsWatchContext(
    __inout PWS_WATCH_CONTEXT Context
    )
{
    if (_InterlockedDecrement(&Context->RefCount) == 0)
    {
        PSINGLE_LIST_ENTRY listEntry;

        if (Context->SymbolProvider)
            PhDereferenceObject(Context->SymbolProvider);

        // Free all unused results.

        PhAcquireQueuedLockExclusive(&Context->ResultListLock);

        listEntry = Context->ResultListHead.Next;

        while (listEntry)
        {
            PSYMBOL_LOOKUP_RESULT result;

            result = CONTAINING_RECORD(listEntry, SYMBOL_LOOKUP_RESULT, ListEntry);
            listEntry = listEntry->Next;
            PhDereferenceObject(result->Symbol);
            PhFree(result);
        }

        PhReleaseQueuedLockExclusive(&Context->ResultListLock);

        PhFree(Context);
    }
}

static NTSTATUS EtpSymbolLookupFunction(
    __in PVOID Parameter
    )
{
    PSYMBOL_LOOKUP_RESULT result;

    result = Parameter;

    // Don't bother looking up the symbol if the window has already closed.
    if (result->Context->Destroying)
    {
        EtpDereferenceWsWatchContext(result->Context);
        PhFree(result);
        return STATUS_SUCCESS;
    }

    result->Symbol = PhGetSymbolFromAddress(
        result->Context->SymbolProvider,
        (ULONG64)result->Address,
        NULL,
        NULL,
        NULL,
        NULL
        );

    // Fail if we don't have a symbol.
    if (!result->Symbol)
    {
        EtpDereferenceWsWatchContext(result->Context);
        PhFree(result);
        return STATUS_SUCCESS;
    }

    PhAcquireQueuedLockExclusive(&result->Context->ResultListLock);
    PushEntryList(&result->Context->ResultListHead, &result->ListEntry);
    PhReleaseQueuedLockExclusive(&result->Context->ResultListLock);
    EtpDereferenceWsWatchContext(result->Context);

    return STATUS_SUCCESS;
}

static VOID EtpQueueSymbolLookup(
    __in PWS_WATCH_CONTEXT Context,
    __in PVOID Address
    )
{
    PSYMBOL_LOOKUP_RESULT result;

    result = PhAllocate(sizeof(SYMBOL_LOOKUP_RESULT));
    result->Context = Context;
    result->Address = Address;
    EtpReferenceWsWatchContext(Context);

    PhQueueItemGlobalWorkQueue(EtpSymbolLookupFunction, result);
}

static PPH_STRING EtpGetBasicSymbol(
    __in PPH_SYMBOL_PROVIDER SymbolProvider,
    __in ULONG64 Address
    )
{
    ULONG64 modBase;
    PPH_STRING fileName = NULL;
    PPH_STRING baseName = NULL;
    PPH_STRING symbol;

    modBase = PhGetModuleFromAddress(SymbolProvider, Address, &fileName);

    if (!fileName)
    {
        symbol = PhCreateStringEx(NULL, PH_PTR_STR_LEN * 2);
        PhPrintPointer(symbol->Buffer, (PVOID)Address);
        PhTrimToNullTerminatorString(symbol);
    }
    else
    {
        PH_FORMAT format[3];

        baseName = PhGetBaseName(fileName);

        PhInitFormatSR(&format[0], baseName->sr);
        PhInitFormatS(&format[1], L"+0x");
        PhInitFormatIX(&format[2], (ULONG_PTR)(Address - modBase));

        symbol = PhFormat(format, 3, baseName->Length + 6 + 32);
    }

    if (fileName)
        PhDereferenceObject(fileName);
    if (baseName)
        PhDereferenceObject(baseName);

    return symbol;
}

static VOID EtpProcessSymbolLookupResults(
    __in HWND hwndDlg,
    __in PWS_WATCH_CONTEXT Context
    )
{
    PSINGLE_LIST_ENTRY listEntry;

    // Remove all results.
    PhAcquireQueuedLockExclusive(&Context->ResultListLock);
    listEntry = Context->ResultListHead.Next;
    Context->ResultListHead.Next = NULL;
    PhReleaseQueuedLockExclusive(&Context->ResultListLock);

    // Update the list view with the results.
    while (listEntry)
    {
        PSYMBOL_LOOKUP_RESULT result;

        result = CONTAINING_RECORD(listEntry, SYMBOL_LOOKUP_RESULT, ListEntry);
        listEntry = listEntry->Next;

        PhSetListViewSubItem(
            Context->ListViewHandle,
            PhFindListViewItemByParam(Context->ListViewHandle, -1, result->Address),
            0,
            result->Symbol->Buffer
            );

        PhDereferenceObject(result->Symbol);
        PhFree(result);
    }
}

static BOOLEAN EtpUpdateWsWatch(
    __in HWND hwndDlg,
    __in PWS_WATCH_CONTEXT Context
    )
{
    NTSTATUS status;
    ULONG returnLength;
    PPROCESS_WS_WATCH_INFORMATION_EX wsWatchInfo;

    // Query WS watch information.

    if (!Context->Buffer)
        return TRUE;

    status = NtQueryInformationProcess(
        Context->ProcessHandle,
        ProcessWorkingSetWatchEx,
        Context->Buffer,
        Context->BufferSize,
        &returnLength
        );

    if (status == STATUS_UNSUCCESSFUL)
        return FALSE; // WS Watch is not enabled

    if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(Context->Buffer);
        Context->Buffer = PhAllocate(returnLength);
        Context->BufferSize = returnLength;

        status = NtQueryInformationProcess(
            Context->ProcessHandle,
            ProcessWorkingSetWatchEx,
            Context->Buffer,
            Context->BufferSize,
            &returnLength
            );
    }

    if (!NT_SUCCESS(status))
        return TRUE;

    // Update the hashtable and list view.

    wsWatchInfo = Context->Buffer;

    while (wsWatchInfo->BasicInfo.FaultingPc)
    {
        PPVOID entry;
        WCHAR buffer[PH_INT32_STR_LEN_1];
        INT lvItemIndex;
        ULONG newCount;

        // Update the count in the entry for this instruction pointer, or add a new entry if it doesn't exist.

        entry = PhFindItemSimpleHashtable(Context->Hashtable, wsWatchInfo->BasicInfo.FaultingPc);

        if (entry)
        {
            newCount = PtrToUlong(*entry) + 1;
            *entry = UlongToPtr(newCount);
            lvItemIndex = PhFindListViewItemByParam(Context->ListViewHandle, -1, wsWatchInfo->BasicInfo.FaultingPc);
        }
        else
        {
            PPH_STRING basicSymbol;

            newCount = 1;
            PhAddItemSimpleHashtable(Context->Hashtable, wsWatchInfo->BasicInfo.FaultingPc, UlongToPtr(1));

            // Get a basic symbol name (module+offset).
            basicSymbol = EtpGetBasicSymbol(Context->SymbolProvider, (ULONG64)wsWatchInfo->BasicInfo.FaultingPc);

            lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, basicSymbol->Buffer, wsWatchInfo->BasicInfo.FaultingPc);
            PhDereferenceObject(basicSymbol);

            // Queue a full symbol lookup.
            EtpQueueSymbolLookup(Context, wsWatchInfo->BasicInfo.FaultingPc);
        }

        // Update the count in the list view item.
        PhPrintUInt32(buffer, newCount);
        PhSetListViewSubItem(
            Context->ListViewHandle,
            lvItemIndex,
            1,
            buffer
            );

        wsWatchInfo++;
    }

    EtpProcessSymbolLookupResults(hwndDlg, Context);
    ExtendedListView_SortItems(Context->ListViewHandle);

    return TRUE;
}

static BOOLEAN NTAPI EnumGenericModulesCallback(
    __in PPH_MODULE_INFO Module,
    __in_opt PVOID Context
    )
{
    PWS_WATCH_CONTEXT context = Context;

    // If we're loading kernel module symbols for a process other than 
    // System, ignore modules which are in user space. This may happen 
    // in Windows 7.
    if (
        context->LoadingSymbolsForProcessId == SYSTEM_PROCESS_ID &&
        (ULONG_PTR)Module->BaseAddress <= PhSystemBasicInformation.MaximumUserModeAddress
        )
        return TRUE;

    PhLoadModuleSymbolProvider(context->SymbolProvider, Module->FileName->Buffer,
        (ULONG64)Module->BaseAddress, Module->Size);

    return TRUE;
}

INT_PTR CALLBACK EtpWsWatchDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PWS_WATCH_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PWS_WATCH_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PWS_WATCH_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            context->WindowHandle = hwndDlg;
            context->ListViewHandle = lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 340, L"Instruction");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"Count");
            PhSetExtendedListView(lvHandle);
            ExtendedListView_SetSort(lvHandle, 1, DescendingSortOrder);

            context->Hashtable = PhCreateSimpleHashtable(64);
            context->BufferSize = 0x2000;
            context->Buffer = PhAllocate(context->BufferSize);

            PhInitializeQueuedLock(&context->ResultListLock);
            context->SymbolProvider = PhCreateSymbolProvider(context->ProcessItem->ProcessId);
            PhLoadSymbolProviderOptions(context->SymbolProvider);

            if (!context->SymbolProvider || !context->SymbolProvider->IsRealHandle)
            {
                PhShowError(hwndDlg, L"Unable to open the process.");
                EndDialog(hwndDlg, IDCANCEL);
                break;
            }

            context->ProcessHandle = context->SymbolProvider->ProcessHandle;

            // Load symbols for both process and kernel modules.
            context->LoadingSymbolsForProcessId = context->ProcessItem->ProcessId;
            PhEnumGenericModules(
                NULL,
                context->ProcessHandle,
                0,
                EnumGenericModulesCallback,
                context
                );
            context->LoadingSymbolsForProcessId = SYSTEM_PROCESS_ID;
            PhEnumGenericModules(
                SYSTEM_PROCESS_ID,
                NULL,
                0,
                EnumGenericModulesCallback,
                context
                );

            context->Enabled = EtpUpdateWsWatch(hwndDlg, context);

            if (context->Enabled)
            {
                // WS Watch is already enabled for the process. Enable updating.
                EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE), FALSE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_WSWATCHENABLED), SW_SHOW);
                SetTimer(hwndDlg, 1, 1000, NULL);
            }
            else
            {
                // WS Watch has not yet been enabled for the process.
            }
        }
        break;
    case WM_DESTROY:
        {
            context->Destroying = TRUE;

            PhDereferenceObject(context->Hashtable);

            if (context->Buffer)
            {
                PhFree(context->Buffer);
                context->Buffer = NULL;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_ENABLE:
                {
                    NTSTATUS status;
                    HANDLE processHandle;

                    if (NT_SUCCESS(status = PhOpenProcess(
                        &processHandle,
                        PROCESS_SET_INFORMATION,
                        context->ProcessItem->ProcessId
                        )))
                    {
                        status = NtSetInformationProcess(
                            processHandle,
                            ProcessWorkingSetWatchEx,
                            NULL,
                            0
                            );
                        NtClose(processHandle);
                    }

                    if (NT_SUCCESS(status))
                    {
                        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE), FALSE);
                        ShowWindow(GetDlgItem(hwndDlg, IDC_WSWATCHENABLED), SW_SHOW);
                        SetTimer(hwndDlg, 1, 1000, NULL);
                    }
                    else
                    {
                        PhShowStatus(hwndDlg, L"Unable to enable WS watch", status, 0);
                    }
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_TIMER:
        {
            switch (wParam)
            {
            case 1:
                {
                    EtpUpdateWsWatch(hwndDlg, context);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
