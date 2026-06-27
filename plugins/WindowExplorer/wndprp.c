/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2018-2023
 *
 */

#include "wndexp.h"
#include <appresolver.h>
#include <settings.h>
#include <workqueue.h>
#include <symprv.h>
#include <mapldr.h>

#include <d3dkmthk.h>
#include <shellapi.h>
#include <propsys.h>
#include <propvarutil.h>

#define WEM_RESOLVE_DONE (WM_APP + 1234)
#define WE_WINDOW_PROPERTIES_CONTEXT_SLOT 0x57704f70
#define WE_WINDOW_PROPERTIES_HIGHLIGHT_TIMER 0x5770

typedef struct _SYMBOL_RESOLVE_CONTEXT
{
    LIST_ENTRY ListEntry;
    PVOID Address;
    PPH_STRING Symbol;
    PH_SYMBOL_RESOLVE_LEVEL ResolveLevel;
    HWND NotifyWindow;
    PWINDOW_PROPERTIES_CONTEXT Context;
    ULONG Id;
} SYMBOL_RESOLVE_CONTEXT, *PSYMBOL_RESOLVE_CONTEXT;

typedef struct _WINDOW_PROPERTIES_GENERAL_PAGE_CONTEXT
{
    PWINDOW_PROPERTIES_CONTEXT Parent;
    HWND ListViewHandle;
} WINDOW_PROPERTIES_GENERAL_PAGE_CONTEXT, *PWINDOW_PROPERTIES_GENERAL_PAGE_CONTEXT;

typedef struct _WINDOW_PROPERTIES_PROPLIST_PAGE_CONTEXT
{
    PWINDOW_PROPERTIES_CONTEXT Parent;
    HWND ListViewHandle;
} WINDOW_PROPERTIES_PROPLIST_PAGE_CONTEXT, *PWINDOW_PROPERTIES_PROPLIST_PAGE_CONTEXT;

typedef struct _WINDOW_PROPERTIES_PROPSTORE_PAGE_CONTEXT
{
    PWINDOW_PROPERTIES_CONTEXT Parent;
    HWND ListViewHandle;
} WINDOW_PROPERTIES_PROPSTORE_PAGE_CONTEXT, *PWINDOW_PROPERTIES_PROPSTORE_PAGE_CONTEXT;

typedef struct _WINDOW_PROPERTIES_DWMATTRIBUTES_PAGE_CONTEXT
{
    PWINDOW_PROPERTIES_CONTEXT Parent;
    HWND ListViewHandle;
} WINDOW_PROPERTIES_DWMATTRIBUTES_PAGE_CONTEXT, *PWINDOW_PROPERTIES_DWMATTRIBUTES_PAGE_CONTEXT;

typedef struct _STRING_INTEGER_PAIR
{
    PWSTR String;
    ULONG Integer;
} STRING_INTEGER_PAIR, *PSTRING_INTEGER_PAIR;

typedef enum _WINDOW_PROPERTIES_CATEGORY
{
    WINDOW_PROPERTIES_CATEGORY_GENERAL,
    WINDOW_PROPERTIES_CATEGORY_CLASS
} WINDOW_PROPERTIES_CATEGORY;

typedef enum _WINDOW_PROPERTIES_INDEX
{
    WINDOW_PROPERTIES_INDEX_APPID,
    WINDOW_PROPERTIES_INDEX_THREAD,
    WINDOW_PROPERTIES_INDEX_RECT,
    WINDOW_PROPERTIES_INDEX_NORMALRECT,
    WINDOW_PROPERTIES_INDEX_CLIENTRECT,
    WINDOW_PROPERTIES_INDEX_INSTANCE,
    WINDOW_PROPERTIES_INDEX_MENUHANDLE,
    WINDOW_PROPERTIES_INDEX_USERDATA,
    WINDOW_PROPERTIES_INDEX_UNICODE,
    WINDOW_PROPERTIES_INDEX_WNDTEXT,
    WINDOW_PROPERTIES_INDEX_WNDHANDLE,
    WINDOW_PROPERTIES_INDEX_WNDMSGONLY,
    WINDOW_PROPERTIES_INDEX_WNDEXTRA,
    WINDOW_PROPERTIES_INDEX_WNDPROC,
    WINDOW_PROPERTIES_INDEX_DLGPROC,
    WINDOW_PROPERTIES_INDEX_DLGCTLID,
    WINDOW_PROPERTIES_INDEX_FONTNAME,
    WINDOW_PROPERTIES_INDEX_STYLES,
    WINDOW_PROPERTIES_INDEX_EXSTYLES,
    WINDOW_PROPERTIES_INDEX_AUTOMATION,
    WINDOW_PROPERTIES_INDEX_DPICONTEXT,
    WINDOW_PROPERTIES_INDEX_MONITOR,
    WINDOW_PROPERTIES_INDEX_TOPLEVEL,
    WINDOW_PROPERTIES_INDEX_CLOAKED,
    WINDOW_PROPERTIES_INDEX_IAMID,
    WINDOW_PROPERTIES_INDEX_IMEWND,
    WINDOW_PROPERTIES_INDEX_D3DKMT_EXCLUSIVE,

    WINDOW_PROPERTIES_INDEX_CLASS_NAME,
    WINDOW_PROPERTIES_INDEX_CLASS_BASENAME,
    WINDOW_PROPERTIES_INDEX_CLASS_ATOM,
    WINDOW_PROPERTIES_INDEX_CLASS_STYLES,
    WINDOW_PROPERTIES_INDEX_CLASS_INSTANCE,
    WINDOW_PROPERTIES_INDEX_CLASS_LARGEICON,
    WINDOW_PROPERTIES_INDEX_CLASS_SMALLICON,
    WINDOW_PROPERTIES_INDEX_CLASS_CURSOR,
    WINDOW_PROPERTIES_INDEX_CLASS_BACKBRUSH,
    WINDOW_PROPERTIES_INDEX_CLASS_MENUNAME,
    WINDOW_PROPERTIES_INDEX_CLASS_WNDEXTRA,
    WINDOW_PROPERTIES_INDEX_CLASS_WNDPROC,
} WINDOW_PROPERTIES_INDEX;

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS WepPropertiesThreadStart(
    _In_ PVOID Parameter
    );

INT_PTR CALLBACK WepWindowGeneralDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK WepWindowPropListDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK WepWindowPropStoreDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK WepWindowAttributeDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID NTAPI WepWindowPropertiesSheetInitialized(
    _In_ HWND HostHandle,
    _In_opt_ PVOID Context
    );

LRESULT CALLBACK WepWindowPropertiesSheetWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID WepShowWindowPropertiesOptionsMenu(
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    );

VOID WepLayoutWindowPropertiesOptionsButton(
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    );

#define DEFINE_PAIR(Symbol) { TEXT(#Symbol), Symbol }

#define WS_EX_DRAGDETECT 0x00000002
#define WS_EX_VISIBLEWHENOTGHOSTED 0x00000800
#define WS_EX_FORCELEGACYRESIZENC 0x00800000
#define WS_EX_UISTATEACTIVE 0x04000000
#define WS_EX_REDIRECTED 0x20000000
#define WS_EX_UISTATEACCELHIDDEN 0x40000000
#define WS_EX_UISTATEFOCUSHIDDEN 0x80000000
#define WS_EX_SETANSICREATOR 0x80000000

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
    DEFINE_PAIR(WS_EX_DLGMODALFRAME),       // 0x1
    DEFINE_PAIR(WS_EX_DRAGDETECT),          // 0x2
    DEFINE_PAIR(WS_EX_NOPARENTNOTIFY),      // 0x4
    DEFINE_PAIR(WS_EX_TOPMOST),             // 0x8
    DEFINE_PAIR(WS_EX_ACCEPTFILES),         // 0x10
    DEFINE_PAIR(WS_EX_TRANSPARENT),         // 0x20
    DEFINE_PAIR(WS_EX_MDICHILD),            // 0x40
    DEFINE_PAIR(WS_EX_TOOLWINDOW),          // 0x80
    DEFINE_PAIR(WS_EX_WINDOWEDGE),          // 0x100
    DEFINE_PAIR(WS_EX_PALETTEWINDOW),       // 0x188
    DEFINE_PAIR(WS_EX_CLIENTEDGE),          // 0x200
    DEFINE_PAIR(WS_EX_OVERLAPPEDWINDOW),    // 0x300
    DEFINE_PAIR(WS_EX_CONTEXTHELP),         // 0x400
    DEFINE_PAIR(WS_EX_VISIBLEWHENOTGHOSTED),// 0x800
    DEFINE_PAIR(WS_EX_RIGHT),               // 0x1000
    DEFINE_PAIR(WS_EX_RTLREADING),          // 0x2000
    DEFINE_PAIR(WS_EX_LEFTSCROLLBAR),       // 0x4000
    DEFINE_PAIR(WS_EX_CONTROLPARENT),       // 0x10000
    DEFINE_PAIR(WS_EX_STATICEDGE),          // 0x20000
    DEFINE_PAIR(WS_EX_APPWINDOW),           // 0x40000
    DEFINE_PAIR(WS_EX_LAYERED),             // 0x80000
    DEFINE_PAIR(WS_EX_NOINHERITLAYOUT),     // 0x100000
    DEFINE_PAIR(WS_EX_NOREDIRECTIONBITMAP), // 0x200000
    DEFINE_PAIR(WS_EX_LAYOUTRTL),           // 0x400000
    DEFINE_PAIR(WS_EX_FORCELEGACYRESIZENC), // 0x800000
    DEFINE_PAIR(WS_EX_COMPOSITED),          // 0x2000000
    DEFINE_PAIR(WS_EX_UISTATEACTIVE),       // 0x4000000
    DEFINE_PAIR(WS_EX_NOACTIVATE),          // 0x8000000
    DEFINE_PAIR(WS_EX_REDIRECTED),          // 0x20000000
    DEFINE_PAIR(WS_EX_UISTATEACCELHIDDEN),  // 0x40000000
    DEFINE_PAIR(WS_EX_UISTATEFOCUSHIDDEN),  // 0x80000000
};

static STRING_INTEGER_PAIR WepClassStylePairs[] =
{
    DEFINE_PAIR(CS_VREDRAW),              // 0x1
    DEFINE_PAIR(CS_HREDRAW),              // 0x2
    DEFINE_PAIR(CS_DBLCLKS),              // 0x8
    DEFINE_PAIR(CS_OWNDC),
    DEFINE_PAIR(CS_CLASSDC),
    DEFINE_PAIR(CS_PARENTDC),
    DEFINE_PAIR(CS_NOCLOSE),              // 0x200
    DEFINE_PAIR(CS_SAVEBITS),             // 0x800
    DEFINE_PAIR(CS_BYTEALIGNCLIENT),
    DEFINE_PAIR(CS_BYTEALIGNWINDOW),
    DEFINE_PAIR(CS_GLOBALCLASS),
    DEFINE_PAIR(CS_IME),
    DEFINE_PAIR(CS_DROPSHADOW)
};

