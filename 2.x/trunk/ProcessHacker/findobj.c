/*
 * Process Hacker -
 *   object search
 *
 * Copyright (C) 2010-2011 wj32
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

#include <phapp.h>
#include <windowsx.h>

#define WM_PH_SEARCH_UPDATE (WM_APP + 801)
#define WM_PH_SEARCH_FINISHED (WM_APP + 802)

typedef enum _PHP_OBJECT_RESULT_TYPE
{
    HandleSearchResult,
    ModuleSearchResult,
    MappedFileSearchResult
} PHP_OBJECT_RESULT_TYPE;

typedef struct _PHP_OBJECT_SEARCH_RESULT
{
    HANDLE ProcessId;
    PHP_OBJECT_RESULT_TYPE ResultType;

    HANDLE Handle;
    PPH_STRING TypeName;
    PPH_STRING Name;
    PPH_STRING ProcessName;

    WCHAR HandleString[PH_PTR_STR_LEN_1];

    SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Info;
} PHP_OBJECT_SEARCH_RESULT, *PPHP_OBJECT_SEARCH_RESULT;

INT_PTR CALLBACK PhpFindObjectsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

NTSTATUS PhpFindObjectsThreadStart(
    __in PVOID Parameter
    );

HWND PhFindObjectsWindowHandle = NULL;
HWND PhFindObjectsListViewHandle = NULL;
static PH_LAYOUT_MANAGER WindowLayoutManager;
static RECT MinimumSize;

static HANDLE SearchThreadHandle = NULL;
static BOOLEAN SearchStop;
static PPH_STRING SearchString;
static PPH_LIST SearchResults = NULL;
static ULONG SearchResultsAddIndex;
static PH_QUEUED_LOCK SearchResultsLock = PH_QUEUED_LOCK_INIT;

static ULONG64 SearchPointer;
static BOOLEAN UseSearchPointer;

VOID PhShowFindObjectsDialog(
    VOID
    )
{
    if (!PhFindObjectsWindowHandle)
    {
        PhFindObjectsWindowHandle = CreateDialog(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_FINDOBJECTS),
            PhMainWndHandle,
            PhpFindObjectsDlgProc
            );
    }

    if (!IsWindowVisible(PhFindObjectsWindowHandle))
        ShowWindow(PhFindObjectsWindowHandle, SW_SHOW);
    else
        SetForegroundWindow(PhFindObjectsWindowHandle);
}

VOID PhpInitializeFindObjMenu(
    __in HMENU Menu,
    __in PPHP_OBJECT_SEARCH_RESULT *Results,
    __in ULONG NumberOfResults
    )
{
    BOOLEAN allCanBeClosed = TRUE;
    ULONG i;

    if (NumberOfResults == 1)
    {
        // Nothing
    }
    else
    {
        PhEnableAllMenuItems(Menu, FALSE);
        PhEnableMenuItem(Menu, ID_OBJECT_COPY, TRUE);
    }

    for (i = 0; i < NumberOfResults; i++)
    {
        if (Results[i]->ResultType != HandleSearchResult)
        {
            allCanBeClosed = FALSE;
            break;
        }
    }

    PhEnableMenuItem(Menu, ID_OBJECT_CLOSE, allCanBeClosed);
}

INT NTAPI PhpObjectProcessCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in_opt PVOID Context
    )
{
    PPHP_OBJECT_SEARCH_RESULT item1 = Item1;
    PPHP_OBJECT_SEARCH_RESULT item2 = Item2;
    INT result;

    result = PhCompareStringWithNull(item1->ProcessName, item2->ProcessName, TRUE);

    if (result != 0)
        return result;
    else
        return uintptrcmp((ULONG_PTR)item1->ProcessId, (ULONG_PTR)item2->ProcessId);
}

INT NTAPI PhpObjectTypeCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in_opt PVOID Context
    )
{
    PPHP_OBJECT_SEARCH_RESULT item1 = Item1;
    PPHP_OBJECT_SEARCH_RESULT item2 = Item2;

    return PhCompareString(item1->TypeName, item2->TypeName, TRUE);
}

INT NTAPI PhpObjectNameCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in_opt PVOID Context
    )
{
    PPHP_OBJECT_SEARCH_RESULT item1 = Item1;
    PPHP_OBJECT_SEARCH_RESULT item2 = Item2;

    return PhCompareString(item1->Name, item2->Name, TRUE);
}

INT NTAPI PhpObjectHandleCompareFunction(
    __in PVOID Item1,
    __in PVOID Item2,
    __in_opt PVOID Context
    )
{
    PPHP_OBJECT_SEARCH_RESULT item1 = Item1;
    PPHP_OBJECT_SEARCH_RESULT item2 = Item2;

    return uintptrcmp((ULONG_PTR)item1->Handle, (ULONG_PTR)item2->Handle);
}

static INT_PTR CALLBACK PhpFindObjectsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            PhFindObjectsListViewHandle = lvHandle = GetDlgItem(hwndDlg, IDC_RESULTS);

            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_FILTER),
                NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK),
                NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&WindowLayoutManager, lvHandle,
                NULL, PH_ANCHOR_ALL);

            MinimumSize.left = 0;
            MinimumSize.top = 0;
            MinimumSize.right = 150;
            MinimumSize.bottom = 100;
            MapDialogRect(hwndDlg, &MinimumSize);

            PhRegisterDialog(hwndDlg);

            PhLoadWindowPlacementFromSetting(L"FindObjWindowPosition", L"FindObjWindowSize", hwndDlg);

            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Process");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Type");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Handle");

            PhSetExtendedListView(lvHandle);
            ExtendedListView_SetSortFast(lvHandle, TRUE);
            ExtendedListView_SetCompareFunction(lvHandle, 0, PhpObjectProcessCompareFunction);
            ExtendedListView_SetCompareFunction(lvHandle, 1, PhpObjectTypeCompareFunction);
            ExtendedListView_SetCompareFunction(lvHandle, 2, PhpObjectNameCompareFunction);
            ExtendedListView_SetCompareFunction(lvHandle, 3, PhpObjectHandleCompareFunction);
            PhLoadListViewColumnsFromSetting(L"FindObjListViewColumns", lvHandle);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveWindowPlacementToSetting(L"FindObjWindowPosition", L"FindObjWindowSize", hwndDlg);
            PhSaveListViewColumnsToSetting(L"FindObjListViewColumns", PhFindObjectsListViewHandle);
        }
        break;
    case WM_SHOWWINDOW:
        {
            SetFocus(GetDlgItem(hwndDlg, IDC_FILTER));
            Edit_SetSel(GetDlgItem(hwndDlg, IDC_FILTER), 0, -1);
        }
        break;
    case WM_CLOSE:
        {
            ShowWindow(hwndDlg, SW_HIDE);
            // IMPORTANT
            // Set the result to 0 so the default dialog message
            // handler doesn't invoke IDCANCEL, which will send
            // WM_CLOSE, creating an infinite loop.
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0);
        }
        return TRUE;
    case WM_SETCURSOR:
        {
            if (SearchThreadHandle)
            {
                SetCursor(LoadCursor(NULL, IDC_WAIT));
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                return TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDOK:
                {
                    // Don't continue if the user requested cancellation.
                    if (SearchStop)
                        break;

                    if (!SearchThreadHandle)
                    {
                        ULONG i;

                        // Cleanup previous results.

                        ListView_DeleteAllItems(PhFindObjectsListViewHandle);

                        if (SearchResults)
                        {
                            for (i = 0; i < SearchResults->Count; i++)
                            {
                                PPHP_OBJECT_SEARCH_RESULT searchResult = SearchResults->Items[i];

                                PhDereferenceObject(searchResult->TypeName);
                                PhDereferenceObject(searchResult->Name);

                                if (searchResult->ProcessName)
                                    PhDereferenceObject(searchResult->ProcessName);

                                PhFree(searchResult);
                            }

                            PhDereferenceObject(SearchResults);
                        }

                        // Start the search.

                        SearchString = PhGetWindowText(GetDlgItem(hwndDlg, IDC_FILTER));
                        SearchResults = PhCreateList(128);
                        SearchResultsAddIndex = 0;

                        SearchThreadHandle = PhCreateThread(0, PhpFindObjectsThreadStart, NULL);

                        if (!SearchThreadHandle)
                            break;

                        SetDlgItemText(hwndDlg, IDOK, L"Cancel");

                        SetCursor(LoadCursor(NULL, IDC_WAIT));
                    }
                    else
                    {
                        SearchStop = TRUE;
                        EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
                    }
                }
                break;
            case IDCANCEL:
                {
                    SendMessage(hwndDlg, WM_CLOSE, 0, 0);
                }
                break;
            case ID_OBJECT_CLOSE:
                {
                    PPHP_OBJECT_SEARCH_RESULT *results;
                    ULONG numberOfResults;
                    ULONG i;

                    PhGetSelectedListViewItemParams(
                        PhFindObjectsListViewHandle,
                        &results,
                        &numberOfResults
                        );

                    if (numberOfResults != 0 && PhShowConfirmMessage(
                        hwndDlg,
                        L"close",
                        numberOfResults == 1 ? L"the selected handle" : L"the selected handles",
                        L"Closing handles may cause system instability and data corruption.",
                        FALSE
                        ))
                    {
                        for (i = 0; i < numberOfResults; i++)
                        {
                            NTSTATUS status;
                            HANDLE processHandle;

                            if (results[i]->ResultType != HandleSearchResult)
                                continue;

                            if (NT_SUCCESS(status = PhOpenProcess(
                                &processHandle,
                                PROCESS_DUP_HANDLE,
                                results[i]->ProcessId
                                )))
                            {
                                if (NT_SUCCESS(status = PhDuplicateObject(
                                    processHandle,
                                    results[i]->Handle,
                                    NULL,
                                    NULL,
                                    0,
                                    0,
                                    DUPLICATE_CLOSE_SOURCE
                                    )))
                                {
                                    PhRemoveListViewItem(PhFindObjectsListViewHandle,
                                        PhFindListViewItemByParam(PhFindObjectsListViewHandle, 0, results[i]));
                                }

                                NtClose(processHandle);
                            }

                            if (!NT_SUCCESS(status))
                            {
                                if (!PhShowContinueStatus(hwndDlg,
                                    PhaFormatString(L"Unable to close \"%s\"", results[i]->Name->Buffer)->Buffer,
                                    status,
                                    0
                                    ))
                                    break;
                            }
                        }
                    }

                    PhFree(results);
                }
                break;
            case ID_OBJECT_PROCESSPROPERTIES:
                {
                    PPHP_OBJECT_SEARCH_RESULT result =
                        PhGetSelectedListViewItemParam(PhFindObjectsListViewHandle);

                    if (result)
                    {
                        PPH_PROCESS_ITEM processItem;

                        if (processItem = PhReferenceProcessItem(result->ProcessId))
                        {
                            ProcessHacker_ShowProcessProperties(PhMainWndHandle, processItem);
                            PhDereferenceObject(processItem);
                        }
                    }
                }
                break;
            case ID_OBJECT_PROPERTIES:
                {
                    PPHP_OBJECT_SEARCH_RESULT result =
                        PhGetSelectedListViewItemParam(PhFindObjectsListViewHandle);

                    if (result)
                    {
                        if (result->ResultType == HandleSearchResult)
                        {
                            PPH_HANDLE_ITEM handleItem;

                            handleItem = PhCreateHandleItem(&result->Info);

                            handleItem->BestObjectName = handleItem->ObjectName = result->Name;
                            PhReferenceObjectEx(result->Name, 2);

                            handleItem->TypeName = result->TypeName;
                            PhReferenceObject(result->TypeName);

                            PhShowHandleProperties(
                                hwndDlg,
                                result->ProcessId,
                                handleItem
                                );
                            PhDereferenceObject(handleItem);
                        }
                        else
                        {
                            // DLL or Mapped File. Just show file properties.
                            PhShellProperties(hwndDlg, result->Name->Buffer);
                        }
                    }
                }
                break;
            case ID_OBJECT_COPY:
                {
                    PhCopyListView(PhFindObjectsListViewHandle);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == PhFindObjectsListViewHandle)
                    {
                        SendMessage(hwndDlg, WM_COMMAND, ID_OBJECT_PROPERTIES, 0);
                    }
                }
                break;
            case LVN_KEYDOWN:
                {
                    if (header->hwndFrom == PhFindObjectsListViewHandle)
                    {
                        LPNMLVKEYDOWN keyDown = (LPNMLVKEYDOWN)header;

                        switch (keyDown->wVKey)
                        {
                        case 'C':
                            if (GetKeyState(VK_CONTROL) < 0)
                                SendMessage(hwndDlg, WM_COMMAND, ID_OBJECT_COPY, 0);
                            break;
                        case VK_DELETE:
                            SendMessage(hwndDlg, WM_COMMAND, ID_OBJECT_CLOSE, 0);
                            break;
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == PhFindObjectsListViewHandle)
            {
                POINT point;
                PPHP_OBJECT_SEARCH_RESULT *results;
                ULONG numberOfResults;

                point.x = (SHORT)LOWORD(lParam);
                point.y = (SHORT)HIWORD(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint((HWND)wParam, &point);

                PhGetSelectedListViewItemParams(PhFindObjectsListViewHandle, &results, &numberOfResults);

                if (numberOfResults != 0)
                {
                    HMENU menu;
                    HMENU subMenu;

                    menu = LoadMenu(PhInstanceHandle, MAKEINTRESOURCE(IDR_FINDOBJ));
                    subMenu = GetSubMenu(menu, 0);

                    SetMenuDefaultItem(subMenu, ID_OBJECT_PROPERTIES, FALSE);
                    PhpInitializeFindObjMenu(
                        subMenu,
                        results,
                        numberOfResults
                        );

                    PhShowContextMenu(
                        hwndDlg,
                        PhFindObjectsListViewHandle,
                        subMenu,
                        point
                        );
                    DestroyMenu(menu);
                }

                PhFree(results);
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&WindowLayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, MinimumSize.right, MinimumSize.bottom);
        }
        break;
    case WM_PH_SEARCH_UPDATE:
        {
            HWND lvHandle;
            ULONG i;

            lvHandle = GetDlgItem(hwndDlg, IDC_RESULTS);

            ExtendedListView_SetRedraw(lvHandle, FALSE);

            PhAcquireQueuedLockExclusive(&SearchResultsLock);

            for (i = SearchResultsAddIndex; i < SearchResults->Count; i++)
            {
                PPHP_OBJECT_SEARCH_RESULT searchResult = SearchResults->Items[i];
                CLIENT_ID clientId;
                PPH_PROCESS_ITEM processItem;
                PPH_STRING clientIdName;
                INT lvItemIndex;

                clientId.UniqueProcess = searchResult->ProcessId;
                clientId.UniqueThread = NULL;

                processItem = PhReferenceProcessItem(clientId.UniqueProcess);
                clientIdName = PhGetClientIdNameEx(&clientId, processItem ? processItem->ProcessName : NULL);

                lvItemIndex = PhAddListViewItem(
                    lvHandle,
                    MAXINT,
                    clientIdName->Buffer,
                    searchResult
                    );

                PhDereferenceObject(clientIdName);

                if (processItem)
                {
                    searchResult->ProcessName = processItem->ProcessName;
                    PhReferenceObject(searchResult->ProcessName);
                    PhDereferenceObject(processItem);
                }
                else
                {
                    searchResult->ProcessName = NULL;
                }

                PhSetListViewSubItem(lvHandle, lvItemIndex, 1, searchResult->TypeName->Buffer);
                PhSetListViewSubItem(lvHandle, lvItemIndex, 2, searchResult->Name->Buffer);
                PhSetListViewSubItem(lvHandle, lvItemIndex, 3, searchResult->HandleString);
            }

            SearchResultsAddIndex = i;

            PhReleaseQueuedLockExclusive(&SearchResultsLock);

            ExtendedListView_SetRedraw(lvHandle, TRUE);
        }
        break;
    case WM_PH_SEARCH_FINISHED:
        {
            // Add any un-added items.
            SendMessage(hwndDlg, WM_PH_SEARCH_UPDATE, 0, 0);

            PhDereferenceObject(SearchString);

            NtWaitForSingleObject(SearchThreadHandle, FALSE, NULL);
            NtClose(SearchThreadHandle);
            SearchThreadHandle = NULL;
            SearchStop = FALSE;

            ExtendedListView_SortItems(GetDlgItem(hwndDlg, IDC_RESULTS));

            SetDlgItemText(hwndDlg, IDOK, L"Find");
            EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);

            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
        break;
    }

    return FALSE;
}

static BOOLEAN NTAPI EnumModulesCallback(
    __in PPH_MODULE_INFO Module,
    __in_opt PVOID Context
    )
{
    PPH_STRING upperFileName;

    upperFileName = PhDuplicateString(Module->FileName);
    PhUpperString(upperFileName);

    if (
        PhFindStringInString(upperFileName, 0, SearchString->Buffer) != -1 ||
        (UseSearchPointer && Module->BaseAddress == (PVOID)SearchPointer)
        )
    {
        PPHP_OBJECT_SEARCH_RESULT searchResult;
        PWSTR typeName;

        switch (Module->Type)
        {
        case PH_MODULE_TYPE_MAPPED_FILE:
            typeName = L"Mapped File";
            break;
        case PH_MODULE_TYPE_MAPPED_IMAGE:
            typeName = L"Mapped Image";
            break;
        default:
            typeName = L"DLL";
            break;
        }

        searchResult = PhAllocate(sizeof(PHP_OBJECT_SEARCH_RESULT));
        searchResult->ProcessId = (HANDLE)Context;
        searchResult->ResultType = (Module->Type == PH_MODULE_TYPE_MAPPED_FILE || Module->Type == PH_MODULE_TYPE_MAPPED_IMAGE) ? MappedFileSearchResult : ModuleSearchResult;
        searchResult->Handle = (HANDLE)Module->BaseAddress;
        searchResult->TypeName = PhCreateString(typeName);
        PhReferenceObject(Module->FileName);
        searchResult->Name = Module->FileName;
        PhPrintPointer(searchResult->HandleString, Module->BaseAddress);
        memset(&searchResult->Info, 0, sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX));

        PhAcquireQueuedLockExclusive(&SearchResultsLock);

        PhAddItemList(SearchResults, searchResult);

        // Update the search results in batches of 40.
        if (SearchResults->Count % 40 == 0)
            PostMessage(PhFindObjectsWindowHandle, WM_PH_SEARCH_UPDATE, 0, 0);

        PhReleaseQueuedLockExclusive(&SearchResultsLock);
    }

    PhDereferenceObject(upperFileName);

    return TRUE;
}

static NTSTATUS PhpFindObjectsThreadStart(
    __in PVOID Parameter
    )
{
    PSYSTEM_HANDLE_INFORMATION_EX handles;
    PPH_HASHTABLE processHandleHashtable;
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;
    ULONG i;

    // Refuse to search with no filter.
    if (SearchString->Length == 0)
        goto Exit;

    // Try to get a search pointer from the search string.
    UseSearchPointer = PhStringToInteger64(&SearchString->sr, 0, &SearchPointer);

    PhUpperString(SearchString);

    if (NT_SUCCESS(PhEnumHandlesEx(&handles)))
    {
        processHandleHashtable = PhCreateSimpleHashtable(8);

        for (i = 0; i < handles->NumberOfHandles; i++)
        {
            PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handleInfo = &handles->Handles[i];
            PPVOID processHandlePtr;
            HANDLE processHandle;
            PPH_STRING typeName;
            PPH_STRING bestObjectName;

            if (SearchStop)
                break;

            // Open a handle to the process if we don't already have one.

            processHandlePtr = PhFindItemSimpleHashtable(
                processHandleHashtable,
                (PVOID)handleInfo->UniqueProcessId
                );

            if (processHandlePtr)
            {
                processHandle = (HANDLE)*processHandlePtr;
            }
            else
            {
                if (NT_SUCCESS(PhOpenProcess(
                    &processHandle,
                    PROCESS_DUP_HANDLE,
                    (HANDLE)handleInfo->UniqueProcessId
                    )))
                {
                    PhAddItemSimpleHashtable(
                        processHandleHashtable,
                        (PVOID)handleInfo->UniqueProcessId,
                        processHandle
                        );
                }
                else
                {
                    continue;
                }
            }

            // Get handle information.

            if (NT_SUCCESS(PhGetHandleInformation(
                processHandle,
                (HANDLE)handleInfo->HandleValue,
                handleInfo->ObjectTypeIndex,
                NULL,
                &typeName,
                NULL,
                &bestObjectName
                )))
            {
                PPH_STRING upperBestObjectName;

                upperBestObjectName = PhDuplicateString(bestObjectName);
                PhUpperString(upperBestObjectName);

                if (
                    PhFindStringInString(upperBestObjectName, 0, SearchString->Buffer) != -1 ||
                    (UseSearchPointer && handleInfo->Object == (PVOID)SearchPointer)
                    )
                {
                    PPHP_OBJECT_SEARCH_RESULT searchResult;

                    searchResult = PhAllocate(sizeof(PHP_OBJECT_SEARCH_RESULT));
                    searchResult->ProcessId = (HANDLE)handleInfo->UniqueProcessId;
                    searchResult->ResultType = HandleSearchResult;
                    searchResult->Handle = (HANDLE)handleInfo->HandleValue;
                    searchResult->TypeName = typeName;
                    searchResult->Name = bestObjectName;
                    PhPrintPointer(searchResult->HandleString, (PVOID)searchResult->Handle);
                    searchResult->Info = *handleInfo;

                    PhAcquireQueuedLockExclusive(&SearchResultsLock);

                    PhAddItemList(SearchResults, searchResult);

                    // Update the search results in batches of 40.
                    if (SearchResults->Count % 40 == 0)
                        PostMessage(PhFindObjectsWindowHandle, WM_PH_SEARCH_UPDATE, 0, 0);

                    PhReleaseQueuedLockExclusive(&SearchResultsLock);
                }
                else
                {
                    PhDereferenceObject(typeName);
                    PhDereferenceObject(bestObjectName);
                }

                PhDereferenceObject(upperBestObjectName);
            }
        }

        {
            PPH_KEY_VALUE_PAIR entry;

            i = 0;

            while (PhEnumHashtable(processHandleHashtable, &entry, &i))
                NtClose((HANDLE)entry->Value);
        }

        PhDereferenceObject(processHandleHashtable);
        PhFree(handles);
    }

    if (NT_SUCCESS(PhEnumProcesses(&processes)))
    {
        process = PH_FIRST_PROCESS(processes);

        do
        {
            PhEnumGenericModules(
                process->UniqueProcessId,
                NULL,
                PH_ENUM_GENERIC_MAPPED_FILES | PH_ENUM_GENERIC_MAPPED_IMAGES,
                EnumModulesCallback,
                (PVOID)process->UniqueProcessId
                );
        } while (process = PH_NEXT_PROCESS(process));

        PhFree(processes);
    }

Exit:
    PostMessage(PhFindObjectsWindowHandle, WM_PH_SEARCH_FINISHED, 0, 0);

    return STATUS_SUCCESS;
}
