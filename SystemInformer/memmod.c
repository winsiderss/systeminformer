/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2023
 *
 */

#include <phapp.h>
#include <procprv.h>
#include <settings.h>
#include <emenu.h>
#include <symprv.h>
#include <workqueue.h>

typedef struct _PH_IMAGE_MODIFIED_DIALOG_CONTEXT
{
    LONG RefCount;

    HWND WindowHandle;
    HWND ListViewHandle;
    HWND ParentWindowHandle;
    PPH_PROCESS_ITEM ProcessItem;
    PH_LAYOUT_MANAGER LayoutManager;

    BOOLEAN Destroying;

    HANDLE ProcessHandle;
    ULONG ImageQueryFailures;
    PPH_LIST ImageAddressList;
    PPH_LIST PageTamperingList;

    PH_INITONCE SymbolProviderInitOnce;
    PPH_SYMBOL_PROVIDER SymbolProvider;
    SINGLE_LIST_ENTRY ResultListHead;
    PH_QUEUED_LOCK ResultListLock;
} PH_IMAGE_MODIFIED_DIALOG_CONTEXT, *PPH_IMAGE_MODIFIED_DIALOG_CONTEXT;

typedef struct _PH_IMAGE_MAPPED_BASE_ENTRY
{
    PVOID ImageBase;
    SIZE_T SizeOfImage;
    PPH_STRING FileName;
} PH_IMAGE_MAPPED_BASE_ENTRY, *PPH_IMAGE_MAPPED_BASE_ENTRY;

typedef struct _PH_IMAGE_MAPPED_FAILURE_ENTRY
{
    PVOID BaseAddress;
    SIZE_T SizeOfImage;
    PVOID VirtualAddress;
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Valid : 1;
            ULONG Unused : 31;
        };
    };
} PH_IMAGE_MAPPED_FAILURE_ENTRY, *PPH_IMAGE_MAPPED_FAILURE_ENTRY;