PPH_OBJECT_TYPE WeWindowItemType = NULL;
PPH_OBJECT_TYPE WeWindowPropertiesPageContextType = NULL;

PVOID WepCreateWindowPropertiesPageContext(
    _In_ SIZE_T Size,
    _In_ PWINDOW_PROPERTIES_CONTEXT Parent
    )
{
    PWINDOW_PROPERTIES_GENERAL_PAGE_CONTEXT context;

    if (!WeWindowPropertiesPageContextType)
    {
        WeWindowPropertiesPageContextType = PhCreateObjectType(L"WindowPropertiesPageContextType", 0, NULL);
    }

    context = PhCreateObjectZero(Size, WeWindowPropertiesPageContextType);
    context->Parent = Parent;

    return context;
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI WeWindowPropertiesItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PWINDOW_PROPERTIES_CONTEXT context = Object;

    PLIST_ENTRY listEntry;

    PhClearReference(&context->SymbolProvider);

    // Destroy results that have not been processed by any property pages.

    listEntry = context->ResolveListHead.Flink;

    while (listEntry != &context->ResolveListHead)
    {
        PSYMBOL_RESOLVE_CONTEXT resolveContext;

        resolveContext = CONTAINING_RECORD(listEntry, SYMBOL_RESOLVE_CONTEXT, ListEntry);
        listEntry = listEntry->Flink;

        PhClearReference(&resolveContext->Symbol);
        PhFree(resolveContext);
    }

    PhClearReference(&context->WndProcSymbol);
    PhClearReference(&context->DlgProcSymbol);
    PhClearReference(&context->ClassWndProcSymbol);

    if (context->TreeWindowFont)
        DeleteFont(context->TreeWindowFont);
}

BOOLEAN WeShowWindowProperties(
    _In_ HWND ParentWindowHandle,
    _In_ HWND WindowHandle
    )
{
    PWINDOW_PROPERTIES_CONTEXT context;
    NTSTATUS status;
    CLIENT_ID clientId;

    if (!WeWindowItemType)
    {
        WeWindowItemType = PhCreateObjectType(L"WindowPropertiesItemType", 0, WeWindowPropertiesItemDeleteProcedure);
    }

    if (!IsWindow(WindowHandle))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to display window properties.", STATUS_GRAPHICS_PRESENT_INVALID_WINDOW, 0);
        return FALSE;
    }

    status = PhGetWindowClientId(WindowHandle, &clientId);

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to display window properties.", status, 0);
        return FALSE;
    }

    //PhQueryWindowRealProcess(WindowHandle);
    //if (
    //    ClientId->UniqueThread != clientId.UniqueThread ||
    //    ClientId->UniqueProcess != clientId.UniqueProcess
    //    )
    //{
    //    PhShowStatus(ParentWindowHandle, L"Unable to display window properties.", STATUS_GRAPHICS_PRESENT_INVALID_WINDOW, 0);
    //    return FALSE;
    //}

    context = PhCreateObjectZero(sizeof(WINDOW_PROPERTIES_CONTEXT), WeWindowItemType);
    context->WindowHandle = WindowHandle;
    context->ParentWindowHandle = ParentWindowHandle;
    context->ClientId.UniqueProcess = clientId.UniqueProcess;
    context->ClientId.UniqueThread = clientId.UniqueThread;
    context->TreeWindowFont = PhCreateTreeWindowFont(PhGetWindowDpi(ParentWindowHandle));

    PhInitializeInitOnce(&context->SymbolProviderInitOnce);
    InitializeListHead(&context->ResolveListHead);
    PhInitializeQueuedLock(&context->ResolveListLock);

    PhCreateThread2(WepPropertiesThreadStart, context);
    return TRUE;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS WepPropertiesThreadStart(
    _In_ PVOID Parameter
    )
{
    PWINDOW_PROPERTIES_CONTEXT context = Parameter;
    PPV_PROPCONTEXT propContext;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    if (propContext = HdCreatePropContext(
        PhaFormatString(L"Window %Ix", (ULONG_PTR)context->WindowHandle)->Buffer,
        context,
        WepWindowPropertiesSheetInitialized
        ))
    {
        PPV_PROPPAGECONTEXT newPage;

        // General
        newPage = PvCreatePropPageContext(
            L"General",
            L"General",
            MAKEINTRESOURCE(IDD_WNDGENERAL),
            WepWindowGeneralDlgProc,
            WepCreateWindowPropertiesPageContext(sizeof(WINDOW_PROPERTIES_GENERAL_PAGE_CONTEXT), context));
        PvAddPropPage(propContext, newPage);

        // Properties
        newPage = PvCreatePropPageContext(
            L"Properties",
            L"Properties",
            MAKEINTRESOURCE(IDD_WNDPROPLIST),
            WepWindowPropListDlgProc,
            WepCreateWindowPropertiesPageContext(sizeof(WINDOW_PROPERTIES_PROPLIST_PAGE_CONTEXT), context));
        PvAddPropPage(propContext, newPage);

        // Property store
        newPage = PvCreatePropPageContext(
            L"PropertyStore",
            L"Property Store",
            MAKEINTRESOURCE(IDD_WNDPROPSTORAGE),
            WepWindowPropStoreDlgProc,
            WepCreateWindowPropertiesPageContext(sizeof(WINDOW_PROPERTIES_PROPSTORE_PAGE_CONTEXT), context));
        PvAddPropPage(propContext, newPage);
        
        // DWM attributes
        newPage = PvCreatePropPageContext(
            L"DwmAttributes",
            L"DWM Attributes",
            MAKEINTRESOURCE(IDD_WNDPROPSTORAGE),
            WepWindowAttributeDlgProc,
            WepCreateWindowPropertiesPageContext(sizeof(WINDOW_PROPERTIES_DWMATTRIBUTES_PAGE_CONTEXT), context));
        PvAddPropPage(propContext, newPage);

        // Preview page
        //newPage = PvCreatePropPageContext(
        //    MAKEINTRESOURCE(IDD_WNDPREVIEW),
        //    WepWindowPreviewDlgProc,
        //    context);
        //PvAddPropPage(propContext, newPage);

        PhPropSheetNewBuilderShowModal(propContext->Builder);
        PhDereferenceObject(propContext);
    }

    PhDeleteAutoPool(&autoPool);

    PhDereferenceObject(context);

    return STATUS_SUCCESS;
}

VOID WepLayoutWindowPropertiesOptionsButton(
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    RECT clientRect;
    LONG dpiValue;
    LONG padding;
    LONG buttonWidth;
    LONG buttonHeight;

    if (!Context->PropertySheetHandle || !Context->OptionsButtonHandle)
        return;

    dpiValue = PhGetWindowDpi(Context->PropertySheetHandle);
    padding = MulDiv(8, dpiValue, USER_DEFAULT_SCREEN_DPI);
    buttonWidth = MulDiv(75, dpiValue, USER_DEFAULT_SCREEN_DPI);
    buttonHeight = MulDiv(23, dpiValue, USER_DEFAULT_SCREEN_DPI);
    GetClientRect(Context->PropertySheetHandle, &clientRect);

    SetWindowPos(
        Context->OptionsButtonHandle,
        NULL,
        padding,
        clientRect.bottom - padding - buttonHeight,
        buttonWidth,
        buttonHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
}

VOID WepShowWindowPropertiesOptionsMenu(
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    RECT rect;
    PPH_EMENU menu;
    PPH_EMENU_ITEM selectedItem;

    if (!IsWindow(Context->WindowHandle))
        return;

    menu = PhCreateEMenu();
    WepCreateWindowMenu(menu, FALSE, FALSE);
    WepSetWindowMenuState(menu, Context->WindowHandle);
    GetWindowRect(Context->OptionsButtonHandle, &rect);

    selectedItem = PhShowEMenu(
        menu,
        Context->PropertySheetHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_BOTTOM,
        rect.left,
        rect.top
        );

    if (selectedItem && selectedItem->Id != ULONG_MAX)
    {
        if (selectedItem->Id == ID_WINDOW_HIGHLIGHT)
        {
            if (Context->HighlightingWindow && (Context->HighlightingWindowCount & 1))
                WeInvertWindowBorder(Context->HighlightingWindow);

            Context->HighlightingWindow = Context->WindowHandle;
            Context->HighlightingWindowCount = 10;
            SetTimer(Context->PropertySheetHandle, WE_WINDOW_PROPERTIES_HIGHLIGHT_TIMER, 100, NULL);
        }
        else
        {
            WepExecuteWindowCommand(Context->PropertySheetHandle, Context->WindowHandle, selectedItem->Id);
        }
    }

    PhDestroyEMenu(menu);
}

VOID NTAPI WepWindowPropertiesSheetInitialized(
    _In_ HWND HostHandle,
    _In_opt_ PVOID Context
    )
{
    PWINDOW_PROPERTIES_CONTEXT context = Context;
    HWND closeButtonHandle;
    HFONT fontHandle;

    context->PropertySheetHandle = HostHandle;
    context->OptionsButtonHandle = PhCreateWindow(
        WC_BUTTON,
        L"Options",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0,
        0,
        0,
        0,
        HostHandle,
        (HMENU)IDC_OPTIONS,
        PluginInstance->DllBase,
        NULL
        );

    closeButtonHandle = GetDlgItem(HostHandle, IDCANCEL);
    fontHandle = (HFONT)SendMessage(closeButtonHandle, WM_GETFONT, 0, 0);

    if (fontHandle)
        SetWindowFont(context->OptionsButtonHandle, fontHandle, FALSE);

    WepLayoutWindowPropertiesOptionsButton(context);
    PhSetWindowContext(HostHandle, WE_WINDOW_PROPERTIES_CONTEXT_SLOT, context);
    context->PropertySheetWindowProcedure = PhGetWindowProcedure(HostHandle);
    PhSetWindowProcedure(HostHandle, WepWindowPropertiesSheetWndProc);
}

LRESULT CALLBACK WepWindowPropertiesSheetWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWINDOW_PROPERTIES_CONTEXT context;
    WNDPROC oldWindowProcedure;

    context = PhGetWindowContext(WindowHandle, WE_WINDOW_PROPERTIES_CONTEXT_SLOT);

    if (!context)
        return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);

    oldWindowProcedure = context->PropertySheetWindowProcedure;

    switch (WindowMessage)
    {
    case WM_COMMAND:
        if (GET_WM_COMMAND_HWND(wParam, lParam) == context->OptionsButtonHandle)
        {
            WepShowWindowPropertiesOptionsMenu(context);
            return 0;
        }
        break;
    case WM_TIMER:
        if (wParam == WE_WINDOW_PROPERTIES_HIGHLIGHT_TIMER && context->HighlightingWindowCount != 0)
        {
            if (!IsWindow(context->HighlightingWindow))
            {
                KillTimer(WindowHandle, WE_WINDOW_PROPERTIES_HIGHLIGHT_TIMER);
                context->HighlightingWindow = NULL;
                context->HighlightingWindowCount = 0;
                return 0;
            }

            WeInvertWindowBorder(context->HighlightingWindow);

            if (--context->HighlightingWindowCount == 0)
            {
                KillTimer(WindowHandle, WE_WINDOW_PROPERTIES_HIGHLIGHT_TIMER);
                context->HighlightingWindow = NULL;
            }

            return 0;
        }
        break;
    case WM_SIZE:
    case WM_DPICHANGED:
        {
            LRESULT result = CallWindowProc(oldWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);

            WepLayoutWindowPropertiesOptionsButton(context);
            return result;
        }
    case WM_NCDESTROY:
        {
            if (context->HighlightingWindow && (context->HighlightingWindowCount & 1))
                WeInvertWindowBorder(context->HighlightingWindow);

            KillTimer(WindowHandle, WE_WINDOW_PROPERTIES_HIGHLIGHT_TIMER);
            PhRemoveWindowContext(WindowHandle, WE_WINDOW_PROPERTIES_CONTEXT_SLOT);
            PhSetWindowProcedure(WindowHandle, oldWindowProcedure);
            context->PropertySheetHandle = NULL;
            context->OptionsButtonHandle = NULL;
            context->PropertySheetWindowProcedure = NULL;
            return CallWindowProc(oldWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);
        }
    }

    return CallWindowProc(oldWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS WepResolveSymbolFunction(
    _In_ PVOID Parameter
    )
{
    PSYMBOL_RESOLVE_CONTEXT context = Parameter;

    if (PhBeginInitOnce(&context->Context->SymbolProviderInitOnce))
    {
        PhLoadSymbolProviderOptions(context->Context->SymbolProvider);
        PhLoadModulesForVirtualSymbolProvider(context->Context->SymbolProvider, context->Context->ClientId.UniqueProcess, NULL);
        PhEndInitOnce(&context->Context->SymbolProviderInitOnce);
    }

    context->Symbol = PhGetSymbolFromAddress(
        context->Context->SymbolProvider,
        context->Address,
        &context->ResolveLevel,
        NULL,
        NULL,
        NULL
        );

    // Fail if we don't have a symbol.
    if (!context->Symbol)
    {
        PhDereferenceObject(context->Context);
        PhFree(context);
        return STATUS_SUCCESS;
    }

    PhAcquireQueuedLockExclusive(&context->Context->ResolveListLock);
    InsertHeadListNoFence(&context->Context->ResolveListHead, &context->ListEntry);
    PhReleaseQueuedLockExclusive(&context->Context->ResolveListLock);

    PostMessage(context->NotifyWindow, WEM_RESOLVE_DONE, 0, (LPARAM)context);

    PhDereferenceObject(context->Context);
    return STATUS_SUCCESS;
}

VOID WepQueueResolveSymbol(
    _In_ PWINDOW_PROPERTIES_CONTEXT Context,
    _In_ HWND NotifyWindow,
    _In_ PVOID Address,
    _In_ ULONG Id
    )
{
    PSYMBOL_RESOLVE_CONTEXT resolveContext;

    if (!Context->SymbolProvider)
    {
        Context->SymbolProvider = PhCreateSymbolProvider(NULL);
    }

    PhReferenceObject(Context);

    resolveContext = PhAllocateZero(sizeof(SYMBOL_RESOLVE_CONTEXT));
    resolveContext->Address = Address;
    resolveContext->Symbol = NULL;
    resolveContext->ResolveLevel = PhsrlInvalid;
    resolveContext->NotifyWindow = NotifyWindow;
    resolveContext->Context = Context;
    resolveContext->Id = Id;

    PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), WepResolveSymbolFunction, resolveContext);
}

