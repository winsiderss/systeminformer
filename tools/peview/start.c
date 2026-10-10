/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2017-2026
 *
 */

#include <peview.h>
#include <shellapi.h>

static HWND PvStartPageWindowHandle;
static BOOLEAN PvStartPageLoading;
#define PV_STARTPAGE_BEGIN_LOADING (WM_APP + 0x353)

VOID PvStartPageFinishLoading(VOID)
{
    if (PvStartPageWindowHandle)
    {
        HWND windowHandle = PvStartPageWindowHandle;
        PvStartPageWindowHandle = NULL;
        DestroyWindow(windowHandle);
    }
    PvStartPageLoading = FALSE;
}

INT_PTR CALLBACK PvpStartPageDialogProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

/**
 * Shows the start page and runs its message loop until a file is chosen or the
 * window is closed. The window stays open until PvStartPageFinishLoading.
 *
 * \param ShowCommand The initial show state of the window.
 * \return TRUE if the start page was created, otherwise FALSE.
 */
BOOLEAN PvShowStartPage(
    _In_ LONG ShowCommand
    )
{
    MSG message;

    // Modeless: retain the actual launcher until the properties window is shown.
    // Both debug and release continue in this process instead of relaunching.
    PvStartPageWindowHandle = PhCreateDialog(PhInstanceHandle,
        MAKEINTRESOURCE(IDD_STARTPAGE), NULL, PvpStartPageDialogProc, NULL);
    if (!PvStartPageWindowHandle)
        return FALSE;
    ShowWindow(PvStartPageWindowHandle, ShowCommand);
    while (PvStartPageWindowHandle && !PvStartPageLoading)
    {
        BOOL result = GetMessage(&message, NULL, 0, 0);
        if (result <= 0)
            break;
        if (!IsDialogMessage(PvStartPageWindowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }

    return TRUE;
}

#define PV_RECENT_FILE_LIMIT 10
#define PV_RECENT_FILE_SEPARATOR L'\n'

typedef struct _PV_STARTPAGE_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HWND SearchHandle;
    HIMAGELIST ImageListHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPH_LIST RecentFileList; // PPH_STRING, most recent first
    ULONG_PTR SearchMatchHandle;
    BOOLEAN UpdatingTheme;
} PV_STARTPAGE_CONTEXT, *PPV_STARTPAGE_CONTEXT;

// Recent file groups, listed most-recent first to mirror the shell "recent" layout. (dmex)
#define PV_RECENT_GROUP_TODAY       0
#define PV_RECENT_GROUP_YESTERDAY   1
#define PV_RECENT_GROUP_THISWEEK    2
#define PV_RECENT_GROUP_THISMONTH   3
#define PV_RECENT_GROUP_THISYEAR    4
#define PV_RECENT_GROUP_OLDER       5

/**
 * Computes a day number from a calendar date using the days-from-civil algorithm.
 *
 * \param Year The calendar year.
 * \param Month The calendar month (1-12).
 * \param Day The calendar day (1-31).
 * \return A monotonically increasing day index suitable for date arithmetic.
 */
LONG64 PvDaysFromCivil(
    _In_ LONG Year,
    _In_ LONG Month,
    _In_ LONG Day
    )
{
    LONG64 y = Year;
    LONG64 era;
    LONG64 yoe;
    LONG64 doy;
    LONG64 doe;

    y -= Month <= 2;
    era = (y >= 0 ? y : y - 399) / 400;
    yoe = y - era * 400;
    doy = (153 * (Month + (Month > 2 ? -3 : 9)) + 2) / 5 + Day - 1;
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

    return era * 146097 + doe - 719468;
}

/**
 * Returns the display name for a recent-file group.
 *
 * \param GroupId One of the PV_RECENT_GROUP_* identifiers.
 * \return The group heading text.
 */
PCWSTR PvGetRecentFileGroupName(
    _In_ LONG GroupId
    )
{
    switch (GroupId)
    {
    case PV_RECENT_GROUP_TODAY:
        return L"Today";
    case PV_RECENT_GROUP_YESTERDAY:
        return L"Yesterday";
    case PV_RECENT_GROUP_THISWEEK:
        return L"This week";
    case PV_RECENT_GROUP_THISMONTH:
        return L"This month";
    case PV_RECENT_GROUP_THISYEAR:
        return L"This year";
    default:
        return L"Older";
    }
}

/**
 * Buckets a file time into one of the recent-file groups relative to the current time.
 *
 * \param LastWriteTime The file's last write time.
 * \return One of the PV_RECENT_GROUP_* identifiers.
 */
LONG PvGetRecentFileGroupId(
    _In_ PLARGE_INTEGER LastWriteTime
    )
{
    LARGE_INTEGER now;
    SYSTEMTIME nowLocal;
    SYSTEMTIME fileLocal;
    LONG64 nowDays;
    LONG64 fileDays;
    LONG64 diffDays;

    PhQuerySystemTime(&now);
    PhLargeIntegerToLocalSystemTime(&nowLocal, &now);
    PhLargeIntegerToLocalSystemTime(&fileLocal, LastWriteTime);

    nowDays = PvDaysFromCivil(nowLocal.wYear, nowLocal.wMonth, nowLocal.wDay);
    fileDays = PvDaysFromCivil(fileLocal.wYear, fileLocal.wMonth, fileLocal.wDay);
    diffDays = nowDays - fileDays;

    if (diffDays <= 0)
        return PV_RECENT_GROUP_TODAY;
    if (diffDays == 1)
        return PV_RECENT_GROUP_YESTERDAY;
    if (diffDays < 7)
        return PV_RECENT_GROUP_THISWEEK;
    if (nowLocal.wYear == fileLocal.wYear && nowLocal.wMonth == fileLocal.wMonth)
        return PV_RECENT_GROUP_THISMONTH;
    if (nowLocal.wYear == fileLocal.wYear)
        return PV_RECENT_GROUP_THISYEAR;

    return PV_RECENT_GROUP_OLDER;
}

/**
 * Reads the recent file list from settings.
 *
 * \return A list of PPH_STRING file names, most recent first. The caller is responsible for
 * dereferencing the entries and the list.
 */
PPH_LIST PvLoadRecentFileList(
    VOID
    )
{
    PPH_LIST fileList;
    PPH_STRING setting;
    PH_STRINGREF remaining;
    PH_STRINGREF part;

    fileList = PhCreateList(PV_RECENT_FILE_LIMIT);
    setting = PhGetStringSetting(L"RecentFiles");
    remaining = setting->sr;

    while (remaining.Length != 0)
    {
        PhSplitStringRefAtChar(&remaining, PV_RECENT_FILE_SEPARATOR, &part, &remaining);

        if (part.Length == 0)
            continue;
        if (fileList->Count >= PV_RECENT_FILE_LIMIT)
            break;

        PhAddItemList(fileList, PhCreateString2(&part));
    }

    PhDereferenceObject(setting);

    return fileList;
}

/**
 * Writes the recent file list to settings.
 *
 * \param FileList A list of PPH_STRING file names, most recent first.
 */
VOID PvSaveRecentFileList(
    _In_ PPH_LIST FileList
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 0x100);

    for (ULONG i = 0; i < FileList->Count && i < PV_RECENT_FILE_LIMIT; i++)
    {
        PPH_STRING fileName = FileList->Items[i];

        if (PhIsNullOrEmptyString(fileName))
            continue;

        if (stringBuilder.String->Length != 0)
            PhAppendCharStringBuilder(&stringBuilder, PV_RECENT_FILE_SEPARATOR);

        PhAppendStringBuilder(&stringBuilder, &fileName->sr);
    }

    PhSetStringSetting2(L"RecentFiles", &stringBuilder.String->sr);

    PhDeleteStringBuilder(&stringBuilder);
}

