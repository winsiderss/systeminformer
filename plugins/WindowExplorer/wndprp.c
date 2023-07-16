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
#include <workqueue.h>
#include <symprv.h>
#include <mapldr.h>

#include <shellapi.h>
#include <propsys.h>
#include <propvarutil.h>

#define WEM_RESOLVE_DONE (WM_APP + 1234)

typedef struct _WINDOW_PROPERTIES_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND ListViewHandle;
    HWND PropsListViewHandle;
    HWND PropStoreListViewHandle;

    HICON WindowIcon;
    ULONG PropsListCount;

    BOOLEAN MessageOnlyWindow;

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

typedef enum _WINDOW_PROPERTIES_CATEGORY
{
    WINDOW_PROPERTIES_CATEGORY_GENERAL,
    WINDOW_PROPERTIES_CATEGORY_CLASS
} WINDOW_PROPERTIES_CATEGORY;

typedef enum _NETADAPTER_DETAILS_INDEX
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
} NETADAPTER_DETAILS_INDEX;

NTSTATUS WepPropertiesThreadStart(
    _In_ PVOID Parameter
    );

INT_PTR CALLBACK WepWindowGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK WepWindowPropListDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK WepWindowPropStoreDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK WepWindowPreviewDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#define DEFINE_PAIR(Symbol) { TEXT(#Symbol), Symbol }

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
    DEFINE_PAIR(WS_EX_NOREDIRECTIONBITMAP),
    DEFINE_PAIR(WS_EX_LAYOUTRTL),
    DEFINE_PAIR(WS_EX_COMPOSITED),
    DEFINE_PAIR(WS_EX_NOACTIVATE),
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

PPH_OBJECT_TYPE WeWindowItemType = NULL;

VOID NTAPI WeWindowItemDeleteProcedure(
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
}

BOOLEAN WeShowWindowProperties(
    _In_ HWND ParentWindowHandle,
    _In_ HWND WindowHandle,
    _In_ BOOLEAN MessageOnlyWindow,
    _In_ PCLIENT_ID ClientId
    )
{
    PWINDOW_PROPERTIES_CONTEXT context;
    ULONG threadId;
    ULONG processId;

    if (!WeWindowItemType)
        WeWindowItemType = PhCreateObjectType(L"WindowItemType", 0, WeWindowItemDeleteProcedure);

    if (!IsWindow(WindowHandle))
        return FALSE;

    processId = 0;
    threadId = GetWindowThreadProcessId(WindowHandle, &processId);

    if (ClientId->UniqueProcess != UlongToHandle(processId))
        return FALSE;
    if (ClientId->UniqueThread != UlongToHandle(threadId))
        return FALSE;

    context = PhCreateObjectZero(sizeof(WINDOW_PROPERTIES_CONTEXT), WeWindowItemType);
    context->WindowHandle = WindowHandle;
    context->ParentWindowHandle = ParentWindowHandle;
    context->MessageOnlyWindow = MessageOnlyWindow;
    context->ClientId.UniqueProcess = UlongToHandle(processId);
    context->ClientId.UniqueThread = UlongToHandle(threadId);

    PhInitializeInitOnce(&context->SymbolProviderInitOnce);
    InitializeListHead(&context->ResolveListHead);
    PhInitializeQueuedLock(&context->ResolveListLock);

    PhCreateThread2(WepPropertiesThreadStart, context);
    return TRUE;
}

NTSTATUS WepPropertiesThreadStart(
    _In_ PVOID Parameter
    )
{
    PWINDOW_PROPERTIES_CONTEXT context = Parameter;
    PPV_PROPCONTEXT propContext;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    if (propContext = HdCreatePropContext(PhaFormatString(L"Window %Ix", (ULONG_PTR)context->WindowHandle)->Buffer))
    {
        PPV_PROPPAGECONTEXT newPage;

        // General
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_WNDGENERAL),
            WepWindowGeneralDlgProc,
            context);
        PvAddPropPage(propContext, newPage);

        // Properties
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_WNDPROPLIST),
            WepWindowPropListDlgProc,
            context);
        PvAddPropPage(propContext, newPage);

        // Property store
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_WNDPROPSTORAGE),
            WepWindowPropStoreDlgProc,
            context);
        PvAddPropPage(propContext, newPage);

        if (PhGetIntegerSetting(SETTING_NAME_WINDOW_ENABLE_PREVIEW))
        {
            // Preview page
            newPage = PvCreatePropPageContext(
                MAKEINTRESOURCE(IDD_WNDPREVIEW),
                WepWindowPreviewDlgProc,
                context);
            PvAddPropPage(propContext, newPage);
        }

        PhModalPropertySheet(&propContext->PropSheetHeader);
        PhDereferenceObject(propContext);
    }

    PhDeleteAutoPool(&autoPool);

    PhDereferenceObject(context);

    return STATUS_SUCCESS;
}

