/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 */

#ifndef _PH_TREEHDR_H
#define _PH_TREEHDR_H

#include <guisup.h>

EXTERN_C_START

#define PH_HEADERNEW_CLASSNAME L"PhHeaderNew"
#define PH_HEADERNEW_DARK_CLASSNAME L"DarkMode_ItemsView::Header"

/**
 * Registers the PhHeaderNew window class.
 *
 * The class behaves as a drop-in replacement for WC_HEADER: it responds to the
 * HDM_* messages and notifies its parent through WM_NOTIFY with HDN_* / NM_*
 * codes. Layout is driven by HDM_LAYOUT exactly as with SysHeader32.
 * \return The class atom, or RTL_ATOM_INVALID_ATOM on failure.
 */
PHLIBAPI
RTL_ATOM
NTAPI
PhHeaderNewInitialization(
    VOID
    );

typedef enum _PH_HEADERNEW_THEME
{
    PhHeaderNewThemeWin7 = 0,     // classic uxtheme header (aero look)
    PhHeaderNewThemeWin10 = 1,    // flat custom-painted header
    PhHeaderNewThemeUxTheme = 2   // native/dark themed header (default)
} PH_HEADERNEW_THEME;

// Runtime flags (PH_HEADERNEW_CREATEPARAMS.Flags and PHHM_SETFLAGS)
#define PHHF_NOTHEMEBACKGROUND 0x00000001 // never paint the item background
#define PHHF_NODIVIDERS        0x00000002 // suppress the column dividers
#define PHHF_BOTTOMBORDER      0x00000004 // draw a border along the bottom edge

// Messages
#define PHHM_FIRST          (WM_USER + 0x700)
#define PHHM_SETTHEME       (PHHM_FIRST + 1)  // wParam = PH_HEADERNEW_THEME
#define PHHM_GETTHEME       (PHHM_FIRST + 2)
#define PHHM_SETTHEMEDARK   (PHHM_FIRST + 3)  // wParam = BOOLEAN
#define PHHM_GETTHEMEDARK   (PHHM_FIRST + 4)
#define PHHM_SETHEIGHT      (PHHM_FIRST + 5)  // wParam = px, 0 = measure from font
#define PHHM_GETHEIGHT      (PHHM_FIRST + 6)
#define PHHM_SETPADDING     (PHHM_FIRST + 7)  // wParam = cx, lParam = cy
#define PHHM_INVALIDATEITEM (PHHM_FIRST + 8)  // wParam = index
#define PHHM_SETCALLBACK    (PHHM_FIRST + 9)  // wParam = callback, lParam = context
#define PHHM_SETFLAGS       (PHHM_FIRST + 10) // wParam = PHHF_*, returns old flags
#define PHHM_GETFLAGS       (PHHM_FIRST + 11)
#define PHHM_LAST           PHHM_GETFLAGS

typedef _Function_class_(PH_HEADERNEW_MESSAGE_CALLBACK)
BOOLEAN NTAPI PH_HEADERNEW_MESSAGE_CALLBACK(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );
typedef PH_HEADERNEW_MESSAGE_CALLBACK *PPH_HEADERNEW_MESSAGE_CALLBACK;

typedef struct _PH_HEADERNEW_CREATEPARAMS
{
    ULONG Size;
    ULONG Flags;
    PPH_HEADERNEW_MESSAGE_CALLBACK Callback;
    PVOID Context;
    PH_HEADERNEW_THEME Theme;
    // Add new fields here.
} PH_HEADERNEW_CREATEPARAMS, *PPH_HEADERNEW_CREATEPARAMS;

#if defined(_PHLIB_)

EXTERN_C LRESULT PhHeaderNewSendMessage(
    _In_ HWND WindowHandle,
    _In_ ULONG WindowMessage,
    _Pre_maybenull_ _Post_valid_ WPARAM wParam,
    _Pre_maybenull_ _Post_valid_ LPARAM lParam
    );

#define PhHeaderNew_SetTheme(WindowHandle, Theme) \
    ((BOOL)PhHeaderNewSendMessage((WindowHandle), PHHM_SETTHEME, (WPARAM)(Theme), 0))
#define PhHeaderNew_GetTheme(WindowHandle) \
    ((PH_HEADERNEW_THEME)PhHeaderNewSendMessage((WindowHandle), PHHM_GETTHEME, 0, 0))
