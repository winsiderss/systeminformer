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
#include <symprv.h>
#include <windowsx.h>

#define NUMBER_OF_PAGES 4
#define WEM_RESOLVE_DONE (WM_APP + 1234)

typedef struct _WINDOW_PROPERTIES_CONTEXT
{
    LONG RefCount;

    HWND ParentWindowHandle;

    HWND WindowHandle;
    CLIENT_ID ClientId;
    PH_INITONCE SymbolProviderInitOnce;
    PPH_SYMBOL_PROVIDER SymbolProvider;
    LIST_ENTRY ResolveListHead;
    PH_QUEUED_LOCK ResolveListLock;

    PPH_STRING WndProcSymbol;
    ULONG WndProcResolving;
    PPH_STRING DlgProcSymbol;
    ULONG DlgProcResolving;
    PPH_STRING ClassWndProcSymbol;
    ULONG ClassWndProcResolving;

    BOOLEAN HookDataValid;
    BOOLEAN HookDataSuccess;
    ULONG_PTR WndProc;
    ULONG_PTR DlgProc;
    WNDCLASSEX ClassInfo;
} WINDOW_PROPERTIES_CONTEXT, *PWINDOW_PROPERTIES_CONTEXT;

typedef struct _SYMBOL_RESOLVE_CONTEXT
{
    LIST_ENTRY ListEntry;
    ULONG64 Address;
    PPH_STRING Symbol;
    PH_SYMBOL_RESOLVE_LEVEL ResolveLevel;
    HWND NotifyWindow;
    PWINDOW_PROPERTIES_CONTEXT Context;
    ULONG Id;
} SYMBOL_RESOLVE_CONTEXT, *PSYMBOL_RESOLVE_CONTEXT;

typedef struct _STRING_INTEGER_PAIR
{
    PWSTR String;
    ULONG Integer;
} STRING_INTEGER_PAIR, *PSTRING_INTEGER_PAIR;

VOID WepReferenceWindowPropertiesContext(
    _Inout_ PWINDOW_PROPERTIES_CONTEXT Context
    );

VOID WepDereferenceWindowPropertiesContext(
    _Inout_ PWINDOW_PROPERTIES_CONTEXT Context
    );

HWND WepCreateWindowProperties(
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    );

INT CALLBACK WepPropSheetProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam
    );

LRESULT CALLBACK WepPropSheetWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

HPROPSHEETPAGE WepCommonCreatePage(
    _In_ PWINDOW_PROPERTIES_CONTEXT Context,
    _In_ PWSTR Template,
    _In_ DLGPROC DlgProc
    );

INT CALLBACK WepCommonPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    );

NTSTATUS WepPropertiesThreadStart(
    _In_ PVOID Parameter
    );

INT_PTR CALLBACK WepWindowGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK WepWindowStylesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK WepWindowClassDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK WepWindowPropertiesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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

static STRING_INTEGER_PAIR WepClassStylePairs[] =
{
    DEFINE_PAIR(CS_VREDRAW),
    DEFINE_PAIR(CS_HREDRAW),
    DEFINE_PAIR(CS_DBLCLKS),
    DEFINE_PAIR(CS_OWNDC),
    DEFINE_PAIR(CS_CLASSDC),
    DEFINE_PAIR(CS_PARENTDC),
    DEFINE_PAIR(CS_NOCLOSE),
    DEFINE_PAIR(CS_SAVEBITS),
    DEFINE_PAIR(CS_BYTEALIGNCLIENT),
    DEFINE_PAIR(CS_BYTEALIGNWINDOW),
    DEFINE_PAIR(CS_GLOBALCLASS),
    DEFINE_PAIR(CS_IME),
    DEFINE_PAIR(CS_DROPSHADOW)
};

HANDLE WePropertiesThreadHandle = NULL;
CLIENT_ID WePropertiesThreadClientId;
PH_EVENT WePropertiesThreadReadyEvent = PH_EVENT_INIT;
PPH_LIST WePropertiesCreateList;
PPH_LIST WePropertiesWindowList;
PH_QUEUED_LOCK WePropertiesCreateLock = PH_QUEUED_LOCK_INIT;