NTSTATUS WepResolveSymbolFunction(
    _In_ PVOID Parameter
    )
{
    PSYMBOL_RESOLVE_CONTEXT context = Parameter;

    if (PhBeginInitOnce(&context->Context->SymbolProviderInitOnce))
    {
        PhLoadSymbolProviderOptions(context->Context->SymbolProvider);
        PhLoadModulesForVirtualSymbolProvider(context->Context->SymbolProvider, context->Context->ClientId.UniqueProcess);
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
        PhDereferenceObject(context->Context);
        PhFree(context);
        return STATUS_SUCCESS;
    }

    PhAcquireQueuedLockExclusive(&context->Context->ResolveListLock);
    InsertHeadList(&context->Context->ResolveListHead, &context->ListEntry);
    PhReleaseQueuedLockExclusive(&context->Context->ResolveListLock);

    PostMessage(context->NotifyWindow, WEM_RESOLVE_DONE, 0, (LPARAM)context);

    PhDereferenceObject(context->Context);

    return STATUS_SUCCESS;
}

VOID WepQueueResolveSymbol(
    _In_ PWINDOW_PROPERTIES_CONTEXT Context,
    _In_ HWND NotifyWindow,
    _In_ ULONG64 Address,
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

BOOL WepIsTopLevelWindow(
    _In_ HWND WindowHandle
    )
{
    static BOOL (WINAPI* IsTopLevelWindow_I)(
        _In_ HWND hwnd
        ) = NULL;

    if (!IsTopLevelWindow_I)
        IsTopLevelWindow_I = PhGetModuleProcAddress(L"user32.dll", "IsTopLevelWindow");

    if (!IsTopLevelWindow_I)
        return FALSE;

    return IsTopLevelWindow_I(WindowHandle);
}

BOOL WepIsInDesktopWindowBand(
    _In_ HWND WindowHandle
    )
{
    static BOOL (WINAPI* IsInDesktopWindowBand_I)(
        _In_ HWND hwnd
        ) = NULL;

    if (!IsInDesktopWindowBand_I)
        IsInDesktopWindowBand_I = PhGetModuleProcAddress(L"user32.dll", "IsInDesktopWindowBand");

    if (!IsInDesktopWindowBand_I)
        return FALSE;

    return !!IsInDesktopWindowBand_I(WindowHandle);
}

HICON WepGetInternalWindowIcon(
    _In_ HWND WindowHandle,
    _In_ UINT IconType
    )
{
    static HICON (WINAPI *InternalGetWindowIcon_I)(
        _In_ HWND hwnd,
        _In_ UINT iconType
        ) = NULL;

    if (!InternalGetWindowIcon_I)
        InternalGetWindowIcon_I = PhGetModuleProcAddress(L"user32.dll", "InternalGetWindowIcon");

    if (!InternalGetWindowIcon_I)
        return NULL;

    return InternalGetWindowIcon_I(WindowHandle, IconType);
}

HICON WepGetWindowIcon(
    _In_ HWND WindowHandle
    )
{
    ULONG_PTR windowIcon = 0;
    LRESULT status;

    status = SendMessageTimeout(
        WindowHandle,
        WM_GETICON,
        ICON_SMALL2,
        0,
        SMTO_ABORTIFHUNG | SMTO_BLOCK,
        100,
        &windowIcon
        );

    if (status == 0 || windowIcon == 0)
    {
        status = SendMessageTimeout(
            WindowHandle,
            WM_GETICON,
            0,
            0,
            SMTO_ABORTIFHUNG | SMTO_BLOCK,
            100,
            &windowIcon
            );
    }

    if (status == 0 || windowIcon == 0)
    {
        windowIcon = GetClassLongPtr(WindowHandle, GCLP_HICONSM);

        if (windowIcon == 0)
            windowIcon = GetClassLongPtr(WindowHandle, GCLP_HICON);
    }

    return (HICON)windowIcon;
}

static BOOLEAN WepWindowHasAutomationProvider(
    _In_ HWND WindowHandle
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI *UiaHasServerSideProvider_I)(
        _In_ HWND WindowHandle
        );

    if (PhBeginInitOnce(&initOnce))
    {
        HANDLE baseAddress;

        if (baseAddress = PhLoadLibrary(L"uiautomationcore.dll"))
        {
            UiaHasServerSideProvider_I = PhGetProcedureAddress(baseAddress, "UiaHasServerSideProvider", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!UiaHasServerSideProvider_I)
        return FALSE;

    return !!UiaHasServerSideProvider_I(WindowHandle);
}

static BOOLEAN WepIsWindowCloaked(
    _In_ HWND WindowHandle
    )
{
#define DWMWA_CLOAKED 14
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HRESULT (WINAPI *DwmGetWindowAttribute_I)(
        _In_ HWND hwnd,
        _In_ DWORD dwAttribute,
        _Out_ PVOID pvAttribute,
        _In_ DWORD cbAttribute
        );
    BOOL windowCloaked = FALSE;

    if (PhBeginInitOnce(&initOnce))
    {
        HANDLE baseAddress;

        if (baseAddress = PhLoadLibrary(L"dwmapi.dll"))
        {
            DwmGetWindowAttribute_I = PhGetProcedureAddress(baseAddress, "DwmGetWindowAttribute", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (DwmGetWindowAttribute_I)
    {
        DwmGetWindowAttribute_I(WindowHandle, DWMWA_CLOAKED, &windowCloaked, sizeof(BOOL));
    }

    return !!windowCloaked;
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

static ULONG WepGetWindowBand(
    _In_ HWND WindowHandle
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI *GetWindowBand_I)(
        _In_ HWND hwnd,
        _Out_ PULONG pdwBand
        );
    ULONG windowBand = ULONG_MAX;

    if (PhBeginInitOnce(&initOnce))
    {
        HANDLE baseAddress;

        if (baseAddress = PhLoadLibrary(L"user32.dll"))
        {
            GetWindowBand_I = PhGetProcedureAddress(baseAddress, "GetWindowBand", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    // "Identity and Access Management (IAM)" (dmex)

    if (GetWindowBand_I)
    {
        GetWindowBand_I(WindowHandle, &windowBand);
    }

    return windowBand;
}

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
    _In_ HWND hwndDlg,
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
    HMENU menuHandle;
    PVOID instanceHandle;
    PVOID userdataHandle;
    ULONG windowId;
    ULONG windowExtra;
    HFONT fontHandle;

    menuHandle = GetMenu(Context->WindowHandle);
    instanceHandle = (PVOID)GetWindowLongPtr(Context->WindowHandle, GWLP_HINSTANCE);
    userdataHandle = (PVOID)GetWindowLongPtr(Context->WindowHandle, GWLP_USERDATA);
    windowId = (ULONG)GetWindowLongPtr(Context->WindowHandle, GWLP_ID);
    windowExtra = (ULONG)GetClassLongPtr(Context->WindowHandle, GCL_CBWNDEXTRA); // GetWindowLongPtr
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
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_WNDTEXT, 1, Context->MessageOnlyWindow ? L"N/A" : PhGetStringOrEmpty(PH_AUTO(PhGetWindowText(Context->WindowHandle))));
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_WNDHANDLE, 1, PhaFormatString(L"0x%Ix", (ULONG_PTR)Context->WindowHandle)->Buffer);
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_WNDMSGONLY, 1, Context->MessageOnlyWindow ? L"Yes" : L"No");
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_WNDEXTRA, 1, PhaFormatString(L"%lu bytes (%s)", windowExtra, PhaFormatSize(windowExtra, ULONG_MAX)->Buffer)->Buffer);
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_DLGCTLID, 1, PhaFormatString(L"%lu", windowId)->Buffer);

    if (Context->MessageOnlyWindow)
    {
        PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_FONTNAME, 1, L"N/A");
    }
    else
    {
        if (fontHandle = (HFONT)SendMessage(Context->WindowHandle, WM_GETFONT, 0, 0))
        {
            LOGFONT logFont;

            if (GetObject(fontHandle, sizeof(LOGFONT), &logFont))
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
        WepQueueResolveSymbol(Context, hwndDlg, Context->WndProc, 1);
    }

    if (Context->DlgProc != 0)
    {
        Context->DlgProcResolving++;
        WepQueueResolveSymbol(Context, hwndDlg, Context->DlgProc, 2);
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
        if (WepIsTopLevelWindow(Context->WindowHandle))
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
        if (WepIsWindowCloaked(Context->WindowHandle))
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
        ULONG bandId = WepGetWindowBand(Context->WindowHandle);

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
            if (windowInfo.dwExStyle & WepExtendedStylePairs[i].Integer)
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
    _In_ HWND hwndDlg,
    _In_ HWND ListViewHandle,
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    WCHAR className[256];
    WCHAR classBaseName[256];
    ULONG classExtra;

    if (!GetClassName(Context->WindowHandle, className, RTL_NUMBER_OF(className)))
        className[0] = UNICODE_NULL;
    if (!RealGetWindowClassW(Context->WindowHandle, classBaseName, RTL_NUMBER_OF(classBaseName)))
        classBaseName[0] = UNICODE_NULL;

    Context->ClassInfo.cbSize = sizeof(WNDCLASSEX);
    GetClassInfoEx(NULL, className, &Context->ClassInfo);
    classExtra = (ULONG)GetClassLongPtr(Context->WindowHandle, GCL_CBCLSEXTRA);

    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_NAME, 1, className);
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_BASENAME, 1, classBaseName);
    PhSetListViewSubItem(ListViewHandle, WINDOW_PROPERTIES_INDEX_CLASS_ATOM, 1, PhaFormatString(L"0x%Ix", GetClassLongPtr(Context->WindowHandle, GCW_ATOM))->Buffer);
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
        WepQueueResolveSymbol(Context, hwndDlg, (ULONG_PTR)Context->ClassInfo.lpfnWndProc, 3);
    }

    WepRefreshWindowClassInfoSymbols(ListViewHandle, Context);
}