NTSTATUS NTAPI PhEnumImagesForTamperingCallback(
    _In_ HANDLE ProcessHandle,
    _In_ PMEMORY_BASIC_INFORMATION BasicInformation,
    _In_ PVOID Context
    )
{
    PPH_IMAGE_MODIFIED_DIALOG_CONTEXT context = Context;

    if (
        BasicInformation->Type == MEM_IMAGE &&
        BasicInformation->AllocationBase == BasicInformation->BaseAddress
        )
    {
        PVOID imageBase;
        SIZE_T imageSize;

        if (NT_SUCCESS(PhGetProcessMappedImageBaseFromAddress(
            ProcessHandle,
            BasicInformation->BaseAddress,
            &imageBase,
            &imageSize
            )))
        {
            BOOLEAN found = FALSE;

            for (ULONG i = 0; i < context->ImageAddressList->Count; i++)
            {
                PPH_IMAGE_MAPPED_BASE_ENTRY entry = context->ImageAddressList->Items[i];

                if (entry->ImageBase == imageBase)
                {
                    found = TRUE;
                    break;
                }
            }

            if (!found)
            {
                PPH_IMAGE_MAPPED_BASE_ENTRY entry;
                PPH_STRING fileName;

                entry = PhAllocateZero(sizeof(PH_IMAGE_MAPPED_BASE_ENTRY));
                entry->ImageBase = imageBase;
                entry->SizeOfImage = imageSize;

                if (NT_SUCCESS(PhGetProcessMappedFileName(ProcessHandle, entry->ImageBase, &fileName)))
                {
                    entry->FileName = fileName;
                }

                PhAddItemList(context->ImageAddressList, entry);
            }
        }
        else
        {
            context->ImageQueryFailures++;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PhpEnumVirtualMemoryAttributesCallback(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T SizeOfImage,
    _In_ ULONG_PTR NumberOfEntries,
    _In_ PMEMORY_WORKING_SET_EX_INFORMATION Blocks,
    _In_ PVOID Context
    )
{
    PPH_IMAGE_MODIFIED_DIALOG_CONTEXT context = Context;

    for (ULONG_PTR i = 0; i < NumberOfEntries; i++)
    {
        PMEMORY_WORKING_SET_EX_INFORMATION page = &Blocks[i];
        PMEMORY_WORKING_SET_EX_BLOCK block = &page->u1.VirtualAttributes;

        if (!block->SharedOriginal)
        {
            PPH_IMAGE_MAPPED_FAILURE_ENTRY entry;

            entry = PhAllocateZero(sizeof(PH_IMAGE_MAPPED_FAILURE_ENTRY));
            entry->BaseAddress = BaseAddress;
            entry->SizeOfImage = SizeOfImage;
            entry->VirtualAddress = page->VirtualAddress;
            entry->Valid = !!block->Valid;

            PhAddItemList(context->PageTamperingList, entry);
        }
    }

    return STATUS_SUCCESS;
}

typedef struct _SYMBOL_LOOKUP_RESULT
{
    SINGLE_LIST_ENTRY ListEntry;
    PPH_IMAGE_MODIFIED_DIALOG_CONTEXT Context;
    PVOID Address;
    PPH_STRING Symbol;
    PPH_STRING FileName;
} SYMBOL_LOOKUP_RESULT, *PSYMBOL_LOOKUP_RESULT;

VOID PhpCleanupPageModifiedLists(
    _In_ PPH_IMAGE_MODIFIED_DIALOG_CONTEXT Context
    )
{
    PSINGLE_LIST_ENTRY listEntry;

    if (Context->ImageAddressList)
    {
        for (ULONG i = 0; i < Context->ImageAddressList->Count; i++)
        {
            PPH_IMAGE_MAPPED_BASE_ENTRY entry = Context->ImageAddressList->Items[i];
            PhClearReference(&entry->FileName);
            PhFree(entry);
        }
        PhClearReference(&Context->ImageAddressList);
    }

    if (Context->PageTamperingList)
    {
        for (ULONG i = 0; i < Context->PageTamperingList->Count; i++)
            PhFree(Context->PageTamperingList->Items[i]);
        PhClearReference(&Context->PageTamperingList);
    }

    // Free all unused results.

    PhAcquireQueuedLockExclusive(&Context->ResultListLock);

    listEntry = Context->ResultListHead.Next;

    while (listEntry)
    {
        PSYMBOL_LOOKUP_RESULT result;

        result = CONTAINING_RECORD(listEntry, SYMBOL_LOOKUP_RESULT, ListEntry);
        listEntry = listEntry->Next;

        PhClearReference(&result->Symbol);
        PhClearReference(&result->FileName);

        PhFree(result);
    }

    PhReleaseQueuedLockExclusive(&Context->ResultListLock);
}

VOID PhpLimitedSymbolReferenceContext(
    _In_ PPH_IMAGE_MODIFIED_DIALOG_CONTEXT Context
    )
{
    _InterlockedIncrement(&Context->RefCount);
}

VOID PhpLimitedSymbolDereferenceContext(
    _In_ PPH_IMAGE_MODIFIED_DIALOG_CONTEXT Context
    )
{
    if (_InterlockedDecrement(&Context->RefCount) == 0)
    {
        if (Context->SymbolProvider)
            PhDereferenceObject(Context->SymbolProvider);
        if (Context->ProcessHandle)
            NtClose(Context->ProcessHandle);

        PhpCleanupPageModifiedLists(Context);

        PhDereferenceObject(Context->ProcessItem);

        PhFree(Context);
    }
}

static NTSTATUS PhpLimitedSymbolProviderLookupFunction(
    _In_ PVOID Parameter
    )
{
    PSYMBOL_LOOKUP_RESULT result = Parameter;

    if (PhBeginInitOnce(&result->Context->SymbolProviderInitOnce))
    {
        PhLoadSymbolProviderOptions(result->Context->SymbolProvider);
        PhLoadModulesForVirtualSymbolProvider(result->Context->SymbolProvider, result->Context->ProcessItem->ProcessId);

        for (ULONG i = 0; i < result->Context->ImageAddressList->Count; i++)
        {
            PPH_IMAGE_MAPPED_BASE_ENTRY entry = result->Context->ImageAddressList->Items[i];

            if (entry->FileName)
            {
                PhLoadFileNameSymbolProvider(
                    result->Context->SymbolProvider,
                    entry->FileName,
                    (ULONG64)entry->ImageBase,
                    (ULONG)entry->SizeOfImage
                    );
            }
            else
            {
                result->Context->ImageQueryFailures++;
            }
        }

        PhEndInitOnce(&result->Context->SymbolProviderInitOnce);
    }

    if (result->Context->Destroying)
    {
        PhpLimitedSymbolDereferenceContext(result->Context);
        PhFree(result);
        return STATUS_SUCCESS;
    }

    result->Symbol = PhGetSymbolFromAddress(
        result->Context->SymbolProvider,
        (ULONG64)result->Address,
        NULL,
        &result->FileName,
        NULL,
        NULL
        );

    // Fail if we don't have a symbol.
    if (!result->Symbol)
    {
        PhpLimitedSymbolDereferenceContext(result->Context);
        PhFree(result);
        return STATUS_SUCCESS;
    }

    PhAcquireQueuedLockExclusive(&result->Context->ResultListLock);
    PushEntryList(&result->Context->ResultListHead, &result->ListEntry);
    PhReleaseQueuedLockExclusive(&result->Context->ResultListLock);
    PhpLimitedSymbolDereferenceContext(result->Context);

    return STATUS_SUCCESS;
}

static VOID PhpLimitedSymbolProviderQueueSymbolLookup(
    _In_ PPH_IMAGE_MODIFIED_DIALOG_CONTEXT Context,
    _In_ PVOID Address
    )
{
    PSYMBOL_LOOKUP_RESULT result;

    if (!Context->SymbolProvider)
    {
        Context->SymbolProvider = PhCreateSymbolProvider(NULL);
    }

    result = PhAllocateZero(sizeof(SYMBOL_LOOKUP_RESULT));
    result->Context = Context;
    result->Address = Address;
    PhpLimitedSymbolReferenceContext(Context);

    PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), PhpLimitedSymbolProviderLookupFunction, result);
}

static VOID PhpProcessSymbolLookupResults(
    _In_ PPH_IMAGE_MODIFIED_DIALOG_CONTEXT Context
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
        INT lvItemIndex;

        result = CONTAINING_RECORD(listEntry, SYMBOL_LOOKUP_RESULT, ListEntry);
        listEntry = listEntry->Next;

        lvItemIndex = PhFindListViewItemByParam(Context->ListViewHandle, INT_ERROR, result->Address);

        if (lvItemIndex != INT_ERROR)
        {
            PhSetListViewSubItem(
                Context->ListViewHandle,
                lvItemIndex,
                5,
                PhGetString(result->Symbol)
                );
        }

        PhDereferenceObject(result->Symbol);
        PhFree(result);
    }
}