VOID PvDestroyRecentFileList(
    _In_ PPH_LIST FileList
    )
{
    PhDereferenceObjects(FileList->Items, FileList->Count);
    PhDereferenceObject(FileList);
}

/**
 * Promotes a file name to the top of the recent file list, removing any existing entry
 * for the same file and trimming the list to PV_RECENT_FILE_LIMIT entries.
 *
 * \param FileName The file name to add.
 */
VOID PvAddRecentFile(
    _In_ PPH_STRING FileName
    )
{
    PPH_LIST fileList;

    if (PhIsNullOrEmptyString(FileName))
        return;

    fileList = PvLoadRecentFileList();

    for (ULONG i = 0; i < fileList->Count; i++)
    {
        PPH_STRING entry = fileList->Items[i];

        if (PhEqualString(entry, FileName, TRUE))
        {
            PhRemoveItemList(fileList, i);
            PhDereferenceObject(entry);
            break;
        }
    }

    PhInsertItemList(fileList, 0, PhReferenceObject(FileName));

    while (fileList->Count > PV_RECENT_FILE_LIMIT)
    {
        PPH_STRING entry = fileList->Items[fileList->Count - 1];

        PhRemoveItemList(fileList, fileList->Count - 1);
        PhDereferenceObject(entry);
    }

    PvSaveRecentFileList(fileList);

    PvDestroyRecentFileList(fileList);
}

