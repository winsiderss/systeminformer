/*
 * Process Hacker - 
 *   thread wait analysis
 * 
 * Copyright (C) 2010-2011 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * There are two ways of seeing what a thread is waiting on. The first method 
 * is to walk the stack of a thread and read the arguments to whatever system 
 * call it is blocking on; this only works on x86 because on x64 the arguments 
 * are passed in registers (at least the first four are). The second method 
 * involves using the ThreadLastSystemCall info class for NtQueryInformationThread 
 * to retrieve the first argument to the system call the thread is blocking on. 
 * This is obviously only useful for NtWaitForSingleObject.
 *
 * There are other methods for specific scenarios, like USER messages.
 */

#include <phapp.h>

typedef HWND (WINAPI *_GetSendMessageReceiver)(
    __in HANDLE ThreadId
    );

typedef NTSTATUS (NTAPI *_NtAlpcQueryInformation)(
    __in HANDLE PortHandle,
    __in ALPC_PORT_INFORMATION_CLASS PortInformationClass,
    __out_bcount(Length) PVOID PortInformation,
    __in ULONG Length,
    __out_opt PULONG ReturnLength
    );

typedef struct _ANALYZE_WAIT_CONTEXT
{
    BOOLEAN Found;
    BOOLEAN IsWow64;
    HANDLE ProcessId;
    HANDLE ThreadId;
    HANDLE ProcessHandle;

    PPH_SYMBOL_PROVIDER SymbolProvider;
    PH_STRING_BUILDER StringBuilder;

    PVOID PrevParams[4];
} ANALYZE_WAIT_CONTEXT, *PANALYZE_WAIT_CONTEXT;

VOID PhpAnalyzeWaitPassive(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in HANDLE ThreadId
    );

BOOLEAN NTAPI PhpWalkThreadStackAnalyzeCallback(
    __in PPH_THREAD_STACK_FRAME StackFrame,
    __in_opt PVOID Context
    );

VOID PhpInitializeServiceNumbers();

PPH_STRING PhapGetHandleString(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle
    );

VOID PhpGetWfmoInformation(
    __in HANDLE ProcessHandle,
    __in BOOLEAN IsWow64,
    __in ULONG NumberOfHandles,
    __in PHANDLE AddressOfHandles,
    __in WAIT_TYPE WaitType,
    __in BOOLEAN Alertable,
    __inout PPH_STRING_BUILDER StringBuilder
    );

PPH_STRING PhapGetSendMessageReceiver(
    __in HANDLE ThreadId
    );

PPH_STRING PhapGetAlpcInformation(
    __in HANDLE ThreadId
    );

static PH_INITONCE ServiceNumbersInitOnce = PH_INITONCE_INIT;
static USHORT WfsoNumber = -1;
static USHORT WfmoNumber = -1;

VOID PhUiAnalyzeWaitThread(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in HANDLE ThreadId,
    __in PPH_SYMBOL_PROVIDER SymbolProvider
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
#ifdef _M_X64
    HANDLE processHandle;
    BOOLEAN isWow64;
#endif
    ANALYZE_WAIT_CONTEXT context;

#ifdef _M_X64
    // Determine if the process is WOW64. If not, we use the passive method.

    if (!NT_SUCCESS(status = PhOpenProcess(&processHandle, ProcessQueryAccess, ProcessId)))
    {
        PhShowStatus(hWnd, L"Unable to open the process", status, 0);
        return;
    }

    if (!NT_SUCCESS(status = PhGetProcessIsWow64(processHandle, &isWow64)) || !isWow64)
    {
        PhpAnalyzeWaitPassive(hWnd, ProcessId, ThreadId);
        return;
    }

    NtClose(processHandle);
#endif

    if (!NT_SUCCESS(status = PhOpenThread(
        &threadHandle,
        THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME,
        ThreadId
        )))
    {
        PhShowStatus(hWnd, L"Unable to open the thread", status, 0);
        return;
    }

    context.ProcessId = ProcessId;
    context.ThreadId = ThreadId;

    context.ProcessHandle = SymbolProvider->ProcessHandle;
    context.SymbolProvider = SymbolProvider;
    PhInitializeStringBuilder(&context.StringBuilder, 100);

    PhWalkThreadStack(
        threadHandle,
        SymbolProvider->ProcessHandle,
        PH_WALK_I386_STACK,
        PhpWalkThreadStackAnalyzeCallback,
        &context
        );
    NtClose(threadHandle);

    if (context.Found)
    {
        PhShowInformationDialog(hWnd, context.StringBuilder.String->Buffer);
    }
    else
    {
        PhShowInformation(hWnd, L"The thread does not appear to be waiting.");
    }

    PhDeleteStringBuilder(&context.StringBuilder);
}

