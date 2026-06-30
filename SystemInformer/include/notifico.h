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

#ifndef PH_NOTIFICO_H
#define PH_NOTIFICO_H

extern PPH_LIST PhTrayIconItemList;

typedef enum _PH_TRAY_ICON_ID
{
    PH_TRAY_ICON_ID_NONE,
    PH_TRAY_ICON_ID_CPU_USAGE,
    PH_TRAY_ICON_ID_CPU_HISTORY,
    PH_TRAY_ICON_ID_IO_HISTORY,
    PH_TRAY_ICON_ID_COMMIT_HISTORY,
    PH_TRAY_ICON_ID_PHYSICAL_HISTORY,
    PH_TRAY_ICON_ID_CPU_TEXT,
    PH_TRAY_ICON_ID_IO_TEXT,
    PH_TRAY_ICON_ID_COMMIT_TEXT,
    PH_TRAY_ICON_ID_PHYSICAL_TEXT,
    PH_TRAY_ICON_ID_PLAIN_ICON,
    PH_TRAY_ICON_ID_MAXIMUM
} PH_TRAY_ICON_ID;

typedef enum _PH_TRAY_ICON_GUID
{
    PH_TRAY_ICON_GUID_CPU_USAGE,
    PH_TRAY_ICON_GUID_CPU_HISTORY,
    PH_TRAY_ICON_GUID_IO_HISTORY,
    PH_TRAY_ICON_GUID_COMMIT_HISTORY,
    PH_TRAY_ICON_GUID_PHYSICAL_HISTORY,
    PH_TRAY_ICON_GUID_CPU_TEXT,
    PH_TRAY_ICON_GUID_IO_TEXT,
    PH_TRAY_ICON_GUID_COMMIT_TEXT,
    PH_TRAY_ICON_GUID_PHYSICAL_TEXT,
    PH_TRAY_ICON_GUID_PLAIN_ICON,
    PH_TRAY_ICON_GUID_MAXIMUM
} PH_TRAY_ICON_GUID;

#define PH_TRAY_ICON_ID_PLUGIN 0x80

#define PH_ICON_LIMIT 0x80000000
#define PH_ICON_ALL 0xffffffff

// begin_phapppub
typedef struct _PH_NF_ICON PH_NF_ICON, *PPH_NF_ICON;

typedef _Function_class_(PH_NF_UPDATE_REGISTERED_ICON)
VOID NTAPI PH_NF_UPDATE_REGISTERED_ICON(
    _In_ PPH_NF_ICON Icon
    );
typedef PH_NF_UPDATE_REGISTERED_ICON* PPH_NF_UPDATE_REGISTERED_ICON;

typedef _Function_class_(PH_NF_BEGIN_BITMAP)
VOID NTAPI PH_NF_BEGIN_BITMAP(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ HBITMAP *Bitmap,
    _Out_opt_ PVOID *Bits,
    _Out_ HDC *Hdc,
    _Out_ HBITMAP *OldBitmap
    );
typedef PH_NF_BEGIN_BITMAP* PPH_NF_BEGIN_BITMAP;

typedef struct _PH_NF_POINTERS
{
    PPH_NF_BEGIN_BITMAP BeginBitmap;
} PH_NF_POINTERS, *PPH_NF_POINTERS;

#define PH_NF_UPDATE_IS_BITMAP 0x1
#define PH_NF_UPDATE_DESTROY_RESOURCE 0x2

typedef _Function_class_(PH_NF_ICON_UPDATE_CALLBACK)
VOID NTAPI PH_NF_ICON_UPDATE_CALLBACK(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );
typedef PH_NF_ICON_UPDATE_CALLBACK* PPH_NF_ICON_UPDATE_CALLBACK;

typedef _Function_class_(PH_NF_ICON_MESSAGE_CALLBACK)
BOOLEAN NTAPI PH_NF_ICON_MESSAGE_CALLBACK(
    _In_ PPH_NF_ICON Icon,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam,
    _In_opt_ PVOID Context
    );
typedef PH_NF_ICON_MESSAGE_CALLBACK* PPH_NF_ICON_MESSAGE_CALLBACK;

// Special messages
// The message type is stored in LOWORD(LParam), and the message data is in WParam.

#define PH_NF_MSG_SHOWMINIINFOSECTION (WM_APP + 1)

typedef struct _PH_NF_MSG_SHOWMINIINFOSECTION_DATA
{
    PWSTR SectionName; // NULL to leave unchanged
} PH_NF_MSG_SHOWMINIINFOSECTION_DATA, *PPH_NF_MSG_SHOWMINIINFOSECTION_DATA;

// Structures and internal functions

#define PH_NF_ICON_ENABLED 0x1
#define PH_NF_ICON_UNAVAILABLE 0x2
#define PH_NF_ICON_NOSHOW_MINIINFO 0x4
// end_phapppub

// begin_phapppub
typedef struct _PH_PLUGIN PH_PLUGIN, *PPH_PLUGIN;

typedef struct _PH_NF_ICON
{
    // Public

    PPH_PLUGIN Plugin;
    ULONG SubId;
    PVOID Context;
    PPH_NF_POINTERS Pointers;
// end_phapppub

    // Private

    PCWSTR Text;
    ULONG Flags;
    ULONG IconId;
    GUID IconGuid;
    PPH_NF_ICON_UPDATE_CALLBACK UpdateCallback;
    PPH_NF_ICON_MESSAGE_CALLBACK MessageCallback;

    PPH_STRING TextCache;
// begin_phapppub
} PH_NF_ICON, *PPH_NF_ICON;
// end_phapppub

