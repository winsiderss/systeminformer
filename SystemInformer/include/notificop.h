/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2016
 *     dmex    2017-2026
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

/**
 * Gets the application icon.
 *
 * \param DpiValue The DPI value.
 * \return The icon handle.
 */
HICON PhNfGetApplicationIcon(
    _In_opt_ LONG DpiValue
    );

/**
 * Gets the black icon.
 *
 * \return The icon handle.
 */
HICON PhNfpGetBlackIcon(
    VOID
    );

/**
 * Adds a notification icon to the tray.
 *
 * \param Icon The icon.
 * \return TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhNfpAddNotifyIcon(
    _In_ PPH_NF_ICON Icon
    );

/**
 * Removes a notification icon from the tray.
 *
 * \param Icon The icon.
 * \return TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhNfpRemoveNotifyIcon(
    _In_ PPH_NF_ICON Icon
    );

/**
 * Modifies a notification icon in the tray.
 *
 * \param Icon The icon.
 * \param Flags The flags.
 * \param Text The tooltip text.
 * \param IconHandle The icon handle.
 * \return TRUE if successful, FALSE otherwise.
 */
BOOLEAN PhNfpModifyNotifyIcon(
    _In_ PPH_NF_ICON Icon,
    _In_ ULONG Flags,
    _In_opt_ PPH_STRING Text,
    _In_opt_ HICON IconHandle
    );

/**
 * Thread routine for updating notification icons.
 *
 * \param Context The user-defined context.
 * \return NTSTATUS.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhNfpTrayIconUpdateThread(
    _In_opt_ PVOID Context
    );

/**
 * Callback function for process provider updates.
 *
 * \param Parameter The callback parameter.
 * \param Context The user-defined context.
 */
_Function_class_(PH_CALLBACK_FUNCTION)
VOID PhNfpProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

/**
 * Updates a registered notification icon.
 *
 * \param Icon The icon.
 */
VOID PhNfpUpdateRegisteredIcon(
    _In_ PPH_NF_ICON Icon
    );

/**
 * Begins bitmap drawing for notification icons.
 *
 * \param Width The width of the bitmap.
 * \param Height The height of the bitmap.
 * \param Bitmap The bitmap handle.
 * \param Bits The pixel bits.
 * \param Hdc The device context.
 * \param OldBitmap The old bitmap handle.
 */
_Function_class_(PH_NF_BEGIN_BITMAP)
VOID PhNfpBeginBitmap(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ HBITMAP *Bitmap,
    _Out_opt_ PVOID *Bits,
    _Out_ HDC *Hdc,
    _Out_ HBITMAP *OldBitmap
    );

/**
 * Begins bitmap drawing for notification icons with a context.
 *
 * \param Context The bitmap context.
 * \param Width The width of the bitmap.
 * \param Height The height of the bitmap.
 * \param Bitmap The bitmap handle.
 * \param Bits The pixel bits.
 * \param Hdc The device context.
 * \param OldBitmap The old bitmap handle.
 */
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

/**
 * Update callback for the CPU history notification icon.
 *
 * \param Icon The icon.
 * \param NewIconOrBitmap The new icon or bitmap.
 * \param Flags The update flags.
 * \param NewText The new tooltip text.
 * \param Context The user-defined context.
 */
_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpCpuHistoryIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

/**
 * Update callback for the IO history notification icon.
 *
 * \param Icon The icon.
 * \param NewIconOrBitmap The new icon or bitmap.
 * \param Flags The update flags.
 * \param NewText The new tooltip text.
 * \param Context The user-defined context.
 */
_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpIoHistoryIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

/**
 * Update callback for the commit history notification icon.
 *
 * \param Icon The icon.
 * \param NewIconOrBitmap The new icon or bitmap.
 * \param Flags The update flags.
 * \param NewText The new tooltip text.
 * \param Context The user-defined context.
 */
_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpCommitHistoryIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

