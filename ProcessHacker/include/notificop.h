#ifndef PH_NOTIFICOP_H
#define PH_NOTIFICOP_H

typedef struct _PH_NF_BITMAP
{
    BOOLEAN Initialized;
    HDC Hdc;
    BITMAPINFOHEADER Header;
    HBITMAP Bitmap;
    PVOID Bits;
} PH_NF_BITMAP, *PPH_NF_BITMAP;

HICON PhNfpGetBlackIcon(
    VOID
    );

BOOLEAN PhNfpAddNotifyIcon(
    _In_ ULONG Id
    );

BOOLEAN PhNfpRemoveNotifyIcon(
    _In_ ULONG Id
    );

BOOLEAN PhNfpModifyNotifyIcon(
    _In_ ULONG Id,
    _In_ ULONG Flags,
    _In_opt_ PPH_STRING Text,
    _In_opt_ HICON Icon
    );

VOID PhNfpProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID PhNfpUpdateRegisteredIcon(
    _In_ PPH_NF_ICON Icon
    );

VOID PhNfpBeginBitmap(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ HBITMAP *Bitmap,
    _Out_opt_ PVOID *Bits,
    _Out_ HDC *Hdc,
    _Out_ HBITMAP *OldBitmap
    );

VOID PhNfpBeginBitmap2(
    _Inout_ PPH_NF_BITMAP Context,
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ HBITMAP *Bitmap,
    _Out_opt_ PVOID *Bits,
    _Out_ HDC *Hdc,
    _Out_ HBITMAP *OldBitmap
    );

VOID PhNfpUpdateIconCpuHistory(
    VOID
    );

VOID PhNfpUpdateIconIoHistory(
    VOID
    );

VOID PhNfpUpdateIconCommitHistory(
    VOID
    );

VOID PhNfpUpdateIconPhysicalHistory(
    VOID
    );

VOID PhNfpUpdateIconCpuUsage(
    VOID
    );

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
    _In_ DWORD dwTime
    );

VOID PhNfpDisableHover(
    VOID
    );

VOID PhNfpIconRestoreHoverTimerProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ UINT_PTR idEvent,
    _In_ DWORD dwTime
    );

#endif