VOID WeShowWindowProperties(
    _In_ HWND ParentWindowHandle,
    _In_ HWND WindowHandle
    )
{
    PWINDOW_PROPERTIES_CONTEXT context;
    ULONG threadId;
    ULONG processId;

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
    context->RefCount = 1;
    context->ParentWindowHandle = ParentWindowHandle;
    context->WindowHandle = WindowHandle;

    processId = 0;
    threadId = GetWindowThreadProcessId(WindowHandle, &processId);
    context->ClientId.UniqueProcess = UlongToHandle(processId);
    context->ClientId.UniqueThread = UlongToHandle(threadId);
    PhInitializeInitOnce(&context->SymbolProviderInitOnce);
    InitializeListHead(&context->ResolveListHead);
    PhInitializeQueuedLock(&context->ResolveListLock);

    // Queue the window for creation and wake up the host thread.
    PhAcquireQueuedLockExclusive(&WePropertiesCreateLock);
    PhAddItemList(WePropertiesCreateList, context);
    PhReleaseQueuedLockExclusive(&WePropertiesCreateLock);
    PostThreadMessage(HandleToUlong(WePropertiesThreadClientId.UniqueThread), WM_NULL, 0, 0);
}

VOID WepReferenceWindowPropertiesContext(
    _Inout_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    _InterlockedIncrement(&Context->RefCount);
}

VOID WepDereferenceWindowPropertiesContext(
    _Inout_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    if (_InterlockedDecrement(&Context->RefCount) == 0)
    {
        PLIST_ENTRY listEntry;

        PhClearReference(&Context->SymbolProvider);

        // Destroy results that have not been processed by any property pages.

        listEntry = Context->ResolveListHead.Flink;

        while (listEntry != &Context->ResolveListHead)
        {
            PSYMBOL_RESOLVE_CONTEXT resolveContext;

            resolveContext = CONTAINING_RECORD(listEntry, SYMBOL_RESOLVE_CONTEXT, ListEntry);
            listEntry = listEntry->Flink;

            PhClearReference(&resolveContext->Symbol);
            PhFree(resolveContext);
        }

        PhClearReference(&Context->WndProcSymbol);
        PhClearReference(&Context->DlgProcSymbol);
        PhClearReference(&Context->ClassWndProcSymbol);

        PhFree(Context);
    }
}

static HWND WepCreateWindowProperties(
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
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
    // Class
    pages[propSheetHeader.nPages++] = WepCommonCreatePage(
        Context,
        MAKEINTRESOURCE(IDD_WNDCLASS),
        WepWindowClassDlgProc
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam
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
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
    _In_ PWINDOW_PROPERTIES_CONTEXT Context,
    _In_ PWSTR Template,
    _In_ DLGPROC DlgProc
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
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    )
{
    PWINDOW_PROPERTIES_CONTEXT context;

    context = (PWINDOW_PROPERTIES_CONTEXT)ppsp->lParam;

    if (uMsg == PSPCB_ADDREF)
        WepReferenceWindowPropertiesContext(context);
    else if (uMsg == PSPCB_RELEASE)
        WepDereferenceWindowPropertiesContext(context);

    return 1;
}

NTSTATUS WepPropertiesThreadStart(
    _In_ PVOID Parameter
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
            PhAcquireQueuedLockExclusive(&WePropertiesCreateLock);

            for (i = 0; i < WePropertiesCreateList->Count; i++)
            {
                PWINDOW_PROPERTIES_CONTEXT context;
                HWND hwnd;

                context = WePropertiesCreateList->Items[i];
                hwnd = WepCreateWindowProperties(context);
                WepDereferenceWindowPropertiesContext(context);
                PhAddItemList(WePropertiesWindowList, hwnd);
            }

            PhClearList(WePropertiesCreateList);
            PhReleaseQueuedLockExclusive(&WePropertiesCreateLock);
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam,
    _Out_opt_ LPPROPSHEETPAGE *PropSheetPage,
    _Out_opt_ PWINDOW_PROPERTIES_CONTEXT *Context
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
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    if (!Context->HookDataValid)
    {
        PWE_HOOK_SHARED_DATA data;
#ifdef _WIN64
        HANDLE processHandle;
        BOOLEAN isWow64 = FALSE;
#endif

        // The desktop window is owned by CSR. The hook will never work on the desktop window.
        if (Context->WindowHandle == GetDesktopWindow())
        {
            Context->HookDataValid = TRUE;
            return;
        }

#ifdef _WIN64
        // We can't use the hook on WOW64 processes.
        if (NT_SUCCESS(PhOpenProcess(&processHandle, *(PULONG)WeGetProcedureAddress("ProcessQueryAccess"), Context->ClientId.UniqueProcess)))
        {
            PhGetProcessIsWow64(processHandle, &isWow64);
            NtClose(processHandle);
        }

        if (isWow64)
            return;
#endif

        WeHookServerInitialization();

        Context->HookDataSuccess = FALSE;

        if (WeLockServerSharedData(&data))
        {
            if (WeSendServerRequest(Context->WindowHandle))
            {
                Context->WndProc = data->c.WndProc;
                Context->DlgProc = data->c.DlgProc;
                memcpy(&Context->ClassInfo, &data->c.ClassInfo, sizeof(WNDCLASSEX));
                Context->HookDataSuccess = TRUE;
            }

            WeUnlockServerSharedData();
        }

        Context->HookDataValid = TRUE;
    }
}

static BOOLEAN NTAPI EnumGenericModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    )
{
    PWINDOW_PROPERTIES_CONTEXT context = Context;

    PhLoadModuleSymbolProvider(context->SymbolProvider, Module->FileName->Buffer,
        (ULONG64)Module->BaseAddress, Module->Size);

    return TRUE;
}

