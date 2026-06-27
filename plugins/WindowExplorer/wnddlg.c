/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2016-2024
 *
 */

#include "wndexp.h"
#include <commdlg.h>

INT_PTR CALLBACK WepWindowsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK WepWindowsPageProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID WepRemoveWindowNode(
    _In_ PWE_WINDOW_NODE WindowNode,
    _In_opt_ PWE_WINDOW_TREE_CONTEXT Context
    );

HWND WepWindowsDialogHandle = NULL;
HANDLE WepWindowsDialogThreadHandle = NULL;
PH_EVENT WepWindowsInitializedEvent = PH_EVENT_INIT;
PH_STRINGREF WepEmptyWindowsText = PH_STRINGREF_INIT(L"There are no windows to display.");
#define WE_WM_FINDWINDOW (WM_APP + 502)

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS WepShowWindowsDialogThread(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    WepWindowsDialogHandle = PhCreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_WNDLIST),
        NULL,
        WepWindowsDlgProc,
        Parameter
        );

    PhSetEvent(&WepWindowsInitializedEvent);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == INT_ERROR)
            break;

        if (!IsDialogMessage(WepWindowsDialogHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
    PhResetEvent(&WepWindowsInitializedEvent);

    NtClose(WepWindowsDialogThreadHandle);
    WepWindowsDialogThreadHandle = NULL;
    WepWindowsDialogHandle = NULL;

    return STATUS_SUCCESS;
}

VOID WeShowWindowsDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PWE_WINDOW_SELECTOR Selector
    )
{
    if (!WepWindowsDialogThreadHandle)
    {
        PWINDOWS_CONTEXT context;

        context = PhCreateAlloc(sizeof(WINDOWS_CONTEXT));
        memset(context, 0, sizeof(WINDOWS_CONTEXT));
        memcpy(&context->Selector, Selector, sizeof(WE_WINDOW_SELECTOR));
        context->ColorNew = PhGetIntegerSetting(L"ColorNew");
        context->ColorRemoved = PhGetIntegerSetting(L"ColorRemoved");
        context->HighlightingDuration = PhGetIntegerSetting(L"HighlightingDuration");

        if (!NT_SUCCESS(PhCreateThreadEx(&WepWindowsDialogThreadHandle, WepShowWindowsDialogThread, context)))
        {
            PhDereferenceObject(context);
            PhShowError2(ParentWindowHandle, L"Unable to create the window.", L"%s", L"");
            return;
        }

        PhWaitForEvent(&WepWindowsInitializedEvent, NULL);
    }

    PostMessage(WepWindowsDialogHandle, WM_PH_SHOW_DIALOG, 0, 0);
}

VOID WeShowWindowsPropPage(
    _In_ PPH_PLUGIN_PROCESS_PROPCONTEXT Context,
    _In_ PWE_WINDOW_SELECTOR Selector
    )
{
    PWINDOWS_CONTEXT context;

    context = PhCreateAlloc(sizeof(WINDOWS_CONTEXT));
    memset(context, 0, sizeof(WINDOWS_CONTEXT));
    memcpy(&context->Selector, Selector, sizeof(WE_WINDOW_SELECTOR));
    context->ColorNew = PhGetIntegerSetting(L"ColorNew");
    context->ColorRemoved = PhGetIntegerSetting(L"ColorRemoved");
    context->HighlightingDuration = PhGetIntegerSetting(L"HighlightingDuration");

    PhAddProcessPropPage(
        Context->PropContext,
        PhCreateProcessPropPageContextEx(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_WNDLIST), WepWindowsPageProc, context)
        );
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

VOID WepRefreshWindows(
    _In_ PWINDOWS_CONTEXT Context
    )
{
    PhBoostProvider(&Context->ProviderRegistration, NULL);

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);
    WeClearWindowTree(&Context->TreeContext);
    WepAddEnumWindowsByViewMode(Context);
    if (Context->TreeContext.ViewMode == WeWindowViewModeParentChild ||
        Context->TreeContext.ViewMode == WeWindowViewModeOwner ||
        Context->TreeContext.ViewMode == WeWindowViewModeZOrder)
    {
        WeExpandAllWindowNodes(&Context->TreeContext, TRUE);
    }
    TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
}

//VOID WepEnumerateParentChildWindows(
//    _In_ PWINDOWS_CONTEXT Context,
//    _In_opt_ HANDLE FilterProcessId,
//    _In_opt_ HANDLE FilterThreadId
//    )
//{
//    // This logic is adapted from the commented-out WepRefreshWindows block
//    switch (Context->Selector.Type)
//    {
//    case WeWindowSelectorAll:
//        {
//            PWE_WINDOW_NODE desktopNode;
//            PWE_WINDOW_ITEM windowItem;
//
//            windowItem = WeCreateWindowItem(GetDesktopWindow());
//            windowItem->ParentHandle = NULL;
//
//            PhGetWindowClientId(windowItem->WindowHandle, &windowItem->ClientId);
//            windowItem->WindowVisible = !!IsWindowVisible(windowItem->WindowHandle);
//
//            // Populate window text and class for desktop window
//            {
//                ULONG windowTextLength = 0;
//                WCHAR windowText[0x100];
//
//                if (NT_SUCCESS(PhGetClassName(
//                    windowItem->WindowHandle,
//                    windowText,
//                    RTL_NUMBER_OF(windowText),
//                    &windowTextLength
//                    )) && windowTextLength != 0)
//                {
//                    windowItem->WindowClassText = PhCreateStringEx(windowText, windowTextLength * sizeof(WCHAR));
//                }
//
//                if (NT_SUCCESS(PhGetWindowTextToBuffer(
//                    windowItem->WindowHandle,
//                    PH_GET_WINDOW_TEXT_INTERNAL,
//                    windowText,
//                    RTL_NUMBER_OF(windowText),
//                    &windowTextLength
//                    )) && windowTextLength != 0)
//                {
//                    windowItem->WindowText = PhCreateStringEx(windowText, windowTextLength * sizeof(WCHAR));
//                }
//            }
//
//            desktopNode = WeAddWindowNode(&Context->TreeContext, windowItem, 0);
//
//            if (windowItem->WindowText)
//                PhDereferenceObject(windowItem->WindowText);
//
//            WepFillWindowInfo(&Context->TreeContext, desktopNode);
//
//            // Recursively add child windows of the desktop
//            WepAddEnumChildWindows(
//                Context,
//                desktopNode,
//                desktopNode->WindowHandle,
//                NULL,
//                NULL
//                );
//
//            // Add message-only windows
//            WepAddEnumChildWindows(
//                Context,
//                desktopNode,
//                HWND_MESSAGE,
//                NULL,
//                NULL
//                );
//
//            desktopNode->HasChildren = TRUE;
//        }
//        break;
//    case WeWindowSelectorThread:
//        {
//            WepAddEnumChildWindows(
//                Context,
//                NULL,
//                GetDesktopWindow(),
//                NULL,
//                Context->Selector.Thread.ThreadId
//                );
//
//            WepAddEnumChildWindows(
//                Context,
//                NULL,
//                HWND_MESSAGE,
//                NULL,
//                Context->Selector.Thread.ThreadId
//                );
//        }
//        break;
//    case WeWindowSelectorProcess:
//        {
//            WepAddEnumChildWindows(
//                Context,
//                NULL,
//                GetDesktopWindow(),
//                Context->Selector.Process.ProcessId,
//                NULL
//                );
//
//            WepAddEnumChildWindows(
//                Context,
//                NULL,
//                HWND_MESSAGE,
//                Context->Selector.Process.ProcessId,
//                NULL
//                );
//        }
//        break;
//    case WeWindowSelectorDesktop:
//        {
//            WepAddDesktopWindows(
//                Context,
//                PhGetString(Context->Selector.Desktop.DesktopName)
//                );
//        }
//        break;
//    }
//}
//
//VOID WepEnumerateZOrderWindows(
//    _In_ PWINDOWS_CONTEXT Context,
//    _In_opt_ HANDLE FilterProcessId,
//    _In_opt_ HANDLE FilterThreadId
//    )
//{
//    HWND currentWindow = GetWindow(GetDesktopWindow(), GW_CHILD); // Get the topmost top-level window
//
//    while (currentWindow)
//    {
//        CLIENT_ID clientId;
//        if (!NT_SUCCESS(PhGetWindowClientId(currentWindow, &clientId)))
//        {
//            currentWindow = GetWindow(currentWindow, GW_HWNDNEXT);
//            continue;
//        }
//
//        if (
//            (!FilterProcessId || clientId.UniqueProcess == FilterProcessId) &&
//            (!FilterThreadId || clientId.UniqueThread == FilterThreadId)
//            )
//        {
//            // Add currentWindow as a root node. In Z-order view, all top-level windows are "siblings" conceptually.
//            // The TreeNew callback will handle GW_HWNDNEXT for displaying sub-items.
//            PWE_WINDOW_NODE windowNode = WepAddChildWindowNode(&Context->TreeContext, NULL, currentWindow);
//            if (windowNode)
//            {
//                // Set HasChildren to TRUE if there's a next window in Z-order
//                windowNode->HasChildren = (GetWindow(currentWindow, GW_HWNDNEXT) != NULL);
//            }
//        }
//        currentWindow = GetWindow(currentWindow, GW_HWNDNEXT);
//    }
//}

// Helper function to find or create a node for an HWND
//PWE_WINDOW_NODE WepGetOrCreateWindowNode(
//    _In_ PWINDOWS_CONTEXT Context,
//    _In_ HWND WindowHandle
//    )
//{
//    PWE_WINDOW_NODE node = WeFindWindowNode(&Context->TreeContext, WindowHandle);
//    if (!node)
//    {
//        PWE_WINDOW_ITEM windowItem = WeCreateWindowItem(WindowHandle);
//        PhGetWindowClientId(WindowHandle, &windowItem->ClientId);
//        windowItem->WindowVisible = !!IsWindowVisible(WindowHandle);
//
//        // Populate text and class for the new window item
//        {
//            ULONG windowTextLength = 0;
//            WCHAR windowText[0x100];
//
//            if (NT_SUCCESS(PhGetClassName(
//                WindowHandle,
//                windowText,
//                RTL_NUMBER_OF(windowText),
//                &windowTextLength
//                )) && windowTextLength != 0)
//            {
//                windowItem->WindowClassText = PhCreateStringEx(windowText, windowTextLength * sizeof(WCHAR));
//            }
//
//            if (NT_SUCCESS(PhGetWindowTextToBuffer(
//                WindowHandle,
//                PH_GET_WINDOW_TEXT_INTERNAL,
//                windowText,
//                RTL_NUMBER_OF(windowText),
//                &windowTextLength
//                )) && windowTextLength != 0)
//            {
//                windowItem->WindowText = PhCreateStringEx(windowText, windowTextLength * sizeof(WCHAR));
//            }
//        }
//
//        node = WeAddWindowNode(&Context->TreeContext, windowItem, 0); // Add as root for now, parent will be set later
//        if (windowItem->WindowText)
//            PhDereferenceObject(windowItem->WindowText);
//        WepFillWindowInfo(&Context->TreeContext, node);
//    }
//    return node;
//}