VOID PhD3DKMTQueryVidPnExclusiveOwnership(
    _In_ HWND ListViewHandle,
    _In_ HANDLE ProcessHandle,
    _In_ HWND WindowHandle
    )
{
    D3DKMT_QUERYVIDPNEXCLUSIVEOWNERSHIP queryInfo;

    memset(&queryInfo, 0, sizeof(D3DKMT_QUERYVIDPNEXCLUSIVEOWNERSHIP));
    queryInfo.hProcess = ProcessHandle;
    queryInfo.hWindow = WindowHandle;

    if (NT_SUCCESS(PhQueryDirectXExclusiveOwnership(&queryInfo)))
    {
        PWSTR ownerTypeString = L"Unknown";

        switch (queryInfo.OwnerType)
        {
        case D3DKMT_VIDPNSOURCEOWNER_UNOWNED:
            ownerTypeString = L"Unowned";
            break;
        case D3DKMT_VIDPNSOURCEOWNER_SHARED:
            ownerTypeString = L"Shared";
            break;
        case D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE:
            ownerTypeString = L"Exclusive";
            break;
        case D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVEGDI:
            ownerTypeString = L"Exclusive (GDI)";
            break;
        case D3DKMT_VIDPNSOURCEOWNER_EMULATED:
            ownerTypeString = L"Emulated";
            break;
        }

        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_D3DKMT_EXCLUSIVE, 1, PhaFormatString(
            L"%s (Source: %u, LUID: %08x-%08x)",
            ownerTypeString,
            queryInfo.VidPnSourceId,
            queryInfo.AdapterLuid.HighPart,
            queryInfo.AdapterLuid.LowPart
            )->Buffer);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_D3DKMT_EXCLUSIVE, 1, L"N/A");
    }
}


typedef enum _WINDOW_BAND
{
    ZBID_DEFAULT = 0,
    ZBID_DESKTOP = 1,
    ZBID_UIACCESS = 2,
    ZBID_IMMERSIVE_IHM = 3,
    ZBID_IMMERSIVE_NOTIFICATION = 4,
    ZBID_IMMERSIVE_APPCHROME = 5,
    ZBID_IMMERSIVE_MOGO = 6,
    ZBID_IMMERSIVE_EDGY = 7,
    ZBID_IMMERSIVE_INACTIVEMOBODY = 8,
    ZBID_IMMERSIVE_INACTIVEDOCK = 9,
    ZBID_IMMERSIVE_ACTIVEMOBODY = 10,
    ZBID_IMMERSIVE_ACTIVEDOCK = 11,
    ZBID_IMMERSIVE_BACKGROUND = 12,
    ZBID_IMMERSIVE_SEARCH = 13,
    ZBID_GENUINE_WINDOWS = 14,
    ZBID_IMMERSIVE_RESTRICTED = 15,
    ZBID_SYSTEM_TOOLS = 16,
    ZBID_LOCK = 17,
    ZBID_ABOVELOCK_UX = 18,
} WINDOW_BAND;

PPH_STRING WepFormatRect(
    _In_ PRECT Rect
    )
{
    return PhaFormatString(L"(%ld, %ld) - (%ld, %ld) [%ldx%ld]",
        Rect->left, Rect->top, Rect->right, Rect->bottom,
        Rect->right - Rect->left, Rect->bottom - Rect->top);
}

VOID WepRefreshWindowGeneralInfoSymbols(
    _In_ HWND ListViewHandle,
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    if (Context->WndProcResolving != 0)
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_WNDPROC, 1, PhaFormatString(L"0x%Ix (resolving...)", Context->WndProc)->Buffer);
    else if (Context->WndProcSymbol)
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_WNDPROC, 1, PhaFormatString(L"0x%Ix (%s)", Context->WndProc, Context->WndProcSymbol->Buffer)->Buffer);
    else if (Context->WndProc != 0)
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_WNDPROC, 1, PhaFormatString(L"0x%Ix", Context->WndProc)->Buffer);
    else
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_WNDPROC, 1, L"Unknown");

    if (Context->DlgProcResolving != 0)
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_DLGPROC, 1, PhaFormatString(L"0x%Ix (resolving...)", Context->DlgProc)->Buffer);
    else if (Context->DlgProcSymbol)
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_DLGPROC, 1, PhaFormatString(L"0x%Ix (%s)", Context->DlgProc, Context->DlgProcSymbol->Buffer)->Buffer);
    else if (Context->DlgProc != 0)
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_DLGPROC, 1, PhaFormatString(L"0x%Ix", Context->DlgProc)->Buffer);
    else if (Context->WndProc != 0)
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_DLGPROC, 1, L"N/A");
    else
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_DLGPROC, 1, L"Unknown");
}

