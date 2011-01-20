/*
 * Process Hacker Window Explorer - 
 *   window properties
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

// Since the main message loop doesn't support property sheets, 
// we need a separate thread to run property sheets on.

#include "wndexp.h"
#include "resource.h"

typedef struct _WINDOW_PROPERTIES_CONTEXT
{
    HWND ParentWindowHandle;
    HWND WindowHandle;
} WINDOW_PROPERTIES_CONTEXT, *PWINDOW_PROPERTIES_CONTEXT;

HWND WepCreateWindowProperties(
    __in PWINDOW_PROPERTIES_CONTEXT Context
    );

NTSTATUS WepPropertiesThreadStart(
    __in PVOID Parameter
    );

INT_PTR CALLBACK WepWindowPropertiesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

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
    HWND hwnd;
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

    context = PhAllocate(sizeof(WINDOW_PROPERTIES_CONTEXT));
    memset(context, 0, sizeof(WINDOW_PROPERTIES_CONTEXT));
    context->ParentWindowHandle = ParentWindowHandle;
    context->WindowHandle = WindowHandle;

    // Queue the window for creation and wake up the host thread.
    PhAddItemList(WePropertiesCreateList, context);
    PostThreadMessage((ULONG)WePropertiesThreadClientId.UniqueThread, WM_NULL, 0, 0);
}

HWND WepCreateWindowProperties(
    __in PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[4];

    propSheetHeader.dwFlags =
        PSH_MODELESS |
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hwndParent = Context->ParentWindowHandle;
    propSheetHeader.pszCaption = PhaFormatString(L"Window %Ix", Context->WindowHandle)->Buffer;
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    // Properties page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.hInstance = PluginInstance->DllBase;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_WNDPROPS);
    propSheetPage.pfnDlgProc = WepWindowPropertiesDlgProc;
    propSheetPage.lParam = (LPARAM)Context;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    return (HWND)PropertySheet(&propSheetHeader);
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
                HWND hwnd;

                hwnd = WepCreateWindowProperties(WePropertiesCreateList->Items[i]);
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

        PhDrainAutoPool(&autoPool);

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
    }

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK WepWindowPropertiesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    return FALSE;
}