//VOID WepEnumerateOwnerChainWindows(
//    _In_ PWINDOWS_CONTEXT Context,
//    _In_opt_ HANDLE FilterProcessId,
//    _In_opt_ HANDLE FilterThreadId
//    )
//{
//    // First pass: Add all top-level windows to NodeList and NodeRootList (temporarily as roots)
//    // Then second pass: Identify owner-owned relationships and adjust parent/children lists
//
//    PPH_LIST allTopLevelWindows = PhCreateList(100);
//    HWND currentWindow = GetWindow(GetDesktopWindow(), GW_CHILD); // Get the topmost top-level window
//
//    while (currentWindow)
//    {
//        CLIENT_ID clientId;
//        if (!NT_SUCCESS(PhGetWindowClientId(currentWindow, &clientId)))
//        {
//            currentWindow = GetWindow(currentWindow, GW_HWNDNEXT);
//            continue;
//        }
//
//        if (
//            (!FilterProcessId || clientId.UniqueProcess == FilterProcessId) &&
//            (!FilterThreadId || clientId.UniqueThread == FilterThreadId)
//            )
//        {
//            PhAddItemList(allTopLevelWindows, currentWindow);
//            // We'll create nodes for these in the next pass
//        }
//        currentWindow = GetWindow(currentWindow, GW_HWNDNEXT);
//    }
//
//    // Now process these windows to build owner-chain hierarchy
//    for (ULONG i = 0; i < allTopLevelWindows->Count; i++)
//    {
//        HWND windowHandle = (HWND)allTopLevelWindows->Items[i];
//        HWND ownerHandle = GetWindow(windowHandle, GW_OWNER);
//
//        PWE_WINDOW_NODE windowNode = WepGetOrCreateWindowNode(Context, windowHandle);
//
//        if (ownerHandle && ownerHandle != GetDesktopWindow()) // Check for a valid owner
//        {
//            PWE_WINDOW_NODE ownerNode = WepGetOrCreateWindowNode(Context, ownerHandle);
//            if (ownerNode)
//            {
//                // Remove from root list if it was added as a root initially
//                ULONG index;
//                if ((index = PhFindItemList(Context->TreeContext.NodeRootList, windowNode)) != ULONG_MAX)
//                {
//                    PhRemoveItemList(Context->TreeContext.NodeRootList, index);
//                }
//
//                // Set parent and add to children list for owner-owned relationship
//                // For owner chain, Node->Children will represent owned windows
//                if (!windowNode->WindowItem->ParentHandle) // Only set if not already parented (e.g. from GW_CHILD)
//                    windowNode->WindowItem->ParentHandle = ownerHandle;
//
//                windowNode->Parent = ownerNode; // This sets the logical parent for the tree
//                if (PhFindItemList(ownerNode->Children, windowNode) == ULONG_MAX) // Avoid duplicates
//                    PhAddItemList(ownerNode->Children, windowNode);
//                ownerNode->HasChildren = TRUE;
//            }
//        }
//        else
//        {
//            // Ensure it's in the root list if it has no owner or owner is desktop
//            ULONG index;
//            if ((index = PhFindItemList(Context->TreeContext.NodeRootList, windowNode)) == ULONG_MAX)
//            {
//                 PhAddItemList(Context->TreeContext.NodeRootList, windowNode);
//            }
//        }
//    }
//    PhDereferenceObject(allTopLevelWindows); // Free the temporary list
//}
//
//PPH_STRING WepGetWindowTitleForSelector(
//    _In_ PWE_WINDOW_SELECTOR Selector
//    )
//{
//    switch (Selector->Type)
//    {
//    case WeWindowSelectorAll:
//        {
//            return PhCreateString(L"Windows - All");
//        }
//        break;
//    case WeWindowSelectorThread:
//        {
//            return PhFormatString(L"Windows - Thread %lu", HandleToUlong(Selector->Thread.ThreadId));
//        }
//        break;
//    case WeWindowSelectorProcess:
//        {
//            CLIENT_ID clientId;
//
//            clientId.UniqueProcess = Selector->Process.ProcessId;
//            clientId.UniqueThread = NULL;
//
//            return PhConcatStrings2(L"Windows - ", PH_AUTO_T(PH_STRING, PhGetClientIdName(&clientId))->Buffer);
//        }
//        break;
//    case WeWindowSelectorDesktop:
//        {
//            return PhFormatString(L"Windows - Desktop \"%s\"", Selector->Desktop.DesktopName->Buffer);
//        }
//        break;
//    default:
//        return PhCreateString(L"Windows");
//    }
//}
//

// Forward declaration
LRESULT CALLBACK OverlayWndProc(HWND WindowHandle, UINT msg, WPARAM wParam, LPARAM lParam);

// Handle to the overlay window (global or static as needed)
static HWND g_OverlayWnd = NULL;

/**
 * Create the overlay window and show a border at the given rect (screen coordinates)
 *
 * \param rect
 */
void ShowScreenOverlayBox(const RECT* rect)
{
    //if (g_OverlayWnd)
    //    DestroyWindow(g_OverlayWnd);

    //// Register window class once
    //static ATOM overlayClass = 0;
    //if (!overlayClass)
    //{
    //    WNDCLASS wc = { 0 };
    //    wc.lpfnWndProc = OverlayWndProc;
    //    wc.hInstance = (HINSTANCE)PhInstanceHandle;
    //    wc.lpszClassName = L"ScreenOverlayBox";
    //    wc.hCursor = LoadCursor(NULL, IDC_CROSS);
    //    overlayClass = RegisterClass(&wc);
    //}

    //int width = rect->right - rect->left;
    //int height = rect->bottom - rect->top;

    //g_OverlayWnd = CreateWindowEx(
    //    WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
    //    L"ScreenOverlayBox", L"",
    //    WS_POPUP,
    //    rect->left, rect->top, width, height,
    //    NULL, NULL, (HINSTANCE)PhInstanceHandle, NULL
    //);

    //// Set transparency (0=fully transparent, 255=opaque)
    //SetLayeredWindowAttributes(g_OverlayWnd, 0, 1, LWA_ALPHA);

    //ShowWindow(g_OverlayWnd, SW_SHOWNOACTIVATE);
    //UpdateWindow(g_OverlayWnd);
}

/**
 * Destroy the overlay window when done
 *
 */
void HideScreenOverlayBox()
{
    if (g_OverlayWnd)
    {
        DestroyWindow(g_OverlayWnd);
        g_OverlayWnd = NULL;
    }
}

/**
 * Window procedure for the overlay
 *
 * \param WindowHandle
 * \param msg
 * \param wParam
 * \param lParam
 * \return
 */
LRESULT CALLBACK OverlayWndProc(HWND WindowHandle, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(WindowHandle, &ps);
            RECT rc;
            GetClientRect(WindowHandle, &rc);

            // Draw a thick border (e.g., 3px, color: red)
            HPEN pen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
            HBRUSH brush = (HBRUSH)PhGetStockBrush(NULL_BRUSH);
            HGDIOBJ oldPen = SelectPen(hdc, pen);
            HGDIOBJ oldBrush = SelectBrush(hdc, brush);

            Rectangle(hdc, 0, 0, rc.right, rc.bottom);

            SelectPen(hdc, oldPen);
            SelectBrush(hdc, oldBrush);
            DeletePen(pen);
            EndPaint(WindowHandle, &ps);
            return 0;
        }
    case WM_DESTROY:
        return 0;
    default:
        return DefWindowProc(WindowHandle, msg, wParam, lParam);
    }
}

/**
 * \brief WepCreateWindowMenu
 *
 * \param WindowMenu
 * \return
 */

PPH_EMENU WepCreateWindowMenu(
    _In_ PPH_EMENU WindowMenu,
    _In_ BOOLEAN IncludeProperties,
    _In_ BOOLEAN IncludeCopy
    )
{
    PPH_EMENU_ITEM menuItem;

    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_BRINGTOFRONT, L"Bring to front", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_RESTORE, L"Restore", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_MINIMIZE, L"Minimize", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_MAXIMIZE, L"Maximize", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_CLOSE, L"Close", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_DESTROY, L"Destroy", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(WindowMenu, PhCreateEMenuSeparator(), ULONG_MAX);

    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_VISIBLE, L"Visible", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_ENABLED, L"Enabled", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_ALWAYSONTOP, L"Always on top", NULL, NULL), ULONG_MAX);

    menuItem = PhCreateEMenuItem(0, 0, L"&Opacity", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_10, L"&10%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_20, L"&20%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_30, L"&30%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_40, L"&40%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_50, L"&50%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_60, L"&60%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_70, L"&70%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_80, L"&80%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_90, L"&90%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_OPAQUE, L"&Opaque", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(WindowMenu, menuItem, ULONG_MAX);

    PhInsertEMenuItem(WindowMenu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_INSPECT, L"&Inspect", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_SETDPI, L"DPI", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_OPENFILELOCATION, L"Open &file location", NULL, NULL), ULONG_MAX);

    PhInsertEMenuItem(WindowMenu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_HIGHLIGHT, L"Highlight", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_GOTOPROCESS, L"Go to process...", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_GOTOTHREAD, L"Go to thread...", NULL, NULL), ULONG_MAX);
    if (IncludeProperties)
        PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_PROPERTIES, L"Properties", NULL, NULL), ULONG_MAX);

    if (IncludeCopy)
    {
        PhInsertEMenuItem(WindowMenu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_COPY, L"Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
    }

    return WindowMenu;
}

