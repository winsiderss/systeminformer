/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "agenttools.h"
#include <mapldr.h>

// Windows across the whole desktop, rather than get_process_windows' one process. What this is
// for: finding the window that is not responding, finding which process owns a dialog nobody can
// place, and telling a window that is genuinely on screen from one that is merely not hidden.
//
// Enumeration follows WindowExplorer: PhEnumWindowsEx rather than EnumWindows, because EnumWindows
// does not return the windows of packaged applications, and message-only windows are a separate
// tree that has to be walked on its own.

typedef enum _AT_WINDOW_SCOPE
{
    AtWindowScopeTopLevel,
    AtWindowScopeAll,
    AtWindowScopeMessageOnly
} AT_WINDOW_SCOPE;

typedef struct _AT_WINDOW_LIST_CONTEXT
{
    AT_ROWS Rows;
    AT_WINDOW_SCOPE Scope;
    BOOLEAN VisibleOnly;
    HANDLE ProcessId;
    BOOLEAN HaveProcessId;
    PPH_STRING TitleContains;
    PPH_STRING ClassName;
    ULONG ZOrder;
} AT_WINDOW_LIST_CONTEXT, *PAT_WINDOW_LIST_CONTEXT;

// A window can be WS_VISIBLE and still not be on screen: the shell cloaks the windows of suspended
// packaged applications and of virtual desktops other than the current one. Without this a triage
// tool reports a dozen visible windows that nobody can see.
BOOLEAN AtpIsWindowCloaked(
    _In_ HWND WindowHandle,
    _Out_ PBOOLEAN Cloaked
    )
{
    static HRESULT (WINAPI *dwmGetWindowAttribute)(HWND, DWORD, PVOID, DWORD) = NULL;
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    ULONG cloaked = 0;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID dwmapi;

        if (dwmapi = PhLoadLibrary(L"dwmapi.dll"))
            dwmGetWindowAttribute = PhGetProcedureAddress(dwmapi, "DwmGetWindowAttribute", 0);

        PhEndInitOnce(&initOnce);
    }

    if (!dwmGetWindowAttribute)
        return FALSE;

    // DWMWA_CLOAKED
    if (HR_FAILED(dwmGetWindowAttribute(WindowHandle, 14, &cloaked, sizeof(cloaked))))
        return FALSE;

    *Cloaked = !!cloaked;

    return TRUE;
}

VOID AtpAddWindowRect(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PRECT Rect
    )
{
    PVOID entry;

    entry = PhCreateJsonObject();
    PhAddJsonObjectInt64(entry, "left", Rect->left);
    PhAddJsonObjectInt64(entry, "top", Rect->top);
    PhAddJsonObjectInt64(entry, "right", Rect->right);
    PhAddJsonObjectInt64(entry, "bottom", Rect->bottom);
    PhAddJsonObjectInt64(entry, "width", (LONG)(Rect->right - Rect->left));
    PhAddJsonObjectInt64(entry, "height", (LONG)(Rect->bottom - Rect->top));
    PhAddJsonObjectValue(Object, Key, entry);
}

VOID AtpAddWindowIdentity(
    _In_ PVOID Object,
    _In_ HWND WindowHandle
    )
{
    CLIENT_ID clientId;
    PPH_PROCESS_ITEM processItem;
    PPH_STRING text;
    WCHAR className[256];

    clientId.UniqueProcess = NULL;
    clientId.UniqueThread = UlongToHandle(GetWindowThreadProcessId(WindowHandle, (PDWORD)&clientId.UniqueProcess));

    AtJsonAddPointer(Object, "handle", WindowHandle);
    PhAddJsonObjectUInt64(Object, "pid", HandleToUlong(clientId.UniqueProcess));
    PhAddJsonObjectUInt64(Object, "tid", HandleToUlong(clientId.UniqueThread));

    if (processItem = PhReferenceProcessItem(clientId.UniqueProcess))
    {
        AtJsonAddString(Object, "process_name", processItem->ProcessName);
        PhDereferenceObject(processItem);
    }
    else
    {
        AtJsonAddNull(Object, "process_name");
    }

    text = PhGetWindowText(WindowHandle);
    AtJsonAddString(Object, "title", text);
    PhClearReference(&text);

    if (NT_SUCCESS(PhGetClassName(WindowHandle, className, RTL_NUMBER_OF(className), NULL)))
        AtJsonAddStringZ(Object, "class_name", className);
    else
        AtJsonAddNull(Object, "class_name");
}