/**
 * Performs stage 1 of the notification icon load process.
 */
VOID PhNfLoadStage1(
    VOID
    );

/**
 * Performs stage 2 of the notification icon load process.
 */
VOID PhNfLoadStage2(
    VOID
    );

/**
 * Saves notification icon settings.
 */
VOID PhNfSaveSettings(
    VOID
    );

/**
 * Performs uninitialization of the notification icon system.
 */
VOID PhNfUninitialization(
    VOID
    );

/**
 * Forwards a window message to the notification icon system.
 *
 * \param WindowHandle The window handle.
 * \param WParam The WPARAM value.
 * \param LParam The LPARAM value.
 */
VOID PhNfForwardMessage(
    _In_ HWND WindowHandle,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam
    );

/**
 * Sets the visibility of a notification icon.
 *
 * \param Icon The icon.
 * \param Visible TRUE to show the icon, FALSE to hide it.
 */
VOID PhNfSetVisibleIcon(
    _In_ PPH_NF_ICON Icon,
    _In_ BOOLEAN Visible
    );

/**
 * Shows a balloon tip notification.
 *
 * \param Title The title of the notification.
 * \param Text The text of the notification.
 * \param Timeout The timeout in milliseconds.
 * \return TRUE if the notification was shown, FALSE otherwise.
 */
BOOLEAN PhNfShowBalloonTip(
    _In_ PCWSTR Title,
    _In_ PCWSTR Text,
    _In_ ULONG Timeout
    );

/**
 * Shows a balloon tip notification with a callback.
 *
 * \param Title The title of the notification.
 * \param Text The text of the notification.
 * \param Timeout The timeout in milliseconds.
 * \param ToastCallback The callback function.
 * \param Context The user-defined context.
 * \return HRESULT.
 */
HRESULT PhNfShowBalloonTipEx(
    _In_ PCWSTR Title,
    _In_ PCWSTR Text,
    _In_ ULONG Timeout,
    _In_opt_ PPH_TOAST_CALLBACK ToastCallback,
    _In_opt_ PVOID Context
    );

/**
 * Converts a bitmap to an icon.
 *
 * \param Bitmap The bitmap handle.
 * \return The icon handle.
 */
HICON PhNfBitmapToIcon(
    _In_ HBITMAP Bitmap
    );

/**
 * Registers a notification icon.
 *
 * \param Plugin The plugin registering the icon.
 * \param Id The sub-ID of the icon.
 * \param Guid The GUID of the icon.
 * \param Context The user-defined context.
 * \param Text The tooltip text.
 * \param Flags The icon flags.
 * \param UpdateCallback The icon update callback.
 * \param MessageCallback The icon message callback.
 * \return The registered icon.
 */
PPH_NF_ICON PhNfRegisterIcon(
    _In_opt_ PPH_PLUGIN Plugin,
    _In_ ULONG Id,
    _In_ GUID Guid,
    _In_opt_ PVOID Context,
    _In_ PCWSTR Text,
    _In_ ULONG Flags,
    _In_opt_ PPH_NF_ICON_UPDATE_CALLBACK UpdateCallback,
    _In_opt_ PPH_NF_ICON_MESSAGE_CALLBACK MessageCallback
    );

/**
 * Gets a notification icon by its sub-ID.
 *
 * \param SubId The sub-ID of the icon.
 * \return The icon, or NULL if not found.
 */
PPH_NF_ICON PhNfGetIconById(
    _In_ ULONG Id
    );

/**
 * Finds a notification icon by its plugin name and sub-ID.
 *
 * \param PluginName The plugin name.
 * \param SubId The sub-ID of the icon.
 * \return The icon, or NULL if not found.
 */
PPH_NF_ICON PhNfFindIcon(
    _In_ PPH_STRINGREF PluginName,
    _In_ ULONG SubId
    );

/**
 * Checks if notification icons are enabled.
 *
 * \return TRUE if icons are enabled, FALSE otherwise.
 */
BOOLEAN PhNfIconsEnabled(
    VOID
    );

/**
 * Notifies the notification icon system that the mini-information window has been pinned or unpinned.
 *
 * \param Pinned TRUE if the window was pinned, FALSE if it was unpinned.
 */
VOID PhNfNotifyMiniInfoPinned(
    _In_ BOOLEAN Pinned
    );

// begin_phapppub
// Public registration data

typedef struct _PH_NF_ICON_REGISTRATION_DATA
{
    PPH_NF_ICON_UPDATE_CALLBACK UpdateCallback;
    PPH_NF_ICON_MESSAGE_CALLBACK MessageCallback;
} PH_NF_ICON_REGISTRATION_DATA, *PPH_NF_ICON_REGISTRATION_DATA;

/**
 * Draws text for a tray icon.
 *
 * \param hdc The device context.
 * \param Bits The pixel bits.
 * \param DrawInfo The graph draw info.
 * \param Text The text to draw.
 */
VOID PhDrawTrayIconText(
    _In_ HDC hdc,
    _In_ PVOID Bits,
    _Inout_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ PPH_STRINGREF Text
    );

/**
 * Gets the font for tray icons.
 *
 * \param DpiValue The DPI value.
 * \return The font handle.
 */
HFONT PhNfGetTrayIconFont(
    _In_opt_ LONG DpiValue
    );

// end_phapppub

#endif
