/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2025
 *
 */

#ifndef _PH_GRAPHBAR_H
#define _PH_GRAPHBAR_H

EXTERN_C_START

#define PH_GRAPHBAR_CLASSNAME L"PhGraphBar"

// Styles

#define PH_GRAPHBAR_STYLE_LEGEND 0x0001 // Show legend under bar
#define PH_GRAPHBAR_STYLE_SEPARATORS 0x0002 // 1px separators between segments
#define PH_GRAPHBAR_STYLE_NO_BORDER 0x0004 // No outer rounded border

typedef _Function_class_(PH_GRAPHBAR_MESSAGE_CALLBACK)
BOOLEAN NTAPI PH_GRAPHBAR_MESSAGE_CALLBACK(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );
typedef PH_GRAPHBAR_MESSAGE_CALLBACK* PPH_GRAPHBAR_MESSAGE_CALLBACK;

typedef struct _PH_GRAPHBAR_CREATEPARAMS
{
    ULONG Size;
    PPH_GRAPHBAR_MESSAGE_CALLBACK Callback;
    PVOID Context;
} PH_GRAPHBAR_CREATEPARAMS, *PPH_GRAPHBAR_CREATEPARAMS;

typedef struct _PH_GRAPHBAR_MEMORY
{
    ULONG64 HardwareReserved;
    ULONG64 InUse;
    ULONG64 Modified;
    ULONG64 Standby;
    ULONG64 Free;
} PH_GRAPHBAR_MEMORY, *PPH_GRAPHBAR_MEMORY;

typedef struct _PH_GRAPHBAR_SEGMENT
{
    ULONG64 Value;
    COLORREF Color;
    PCWSTR Name;
} PH_GRAPHBAR_SEGMENT, *PPH_GRAPHBAR_SEGMENT;

typedef struct _PH_GRAPHBAR_CONTEXT
{
    HWND WindowHandle;
    HWND ParentHandle;
    HWND TooltipHandle;
    ULONG_PTR Id;
    LONG Style;
    LONG EnableRedraw;
    BOOLEAN Hot;

    POINT LastCursorLocation;
    PPH_STRING LastTooltipText;
    PH_GRAPHBAR_MEMORY Memory;

    PPH_GRAPHBAR_MESSAGE_CALLBACK Callback;
    PVOID Context;
} PH_GRAPHBAR_CONTEXT, *PPH_GRAPHBAR_CONTEXT;

// Notifications

#define GBN_DRAWBAR (WM_USER + 1451)
#define GBN_GETTOOLTIPTEXT (WM_USER + 1452)
#define GBN_MOUSEEVENT (WM_USER + 1453)
#define GBN_GETMEMORY (WM_USER + 1454)

typedef struct _PH_GRAPHBAR_DRAWBAR
{
    NMHDR Header;
    HDC hdc;
    RECT Rect;
} PH_GRAPHBAR_DRAWBAR, *PPH_GRAPHBAR_DRAWBAR;

typedef struct _PH_GRAPHBAR_GETTOOLTIPTEXT
{
    NMHDR Header;
    PPH_STRING Text;
    LONG CursorX;
    RECT Rect;
} PH_GRAPHBAR_GETTOOLTIPTEXT, *PPH_GRAPHBAR_GETTOOLTIPTEXT;

typedef struct _PH_GRAPHBAR_GETMEMORY
{
    NMHDR Header;
    PH_GRAPHBAR_MEMORY Memory;
} PH_GRAPHBAR_GETMEMORY, *PPH_GRAPHBAR_GETMEMORY;

LRESULT CALLBACK PhGraphBarWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

PHLIBAPI
RTL_ATOM
NTAPI
PhGraphBarInitialization(
    VOID
    );

EXTERN_C_END

#endif
