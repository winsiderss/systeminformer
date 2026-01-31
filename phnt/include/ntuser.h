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
    _In_opt_ HWND ParentWindowHandle,
    _In_opt_ BOOL IncludeChildren,
    _In_opt_ BOOL ExcludeImmersive,
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
BOOL
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
BOOL
NTAPI
NtUserCloseWindowStation(
    _In_ HWINSTA WindowStationHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
/**
 * The NtUserDisableProcessWindowFiltering routine disables window filtering
 * so you can enumerate immersive windows from the desktop.
 *
 * \return Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/sbscs/application-manifests#disableWindowFiltering
 */
NTSYSCALLAPI
BOOL
NTAPI
NtUserDisableProcessWindowFiltering(
    VOID
    );
#endif

NTSYSCALLAPI
HANDLE
NTAPI
NtUserGetProp(
    _In_ HWND WindowHandle,
    _In_ PCWSTR String
    );

NTSYSCALLAPI
HANDLE
NTAPI
NtUserGetProp2(
    _In_ HWND WindowHandle,
    _In_ PCUNICODE_STRING String
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
 * \param Command One of the CONSOLECONTROL values indicating which console control function should be executed.
 * \param ConsoleInformation A pointer to one of the  structures specifying additional data for the requested console control function.
 * \param ConsoleInformationLength The size of the structure pointed to by the ConsoleInformation parameter.
 * \return Successful or errant status.
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
 * \param Command One of the CONSOLECONTROL values indicating which console control function should be executed.
 * \param ConsoleInformation A pointer to one of the  structures specifying additional data for the requested console control function.
 * \param ConsoleInformationLength The size of the structure pointed to by the ConsoleInformation parameter.
 * \return Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
ConsoleControl(
    _In_ CONSOLECONTROL Command,
    _In_reads_bytes_(ConsoleInformationLength) PVOID ConsoleInformation,
    _In_ ULONG ConsoleInformationLength
    );

/**
 * The NtUserGetClassName routine retrieves a string that specifies the window type.
 *
 * \param WindowHandle A handle to the window and, indirectly, the class to which the window belongs.
 * \param RealClassName Return the superclass or baseclass name when the window is a superclass.
 * \param ClassName A pointer to a string that receives the window type.
 * \return A handle to the foreground window, or NULL if no foreground window exists.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-realgetwindowclassw
 */
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetClassName(
    _In_ HWND WindowHandle,
    _In_ BOOL RealClassName,
    _Out_ PUNICODE_STRING ClassName
    );

/**
 * The NtUserGetForegroundWindow routine retrieves a handle to the foreground window.
 *
 * \return A handle to the foreground window, or NULL if no foreground window exists.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getforegroundwindow
 */
NTSYSCALLAPI
HWND
NTAPI
NtUserGetForegroundWindow(
    VOID
    );

NTSYSCALLAPI
BOOL
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
BOOL
NTAPI
NtUserGetIconSize(
    _In_ HGDIOBJ IconOrCursorHandle,
    _In_ LOGICAL IsCursorHandle,
    _Out_ PULONG XX,
    _Out_ PULONG YY
    );

/**
 * The NtUserGetProcessWindowStation routine retrieves the window station handle associated with the current process.
 *
 * \return A handle to the window station, or NULL if the operation fails.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getprocesswindowstation
 */
NTSYSCALLAPI
HWINSTA
NTAPI
NtUserGetProcessWindowStation(
    VOID
    );

typedef enum _PROCESS_UICONTEXT
{
    PROCESS_UICONTEXT_DESKTOP,
    PROCESS_UICONTEXT_IMMERSIVE,
    PROCESS_UICONTEXT_IMMERSIVE_BROKER,
    PROCESS_UICONTEXT_IMMERSIVE_BROWSER
} PROCESS_UICONTEXT;

typedef enum _PROCESS_UI_FLAGS
{
    PROCESS_UIF_NONE,
    PROCESS_UIF_AUTHORING_MODE,
    PROCESS_UIF_RESTRICTIONS_DISABLED
} PROCESS_UI_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(PROCESS_UI_FLAGS);

typedef struct _PROCESS_UICONTEXT_INFORMATION
{
    PROCESS_UICONTEXT ProcessUIContext;
    PROCESS_UI_FLAGS Flags;
} PROCESS_UICONTEXT_INFORMATION, *PPROCESS_UICONTEXT_INFORMATION;
    
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserGetProcessUIContextInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_UICONTEXT_INFORMATION UIContext
    );
    
NTSYSCALLAPI
BOOL
NTAPI
GetProcessUIContextInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_UICONTEXT_INFORMATION UIContext
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
 * \param ObjectAttributes The name of the window station to be opened. Window station names are case-insensitive. This window station must belong to the current session.
 * \param DesiredAccess The access to the window station.
 * \return Successful or errant status.
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
BOOL
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
BOOL
NTAPI
NtUserSetProcessWindowStation(
    _In_ HWINSTA WindowStationHandle
    );

// rev // Valid bit masks enforced by NtUserSetProcessWin32Capabilities
#define PROC_CAP_FLAGS1_VALID_MASK     0x00000007u    // bits 0-2
#define PROC_CAP_FLAGS2_VALID_MASK     0x00000007u    // bits 0-2
#define PROC_CAP_ENABLE_VALID_MASK     0x00000001u    // bit 0
#define PROC_CAP_DISABLE_VALID_MASK    0x00000001u    // bit 0

#define PROC_CAP_FLAGS1_BIT0           0x00000001u
#define PROC_CAP_FLAGS1_BIT1           0x00000002u
#define PROC_CAP_FLAGS1_BIT2           0x00000004u

#define PROC_CAP_FLAGS2_BIT0           0x00000001u
#define PROC_CAP_FLAGS2_BIT1           0x00000002u
#define PROC_CAP_FLAGS2_BIT2           0x00000004u

#define PROC_CAP_ENABLE_BIT0           0x00000001u
#define PROC_CAP_DISABLE_BIT0          0x00000001u

#define PROC_CAP_FLAGS1_INVALID(x)     (((x) & ~PROC_CAP_FLAGS1_VALID_MASK) != 0)
#define PROC_CAP_FLAGS2_INVALID(x)     (((x) & ~PROC_CAP_FLAGS2_VALID_MASK) != 0)
#define PROC_CAP_ENABLE_INVALID(x)     (((x) & ~PROC_CAP_ENABLE_VALID_MASK) != 0)
#define PROC_CAP_DISABLE_INVALID(x)    (((x) & ~PROC_CAP_DISABLE_VALID_MASK) != 0)

// rev
typedef struct _USER_PROCESS_CAP_ENTRY
{
    HANDLE ProcessHandle;
    ULONG Flags1;
    ULONG Flags2;
    ULONG EnableMask;
    ULONG DisableMask;
} USER_PROCESS_CAP_ENTRY, *PUSER_PROCESS_CAP_ENTRY;

// rev
typedef struct _USER_PROCESS_CAP_INTERNAL
{
    PVOID ProcessObject;
    ULONG SessionId;
    ULONG Reserved;
    ULONGLONG FlagsPacked;
    ULONGLONG CapabilityPacked;
} USER_PROCESS_CAP_INTERNAL, *PUSER_PROCESS_CAP_INTERNAL;

// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserSetProcessWin32Capabilities(
    _In_reads_(Count) const USER_PROCESS_CAP_ENTRY* Capabilities,
    _In_ ULONG Count
    );

