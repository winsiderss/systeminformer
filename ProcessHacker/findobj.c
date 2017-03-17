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

#include <windowsx.h>

#include <emenu.h>
#include <hndlinfo.h>
#include <kphuser.h>
#include <workqueue.h>

#include <hndlmenu.h>
#include <hndlprv.h>
#include <mainwnd.h>
#include <procprv.h>
#include <proctree.h>
#include <settings.h>

#include "pcre/pcre2.h"

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

typedef struct _PHP_OBJECT_SEARCH_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HWND FilterHandle;
    PH_LAYOUT_MANAGER WindowLayoutManager;
    RECT MinimumSize;

    HANDLE SearchThreadHandle;
    BOOLEAN SearchStop;
    PPH_STRING SearchString;
    pcre2_code *SearchRegexCompiledExpression;
    pcre2_match_data *SearchRegexMatchData;
    PPH_LIST SearchResults;
    ULONG SearchResultsAddIndex;
    PH_QUEUED_LOCK SearchResultsLock;// = PH_QUEUED_LOCK_INIT;

    ULONG64 SearchPointer;
    BOOLEAN UseSearchPointer;
} PHP_OBJECT_SEARCH_CONTEXT, *PPHP_OBJECT_SEARCH_CONTEXT;

INT_PTR CALLBACK PhpFindObjectsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

NTSTATUS PhpFindObjectsThreadStart(
    _In_ PVOID Parameter
    );

