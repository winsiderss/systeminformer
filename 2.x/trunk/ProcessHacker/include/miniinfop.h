#ifndef MINIINFOP_H
#define MINIINFOP_H

// Constants

#define MIP_CONTAINER_CLASSNAME L"ProcessHackerMiniInfo"

#define MIP_TIMER_PIN_FIRST 1
#define MIP_TIMER_PIN_LAST (MIP_TIMER_PIN_FIRST + MaxMiniInfoPinType - 1)

#define MIP_MSG_FIRST (WM_APP + 150)
#define MIP_MSG_UPDATE (WM_APP + 150)
#define MIP_MSG_LAST (WM_APP + 151)

#define MIP_UNPIN_CHILD_CONTROL_DELAY 1000
#define MIP_UNPIN_HOVER_DELAY 250

#define MIP_SEPARATOR_HEIGHT 2
#define MIP_PADDING_SIZE 3

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

VOID PhMipContainerOnActivate(
    _In_ ULONG Type,
    _In_ BOOLEAN Minimized
    );

VOID PhMipContainerOnSize(
    VOID
    );

VOID PhMipContainerOnSizing(
    _In_ ULONG Edge,
    _In_ PRECT DragRectangle
    );

VOID PhMipContainerOnExitSizeMove(
    VOID
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

VOID PhMipOnCommand(
    _In_ ULONG Id,
    _In_ ULONG Code
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

BOOLEAN PhMipOnDrawItem(
    _In_ ULONG_PTR Id,
    _In_ DRAWITEMSTRUCT *DrawItemStruct
    );

VOID PhMipOnUserMessage(
    _In_ ULONG Message,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam
    );

// Framework

typedef enum _MIP_ADJUST_PIN_RESULT
{
    NoAdjustPinResult,
    ShowAdjustPinResult,
    HideAdjustPinResult
} MIP_ADJUST_PIN_RESULT;

BOOLEAN NTAPI PhMipMessageLoopFilter(
    _In_ PMSG Message,
    _In_ PVOID Context
    );

VOID NTAPI PhMipUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

MIP_ADJUST_PIN_RESULT PhMipAdjustPin(
    _In_ PH_MINIINFO_PIN_TYPE PinType,
    _In_ LONG PinCount
    );

VOID PhMipCalculateWindowRectangle(
    _In_ PPOINT SourcePoint,
    _Out_ PPH_RECTANGLE WindowRectangle
    );

VOID PhMipInitializeParameters(
    VOID
    );

PPH_MINIINFO_SECTION PhMipCreateSection(
    _In_ PPH_MINIINFO_SECTION Template
    );

VOID PhMipDestroySection(
    _In_ PPH_MINIINFO_SECTION Section
    );

PPH_MINIINFO_SECTION PhMipFindSection(
    _In_ PPH_STRINGREF Name
    );

PPH_MINIINFO_SECTION PhMipCreateInternalSection(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_ PPH_MINIINFO_SECTION_CALLBACK Callback
    );

VOID PhMipCreateSectionDialog(
    _In_ PPH_MINIINFO_SECTION Section
    );

VOID PhMipChangeSection(
    _In_ PPH_MINIINFO_SECTION NewSection
    );

VOID PhMipSetSectionText(
    _In_ struct _PH_MINIINFO_SECTION *Section,
    _In_opt_ PPH_STRING Text
    );

VOID PhMipUpdateSectionText(
    _In_ PPH_MINIINFO_SECTION Section
    );

VOID PhMipLayout(
    VOID
    );

VOID PhMipBeginChildControlPin(
    VOID
    );

VOID PhMipEndChildControlPin(
    VOID
    );

VOID PhMipSetPinned(
    _In_ BOOLEAN Pinned
    );

LRESULT CALLBACK PhMipSectionControlHookWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// List-based section

PPH_MINIINFO_LIST_SECTION PhMipCreateListSection(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_ PPH_MINIINFO_LIST_SECTION Template
    );

PPH_MINIINFO_LIST_SECTION PhMipCreateInternalListSection(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_ PPH_MINIINFO_LIST_SECTION_CALLBACK Callback,
    _In_opt_ PC_COMPARE_FUNCTION CompareFunction
    );

BOOLEAN PhMipListSectionCallback(
    _In_ PPH_MINIINFO_SECTION Section,
    _In_ PH_MINIINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

INT_PTR CALLBACK PhMipListSectionDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhMipTickListSection(
    _In_ PPH_MINIINFO_LIST_SECTION ListSection
    );

// CPU section

BOOLEAN PhMipCpuListSectionCallback(
    _In_ struct _PH_MINIINFO_LIST_SECTION *ListSection,
    _In_ PH_MINIINFO_LIST_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

#endif