VOID WepRefreshAutomationProvider(
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    if (Context->MessageOnlyWindow)
    {
        PhSetListViewSubItem(Context->ListViewHandle, WINDOW_PROPERTIES_INDEX_AUTOMATION, 1, L"N/A");
    }
    else
    {
        if (WepWindowHasAutomationProvider(Context->WindowHandle))
            PhSetListViewSubItem(Context->ListViewHandle, WINDOW_PROPERTIES_INDEX_AUTOMATION, 1, L"Yes");
        else
            PhSetListViewSubItem(Context->ListViewHandle, WINDOW_PROPERTIES_INDEX_AUTOMATION, 1, L"No");
    }
}

VOID WepRefreshDpiContext(
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static DPI_AWARENESS_CONTEXT (WINAPI* GetWindowDpiAwarenessContext_I)(_In_ HWND hwnd) = NULL;
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
        PhSetListViewSubItem(Context->ListViewHandle, WINDOW_PROPERTIES_INDEX_DPICONTEXT, 1, L"N/A");
    }
    else
    {
        DPI_AWARENESS_CONTEXT dpiContext = GetWindowDpiAwarenessContext_I(Context->WindowHandle);

        if (AreDpiAwarenessContextsEqual_I(dpiContext, DPI_AWARENESS_CONTEXT_UNAWARE))
        {
            PhSetListViewSubItem(
                Context->ListViewHandle,
                WINDOW_PROPERTIES_INDEX_DPICONTEXT,
                1,
                L"Unaware"
                );
        }
        else if (AreDpiAwarenessContextsEqual_I(dpiContext, DPI_AWARENESS_CONTEXT_SYSTEM_AWARE))
        {
            PhSetListViewSubItem (
                Context->ListViewHandle,
                WINDOW_PROPERTIES_INDEX_DPICONTEXT,
                1,
                L"System aware"
                );
        }
        else if (AreDpiAwarenessContextsEqual_I(dpiContext,DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
        {
            PhSetListViewSubItem (
                Context->ListViewHandle,
                WINDOW_PROPERTIES_INDEX_DPICONTEXT,
                1,
                L"Per-monitor aware"
                );
        }
        else if (AreDpiAwarenessContextsEqual_I(dpiContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
        {
            PhSetListViewSubItem(
                Context->ListViewHandle,
                WINDOW_PROPERTIES_INDEX_DPICONTEXT,
                1,
                L"Per-monitor V2"
                );
        }
        else if (AreDpiAwarenessContextsEqual_I(dpiContext, DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED))
        {
            PhSetListViewSubItem(
                Context->ListViewHandle,
                WINDOW_PROPERTIES_INDEX_DPICONTEXT,
                1,
                L"Unaware (GDI scaled)"
                );
        }
        else
        {
            PhSetListViewSubItem(Context->ListViewHandle, WINDOW_PROPERTIES_INDEX_DPICONTEXT, 1, PhaFormatString(
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
            windowIcon = WepGetInternalWindowIcon(Context->WindowHandle, ICON_BIG);
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
            PhGetStockApplicationIcon(NULL, &windowIcon);
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
        PPH_STRING text = NULL;

        if (!Context->MessageOnlyWindow)
        {
            text = PhGetWindowText(Context->WindowHandle);
        }

        if (!PhIsNullOrEmptyString(text))
        {
            PhSetWindowText(GetDlgItem(WindowHandle, IDC_WINDOWTEXT), PhGetString(text));
        }
        else
        {
            WCHAR className[256];

            if (!GetClassName(Context->WindowHandle, className, RTL_NUMBER_OF(className)))
                className[0] = UNICODE_NULL;

            PhSetWindowText(GetDlgItem(WindowHandle, IDC_WINDOWTEXT), className);
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
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);

    WepWindowRefreshGeneralPageHeader(WindowHandle, Context);
    WepGeneralAddListViewItemGroups(Context->ListViewHandle);
    WepRefreshWindowGeneralInfo(WindowHandle, Context->ListViewHandle, Context);
    WepRefreshWindowStyles(Context->ListViewHandle, Context);
    WepRefreshWindowClassInfo(WindowHandle, Context->ListViewHandle, Context);
    WepRefreshAutomationProvider(Context);
    WepRefreshDpiContext(Context);

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

INT_PTR CALLBACK WepWindowGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWINDOW_PROPERTIES_CONTEXT context;
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    context = (PWINDOW_PROPERTIES_CONTEXT)propPageContext->Context;

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_WINDOWINFO);

            PhSetApplicationWindowIcon(GetParent(hwndDlg));

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 180, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Value");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_WINDOWS_PROPERTY_COLUMNS, context->ListViewHandle);

            WepWindowRefreshGeneralPage(hwndDlg, context);

            if (!PhGetIntegerPairSetting(SETTING_NAME_WINDOWS_PROPERTY_POSITION).X) // HACK
            {
                PhCenterWindow(GetParent(hwndDlg), context->ParentWindowHandle);
                ExtendedListView_SetColumnWidth(context->ListViewHandle, 1, ELVSCW_AUTOSIZE_REMAININGSPACE);
            }

            if (!!PhGetIntegerSetting(L"EnableThemeSupport")) // TODO: Required for compat (dmex)
                PhInitializeWindowTheme(GetParent(hwndDlg), !!PhGetIntegerSetting(L"EnableThemeSupport"));
            else
                PhInitializeWindowTheme(hwndDlg, FALSE);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(SETTING_NAME_WINDOWS_PROPERTY_COLUMNS, context->ListViewHandle);

            if (context->WindowIcon)
            {
                DestroyIcon(context->WindowIcon);
            }
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_WINDOWGROUPBOX), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_WINDOWTEXT), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_APPIDTEXT), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, context->ListViewHandle, dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

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

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
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
            else if (resolveContext->Id == 3)
            {
                PhAcquireQueuedLockExclusive(&context->ResolveListLock);
                RemoveEntryList(&resolveContext->ListEntry);
                PhReleaseQueuedLockExclusive(&context->ResolveListLock);

                if (resolveContext->ResolveLevel != PhsrlModule && resolveContext->ResolveLevel != PhsrlFunction)
                    PhClearReference(&resolveContext->Symbol);

                PhMoveReference(&context->ClassWndProcSymbol, resolveContext->Symbol);
                PhFree(resolveContext);

                context->ClassWndProcResolving--;
            }

            WepRefreshWindowGeneralInfoSymbols(context->ListViewHandle, context);
            WepRefreshWindowClassInfoSymbols(context->ListViewHandle, context);
        }
        break;
    case WM_PH_UPDATE_DIALOG:
        {
            WepWindowRefreshGeneralPage(hwndDlg, context);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)GetStockBrush(DC_BRUSH);
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    static PH_LAYOUT_MANAGER LayoutManager;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PWEP_WINDOW_PROPEDIT_CONTEXT context = (PWEP_WINDOW_PROPEDIT_CONTEXT)lParam;

            PhSetApplicationWindowIcon(hwndDlg);

            PhSetWindowText(hwndDlg, L"Property Editor");
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_NAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_VALUE), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (!context->WindowPropCreate)
            {
                WCHAR value[PH_INT64_STR_LEN_1];

                PhPrintPointer(value, (PVOID)GetProp(context->TargetWindowHandle, PhGetString(context->WindowPropString)));
                PhSetDialogItemText(hwndDlg, IDC_NAME, PhGetString(context->WindowPropString));
                PhSetDialogItemText(hwndDlg, IDC_VALUE, value);

                EnableWindow(GetDlgItem(hwndDlg, IDC_NAME), FALSE);
            }

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDCANCEL));

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    PWEP_WINDOW_PROPEDIT_CONTEXT context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
                    PPH_STRING windowPropName = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_NAME)));
                    PPH_STRING windowPropValue = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_VALUE)));
                    ULONG64 value = 0;

                    PhStringToInteger64(&windowPropValue->sr, 0, &value);

                    if (!context->WindowPropCreate && PhIsNullOrEmptyString(windowPropName))
                    {
                        PhShowError2(hwndDlg, L"Unable to add window property.", L"%s", L"The property name is empty.");
                        break;
                    }

                    if (context->WindowPropCreate)
                    {
                        if (!SetProp(context->TargetWindowHandle, PhGetString(windowPropName), (HANDLE)value))
                        {
                            PhShowStatus(hwndDlg, L"Unable to create the window property.", 0, GetLastError());
                            break;
                        }
                    }
                    else
                    {
                        if (!SetProp(context->TargetWindowHandle, PhGetString(context->WindowPropString), (HANDLE)value))
                        {
                            PhShowStatus(hwndDlg, L"Unable to update the window property.", 0, GetLastError());
                            break;
                        }
                    }

                    EndDialog(hwndDlg, IDOK);
                }
                break;
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

