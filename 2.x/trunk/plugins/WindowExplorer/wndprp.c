/*
 * Process Hacker Window Explorer - 
 *   window properties
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

// Since the main message loop doesn't support property sheets, 
// we need a separate thread to run property sheets on.

#include "wndexp.h"
#include "resource.h"
#include <windowsx.h>

#define NUMBER_OF_PAGES 3

typedef struct _WINDOW_PROPERTIES_CONTEXT
{
    HWND ParentWindowHandle;
    HWND WindowHandle;

    BOOLEAN HookDataValid;
    ULONG_PTR WndProc;
    WNDCLASSEX ClassInfo;
} WINDOW_PROPERTIES_CONTEXT, *PWINDOW_PROPERTIES_CONTEXT;

typedef struct _STRING_INTEGER_PAIR
{
    PWSTR String;
    ULONG Integer;
} STRING_INTEGER_PAIR, *PSTRING_INTEGER_PAIR;

HWND WepCreateWindowProperties(
    __in PWINDOW_PROPERTIES_CONTEXT Context
    );

INT CALLBACK WepPropSheetProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in LPARAM lParam
    );

LRESULT CALLBACK WepPropSheetWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

HPROPSHEETPAGE WepCommonCreatePage(
    __in PWINDOW_PROPERTIES_CONTEXT Context,
    __in PWSTR Template,
    __in DLGPROC DlgProc
    );

INT CALLBACK WepCommonPropPageProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in LPPROPSHEETPAGE ppsp
    );

NTSTATUS WepPropertiesThreadStart(
    __in PVOID Parameter
    );

INT_PTR CALLBACK WepWindowGeneralDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK WepWindowStylesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK WepWindowPropertiesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

#define DEFINE_PAIR(Symbol) { L#Symbol, Symbol }

static STRING_INTEGER_PAIR WepStylePairs[] =
{
    DEFINE_PAIR(WS_POPUP),
    DEFINE_PAIR(WS_CHILD),
    DEFINE_PAIR(WS_MINIMIZE),
    DEFINE_PAIR(WS_VISIBLE),
    DEFINE_PAIR(WS_DISABLED),
    DEFINE_PAIR(WS_CLIPSIBLINGS),
    DEFINE_PAIR(WS_CLIPCHILDREN),
    DEFINE_PAIR(WS_MAXIMIZE),
    DEFINE_PAIR(WS_BORDER),
    DEFINE_PAIR(WS_DLGFRAME),
    DEFINE_PAIR(WS_VSCROLL),
    DEFINE_PAIR(WS_HSCROLL),
    DEFINE_PAIR(WS_SYSMENU),
    DEFINE_PAIR(WS_THICKFRAME),
    DEFINE_PAIR(WS_GROUP),
    DEFINE_PAIR(WS_TABSTOP),
    DEFINE_PAIR(WS_MINIMIZEBOX),
    DEFINE_PAIR(WS_MAXIMIZEBOX)
};

static STRING_INTEGER_PAIR WepExtendedStylePairs[] =
{
    DEFINE_PAIR(WS_EX_DLGMODALFRAME),
    DEFINE_PAIR(WS_EX_NOPARENTNOTIFY),
    DEFINE_PAIR(WS_EX_TOPMOST),
    DEFINE_PAIR(WS_EX_ACCEPTFILES),
    DEFINE_PAIR(WS_EX_TRANSPARENT),
    DEFINE_PAIR(WS_EX_MDICHILD),
    DEFINE_PAIR(WS_EX_TOOLWINDOW),
    DEFINE_PAIR(WS_EX_WINDOWEDGE),
    DEFINE_PAIR(WS_EX_CLIENTEDGE),
    DEFINE_PAIR(WS_EX_CONTEXTHELP),
    DEFINE_PAIR(WS_EX_RIGHT),
    DEFINE_PAIR(WS_EX_RTLREADING),
    DEFINE_PAIR(WS_EX_LEFTSCROLLBAR),
    DEFINE_PAIR(WS_EX_CONTROLPARENT),
    DEFINE_PAIR(WS_EX_STATICEDGE),
    DEFINE_PAIR(WS_EX_APPWINDOW),
    DEFINE_PAIR(WS_EX_LAYERED),
    DEFINE_PAIR(WS_EX_NOINHERITLAYOUT),
    DEFINE_PAIR(WS_EX_LAYOUTRTL),
    DEFINE_PAIR(WS_EX_COMPOSITED),
    DEFINE_PAIR(WS_EX_NOACTIVATE)
};

HANDLE WePropertiesThreadHandle = NULL;
CLIENT_ID WePropertiesThreadClientId;
PH_EVENT WePropertiesThreadReadyEvent = PH_EVENT_INIT;
PPH_LIST WePropertiesCreateList;
PPH_LIST WePropertiesWindowList;

VOID WeShowWindowProperties(
    __in HWND ParentWindowHandle,
    __in HWND WindowHandle
    )
{
    PWINDOW_PROPERTIES_CONTEXT context;

    if (!WePropertiesCreateList)
        WePropertiesCreateList = PhCreateList(4);
    if (!WePropertiesWindowList)
        WePropertiesWindowList = PhCreateList(4);

    if (!WePropertiesThreadHandle)
    {
        WePropertiesThreadHandle = PhCreateThread(0, WepPropertiesThreadStart, NULL);
        PhWaitForEvent(&WePropertiesThreadReadyEvent, NULL);
    }

    if (NT_SUCCESS(PhCreateAlloc(&context, sizeof(WINDOW_PROPERTIES_CONTEXT))))
    {
        memset(context, 0, sizeof(WINDOW_PROPERTIES_CONTEXT));
        context->ParentWindowHandle = ParentWindowHandle;
        context->WindowHandle = WindowHandle;

        // Queue the window for creation and wake up the host thread.
        PhAddItemList(WePropertiesCreateList, context);
        PostThreadMessage((ULONG)WePropertiesThreadClientId.UniqueThread, WM_NULL, 0, 0);
    }
}

static HWND WepCreateWindowProperties(
    __in PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    HPROPSHEETPAGE pages[NUMBER_OF_PAGES];

    propSheetHeader.dwFlags =
        PSH_MODELESS |
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE |
        PSH_USECALLBACK;
    propSheetHeader.hwndParent = Context->ParentWindowHandle;
    propSheetHeader.pszCaption = PhaFormatString(L"Window %Ix", Context->WindowHandle)->Buffer;
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;
    propSheetHeader.pfnCallback = WepPropSheetProc;

    // General
    pages[propSheetHeader.nPages++] = WepCommonCreatePage(
        Context,
        MAKEINTRESOURCE(IDD_WNDGENERAL),
        WepWindowGeneralDlgProc
        );
    // Styles
    pages[propSheetHeader.nPages++] = WepCommonCreatePage(
        Context,
        MAKEINTRESOURCE(IDD_WNDSTYLES),
        WepWindowStylesDlgProc
        );
    // Properties
    pages[propSheetHeader.nPages++] = WepCommonCreatePage(
        Context,
        MAKEINTRESOURCE(IDD_WNDPROPS),
        WepWindowPropertiesDlgProc
        );

    return (HWND)PropertySheet(&propSheetHeader);
}

static INT CALLBACK WepPropSheetProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case PSCB_INITIALIZED:
        {
            WNDPROC oldWndProc;
            HWND refreshButtonHandle;

            oldWndProc = (WNDPROC)GetWindowLongPtr(hwndDlg, GWLP_WNDPROC);
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)WepPropSheetWndProc);
            SetProp(hwndDlg, L"OldWndProc", (HANDLE)oldWndProc);

            // Hide the Cancel button.
            ShowWindow(GetDlgItem(hwndDlg, IDCANCEL), SW_HIDE);
            // Set the OK button's text to "Close".
            SetDlgItemText(hwndDlg, IDOK, L"Close");
            // Add the Refresh button.
            refreshButtonHandle = CreateWindow(L"BUTTON", L"Refresh", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 3, 3, hwndDlg, (HMENU)IDC_REFRESH,
                PluginInstance->DllBase, NULL);
            SendMessage(refreshButtonHandle, WM_SETFONT, (WPARAM)SendMessage(GetDlgItem(hwndDlg, IDOK), WM_GETFONT, 0, 0), FALSE);
        }
        break;
    }

    return 0;
}

LRESULT CALLBACK WepPropSheetWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    WNDPROC oldWndProc = (WNDPROC)GetProp(hwnd, L"OldWndProc");

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            RemoveProp(hwnd, L"OldWndProc");
            RemoveProp(hwnd, L"Moved");
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!GetProp(hwnd, L"Moved"))
            {
                // Move the Refresh button to where the OK button is, and move the OK button to 
                // where the Cancel button is.
                // This must be done here because in the prop sheet callback the buttons are not 
                // in the right places.
                PhCopyControlRectangle(hwnd, GetDlgItem(hwnd, IDOK), GetDlgItem(hwnd, IDC_REFRESH));
                PhCopyControlRectangle(hwnd, GetDlgItem(hwnd, IDCANCEL), GetDlgItem(hwnd, IDOK));
                SetProp(hwnd, L"Moved", (HANDLE)1);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_REFRESH:
                {
                    ULONG i;
                    HWND pageHandle;

                    // Broadcast the message to all property pages.
                    for (i = 0; i < NUMBER_OF_PAGES; i++)
                    {
                        if (pageHandle = PropSheet_IndexToHwnd(hwnd, i))
                            SendMessage(pageHandle, WM_COMMAND, IDC_REFRESH, 0);
                    }
                }
                break;
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

static HPROPSHEETPAGE WepCommonCreatePage(
    __in PWINDOW_PROPERTIES_CONTEXT Context,
    __in PWSTR Template,
    __in DLGPROC DlgProc
    )
{
    HPROPSHEETPAGE propSheetPageHandle;
    PROPSHEETPAGE propSheetPage;

    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_USECALLBACK;
    propSheetPage.hInstance = PluginInstance->DllBase;
    propSheetPage.pszTemplate = Template;
    propSheetPage.pfnDlgProc = DlgProc;
    propSheetPage.lParam = (LPARAM)Context;
    propSheetPage.pfnCallback = WepCommonPropPageProc;

    propSheetPageHandle = CreatePropertySheetPage(&propSheetPage);

    return propSheetPageHandle;
}

static INT CALLBACK WepCommonPropPageProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in LPPROPSHEETPAGE ppsp
    )
{
    PWINDOW_PROPERTIES_CONTEXT context;

    context = (PWINDOW_PROPERTIES_CONTEXT)ppsp->lParam;

    if (uMsg == PSPCB_ADDREF)
        PhReferenceObject(context);
    else if (uMsg == PSPCB_RELEASE)
        PhDereferenceObject(context);

    return 1;
}

NTSTATUS WepPropertiesThreadStart(
    __in PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    BOOL result;
    MSG message;
    BOOLEAN processed;
    ULONG i;

    PhInitializeAutoPool(&autoPool);

    WePropertiesThreadClientId = NtCurrentTeb()->ClientId;

    // Force the creation of the message queue so PostThreadMessage works.
    PeekMessage(&message, NULL, WM_USER, WM_USER, PM_NOREMOVE);
    PhSetEvent(&WePropertiesThreadReadyEvent);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (WePropertiesCreateList->Count != 0)
        {
            for (i = 0; i < WePropertiesCreateList->Count; i++)
            {
                PWINDOW_PROPERTIES_CONTEXT context;
                HWND hwnd;

                context = WePropertiesCreateList->Items[i];
                hwnd = WepCreateWindowProperties(context);
                PhDereferenceObject(context);
                PhAddItemList(WePropertiesWindowList, hwnd);
            }

            PhClearList(WePropertiesCreateList);
        }

        processed = FALSE;

        for (i = 0; i < WePropertiesWindowList->Count; i++)
        {
            if (PropSheet_IsDialogMessage(WePropertiesWindowList->Items[i], &message))
            {
                processed = TRUE;
                break;
            }
        }

        if (!processed)
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        // Destroy properties windows when necessary.
        for (i = 0; i < WePropertiesWindowList->Count; i++)
        {
            if (!PropSheet_GetCurrentPageHwnd(WePropertiesWindowList->Items[i]))
            {
                DestroyWindow(WePropertiesWindowList->Items[i]);
                PhRemoveItemList(WePropertiesWindowList, i);
                i--;
            }
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

FORCEINLINE BOOLEAN WepPropPageDlgProcHeader(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in LPARAM lParam,
    __out_opt LPPROPSHEETPAGE *PropSheetPage,
    __out_opt PWINDOW_PROPERTIES_CONTEXT *Context
    )
{
    LPPROPSHEETPAGE propSheetPage;

    if (uMsg == WM_INITDIALOG)
    {
        propSheetPage = (LPPROPSHEETPAGE)lParam;
        // Save the context.
        SetProp(hwndDlg, L"PropSheetPage", (HANDLE)lParam);
    }
    else
    {
        propSheetPage = (LPPROPSHEETPAGE)GetProp(hwndDlg, L"PropSheetPage");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"PropSheetPage");
    }

    if (!propSheetPage)
        return FALSE;

    if (PropSheetPage)
        *PropSheetPage = propSheetPage;
    if (Context)
        *Context = (PWINDOW_PROPERTIES_CONTEXT)propSheetPage->lParam;

    return TRUE;
}

static VOID WepEnsureHookDataValid(
    __in PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    if (!Context->HookDataValid)
    {   
        PWE_HOOK_SHARED_DATA data;

        WeHookServerInitialization();

        WeLockServerSharedData(&data);

        if (WeSendServerRequest(Context->WindowHandle))
        {
            Context->WndProc = data->c.WndProc;
            memcpy(&Context->ClassInfo, &data->c.ClassInfo, sizeof(WNDCLASSEX));
        }

        WeUnlockServerSharedData();

        Context->HookDataValid = TRUE;
    }
}

static PPH_STRING WepFormatRect(
    __in PRECT Rect
    )
{
    return PhaFormatString(L"(%d, %d) - (%d, %d) [%dx%d]",
        Rect->left, Rect->top, Rect->right, Rect->bottom,
        Rect->right - Rect->left, Rect->bottom - Rect->top);
}

static VOID WepRefreshWindowGeneralInfo(
    __in HWND hwndDlg,
    __in PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    PPH_STRING windowText;
    ULONG threadId;
    ULONG processId;
    CLIENT_ID clientId;
    PPH_STRING clientIdName;
    WINDOWINFO windowInfo = { sizeof(WINDOWINFO) };
    WINDOWPLACEMENT windowPlacement = { sizeof(WINDOWPLACEMENT) };
    MONITORINFO monitorInfo = { sizeof(MONITORINFO) };

    threadId = GetWindowThreadProcessId(Context->WindowHandle, &processId);

    if (threadId)
    {
        clientId.UniqueProcess = UlongToHandle(processId);
        clientId.UniqueThread = UlongToHandle(threadId);
        clientIdName = PhGetClientIdName(&clientId);
        SetDlgItemText(hwndDlg, IDC_THREAD, clientIdName->Buffer);
        PhDereferenceObject(clientIdName);
    }

    windowText = PHA_DEREFERENCE(PhGetWindowText(Context->WindowHandle));
    SetDlgItemText(hwndDlg, IDC_TEXT, PhGetStringOrEmpty(windowText));

    if (GetWindowInfo(Context->WindowHandle, &windowInfo))
    {
        SetDlgItemText(hwndDlg, IDC_RECTANGLE, WepFormatRect(&windowInfo.rcWindow)->Buffer);
        SetDlgItemText(hwndDlg, IDC_CLIENTRECTANGLE, WepFormatRect(&windowInfo.rcClient)->Buffer);
    }
    else
    {
        SetDlgItemText(hwndDlg, IDC_RECTANGLE, L"N/A");
        SetDlgItemText(hwndDlg, IDC_CLIENTRECTANGLE, L"N/A");
    }

    if (GetWindowPlacement(Context->WindowHandle, &windowPlacement))
    {
        // The rectangle is in workspace coordinates. Convert the values back to screen coordinates.
        if (GetMonitorInfo(MonitorFromRect(&windowPlacement.rcNormalPosition, MONITOR_DEFAULTTOPRIMARY), &monitorInfo))
        {
            windowPlacement.rcNormalPosition.left += monitorInfo.rcWork.left;
            windowPlacement.rcNormalPosition.top += monitorInfo.rcWork.top;
            windowPlacement.rcNormalPosition.right += monitorInfo.rcWork.left;
            windowPlacement.rcNormalPosition.bottom += monitorInfo.rcWork.top;
        }

        SetDlgItemText(hwndDlg, IDC_NORMALRECTANGLE, WepFormatRect(&windowPlacement.rcNormalPosition)->Buffer);
    }
    else
    {
        SetDlgItemText(hwndDlg, IDC_NORMALRECTANGLE, L"N/A");
    }

    SetDlgItemText(hwndDlg, IDC_INSTANCEHANDLE, PhaFormatString(L"0x%Ix", GetWindowLongPtr(Context->WindowHandle, GWLP_HINSTANCE))->Buffer);
    SetDlgItemText(hwndDlg, IDC_MENUHANDLE, PhaFormatString(L"0x%Ix", GetMenu(Context->WindowHandle))->Buffer);

    WepEnsureHookDataValid(Context);

    SetDlgItemText(hwndDlg, IDC_WINDOWPROC, PhaFormatString(L"0x%Ix", Context->WndProc)->Buffer);
}

INT_PTR CALLBACK WepWindowGeneralDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PWINDOW_PROPERTIES_CONTEXT context;

    if (!WepPropPageDlgProcHeader(hwndDlg, uMsg, lParam, NULL, &context))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            WepRefreshWindowGeneralInfo(hwndDlg, context);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_REFRESH:
                context->HookDataValid = FALSE;
                WepRefreshWindowGeneralInfo(hwndDlg, context);
                break;
            }
        }
        break;
    }

    return FALSE;
}

static VOID WepRefreshWindowStyles(
    __in HWND hwndDlg,
    __in PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    WINDOWINFO windowInfo = { sizeof(WINDOWINFO) };
    HWND stylesListBox;
    HWND extendedStylesListBox;
    ULONG i;

    stylesListBox = GetDlgItem(hwndDlg, IDC_STYLESLIST);
    extendedStylesListBox = GetDlgItem(hwndDlg, IDC_EXTENDEDSTYLESLIST);

    ListBox_ResetContent(stylesListBox);
    ListBox_ResetContent(extendedStylesListBox);

    if (GetWindowInfo(Context->WindowHandle, &windowInfo))
    {
        SetDlgItemText(hwndDlg, IDC_STYLES, PhaFormatString(L"0x%x", windowInfo.dwStyle)->Buffer);
        SetDlgItemText(hwndDlg, IDC_EXTENDEDSTYLES, PhaFormatString(L"0x%x", windowInfo.dwExStyle)->Buffer);

        for (i = 0; i < sizeof(WepStylePairs) / sizeof(STRING_INTEGER_PAIR); i++)
        {
            if (windowInfo.dwStyle & WepStylePairs[i].Integer)
            {
                // Skip irrelevant styles.

                if (WepStylePairs[i].Integer == WS_MAXIMIZEBOX ||
                    WepStylePairs[i].Integer == WS_MINIMIZEBOX)
                {
                    if (windowInfo.dwStyle & WS_CHILD)
                        continue;
                }

                if (WepStylePairs[i].Integer == WS_TABSTOP ||
                    WepStylePairs[i].Integer == WS_GROUP)
                {
                    if (!(windowInfo.dwStyle & WS_CHILD))
                        continue;
                }

                ListBox_AddString(stylesListBox, WepStylePairs[i].String);
            }
        }

        for (i = 0; i < sizeof(WepExtendedStylePairs) / sizeof(STRING_INTEGER_PAIR); i++)
        {
            if (windowInfo.dwExStyle & WepExtendedStylePairs[i].Integer)
            {
                ListBox_AddString(extendedStylesListBox, WepExtendedStylePairs[i].String);
            }
        }
    }
    else
    {
        SetDlgItemText(hwndDlg, IDC_STYLES, L"N/A");
        SetDlgItemText(hwndDlg, IDC_EXTENDEDSTYLES, L"N/A");
    }
}

INT_PTR CALLBACK WepWindowStylesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PWINDOW_PROPERTIES_CONTEXT context;

    if (!WepPropPageDlgProcHeader(hwndDlg, uMsg, lParam, NULL, &context))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            WepRefreshWindowStyles(hwndDlg, context);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_REFRESH:
                WepRefreshWindowStyles(hwndDlg, context);
                break;
            }
        }
        break;
    }

    return FALSE;
}

static BOOL CALLBACK EnumPropsExCallback(
    __in HWND hwnd,
    __in LPTSTR lpszString,
    __in HANDLE hData,
    __in ULONG_PTR dwData
    )
{
    INT lvItemIndex;
    PWSTR propName;
    WCHAR value[PH_PTR_STR_LEN_1];

    propName = lpszString;

    if ((ULONG_PTR)lpszString < 0x10000)
    {
        // The property has an atom name.
        propName = PhaFormatString(L"(Atom %u)", (ULONG)lpszString)->Buffer;
    }

    lvItemIndex = PhAddListViewItem((HWND)dwData, MAXINT, propName, NULL);

    PhPrintPointer(value, (PVOID)hData);
    PhSetListViewSubItem((HWND)dwData, lvItemIndex, 1, value);

    return TRUE;
}

static VOID WepRefreshWindowProps(
    __in HWND hwndDlg,
    __in HWND ListViewHandle,
    __in PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);
    EnumPropsEx(Context->WindowHandle, EnumPropsExCallback, (LPARAM)ListViewHandle);
    ExtendedListView_SortItems(ListViewHandle);
    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK WepWindowPropertiesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PWINDOW_PROPERTIES_CONTEXT context;

    if (!WepPropPageDlgProcHeader(hwndDlg, uMsg, lParam, NULL, &context))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");

            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 160, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Value");
            PhSetExtendedListView(lvHandle);

            WepRefreshWindowProps(hwndDlg, lvHandle, context);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_REFRESH:
                WepRefreshWindowProps(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST), context);
                break;
            }
        }
        break;
    }

    return FALSE;
}
