/*
 * Debugger support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTDBG_H
#define _NTDBG_H

//
// Debugging
//

/**
 * Causes a user-mode breakpoint to occur.
 */
NTSYSAPI
VOID
NTAPI
DbgUserBreakPoint(
    VOID
    );

/**
 * Causes a breakpoint to occur.
 */
NTSYSAPI
VOID
NTAPI
DbgBreakPoint(
    VOID
    );

/**
 * Causes a breakpoint to occur with a specific status.
 *
 * \param Status The status code to be associated with the breakpoint.
 */
NTSYSAPI
VOID
NTAPI
DbgBreakPointWithStatus(
    _In_ ULONG Status
    );

#define DBG_STATUS_CONTROL_C 1
#define DBG_STATUS_SYSRQ 2
#define DBG_STATUS_BUGCHECK_FIRST 3
#define DBG_STATUS_BUGCHECK_SECOND 4
#define DBG_STATUS_FATAL 5
#define DBG_STATUS_DEBUG_CONTROL 6
#define DBG_STATUS_WORKER 7

/**
 * Sends a message to the kernel debugger.
 *
 * \param Format A pointer to a printf-style format string.
 * \param ... Arguments for the format string.
 * \return ULONG The number of characters printed.
 * \remarks Debugger output/prompting follows debug filter and debugger availability, and may use
 * breakpoint/exception-based communication. Return values are not a reliable statement that a human received
 * text. Avoid sending secrets or invoking an interactive prompt in unattended code.
 */
NTSYSAPI
ULONG
STDAPIVCALLTYPE
DbgPrint(
    _In_z_ _Printf_format_string_ PCCH Format,
    ...
    );

/**
 * Sends a message to the kernel debugger with a component ID and level.
 *
 * \param ComponentId The ID of the component that is sending the message.
 * \param Level The importance level of the message.
 * \param Format A pointer to a printf-style format string.
 * \param ... Arguments for the format string.
 * \return ULONG The number of characters printed.
 * \remarks Debugger output/prompting follows debug filter and debugger availability, and may use
 * breakpoint/exception-based communication. Return values are not a reliable statement that a human received
 * text. Avoid sending secrets or invoking an interactive prompt in unattended code.
 */
NTSYSAPI
ULONG
STDAPIVCALLTYPE
DbgPrintEx(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_z_ _Printf_format_string_ PCCH Format,
    ...
    );

/**
 * Sends a message to the kernel debugger with a component ID and level (va_list version).
 *
 * \param ComponentId The ID of the component that is sending the message.
 * \param Level The importance level of the message.
 * \param Format A pointer to a printf-style format string.
 * \param arglist A list of arguments for the format string.
 * \return ULONG The number of characters printed.
 */
NTSYSAPI
ULONG
NTAPI
vDbgPrintEx(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_z_ PCCH Format,
    _In_ va_list arglist
    );

/**
 * Sends a message to the kernel debugger with a prefix, component ID, and level.
 *
 * \param Prefix A pointer to a string to be prefixed to the message.
 * \param ComponentId The ID of the component that is sending the message.
 * \param Level The importance level of the message.
 * \param Format A pointer to a printf-style format string.
 * \param arglist A list of arguments for the format string.
 * \return ULONG The number of characters printed.
 */
NTSYSAPI
ULONG
NTAPI
vDbgPrintExWithPrefix(
    _In_z_ PCCH Prefix,
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_z_ PCCH Format,
    _In_ va_list arglist
    );

/**
 * Sends a message to the kernel debugger and returns Control-C status.
 *
 * \param Format A pointer to a printf-style format string.
 * \param ... Arguments for the format string.
 * \return ULONG The number of characters printed.
 * \remarks Debugger output/prompting follows debug filter and debugger availability, and may use
 * breakpoint/exception-based communication. Return values are not a reliable statement that a human received
 * text. Avoid sending secrets or invoking an interactive prompt in unattended code.
 */