VOID AtpAddWindowState(
    _In_ PVOID Object,
    _In_ HWND WindowHandle
    )
{
    RECT rect;
    BOOLEAN cloaked;

    PhAddJsonObjectBoolean(Object, "is_visible", !!IsWindowVisible(WindowHandle));
    PhAddJsonObjectBoolean(Object, "is_minimized", !!IsIconic(WindowHandle));
    PhAddJsonObjectBoolean(Object, "is_maximized", !!IsZoomed(WindowHandle));
    PhAddJsonObjectBoolean(Object, "is_enabled", !!IsWindowEnabled(WindowHandle));
    PhAddJsonObjectBoolean(Object, "is_hung", !!IsHungAppWindow(WindowHandle));

    if (AtpIsWindowCloaked(WindowHandle, &cloaked))
        PhAddJsonObjectBoolean(Object, "is_cloaked", cloaked);
    else
        AtJsonAddNull(Object, "is_cloaked");

    if (GetWindowRect(WindowHandle, &rect))
        AtpAddWindowRect(Object, "rect", &rect);
    else
        AtJsonAddNull(Object, "rect");
}

BOOLEAN AtpWindowMatches(
    _In_ PAT_WINDOW_LIST_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    HANDLE processId = NULL;

    if (Context->VisibleOnly && !IsWindowVisible(WindowHandle))
        return FALSE;

    if (Context->HaveProcessId)
    {
        GetWindowThreadProcessId(WindowHandle, (PDWORD)&processId);

        if (processId != Context->ProcessId)
            return FALSE;
    }

    if (Context->ClassName)
    {
        WCHAR className[256];

        if (!NT_SUCCESS(PhGetClassName(WindowHandle, className, RTL_NUMBER_OF(className), NULL)))
            return FALSE;

        if (!PhEqualStringZ(className, Context->ClassName->Buffer, TRUE))
            return FALSE;
    }

    if (Context->TitleContains)
    {
        PPH_STRING text = PhGetWindowText(WindowHandle);
        BOOLEAN matched = AtContainsString(text, Context->TitleContains);

        PhClearReference(&text);

        if (!matched)
            return FALSE;
    }

    return TRUE;
}

VOID AtpAddWindowRow(
    _In_ PAT_WINDOW_LIST_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ ULONG ZOrder
    )
{
    PVOID row;

    if (!AtpWindowMatches(Context, WindowHandle))
        return;

    row = PhCreateJsonObject();
    AtpAddWindowIdentity(row, WindowHandle);
    AtpAddWindowState(row, WindowHandle);

    // The enumeration order is the z-order, so this is the position among the windows the
    // enumeration walked, before any filter dropped rows. Zero is the topmost.
    PhAddJsonObjectUInt64(row, "z_order", ZOrder);

    AtAddRow(&Context->Rows, row);
}

VOID AtpEnumerateChildWindows(
    _In_ PAT_WINDOW_LIST_CONTEXT Context,
    _In_opt_ HWND ParentHandle
    )
{
    HWND child = NULL;
    ULONG i = 0;

    // FindWindowEx rather than GetWindow(GW_CHILD): the message-only parent has no child chain to
    // walk, so GetWindow reports that this machine has no message-only windows at all, which is
    // never true. The iteration cap is against a list that changes while it is being walked.
    while (i < 0x4000 && (child = FindWindowEx(ParentHandle, child, NULL, NULL)))
    {
        AtpAddWindowRow(Context, child, Context->ZOrder++);
        AtpEnumerateChildWindows(Context, child);
        i++;
    }
}

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN NTAPI AtpListWindowsCallback(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    PAT_WINDOW_LIST_CONTEXT context = Context;

    if (!context)
        return FALSE;

    AtpAddWindowRow(context, WindowHandle, context->ZOrder++);

    if (context->Scope == AtWindowScopeAll)
        AtpEnumerateChildWindows(context, WindowHandle);

    return TRUE;
}

