/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2011-2020
 *
 */

#include "updater.h"

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            if (PhGetIntegerSetting(SETTING_NAME_AUTO_CHECK))
            {
                ULONG lastTimeUpdateSeconds;
                LARGE_INTEGER lastTimeUpdateTicks;

                Button_SetCheck(GetDlgItem(WindowHandle, IDC_AUTOCHECKBOX), BST_CHECKED);

                if (lastTimeUpdateSeconds = PhGetIntegerSetting(SETTING_NAME_LAST_CHECK))
                {
                    PPH_STRING timeRelativeString;
                    PPH_STRING timeString;
                    LARGE_INTEGER time;
                    LARGE_INTEGER currentTime;
                    SYSTEMTIME timeFields;

                    PhSecondsSince1970ToTime(lastTimeUpdateSeconds, &lastTimeUpdateTicks);

                    time.QuadPart = lastTimeUpdateTicks.QuadPart;
                    PhLargeIntegerToLocalSystemTime(&timeFields, &time);
                    timeString = PhaFormatDateTime(&timeFields);

                    PhQuerySystemTime(&currentTime);
                    timeRelativeString = PH_AUTO(PhFormatTimeSpanRelative(currentTime.QuadPart - lastTimeUpdateTicks.QuadPart));

                    PhSetDialogItemText(WindowHandle, IDC_TEXT, PhaFormatString(
                        L"Last update check: %s (%s ago)",
                        PhGetStringOrEmpty(timeString),
                        PhGetStringOrEmpty(timeRelativeString)
                        )->Buffer);

                    time.QuadPart = lastTimeUpdateTicks.QuadPart + (7 * PH_TICKS_PER_DAY);
                    PhLargeIntegerToLocalSystemTime(&timeFields, &time);
                    timeString = PhaFormatDateTime(&timeFields);

                    time.QuadPart = time.QuadPart - currentTime.QuadPart;
                    if (time.QuadPart > 0)
                    {
                        timeRelativeString = PH_AUTO(PhFormatTimeSpanRelative(time.QuadPart));
                        PhSetDialogItemText(WindowHandle, IDC_TEXT2, PhaFormatString(
                            L"Next update check: %s (%s)",
                            PhGetStringOrEmpty(timeString),
                            PhGetStringOrEmpty(timeRelativeString)
                            )->Buffer);
                    }
                    else
                    {
                        PhSetDialogItemText(WindowHandle, IDC_TEXT2, PhaFormatString(
                            L"Next update check: %s",
                            PhGetStringOrEmpty(timeString)
                            )->Buffer);
                    }
                }
            }

            if (PhGetIntegerSetting(SETTING_NAME_UPDATE_MODE))
            {
                Button_SetCheck(GetDlgItem(WindowHandle, IDC_SHOWSTARTPROMPTCHECK), BST_CHECKED);
            }

            if (PhGetIntegerSetting(SETTING_NAME_AUTO_CHECK_PAGE))
            {
                Button_SetCheck(GetDlgItem(WindowHandle, IDC_SKIPWELCOMEPAGECHECK), BST_CHECKED);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_AUTOCHECKBOX:
                {
                    PhSetIntegerSetting(SETTING_NAME_AUTO_CHECK,
                        Button_GetCheck(GET_WM_COMMAND_HWND(wParam, lParam)) == BST_CHECKED);
                }
                break;
            case IDC_SHOWSTARTPROMPTCHECK:
                {
                    PhSetIntegerSetting(SETTING_NAME_UPDATE_MODE,
                        Button_GetCheck(GET_WM_COMMAND_HWND(wParam, lParam)) == BST_CHECKED);
                }
                break;
            case IDC_SKIPWELCOMEPAGECHECK:
                {
                    PhSetIntegerSetting(SETTING_NAME_AUTO_CHECK_PAGE,
                        Button_GetCheck(GET_WM_COMMAND_HWND(wParam, lParam)) == BST_CHECKED);
                }
                break;
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

typedef struct _PH_UPDATER_COMMIT_ENTRY
{
    PPH_STRING CommitHashString;
    PPH_STRING CommitUrlString;
    PPH_STRING CommitMessageString;
    PPH_STRING CommitAuthorString;
    PPH_STRING CommitDateString;
    UINT64 CommitMessageCommentCount;
} PH_UPDATER_COMMIT_ENTRY, *PPH_UPDATER_COMMIT_ENTRY;

PPH_STRING TrimString(
    _In_ PPH_STRING String
    )
{
    static PH_STRINGREF whitespace = PH_STRINGREF_INIT(L"  ");
    return PhCreateString3(&String->sr, 0, &whitespace);
}

_Success_(return)
BOOLEAN PhpUpdaterExtractCoAuthorName(
    _In_ PPH_STRING CommitMessage,
    _Out_opt_ PPH_STRING* CommitCoAuthorName
    )
{
    ULONG_PTR authoredByNameIndex;
    ULONG_PTR authoredByNameLength;
    PPH_STRING authoredByName;

    // Co-Authored-By: NAME <EMAIL>

    if ((authoredByNameIndex = PhFindStringInString(CommitMessage, 0, L"Co-Authored-By:")) == SIZE_MAX)
        return FALSE;
    if ((authoredByNameLength = PhFindStringInString(CommitMessage, authoredByNameIndex, L" <")) == SIZE_MAX)
        return FALSE;
    if ((authoredByNameLength = authoredByNameLength - authoredByNameIndex) == 0)
        return FALSE;

    authoredByName = PhSubstring(
        CommitMessage,
        authoredByNameIndex + (RTL_NUMBER_OF(L"Co-Authored-By:") - 1),
        authoredByNameLength - (RTL_NUMBER_OF(L"Co-Authored-By:") - 1)
        );

    if (CommitCoAuthorName)
        *CommitCoAuthorName = TrimString(authoredByName);

    PhDereferenceObject(authoredByName);
    return TRUE;
}

PPH_LIST PhpUpdaterQueryCommitHistory(
    VOID
    )
{
    NTSTATUS status;
    PPH_LIST results = NULL;
    PPH_BYTES jsonString = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PVOID jsonObject = NULL;
    ULONG i;
    ULONG arrayLength;

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"api.github.com", PH_HTTP_DEFAULT_HTTPS_PORT)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, NULL, L"/repos/winsiderss/systeminformer/commits", PH_HTTP_FLAG_SECURE)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, PH_HTTP_NO_ADDITIONAL_HEADERS, 0, PH_HTTP_NO_REQUEST_DATA, 0, 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &jsonString)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhCreateJsonParserEx(&jsonObject, jsonString, FALSE)))
        goto CleanupExit;

    if (PhGetJsonObjectType(jsonObject) != PH_JSON_OBJECT_TYPE_ARRAY)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    if (!(arrayLength = PhGetJsonArrayLength(jsonObject)))
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    if (arrayLength > 0)
    {
        results = PhCreateList(arrayLength);

        for (i = 0; i < arrayLength; i++)
        {
            PPH_UPDATER_COMMIT_ENTRY entry;
            PVOID jsonArrayObject;
            PVOID jsonCommitObject;
            PVOID jsonAuthorObject;

            if (!(jsonArrayObject = PhGetJsonArrayIndexObject(jsonObject, i)))
                continue;

            entry = PhAllocateZero(sizeof(PH_UPDATER_COMMIT_ENTRY));
            entry->CommitHashString = PhGetJsonValueAsString(jsonArrayObject, "sha");
            entry->CommitUrlString = PhGetJsonValueAsString(jsonArrayObject, "html_url");

            if (jsonCommitObject = PhGetJsonObject(jsonArrayObject, "commit"))
            {
                entry->CommitMessageString = PhGetJsonValueAsString(jsonCommitObject, "message");
                entry->CommitMessageCommentCount = PhGetJsonValueAsUInt64(jsonCommitObject, "comment_count");

                if (jsonAuthorObject = PhGetJsonObject(jsonCommitObject, "author"))
                {
                    entry->CommitAuthorString = PhGetJsonValueAsString(jsonAuthorObject, "name");
                    entry->CommitDateString = PhGetJsonValueAsString(jsonAuthorObject, "date");
                }
            }

            //if (jsonAuthorObject = PhGetJsonObject(jsonArrayObject, "author"))
            //{
            //    entry->CommitAuthorString = PhGetJsonValueAsString(jsonAuthorObject, "login");
            //}

            if (!(
                PhIsNullOrEmptyString(entry->CommitHashString) &&
                PhIsNullOrEmptyString(entry->CommitUrlString) &&
                PhIsNullOrEmptyString(entry->CommitMessageString) &&
                PhIsNullOrEmptyString(entry->CommitAuthorString) &&
                PhIsNullOrEmptyString(entry->CommitDateString)
                ))
            {

            }

            if (!PhIsNullOrEmptyString(entry->CommitMessageString))
            {
                PH_STRING_BUILDER sb;
                PPH_STRING authorNameString = NULL;
                ULONG_PTR commitMessageAuthorIndex;

                PhInitializeStringBuilder(&sb, 0x100);

                for (SIZE_T j = 0; j < entry->CommitMessageString->Length / sizeof(WCHAR); j++)
                {
                    if (entry->CommitMessageString->Data[j] == L'\n')
                        PhAppendStringBuilder2(&sb, L" ");
                    else
                        PhAppendCharStringBuilder(&sb, entry->CommitMessageString->Data[j]);
                }

                PhMoveReference(&entry->CommitMessageString, PhFinalStringBuilderString(&sb));

                if (PhpUpdaterExtractCoAuthorName(
                    entry->CommitMessageString,
                    &authorNameString
                    ))
                {
                    PhMoveReference(&entry->CommitAuthorString, authorNameString);
                }

                if ((commitMessageAuthorIndex = PhFindStringInString(entry->CommitMessageString, 0, L"Co-Authored-By:")) != SIZE_MAX)
                {
                    PhMoveReference(
                        &entry->CommitMessageString,
                        PhSubstring(entry->CommitMessageString, 0, (ULONG)commitMessageAuthorIndex - 2) // minus space
                        );
                }
            }

            PhAddItemList(results, entry);
        }
    }

