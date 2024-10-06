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

#endif
