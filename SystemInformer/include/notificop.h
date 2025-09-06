/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2016
 *     dmex    2017-2023
 *
 */

#ifndef PH_NOTIFICOP_H
#define PH_NOTIFICOP_H

#define PH_NF_ENABLE_WORKQUEUE 1

typedef struct _PH_NF_WORKQUEUE_DATA
{
    SLIST_ENTRY ListEntry;
    PPH_NF_ICON Icon;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG Add : 1;
            ULONG Delete : 1;
            ULONG Update : 1;
            ULONG ShowBalloon : 1;
            ULONG Spare : 28;
        };
    };

    PPH_STRING BalloonTitle;
    PPH_STRING BalloonText;
    ULONG BalloonTimeout;
} PH_NF_WORKQUEUE_DATA, *PPH_NF_WORKQUEUE_DATA;

typedef struct _PH_NF_BITMAP
{
    BOOLEAN Initialized;
    HDC Hdc;
    HBITMAP Bitmap;
    LPRGBQUAD Bits;
    LONG Width;
    LONG Height;
    LONG TaskbarDpi;
} PH_NF_BITMAP, *PPH_NF_BITMAP;

HICON PhNfGetApplicationIcon(
    _In_opt_ LONG DpiValue
    );

HICON PhNfpGetBlackIcon(
    VOID
    );

BOOLEAN PhNfpAddNotifyIcon(
    _In_ PPH_NF_ICON Icon
    );

BOOLEAN PhNfpRemoveNotifyIcon(
    _In_ PPH_NF_ICON Icon
    );

BOOLEAN PhNfpModifyNotifyIcon(
    _In_ PPH_NF_ICON Icon,
    _In_ ULONG Flags,
    _In_opt_ PPH_STRING Text,
    _In_opt_ HICON IconHandle
    );

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhNfpTrayIconUpdateThread(
    _In_opt_ PVOID Context
    );

_Function_class_(PH_CALLBACK_FUNCTION)
VOID PhNfpProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID PhNfpUpdateRegisteredIcon(
    _In_ PPH_NF_ICON Icon
    );

_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpBeginBitmap(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ HBITMAP *Bitmap,
    _Out_opt_ PVOID *Bits,
    _Out_ HDC *Hdc,
    _Out_ HBITMAP *OldBitmap
    );

_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpBeginBitmap2(
    _Inout_ PPH_NF_BITMAP Context,
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ HBITMAP *Bitmap,
    _Out_opt_ PVOID *Bits,
    _Out_ HDC *Hdc,
    _Out_ HBITMAP *OldBitmap
    );

_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpCpuHistoryIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );
_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpIoHistoryIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpCommitHistoryIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpPhysicalHistoryIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpCpuUsageIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

// Text icons

_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpCpuUsageTextIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpIoUsageTextIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpCommitTextIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpPhysicalUsageTextIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

// plain icon
_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpPlainIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

_Success_(return)
BOOLEAN PhNfpGetShowMiniInfoSectionData(
    _In_ ULONG IconIndex,
    _In_ PPH_NF_ICON RegisteredIcon,
    _Out_ PPH_NF_MSG_SHOWMINIINFOSECTION_DATA Data
    );

#define NFP_ICON_CLICK_ACTIVATE_DELAY 140
#define NFP_ICON_RESTORE_HOVER_DELAY 1000

VOID PhNfpIconClickActivateTimerProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ UINT_PTR idEvent,
    _In_ ULONG dwTime
    );

VOID PhNfpDisableHover(
    VOID
    );

VOID PhNfpIconRestoreHoverTimerProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ UINT_PTR idEvent,
    _In_ ULONG dwTime
    );

VOID PhNfpIconDisablePopupHoverWin11Workaround(
    VOID
    );

VOID PhNfpIconShowPopupHoverTimerProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ UINT_PTR idEvent,
    _In_ ULONG dwTime
    );

#endif