CleanupExit:

    if (httpContext)
    {
        PhHttpDestroy(httpContext);
    }

    if (jsonObject)
    {
        PhFreeJsonObject(jsonObject);
    }

    PhClearReference(&jsonString);

    return results;
}

#define WM_UPDATER_COMMITS (WM_APP + 1)

typedef struct _PH_UPDATER_COMMIT_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HFONT ListViewBoldFont;
    PPH_STRING BuildMessage;

    PPH_STRING CurrentCommitHash;
    PPH_STRING LatestCommitHash;

    PH_LAYOUT_MANAGER LayoutManager;
} PH_UPDATER_COMMIT_CONTEXT, *PPH_UPDATER_COMMIT_CONTEXT;

PPH_STRING PhpUpdaterCommitStringToTime(
    _In_ PPH_STRING Time
    )
{
    SYSTEMTIME time;
    SYSTEMTIME localTime = { 0 };
    PH_STRINGREF yyPart;
    PH_STRINGREF mmPartSr;
    PH_STRINGREF ddPartSr;
    PH_STRINGREF hrPartSr;
    PH_STRINGREF mnPartSr;
    PH_STRINGREF ssPartSr;
    PH_STRINGREF remainingPart;
    ULONG64 year, month, day, hour, minute, second;

    // %hu-%hu-%huT%hu:%hu:%huZ
    remainingPart = PhGetStringRef(Time);

    if (!PhSplitStringRefAtChar(&remainingPart, L'-', &yyPart, &remainingPart))
        return NULL;
    if (!PhSplitStringRefAtChar(&remainingPart, L'-', &mmPartSr, &remainingPart))
        return NULL;
    if (!PhSplitStringRefAtChar(&remainingPart, L'T', &ddPartSr, &remainingPart))
        return NULL;
    if (!PhSplitStringRefAtChar(&remainingPart, L':', &hrPartSr, &remainingPart))
        return NULL;
    if (!PhSplitStringRefAtChar(&remainingPart, L':', &mnPartSr, &remainingPart))
        return NULL;
    if (!PhSplitStringRefAtChar(&remainingPart, L'Z', &ssPartSr, &remainingPart))
        return NULL;

    if (!PhStringToUInt64(&yyPart, 10, &year))
        return NULL;
    if (!PhStringToUInt64(&mmPartSr, 10, &month))
        return NULL;
    if (!PhStringToUInt64(&ddPartSr, 10, &day))
        return NULL;
    if (!PhStringToUInt64(&hrPartSr, 10, &hour))
        return NULL;
    if (!PhStringToUInt64(&mnPartSr, 10, &minute))
        return NULL;
    if (!PhStringToUInt64(&ssPartSr, 10, &second))
        return NULL;

    memset(&time, 0, sizeof(SYSTEMTIME));

    if (!NT_SUCCESS(RtlULong64ToUShort(year, &time.wYear)))
        return NULL;
    if (!NT_SUCCESS(RtlULong64ToUShort(month, &time.wMonth)))
        return NULL;
    if (!NT_SUCCESS(RtlULong64ToUShort(day, &time.wDay)))
        return NULL;
    if (!NT_SUCCESS(RtlULong64ToUShort(hour, &time.wHour)))
        return NULL;
    if (!NT_SUCCESS(RtlULong64ToUShort(minute, &time.wMinute)))
        return NULL;
    if (!NT_SUCCESS(RtlULong64ToUShort(second, &time.wSecond)))
        return NULL;
    if (!PhSystemTimeToTzSpecificLocalTime(&time, &localTime))
        return NULL;

    //return PhFormatDateTime(&localTime);
    return PhFormatDate(&localTime, NULL);
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI PhpUpdaterQueryCommitHistoryThread(
    _In_ PVOID ThreadParameter
    )
{
    HWND windowHandle = (HWND)ThreadParameter;
    PPH_LIST commentHistoryList;

    if (commentHistoryList = PhpUpdaterQueryCommitHistory())
    {
        PostMessage(windowHandle, WM_UPDATER_COMMITS, 0, (LPARAM)commentHistoryList);
    }

    return STATUS_SUCCESS;
}

VOID PhpUpdaterFreeListViewEntries(
    _In_ PPH_UPDATER_COMMIT_CONTEXT Context
    )
{
    INT index = INT_ERROR;

    while ((index = PhFindListViewItemByFlags(Context->ListViewHandle, index, LVNI_ALL)) != INT_ERROR)
    {
        PPH_UPDATER_COMMIT_ENTRY entry;

        if (PhGetListViewItemParam(Context->ListViewHandle, index, &entry))
        {
            if (entry->CommitHashString)
                PhDereferenceObject(entry->CommitHashString);
            if (entry->CommitUrlString)
                PhDereferenceObject(entry->CommitUrlString);
            if (entry->CommitMessageString)
                PhDereferenceObject(entry->CommitMessageString);
            if (entry->CommitAuthorString)
                PhDereferenceObject(entry->CommitAuthorString);
            if (entry->CommitDateString)
                PhDereferenceObject(entry->CommitDateString);

            PhFree(entry);
        }
    }
}

INT_PTR CALLBACK TextDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_UPDATER_COMMIT_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PH_UPDATER_COMMIT_CONTEXT));

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_COMMITS);
            context->ListViewBoldFont = PhDuplicateFontWithNewWeight(GetWindowFont(context->ListViewHandle), FW_BOLD);

            PhSetApplicationWindowIconEx(hwndDlg, PhGetWindowDpi(hwndDlg));

            PhSetListViewStyle(context->ListViewHandle, FALSE, FALSE); // TRUE, TRUE (tooltips)
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 120, L"Date");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Author");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 250, L"Comments");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Commit");
            PhSetExtendedListView(context->ListViewHandle);

            PhLoadListViewColumnsFromSetting(SETTING_NAME_CHANGELOG_COLUMNS, context->ListViewHandle);
            PhLoadListViewSortColumnsFromSetting(SETTING_NAME_CHANGELOG_SORTCOLUMN, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhValidWindowPlacementFromSetting(SETTING_NAME_CHANGELOG_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_CHANGELOG_WINDOW_POSITION, SETTING_NAME_CHANGELOG_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDCANCEL));

            {
                PPH_UPDATER_CONTEXT updater = ((PPH_UPDATER_CONTEXT)lParam);
                context->CurrentCommitHash = PhGetPhVersionHash();
                if (updater) context->LatestCommitHash = PhReferenceObject(updater->CommitHash);

                if (
                    context->CurrentCommitHash &&
                    context->LatestCommitHash &&
                    PhEqualString2(context->CurrentCommitHash, L"\"\"", TRUE)
                    )
                {
                    PhDereferenceObject(context->CurrentCommitHash);
                    context->CurrentCommitHash = PhReferenceObject(context->LatestCommitHash);
                }

                if (
                    context->CurrentCommitHash &&
                    context->LatestCommitHash &&
                    PhEqualString(context->CurrentCommitHash, context->LatestCommitHash, TRUE)
                    )
                {
                    PhDereferenceObject(context->LatestCommitHash);
                    context->LatestCommitHash = NULL;
                }
            }

            //PhSetWindowText(GetDlgItem(hwndDlg, IDC_TEXT), PhGetString(context->BuildMessage));
            PhCreateThread2(PhpUpdaterQueryCommitHistoryThread, hwndDlg);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewSortColumnsToSetting(SETTING_NAME_CHANGELOG_SORTCOLUMN, context->ListViewHandle);
            PhSaveListViewColumnsToSetting(SETTING_NAME_CHANGELOG_COLUMNS, context->ListViewHandle);
            PhSaveWindowPlacementToSetting(SETTING_NAME_CHANGELOG_WINDOW_POSITION, SETTING_NAME_CHANGELOG_WINDOW_SIZE, hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->LatestCommitHash)
                PhDereferenceObject(context->LatestCommitHash);
            if (context->CurrentCommitHash)
                PhDereferenceObject(context->CurrentCommitHash);
            if (context->ListViewBoldFont)
                DeleteFont(context->ListViewBoldFont);

            PhpUpdaterFreeListViewEntries(context);
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_DPICHANGED:
        {
            LONG windowDpi = HIWORD(wParam);

            PhSetApplicationWindowIconEx(hwndDlg, windowDpi);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            //case LVN_GETINFOTIP:
            //    {
            //        NMLVGETINFOTIP* getInfoTip = (NMLVGETINFOTIP*)lParam;
            //        PH_STRINGREF tip;
            //        PhInitializeStringRefLongHint(&tip, L"Commit: Author: Author Date: Committer: Commit Date: Commit Message:");
            //        PhCopyListViewInfoTip(getInfoTip, &tip);
            //    }
            //    break;
            case LVN_GETEMPTYMARKUP:
                {
                    NMLVEMPTYMARKUP* listview = (NMLVEMPTYMARKUP*)lParam;

                    listview->dwFlags = EMF_CENTERED;
                    wcsncpy_s(listview->szMarkup, RTL_NUMBER_OF(listview->szMarkup), L"Querying changelog...", _TRUNCATE);

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
                break;
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == context->ListViewHandle)
                    {
                        PPH_UPDATER_COMMIT_ENTRY entry;
                        PPH_STRING commitHashUrl;

                        if (entry = PhGetSelectedListViewItemParam(context->ListViewHandle))
                        {
                            if (commitHashUrl = PhConcatStrings2(
                                L"https://github.com/winsiderss/systeminformer/commit/",
                                PhGetString(entry->CommitHashString)
                                ))
                            {
                                PhShellExecute(hwndDlg, PhGetString(commitHashUrl), NULL);
                                PhDereferenceObject(commitHashUrl);
                            }
                        }
                    }
                }
                break;
            case NM_CUSTOMDRAW:
                {
                    if (header->hwndFrom == context->ListViewHandle)
                    {
                        LPNMLVCUSTOMDRAW customDraw = (LPNMLVCUSTOMDRAW)header;

                        switch (customDraw->nmcd.dwDrawStage)
                        {
                        case CDDS_PREPAINT:
                            {
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
                                return CDRF_NOTIFYITEMDRAW;
                            }
                            break;
                        case CDDS_ITEMPREPAINT:
                            {
                                HFONT newFont = NULL;
                                PPH_STRING commitHash;
                                PPH_STRING shortCommitHash;

                                if (commitHash = PhGetListViewItemText(context->ListViewHandle, (INT)customDraw->nmcd.dwItemSpec, 3))
                                {
                                    if (context->LatestCommitHash && (shortCommitHash = PhSubstring(context->LatestCommitHash, 0, 7)))
                                    {
                                        if (PhEqualString(commitHash, shortCommitHash, TRUE))
                                        {
                                            newFont = context->ListViewBoldFont;
                                            customDraw->clrText = RGB(0, 0, 0xff);
                                            SelectFont(customDraw->nmcd.hdc, newFont);
                                        }

                                        PhDereferenceObject(shortCommitHash);
                                    }

                                    if (context->CurrentCommitHash && (shortCommitHash = PhSubstring(context->CurrentCommitHash, 0, 7)))
                                    {
                                        if (PhEqualString(commitHash, shortCommitHash, TRUE))
                                        {
                                            newFont = context->ListViewBoldFont;
                                            if (PhGetIntegerSetting(L"EnableThemeSupport"))
                                                customDraw->clrText = RGB(125, 125, 125);
                                            else
                                                customDraw->clrText = GetSysColor(COLOR_WINDOWTEXT);
                                            SelectFont(customDraw->nmcd.hdc, newFont);
                                        }

                                        PhDereferenceObject(shortCommitHash);
                                    }

                                    PhDereferenceObject(commitHash);
                                }

                                if (newFont)
                                {
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
                                    return CDRF_NEWFONT;
                                }
                                else
                                {
                                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, CDRF_DODEFAULT);
                                    return CDRF_DODEFAULT;
                                }

                                //SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, CDRF_NOTIFYSUBITEMDRAW);
                                //return CDRF_NOTIFYSUBITEMDRAW;
                            }
                            break;
                        //case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
                        //    {
                        //        if (customDraw->iSubItem == 1)
                        //        {
                        //            PPH_STRING commitAuthor;
                        //
                        //            if (commitAuthor = PhGetListViewItemText(context->ListViewHandle, (INT)customDraw->nmcd.dwItemSpec, 1))
                        //            {
                        //                if (PhEqualString2(commitAuthor, L"dmex", TRUE))
                        //                {
                        //                    customDraw->clrText = RGB(0xff, 0x0, 0x0);
                        //                }
                        //                else
                        //                {
                        //                    customDraw->clrText = RGB(0x0, 0x0, 0xff);
                        //                }
                        //
                        //                PhDereferenceObject(commitAuthor);
                        //            }
                        //        }
                        //        else
                        //        {
                        //            customDraw->clrText = RGB(0x0, 0x0, 0x0);
                        //        }
                        //
                        //        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
                        //        return CDRF_NEWFONT;
                        //    }
                        //    break;
                        }
                    }
                }
                break;
            }

            PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);

            REFLECT_MESSAGE_DLG(hwndDlg, context->ListViewHandle, uMsg, wParam, lParam);
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
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"View on Github", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, 2, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        BOOLEAN handled = FALSE;

                        handled = PhHandleCopyListViewEMenuItem(item);

                        if (!handled)
                        {
                            switch (item->Id)
                            {
                            case 1:
                                {
                                    PPH_UPDATER_COMMIT_ENTRY entry;
                                    PPH_STRING commitHashUrl;
                                    INT lvItemIndex;

                                    lvItemIndex = PhFindListViewItemByFlags(context->ListViewHandle, INT_ERROR, LVNI_SELECTED);

                                    if (lvItemIndex != INT_ERROR)
                                    {
                                        if (PhGetListViewItemParam(context->ListViewHandle, lvItemIndex, &entry))
                                        {
                                            if (commitHashUrl = PhConcatStrings2(
                                                L"https://github.com/winsiderss/systeminformer/commit/",
                                                PhGetString(entry->CommitHashString)
                                                ))
                                            {
                                                PhShellExecute(hwndDlg, PhGetString(commitHashUrl), NULL);
                                                PhDereferenceObject(commitHashUrl);
                                            }
                                        }
                                    }
                                }
                                break;
                            case 2:
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
    case WM_UPDATER_COMMITS:
        {
            PPH_LIST commitList = (PPH_LIST)lParam;

            if (!commitList)
                break;

            ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);

            for (ULONG i = 0; i < commitList->Count; i++)
            {
                PPH_UPDATER_COMMIT_ENTRY entry = commitList->Items[i];
                INT lvItemIndex;
                PPH_STRING shortCommitHash;
                PPH_STRING commitTimeString;

                commitTimeString = PhpUpdaterCommitStringToTime(entry->CommitDateString);
                lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, PhGetStringOrEmpty(commitTimeString), entry);
                if (commitTimeString) PhDereferenceObject(commitTimeString);

                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 1, PhGetStringOrEmpty(entry->CommitAuthorString));
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 2, PhGetStringOrEmpty(entry->CommitMessageString));

                if (shortCommitHash = PhSubstring(entry->CommitHashString, 0, 7))
                {
                    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 3, PhGetStringOrEmpty(shortCommitHash));
                    PhDereferenceObject(shortCommitHash);
                }
            }

            ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);

            PhDereferenceObject(commitList);
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}