VOID WepFreeListViewWindowProps(
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    INT index = INT_ERROR;

    while ((index = PhFindListViewItemByFlags(Context->ListViewHandle, index, LVNI_ALL)) != INT_ERROR)
    {
        PPH_STRING param;

        if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
        {
            PhClearReference(&param);
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
    PWINDOW_PROPERTIES_CONTEXT context = (PWINDOW_PROPERTIES_CONTEXT)Context;
    INT lvItemIndex;
    PPH_STRING propName;
    WCHAR value[PH_INT64_STR_LEN_1];

    if (IS_INTRESOURCE(Name))
        propName = PhFormatString(L"#%hu", PtrToUshort(Name));
    else
        propName = PhCreateString(Name);

    PhPrintUInt32(value, ++context->PropsListCount);
    lvItemIndex = PhAddListViewItem(context->PropsListViewHandle, MAXINT, value, propName);

    if (IS_INTRESOURCE(Name)) // This is an integer atom.
        PhSetListViewSubItem(context->PropsListViewHandle, lvItemIndex, 1, PhGetString(propName));
    else
        PhSetListViewSubItem(context->PropsListViewHandle, lvItemIndex, 1, Name);

    PhPrintPointer(value, (PVOID)Value);
    PhSetListViewSubItem(context->PropsListViewHandle, lvItemIndex, 2, value);

    if (!IS_INTRESOURCE(Name))
    {
        PROPERTYKEY propkey;
        PWSTR propKeyName;

        propName = PhCreateString(Name);

        if (HR_SUCCESS(PSGetPropertyKeyFromName(PhGetString(propName), &propkey)))
        {
            if (HR_SUCCESS(PSGetNameFromPropertyKey(&propkey, &propKeyName)))
            {
                PhSetListViewSubItem(context->PropsListViewHandle, lvItemIndex, 3, propKeyName);
                CoTaskMemFree(propKeyName);
            }
        }
    }

    return TRUE;
}

VOID WepRefreshWindowProps(
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    WepFreeListViewWindowProps(Context);
    Context->PropsListCount = 0;

    ExtendedListView_SetRedraw(Context->PropsListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->PropsListViewHandle);

    EnumPropsEx(Context->WindowHandle, WepEnumPropsExCallback, (LPARAM)Context);

    ExtendedListView_SortItems(Context->PropsListViewHandle);
    ExtendedListView_SetRedraw(Context->PropsListViewHandle, TRUE);
}

INT_PTR CALLBACK WepWindowPropListDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWINDOW_PROPERTIES_CONTEXT context;
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    context = (PWINDOW_PROPERTIES_CONTEXT)propPageContext->Context;

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->PropsListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->PropsListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->PropsListViewHandle, L"explorer");
            PhAddListViewColumn(context->PropsListViewHandle, 0, 0, 0, LVCFMT_LEFT, 80, L"#");
            PhAddListViewColumn(context->PropsListViewHandle, 1, 1, 1, LVCFMT_LEFT, 160, L"Name");
            PhAddListViewColumn(context->PropsListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"Value");
            PhAddListViewColumn(context->PropsListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Alias");
            PhSetExtendedListView(context->PropsListViewHandle);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_WINDOWS_PROPLIST_COLUMNS, context->PropsListViewHandle);

            WepRefreshWindowProps(context);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(SETTING_NAME_WINDOWS_PROPLIST_COLUMNS, context->PropsListViewHandle);

            WepFreeListViewWindowProps(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, context->PropsListViewHandle, dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->PropsListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->PropsListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->PropsListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->PropsListViewHandle, &listviewItems, &numberOfItems);

                menu = PhCreateEMenu();
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_ADD, L"Add", NULL, NULL), ULONG_MAX);

                if (numberOfItems != 0)
                {
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDD_EDITENV, L"Edit", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_DELETE, L"Delete", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->PropsListViewHandle);
                }

                item = PhShowEMenu(
                    menu,
                    hwndDlg,
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
                                windowPropEditContext.TargetWindowHandle = context->WindowHandle;
                                windowPropEditContext.WindowPropCreate = TRUE;

                                PhDialogBox(
                                    PluginInstance->DllBase,
                                    MAKEINTRESOURCE(IDD_WNDPROPEDIT),
                                    hwndDlg,
                                    WepWindowPropEditDlgProc,
                                    &windowPropEditContext
                                    );

                                //WepRefreshWindowProps(context);
                                PvRefreshChildWindows(hwndDlg);
                            }
                            break;
                        case PHAPP_IDD_EDITENV:
                            {
                                WEP_WINDOW_PROPEDIT_CONTEXT windowPropEditContext;

                                memset(&windowPropEditContext, 0, sizeof(WEP_WINDOW_PROPEDIT_CONTEXT));
                                windowPropEditContext.TargetWindowHandle = context->WindowHandle;
                                windowPropEditContext.WindowPropString = listviewItems[0];

                                PhDialogBox(
                                    PluginInstance->DllBase,
                                    MAKEINTRESOURCE(IDD_WNDPROPEDIT),
                                    hwndDlg,
                                    WepWindowPropEditDlgProc,
                                    &windowPropEditContext
                                    );

                                //WepRefreshWindowProps(context);
                                PvRefreshChildWindows(hwndDlg);
                            }
                            break;
                        case PHAPP_IDC_DELETE:
                            {
                                if (PhGetIntegerSetting(L"EnableWarnings") && !PhShowConfirmMessage(
                                    hwndDlg,
                                    L"remove",
                                    L"the window property",
                                    L"The window property will be permanently deleted.",
                                    FALSE
                                    ))
                                {
                                    break;
                                }

                                RemoveProp(context->WindowHandle, PhGetString(listviewItems[0]));

                                ULONG status = GetLastError();
                                if (status != ERROR_SUCCESS)
                                    PhShowStatus(hwndDlg, L"Unable to remove the window property.", 0, status);

                                //WepRefreshWindowProps(context);
                                PvRefreshChildWindows(hwndDlg);
                            }
                            break;
                        case PHAPP_IDC_COPY:
                            {
                                PhCopyListView(context->PropsListViewHandle);
                            }
                            break;
                        }
                    }
                }

                PhDestroyEMenu(menu);

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
    _In_ PWINDOW_PROPERTIES_CONTEXT Context
    )
{
    IPropertyStore *propstore;
    ULONG count;
    ULONG i;

    ExtendedListView_SetRedraw(Context->PropStoreListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->PropStoreListViewHandle);

    if (SUCCEEDED(SHGetPropertyStoreForWindow(Context->WindowHandle, &IID_IPropertyStore, &propstore)))
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
                    lvItemIndex = PhAddListViewItem(Context->PropStoreListViewHandle, MAXINT, value, NULL);

                    if (SUCCEEDED(PSGetNameFromPropertyKey(&propkey, &propKeyName)))
                    {
                        PhSetListViewSubItem(Context->PropStoreListViewHandle, lvItemIndex, 1, propKeyName);
                        CoTaskMemFree(propKeyName);
                    }
                    else
                    {
                        WCHAR propKeyString[PKEYSTR_MAX];

                        if (SUCCEEDED(PSStringFromPropertyKey(&propkey, propKeyString, RTL_NUMBER_OF(propKeyString))))
                            PhSetListViewSubItem(Context->PropStoreListViewHandle, lvItemIndex, 1, propKeyString);
                        else
                            PhSetListViewSubItem(Context->PropStoreListViewHandle, lvItemIndex, 1, L"Unknown");
                    }

                    if (SUCCEEDED(IPropertyStore_GetValue(propstore, &propkey, &propKeyVariant)))
                    {
                        if (SUCCEEDED(PSFormatForDisplayAlloc(&propkey, &propKeyVariant, PDFF_DEFAULT, &propKeyName)))
                        {
                            PhSetListViewSubItem(Context->PropStoreListViewHandle, lvItemIndex, 2, propKeyName);
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

    ExtendedListView_SortItems(Context->PropStoreListViewHandle);
    ExtendedListView_SetRedraw(Context->PropStoreListViewHandle, TRUE);
}

INT_PTR CALLBACK WepWindowPropStoreDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWINDOW_PROPERTIES_CONTEXT context;
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    context = (PWINDOW_PROPERTIES_CONTEXT)propPageContext->Context;

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->PropStoreListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->PropStoreListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->PropStoreListViewHandle, L"explorer");
            PhAddListViewColumn(context->PropStoreListViewHandle, 0, 0, 0, LVCFMT_LEFT, 80, L"#");
            PhAddListViewColumn(context->PropStoreListViewHandle, 1, 1, 1, LVCFMT_LEFT, 160, L"Name");
            PhAddListViewColumn(context->PropStoreListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"Value");
            PhSetExtendedListView(context->PropStoreListViewHandle);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_WINDOWS_PROPSTORAGE_COLUMNS, context->PropStoreListViewHandle);

            WepRefreshWindowPropertyStorage(context);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(SETTING_NAME_WINDOWS_PROPSTORAGE_COLUMNS, context->PropStoreListViewHandle);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, context->PropStoreListViewHandle, dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->PropStoreListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->PropStoreListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->PropStoreListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->PropStoreListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->PropStoreListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
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
                                    PhCopyListView(context->PropStoreListViewHandle);
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