VOID PhpAnalyzeWaitPassive(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in HANDLE ThreadId
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE threadHandle;
    THREAD_LAST_SYSCALL_INFORMATION lastSystemCall;
    PH_STRING_BUILDER stringBuilder;
    PPH_STRING string;

    PhpInitializeServiceNumbers();

    if (!NT_SUCCESS(status = PhOpenThread(&threadHandle, THREAD_GET_CONTEXT, ThreadId)))
    {
        PhShowStatus(hWnd, L"Unable to open the thread", status, 0);
        return;
    }

    if (!NT_SUCCESS(status = NtQueryInformationThread(
        threadHandle,
        ThreadLastSystemCall,
        &lastSystemCall,
        sizeof(THREAD_LAST_SYSCALL_INFORMATION),
        NULL
        )))
    {
        NtClose(threadHandle);
        PhShowInformation(hWnd, L"Unable to determine whether the thread is waiting.");
        return;
    }

    if (!NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_DUP_HANDLE, ProcessId)))
    {
        NtClose(threadHandle);
        PhShowStatus(hWnd, L"Unable to open the process", status, 0);
        return;
    }

    PhInitializeStringBuilder(&stringBuilder, 100);

    if (lastSystemCall.SystemCallNumber == WfsoNumber)
    {
        string = PhapGetHandleString(processHandle, lastSystemCall.FirstArgument);

        PhAppendFormatStringBuilder(&stringBuilder, L"Thread is waiting for:\r\n");
        PhAppendStringBuilder(&stringBuilder, string);
    }
    else if (lastSystemCall.SystemCallNumber == WfmoNumber)
    {
        PhAppendStringBuilder2(&stringBuilder, L"Thread is waiting for multiple objects.");
    }
    else
    {
        string = PhapGetSendMessageReceiver(ThreadId);

        if (string)
        {
            PhAppendStringBuilder2(&stringBuilder, L"Thread is sending a USER message:\r\n");
            PhAppendStringBuilder(&stringBuilder, string);
        }
        else
        {
            string = PhapGetAlpcInformation(ThreadId);

            if (string)
            {
                PhAppendStringBuilder2(&stringBuilder, L"Thread is waiting for an ALPC port:\r\n");
                PhAppendStringBuilder(&stringBuilder, string);
            }
        }
    }

    if (stringBuilder.String->Length == 0)
        PhAppendStringBuilder2(&stringBuilder, L"Unable to determine why the thread is waiting.");

    PhShowInformationDialog(hWnd, stringBuilder.String->Buffer);

    PhDeleteStringBuilder(&stringBuilder);
    NtClose(processHandle);
    NtClose(threadHandle);
}