/**
 * Update callback for the physical history notification icon.
 *
 * \param Icon The icon.
 * \param NewIconOrBitmap The new icon or bitmap.
 * \param Flags The update flags.
 * \param NewText The new tooltip text.
 * \param Context The user-defined context.
 */
_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpPhysicalHistoryIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

/**
 * Update callback for the CPU usage notification icon.
 *
 * \param Icon The icon.
 * \param NewIconOrBitmap The new icon or bitmap.
 * \param Flags The update flags.
 * \param NewText The new tooltip text.
 * \param Context The user-defined context.
 */
_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpCpuUsageIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

// Text icons

/**
 * Update callback for the CPU usage text notification icon.
 *
 * \param Icon The icon.
 * \param NewIconOrBitmap The new icon or bitmap.
 * \param Flags The update flags.
 * \param NewText The new tooltip text.
 * \param Context The user-defined context.
 */
_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpCpuUsageTextIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

/**
 * Update callback for the IO usage text notification icon.
 *
 * \param Icon The icon.
 * \param NewIconOrBitmap The new icon or bitmap.
 * \param Flags The update flags.
 * \param NewText The new tooltip text.
 * \param Context The user-defined context.
 */
_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpIoUsageTextIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

/**
 * Update callback for the commit usage text notification icon.
 *
 * \param Icon The icon.
 * \param NewIconOrBitmap The new icon or bitmap.
 * \param Flags The update flags.
 * \param NewText The new tooltip text.
 * \param Context The user-defined context.
 */
_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpCommitTextIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

/**
 * Update callback for the physical usage text notification icon.
 *
 * \param Icon The icon.
 * \param NewIconOrBitmap The new icon or bitmap.
 * \param Flags The update flags.
 * \param NewText The new tooltip text.
 * \param Context The user-defined context.
 */
_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpPhysicalUsageTextIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

// plain icon
/**
 * Update callback for the plain (static) notification icon.
 *
 * \param Icon The icon.
 * \param NewIconOrBitmap The new icon or bitmap.
 * \param Flags The update flags.
 * \param NewText The new tooltip text.
 * \param Context The user-defined context.
 */
_Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID PhNfpPlainIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

/**
 * Gets the show mini-information section data for an icon.
 *
 * \param IconIndex The index of the icon.
 * \param RegisteredIcon The registered icon.
 * \param Data A variable which receives the section data.
 * \return TRUE if successful, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhNfpGetShowMiniInfoSectionData(
    _In_ ULONG IconIndex,
    _In_ PPH_NF_ICON RegisteredIcon,
    _Out_ PPH_NF_MSG_SHOWMINIINFOSECTION_DATA Data
    );

#define NFP_ICON_CLICK_ACTIVATE_DELAY 140
#define NFP_ICON_RESTORE_HOVER_DELAY 1000

/**
 * Timer procedure for activating an icon after a click.
 *
 * \param WindowHandle The window handle.
 * \param uMsg The window message.
 * \param idEvent The timer ID.
 * \param dwTime The current system time.
 */
VOID PhNfpIconClickActivateTimerProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ UINT_PTR idEvent,
    _In_ ULONG dwTime
    );

/**
 * Disables icon hover effects.
 */
VOID PhNfpDisableHover(
    VOID
    );

/**
 * Timer procedure for restoring icon hover effects.
 *
 * \param WindowHandle The window handle.
 * \param uMsg The window message.
 * \param idEvent The timer ID.
 * \param dwTime The current system time.
 */
VOID PhNfpIconRestoreHoverTimerProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ UINT_PTR idEvent,
    _In_ ULONG dwTime
    );

/**
 * Disables the popup hover workaround for Windows 11.
 */
VOID PhNfpIconDisablePopupHoverWin11Workaround(
    VOID
    );

/**
 * Timer procedure for showing a popup hover on Windows 11.
 *
 * \param WindowHandle The window handle.
 * \param uMsg The window message.
 * \param idEvent The timer ID.
 * \param dwTime The current system time.
 */
VOID PhNfpIconShowPopupHoverTimerProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ UINT_PTR idEvent,
    _In_ ULONG dwTime
    );

#endif