VOID AtpListWindows(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_WINDOW_LIST_CONTEXT context;
    PPH_STRING scope;
    PVOID visibleMember;
    PVOID structured;
    ULONG64 processId;

    memset(&context, 0, sizeof(AT_WINDOW_LIST_CONTEXT));
    context.Scope = AtWindowScopeTopLevel;
    context.VisibleOnly = TRUE;
    AtInitializeRows(&context.Rows, Call->Arguments);

    if (scope = AtGetArgumentString(Call->Arguments, "scope"))
    {
        if (PhEqualString2(scope, L"all", TRUE))
            context.Scope = AtWindowScopeAll;
        else if (PhEqualString2(scope, L"message_only", TRUE))
            context.Scope = AtWindowScopeMessageOnly;
        else if (!PhEqualString2(scope, L"top_level", TRUE))
        {
            AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"scope must be top_level, all or message_only.");
            PhDereferenceObject(scope);
            AtDeleteRows(&context.Rows);
            return;
        }

        PhDereferenceObject(scope);
    }

    if (visibleMember = AtJsonGetObjectMember(Call->Arguments, "visible_only", PH_JSON_OBJECT_TYPE_BOOLEAN))
        context.VisibleOnly = AtJsonGetObjectBoolean(Call->Arguments, "visible_only");

    if (AtGetArgumentUInt64(Call->Arguments, "pid", &processId))
    {
        context.HaveProcessId = TRUE;
        context.ProcessId = UlongToHandle((ULONG)processId);
    }

    context.TitleContains = AtGetArgumentString(Call->Arguments, "title_contains");
    context.ClassName = AtGetArgumentString(Call->Arguments, "class_name");

    if (context.Scope == AtWindowScopeMessageOnly)
    {
        // Message-only windows are children of a dedicated parent and are never reached by the
        // desktop enumeration; a process that only owns one has no windows as far as anything
        // else here is concerned.
        AtpEnumerateChildWindows(&context, HWND_MESSAGE);
    }
    else
    {
        PhEnumWindowsEx(NULL, AtpListWindowsCallback, &context);
    }

    structured = PhCreateJsonObject();
    AtAddRows(structured, "windows", &context.Rows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&context.Rows);
    PhClearReference(&context.TitleContains);
    PhClearReference(&context.ClassName);
}

VOID AtpAddWindowStyles(
    _In_ PVOID Object,
    _In_ HWND WindowHandle
    )
{
    LONG_PTR style;
    LONG_PTR exStyle;
    PVOID styles;
    PVOID exStyles;
    BOOLEAN isChild;

    style = GetWindowLongPtr(WindowHandle, GWL_STYLE);
    exStyle = GetWindowLongPtr(WindowHandle, GWL_EXSTYLE);

    AtJsonAddHex(Object, "style", (ULONG)style);
    AtJsonAddHex(Object, "extended_style", (ULONG)exStyle);

    isChild = !!FlagOn(style, WS_CHILD);

    styles = PhCreateJsonObject();
    PhAddJsonObjectBoolean(styles, "child", isChild);
    PhAddJsonObjectBoolean(styles, "popup", !!FlagOn(style, WS_POPUP));
    PhAddJsonObjectBoolean(styles, "visible", !!FlagOn(style, WS_VISIBLE));
    PhAddJsonObjectBoolean(styles, "disabled", !!FlagOn(style, WS_DISABLED));
    PhAddJsonObjectBoolean(styles, "minimized", !!FlagOn(style, WS_MINIMIZE));
    PhAddJsonObjectBoolean(styles, "maximized", !!FlagOn(style, WS_MAXIMIZE));
    PhAddJsonObjectBoolean(styles, "border", !!FlagOn(style, WS_BORDER));
    PhAddJsonObjectBoolean(styles, "caption", (style & WS_CAPTION) == WS_CAPTION);
    PhAddJsonObjectBoolean(styles, "system_menu", !!FlagOn(style, WS_SYSMENU));
    PhAddJsonObjectBoolean(styles, "sizing_frame", !!FlagOn(style, WS_THICKFRAME));
    PhAddJsonObjectBoolean(styles, "clip_children", !!FlagOn(style, WS_CLIPCHILDREN));
    PhAddJsonObjectBoolean(styles, "clip_siblings", !!FlagOn(style, WS_CLIPSIBLINGS));

    // WS_MINIMIZEBOX and WS_MAXIMIZEBOX are the same two bits as WS_GROUP and WS_TABSTOP; which
    // pair a bit means depends on whether the window is a child. Reporting all four would be
    // wrong about two of them for every window.
    if (isChild)
    {
        PhAddJsonObjectBoolean(styles, "group", !!FlagOn(style, WS_GROUP));
        PhAddJsonObjectBoolean(styles, "tab_stop", !!FlagOn(style, WS_TABSTOP));
        AtJsonAddNull(styles, "minimize_box");
        AtJsonAddNull(styles, "maximize_box");
    }
    else
    {
        AtJsonAddNull(styles, "group");
        AtJsonAddNull(styles, "tab_stop");
        PhAddJsonObjectBoolean(styles, "minimize_box", !!FlagOn(style, WS_MINIMIZEBOX));
        PhAddJsonObjectBoolean(styles, "maximize_box", !!FlagOn(style, WS_MAXIMIZEBOX));
    }

    PhAddJsonObjectValue(Object, "styles", styles);

    exStyles = PhCreateJsonObject();
    PhAddJsonObjectBoolean(exStyles, "topmost", !!FlagOn(exStyle, WS_EX_TOPMOST));
    PhAddJsonObjectBoolean(exStyles, "tool_window", !!FlagOn(exStyle, WS_EX_TOOLWINDOW));
    PhAddJsonObjectBoolean(exStyles, "app_window", !!FlagOn(exStyle, WS_EX_APPWINDOW));
    PhAddJsonObjectBoolean(exStyles, "transparent", !!FlagOn(exStyle, WS_EX_TRANSPARENT));
    PhAddJsonObjectBoolean(exStyles, "layered", !!FlagOn(exStyle, WS_EX_LAYERED));
    PhAddJsonObjectBoolean(exStyles, "no_activate", !!FlagOn(exStyle, WS_EX_NOACTIVATE));
    PhAddJsonObjectBoolean(exStyles, "accept_files", !!FlagOn(exStyle, WS_EX_ACCEPTFILES));
    PhAddJsonObjectBoolean(exStyles, "mdi_child", !!FlagOn(exStyle, WS_EX_MDICHILD));
    PhAddJsonObjectBoolean(exStyles, "client_edge", !!FlagOn(exStyle, WS_EX_CLIENTEDGE));
    PhAddJsonObjectBoolean(exStyles, "no_redirection_bitmap", !!FlagOn(exStyle, WS_EX_NOREDIRECTIONBITMAP));
    PhAddJsonObjectValue(Object, "extended_styles", exStyles);
}