/**
 * Creates the listview imagelist at the current window DPI, replacing any existing imagelist.
 *
 * \param Context The start page context.
 */
VOID PvCreateStartPageImageList(
    _In_ PPV_STARTPAGE_CONTEXT Context
    )
{
    LONG dpiValue = PhGetWindowDpi(Context->WindowHandle);

    if (Context->ImageListHandle)
    {
        PhImageListDestroy(Context->ImageListHandle);
        Context->ImageListHandle = NULL;
    }

    if (Context->ImageListHandle = PhImageListCreate(
        PhGetSystemMetrics(SM_CXSMICON, dpiValue),
        PhGetSystemMetrics(SM_CYSMICON, dpiValue),
        ILC_MASK | ILC_COLOR32,
        PV_RECENT_FILE_LIMIT,
        PV_RECENT_FILE_LIMIT
        ))
    {
        ListView_SetImageList(Context->ListViewHandle, Context->ImageListHandle, LVSIL_SMALL);
    }
}

// Start-page groups need more separation than the dense property-page cards.
static VOID PvpStartPageApplyGroupSpacing(
    _In_ HWND ListViewHandle
    )
{
    LVGROUPMETRICS metrics = { sizeof(LVGROUPMETRICS) };
    LONG dpiValue = PhGetWindowDpi(ListViewHandle);

    metrics.mask = LVGMF_BORDERSIZE;
    // The adjacent bottom/top borders combine to a 16-DIP inter-section gap.
    // Keep horizontal borders zero so full-width report rows stay inside cards.
    metrics.Top = PhScaleToDisplay(8, dpiValue);
    metrics.Bottom = PhScaleToDisplay(8, dpiValue);
    ListView_SetGroupMetrics(ListViewHandle, &metrics);
}

/**
 * Reloads the recent file list, discards entries that no longer exist and repopulates
 * the listview.
 *
 * \param Context The start page context.
 */