VOID WepRefreshWindowGeneralInfo(
    _In_ HWND WindowHandle,
    _In_ HWND ListViewHandle,
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    WINDOWINFO windowInfo = { sizeof(WINDOWINFO) };
    WINDOWPLACEMENT windowPlacement = { sizeof(WINDOWPLACEMENT) };
    MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
    HANDLE processHandle;
    PPH_STRING fileName = NULL;
    PPH_STRING appIdText;
    HMENU menuHandle = NULL;
    PVOID instanceHandle;
    PVOID userdataHandle;
    ULONG windowId;
    ULONG windowExtra;

    //PWND window;

    //menuHandle = GetMenu(Context->WindowHandle);

    //// Fast path: read the fields directly from the mapped window object when available.
    //if (window = PhValidateHwnd(Context->WindowHandle))
    //{
    //    instanceHandle = (PVOID)window->InstanceHandle;
    //    userdataHandle = (PVOID)window->UserData;
    //    windowId = (ULONG)window->WindowIdOrParent;
    //    windowExtra = window->ExtraDataSize;
    //}
    //else
    {
        instanceHandle = (PVOID)GetWindowLongPtr(Context->WindowHandle, GWLP_HINSTANCE);
        userdataHandle = (PVOID)GetWindowLongPtr(Context->WindowHandle, GWLP_USERDATA);
        windowId = (ULONG)GetWindowLongPtr(Context->WindowHandle, GWLP_ID);
        windowExtra = (ULONG)GetClassLongPtr(Context->WindowHandle, GCL_CBWNDEXTRA); // GetWindowLongPtr
    }
    // TODO: GetWindowLongPtr(Context->WindowHandle, GCLP_WNDPROC);

    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_THREAD, 1, PH_AUTO_T(PH_STRING, PhGetClientIdName(&Context->ClientId))->Buffer);

    if (GetWindowInfo(Context->WindowHandle, &windowInfo))
    {
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_RECT, 1, WepFormatRect(&windowInfo.rcWindow)->Buffer);
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLIENTRECT, 1, WepFormatRect(&windowInfo.rcClient)->Buffer);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_RECT, 1, L"N/A");
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLIENTRECT, 1, L"N/A");
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

        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_NORMALRECT, 1, WepFormatRect(&windowPlacement.rcNormalPosition)->Buffer);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_NORMALRECT, 1, L"N/A");
    }

    if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, Context->ClientId.UniqueProcess)))
    {
        if (NT_SUCCESS(PhGetProcessMappedFileName(processHandle, instanceHandle, &fileName)))
        {
            PhMoveReference(&fileName, PhGetFileName(fileName));
            PhMoveReference(&fileName, PhGetBaseName(fileName));
        }

        NtClose(processHandle);
    }

    if (fileName)
    {
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_INSTANCE, 1, PhaFormatString(
            L"0x%Ix (%s)",
            (ULONG_PTR)instanceHandle,
            PhGetStringOrEmpty(fileName)
            )->Buffer);
        PhDereferenceObject(fileName);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_INSTANCE, 1, PhaFormatString(
            L"0x%Ix",
            (ULONG_PTR)instanceHandle
            )->Buffer);
    }

    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_MENUHANDLE, 1, PhaFormatString(L"0x%Ix", (ULONG_PTR)menuHandle)->Buffer);
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_USERDATA, 1, PhaFormatString(L"0x%Ix", (ULONG_PTR)userdataHandle)->Buffer);
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_UNICODE, 1, IsWindowUnicode(Context->WindowHandle) ? L"Yes" : L"No");
    //PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_WNDTEXT, 1, Context->MessageOnlyWindow ? L"N/A" : PhGetStringOrEmpty(PH_AUTO(PhGetWindowText(Context->WindowHandle))));
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_WNDHANDLE, 1, PhaFormatString(L"0x%Ix", (ULONG_PTR)Context->WindowHandle)->Buffer);
    //PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_WNDMSGONLY, 1, Context->MessageOnlyWindow ? L"Yes" : L"No");
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_WNDEXTRA, 1, PhaFormatString(L"%lu bytes (%s) (%s)", windowExtra, PhaFormatSize(windowExtra, ULONG_MAX)->Buffer, WeHashWindowExtraBytes(Context->WindowHandle)->Buffer)->Buffer);
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_DLGCTLID, 1, PhaFormatString(L"%lu (0x%x)", windowId, windowId)->Buffer);

    //if (Context->MessageOnlyWindow)
    //{
    //    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_FONTNAME, 1, L"N/A");
    //}
    //else
    {
        ULONG_PTR result;

        if (SendMessageTimeout(Context->WindowHandle, WM_GETFONT, 0, 0, SMTO_ABORTIFHUNG, 1000, &result) != 0)
        {
            LOGFONT logFont;

            if (GetObject((HFONT)result, sizeof(LOGFONT), &logFont))
                PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_FONTNAME, 1, logFont.lfFaceName);
            else
                PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_FONTNAME, 1, L"N/A");
        }
        else
        {
            PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_FONTNAME, 1, L"N/A");
        }
    }

    //ULONG version;
    //if (SendMessageTimeout(Context->WindowHandle, CCM_GETVERSION, 0, 0, SMTO_ABORTIFHUNG, 5000, &version))
    //WepQueryProcessWndProc(Context);

    if (Context->WndProc != 0)
    {
        Context->WndProcResolving++;
        WepQueueResolveSymbol(Context, WindowHandle, (PVOID)Context->WndProc, 1);
    }

    if (Context->DlgProc != 0)
    {
        Context->DlgProcResolving++;
        WepQueueResolveSymbol(Context, WindowHandle, (PVOID)Context->DlgProc, 2);
    }

    WepRefreshWindowGeneralInfoSymbols(ListViewHandle, Context);

    if (HR_SUCCESS(PhAppResolverGetAppIdForWindow(Context->WindowHandle, &appIdText)))
    {
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_APPID, 1, PhGetString(appIdText));
        PhDereferenceObject(appIdText);
    }

    // Monitor
    {
        typedef struct _MONITORINFOEX2
        {
            MONITORINFO MonitorInfo;
            WCHAR Device[CCHDEVICENAME];
        } MONITORINFOEX2, *PMONITORINFOEX2;

        HMONITOR monitorHandle;

        if (monitorHandle = MonitorFromWindow(Context->WindowHandle, MONITOR_DEFAULTTONULL))
        {
            MONITORINFOEX2 monitorInfoEx;

            memset(&monitorInfoEx, 0, sizeof(MONITORINFOEX2));
            monitorInfoEx.MonitorInfo.cbSize = sizeof(MONITORINFOEX2);

            if (GetMonitorInfo(monitorHandle, (LPMONITORINFO)&monitorInfoEx))
            {
                _wcslwr(monitorInfoEx.Device);
                PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_MONITOR, 1, monitorInfoEx.Device);
            }
        }
    }

    // IsTopLevelWindow
    {
        if (WeIsTopLevelWindow(Context->WindowHandle))
        {
            PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_TOPLEVEL, 1, L"Yes");
        }
        else
        {
            PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_TOPLEVEL, 1, L"No");
        }
    }

    // Cloaked
    {
        if (WeIsWindowCloaked(Context->WindowHandle))
        {
            PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLOAKED, 1, L"Yes");
        }
        else
        {
            PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLOAKED, 1, L"No");
        }
    }

    // Identity and Access Management (IAM) aka Window Bands
    {
        ULONG bandId = WeGetWindowBand(Context->WindowHandle);

        if (bandId != ULONG_MAX)
        {
            PWSTR string = L"";
            WCHAR value[PH_INT64_STR_LEN_1];

            switch (bandId)
            {
            case ZBID_DEFAULT:
                string = L"Default";
                break;
            case ZBID_DESKTOP:
                string = L"Desktop";
                break;
            case ZBID_UIACCESS:
                string = L"UIAccess";
                break;
            case ZBID_IMMERSIVE_IHM:
                string = L"IHM";
                break;
            case ZBID_IMMERSIVE_NOTIFICATION:
                string = L"Notification";
                break;
            case ZBID_IMMERSIVE_APPCHROME:
                string = L"IMMERSIVE_APPCHROME";
                break;
            case ZBID_IMMERSIVE_MOGO:
                string = L"IMMERSIVE_MOGO";
                break;
            case ZBID_IMMERSIVE_EDGY:
                string = L"IMMERSIVE_EDGY";
                break;
            case ZBID_IMMERSIVE_INACTIVEMOBODY:
                string = L"IMMERSIVE_INACTIVEMOBODY";
                break;
            case ZBID_IMMERSIVE_INACTIVEDOCK:
                string = L"IMMERSIVE_INACTIVEDOCK";
                break;
            case ZBID_IMMERSIVE_ACTIVEMOBODY:
                string = L"IMMERSIVE_ACTIVEMOBODY";
                break;
            case ZBID_IMMERSIVE_ACTIVEDOCK:
                string = L"IMMERSIVE_ACTIVEDOCK";
                break;
            case ZBID_IMMERSIVE_BACKGROUND:
                string = L"IMMERSIVE_BACKGROUND";
                break;
            case ZBID_IMMERSIVE_SEARCH:
                string = L"IMMERSIVE_SEARCH";
                break;
            case ZBID_GENUINE_WINDOWS:
                string = L"GENUINE_WINDOWS";
                break;
            case ZBID_IMMERSIVE_RESTRICTED:
                string = L"IMMERSIVE_RESTRICTED";
                break;
            case ZBID_SYSTEM_TOOLS:
                string = L"SYSTEM_TOOLS";
                break;
            case ZBID_LOCK:
                string = L"Lock";
                break;
            case ZBID_ABOVELOCK_UX:
                string = L"Above-Lock UX";
                break;
            default:
                string = L"[MISSING]";
                break;
            }

            PhPrintPointer(value, UlongToPtr(bandId));
            PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_IAMID, 1, PhaFormatString(
                L"%s (%s)",
                string,
                value
                )->Buffer);
        }
        else
        {
            PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_IAMID, 1, L"N/A");
        }
    }

    // IME
    {
        ULONG_PTR defaultIMEWindow = PhUserQueryWindow(Context->WindowHandle, WindowDefaultImeWindow);

        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_IMEWND, 1, PhaFormatString(
            L"0x%Ix",
            (ULONG_PTR)defaultIMEWindow
            )->Buffer);
    }
}

VOID WepRefreshWindowStyles(
    _In_ HWND ListViewHandle,
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    WINDOWINFO windowInfo = { sizeof(WINDOWINFO) };
    PH_STRING_BUILDER styleStringBuilder;
    PH_STRING_BUILDER styleExStringBuilder;
    ULONG i;

    if (GetWindowInfo(Context->WindowHandle, &windowInfo))
    {
        PhInitializeStringBuilder(&styleStringBuilder, 100);
        PhInitializeStringBuilder(&styleExStringBuilder, 100);

        PhAppendFormatStringBuilder(&styleStringBuilder, L"0x%x (", windowInfo.dwStyle);
        PhAppendFormatStringBuilder(&styleExStringBuilder, L"0x%x (", windowInfo.dwExStyle);

        for (i = 0; i < RTL_NUMBER_OF(WepStylePairs); i++)
        {
            if (FlagOn(windowInfo.dwStyle, WepStylePairs[i].Integer))
            {
                // Skip irrelevant styles.
                //if (WepStylePairs[i].Integer == WS_MAXIMIZEBOX ||
                //    WepStylePairs[i].Integer == WS_MINIMIZEBOX)
                //{
                //    if (windowInfo.dwStyle & WS_CHILD)
                //        continue;
                //}

                //if (WepStylePairs[i].Integer == WS_TABSTOP ||
                //    WepStylePairs[i].Integer == WS_GROUP)
                //{
                //    if (!(windowInfo.dwStyle & WS_CHILD))
                //        continue;
                //}

                PhAppendStringBuilder2(&styleStringBuilder, WepStylePairs[i].String);
                PhAppendStringBuilder2(&styleStringBuilder, L", ");
            }
        }

        if (PhEndsWithString2(styleStringBuilder.String, L", ", FALSE))
        {
            PhRemoveEndStringBuilder(&styleStringBuilder, 2);
            PhAppendCharStringBuilder(&styleStringBuilder, ')');
        }
        else
        {
            PhRemoveEndStringBuilder(&styleStringBuilder, 1);
        }

        for (i = 0; i < RTL_NUMBER_OF(WepExtendedStylePairs); i++)
        {
            if (FlagOn(windowInfo.dwExStyle, WepExtendedStylePairs[i].Integer) == WepExtendedStylePairs[i].Integer)
            {
                PhAppendStringBuilder2(&styleExStringBuilder, WepExtendedStylePairs[i].String);
                PhAppendStringBuilder2(&styleExStringBuilder, L", ");
            }
        }

        if (PhEndsWithString2(styleExStringBuilder.String, L", ", FALSE))
        {
            PhRemoveEndStringBuilder(&styleExStringBuilder, 2);
            PhAppendCharStringBuilder(&styleExStringBuilder, ')');
        }
        else
        {
            PhRemoveEndStringBuilder(&styleExStringBuilder, 1);
        }

        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_STYLES, 1, PhFinalStringBuilderString(&styleStringBuilder)->Buffer);
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_EXSTYLES, 1, PhFinalStringBuilderString(&styleExStringBuilder)->Buffer);

        PhDeleteStringBuilder(&styleStringBuilder);
        PhDeleteStringBuilder(&styleExStringBuilder);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_STYLES, 1, L"N/A");
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_EXSTYLES, 1, L"N/A");
    }
}