#define PhHeaderNew_SetThemeDark(WindowHandle, Dark) \
    ((VOID)PhHeaderNewSendMessage((WindowHandle), PHHM_SETTHEMEDARK, (WPARAM)(Dark), 0))
#define PhHeaderNew_GetThemeDark(WindowHandle) \
    ((BOOLEAN)PhHeaderNewSendMessage((WindowHandle), PHHM_GETTHEMEDARK, 0, 0))
#define PhHeaderNew_SetHeight(WindowHandle, Height) \
    ((VOID)PhHeaderNewSendMessage((WindowHandle), PHHM_SETHEIGHT, (WPARAM)(Height), 0))
#define PhHeaderNew_GetHeight(WindowHandle) \
    ((LONG)PhHeaderNewSendMessage((WindowHandle), PHHM_GETHEIGHT, 0, 0))
#define PhHeaderNew_SetPadding(WindowHandle, cx, cy) \
    ((VOID)PhHeaderNewSendMessage((WindowHandle), PHHM_SETPADDING, (WPARAM)(cx), (LPARAM)(cy)))
#define PhHeaderNew_InvalidateItem(WindowHandle, Index) \
    ((VOID)PhHeaderNewSendMessage((WindowHandle), PHHM_INVALIDATEITEM, (WPARAM)(Index), 0))
#define PhHeaderNew_SetCallback(WindowHandle, Callback, Context) \
    ((VOID)PhHeaderNewSendMessage((WindowHandle), PHHM_SETCALLBACK, (WPARAM)(Callback), (LPARAM)(Context)))
#define PhHeaderNew_SetFlags(WindowHandle, Flags) \
    ((ULONG)PhHeaderNewSendMessage((WindowHandle), PHHM_SETFLAGS, (WPARAM)(Flags), 0))
#define PhHeaderNew_GetFlags(WindowHandle) \
    ((ULONG)PhHeaderNewSendMessage((WindowHandle), PHHM_GETFLAGS, 0, 0))

#else

#define PhHeaderNew_SetTheme(WindowHandle, Theme) \
    ((BOOL)SendMessage((WindowHandle), PHHM_SETTHEME, (WPARAM)(Theme), 0))
#define PhHeaderNew_GetTheme(WindowHandle) \
    ((PH_HEADERNEW_THEME)SendMessage((WindowHandle), PHHM_GETTHEME, 0, 0))
#define PhHeaderNew_SetThemeDark(WindowHandle, Dark) \
    ((VOID)SendMessage((WindowHandle), PHHM_SETTHEMEDARK, (WPARAM)(Dark), 0))
#define PhHeaderNew_GetThemeDark(WindowHandle) \
    ((BOOLEAN)SendMessage((WindowHandle), PHHM_GETTHEMEDARK, 0, 0))
#define PhHeaderNew_SetHeight(WindowHandle, Height) \
    ((VOID)SendMessage((WindowHandle), PHHM_SETHEIGHT, (WPARAM)(Height), 0))
#define PhHeaderNew_GetHeight(WindowHandle) \
    ((LONG)SendMessage((WindowHandle), PHHM_GETHEIGHT, 0, 0))
#define PhHeaderNew_SetPadding(WindowHandle, cx, cy) \
    ((VOID)SendMessage((WindowHandle), PHHM_SETPADDING, (WPARAM)(cx), (LPARAM)(cy)))
#define PhHeaderNew_InvalidateItem(WindowHandle, Index) \
    ((VOID)SendMessage((WindowHandle), PHHM_INVALIDATEITEM, (WPARAM)(Index), 0))
#define PhHeaderNew_SetCallback(WindowHandle, Callback, Context) \
    ((VOID)SendMessage((WindowHandle), PHHM_SETCALLBACK, (WPARAM)(Callback), (LPARAM)(Context)))
#define PhHeaderNew_SetFlags(WindowHandle, Flags) \
    ((ULONG)SendMessage((WindowHandle), PHHM_SETFLAGS, (WPARAM)(Flags), 0))
#define PhHeaderNew_GetFlags(WindowHandle) \
    ((ULONG)SendMessage((WindowHandle), PHHM_GETFLAGS, 0, 0))

#endif

EXTERN_C_END

#endif
