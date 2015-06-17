#ifndef PH_NOTIFICO_H
#define PH_NOTIFICO_H

#define PH_ICON_MINIMUM 0x1
#define PH_ICON_CPU_HISTORY 0x1
#define PH_ICON_IO_HISTORY 0x2
#define PH_ICON_COMMIT_HISTORY 0x4
#define PH_ICON_PHYSICAL_HISTORY 0x8
#define PH_ICON_CPU_USAGE 0x10
#define PH_ICON_DEFAULT_MAXIMUM 0x20
#define PH_ICON_DEFAULT_ALL 0x1f

#define PH_ICON_LIMIT 0x80000000
#define PH_ICON_ALL 0xffffffff

// begin_phapppub
typedef VOID (NTAPI *PPH_NF_UPDATE_REGISTERED_ICON)(
    _In_ struct _PH_NF_ICON *Icon
    );

typedef VOID (NTAPI *PPH_NF_BEGIN_BITMAP)(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ HBITMAP *Bitmap,
    _Out_opt_ PVOID *Bits,
    _Out_ HDC *Hdc,
    _Out_ HBITMAP *OldBitmap
    );

typedef struct _PH_NF_POINTERS
{
    PPH_NF_UPDATE_REGISTERED_ICON UpdateRegisteredIcon;
    PPH_NF_BEGIN_BITMAP BeginBitmap;
} PH_NF_POINTERS, *PPH_NF_POINTERS;

#define PH_NF_UPDATE_IS_BITMAP 0x1
#define PH_NF_UPDATE_DESTROY_RESOURCE 0x2

typedef VOID (NTAPI *PPH_NF_ICON_UPDATE_CALLBACK)(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

typedef BOOLEAN (NTAPI *PPH_NF_ICON_MESSAGE_CALLBACK)(
    _In_ struct _PH_NF_ICON *Icon,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam,
    _In_opt_ PVOID Context
    );

// Special messages
// The message type is stored in LOWORD(LParam), and the message data is in WParam.

#define PH_NF_MSG_SHOWMINIINFOSECTION (WM_APP + 1)

typedef struct _PH_NF_MSG_SHOWMINIINFOSECTION_DATA
{
    PWSTR SectionName; // NULL to leave unchanged
} PH_NF_MSG_SHOWMINIINFOSECTION_DATA, *PPH_NF_MSG_SHOWMINIINFOSECTION_DATA;

// Structures and internal functions

#define PH_NF_ICON_UNAVAILABLE 0x1
#define PH_NF_ICON_SHOW_MINIINFO 0x2
// end_phapppub

// begin_phapppub
typedef struct _PH_NF_ICON
{
    // Public

    struct _PH_PLUGIN *Plugin;
    ULONG SubId;
    PVOID Context;
    PPH_NF_POINTERS Pointers;
// end_phapppub

    // Private

    PWSTR Text;
    ULONG Flags;
    ULONG IconId;
    PPH_NF_ICON_UPDATE_CALLBACK UpdateCallback;
    PPH_NF_ICON_MESSAGE_CALLBACK MessageCallback;
// begin_phapppub
} PH_NF_ICON, *PPH_NF_ICON;
// end_phapppub

VOID PhNfLoadStage1(
    VOID
    );

VOID PhNfLoadStage2(
    VOID
    );

VOID PhNfSaveSettings(
    VOID
    );

VOID PhNfUninitialization(
    VOID
    );

VOID PhNfForwardMessage(
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam
    );

ULONG PhNfGetMaximumIconId(
    VOID
    );

ULONG PhNfTestIconMask(
    _In_ ULONG Id
    );

VOID PhNfSetVisibleIcon(
    _In_ ULONG Id,
    _In_ BOOLEAN Visible
    );

BOOLEAN PhNfShowBalloonTip(
    _In_opt_ ULONG Id,
    _In_ PWSTR Title,
    _In_ PWSTR Text,
    _In_ ULONG Timeout,
    _In_ ULONG Flags
    );

HICON PhNfBitmapToIcon(
    _In_ HBITMAP Bitmap
    );

PPH_NF_ICON PhNfRegisterIcon(
    _In_ struct _PH_PLUGIN *Plugin,
    _In_ ULONG SubId,
    _In_opt_ PVOID Context,
    _In_ PWSTR Text,
    _In_ ULONG Flags,
    _In_opt_ PPH_NF_ICON_UPDATE_CALLBACK UpdateCallback,
    _In_opt_ PPH_NF_ICON_MESSAGE_CALLBACK MessageCallback
    );

PPH_NF_ICON PhNfGetIconById(
    _In_ ULONG Id
    );

PPH_NF_ICON PhNfFindIcon(
    _In_ PPH_STRINGREF PluginName,
    _In_ ULONG SubId
    );

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
// end_phapppub

#endif