static NTSTATUS WepResolveSymbolFunction(
    _In_ PVOID Parameter
    )
{
    PSYMBOL_RESOLVE_CONTEXT context = Parameter;

    if (PhBeginInitOnce(&context->Context->SymbolProviderInitOnce))
    {
        PhEnumGenericModules(context->Context->ClientId.UniqueProcess, NULL, 0, EnumGenericModulesCallback, context->Context);
        PhEndInitOnce(&context->Context->SymbolProviderInitOnce);
    }

    context->Symbol = PhGetSymbolFromAddress(
        context->Context->SymbolProvider,
        (ULONG64)context->Address,
        &context->ResolveLevel,
        NULL,
        NULL,
        NULL
        );

    // Fail if we don't have a symbol.
    if (!context->Symbol)
    {
        WepDereferenceWindowPropertiesContext(context->Context);
        PhFree(context);
        return STATUS_SUCCESS;
    }

    PhAcquireQueuedLockExclusive(&context->Context->ResolveListLock);
    InsertHeadList(&context->Context->ResolveListHead, &context->ListEntry);
    PhReleaseQueuedLockExclusive(&context->Context->ResolveListLock);

    PostMessage(context->NotifyWindow, WEM_RESOLVE_DONE, 0, (LPARAM)context);

    WepDereferenceWindowPropertiesContext(context->Context);

    return STATUS_SUCCESS;
}

static VOID WepQueueResolveSymbol(
    _In_ PWINDOW_PROPERTIES_CONTEXT Context,
    _In_ HWND NotifyWindow,
    _In_ ULONG64 Address,
    _In_ ULONG Id
    )
{
    PSYMBOL_RESOLVE_CONTEXT resolveContext;

    if (!Context->SymbolProvider)
    {
        Context->SymbolProvider = PhCreateSymbolProvider(Context->ClientId.UniqueProcess);
        PhLoadSymbolProviderOptions(Context->SymbolProvider);
    }

    resolveContext = PhAllocate(sizeof(SYMBOL_RESOLVE_CONTEXT));
    resolveContext->Address = Address;
    resolveContext->Symbol = NULL;
    resolveContext->ResolveLevel = PhsrlInvalid;
    resolveContext->NotifyWindow = NotifyWindow;
    resolveContext->Context = Context;
    WepReferenceWindowPropertiesContext(Context);
    resolveContext->Id = Id;

    PhQueueItemGlobalWorkQueue(WepResolveSymbolFunction, resolveContext);
}

static PPH_STRING WepFormatRect(
    _In_ PRECT Rect
    )
{
    return PhaFormatString(L"(%d, %d) - (%d, %d) [%dx%d]",
        Rect->left, Rect->top, Rect->right, Rect->bottom,
        Rect->right - Rect->left, Rect->bottom - Rect->top);
}

