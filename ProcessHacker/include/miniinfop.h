#ifndef MINIINFOP_H
#define MINIINFOP_H

// Constants

#define MIP_CONTAINER_CLASSNAME L"ProcessHackerMiniInfo"

#define MIP_TIMER_PIN_FIRST 1
#define MIP_TIMER_PIN_LAST (MIP_TIMER_PIN_FIRST + MaxMiniInfoPinType - 1)

// Misc.

#define SET_BUTTON_BITMAP(hwndDlg, Id, Bitmap) \
    SendMessage(GetDlgItem(hwndDlg, (Id)), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)(Bitmap))

// Dialog procedure

LRESULT CALLBACK PhMipContainerWndProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhMipMiniInfoDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// Container event handlers

VOID PhMipContainerOnShowWindow(
    _In_ BOOLEAN Showing,
    _In_ ULONG State
    );

VOID PhMipContainerOnSize(
    VOID
    );

VOID PhMipContainerOnSizing(
    _In_ ULONG Edge,
    _In_ PRECT DragRectangle
    );

BOOLEAN PhMipContainerOnEraseBkgnd(
    _In_ HDC hdc
    );

VOID PhMipContainerOnTimer(
    _In_ ULONG Id
    );

// Child dialog event handlers

VOID PhMipOnInitDialog(
    VOID
    );

VOID PhMipOnShowWindow(
    _In_ BOOLEAN Showing,
    _In_ ULONG State
    );

BOOLEAN PhMipOnNotify(
    _In_ NMHDR *Header,
    _Out_ LRESULT *Result
    );

BOOLEAN PhMipOnCtlColorXxx(
    _In_ ULONG Message,
    _In_ HWND hwnd,
    _In_ HDC hdc,
    _Out_ HBRUSH *Brush
    );

// Framework

typedef enum _MIP_ADJUST_PIN_RESULT
{
    NoAdjustPinResult,
    ShowAdjustPinResult,
    HideAdjustPinResult
} MIP_ADJUST_PIN_RESULT;

BOOLEAN PhMipMessageLoopFilter(
    _In_ PMSG Message,
    _In_ PVOID Context
    );

MIP_ADJUST_PIN_RESULT PhMipAdjustPin(
    _In_ PH_MINIINFO_PIN_TYPE PinType,
    _In_ LONG PinCount
    );

VOID PhMipCalculateWindowRectangle(
    _In_ PPOINT SourcePoint,
    _Out_ PPH_RECTANGLE WindowRectangle
    );

#endif