VOID PvRefreshRecentFileList(
    _In_ PPV_STARTPAGE_CONTEXT Context
    )
{
    BOOLEAN pruned = FALSE;
    LONG dpiValue = PhGetWindowDpi(Context->WindowHandle);
    LONG iconWidth = PhGetSystemMetrics(SM_CXSMICON, dpiValue);
    LONG iconHeight = PhGetSystemMetrics(SM_CYSMICON, dpiValue);

    // Guard against a search callback firing before the imagelist is ready. (dmex)
    if (!Context->ImageListHandle)
        return;

    if (Context->RecentFileList)
    {
        PvDestroyRecentFileList(Context->RecentFileList);
        Context->RecentFileList = NULL;
    }

    Context->RecentFileList = PvLoadRecentFileList();

    for (ULONG i = 0; i < Context->RecentFileList->Count; i++)
    {
        PPH_STRING fileName = Context->RecentFileList->Items[i];

        if (!PhDoesFileExistWin32(PhGetString(fileName)))
        {
            PhRemoveItemList(Context->RecentFileList, i);
            PhDereferenceObject(fileName);
            pruned = TRUE;
            i--;
        }
    }

    if (pruned)
    {
        PvSaveRecentFileList(Context->RecentFileList);
    }

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);
    ListView_RemoveAllGroups(Context->ListViewHandle);
    PhImageListSetImageCount(Context->ImageListHandle, 0);

    // Track which groups have been created so empty group headers aren't shown. Groups are
    // created on first use, so they appear in recency order (the list is most-recent first). (dmex)
    BOOLEAN groupAdded[PV_RECENT_GROUP_OLDER + 1] = { FALSE };

    for (ULONG i = 0; i < Context->RecentFileList->Count; i++)
    {
        PPH_STRING fileName = Context->RecentFileList->Items[i];
        PPH_STRING baseName;
        PPH_STRING dateTime = NULL;
        FILE_NETWORK_OPEN_INFORMATION networkOpenInfo;
        HICON iconSmall = NULL;
        LONG index;
        LONG iconIndex;
        LONG groupId = PV_RECENT_GROUP_OLDER;

        baseName = PhGetBaseName(fileName);

        // Filter against the search box, matching both the display name and full path. (dmex)
        if (Context->SearchMatchHandle)
        {
            if (!PvSearchControlMatch(Context->SearchMatchHandle, &fileName->sr) &&
                !(baseName && PvSearchControlMatch(Context->SearchMatchHandle, &baseName->sr)))
            {
                PhClearReference(&baseName);
                continue;
            }
        }

        if (NT_SUCCESS(PhQueryFullAttributesFileWin32(PhGetString(fileName), &networkOpenInfo)))
        {
            SYSTEMTIME localTime;

            groupId = PvGetRecentFileGroupId(&networkOpenInfo.LastWriteTime);
            PhLargeIntegerToLocalSystemTime(&localTime, &networkOpenInfo.LastWriteTime);
            dateTime = PhFormatDateTime(&localTime);
        }

        if (!groupAdded[groupId])
        {
            PhAddListViewGroup(Context->ListViewHandle, groupId, PvGetRecentFileGroupName(groupId));
            groupAdded[groupId] = TRUE;
        }

        index = PhAddListViewGroupItem(
            Context->ListViewHandle,
            groupId,
            MAXINT,
            PhGetStringOrDefault(baseName, PhGetString(fileName)),
            UlongToPtr(i + 1)
            );
        PhSetListViewSubItem(Context->ListViewHandle, index, 1, PhGetString(fileName));
        if (dateTime)
            PhSetListViewSubItem(Context->ListViewHandle, index, 2, PhGetString(dateTime));
        PhClearReference(&dateTime);
        PhClearReference(&baseName);

        if (NT_SUCCESS(PhExtractIconEx(
            &fileName->sr,
            FALSE,
            0,
            0,
            0,
            iconWidth,
            iconHeight,
            NULL,
            &iconSmall
            )) && iconSmall)
        {
            iconIndex = PhImageListAddIcon(Context->ImageListHandle, iconSmall);
            DestroyIcon(iconSmall);

            if (iconIndex != INT_ERROR)
                PhSetListViewItemImageIndex(Context->ListViewHandle, index, iconIndex);
        }
    }

    PvpStartPageApplyGroupSpacing(Context->ListViewHandle);
    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);

    if (Context->RecentFileList->Count == 0)
    {
        PhSetWindowText(GetDlgItem(Context->WindowHandle, IDC_START_TITLE), L"No recent files");
    }
    else
    {
        PhSetWindowText(GetDlgItem(Context->WindowHandle, IDC_START_TITLE), L"Open recent");
    }
}

/**
 * Sets the global file name and closes the start page.
 *
 * \param Context The start page context.
 * \param FileName The file selected by the user.
 */