// Get a basic symbol name (module+offset).
static PPH_STRING PhpLimitedSymbolProviderGetBasicSymbol(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG64 Address
    )
{
    ULONG64 modBase;
    PPH_STRING fileName = NULL;
    PPH_STRING baseName = NULL;
    PPH_STRING symbol;

    modBase = PhGetModuleFromAddress(SymbolProvider, Address, &fileName);

    if (!fileName)
    {
        symbol = PhCreateStringEx(NULL, PH_PTR_STR_LEN * sizeof(WCHAR));
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

NTSTATUS PhCheckProcessImagesForTampering(
    _In_ PPH_IMAGE_MODIFIED_DIALOG_CONTEXT Context
    )
{
    NTSTATUS status;

    Context->ImageAddressList = PhCreateList(20);
    Context->PageTamperingList = PhCreateList(20);

    status = PhEnumVirtualMemory(
        Context->ProcessHandle,
        NULL,
        PhEnumImagesForTamperingCallback,
        Context
        );

    if (!NT_SUCCESS(status))
        return status;

    for (ULONG i = 0; i < Context->ImageAddressList->Count; i++)
    {
        PPH_IMAGE_MAPPED_BASE_ENTRY entry = Context->ImageAddressList->Items[i];

        status = PhEnumVirtualMemoryAttributes( // PhCheckImagePagesForTampering
            Context->ProcessHandle,
            NULL,
            entry->ImageBase,
            entry->SizeOfImage,
            PhpEnumVirtualMemoryAttributesCallback,
            Context
            );

        if (!NT_SUCCESS(status))
        {
            Context->ImageQueryFailures++;
        }
    }

    for (ULONG i = 0; i < Context->PageTamperingList->Count; i++)
    {
        PPH_IMAGE_MAPPED_FAILURE_ENTRY entry = Context->PageTamperingList->Items[i];
        WCHAR value[PH_INT64_STR_LEN_1];
        INT lvItemIndex;

        PhPrintUInt32(value, i + 1);
        lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, value, entry->VirtualAddress);

        for (ULONG j = 0; j < Context->ImageAddressList->Count; j++)
        {
            PPH_IMAGE_MAPPED_BASE_ENTRY image = Context->ImageAddressList->Items[j];

            if (
                !PhIsNullOrEmptyString(image->FileName) &&
                image->ImageBase == entry->BaseAddress &&
                image->SizeOfImage == entry->SizeOfImage
                )
            {
                PPH_STRING fileNameWin32 = PhGetFileName(image->FileName);
                PhMoveReference(&fileNameWin32, PhGetBaseName(fileNameWin32));
                PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, PhGetString(fileNameWin32));
                PhDereferenceObject(fileNameWin32);
                break;
            }
        }

        PhPrintPointer(value, entry->BaseAddress);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, value);
        PhPrintPointer(value, PTR_SUB_OFFSET(entry->VirtualAddress, entry->BaseAddress));
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, value);
        PhPrintPointer(value, entry->VirtualAddress);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 4, value);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 5, L"Resolving symbols...");
        PhpLimitedSymbolProviderQueueSymbolLookup(Context, entry->VirtualAddress);
    }

    PhpProcessSymbolLookupResults(Context);

    return status;
}