NTSYSAPI
ULONG
STDAPIVCALLTYPE
DbgPrintReturnControlC(
    _In_z_ _Printf_format_string_ PCCH Format,
    ...
    );

/**
 * Queries the debug filter state for a component.
 *
 * \param ComponentId The ID of the component.
 * \param Level The importance level.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
DbgQueryDebugFilterState(
    _In_ ULONG ComponentId,
    _In_ ULONG Level
    );

/**
 * Sets the debug filter state for a component.
 *
 * \param ComponentId The ID of the component.
 * \param Level The importance level.
 * \param State The new state for the filter.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
DbgSetDebugFilterState(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_ BOOLEAN State
    );

/**
 * Prompts the user for input.
 *
 * \param Prompt A pointer to the prompt string.
 * \param Response A pointer to the buffer that receives the user response.
 * \param Length The length of the response buffer, in bytes.
 * \return ULONG The number of characters in the response.
 */
NTSYSAPI
ULONG
NTAPI
DbgPrompt(
    _In_ PCCH Prompt,
    _Out_writes_bytes_(Length) PCH Response,
    _In_ ULONG Length
    );

//
// Definitions
//

/**
 * The DBGKM_EXCEPTION structure contains exception information for a debug event.
 */
typedef struct _DBGKM_EXCEPTION
{
    EXCEPTION_RECORD ExceptionRecord;
    ULONG FirstChance;
} DBGKM_EXCEPTION, *PDBGKM_EXCEPTION;

/**
 * The DBGKM_CREATE_THREAD structure contains information about a newly created thread.
 */
typedef struct _DBGKM_CREATE_THREAD
{
    ULONG SubSystemKey;
    PVOID StartAddress;
} DBGKM_CREATE_THREAD, *PDBGKM_CREATE_THREAD;

/**
 * The DBGKM_CREATE_PROCESS structure contains information about a newly created process.
 */
typedef struct _DBGKM_CREATE_PROCESS
{
    ULONG SubSystemKey;
    HANDLE FileHandle;
    PVOID BaseOfImage;
    ULONG DebugInfoFileOffset;
    ULONG DebugInfoSize;
    DBGKM_CREATE_THREAD InitialThread;
} DBGKM_CREATE_PROCESS, *PDBGKM_CREATE_PROCESS;

/**
 * The DBGKM_EXIT_THREAD structure contains the exit status of a thread.
 */
typedef struct _DBGKM_EXIT_THREAD
{
    NTSTATUS ExitStatus;
} DBGKM_EXIT_THREAD, *PDBGKM_EXIT_THREAD;

/**
 * The DBGKM_EXIT_PROCESS structure contains the exit status of a process.
 */
typedef struct _DBGKM_EXIT_PROCESS
{
    NTSTATUS ExitStatus;
} DBGKM_EXIT_PROCESS, *PDBGKM_EXIT_PROCESS;

/**
 * The DBGKM_LOAD_DLL structure contains information about a loaded DLL.
 */
typedef struct _DBGKM_LOAD_DLL
{
    HANDLE FileHandle;
    PVOID BaseOfDll;
    ULONG DebugInfoFileOffset;
    ULONG DebugInfoSize;
    PVOID NamePointer;
} DBGKM_LOAD_DLL, *PDBGKM_LOAD_DLL;

/**
 * The DBGKM_UNLOAD_DLL structure contains the base address of an unloaded DLL.
 */
typedef struct _DBGKM_UNLOAD_DLL
{
    PVOID BaseAddress;
} DBGKM_UNLOAD_DLL, *PDBGKM_UNLOAD_DLL;

/**
 * The DBG_STATE enumeration identifies the debug state change reported in DBGUI_WAIT_STATE_CHANGE.
 * \remarks NewState selects the active member of DBGUI_WAIT_STATE_CHANGE.StateInfo.
 * Exception, breakpoint, and single-step state changes all use StateInfo.Exception.
 */