VOID PvpStartPageSelectFile(
    _In_ PPV_STARTPAGE_CONTEXT Context,
    _In_ PPH_STRING FileName
    )
{
    if (PvStartPageLoading || PhIsNullOrEmptyString(FileName))
        return;

    PhSetReference(&PvFileName, FileName);

    PostMessage(Context->WindowHandle, PV_STARTPAGE_BEGIN_LOADING, 0, 0);
}

/**
 * Returns the index of the selected recent file entry.
 *
 * \param Context The start page context.
 * \return The index into the recent file list, or ULONG_MAX when nothing is selected.
 */
ULONG PvpGetSelectedRecentFileIndex(
    _In_ PPV_STARTPAGE_CONTEXT Context
    )
{
    LONG index;
    PVOID param;
    ULONG listIndex;

    index = ListView_GetNextItem(Context->ListViewHandle, INT_ERROR, LVNI_SELECTED);

    if (index == INT_ERROR)
        return ULONG_MAX;
    if (!PhGetListViewItemParam(Context->ListViewHandle, index, &param))
        return ULONG_MAX;

    // Note: The param is the list index biased by one so that entry zero isn't NULL. (dmex)
    listIndex = PtrToUlong(param);

    if (listIndex == 0 || listIndex > Context->RecentFileList->Count)
        return ULONG_MAX;

    return listIndex - 1;
}

_Success_(return != NULL)
PPH_STRING PvpGetSelectedRecentFile(
    _In_ PPV_STARTPAGE_CONTEXT Context
    )
{
    ULONG index;

    index = PvpGetSelectedRecentFileIndex(Context);

    if (index != ULONG_MAX)
        return Context->RecentFileList->Items[index];

    return NULL;
}