INT_PTR CALLBACK PhPageModifiedDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_IMAGE_MODIFIED_DIALOG_CONTEXT context = NULL;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = (PPH_IMAGE_MODIFIED_DIALOG_CONTEXT)lParam;

        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = WindowHandle;
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_LIST);

            PhSetApplicationWindowIcon(WindowHandle);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 50, L"Index");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"File");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"ImageBase");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Offset");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Address");
            PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 100, L"Symbol");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"MemoryModifiedListViewColumns", context->ListViewHandle);
            PhLoadListViewSortColumnsFromSetting(L"MemoryModifiedListViewSort", context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhGetIntegerPairSetting(L"MemoryModifiedWindowPosition").X)
                PhLoadWindowPlacementFromSetting(L"MemoryModifiedWindowPosition", L"MemoryModifiedWindowSize", WindowHandle);
            else
                PhCenterWindow(WindowHandle, context->ParentWindowHandle);

            {
                PPH_STRING windowTitle;

                windowTitle = PhGetWindowText(WindowHandle);
                PhMoveReference(&windowTitle, PhFormatString(
                    L"%s: %s (%lu)",
                    PhGetStringOrEmpty(windowTitle),
                    PhGetStringOrEmpty(context->ProcessItem->ProcessName),
                    HandleToUlong(context->ProcessItem->ProcessId)
                    ));

                PhSetWindowText(WindowHandle, PhGetStringOrEmpty(windowTitle));
                PhDereferenceObject(windowTitle);
            }

            PhSetTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT, 1000, NULL);

            PhInitializeWindowTheme(WindowHandle, PhEnableThemeSupport);

            {
                NTSTATUS status;

                status = PhCheckProcessImagesForTampering(context);

                if (!NT_SUCCESS(status))
                {
                    PhShowStatus(context->ParentWindowHandle, L"Unable to perform the scan", status, 0);
                    EndDialog(WindowHandle, IDCANCEL);
                }
            }
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewSortColumnsToSetting(L"MemoryModifiedListViewSort", context->ListViewHandle);
            PhSaveListViewColumnsToSetting(L"MemoryModifiedListViewColumns", context->ListViewHandle);
            PhSaveWindowPlacementToSetting(L"MemoryModifiedWindowPosition", L"MemoryModifiedWindowSize", WindowHandle);

            context->Destroying = TRUE;

            PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhpLimitedSymbolDereferenceContext(context);

            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_TIMER:
        {
            switch (wParam)
            {
            case PH_WINDOW_TIMER_DEFAULT:
                PhpProcessSymbolLookupResults(context);
                break;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(WindowHandle);
                break;
            case IDC_REFRESH:
                {
                    ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
                    ListView_DeleteAllItems(context->ListViewHandle);
                    ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);

                    PhpCleanupPageModifiedLists(context);

                    {
                        NTSTATUS status;

                        status = PhCheckProcessImagesForTampering(context);

                        if (!NT_SUCCESS(status))
                        {
                            PhShowStatus(WindowHandle, L"Unable to perform the scan", status, 0);
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        WindowHandle,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case IDC_COPY:
                                PhCopyListView(context->ListViewHandle);
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID PhShowImagePageModifiedDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_IMAGE_MODIFIED_DIALOG_CONTEXT context;
    NTSTATUS status;
    HANDLE processHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION,
        ProcessItem->ProcessId
        );

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to open the process.", status, 0);
        return;
    }

    context = PhAllocateZero(sizeof(PH_IMAGE_MODIFIED_DIALOG_CONTEXT));
    context->RefCount = 1;
    context->ProcessItem = PhReferenceObject(ProcessItem);
    context->ParentWindowHandle = ParentWindowHandle;
    context->ProcessHandle = processHandle;

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_MODIFIEDPAGES),
        NULL,
        PhPageModifiedDlgProc,
        context
        );
}