VOID WepSetWindowMenuState(
    _In_ PPH_EMENU WindowMenu,
    _In_ HWND WindowHandle
    )
{
    WINDOWPLACEMENT placement = { sizeof(placement) };
    BYTE alpha;
    ULONG flags;
    ULONG i;
    ULONG id;

    GetWindowPlacement(WindowHandle, &placement);

    if (placement.showCmd == SW_MINIMIZE)
        PhSetFlagsEMenuItem(WindowMenu, ID_WINDOW_MINIMIZE, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    else if (placement.showCmd == SW_MAXIMIZE)
        PhSetFlagsEMenuItem(WindowMenu, ID_WINDOW_MAXIMIZE, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    else if (placement.showCmd == SW_NORMAL)
        PhSetFlagsEMenuItem(WindowMenu, ID_WINDOW_RESTORE, PH_EMENU_DISABLED, PH_EMENU_DISABLED);

    PhSetFlagsEMenuItem(WindowMenu, ID_WINDOW_VISIBLE, PH_EMENU_CHECKED,
        WeIsWindowEffectivelyVisible(WindowHandle) ? PH_EMENU_CHECKED : 0);
    PhSetFlagsEMenuItem(WindowMenu, ID_WINDOW_ENABLED, PH_EMENU_CHECKED,
        IsWindowEnabled(WindowHandle) ? PH_EMENU_CHECKED : 0);
    PhSetFlagsEMenuItem(WindowMenu, ID_WINDOW_ALWAYSONTOP, PH_EMENU_CHECKED,
        (GetWindowLong(WindowHandle, GWL_EXSTYLE) & WS_EX_TOPMOST) ? PH_EMENU_CHECKED : 0);

    if (GetLayeredWindowAttributes(WindowHandle, NULL, &alpha, &flags))
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
        PhSetFlagsEMenuItem(WindowMenu, id, PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK,
            PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
    }
}

BOOLEAN WepExecuteWindowCommand(
    _In_ HWND ParentWindowHandle,
    _In_ HWND WindowHandle,
    _In_ ULONG CommandId
    )
{
    CLIENT_ID clientId;

    if (!IsWindow(WindowHandle))
        return FALSE;

    switch (CommandId)
    {
    case ID_WINDOW_BRINGTOFRONT:
        {
            WINDOWPLACEMENT placement = { sizeof(placement) };

            if (GetWindowPlacement(WindowHandle, &placement) &&
                (placement.showCmd == SW_SHOWMINIMIZED || placement.showCmd == SW_MINIMIZE))
            {
                ShowWindowAsync(WindowHandle, SW_RESTORE);
            }

            SetForegroundWindow(WindowHandle);
        }
        break;
    case ID_WINDOW_RESTORE:
        ShowWindowAsync(WindowHandle, SW_RESTORE);
        break;
    case ID_WINDOW_MINIMIZE:
        ShowWindowAsync(WindowHandle, SW_MINIMIZE);
        break;
    case ID_WINDOW_MAXIMIZE:
        ShowWindowAsync(WindowHandle, SW_MAXIMIZE);
        break;
    case ID_WINDOW_CLOSE:
        PostMessage(WindowHandle, WM_CLOSE, 0, 0);
        break;
    case ID_WINDOW_DESTROY:
        {
            NTSTATUS status;

            if (!NT_SUCCESS(PhGetWindowClientId(WindowHandle, &clientId)))
                return FALSE;

            status = WeDestroyRemoteWindow(WindowHandle, clientId.UniqueProcess);

            if (!NT_SUCCESS(status))
                PhShowStatus(ParentWindowHandle, L"Unable to destroy the window.", status, 0);
        }
        break;
    case ID_WINDOW_VISIBLE:
        {
            if (WeIsWindowEffectivelyVisible(WindowHandle))
            {
                ShowWindowAsync(WindowHandle, SW_HIDE);
            }
            else
            {
                BOOLEAN showParents = TRUE;
                HWND parentWindow = GetParent(WindowHandle);

                while (parentWindow && parentWindow != GetDesktopWindow())
                {
                    if (!IsWindowVisible(parentWindow))
                    {
                        showParents = MessageBoxW(
                            ParentWindowHandle,
                            L"The parent window(s) of this window are currently invisible.\n\n"
                            L"Do you want to make the parent window(s) visible along with this window?",
                            L"Confirm Parent Visibility Change",
                            MB_YESNO | MB_ICONQUESTION
                            ) == IDYES;
                        break;
                    }

                    parentWindow = GetParent(parentWindow);
                }

                ShowWindowAsync(WindowHandle, SW_SHOW);

                if (showParents)
                {
                    parentWindow = GetParent(WindowHandle);

                    while (parentWindow && parentWindow != GetDesktopWindow())
                    {
                        if (!IsWindowVisible(parentWindow))
                            ShowWindowAsync(parentWindow, SW_SHOW);

                        parentWindow = GetParent(parentWindow);
                    }
                }
            }
        }
        break;
    case ID_WINDOW_ENABLED:
        EnableWindow(WindowHandle, !IsWindowEnabled(WindowHandle));
        break;
    case ID_WINDOW_ALWAYSONTOP:
        SetWindowPos(
            WindowHandle,
            (GetWindowLong(WindowHandle, GWL_EXSTYLE) & WS_EX_TOPMOST) ? HWND_NOTOPMOST : HWND_TOPMOST,
            0,
            0,
            0,
            0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE
            );
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
            ULONG opacity = CommandId - ID_OPACITY_10 + 1;

            if (opacity == 10)
            {
                PhSetWindowExStyle(WindowHandle, WS_EX_LAYERED, 0);
                RedrawWindow(WindowHandle, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
            }
            else
            {
                PhSetWindowExStyle(WindowHandle, WS_EX_LAYERED, WS_EX_LAYERED);
                SetLayeredWindowAttributes(WindowHandle, 0, (BYTE)(255 * opacity / 10), LWA_ALPHA);
            }
        }
        break;
    case ID_WINDOW_GOTOTHREAD:
        {
            PPH_PROCESS_ITEM processItem;
            PPH_PROCESS_PROPCONTEXT propContext;

            if (!NT_SUCCESS(PhGetWindowClientId(WindowHandle, &clientId)))
                return FALSE;

            if (processItem = PhReferenceProcessItem(clientId.UniqueProcess))
            {
                if (propContext = PhCreateProcessPropContext(NULL, processItem))
                {
                    PhSetSelectThreadIdProcessPropContext(propContext, clientId.UniqueThread);
                    PhShowProcessProperties(propContext);
                    PhDereferenceObject(propContext);
                }

                PhDereferenceObject(processItem);
            }
            else
            {
                PhShowError2(ParentWindowHandle, L"The process does not exist.", L"%s", L"");
            }
        }
        break;
    case ID_WINDOW_GOTOPROCESS:
        {
            PPH_PROCESS_ITEM processItem;
            PPH_PROCESS_NODE processNode;

            if (!NT_SUCCESS(PhGetWindowClientId(WindowHandle, &clientId)))
                return FALSE;

            if (processItem = PhReferenceProcessItem(clientId.UniqueProcess))
            {
                if (processNode = PhFindProcessNode(processItem->ProcessId))
                {
                    SystemInformer_SelectTabPage(0);
                    SystemInformer_SelectProcessNode(processNode);
                    SystemInformer_ToggleVisible(FALSE);
                }

                PhDereferenceObject(processItem);
            }
            else
            {
                PhShowError2(ParentWindowHandle, L"The process does not exist.", L"%s", L"");
            }
        }
        break;
    case ID_WINDOW_INSPECT:
    case ID_WINDOW_OPENFILELOCATION:
        {
            PPH_PROCESS_ITEM processItem;
            PPH_STRING fileName = NULL;

            if (!NT_SUCCESS(PhGetWindowClientId(WindowHandle, &clientId)))
                return FALSE;

            if (processItem = PhReferenceProcessItem(clientId.UniqueProcess))
            {
                if (processItem->QueryHandle)
                    PhGetProcessImageFileNameWin32(processItem->QueryHandle, &fileName);

                PhDereferenceObject(processItem);
            }

            if (!PhIsNullOrEmptyString(fileName) && PhDoesFileExistWin32(fileName->Buffer))
            {
                PhShellExecuteUserString(
                    ParentWindowHandle,
                    CommandId == ID_WINDOW_INSPECT ? SETTING_PROGRAM_INSPECT_EXECUTABLES : SETTING_FILE_BROWSE_EXECUTABLE,
                    fileName->Buffer,
                    FALSE,
                    CommandId == ID_WINDOW_INSPECT ?
                        L"Make sure the PE Viewer executable file is present." :
                        L"Make sure the Explorer executable file is present."
                    );
            }

            PhClearReference(&fileName);
        }
        break;
    case ID_WINDOW_SETDPI:
        {
            PPH_STRING selectedChoice = NULL;

            while (PhaChoiceDialog(
                ParentWindowHandle,
                L"Enter new Window DPI:",
                L"Enter new Window DPI:",
                NULL,
                0,
                NULL,
                PH_CHOICE_DIALOG_USER_CHOICE,
                &selectedChoice,
                NULL,
                NULL
                ))
            {
                LONG64 value;

                if (selectedChoice->Length == 0)
                    continue;

                if (PhStringToInteger64(&selectedChoice->sr, 0, &value))
                {
                    WeSetWindowToDpiForTesting(WindowHandle, (LONG)value);
                    break;
                }
            }
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

LRESULT CALLBACK WepFindWindowButtonSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    WNDPROC oldWndProc;

    if (!(oldWndProc = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT)))
        return 0;

    switch (WindowMessage)
    {
    case WM_NCDESTROY:
        PhSetWindowProcedure(WindowHandle, oldWndProc);
        PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
        break;
    case WM_LBUTTONDOWN:
        PostMessage(GetParent(WindowHandle), WE_WM_FINDWINDOW, 0, 0);
        break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
}

VOID WeSnapshotModeStart(
    _In_ HWND DialogHandle,
    _In_ PWINDOWS_CONTEXT Context
    )
{
    CLIENT_ID clientId;
    PWE_WINDOW_NODE windowNode;
    HWND selectedWindow;

    if (PhGetIntegerSetting(SETTING_NAME_WINDOW_FIND_SNAPSHOT))
        selectedWindow = PhSelectWindowFromScreenSnapshot();
    else
        selectedWindow = PhSelectWindowFromScreenTargeting(DialogHandle, FALSE);

    // Process the selected window
    if (selectedWindow && IsWindow(selectedWindow))
    {
        // Resolve ghost windows
        HWND hungWindow = PhHungWindowFromGhostWindow(selectedWindow);
        if (hungWindow)
            selectedWindow = hungWindow;

        // Check if it's not our own process
        if (NT_SUCCESS(PhGetWindowClientId(selectedWindow, &clientId)) &&
            clientId.UniqueProcess != NtCurrentProcessId())
        {
            // Find and select the window node in the tree
            if (windowNode = WeFindWindowNode(&Context->TreeContext, selectedWindow))
            {
                WeSelectAndEnsureVisibleWindowNodes(&Context->TreeContext, &windowNode, 1);
                SetFocus(Context->TreeNewHandle);
            }
            else
            {
                WeDeselectAllWindowNodes(&Context->TreeContext);
            }
        }
    }
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI WepWindowNotifyEventChangeCallback(
    _In_ PVOID Parameter,
    _In_ PWINDOWS_CONTEXT Context
    )
{
    PMSG message = Parameter;

    switch (message->wParam)
    {
    case WM_DPICHANGED:
        {
            HFONT treeWindowFont;

            if (treeWindowFont = PhCreateTreeWindowFont(PhGetWindowDpi(Context->WindowHandle)))
                PhSwapReferenceFont(&Context->TreeWindowFont, Context->TreeNewHandle, treeWindowFont, TRUE);
        }
        break;
    }
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI WepWindowProviderAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PWINDOWS_CONTEXT context = Context;
    PWE_WINDOW_ITEM windowItem = Parameter;

    if (!(context && windowItem))
        return;

    PhReferenceObject(windowItem);
    PhPushProviderEventQueue(&context->EventQueue, ProviderAddedEvent, windowItem, context->WindowProviderRunCount);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI WepWindowProviderModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PWINDOWS_CONTEXT context = Context;
    PWE_WINDOW_ITEM windowItem = Parameter;

    if (!(context && windowItem))
        return;

    PhReferenceObject(windowItem);
    PhPushProviderEventQueue(&context->EventQueue, ProviderModifiedEvent, windowItem, context->WindowProviderRunCount);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI WepWindowProviderRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PWINDOWS_CONTEXT context = Context;
    PWE_WINDOW_ITEM windowItem = Parameter;

    if (!(context && windowItem))
        return;

    PhReferenceObject(windowItem);
    PhPushProviderEventQueue(&context->EventQueue, ProviderRemovedEvent, windowItem, context->WindowProviderRunCount);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI WepWindowProviderUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ULONG runCount = PtrToUlong(Parameter);
    PWINDOWS_CONTEXT context = Context;

    if (!context)
        return;

    PostMessage(context->WindowHandle, WE_WM_WINDOWS_UPDATED, (WPARAM)runCount, 0);
}

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI WepWindowsSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PWE_WINDOW_NODE lastSelectedNode;
    PWINDOWS_CONTEXT context = Context;

    assert(context);

    context->TreeContext.SearchMatchHandle = MatchHandle;

    if (!context->TreeContext.SearchMatchHandle)
    {
        lastSelectedNode = WeGetSelectedWindowNode(&context->TreeContext);
    }
    else
    {
        WeExpandAllWindowNodes(&context->TreeContext, TRUE);
        lastSelectedNode = NULL;
    }

    PhApplyTreeNewFilters(&context->TreeContext.FilterSupport);

    if (lastSelectedNode)
    {
        TreeNew_EnsureVisible(context->TreeNewHandle, &lastSelectedNode->Node);
    }
}

INT_PTR CALLBACK WepWindowsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWINDOWS_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = (PWINDOWS_CONTEXT)lParam;
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
            context->WindowHandle = WindowHandle;
            context->TreeNewHandle = GetDlgItem(WindowHandle, IDC_LIST);
            context->SearchBoxHandle = GetDlgItem(WindowHandle, IDC_SEARCHEDIT);
            context->FindWindowButtonHandle = GetDlgItem(WindowHandle, IDC_FINDWINDOW);
            context->PauseResumeButtonHandle = GetDlgItem(WindowHandle, IDC_PAUSERESUME);
            context->TreeWindowFont = PhCreateTreeWindowFont(PhGetWindowDpi(WindowHandle));

            WeInitializeWindowTree(WindowHandle, context->TreeNewHandle, &context->TreeContext);
            context->TreeContext.ColorNew = context->ColorNew;
            context->TreeContext.ColorRemoved = context->ColorRemoved;
            TreeNew_SetEmptyText(context->TreeNewHandle, &WepEmptyWindowsText, 0);
            SetWindowFont(context->TreeNewHandle, context->TreeWindowFont, TRUE);
            WeInitializeWindowTreeImageList(&context->TreeContext, &context->Selector);

            PhSetApplicationWindowIcon(WindowHandle);
            //PhSetWindowText(WindowHandle, PH_AUTO_T(PH_STRING, WepGetWindowTitleForSelector(&context->Selector))->Buffer);

            PhCreateSearchControl(
                WindowHandle,
                context->SearchBoxHandle,
                L"Search Windows (Ctrl+K)",
                WepWindowsSearchControlCallback,
                context
                );

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_SEARCHEDIT), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_LIST), NULL, PH_ANCHOR_ALL);

            if (PhValidWindowPlacementFromSetting(SETTING_NAME_WINDOWS_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_WINDOWS_WINDOW_POSITION, SETTING_NAME_WINDOWS_WINDOW_SIZE, WindowHandle);
            else
                PhCenterWindow(WindowHandle, NULL);

            // Subclass the button.
            context->FindWindowButtonWindowProc = PhGetWindowProcedure(context->FindWindowButtonHandle);
            PhSetWindowContext(context->FindWindowButtonHandle, PH_WINDOW_CONTEXT_DEFAULT, context->FindWindowButtonWindowProc);
            PhSetWindowProcedure(context->FindWindowButtonHandle, WepFindWindowButtonSubclassProc);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackWindowNotifyEvent),
                WepWindowNotifyEventChangeCallback,
                context,
                &context->WindowNotifyCallbackRegistration
                );

            {
                ULONG updateInterval = PhGetIntegerSetting(L"UpdateInterval");

                if (updateInterval == 0)
                    updateInterval = 1000;

                PhInitializeProviderEventQueue(&context->EventQueue, 100);

                context->WindowProvider = WeCreateWindowProvider();
                PhRegisterCallback(
                    &context->WindowProvider->WindowAddedEvent,
                    WepWindowProviderAddedHandler,
                    context,
                    &context->WindowProviderAddedRegistration
                    );
                PhRegisterCallback(
                    &context->WindowProvider->WindowModifiedEvent,
                    WepWindowProviderModifiedHandler,
                    context,
                    &context->WindowProviderModifiedRegistration
                    );
                PhRegisterCallback(
                    &context->WindowProvider->WindowRemovedEvent,
                    WepWindowProviderRemovedHandler,
                    context,
                    &context->WindowProviderRemovedRegistration
                    );
                PhRegisterCallback(
                    &context->WindowProvider->WindowUpdatedEvent,
                    WepWindowProviderUpdatedHandler,
                    context,
                    &context->WindowProviderUpdatedRegistration
                    );

                PhInitializeProviderThread(&context->ProviderThread, updateInterval);
                PhRegisterProvider(&context->ProviderThread, WeWindowProviderUpdate, context, &context->ProviderRegistration);
                PhSetEnabledProvider(&context->ProviderRegistration, TRUE);
                PhStartProviderThread(&context->ProviderThread);
            }

            WepRefreshWindows(context); // Initial refresh or provider boost

            //if (context->TreeContext.EnableStateHighlighting)
            //    PhSetTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT, 100, NULL);

            PhSetDialogFocus(WindowHandle, context->TreeNewHandle);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);

            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackWindowNotifyEvent), &context->WindowNotifyCallbackRegistration);

            PhSetEnabledProvider(&context->ProviderRegistration, FALSE);
            PhUnregisterProvider(&context->ProviderRegistration);
            PhStopProviderThread(&context->ProviderThread);
            PhDeleteProviderThread(&context->ProviderThread);

            PhUnregisterCallback(&context->WindowProvider->WindowAddedEvent, &context->WindowProviderAddedRegistration);
            PhUnregisterCallback(&context->WindowProvider->WindowModifiedEvent, &context->WindowProviderModifiedRegistration);
            PhUnregisterCallback(&context->WindowProvider->WindowRemovedEvent, &context->WindowProviderRemovedRegistration);
            PhUnregisterCallback(&context->WindowProvider->WindowUpdatedEvent, &context->WindowProviderUpdatedRegistration);

            PhDeleteProviderEventQueue(&context->EventQueue);
            WeDeleteWindowProvider(context->WindowProvider);

            if (context->TreeWindowFont)
            {
                DeleteFont(context->TreeWindowFont);
            }

            PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);

            PhSaveWindowPlacementToSetting(SETTING_NAME_WINDOWS_WINDOW_POSITION, SETTING_NAME_WINDOWS_WINDOW_SIZE, WindowHandle);

            PhDeleteLayoutManager(&context->LayoutManager);

            WeDeleteWindowTree(&context->TreeContext);
            WepDeleteWindowSelector(&context->Selector);

            PhDereferenceObject(context);

            PostQuitMessage(0);
        }
        break;
    case WM_PH_SHOW_DIALOG:
        {
            if (IsMinimized(WindowHandle))
                ShowWindow(WindowHandle, SW_RESTORE);
            else
                ShowWindow(WindowHandle, SW_SHOW);

            SetForegroundWindow(WindowHandle);

            PhBoostProvider(&context->ProviderRegistration, NULL);
        }
        break;
    case WE_WM_WINDOWS_UPDATED:
        {
            ULONG upToRunId = (ULONG)wParam;
            PPH_PROVIDER_EVENT events;
            ULONG count = 0;
            ULONG i;

            events = PhFlushProviderEventQueue(&context->EventQueue, upToRunId, &count);

            WeTickWindowNodes(context, &context->TreeContext);

            if (events)
            {
                TreeNew_SetRedraw(context->TreeNewHandle, FALSE);

                for (i = 0; i < count; i++)
                {
                    PH_PROVIDER_EVENT_TYPE type = PH_PROVIDER_EVENT_TYPE(events[i]);
                    PWE_WINDOW_ITEM windowItem = PH_PROVIDER_EVENT_OBJECT(events[i]);

                    switch (type)
                    {
                    case ProviderAddedEvent:
                        {
                            if (!WeFindWindowNode(&context->TreeContext, windowItem->WindowHandle))
                                WeAddWindowNode(&context->TreeContext, windowItem, events[i].RunId);

                            PhDereferenceObject(windowItem);
                        }
                        break;
                    case ProviderModifiedEvent:
                        {
                            PWE_WINDOW_NODE windowNode = WeFindWindowNode(&context->TreeContext, windowItem->WindowHandle);

                            if (windowNode)
                                WeUpdateWindowNode(windowNode, &context->TreeContext);

                            PhDereferenceObject(windowItem);
                        }
                        break;
                    case ProviderRemovedEvent:
                        {
                            PWE_WINDOW_NODE windowNode = WeFindWindowNode(&context->TreeContext, windowItem->WindowHandle);

                            if (windowNode)
                                WeRemoveWindowNode(windowNode, &context->TreeContext);

                            PhDereferenceObject(windowItem);
                        }
                        break;
                    }
                }

                PhFree(events);
            }

            if (count != 0)
            {
                // Structure newly added nodes in the same cycle they were added. WeTickWindowNodes
                // (called above, before these events) is what normally honors NeedsNodesStructured,
                // so without this the new nodes wouldn't appear until the next provider tick - by
                // which point PH_TICK_SH_STATE_TN has already aged out their "new item" highlight,
                // making them appear instantly with no green.
                if (NeedsNodesStructured)
                {
                    NeedsNodesStructured = FALSE;
                    TreeNew_NodesStructured(context->TreeNewHandle);
                }

                TreeNew_SetRedraw(context->TreeNewHandle, TRUE);
                InvalidateRect(context->TreeNewHandle, NULL, FALSE);
            }

        }
        break;
    case WM_COMMAND:
        {
            ULONG commandId = GET_WM_COMMAND_ID(wParam, lParam);
            PWE_WINDOW_NODE selectedNode = WeGetSelectedWindowNode(&context->TreeContext);

            if (selectedNode && WepExecuteWindowCommand(WindowHandle, selectedNode->WindowHandle, commandId))
            {
                PhBoostProvider(&context->ProviderRegistration, NULL);
                return TRUE;
            }

            switch (commandId)
            {
            case IDCANCEL:
                DestroyWindow(WindowHandle);
                break;
            case IDC_REFRESH:
                {
                    WepRefreshWindows(context);

                    PhApplyTreeNewFilters(&context->TreeContext.FilterSupport);
                }
                break;
            case IDC_OPTIONS:
                {
                    RECT rect;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM enumMessageOnlyItem;
                    PPH_EMENU_ITEM enumNonVisibleItem;
                    PPH_EMENU_ITEM highlightMessageOnlyItem;
                    PPH_EMENU_ITEM iconsItem;
                    PPH_EMENU_ITEM desktopItem;
                    PPH_EMENU_ITEM parentChildItem;
                    PPH_EMENU_ITEM zOrderItem;
                    PPH_EMENU_ITEM ownerItem;
                    PPH_EMENU_ITEM findSnapshotItem;
                    PPH_EMENU_ITEM selectedItem;

                    GetWindowRect(GetDlgItem(WindowHandle, IDC_OPTIONS), &rect);

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, enumMessageOnlyItem = PhCreateEMenuItem(0, ID_WINDOW_OPTIONS_ENUM_MESSAGEONLY, L"Enumerate message-only windows", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, enumNonVisibleItem = PhCreateEMenuItem(0, ID_WINDOW_OPTIONS_ENUM_NONVISIBLE, L"Enumerate non-visible windows", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightMessageOnlyItem = PhCreateEMenuItem(0, ID_WINDOW_OPTIONS_HIGHLIGHT_MESSAGEONLY, L"Highlight message-only windows", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, iconsItem = PhCreateEMenuItem(0, 1, L"Enable icons", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, desktopItem = PhCreateEMenuItem(0, 3, L"Show desktop windows", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, findSnapshotItem = PhCreateEMenuItem(0, ID_WINDOW_OPTIONS_FIND_SNAPSHOT, L"Use snapshot window finder", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, parentChildItem = PhCreateEMenuItem(0, ID_VIEW_MODE_PARENTCHILD, L"Enumerate windows", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, zOrderItem = PhCreateEMenuItem(0, ID_VIEW_MODE_ZORDER, L"Enumerate windows by z-order", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, ownerItem = PhCreateEMenuItem(0, ID_VIEW_MODE_OWNER, L"Enumerate windows by owner", NULL, NULL), ULONG_MAX);

                    if (PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_MESSAGEONLY))
                        enumMessageOnlyItem->Flags |= PH_EMENU_CHECKED;
                    if (PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_NONVISIBLE))
                        enumNonVisibleItem->Flags |= PH_EMENU_CHECKED;
                    if (PhGetIntegerSetting(SETTING_NAME_WINDOW_HIGHLIGHT_MESSAGEONLY))
                        highlightMessageOnlyItem->Flags |= PH_EMENU_CHECKED;
                    if (PhGetIntegerSetting(SETTING_NAME_WINDOW_ENABLE_ICONS))
                        iconsItem->Flags |= PH_EMENU_CHECKED;
                    if (PhGetIntegerSetting(SETTING_NAME_SHOW_DESKTOP_WINDOWS))
                        desktopItem->Flags |= PH_EMENU_CHECKED;
                    if (PhGetIntegerSetting(SETTING_NAME_WINDOW_FIND_SNAPSHOT))
                        findSnapshotItem->Flags |= PH_EMENU_CHECKED;

                    PhSetFlagsEMenuItem(menu, ID_VIEW_MODE_PARENTCHILD, PH_EMENU_RADIOCHECK | PH_EMENU_CHECKED,
                        (context->TreeContext.ViewMode == WeWindowViewModeParentChild) ? (PH_EMENU_RADIOCHECK | PH_EMENU_CHECKED) : 0);
                    PhSetFlagsEMenuItem(menu, ID_VIEW_MODE_ZORDER, PH_EMENU_RADIOCHECK | PH_EMENU_CHECKED,
                        (context->TreeContext.ViewMode == WeWindowViewModeZOrder) ? (PH_EMENU_RADIOCHECK | PH_EMENU_CHECKED) : 0);
                    PhSetFlagsEMenuItem(menu, ID_VIEW_MODE_OWNER, PH_EMENU_RADIOCHECK | PH_EMENU_CHECKED,
                        (context->TreeContext.ViewMode == WeWindowViewModeOwner) ? (PH_EMENU_RADIOCHECK | PH_EMENU_CHECKED) : 0);

                    selectedItem = PhShowEMenu(
                        menu,
                        WindowHandle,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        rect.left,
                        rect.bottom
                        );

                    if (selectedItem && selectedItem->Id)
                    {
                        switch (selectedItem->Id)
                        {
                        case ID_WINDOW_OPTIONS_ENUM_MESSAGEONLY:
                            {
                                BOOLEAN value = !PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_MESSAGEONLY);
                                PhSetIntegerSetting(SETTING_NAME_WINDOW_ENUM_MESSAGEONLY, value);
                                context->TreeContext.EnableMessageOnly = value;
                                PhBoostProvider(&context->ProviderRegistration, NULL);
                            }
                            break;
                        case ID_WINDOW_OPTIONS_ENUM_NONVISIBLE:
                            {
                                BOOLEAN value = !PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_NONVISIBLE);
                                PhSetIntegerSetting(SETTING_NAME_WINDOW_ENUM_NONVISIBLE, value);
                                context->TreeContext.EnableWindowVisible = value;
                                // Let the background provider reconcile the tree so newly-included windows flash the
                                // "new item" highlight (and now-excluded windows are removed) instead of an instant
                                // clear + repopulate via WepRefreshWindows.
                                PhBoostProvider(&context->ProviderRegistration, NULL);
                            }
                            break;
                        case ID_WINDOW_OPTIONS_HIGHLIGHT_MESSAGEONLY:
                            {
                                BOOLEAN value = !PhGetIntegerSetting(SETTING_NAME_WINDOW_HIGHLIGHT_MESSAGEONLY);
                                PhSetIntegerSetting(SETTING_NAME_WINDOW_HIGHLIGHT_MESSAGEONLY, value);
                                context->TreeContext.HighlightMessageOnly = value;
                                WeInvalidateWindowTreeColors(&context->TreeContext);
                            }
                            break;
                        case ID_WINDOW_OPTIONS_FIND_SNAPSHOT:
                            {
                                BOOLEAN value = !PhGetIntegerSetting(SETTING_NAME_WINDOW_FIND_SNAPSHOT);
                                PhSetIntegerSetting(SETTING_NAME_WINDOW_FIND_SNAPSHOT, value);
                            }
                            break;
                        case 1: // Enable icons
                            {
                                BOOLEAN value = !PhGetIntegerSetting(SETTING_NAME_WINDOW_ENABLE_ICONS);
                                PhSetIntegerSetting(SETTING_NAME_WINDOW_ENABLE_ICONS, value);
                                WeSetWindowTreeIconsEnabled(&context->TreeContext, value);
                            }
                            break;
                        case 3: // Show desktop windows
                            {
                                BOOLEAN value = !PhGetIntegerSetting(SETTING_NAME_SHOW_DESKTOP_WINDOWS);
                                PhSetIntegerSetting(SETTING_NAME_SHOW_DESKTOP_WINDOWS, value);
                               // WepRefreshWindows(context);
                                PhApplyTreeNewFilters(&context->TreeContext.FilterSupport);
                            }
                            break;
                        case ID_VIEW_MODE_PARENTCHILD:
                        case ID_VIEW_MODE_ZORDER:
                        case ID_VIEW_MODE_OWNER:
                            {
                                WE_WINDOW_VIEW_MODE newViewMode;
                                if (selectedItem->Id == ID_VIEW_MODE_PARENTCHILD) newViewMode = WeWindowViewModeParentChild;
                                else if (selectedItem->Id == ID_VIEW_MODE_ZORDER) newViewMode = WeWindowViewModeZOrder;
                                else newViewMode = WeWindowViewModeOwner;

                                if (context->TreeContext.ViewMode != newViewMode)
                                {
                                    context->TreeContext.ViewMode = newViewMode;
                                    WepRefreshWindows(context);
                                    PhApplyTreeNewFilters(&context->TreeContext.FilterSupport);
                                }
                            }
                            break;
                        }
                    }

                    PhDestroyEMenu(menu);
                }
                break;
            case ID_SHOWCONTEXTMENU:
                {
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
                    PWE_WINDOW_NODE *windows;
                    ULONG numberOfWindows;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM selectedItem;

                    if (!WeGetSelectedWindowNodes(&context->TreeContext, &windows, &numberOfWindows))
                        break;

                    if (numberOfWindows != 0)
                    {
                        menu = PhCreateEMenu();
                        WepCreateWindowMenu(menu, TRUE, TRUE);
                        PhInsertCopyCellEMenuItem(menu, ID_WINDOW_COPY, context->TreeNewHandle, contextMenuEvent->Column);
                        PhSetFlagsEMenuItem(menu, ID_WINDOW_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

                        if (numberOfWindows == 1)
                        {
                            WepSetWindowMenuState(menu, windows[0]->WindowHandle);
                        }
                        else
                        {
                            PhSetFlagsAllEMenuItems(menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                            PhSetFlagsEMenuItem(menu, ID_WINDOW_COPY, PH_EMENU_DISABLED, 0);
                        }

                        selectedItem = PhShowEMenu(
                            menu,
                            WindowHandle,
                            PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            contextMenuEvent->Location.x,
                            contextMenuEvent->Location.y
                            );

                        if (selectedItem && selectedItem->Id != ULONG_MAX)
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
                        WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };

                        if (!GetWindowPlacement(selectedNode->WindowHandle, &placement))
                            break;

                        if (placement.showCmd == SW_SHOWMINIMIZED || placement.showCmd == SW_MINIMIZE)
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
                        //WeWindowEndTask(selectedNode->WindowHandle, TRUE);
                        PostMessage(selectedNode->WindowHandle, WM_CLOSE, 0, 0);

                    }
                }
                break;
                case ID_WINDOW_VISIBLE:
                    {
                        PWE_WINDOW_NODE selectedNode;
                        HWND currentWindow;

                        if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                        {
                            currentWindow = selectedNode->WindowHandle;

                            if (WeIsWindowEffectivelyVisible(currentWindow)) // If it's effectively visible, hide it.
                            {
                                selectedNode->WindowItem->WindowVisible = FALSE;
                                ShowWindowAsync(currentWindow, SW_HIDE);
                            }
                            else // If not effectively visible, make it and its parents visible.
                            {
                                BOOL invisibleParentFound = FALSE;
                                HWND tempParent = GetParent(currentWindow);

                                while (tempParent && tempParent != GetDesktopWindow())
                                {
                                    if (!IsWindowVisible(tempParent))
                                    {
                                        invisibleParentFound = TRUE;
                                        break;
                                    }
                                    tempParent = GetParent(tempParent);
                                }

                                if (invisibleParentFound)
                                {
                                    if (MessageBoxW(
                                        context->WindowHandle, // Parent window for the message box
                                        L"The parent window(s) of this window are currently invisible.\n\n"
                                        L"Do you want to make the parent window(s) visible along with this window?",
                                        L"Confirm Parent Visibility Change",
                                        MB_YESNO | MB_ICONQUESTION
                                    ) == IDNO)
                                    {
                                        // User chose not to make parents visible, so just make the current window visible (current behavior)
                                        selectedNode->WindowItem->WindowVisible = TRUE;
                                        ShowWindowAsync(currentWindow, SW_SHOW);
                                    }
                                    else
                                    {
                                        // User confirmed, make current window and its parents visible
                                        selectedNode->WindowItem->WindowVisible = TRUE;
                                        ShowWindowAsync(currentWindow, SW_SHOW);

                                        HWND parent = GetParent(currentWindow);
                                        while (parent && parent != GetDesktopWindow())
                                        {
                                            if (!IsWindowVisible(parent))
                                            {
                                                ShowWindowAsync(parent, SW_SHOW);
                                            }
                                            parent = GetParent(parent);
                                        }
                                    }
                                }
                                else // No invisible parents found, just make current window visible
                                {
                                    selectedNode->WindowItem->WindowVisible = TRUE;
                                    ShowWindowAsync(currentWindow, SW_SHOW);
                                }
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
                        PhSetTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT, 100, NULL);
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
                        if (processItem = PhReferenceProcessItem(selectedNode->WindowItem->ClientId.UniqueProcess))
                        {
                            if (propContext = PhCreateProcessPropContext(NULL, processItem))
                            {
                                PhSetSelectThreadIdProcessPropContext(propContext, selectedNode->WindowItem->ClientId.UniqueThread);
                                PhShowProcessProperties(propContext);
                                PhDereferenceObject(propContext);
                            }

                            PhDereferenceObject(processItem);
                        }
                        else
                        {
                            PhShowError2(WindowHandle, L"The process does not exist.", L"%s", L"");
                        }
                    }
                }
                break;
            case ID_WINDOW_GOTOPROCESS:
                {
                    PWE_WINDOW_NODE selectedNode;
                    PPH_PROCESS_ITEM processItem;
                    PPH_PROCESS_NODE processNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (processItem = PhReferenceProcessItem(selectedNode->WindowItem->ClientId.UniqueProcess))
                        {
                            if (processNode = PhFindProcessNode(processItem->ProcessId))
                            {
                                SystemInformer_SelectTabPage(0);
                                SystemInformer_SelectProcessNode(processNode);
                                SystemInformer_ToggleVisible(FALSE);
                            }

                            PhDereferenceObject(processItem);
                        }
                        else
                        {
                            PhShowError2(WindowHandle, L"The process does not exist.", L"%s", L"");
                        }
                    }
                }
                break;
            case ID_WINDOW_INSPECT:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (
                            !PhIsNullOrEmptyString(selectedNode->FileNameWin32) &&
                            PhDoesFileExistWin32(PhGetString(selectedNode->FileNameWin32))
                            )
                        {
                            PhShellExecuteUserString(
                                WindowHandle,
                                SETTING_PROGRAM_INSPECT_EXECUTABLES,
                                PhGetString(selectedNode->FileNameWin32),
                                FALSE,
                                L"Make sure the PE Viewer executable file is present."
                                );
                        }
                    }
                }
                break;
            case ID_WINDOW_OPENFILELOCATION:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (
                            !PhIsNullOrEmptyString(selectedNode->FileNameWin32) &&
                            PhDoesFileExistWin32(PhGetString(selectedNode->FileNameWin32))
                            )
                        {
                            PhShellExecuteUserString(
                                WindowHandle,
                                SETTING_FILE_BROWSE_EXECUTABLE,
                                PhGetString(selectedNode->FileNameWin32),
                                FALSE,
                                L"Make sure the Explorer executable file is present."
                                );
                        }
                    }
                }
                break;
            case ID_WINDOW_PROPERTIES:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        WeShowWindowProperties(
                            WindowHandle,
                            selectedNode->WindowHandle
                            );
                    }
                }
                break;
            case ID_WINDOW_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                    PhSetClipboardString(WindowHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            case ID_WINDOW_SETDPI:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        PPH_STRING selectedChoice = NULL;

                        while (PhaChoiceDialog(
                            WindowHandle,
                            L"Enter new Window DPI:",
                            L"Enter new Window DPI:",
                            NULL,
                            0,
                            NULL,
                            PH_CHOICE_DIALOG_USER_CHOICE,
                            &selectedChoice,
                            NULL,
                            NULL
                            ))
                        {
                            LONG64 value;

                            if (selectedChoice->Length == 0)
                                continue;

                            if (PhStringToInteger64(&selectedChoice->sr, 0, &value))
                            {
                                WeSetWindowToDpiForTesting(selectedNode->WindowHandle, (LONG)value);
                                break;
                            }
                        }
                    }
                }
                break;
            case IDC_PAUSERESUME:
                {
                    context->ProviderPaused = !context->ProviderPaused;

                    PhSetEnabledProvider(&context->ProviderRegistration, !context->ProviderPaused);

                    if (context->ProviderPaused)
                    {
                        SetWindowText(context->PauseResumeButtonHandle, L"Resume");
                    }
                    else
                    {
                        SetWindowText(context->PauseResumeButtonHandle, L"Pause");
                    }
                }
                break;
            }
        }
        break;
    case WM_KEYDOWN:
        {
            if (LOWORD(wParam) == 'K')
            {
                if (GetKeyState(VK_CONTROL) < 0)
                {
                    SetFocus(context->SearchBoxHandle);
                    return TRUE;
                }
            }
        }
        break;
    case WM_TIMER:
        {
            switch (wParam)
            {
            case PH_WINDOW_TIMER_DEFAULT:
                {
                    //if (context->TreeContext.EnableStateHighlighting)
                    //{
                    //    PH_TICK_SH_STATE_TN(
                    //        WE_WINDOW_NODE,
                    //        ShState,
                    //        context->TreeContext.NodeStateList,
                    //        WepRemoveWindowNode,
                    //        context->HighlightingDuration,
                    //        context->TreeNewHandle,
                    //        TRUE,
                    //        NULL,
                    //        &context->TreeContext
                    //        );
                    //}

                    //if (context->TreeContext.NodeRestructure)
                    //{
                    //    TreeNew_NodesStructured(context->TreeNewHandle);
                    //    context->TreeContext.NodeRestructure = FALSE;
                    //}

                    //if (context->HighlightingWindowCount > 0)
                    //{
                    //    WeInvertWindowBorder(context->HighlightingWindow);

                    //    if (--context->HighlightingWindowCount == 0 && !context->TreeContext.EnableStateHighlighting)
                    //    {
                    //        PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);
                    //    }
                    //}
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
    //case WM_PARENTNOTIFY:
    //    {
    //        PhSetWindowExStyle(context->FindWindowButtonHandle, WS_EX_NOPARENTNOTIFY, 0);
    //
    //        if (LOWORD(wParam) == WM_LBUTTONDOWN)
    //        {
    //            POINT point;
    //
    //            point.x = GET_X_LPARAM(lParam);
    //            point.y = GET_Y_LPARAM(lParam);
    //
    //            if (ChildWindowFromPoint(WindowHandle, point) == context->FindWindowButtonHandle)
    //            {
    //                PostMessage(WindowHandle, WE_WM_FINDWINDOW, 0, 0);
    //            }
    //        }
    //    }
    //    break;
    case WM_DPICHANGED:
        {
            HFONT treeWindowFont;

            if (treeWindowFont = PhCreateTreeWindowFont(PhGetWindowDpi(WindowHandle)))
                PhSwapReferenceFont(&context->TreeWindowFont, context->TreeNewHandle, treeWindowFont, TRUE);

            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WE_WM_FINDWINDOW:
        {
            WeSnapshotModeStart(WindowHandle, context);
        }
        break;
    case WM_MOUSEMOVE:
        {
            if (context->TargetingWindow)
            {
                POINT cursorPos;
                HWND windowOverMouse;

                if (!PhGetMessagePos(&cursorPos))
                    break;

                windowOverMouse = WindowFromPoint(cursorPos);

                if (context->TargetingCurrentWindow != windowOverMouse)
                {
                    if (context->TargetingCurrentWindow && context->TargetingCurrentWindowDraw)
                    {
                        // Invert the old border (to remove it).
                        WeDrawWindowBorderForTargeting(context->TargetingCurrentWindow);
                    }

                    if (windowOverMouse)
                    {
                        CLIENT_ID clientId;

                        // Draw a rectangle over the current window (but not if it's one of our own).
                        if (
                            NT_SUCCESS(PhGetWindowClientId(windowOverMouse, &clientId)) &&
                            clientId.UniqueProcess != NtCurrentProcessId()
                            )
                        {
                            WeDrawWindowBorderForTargeting(windowOverMouse);
                            context->TargetingCurrentWindowDraw = TRUE;
                        }
                        else
                        {
                            context->TargetingCurrentWindowDraw = FALSE;
                        }
                    }

                    context->TargetingCurrentWindow = windowOverMouse;
                }
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            if (context->TargetingWindow)
            {
                CLIENT_ID clientId;

                context->TargetingCompleted = TRUE;

                // Reset the original cursor.
                PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

                // Bring the window back to the top, and preserve the Always on Top setting.
                SetWindowPos(WindowHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                context->TargetingWindow = FALSE;
                ReleaseCapture();

                if (context->TargetingCurrentWindow && IsWindow(context->TargetingCurrentWindow))
                {
                    if (context->TargetingCurrentWindowDraw)
                    {
                        // Remove the border on the window we found.
                        WeDrawWindowBorderForTargeting(context->TargetingCurrentWindow);
                        context->TargetingCurrentWindowDraw = FALSE;
                    }

                    //if (ResolveGhostWindows)
                    {
                        HWND hungWindow = PhHungWindowFromGhostWindow(context->TargetingCurrentWindow);

                        if (hungWindow)
                            context->TargetingCurrentWindow = hungWindow;
                    }

                    if (
                        NT_SUCCESS(PhGetWindowClientId(context->TargetingCurrentWindow, &clientId)) &&
                        clientId.UniqueProcess != NtCurrentProcessId()
                        )
                    {
                        PWE_WINDOW_NODE windowNode;

                        if (windowNode = WeFindWindowNode(&context->TreeContext, context->TargetingCurrentWindow))
                        {
                            WeSelectAndEnsureVisibleWindowNodes(&context->TreeContext, &windowNode, 1);

                            SetFocus(context->TreeNewHandle); // HACK (dmex)
                        }
                        else
                        {
                            WeDeselectAllWindowNodes(&context->TreeContext);
                        }
                    }
                }
            }
        }
        break;
    case WM_CAPTURECHANGED:
        {
            if (context->TargetingWindow && !context->TargetingCompleted)
            {
                // The user cancelled the targeting, probably by pressing the Esc key.

                // Remove the border on the currently selected window.
                if (context->TargetingCurrentWindow && IsWindow(context->TargetingCurrentWindow))
                {
                    if (context->TargetingCurrentWindowDraw)
                    {
                        // Remove the border on the window we found.
                        WeDrawWindowBorderForTargeting(context->TargetingCurrentWindow);
                        context->TargetingCurrentWindowDraw = FALSE;
                    }
                }

                // Reset cursor
                PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

                SetWindowPos(WindowHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                context->TargetingWindow = FALSE;
                context->TargetingCompleted = TRUE;
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

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI WepWindowsPageSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PWINDOWS_CONTEXT context = Context;

    assert(context);

    context->TreeContext.SearchMatchHandle = MatchHandle;

    if (!context->TreeContext.SearchMatchHandle)
        WeExpandAllWindowNodes(&context->TreeContext, TRUE);

    PhApplyTreeNewFilters(&context->TreeContext.FilterSupport);
}

INT_PTR CALLBACK WepWindowsPageProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWINDOWS_CONTEXT context;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;

    if (PhPropPageDlgProcHeader(WindowHandle, WindowMessage, lParam, NULL, &propPageContext, &processItem))
    {
        context = propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = WindowHandle;
            context->TreeNewHandle = GetDlgItem(WindowHandle, IDC_LIST);
            context->SearchBoxHandle = GetDlgItem(WindowHandle, IDC_SEARCHEDIT);
            context->FindWindowButtonHandle = GetDlgItem(WindowHandle, IDC_FINDWINDOW);
            context->TreeWindowFont = PhCreateTreeWindowFont(PhGetWindowDpi(WindowHandle));

            WeInitializeWindowTree(WindowHandle, context->TreeNewHandle, &context->TreeContext);
            context->TreeContext.ColorNew = context->ColorNew;
            context->TreeContext.ColorRemoved = context->ColorRemoved;
            TreeNew_SetEmptyText(context->TreeNewHandle, &WepEmptyWindowsText, 0);
            SetWindowFont(context->TreeNewHandle, context->TreeWindowFont, TRUE);
            WeInitializeWindowTreeImageList(&context->TreeContext, &context->Selector);

            PhSetApplicationWindowIcon(WindowHandle);
            //PhSetWindowText(WindowHandle, PH_AUTO_T(PH_STRING, WepGetWindowTitleForSelector(&context->Selector))->Buffer);

            PhCreateSearchControl(
                WindowHandle,
                context->SearchBoxHandle,
                L"Search Windows (Ctrl+K)",
                WepWindowsSearchControlCallback,
                context
                );

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_SEARCHEDIT), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_LIST), NULL, PH_ANCHOR_ALL);

            if (PhValidWindowPlacementFromSetting(SETTING_NAME_WINDOWS_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_WINDOWS_WINDOW_POSITION, SETTING_NAME_WINDOWS_WINDOW_SIZE, WindowHandle);
            else
                PhCenterWindow(WindowHandle, NULL);

            // Subclass the button.
            context->FindWindowButtonWindowProc = PhGetWindowProcedure(context->FindWindowButtonHandle);
            PhSetWindowContext(context->FindWindowButtonHandle, PH_WINDOW_CONTEXT_DEFAULT, context->FindWindowButtonWindowProc);
            PhSetWindowProcedure(context->FindWindowButtonHandle, WepFindWindowButtonSubclassProc);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackWindowNotifyEvent),
                WepWindowNotifyEventChangeCallback,
                context,
                &context->WindowNotifyCallbackRegistration
                );

            {
                ULONG updateInterval = PhGetIntegerSetting(L"UpdateInterval");

                if (updateInterval == 0)
                    updateInterval = 1000;

                PhInitializeProviderEventQueue(&context->EventQueue, 100);

                context->WindowProvider = WeCreateWindowProvider();
                PhRegisterCallback(
                    &context->WindowProvider->WindowAddedEvent,
                    WepWindowProviderAddedHandler,
                    context,
                    &context->WindowProviderAddedRegistration
                    );
                PhRegisterCallback(
                    &context->WindowProvider->WindowModifiedEvent,
                    WepWindowProviderModifiedHandler,
                    context,
                    &context->WindowProviderModifiedRegistration
                    );
                PhRegisterCallback(
                    &context->WindowProvider->WindowRemovedEvent,
                    WepWindowProviderRemovedHandler,
                    context,
                    &context->WindowProviderRemovedRegistration
                    );
                PhRegisterCallback(
                    &context->WindowProvider->WindowUpdatedEvent,
                    WepWindowProviderUpdatedHandler,
                    context,
                    &context->WindowProviderUpdatedRegistration
                    );

                PhInitializeProviderThread(&context->ProviderThread, updateInterval);
                PhRegisterProvider(
                    &context->ProviderThread,
                    WeWindowProviderUpdate,
                    context,
                    &context->ProviderRegistration
                    );
                PhSetEnabledProvider(&context->ProviderRegistration, TRUE);
                PhStartProviderThread(&context->ProviderThread);
            }

            WepRefreshWindows(context); // Initial refresh or provider boost

            if (context->TreeContext.EnableStateHighlighting)
                PhSetTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT, 100, NULL);

            PhSetDialogFocus(WindowHandle, context->TreeNewHandle);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));


            //context->WindowHandle = WindowHandle;
            //context->TreeNewHandle = GetDlgItem(WindowHandle, IDC_LIST);
            //context->SearchBoxHandle = GetDlgItem(WindowHandle, IDC_SEARCHEDIT);
            //context->FindWindowButtonHandle = GetDlgItem(WindowHandle, IDC_FINDWINDOW);

            //PhCreateSearchControl(
            //    WindowHandle,
            //    context->SearchBoxHandle,
            //    L"Search Windows (Ctrl+K)",
            //    WepWindowsPageSearchControlCallback,
            //    context
            //    );

            //// Initialize the tree FIRST before registering provider callbacks
            //WeInitializeWindowTree(WindowHandle, context->TreeNewHandle, &context->TreeContext);
            //context->TreeContext.ColorNew = context->ColorNew;
            //context->TreeContext.ColorRemoved = context->ColorRemoved;
            //TreeNew_SetEmptyText(context->TreeNewHandle, &WepEmptyWindowsText, 0);
            //WeInitializeWindowTreeImageList(&context->TreeContext, &context->Selector);

            ////if (context->EnableWindowProvider)
            ////{
            ////    PhRegisterCallback(
            ////        &context->WindowProvider->WindowAddedEvent,
            ////        WepWindowProviderAddedHandler,
            ////        context,
            ////        &context->WindowProviderAddedRegistration
            ////        );
            ////    PhRegisterCallback(
            ////        &context->WindowProvider->WindowModifiedEvent,
            ////        WepWindowProviderModifiedHandler,
            ////        context,
            ////        &context->WindowProviderModifiedRegistration
            ////        );
            ////    PhRegisterCallback(
            ////        &context->WindowProvider->WindowRemovedEvent,
            ////        WepWindowProviderRemovedHandler,
            ////        context,
            ////        &context->WindowProviderRemovedRegistration
            ////        );
            ////    PhRegisterCallback(
            ////        &context->WindowProvider->WindowUpdatedEvent,
            ////        WepWindowProviderUpdatedHandler,
            ////        context,
            ////        &context->WindowProviderUpdatedRegistration
            ////        );

            ////    // Now it's safe to start the provider thread
            ////    PhStartProviderThread(&context->ProviderThread);
            ////}

            //PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            //PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_SEARCHEDIT), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            //PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_LIST), NULL, PH_ANCHOR_ALL);

            //// Subclass the button.
            //context->FindWindowButtonWindowProc = PhGetWindowProcedure(context->FindWindowButtonHandle);
            //PhSetWindowContext(context->FindWindowButtonHandle, PH_WINDOW_CONTEXT_DEFAULT, context->FindWindowButtonWindowProc);
            //PhSetWindowProcedure(context->FindWindowButtonHandle, WepFindWindowButtonSubclassProc);

            //WepRefreshWindows(context);

            //if (context->TreeContext.EnableStateHighlighting)
            //    PhSetTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT, 100, NULL);

            //PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));
        }
        break;
    case WM_DESTROY:
        {
            PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);

            PhSetEnabledProvider(&context->ProviderRegistration, FALSE);
            PhUnregisterProvider(&context->ProviderRegistration);

            PhStopProviderThread(&context->ProviderThread);
            PhDeleteProviderThread(&context->ProviderThread);

            PhUnregisterCallback(&context->WindowProvider->WindowAddedEvent, &context->WindowProviderAddedRegistration);
            PhUnregisterCallback(&context->WindowProvider->WindowModifiedEvent, &context->WindowProviderModifiedRegistration);
            PhUnregisterCallback(&context->WindowProvider->WindowRemovedEvent, &context->WindowProviderRemovedRegistration);
            PhUnregisterCallback(&context->WindowProvider->WindowUpdatedEvent, &context->WindowProviderUpdatedRegistration);

            //PhUnregisterProvider(&context->ProviderRegistration);
            PhDeleteProviderEventQueue(&context->EventQueue);
            WeDeleteWindowProvider(context->WindowProvider);

            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackWindowNotifyEvent), &context->WindowNotifyCallbackRegistration);

            //if (context->EnableWindowProvider)
            //{
            //    PhUnregisterCallback(&context->WindowProvider->WindowAddedEvent, &context->WindowProviderAddedRegistration);
            //    PhUnregisterCallback(&context->WindowProvider->WindowModifiedEvent, &context->WindowProviderModifiedRegistration);
            //    PhUnregisterCallback(&context->WindowProvider->WindowRemovedEvent, &context->WindowProviderRemovedRegistration);
            //    PhUnregisterCallback(&context->WindowProvider->WindowUpdatedEvent, &context->WindowProviderUpdatedRegistration);

            //    PhSetEnabledProvider(&context->ProviderRegistration, FALSE);
            //    PhUnregisterProvider(&context->ProviderRegistration);
            //    PhDeleteProviderEventQueue(&context->EventQueue);
            //    WeDeleteWindowProvider(context->WindowProvider);
            //}

            PhDeleteLayoutManager(&context->LayoutManager);

            WeDeleteWindowTree(&context->TreeContext);
            WepDeleteWindowSelector(&context->Selector);
            PhDereferenceObject(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (PhBeginPropPageLayout(WindowHandle, propPageContext))
                PhEndPropPageLayout(WindowHandle, propPageContext);
        }
        break;
    case WE_WM_WINDOWS_UPDATED:
        {
            ULONG upToRunId = (ULONG)wParam;
            PPH_PROVIDER_EVENT events;
            ULONG count = 0;
            ULONG i;

            events = PhFlushProviderEventQueue(&context->EventQueue, upToRunId, &count);

            WeTickWindowNodes(context, &context->TreeContext);

            if (events)
            {
                TreeNew_SetRedraw(context->TreeNewHandle, FALSE);

                for (i = 0; i < count; i++)
                {
                    PH_PROVIDER_EVENT_TYPE type = PH_PROVIDER_EVENT_TYPE(events[i]);
                    PWE_WINDOW_ITEM windowItem = PH_PROVIDER_EVENT_OBJECT(events[i]);

                    switch (type)
                    {
                    case ProviderAddedEvent:
                        {
                            if (!WeFindWindowNode(&context->TreeContext, windowItem->WindowHandle))
                                WeAddWindowNode(&context->TreeContext, windowItem, events[i].RunId);

                            PhDereferenceObject(windowItem);
                        }
                        break;
                    case ProviderModifiedEvent:
                        {
                            PWE_WINDOW_NODE node;

                            if (node = WeFindWindowNode(&context->TreeContext, windowItem->WindowHandle))
                            {
                                WeUpdateWindowNode(node, &context->TreeContext);
                            }

                            PhDereferenceObject(windowItem);
                        }
                        break;
                    case ProviderRemovedEvent:
                        {
                            PWE_WINDOW_NODE node;

                            if (node = WeFindWindowNode(&context->TreeContext, windowItem->WindowHandle))
                            {
                                WeRemoveWindowNode(node, &context->TreeContext);
                            }

                            PhDereferenceObject(windowItem);
                        }
                        break;
                    }
                }

                PhFree(events);
            }

            if (count != 0)
            {
                // Structure newly added nodes in the same cycle they were added. WeTickWindowNodes
                // (called above, before these events) is what normally honors NeedsNodesStructured,
                // so without this the new nodes wouldn't appear until the next provider tick - by
                // which point PH_TICK_SH_STATE_TN has already aged out their "new item" highlight,
                // making them appear instantly with no green.
                if (NeedsNodesStructured)
                {
                    NeedsNodesStructured = FALSE;
                    TreeNew_NodesStructured(context->TreeNewHandle);
                }

                TreeNew_SetRedraw(context->TreeNewHandle, TRUE);
                InvalidateRect(context->TreeNewHandle, NULL, FALSE);
            }
        }
        break;
    case WM_COMMAND:
        {
            ULONG commandId = GET_WM_COMMAND_ID(wParam, lParam);
            PWE_WINDOW_NODE selectedNode = WeGetSelectedWindowNode(&context->TreeContext);

            if (selectedNode && WepExecuteWindowCommand(WindowHandle, selectedNode->WindowHandle, commandId))
            {
                PhBoostProvider(&context->ProviderRegistration, NULL);
                return TRUE;
            }

            switch (commandId)
            {
            case IDC_REFRESH:
                {
                    WepRefreshWindows(context);

                    PhApplyTreeNewFilters(&context->TreeContext.FilterSupport);
                }
                break;
            case IDC_OPTIONS:
                {
                    RECT rect;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM enumMessageOnlyItem;
                    PPH_EMENU_ITEM enumNonVisibleItem;
                    PPH_EMENU_ITEM highlightMessageOnlyItem;
                    PPH_EMENU_ITEM iconsItem;
                    PPH_EMENU_ITEM desktopItem;
                    PPH_EMENU_ITEM parentChildItem;
                    PPH_EMENU_ITEM zOrderItem;
                    PPH_EMENU_ITEM ownerItem;
                    PPH_EMENU_ITEM selectedItem;

                    GetWindowRect(GetDlgItem(WindowHandle, IDC_OPTIONS), &rect);

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, enumMessageOnlyItem = PhCreateEMenuItem(0, ID_WINDOW_OPTIONS_ENUM_MESSAGEONLY, L"Enumerate message-only windows", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, enumNonVisibleItem = PhCreateEMenuItem(0, ID_WINDOW_OPTIONS_ENUM_NONVISIBLE, L"Enumerate non-visible windows", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightMessageOnlyItem = PhCreateEMenuItem(0, ID_WINDOW_OPTIONS_HIGHLIGHT_MESSAGEONLY, L"Highlight message-only windows", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, iconsItem = PhCreateEMenuItem(0, 1, L"Enable icons", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, desktopItem = PhCreateEMenuItem(0, 3, L"Show desktop windows", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, parentChildItem = PhCreateEMenuItem(0, ID_VIEW_MODE_PARENTCHILD, L"Enumerate windows", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, zOrderItem = PhCreateEMenuItem(0, ID_VIEW_MODE_ZORDER, L"Enumerate windows by z-order", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, ownerItem = PhCreateEMenuItem(0, ID_VIEW_MODE_OWNER, L"Enumerate windows by owner", NULL, NULL), ULONG_MAX);

                    if (PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_MESSAGEONLY))
                        enumMessageOnlyItem->Flags |= PH_EMENU_CHECKED;
                    if (PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_NONVISIBLE))
                        enumNonVisibleItem->Flags |= PH_EMENU_CHECKED;
                    if (PhGetIntegerSetting(SETTING_NAME_WINDOW_HIGHLIGHT_MESSAGEONLY))
                        highlightMessageOnlyItem->Flags |= PH_EMENU_CHECKED;
                    if (PhGetIntegerSetting(SETTING_NAME_WINDOW_ENABLE_ICONS))
                        iconsItem->Flags |= PH_EMENU_CHECKED;
                    if (PhGetIntegerSetting(SETTING_NAME_SHOW_DESKTOP_WINDOWS))
                        desktopItem->Flags |= PH_EMENU_CHECKED;

                    PhSetFlagsEMenuItem(menu, ID_VIEW_MODE_PARENTCHILD, PH_EMENU_RADIOCHECK | PH_EMENU_CHECKED,
                        (context->TreeContext.ViewMode == WeWindowViewModeParentChild) ? (PH_EMENU_RADIOCHECK | PH_EMENU_CHECKED) : 0);
                    PhSetFlagsEMenuItem(menu, ID_VIEW_MODE_ZORDER, PH_EMENU_RADIOCHECK | PH_EMENU_CHECKED,
                        (context->TreeContext.ViewMode == WeWindowViewModeZOrder) ? (PH_EMENU_RADIOCHECK | PH_EMENU_CHECKED) : 0);
                    PhSetFlagsEMenuItem(menu, ID_VIEW_MODE_OWNER, PH_EMENU_RADIOCHECK | PH_EMENU_CHECKED,
                        (context->TreeContext.ViewMode == WeWindowViewModeOwner) ? (PH_EMENU_RADIOCHECK | PH_EMENU_CHECKED) : 0);

                    selectedItem = PhShowEMenu(
                        menu,
                        WindowHandle,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        rect.left,
                        rect.bottom
                        );

                    if (selectedItem && selectedItem->Id)
                    {
                        switch (selectedItem->Id)
                        {
                        case ID_WINDOW_OPTIONS_ENUM_MESSAGEONLY:
                            {
                                BOOLEAN value = !PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_MESSAGEONLY);
                                PhSetIntegerSetting(SETTING_NAME_WINDOW_ENUM_MESSAGEONLY, value);
                                context->TreeContext.EnableMessageOnly = value;
                                PhBoostProvider(&context->ProviderRegistration, NULL);
                            }
                            break;
                        case ID_WINDOW_OPTIONS_ENUM_NONVISIBLE:
                            {
                                BOOLEAN value = !PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_NONVISIBLE);
                                PhSetIntegerSetting(SETTING_NAME_WINDOW_ENUM_NONVISIBLE, value);
                                context->TreeContext.EnableWindowVisible = value;
                                // Let the background provider reconcile the tree so newly-included windows flash the
                                // "new item" highlight (and now-excluded windows are removed) instead of an instant
                                // clear + repopulate via WepRefreshWindows.
                                PhBoostProvider(&context->ProviderRegistration, NULL);
                            }
                            break;
                        case ID_WINDOW_OPTIONS_HIGHLIGHT_MESSAGEONLY:
                            {
                                BOOLEAN value = !PhGetIntegerSetting(SETTING_NAME_WINDOW_HIGHLIGHT_MESSAGEONLY);
                                PhSetIntegerSetting(SETTING_NAME_WINDOW_HIGHLIGHT_MESSAGEONLY, value);
                                context->TreeContext.HighlightMessageOnly = value;
                                WeInvalidateWindowTreeColors(&context->TreeContext);
                            }
                            break;
                        case 1:
                            {
                                BOOLEAN value = !PhGetIntegerSetting(SETTING_NAME_WINDOW_ENABLE_ICONS);
                                PhSetIntegerSetting(SETTING_NAME_WINDOW_ENABLE_ICONS, value);
                                WeSetWindowTreeIconsEnabled(&context->TreeContext, value);
                            }
                            break;
                        case 3:
                            {
                                BOOLEAN value = !PhGetIntegerSetting(SETTING_NAME_SHOW_DESKTOP_WINDOWS);
                                PhSetIntegerSetting(SETTING_NAME_SHOW_DESKTOP_WINDOWS, value);
                                PhApplyTreeNewFilters(&context->TreeContext.FilterSupport);
                            }
                            break;
                        case ID_VIEW_MODE_PARENTCHILD:
                        case ID_VIEW_MODE_ZORDER:
                        case ID_VIEW_MODE_OWNER:
                            {
                                WE_WINDOW_VIEW_MODE newViewMode;

                                if (selectedItem->Id == ID_VIEW_MODE_PARENTCHILD)
                                    newViewMode = WeWindowViewModeParentChild;
                                else if (selectedItem->Id == ID_VIEW_MODE_ZORDER)
                                    newViewMode = WeWindowViewModeZOrder;
                                else
                                    newViewMode = WeWindowViewModeOwner;

                                if (context->TreeContext.ViewMode != newViewMode)
                                {
                                    context->TreeContext.ViewMode = newViewMode;
                                    WepRefreshWindows(context);
                                    PhApplyTreeNewFilters(&context->TreeContext.FilterSupport);
                                }
                            }
                            break;
                        }
                    }

                    PhDestroyEMenu(menu);
                }
                break;
            case ID_SHOWCONTEXTMENU:
                {
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
                    PWE_WINDOW_NODE *windows;
                    ULONG numberOfWindows;
                    PPH_EMENU menu;
                    PPH_EMENU selectedItem;

                    if (!WeGetSelectedWindowNodes(&context->TreeContext, &windows, &numberOfWindows))
                        break;

                    if (numberOfWindows != 0)
                    {
                        menu = PhCreateEMenu();
                        WepCreateWindowMenu(menu, TRUE, TRUE);
                        PhInsertCopyCellEMenuItem(menu, ID_WINDOW_COPY, context->TreeNewHandle, contextMenuEvent->Column);
                        PhSetFlagsEMenuItem(menu, ID_WINDOW_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

                        if (numberOfWindows == 1)
                        {
                            WepSetWindowMenuState(menu, windows[0]->WindowHandle);
                        }
                        else
                        {
                            PhSetFlagsAllEMenuItems(menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                            PhSetFlagsEMenuItem(menu, ID_WINDOW_COPY, PH_EMENU_DISABLED, 0);
                        }

                        selectedItem = PhShowEMenu(
                            menu,
                            WindowHandle,
                            PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            contextMenuEvent->Location.x,
                            contextMenuEvent->Location.y
                            );

                        if (selectedItem && selectedItem->Id != ULONG_MAX)
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
                        //WeWindowEndTask(selectedNode->WindowHandle, TRUE);
                        PostMessage(selectedNode->WindowHandle, WM_CLOSE, 0, 0);
                    }
                }
                break;
            case ID_WINDOW_DESTROY:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        NTSTATUS status = WeDestroyRemoteWindow(
                            selectedNode->WindowHandle,
                            selectedNode->WindowItem->ClientId.UniqueProcess
                            );

                        if (!NT_SUCCESS(status))
                            PhShowStatus(WindowHandle, L"Unable to destroy the window.", status, 0);
                    }
                }
                break;
            case ID_WINDOW_VISIBLE:
                {
                    PWE_WINDOW_NODE selectedNode;
                    HWND currentWindow;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        currentWindow = selectedNode->WindowHandle;

                        if (WeIsWindowEffectivelyVisible(currentWindow)) // If it's effectively visible, hide it.
                        {
                            selectedNode->WindowItem->WindowVisible = FALSE;
                            ShowWindowAsync(currentWindow, SW_HIDE);
                        }
                        else // If not effectively visible, make it and its parents visible.
                        {
                            BOOL invisibleParentFound = FALSE;
                            HWND tempParent = GetParent(currentWindow);
                            while (tempParent && tempParent != GetDesktopWindow())
                            {
                                if (!IsWindowVisible(tempParent))
                                {
                                    invisibleParentFound = TRUE;
                                    break;
                                }
                                tempParent = GetParent(tempParent);
                            }

                            if (invisibleParentFound)
                            {
                                if (MessageBoxW(
                                    context->WindowHandle, // Parent window for the message box
                                    L"The parent window(s) of this window are currently invisible.\n\n"
                                    L"Do you want to make the parent window(s) visible along with this window?",
                                    L"Confirm Parent Visibility Change",
                                    MB_YESNO | MB_ICONQUESTION
                                ) == IDNO)
                                {
                                    // User chose not to make parents visible, so just make the current window visible (current behavior)
                                    selectedNode->WindowItem->WindowVisible = TRUE;
                                    ShowWindowAsync(currentWindow, SW_SHOW);
                                }
                                else
                                {
                                    // User confirmed, make current window and its parents visible
                                    selectedNode->WindowItem->WindowVisible = TRUE;
                                    ShowWindowAsync(currentWindow, SW_SHOW);

                                    HWND parent = GetParent(currentWindow);
                                    while (parent && parent != GetDesktopWindow())
                                    {
                                        if (!IsWindowVisible(parent))
                                        {
                                            ShowWindowAsync(parent, SW_SHOW);
                                        }
                                        parent = GetParent(parent);
                                    }
                                }
                            }
                            else // No invisible parents found, just make current window visible
                            {
                                selectedNode->WindowItem->WindowVisible = TRUE;
                                ShowWindowAsync(currentWindow, SW_SHOW);
                            }
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
                        PhSetTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT, 100, NULL);
                    }
                }
                break;
            case ID_WINDOW_GOTOTHREAD:
                {
                    PWE_WINDOW_NODE selectedNode;
                    PPH_PROCESS_ITEM selectedProcessItem;
                    PPH_PROCESS_PROPCONTEXT propContext;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (selectedProcessItem = PhReferenceProcessItem(selectedNode->WindowItem->ClientId.UniqueProcess))
                        {
                            if (propContext = PhCreateProcessPropContext(NULL, selectedProcessItem))
                            {
                                PhSetSelectThreadIdProcessPropContext(propContext, selectedNode->WindowItem->ClientId.UniqueThread);
                                PhShowProcessProperties(propContext);
                                PhDereferenceObject(propContext);
                            }

                            PhDereferenceObject(selectedProcessItem);
                        }
                        else
                        {
                            PhShowError2(WindowHandle, L"The window does not exist.", L"%s", L"");
                        }
                    }
                }
                break;
            case ID_WINDOW_INSPECT:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (
                            !PhIsNullOrEmptyString(selectedNode->FileNameWin32) &&
                            PhDoesFileExistWin32(PhGetString(selectedNode->FileNameWin32))
                            )
                        {
                            PhShellExecuteUserString(
                                WindowHandle,
                                SETTING_PROGRAM_INSPECT_EXECUTABLES,
                                PhGetString(selectedNode->FileNameWin32),
                                FALSE,
                                L"Make sure the PE Viewer executable file is present."
                                );
                        }
                    }
                }
                break;
            case ID_WINDOW_OPENFILELOCATION:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (
                            !PhIsNullOrEmptyString(selectedNode->FileNameWin32) &&
                            PhDoesFileExistWin32(PhGetString(selectedNode->FileNameWin32))
                            )
                        {
                            PhShellExecuteUserString(
                                WindowHandle,
                                SETTING_FILE_BROWSE_EXECUTABLE,
                                PhGetString(selectedNode->FileNameWin32),
                                FALSE,
                                L"Make sure the Explorer executable file is present."
                                );
                        }
                    }
                }
                break;
            case ID_WINDOW_PROPERTIES:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (!WeShowWindowProperties(WindowHandle, selectedNode->WindowHandle))
                        {
                            PhShowError2(WindowHandle, L"The window does not exist.", L"%s", L"");
                        }
                    }
                }
                break;
            case ID_WINDOW_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                    PhSetClipboardString(WindowHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            case ID_WINDOW_SETDPI:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        PPH_STRING selectedChoice = NULL;

                        while (PhaChoiceDialog(
                            WindowHandle,
                            L"Enter new Window DPI:",
                            L"Enter new Window DPI:",
                            NULL,
                            0,
                            NULL,
                            PH_CHOICE_DIALOG_USER_CHOICE,
                            &selectedChoice,
                            NULL,
                            NULL
                            ))
                        {
                            LONG64 value;

                            if (selectedChoice->Length == 0)
                                continue;

                            if (PhStringToInteger64(&selectedChoice->sr, 0, &value))
                            {
                                WeSetWindowToDpiForTesting(selectedNode->WindowHandle, (LONG)value);
                                break;
                            }
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_TIMER:
        {
            switch (wParam)
            {
            case PH_WINDOW_TIMER_DEFAULT:
                {
                    //if (context->TreeContext.EnableStateHighlighting)
                    //{
                    //    PH_TICK_SH_STATE_TN(
                    //        WE_WINDOW_NODE,
                    //        ShState,
                    //        context->TreeContext.NodeStateList,
                    //        WepRemoveWindowNode,
                    //        context->HighlightingDuration,
                    //        context->TreeNewHandle,
                    //        TRUE,
                    //        NULL,
                    //        &context->TreeContext
                    //        );
                    //}

                    //if (context->TreeContext.NodeRestructure)
                    //{
                    //    TreeNew_NodesStructured(context->TreeNewHandle);
                    //    context->TreeContext.NodeRestructure = FALSE;
                    //}

                    //if (context->HighlightingWindowCount > 0)
                    //{
                    //    WeInvertWindowBorder(context->HighlightingWindow);

                    //    if (--context->HighlightingWindowCount == 0 && !context->TreeContext.EnableStateHighlighting)
                    //        PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);
                    //}
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
                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, (LPARAM)context->TreeNewHandle);
                return TRUE;
            }
        }
        break;
    case WM_KEYDOWN:
        {
            if (LOWORD(wParam) == 'K')
            {
                if (GetKeyState(VK_CONTROL) < 0)
                {
                    SetFocus(context->SearchBoxHandle);
                    return TRUE;
                }
            }
        }
        break;
    case WE_WM_FINDWINDOW:
        {
            WeSnapshotModeStart(WindowHandle, context);
        }
        break;
    case WM_MOUSEMOVE:
        {
            if (context->TargetingWindow)
            {
                POINT cursorPos;
                HWND windowOverMouse;
                CLIENT_ID clientId;

                if (!PhGetMessagePos(&cursorPos))
                    break;

                windowOverMouse = WindowFromPoint(cursorPos);

                if (context->TargetingCurrentWindow != windowOverMouse)
                {
                    if (context->TargetingCurrentWindow && context->TargetingCurrentWindowDraw)
                    {
                        // Invert the old border (to remove it).
                        WeDrawWindowBorderForTargeting(context->TargetingCurrentWindow);
                    }

                    if (windowOverMouse && IsWindow(windowOverMouse))
                    {
                        // Draw a rectangle over the current window (but not if it's one of our own).
                        if (
                            NT_SUCCESS(PhGetWindowClientId(windowOverMouse, &clientId)) &&
                            clientId.UniqueProcess != NtCurrentProcessId()
                            )
                        {
                            WeDrawWindowBorderForTargeting(windowOverMouse);
                            context->TargetingCurrentWindowDraw = TRUE;
                        }
                        else
                        {
                            context->TargetingCurrentWindowDraw = FALSE;
                        }
                    }
                    else
                    {
                        context->TargetingCurrentWindowDraw = FALSE;
                    }

                    context->TargetingCurrentWindow = windowOverMouse;
                }
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            if (context->TargetingWindow)
            {
                CLIENT_ID clientId;

                context->TargetingCompleted = TRUE;

                // Reset the original cursor.
                PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

                // Bring the window back to the top, and preserve the Always on Top setting.
                SetWindowPos(GetParent(WindowHandle), HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                context->TargetingWindow = FALSE;
                ReleaseCapture();

                if (context->TargetingCurrentWindow && IsWindow(context->TargetingCurrentWindow))
                {
                    if (context->TargetingCurrentWindowDraw)
                    {
                        // Remove the border on the window we found.
                        WeDrawWindowBorderForTargeting(context->TargetingCurrentWindow);
                        context->TargetingCurrentWindowDraw = FALSE;
                    }

                    //if (ResolveGhostWindows)
                    {
                        HWND hungWindow = PhHungWindowFromGhostWindow(context->TargetingCurrentWindow);

                        if (hungWindow)
                            context->TargetingCurrentWindow = hungWindow;
                    }

                    if (
                        NT_SUCCESS(PhGetWindowClientId(context->TargetingCurrentWindow, &clientId)) &&
                        clientId.UniqueProcess != NtCurrentProcessId()
                        )
                    {
                        PWE_WINDOW_NODE windowNode;

                        if (windowNode = WeFindWindowNode(&context->TreeContext, context->TargetingCurrentWindow))
                        {
                            WeSelectAndEnsureVisibleWindowNodes(&context->TreeContext, &windowNode, 1);

                            SetFocus(context->TreeNewHandle);
                        }
                        else
                        {
                            WeDeselectAllWindowNodes(&context->TreeContext);
                        }
                    }
                }
            }
        }
        break;
    case WM_CAPTURECHANGED:
        {
            if (context->TargetingWindow && !context->TargetingCompleted)
            {
                // The user cancelled the targeting, probably by pressing the Esc key.

                if (context->TargetingCurrentWindowDraw)
                {
                    // Remove the border on the currently selected window.
                    if (context->TargetingCurrentWindow && IsWindow(context->TargetingCurrentWindow))
                    {
                        WeDrawWindowBorderForTargeting(context->TargetingCurrentWindow);
                    }

                    context->TargetingCurrentWindowDraw = FALSE;
                }

                // Reset cursor
                PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

                SetWindowPos(GetParent(WindowHandle), HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                context->TargetingWindow = FALSE;
                context->TargetingCompleted = TRUE;
            }
        }
        break;
    }

    return FALSE;
}