typedef enum _DBG_STATE
{
    DbgIdle,                        // No debug events
    DbgReplyPending,                // Reply-pending state
    DbgCreateThreadStateChange,     // StateInfo.CreateThread
    DbgCreateProcessStateChange,    // StateInfo.CreateProcessInfo
    DbgExitThreadStateChange,       // StateInfo.ExitThread
    DbgExitProcessStateChange,      // StateInfo.ExitProcess
    DbgExceptionStateChange,        // StateInfo.Exception
    DbgBreakpointStateChange,       // StateInfo.Exception; ExceptionCode == STATUS_BREAKPOINT
    DbgSingleStepStateChange,       // StateInfo.Exception; ExceptionCode == STATUS_SINGLE_STEP
    DbgLoadDllStateChange,          // StateInfo.LoadDll
    DbgUnloadDllStateChange         // StateInfo.UnloadDll
} DBG_STATE, *PDBG_STATE;

/**
 * The DBGUI_CREATE_THREAD structure contains UI-level information about a newly created thread.
 */
typedef struct _DBGUI_CREATE_THREAD
{
    HANDLE HandleToThread;
    DBGKM_CREATE_THREAD NewThread;
} DBGUI_CREATE_THREAD, *PDBGUI_CREATE_THREAD;

/**
 * The DBGUI_CREATE_PROCESS structure contains UI-level information about a newly created process.
 */
typedef struct _DBGUI_CREATE_PROCESS
{
    HANDLE HandleToProcess;
    HANDLE HandleToThread;
    DBGKM_CREATE_PROCESS NewProcess;
} DBGUI_CREATE_PROCESS, *PDBGUI_CREATE_PROCESS;

/**
 * The DBGUI_WAIT_STATE_CHANGE structure contains a debug state change returned by NtWaitForDebugEvent.
 * \remarks NewState identifies the event and selects the active StateInfo member.
 * AppClientId contains the process and thread identifiers of the event source.
 * DbgExceptionStateChange, DbgBreakpointStateChange, and DbgSingleStepStateChange share the Exception member.
 * Check the wait status before using the event data; a timeout or an alerted wait does not supply an event.
 * On x64, the structure is 0xB8 bytes, with NewState at 0x00, AppClientId at 0x08, and StateInfo at 0x18.
 */
typedef struct _DBGUI_WAIT_STATE_CHANGE
{
    DBG_STATE NewState;
    CLIENT_ID AppClientId;
    union
    {
        DBGKM_EXCEPTION Exception;              // DbgExceptionStateChange, DbgBreakpointStateChange, DbgSingleStepStateChange
        DBGUI_CREATE_THREAD CreateThread;       // DbgCreateThreadStateChange
        DBGUI_CREATE_PROCESS CreateProcessInfo; // DbgCreateProcessStateChange
        DBGKM_EXIT_THREAD ExitThread;           // DbgExitThreadStateChange
        DBGKM_EXIT_PROCESS ExitProcess;         // DbgExitProcessStateChange
        DBGKM_LOAD_DLL LoadDll;                 // DbgLoadDllStateChange
        DBGKM_UNLOAD_DLL UnloadDll;             // DbgUnloadDllStateChange
    } StateInfo;
} DBGUI_WAIT_STATE_CHANGE, *PDBGUI_WAIT_STATE_CHANGE;

#define DEBUG_READ_EVENT 0x0001
#define DEBUG_PROCESS_ASSIGN 0x0002
#define DEBUG_SET_INFORMATION 0x0004
#define DEBUG_QUERY_INFORMATION 0x0008
#define DEBUG_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | \
    DEBUG_READ_EVENT | DEBUG_PROCESS_ASSIGN | DEBUG_SET_INFORMATION | \
    DEBUG_QUERY_INFORMATION)

#define DEBUG_KILL_ON_CLOSE 0x1

