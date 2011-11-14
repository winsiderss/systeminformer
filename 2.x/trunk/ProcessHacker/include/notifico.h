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

typedef VOID (NTAPI *PPH_NF_UPDATE_REGISTERED_ICON)(
    __in struct _PH_NF_ICON *Icon
    );

typedef VOID (NTAPI *PPH_NF_BEGIN_BITMAP)(
    __out PULONG Width,
    __out PULONG Height,
    __out HBITMAP *Bitmap,
    __out_opt PVOID *Bits,
    __out HDC *Hdc,
    __out HBITMAP *OldBitmap
    );

typedef struct _PH_NF_POINTERS
{
    PPH_NF_UPDATE_REGISTERED_ICON UpdateRegisteredIcon;
    PPH_NF_BEGIN_BITMAP BeginBitmap;
} PH_NF_POINTERS, *PPH_NF_POINTERS;

#define PH_NF_UPDATE_IS_BITMAP 0x1
#define PH_NF_UPDATE_DESTROY_RESOURCE 0x2

typedef VOID (NTAPI *PPH_NF_ICON_UPDATE_CALLBACK)(
    __in struct _PH_NF_ICON *Icon,
    __out PVOID *NewIconOrBitmap,
    __out PULONG Flags,
    __out PPH_STRING *NewText,
    __in_opt PVOID Context
    );

typedef BOOLEAN (NTAPI *PPH_NF_ICON_MESSAGE_CALLBACK)(
    __in struct _PH_NF_ICON *Icon,
    __in ULONG_PTR WParam,
    __in ULONG_PTR LParam,
    __in_opt PVOID Context
    );

#define PH_NF_ICON_UNAVAILABLE 0x1

typedef struct _PH_NF_ICON
{
    struct _PH_PLUGIN *Plugin;
    ULONG SubId;
    PVOID Context;
    PPH_NF_POINTERS Pointers;

    PWSTR Text;
    ULONG Flags;
    ULONG IconId;
    PPH_NF_ICON_UPDATE_CALLBACK UpdateCallback;
    PPH_NF_ICON_MESSAGE_CALLBACK MessageCallback;
} PH_NF_ICON, *PPH_NF_ICON;

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
    __in ULONG_PTR WParam,
    __in ULONG_PTR LParam
    );

ULONG PhNfGetMaximumIconId(
    VOID
    );

ULONG PhNfTestIconMask(
    __in ULONG Id
    );

VOID PhNfSetVisibleIcon(
    __in ULONG Id,
    __in BOOLEAN Visible
    );

BOOLEAN PhNfShowBalloonTip(
    __in_opt ULONG Id,
    __in PWSTR Title,
    __in PWSTR Text,
    __in ULONG Timeout,
    __in ULONG Flags
    );

HICON PhNfBitmapToIcon(
    __in HBITMAP Bitmap
    );

PPH_NF_ICON PhNfRegisterIcon(
    __in struct _PH_PLUGIN *Plugin,
    __in ULONG SubId,
    __in_opt PVOID Context,
    __in PWSTR Text,
    __in ULONG Flags,
    __in_opt PPH_NF_ICON_UPDATE_CALLBACK UpdateCallback,
    __in_opt PPH_NF_ICON_MESSAGE_CALLBACK MessageCallback
    );

PPH_NF_ICON PhNfGetIconById(
    __in ULONG Id
    );

PPH_NF_ICON PhNfFindIcon(
    __in PPH_STRINGREF PluginName,
    __in ULONG SubId
    );

// Public registration data

typedef struct _PH_NF_ICON_REGISTRATION_DATA
{
    PPH_NF_ICON_UPDATE_CALLBACK UpdateCallback;
    PPH_NF_ICON_MESSAGE_CALLBACK MessageCallback;
} PH_NF_ICON_REGISTRATION_DATA, *PPH_NF_ICON_REGISTRATION_DATA;

#endif
