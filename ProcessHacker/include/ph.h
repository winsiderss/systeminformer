#ifndef PH_H
#define PH_H

#include <phbase.h>
#include <stdarg.h>

// process

#ifndef PROCESS_PRIVATE
extern PPH_OBJECT_TYPE PhProcessItemType;
#endif

typedef struct _PH_PROCESS_ITEM
{
    HANDLE ProcessId;
    HANDLE ParentProcessId;
    PPH_STRING ProcessName;
    ULONG SessionId;

    HICON SmallIcon;
    HICON LargeIcon;

    PPH_STRING FileName;
    PPH_STRING CommandLine;

    LARGE_INTEGER CreateTime;

    PPH_STRING UserName;
    ULONG IntegrityLevel;
    PPH_STRING IntegrityString;

    ULONG HasParent : 1;
    ULONG IsBeingDebugged : 1;
    ULONG IsDotNet : 1;
    ULONG IsElevated : 1;
    ULONG IsInJob : 1;
    ULONG IsInSignificantJob : 1;
    ULONG IsPacked : 1;
    ULONG IsPosix : 1;
    ULONG IsWow64 : 1;

    WCHAR ProcessIdString[PH_INT_STR_LEN_1];
    WCHAR ParentProcessIdString[PH_INT_STR_LEN_1];
    WCHAR SessionIdString[PH_INT_STR_LEN_1];

    FLOAT CpuUsage; // from 0 to 1
} PH_PROCESS_ITEM, *PPH_PROCESS_ITEM;

BOOLEAN PhInitializeProcessItem();

PPH_PROCESS_ITEM PhCreateProcessItem(
    __in HANDLE ProcessId
    );

#define PH_FIRST_PROCESS(Processes) ((PSYSTEM_PROCESS_INFORMATION)(Processes))
#define PH_NEXT_PROCESS(Process) ( \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NextEntryOffset ? \
    (PSYSTEM_PROCESS_INFORMATION)((PCHAR)(Process) + \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NextEntryOffset) : \
    NULL \
    )

NTSTATUS PhEnumProcesses(
    __out PPVOID Processes
    );

// support

#define PH_MAX_MESSAGE_SIZE 400

INT PhShowMessage(
    __in HWND hWnd,
    __in ULONG Type,
    __in PWSTR Format,
    ...
    );

INT PhShowMessage_V(
    __in HWND hWnd,
    __in ULONG Type,
    __in PWSTR Format,
    __in va_list ArgPtr
    );

#define PhShowError(hWnd, Format, ...) PhShowMessage(hWnd, MB_OK | MB_ICONERROR, Format, __VA_ARGS__) 

#endif