static VOID WepRefreshWindowGeneralInfoSymbols(
    _In_ HWND hwndDlg,
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    if (Context->WndProcResolving != 0)
        SetDlgItemText(hwndDlg, IDC_WINDOWPROC, PhaFormatString(L"0x%Ix (resolving...)", Context->WndProc)->Buffer);
    else if (Context->WndProcSymbol)
        SetDlgItemText(hwndDlg, IDC_WINDOWPROC, PhaFormatString(L"0x%Ix (%s)", Context->WndProc, Context->WndProcSymbol->Buffer)->Buffer);
    else if (Context->WndProc != 0)
        SetDlgItemText(hwndDlg, IDC_WINDOWPROC, PhaFormatString(L"0x%Ix", Context->WndProc)->Buffer);
    else
        SetDlgItemText(hwndDlg, IDC_WINDOWPROC, L"Unknown");

    if (Context->DlgProcResolving != 0)
        SetDlgItemText(hwndDlg, IDC_DIALOGPROC, PhaFormatString(L"0x%Ix (resolving...)", Context->DlgProc)->Buffer);
    else if (Context->DlgProcSymbol)
        SetDlgItemText(hwndDlg, IDC_DIALOGPROC, PhaFormatString(L"0x%Ix (%s)", Context->DlgProc, Context->DlgProcSymbol->Buffer)->Buffer);
    else if (Context->DlgProc != 0)
        SetDlgItemText(hwndDlg, IDC_DIALOGPROC, PhaFormatString(L"0x%Ix", Context->DlgProc)->Buffer);
    else if (Context->WndProc != 0)
        SetDlgItemText(hwndDlg, IDC_DIALOGPROC, L"N/A");
    else
        SetDlgItemText(hwndDlg, IDC_DIALOGPROC, L"Unknown");
}

static VOID WepRefreshWindowGeneralInfo(
    _In_ HWND hwndDlg,
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    PPH_STRING windowText;
    PPH_STRING clientIdName;
    WINDOWINFO windowInfo = { sizeof(WINDOWINFO) };
    WINDOWPLACEMENT windowPlacement = { sizeof(WINDOWPLACEMENT) };
    MONITORINFO monitorInfo = { sizeof(MONITORINFO) };

    clientIdName = PhGetClientIdName(&Context->ClientId);
    SetDlgItemText(hwndDlg, IDC_THREAD, clientIdName->Buffer);
    PhDereferenceObject(clientIdName);

    windowText = PhAutoDereferenceObject(PhGetWindowText(Context->WindowHandle));
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
    SetDlgItemText(hwndDlg, IDC_USERDATA, PhaFormatString(L"0x%Ix", GetWindowLongPtr(Context->WindowHandle, GWLP_USERDATA))->Buffer);
    SetDlgItemText(hwndDlg, IDC_UNICODE, IsWindowUnicode(Context->WindowHandle) ? L"Yes" : L"No");
    SetDlgItemText(hwndDlg, IDC_CTRLID, PhaFormatString(L"%lu", GetDlgCtrlID(Context->WindowHandle))->Buffer);

    WepEnsureHookDataValid(Context);

    if (Context->WndProc != 0)
    {
        Context->WndProcResolving++;
        WepQueueResolveSymbol(Context, hwndDlg, Context->WndProc, 1);
    }

    if (Context->DlgProc != 0)
    {
        Context->DlgProcResolving++;
        WepQueueResolveSymbol(Context, hwndDlg, Context->DlgProc, 2);
    }

    WepRefreshWindowGeneralInfoSymbols(hwndDlg, Context);
}