VOID WepRefreshClassStyles(
    _In_ HWND ListViewHandle,
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG i;

    PhInitializeStringBuilder(&stringBuilder, 100);
    PhAppendFormatStringBuilder(&stringBuilder, L"0x%x (", Context->ClassInfo.style);

    for (i = 0; i < RTL_NUMBER_OF(WepClassStylePairs); i++)
    {
        if (Context->ClassInfo.style & WepClassStylePairs[i].Integer)
        {
            PhAppendStringBuilder2(&stringBuilder, WepClassStylePairs[i].String);
            PhAppendStringBuilder2(&stringBuilder, L", ");
        }
    }

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
    {
        PhRemoveEndStringBuilder(&stringBuilder, 2);
        PhAppendCharStringBuilder(&stringBuilder, ')');
    }
    else
    {
        // No styles. Remove the brackets.
        PhRemoveEndStringBuilder(&stringBuilder, 1);
    }

    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_STYLES, 1, PhFinalStringBuilderString(&stringBuilder)->Buffer);
    PhDeleteStringBuilder(&stringBuilder);
}

VOID WepRefreshClassModule(
    _In_ HWND ListViewHandle,
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    HANDLE processHandle;
    PPH_STRING fileName = NULL;
    PVOID instanceHandle = (PVOID)GetClassLongPtr(Context->WindowHandle, GCLP_HMODULE);

    if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, Context->ClientId.UniqueProcess)))
    {
        if (NT_SUCCESS(PhGetProcessMappedFileName(processHandle, instanceHandle, &fileName)))
        {
            PhMoveReference(&fileName, PhGetFileName(fileName));
            PhMoveReference(&fileName, PhGetBaseName(fileName));
        }

        NtClose(processHandle);
    }

    if (fileName)
    {
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_INSTANCE, 1, PhaFormatString(
            L"0x%Ix (%s)",
            (ULONG_PTR)instanceHandle,
            PhGetStringOrEmpty(fileName)
            )->Buffer);
        PhDereferenceObject(fileName);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_INSTANCE, 1, PhaFormatString(
            L"0x%Ix",
            (ULONG_PTR)instanceHandle
            )->Buffer);
    }
}

VOID WepRefreshWindowClassInfoSymbols(
    _In_ HWND ListViewHandle,
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    if (Context->ClassWndProcResolving != 0)
    {
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_WNDPROC, 1, PhaFormatString(
            L"0x%Ix (resolving...)",
            (ULONG_PTR)Context->ClassInfo.lpfnWndProc
            )->Buffer);
    }
    else if (Context->ClassWndProcSymbol)
    {
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_WNDPROC, 1, PhaFormatString(
            L"0x%Ix (%s)",
            (ULONG_PTR)Context->ClassInfo.lpfnWndProc,
            Context->ClassWndProcSymbol->Buffer
            )->Buffer);
    }
    else if (Context->ClassInfo.lpfnWndProc)
    {
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_WNDPROC, 1, PhaFormatString(
            L"0x%Ix",
            (ULONG_PTR)Context->ClassInfo.lpfnWndProc
            )->Buffer);
    }
    else
    {
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_WNDPROC, 1, L"Unknown");
    }
}

VOID WepRefreshWindowClassInfo(
    _In_ HWND WindowHandle,
    _In_ HWND ListViewHandle,
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    WCHAR className[256];
    WCHAR classBaseName[256];
    ULONG classExtra;

    if (!NT_SUCCESS(PhGetClassName(Context->WindowHandle, className, RTL_NUMBER_OF(className), NULL)))
        className[0] = UNICODE_NULL;
    if (!RealGetWindowClassW(Context->WindowHandle, classBaseName, RTL_NUMBER_OF(classBaseName)))
        classBaseName[0] = UNICODE_NULL;

    Context->ClassInfo.cbSize = sizeof(WNDCLASSEX);
    Context->ClassAtom = PhGetClassInfoEx(NULL, className, &Context->ClassInfo);
    classExtra = (ULONG)GetClassLongPtr(Context->WindowHandle, GCL_CBCLSEXTRA);

    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_NAME, 1, className);
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_BASENAME, 1, classBaseName);
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_ATOM, 1, PhaFormatString(L"0x%Ix", (ULONG_PTR)Context->ClassAtom)->Buffer);
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_LARGEICON, 1, PhaFormatString(L"0x%Ix", (ULONG_PTR)Context->ClassInfo.hIcon)->Buffer);
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_SMALLICON, 1, PhaFormatString(L"0x%Ix", (ULONG_PTR)Context->ClassInfo.hIconSm)->Buffer);
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_MENUNAME, 1, PhaFormatString(L"0x%Ix", (ULONG_PTR)Context->ClassInfo.lpszMenuName)->Buffer);
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_CURSOR, 1, PhaFormatString(L"0x%Ix", (ULONG_PTR)Context->ClassInfo.hCursor)->Buffer);
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_BACKBRUSH, 1, PhaFormatString(L"0x%Ix", (ULONG_PTR)Context->ClassInfo.hbrBackground)->Buffer);
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_WNDEXTRA, 1, PhaFormatString(L"%lu bytes (%s)", classExtra, PhaFormatSize(classExtra, ULONG_MAX)->Buffer)->Buffer);

    WepRefreshClassStyles(ListViewHandle, Context);
    WepRefreshClassModule(ListViewHandle, Context);

    if (!Context->ClassInfo.lpfnWndProc)
    {
        Context->ClassInfo.lpfnWndProc = (WNDPROC)GetClassLongPtr(Context->WindowHandle, GCLP_WNDPROC);
    }

    if (Context->ClassInfo.lpfnWndProc)
    {
        Context->ClassWndProcResolving++;
        WepQueueResolveSymbol(Context, WindowHandle, (PVOID)Context->ClassInfo.lpfnWndProc, 3);
    }

    WepRefreshWindowClassInfoSymbols(ListViewHandle, Context);
}

VOID WepRefreshAutomationProvider(
    _In_ PWINDOW_PROPERTIES_CONTEXT Context,
    _In_ HWND ListViewHandle
    )
{
    //if (Context->MessageOnlyWindow)
    //{
    //    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_AUTOMATION, 1, L"N/A");
    //}
    //else
    {
        if (WeWindowHasAutomationProvider(Context->WindowHandle))
            PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_AUTOMATION, 1, L"Yes");
        else
            PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_AUTOMATION, 1, L"No");
    }
}