/**
 * The DEBUGOBJECTINFOCLASS enumeration defines the information classes for debug objects.
 * \remarks NtSetInformationDebugObject supports only DebugObjectKillProcessOnExitInformation.
 * The information buffer must contain a ULONG (4 bytes) with a value of 0 or DEBUG_KILL_ON_CLOSE.
 * The debug object handle must have DEBUG_SET_INFORMATION access.
 * Other information classes or flag bits are rejected with STATUS_INVALID_PARAMETER.
 * An incorrect buffer length is rejected with STATUS_INFO_LENGTH_MISMATCH.
 */
typedef enum _DEBUGOBJECTINFOCLASS
{
    DebugObjectUnusedInformation,
    DebugObjectKillProcessOnExitInformation, // s: ULONG (DEBUG_KILL_ON_CLOSE)
    MaxDebugObjectInfoClass
} DEBUGOBJECTINFOCLASS, *PDEBUGOBJECTINFOCLASS;

//
// System calls
//

/**
 * Creates a debug object.
 *
 * \param DebugObjectHandle A pointer to a variable that receives the debug object handle.
 * \param DesiredAccess The access rights desired for the debug object.
 * \param ObjectAttributes Optional. A pointer to an OBJECT_ATTRIBUTES structure that specifies the attributes
 * of the debug object.
 * \param Flags Flags for the debug object creation. (DEBUG_KILL_ON_CLOSE)
 * \return NTSTATUS Successful or errant status.
 * \remarks Creates a debugger event object; it does not attach to any process. The Flags mask admits only bit
 * 0x1. That bit controls kill-on-close policy. Pass zero to leave kill-on-close disabled, and close the
 * returned debug-object handle with NtClose.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateDebugObject(
    _Out_ PHANDLE DebugObjectHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG Flags
    );

/**
 * Attaches a debugger to an active process.
 *
 * \param ProcessHandle A handle to the process to be debugged.
 * \param DebugObjectHandle A handle to the debug object.
 * \return NTSTATUS Successful or errant status.
 * \remarks Attaches a debug object to a process and therefore changes execution/debug-event
 * behavior. The native entry rejects attaching to the caller's current process or the initial system process
 * and applies protected-process compatibility checks. Creating an unattached debug object does not test this path.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDebugActiveProcess(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE DebugObjectHandle
    );

/**
 * Continues a thread that was stopped by a debug event.
 *
 * \param DebugObjectHandle A handle to the debug object.
 * \param ClientId A pointer to a CLIENT_ID structure that identifies the thread to be continued.
 * \param ContinueStatus The status code to use when continuing the thread.
 * \return NTSTATUS Successful or errant status.
 * \remarks Resumes a previously reported debugger event identified by its client IDs. Supplying
 * DBG_CONTINUE alone does not create an event to continue; an empty owned debug object returns
 * STATUS_INVALID_PARAMETER. Do not confuse this operation with resuming an arbitrary suspended thread.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDebugContinue(
    _In_ HANDLE DebugObjectHandle,
    _In_ PCLIENT_ID ClientId,
    _In_ NTSTATUS ContinueStatus
    );

/**
 * Stops debugging a process.
 *
 * \param ProcessHandle A handle to the process.
 * \param DebugObjectHandle A handle to the debug object.
 * \return NTSTATUS Successful or errant status.
 * \remarks Removes the specified debug association rather than closing either handle. The
 * entry references the process and checks protected-process compatibility before invoking the
 * removal path. Pending debugger-event and target-lifetime handling is separate from ordinary NtClose cleanup.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRemoveProcessDebug(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE DebugObjectHandle
    );

/**
 * Sets information for a debug object.
 *
 * \param DebugObjectHandle A handle to the debug object.
 * \param DebugObjectInformationClass The information class to be set.
 * \param DebugInformation A pointer to the buffer that contains the information.
 * \param DebugInformationLength The length of the information buffer, in bytes.
 * \param ReturnLength Optional. A pointer to a variable that receives the number of bytes returned.
 * \return NTSTATUS Successful or errant status.
 * \remarks The setter accepts only class 1, exactly 4 input bytes, and flag bit
 * 0x1. It requires debug-object access 0x4. ReturnLength is initialized to zero and becomes 4 on a length
 * mismatch; it is not a general count of data returned by this setter. Input alignment is checked at 4 bytes.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationDebugObject(
    _In_ HANDLE DebugObjectHandle,
    _In_ DEBUGOBJECTINFOCLASS DebugObjectInformationClass,
    _In_reads_bytes_(DebugInformationLength) PVOID DebugInformation,
    _In_ ULONG DebugInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

/**
 * Waits for a debug event to occur.
 *
 * \param DebugObjectHandle A handle to the debug object.
 * \param Alertable Specifies whether the wait is alertable.
 * \param Timeout Optional. A pointer to a LARGE_INTEGER structure that specifies the timeout.
 * \param WaitStateChange A pointer to a DBGUI_WAIT_STATE_CHANGE structure that receives information about the debug event.
 * \return NTSTATUS Successful or errant status.
 * \remarks Waits for debugger events, not ordinary process completion. A zero timeout on an
 * unattached empty debug object returns STATUS_TIMEOUT; the state-change output must not be interpreted as
 * a delivered event in that case. A real event requires the debugger's corresponding continue protocol.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForDebugEvent(
    _In_ HANDLE DebugObjectHandle,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_ PDBGUI_WAIT_STATE_CHANGE WaitStateChange
    );

//
// Debugging UI
//

/**
 * Connects the current thread to the debugger.
 *
 * \return NTSTATUS Successful or errant status.
 * \remarks Uses the calling TEB debug-object slot, creating a debug object if required. 
 * Creation is not attachment to a target process; keep slot ownership and handle closure consistent with later debug operations.
 */