INT_PTR CALLBACK WepWindowGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
                PhClearReference(&context->WndProcSymbol);
                WepRefreshWindowGeneralInfo(hwndDlg, context);
                break;
            }
        }
        break;
    case WEM_RESOLVE_DONE:
        {
            PSYMBOL_RESOLVE_CONTEXT resolveContext = (PSYMBOL_RESOLVE_CONTEXT)lParam;

            if (resolveContext->Id == 1)
            {
                PhAcquireQueuedLockExclusive(&context->ResolveListLock);
                RemoveEntryList(&resolveContext->ListEntry);
                PhReleaseQueuedLockExclusive(&context->ResolveListLock);

                if (resolveContext->ResolveLevel != PhsrlModule && resolveContext->ResolveLevel != PhsrlFunction)
                    PhClearReference(&resolveContext->Symbol);

                PhMoveReference(&context->WndProcSymbol, resolveContext->Symbol);
                PhFree(resolveContext);

                context->WndProcResolving--;
            }
            else if (resolveContext->Id == 2)
            {
                PhAcquireQueuedLockExclusive(&context->ResolveListLock);
                RemoveEntryList(&resolveContext->ListEntry);
                PhReleaseQueuedLockExclusive(&context->ResolveListLock);

                if (resolveContext->ResolveLevel != PhsrlModule && resolveContext->ResolveLevel != PhsrlFunction)
                    PhClearReference(&resolveContext->Symbol);

                PhMoveReference(&context->DlgProcSymbol, resolveContext->Symbol);
                PhFree(resolveContext);

                context->DlgProcResolving--;
            }

            WepRefreshWindowGeneralInfoSymbols(hwndDlg, context);
        }
        break;
    }

    return FALSE;
}