VOID WepRefreshDpiContext(
    _In_ PWINDOW_PROPERTIES_CONTEXT Context,
    _In_ HWND ListViewHandle
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static DPI_AWARENESS_CONTEXT (WINAPI* GetWindowDpiAwarenessContext_I)(_In_ HWND WindowHandle) = NULL;
    static BOOL (WINAPI* AreDpiAwarenessContextsEqual_I)(_In_ DPI_AWARENESS_CONTEXT dpiContextA, _In_ DPI_AWARENESS_CONTEXT dpiContextB) = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"user32.dll"))
        {
           GetWindowDpiAwarenessContext_I = PhGetProcedureAddress(baseAddress, "GetWindowDpiAwarenessContext", 0);
           AreDpiAwarenessContextsEqual_I = PhGetProcedureAddress(baseAddress, "AreDpiAwarenessContextsEqual", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    // Windows 10, version 1607+
    if (!(GetWindowDpiAwarenessContext_I && AreDpiAwarenessContextsEqual_I))
    {
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_DPICONTEXT, 1, L"N/A");
    }
    else
    {
        DPI_AWARENESS_CONTEXT dpiContext = GetWindowDpiAwarenessContext_I(Context->WindowHandle);

        if (AreDpiAwarenessContextsEqual_I(dpiContext, DPI_AWARENESS_CONTEXT_UNAWARE))
        {
            PhSetListViewSubItem(
                ListViewHandle,
                WINDOW_PROPERTIES_INDEX_DPICONTEXT,
                1,
                L"Unaware"
                );
        }
        else if (AreDpiAwarenessContextsEqual_I(dpiContext, DPI_AWARENESS_CONTEXT_SYSTEM_AWARE))
        {
            PhSetListViewSubItem (
                ListViewHandle,
                WINDOW_PROPERTIES_INDEX_DPICONTEXT,
                1,
                L"System aware"
                );
        }
        else if (AreDpiAwarenessContextsEqual_I(dpiContext,DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
        {
            PhSetListViewSubItem (
                ListViewHandle,
                WINDOW_PROPERTIES_INDEX_DPICONTEXT,
                1,
                L"Per-monitor aware"
                );
        }
        else if (AreDpiAwarenessContextsEqual_I(dpiContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
        {
            PhSetListViewSubItem(
                ListViewHandle,
                WINDOW_PROPERTIES_INDEX_DPICONTEXT,
                1,
                L"Per-monitor V2"
                );
        }
        else if (AreDpiAwarenessContextsEqual_I(dpiContext, DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED))
        {
            PhSetListViewSubItem(
                ListViewHandle,
                WINDOW_PROPERTIES_INDEX_DPICONTEXT,
                1,
                L"Unaware (GDI scaled)"
                );
        }
        else
        {
            PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_DPICONTEXT, 1, PhaFormatString(
                L"Unknown (0x%Ix)",
                (ULONG_PTR)dpiContext
                )->Buffer);
        }
    }
}

VOID WepGeneralAddListViewItemGroups(
    _In_ HWND ListViewHandle
    )
{
    ListView_EnableGroupView(ListViewHandle, TRUE);
    PhAddListViewGroup(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, L"General");
    PhAddListViewGroup(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_CLASS, L"Class");

    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_APPID, L"AppId", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_THREAD, L"Thread", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_RECT, L"Rectangle", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_NORMALRECT, L"Normal rectangle", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_CLIENTRECT, L"Client rectangle", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_INSTANCE, L"Instance handle", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_MENUHANDLE, L"Menu handle", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_USERDATA, L"User data", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_UNICODE, L"Unicode", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_WNDTEXT, L"Window text", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_WNDHANDLE, L"Window handle", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_WNDMSGONLY, L"Window message-only", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_WNDEXTRA, L"Window extra bytes", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_WNDPROC, L"Window procedure", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_DLGPROC, L"Dialog procedure", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_DLGCTLID, L"Dialog control ID", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_FONTNAME, L"Font", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_STYLES, L"Styles", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_EXSTYLES, L"Extended styles", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_AUTOMATION, L"Automation server", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_DPICONTEXT, L"DPI Context", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_MONITOR, L"Monitor", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_TOPLEVEL, L"Top level", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_CLOAKED, L"Cloaked", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_IAMID, L"Band", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_IMEWND, L"IME Window", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_GENERAL, WINDOW_PROPERTIES_INDEX_D3DKMT_EXCLUSIVE, L"Exclusive ownership", NULL);

    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_CLASS, WINDOW_PROPERTIES_INDEX_CLASS_NAME, L"Name", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_CLASS, WINDOW_PROPERTIES_INDEX_CLASS_BASENAME, L"Base name", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_CLASS, WINDOW_PROPERTIES_INDEX_CLASS_ATOM, L"Atom", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_CLASS, WINDOW_PROPERTIES_INDEX_CLASS_STYLES, L"Styles", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_CLASS, WINDOW_PROPERTIES_INDEX_CLASS_INSTANCE, L"Instance handle", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_CLASS, WINDOW_PROPERTIES_INDEX_CLASS_LARGEICON, L"Large icon handle", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_CLASS, WINDOW_PROPERTIES_INDEX_CLASS_SMALLICON, L"Small icon handle", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_CLASS, WINDOW_PROPERTIES_INDEX_CLASS_CURSOR, L"Cursor handle", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_CLASS, WINDOW_PROPERTIES_INDEX_CLASS_BACKBRUSH, L"Background brush", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_CLASS, WINDOW_PROPERTIES_INDEX_CLASS_MENUNAME, L"Menu name", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_CLASS, WINDOW_PROPERTIES_INDEX_CLASS_WNDEXTRA, L"Window extra bytes", NULL);
    PhAddListViewGroupItem(ListViewHandle, WINDOW_PROPERTIES_CATEGORY_CLASS, WINDOW_PROPERTIES_INDEX_CLASS_WNDPROC, L"Window procedure", NULL);
}

VOID WepWindowRefreshGeneralPageHeader(
    _In_ HWND WindowHandle,
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    // TODO: AppId, ImageBase or something else for line two? (dmex)

    // Window icon
    {
        PPH_PROCESS_ITEM processItem;
        HICON windowIcon = NULL;

        if (PhGetIntegerSetting(SETTING_NAME_WINDOW_ENABLE_ICONS_INTERNAL))
        {
            windowIcon = WeGetInternalWindowIcon(Context->WindowHandle, ICON_BIG);
        }
        else
        {
            if (processItem = PhReferenceProcessItem(Context->ClientId.UniqueProcess))
            {
                if (PhTestEvent(&processItem->Stage1Event))
                {
                    windowIcon = PhGetImageListIcon((ULONG)processItem->SmallIconIndex, TRUE);
                }

                PhDereferenceObject(processItem);
            }
        }

        if (!windowIcon)
        {
            PhGetStockApplicationIcon(NULL, &windowIcon, PhGetWindowDpi(WindowHandle));
        }

        if (windowIcon)
        {
            Static_SetIcon(GetDlgItem(WindowHandle, IDC_WINDOWICON), windowIcon);
        }

        if (Context->WindowIcon) DestroyIcon(Context->WindowIcon);
        Context->WindowIcon = windowIcon;
    }

    // Window text
    {
        PPH_STRING text = PhGetWindowText(Context->WindowHandle);

        if (PhIsNullOrEmptyString(text))
        {
            ULONG classNameLength = 0;
            WCHAR className[0x100];

            if (NT_SUCCESS(PhGetClassName(Context->WindowHandle, className, RTL_NUMBER_OF(className), &classNameLength)))
            {
                className[classNameLength] = UNICODE_NULL;
            }
            else
            {
                className[0] = UNICODE_NULL;
            }

            PhSetWindowText(GetDlgItem(WindowHandle, IDC_WINDOWTEXT), className);
        }
        else
        {
            PhSetWindowText(GetDlgItem(WindowHandle, IDC_WINDOWTEXT), PhGetString(text));
        }

        PhClearReference(&text);
    }

    // Window appid
    {
        PPH_STRING appIdText;

        if (HR_SUCCESS(PhAppResolverGetAppIdForWindow(Context->WindowHandle, &appIdText)))
        {
            PhSetWindowText(GetDlgItem(WindowHandle, IDC_APPIDTEXT), PhGetString(appIdText));
            PhDereferenceObject(appIdText);
        }
    }
}

VOID WepWindowRefreshGeneralPage(
    _In_ HWND WindowHandle,
    _In_ PWINDOW_PROPERTIES_GENERAL_PAGE_CONTEXT Context
    )
{
    PWINDOW_PROPERTIES_CONTEXT parentContext = Context->Parent;

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);
    WepWindowRefreshGeneralPageHeader(WindowHandle, parentContext);
    WepGeneralAddListViewItemGroups(Context->ListViewHandle);
    WepRefreshWindowGeneralInfo(WindowHandle, Context->ListViewHandle, parentContext);
    WepRefreshWindowStyles(Context->ListViewHandle, parentContext);
    WepRefreshWindowClassInfo(WindowHandle, Context->ListViewHandle, parentContext);
    WepRefreshAutomationProvider(parentContext, Context->ListViewHandle);
    WepRefreshDpiContext(parentContext, Context->ListViewHandle);

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

INT_PTR CALLBACK WepWindowGeneralDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWINDOW_PROPERTIES_GENERAL_PAGE_CONTEXT context;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(WindowHandle, WindowMessage, lParam, &propPageContext))
        return FALSE;

    context = (PWINDOW_PROPERTIES_GENERAL_PAGE_CONTEXT)propPageContext->Context;

    if (!context || !context->Parent)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_WINDOWINFO);

            PhSetApplicationWindowIcon(GetParent(WindowHandle));

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            SetWindowFont(context->ListViewHandle, context->Parent->TreeWindowFont, TRUE);
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 180, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Value");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_WINDOWS_PROPERTY_COLUMNS, context->ListViewHandle);

            if (!PhValidWindowPlacementFromSetting(SETTING_NAME_WINDOWS_PROPERTY_POSITION))
            {
                PhCenterWindow(GetParent(WindowHandle), context->Parent->ParentWindowHandle);
            }

            WepWindowRefreshGeneralPage(WindowHandle, context);

            ExtendedListView_SetColumnWidth(context->ListViewHandle, 1, ELVSCW_AUTOSIZE_REMAININGSPACE);

            if (PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT))
                PhInitializeWindowTheme(GetParent(WindowHandle), !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));
            else
                PhInitializeWindowTheme(WindowHandle, FALSE);
        }
        break;
    case WM_DESTROY:
        {
            if (context->ListViewHandle)
            {
                PhSaveListViewColumnsToSetting(SETTING_NAME_WINDOWS_PROPERTY_COLUMNS, context->ListViewHandle);
                context->ListViewHandle = NULL;
            }

            if (context->Parent->WindowIcon)
            {
                DestroyIcon(context->Parent->WindowIcon);
                context->Parent->WindowIcon = NULL;
            }
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(WindowHandle, WindowHandle, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(WindowHandle, GetDlgItem(WindowHandle, IDC_WINDOWGROUPBOX), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
                PvAddPropPageLayoutItem(WindowHandle, context->ListViewHandle, dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(WindowHandle);

                ExtendedListView_SetColumnWidth(context->ListViewHandle, 1, ELVSCW_AUTOSIZE_REMAININGSPACE);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems = NULL;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                if (PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems))
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        WindowHandle,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case PHAPP_IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                    PhFree(listviewItems);
                }
            }
        }
        break;
    case WEM_RESOLVE_DONE:
        {
            PSYMBOL_RESOLVE_CONTEXT resolveContext = (PSYMBOL_RESOLVE_CONTEXT)lParam;

            if (resolveContext->Id == 1)
            {
                PhAcquireQueuedLockExclusive(&context->Parent->ResolveListLock);
                RemoveEntryListNoFence(&resolveContext->ListEntry);
                PhReleaseQueuedLockExclusive(&context->Parent->ResolveListLock);

                if (resolveContext->ResolveLevel != PhsrlModule && resolveContext->ResolveLevel != PhsrlFunction)
                    PhClearReference(&resolveContext->Symbol);

                PhMoveReference(&context->Parent->WndProcSymbol, resolveContext->Symbol);
                PhFree(resolveContext);

                context->Parent->WndProcResolving--;
            }
            else if (resolveContext->Id == 2)
            {
                PhAcquireQueuedLockExclusive(&context->Parent->ResolveListLock);
                RemoveEntryListNoFence(&resolveContext->ListEntry);
                PhReleaseQueuedLockExclusive(&context->Parent->ResolveListLock);

                if (resolveContext->ResolveLevel != PhsrlModule && resolveContext->ResolveLevel != PhsrlFunction)
                    PhClearReference(&resolveContext->Symbol);

                PhMoveReference(&context->Parent->DlgProcSymbol, resolveContext->Symbol);
                PhFree(resolveContext);

                context->Parent->DlgProcResolving--;
            }
            else if (resolveContext->Id == 3)
            {
                PhAcquireQueuedLockExclusive(&context->Parent->ResolveListLock);
                RemoveEntryListNoFence(&resolveContext->ListEntry);
                PhReleaseQueuedLockExclusive(&context->Parent->ResolveListLock);

                if (resolveContext->ResolveLevel != PhsrlModule && resolveContext->ResolveLevel != PhsrlFunction)
                    PhClearReference(&resolveContext->Symbol);

                PhMoveReference(&context->Parent->ClassWndProcSymbol, resolveContext->Symbol);
                PhFree(resolveContext);

                context->Parent->ClassWndProcResolving--;
            }

            WepRefreshWindowGeneralInfoSymbols(context->ListViewHandle, context->Parent);
            WepRefreshWindowClassInfoSymbols(context->ListViewHandle, context->Parent);
        }
        break;
    case WM_PH_UPDATE_DIALOG:
        {
            //WepWindowRefreshGeneralPage(WindowHandle, context);
        }
        break;
    case WM_SIZE:
        {
            //ExtendedListView_SetColumnWidth(context->ListViewHandle, 1, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            HBRUSH brush;

            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));

            brush = PhGetStockBrush(DC_BRUSH);
            SelectBrush((HDC)wParam, brush);
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));

            return (INT_PTR)brush;
        }
        break;
    }

    return FALSE;
}
typedef struct _WEP_WINDOW_PROPEDIT_CONTEXT
{
    HWND WindowHandle;
    HWND TargetWindowHandle;
    PPH_STRING WindowPropString;
    BOOLEAN WindowPropCreate;
} WEP_WINDOW_PROPEDIT_CONTEXT, *PWEP_WINDOW_PROPEDIT_CONTEXT;