// https://docs.microsoft.com/en-us/windows/win32/dwm/dwmdxgetwindowsharedsurface
HRESULT (WINAPI* DwmDxGetWindowSharedSurface_I)(
    _In_ HWND WindowHandle,
    _In_ LUID luidAdapter,
    _In_opt_ HMONITOR hmonitorAssociation,
    _In_ ULONG Flags,
    _Inout_ ULONG* pfmtWindow, // DXGI_FORMAT
    _Out_ HANDLE* phDxSurface,
    _Out_ UINT64* puiUpdateId
    );

// https://docs.microsoft.com/en-us/windows/win32/dwm/dwmdxupdatewindowsharedsurface
HRESULT (WINAPI* DwmDxUpdateWindowSharedSurface_I)(
    _In_ HWND WindowHandle,
    _In_ UINT64 uiUpdateId,
    _In_ ULONG dwFlags,
    _In_opt_ HMONITOR hmonitorAssociation,
    _In_ RECT* prc
    );

// rev
BOOL (WINAPI* DwmGetDxSharedSurface_I)(
    _In_ HWND WindowHandle,
    _Out_ HANDLE* phDxSurface,
    _Out_ LUID* pluidAdapter,
    _Out_ ULONG* pfmtWindow, // DXGI_FORMAT
    _Out_ ULONG* pPresentFlags,
    _Out_ UINT64* puiUpdateId
    );

