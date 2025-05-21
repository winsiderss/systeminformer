/*
 * Win32k NT User definitions.
 *
 * This file is part of System Informer.
 */

#ifndef _NTUSER_H
#define _NTUSER_H

typedef enum _USERTHREADINFOCLASS USERTHREADINFOCLASS;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserAttachThreadInput(
    _In_ ULONG IdAttach,
    _In_ ULONG IdAttachTo,
    _In_ BOOL Attach
    );

NTSYSCALLAPI
HDC
NTAPI
NtUserBeginPaint(
    _In_ HWND WindowHandle,
    _Inout_ LPPAINTSTRUCT lpPaint
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserBlockInput(
    _In_ BOOL BlockInput
    );

#define FW_BOTH 0
#define FW_16BIT 1
#define FW_32BIT 2

NTSYSCALLAPI
HWND
NTAPI
NtUserFindWindowEx(
    _In_opt_ HWND hwndParent,
    _In_opt_ HWND hwndChild,
    _In_ PCUNICODE_STRING ClassName,
    _In_ PCUNICODE_STRING WindowName,
    _In_ ULONG Type // FW_*
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserBuildHwndList(
    _In_opt_ HANDLE DesktopHandle,
    _In_opt_ HWND StartWindowHandle,
    _In_opt_ LOGICAL IncludeChildren,
    _In_opt_ LOGICAL ExcludeImmersive,
    _In_opt_ ULONG ThreadId,
    _In_ ULONG HwndListInformationLength,
    _Out_writes_bytes_(HwndListInformationLength) PVOID HwndListInformation,
    _Out_ PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserBuildNameList(
    _In_ HWINSTA WindowStationHandle, // GetProcessWindowStation
    _In_ ULONG NameListInformationLength,
    _Out_writes_bytes_(NameListInformationLength) PVOID NameListInformation,
    _Out_opt_ PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserBuildPropList(
    _In_ HWINSTA WindowStationHandle,
    _In_ ULONG PropListInformationLength,
    _Out_writes_bytes_(PropListInformationLength) PVOID PropListInformation,
    _Out_opt_ PULONG ReturnLength
    );

NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCanCurrentThreadChangeForeground(
    VOID
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserCalculatePopupWindowPosition(
    _In_ const POINT* anchorPoint,
    _In_ const SIZE* windowSize,
    _In_ ULONG flags,
    _Inout_ RECT* excludeRect,
    _Inout_ RECT* popupWindowPosition
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserCheckAccessForIntegrityLevel(
    _In_ ULONG ProcessIdFirst,
    _In_ ULONG ProcessIdSecond,
    _Out_ PBOOLEAN GrantedAccess
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserCheckProcessForClipboardAccess(
    _In_ ULONG ProcessId,
    _Out_ PULONG GrantedAccess
    );

NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCloseWindowStation(
    _In_ HWINSTA WindowStationHandle
    );

NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDisableProcessWindowsGhosting(
    VOID
    );

typedef enum _CONSOLECONTROL
{
    ConsoleSetVDMCursorBounds = 0, // RECT
    ConsoleNotifyConsoleApplication = 1, // CONSOLE_PROCESS_INFO
    ConsoleFullscreenSwitch = 2,
    ConsoleSetCaretInfo = 3, // CONSOLE_CARET_INFO
    ConsoleSetReserveKeys = 4,
    ConsoleSetForeground = 5, // CONSOLESETFOREGROUND
    ConsoleSetWindowOwner = 6, // CONSOLEWINDOWOWNER
    ConsoleEndTask = 7, // CONSOLEENDTASK
} CONSOLECONTROL;

#define CPI_NEWPROCESSWINDOW 0x0001

typedef struct _CONSOLE_PROCESS_INFO
{
    ULONG ProcessID;
    ULONG Flags;
} CONSOLE_PROCESS_INFO, *PCONSOLE_PROCESS_INFO;

typedef struct _CONSOLE_CARET_INFO
{
    HWND WindowHandle;
    RECT Rect;
} CONSOLE_CARET_INFO, *PCONSOLE_CARET_INFO;

typedef struct _CONSOLESETFOREGROUND
{
    HANDLE ProcessHandle;
    BOOL Foreground;
} CONSOLESETFOREGROUND, *PCONSOLESETFOREGROUND;

typedef struct _CONSOLEWINDOWOWNER
{
    HWND WindowHandle;
    ULONG ProcessId;
    ULONG ThreadId;
} CONSOLEWINDOWOWNER, *PCONSOLEWINDOWOWNER;

typedef struct _CONSOLEENDTASK
{
    HANDLE ProcessId;
    HWND WindowHandle;
    ULONG ConsoleEventCode;
    ULONG ConsoleFlags;
} CONSOLEENDTASK, *PCONSOLEENDTASK;

/**
 * Performs special kernel operations for console host applications. (win32u.dll)
 *
 * This includes reparenting the console window, allowing the console to pass foreground rights
 * on to launched console subsystem applications and terminating attached processes.
 *
 * @param Command One of the CONSOLECONTROL values indicating which console control function should be executed.
 * @param ConsoleInformation A pointer to one of the  structures specifying additional data for the requested console control function.
 * @param ConsoleInformationLength The size of the structure pointed to by the ConsoleInformation parameter.
 * @return Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserConsoleControl(
    _In_ CONSOLECONTROL Command,
    _In_reads_bytes_(ConsoleInformationLength) PVOID ConsoleInformation,
    _In_ ULONG ConsoleInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserCreateWindowStation(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE KeyboardLayoutHandle,
    _In_opt_ PVOID KeyboardLayoutOffset,
    _In_opt_ PVOID NlsTableOffset,
    _In_opt_ PVOID KeyboardDescriptor,
    _In_opt_ PCUNICODE_STRING LanguageIdString,
    _In_opt_ ULONG KeyboardLocale
    );

/**
 * Performs special kernel operations for console host applications. (user32.dll)
 *
 * This includes reparenting the console window, allowing the console to pass foreground rights
 * on to launched console subsystem applications and terminating attached processes.
 *
 * @param Command One of the CONSOLECONTROL values indicating which console control function should be executed.
 * @param ConsoleInformation A pointer to one of the  structures specifying additional data for the requested console control function.
 * @param ConsoleInformationLength The size of the structure pointed to by the ConsoleInformation parameter.
 * @return Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
ConsoleControl(
    _In_ CONSOLECONTROL Command,
    _In_reads_bytes_(ConsoleInformationLength) PVOID ConsoleInformation,
    _In_ ULONG ConsoleInformationLength
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserGetClassName(
    _In_ HWND WindowHandle,
    _In_ BOOL Real,
    _Out_ PUNICODE_STRING ClassName
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserGetForegroundWindow(
    VOID
    );

NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetIconInfo(
    _In_ HICON IconOrCursorHandle,
    _Out_ PICONINFO Iconinfo,
    _Inout_opt_ PUNICODE_STRING Name,
    _Inout_opt_ PUNICODE_STRING ResourceId,
    _Out_opt_ PULONG ColorBits,
    _In_ LOGICAL IsCursorHandle
    );

NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetIconSize(
    _In_ HGDIOBJ IconOrCursorHandle,
    _In_ LOGICAL IsCursorHandle,
    _Out_ PULONG XX,
    _Out_ PULONG YY
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserGetProcessWindowStation(
    VOID
    );

NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserGetThreadState(
    _In_ ULONG UserThreadState
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserGhostWindowFromHungWindow(
    _In_ HWND WindowHandle
    );

NTSYSAPI
HWND
NTAPI
GhostWindowFromHungWindow(
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserHungWindowFromGhostWindow(
    _In_ HWND WindowHandle
    );

NTSYSAPI
HWND
NTAPI
HungWindowFromGhostWindow(
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
ULONG
NTAPI
NtUserInternalGetWindowText(
    _In_ HWND WindowHandle,
    _Out_writes_to_(cchMaxCount, return + 1) LPWSTR pString,
    _In_ ULONG cchMaxCount
    );

NTSYSCALLAPI
HICON
NTAPI
NtUserInternalGetWindowIcon(
    _In_ HWND WindowHandle,
    _In_ ULONG IconType
    );

NTSYSCALLAPI
HANDLE
NTAPI
NtUserOpenDesktop(
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG Flags,
    _In_ ACCESS_MASK DesiredAccess
    );

/**
 * Opens the specified window station.
 *
 * @param ObjectAttributes The name of the window station to be opened. Window station names are case-insensitive. This window station must belong to the current session.
 * @param DesiredAccess The access to the window station.
 * @return Successful or errant status.
 */
NTSYSCALLAPI
HWINSTA
NTAPI
NtUserOpenWindowStation(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ACCESS_MASK DesiredAccess
    );

typedef enum _WINDOWINFOCLASS
{
    WindowProcess = 0, // q: ULONG (Process ID)
    WindowRealProcess = 1, // q: ULONG (Process ID)
    WindowThread = 2, // q: ULONG (Thread ID)
    WindowActiveWindow = 3, // q: HWND
    WindowFocusWindow = 4, // q: HWND
    WindowIsHung = 5, // q: BOOLEAN
    WindowClientBase = 6, // q: PVOID
    WindowIsForegroundThread = 7, // q: BOOLEAN
    WindowDefaultImeWindow = 8, // q: HWND
    WindowDefaultInputContext = 9, // q: HIMC
} WINDOWINFOCLASS, *PWINDOWINFOCLASS;

NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserQueryWindow(
    _In_ HWND WindowHandle,
    _In_ WINDOWINFOCLASS WindowInfo
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserSetActiveWindow(
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
LPARAM
NTAPI
NtUserSetMessageExtraInfo(
    _In_ LPARAM lParam
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserSetChildWindowNoActivate(
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserSetFocus(
    _In_ HWND WindowHandle
    );

// User32 ordinal 2005
NTSYSAPI
LOGICAL
NTAPI
SetChildWindowNoActivate(
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ USERTHREADINFOCLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength
    );

NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetProcessWindowStation(
    _In_ HWINSTA WindowStationHandle
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserSetWindowPlacement(
    _In_  HWND WindowHandle,
    _Inout_ const WINDOWPLACEMENT* lpwndpl
    );

NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWindowStationUser(
    _In_ HWINSTA WindowStationHandle,
    _In_ PLUID UserLogonId,
    _In_ PSID UserSid,
    _In_ ULONG UserSidLength
    );

NTSYSAPI
LOGICAL
NTAPI
SetWindowStationUser(
    _In_ HWINSTA WindowStationHandle,
    _In_ PLUID UserLogonId,
    _In_ PSID UserSid,
    _In_ ULONG UserSidLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserTestForInteractiveUser(
    _In_ PLUID AuthenticationId
    );

NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSwitchDesktop(
    _In_ HDESK DesktopHandle,
    _In_opt_ ULONG Flags,
    _In_opt_ ULONG FadeTime
    );

NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetThreadDesktop(
    _In_ HDESK DesktopHandle
    );

NTSYSAPI
HWND
NTAPI
ChildWindowFromPoint(
    _In_ HWND WindowHandle,
    _In_ POINT pt
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserChildWindowFromPointEx(
    _In_ HWND WindowHandle,
    _In_ POINT pt,
    _In_ ULONG flags
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserClipCursor(
    _In_ const RECT* lpRect
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserCloseDesktop(
    _In_ HDESK DesktopHandle
    );

NTSYSCALLAPI
LONG
NTAPI
NtUserCopyAcceleratorTable(
    _In_ HACCEL hAccelSrc,
    _In_ LPACCEL lpAccelDst,
    _In_ LONG cAccelEntries
    );

NTSYSCALLAPI
HACCEL
NTAPI
NtUserCreateAcceleratorTable(
    _In_ LPACCEL paccel,
    _In_ LONG cAccel
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserDeleteMenu(
    _In_ HMENU MenuHandle,
    _In_ ULONG Position,
    _In_ ULONG Flags
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserDestroyMenu(
    _In_ HMENU MenuHandle
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserDestroyWindow(
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserDragDetect(
    _In_ HWND WindowHandle,
    _In_ POINT pt
    );

NTSYSCALLAPI
ULONG
NTAPI
NtUserDragObject(
    _In_ HWND WindowHandleParent,
    _In_ HWND WindowHandleFrom,
    _In_ ULONG fmt,
    _In_ ULONG_PTR data,
    _In_ HCURSOR hcur
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserDrawAnimatedRects(
    _In_ HWND WindowHandle,
    _In_ int idAni,
    _In_ const RECT* lprcFrom,
    _In_ const RECT* lprcTo
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserEndMenu(
    VOID
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserEndPaint(
    _In_ HWND WindowHandle,
    _Inout_ const PAINTSTRUCT* lpPaint
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserEnumDisplayMonitors(
    _In_ HDC hdc,
    _In_ LPCRECT lprcClip,
    _In_ MONITORENUMPROC lpfnEnum,
    _In_ LPARAM dwData
    );

NTSYSCALLAPI
HRGN
NTAPI
NtUserExcludeUpdateRgn(
    _In_ HDC hDC,
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserFlashWindowEx(
    _In_ PFLASHWINFO pfwi
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserGetAncestor(
    _In_ HWND WindowHandle,
    _In_ ULONG gaFlags
    );

NTSYSCALLAPI
ULONG
NTAPI
NtUserGetCaretBlinkTime(
    VOID
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserGetCaretPos(
    _In_ LPPOINT lpPoint
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserGetClipCursor(
    _In_ LPRECT lpRect
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserGetComboBoxInfo(
    _In_ HWND WindowHandleCombo,
    _Inout_ PCOMBOBOXINFO pcbi
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserGetCurrentInputMessageSource(
    _Inout_ INPUT_MESSAGE_SOURCE* InputMessageSource
    );

NTSYSCALLAPI
HCURSOR
NTAPI
NtUserGetCursor(
    VOID
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserGetCursorInfo(
    _In_ PCURSORINFO pci
    );

NTSYSCALLAPI
HDC
NTAPI
NtUserGetDCEx(
    _In_ HWND WindowHandle,
    _In_ HRGN hrgnClip,
    _In_ ULONG flags
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserGetDisplayAutoRotationPreferences(
    _In_ ORIENTATION_PREFERENCE* pOrientation
    );

NTSYSCALLAPI
ULONG
NTAPI
NtUserGetDoubleClickTime(
    VOID
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserGetGUIThreadInfo(
    _In_ ULONG idThread,
    _In_ PGUITHREADINFO pgui
    );

#ifndef GR_GDIOBJECTS
#define GR_GDIOBJECTS 0
#endif
#ifndef GR_USEROBJECTS
#define GR_USEROBJECTS 1
#endif
#ifndef GR_GDIOBJECTS_PEAK
#define GR_GDIOBJECTS_PEAK 2
#endif
#ifndef GR_USEROBJECTS_PEAK
#define GR_USEROBJECTS_PEAK 4
#endif

NTSYSCALLAPI
ULONG
NTAPI
NtUserGetGuiResources(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Flags
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserGetLayeredWindowAttributes(
    _In_ HWND WindowHandle,
    _Out_opt_ COLORREF* Key,
    _Out_opt_ PBYTE Alpha,
    _Out_opt_ PULONG Flags
    );

NTSYSCALLAPI
ULONG
NTAPI
NtUserGetListBoxInfo(
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserGetMenuBarInfo(
    _In_ HWND WindowHandle,
    _In_ LONG idObject,
    _In_ LONG idItem,
    _In_ PMENUBARINFO pmbi
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserGetMenuItemRect(
    _In_ HWND WindowHandle,
    _In_ HMENU MenuHandle,
    _In_ ULONG MenuIndex,
    _In_ PRECT MenuRect
    );

NTSYSCALLAPI
LONG
NTAPI
NtUserGetMouseMovePointsEx(
    _In_ ULONG MouseMovePointsSize,
    _In_ LPMOUSEMOVEPOINT InputBuffer,
    _Out_ LPMOUSEMOVEPOINT OutputBuffer,
    _In_ LONG OutputBufferCount,
    _In_ ULONG Resolution
    );

NTSYSCALLAPI
ULONG
NTAPI
NtUserGetRawInputData(
    _In_ HRAWINPUT RawInputData,
    _In_ ULONG RawInputCommand,
    _Out_opt_ PVOID RawInputBuffer,
    _Inout_ PULONG RawInputBufferSize,
    _In_ ULONG RawInputHeaderSize
    );

NTSYSCALLAPI
ULONG
NTAPI
NtUserGetRawInputDeviceList(
    _In_ PRAWINPUTDEVICELIST RawInputDeviceList,
    _Inout_ PULONG RawInputDeviceCount,
    _In_ ULONG RawInputDeviceSize
    );

NTSYSCALLAPI
ULONG
NTAPI
NtUserGetRegisteredRawInputDevices(
    _Out_opt_ PRAWINPUTDEVICE RawInputDevices,
    _Inout_ PULONG RawInputDeviceCount,
    _In_ ULONG RawInputDeviceSize
    );

NTSYSCALLAPI
HMENU
NTAPI
NtUserGetSendMessageReceiver(
    _In_ HANDLE ThreadId
    );

NTSYSAPI
HWND
NTAPI
GetSendMessageReceiver(
    _In_ HANDLE ThreadId
    );

NTSYSCALLAPI
HMENU
NTAPI
NtUserGetSystemMenu(
    _In_ HWND WindowHandle,
    _In_ BOOL Revert
    );

NTSYSCALLAPI
HDESK
NTAPI
NtUserGetThreadDesktop(
    _In_ ULONG ThreadId
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserGetTitleBarInfo(
    _In_ HWND WindowHandle,
    _In_ PTITLEBARINFO pti
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserGetObjectInformation(
    _In_ HANDLE ObjectHandle,
    _In_ LONG Index,
    _In_ PVOID vInfo,
    _In_ ULONG Length,
    _In_ PULONG LengthNeeded
    );

NTSYSCALLAPI
HDC
NTAPI
NtUserGetWindowDC(
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserGetWindowPlacement(
    _In_ HWND WindowHandle,
    _Inout_ PWINDOWPLACEMENT WindowPlacement
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserHiliteMenuItem(
    _In_ HWND WindowHandle,
    _In_ HMENU MenuHandle,
    _In_ ULONG IDHiliteItem,
    _In_ ULONG Hilite
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserInvalidateRect(
    _In_ HWND WindowHandle,
    _In_ const RECT* Rect,
    _In_ BOOL Erase
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserInvalidateRgn(
    _In_ HWND WindowHandle,
    _In_ HRGN RgnHandle,
    _In_ BOOL Erase
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserIsTouchWindow(
    _In_ HWND WindowHandle,
    _In_ PULONG Flags
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserKillTimer(
    _In_ HWND WindowHandle,
    _In_ ULONG_PTR IDEvent
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserLockWorkStation(
    VOID
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserLogicalToPhysicalPoint(
    _In_ HWND WindowHandle,
    _In_ LPPOINT lpPoint
    );

NTSYSCALLAPI
LONG
NTAPI
NtUserMenuItemFromPoint(
    _In_ HWND WindowHandle,
    _In_ HMENU MenuHandle,
    _In_ POINT ptScreen
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserMoveWindow(
    _In_ HWND WindowHandle,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ BOOL Repaint
    );

NTSYSCALLAPI
HDESK
NTAPI
NtUserOpenInputDesktop(
    _In_ ULONG Flags,
    _In_ BOOL Inherit,
    _In_ ACCESS_MASK DesiredAccess
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserPhysicalToLogicalPoint(
    _In_ HWND WindowHandle,
    _In_ LPPOINT Point
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserPrintWindow(
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ ULONG Flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ USERTHREADINFOCLASS ThreadInformationClass,
    _Out_writes_bytes_(ReturnLength) PVOID ThreadInformation,
    _Out_opt_ PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ USERTHREADINFOCLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength
    );

NTSYSAPI
BOOL
NTAPI
QuerySendMessage(
    _Inout_ MSG* pMsg
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRaiseLowerShellWindow(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN SetWithOptions
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserRedrawWindow(
    _In_ HWND WindowHandle,
    _In_ const PRECT lprcUpdate,
    _In_ HRGN hrgnUpdate,
    _In_ ULONG Flags
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserRealChildWindowFromPoint(
    _In_ HWND WindowHandleParent,
    _In_ POINT ptParentClientCoords
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserRegisterHotKey(
    _In_ HWND WindowHandle,
    _In_ LONG Id,
    _In_ ULONG fsModifiers,
    _In_ ULONG vk
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserRemoveMenu(
    _In_ HMENU MenuHandle,
    _In_ ULONG Position,
    _In_ ULONG Flags
    );

NTSYSCALLAPI
ULONG
NTAPI
NtUserSendInput(
    _In_ ULONG Count,
    _In_ LPINPUT Inputs,
    _In_ LONG Size
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserSetActiveWindow(
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserSetCapture(
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserSetTimer(
    _In_ HWND WindowHandle,
    _In_ ULONG_PTR IDEvent,
    _In_ ULONG Elapse,
    _In_ TIMERPROC TimerFunc,
    _In_ ULONG ToleranceDelay
    );

NTSYSCALLAPI
USHORT
NTAPI
NtUserSetClassWord(
    _In_ HWND WindowHandle,
    _In_ LONG Index,
    _In_ USHORT NewWord
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserSetCursorPos(
    _In_ LONG X,
    _In_ LONG Y
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserSetFocus(
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserSetLayeredWindowAttributes(
    _In_ HWND WindowHandle,
    _In_ COLORREF Key,
    _In_ BYTE Alpha,
    _In_ ULONG Flags
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserSetProcessRestrictionExemption(
    _In_ BOOL EnableExemption
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserSetWindowPos(
    _In_ HWND WindowHandle,
    _In_ HWND WindowHandleInsertAfter,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ LONG cx,
    _In_ LONG cy,
    _In_ ULONG Flags
    );

FORCEINLINE
BOOL
NTAPI
NtUserBringWindowToTop(
    _In_ HWND WindowHandle
    )
{
    return NtUserSetWindowPos(
        WindowHandle,
        NULL,
        0, 0, 0, 0,
        3
        );
}

NTSYSCALLAPI
USHORT
NTAPI
NtUserSetWindowWord(
    _In_ HWND WindowHandle,
    _In_ LONG Index,
    _In_ USHORT NewWord
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserSetForegroundWindowForApplication(
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserShellForegroundBoostProcess(
    _In_ HANDLE ProcessHandle,
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
ULONG
NTAPI
NtUserSetAdditionalForegroundBoostProcesses(
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
ULONG
NTAPI
NtUserSetAdditionalPowerThrottlingProcess(
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
LONG
NTAPI
NtUserShowCursor(
    _In_ BOOL Show
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserShowWindow(
    _In_ HWND WindowHandle,
    _In_ LONG CmdShow
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserShowWindowAsync(
    _In_ HWND WindowHandle,
    _In_ LONG CmdShow
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserShutdownBlockReasonQuery(
    _In_ HWND WindowHandle,
    _Out_ PWSTR Buffer,
    _Inout_ PULONG BufferCount
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserShutdownReasonDestroy(
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserTrackMouseEvent(
    _In_ LPTRACKMOUSEEVENT EventTrack
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserTrackPopupMenuEx(
    _In_ HMENU MenuHandle,
    _In_ ULONG Flags,
    _In_ LONG x,
    _In_ LONG y,
    _In_ HWND WindowHandle,
    _In_ LPTPMPARAMS lptpm
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserUnhookWinEvent(
    _In_ HWINEVENTHOOK WinEventHookHandle
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserUnregisterHotKey(
    _In_ HWND WindowHandle,
    _In_ LONG Id
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserUserHandleGrantAccess(
    _In_ HANDLE UserHandle,
    _In_ HANDLE Job,
    _In_ BOOL Grant
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserValidateRect(
    _In_ HWND WindowHandle,
    _In_ const RECT* Rect
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserWindowFromDC(
    _In_ HDC Hdc
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserWindowFromPhysicalPoint(
    _In_ POINT Point
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserWindowFromPoint(
    _In_ POINT Point
    );

typedef NTSTATUS FN_DISPATCH(PVOID);
typedef FN_DISPATCH* PFN_DISPATCH;

// Peb!KernelCallbackTable = user32.dll!apfnDispatch
typedef struct _KERNEL_CALLBACK_TABLE
{
    PFN_DISPATCH __fnCOPYDATA;
    PFN_DISPATCH __fnCOPYGLOBALDATA;
    PFN_DISPATCH __fnEMPTY1;
    PFN_DISPATCH __fnNCDESTROY;
    PFN_DISPATCH __fnDWORDOPTINLPMSG;
    PFN_DISPATCH __fnINOUTDRAG;
    PFN_DISPATCH __fnGETTEXTLENGTHS1;
    PFN_DISPATCH __fnINCNTOUTSTRING;
    PFN_DISPATCH __fnINCNTOUTSTRINGNULL;
    PFN_DISPATCH __fnINLPCOMPAREITEMSTRUCT;
    PFN_DISPATCH __fnINLPCREATESTRUCT;
    PFN_DISPATCH __fnINLPDELETEITEMSTRUCT;
    PFN_DISPATCH __fnINLPDRAWITEMSTRUCT;
    PFN_DISPATCH __fnPOPTINLPUINT1;
    PFN_DISPATCH __fnPOPTINLPUINT2;
    PFN_DISPATCH __fnINLPMDICREATESTRUCT;
    PFN_DISPATCH __fnINOUTLPMEASUREITEMSTRUCT;
    PFN_DISPATCH __fnINLPWINDOWPOS;
    PFN_DISPATCH __fnINOUTLPPOINT51;
    PFN_DISPATCH __fnINOUTLPSCROLLINFO;
    PFN_DISPATCH __fnINOUTLPRECT;
    PFN_DISPATCH __fnINOUTNCCALCSIZE;
    PFN_DISPATCH __fnINOUTLPPOINT52;
    PFN_DISPATCH __fnINPAINTCLIPBRD;
    PFN_DISPATCH __fnINSIZECLIPBRD;
    PFN_DISPATCH __fnINDESTROYCLIPBRD;
    PFN_DISPATCH __fnINSTRINGNULL1;
    PFN_DISPATCH __fnINSTRINGNULL2;
    PFN_DISPATCH __fnINDEVICECHANGE;
    PFN_DISPATCH __fnPOWERBROADCAST;
    PFN_DISPATCH __fnINLPUAHDRAWMENU1;
    PFN_DISPATCH __fnOPTOUTLPDWORDOPTOUTLPDWORD1;
    PFN_DISPATCH __fnOPTOUTLPDWORDOPTOUTLPDWORD2;
    PFN_DISPATCH __fnOUTDWORDINDWORD;
    PFN_DISPATCH __fnOUTLPRECT;
    PFN_DISPATCH __fnOUTSTRING;
    PFN_DISPATCH __fnPOPTINLPUINT3;
    PFN_DISPATCH __fnPOUTLPINT;
    PFN_DISPATCH __fnSENTDDEMSG;
    PFN_DISPATCH __fnINOUTSTYLECHANGE1;
    PFN_DISPATCH __fnHkINDWORD;
    PFN_DISPATCH __fnHkINLPCBTACTIVATESTRUCT;
    PFN_DISPATCH __fnHkINLPCBTCREATESTRUCT;
    PFN_DISPATCH __fnHkINLPDEBUGHOOKSTRUCT;
    PFN_DISPATCH __fnHkINLPMOUSEHOOKSTRUCTEX1;
    PFN_DISPATCH __fnHkINLPKBDLLHOOKSTRUCT;
    PFN_DISPATCH __fnHkINLPMSLLHOOKSTRUCT;
    PFN_DISPATCH __fnHkINLPMSG;
    PFN_DISPATCH __fnHkINLPRECT;
    PFN_DISPATCH __fnHkOPTINLPEVENTMSG;
    PFN_DISPATCH __xxxClientCallDelegateThread;
    PFN_DISPATCH __ClientCallDummyCallback1;
    PFN_DISPATCH __ClientCallDummyCallback2;
    PFN_DISPATCH __fnSHELLWINDOWMANAGEMENTCALLOUT;
    PFN_DISPATCH __fnSHELLWINDOWMANAGEMENTNOTIFY;
    PFN_DISPATCH __ClientCallDummyCallback3;
    PFN_DISPATCH __xxxClientCallDitThread;
    PFN_DISPATCH __xxxClientEnableMMCSS;
    PFN_DISPATCH __xxxClientUpdateDpi;
    PFN_DISPATCH __xxxClientExpandStringW;
    PFN_DISPATCH __ClientCopyDDEIn1;
    PFN_DISPATCH __ClientCopyDDEIn2;
    PFN_DISPATCH __ClientCopyDDEOut1;
    PFN_DISPATCH __ClientCopyDDEOut2;
    PFN_DISPATCH __ClientCopyImage;
    PFN_DISPATCH __ClientEventCallback;
    PFN_DISPATCH __ClientFindMnemChar;
    PFN_DISPATCH __ClientFreeDDEHandle;
    PFN_DISPATCH __ClientFreeLibrary;
    PFN_DISPATCH __ClientGetCharsetInfo;
    PFN_DISPATCH __ClientGetDDEFlags;
    PFN_DISPATCH __ClientGetDDEHookData;
    PFN_DISPATCH __ClientGetListboxString;
    PFN_DISPATCH __ClientGetMessageMPH;
    PFN_DISPATCH __ClientLoadImage;
    PFN_DISPATCH __ClientLoadLibrary;
    PFN_DISPATCH __ClientLoadMenu;
    PFN_DISPATCH __ClientLoadLocalT1Fonts;
    PFN_DISPATCH __ClientPSMTextOut;
    PFN_DISPATCH __ClientLpkDrawTextEx;
    PFN_DISPATCH __ClientExtTextOutW;
    PFN_DISPATCH __ClientGetTextExtentPointW;
    PFN_DISPATCH __ClientCharToWchar;
    PFN_DISPATCH __ClientAddFontResourceW;
    PFN_DISPATCH __ClientThreadSetup;
    PFN_DISPATCH __ClientDeliverUserApc;
    PFN_DISPATCH __ClientNoMemoryPopup;
    PFN_DISPATCH __ClientMonitorEnumProc;
    PFN_DISPATCH __ClientCallWinEventProc;
    PFN_DISPATCH __ClientWaitMessageExMPH;
    PFN_DISPATCH __ClientCallDummyCallback4;
    PFN_DISPATCH __ClientCallDummyCallback5;
    PFN_DISPATCH __ClientImmLoadLayout;
    PFN_DISPATCH __ClientImmProcessKey;
    PFN_DISPATCH __fnIMECONTROL;
    PFN_DISPATCH __fnINWPARAMDBCSCHAR;
    PFN_DISPATCH __fnGETTEXTLENGTHS2;
    PFN_DISPATCH __ClientCallDummyCallback6;
    PFN_DISPATCH __ClientLoadStringW;
    PFN_DISPATCH __ClientLoadOLE;
    PFN_DISPATCH __ClientRegisterDragDrop;
    PFN_DISPATCH __ClientRevokeDragDrop;
    PFN_DISPATCH __fnINOUTMENUGETOBJECT;
    PFN_DISPATCH __ClientPrinterThunk;
    PFN_DISPATCH __fnOUTLPCOMBOBOXINFO;
    PFN_DISPATCH __fnOUTLPSCROLLBARINFO;
    PFN_DISPATCH __fnINLPUAHDRAWMENU2;
    PFN_DISPATCH __fnINLPUAHDRAWMENUITEM;
    PFN_DISPATCH __fnINLPUAHDRAWMENU3;
    PFN_DISPATCH __fnINOUTLPUAHMEASUREMENUITEM;
    PFN_DISPATCH __fnINLPUAHDRAWMENU4;
    PFN_DISPATCH __fnOUTLPTITLEBARINFOEX;
    PFN_DISPATCH __fnTOUCH;
    PFN_DISPATCH __fnGESTURE;
    PFN_DISPATCH __fnPOPTINLPUINT4;
    PFN_DISPATCH __fnPOPTINLPUINT5;
    PFN_DISPATCH __xxxClientCallDefaultInputHandler;
    PFN_DISPATCH __fnEMPTY2;
    PFN_DISPATCH __ClientRimDevCallback;
    PFN_DISPATCH __xxxClientCallMinTouchHitTestingCallback;
    PFN_DISPATCH __ClientCallLocalMouseHooks;
    PFN_DISPATCH __xxxClientBroadcastThemeChange;
    PFN_DISPATCH __xxxClientCallDevCallbackSimple;
    PFN_DISPATCH __xxxClientAllocWindowClassExtraBytes;
    PFN_DISPATCH __xxxClientFreeWindowClassExtraBytes;
    PFN_DISPATCH __fnGETWINDOWDATA;
    PFN_DISPATCH __fnINOUTSTYLECHANGE2;
    PFN_DISPATCH __fnHkINLPMOUSEHOOKSTRUCTEX2;
    PFN_DISPATCH __xxxClientCallDefWindowProc;
    PFN_DISPATCH __fnSHELLSYNCDISPLAYCHANGED;
    PFN_DISPATCH __fnHkINLPCHARHOOKSTRUCT;
    PFN_DISPATCH __fnINTERCEPTEDWINDOWACTION;
    PFN_DISPATCH __xxxTooltipCallback;
    PFN_DISPATCH __xxxClientInitPSBInfo;
    PFN_DISPATCH __xxxClientDoScrollMenu;
    PFN_DISPATCH __xxxClientEndScroll;
    PFN_DISPATCH __xxxClientDrawSize;
    PFN_DISPATCH __xxxClientDrawScrollBar;
    PFN_DISPATCH __xxxClientHitTestScrollBar;
    PFN_DISPATCH __xxxClientTrackInit;
} KERNEL_CALLBACK_TABLE, *PKERNEL_CALLBACK_TABLE;

#endif
