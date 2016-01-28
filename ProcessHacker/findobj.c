/*
 * Process Hacker -
 *   object search
 *
 * Copyright (C) 2010-2016 wj32
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
#include <emenu.h>
#include <kphuser.h>
#include <settings.h>
#include <procprpp.h>
#include "pcre/pcre2.h"
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

NTSTATUS PhpFindObjectsThreadStart(
    _In_ PVOID Parameter
    );

HWND PhFindObjectsWindowHandle = NULL;
HWND PhFindObjectsListViewHandle = NULL;
static PH_LAYOUT_MANAGER WindowLayoutManager;
static RECT MinimumSize;

static HANDLE SearchThreadHandle = NULL;
static BOOLEAN SearchStop;
static PPH_STRING SearchString;
static pcre2_code *SearchRegexCompiledExpression;
static pcre2_match_data *SearchRegexMatchData;
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

    SetForegroundWindow(PhFindObjectsWindowHandle);
}

VOID PhpInitializeFindObjMenu(
    _In_ PPH_EMENU Menu,
    _In_ PPHP_OBJECT_SEARCH_RESULT *Results,
    _In_ ULONG NumberOfResults
    )
{
    BOOLEAN allCanBeClosed = TRUE;
    ULONG i;

    if (NumberOfResults == 1)
    {
        PH_HANDLE_ITEM_INFO info;

        info.ProcessId = Results[0]->ProcessId;
        info.Handle = Results[0]->Handle;
        info.TypeName = Results[0]->TypeName;
        info.BestObjectName = Results[0]->Name;
        PhInsertHandleObjectPropertiesEMenuItems(Menu, ID_OBJECT_PROPERTIES, FALSE, &info);
    }
    else
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        PhEnableEMenuItem(Menu, ID_OBJECT_COPY, TRUE);
    }

    for (i = 0; i < NumberOfResults; i++)
    {
        if (Results[i]->ResultType != HandleSearchResult)
        {
            allCanBeClosed = FALSE;
            break;
        }
    }

    PhEnableEMenuItem(Menu, ID_OBJECT_CLOSE, allCanBeClosed);
}

INT NTAPI PhpObjectProcessCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
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
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PPHP_OBJECT_SEARCH_RESULT item1 = Item1;
    PPHP_OBJECT_SEARCH_RESULT item2 = Item2;

    return PhCompareString(item1->TypeName, item2->TypeName, TRUE);
}

INT NTAPI PhpObjectNameCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PPHP_OBJECT_SEARCH_RESULT item1 = Item1;
    PPHP_OBJECT_SEARCH_RESULT item2 = Item2;

    return PhCompareString(item1->Name, item2->Name, TRUE);
}

INT NTAPI PhpObjectHandleCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PPHP_OBJECT_SEARCH_RESULT item1 = Item1;
    PPHP_OBJECT_SEARCH_RESULT item2 = Item2;

    return uintptrcmp((ULONG_PTR)item1->Handle, (ULONG_PTR)item2->Handle);
}

static INT_PTR CALLBACK PhpFindObjectsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_REGEX),
                NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
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

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_REGEX), PhGetIntegerSetting(L"FindObjRegex") ? BST_CHECKED : BST_UNCHECKED);
        }
        break;
    case WM_DESTROY:
        {
            PhSetIntegerSetting(L"FindObjRegex", Button_GetCheck(GetDlgItem(hwndDlg, IDC_REGEX)) == BST_CHECKED);
            PhSaveWindowPlacementToSetting(L"FindObjWindowPosition", L"FindObjWindowSize", hwndDlg);
            PhSaveListViewColumnsToSetting(L"FindObjListViewColumns", PhFindObjectsListViewHandle);
        }
        break;
    case WM_SHOWWINDOW:
        {
            SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwndDlg, IDC_FILTER), TRUE);
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

                        PhMoveReference(&SearchString, PhGetWindowText(GetDlgItem(hwndDlg, IDC_FILTER)));

                        if (SearchRegexCompiledExpression)
                        {
                            pcre2_code_free(SearchRegexCompiledExpression);
                            SearchRegexCompiledExpression = NULL;
                        }

                        if (SearchRegexMatchData)
                        {
                            pcre2_match_data_free(SearchRegexMatchData);
                            SearchRegexMatchData = NULL;
                        }

                        if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_REGEX)) == BST_CHECKED)
                        {
                            int errorCode;
                            PCRE2_SIZE errorOffset;

                            SearchRegexCompiledExpression = pcre2_compile(
                                SearchString->Buffer,
                                SearchString->Length / sizeof(WCHAR),
                                PCRE2_CASELESS | PCRE2_DOTALL,
                                &errorCode,
                                &errorOffset,
                                NULL
                                );

                            if (!SearchRegexCompiledExpression)
                            {
                                PhShowError(hwndDlg, L"Unable to compile the regular expression: \"%s\" at position %zu.",
                                    PhGetStringOrDefault(PhAutoDereferenceObject(PhPcre2GetErrorMessage(errorCode)), L"Unknown error"),
                                    errorOffset
                                    );
                                break;
                            }

                            SearchRegexMatchData = pcre2_match_data_create_from_pattern(SearchRegexCompiledExpression, NULL);
                        }

                        // Clean up previous results.

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

                        SearchResults = PhCreateList(128);
                        SearchResultsAddIndex = 0;

                        SearchThreadHandle = PhCreateThread(0, PhpFindObjectsThreadStart, NULL);

                        if (!SearchThreadHandle)
                        {
                            PhClearReference(&SearchResults);
                            break;
                        }

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
            case ID_HANDLE_OBJECTPROPERTIES1:
            case ID_HANDLE_OBJECTPROPERTIES2:
                {
                    PPHP_OBJECT_SEARCH_RESULT result =
                        PhGetSelectedListViewItemParam(PhFindObjectsListViewHandle);

                    if (result)
                    {
                        PH_HANDLE_ITEM_INFO info;

                        info.ProcessId = result->ProcessId;
                        info.Handle = result->Handle;
                        info.TypeName = result->TypeName;
                        info.BestObjectName = result->Name;

                        if (LOWORD(wParam) == ID_HANDLE_OBJECTPROPERTIES1)
                            PhShowHandleObjectProperties1(hwndDlg, &info);
                        else
                            PhShowHandleObjectProperties2(hwndDlg, &info);
                    }
                }
                break;
            case ID_OBJECT_GOTOOWNINGPROCESS:
                {
                    PPHP_OBJECT_SEARCH_RESULT result =
                        PhGetSelectedListViewItemParam(PhFindObjectsListViewHandle);

                    if (result)
                    {
                        PPH_PROCESS_NODE processNode;

                        if (processNode = PhFindProcessNode(result->ProcessId))
                        {
                            ProcessHacker_SelectTabPage(PhMainWndHandle, 0);
                            ProcessHacker_SelectProcessNode(PhMainWndHandle, processNode);
                            ProcessHacker_ToggleVisible(PhMainWndHandle, TRUE);
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
                        case 'A':
                            if (GetKeyState(VK_CONTROL) < 0)
                                PhSetStateAllListViewItems(PhFindObjectsListViewHandle, LVIS_SELECTED, LVIS_SELECTED);
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
                    PPH_EMENU menu;

                    menu = PhCreateEMenu();
                    PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_FINDOBJ), 0);
                    PhSetFlagsEMenuItem(menu, ID_OBJECT_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

                    PhpInitializeFindObjMenu(menu, results, numberOfResults);
                    PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );
                    PhDestroyEMenu(menu);
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
                    PhSetReference(&searchResult->ProcessName, processItem->ProcessName);
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
            NTSTATUS handleSearchStatus = (NTSTATUS)wParam;

            // Add any un-added items.
            SendMessage(hwndDlg, WM_PH_SEARCH_UPDATE, 0, 0);

            NtWaitForSingleObject(SearchThreadHandle, FALSE, NULL);
            NtClose(SearchThreadHandle);
            SearchThreadHandle = NULL;
            SearchStop = FALSE;

            ExtendedListView_SortItems(GetDlgItem(hwndDlg, IDC_RESULTS));

            SetDlgItemText(hwndDlg, IDOK, L"Find");
            EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);

            SetCursor(LoadCursor(NULL, IDC_ARROW));

            if (handleSearchStatus == STATUS_INSUFFICIENT_RESOURCES)
            {
                PhShowWarning(
                    hwndDlg,
                    L"Unable to search for handles because the total number of handles on the system is too large. "
                    L"Please check if there are any processes with an extremely large number of handles open."
                    );
            }
        }
        break;
    }

    return FALSE;
}

static BOOLEAN MatchSearchString(
    _In_ PPH_STRINGREF Input
    )
{
    if (SearchRegexCompiledExpression && SearchRegexMatchData)
    {
        return pcre2_match(
            SearchRegexCompiledExpression,
            Input->Buffer,
            Input->Length / sizeof(WCHAR),
            0,
            0,
            SearchRegexMatchData,
            NULL
            ) >= 0;
    }
    else
    {
        return PhFindStringInStringRef(Input, &SearchString->sr, TRUE) != -1;
    }
}

typedef struct _SEARCH_HANDLE_CONTEXT
{
    BOOLEAN NeedToFree;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HandleInfo;
    HANDLE ProcessHandle;
} SEARCH_HANDLE_CONTEXT, *PSEARCH_HANDLE_CONTEXT;

static NTSTATUS NTAPI SearchHandleFunction(
    _In_ PVOID Parameter
    )
{
    PSEARCH_HANDLE_CONTEXT context = Parameter;
    PPH_STRING typeName;
    PPH_STRING bestObjectName;

    if (!SearchStop && NT_SUCCESS(PhGetHandleInformation(
        context->ProcessHandle,
        (HANDLE)context->HandleInfo->HandleValue,
        context->HandleInfo->ObjectTypeIndex,
        NULL,
        &typeName,
        NULL,
        &bestObjectName
        )))
    {
        PPH_STRING upperBestObjectName;

        upperBestObjectName = PhDuplicateString(bestObjectName);
        PhUpperString(upperBestObjectName);

        if (MatchSearchString(&upperBestObjectName->sr) ||
            (UseSearchPointer && context->HandleInfo->Object == (PVOID)SearchPointer))
        {
            PPHP_OBJECT_SEARCH_RESULT searchResult;

            searchResult = PhAllocate(sizeof(PHP_OBJECT_SEARCH_RESULT));
            searchResult->ProcessId = (HANDLE)context->HandleInfo->UniqueProcessId;
            searchResult->ResultType = HandleSearchResult;
            searchResult->Handle = (HANDLE)context->HandleInfo->HandleValue;
            searchResult->TypeName = typeName;
            searchResult->Name = bestObjectName;
            PhPrintPointer(searchResult->HandleString, (PVOID)searchResult->Handle);
            searchResult->Info = *context->HandleInfo;

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

    if (context->NeedToFree)
        PhFree(context);

    return STATUS_SUCCESS;
}

static BOOLEAN NTAPI EnumModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    )
{
    PPH_STRING upperFileName;

    upperFileName = PhDuplicateString(Module->FileName);
    PhUpperString(upperFileName);

    if (MatchSearchString(&upperFileName->sr) ||
        (UseSearchPointer && Module->BaseAddress == (PVOID)SearchPointer))
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
        PhSetReference(&searchResult->Name, Module->FileName);
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
    _In_ PVOID Parameter
    )
{
    NTSTATUS status = STATUS_SUCCESS;
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

    if (NT_SUCCESS(status = PhEnumHandlesEx(&handles)))
    {
        static PH_INITONCE initOnce = PH_INITONCE_INIT;
        static ULONG fileObjectTypeIndex = -1;

        BOOLEAN useWorkQueue = FALSE;
        PH_WORK_QUEUE workQueue;
        processHandleHashtable = PhCreateSimpleHashtable(8);

        if (!KphIsConnected() && WindowsVersion >= WINDOWS_VISTA)
        {
            useWorkQueue = TRUE;
            PhInitializeWorkQueue(&workQueue, 1, 20, 1000);

            if (PhBeginInitOnce(&initOnce))
            {
                UNICODE_STRING fileTypeName;

                RtlInitUnicodeString(&fileTypeName, L"File");
                fileObjectTypeIndex = PhGetObjectTypeNumber(&fileTypeName);
                PhEndInitOnce(&initOnce);
            }
        }

        for (i = 0; i < handles->NumberOfHandles; i++)
        {
            PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handleInfo = &handles->Handles[i];
            PVOID *processHandlePtr;
            HANDLE processHandle;

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

            if (useWorkQueue && handleInfo->ObjectTypeIndex == (USHORT)fileObjectTypeIndex)
            {
                PSEARCH_HANDLE_CONTEXT searchHandleContext;

                searchHandleContext = PhAllocate(sizeof(SEARCH_HANDLE_CONTEXT));
                searchHandleContext->NeedToFree = TRUE;
                searchHandleContext->HandleInfo = handleInfo;
                searchHandleContext->ProcessHandle = processHandle;
                PhQueueItemWorkQueue(&workQueue, SearchHandleFunction, searchHandleContext);
            }
            else
            {
                SEARCH_HANDLE_CONTEXT searchHandleContext;

                searchHandleContext.NeedToFree = FALSE;
                searchHandleContext.HandleInfo = handleInfo;
                searchHandleContext.ProcessHandle = processHandle;
                SearchHandleFunction(&searchHandleContext);
            }
        }

        if (useWorkQueue)
        {
            PhWaitForWorkQueue(&workQueue);
            PhDeleteWorkQueue(&workQueue);
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
    PostMessage(PhFindObjectsWindowHandle, WM_PH_SEARCH_FINISHED, status, 0);

    return STATUS_SUCCESS;
}
