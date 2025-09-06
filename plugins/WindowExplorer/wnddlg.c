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

typedef struct _WINDOWS_CONTEXT
{
    HWND TreeNewHandle;
    HWND SearchBoxHandle;
    HFONT TreeWindowFont;

    HWND FindWindowButtonHandle;
    WNDPROC FindWindowButtonWindowProc;
    PH_CALLBACK_REGISTRATION WindowNotifyCallbackRegistration;

    WE_WINDOW_TREE_CONTEXT TreeContext;
    WE_WINDOW_SELECTOR Selector;

    PH_LAYOUT_MANAGER LayoutManager;

    HWND HighlightingWindow;
    ULONG HighlightingWindowCount;

    BOOLEAN TargetingWindow;
    BOOLEAN TargetingCurrentWindowDraw;
    BOOLEAN TargetingCompleted;
    HWND TargetingCurrentWindow;
} WINDOWS_CONTEXT, *PWINDOWS_CONTEXT;

typedef struct _WE_WINDOW_ENUM_CONTEXT
{
    _In_ PWINDOWS_CONTEXT WindowContext;
    _In_ PWE_WINDOW_NODE RootNode;
    _In_opt_ HANDLE FilterProcessId;
    _In_opt_ HANDLE FilterThreadId;
} WE_WINDOW_ENUM_CONTEXT, *PWE_WINDOW_ENUM_CONTEXT;

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

