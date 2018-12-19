/*
 * Process Hacker Window Explorer -
 *   window tree dialog
 *
 * Copyright (C) 2011 wj32
 * Copyright (C) 2016-2018 dmex
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

#include "wndexp.h"

typedef struct _WINDOWS_CONTEXT
{
    HWND TreeNewHandle;
    HWND SearchBoxHandle;
    WE_WINDOW_TREE_CONTEXT TreeContext;
    WE_WINDOW_SELECTOR Selector;

    PH_LAYOUT_MANAGER LayoutManager;

    HWND HighlightingWindow;
    ULONG HighlightingWindowCount;
} WINDOWS_CONTEXT, *PWINDOWS_CONTEXT;

VOID WepShowWindowsDialogCallback(
    _In_ PVOID Parameter
    );

INT_PTR CALLBACK WepWindowsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK WepWindowsPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID WeShowWindowsDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PWE_WINDOW_SELECTOR Selector
    )
{
    PWINDOWS_CONTEXT context;

    context = PhAllocateZero(sizeof(WINDOWS_CONTEXT));
    memcpy(&context->Selector, Selector, sizeof(WE_WINDOW_SELECTOR));

    ProcessHacker_Invoke(WE_PhMainWndHandle, WepShowWindowsDialogCallback, context);
}

VOID WeShowWindowsPropPage(
    _In_ PPH_PLUGIN_PROCESS_PROPCONTEXT Context,
    _In_ PWE_WINDOW_SELECTOR Selector
    )
{
    PWINDOWS_CONTEXT context;

    context = PhAllocateZero(sizeof(WINDOWS_CONTEXT));
    memcpy(&context->Selector, Selector, sizeof(WE_WINDOW_SELECTOR));

    PhAddProcessPropPage(
        Context->PropContext,
        PhCreateProcessPropPageContextEx(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_WNDLIST), WepWindowsPageProc, context)
        );
}

VOID WepShowWindowsDialogCallback(
    _In_ PVOID Parameter
    )
{
    HWND hwnd;
    PWINDOWS_CONTEXT context = Parameter;

    hwnd = CreateDialogParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_WNDLIST),
        NULL,
        WepWindowsDlgProc,
        (LPARAM)context
        );
    ShowWindow(hwnd, SW_SHOW);
}

VOID WepDeleteWindowSelector(
    _In_ PWE_WINDOW_SELECTOR Selector
    )
{
    switch (Selector->Type)
    {
    case WeWindowSelectorDesktop:
        PhDereferenceObject(Selector->Desktop.DesktopName);
        break;
    }
}

VOID WepFillWindowInfo(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ PWE_WINDOW_NODE Node
    )
{
    PPH_STRING windowText;
    ULONG threadId;
    ULONG processId;

    PhPrintPointer(Node->WindowHandleString, Node->WindowHandle);
    GetClassName(Node->WindowHandle, Node->WindowClass, sizeof(Node->WindowClass) / sizeof(WCHAR));

    if (PhGetWindowTextEx(Node->WindowHandle, PH_GET_WINDOW_TEXT_INTERNAL, &windowText) > 0)
        PhMoveReference(&Node->WindowText, windowText);

    if (PhIsNullOrEmptyString(Node->WindowText))
        PhMoveReference(&Node->WindowText, PhReferenceEmptyString());

    threadId = GetWindowThreadProcessId(Node->WindowHandle, &processId);
    Node->ClientId.UniqueProcess = UlongToHandle(processId);
    Node->ClientId.UniqueThread = UlongToHandle(threadId);
    Node->ThreadString = PhGetClientIdName(&Node->ClientId);

    Node->WindowVisible = !!IsWindowVisible(Node->WindowHandle);
    Node->HasChildren = !!FindWindowEx(Node->WindowHandle, NULL, NULL, NULL);

    if (processId)
    {
        HANDLE processHandle;
        PVOID instanceHandle;
        PPH_STRING fileName;

        if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, UlongToHandle(processId))))
        {
            if (!(instanceHandle = (PVOID)GetWindowLongPtr(Node->WindowHandle, GWLP_HINSTANCE)))
            {
                instanceHandle = (PVOID)GetClassLongPtr(Node->WindowHandle, GCLP_HMODULE);
            }

            if (instanceHandle)
            {
                if (NT_SUCCESS(PhGetProcessMappedFileName(processHandle, instanceHandle, &fileName)))
                {
                    PhMoveReference(&fileName, PhResolveDevicePrefix(fileName));
                    PhMoveReference(&fileName, PhGetBaseName(fileName));

                    PhMoveReference(&Node->ModuleString, fileName);
                }
            }

            NtClose(processHandle);
        }
    }

    if (Context->FilterSupport.FilterList) // Note: Apply filter after filling window node data. (dmex)
        Node->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &Node->Node);
}

PWE_WINDOW_NODE WepAddChildWindowNode(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_opt_ PWE_WINDOW_NODE ParentNode,
    _In_ HWND hwnd
    )
{
    PWE_WINDOW_NODE childNode;

    childNode = WeAddWindowNode(Context);
    childNode->WindowHandle = hwnd;

    WepFillWindowInfo(Context, childNode);

    if (ParentNode)
    {
        // This is a child node.
        childNode->Node.Expanded = TRUE;
        childNode->Parent = ParentNode;

        PhAddItemList(ParentNode->Children, childNode);
    }
    else
    {
        // This is a root node.
        childNode->Node.Expanded = TRUE;

        PhAddItemList(Context->NodeRootList, childNode);
    }

    return childNode;
}

VOID WepAddChildWindows(
    _In_ PWINDOWS_CONTEXT Context,
    _In_opt_ PWE_WINDOW_NODE ParentNode,
    _In_ HWND hwnd,
    _In_opt_ HANDLE FilterProcessId,
    _In_opt_ HANDLE FilterThreadId
    )
{
    HWND childWindow = NULL;
    ULONG i = 0;

    // We use FindWindowEx because EnumWindows doesn't return Metro app windows.
    // Set a reasonable limit to prevent infinite loops.
    while (i < 0x800 && (childWindow = FindWindowEx(hwnd, childWindow, NULL, NULL)))
    {
        ULONG processId;
        ULONG threadId;

        threadId = GetWindowThreadProcessId(childWindow, &processId);

        if (
            (!FilterProcessId || UlongToHandle(processId) == FilterProcessId) &&
            (!FilterThreadId || UlongToHandle(threadId) == FilterThreadId)
            )
        {
            PWE_WINDOW_NODE childNode = WepAddChildWindowNode(&Context->TreeContext, ParentNode, childWindow);

            if (childNode->HasChildren)
            {
                WepAddChildWindows(Context, childNode, childWindow, NULL, NULL);
            }
        }

        i++;
    }
}

BOOL CALLBACK WepEnumDesktopWindowsProc(
    _In_ HWND hwnd,
    _In_ LPARAM lParam
    )
{
    PWINDOWS_CONTEXT context = (PWINDOWS_CONTEXT)lParam;

    WepAddChildWindowNode(&context->TreeContext, NULL, hwnd);

    return TRUE;
}

VOID WepAddDesktopWindows(
    _In_ PWINDOWS_CONTEXT Context,
    _In_ PWSTR DesktopName
    )
{
    HDESK desktopHandle;

    if (desktopHandle = OpenDesktop(DesktopName, 0, FALSE, DESKTOP_ENUMERATE))
    {
        EnumDesktopWindows(desktopHandle, WepEnumDesktopWindowsProc, (LPARAM)Context);
        CloseDesktop(desktopHandle);
    }
}

VOID WepRefreshWindows(
    _In_ PWINDOWS_CONTEXT Context
    )
{
    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);
    WeClearWindowTree(&Context->TreeContext);

    switch (Context->Selector.Type)
    {
    case WeWindowSelectorAll:
        {
            PWE_WINDOW_NODE desktopNode;

            desktopNode = WeAddWindowNode(&Context->TreeContext);
            desktopNode->WindowHandle = GetDesktopWindow();
            WepFillWindowInfo(&Context->TreeContext, desktopNode);

            PhAddItemList(Context->TreeContext.NodeRootList, desktopNode);

            WepAddChildWindows(Context, desktopNode, desktopNode->WindowHandle, NULL, NULL);

            desktopNode->HasChildren = TRUE;
        }
        break;
    case WeWindowSelectorThread:
        {
            WepAddChildWindows(Context, NULL, GetDesktopWindow(), NULL, Context->Selector.Thread.ThreadId);
        }
        break;
    case WeWindowSelectorProcess:
        {
            WepAddChildWindows(Context, NULL, GetDesktopWindow(), Context->Selector.Process.ProcessId, NULL);
        }
        break;
    case WeWindowSelectorDesktop:
        {
            WepAddDesktopWindows(Context, Context->Selector.Desktop.DesktopName->Buffer);
        }
        break;
    }

    TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
}

PPH_STRING WepGetWindowTitleForSelector(
    _In_ PWE_WINDOW_SELECTOR Selector
    )
{
    switch (Selector->Type)
    {
    case WeWindowSelectorAll:
        {
            return PhCreateString(L"Windows - All");
        }
        break;
    case WeWindowSelectorThread:
        {
            return PhFormatString(L"Windows - Thread %lu", HandleToUlong(Selector->Thread.ThreadId));
        }
        break;
    case WeWindowSelectorProcess:
        {
            CLIENT_ID clientId;

            clientId.UniqueProcess = Selector->Process.ProcessId;
            clientId.UniqueThread = NULL;

            return PhConcatStrings2(L"Windows - ", PH_AUTO_T(PH_STRING, PhGetClientIdName(&clientId))->Buffer);
        }
        break;
    case WeWindowSelectorDesktop:
        {
            return PhFormatString(L"Windows - Desktop \"%s\"", Selector->Desktop.DesktopName->Buffer);
        }
        break;
    default:
        return PhCreateString(L"Windows");
    }
}

INT_PTR CALLBACK WepWindowsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWINDOWS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PWINDOWS_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PH_RECTANGLE windowRectangle;

            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)PH_LOAD_SHARED_ICON_SMALL(WE_PhInstanceHandle, MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER)));
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)PH_LOAD_SHARED_ICON_LARGE(WE_PhInstanceHandle, MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER)));

            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            context->SearchBoxHandle = GetDlgItem(hwndDlg, IDC_SEARCHEDIT);

            SetWindowText(hwndDlg, PH_AUTO_T(PH_STRING, WepGetWindowTitleForSelector(&context->Selector))->Buffer);

            PhCreateSearchControl(hwndDlg, context->SearchBoxHandle, L"Search Windows (Ctrl+K)");
            WeInitializeWindowTree(hwndDlg, context->TreeNewHandle, &context->TreeContext);

            PhRegisterDialog(hwndDlg);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SEARCHEDIT), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_LIST), NULL, PH_ANCHOR_ALL);

            // Set up the window position and size.
            windowRectangle.Position = PhGetIntegerPairSetting(SETTING_NAME_WINDOWS_WINDOW_POSITION);
            windowRectangle.Size = PhGetScalableIntegerPairSetting(SETTING_NAME_WINDOWS_WINDOW_SIZE, TRUE).Pair;
            PhAdjustRectangleToWorkingArea(NULL, &windowRectangle);

            MoveWindow(hwndDlg, windowRectangle.Left, windowRectangle.Top, windowRectangle.Width, windowRectangle.Height, FALSE);

            // Implement cascading by saving an offsetted rectangle.
            windowRectangle.Left += 20;
            windowRectangle.Top += 20;
            PhSetIntegerPairSetting(SETTING_NAME_WINDOWS_WINDOW_POSITION, windowRectangle.Position);

            WepRefreshWindows(context);

            // HACK
            //PhSetDialogFocus(GetParent(hwndDlg), GetDlgItem(GetParent(hwndDlg), IDCANCEL));
            PhSetDialogFocus(hwndDlg, context->TreeNewHandle);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {   
            PhSaveWindowPlacementToSetting(SETTING_NAME_WINDOWS_WINDOW_POSITION, SETTING_NAME_WINDOWS_WINDOW_SIZE, hwndDlg);  

            PhDeleteLayoutManager(&context->LayoutManager);

            PhUnregisterDialog(hwndDlg);

            WeDeleteWindowTree(&context->TreeContext);
            WepDeleteWindowSelector(&context->Selector);
            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case EN_CHANGE:
                {
                    PPH_STRING newSearchboxText;

                    if (GET_WM_COMMAND_HWND(wParam, lParam) != context->SearchBoxHandle)
                        break;

                    newSearchboxText = PH_AUTO(PhGetWindowText(context->SearchBoxHandle));

                    if (!PhEqualString(context->TreeContext.SearchboxText, newSearchboxText, FALSE))
                    {
                        PhSwapReference(&context->TreeContext.SearchboxText, newSearchboxText);

                        if (!PhIsNullOrEmptyString(context->TreeContext.SearchboxText))
                            WeExpandAllWindowNodes(&context->TreeContext, TRUE);

                        PhApplyTreeNewFilters(&context->TreeContext.FilterSupport);

                        TreeNew_NodesStructured(context->TreeNewHandle);
                        // PhInvokeCallback(&SearchChangedEvent, SearchboxText);
                    }
                }
                break;
            }

            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                DestroyWindow(hwndDlg);
                break;
            case IDC_REFRESH:
                {
                    WepRefreshWindows(context);

                    PhApplyTreeNewFilters(&context->TreeContext.FilterSupport);

                    TreeNew_NodesStructured(context->TreeNewHandle);
                }
                break;
            case ID_SHOWCONTEXTMENU:
                {
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
                    PWE_WINDOW_NODE *windows;
                    ULONG numberOfWindows;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM selectedItem;

                    WeGetSelectedWindowNodes(
                        &context->TreeContext,
                        &windows,
                        &numberOfWindows
                        );

                    if (numberOfWindows != 0)
                    {
                        menu = PhCreateEMenu();
                        PhLoadResourceEMenuItem(menu, PluginInstance->DllBase, MAKEINTRESOURCE(IDR_WINDOW), 0);
                        PhInsertCopyCellEMenuItem(menu, ID_WINDOW_COPY, context->TreeNewHandle, contextMenuEvent->Column);
                        PhSetFlagsEMenuItem(menu, ID_WINDOW_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

                        if (numberOfWindows == 1)
                        {
                            WINDOWPLACEMENT placement = { sizeof(placement) };
                            BYTE alpha;
                            ULONG flags;
                            ULONG i;
                            ULONG id;

                            // State

                            GetWindowPlacement(windows[0]->WindowHandle, &placement);

                            if (placement.showCmd == SW_MINIMIZE)
                                PhSetFlagsEMenuItem(menu, ID_WINDOW_MINIMIZE, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                            else if (placement.showCmd == SW_MAXIMIZE)
                                PhSetFlagsEMenuItem(menu, ID_WINDOW_MAXIMIZE, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                            else if (placement.showCmd == SW_NORMAL)
                                PhSetFlagsEMenuItem(menu, ID_WINDOW_RESTORE, PH_EMENU_DISABLED, PH_EMENU_DISABLED);

                            // Visible

                            PhSetFlagsEMenuItem(menu, ID_WINDOW_VISIBLE, PH_EMENU_CHECKED,
                                (GetWindowLong(windows[0]->WindowHandle, GWL_STYLE) & WS_VISIBLE) ? PH_EMENU_CHECKED : 0);

                            // Enabled

                            PhSetFlagsEMenuItem(menu, ID_WINDOW_ENABLED, PH_EMENU_CHECKED,
                                !(GetWindowLong(windows[0]->WindowHandle, GWL_STYLE) & WS_DISABLED) ? PH_EMENU_CHECKED : 0);

                            // Always on Top

                            PhSetFlagsEMenuItem(menu, ID_WINDOW_ALWAYSONTOP, PH_EMENU_CHECKED,
                                (GetWindowLong(windows[0]->WindowHandle, GWL_EXSTYLE) & WS_EX_TOPMOST) ? PH_EMENU_CHECKED : 0);

                            // Opacity

                            if (GetLayeredWindowAttributes(windows[0]->WindowHandle, NULL, &alpha, &flags))
                            {
                                if (!(flags & LWA_ALPHA))
                                    alpha = 255;
                            }
                            else
                            {
                                alpha = 255;
                            }

                            if (alpha == 255)
                            {
                                id = ID_OPACITY_OPAQUE;
                            }
                            else
                            {
                                id = 0;

                                // Due to integer division, we cannot use simple arithmetic to calculate which menu item to check.
                                for (i = 0; i < 10; i++)
                                {
                                    if (alpha == (BYTE)(255 * (i + 1) / 10))
                                    {
                                        id = ID_OPACITY_10 + i;
                                        break;
                                    }
                                }
                            }

                            if (id != 0)
                            {
                                PhSetFlagsEMenuItem(menu, id, PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK,
                                    PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
                            }
                        }
                        else
                        {
                            PhSetFlagsAllEMenuItems(menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                            PhSetFlagsEMenuItem(menu, ID_WINDOW_COPY, PH_EMENU_DISABLED, 0);
                        }

                        selectedItem = PhShowEMenu(
                            menu, 
                            hwndDlg, 
                            PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP, 
                            contextMenuEvent->Location.x, 
                            contextMenuEvent->Location.y
                            );

                        if (selectedItem && selectedItem->Id != -1)
                        {
                            BOOLEAN handled = FALSE;

                            handled = PhHandleCopyCellEMenuItem(selectedItem);
                        }

                        PhDestroyEMenu(menu);
                    }
                }
                break;
            case ID_WINDOW_BRINGTOFRONT:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        WINDOWPLACEMENT placement = { sizeof(placement) };

                        GetWindowPlacement(selectedNode->WindowHandle, &placement);

                        if (placement.showCmd == SW_SHOWMINIMIZED)
                        {
                            ShowWindowAsync(selectedNode->WindowHandle, SW_RESTORE);
                        }

                        SetForegroundWindow(selectedNode->WindowHandle);
                    }
                }
                break;
            case ID_WINDOW_RESTORE:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        ShowWindowAsync(selectedNode->WindowHandle, SW_RESTORE);
                    }
                }
                break;
            case ID_WINDOW_MINIMIZE:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        ShowWindowAsync(selectedNode->WindowHandle, SW_MINIMIZE);
                    }
                }
                break;
            case ID_WINDOW_MAXIMIZE:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        ShowWindowAsync(selectedNode->WindowHandle, SW_MAXIMIZE);
                    }
                }
                break;
            case ID_WINDOW_CLOSE:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        PostMessage(selectedNode->WindowHandle, WM_CLOSE, 0, 0);
                    }
                }
                break;
            case ID_WINDOW_VISIBLE:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (IsWindowVisible(selectedNode->WindowHandle))
                        {
                            selectedNode->WindowVisible = FALSE;
                            ShowWindowAsync(selectedNode->WindowHandle, SW_HIDE);
                        }
                        else
                        {
                            selectedNode->WindowVisible = TRUE;
                            ShowWindowAsync(selectedNode->WindowHandle, SW_SHOW);
                        }

                        PhInvalidateTreeNewNode(&selectedNode->Node, TN_CACHE_COLOR);
                        TreeNew_InvalidateNode(context->TreeNewHandle, &selectedNode->Node);
                    }
                }
                break;
            case ID_WINDOW_ENABLED:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        EnableWindow(selectedNode->WindowHandle, !IsWindowEnabled(selectedNode->WindowHandle));
                    }
                }
                break;
            case ID_WINDOW_ALWAYSONTOP:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        LOGICAL topMost;

                        topMost = GetWindowLong(selectedNode->WindowHandle, GWL_EXSTYLE) & WS_EX_TOPMOST;
                        SetWindowPos(selectedNode->WindowHandle, topMost ? HWND_NOTOPMOST : HWND_TOPMOST,
                            0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
                    }
                }
                break;
            case ID_OPACITY_10:
            case ID_OPACITY_20:
            case ID_OPACITY_30:
            case ID_OPACITY_40:
            case ID_OPACITY_50:
            case ID_OPACITY_60:
            case ID_OPACITY_70:
            case ID_OPACITY_80:
            case ID_OPACITY_90:
            case ID_OPACITY_OPAQUE:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        ULONG opacity;

                        opacity = ((ULONG)LOWORD(wParam) - ID_OPACITY_10) + 1;

                        if (opacity == 10)
                        {
                            // Remove the WS_EX_LAYERED bit since it is not needed.
                            PhSetWindowExStyle(selectedNode->WindowHandle, WS_EX_LAYERED, 0);
                            RedrawWindow(selectedNode->WindowHandle, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
                        }
                        else
                        {
                            // Add the WS_EX_LAYERED bit so opacity will work.
                            PhSetWindowExStyle(selectedNode->WindowHandle, WS_EX_LAYERED, WS_EX_LAYERED);
                            SetLayeredWindowAttributes(selectedNode->WindowHandle, 0, (BYTE)(255 * opacity / 10), LWA_ALPHA);
                        }
                    }
                }
                break;
            case ID_WINDOW_HIGHLIGHT:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (context->HighlightingWindow)
                        {
                            if (context->HighlightingWindowCount & 1)
                                WeInvertWindowBorder(context->HighlightingWindow);
                        }

                        context->HighlightingWindow = selectedNode->WindowHandle;
                        context->HighlightingWindowCount = 10;
                        SetTimer(hwndDlg, 9, 100, NULL);
                    }
                }
                break;
            case ID_WINDOW_GOTOTHREAD:
                {
                    PWE_WINDOW_NODE selectedNode;
                    PPH_PROCESS_ITEM processItem;
                    PPH_PROCESS_PROPCONTEXT propContext;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (processItem = PhReferenceProcessItem(selectedNode->ClientId.UniqueProcess))
                        {
                            if (propContext = PhCreateProcessPropContext(WE_PhMainWndHandle, processItem))
                            {
                                PhSetSelectThreadIdProcessPropContext(propContext, selectedNode->ClientId.UniqueThread);
                                PhShowProcessProperties(propContext);
                                PhDereferenceObject(propContext);
                            }

                            PhDereferenceObject(processItem);
                        }
                        else
                        {
                            PhShowError(hwndDlg, L"The process does not exist.");
                        }
                    }
                }
                break;
            case ID_WINDOW_PROPERTIES:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                        WeShowWindowProperties(hwndDlg, selectedNode->WindowHandle);
                }
                break;
            case ID_WINDOW_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                    PhSetClipboardString(hwndDlg, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            }
        }
        break;
    case WM_TIMER:
        {
            switch (wParam)
            {
            case 9:
                {
                    WeInvertWindowBorder(context->HighlightingWindow);

                    if (--context->HighlightingWindowCount == 0)
                        KillTimer(hwndDlg, 9);
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
    }

    return FALSE;
}

INT_PTR CALLBACK WepWindowsPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWINDOWS_CONTEXT context;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, NULL, &propPageContext, &processItem))
    {
        context = propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            context->SearchBoxHandle = GetDlgItem(hwndDlg, IDC_SEARCHEDIT);

            PhCreateSearchControl(hwndDlg, context->SearchBoxHandle, L"Search Windows (Ctrl+K)");

            WeInitializeWindowTree(hwndDlg, context->TreeNewHandle, &context->TreeContext);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SEARCHEDIT), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_LIST), NULL, PH_ANCHOR_ALL);

            WepRefreshWindows(context);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            WeDeleteWindowTree(&context->TreeContext);
            WepDeleteWindowSelector(&context->Selector);
            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (PhBeginPropPageLayout(hwndDlg, propPageContext))
                PhEndPropPageLayout(hwndDlg, propPageContext);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case EN_CHANGE:
                {
                    PPH_STRING newSearchboxText;

                    if (GET_WM_COMMAND_HWND(wParam, lParam) != context->SearchBoxHandle)
                        break;

                    newSearchboxText = PH_AUTO(PhGetWindowText(context->SearchBoxHandle));

                    if (!PhEqualString(context->TreeContext.SearchboxText, newSearchboxText, FALSE))
                    {
                        PhSwapReference(&context->TreeContext.SearchboxText, newSearchboxText);

                        if (!PhIsNullOrEmptyString(context->TreeContext.SearchboxText))
                            WeExpandAllWindowNodes(&context->TreeContext, TRUE);

                        PhApplyTreeNewFilters(&context->TreeContext.FilterSupport);

                        TreeNew_NodesStructured(context->TreeNewHandle);
                        // PhInvokeCallback(&SearchChangedEvent, SearchboxText);
                    }
                }
                break;
            }

            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_REFRESH:
                {
                    WepRefreshWindows(context);

                    PhApplyTreeNewFilters(&context->TreeContext.FilterSupport);

                    TreeNew_NodesStructured(context->TreeNewHandle);
                }
                break;
            case ID_SHOWCONTEXTMENU:
                {
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
                    PWE_WINDOW_NODE *windows;
                    ULONG numberOfWindows;
                    PPH_EMENU menu;
                    PPH_EMENU selectedItem;

                    WeGetSelectedWindowNodes(
                        &context->TreeContext,
                        &windows,
                        &numberOfWindows
                        );

                    if (numberOfWindows != 0)
                    {
                        menu = PhCreateEMenu();
                        PhLoadResourceEMenuItem(menu, PluginInstance->DllBase, MAKEINTRESOURCE(IDR_WINDOW), 0);
                        PhInsertCopyCellEMenuItem(menu, ID_WINDOW_COPY, context->TreeNewHandle, contextMenuEvent->Column);
                        PhSetFlagsEMenuItem(menu, ID_WINDOW_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

                        if (numberOfWindows == 1)
                        {
                            WINDOWPLACEMENT placement = { sizeof(placement) };
                            BYTE alpha;
                            ULONG flags;
                            ULONG i;
                            ULONG id;

                            // State

                            GetWindowPlacement(windows[0]->WindowHandle, &placement);

                            if (placement.showCmd == SW_MINIMIZE)
                                PhSetFlagsEMenuItem(menu, ID_WINDOW_MINIMIZE, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                            else if (placement.showCmd == SW_MAXIMIZE)
                                PhSetFlagsEMenuItem(menu, ID_WINDOW_MAXIMIZE, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                            else if (placement.showCmd == SW_NORMAL)
                                PhSetFlagsEMenuItem(menu, ID_WINDOW_RESTORE, PH_EMENU_DISABLED, PH_EMENU_DISABLED);

                            // Visible

                            PhSetFlagsEMenuItem(menu, ID_WINDOW_VISIBLE, PH_EMENU_CHECKED,
                                (GetWindowLong(windows[0]->WindowHandle, GWL_STYLE) & WS_VISIBLE) ? PH_EMENU_CHECKED : 0);

                            // Enabled

                            PhSetFlagsEMenuItem(menu, ID_WINDOW_ENABLED, PH_EMENU_CHECKED,
                                !(GetWindowLong(windows[0]->WindowHandle, GWL_STYLE) & WS_DISABLED) ? PH_EMENU_CHECKED : 0);

                            // Always on Top

                            PhSetFlagsEMenuItem(menu, ID_WINDOW_ALWAYSONTOP, PH_EMENU_CHECKED,
                                (GetWindowLong(windows[0]->WindowHandle, GWL_EXSTYLE) & WS_EX_TOPMOST) ? PH_EMENU_CHECKED : 0);

                            // Opacity

                            if (GetLayeredWindowAttributes(windows[0]->WindowHandle, NULL, &alpha, &flags))
                            {
                                if (!(flags & LWA_ALPHA))
                                    alpha = 255;
                            }
                            else
                            {
                                alpha = 255;
                            }

                            if (alpha == 255)
                            {
                                id = ID_OPACITY_OPAQUE;
                            }
                            else
                            {
                                id = 0;

                                // Due to integer division, we cannot use simple arithmetic to calculate which menu item to check.
                                for (i = 0; i < 10; i++)
                                {
                                    if (alpha == (BYTE)(255 * (i + 1) / 10))
                                    {
                                        id = ID_OPACITY_10 + i;
                                        break;
                                    }
                                }
                            }

                            if (id != 0)
                            {
                                PhSetFlagsEMenuItem(menu, id, PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK,
                                    PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
                            }
                        }
                        else
                        {
                            PhSetFlagsAllEMenuItems(menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                            PhSetFlagsEMenuItem(menu, ID_WINDOW_COPY, PH_EMENU_DISABLED, 0);
                        }

                        selectedItem = PhShowEMenu(
                            menu,
                            hwndDlg,
                            PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            contextMenuEvent->Location.x,
                            contextMenuEvent->Location.y
                            );

                        if (selectedItem && selectedItem->Id != -1)
                        {
                            BOOLEAN handled = FALSE;

                            handled = PhHandleCopyCellEMenuItem(selectedItem);
                        }

                        PhDestroyEMenu(menu);
                    }
                }
                break;
            case ID_WINDOW_BRINGTOFRONT:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        WINDOWPLACEMENT placement = { sizeof(placement) };

                        GetWindowPlacement(selectedNode->WindowHandle, &placement);

                        if (placement.showCmd == SW_MINIMIZE)
                            ShowWindowAsync(selectedNode->WindowHandle, SW_RESTORE);
                        else
                            SetForegroundWindow(selectedNode->WindowHandle);
                    }
                }
                break;
            case ID_WINDOW_RESTORE:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        ShowWindowAsync(selectedNode->WindowHandle, SW_RESTORE);
                    }
                }
                break;
            case ID_WINDOW_MINIMIZE:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        ShowWindowAsync(selectedNode->WindowHandle, SW_MINIMIZE);
                    }
                }
                break;
            case ID_WINDOW_MAXIMIZE:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        ShowWindowAsync(selectedNode->WindowHandle, SW_MAXIMIZE);
                    }
                }
                break;
            case ID_WINDOW_CLOSE:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        PostMessage(selectedNode->WindowHandle, WM_CLOSE, 0, 0);
                    }
                }
                break;
            case ID_WINDOW_VISIBLE:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (IsWindowVisible(selectedNode->WindowHandle))
                        {
                            selectedNode->WindowVisible = FALSE;
                            ShowWindowAsync(selectedNode->WindowHandle, SW_HIDE);
                        }
                        else
                        {
                            selectedNode->WindowVisible = TRUE;
                            ShowWindowAsync(selectedNode->WindowHandle, SW_SHOW);
                        }

                        PhInvalidateTreeNewNode(&selectedNode->Node, TN_CACHE_COLOR);
                        TreeNew_InvalidateNode(context->TreeNewHandle, &selectedNode->Node);
                    }
                }
                break;
            case ID_WINDOW_ENABLED:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        EnableWindow(selectedNode->WindowHandle, !IsWindowEnabled(selectedNode->WindowHandle));
                    }
                }
                break;
            case ID_WINDOW_ALWAYSONTOP:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        LOGICAL topMost;

                        topMost = GetWindowLong(selectedNode->WindowHandle, GWL_EXSTYLE) & WS_EX_TOPMOST;
                        SetWindowPos(selectedNode->WindowHandle, topMost ? HWND_NOTOPMOST : HWND_TOPMOST,
                            0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
                    }
                }
                break;
            case ID_OPACITY_10:
            case ID_OPACITY_20:
            case ID_OPACITY_30:
            case ID_OPACITY_40:
            case ID_OPACITY_50:
            case ID_OPACITY_60:
            case ID_OPACITY_70:
            case ID_OPACITY_80:
            case ID_OPACITY_90:
            case ID_OPACITY_OPAQUE:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        ULONG opacity;

                        opacity = ((ULONG)LOWORD(wParam) - ID_OPACITY_10) + 1;

                        if (opacity == 10)
                        {
                            // Remove the WS_EX_LAYERED bit since it is not needed.
                            PhSetWindowExStyle(selectedNode->WindowHandle, WS_EX_LAYERED, 0);
                            RedrawWindow(selectedNode->WindowHandle, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
                        }
                        else
                        {
                            // Add the WS_EX_LAYERED bit so opacity will work.
                            PhSetWindowExStyle(selectedNode->WindowHandle, WS_EX_LAYERED, WS_EX_LAYERED);
                            SetLayeredWindowAttributes(selectedNode->WindowHandle, 0, (BYTE)(255 * opacity / 10), LWA_ALPHA);
                        }
                    }
                }
                break;
            case ID_WINDOW_HIGHLIGHT:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (context->HighlightingWindow)
                        {
                            if (context->HighlightingWindowCount & 1)
                                WeInvertWindowBorder(context->HighlightingWindow);
                        }

                        context->HighlightingWindow = selectedNode->WindowHandle;
                        context->HighlightingWindowCount = 10;
                        SetTimer(hwndDlg, 9, 100, NULL);
                    }
                }
                break;
            case ID_WINDOW_GOTOTHREAD:
                {
                    PWE_WINDOW_NODE selectedNode;
                    PPH_PROCESS_ITEM processItem;
                    PPH_PROCESS_PROPCONTEXT propContext;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (processItem = PhReferenceProcessItem(selectedNode->ClientId.UniqueProcess))
                        {
                            if (propContext = PhCreateProcessPropContext(WE_PhMainWndHandle, processItem))
                            {
                                PhSetSelectThreadIdProcessPropContext(propContext, selectedNode->ClientId.UniqueThread);
                                PhShowProcessProperties(propContext);
                                PhDereferenceObject(propContext);
                            }

                            PhDereferenceObject(processItem);
                        }
                        else
                        {
                            PhShowError(hwndDlg, L"The process does not exist.");
                        }
                    }
                }
                break;
            case ID_WINDOW_PROPERTIES:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                        WeShowWindowProperties(hwndDlg, selectedNode->WindowHandle);
                }
                break;
            case ID_WINDOW_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                    PhSetClipboardString(hwndDlg, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            }
        }
        break;
    case WM_TIMER:
        {
            switch (wParam)
            {
            case 9:
                {
                    WeInvertWindowBorder(context->HighlightingWindow);

                    if (--context->HighlightingWindowCount == 0)
                        KillTimer(hwndDlg, 9);
                }
                break;
            }
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);  
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)GetDlgItem(hwndDlg, IDC_REFRESH));
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}