VOID PvpShowRecentFileMenu(
    _In_ PPV_STARTPAGE_CONTEXT Context,
    _In_ LPARAM lParam
    )
{
    POINT point;
    PPH_STRING fileName;
    PPH_EMENU menu;
    PPH_EMENU_ITEM item;

    point.x = GET_X_LPARAM(lParam);
    point.y = GET_Y_LPARAM(lParam);

    if (point.x == -1 && point.y == -1)
        PvGetListViewContextMenuPoint(Context->ListViewHandle, &point);

    fileName = PvpGetSelectedRecentFile(Context);

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"&Open", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2, L"Open &containing folder", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 3, L"Copy &path", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 4, L"&Remove from list", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 5, L"Clear &list", NULL, NULL), ULONG_MAX);
    PhSetFlagsEMenuItem(menu, 1, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

    if (!fileName)
    {
        PhSetFlagsEMenuItem(menu, 1, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        PhSetFlagsEMenuItem(menu, 2, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        PhSetFlagsEMenuItem(menu, 3, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        PhSetFlagsEMenuItem(menu, 4, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }

    if (Context->RecentFileList->Count == 0)
    {
        PhSetFlagsEMenuItem(menu, 5, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }

    item = PhShowEMenu(
        menu,
        Context->WindowHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        point.x,
        point.y
        );

    if (item)
    {
        switch (item->Id)
        {
        case 1:
            {
                if (fileName)
                    PvpStartPageSelectFile(Context, fileName);
            }
            break;
        case 2:
            {
                if (fileName)
                    PhShellExploreFile(Context->WindowHandle, PhGetString(fileName));
            }
            break;
        case 3:
            {
                if (fileName)
                    PhSetClipboardString(Context->WindowHandle, &fileName->sr);
            }
            break;
        case 4:
            {
                ULONG index = PvpGetSelectedRecentFileIndex(Context);

                if (index != ULONG_MAX)
                {
                    PhRemoveItemList(Context->RecentFileList, index);
                    PhDereferenceObject(fileName);

                    PvSaveRecentFileList(Context->RecentFileList);
                    PvRefreshRecentFileList(Context);
                }
            }
            break;
        case 5:
            {
                PhSetStringSetting(L"RecentFiles", L"");
                PvRefreshRecentFileList(Context);
            }
            break;
        }
    }

    PhDestroyEMenu(menu);
}

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI PvpStartPageSearchCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPV_STARTPAGE_CONTEXT context = Context;

    if (!context)
        return;

    context->SearchMatchHandle = MatchHandle;

    PvRefreshRecentFileList(context);
}

static VOID PvpStartPageApplyTheme(
    _In_ PPV_STARTPAGE_CONTEXT Context,
    _In_ BOOLEAN Refresh
    )
{
    // SetWindowTheme synchronously sends WM_THEMECHANGED. Do not rebuild the
    // palette or subclass chain while a theme update is already in progress.
    if (Context->UpdatingTheme)
        return;

    Context->UpdatingTheme = TRUE;
    if (Refresh)
        PvReapplyTheme(Context->WindowHandle);
    else
        PvThemeInitializePageDialog(Context->WindowHandle, PhEnableThemeSupport);

    // Generic initialization can replace list colors. Apply ours last, after
    // group view is enabled, so group cards/buttons use the PE Viewer palette.
    PvThemeApplyListView(Context->ListViewHandle);
    PvpStartPageApplyGroupSpacing(Context->ListViewHandle);
    PvThemeApplyControl(Context->SearchHandle);
    PvThemeApplyWindowFrame(Context->WindowHandle);
    RedrawWindow(Context->WindowHandle, NULL, NULL,
        RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
    Context->UpdatingTheme = FALSE;
}

INT_PTR CALLBACK PvpStartPageDialogProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_STARTPAGE_CONTEXT context;

    // Note: This arrives before WM_INITDIALOG so it can't rely on the context. (dmex)
    if (WindowMessage == WM_GETMINMAXINFO)
    {
        LONG dpiValue = PhGetWindowDpi(WindowHandle);
        PMINMAXINFO minMaxInfo = (PMINMAXINFO)lParam;

        minMaxInfo->ptMinTrackSize.x = PhScaleToDisplay(400, dpiValue);
        minMaxInfo->ptMinTrackSize.y = PhScaleToDisplay(260, dpiValue);
        return TRUE;
    }

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_STARTPAGE_CONTEXT));
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
            HICON smallIcon;
            HICON largeIcon;
            LONG dpiValue;

            context->UpdatingTheme = TRUE;
            context->WindowHandle = WindowHandle;
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_START_RECENT);
            context->SearchHandle = GetDlgItem(WindowHandle, IDC_START_SEARCH);
            dpiValue = PhGetWindowDpi(WindowHandle);

            PhCenterWindow(WindowHandle, NULL);

            PhGetStockApplicationIcon(&smallIcon, &largeIcon, dpiValue);
            SendMessage(WindowHandle, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);
            SendMessage(WindowHandle, WM_SETICON, ICON_BIG, (LPARAM)largeIcon);

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_START_TITLE), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->SearchHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_START_GETSTARTED), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_START_INTRO), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_START_OPEN_FILE), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDCANCEL), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PvCreateSearchControl(
                WindowHandle,
                context->SearchHandle,
                L"Search recent (Alt+S)",
                PvpStartPageSearchCallback,
                context
                );

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 340, L"Path");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 140, L"Date modified");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"StartWindowListViewColumns", context->ListViewHandle);

            ListView_EnableGroupView(context->ListViewHandle, TRUE);

            PvCreateStartPageImageList(context);
            PvRefreshRecentFileList(context);

            PhLoadWindowPlacementFromSetting(L"StartWindowPosition", L"StartWindowSize", WindowHandle);

            DragAcceptFiles(WindowHandle, TRUE);

            context->UpdatingTheme = FALSE;
            PvpStartPageApplyTheme(context, FALSE);
        }
        break;
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_THEMECHANGED:
        PvpStartPageApplyTheme(context, TRUE);
        break;
    case PV_STARTPAGE_BEGIN_LOADING:
        {
            if (!PvStartPageLoading)
            {
                PvStartPageLoading = TRUE;

                // Hide the launcher controls, but keep its HWND/frame and palette.
                for (HWND child = GetWindow(WindowHandle, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT))
                {
                    ShowWindow(child, SW_HIDE);
                }

                UpdateWindow(WindowHandle);
            }
        }
        return TRUE;
    case WM_CLOSE:
        if (PvStartPageLoading)
        {
            // Cancel the pending transition using the model owner's normal cleanup.
            HWND propertiesWindow = PvGetPePropertiesWindowHandle();
            if (propertiesWindow)
                DestroyWindow(propertiesWindow);
        }
        else
            PhClearReference(&PvFileName);
        DestroyWindow(WindowHandle);
        return TRUE;
    case WM_DESTROY:
        {
            PvStartPageWindowHandle = NULL;
        }

        {
            PhSaveWindowPlacementToSetting(L"StartWindowPosition", L"StartWindowSize", WindowHandle);
            PhSaveListViewColumnsToSetting(L"StartWindowListViewColumns", context->ListViewHandle);

            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->RecentFileList)
                PvDestroyRecentFileList(context->RecentFileList);
            if (context->ImageListHandle)
                PhImageListDestroy(context->ImageListHandle);

            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            if (!PvStartPageLoading)
                PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_DPICHANGED:
        {
            PvCreateStartPageImageList(context);
            PvRefreshRecentFileList(context);
            PvpStartPageApplyTheme(context, FALSE);
        }
        break;
    case WM_COMMAND:
        {
            if (PvStartPageLoading && LOWORD(wParam) != IDCANCEL)
                return TRUE;
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_START_OPEN_FILE:
                {
                    if (PvSelectFile())
                        PostMessage(WindowHandle, PV_STARTPAGE_BEGIN_LOADING, 0, 0);
                }
                break;
            case IDCANCEL:
                {
                    SendMessage(WindowHandle, WM_CLOSE, 0, 0);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->hwndFrom == context->ListViewHandle && header->code == LVN_ITEMACTIVATE)
            {
                PPH_STRING fileName;

                if (fileName = PvpGetSelectedRecentFile(context))
                    PvpStartPageSelectFile(context, fileName);
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
                PvpShowRecentFileMenu(context, lParam);
        }
        break;
    case WM_DROPFILES:
        {
            HDROP dropHandle = (HDROP)wParam;
            WCHAR fileName[MAX_PATH] = L"";

            if (DragQueryFile(dropHandle, 0, fileName, RTL_NUMBER_OF(fileName)))
            {
                PPH_STRING dropFileName = PhCreateString(fileName);

                DragFinish(dropHandle);

                PvpStartPageSelectFile(context, dropFileName);

                PhDereferenceObject(dropFileName);
                return TRUE;
            }

            DragFinish(dropHandle);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        {
            HBRUSH brush;

            if (brush = PvThemeHandleCtlColor((HDC)wParam, WindowMessage == WM_CTLCOLORSTATIC))
                return (INT_PTR)brush;
        }
        break;
    }

    return FALSE;
}

BOOLEAN PvSelectFile(
    VOID
    )
{
    static PH_FILETYPE_FILTER filters[] =
    {
        { L"Supported files (*.exe;*.dll;*.com;*.ocx;*.sys;*.scr;*.cpl;*.ax;*.acm;*.lib;*.winmd;*.mui;*.mun;*.efi;*.pdb)", L"*.exe;*.dll;*.com;*.ocx;*.sys;*.scr;*.cpl;*.ax;*.acm;*.lib;*.winmd;*.mui;*.mun;*.efi;*.pdb" },
        { L"All files (*.*)", L"*.*" }
    };
    PVOID fileDialog;

    fileDialog = PhCreateOpenFileDialog();
    PhSetFileDialogOptions(fileDialog, PH_FILEDIALOG_SHOWHIDDEN | PH_FILEDIALOG_NOPATHVALIDATE);
    PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));
    if (PhShowFileDialog(NULL, fileDialog))
    {
        PhMoveReference(&PvFileName, PhGetFileDialogFileName(fileDialog));
        PhFreeFileDialog(fileDialog);
        return !PhIsNullOrEmptyString(PvFileName);
    }
    PhFreeFileDialog(fileDialog);
    return FALSE;
}