static INT_PTR CALLBACK WepWindowPropEditDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    static PH_LAYOUT_MANAGER LayoutManager;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            PWEP_WINDOW_PROPEDIT_CONTEXT context = (PWEP_WINDOW_PROPEDIT_CONTEXT)lParam;

            PhSetApplicationWindowIcon(WindowHandle);

            PhSetWindowText(WindowHandle, L"Property Editor");
            PhCenterWindow(WindowHandle, GetParent(WindowHandle));

            PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);

            PhInitializeLayoutManager(&LayoutManager, WindowHandle);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(WindowHandle, IDC_NAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(WindowHandle, IDC_VALUE), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(WindowHandle, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(WindowHandle, IDCANCEL), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (!context->WindowPropCreate)
            {
                WCHAR value[PH_INT64_STR_LEN_1];

                PhPrintPointer(value, (PVOID)GetProp(context->TargetWindowHandle, PhGetString(context->WindowPropString)));
                PhSetDialogItemText(WindowHandle, IDC_NAME, PhGetString(context->WindowPropString));
                PhSetDialogItemText(WindowHandle, IDC_VALUE, value);

                EnableWindow(GetDlgItem(WindowHandle, IDC_NAME), FALSE);
            }

            PhSetDialogFocus(WindowHandle, GetDlgItem(WindowHandle, IDCANCEL));

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);

            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_SIZE:
         {
             PhLayoutManagerLayout(&LayoutManager);
         }
         break;
     case WM_DPICHANGED_AFTERPARENT:
         {
             PhLayoutManagerUpdate(&LayoutManager, PhGetWindowDpi(WindowHandle));
             PhLayoutManagerLayout(&LayoutManager);
         }
         break;
     case WM_COMMAND:
         {
             switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(WindowHandle, IDCANCEL);
                break;
            case IDOK:
                {
                    PWEP_WINDOW_PROPEDIT_CONTEXT context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
                    PPH_STRING windowPropName = PH_AUTO(PhGetWindowText(GetDlgItem(WindowHandle, IDC_NAME)));
                    PPH_STRING windowPropValue = PH_AUTO(PhGetWindowText(GetDlgItem(WindowHandle, IDC_VALUE)));
                    LONG64 value = 0;

                    PhStringToInteger64(&windowPropValue->sr, 0, &value);

                    if (!context->WindowPropCreate && PhIsNullOrEmptyString(windowPropName))
                    {
                        PhShowError2(WindowHandle, L"Unable to add window property.", L"%s", L"The property name is empty.");
                        break;
                    }

                    if (context->WindowPropCreate)
                    {
                        if (!SetProp(context->TargetWindowHandle, PhGetString(windowPropName), (HANDLE)value))
                        {
                            PhShowStatus(WindowHandle, L"Unable to create the window property.", 0, GetLastError());
                            break;
                        }
                    }
                    else
                    {
                        if (!SetProp(context->TargetWindowHandle, PhGetString(context->WindowPropString), (HANDLE)value))
                        {
                            PhShowStatus(WindowHandle, L"Unable to update the window property.", 0, GetLastError());
                            break;
                        }
                    }

                    EndDialog(WindowHandle, IDOK);
                }
                break;
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

VOID WepFreeListViewWindowProps(
    _In_ PWINDOW_PROPERTIES_PROPLIST_PAGE_CONTEXT Context
    )
{
    INT index = INT_ERROR;

    if (!Context->ListViewHandle || !IsWindow(Context->ListViewHandle))
        return;

    while ((index = PhFindListViewItemByFlags(Context->ListViewHandle, index, LVNI_ALL)) != INT_ERROR)
    {
        PPH_STRING param;

        if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
        {
            PhClearReference(&param);
            PhSetListViewItemParam(Context->ListViewHandle, index, NULL);
        }
    }
}

BOOL CALLBACK WepEnumPropsExCallback(
    _In_ HWND WindowHandle,
    _In_ PWSTR Name,
    _In_ HANDLE Value,
    _In_ ULONG_PTR Context
    )
{
    PWINDOW_PROPERTIES_PROPLIST_PAGE_CONTEXT context = (PWINDOW_PROPERTIES_PROPLIST_PAGE_CONTEXT)Context;
    PWINDOW_PROPERTIES_CONTEXT parentContext = context->Parent;
    INT lvItemIndex;
    PPH_STRING propName;
    WCHAR value[PH_INT64_STR_LEN_1];

    if (IS_INTRESOURCE(Name))
        propName = PhFormatString(L"#%hu", PtrToUshort(Name));
    else
        propName = PhCreateString(Name);

    PhPrintUInt32(value, ++parentContext->PropsListCount);
    lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, value, propName);

    if (IS_INTRESOURCE(Name)) // This is an integer atom.
        PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 1, PhGetString(propName));
    else
        PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 1, Name);

    PhPrintPointer(value, (PVOID)Value);
    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 2, value);

    if (!IS_INTRESOURCE(Name))
    {
        PROPERTYKEY propkey;
        PWSTR propKeyName;

        if (HR_SUCCESS(PSGetPropertyKeyFromName(PhGetString(propName), &propkey)))
        {
            if (HR_SUCCESS(PSGetNameFromPropertyKey(&propkey, &propKeyName)))
            {
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 3, propKeyName);
                CoTaskMemFree(propKeyName);
            }
        }
    }

    return TRUE;
}

VOID WepRefreshWindowProps(
    _In_ PWINDOW_PROPERTIES_PROPLIST_PAGE_CONTEXT Context
    )
{
    PWINDOW_PROPERTIES_CONTEXT parentContext = Context->Parent;

    WepFreeListViewWindowProps(Context);
    parentContext->PropsListCount = 0;

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);

    EnumPropsEx(parentContext->WindowHandle, WepEnumPropsExCallback, (LPARAM)Context);

    ExtendedListView_SortItems(Context->ListViewHandle);
    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

INT_PTR CALLBACK WepWindowPropListDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWINDOW_PROPERTIES_PROPLIST_PAGE_CONTEXT context;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(WindowHandle, WindowMessage, lParam, &propPageContext))
        return FALSE;

    context = (PWINDOW_PROPERTIES_PROPLIST_PAGE_CONTEXT)propPageContext->Context;

    if (!context || !context->Parent)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            SetWindowFont(context->ListViewHandle, context->Parent->TreeWindowFont, TRUE);
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 80, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 160, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"Value");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Alias");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_WINDOWS_PROPLIST_COLUMNS, context->ListViewHandle);

            WepRefreshWindowProps(context);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));
        }
        break;
    case WM_DESTROY:
        {
            if (context->ListViewHandle)
            {
                PhSaveListViewColumnsToSetting(SETTING_NAME_WINDOWS_PROPLIST_COLUMNS, context->ListViewHandle);

                WepFreeListViewWindowProps(context);
                context->ListViewHandle = NULL;
            }
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(WindowHandle, WindowHandle, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(WindowHandle, context->ListViewHandle, dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(WindowHandle);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                menu = PhCreateEMenu();
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_ADD, L"Add", NULL, NULL), ULONG_MAX);

                if (PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems))
                {
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDD_EDITENV, L"Edit", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_DELETE, L"Delete", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->ListViewHandle);
                }

                item = PhShowEMenu(
                    menu,
                    WindowHandle,
                    PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_TOP,
                    point.x,
                    point.y
                    );

                if (item)
                {
                    if (!PhHandleCopyListViewEMenuItem(item))
                    {
                        switch (item->Id)
                        {
                        case PHAPP_IDC_ADD:
                            {
                                WEP_WINDOW_PROPEDIT_CONTEXT windowPropEditContext;

                                memset(&windowPropEditContext, 0, sizeof(WEP_WINDOW_PROPEDIT_CONTEXT));
                                windowPropEditContext.TargetWindowHandle = context->Parent->WindowHandle;
                                windowPropEditContext.WindowPropCreate = TRUE;

                                PhDialogBox(
                                    PluginInstance->DllBase,
                                    MAKEINTRESOURCE(IDD_WNDPROPEDIT),
                                    WindowHandle,
                                    WepWindowPropEditDlgProc,
                                    &windowPropEditContext
                                    );

                                //WepRefreshWindowProps(context);
                                PvRefreshChildWindows(WindowHandle);
                            }
                            break;
                        case PHAPP_IDD_EDITENV:
                            {
                                WEP_WINDOW_PROPEDIT_CONTEXT windowPropEditContext;

                                memset(&windowPropEditContext, 0, sizeof(WEP_WINDOW_PROPEDIT_CONTEXT));
                                windowPropEditContext.TargetWindowHandle = context->Parent->WindowHandle;
                                windowPropEditContext.WindowPropString = listviewItems[0];

                                PhDialogBox(
                                    PluginInstance->DllBase,
                                    MAKEINTRESOURCE(IDD_WNDPROPEDIT),
                                    WindowHandle,
                                    WepWindowPropEditDlgProc,
                                    &windowPropEditContext
                                    );

                                //WepRefreshWindowProps(context);
                                PvRefreshChildWindows(WindowHandle);
                            }
                            break;
                        case PHAPP_IDC_DELETE:
                            {
                                NTSTATUS status;

                                if (PhGetIntegerSetting(SETTING_ENABLE_WARNINGS) && !PhShowConfirmMessage(
                                    WindowHandle,
                                    L"remove",
                                    L"the window property",
                                    L"The window property will be permanently deleted.",
                                    FALSE
                                    ))
                                {
                                    break;
                                }

                                RemoveProp(context->Parent->WindowHandle, PhGetString(listviewItems[0]));

                                status = PhGetLastWin32ErrorAsNtStatus();
                                if (status != STATUS_CANCELLED)
                                    PhShowStatus(WindowHandle, L"Unable to remove the window property.", status, 0);

                                //WepRefreshWindowProps(context);
                                PvRefreshChildWindows(WindowHandle);
                            }
                            break;
                        case PHAPP_IDC_COPY:
                            {
                                PhCopyListView(context->ListViewHandle);
                            }
                            break;
                        }
                    }
                }

                PhDestroyEMenu(menu);

                if (listviewItems)
                    PhFree(listviewItems);
            }
        }
        break;
    case WM_PH_UPDATE_DIALOG:
        {
            WepRefreshWindowProps(context);
        }
        break;
    }

    return FALSE;
}

VOID WepRefreshWindowPropertyStorage(
    _In_ PWINDOW_PROPERTIES_PROPSTORE_PAGE_CONTEXT Context
    )
{
    PWINDOW_PROPERTIES_CONTEXT parentContext = Context->Parent;
    IPropertyStore *propstore;
    ULONG count;
    ULONG i;

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);

    if (SUCCEEDED(SHGetPropertyStoreForWindow(
        parentContext->WindowHandle,
        &IID_IPropertyStore,
        &propstore
        )))
    {
        if (SUCCEEDED(IPropertyStore_GetCount(propstore, &count)))
        {
            for (i = 0; i < count; i++)
            {
                PROPERTYKEY propkey;

                if (SUCCEEDED(IPropertyStore_GetAt(propstore, i, &propkey)))
                {
                    INT lvItemIndex;
                    PROPVARIANT propKeyVariant = { 0 };
                    PWSTR propKeyName;
                    WCHAR value[PH_INT64_STR_LEN_1];

                    PhPrintUInt32(value, i + 1);
                    lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, value, NULL);

                    if (SUCCEEDED(PSGetNameFromPropertyKey(&propkey, &propKeyName)))
                    {
                        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, propKeyName);
                        CoTaskMemFree(propKeyName);
                    }
                    else
                    {
                        WCHAR propKeyString[PKEYSTR_MAX];

                        if (SUCCEEDED(PSStringFromPropertyKey(&propkey, propKeyString, RTL_NUMBER_OF(propKeyString))))
                            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, propKeyString);
                        else
                            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, L"Unknown");
                    }

                    if (SUCCEEDED(IPropertyStore_GetValue(propstore, &propkey, &propKeyVariant)))
                    {
                        if (SUCCEEDED(PSFormatForDisplayAlloc(&propkey, &propKeyVariant, PDFF_DEFAULT, &propKeyName)))
                        {
                            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, propKeyName);
                            CoTaskMemFree(propKeyName);
                        }

                        //if (SUCCEEDED(PropVariantToStringAlloc(&propKeyVariant, &propKeyName)))
                        //{
                        //    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, propKeyName);
                        //    CoTaskMemFree(propKeyName);
                        //}

                        PropVariantClear(&propKeyVariant);
                    }
                }
            }
        }

        IPropertyStore_Release(propstore);
    }

    ExtendedListView_SortItems(Context->ListViewHandle);
    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