static VOID WepRefreshWindowStyles(
    _In_ HWND hwndDlg,
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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

static VOID WepRefreshWindowClassInfoSymbols(
    _In_ HWND hwndDlg,
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    if (Context->ClassWndProcResolving != 0)
        SetDlgItemText(hwndDlg, IDC_WINDOWPROC, PhaFormatString(L"0x%Ix (resolving...)", Context->ClassInfo.lpfnWndProc)->Buffer);
    else if (Context->ClassWndProcSymbol)
        SetDlgItemText(hwndDlg, IDC_WINDOWPROC, PhaFormatString(L"0x%Ix (%s)", Context->ClassInfo.lpfnWndProc, Context->ClassWndProcSymbol->Buffer)->Buffer);
    else if (Context->ClassInfo.lpfnWndProc)
        SetDlgItemText(hwndDlg, IDC_WINDOWPROC, PhaFormatString(L"0x%Ix", Context->ClassInfo.lpfnWndProc)->Buffer);
    else
        SetDlgItemText(hwndDlg, IDC_WINDOWPROC, L"Unknown");
}

static VOID WepRefreshWindowClassInfo(
    _In_ HWND hwndDlg,
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    WCHAR className[256];
    PH_STRING_BUILDER stringBuilder;
    ULONG i;

    if (!GetClassName(Context->WindowHandle, className, sizeof(className) / sizeof(WCHAR)))
        className[0] = 0;

    WepEnsureHookDataValid(Context);

    if (!Context->HookDataSuccess)
    {
        Context->ClassInfo.cbSize = sizeof(WNDCLASSEX);
        GetClassInfoEx(NULL, className, &Context->ClassInfo);
    }

    SetDlgItemText(hwndDlg, IDC_NAME, className);
    SetDlgItemText(hwndDlg, IDC_ATOM, PhaFormatString(L"0x%x", GetClassWord(Context->WindowHandle, GCW_ATOM))->Buffer);
    SetDlgItemText(hwndDlg, IDC_INSTANCEHANDLE, PhaFormatString(L"0x%Ix", GetClassLongPtr(Context->WindowHandle, GCLP_HMODULE))->Buffer);
    SetDlgItemText(hwndDlg, IDC_ICONHANDLE, PhaFormatString(L"0x%Ix", Context->ClassInfo.hIcon)->Buffer);
    SetDlgItemText(hwndDlg, IDC_SMALLICONHANDLE, PhaFormatString(L"0x%Ix", Context->ClassInfo.hIconSm)->Buffer);
    SetDlgItemText(hwndDlg, IDC_MENUNAME, PhaFormatString(L"0x%Ix", Context->ClassInfo.lpszMenuName)->Buffer);

    PhInitializeStringBuilder(&stringBuilder, 100);
    PhAppendFormatStringBuilder(&stringBuilder, L"0x%x (", Context->ClassInfo.style);

    for (i = 0; i < sizeof(WepClassStylePairs) / sizeof(STRING_INTEGER_PAIR); i++)
    {
        if (Context->ClassInfo.style & WepClassStylePairs[i].Integer)
        {
            PhAppendStringBuilder2(&stringBuilder, WepClassStylePairs[i].String);
            PhAppendStringBuilder2(&stringBuilder, L" | ");
        }
    }

    if (PhEndsWithString2(stringBuilder.String, L" | ", FALSE))
    {
        PhRemoveEndStringBuilder(&stringBuilder, 3);
        PhAppendCharStringBuilder(&stringBuilder, ')');
    }
    else
    {
        // No styles. Remove the brackets.
        PhRemoveEndStringBuilder(&stringBuilder, 1);
    }

    SetDlgItemText(hwndDlg, IDC_STYLES, stringBuilder.String->Buffer);
    PhDeleteStringBuilder(&stringBuilder);

    // TODO: Add symbols for these values.
    SetDlgItemText(hwndDlg, IDC_CURSORHANDLE, PhaFormatString(L"0x%Ix", Context->ClassInfo.hCursor)->Buffer);
    SetDlgItemText(hwndDlg, IDC_BACKGROUNDBRUSH, PhaFormatString(L"0x%Ix", Context->ClassInfo.hbrBackground)->Buffer);

    if (Context->ClassInfo.lpfnWndProc)
    {
        Context->ClassWndProcResolving++;
        WepQueueResolveSymbol(Context, hwndDlg, (ULONG_PTR)Context->ClassInfo.lpfnWndProc, 0);
    }

    WepRefreshWindowClassInfoSymbols(hwndDlg, Context);
}

INT_PTR CALLBACK WepWindowClassDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWINDOW_PROPERTIES_CONTEXT context;

    if (!WepPropPageDlgProcHeader(hwndDlg, uMsg, lParam, NULL, &context))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            WepRefreshWindowClassInfo(hwndDlg, context);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_REFRESH:
                context->HookDataValid = FALSE;
                PhClearReference(&context->ClassWndProcSymbol);
                WepRefreshWindowClassInfo(hwndDlg, context);
                break;
            }
        }
        break;
    case WEM_RESOLVE_DONE:
        {
            PSYMBOL_RESOLVE_CONTEXT resolveContext = (PSYMBOL_RESOLVE_CONTEXT)lParam;

            PhAcquireQueuedLockExclusive(&context->ResolveListLock);
            RemoveEntryList(&resolveContext->ListEntry);
            PhReleaseQueuedLockExclusive(&context->ResolveListLock);

            if (resolveContext->ResolveLevel != PhsrlModule && resolveContext->ResolveLevel != PhsrlFunction)
                PhClearReference(&resolveContext->Symbol);

            PhMoveReference(&context->ClassWndProcSymbol, resolveContext->Symbol);
            PhFree(resolveContext);

            context->ClassWndProcResolving--;
            WepRefreshWindowClassInfoSymbols(hwndDlg, context);
        }
        break;
    }

    return FALSE;
}

static BOOL CALLBACK EnumPropsExCallback(
    _In_ HWND hwnd,
    _In_ LPTSTR lpszString,
    _In_ HANDLE hData,
    _In_ ULONG_PTR dwData
    )
{
    INT lvItemIndex;
    PWSTR propName;
    WCHAR value[PH_PTR_STR_LEN_1];

    propName = lpszString;

    if ((ULONG_PTR)lpszString < USHRT_MAX)
    {
        // This is an integer atom.
        propName = PhaFormatString(L"#%lu", (ULONG_PTR)lpszString)->Buffer;
    }

    lvItemIndex = PhAddListViewItem((HWND)dwData, MAXINT, propName, NULL);

    PhPrintPointer(value, (PVOID)hData);
    PhSetListViewSubItem((HWND)dwData, lvItemIndex, 1, value);

    return TRUE;
}

static VOID WepRefreshWindowProps(
    _In_ HWND hwndDlg,
    _In_ HWND ListViewHandle,
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);
    EnumPropsEx(Context->WindowHandle, EnumPropsExCallback, (LPARAM)ListViewHandle);
    ExtendedListView_SortItems(ListViewHandle);
    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK WepWindowPropertiesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