VOID PhShowFindObjectsDialog(
    VOID
    )
{
    HWND findObjectsWindowHandle = CreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_FINDOBJECTS),
        NULL,
        PhpFindObjectsDlgProc
        );

    if (!IsWindowVisible(findObjectsWindowHandle))
        ShowWindow(findObjectsWindowHandle, SW_SHOW);

    SetForegroundWindow(findObjectsWindowHandle);
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
    PPHP_OBJECT_SEARCH_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhCreateAlloc(sizeof(PHP_OBJECT_SEARCH_CONTEXT));
        memset(context, 0, sizeof(PHP_OBJECT_SEARCH_CONTEXT));

        SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)context);
    }
    else
    {
        context = (PPHP_OBJECT_SEARCH_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());

        if (uMsg == WM_DESTROY)
        {
            RemoveProp(hwndDlg, PhMakeContextAtom());
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_RESULTS);
            context->FilterHandle = GetDlgItem(hwndDlg, IDC_FILTER);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            PhInitializeLayoutManager(&context->WindowLayoutManager, hwndDlg);
            PhAddLayoutItem(&context->WindowLayoutManager, context->FilterHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->WindowLayoutManager, GetDlgItem(hwndDlg, IDC_REGEX), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->WindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->WindowLayoutManager, context->ListViewHandle,  NULL, PH_ANCHOR_ALL);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 150;
            context->MinimumSize.bottom = 100;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            PhRegisterDialog(hwndDlg);

            PhLoadWindowPlacementFromSetting(L"FindObjWindowPosition", L"FindObjWindowSize", hwndDlg);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Process");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Type");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Handle");

            PhSetExtendedListView(context->ListViewHandle);
            ExtendedListView_SetSortFast(context->ListViewHandle, TRUE);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 0, PhpObjectProcessCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 1, PhpObjectTypeCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 2, PhpObjectNameCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 3, PhpObjectHandleCompareFunction);
            PhLoadListViewColumnsFromSetting(L"FindObjListViewColumns", context->ListViewHandle);

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_REGEX), PhGetIntegerSetting(L"FindObjRegex") ? BST_CHECKED : BST_UNCHECKED);
        }
        break;
    case WM_DESTROY:
        {
            PhSetIntegerSetting(L"FindObjRegex", Button_GetCheck(GetDlgItem(hwndDlg, IDC_REGEX)) == BST_CHECKED);
            PhSaveWindowPlacementToSetting(L"FindObjWindowPosition", L"FindObjWindowSize", hwndDlg);
            PhSaveListViewColumnsToSetting(L"FindObjListViewColumns", context->ListViewHandle);
            //PostQuitMessage(0);
        }
        break;
    case WM_SHOWWINDOW:
        {
            SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)context->FilterHandle, TRUE);
            Edit_SetSel(context->FilterHandle, 0, -1);
        }
        break;
    case WM_SETCURSOR:
        {
            if (context->SearchThreadHandle)
            {
                SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
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
                    if (context->SearchStop)
                        break;

                    if (!context->SearchThreadHandle)
                    {
                        ULONG i;

                        PhMoveReference(&context->SearchString, PhGetWindowText(context->FilterHandle));

                        if (context->SearchRegexCompiledExpression)
                        {
                            pcre2_code_free(context->SearchRegexCompiledExpression);
                            context->SearchRegexCompiledExpression = NULL;
                        }

                        if (context->SearchRegexMatchData)
                        {
                            pcre2_match_data_free(context->SearchRegexMatchData);
                            context->SearchRegexMatchData = NULL;
                        }

                        if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_REGEX)) == BST_CHECKED)
                        {
                            int errorCode;
                            PCRE2_SIZE errorOffset;

                            context->SearchRegexCompiledExpression = pcre2_compile(
                                context->SearchString->Buffer,
                                context->SearchString->Length / sizeof(WCHAR),
                                PCRE2_CASELESS | PCRE2_DOTALL,
                                &errorCode,
                                &errorOffset,
                                NULL
                                );

                            if (!context->SearchRegexCompiledExpression)
                            {
                                PhShowError(hwndDlg, L"Unable to compile the regular expression: \"%s\" at position %zu.",
                                    PhGetStringOrDefault(PH_AUTO(PhPcre2GetErrorMessage(errorCode)), L"Unknown error"),
                                    errorOffset
                                    );
                                break;
                            }

                            context->SearchRegexMatchData = pcre2_match_data_create_from_pattern(context->SearchRegexCompiledExpression, NULL);
                        }

                        // Clean up previous results.

                        ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
                        ListView_DeleteAllItems(context->ListViewHandle);
                        ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);

                        if (context->SearchResults)
                        {
                            for (i = 0; i < context->SearchResults->Count; i++)
                            {
                                PPHP_OBJECT_SEARCH_RESULT searchResult = context->SearchResults->Items[i];

                                PhDereferenceObject(searchResult->TypeName);
                                PhDereferenceObject(searchResult->Name);

                                if (searchResult->ProcessName)
                                    PhDereferenceObject(searchResult->ProcessName);

                                PhFree(searchResult);
                            }

                            PhDereferenceObject(context->SearchResults);
                        }

                        // Start the search.

                        context->SearchResults = PhCreateList(128);
                        context->SearchResultsAddIndex = 0;

                        PhReferenceObject(context);
                        context->SearchThreadHandle = PhCreateThread(0, PhpFindObjectsThreadStart, context);

                        if (!context->SearchThreadHandle)
                        {
                            PhDereferenceObject(context);
                            PhClearReference(&context->SearchResults);
                            break;
                        }

                        SetDlgItemText(hwndDlg, IDOK, L"Cancel");

                        SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
                    }
                    else
                    {
                        context->SearchStop = TRUE;
                        EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
                    }
                }
                break;
            case IDCANCEL:
                {
                    DestroyWindow(hwndDlg);
                }
                break;
            case ID_OBJECT_CLOSE:
                {
                    PPHP_OBJECT_SEARCH_RESULT *results;
                    ULONG numberOfResults;
                    ULONG i;

                    PhGetSelectedListViewItemParams(
                        context->ListViewHandle,
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
                                if (NT_SUCCESS(status = NtDuplicateObject(
                                    processHandle,
                                    results[i]->Handle,
                                    NULL,
                                    NULL,
                                    0,
                                    0,
                                    DUPLICATE_CLOSE_SOURCE
                                    )))
                                {
                                    PhRemoveListViewItem(context->ListViewHandle,
                                        PhFindListViewItemByParam(context->ListViewHandle, 0, results[i]));
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
                        PhGetSelectedListViewItemParam(context->ListViewHandle);

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
                        PhGetSelectedListViewItemParam(context->ListViewHandle);

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
                        PhGetSelectedListViewItemParam(context->ListViewHandle);

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
                    PhCopyListView(context->ListViewHandle);
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
                    if (header->hwndFrom == context->ListViewHandle)
                    {
                        SendMessage(hwndDlg, WM_COMMAND, ID_OBJECT_PROPERTIES, 0);
                    }
                }
                break;
            case LVN_KEYDOWN:
                {
                    if (header->hwndFrom == context->ListViewHandle)
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
                                PhSetStateAllListViewItems(context->ListViewHandle, LVIS_SELECTED, LVIS_SELECTED);
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
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPHP_OBJECT_SEARCH_RESULT *results;
                ULONG numberOfResults;

                point.x = (SHORT)LOWORD(lParam);
                point.y = (SHORT)HIWORD(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint((HWND)wParam, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &results, &numberOfResults);

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
            PhLayoutManagerLayout(&context->WindowLayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, context->MinimumSize.right, context->MinimumSize.bottom);
        }
        break;
    case WM_PH_SEARCH_UPDATE:
        {
            ULONG i;

            ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);

            PhAcquireQueuedLockExclusive(&context->SearchResultsLock);

            for (i = context->SearchResultsAddIndex; i < context->SearchResults->Count; i++)
            {
                PPHP_OBJECT_SEARCH_RESULT searchResult = context->SearchResults->Items[i];
                CLIENT_ID clientId;
                PPH_PROCESS_ITEM processItem;
                PPH_STRING clientIdName;
                INT lvItemIndex;

                clientId.UniqueProcess = searchResult->ProcessId;
                clientId.UniqueThread = NULL;

                processItem = PhReferenceProcessItem(clientId.UniqueProcess);
                clientIdName = PhGetClientIdNameEx(&clientId, processItem ? processItem->ProcessName : NULL);

                lvItemIndex = PhAddListViewItem(
                    context->ListViewHandle,
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

                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 1, searchResult->TypeName->Buffer);
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 2, searchResult->Name->Buffer);
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 3, searchResult->HandleString);
            }

            context->SearchResultsAddIndex = i;

            PhReleaseQueuedLockExclusive(&context->SearchResultsLock);

            ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);
        }
        break;
    case WM_PH_SEARCH_FINISHED:
        {
            NTSTATUS handleSearchStatus = (NTSTATUS)wParam;

            // Add any un-added items.
            SendMessage(hwndDlg, WM_PH_SEARCH_UPDATE, 0, 0);

            NtWaitForSingleObject(context->SearchThreadHandle, FALSE, NULL);
            NtClose(context->SearchThreadHandle);
            context->SearchThreadHandle = NULL;
            context->SearchStop = FALSE;

            ExtendedListView_SortItems(context->ListViewHandle);

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
    _In_ PPHP_OBJECT_SEARCH_CONTEXT Context,
    _In_ PPH_STRINGREF Input
    )
{
    if (Context->SearchRegexCompiledExpression && Context->SearchRegexMatchData)
    {
        return pcre2_match(
            Context->SearchRegexCompiledExpression,
            Input->Buffer,
            Input->Length / sizeof(WCHAR),
            0,
            0,
            Context->SearchRegexMatchData,
            NULL
            ) >= 0;
    }
    else
    {
        return PhFindStringInStringRef(Input, &Context->SearchString->sr, TRUE) != -1;
    }
}