VOID AtpGetWindowInfo(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    ULONG64 handleValue;
    HWND windowHandle;
    HWND relative;
    PVOID structured;
    WINDOWPLACEMENT placement;
    RECT rect;
    ULONG childCount = 0;
    HWND child;

    if (!AtGetArgumentPointer(Call->Arguments, "handle", &handleValue) || handleValue == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"handle is required.");
        return;
    }

    windowHandle = (HWND)(ULONG_PTR)handleValue;

    if (!IsWindow(windowHandle))
    {
        AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"That window handle does not name a window.");
        return;
    }

    structured = PhCreateJsonObject();
    AtpAddWindowIdentity(structured, windowHandle);
    AtpAddWindowState(structured, windowHandle);
    AtpAddWindowStyles(structured, windowHandle);

    PhAddJsonObjectBoolean(structured, "is_unicode", !!IsWindowUnicode(windowHandle));

    if (relative = GetAncestor(windowHandle, GA_PARENT))
        AtJsonAddPointer(structured, "parent", relative);
    else
        AtJsonAddNull(structured, "parent");

    if (relative = GetWindow(windowHandle, GW_OWNER))
        AtJsonAddPointer(structured, "owner", relative);
    else
        AtJsonAddNull(structured, "owner");

    AtJsonAddPointer(structured, "root", GetAncestor(windowHandle, GA_ROOT));

    for (child = GetWindow(windowHandle, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT))
        childCount++;

    PhAddJsonObjectUInt64(structured, "child_count", childCount);

    if (GetClientRect(windowHandle, &rect))
        AtpAddWindowRect(structured, "client_rect", &rect);
    else
        AtJsonAddNull(structured, "client_rect");

    memset(&placement, 0, sizeof(WINDOWPLACEMENT));
    placement.length = sizeof(WINDOWPLACEMENT);

    // Where the window sits when it is neither minimised nor maximised, which is the position it
    // will be restored to and is not the same as its current rectangle.
    if (GetWindowPlacement(windowHandle, &placement))
        AtpAddWindowRect(structured, "restore_rect", &placement.rcNormalPosition);
    else
        AtJsonAddNull(structured, "restore_rect");

    PhAddJsonObjectUInt64(structured, "dpi", PhGetWindowDpi(windowHandle));
    PhAddJsonObjectUInt64(structured, "control_id", (ULONG)GetWindowLongPtr(windowHandle, GWLP_ID));

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

VOID AtWindowInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionListWindows:
        AtpListWindows(Call, Result);
        break;
    case AtActionGetWindowInfo:
        AtpGetWindowInfo(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