VOID WepAddEnumChildWindows(
    _In_ PWINDOWS_CONTEXT Context,
    _In_opt_ PWE_WINDOW_NODE ParentNode,
    _In_ HWND hwnd,
    _In_opt_ HANDLE FilterProcessId,
    _In_opt_ HANDLE FilterThreadId
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

        context = PhAllocateZero(sizeof(WINDOWS_CONTEXT));
        memcpy(&context->Selector, Selector, sizeof(WE_WINDOW_SELECTOR));

        if (!NT_SUCCESS(PhCreateThreadEx(&WepWindowsDialogThreadHandle, WepShowWindowsDialogThread, context)))
        {
            PhFree(context);
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

    context = PhAllocateZero(sizeof(WINDOWS_CONTEXT));
    memcpy(&context->Selector, Selector, sizeof(WE_WINDOW_SELECTOR));

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

VOID WepFillWindowInfo(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ PWE_WINDOW_NODE Node
    )
{
    PPH_STRING windowText;

    PhPrintPointer(Node->WindowHandleString, Node->WindowHandle);
    GetClassName(Node->WindowHandle, Node->WindowClass, RTL_NUMBER_OF(Node->WindowClass));

    if (PhGetWindowTextEx(Node->WindowHandle, PH_GET_WINDOW_TEXT_INTERNAL, &windowText) > 0)
        PhMoveReference(&Node->WindowText, windowText);

    if (PhIsNullOrEmptyString(Node->WindowText))
        PhMoveReference(&Node->WindowText, PhReferenceEmptyString());

    PhGetWindowClientId(Node->WindowHandle, &Node->ClientId);
    Node->ThreadString = PhGetClientIdName(&Node->ClientId);

    Node->WindowVisible = !!IsWindowVisible(Node->WindowHandle);
    Node->HasChildren = !!FindWindowEx(Node->WindowHandle, NULL, NULL, NULL);

    if (Node->ClientId.UniqueProcess)
    {
        PPH_PROCESS_ITEM processItem;
        PVOID instanceHandle;
        PPH_STRING fileName;

        if (processItem = PhReferenceProcessItem(Node->ClientId.UniqueProcess))
        {
            if (processItem->QueryHandle)
            {
                if (!(instanceHandle = (PVOID)GetWindowLongPtr(Node->WindowHandle, GWLP_HINSTANCE)))
                {
                    instanceHandle = (PVOID)GetClassLongPtr(Node->WindowHandle, GCLP_HMODULE);
                }

                if (instanceHandle)
                {
                    if (NT_SUCCESS(PhGetProcessMappedFileName(processItem->QueryHandle, instanceHandle, &fileName)))
                    {
                        PhMoveReference(&fileName, PhGetFileName(fileName));
                        PhSetReference(&Node->FileNameWin32, fileName);

                        PhMoveReference(&fileName, PhGetBaseName(fileName));
                        PhMoveReference(&Node->ModuleString, fileName);
                    }
                }
            }

            PhDereferenceObject(processItem);
        }
    }

    if (Context->FilterSupport.FilterList) // Note: Apply filter after filling window node data. (dmex)
        Node->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &Node->Node);
}

PWE_WINDOW_NODE WepAddChildWindowNode(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_opt_ PWE_WINDOW_NODE ParentNode,
    _In_ HWND WindowHandle
    )
{
    PWE_WINDOW_NODE childNode;

    if (PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_ALTERNATE))
    {
        if (childNode = WeFindWindowNode(Context, WindowHandle))
            return childNode;
    }

    childNode = WeAddWindowNode(Context, WindowHandle);
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

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN CALLBACK WepEnumChildWindowsProc(
    _In_ HWND WindowHandle,
    _In_ PVOID Context
    )
{
    PWE_WINDOW_ENUM_CONTEXT context = (PWE_WINDOW_ENUM_CONTEXT)Context;
    PWE_WINDOW_NODE childNode;
    CLIENT_ID clientId;

    if (!NT_SUCCESS(PhGetWindowClientId(WindowHandle, &clientId)))
        return TRUE;

    if (
        (!context->FilterProcessId || clientId.UniqueProcess == context->FilterProcessId) &&
        (!context->FilterThreadId || clientId.UniqueThread == context->FilterThreadId)
        )
    {
        childNode = WepAddChildWindowNode(
            &context->WindowContext->TreeContext,
            context->RootNode,
            WindowHandle
            );

        if (childNode->HasChildren)
        {
            WepAddEnumChildWindows(
                context->WindowContext,
                childNode,
                WindowHandle,
                context->FilterProcessId,
                context->FilterThreadId
                );
        }
    }

    return TRUE;
}

VOID WepAddEnumChildWindows(
    _In_ PWINDOWS_CONTEXT Context,
    _In_opt_ PWE_WINDOW_NODE ParentNode,
    _In_ HWND WindowHandle,
    _In_opt_ HANDLE FilterProcessId,
    _In_opt_ HANDLE FilterThreadId
    )
{
    if (PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_ALTERNATE) && WindowHandle != HWND_MESSAGE)
    {
        WE_WINDOW_ENUM_CONTEXT enumContext;

        enumContext.WindowContext = Context;
        enumContext.RootNode = ParentNode;
        enumContext.FilterProcessId = FilterProcessId;
        enumContext.FilterThreadId = FilterThreadId;

        PhEnumChildWindows(WindowHandle, ULONG_MAX, WepEnumChildWindowsProc, &enumContext);
    }
    else
    {
        HWND childWindow = NULL;
        ULONG i = 0;

        // We use FindWindowEx because EnumWindows doesn't return Metro app windows.
        // Set a reasonable limit to prevent infinite loops.
        while (i < 0x4000 && (childWindow = FindWindowEx(WindowHandle, childWindow, NULL, NULL)))
        {
            CLIENT_ID clientId;

            if (!NT_SUCCESS(PhGetWindowClientId(childWindow, &clientId)))
                continue;

            if (
                (!FilterProcessId || clientId.UniqueProcess == FilterProcessId) &&
                (!FilterThreadId || clientId.UniqueThread == FilterThreadId)
                )
            {
                PWE_WINDOW_NODE childNode = WepAddChildWindowNode(&Context->TreeContext, ParentNode, childWindow);

                if (WindowHandle == HWND_MESSAGE)
                {
                    childNode->WindowMessageOnly = TRUE;
                }

                if (childNode->HasChildren)
                {
                    WepAddEnumChildWindows(Context, childNode, childWindow, NULL, NULL);
                }
            }

            i++;
        }
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

            desktopNode = WeAddWindowNode(&Context->TreeContext, GetDesktopWindow());
            WepFillWindowInfo(&Context->TreeContext, desktopNode);

            PhAddItemList(Context->TreeContext.NodeRootList, desktopNode); // HACK

            WepAddEnumChildWindows(
                Context,
                desktopNode,
                desktopNode->WindowHandle,
                NULL,
                NULL
                );

            WepAddEnumChildWindows(
                Context,
                desktopNode,
                HWND_MESSAGE,
                NULL,
                NULL
                );

            desktopNode->HasChildren = TRUE;
        }
        break;
    case WeWindowSelectorThread:
        {
            WepAddEnumChildWindows(
                Context,
                NULL,
                GetDesktopWindow(),
                NULL,
                Context->Selector.Thread.ThreadId
                );

            WepAddEnumChildWindows(
                Context,
                NULL,
                HWND_MESSAGE,
                NULL,
                Context->Selector.Thread.ThreadId
                );
        }
        break;
    case WeWindowSelectorProcess:
        {
            WepAddEnumChildWindows(
                Context,
                NULL,
                GetDesktopWindow(),
                Context->Selector.Process.ProcessId,
                NULL
                );

            WepAddEnumChildWindows(
                Context,
                NULL,
                HWND_MESSAGE,
                Context->Selector.Process.ProcessId,
                NULL
                );
        }
        break;
    case WeWindowSelectorDesktop:
        {
            WepAddDesktopWindows(
                Context,
                PhGetString(Context->Selector.Desktop.DesktopName)
                );
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

VOID DrawWindowBorderForTargeting(
    _In_ HWND hWnd
    )
{
    RECT rect;
    HDC hdc;
    LONG dpiValue;

    GetWindowRect(hWnd, &rect);
    hdc = GetWindowDC(hWnd);

    if (hdc)
    {
        INT penWidth;
        INT oldDc;
        HPEN pen;
        HBRUSH brush;

        dpiValue = PhGetWindowDpi(hWnd);
        penWidth = PhGetSystemMetrics(SM_CXBORDER, dpiValue) * 3;

        oldDc = SaveDC(hdc);

        // Get an inversion effect.
        SetROP2(hdc, R2_NOT);

        pen = CreatePen(PS_INSIDEFRAME, penWidth, RGB(0x00, 0x00, 0x00));
        SelectPen(hdc, pen);

        brush = GetStockBrush(NULL_BRUSH);
        SelectBrush(hdc, brush);

        // Draw the rectangle.
        Rectangle(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top);

        // Cleanup.
        DeletePen(pen);

        RestoreDC(hdc, oldDc);
        ReleaseDC(hWnd, hdc);
    }
}

PPH_EMENU WepCreateWindowMenu(
    _In_ PPH_EMENU WindowMenu
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
    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_PROPERTIES, L"Properties", NULL, NULL), ULONG_MAX);

    PhInsertEMenuItem(WindowMenu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(WindowMenu, PhCreateEMenuItem(0, ID_WINDOW_COPY, L"Copy\bCtrl+C", NULL, NULL), ULONG_MAX);

    return WindowMenu;
}

LRESULT CALLBACK WepFindWindowButtonSubclassProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    WNDPROC oldWndProc;

    if (!(oldWndProc = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT)))
        return 0;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        PhSetWindowProcedure(hwndDlg, oldWndProc);
        PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        break;
    case WM_LBUTTONDOWN:
        PostMessage(GetParent(hwndDlg), WE_WM_FINDWINDOW, 0, 0);
        break;
    }

    return CallWindowProc(oldWndProc, hwndDlg, uMsg, wParam, lParam);
}

