/*
 * Process Hacker ToolStatus -
 *   main program
 *
 * Copyright (C) 2010-2012 wj32
 * Copyright (C) 2011-2012 dmex
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

#include "toolstatus.h"

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );
VOID NTAPI ProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );
VOID NTAPI TabPageUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );
VOID NTAPI LayoutPaddingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );
BOOLEAN NTAPI MessageLoopFilter(
    __in PMSG Message,
    __in PVOID Context
    );

LRESULT CALLBACK MainWndSubclassProc(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam,
    __in UINT_PTR uIdSubclass,
    __in DWORD_PTR dwRefData
    );

VOID ApplyToolbarSettings(
    VOID
    );
VOID DrawWindowBorderForTargeting(
    __in HWND hWnd
    );
VOID UpdateStatusBar(
    VOID
    );
VOID ShowStatusMenu(
    __in PPOINT Point
    );

#define TARGETING_MODE_NORMAL 0 // select process
#define TARGETING_MODE_THREAD 1 // select process and thread
#define TARGETING_MODE_KILL 2 // Find Window and Kill

#define NUMBER_OF_CONTROLS 3
#define NUMBER_OF_BUTTONS 7
#define NUMBER_OF_SEPARATORS 2

static HWND ReBarHandle = NULL;
static HWND TextboxHandle = NULL;
static HWND ToolBarHandle = NULL;
static HWND StatusBarHandle = NULL;
static HWND TargetingCurrentWindow = NULL;
static HFONT FontHandle = NULL;
static RECT ReBarRect = { 0 };
static RECT StatusBarRect = { 0 };
static ULONG StatusMask = 0;
static ULONG IdRangeBase = 0;
static ULONG TargetingMode = 0;
static ULONG ToolBarIdRangeBase = 0;
static ULONG ToolBarIdRangeEnd = 0;
static ULONG ProcessesUpdatedCount = 0;
static ULONG StatusBarMaxWidths[STATUS_COUNT] = { 0 };
static BOOLEAN TargetingWindow = FALSE;
static BOOLEAN TargetingCurrentWindowDraw = FALSE;
static BOOLEAN TargetingCompleted = FALSE;
static HIMAGELIST ToolBarImageList = NULL;
static HACCEL AcceleratorTable = NULL;

#define ID_SEARCH_CLEAR (WM_USER + 1)

static BOOLEAN WordMatch(
    __in PPH_STRINGREF Text,
    __in PPH_STRINGREF Search,
    __in BOOLEAN IgnoreCase
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    remainingPart = *Search;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, ' ', &part, &remainingPart);

        if (part.Length != 0)
        {
            if (PhFindStringInStringRef(Text, &part, IgnoreCase) != -1)
                return TRUE;
        }
    }

    return FALSE;
}

static VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    LOGFONT logFont;

    memset(&logFont, 0, sizeof(LOGFONT));

    logFont.lfHeight = 14;
    logFont.lfWeight = FW_NORMAL;

    wcscpy_s(
        logFont.lfFaceName, 
        _countof(logFont.lfFaceName), 
        L"MS Shell Dlg 2"
        );

    // Create the font handle
    FontHandle = CreateFontIndirect(&logFont);

    EnableToolBar = !!PhGetIntegerSetting(L"ProcessHacker.ToolStatus.EnableToolBar");
    EnableSearch = !!PhGetIntegerSetting(L"ProcessHacker.ToolStatus.EnableSearch"); 
    EnableStatusBar = !!PhGetIntegerSetting(L"ProcessHacker.ToolStatus.EnableStatusBar"); 

    StatusMask = PhGetIntegerSetting(L"ProcessHacker.ToolStatus.StatusMask");
    DisplayStyle = (TOOLBAR_DISPLAY_STYLE)PhGetIntegerSetting(L"ProcessHacker.ToolStatus.ToolbarDisplayStyle");    

    IdRangeBase = PhPluginReserveIds(NUMBER_OF_CONTROLS + NUMBER_OF_BUTTONS);
    ToolBarIdRangeBase = IdRangeBase + NUMBER_OF_CONTROLS;
    ToolBarIdRangeEnd = ToolBarIdRangeBase + NUMBER_OF_BUTTONS;
}

static VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    DialogBox(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        (HWND)Parameter,
        OptionsDlgProc
        );
}

LOGICAL DllMain(
    __in HINSTANCE Instance,
    __in ULONG Reason,
    __reserved PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;
            PH_SETTING_CREATE settings[] =
            {
                { IntegerSettingType, L"ProcessHacker.ToolStatus.EnableToolBar", L"1" },
                { IntegerSettingType, L"ProcessHacker.ToolStatus.EnableStatusBar", L"1" },
                { IntegerSettingType, L"ProcessHacker.ToolStatus.EnableSearch", L"1" },
                { IntegerSettingType, L"ProcessHacker.ToolStatus.ResolveGhostWindows", L"1" },
                { IntegerSettingType, L"ProcessHacker.ToolStatus.StatusMask", L"d" },
                { IntegerSettingType, L"ProcessHacker.ToolStatus.ToolbarDisplayStyle", L"1" }
            };

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.ToolStatus", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Toolbar and Status Bar";
            info->Author = L"dmex & wj32";
            info->Description = L"Adds a toolbar and a status bar.";
            info->HasOptions = TRUE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessesUpdated),
                ProcessesUpdatedCallback,
                NULL,
                &ProcessesUpdatedCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowTabChanged),
                TabPageUpdatedCallback,
                NULL,
                &TabPageCallbackRegistration
                );
            
            PhAddSettings(settings, _countof(settings));

            AcceleratorTable = LoadAccelerators(
                Instance,
                MAKEINTRESOURCE(IDR_MAINWND_ACCEL)
                );
        }
        break;
    }

    return TRUE;
}

static BOOLEAN ProcessTreeFilterCallback(
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    )
{
    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;

    // Check if the textbox actually contains anything.
    if (GetWindowTextLength(TextboxHandle) > 0)
    {
        BOOLEAN itemFound = FALSE;
        PH_STRINGREF pidStringRef;
        PPH_STRING textboxText;

        textboxText = PhGetWindowText(TextboxHandle);

        if (textboxText)
        {
            PhInitializeStringRef(&pidStringRef, processNode->ProcessItem->ProcessIdString);

            // Search process names
            if (processNode->ProcessItem->ProcessName)
            {
                if (WordMatch(&processNode->ProcessItem->ProcessName->sr, &textboxText->sr, TRUE))
                {
                    itemFound = TRUE;
                }
            }

            // Search process company
            if (processNode->ProcessItem->VersionInfo.CompanyName)
            {
                if (WordMatch(&processNode->ProcessItem->VersionInfo.CompanyName->sr, &textboxText->sr, TRUE))
                {
                    itemFound = TRUE;
                }
            }

            // Search process descriptions
            if (processNode->ProcessItem->VersionInfo.FileDescription)
            {
                if (WordMatch(&processNode->ProcessItem->VersionInfo.FileDescription->sr, &textboxText->sr, TRUE))
                {
                    itemFound = TRUE;
                }
            }

            // Search process PIDs
            if (WordMatch(&pidStringRef, &textboxText->sr, TRUE))
            {
                itemFound = TRUE;
            }

            PhDereferenceObject(textboxText);
        }

        return itemFound;
    }

    // show all items
    return TRUE;
}

static BOOLEAN ServiceTreeFilterCallback(
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    )
{
    PPH_SERVICE_NODE serviceNode = (PPH_SERVICE_NODE)Node;

    // Check if the textbox actually contains anything.
    if (GetWindowTextLength(TextboxHandle) > 0)
    {
        BOOLEAN itemFound = FALSE;
        PH_STRINGREF pidStringRef;
        PPH_STRING textboxText;

        textboxText = PhGetWindowText(TextboxHandle);

        if (textboxText)
        {
            PhInitializeStringRef(&pidStringRef, serviceNode->ServiceItem->ProcessIdString);

            // Search service name.
            if (serviceNode->ServiceItem->Name)
            {
                if (WordMatch(&serviceNode->ServiceItem->Name->sr, &textboxText->sr, TRUE))
                {
                    itemFound = TRUE;
                }
            }

            // Search service display name.
            if (serviceNode->ServiceItem->DisplayName)
            {
                if (WordMatch(&serviceNode->ServiceItem->DisplayName->sr, &textboxText->sr, TRUE))
                {
                    itemFound = TRUE;
                }
            }

            // Search process PIDs.
            if (WordMatch(&pidStringRef, &textboxText->sr, TRUE))
            {
                itemFound = TRUE;
            }

            PhDereferenceObject(textboxText);
        }

        return itemFound;
    }

    // show all items
    return TRUE;
}

static BOOLEAN NetworkTreeFilterCallback(
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    )
{
    PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)Node;

     // Check if the textbox actually contains anything.
    if (GetWindowTextLength(TextboxHandle) > 0)
    {
        PH_STRINGREF pidStringRef;
        BOOLEAN itemFound = FALSE;
        PPH_STRING textboxText;
        WCHAR pidString[32];

        textboxText = PhGetWindowText(TextboxHandle);

        if (textboxText)
        {
            // Search PID
            PhPrintUInt32(pidString, (ULONG)networkNode->NetworkItem->ProcessId);
            PhInitializeStringRef(&pidStringRef, pidString);

            // Search network process PIDs
            if (WordMatch(&pidStringRef, &textboxText->sr, TRUE))
            {
                itemFound = TRUE;
            }

            // Search connection process name
            if (networkNode->NetworkItem->ProcessName)
            {
                if (WordMatch(&networkNode->NetworkItem->ProcessName->sr, &textboxText->sr, TRUE))
                {
                    itemFound = TRUE;
                }
            }

            // Search connection local IP address
            if (networkNode->NetworkItem->LocalAddressString)
            {
                PH_STRINGREF localAddressRef;

                PhInitializeStringRef(&localAddressRef, networkNode->NetworkItem->LocalAddressString);

                if (WordMatch(&localAddressRef, &textboxText->sr, TRUE))
                {
                    itemFound = TRUE;
                }
            }

            if (networkNode->NetworkItem->LocalPortString)
            {
                PH_STRINGREF localPortRef;

                PhInitializeStringRef(&localPortRef, networkNode->NetworkItem->LocalPortString);

                if (WordMatch(&localPortRef, &textboxText->sr, TRUE))
                {
                    itemFound = TRUE;
                }
            }

            if (networkNode->NetworkItem->RemoteAddressString)
            {
                PH_STRINGREF remoteAddressRef;

                PhInitializeStringRef(&remoteAddressRef, networkNode->NetworkItem->RemoteAddressString);

                if (WordMatch(&remoteAddressRef, &textboxText->sr, TRUE))
                {
                    itemFound = TRUE;
                }
            }
                        
            if (networkNode->NetworkItem->RemotePortString)
            {
                PH_STRINGREF remotePortRef;

                PhInitializeStringRef(&remotePortRef, networkNode->NetworkItem->RemotePortString);

                if (WordMatch(&remotePortRef, &textboxText->sr, TRUE))
                {
                    itemFound = TRUE;
                }
            }

            if (networkNode->NetworkItem->RemoteHostString)
            {
                if (WordMatch(&networkNode->NetworkItem->RemoteHostString->sr, &textboxText->sr, TRUE))
                {
                    itemFound = TRUE;
                }
            }
          
            PhDereferenceObject(textboxText);
        }
        
        return itemFound;
    }

    // Textbox empty, allow all items to be shown.
    return TRUE;
}

static VOID RebarCreate(
    __in HWND ParentHandle
    )
{
    REBARINFO rebarInfo = { sizeof(REBARINFO) };

    // Create the rebar
    ReBarHandle = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        REBARCLASSNAME,
        NULL,
        WS_CHILD | WS_BORDER | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CCS_NODIVIDER | CCS_TOP,// | RBS_DBLCLKTOGGLE | RBS_VARHEIGHT,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        ParentHandle,
        NULL,
        (HINSTANCE)PluginInstance->DllBase,
        NULL
        );

    // no imagelist to attach to rebar
    SendMessage(ReBarHandle, RB_SETBARINFO, 0, (LPARAM)&rebarInfo);
}

static VOID RebarAddMenuItem(
    __in HWND WindowHandle,
    __in HWND ChildHandle,
    __in UINT ID,
    __in UINT cyMinChild,
    __in UINT cxMinChild
    )
{
    REBARBANDINFO rBandInfo = { REBARBANDINFO_V6_SIZE }; 
    rBandInfo.fMask = RBBIM_STYLE | RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
    rBandInfo.fStyle = RBBS_HIDETITLE | RBBS_CHILDEDGE | RBBS_NOGRIPPER | RBBS_FIXEDSIZE;
    
    rBandInfo.wID = ID;
    rBandInfo.hwndChild = ChildHandle;
    rBandInfo.cyMinChild = cyMinChild;
    rBandInfo.cxMinChild = cxMinChild;

    SendMessage(WindowHandle, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rBandInfo);
}

VOID ToolBarCreateImageList(
    __in HWND WindowHandle
    )
{
    // Create the toolbar imagelist
    ToolBarImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);
    // Set the number of images
    ImageList_SetImageCount(ToolBarImageList, 7);
    // Add the images to the imagelist - same index as the first tbButtonArray field
    PhSetImageListBitmap(ToolBarImageList, 0, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_ARROW_REFRESH));
    PhSetImageListBitmap(ToolBarImageList, 1, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_COG_EDIT));
    PhSetImageListBitmap(ToolBarImageList, 2, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_FIND));
    PhSetImageListBitmap(ToolBarImageList, 3, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_CHART_LINE));
    PhSetImageListBitmap(ToolBarImageList, 4, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION));
    PhSetImageListBitmap(ToolBarImageList, 5, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION_GO));
    PhSetImageListBitmap(ToolBarImageList, 6, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_CROSS));

    // Configure the toolbar imagelist
    SendMessage(WindowHandle, TB_SETIMAGELIST, 0, (LPARAM)ToolBarImageList); 
}

VOID ToolbarAddMenuItems(
    __in HWND WindowHandle
    )
{
    TBBUTTON tbButtonArray[] =
    {
        { 0, ToolBarIdRangeBase + 0, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)L"Refresh" },
        { 1, ToolBarIdRangeBase + 1, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)L"Options" },
        { 0, 0, 0, BTNS_SEP, { 0 }, 0, 0 },
        { 2, ToolBarIdRangeBase + 2, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)L"Find Handles or DLLs" },
        { 3, ToolBarIdRangeBase + 3, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)L"System Information" },
        { 0, 0, 0, BTNS_SEP, { 0 }, 0, 0 },
        { 4, ToolBarIdRangeBase + 4, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)L"Find Window" },
        { 5, ToolBarIdRangeBase + 5, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)L"Find Window and Thread" },
        { 6, ToolBarIdRangeBase + 6, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)L"Find Window and Kill" }
    };

    // Add the buttons to the toolbar
    SendMessage(WindowHandle, TB_ADDBUTTONS, _countof(tbButtonArray), (LPARAM)tbButtonArray);
}

static VOID NTAPI MainWindowShowingCallback(
     __in_opt PVOID Parameter,
     __in_opt PVOID Context
    )
{       
    RebarCreate(PhMainWndHandle);

    if (EnableToolBar)
    {
        ToolBarHandle = CreateWindowEx(
            0,
            TOOLBARCLASSNAME,
            NULL,
            WS_CHILD | WS_VISIBLE | CCS_NORESIZE | CCS_NODIVIDER | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT | TBSTYLE_DROPDOWN,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ReBarHandle,
            (HMENU)(IdRangeBase + 1),
            (HINSTANCE)PluginInstance->DllBase,
            NULL
            );

        // Set the toolbar struct size.
        SendMessage(ToolBarHandle, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        // Set the extended toolbar styles.
        SendMessage(ToolBarHandle, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_HIDECLIPPEDBUTTONS);
        
        ToolBarCreateImageList(ToolBarHandle);
        ToolbarAddMenuItems(ToolBarHandle);
 
        // inset the toolbar into the rebar control
        RebarAddMenuItem(ReBarHandle, ToolBarHandle, IdRangeBase + 1, 22, 0);
    }

    if (EnableSearch)
    {
        TextboxHandle = CreateWindowEx(
            0,//WS_EX_STATICEDGE,
            WC_EDIT,
            NULL,
            WS_CHILD | WS_VISIBLE,// | WS_BORDER,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            PhMainWndHandle,
            (HMENU)(IdRangeBase + 2),
            (HINSTANCE)PluginInstance->DllBase,
            NULL
            );

        // Set Searchbox control font
        SendMessage(TextboxHandle, WM_SETFONT, (WPARAM)FontHandle, MAKELPARAM(TRUE, 0));

        if (WindowsVersion < WINDOWS_VISTA)
        {
            // recalculate the margins
            SendMessage(
                TextboxHandle, 
                EM_SETMARGINS, 
                EC_LEFTMARGIN | EC_RIGHTMARGIN, 
                0
                );
        }
              
        // Set initial text
        Edit_SetCueBannerText(TextboxHandle, L"Search Processes (Ctrl+K)");

        // inset a paint region into the edit control NC window area       
        InsertButton(TextboxHandle, ID_SEARCH_CLEAR, 25);

        // inset the search (edit) control into the rebar control
        RebarAddMenuItem(ReBarHandle, TextboxHandle, IdRangeBase + 2, 20, 200);

        PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), (PPH_TN_FILTER_FUNCTION)ProcessTreeFilterCallback, NULL);
        PhAddTreeNewFilter(PhGetFilterSupportServiceTreeList(), (PPH_TN_FILTER_FUNCTION)ServiceTreeFilterCallback, NULL);
        PhAddTreeNewFilter(PhGetFilterSupportNetworkTreeList(), (PPH_TN_FILTER_FUNCTION)NetworkTreeFilterCallback, NULL);

        ApplyToolbarSettings();
    }

    if (EnableStatusBar)
    {
        StatusBarHandle = CreateWindowEx(
            0,
            STATUSCLASSNAME,
            NULL,
            WS_CHILD | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP | SBARS_TOOLTIPS,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            PhMainWndHandle,
            (HMENU)IdRangeBase,
            (HINSTANCE)PluginInstance->DllBase,
            NULL
            );
    }

    SetWindowSubclass(PhMainWndHandle, MainWndSubclassProc, 0, 0);
    
    PhRegisterMessageLoopFilter(MessageLoopFilter, NULL);
    PhRegisterCallback(
        ProcessHacker_GetCallbackLayoutPadding(PhMainWndHandle), 
        LayoutPaddingCallback, 
        NULL, 
        &LayoutPaddingCallbackRegistration
        );  
}

static VOID NTAPI ProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    ProcessesUpdatedCount++;

    if (EnableStatusBar)
        UpdateStatusBar();
}

static VOID NTAPI TabPageUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    INT index = (INT)Parameter;

    if (TextboxHandle)
    {
        switch (index)
        {
        case 0:
            {
                Edit_SetCueBannerText(TextboxHandle, L"Search Processes (Ctrl+K)");
            }
            break;
        case 1:
            {
                Edit_SetCueBannerText(TextboxHandle, L"Search Services (Ctrl+K)");
            }
            break;
        case 2:
            {
                Edit_SetCueBannerText(TextboxHandle, L"Search Network (Ctrl+K)");
            }
            break;
        default:
            {
                // Disable the textbox if we're on an unsupported tab.
                Edit_SetCueBannerText(TextboxHandle, L"Search Disabled");
            }
            break;
        }
    }
}

static VOID NTAPI LayoutPaddingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_LAYOUT_PADDING_DATA data = (PPH_LAYOUT_PADDING_DATA)Parameter;

    if (EnableToolBar)
    {
        data->Padding.top += ReBarRect.bottom; // Width
    }

    if (EnableStatusBar)
    {
        data->Padding.bottom += StatusBarRect.bottom;
    }
}

static VOID DrawWindowBorderForTargeting(
    __in HWND hWnd
    )
{
    RECT rect;
    HDC hdc;

    GetWindowRect(hWnd, &rect);
    hdc = GetWindowDC(hWnd);

    if (hdc)
    {
        ULONG penWidth = GetSystemMetrics(SM_CXBORDER) * 3;
        INT oldDc;
        HPEN pen;
        HBRUSH brush;

        oldDc = SaveDC(hdc);

        // Get an inversion effect.
        SetROP2(hdc, R2_NOT);

        pen = CreatePen(PS_INSIDEFRAME, penWidth, RGB(0x00, 0x00, 0x00));
        SelectObject(hdc, pen);

        brush = GetStockObject(NULL_BRUSH);
        SelectObject(hdc, brush);

        // Draw the rectangle.
        Rectangle(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top);

        // Cleanup.
        DeleteObject(pen);

        RestoreDC(hdc, oldDc);
        ReleaseDC(hWnd, hdc);
    }
}

static BOOLEAN NTAPI MessageLoopFilter(
    __in PMSG Message,
    __in PVOID Context
    )
{
    if (
        Message->hwnd == PhMainWndHandle ||
        IsChild(PhMainWndHandle, Message->hwnd)
        )
    {
        if (TranslateAccelerator(PhMainWndHandle, AcceleratorTable, Message))
            return TRUE;
    }

    return FALSE;
}

static LRESULT CALLBACK MainWndSubclassProc(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam,
    __in UINT_PTR uIdSubclass,
    __in DWORD_PTR dwRefData
    )
{
    switch (uMsg)
    {
    case WM_COMMAND:
        {
            ULONG id = (ULONG)(USHORT)LOWORD(wParam);
            ULONG toolbarId;
     
            switch (HIWORD(wParam))
            {
            case BN_CLICKED:
                {
                    if (id != ID_SEARCH_CLEAR)
                        break;

                    SetFocus(TextboxHandle);
                    Edit_SetSel(TextboxHandle, 0, -1);
                    SetWindowText(TextboxHandle, L"");

                    goto DefaultWndProc;
                }
                break;
            case EN_CHANGE:
                {
                    PhApplyTreeNewFilters(PhGetFilterSupportProcessTreeList());
                    PhApplyTreeNewFilters(PhGetFilterSupportServiceTreeList());
                    PhApplyTreeNewFilters(PhGetFilterSupportNetworkTreeList());
                    goto DefaultWndProc;
                }
            }

            if (EnableToolBar && id == ID_SEARCH)
            {
                SetFocus(TextboxHandle);
                Edit_SetSel(TextboxHandle, 0, -1);
            }

            if (id >= ToolBarIdRangeBase && id < ToolBarIdRangeEnd)
            {
                toolbarId = id - ToolBarIdRangeBase;

                switch (toolbarId)
                {
                case TIDC_REFRESH:
                    SendMessage(hWnd, WM_COMMAND, PHAPP_ID_VIEW_REFRESH, 0);
                    break;
                case TIDC_OPTIONS:
                    SendMessage(hWnd, WM_COMMAND, PHAPP_ID_HACKER_OPTIONS, 0);
                    break;
                case TIDC_FINDOBJ:
                    SendMessage(hWnd, WM_COMMAND, PHAPP_ID_HACKER_FINDHANDLESORDLLS, 0);
                    break;
                case TIDC_SYSINFO:
                    SendMessage(hWnd, WM_COMMAND, PHAPP_ID_VIEW_SYSTEMINFORMATION, 0);
                    break;
                }

                goto DefaultWndProc;
            }

            // If we're targeting and the user presses the Esc key, cancel the targeting.
            // We also make sure the window doesn't get closed, by filtering out the message.
            if (id == PHAPP_ID_ESC_EXIT && TargetingWindow)
            {
                TargetingWindow = FALSE;
                ReleaseCapture();

                goto DefaultWndProc;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            if (hdr->hwndFrom == ReBarHandle)
            {
                if (hdr->code == RBN_HEIGHTCHANGE)
                {
                    // HACK: Invoke LayoutPaddingCallback and adjust rebar for multiple rows.
                    PostMessage(PhMainWndHandle, WM_SIZE, 0L, 0L);

                    if (EnableToolBar)
                    {
                        PostMessage(ReBarHandle, WM_SIZE, 0, 0);
                        GetClientRect(ReBarHandle, &ReBarRect);
                    }

                    if (EnableStatusBar)
                    {
                        PostMessage(StatusBarHandle, WM_SIZE, 0, 0);
                        GetClientRect(StatusBarHandle, &StatusBarRect);
                    }
                }

                goto DefaultWndProc;
            }
            else if (hdr->hwndFrom == ToolBarHandle)
            {
                switch (hdr->code)
                {
                case TBN_BEGINDRAG:
                    {
                        LPNMTOOLBAR toolbar = (LPNMTOOLBAR)hdr;
                        ULONG id;

                        id = (ULONG)toolbar->iItem - ToolBarIdRangeBase;

                        if (id == TIDC_FINDWINDOW || id == TIDC_FINDWINDOWTHREAD || id == TIDC_FINDWINDOWKILL)
                        {
                            // Direct all mouse events to this window.
                            SetCapture(hWnd);
                            // Send the window to the bottom.
                            SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                            TargetingWindow = TRUE;
                            TargetingCurrentWindow = NULL;
                            TargetingCurrentWindowDraw = FALSE;
                            TargetingCompleted = FALSE;

                            switch (id)
                            {
                            case TIDC_FINDWINDOW:
                                TargetingMode = TARGETING_MODE_NORMAL;
                                break;
                            case TIDC_FINDWINDOWTHREAD:
                                TargetingMode = TARGETING_MODE_THREAD;
                                break;
                            case TIDC_FINDWINDOWKILL:
                                TargetingMode = TARGETING_MODE_KILL;
                                break;
                            }

                            SendMessage(hWnd, WM_MOUSEMOVE, 0, 0);
                        }
                    }
                    break;
                }

                goto DefaultWndProc;
            }
            else if (hdr->hwndFrom == StatusBarHandle)
            {
                switch (hdr->code)
                {
                case NM_RCLICK:
                    {
                        POINT cursorPos;

                        GetCursorPos(&cursorPos);
                        ShowStatusMenu(&cursorPos);
                    }
                    break;
                }

                goto DefaultWndProc;
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            if (TargetingWindow)
            {
                POINT cursorPos;
                HWND windowOverMouse;
                ULONG processId;
                ULONG threadId;

                GetCursorPos(&cursorPos);
                windowOverMouse = WindowFromPoint(cursorPos);

                if (TargetingCurrentWindow != windowOverMouse)
                {
                    if (TargetingCurrentWindow && TargetingCurrentWindowDraw)
                    {
                        // Invert the old border (to remove it).
                        DrawWindowBorderForTargeting(TargetingCurrentWindow);
                    }

                    if (windowOverMouse)
                    {
                        threadId = GetWindowThreadProcessId(windowOverMouse, &processId);

                        // Draw a rectangle over the current window (but not if it's one of our own).
                        if (UlongToHandle(processId) != NtCurrentProcessId())
                        {
                            DrawWindowBorderForTargeting(windowOverMouse);
                            TargetingCurrentWindowDraw = TRUE;
                        }
                        else
                        {
                            TargetingCurrentWindowDraw = FALSE;
                        }
                    }

                    TargetingCurrentWindow = windowOverMouse;
                }

                goto DefaultWndProc;
            }
        }
        break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        {
            if (TargetingWindow)
            {
                ULONG processId;
                ULONG threadId;

                TargetingCompleted = TRUE;

                // Bring the window back to the top, and preserve the Always on Top setting.
                SetWindowPos(PhMainWndHandle, PhGetIntegerSetting(L"MainWindowAlwaysOnTop") ? HWND_TOPMOST : HWND_TOP,
                    0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                TargetingWindow = FALSE;
                ReleaseCapture();

                if (TargetingCurrentWindow)
                {
                    if (TargetingCurrentWindowDraw)
                    {
                        // Remove the border on the window we found.
                        DrawWindowBorderForTargeting(TargetingCurrentWindow);
                    }

                    if (PhGetIntegerSetting(L"ProcessHacker.ToolStatus.ResolveGhostWindows"))
                    {
                        // This is an undocumented function exported by user32.dll that
                        // retrieves the hung window represented by a ghost window.
                        static HWND (WINAPI *HungWindowFromGhostWindow_I)(
                            __in HWND hWnd
                            );

                        if (!HungWindowFromGhostWindow_I)
                            HungWindowFromGhostWindow_I = PhGetProcAddress(L"user32.dll", "HungWindowFromGhostWindow");

                        if (HungWindowFromGhostWindow_I)
                        {
                            HWND hungWindow = HungWindowFromGhostWindow_I(TargetingCurrentWindow);

                            // The call will have failed if the window wasn't actually a ghost
                            // window.
                            if (hungWindow)
                                TargetingCurrentWindow = hungWindow;
                        }
                    }

                    threadId = GetWindowThreadProcessId(TargetingCurrentWindow, &processId);

                    if (threadId && processId && UlongToHandle(processId) != NtCurrentProcessId())
                    {
                        PPH_PROCESS_NODE processNode;

                        processNode = PhFindProcessNode(UlongToHandle(processId));

                        if (processNode)
                        {
                            ProcessHacker_SelectTabPage(hWnd, 0);
                            ProcessHacker_SelectProcessNode(hWnd, processNode);
                        }

                        switch (TargetingMode)
                        {
                        case TARGETING_MODE_THREAD:
                            {
                                PPH_PROCESS_PROPCONTEXT propContext;
                                PPH_PROCESS_ITEM processItem;

                                if (processItem = PhReferenceProcessItem(UlongToHandle(processId)))
                                {
                                    if (propContext = PhCreateProcessPropContext(hWnd, processItem))
                                    {
                                        PhSetSelectThreadIdProcessPropContext(propContext, UlongToHandle(threadId));
                                        PhShowProcessProperties(propContext);
                                        PhDereferenceObject(propContext);
                                    }

                                    PhDereferenceObject(processItem);
                                }
                                else
                                {
                                    PhShowError(hWnd, L"The process (PID %u) does not exist.", processId);
                                }
                            }
                            break;
                        case TARGETING_MODE_KILL:
                            {
                                PPH_PROCESS_ITEM processItem;

                                if (processItem = PhReferenceProcessItem(UlongToHandle(processId)))
                                {
                                    PhUiTerminateProcesses(hWnd, &processItem, 1);
                                    PhDereferenceObject(processItem);
                                }
                                else
                                {
                                    PhShowError(hWnd, L"The process (PID %u) does not exist.", processId);
                                }
                            }
                            break;
                        }
                    }
                }

                goto DefaultWndProc;
            }
        }
        break;
    case WM_CAPTURECHANGED:
        {
            if (!TargetingCompleted)
            {
                // The user cancelled the targeting, probably by pressing the Esc key.

                // Remove the border on the currently selected window.
                if (TargetingCurrentWindow)
                {
                    if (TargetingCurrentWindowDraw)
                    {
                        // Remove the border on the window we found.
                        DrawWindowBorderForTargeting(TargetingCurrentWindow);
                    }
                }

                SetWindowPos(PhMainWndHandle, PhGetIntegerSetting(L"MainWindowAlwaysOnTop") ? HWND_TOPMOST : HWND_TOP,
                    0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                TargetingCompleted = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            if (EnableToolBar)
            {
                PostMessage(ReBarHandle, WM_SIZE, 0, 0);
                GetClientRect(ReBarHandle, &ReBarRect);
            }

            if (EnableStatusBar)
            {
                PostMessage(StatusBarHandle, WM_SIZE, 0, 0);
                GetClientRect(StatusBarHandle, &StatusBarRect);
            }

            ProcessHacker_InvalidateLayoutPadding(hWnd);
        }
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);

DefaultWndProc:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

VOID ApplyToolbarSettings(
    VOID
    )
{
    BOOLEAN buttonHasText[NUMBER_OF_BUTTONS + NUMBER_OF_SEPARATORS] = { TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE };
    ULONG i;

    if (EnableToolBar)
    {
        if (ToolBarHandle)
            ShowWindow(ToolBarHandle, SW_SHOW);
        if (ReBarHandle)
            ShowWindow(ReBarHandle, SW_SHOW);
    }
    else
    {         
        // Clear searchbox
        if (TextboxHandle)
        {
            Edit_SetSel(TextboxHandle, 0, -1);         
            SetWindowText(TextboxHandle, L"");
        }

        if (ToolBarHandle)
            ShowWindow(ToolBarHandle, SW_HIDE);
        if (ReBarHandle)
            ShowWindow(ReBarHandle, SW_HIDE);  
    }

    if (EnableStatusBar)
    {    
        if (StatusBarHandle)
            ShowWindow(StatusBarHandle, SW_SHOW);
    }
    else
    {
        if (StatusBarHandle)
            ShowWindow(StatusBarHandle, SW_HIDE);
    }

    for (i = 0; i < SendMessage(ToolBarHandle, TB_BUTTONCOUNT, NULL, NULL); i++) // NUMBER_OF_BUTTONS + NUMBER_OF_SEPARATORS
    {
        TBBUTTONINFO button = { sizeof(TBBUTTONINFO) };
        button.dwMask = TBIF_BYINDEX | TBIF_STYLE | TBIF_COMMAND;

        // Get settings for first button.
        SendMessage(ToolBarHandle, TB_GETBUTTONINFO, i, (LPARAM)&button);

        // Skip separator buttons.
        if (button.fsStyle != BTNS_SEP)
        {
            switch (DisplayStyle)
            {
            case ImageOnly:
                {
                    button.fsStyle = button.fsStyle | BTNS_AUTOSIZE;
                }
                break;
            case SelectiveText:
                {
                    button.fsStyle = button.fsStyle | BTNS_AUTOSIZE;

                    if (buttonHasText[i])
                    {
                        button.fsStyle = BTNS_SHOWTEXT;
                    }
                }
                break;
            default: //case AllText:
                {
                    button.fsStyle = BTNS_SHOWTEXT;
                }
                break;
            }

            // Set updated button info.
            SendMessage(ToolBarHandle, TB_SETBUTTONINFO, i, (LPARAM)&button);
        }
    }

    // Repaint the toolbar.
    InvalidateRect(ToolBarHandle, NULL, TRUE);
}

static VOID UpdateStatusBar(
    VOID
    )
{
    static ULONG lastTickCount = 0;

    PPH_STRING text[STATUS_COUNT];
    ULONG widths[STATUS_COUNT];
    ULONG i;
    ULONG index;
    ULONG count;
    HDC hdc;
    PH_PLUGIN_SYSTEM_STATISTICS statistics;
    BOOLEAN resetMaxWidths = FALSE;

    if (ProcessesUpdatedCount < 2)
        return;

    if (!(StatusMask & (STATUS_MAXIMUM - 1)))
    {
        // The status bar doesn't cope well with 0 parts.
        widths[0] = -1;
        SendMessage(StatusBarHandle, SB_SETPARTS, 1, (LPARAM)widths);
        SendMessage(StatusBarHandle, SB_SETTEXT, 0, (LPARAM)L"");
        return;
    }

    PhPluginGetSystemStatistics(&statistics);

    hdc = GetDC(StatusBarHandle);
    SelectObject(hdc, (HFONT)SendMessage(StatusBarHandle, WM_GETFONT, 0, 0));

    // Reset max. widths for Max. CPU Process and Max. I/O Process parts once in a while.
    {
        ULONG tickCount;

        tickCount = GetTickCount();

        if (tickCount - lastTickCount >= 10000)
        {
            resetMaxWidths = TRUE;
            lastTickCount = tickCount;
        }
    }

    count = 0;
    index = 0;

    for (i = STATUS_MINIMUM; i != STATUS_MAXIMUM; i <<= 1)
    {
        if (StatusMask & i)
        {
            SIZE size;
            PPH_PROCESS_ITEM processItem;
            ULONG width;

            switch (i)
            {
            case STATUS_CPUUSAGE:
                text[count] = PhFormatString(L"CPU Usage: %.2f%%", (statistics.CpuKernelUsage + statistics.CpuUserUsage) * 100);
                break;
            case STATUS_COMMIT:
                text[count] = PhFormatString(L"Commit Charge: %.2f%%",
                    (FLOAT)statistics.CommitPages * 100 / statistics.Performance->CommitLimit);
                break;
            case STATUS_PHYSICAL:
                text[count] = PhFormatString(L"Physical Memory: %.2f%%",
                    (FLOAT)statistics.PhysicalPages * 100 / PhSystemBasicInformation.NumberOfPhysicalPages);
                break;
            case STATUS_NUMBEROFPROCESSES:
                text[count] = PhConcatStrings2(L"Processes: ",
                    PhaFormatUInt64(statistics.NumberOfProcesses, TRUE)->Buffer);
                break;
            case STATUS_NUMBEROFTHREADS:
                text[count] = PhConcatStrings2(L"Threads: ",
                    PhaFormatUInt64(statistics.NumberOfThreads, TRUE)->Buffer);
                break;
            case STATUS_NUMBEROFHANDLES:
                text[count] = PhConcatStrings2(L"Handles: ",
                    PhaFormatUInt64(statistics.NumberOfHandles, TRUE)->Buffer);
                break;
            case STATUS_IOREADOTHER:
                text[count] = PhConcatStrings2(L"I/O R+O: ", PhaFormatSize(
                    statistics.IoReadDelta.Delta + statistics.IoOtherDelta.Delta, -1)->Buffer);
                break;
            case STATUS_IOWRITE:
                text[count] = PhConcatStrings2(L"I/O W: ", PhaFormatSize(
                    statistics.IoWriteDelta.Delta, -1)->Buffer);
                break;
            case STATUS_MAXCPUPROCESS:
                if (statistics.MaxCpuProcessId && (processItem = PhReferenceProcessItem(statistics.MaxCpuProcessId)))
                {
                    if (!PH_IS_FAKE_PROCESS_ID(processItem->ProcessId))
                    {
                        text[count] = PhFormatString(
                            L"%s (%u): %.2f%%",
                            processItem->ProcessName->Buffer,
                            (ULONG)processItem->ProcessId,
                            processItem->CpuUsage * 100
                            );
                    }
                    else
                    {
                        text[count] = PhFormatString(
                            L"%s: %.2f%%",
                            processItem->ProcessName->Buffer,
                            processItem->CpuUsage * 100
                            );
                    }

                    PhDereferenceObject(processItem);
                }
                else
                {
                    text[count] = PhCreateString(L"-");
                }

                if (resetMaxWidths)
                    StatusBarMaxWidths[index] = 0;

                break;
            case STATUS_MAXIOPROCESS:
                if (statistics.MaxIoProcessId && (processItem = PhReferenceProcessItem(statistics.MaxIoProcessId)))
                {
                    if (!PH_IS_FAKE_PROCESS_ID(processItem->ProcessId))
                    {
                        text[count] = PhFormatString(
                            L"%s (%u): %s",
                            processItem->ProcessName->Buffer,
                            (ULONG)processItem->ProcessId,
                            PhaFormatSize(processItem->IoReadDelta.Delta + processItem->IoWriteDelta.Delta + processItem->IoOtherDelta.Delta, -1)->Buffer
                            );
                    }
                    else
                    {
                        text[count] = PhFormatString(
                            L"%s: %s",
                            processItem->ProcessName->Buffer,
                            PhaFormatSize(processItem->IoReadDelta.Delta + processItem->IoWriteDelta.Delta + processItem->IoOtherDelta.Delta, -1)->Buffer
                            );
                    }

                    PhDereferenceObject(processItem);
                }
                else
                {
                    text[count] = PhCreateString(L"-");
                }

                if (resetMaxWidths)
                    StatusBarMaxWidths[index] = 0;

                break;
            }

            if (!GetTextExtentPoint32(hdc, text[count]->Buffer, (ULONG)text[count]->Length / 2, &size))
                size.cx = 200;

            if (count != 0)
                widths[count] = widths[count - 1];
            else
                widths[count] = 0;

            width = size.cx + 10;

            if (width <= StatusBarMaxWidths[index])
                width = StatusBarMaxWidths[index];
            else
                StatusBarMaxWidths[index] = width;

            widths[count] += width;

            count++;
        }
        else
        {
            StatusBarMaxWidths[index] = 0;
        }

        index++;
    }

    ReleaseDC(StatusBarHandle, hdc);

    SendMessage(StatusBarHandle, SB_SETPARTS, count, (LPARAM)widths);

    for (i = 0; i < count; i++)
    {
        SendMessage(StatusBarHandle, SB_SETTEXT, i, (LPARAM)text[i]->Buffer);
        PhDereferenceObject(text[i]);
    }
}

static VOID ShowStatusMenu(
    __in PPOINT Point
    )
{
    HMENU menu;
    HMENU subMenu;
    ULONG i;
    ULONG id;
    ULONG bit;

    menu = LoadMenu(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDR_STATUS)
        );

    subMenu = GetSubMenu(menu, 0);

    // Check the enabled items.
    for (i = STATUS_MINIMUM; i != STATUS_MAXIMUM; i <<= 1)
    {
        if (StatusMask & i)
        {
            switch (i)
            {
            case STATUS_CPUUSAGE:
                id = ID_STATUS_CPUUSAGE;
                break;
            case STATUS_COMMIT:
                id = ID_STATUS_COMMITCHARGE;
                break;
            case STATUS_PHYSICAL:
                id = ID_STATUS_PHYSICALMEMORY;
                break;
            case STATUS_NUMBEROFPROCESSES:
                id = ID_STATUS_NUMBEROFPROCESSES;
                break;
            case STATUS_NUMBEROFTHREADS:
                id = ID_STATUS_NUMBEROFTHREADS;
                break;
            case STATUS_NUMBEROFHANDLES:
                id = ID_STATUS_NUMBEROFHANDLES;
                break;
            case STATUS_IOREADOTHER:
                id = ID_STATUS_IO_RO;
                break;
            case STATUS_IOWRITE:
                id = ID_STATUS_IO_W;
                break;
            case STATUS_MAXCPUPROCESS:
                id = ID_STATUS_MAX_CPU_PROCESS;
                break;
            case STATUS_MAXIOPROCESS:
                id = ID_STATUS_MAX_IO_PROCESS;
                break;
            }

            CheckMenuItem(subMenu, id, MF_CHECKED);
        }
    }

    id = (ULONG)TrackPopupMenu(
        subMenu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
        Point->x,
        Point->y,
        0,
        PhMainWndHandle,
        NULL
        );

    DestroyMenu(menu);

    switch (id)
    {
    case ID_STATUS_CPUUSAGE:
        bit = STATUS_CPUUSAGE;
        break;
    case ID_STATUS_COMMITCHARGE:
        bit = STATUS_COMMIT;
        break;
    case ID_STATUS_PHYSICALMEMORY:
        bit = STATUS_PHYSICAL;
        break;
    case ID_STATUS_NUMBEROFPROCESSES:
        bit = STATUS_NUMBEROFPROCESSES;
        break;
    case ID_STATUS_NUMBEROFTHREADS:
        bit = STATUS_NUMBEROFTHREADS;
        break;
    case ID_STATUS_NUMBEROFHANDLES:
        bit = STATUS_NUMBEROFHANDLES;
        break;
    case ID_STATUS_IO_RO:
        bit = STATUS_IOREADOTHER;
        break;
    case ID_STATUS_IO_W:
        bit = STATUS_IOWRITE;
        break;
    case ID_STATUS_MAX_CPU_PROCESS:
        bit = STATUS_MAXCPUPROCESS;
        break;
    case ID_STATUS_MAX_IO_PROCESS:
        bit = STATUS_MAXIOPROCESS;
        break;
    default:
        return;
    }

    StatusMask ^= bit;
    PhSetIntegerSetting(L"ProcessHacker.ToolStatus.StatusMask", StatusMask);

    UpdateStatusBar();
}