typedef struct _SEARCH_HANDLE_CONTEXT
{
    BOOLEAN NeedToFree;
    HANDLE ProcessHandle;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HandleInfo;
    PPHP_OBJECT_SEARCH_CONTEXT ParentContext;
} SEARCH_HANDLE_CONTEXT, *PSEARCH_HANDLE_CONTEXT;

static NTSTATUS NTAPI SearchHandleFunction(
    _In_ PVOID Parameter
    )
{
    PSEARCH_HANDLE_CONTEXT context = Parameter;
    PPH_STRING typeName;
    PPH_STRING bestObjectName;

    if (!context->ParentContext->SearchStop && NT_SUCCESS(PhGetHandleInformation(
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
        _wcsupr(upperBestObjectName->Buffer);

        if (MatchSearchString(context->ParentContext, &upperBestObjectName->sr) ||
            (context->ParentContext->UseSearchPointer && context->HandleInfo->Object == (PVOID)context->ParentContext->SearchPointer))
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

            PhAcquireQueuedLockExclusive(&context->ParentContext->SearchResultsLock);

            PhAddItemList(context->ParentContext->SearchResults, searchResult);

            // Update the search results in batches of 40.
            if (context->ParentContext->SearchResults->Count % 40 == 0)
                PostMessage(context->ParentContext->WindowHandle, WM_PH_SEARCH_UPDATE, 0, 0);

            PhReleaseQueuedLockExclusive(&context->ParentContext->SearchResultsLock);
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

typedef struct _SEARCH_MODULE_CONTEXT
{
    HANDLE ProcessId;
    PPHP_OBJECT_SEARCH_CONTEXT ParentContext;
} SEARCH_MODULE_CONTEXT, *PSEARCH_MODULE_CONTEXT;

static BOOLEAN NTAPI EnumModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    )
{
    PPH_STRING upperFileName;
    PSEARCH_MODULE_CONTEXT context = Context;

    upperFileName = PhDuplicateString(Module->FileName);
    _wcsupr(upperFileName->Buffer);

    if (MatchSearchString(context->ParentContext, &upperFileName->sr) ||
        (context->ParentContext->UseSearchPointer && Module->BaseAddress == (PVOID)context->ParentContext->SearchPointer))
    {
        PPHP_OBJECT_SEARCH_RESULT searchResult;
        PWSTR typeName;

        switch (Module->Type)
        {
        case PH_MODULE_TYPE_MAPPED_FILE:
            typeName = L"Mapped file";
            break;
        case PH_MODULE_TYPE_MAPPED_IMAGE:
            typeName = L"Mapped image";
            break;
        default:
            typeName = L"DLL";
            break;
        }

        searchResult = PhAllocate(sizeof(PHP_OBJECT_SEARCH_RESULT));
        searchResult->ProcessId = context->ProcessId;
        searchResult->ResultType = (Module->Type == PH_MODULE_TYPE_MAPPED_FILE || Module->Type == PH_MODULE_TYPE_MAPPED_IMAGE) ? MappedFileSearchResult : ModuleSearchResult;
        searchResult->Handle = (HANDLE)Module->BaseAddress;
        searchResult->TypeName = PhCreateString(typeName);
        PhSetReference(&searchResult->Name, Module->FileName);
        PhPrintPointer(searchResult->HandleString, Module->BaseAddress);
        memset(&searchResult->Info, 0, sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX));

        PhAcquireQueuedLockExclusive(&context->ParentContext->SearchResultsLock);

        PhAddItemList(context->ParentContext->SearchResults, searchResult);

        // Update the search results in batches of 40.
        if (context->ParentContext->SearchResults->Count % 40 == 0)
            PostMessage(context->ParentContext->WindowHandle, WM_PH_SEARCH_UPDATE, 0, 0);

        PhReleaseQueuedLockExclusive(&context->ParentContext->SearchResultsLock);
    }

    PhDereferenceObject(upperFileName);

    return TRUE;
}

static NTSTATUS PhpFindObjectsThreadStart(
    _In_ PVOID Parameter
    )
{
    PPHP_OBJECT_SEARCH_CONTEXT context = Parameter;
    NTSTATUS status = STATUS_SUCCESS;
    PSYSTEM_HANDLE_INFORMATION_EX handles;
    PPH_HASHTABLE processHandleHashtable;
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;
    ULONG i;

    // Refuse to search with no filter.
    if (context->SearchString->Length == 0)
        goto Exit;

    // Try to get a search pointer from the search string.
    context->UseSearchPointer = PhStringToInteger64(&context->SearchString->sr, 0, &context->SearchPointer);

    _wcsupr(context->SearchString->Buffer);

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

            if (context->SearchStop)
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
                searchHandleContext->ParentContext = context;

                PhReferenceObject(context);
                PhQueueItemWorkQueue(&workQueue, SearchHandleFunction, searchHandleContext);
            }
            else
            {
                SEARCH_HANDLE_CONTEXT searchHandleContext;

                searchHandleContext.NeedToFree = FALSE;
                searchHandleContext.HandleInfo = handleInfo;
                searchHandleContext.ProcessHandle = processHandle;
                searchHandleContext.ParentContext = context;

                PhReferenceObject(context);
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
            SEARCH_MODULE_CONTEXT searchModuleContext;

            searchModuleContext.ParentContext = context;
            searchModuleContext.ProcessId = process->UniqueProcessId;

            if (context->SearchStop)
                break;

            PhEnumGenericModules(
                process->UniqueProcessId,
                NULL,
                PH_ENUM_GENERIC_MAPPED_FILES | PH_ENUM_GENERIC_MAPPED_IMAGES,
                EnumModulesCallback,
                &searchModuleContext
                );
        } while (process = PH_NEXT_PROCESS(process));

        PhFree(processes);
    }

Exit:
    PostMessage(context->WindowHandle, WM_PH_SEARCH_FINISHED, status, 0);

    PhDereferenceObject(context);
    return STATUS_SUCCESS;
}