// rev
HBITMAP (WINAPI* CreateBitmapFromDxSurface_I)(
    _In_ HDC hdc,
    _In_ UINT32 Width,
    _In_ UINT32 Height,
    _In_ ULONG pfmtWindow, // DXGI_FORMAT
    _In_opt_ HANDLE phDxSurface
    );

HBITMAP WepGetWindowSharedSurface(
    _In_ HWND WindowHandle,
    _In_ HDC WindowHdc
    )
{
    HBITMAP bitmapHandle = NULL;
    HANDLE surfaceHandle = NULL;
    LUID adapterLuid;
    ULONG bitmapFormat = 0;
    ULONG presentFlags = 0;
    UINT64 updateId = 0;

    if (!DwmGetDxSharedSurface_I)
        DwmGetDxSharedSurface_I = PhGetModuleProcAddress(L"user32.dll", "DwmGetDxSharedSurface");
    if (!CreateBitmapFromDxSurface_I)
        CreateBitmapFromDxSurface_I = PhGetModuleProcAddress(L"gdi32.dll", "CreateBitmapFromDxSurface");

    if (!DwmGetDxSharedSurface_I)
        return NULL;
    if (!CreateBitmapFromDxSurface_I)
        return NULL;

    if (DwmGetDxSharedSurface_I(
        WindowHandle,
        &surfaceHandle,
        &adapterLuid,
        &bitmapFormat,
        &presentFlags,
        &updateId
        ))
    {
        RECT windowRect;

        GetWindowRect(WindowHandle, &windowRect); // TODO: Include dwm window borders.

        bitmapHandle = CreateBitmapFromDxSurface_I(
            WindowHdc,
            windowRect.right,
            windowRect.bottom,
            bitmapFormat,
            surfaceHandle
            );
    }

    return bitmapHandle;
}