BOOLEAN WepWindowEndTask(
    _In_ HWND WindowHandle,
    _In_ BOOL Force
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI* EndTask_I)(
        _In_ HWND WindowHandle,
        _In_ BOOL ShutDown,
        _In_ BOOL Force
        );

    if (PhBeginInitOnce(&initOnce))
    {
        HANDLE moduleHandle;

        if (moduleHandle = PhLoadLibrary(L"user32.dll"))
        {
            EndTask_I = PhGetProcedureAddress(moduleHandle, "EndTask", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!EndTask_I)
        return FALSE;

    return !!EndTask_I(WindowHandle, FALSE, Force);
}

static VOID WepSetWindowToDpiForTesting(
    _In_ HWND WindowHandle,
    _In_ LONG WindowDPI
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI *NtUserForceWindowToDpiForTest_I)(
        _In_ HWND hwnd,
        _In_ UINT32 dpi
        );

    if (PhBeginInitOnce(&initOnce))
    {
        HANDLE baseAddress;

        if (baseAddress = PhLoadLibrary(L"win32u.dll"))
        {
            NtUserForceWindowToDpiForTest_I = PhGetProcedureAddress(baseAddress, "NtUserForceWindowToDpiForTest", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (NtUserForceWindowToDpiForTest_I)
    {
        NtUserForceWindowToDpiForTest_I(WindowHandle, WindowDPI);
    }
}

VOID WepDestroyRemoteWindow(
    _In_ HWND WindowHandle,
    _In_ HWND TargetWindow,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE processHandle = NULL;

    // Windows 8 requires ALL_ACCESS for PLM execution requests. (dmex)
    if (PhWindowsVersion >= WINDOWS_8 && PhWindowsVersion <= WINDOWS_8_1)
    {
        status = PhOpenProcess(
            &processHandle,
            PROCESS_ALL_ACCESS,
            ProcessId
            );
    }

    // Windows 10 and above require SET_LIMITED for PLM execution requests. (dmex)
    if (!NT_SUCCESS(status))
    {
        status = PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_INFORMATION | PROCESS_SET_LIMITED_INFORMATION |
            PROCESS_VM_READ | SYNCHRONIZE,
            ProcessId
            );
    }

    if (NT_SUCCESS(status))
    {
        status = PhDestroyWindowRemote(
            processHandle,
            TargetWindow
            );

        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(WindowHandle, L"Unable to destroy the window.", status, 0);
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
            if (Context->TreeWindowFont) DeleteFont(Context->TreeWindowFont);
            Context->TreeWindowFont = PhDuplicateFont(SystemInformer_GetFont());

            SetWindowFont(Context->TreeNewHandle, Context->TreeWindowFont, TRUE);
        }
        break;
    }
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

    TreeNew_NodesStructured(context->TreeNewHandle);

    if (lastSelectedNode)
    {
        TreeNew_EnsureVisible(context->TreeNewHandle, &lastSelectedNode->Node);
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
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            context->SearchBoxHandle = GetDlgItem(hwndDlg, IDC_SEARCHEDIT);
            context->FindWindowButtonHandle = GetDlgItem(hwndDlg, IDC_FINDWINDOW);
            context->TreeWindowFont = PhDuplicateFont(SystemInformer_GetFont());

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackWindowNotifyEvent),
                WepWindowNotifyEventChangeCallback,
                context,
                &context->WindowNotifyCallbackRegistration
                );

            PhSetApplicationWindowIcon(hwndDlg);
            PhSetWindowText(hwndDlg, PH_AUTO_T(PH_STRING, WepGetWindowTitleForSelector(&context->Selector))->Buffer);

            PhCreateSearchControl(
                hwndDlg,
                context->SearchBoxHandle,
                L"Search Windows (Ctrl+K)",
                WepWindowsSearchControlCallback,
                context
                );

            WeInitializeWindowTree(hwndDlg, context->TreeNewHandle, &context->TreeContext);
            TreeNew_SetEmptyText(context->TreeNewHandle, &WepEmptyWindowsText, 0);
            SetWindowFont(context->TreeNewHandle, context->TreeWindowFont, TRUE);
            WeInitializeWindowTreeImageList(&context->TreeContext, &context->Selector);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SEARCHEDIT), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_LIST), NULL, PH_ANCHOR_ALL);

            if (PhValidWindowPlacementFromSetting(SETTING_NAME_WINDOWS_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_WINDOWS_WINDOW_POSITION, SETTING_NAME_WINDOWS_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, NULL);

            // Subclass the button.
            context->FindWindowButtonWindowProc = PhGetWindowProcedure(context->FindWindowButtonHandle);
            PhSetWindowContext(context->FindWindowButtonHandle, PH_WINDOW_CONTEXT_DEFAULT, context->FindWindowButtonWindowProc);
            PhSetWindowProcedure(context->FindWindowButtonHandle, WepFindWindowButtonSubclassProc);

            WepRefreshWindows(context);

            PhSetDialogFocus(hwndDlg, context->TreeNewHandle);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackWindowNotifyEvent), &context->WindowNotifyCallbackRegistration);

            if (context->TreeWindowFont)
            {
                DeleteFont(context->TreeWindowFont);
            }

            PhKillTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT);

            PhSaveWindowPlacementToSetting(SETTING_NAME_WINDOWS_WINDOW_POSITION, SETTING_NAME_WINDOWS_WINDOW_SIZE, hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);

            WeDeleteWindowTree(&context->TreeContext);
            WepDeleteWindowSelector(&context->Selector);
            PhFree(context);

            PostQuitMessage(0);
        }
        break;
    case WM_PH_SHOW_DIALOG:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_COMMAND:
        {
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

                    if (!WeGetSelectedWindowNodes(&context->TreeContext, &windows, &numberOfWindows))
                        break;

                    if (numberOfWindows != 0)
                    {
                        menu = PhCreateEMenu();
                        WepCreateWindowMenu(menu);
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
                        //WepWindowEndTask(selectedNode->WindowHandle, TRUE);
                        PostMessage(selectedNode->WindowHandle, WM_CLOSE, 0, 0);

                        WepRefreshWindows(context);
                        PhApplyTreeNewFilters(&context->TreeContext.FilterSupport);
                        TreeNew_NodesStructured(context->TreeNewHandle);
                    }
                }
                break;
            case ID_WINDOW_DESTROY:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        WepDestroyRemoteWindow(
                            hwndDlg,
                            selectedNode->WindowHandle,
                            selectedNode->ClientId.UniqueProcess
                            );

                        WepRefreshWindows(context);
                        PhApplyTreeNewFilters(&context->TreeContext.FilterSupport);
                        TreeNew_NodesStructured(context->TreeNewHandle);
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
                        PhSetTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT, 100, NULL);
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
                            if (propContext = PhCreateProcessPropContext(NULL, processItem))
                            {
                                PhSetSelectThreadIdProcessPropContext(propContext, selectedNode->ClientId.UniqueThread);
                                PhShowProcessProperties(propContext);
                                PhDereferenceObject(propContext);
                            }

                            PhDereferenceObject(processItem);
                        }
                        else
                        {
                            PhShowError2(hwndDlg, L"The window does not exist.", L"%s", L"");
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
                        if (processItem = PhReferenceProcessItem(selectedNode->ClientId.UniqueProcess))
                        {
                            if (processNode = PhFindProcessNode(processItem->ProcessId))
                            {
                                SystemInformer_SelectTabPage(0);
                                PhSelectAndEnsureVisibleProcessNode(processNode);
                            }

                            PhDereferenceObject(processItem);
                        }
                        else
                        {
                            PhShowError2(hwndDlg, L"The window does not exist.", L"%s", L"");
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
                                hwndDlg,
                                L"ProgramInspectExecutables",
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
                                hwndDlg,
                                L"FileBrowseExecutable",
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
                        if (!WeShowWindowProperties(hwndDlg, selectedNode->WindowHandle, !!selectedNode->WindowMessageOnly, &selectedNode->ClientId))
                        {
                            PhShowError2(hwndDlg, L"The window does not exist.", L"%s", L"");
                        }
                    }
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
            case ID_WINDOW_SETDPI:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        PPH_STRING selectedChoice = NULL;

                        while (PhaChoiceDialog(
                            hwndDlg,
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
                                WepSetWindowToDpiForTesting(selectedNode->WindowHandle, (LONG)value);
                                break;
                            }
                        }
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
                    WeInvertWindowBorder(context->HighlightingWindow);

                    if (--context->HighlightingWindowCount == 0)
                        PhKillTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT);
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
    //            if (ChildWindowFromPoint(hwndDlg, point) == context->FindWindowButtonHandle)
    //            {
    //                PostMessage(hwndDlg, WE_WM_FINDWINDOW, 0, 0);
    //            }
    //        }
    //    }
    //    break;
    case WE_WM_FINDWINDOW:
        {
            // Direct all mouse events to this window.
            SetCapture(hwndDlg);

            // Set the cursor.
            PhSetCursor(PhLoadCursor(NULL, IDC_CROSS));

            // Send the window to the bottom.
            SetWindowPos(hwndDlg, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

            context->TargetingWindow = TRUE;
            context->TargetingCurrentWindow = NULL;
            context->TargetingCurrentWindowDraw = FALSE;
            context->TargetingCompleted = FALSE;

            SendMessage(hwndDlg, WM_MOUSEMOVE, 0, 0);
        }
        break;
    case WM_MOUSEMOVE:
        {
            if (context->TargetingWindow)
            {
                POINT cursorPos;
                HWND windowOverMouse;
                ULONG processId;
                ULONG threadId;

                GetCursorPos(&cursorPos);
                windowOverMouse = WindowFromPoint(cursorPos);

                if (context->TargetingCurrentWindow != windowOverMouse)
                {
                    if (context->TargetingCurrentWindow && context->TargetingCurrentWindowDraw)
                    {
                        // Invert the old border (to remove it).
                        DrawWindowBorderForTargeting(context->TargetingCurrentWindow);
                    }

                    if (windowOverMouse)
                    {
                        threadId = GetWindowThreadProcessId(windowOverMouse, &processId);

                        // Draw a rectangle over the current window (but not if it's one of our own).
                        if (UlongToHandle(processId) != NtCurrentProcessId())
                        {
                            DrawWindowBorderForTargeting(windowOverMouse);
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
                ULONG processId;
                ULONG threadId;

                context->TargetingCompleted = TRUE;

                // Reset the original cursor.
                PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

                // Bring the window back to the top, and preserve the Always on Top setting.
                SetWindowPos(hwndDlg, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                context->TargetingWindow = FALSE;
                ReleaseCapture();

                if (context->TargetingCurrentWindow)
                {
                    if (context->TargetingCurrentWindowDraw)
                    {
                        // Remove the border on the window we found.
                        DrawWindowBorderForTargeting(context->TargetingCurrentWindow);
                    }

                    //if (ResolveGhostWindows)
                    {
                        HWND hungWindow = PhHungWindowFromGhostWindow(context->TargetingCurrentWindow);

                        if (hungWindow)
                            context->TargetingCurrentWindow = hungWindow;
                    }

                    threadId = GetWindowThreadProcessId(context->TargetingCurrentWindow, &processId);

                    if (threadId && processId && UlongToHandle(processId) != NtCurrentProcessId())
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
            if (!context->TargetingCompleted)
            {
                // The user cancelled the targeting, probably by pressing the Esc key.

                // Remove the border on the currently selected window.
                if (context->TargetingCurrentWindow)
                {
                    if (context->TargetingCurrentWindowDraw)
                    {
                        // Remove the border on the window we found.
                        DrawWindowBorderForTargeting(context->TargetingCurrentWindow);
                    }
                }

                SetWindowPos(hwndDlg, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                context->TargetingCompleted = TRUE;
            }
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

    TreeNew_NodesStructured(context->TreeNewHandle);
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
            context->FindWindowButtonHandle = GetDlgItem(hwndDlg, IDC_FINDWINDOW);

            PhCreateSearchControl(
                hwndDlg,
                context->SearchBoxHandle,
                L"Search Windows (Ctrl+K)",
                WepWindowsPageSearchControlCallback,
                context
                );

            WeInitializeWindowTree(hwndDlg, context->TreeNewHandle, &context->TreeContext);
            TreeNew_SetEmptyText(context->TreeNewHandle, &WepEmptyWindowsText, 0);
            WeInitializeWindowTreeImageList(&context->TreeContext, &context->Selector);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SEARCHEDIT), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_LIST), NULL, PH_ANCHOR_ALL);

            // Subclass the button.
            context->FindWindowButtonWindowProc = PhGetWindowProcedure(context->FindWindowButtonHandle);
            PhSetWindowContext(context->FindWindowButtonHandle, PH_WINDOW_CONTEXT_DEFAULT, context->FindWindowButtonWindowProc);
            PhSetWindowProcedure(context->FindWindowButtonHandle, WepFindWindowButtonSubclassProc);

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

                    if (!WeGetSelectedWindowNodes(&context->TreeContext, &windows, &numberOfWindows))
                        break;

                    if (numberOfWindows != 0)
                    {
                        menu = PhCreateEMenu();
                        WepCreateWindowMenu(menu);
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
                        //WepWindowEndTask(selectedNode->WindowHandle, TRUE);
                        PostMessage(selectedNode->WindowHandle, WM_CLOSE, 0, 0);
                    }
                }
                break;
            case ID_WINDOW_DESTROY:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        WepDestroyRemoteWindow(
                            hwndDlg,
                            selectedNode->WindowHandle,
                            selectedNode->ClientId.UniqueProcess
                            );
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
                        PhSetTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT, 100, NULL);
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
                        if (selectedProcessItem = PhReferenceProcessItem(selectedNode->ClientId.UniqueProcess))
                        {
                            if (propContext = PhCreateProcessPropContext(NULL, selectedProcessItem))
                            {
                                PhSetSelectThreadIdProcessPropContext(propContext, selectedNode->ClientId.UniqueThread);
                                PhShowProcessProperties(propContext);
                                PhDereferenceObject(propContext);
                            }

                            PhDereferenceObject(selectedProcessItem);
                        }
                        else
                        {
                            PhShowError2(hwndDlg, L"The window does not exist.", L"%s", L"");
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
                                hwndDlg,
                                L"ProgramInspectExecutables",
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
                                hwndDlg,
                                L"FileBrowseExecutable",
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
                        if (!WeShowWindowProperties(hwndDlg, selectedNode->WindowHandle, !!selectedNode->WindowMessageOnly, &selectedNode->ClientId))
                        {
                            PhShowError2(hwndDlg, L"The window does not exist.", L"%s", L"");
                        }
                    }
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
            case ID_WINDOW_SETDPI:
                {
                    PWE_WINDOW_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        PPH_STRING selectedChoice = NULL;

                        while (PhaChoiceDialog(
                            hwndDlg,
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
                                WepSetWindowToDpiForTesting(selectedNode->WindowHandle, (LONG)value);
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
                    WeInvertWindowBorder(context->HighlightingWindow);

                    if (--context->HighlightingWindowCount == 0)
                        PhKillTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT);
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
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)context->TreeNewHandle);
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
            // Direct all mouse events to this window.
            SetCapture(hwndDlg);

            // Set the cursor.
            PhSetCursor(PhLoadCursor(NULL, IDC_CROSS));

            // Send the window to the bottom.
            SetWindowPos(GetParent(hwndDlg), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

            context->TargetingWindow = TRUE;
            context->TargetingCurrentWindow = NULL;
            context->TargetingCurrentWindowDraw = FALSE;
            context->TargetingCompleted = FALSE;

            SendMessage(hwndDlg, WM_MOUSEMOVE, 0, 0);
        }
        break;
    case WM_MOUSEMOVE:
        {
            if (context->TargetingWindow)
            {
                POINT cursorPos;
                HWND windowOverMouse;
                ULONG processId;
                ULONG threadId;

                GetCursorPos(&cursorPos);
                windowOverMouse = WindowFromPoint(cursorPos);

                if (context->TargetingCurrentWindow != windowOverMouse)
                {
                    if (context->TargetingCurrentWindow && context->TargetingCurrentWindowDraw)
                    {
                        // Invert the old border (to remove it).
                        DrawWindowBorderForTargeting(context->TargetingCurrentWindow);
                    }

                    if (windowOverMouse)
                    {
                        threadId = GetWindowThreadProcessId(windowOverMouse, &processId);

                        // Draw a rectangle over the current window (but not if it's one of our own).
                        if (UlongToHandle(processId) != NtCurrentProcessId())
                        {
                            DrawWindowBorderForTargeting(windowOverMouse);
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
                ULONG processId;
                ULONG threadId;

                context->TargetingCompleted = TRUE;

                // Reset the original cursor.
                PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

                // Bring the window back to the top, and preserve the Always on Top setting.
                SetWindowPos(GetParent(hwndDlg), HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                context->TargetingWindow = FALSE;
                ReleaseCapture();

                if (context->TargetingCurrentWindow)
                {
                    if (context->TargetingCurrentWindowDraw)
                    {
                        // Remove the border on the window we found.
                        DrawWindowBorderForTargeting(context->TargetingCurrentWindow);
                    }

                    //if (ResolveGhostWindows)
                    {
                        HWND hungWindow = PhHungWindowFromGhostWindow(context->TargetingCurrentWindow);

                        if (hungWindow)
                            context->TargetingCurrentWindow = hungWindow;
                    }

                    threadId = GetWindowThreadProcessId(context->TargetingCurrentWindow, &processId);

                    if (threadId && processId && UlongToHandle(processId) != NtCurrentProcessId())
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
            if (!context->TargetingCompleted)
            {
                // The user cancelled the targeting, probably by pressing the Esc key.

                // Remove the border on the currently selected window.
                if (context->TargetingCurrentWindow)
                {
                    if (context->TargetingCurrentWindowDraw)
                    {
                        // Remove the border on the window we found.
                        DrawWindowBorderForTargeting(context->TargetingCurrentWindow);
                    }
                }

                SetWindowPos(GetParent(hwndDlg), HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                context->TargetingCompleted = TRUE;
            }
        }
        break;
    }

    return FALSE;
}
