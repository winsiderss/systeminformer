/*
 * Process Hacker Window Explorer - 
 *   window tree dialog
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

#include "wndexp.h"
#include "resource.h"

typedef struct _WINDOWS_CONTEXT
{
    HWND TreeListHandle;
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

    ProcessHacker_Invoke(PhMainWndHandle, WepShowWindowsDialogCallback, context);
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
        PhMainWndHandle,
        WepWindowsDlgProc,
        (LPARAM)context
        );
    ShowWindow(hwnd, SW_SHOW);
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

VOID WepRefreshWindows(
    __in PWINDOWS_CONTEXT Context
    )
{
    TreeList_SetRedraw(Context->TreeListHandle, FALSE);
    WeClearWindowTree(&Context->TreeContext);
    TreeList_NodesStructured(Context->TreeListHandle);

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
    }

    TreeList_SetRedraw(Context->TreeListHandle, TRUE);
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

            context->TreeListHandle = GetDlgItem(hwndDlg, IDC_LIST);
            WeInitializeWindowTree(hwndDlg, context->TreeListHandle, &context->TreeContext);

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

            PhLoadWindowPlacementFromSetting(NULL, SETTING_NAME_WINDOWS_WINDOW_SIZE, hwndDlg);

            windowTitle = WepGetWindowTitleForSelector(&context->Selector);
            SetWindowText(hwndDlg, windowTitle->Buffer);
            PhDereferenceObject(windowTitle);

            WepRefreshWindows(context);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveWindowPlacementToSetting(NULL, SETTING_NAME_WINDOWS_WINDOW_SIZE, hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);
            PhUnregisterDialog(hwndDlg);

            WeDeleteWindowTree(&context->TreeContext);
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
                    PWE_WINDOW_NODE *windows;
                    ULONG numberOfWindows;
                    HMENU menu;
                    HMENU subMenu;
                    POINT location;

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

                            GetWindowPlacement(windows[0]->WindowHandle, &placement);

                            if (placement.showCmd == SW_MINIMIZE)
                                PhEnableMenuItem(subMenu, ID_WINDOW_MINIMIZE, FALSE);
                            else if (placement.showCmd == SW_MAXIMIZE)
                                PhEnableMenuItem(subMenu, ID_WINDOW_MAXIMIZE, FALSE);
                            else if (placement.showCmd == SW_NORMAL)
                                PhEnableMenuItem(subMenu, ID_WINDOW_RESTORE, FALSE);
                        }
                        else
                        {
                            PhEnableAllMenuItems(subMenu, FALSE);
                            PhEnableMenuItem(subMenu, ID_WINDOW_COPY, TRUE);
                        }

                        location.x = LOWORD(lParam);
                        location.y = HIWORD(lParam);
                        ClientToScreen(context->TreeListHandle, &location);

                        TrackPopupMenu(
                            subMenu,
                            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
                            location.x,
                            location.y,
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
                        if (IsWindow(selectedNode->WindowHandle))
                        {
                            WINDOWPLACEMENT placement = { sizeof(placement) };

                            GetWindowPlacement(selectedNode->WindowHandle, &placement);

                            if (placement.showCmd == SW_MINIMIZE)
                                ShowWindowAsync(selectedNode->WindowHandle, SW_RESTORE);
                            else
                                SetForegroundWindow(selectedNode->WindowHandle);
                        }
                    }
                }
                break;
            case ID_WINDOW_RESTORE:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (IsWindow(selectedNode->WindowHandle))
                        {
                            ShowWindowAsync(selectedNode->WindowHandle, SW_RESTORE);
                        }
                    }
                }
                break;
            case ID_WINDOW_MINIMIZE:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (IsWindow(selectedNode->WindowHandle))
                        {
                            ShowWindowAsync(selectedNode->WindowHandle, SW_MINIMIZE);
                        }
                    }
                }
                break;
            case ID_WINDOW_MAXIMIZE:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (IsWindow(selectedNode->WindowHandle))
                        {
                            ShowWindowAsync(selectedNode->WindowHandle, SW_MAXIMIZE);
                        }
                    }
                }
                break;
            case ID_WINDOW_SHOWHIDE:
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

                        PhInvalidateTreeListNode(&selectedNode->Node, TLIN_COLOR);
                        InvalidateRect(context->TreeListHandle, NULL, TRUE);
                    }
                }
                break;
            case ID_WINDOW_CLOSE:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (IsWindow(selectedNode->WindowHandle))
                        {
                            PostMessage(selectedNode->WindowHandle, WM_CLOSE, 0, 0);
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
                            if (propContext = PhCreateProcessPropContext(PhMainWndHandle, processItem))
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
                    PhShowInformation(hwndDlg, L"Properties");
                }
                break;
            case ID_WINDOW_COPY:
                {
                    PPH_FULL_STRING text;

                    text = PhGetTreeListText(context->TreeListHandle, WEWNTLC_MAXIMUM);
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
                TreeList_SetRedraw(context->TreeListHandle, FALSE);
                WepAddChildWindows(context, node);
                node->Opened = TRUE;
                TreeList_SetRedraw(context->TreeListHandle, TRUE);
            }
        }
        break;
    }

    return FALSE;
}