static BOOLEAN NTAPI PhpWalkThreadStackAnalyzeCallback(
    __in PPH_THREAD_STACK_FRAME StackFrame,
    __in_opt PVOID Context
    )
{
    PANALYZE_WAIT_CONTEXT context = (PANALYZE_WAIT_CONTEXT)Context;
    PPH_STRING name;

    name = PhGetSymbolFromAddress(
        context->SymbolProvider,
        (ULONG64)StackFrame->PcAddress,
        NULL,
        NULL,
        NULL,
        NULL
        );

    if (!name)
        return TRUE;

    context->Found = TRUE;

#define FUNC_MATCH(Name) PhStartsWithString2(name, L##Name, TRUE)
#define NT_FUNC_MATCH(Name) ( \
    PhStartsWithString2(name, L"ntdll.dll!Nt" L##Name, TRUE) || \
    PhStartsWithString2(name, L"ntdll.dll!Zw" L##Name, TRUE) \
    )

    if (!name)
    {
        // Dummy
    }
    else if (FUNC_MATCH("kernel32.dll!Sleep"))
    {
        PhAppendFormatStringBuilder(
            &context->StringBuilder,
            L"Thread is sleeping. Timeout: %u milliseconds.",
            (ULONG)StackFrame->Params[0]
            );
    }
    else if (NT_FUNC_MATCH("DelayExecution"))
    {
        BOOLEAN alertable = !!StackFrame->Params[0];
        PVOID timeoutAddress = StackFrame->Params[1];
        LARGE_INTEGER timeout;

        if (NT_SUCCESS(PhReadVirtualMemory(
            context->ProcessHandle,
            timeoutAddress,
            &timeout,
            sizeof(LARGE_INTEGER),
            NULL
            )))
        {
            if (timeout.QuadPart < 0)
            {
                PhAppendFormatStringBuilder(
                    &context->StringBuilder,
                    L"Thread is sleeping. Timeout: %I64u milliseconds.",
                    -timeout.QuadPart / PH_TIMEOUT_MS
                    );
            }
            else
            {
                // TODO
            }
        }
        else
        {
            PhAppendStringBuilder2(
                &context->StringBuilder,
                L"Thread is sleeping."
                );
        }
    }
    else if (NT_FUNC_MATCH("DeviceIoControlFile"))
    {
        HANDLE handle = (HANDLE)StackFrame->Params[0];

        PhAppendStringBuilder2(
            &context->StringBuilder,
            L"Thread is waiting for an I/O control request:\r\n"
            );
        PhAppendStringBuilder(
            &context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else if (NT_FUNC_MATCH("FsControlFile"))
    {
        HANDLE handle = StackFrame->Params[0];

        PhAppendStringBuilder2(
            &context->StringBuilder,
            L"Thread is waiting for a FS control request:\r\n"
            );
        PhAppendStringBuilder(
            &context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else if (NT_FUNC_MATCH("QueryObject"))
    {
        HANDLE handle = StackFrame->Params[0];

        // Use the KiFastSystemCall args if the handle we have is wrong.
        if ((ULONG_PTR)handle % 4 != 0 || !handle)
            handle = context->PrevParams[1];

        PhAppendStringBuilder2(
            &context->StringBuilder,
            L"Thread is querying an object:\r\n"
            );
        PhAppendStringBuilder(
            &context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else if (NT_FUNC_MATCH("ReadFile") || NT_FUNC_MATCH("WriteFile"))
    {
        HANDLE handle = StackFrame->Params[0];

        PhAppendStringBuilder2(
            &context->StringBuilder,
            L"Thread is waiting for file I/O:\r\n"
            );
        PhAppendStringBuilder(
            &context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else if (NT_FUNC_MATCH("RemoveIoCompletion"))
    {
        HANDLE handle = StackFrame->Params[0];

        PhAppendStringBuilder2(
            &context->StringBuilder,
            L"Thread is waiting for an I/O completion port:\r\n"
            );
        PhAppendStringBuilder(
            &context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else if (
        NT_FUNC_MATCH("ReplyWaitReceivePort") ||
        NT_FUNC_MATCH("RequestWaitReplyPort") ||
        NT_FUNC_MATCH("AlpcSendWaitReceivePort")
        )
    {
        HANDLE handle = StackFrame->Params[0];

        PhAppendStringBuilder2(
            &context->StringBuilder,
            L"Thread is waiting for a LPC port:\r\n"
            );
        PhAppendStringBuilder(
            &context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else if (
        NT_FUNC_MATCH("SetHighWaitLowEventPair") ||
        NT_FUNC_MATCH("SetLowWaitHighEventPair") ||
        NT_FUNC_MATCH("WaitHighEventPair") ||
        NT_FUNC_MATCH("WaitLowEventPair")
        )
    {
        HANDLE handle = StackFrame->Params[0];

        if ((ULONG_PTR)handle % 4 != 0 || !handle)
            handle = context->PrevParams[1];

        PhAppendFormatStringBuilder(
            &context->StringBuilder,
            L"Thread is waiting (%s) for an event pair:\r\n",
            name->Buffer
            );
        PhAppendStringBuilder(
            &context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else if (
        FUNC_MATCH("user32.dll!NtUserGetMessage") ||
        FUNC_MATCH("user32.dll!NtUserWaitMessage")
        )
    {
        PhAppendStringBuilder2(
            &context->StringBuilder,
            L"Thread is waiting for a USER message.\r\n"
            );
    }
    else if (FUNC_MATCH("user32.dll!NtUserMessageCall"))
    {
        PPH_STRING receiverString;

        PhAppendStringBuilder2(
            &context->StringBuilder,
            L"Thread is sending a USER message:\r\n"
            );

        receiverString = PhapGetSendMessageReceiver(context->ThreadId);

        if (receiverString)
        {
            PhAppendStringBuilder(&context->StringBuilder, receiverString);
            PhAppendStringBuilder2(&context->StringBuilder, L"\r\n");
        }
        else
        {
            PhAppendStringBuilder2(&context->StringBuilder, L"Unknown.\r\n");
        }
    }
    else if (NT_FUNC_MATCH("WaitForDebugEvent"))
    {
        HANDLE handle = StackFrame->Params[0];

        PhAppendStringBuilder2(
            &context->StringBuilder,
            L"Thread is waiting for a debug event:\r\n"
            );
        PhAppendStringBuilder(
            &context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else if (
        NT_FUNC_MATCH("WaitForKeyedEvent") ||
        NT_FUNC_MATCH("ReleaseKeyedEvent")
        )
    {
        HANDLE handle = StackFrame->Params[0];
        PVOID key = StackFrame->Params[1];

        PhAppendFormatStringBuilder(
            &context->StringBuilder,
            L"Thread is waiting (%s) for a keyed event (key 0x%Ix):\r\n",
            name->Buffer,
            key
            );
        PhAppendStringBuilder(
            &context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else if (
        NT_FUNC_MATCH("WaitForMultipleObjects") ||
        FUNC_MATCH("kernel32.dll!WaitForMultipleObjects")
        )
    {
        ULONG numberOfHandles = (ULONG)StackFrame->Params[0];
        PVOID addressOfHandles = StackFrame->Params[1];
        WAIT_TYPE waitType = (WAIT_TYPE)StackFrame->Params[2];
        BOOLEAN alertable = !!StackFrame->Params[3];

        if (numberOfHandles > MAXIMUM_WAIT_OBJECTS)
        {
            numberOfHandles = (ULONG)context->PrevParams[1];
            addressOfHandles = context->PrevParams[2];
            waitType = (WAIT_TYPE)context->PrevParams[3];
            alertable = FALSE;
        }

        PhpGetWfmoInformation(
            context->ProcessHandle,
            TRUE, // on x64 this function is only called for WOW64 processes
            numberOfHandles,
            addressOfHandles,
            waitType,
            alertable,
            &context->StringBuilder
            );
    }
    else if (
        NT_FUNC_MATCH("WaitForSingleObject") ||
        FUNC_MATCH("kernel32.dll!WaitForSingleObject")
        )
    {
        HANDLE handle = StackFrame->Params[0];
        BOOLEAN alertable = !!StackFrame->Params[1];

        if ((ULONG_PTR)handle % 4 != 0 || !handle)
        {
            handle = context->PrevParams[1];
            alertable = !!context->PrevParams[2];
        }

        PhAppendFormatStringBuilder(
            &context->StringBuilder,
            L"Thread is waiting (%s) for:\r\n",
            alertable ? L"alertable" : L"non-alertable"
            );
        PhAppendStringBuilder(
            &context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else if (NT_FUNC_MATCH("WaitForWorkViaWorkerFactory"))
    {
        HANDLE handle = StackFrame->Params[0];

        PhAppendStringBuilder2(
            &context->StringBuilder,
            L"Thread is waiting for work from a worker factory:\r\n"
            );
        PhAppendStringBuilder(
            &context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else
    {
        context->Found = FALSE;
    }

    PhDereferenceObject(name);
    memcpy(&context->PrevParams, StackFrame->Params, sizeof(StackFrame->Params));

    return !context->Found;
}

static BOOLEAN PhpWaitUntilThreadIsWaiting(
    __in HANDLE ThreadHandle
    )
{
    ULONG attempts;
    BOOLEAN isWaiting = FALSE;
    THREAD_BASIC_INFORMATION basicInfo;

    if (!NT_SUCCESS(PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return FALSE;

    for (attempts = 0; attempts < 5; attempts++)
    {
        PVOID processes;
        PSYSTEM_PROCESS_INFORMATION processInfo;
        ULONG i;
        LARGE_INTEGER interval;

        interval.QuadPart = -100 * PH_TIMEOUT_MS;
        NtDelayExecution(FALSE, &interval);

        if (!NT_SUCCESS(PhEnumProcesses(&processes)))
            break;

        processInfo = PhFindProcessInformation(processes, basicInfo.ClientId.UniqueProcess);

        if (processInfo)
        {
            for (i = 0; i < processInfo->NumberOfThreads; i++)
            {
                if (
                    processInfo->Threads[i].ClientId.UniqueThread == basicInfo.ClientId.UniqueThread &&
                    processInfo->Threads[i].ThreadState == Waiting &&
                    processInfo->Threads[i].WaitReason == UserRequest
                    )
                {
                    isWaiting = TRUE;
                    break;
                }
            }
        }

        PhFree(processes);

        if (isWaiting)
            break;

        interval.QuadPart = -500 * PH_TIMEOUT_MS;
        NtDelayExecution(FALSE, &interval);
    }

    return isWaiting;
}

static VOID PhpGetThreadLastSystemCallNumber(
    __in HANDLE ThreadHandle,
    __out PUSHORT LastSystemCallNumber
    )
{
    THREAD_LAST_SYSCALL_INFORMATION lastSystemCall;

    if (NT_SUCCESS(NtQueryInformationThread(
        ThreadHandle,
        ThreadLastSystemCall,
        &lastSystemCall,
        sizeof(THREAD_LAST_SYSCALL_INFORMATION),
        NULL
        )))
    {
        *LastSystemCallNumber = lastSystemCall.SystemCallNumber;
    }
}

static NTSTATUS PhpWfsoThreadStart(
    __in PVOID Parameter
    )
{
    HANDLE eventHandle;
    LARGE_INTEGER timeout;

    eventHandle = Parameter;

    timeout.QuadPart = -5 * PH_TIMEOUT_SEC;
    NtWaitForSingleObject(eventHandle, FALSE, &timeout);

    return STATUS_SUCCESS;
}

static NTSTATUS PhpWfmoThreadStart(
    __in PVOID Parameter
    )
{
    HANDLE eventHandle;
    LARGE_INTEGER timeout;

    eventHandle = Parameter;

    timeout.QuadPart = -5 * PH_TIMEOUT_SEC;
    NtWaitForMultipleObjects(1, &eventHandle, WaitAll, FALSE, &timeout);

    return STATUS_SUCCESS;
}

static VOID PhpInitializeServiceNumbers()
{
    if (PhBeginInitOnce(&ServiceNumbersInitOnce))
    {
        NTSTATUS status;
        HANDLE eventHandle;
        HANDLE threadHandle;

        // The ThreadLastSystemCall info class only works when the thread is in the Waiting 
        // state. We'll create a thread which blocks on an event object we create, then wait 
        // until it is in the Waiting state. Only then can we query the thread using 
        // ThreadLastSystemCall.

        // NtWaitForSingleObject

        status = NtCreateEvent(&eventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);

        if (NT_SUCCESS(status))
        {
            if (threadHandle = PhCreateThread(0, PhpWfsoThreadStart, eventHandle))
            {
                if (PhpWaitUntilThreadIsWaiting(threadHandle))
                {
                    PhpGetThreadLastSystemCallNumber(threadHandle, &WfsoNumber);
                }

                // Allow the thread to exit.
                NtSetEvent(eventHandle, NULL);
                NtClose(threadHandle);
            }

            NtClose(eventHandle);
        }

        // NtWaitForMultipleObjects

        status = NtCreateEvent(&eventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);

        if (NT_SUCCESS(status))
        {
            if (threadHandle = PhCreateThread(0, PhpWfmoThreadStart, eventHandle))
            {
                if (PhpWaitUntilThreadIsWaiting(threadHandle))
                {
                    PhpGetThreadLastSystemCallNumber(threadHandle, &WfmoNumber);
                }

                NtSetEvent(eventHandle, NULL);
                NtClose(threadHandle);
            }

            NtClose(eventHandle);
        }

        PhEndInitOnce(&ServiceNumbersInitOnce);
    }
}

static PPH_STRING PhapGetHandleString(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle
    )
{
    PPH_STRING typeName = NULL;
    PPH_STRING name = NULL;
    PPH_STRING result;

    PhGetHandleInformation(
        ProcessHandle,
        Handle,
        -1,
        NULL,
        &typeName,
        NULL,
        &name
        );

    if (typeName && name)
    {
        result = PhaFormatString(
            L"Handle 0x%Ix (%s): %s",
            Handle,
            typeName->Buffer,
            !PhIsNullOrEmptyString(name) ? name->Buffer : L"(unnamed object)"
            );
    }
    else
    {
        result = PhaFormatString(
            L"Handle 0x%Ix: (error querying handle)",
            Handle
            );
    }

    if (typeName)
        PhDereferenceObject(typeName);
    if (name)
        PhDereferenceObject(name);

    return result;
}

static VOID PhpGetWfmoInformation(
    __in HANDLE ProcessHandle,
    __in BOOLEAN IsWow64,
    __in ULONG NumberOfHandles,
    __in PHANDLE AddressOfHandles,
    __in WAIT_TYPE WaitType,
    __in BOOLEAN Alertable,
    __inout PPH_STRING_BUILDER StringBuilder
    )
{
    NTSTATUS status;
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    ULONG i;

    status = STATUS_SUCCESS;

    if (NumberOfHandles <= MAXIMUM_WAIT_OBJECTS)
    {
#ifdef _M_X64
        if (IsWow64)
        {
            ULONG handles32[MAXIMUM_WAIT_OBJECTS];

            if (NT_SUCCESS(status = PhReadVirtualMemory(
                ProcessHandle,
                AddressOfHandles,
                handles32,
                NumberOfHandles * sizeof(ULONG),
                NULL
                )))
            {
                for (i = 0; i < NumberOfHandles; i++)
                    handles[i] = UlongToHandle(handles32[i]);
            }
        }
        else
        {
#endif
            status = PhReadVirtualMemory(
                ProcessHandle,
                AddressOfHandles,
                handles,
                NumberOfHandles * sizeof(HANDLE),
                NULL
                );
#ifdef _M_X64
        }
#endif

        if (NT_SUCCESS(status))
        {
            PhAppendFormatStringBuilder(
                StringBuilder,
                L"Thread is waiting (%s, %s) for:\r\n",
                Alertable ? L"alertable" : L"non-alertable",
                WaitType == WaitAll ? L"wait all" : L"wait any"
                );

            for (i = 0; i < NumberOfHandles; i++)
            {
                PhAppendStringBuilder(
                    StringBuilder,
                    PhapGetHandleString(ProcessHandle, handles[i])
                    );
                PhAppendStringBuilder2(
                    StringBuilder,
                    L"\r\n"
                    );
            }
        }
    }

    if (!NT_SUCCESS(status) || NumberOfHandles > MAXIMUM_WAIT_OBJECTS)
    {
        PhAppendStringBuilder2(
            StringBuilder,
            L"Thread is waiting for multiple objects."
            );
    }
}

static PPH_STRING PhapGetSendMessageReceiver(
    __in HANDLE ThreadId
    )
{
    static _GetSendMessageReceiver GetSendMessageReceiver_I;

    HWND windowHandle;
    ULONG threadId;
    ULONG processId;
    CLIENT_ID clientId;
    PPH_STRING clientIdName;

    // GetSendMessageReceiver is an undocumented function exported by 
    // user32.dll. It retrieves the handle of the window which a thread 
    // is sending a message to.

    if (!GetSendMessageReceiver_I)
        GetSendMessageReceiver_I = PhGetProcAddress(L"user32.dll", "GetSendMessageReceiver");

    if (!GetSendMessageReceiver_I)
        return NULL;

    windowHandle = GetSendMessageReceiver_I(ThreadId);

    if (!windowHandle)
        return NULL;

    threadId = GetWindowThreadProcessId(windowHandle, &processId);

    clientId.UniqueProcess = UlongToHandle(processId);
    clientId.UniqueThread = UlongToHandle(threadId);
    clientIdName = PHA_DEREFERENCE(PhGetClientIdName(&clientId));

    return PhaFormatString(L"Window 0x%Ix (%s)", windowHandle, clientIdName->Buffer);
}

static PPH_STRING PhapGetAlpcInformation(
    __in HANDLE ThreadId
    )
{
    static _NtAlpcQueryInformation NtAlpcQueryInformation_I;

    NTSTATUS status;
    PPH_STRING string = NULL;
    HANDLE threadHandle;
    PALPC_SERVER_INFORMATION serverInfo;
    ULONG bufferLength;

    if (!NtAlpcQueryInformation_I)
        NtAlpcQueryInformation_I = PhGetProcAddress(L"ntdll.dll", "NtAlpcQueryInformation");

    if (!NtAlpcQueryInformation_I)
        return NULL;

    if (!NT_SUCCESS(PhOpenThread(&threadHandle, THREAD_QUERY_INFORMATION, ThreadId)))
        return NULL;

    bufferLength = 0x110;
    serverInfo = PhAllocate(bufferLength);
    serverInfo->In.ThreadHandle = threadHandle;

    status = NtAlpcQueryInformation_I(NULL, AlpcServerInformation, serverInfo, bufferLength, &bufferLength);

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(serverInfo);
        serverInfo = PhAllocate(bufferLength);
        serverInfo->In.ThreadHandle = threadHandle;

        status = NtAlpcQueryInformation_I(NULL, AlpcServerInformation, serverInfo, bufferLength, &bufferLength);
    }

    if (NT_SUCCESS(status) && serverInfo->Out.ThreadBlocked)
    {
        CLIENT_ID clientId;
        PPH_STRING clientIdName;

        clientId.UniqueProcess = serverInfo->Out.ConnectedProcessId;
        clientId.UniqueThread = NULL;
        clientIdName = PHA_DEREFERENCE(PhGetClientIdName(&clientId));

        string = PhaFormatString(L"ALPC Port: %.*s (%s)", serverInfo->Out.ConnectionPortName.Length / 2, serverInfo->Out.ConnectionPortName.Buffer, clientIdName->Buffer);
    }

    PhFree(serverInfo);
    NtClose(threadHandle);

    return string;
}