// rev
/**
 * The NtUserSetProcessRestrictionExemption routine marks the current process as exempt from certain win32k/user restrictions.
 * Note: This requires a developer mode/license check.
 *
 * \param Enable Indicates whether to enable or disable the exemption.
 * \return Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserSetProcessRestrictionExemption(
    _In_ BOOL EnableExemption
    );

// rev
/**
 * The NtUserSetProcessUIAccessZorder routine tweaks window z-order behavior for UIAccess scenarios.
 * Note: Set only when the process is not elevated.
 *
 * \return Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserSetProcessUIAccessZorder(
    VOID
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
NtUserSetWindowStationUser(
    _In_ HWINSTA WindowStationHandle,
    _In_ PLUID UserLogonId,
    _In_ PSID UserSid,
    _In_ ULONG UserSidLength
    );

NTSYSAPI
BOOL
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
BOOL
NTAPI
NtUserSwitchDesktop(
    _In_ HDESK DesktopHandle,
    _In_opt_ ULONG Flags,
    _In_opt_ ULONG FadeTime
    );

NTSYSCALLAPI
BOOL
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
HANDLE
NTAPI
GetWindowProcessHandle(
    _In_ HWND WindowHandle,
    _In_ ACCESS_MASK DesiredAccess
    );

NTSYSCALLAPI
HANDLE
NTAPI
NtUserGetWindowProcessHandle(
    _In_ HWND WindowHandle,
    _In_ ACCESS_MASK DesiredAccess
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
    _Out_writes_bytes_(*ReturnLength) PVOID ThreadInformation,
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
NtUserQuerySendMessage(
    _Inout_ PMSG Message
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
NtUserSetWindowPos(
    _In_ HWND WindowHandle,
    _In_opt_ HWND WindowHandleInsertAfter,
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
// Send to the window registered with NtUserRegisterCloakedNotification
// when cloak state of the window has changed
// wParam - if window cloak state changed contains cloaking value
//          which can be one/all of the below
//          DWM_CLOAKED_APP(0x0000001).The window was cloaked by its owner application.
//          DWM_CLOAKED_SHELL(0x0000002).The window was cloaked by the Shell.
//          0 - window is not cloaked
//
// lParam - 0 (unused)
//
#define WM_CLOAKED_STATE_CHANGED 0x0347

NTSYSCALLAPI
BOOL
NTAPI
NtUserRegisterCloakedNotification(
    _In_ HWND WindowHandle,
    _In_ BOOL Register
    );

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

// rev
NTSYSCALLAPI
ULONG
NTAPI
NtUserSetAdditionalPowerThrottlingProcess(
    _In_ HWND WindowHandle,
    _In_ ULONG ProcessHandlesCount,
    _In_reads_(ProcessHandlesCount) PHANDLE ProcessHandles
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

// rev // valid since 20H1
#define SFI_CREATEMENU                              0 // NtUserCallNoParam
#define SFI_CREATEPOPUPMENU                         1 // NtUserCallNoParam
#define SFI_ALLOWFOREGROUNDACTIVATION               2 // NtUserCallNoParam
#define SFI_CANCELQUEUEEVENTCOMPLETIONPACKET        3 // NtUserCallNoParam
#define SFI_CLEARWAKEMASK                           4 // NtUserCallNoParam
#define SFI_CREATESYSTEMTHREADS                     5 // NtUserCallNoParam
#define SFI_DESTROYCARET                            6 // NtUserCallNoParam
#define SFI_DISABLEPROCESSWINDOWSGHOSTING           7 // NtUserCallNoParam
#define SFI_DRAINTHREADCOREMESSAGINGCOMPLETIONS     8 // NtUserCallNoParam
#define SFI_GETDEVICECHANGEINFO                     9 // NtUserCallNoParam
#define SFI_GETIMESHOWSTATUS                        10 // NtUserCallNoParam
#define SFI_GETINPUTDESKTOP                         11 // NtUserCallNoParam
#define SFI_GETMESSAGEPOS                           12 // NtUserCallNoParam
#define SFI_GETQUEUEIOCP                            13 // NtUserCallNoParam
#define SFI_GETUNPREDICTEDMESSAGEPOS                14 // NtUserCallNoParam
#define SFI_HANDLESYSTEMTHREADCREATIONFAILURE       15 // NtUserCallNoParam
#define SFI_HIDECURSORNOCAPTURE                     16 // NtUserCallNoParam
#define SFI_ISQUEUEATTACHED                         17 // NtUserCallNoParam
#define SFI_LOADCURSORSANDICONS                     18 // NtUserCallNoParam
#define SFI_LOADUSERAPIHOOK                         19 // NtUserCallNoParam
#define SFI_PREPAREFORLOGOFF                        20 // NtUserCallNoParam
#define SFI_REASSOCIATEQUEUEEVENTCOMPLETIONPACKET   21 // NtUserCallNoParam
#define SFI_RELEASECAPTURE                          22 // NtUserCallNoParam
#define SFI_REMOVEQUEUECOMPLETION                   23 // NtUserCallNoParam
#define SFI_RESETDBLCLK                             24 // NtUserCallNoParam
#define SFI_ZAPACTIVEANDFOCUS                       25 // NtUserCallNoParam
#define SFI_REMOTECONSOLESHADOWSTOP                 26 // NtUserCallNoParam
#define SFI_REMOTEDISCONNECT                        27 // NtUserCallNoParam
#define SFI_REMOTESHADOWSETUP                       30 // NtUserCallNoParam
#define SFI_REMOTESHADOWSTOP                        31 // NtUserCallNoParam
#define SFI_REMOTEPASSTHRUENABLE                    32 // NtUserCallNoParam
#define SFI_REMOTEPASSTHRUDISABLE                   33 // NtUserCallNoParam
#define SFI_REMOTECONNECTSTATE                      34 // NtUserCallNoParam
#define SFI_UPDATEPERUSERIMMENABLING                36 // NtUserCallNoParam
#define SFI_USERPOWERCALLOUTWORKER                  37 // NtUserCallNoParam
#define SFI_WAKERITFORSHUTDOWN                      38 // NtUserCallNoParam
#define SFI_DOINITMESSAGEPUMPHOOK                   39 // NtUserCallNoParam
#define SFI_DOUNINITMESSAGEPUMPHOOK                 40 // NtUserCallNoParam
#define SFI_ENABLEMOUSEINPOINTERFORTHREAD           41 // NtUserCallNoParam
#define SFI_DEFERREDDESKTOPROTATION                 42 // NtUserCallNoParam
#define SFI_ENABLEPERMONITORMENUSCALING             43 // NtUserCallNoParam
#define SFI_BEGINDEFERWINDOWPOS                     44 // NtUserCallOneParam
#define SFI_GETSENDMESSAGERECEIVER                  45 // NtUserCallOneParam
#define SFI_ALLOWSETFOREGROUNDWINDOW                46 // NtUserCallOneParam
#define SFI_CSDDEUNINITIALIZE                       47 // NtUserCallOneParam
#define SFI_ENUMCLIPBOARDFORMATS                    49 // NtUserCallOneParam
#define SFI_GETINPUTEVENT                           50 // NtUserCallOneParam
#define SFI_GETKEYBOARDTYPE                         51 // NtUserCallOneParam
#define SFI_GETPROCESSDEFAULTLAYOUT                 52 // NtUserCallOneParam
#define SFI_GETWINSTATIONINFO                       53 // NtUserCallOneParam
#define SFI_LOCKSETFOREGROUNDWINDOW                 54 // NtUserCallOneParam
#define SFI_LW_LOADFONTS                            55 // NtUserCallOneParam
#define SFI_MAPDESKTOPOBJECT                        56 // NtUserCallOneParam
#define SFI_MESSAGEBEEP                             57 // NtUserCallOneParam
#define SFI_PLAYEVENTSOUND                          58 // NtUserCallOneParam
#define SFI_POSTQUITMESSAGE                         59 // NtUserCallOneParam
#define SFI_REALIZEPALETTE                          60 // NtUserCallOneParam
#define SFI_REGISTERLPK                             61 // NtUserCallOneParam
#define SFI_REGISTERSYSTEMTHREAD                    62 // NtUserCallOneParam
#define SFI_REMOTERECONNECT                         63 // NtUserCallOneParam
#define SFI_REMOTETHINWIRESTATS                     64 // NtUserCallOneParam
#define SFI_REMOTENOTIFY                            65 // NtUserCallOneParam
#define SFI_REPLYMESSAGE                            66 // NtUserCallOneParam
#define SFI_SETCARETBLINKTIME                       67 // NtUserCallOneParam
#define SFI_SETDOUBLECLICKTIME                      68 // NtUserCallOneParam
#define SFI_SETMESSAGEEXTRAINFO                     69 // NtUserCallOneParam
#define SFI_SETPROCESSDEFAULTLAYOUT                 70 // NtUserCallOneParam
#define SFI_SETWATERMARKSTRINGS                     71 // NtUserCallOneParam
#define SFI_SHOWSTARTGLASS                          72 // NtUserCallOneParam
#define SFI_SWAPMOUSEBUTTON                         73 // NtUserCallOneParam
#define SFI_WOWMODULEUNLOAD                         74 // NtUserCallOneParam
#define SFI_DWMLOCKSCREENUPDATES                    75 // NtUserCallOneParam
#define SFI_ENABLESESSIONFORMMCSS                   76 // NtUserCallOneParam
#define SFI_SETWAITFORQUEUEATTACH                   77 // NtUserCallOneParam
#define SFI_THREADMESSAGEQUEUEATTACHED              78 // NtUserCallOneParam
#define SFI_ENSUREDPIDEPSYSMETCACHEFORPLATEAU       80 // NtUserCallOneParam
#define SFI_FORCEENABLENUMPADTRANSLATION            81 // NtUserCallOneParam
#define SFI_SETTSFEVENTSTATE                        82 // NtUserCallOneParam
#define SFI_SETSHELLCHANGENOTIFYHWND                83 // NtUserCallOneParam
#define SFI_DEREGISTERSHELLHOOKWINDOW               84 // NtUserCallHwnd
#define SFI_DWP_GETENABLEDPOPUPOFFSET               85 // NtUserCallHwnd
#define SFI_GETMODERNAPPWINDOW                      86 // NtUserCallHwnd
#define SFI_GETWINDOWCONTEXTHELPID                  87 // NtUserCallHwnd
#define SFI_REGISTERSHELLHOOKWINDOW                 88 // NtUserCallHwnd
#define SFI_SETMSGBOX                               89 // NtUserCallHwnd
#define SFI_INITTHREADCOREMESSAGINGIOCP             90 // NtUserCallHwnd, NtUserCallHwndSafe
#define SFI_SCHEDULEDISPATCHNOTIFICATION            91 // NtUserCallHwnd, NtUserCallHwndSafe
#define SFI_SETPROGMANWINDOW                        92 // NtUserCallHwndOpt
#define SFI_SETTASKMANWINDOW                        93 // NtUserCallHwndOpt
#define SFI_GETCLASSICOCUR                          94 // NtUserCallHwndParam
#define SFI_CLEARWINDOWSTATE                        95 // NtUserCallHwndParam
#define SFI_KILLSYSTEMTIMER                         96 // NtUserCallHwndParam
#define SFI_NOTIFYOVERLAYWINDOW                     97 // NtUserCallHwndParam
#define SFI_SETDIALOGPOINTER                        99 // NtUserCallHwndParam
#define SFI_SETVISIBLE                              100 // NtUserCallHwndParam
#define SFI_SETWINDOWCONTEXTHELPID                  101 // NtUserCallHwndParam
#define SFI_SETWINDOWSTATE                          102 // NtUserCallHwndParam
#define SFI_REGISTERWINDOWARRANGEMENTCALLOUT        103 // NtUserCallHwndParam
#define SFI_ENABLEMODERNAPPWINDOWKEYBOARDINTERCEPT  104 // NtUserCallHwndParam
#define SFI_ARRANGEICONICWINDOWS                    105 // NtUserCallHwndLock
#define SFI_DRAWMENUBAR                             106 // NtUserCallHwndLock
#define SFI_CHECKIMESHOWSTATUSINTHREAD              107 // NtUserCallHwndLock, NtUserCallHwndLockSafe
#define SFI_GETSYSMENUOFFSET                        108 // NtUserCallHwndLock
#define SFI_REDRAWFRAME                             109 // NtUserCallHwndLock
#define SFI_REDRAWFRAMEANDHOOK                      110 // NtUserCallHwndLock
#define SFI_SETDIALOGSYSTEMMENU                     111 // NtUserCallHwndLock
#define SFI_SETFOREGROUNDWINDOW                     112 // NtUserCallHwndLock
#define SFI_SETSYSMENU                              113 // NtUserCallHwndLock
#define SFI_UPDATECLIENTRECT                        114 // NtUserCallHwndLock
#define SFI_UPDATEWINDOW                            115 // NtUserCallHwndLock
#define SFI_SETCANCELROTATIONDELAYHINTWINDOW        116 // NtUserCallHwndLock
#define SFI_GETWINDOWTRACKINFOASYNC                 117 // NtUserCallHwndLock
#define SFI_BROADCASTIMESHOWSTATUSCHANGE            118 // NtUserCallHwndParamLock
#define SFI_SETMODERNAPPWINDOW                      119 // NtUserCallHwndParamLock
#define SFI_REDRAWTITLE                             120 // NtUserCallHwndParamLock
#define SFI_SHOWOWNEDPOPUPS                         121 // NtUserCallHwndParamLock
#define SFI_SWITCHTOTHISWINDOW                      122 // NtUserCallHwndParamLock
#define SFI_UPDATEWINDOWS                           123 // NtUserCallHwndParamLock
#define SFI_VALIDATERGN                             124 // NtUserCallHwndParamLock
#define SFI_ENABLEWINDOW                            125 // NtUserCallHwndParamLock, NtUserCallHwndParamLockSafe
#define SFI_CHANGEWINDOWMESSAGEFILTER               126 // NtUserCallTwoParam
#define SFI_GETCURSORPOS                            127 // NtUserCallTwoParam
#define SFI_INITANSIOEM                             128 // NtUserCallTwoParam
#define SFI_NLSKBDSENDIMENOTIFICATION               129 // NtUserCallTwoParam
#define SFI_REGISTERGHOSTWINDOW                     130 // NtUserCallTwoParam
#define SFI_REGISTERLOGONPROCESS                    131 // NtUserCallTwoParam
#define SFI_REGISTERSIBLINGFROSTWINDOW              132 // NtUserCallTwoParam
#define SFI_REGISTERUSERHUNGAPPHANDLERS             133 // NtUserCallTwoParam
#define SFI_REMOTESHADOWCLEANUP                     134 // NtUserCallTwoParam
#define SFI_REMOTESHADOWSTART                       135 // NtUserCallTwoParam
#define SFI_SETCARETPOS                             136 // NtUserCallTwoParam
#define SFI_SETTHREADQUEUEMERGESETTING              137 // NtUserCallTwoParam
#define SFI_UNHOOKWINDOWSHOOK                       138 // NtUserCallTwoParam
#define SFI_ENABLESHELLWINDOWMANAGEMENTBEHAVIOR     139 // NtUserCallTwoParam
#define SFI_CITSETINFO                              140 // NtUserCallTwoParam
#define SFI_SCALESYSTEMMETRICFORDPIWITHOUTCACHE     141 // NtUserCallTwoParam

// N.B.
// Windows 10 uses NtUserCall* dispatch routines to invoke ~140 functions
// by index (note that our index table is valid only since 20H1).
// Windows 11 uses the newly introduced syscalls for each function instead.

// private // before WIN11
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserCallNoParam(
    _In_ ULONG xpfnProc
    );

// private // before WIN11
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserCallOneParam(
    _In_ ULONG_PTR Param,
    _In_ ULONG xpfnProc
    );

// private // before WIN11
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserCallHwnd(
    _In_ HWND hwnd,
    _In_ ULONG xpfnProc
    );

// private // before WIN11
#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS5)
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserCallHwndSafe(
    _In_ HWND hwnd,
    _In_ ULONG xpfnProc
    );
#endif

// private // before WIN11
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserCallHwndOpt(
    _In_opt_ HWND hwnd,
    _In_ ULONG xpfnProc
    );

// private // before WIN11
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserCallHwndParam(
    _In_ HWND hwnd,
    _In_ ULONG_PTR Param,
    _In_ ULONG xpfnProc
    );

// private // before WIN11
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserCallHwndLock(
    _In_ HWND hwnd,
    _In_ ULONG xpfnProc
    );

// private // before WIN11
#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS5)
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserCallHwndLockSafe(
    _In_ HWND hwnd,
    _In_ ULONG xpfnProc
    );
#endif

// private // before WIN11
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserCallHwndParamLock(
    _In_ HWND hwnd,
    _In_ ULONG_PTR Param,
    _In_ ULONG xpfnProc
    );

// private // before WIN11
#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS5)
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserCallHwndParamLockSafe(
    _In_ HWND hwnd,
    _In_ ULONG_PTR Param,
    _In_ ULONG xpfnProc
    );
#endif

// private // before WIN11
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserCallTwoParam(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2,
    _In_ ULONG xpfnProc
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

// private // NtUserCallNoParam(SFI_CREATEMENU) before WIN11
_Success_(return != NULL)
_Must_inspect_result_
NTSYSCALLAPI
HMENU
NTAPI
NtUserCreateMenu(
    VOID
    );

// private // NtUserCallNoParam(SFI_CREATEPOPUPMENU) before WIN11
_Success_(return != NULL)
_Must_inspect_result_
NTSYSCALLAPI
HMENU
NTAPI
NtUserCreatePopupMenu(
    VOID
    );

// private // NtUserCallNoParam(SFI_ALLOWFOREGROUNDACTIVATION) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserAllowForegroundActivation(
    VOID
    );

// rev // NtUserCallNoParam(SFI_CANCELQUEUEEVENTCOMPLETIONPACKET) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCancelQueueEventCompletionPacket(
    VOID
    );

// private // NtUserCallNoParam(SFI_CLEARWAKEMASK) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserClearWakeMask(
    VOID
    );

// private // NtUserCallNoParam(SFI_CREATESYSTEMTHREADS) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCreateSystemThreads(
    VOID
    );

// private // NtUserCallNoParam(SFI_DESTROYCARET) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDestroyCaret(
    VOID
    );

// private // NtUserCallNoParam(SFI_DISABLEPROCESSWINDOWSGHOSTING) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDisableProcessWindowsGhosting(
    VOID
    );

// rev // NtUserCallNoParam(SFI_DRAINTHREADCOREMESSAGINGCOMPLETIONS) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDrainThreadCoreMessagingCompletions(
    VOID
    );

// private // NtUserCallNoParam(SFI_GETDEVICECHANGEINFO) before WIN11
_Success_(return != 0)
_Must_inspect_result_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetDeviceChangeInfo(
    VOID
    );

// private // NtUserCallNoParam(SFI_GETIMESHOWSTATUS) before WIN11
_Success_(return != 0)
_Must_inspect_result_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetIMEShowStatus(
    VOID
    );

// private // NtUserCallNoParam(SFI_GETINPUTDESKTOP) before WIN11
_Success_(return != NULL)
_Must_inspect_result_
NTSYSCALLAPI
HDESK
NTAPI
NtUserGetInputDesktop(
    VOID
    );

// private // NtUserCallNoParam(SFI_GETMESSAGEPOS) before WIN11
_Must_inspect_result_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetMessagePos(
    VOID
    );

// rev // NtUserCallNoParam(SFI_GETQUEUEIOCP) before WIN11
_Must_inspect_result_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserGetQueueIocp(
    VOID
    );

// private // NtUserCallNoParam(SFI_GETUNPREDICTEDMESSAGEPOS) before WIN11
_Must_inspect_result_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetUnpredictedMessagePos(
    VOID
    );

// private // NtUserCallNoParam(SFI_HANDLESYSTEMTHREADCREATIONFAILURE) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserHandleSystemThreadCreationFailure(
    VOID
    );

// private // NtUserCallNoParam(SFI_HIDECURSORNOCAPTURE) before WIN11
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserHideCursorNoCapture(
    VOID
    );

// private // NtUserCallNoParam(SFI_ISQUEUEATTACHED) before WIN11
_Must_inspect_result_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserIsQueueAttached(
    VOID
    );

// private // NtUserCallNoParam(SFI_LOADCURSORSANDICONS) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserLoadCursorsAndIcons(
    VOID
    );

// private // NtUserCallNoParam(SFI_LOADUSERAPIHOOK) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserLoadUserApiHook(
    VOID
    );

// private // NtUserCallNoParam(SFI_PREPAREFORLOGOFF) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserPrepareForLogoff(
    VOID
    );

// rev // NtUserCallNoParam(SFI_REASSOCIATEQUEUEEVENTCOMPLETIONPACKET) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserReassociateQueueEventCompletionPacket(
    VOID
    );

// private // NtUserCallNoParam(SFI_RELEASECAPTURE) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserReleaseCapture(
    VOID
    );

// rev // NtUserCallNoParam(SFI_REMOVEQUEUECOMPLETION) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRemoveQueueCompletion(
    VOID
    );

// private // NtUserCallNoParam(SFI_RESETDBLCLK) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserResetDblClk(
    VOID
    );

// private // NtUserCallNoParam(SFI_ZAPACTIVEANDFOCUS) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserZapActiveAndFocus(
    VOID
    );

// private // NtUserCallNoParam(SFI_REMOTECONSOLESHADOWSTOP) before WIN11
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteConsoleShadowStop(
    VOID
    );

// private // NtUserCallNoParam(SFI_REMOTEDISCONNECT) before WIN11
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteDisconnect(
    VOID
    );

// private // NtUserCallNoParam(SFI_REMOTESHADOWSETUP) before WIN11
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteShadowSetup(
    VOID
    );

// private // NtUserCallNoParam(SFI_REMOTESHADOWSTOP) before WIN11
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteShadowStop(
    VOID
    );

// private // NtUserCallNoParam(SFI_REMOTEPASSTHRUENABLE) before WIN11
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemotePassthruEnable(
    VOID
    );

// private // NtUserCallNoParam(SFI_REMOTEPASSTHRUDISABLE) before WIN11
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemotePassthruDisable(
    VOID
    );

// private
#define CTX_W32_CONNECT_STATE_CONSOLE           0
#define CTX_W32_CONNECT_STATE_IDLE              1
#define CTX_W32_CONNECT_STATE_EXIT_IN_PROGRESS  2
#define CTX_W32_CONNECT_STATE_CONNECTED         3
#define CTX_W32_CONNECT_STATE_DISCONNECTED      4

// private // NtUserCallNoParam(SFI_REMOTECONNECTSTATE) before WIN11
_Must_inspect_result_
NTSYSCALLAPI
ULONG
NTAPI
NtUserRemoteConnectState(
    VOID
    );

// private // NtUserCallNoParam(SFI_UPDATEPERUSERIMMENABLING) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUpdatePerUserImmEnabling(
    VOID
    );

// private // NtUserCallNoParam(SFI_USERPOWERCALLOUTWORKER) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUserPowerCalloutWorker(
    VOID
    );

// private // NtUserCallNoParam(SFI_WAKERITFORSHUTDOWN) before WIN11
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserWakeRITForShutdown(
    VOID
    );

// private // NtUserCallNoParam(SFI_DOINITMESSAGEPUMPHOOK) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDoInitMessagePumpHook(
    VOID
    );

// private // NtUserCallNoParam(SFI_DOUNINITMESSAGEPUMPHOOK) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDoUninitMessagePumpHook(
    VOID
    );

// rev // NtUserCallNoParam(SFI_ENABLEMOUSEINPOINTERFORTHREAD) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableMouseInPointerForThread(
    VOID
    );

// private // NtUserCallNoParam(SFI_DEFERREDDESKTOPROTATION) before WIN11
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserDeferredDesktopRotation(
    VOID
    );

// private // NtUserCallNoParam(SFI_ENABLEPERMONITORMENUSCALING) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnablePerMonitorMenuScaling(
    VOID
    );

// private // NtUserCallOneParam(SFI_BEGINDEFERWINDOWPOS) before WIN11
_Success_(return != NULL)
_Must_inspect_result_
NTSYSCALLAPI
HDWP
NTAPI
NtUserBeginDeferWindowPos(
    _In_ ULONG NumWindowsHint
    );

// private // NtUserCallOneParam(SFI_GETSENDMESSAGERECEIVER) before WIN11
_Success_(return != NULL)
_Must_inspect_result_
NTSYSCALLAPI
HWND
NTAPI
NtUserGetSendMessageReceiver(
    _In_ ULONG ThreadIdSender
    );

// private // NtUserCallOneParam(SFI_ALLOWSETFOREGROUNDWINDOW) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserAllowSetForegroundWindow(
    _In_ ULONG ProcessId
    );

// private // NtUserCallOneParam(SFI_CSDDEUNINITIALIZE) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCsDdeUninitialize(
    _In_ HANDLE hInst
    );

// private // NtUserCallOneParam(SFI_ENUMCLIPBOARDFORMATS) before WIN11
_Success_(return != 0)
_Must_inspect_result_
NTSYSCALLAPI
ULONG
NTAPI
NtUserEnumClipboardFormats(
    _In_ ULONG Format
    );

// private // NtUserCallOneParam(SFI_GETINPUTEVENT) before WIN11
_Success_(return != NULL)
_Must_inspect_result_
NTSYSCALLAPI
HANDLE
NTAPI
NtUserGetInputEvent(
    _In_ ULONG WakeMask // QS_* WinUser.h
    );

// rev (MSDN)
#define KEYBOARD_TYPE           0
#define KEYBOARD_SUBTYPE        1
#define KEYBOARD_FUNCTION_KEY   2

// private // NtUserCallOneParam(SFI_GETKEYBOARDTYPE) before WIN11
_Must_inspect_result_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetKeyboardType(
    _In_ ULONG TypeFlag // KEYBOARD_*
    );

// private // NtUserCallOneParam(SFI_GETPROCESSDEFAULTLAYOUT) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetProcessDefaultLayout(
    _Out_ PULONG DefaultLayout
    );

// private
#define WPROTOCOLNAME_LENGTH    10
#define WAUDIONAME_LENGTH       10

// private
typedef struct tagWSINFO
{
    WCHAR ProtocolName[WPROTOCOLNAME_LENGTH];
    WCHAR AudioDriverName[WAUDIONAME_LENGTH];
} WSINFO, *PWSINFO;

// private // NtUserCallOneParam(SFI_GETWINSTATIONINFO) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetWinStationInfo(
    _Out_ PWSINFO WsInfo
    );

// private // NtUserCallOneParam(SFI_LOCKSETFOREGROUNDWINDOW) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserLockSetForegroundWindow(
    _In_ ULONG LockCode // LSFW_* WinUser.h
    );

// private // NtUserCallOneParam(SFI_LW_LOADFONTS) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserLW_LoadFonts(
    _In_ LOGICAL Remote
    );

// private // NtUserCallOneParam(SFI_MAPDESKTOPOBJECT) before WIN11
_Success_(return != NULL)
_Must_inspect_result_
_Ret_maybenull_
NTSYSCALLAPI
PVOID
NTAPI
NtUserMapDesktopObject(
    _In_ HANDLE h
    );

// private // NtUserCallOneParam(SFI_MESSAGEBEEP) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserMessageBeep(
    _In_ ULONG Type // MB_* (MB_ICONMASK) WinUser.h
    );

// private
#define USER_SOUND_DEFAULT              0
#define USER_SOUND_SYSTEMHAND           1
#define USER_SOUND_SYSTEMQUESTION       2
#define USER_SOUND_SYSTEMEXCLAMATION    3
#define USER_SOUND_SYSTEMASTERISK       4
#define USER_SOUND_MENUPOPUP            5
#define USER_SOUND_MENUCOMMAND          6
#define USER_SOUND_OPEN                 7
#define USER_SOUND_CLOSE                8
#define USER_SOUND_RESTOREUP            9
#define USER_SOUND_RESTOREDOWN          10
#define USER_SOUND_MINIMIZE             11
#define USER_SOUND_MAXIMIZE             12
#define USER_SOUND_SNAPSHOT             13

// private // NtUserCallOneParam(SFI_PLAYEVENTSOUND) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserPlayEventSound(
    _In_ ULONG idSound // USER_SOUND_*
    );

// private // NtUserCallOneParam(SFI_POSTQUITMESSAGE) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserPostQuitMessage(
    _In_ LONG ExitCode
    );

// private // NtUserCallOneParam(SFI_REALIZEPALETTE) before WIN11
NTSYSCALLAPI
ULONG
NTAPI
NtUserRealizePalette(
    _In_ HDC hdc
    );

// private
#define LPK_TABBED_TEXT_OUT 0
#define LPK_PSM_TEXT_OUT    1
#define LPK_DRAW_TEXT_EX    2
#define LPK_EDIT_CONTROL    3

// rev
#define LPK_FLAG_TABBED_TEXT_OUT (1 << LPK_TABBED_TEXT_OUT)
#define LPK_FLAG_PSM_TEXT_OUT    (1 << LPK_PSM_TEXT_OUT)
#define LPK_FLAG_DRAW_TEXT_EX    (1 << LPK_DRAW_TEXT_EX)
#define LPK_FLAG_EDIT_CONTROL    (1 << LPK_EDIT_CONTROL)

// private // NtUserCallOneParam(SFI_REGISTERLPK) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterLPK(
    _In_ ULONG LpkEntryPoints // LPK_FLAG_*
    );

// private
#define RST_DONTATTACHQUEUE     0x00000001
#define RST_DONTJOURNALATTACH   0x00000002

// private // NtUserCallOneParam(SFI_REGISTERSYSTEMTHREAD) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterSystemThread(
    _In_ ULONG Flags // RST_*
    );

// private // NtUserCallOneParam(SFI_REMOTERECONNECT) before WIN11
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteReconnect(
    _In_ struct _DOCONNECTDATA* DoConnectData
    );

// private // NtUserCallOneParam(SFI_REMOTETHINWIRESTATS) before WIN11
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteThinwireStats(
    _Out_ struct CACHE_STATISTICS* Stats
    );

// private // NtUserCallOneParam(SFI_REMOTENOTIFY) before WIN11
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteNotify(
    _In_ struct _DONOTIFYDATA* DoNotifyData
    );

// private // NtUserCallOneParam(SFI_REPLYMESSAGE) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserReplyMessage(
    _In_ LRESULT Result
    );

// private // NtUserCallOneParam(SFI_SETCARETBLINKTIME) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetCaretBlinkTime(
    _In_ ULONG Milliseconds
    );

// private // NtUserCallOneParam(SFI_SETDOUBLECLICKTIME) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetDoubleClickTime(
    _In_ ULONG Milliseconds
    );

// private // NtUserCallOneParam(SFI_SETMESSAGEEXTRAINFO) before WIN11
NTSYSCALLAPI
LPARAM
NTAPI
NtUserSetMessageExtraInfo(
    _In_ LPARAM Param
    );

// private // NtUserCallOneParam(SFI_SETPROCESSDEFAULTLAYOUT) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetProcessDefaultLayout(
    _In_ ULONG DefaultLayout // LAYOUT_* wingdi.h
    );

// private // NtUserCallOneParam(SFI_SETWATERMARKSTRINGS) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWatermarkStrings(
    _In_ PCUNICODE_STRING StringTable
    );

// private // NtUserCallOneParam(SFI_SHOWSTARTGLASS) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserShowStartGlass(
    _In_ ULONG Timeout
    );

// private // NtUserCallOneParam(SFI_SWAPMOUSEBUTTON) before WIN11
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSwapMouseButton(
    _In_ LOGICAL SwapButtons
    );

// private // NtUserCallOneParam(SFI_WOWMODULEUNLOAD) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserWOWModuleUnload(
    _In_ HANDLE Module
    );

// private // NtUserCallOneParam(SFI_DWMLOCKSCREENUPDATES) before WIN11
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserDwmLockScreenUpdates(
    _In_ LOGICAL LockUpdates
    );

// private // NtUserCallOneParam(SFI_ENABLESESSIONFORMMCSS) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserEnableSessionForMMCSS(
    _In_ LOGICAL Enable
    );

// private // NtUserCallOneParam(SFI_SETWAITFORQUEUEATTACH) before WIN11
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWaitForQueueAttach(
    _In_ LOGICAL Waiting
    );

// private // NtUserCallOneParam(SFI_THREADMESSAGEQUEUEATTACHED) before WIN11
_Must_inspect_result_
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserThreadMessageQueueAttached(
    _In_opt_ ULONG ThreadId
    );

// private // NtUserCallOneParam(SFI_ENSUREDPIDEPSYSMETCACHEFORPLATEAU) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnsureDpiDepSysMetCacheForPlateau(
    _In_ ULONG Dpi
    );

// private // NtUserCallOneParam(SFI_FORCEENABLENUMPADTRANSLATION) before WIN11
NTSYSCALLAPI
UINT_PTR
NTAPI
NtUserForceEnableNumpadTranslation(
    _In_ LOGICAL ForceNumlockTranslation
    );

// rev // NtUserCallOneParam(SFI_SETTSFEVENTSTATE) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetTSFEventState(
    _In_ ULONG StateFlags
    );

// rev // NtUserCallOneParam(SFI_SETSHELLCHANGENOTIFYHWND) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetShellChangeNotifyHWND(
    _In_ HWND hwnd
    );

// private // NtUserCallHwnd(SFI_DEREGISTERSHELLHOOKWINDOW) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDeregisterShellHookWindow(
    _In_ HWND hwnd
    );

// rev // NtUserCallHwnd(SFI_DWP_GETENABLEDPOPUPOFFSET) before WIN11
_Must_inspect_result_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserDWP_GetEnabledPopupOffset(
    _In_ HWND hwnd
    );

// private // NtUserCallHwnd(SFI_GETMODERNAPPWINDOW) before WIN11
_Success_(return != NULL)
_Must_inspect_result_
NTSYSCALLAPI
HWND
NTAPI
NtUserGetModernAppWindow(
    _In_ HWND ShellFrame
    );

// private // NtUserCallHwnd(SFI_GETWINDOWCONTEXTHELPID) before WIN11
_Must_inspect_result_
NTSYSCALLAPI
ULONG
NTAPI
NtUserGetWindowContextHelpId(
    _In_ HWND hwnd
    );

// private // NtUserCallHwnd(SFI_REGISTERSHELLHOOKWINDOW) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterShellHookWindow(
    _In_ HWND hwnd
    );

// private // NtUserCallHwnd(SFI_SETMSGBOX) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetMsgBox(
    _In_ HWND hwnd
    );

// rev // NtUserCallHwnd[Safe](SFI_INITTHREADCOREMESSAGINGIOCP) before WIN11
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserInitThreadCoreMessagingIocp(
    _In_ HWND hwnd
    );

// private // NtUserCallHwnd[Safe](SFI_SCHEDULEDISPATCHNOTIFICATION) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserScheduleDispatchNotification(
    _In_ HWND hwnd
    );

// private // NtUserCallHwndOpt(SFI_SETPROGMANWINDOW) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetProgmanWindow(
    _In_opt_ HWND hwnd
    );

// private // NtUserCallHwndOpt(SFI_SETTASKMANWINDOW) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetTaskmanWindow(
    _In_opt_ HWND hwnd
    );

// private // NtUserCallHwndParam(SFI_GETCLASSICOCUR) before WIN11
_Success_(return != NULL)
_Must_inspect_result_
NTSYSCALLAPI
HCURSOR
NTAPI
NtUserGetClassIcoCur(
    _In_ HWND hwnd,
    _In_ ULONG Index
    );

// private
#define WFWIN40COMPAT           0x0502
#define WFNOANIMATE             0x0710
#define WEFWINDOWEDGE           0x0901
#define WEFCLIENTEDGE           0x0902
#define WEFSTATICEDGE           0x0A02
#define BFRIGHTBUTTON           0x0C20
#define BFRIGHT                 0x0D02
#define BFBOTTOM                0x0D08
#define DFLOCALEDIT             0x0C20
#define CBFOWNERDRAWVAR         0x0C20
#define CBFHASSTRINGS           0x0D02
#define CBFDISABLENOSCROLL      0x0D08
#define EFPASSWORD              0x0C20
#define EFCOMBOBOX              0x0D02
#define EFREADONLY              0x0D08
#define SFWIDELINESPACING       0x0C20
#define SFCENTERIMAGE           0x0D02
#define SFREALSIZEIMAGE         0x0D08
#define WFMAXBOX                0x0E01
#define WFTABSTOP               0x0E01
#define WFSYSMENU               0x0E08
#define WFHSCROLL               0x0E10
#define WFVSCROLL               0x0E20
#define WFBORDER                0x0E80
#define WFCLIPCHILDREN          0x0F02

// private // NtUserCallHwndParam(SFI_CLEARWINDOWSTATE) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserClearWindowState(
    _In_ HWND hwnd,
    _In_ ULONG Flag // WF*, WEF*, BF*, DF*, CBF*, EF*, SF*
    );

// private // NtUserCallHwndParam(SFI_SETWINDOWSTATE) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWindowState(
    _In_ HWND hwnd,
    _In_ ULONG Flag // WF*, WEF*, BF*, DF*, CBF*, EF*, SF*
    );

// private // NtUserCallHwndParam(SFI_KILLSYSTEMTIMER) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserKillSystemTimer(
    _In_ HWND hwnd,
    _In_ UINT_PTR IDEvent
    );

// private // NtUserCallHwndParam(SFI_NOTIFYOVERLAYWINDOW) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserNotifyOverlayWindow(
    _In_ HWND hwnd,
    _In_ LOGICAL Enable
    );

// private // NtUserCallHwndParam(SFI_SETDIALOGPOINTER) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetDialogPointer(
    _In_ HWND hwnd,
    _In_ ULONG_PTR Ptr
    );

// private
#define SV_UNSET            0x0000
#define SV_SET              0x0001
#define SV_CLRFTRUEVIS      0x0002
#define SV_SKIPCOMPOSE      0x0004 // rev
#define SV_CLRFULLSCREEN    0x0008 // rev
#define SV_CLRGHOSTVIS      0x0010 // rev

// private // NtUserCallHwndParam(SFI_SETVISIBLE) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetVisible(
    _In_ HWND hwnd,
    _In_ ULONG Flags // SV_*
    );

// private // NtUserCallHwndParam(SFI_SETWINDOWCONTEXTHELPID) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetWindowContextHelpId(
    _In_ HWND hwnd,
    _In_ ULONG ContextId
    );

// private // NtUserCallHwndParam(SFI_REGISTERWINDOWARRANGEMENTCALLOUT) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterWindowArrangementCallout(
    _In_ HWND hwnd,
    _In_ LOGICAL Register
    );

// private // NtUserCallHwndParam(SFI_ENABLEMODERNAPPWINDOWKEYBOARDINTERCEPT) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableModernAppWindowKeyboardIntercept(
    _In_ HWND hwnd,
    _In_ LOGICAL Enable
    );

// private // NtUserCallHwndLock(SFI_ARRANGEICONICWINDOWS) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
ULONG
NTAPI
NtUserArrangeIconicWindows(
    _In_ HWND hwnd
    );

// private // NtUserCallHwndLock(SFI_DRAWMENUBAR) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserDrawMenuBar(
    _In_ HWND hwnd
    );

// private // NtUserCallHwndLock[Safe](SFI_CHECKIMESHOWSTATUSINTHREAD) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserCheckImeShowStatusInThread(
    _In_ HWND Ime
    );

// rev // NtUserCallHwndLock(SFI_GETSYSMENUOFFSET) before WIN11
_Success_(return != 0)
_Must_inspect_result_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserGetSysMenuOffset(
    _In_ HWND hwnd
    );

// private // NtUserCallHwndLock(SFI_REDRAWFRAME) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRedrawFrame(
    _In_ HWND hwnd
    );

// private // NtUserCallHwndLock(SFI_REDRAWFRAMEANDHOOK) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRedrawFrameAndHook(
    _In_ HWND hwnd
    );

// private // NtUserCallHwndLock(SFI_SETDIALOGSYSTEMMENU) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetDialogSystemMenu(
    _In_ HWND hwnd
    );

// private // NtUserCallHwndLock(SFI_SETFOREGROUNDWINDOW) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetForegroundWindow(
    _In_ HWND hwnd
    );

// private // NtUserCallHwndLock(SFI_SETSYSMENU) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetSysMenu(
    _In_ HWND hwnd
    );

// private // NtUserCallHwndLock(SFI_UPDATECLIENTRECT) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUpdateClientRect(
    _In_ HWND hwnd
    );

// private // NtUserCallHwndLock(SFI_UPDATEWINDOW) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUpdateWindow(
    _In_ HWND hwnd
    );

// private // NtUserCallHwndLock(SFI_SETCANCELROTATIONDELAYHINTWINDOW) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetCancelRotationDelayHintWindow(
    _In_ HWND hwnd
    );

// private // NtUserCallHwndLock(SFI_GETWINDOWTRACKINFOASYNC) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserGetWindowTrackInfoAsync(
    _In_ HWND hwnd
    );

// private // NtUserCallHwndParamLock(SFI_BROADCASTIMESHOWSTATUSCHANGE) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserBroadcastImeShowStatusChange(
    _In_ HWND Ime,
    _In_ LOGICAL Show
    );

// private // NtUserCallHwndParamLock(SFI_SETMODERNAPPWINDOW) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetModernAppWindow(
    _In_ HWND ShellFrame,
    _In_ HWND App
    );

// private
// #define DC_ACTIVE    0x0001 // WinUser.h
// #define DC_SMALLCAP  0x0002 // WinUser.h
// #define DC_ICON      0x0004 // WinUser.h
// #define DC_TEXT      0x0008 // WinUser.h
// #define DC_INBUTTON  0x0010 // WinUser.h
// #define DC_GRADIENT  0x0020 // WinUser.h
#define DC_LAMEBUTTON   0x0400
#define DC_NOVISIBLE    0x0800
// #define DC_BUTTONS   0x1000 // WinUser.h
#define DC_NOSENDMSG    0x2000
#define DC_CENTER       0x4000
#define DC_FRAME        0x8000

// private // NtUserCallHwndParamLock(SFI_REDRAWTITLE) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRedrawTitle(
    _In_ HWND hwnd,
    _In_ ULONG Flags // DC_*
    );

// private // NtUserCallHwndParamLock(SFI_SHOWOWNEDPOPUPS) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserShowOwnedPopups(
    _In_ HWND Owner,
    _In_ LOGICAL Show
    );

// private // NtUserCallHwndParamLock(SFI_SWITCHTOTHISWINDOW) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSwitchToThisWindow(
    _In_ HWND hwnd,
    _In_ LOGICAL AltTab
    );

// private // NtUserCallHwndParamLock(SFI_UPDATEWINDOWS) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUpdateWindows(
    _In_ HWND hwnd,
    _In_ HRGN hrgn
    );

// private // NtUserCallHwndParamLock(SFI_VALIDATERGN) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserValidateRgn(
    _In_ HWND hwnd,
    _In_ HRGN hrgn
    );

// private // NtUserCallHwndParamLock[Safe](SFI_ENABLEWINDOW) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableWindow(
    _In_ HWND hwnd,
    _In_ LOGICAL Enable
    );

// private // NtUserCallTwoParam(SFI_CHANGEWINDOWMESSAGEFILTER) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserChangeWindowMessageFilter(
    _In_ ULONG Message,
    _In_ ULONG Flag // MSGFLT_* WinUser.h
    );

// rev
#define CURSOR_POS_TYPE_LOGICAL 1
#define CURSOR_POS_TYPE_SAVED   2

// private // NtUserCallTwoParam(SFI_GETCURSORPOS) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserGetCursorPos(
    _Out_ PPOINT Point,
    _In_ ULONG CursorPosType // CURSOR_POS_TYPE_*
    );

// private // NtUserCallTwoParam(SFI_INITANSIOEM) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserInitAnsiOem(
    _In_reads_(256) PCHAR OemToAnsi,
    _In_reads_(256) PCHAR AnsiToOem
    );

// private // NtUserCallTwoParam(SFI_NLSKBDSENDIMENOTIFICATION) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserNlsKbdSendIMENotification(
    _In_ ULONG ImeOpen,
    _In_ ULONG ImeConversion
    );

// private // NtUserCallTwoParam(SFI_REGISTERGHOSTWINDOW) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterGhostWindow(
    _In_ HWND Ghost,
    _In_ HWND Hung
    );

// private // NtUserCallTwoParam(SFI_REGISTERLOGONPROCESS) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterLogonProcess(
    _In_ ULONG ProcessId,
    _In_ PLUID LuidConnect
    );

// private // NtUserCallTwoParam(SFI_REGISTERSIBLINGFROSTWINDOW) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterSiblingFrostWindow(
    _In_ HWND hwndFrost,
    _In_ HWND hwnd
    );

// rev
typedef _Function_class_(FNW32ET)
VOID APIENTRY FNW32ET(VOID);
typedef FNW32ET* PFNW32ET;

// private // NtUserCallTwoParam(SFI_REGISTERUSERHUNGAPPHANDLERS) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserRegisterUserHungAppHandlers(
    _In_ PFNW32ET W32EndTask,
    _In_ HANDLE EventWowExec
    );

// private // NtUserCallTwoParam(SFI_REMOTESHADOWCLEANUP) before WIN11
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteShadowCleanup(
    _In_reads_bytes_(ThinwireDataLength) PVOID ThinwireData,
    _In_ ULONG ThinwireDataLength
    );

// private // NtUserCallTwoParam(SFI_REMOTESHADOWSTART) before WIN11
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserRemoteShadowStart(
    _In_reads_bytes_(ThinwireDataLength) PVOID ThinwireData,
    _In_ ULONG ThinwireDataLength
    );

// private // NtUserCallTwoParam(SFI_SETCARETPOS) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetCaretPos(
    _In_ LONG x,
    _In_ LONG y
    );

// private // NtUserCallTwoParam(SFI_SETTHREADQUEUEMERGESETTING) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserSetThreadQueueMergeSetting(
    _In_ ULONG ThreadId,
    _In_ ULONG Flags
    );

// private // NtUserCallTwoParam(SFI_UNHOOKWINDOWSHOOK) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserUnhookWindowsHook(
    _In_ LONG FilterType,
    _In_ HOOKPROC FilterProc
    );

// private // NtUserCallTwoParam(SFI_ENABLESHELLWINDOWMANAGEMENTBEHAVIOR) before WIN11
_Success_(return != 0)
NTSYSCALLAPI
LOGICAL
NTAPI
NtUserEnableShellWindowManagementBehavior(
    _In_ ULONG_PTR Param1,
    _In_ ULONG_PTR Param2
    );

// private // NtUserCallTwoParam(SFI_CITSETINFO) before WIN11
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUserCitSetInfo(
    _In_ ULONG_PTR InfoFlags,
    _In_ ULONG_PTR Info
    );

// private // NtUserCallTwoParam(SFI_SCALESYSTEMMETRICFORDPIWITHOUTCACHE) before WIN11
_Must_inspect_result_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtUserScaleSystemMetricForDPIWithoutCache(
    _In_ ULONG Metric,
    _In_ ULONG ToDpi
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_11

typedef _Function_class_(FN_DISPATCH)
NTSTATUS NTAPI FN_DISPATCH(
    _In_opt_ PVOID Context
    );
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

#endif // _NTUSER_H
