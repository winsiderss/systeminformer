/*
 * Process Hacker - 
 *   object search
 * 
 * Copyright (C) 2010 wj32
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

    WCHAR HandleString[PH_PTR_STR_LEN_1];

    SYSTEM_HANDLE_TABLE_ENTRY_INFO Info;
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

static HANDLE SearchThreadHandle = NULL;
static BOOLEAN SearchStop;
static PPH_STRING SearchString;
static PPH_LIST SearchResults = NULL;
static ULONG SearchResultsAddIndex;
static PH_MUTEX SearchResultsLock;

VOID PhShowFindObjectsDialog()
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
        BringWindowToTop(PhFindObjectsWindowHandle);
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

            PhRegisterDialog(hwndDlg);

            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Process");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Type");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Handle");

            PhSetExtendedListView(lvHandle);
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
                                PhFree(searchResult);
                            }

                            PhDereferenceObject(SearchResults);
                        }

                        // Start the search.

                        SearchString = PhGetWindowText(GetDlgItem(hwndDlg, IDC_FILTER));
                        SearchResults = PhCreateList(128);
                        SearchResultsAddIndex = 0;
                        PhInitializeMutex(&SearchResultsLock);

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
            case NM_RCLICK:
                {
                    if (header->hwndFrom == PhFindObjectsListViewHandle)
                    {
                        LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)header;
                        PPHP_OBJECT_SEARCH_RESULT *results;
                        ULONG numberOfResults;

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
                                itemActivate->ptAction
                                );
                            DestroyMenu(menu);
                        }

                        PhFree(results);
                    }
                }
                break;
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
            PhResizingMinimumSize((PRECT)lParam, wParam, 320, 280);
        }
        break;
    case WM_PH_SEARCH_UPDATE:
        {
            HWND lvHandle;
            ULONG i;

            lvHandle = GetDlgItem(hwndDlg, IDC_RESULTS);

            ExtendedListView_SetRedraw(lvHandle, FALSE);

            PhAcquireMutex(&SearchResultsLock);

            for (i = SearchResultsAddIndex; i < SearchResults->Count; i++)
            {
                PPHP_OBJECT_SEARCH_RESULT searchResult = SearchResults->Items[i];
                CLIENT_ID clientId;
                INT lvItemIndex;

                clientId.UniqueProcess = searchResult->ProcessId;
                clientId.UniqueThread = NULL;

                lvItemIndex = PhAddListViewItem(
                    lvHandle,
                    MAXINT,
                    ((PPH_STRING)PHA_DEREFERENCE(PhGetClientIdName(&clientId)))->Buffer,
                    searchResult
                    );
                PhSetListViewSubItem(lvHandle, lvItemIndex, 1, searchResult->TypeName->Buffer);
                PhSetListViewSubItem(lvHandle, lvItemIndex, 2, searchResult->Name->Buffer);
                PhSetListViewSubItem(lvHandle, lvItemIndex, 3, searchResult->HandleString);
            }

            SearchResultsAddIndex = i;

            PhReleaseMutex(&SearchResultsLock);

            ExtendedListView_SetRedraw(lvHandle, TRUE);
        }
        break;
    case WM_PH_SEARCH_FINISHED:
        {
            // Add any un-added items.
            SendMessage(hwndDlg, WM_PH_SEARCH_UPDATE, 0, 0);

            PhDereferenceObject(SearchString);
            PhDeleteMutex(&SearchResultsLock);

            WaitForSingleObject(SearchThreadHandle, INFINITE);
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

static NTSTATUS PhpFindObjectsThreadStart(
    __in PVOID Parameter
    )
{
    PSYSTEM_HANDLE_INFORMATION handles;
    PPH_HASHTABLE processHandleHashtable;
    ULONG64 searchPointer;
    BOOLEAN useSearchPointer;
    ULONG i;

    // Refuse to search with no filter.
    if (SearchString->Length == 0)
        goto Exit;

    PhLowerString(SearchString);

    // Try to get a search pointer from the search string.
    useSearchPointer = PhStringToInteger64(SearchString->Buffer, 0, &searchPointer);

    if (NT_SUCCESS(PhEnumHandles(&handles)))
    {
        processHandleHashtable = PhCreateSimpleHashtable(8);

        for (i = 0; i < handles->NumberOfHandles; i++)
        {
            PSYSTEM_HANDLE_TABLE_ENTRY_INFO handleInfo = &handles->Handles[i];
            PPVOID processHandlePtr;
            HANDLE processHandle;
            PPH_STRING typeName;
            PPH_STRING bestObjectName;

            if (SearchStop)
                break;

            // Open a handle to the process if we don't already have one.

            processHandlePtr = PhGetSimpleHashtableItem(
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
                    PhAddSimpleHashtableItem(
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
                PPH_STRING lowerBestObjectName;

                lowerBestObjectName = PhDuplicateString(bestObjectName);
                PhLowerString(lowerBestObjectName);

                if (
                    PhStringIndexOfString(lowerBestObjectName, 0, SearchString->Buffer) != -1 ||
                    (useSearchPointer && handleInfo->Object == (PVOID)searchPointer)
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

                    PhAcquireMutex(&SearchResultsLock);

                    PhAddListItem(SearchResults, searchResult);

                    // Update the search results in batches of 40.
                    if (SearchResults->Count % 40 == 0)
                        PostMessage(PhFindObjectsWindowHandle, WM_PH_SEARCH_UPDATE, 0, 0);

                    PhReleaseMutex(&SearchResultsLock);
                }
                else
                {
                    PhDereferenceObject(typeName);
                    PhDereferenceObject(bestObjectName);
                }

                PhDereferenceObject(lowerBestObjectName);
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

Exit:
    PostMessage(PhFindObjectsWindowHandle, WM_PH_SEARCH_FINISHED, 0, 0);

    return STATUS_SUCCESS;
}