NTSYSAPI
NTSTATUS
NTAPI
DbgUiConnectToDbg(
    VOID
    );

/**
 * Retrieves the debug object handle for the current thread.
 *
 * \return HANDLE The debug object handle.
 * \remarks Reads/writes the calling TEB debug-object slot without duplicating, attaching, detaching or closing the handle.
 * The setter is not process debugging initialization; callers own the handle lifetime and must not overwrite unrelated debugger state.
 */
NTSYSAPI
HANDLE
NTAPI
DbgUiGetThreadDebugObject(
    VOID
    );

/**
 * Sets the debug object handle for the current thread.
 *
 * \param DebugObject The debug object handle.
 * \remarks Reads/writes the calling TEB debug-object slot without duplicating, attaching, detaching or closing the handle.
 * The setter is not process debugging initialization; callers own the handle lifetime and must not overwrite unrelated debugger state.
 */
NTSYSAPI
VOID
NTAPI
DbgUiSetThreadDebugObject(
    _In_ HANDLE DebugObject
    );

/**
 * Waits for a debug state change.
 *
 * \param StateChange A pointer to a DBGUI_WAIT_STATE_CHANGE structure that receives the state change information.
 * \param Timeout Optional. A pointer to a LARGE_INTEGER structure that specifies the timeout.
 * \return NTSTATUS Successful or errant status.
 * \remarks Uses the calling TEB debug-object slot for wait, continuation or removal. A timeout is not a valid new debug event.
 * Continuing or detaching affects debuggee execution and must follow an established debugger ownership protocol.
 */
NTSYSAPI
NTSTATUS
NTAPI
DbgUiWaitStateChange(
    _Out_ PDBGUI_WAIT_STATE_CHANGE StateChange,
    _In_opt_ PLARGE_INTEGER Timeout
    );

/**
 * Continues a debug state change.
 *
 * \param AppClientId A pointer to a CLIENT_ID structure that identifies the thread to be continued.
 * \param ContinueStatus The status code to use when continuing the thread.
 * \return NTSTATUS Successful or errant status.
 * \remarks Uses the calling TEB debug-object slot for wait, continuation or removal. A timeout is not a valid new debug event.
 * Continuing or detaching affects debuggee execution and must follow an established debugger ownership protocol.
 */
