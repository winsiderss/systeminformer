/*
 * Process Hacker Window Explorer -
 *   window tree dialog
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

#include "wndexp.h"
#include "resource.h"
#include <windowsx.h>

typedef struct _WINDOWS_CONTEXT
{
    HWND TreeNewHandle;
    WE_WINDOW_TREE_CONTEXT TreeContext;
    WE_WINDOW_SELECTOR Selector;

    PH_LAYOUT_MANAGER LayoutManager;

    HWND HighlightingWindow;
    ULONG HighlightingWindowCount;
} WINDOWS_CONTEXT, *PWINDOWS_CONTEXT;

typedef struct _ADD_CHILD_WINDOWS_CONTEXT
{
    PWINDOWS_CONTEXT Context;
    PWE_WINDOW_NODE Node;
    HANDLE FilterProcessId;
    BOOLEAN TopLevelWindows;
} ADD_CHILD_WINDOWS_CONTEXT, *PADD_CHILD_WINDOWS_CONTEXT;

VOID WepShowWindowsDialogCallback(
    __in PVOID Parameter
    );

INT_PTR CALLBACK WepWindowsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

static RECT MinimumSize = { -1, -1, -1, -1 };

VOID WeShowWindowsDialog(
    __in HWND ParentWindowHandle,
    __in PWE_WINDOW_SELECTOR Selector
    )
{
    PWINDOWS_CONTEXT context;

    context = PhAllocate(sizeof(WINDOWS_CONTEXT));
    memset(context, 0, sizeof(WINDOWS_CONTEXT));
    memcpy(&context->Selector, Selector, sizeof(WE_WINDOW_SELECTOR));

    ProcessHacker_Invoke(WE_PhMainWndHandle, WepShowWindowsDialogCallback, context);
}

VOID WepShowWindowsDialogCallback(
    __in PVOID Parameter
    )
{
    HWND hwnd;
    PWINDOWS_CONTEXT context = Parameter;

    hwnd = CreateDialogParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_WNDLIST),
        WE_PhMainWndHandle,
        WepWindowsDlgProc,
        (LPARAM)context
        );
    ShowWindow(hwnd, SW_SHOW);
}

VOID WepDeleteWindowSelector(
    __in PWE_WINDOW_SELECTOR Selector
    )
{
    switch (Selector->Type)
    {
    case WeWindowSelectorDesktop:
        PhDereferenceObject(Selector->Desktop.DesktopName);
        break;
    }
}

BOOL CALLBACK WepHasChildrenEnumWindowsProc(
    __in HWND hwnd,
    __in LPARAM lParam
    )
{
    PWE_WINDOW_NODE node = (PWE_WINDOW_NODE)lParam;

    node->HasChildren = TRUE;

    return FALSE;
}

VOID WepFillWindowInfo(
    __in PWE_WINDOW_NODE Node
    )
{
    HWND hwnd;
    ULONG threadId;
    ULONG processId;

    hwnd = Node->WindowHandle;

    GetClassName(hwnd, Node->WindowClass, sizeof(Node->WindowClass) / sizeof(WCHAR));
    Node->WindowText = PhGetWindowText(hwnd);

    if (!Node->WindowText)
        Node->WindowText = PhReferenceEmptyString();

    threadId = GetWindowThreadProcessId(hwnd, &processId);
    Node->ClientId.UniqueProcess = UlongToHandle(processId);
    Node->ClientId.UniqueThread = UlongToHandle(threadId);

    Node->WindowVisible = !!IsWindowVisible(hwnd);

    // Determine if the window has children.
    EnumChildWindows(hwnd, WepHasChildrenEnumWindowsProc, (LPARAM)Node);
}

BOOL CALLBACK WepEnumChildWindowsProc(
    __in HWND hwnd,
    __in LPARAM lParam
    )
{
    PADD_CHILD_WINDOWS_CONTEXT context = (PADD_CHILD_WINDOWS_CONTEXT)lParam;
    PWE_WINDOW_NODE childNode;

    // EnumChildWindows gives you all the child windows, even if they are
    // indirect descendants. We only want the direct descendants.
    if (!context->TopLevelWindows && context->Node && GetParent(hwnd) != context->Node->WindowHandle)
        return TRUE;

    if (context->FilterProcessId)
    {
        ULONG processId;

        GetWindowThreadProcessId(hwnd, &processId);

        if (context->FilterProcessId != UlongToHandle(processId))
            return TRUE;
    }

    childNode = WeAddWindowNode(&context->Context->TreeContext);
    childNode->WindowHandle = hwnd;
    WepFillWindowInfo(childNode);

    childNode->Node.Expanded = FALSE;

    if (context->Node)
    {
        // This is a child node.
        childNode->Parent = context->Node;
        PhAddItemList(context->Node->Children, childNode);
    }
    else
    {
        // This is a root node.
        PhAddItemList(context->Context->TreeContext.NodeRootList, childNode);
    }

    return TRUE;
}

VOID WepAddChildWindows(
    __in PWINDOWS_CONTEXT Context,
    __in PWE_WINDOW_NODE Node
    )
{
    ADD_CHILD_WINDOWS_CONTEXT context;

    memset(&context, 0, sizeof(ADD_CHILD_WINDOWS_CONTEXT));
    context.Context = Context;
    context.Node = Node;
    context.TopLevelWindows = FALSE;

    EnumChildWindows(Node->WindowHandle, WepEnumChildWindowsProc, (LPARAM)&context);
}

VOID WepAddTopLevelWindows(
    __in PWINDOWS_CONTEXT Context,
    __in PWE_WINDOW_NODE DesktopNode,
    __in_opt HANDLE FilterProcessId
    )
{
    ADD_CHILD_WINDOWS_CONTEXT context;

    memset(&context, 0, sizeof(ADD_CHILD_WINDOWS_CONTEXT));
    context.Context = Context;
    context.Node = DesktopNode;
    context.FilterProcessId = FilterProcessId;
    context.TopLevelWindows = TRUE;

    EnumWindows(WepEnumChildWindowsProc, (LPARAM)&context);
}

VOID WepAddThreadWindows(
    __in PWINDOWS_CONTEXT Context,
    __in HANDLE ThreadId
    )
{
    ADD_CHILD_WINDOWS_CONTEXT context;

    memset(&context, 0, sizeof(ADD_CHILD_WINDOWS_CONTEXT));
    context.Context = Context;
    context.Node = NULL;
    context.TopLevelWindows = TRUE;

    EnumThreadWindows((ULONG)ThreadId, WepEnumChildWindowsProc, (LPARAM)&context);
}

VOID WepAddDesktopWindows(
    __in PWINDOWS_CONTEXT Context,
    __in PWSTR DesktopName
    )
{
    ADD_CHILD_WINDOWS_CONTEXT context;
    HDESK desktopHandle;

    memset(&context, 0, sizeof(ADD_CHILD_WINDOWS_CONTEXT));
    context.Context = Context;
    context.Node = NULL;
    context.TopLevelWindows = TRUE;

    if (desktopHandle = OpenDesktop(DesktopName, 0, FALSE, DESKTOP_ENUMERATE))
    {
        EnumDesktopWindows(desktopHandle, WepEnumChildWindowsProc, (LPARAM)&context);
        CloseDesktop(desktopHandle);
    }
}

VOID WepRefreshWindows(
    __in PWINDOWS_CONTEXT Context
    )
{
    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);
    WeClearWindowTree(&Context->TreeContext);
    TreeNew_NodesStructured(Context->TreeNewHandle);

    switch (Context->Selector.Type)
    {
    case WeWindowSelectorAll:
        {
            PWE_WINDOW_NODE desktopNode;

            desktopNode = WeAddWindowNode(&Context->TreeContext);
            desktopNode->WindowHandle = GetDesktopWindow();
            WepFillWindowInfo(desktopNode);

            PhAddItemList(Context->TreeContext.NodeRootList, desktopNode);

            WepAddTopLevelWindows(Context, desktopNode, NULL);

            desktopNode->HasChildren = TRUE;
            desktopNode->Opened = TRUE;
        }
        break;
    case WeWindowSelectorThread:
        {
            WepAddThreadWindows(Context, Context->Selector.Thread.ThreadId);
        }
        break;
    case WeWindowSelectorProcess:
        {
            WepAddTopLevelWindows(Context, NULL, Context->Selector.Process.ProcessId);
        }
        break;
    case WeWindowSelectorDesktop:
        {
            WepAddDesktopWindows(Context, Context->Selector.Desktop.DesktopName->Buffer);
        }
        break;
    }

    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
}

PPH_STRING WepGetWindowTitleForSelector(
    __in PWE_WINDOW_SELECTOR Selector
    )
{
    PPH_STRING title;
    CLIENT_ID clientId;
    PPH_STRING clientIdName;

    switch (Selector->Type)
    {
    case WeWindowSelectorAll:
        {
            return PhCreateString(L"Windows - All");
        }
        break;
    case WeWindowSelectorThread:
        {
            return PhFormatString(L"Windows - Thread %u", (ULONG)Selector->Thread.ThreadId);
        }
        break;
    case WeWindowSelectorProcess:
        {
            clientId.UniqueProcess = Selector->Process.ProcessId;
            clientId.UniqueThread = NULL;
            clientIdName = PhGetClientIdName(&clientId);

            title = PhConcatStrings2(L"Windows - ", clientIdName->Buffer);
            PhDereferenceObject(clientIdName);

            return title;
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
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PWINDOWS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PWINDOWS_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PWINDOWS_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_STRING windowTitle;
            PH_RECTANGLE windowRectangle;

            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            WeInitializeWindowTree(hwndDlg, context->TreeNewHandle, &context->TreeContext);

            PhRegisterDialog(hwndDlg);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_LIST), NULL, PH_ANCHOR_ALL);

            if (MinimumSize.left == -1)
            {
                RECT rect;

                rect.left = 0;
                rect.top = 0;
                rect.right = 160;
                rect.bottom = 100;
                MapDialogRect(hwndDlg, &rect);
                MinimumSize = rect;
                MinimumSize.left = 0;
            }

            // Set up the window position and size.

            windowRectangle.Position = PhGetIntegerPairSetting(SETTING_NAME_WINDOWS_WINDOW_POSITION);
            windowRectangle.Size = PhGetIntegerPairSetting(SETTING_NAME_WINDOWS_WINDOW_SIZE);
            PhAdjustRectangleToWorkingArea(hwndDlg, &windowRectangle);

            MoveWindow(hwndDlg, windowRectangle.Left, windowRectangle.Top,
                windowRectangle.Width, windowRectangle.Height, FALSE);

            // Implement cascading by saving an offsetted rectangle.
            windowRectangle.Left += 20;
            windowRectangle.Top += 20;
            PhSetIntegerPairSetting(SETTING_NAME_WINDOWS_WINDOW_POSITION, windowRectangle.Position);

            windowTitle = WepGetWindowTitleForSelector(&context->Selector);
            SetWindowText(hwndDlg, windowTitle->Buffer);
            PhDereferenceObject(windowTitle);

            WepRefreshWindows(context);
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
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            //case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_REFRESH:
                WepRefreshWindows(context);
                break;
            case ID_SHOWCONTEXTMENU:
                {
                    POINT point;
                    PWE_WINDOW_NODE *windows;
                    ULONG numberOfWindows;
                    HMENU menu;
                    HMENU subMenu;

                    point.x = (SHORT)LOWORD(lParam);
                    point.y = (SHORT)HIWORD(lParam);

                    WeGetSelectedWindowNodes(
                        &context->TreeContext,
                        &windows,
                        &numberOfWindows
                        );

                    if (numberOfWindows != 0)
                    {
                        menu = LoadMenu(PluginInstance->DllBase, MAKEINTRESOURCE(IDR_WINDOW));
                        subMenu = GetSubMenu(menu, 0);
                        SetMenuDefaultItem(subMenu, ID_WINDOW_PROPERTIES, FALSE);

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
                                PhEnableMenuItem(subMenu, ID_WINDOW_MINIMIZE, FALSE);
                            else if (placement.showCmd == SW_MAXIMIZE)
                                PhEnableMenuItem(subMenu, ID_WINDOW_MAXIMIZE, FALSE);
                            else if (placement.showCmd == SW_NORMAL)
                                PhEnableMenuItem(subMenu, ID_WINDOW_RESTORE, FALSE);

                            // Visible

                            CheckMenuItem(subMenu, ID_WINDOW_VISIBLE,
                                (GetWindowLong(windows[0]->WindowHandle, GWL_STYLE) & WS_VISIBLE) ? MF_CHECKED : MF_UNCHECKED);

                            // Enabled

                            CheckMenuItem(subMenu, ID_WINDOW_ENABLED,
                                !(GetWindowLong(windows[0]->WindowHandle, GWL_STYLE) & WS_DISABLED) ? MF_CHECKED : MF_UNCHECKED);

                            // Always on Top

                            CheckMenuItem(subMenu, ID_WINDOW_ALWAYSONTOP,
                                (GetWindowLong(windows[0]->WindowHandle, GWL_EXSTYLE) & WS_EX_TOPMOST) ? MF_CHECKED : MF_UNCHECKED);

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
                                CheckMenuRadioItem(subMenu, ID_OPACITY_10, ID_OPACITY_OPAQUE, id, MF_BYCOMMAND);
                            }
                        }
                        else
                        {
                            PhEnableAllMenuItems(subMenu, FALSE);
                            PhEnableMenuItem(subMenu, ID_WINDOW_COPY, TRUE);
                        }

                        TrackPopupMenu(
                            subMenu,
                            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
                            point.x,
                            point.y,
                            0,
                            hwndDlg,
                            NULL
                            );

                        DestroyMenu(menu);
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
                        WeShowWindowProperties(WE_PhMainWndHandle, selectedNode->WindowHandle);
                }
                break;
            case ID_WINDOW_COPY:
                {
                    PPH_FULL_STRING text;

                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                    PhSetClipboardStringEx(hwndDlg, text->Buffer, text->Length);
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
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, MinimumSize.right, MinimumSize.bottom);
        }
        break;
    case WM_WE_PLUSMINUS:
        {
            PWE_WINDOW_NODE node = (PWE_WINDOW_NODE)lParam;

            if (!node->Opened)
            {
                TreeNew_SetRedraw(context->TreeNewHandle, FALSE);
                WepAddChildWindows(context, node);
                node->Opened = TRUE;
                TreeNew_SetRedraw(context->TreeNewHandle, TRUE);
            }
        }
        break;
    }

    return FALSE;
}