INT_PTR CALLBACK WepWindowPreviewDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWINDOW_PROPERTIES_CONTEXT context;
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    context = (PWINDOW_PROPERTIES_CONTEXT)propPageContext->Context;

    if (!context)
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

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            PhSetTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT, 1000, NULL);
        }
        break;
    case WM_DESTROY:
        {
            PhKillTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST), dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC hdc;
            HDC bufferDc;
            RECT clientRect;
            RECT windowClientRect;
            HBITMAP bitmap;

            GetClientRect(hwndDlg, &clientRect);
            GetClientRect(context->WindowHandle, &windowClientRect);

            hdc = BeginPaint(hwndDlg, &paint);
            bufferDc = CreateCompatibleDC(hdc);

            SetBkMode(hdc, TRANSPARENT);
            FillRect(hdc, &clientRect, GetStockBrush(BLACK_BRUSH));

            if (bitmap = WepGetWindowSharedSurface(context->WindowHandle, hdc))
            {
                HBITMAP oldbitmap;
                INT width;
                INT height;

                oldbitmap = SelectBitmap(bufferDc, bitmap);

                if (clientRect.right > windowClientRect.right)
                    width = windowClientRect.right;
                else
                    width = clientRect.right;

                if (clientRect.bottom > windowClientRect.bottom)
                    height = windowClientRect.bottom;
                else
                    height = clientRect.bottom;

                StretchBlt(
                    hdc,
                    0, 0,
                    width,
                    height,
                    bufferDc,
                    0, 0,
                    windowClientRect.right,
                    windowClientRect.bottom,
                    SRCCOPY
                    );

                SelectBitmap(bufferDc, oldbitmap);
                DeleteBitmap(bitmap);
            }
            else
            {
                PH_STRINGREF errorText = PH_STRINGREF_INIT(L"No preview available for this window.");

                SetTextColor(hdc, RGB(0x0, 0x0, 0xFF));
                //SelectFont(hdc, PhApplicationFont);
                DrawText(
                    hdc,
                    errorText.Buffer,
                    (INT)errorText.Length / sizeof(WCHAR),
                    &clientRect,
                    DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE
                    );
            }

            DeleteDC(bufferDc);

            EndPaint(hwndDlg, &paint);
        }
        return TRUE;
    case WM_TIMER:
        {
            InvalidateRect(hwndDlg, NULL, TRUE);
        }
        break;
    }

    return FALSE;
}
