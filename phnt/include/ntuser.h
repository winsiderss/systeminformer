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

NTUSERSYSAPI
ULONG_PTR
NTAPI
NtUserQueryWindow(
    _In_ HWND WindowHandle,
    _In_ WINDOWINFOCLASS WindowInfo
    );

typedef enum _CONSOLEINFOCLASS
{
    ConsoleSetVDMCursorBounds = 0,
    ConsoleNotifyConsoleApplication = 1,
    ConsoleFullscreenSwitch = 2,
    ConsoleSetCaretInfo = 3,
    ConsoleSetReserveKeys = 4,
    ConsoleSetForeground = 5,
    ConsoleSetWindowOwner = 6, // CONSOLESETWINDOWOWNER
    ConsoleEndTask = 7,
} CONSOLEINFOCLASS, *PCONSOLEINFOCLASS

typedef struct _CONSOLESETWINDOWOWNER
{
    HWND WindowHandle;
    ULONG ProcessId;
    ULONG ThreadId;
} CONSOLESETWINDOWOWNER, *PCONSOLESETWINDOWOWNER;

NTUSERSYSAPI
NTSTATUS
NTAPI
NtUserConsoleControl(
    _In_ CONSOLECONTROL ConsoleInformationClass,
    _Inout_updates_bytes_(ConsoleInformationLength) PVOID ConsoleInformation,
    _In_ ULONG ConsoleInformationLength
    );

#endif
