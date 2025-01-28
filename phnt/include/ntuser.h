/*
 * Win32k NT User definitions.
 *
 * This file is part of System Informer.
 */

#ifndef _NTUSER_H
#define _NTUSER_H

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
NTSTATUS
NTAPI
NtUserTestForInteractiveUser(
    _In_ PLUID AuthenticationId
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
ULONG
NTAPI
NtUserGetClassName(
    _In_ HWND WindowHandle,
    _In_ LONGLONG Real,
    _Out_ PUNICODE_STRING ClassName
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
NTSYSCALLAPI
NTSTATUS
NTAPI
ConsoleControl(
    _In_ CONSOLECONTROL Command,
    _In_reads_bytes_(ConsoleInformationLength) PVOID ConsoleInformation,
    _In_ ULONG ConsoleInformationLength
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
    _In_ OBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ACCESS_MASK DesiredAccess
    );

NTSYSCALLAPI
HWINSTA
NTAPI
NtUserCreateWindowStation(
    _In_ OBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE KeyboardLayoutHandle,
    _In_opt_ PVOID KeyboardLayoutOffset,
    _In_opt_ PVOID NlsTableOffset,
    _In_opt_ PVOID KeyboardDescriptor,
    _In_opt_ UNICODE_STRING LanguageIdString,
    _In_opt_ ULONG KeyboardLocale
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
HWND
NTAPI
NtUserGetProcessWindowStation(
    VOID
    );

NTSYSCALLAPI
HWND
NTAPI
NtUserGhostWindowFromHungWindow(
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
GhostWindowFromHungWindow(
    _In_ HWND WindowHandle
    );

NTSYSAPI
HWND
NTAPI
HungWindowFromGhostWindow(
    _In_ HWND WindowHandle
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
NtUserSetProcessWindowStation(
    _In_ HWINSTA WindowStationHandle
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
LOGICAL
NTAPI
NtUserSetChildWindowNoActivate(
    _In_ HWND WindowHandle
    );

// User32 ordinal 2005
NTSYSCALLAPI
LOGICAL
NTAPI
SetChildWindowNoActivate(
    _In_ HWND WindowHandle
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

NTSYSCALLAPI
HANDLE
NTAPI
NtUserOpenDesktop(
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG Flags,
    _In_ ACCESS_MASK DesiredAccess
    );

NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetThreadDesktop(
    _In_ HDESK DesktopHandle
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
NtUserGetForegroundWindow(
    VOID
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
NtUserSetFocus(
    _In_ HWND WindowHandle
    );

NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserGetThreadState(
    _In_ ULONG UserThreadState
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserSetWindowPlacement(
    _In_  HWND WindowHandle,
    _Inout_ const WINDOWPLACEMENT* lpwndpl
    );

NTSYSCALLAPI
BOOL
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

NTSYSCALLAPI
BOOL
NTAPI
tUserCalculatePopupWindowPosition(
    _In_ const POINT* anchorPoint,
    _In_ const SIZE* windowSize,
    _In_ ULONG flags,
    _Inout_ RECT* excludeRect,
    _Inout_ RECT* popupWindowPosition
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserChangeWindowMessageFilterEx(
    _In_ HWND WindowHandle,
    _In_ ULONG message,
    _In_ ULONG action,
    _Inout_ PCHANGEFILTERSTRUCT pChangeFilterStruct
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
    _In_ HDESK hDesktop
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
    _In_ HMENU hMenu,
    _In_ ULONG uPosition,
    _In_ ULONG uFlags
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserDestroyMenu(
    _In_ HMENU hMenu
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
    _In_ ULONG uiFlags
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserGetLayeredWindowAttributes(
    _In_ HWND WindowHandle,
    _In_ COLORREF* pcrKey,
    _In_ BYTE* pbAlpha,
    _In_ ULONG pdwFlags
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
    _In_ HMENU hMenu,
    _In_ ULONG uItem,
    _In_ LPRECT lprcItem
    );

NTSYSCALLAPI
LONG
NTAPI
NtUserGetMouseMovePointsEx(
    _In_ ULONG cbSize,
    _In_ LPMOUSEMOVEPOINT lppt,
    _In_ LPMOUSEMOVEPOINT lpptBuf,
    _In_ LONG nBufPoints,
    _In_ ULONG resolution
    );

NTSYSCALLAPI
ULONG
NTAPI
NtUserGetRawInputData(
    _In_ HRAWINPUT hRawInput,
    _In_ ULONG uiCommand,
    _In_ LPVOID pData,
    _In_ PULONG pcbSize,
    _In_ ULONG cbSizeHeader
    );

NTSYSCALLAPI
ULONG
NTAPI
NtUserGetRawInputDeviceList(
    _In_ PRAWINPUTDEVICELIST pRawInputDeviceList,
    _In_ PULONG puiNumDevices,
    _In_ ULONG cbSize
    );

NTSYSCALLAPI
ULONG
NTAPI
NtUserGetRegisteredRawInputDevices(
    _In_ PRAWINPUTDEVICE pRawInputDevices,
    _In_ PULONG puiNumDevices,
    _In_ ULONG cbSize
    );

NTSYSCALLAPI
HMENU
NTAPI
NtUserGetSystemMenu(
    _In_ HWND WindowHandle,
    _In_ BOOL bRevert
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
    _In_ HANDLE hObj,
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
    _In_opt_ WINDOWPLACEMENT* lpwndpl
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserHiliteMenuItem(
    _In_ HWND WindowHandle,
    _In_ HMENU Menu,
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
    _In_ HRGN hRgn,
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
    _In_ HMENU hMenu,
    _In_ POINT ptScreen
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserMoveWindow(
    _In_ HWND WindowHandle,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ LONG nWidth,
    _In_ LONG nHeight,
    _In_ BOOL bRepaint
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
    _In_ LPPOINT lpPoint
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserPrintWindow(
    _In_ HWND WindowHandle,
    _In_ HDC hdcBlt,
    _In_ ULONG nFlags
    );

typedef enum _USERTHREADINFOCLASS USERTHREADINFOCLASS;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ USERTHREADINFOCLASS ThreadInformationClass,
    _Out_writes_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
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

NTSYSCALLAPI
BOOL
NTAPI
QuerySendMessage(
    _Inout_ MSG* pMsg
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserRedrawWindow(
    _In_ HWND WindowHandle,
    _In_ const PRECT lprcUpdate,
    _In_ HRGN hrgnUpdate,
    _In_ ULONG flags
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
    _In_ LONG id,
    _In_ ULONG fsModifiers,
    _In_ ULONG vk
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserRemoveMenu(
    _In_ HMENU hMenu,
    _In_ ULONG uPosition,
    _In_ ULONG uFlags
    );

NTSYSCALLAPI
ULONG
NTAPI
NtUserSendInput(
    _In_ ULONG cInputs,
    _In_ LPINPUT pInputs,
    _In_ LONG cbSize
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
    _In_ ULONG_PTR nIDEvent,
    _In_ ULONG uElapse,
    _In_ TIMERPROC lpTimerFunc,
    _In_ ULONG uToleranceDelay
    );

NTSYSCALLAPI
WORD
NTAPI
NtUserSetClassWord(
    _In_ HWND WindowHandle,
    _In_ LONG nIndex,
    _In_ WORD wNewWord
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
    _In_ COLORREF crKey,
    _In_ BYTE bAlpha,
    _In_ DWORD dwFlags
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
    _In_ ULONG uFlags
    );

NTSYSCALLAPI
WORD
NTAPI
NtUserSetWindowWord(
    _In_ HWND WindowHandle,
    _In_ LONG nIndex,
    _In_ WORD wNewWord
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
    _In_ BOOL bShow
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserShowWindow(
    _In_ HWND WindowHandle,
    _In_ LONG nCmdShow
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserShowWindowAsync(
    _In_ HWND WindowHandle,
    _In_ LONG nCmdShow
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserShutdownBlockReasonQuery(
    _In_ HWND WindowHandle,
    _Out_ LPWSTR pwszBuff,
    _Inout_ PULONG pcchBuff
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
    _In_ LPTRACKMOUSEEVENT lpEventTrack
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserTrackPopupMenuEx(
    _In_ HMENU hMenu,
    _In_ ULONG uFlags,
    _In_ LONG x,
    _In_ LONG y,
    _In_ HWND WindowHandle,
    _In_ LPTPMPARAMS lptpm
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserUnhookWinEvent(
    _In_ HWINEVENTHOOK hWinEventHook
    );

NTSYSCALLAPI
BOOL
NTAPI
NtUserUnregisterHotKey(
    _In_ HWND WindowHandle,
    _In_ LONG id
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
    _In_ HDC hDC
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

// Peb!KernelCallbackTable = user32.dll!apfnDispatch
typedef struct _KERNEL_CALLBACK_TABLE
{
    ULONG_PTR __fnCOPYDATA;
    ULONG_PTR __fnCOPYGLOBALDATA;
    ULONG_PTR __fnEMPTY1;
    ULONG_PTR __fnNCDESTROY;
    ULONG_PTR __fnDWORDOPTINLPMSG;
    ULONG_PTR __fnINOUTDRAG;
    ULONG_PTR __fnGETTEXTLENGTHS1;
    ULONG_PTR __fnINCNTOUTSTRING;
    ULONG_PTR __fnINCNTOUTSTRINGNULL;
    ULONG_PTR __fnINLPCOMPAREITEMSTRUCT;
    ULONG_PTR __fnINLPCREATESTRUCT;
    ULONG_PTR __fnINLPDELETEITEMSTRUCT;
    ULONG_PTR __fnINLPDRAWITEMSTRUCT;
    ULONG_PTR __fnPOPTINLPUINT1;
    ULONG_PTR __fnPOPTINLPUINT2;
    ULONG_PTR __fnINLPMDICREATESTRUCT;
    ULONG_PTR __fnINOUTLPMEASUREITEMSTRUCT;
    ULONG_PTR __fnINLPWINDOWPOS;
    ULONG_PTR __fnINOUTLPPOINT51;
    ULONG_PTR __fnINOUTLPSCROLLINFO;
    ULONG_PTR __fnINOUTLPRECT;
    ULONG_PTR __fnINOUTNCCALCSIZE;
    ULONG_PTR __fnINOUTLPPOINT52;
    ULONG_PTR __fnINPAINTCLIPBRD;
    ULONG_PTR __fnINSIZECLIPBRD;
    ULONG_PTR __fnINDESTROYCLIPBRD;
    ULONG_PTR __fnINSTRINGNULL1;
    ULONG_PTR __fnINSTRINGNULL2;
    ULONG_PTR __fnINDEVICECHANGE;
    ULONG_PTR __fnPOWERBROADCAST;
    ULONG_PTR __fnINLPUAHDRAWMENU1;
    ULONG_PTR __fnOPTOUTLPDWORDOPTOUTLPDWORD1;
    ULONG_PTR __fnOPTOUTLPDWORDOPTOUTLPDWORD2;
    ULONG_PTR __fnOUTDWORDINDWORD;
    ULONG_PTR __fnOUTLPRECT;
    ULONG_PTR __fnOUTSTRING;
    ULONG_PTR __fnPOPTINLPUINT3;
    ULONG_PTR __fnPOUTLPINT;
    ULONG_PTR __fnSENTDDEMSG;
    ULONG_PTR __fnINOUTSTYLECHANGE1;
    ULONG_PTR __fnHkINDWORD;
    ULONG_PTR __fnHkINLPCBTACTIVATESTRUCT;
    ULONG_PTR __fnHkINLPCBTCREATESTRUCT;
    ULONG_PTR __fnHkINLPDEBUGHOOKSTRUCT;
    ULONG_PTR __fnHkINLPMOUSEHOOKSTRUCTEX1;
    ULONG_PTR __fnHkINLPKBDLLHOOKSTRUCT;
    ULONG_PTR __fnHkINLPMSLLHOOKSTRUCT;
    ULONG_PTR __fnHkINLPMSG;
    ULONG_PTR __fnHkINLPRECT;
    ULONG_PTR __fnHkOPTINLPEVENTMSG;
    ULONG_PTR __xxxClientCallDelegateThread;
    ULONG_PTR __ClientCallDummyCallback1;
    ULONG_PTR __ClientCallDummyCallback2;
    ULONG_PTR __fnSHELLWINDOWMANAGEMENTCALLOUT;
    ULONG_PTR __fnSHELLWINDOWMANAGEMENTNOTIFY;
    ULONG_PTR __ClientCallDummyCallback3;
    ULONG_PTR __xxxClientCallDitThread;
    ULONG_PTR __xxxClientEnableMMCSS;
    ULONG_PTR __xxxClientUpdateDpi;
    ULONG_PTR __xxxClientExpandStringW;
    ULONG_PTR __ClientCopyDDEIn1;
    ULONG_PTR __ClientCopyDDEIn2;
    ULONG_PTR __ClientCopyDDEOut1;
    ULONG_PTR __ClientCopyDDEOut2;
    ULONG_PTR __ClientCopyImage;
    ULONG_PTR __ClientEventCallback;
    ULONG_PTR __ClientFindMnemChar;
    ULONG_PTR __ClientFreeDDEHandle;
    ULONG_PTR __ClientFreeLibrary;
    ULONG_PTR __ClientGetCharsetInfo;
    ULONG_PTR __ClientGetDDEFlags;
    ULONG_PTR __ClientGetDDEHookData;
    ULONG_PTR __ClientGetListboxString;
    ULONG_PTR __ClientGetMessageMPH;
    ULONG_PTR __ClientLoadImage;
    ULONG_PTR __ClientLoadLibrary;
    ULONG_PTR __ClientLoadMenu;
    ULONG_PTR __ClientLoadLocalT1Fonts;
    ULONG_PTR __ClientPSMTextOut;
    ULONG_PTR __ClientLpkDrawTextEx;
    ULONG_PTR __ClientExtTextOutW;
    ULONG_PTR __ClientGetTextExtentPointW;
    ULONG_PTR __ClientCharToWchar;
    ULONG_PTR __ClientAddFontResourceW;
    ULONG_PTR __ClientThreadSetup;
    ULONG_PTR __ClientDeliverUserApc;
    ULONG_PTR __ClientNoMemoryPopup;
    ULONG_PTR __ClientMonitorEnumProc;
    ULONG_PTR __ClientCallWinEventProc;
    ULONG_PTR __ClientWaitMessageExMPH;
    ULONG_PTR __ClientCallDummyCallback4;
    ULONG_PTR __ClientCallDummyCallback5;
    ULONG_PTR __ClientImmLoadLayout;
    ULONG_PTR __ClientImmProcessKey;
    ULONG_PTR __fnIMECONTROL;
    ULONG_PTR __fnINWPARAMDBCSCHAR;
    ULONG_PTR __fnGETTEXTLENGTHS2;
    ULONG_PTR __ClientCallDummyCallback6;
    ULONG_PTR __ClientLoadStringW;
    ULONG_PTR __ClientLoadOLE;
    ULONG_PTR __ClientRegisterDragDrop;
    ULONG_PTR __ClientRevokeDragDrop;
    ULONG_PTR __fnINOUTMENUGETOBJECT;
    ULONG_PTR __ClientPrinterThunk;
    ULONG_PTR __fnOUTLPCOMBOBOXINFO;
    ULONG_PTR __fnOUTLPSCROLLBARINFO;
    ULONG_PTR __fnINLPUAHDRAWMENU2;
    ULONG_PTR __fnINLPUAHDRAWMENUITEM;
    ULONG_PTR __fnINLPUAHDRAWMENU3;
    ULONG_PTR __fnINOUTLPUAHMEASUREMENUITEM;
    ULONG_PTR __fnINLPUAHDRAWMENU4;
    ULONG_PTR __fnOUTLPTITLEBARINFOEX;
    ULONG_PTR __fnTOUCH;
    ULONG_PTR __fnGESTURE;
    ULONG_PTR __fnPOPTINLPUINT4;
    ULONG_PTR __fnPOPTINLPUINT5;
    ULONG_PTR __xxxClientCallDefaultInputHandler;
    ULONG_PTR __fnEMPTY2;
    ULONG_PTR __ClientRimDevCallback;
    ULONG_PTR __xxxClientCallMinTouchHitTestingCallback;
    ULONG_PTR __ClientCallLocalMouseHooks;
    ULONG_PTR __xxxClientBroadcastThemeChange;
    ULONG_PTR __xxxClientCallDevCallbackSimple;
    ULONG_PTR __xxxClientAllocWindowClassExtraBytes;
    ULONG_PTR __xxxClientFreeWindowClassExtraBytes;
    ULONG_PTR __fnGETWINDOWDATA;
    ULONG_PTR __fnINOUTSTYLECHANGE2;
    ULONG_PTR __fnHkINLPMOUSEHOOKSTRUCTEX2;
    ULONG_PTR __xxxClientCallDefWindowProc;
    ULONG_PTR __fnSHELLSYNCDISPLAYCHANGED;
    ULONG_PTR __fnHkINLPCHARHOOKSTRUCT;
    ULONG_PTR __fnINTERCEPTEDWINDOWACTION;
    ULONG_PTR __xxxTooltipCallback;
    ULONG_PTR __xxxClientInitPSBInfo;
    ULONG_PTR __xxxClientDoScrollMenu;
    ULONG_PTR __xxxClientEndScroll;
    ULONG_PTR __xxxClientDrawSize;
    ULONG_PTR __xxxClientDrawScrollBar;
    ULONG_PTR __xxxClientHitTestScrollBar;
    ULONG_PTR __xxxClientTrackInit;
} KERNEL_CALLBACK_TABLE, *PKERNEL_CALLBACK_TABLE;

#endif