INT_PTR CALLBACK WepWindowPropStoreDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWINDOW_PROPERTIES_PROPSTORE_PAGE_CONTEXT context;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(WindowHandle, WindowMessage, lParam, &propPageContext))
        return FALSE;

    context = (PWINDOW_PROPERTIES_PROPSTORE_PAGE_CONTEXT)propPageContext->Context;

    if (!context || !context->Parent)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            SetWindowFont(context->ListViewHandle, context->Parent->TreeWindowFont, TRUE);
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 50, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 150, L"Value");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_WINDOWS_PROPSTORAGE_COLUMNS, context->ListViewHandle);

            WepRefreshWindowPropertyStorage(context);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));
        }
        break;
    case WM_DESTROY:
        {
            if (context->ListViewHandle)
            {
                PhSaveListViewColumnsToSetting(SETTING_NAME_WINDOWS_PROPSTORAGE_COLUMNS, context->ListViewHandle);
                context->ListViewHandle = NULL;
            }
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(WindowHandle, WindowHandle, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(WindowHandle, context->ListViewHandle, dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(WindowHandle);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                if (PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems))
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        WindowHandle,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case PHAPP_IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }
                    PhFree(listviewItems);
                }
        }
        break;
    case WM_PH_UPDATE_DIALOG:
        {
            WepRefreshWindowPropertyStorage(context);
        }
        break;
    }

    return FALSE;
}

VOID WepQueryWindowAttributes(
    _In_ PWINDOW_PROPERTIES_DWMATTRIBUTES_PAGE_CONTEXT Context,
    _In_ WINDOWCOMPOSITIONATTRIB i,
    _In_ PCWSTR Name,
    _Inout_ PULONG Count
    )
{
    HRESULT result;
    WINDOWCOMPOSITIONATTRIBUTEDATA data = { 0 };
    BYTE buffer[256] = { 0 };
    data.Attribute = i;
    data.Data = buffer;
    data.Length = sizeof(buffer);

    result = PhGetWindowCompositionAttribute(Context->Parent->WindowHandle, &data);

    if (SUCCEEDED(result))
    {
        LONG lvItemIndex;
        WCHAR value[PH_INT64_STR_LEN_1];

        PhPrintUInt32(value, (*Count)++);
        lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, value, NULL);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, Name);

        if (i == WCA_EXTENDED_FRAME_BOUNDS || i == WCA_CAPTION_BUTTON_BOUNDS)
        {
            PRECT rect = (PRECT)buffer;
            PPH_STRING string = PhaFormatString(
                L"RECT { left: %d, top: %d, right: %d, bottom: %d }",
                rect->left,
                rect->top,
                rect->right,
                rect->bottom
                );
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, PhGetStringOrEmpty(string));
        }
        else if (i == WCA_ACCENT_POLICY || i == WCA_PART_COLOR || i == WCA_SYSTEMBACKDROP_TYPE)
        {
            PhPrintUInt32IX(value, *(PULONG)buffer);
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, value);
        }
        else
        {
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, *(PBOOL)buffer ? L"true" : L"false");
        }
    }
    else
    {
        //PPH_STRING message;
        LONG lvItemIndex;
        WCHAR value[PH_INT64_STR_LEN_1];

        PhPrintUInt32(value, (*Count)++);
        lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, value, NULL);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, Name);
        PhPrintUInt32IX(value, result);
        //message = PhGetStatusMessage(result, 0);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, PhaFormatString(L"0x%s (Failed)", value)->Buffer);
        //PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, PhaFormatString(L"0x%s (%s)", value, PhGetStringOrDefault(message, L"Failed"))->Buffer);
        //PhClearReference(&message);
    }
}

VOID WepRefreshWindowAttributes(
    _In_ PWINDOW_PROPERTIES_DWMATTRIBUTES_PAGE_CONTEXT Context
    )
{
    ULONG count = 1;

    if (!Context->ListViewHandle)
        return;

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);

    WepQueryWindowAttributes(Context, WCA_NCRENDERING_ENABLED, L"WCA_NCRENDERING_ENABLED", &count);
    WepQueryWindowAttributes(Context, WCA_NCRENDERING_POLICY, L"WCA_NCRENDERING_POLICY", &count);
    WepQueryWindowAttributes(Context, WCA_TRANSITIONS_FORCEDISABLED, L"WCA_TRANSITIONS_FORCEDISABLED", &count);
    WepQueryWindowAttributes(Context, WCA_ALLOW_NCPAINT, L"WCA_ALLOW_NCPAINT", &count);
    WepQueryWindowAttributes(Context, WCA_CAPTION_BUTTON_BOUNDS, L"WCA_CAPTION_BUTTON_BOUNDS", &count);
    WepQueryWindowAttributes(Context, WCA_NONCLIENT_RTL_LAYOUT, L"WCA_NONCLIENT_RTL_LAYOUT", &count);
    WepQueryWindowAttributes(Context, WCA_FORCE_ICONIC_REPRESENTATION, L"WCA_FORCE_ICONIC_REPRESENTATION", &count);
    WepQueryWindowAttributes(Context, WCA_EXTENDED_FRAME_BOUNDS, L"WCA_EXTENDED_FRAME_BOUNDS", &count);
    WepQueryWindowAttributes(Context, WCA_HAS_ICONIC_BITMAP, L"WCA_HAS_ICONIC_BITMAP", &count);
    WepQueryWindowAttributes(Context, WCA_THEME_ATTRIBUTES, L"WCA_THEME_ATTRIBUTES", &count);
    WepQueryWindowAttributes(Context, WCA_NCRENDERING_EXILED, L"WCA_NCRENDERING_EXILED", &count);
    WepQueryWindowAttributes(Context, WCA_NCADORNMENTINFO, L"WCA_NCADORNMENTINFO", &count);
    WepQueryWindowAttributes(Context, WCA_EXCLUDED_FROM_LIVEPREVIEW, L"WCA_EXCLUDED_FROM_LIVEPREVIEW", &count);
    WepQueryWindowAttributes(Context, WCA_VIDEO_OVERLAY_ACTIVE, L"WCA_VIDEO_OVERLAY_ACTIVE", &count);
    WepQueryWindowAttributes(Context, WCA_FORCE_ACTIVEWINDOW_APPEARANCE, L"WCA_FORCE_ACTIVEWINDOW_APPEARANCE", &count);
    WepQueryWindowAttributes(Context, WCA_DISALLOW_PEEK, L"WCA_DISALLOW_PEEK", &count);
    WepQueryWindowAttributes(Context, WCA_CLOAK, L"WCA_CLOAK", &count);
    WepQueryWindowAttributes(Context, WCA_CLOAKED, L"WCA_CLOAKED", &count);
    WepQueryWindowAttributes(Context, WCA_ACCENT_POLICY, L"WCA_ACCENT_POLICY", &count);
    WepQueryWindowAttributes(Context, WCA_FREEZE_REPRESENTATION, L"WCA_FREEZE_REPRESENTATION", &count);
    WepQueryWindowAttributes(Context, WCA_EVER_UNCLOAKED, L"WCA_EVER_UNCLOAKED", &count);
    WepQueryWindowAttributes(Context, WCA_VISUAL_OWNER, L"WCA_VISUAL_OWNER", &count);
    WepQueryWindowAttributes(Context, WCA_HOLOGRAPHIC, L"WCA_HOLOGRAPHIC", &count);
    WepQueryWindowAttributes(Context, WCA_EXCLUDED_FROM_DDA, L"WCA_EXCLUDED_FROM_DDA", &count);
    WepQueryWindowAttributes(Context, WCA_PASSIVEUPDATEMODE, L"WCA_PASSIVEUPDATEMODE", &count);
    WepQueryWindowAttributes(Context, WCA_USEDARKMODECOLORS, L"WCA_USEDARKMODECOLORS", &count);
    WepQueryWindowAttributes(Context, WCA_CORNER_STYLE, L"WCA_CORNER_STYLE", &count);
    WepQueryWindowAttributes(Context, WCA_PART_COLOR, L"WCA_PART_COLOR", &count);
    WepQueryWindowAttributes(Context, WCA_DISABLE_MOVESIZE_FEEDBACK, L"WCA_DISABLE_MOVESIZE_FEEDBACK", &count);
    WepQueryWindowAttributes(Context, WCA_SYSTEMBACKDROP_TYPE, L"WCA_SYSTEMBACKDROP_TYPE", &count);
    WepQueryWindowAttributes(Context, WCA_SET_TAGGED_WINDOW_RECT, L"WCA_SET_TAGGED_WINDOW_RECT", &count);
    WepQueryWindowAttributes(Context, WCA_CLEAR_TAGGED_WINDOW_RECT, L"WCA_CLEAR_TAGGED_WINDOW_RECT", &count);
    WepQueryWindowAttributes(Context, WCA_REMOTEAPP_POLICY, L"WCA_REMOTEAPP_POLICY", &count);
    WepQueryWindowAttributes(Context, WCA_HAS_ACCENT_POLICY, L"WCA_HAS_ACCENT_POLICY", &count);
    WepQueryWindowAttributes(Context, WCA_REDIRECTIONBITMAP_FILL_COLOR, L"WCA_REDIRECTIONBITMAP_FILL_COLOR", &count);
    WepQueryWindowAttributes(Context, WCA_REDIRECTIONBITMAP_ALPHA, L"WCA_REDIRECTIONBITMAP_ALPHA", &count);
    WepQueryWindowAttributes(Context, WCA_BORDER_MARGINS, L"WCA_BORDER_MARGINS", &count);

    ExtendedListView_SortItems(Context->ListViewHandle);
    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

INT_PTR CALLBACK WepWindowAttributeDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWINDOW_PROPERTIES_DWMATTRIBUTES_PAGE_CONTEXT context;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(WindowHandle, WindowMessage, lParam, &propPageContext))
        return FALSE;

    context = (PWINDOW_PROPERTIES_DWMATTRIBUTES_PAGE_CONTEXT)propPageContext->Context;

    if (!context || !context->Parent)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            SetWindowFont(context->ListViewHandle, context->Parent->TreeWindowFont, TRUE);
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 50, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 150, L"Value");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_WINDOWS_DWMATTRIBUTES_COLUMNS, context->ListViewHandle);

            WepRefreshWindowAttributes(context);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));
        }
        break;
    case WM_DESTROY:
        {
            if (context->ListViewHandle)
            {
                PhSaveListViewColumnsToSetting(SETTING_NAME_WINDOWS_DWMATTRIBUTES_COLUMNS, context->ListViewHandle);
                context->ListViewHandle = NULL;
            }
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(WindowHandle, WindowHandle, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(WindowHandle, context->ListViewHandle, dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(WindowHandle);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                if (PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems))
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        WindowHandle,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case PHAPP_IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }
                    PhFree(listviewItems);
                }
        }
        break;
    case WM_PH_UPDATE_DIALOG:
        {
            WepRefreshWindowAttributes(context);
        }
        break;
    }

    return FALSE;
}