NTSYSAPI
NTSTATUS
NTAPI
DbgUiContinue(
    _In_ PCLIENT_ID AppClientId,
    _In_ NTSTATUS ContinueStatus
    );

/**
 * Stops debugging a process.
 *
 * \param Process A handle to the process.
 * \return NTSTATUS Successful or errant status.
 * \remarks Uses the calling TEB debug-object slot for wait, continuation or removal. A timeout is not a valid new debug event.
 * Continuing or detaching affects debuggee execution and must follow an established debugger ownership protocol.
 */
NTSYSAPI
NTSTATUS
NTAPI
DbgUiStopDebugging(
    _In_ HANDLE Process
    );

/**
 * Attaches a debugger to an active process.
 *
 * \param Process A handle to the process.
 * \return NTSTATUS Successful or errant status.
 * \remarks Participates in attaching to a target and/or dispatching a break-in thread. This is execution-affecting debugger machinery, 
 * not a passive inspection helper. Remote process lifecycle and exception behavior are distinct from local state conversion.
 */
NTSYSAPI
NTSTATUS
NTAPI
DbgUiDebugActiveProcess(
    _In_ HANDLE Process
    );

/**
 * Remotely triggers a breakpoint in a process.
 *
 * \param Context A pointer to the context for the breakpoint.
 * \remarks Participates in attaching to a target and/or dispatching a break-in thread. This is execution-affecting debugger machinery, 
 * not a passive inspection helper. Remote process lifecycle and exception behavior are distinct from local state conversion.
 */
NTSYSAPI
VOID
NTAPI
DbgUiRemoteBreakin(
    _In_ PVOID Context
    );

/**
 * Issues a remote breakpoint in a process.
 *
 * \param Process A handle to the process.
 * \return NTSTATUS Successful or errant status.
 * \remarks Participates in attaching to a target and/or dispatching a break-in thread. This is execution-affecting debugger machinery, 
 * not a passive inspection helper. Remote process lifecycle and exception behavior are distinct from local state conversion.
 */
NTSYSAPI
NTSTATUS
NTAPI
DbgUiIssueRemoteBreakin(
    _In_ HANDLE Process
    );

/**
 * Converts a state change structure to a debug event structure.
 *
 * \param StateChange A pointer to a DBGUI_WAIT_STATE_CHANGE structure.
 * \param DebugEvent A pointer to a DEBUG_EVENT structure that receives the converted information.
 * \return NTSTATUS Successful or errant status.
 * \remarks Converts a native debug state to DEBUG_EVENT. The converter writes process/thread IDs before validating the state, 
 * so failure can leave partial output. Exit/exception records can be converted locally; create-thread/process paths additionally query a supplied thread handle. 
 * Do not interpret conversion as attaching to or continuing a debuggee.
 */
NTSYSAPI
NTSTATUS
NTAPI
DbgUiConvertStateChangeStructure(
    _In_ PDBGUI_WAIT_STATE_CHANGE StateChange,
    _Out_ LPDEBUG_EVENT DebugEvent
    );

/**
 * Converts a state change structure to a debug event structure (extended).
 *
 * \param StateChange A pointer to a DBGUI_WAIT_STATE_CHANGE structure.
 * \param DebugEvent A pointer to a DEBUG_EVENT structure that receives the converted information.
 * \return NTSTATUS Successful or errant status.
 * \remarks Converts a native debug state to DEBUG_EVENT. The converter writes process/thread IDs before validating the state, 
 * so failure can leave partial output. Exit/exception records can be converted locally; create-thread/process paths additionally query a supplied thread handle. 
 * Do not interpret conversion as attaching to or continuing a debuggee.
 */
NTSYSAPI
NTSTATUS
NTAPI
DbgUiConvertStateChangeStructureEx(
    _In_ PDBGUI_WAIT_STATE_CHANGE StateChange,
    _Out_ LPDEBUG_EVENT DebugEvent
    );

#endif // _NTDBG_H
