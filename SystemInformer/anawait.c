/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *
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
 * There are other methods for specific scenarios, like USER messages and ALPC
 * calls.
 */

#include <phapp.h>

#include <hndlinfo.h>
#include <symprv.h>

#include <procprv.h>

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
    _In_ HWND hWnd,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId
    );

BOOLEAN NTAPI PhpWalkThreadStackAnalyzeCallback(
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _In_ PVOID Context
    );

VOID PhpAnalyzeWaitFallbacks(
    _In_ PANALYZE_WAIT_CONTEXT Context
    );

VOID PhpInitializeServiceNumbers(
    VOID
    );

PPH_STRING PhpaGetHandleString(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle
    );

VOID PhpGetWfmoInformation(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN IsWow64,
    _In_ ULONG NumberOfHandles,
    _In_ PHANDLE AddressOfHandles,
    _In_ WAIT_TYPE WaitType,
    _In_ BOOLEAN Alertable,
    _Inout_ PPH_STRING_BUILDER StringBuilder
    );

PPH_STRING PhpaGetSendMessageReceiver(
    _In_ HANDLE ThreadId
    );

PPH_STRING PhpaGetAlpcInformation(
    _In_ HANDLE ThreadId
    );

static PH_INITONCE ServiceNumbersInitOnce = PH_INITONCE_INIT;
static USHORT NumberForWfso = USHRT_MAX;
static USHORT NumberForWfmo = USHRT_MAX;
static USHORT NumberForRf = USHRT_MAX;

VOID PhUiAnalyzeWaitThread(
    _In_ HWND hWnd,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    HANDLE processHandle;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif
    CLIENT_ID clientId;
    ANALYZE_WAIT_CONTEXT context;

    if (!NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, ProcessId)))
    {
        PhShowStatus(hWnd, L"Unable to open the process.", status, 0);
        return;
    }

#ifdef _WIN64
    // Determine if the process is WOW64. If not, we use the passive method.

    if (!NT_SUCCESS(status = PhGetProcessIsWow64(processHandle, &isWow64)) || !isWow64)
    {
        PhpAnalyzeWaitPassive(hWnd, ProcessId, ThreadId);

        NtClose(processHandle);
        return;
    }
#endif

    if (!NT_SUCCESS(status = PhOpenThread(
        &threadHandle,
        THREAD_QUERY_LIMITED_INFORMATION | THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME,
        ThreadId
        )))
    {
        PhShowStatus(hWnd, L"Unable to open the thread.", status, 0);
        NtClose(processHandle);
        return;
    }

    memset(&context, 0, sizeof(ANALYZE_WAIT_CONTEXT));
    context.ProcessId = ProcessId;
    context.ThreadId = ThreadId;
    context.ProcessHandle = processHandle;
    context.SymbolProvider = SymbolProvider;
    PhInitializeStringBuilder(&context.StringBuilder, 100);

    clientId.UniqueProcess = ProcessId;
    clientId.UniqueThread = ThreadId;

    PhWalkThreadStack(
        threadHandle,
        processHandle,
        &clientId,
        SymbolProvider,
        PH_WALK_USER_WOW64_STACK,
        PhpWalkThreadStackAnalyzeCallback,
        &context
        );
    NtClose(threadHandle);
    NtClose(processHandle);

    PhpAnalyzeWaitFallbacks(&context);

    if (context.Found)
    {
        PhShowInformationDialog(hWnd, PhFinalStringBuilderString(&context.StringBuilder)->Buffer, 0);
    }
    else
    {
        PhShowInformation2(hWnd, L"The thread does not appear to be waiting.", L"%s", L"");
    }

    PhDeleteStringBuilder(&context.StringBuilder);
}

VOID PhpAnalyzeWaitPassive(
    _In_ HWND hWnd,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId
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
        PhShowStatus(hWnd, L"Unable to open the thread.", status, 0);
        return;
    }

    if (!NT_SUCCESS(status = PhGetThreadLastSystemCall(threadHandle, &lastSystemCall)))
    {
        NtClose(threadHandle);
        PhShowStatus(hWnd, L"Unable to determine whether the thread is waiting.", status, 0);
        return;
    }

    if (!NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_DUP_HANDLE, ProcessId)))
    {
        NtClose(threadHandle);
        PhShowStatus(hWnd, L"Unable to open the process.", status, 0);
        return;
    }

    PhInitializeStringBuilder(&stringBuilder, 100);

    if (lastSystemCall.SystemCallNumber == NumberForWfso)
    {
        string = PhpaGetHandleString(processHandle, lastSystemCall.FirstArgument);

        PhAppendFormatStringBuilder(&stringBuilder, L"Thread is waiting for:\r\n");
        PhAppendStringBuilder(&stringBuilder, &string->sr);
    }
    else if (lastSystemCall.SystemCallNumber == NumberForWfmo)
    {
        PhAppendFormatStringBuilder(&stringBuilder, L"Thread is waiting for multiple (%lu) objects.", PtrToUlong(lastSystemCall.FirstArgument));
    }
    else if (lastSystemCall.SystemCallNumber == NumberForRf)
    {
        string = PhpaGetHandleString(processHandle, lastSystemCall.FirstArgument);

        PhAppendFormatStringBuilder(&stringBuilder, L"Thread is waiting for file I/O:\r\n");
        PhAppendStringBuilder(&stringBuilder, &string->sr);
    }
    else
    {
        string = PhpaGetSendMessageReceiver(ThreadId);

        if (string)
        {
            PhAppendStringBuilder2(&stringBuilder, L"Thread is sending a USER message:\r\n");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
        }
        else
        {
            string = PhpaGetAlpcInformation(ThreadId);

            if (string)
            {
                PhAppendStringBuilder2(&stringBuilder, L"Thread is waiting for an ALPC port:\r\n");
                PhAppendStringBuilder(&stringBuilder, &string->sr);
            }
        }
    }

    if (stringBuilder.String->Length == 0)
        PhAppendStringBuilder2(&stringBuilder, L"Unable to determine why the thread is waiting.");

    PhShowInformationDialog(hWnd, stringBuilder.String->Buffer, 0);

    PhDeleteStringBuilder(&stringBuilder);
    NtClose(processHandle);
    NtClose(threadHandle);
}

BOOLEAN NTAPI PhpWalkThreadStackAnalyzeCallback(
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _In_ PVOID Context
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
            L"Thread is sleeping. Timeout: %lu milliseconds.",
            PtrToUlong(StackFrame->Params[0])
            );
    }
    else if (NT_FUNC_MATCH("DelayExecution"))
    {
        BOOLEAN alertable = !!StackFrame->Params[0];
        PVOID timeoutAddress = StackFrame->Params[1];
        LARGE_INTEGER timeout;

        if (NT_SUCCESS(NtReadVirtualMemory(
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
            &PhpaGetHandleString(context->ProcessHandle, handle)->sr
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
            &PhpaGetHandleString(context->ProcessHandle, handle)->sr
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
            &PhpaGetHandleString(context->ProcessHandle, handle)->sr
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
            &PhpaGetHandleString(context->ProcessHandle, handle)->sr
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
            &PhpaGetHandleString(context->ProcessHandle, handle)->sr
            );
    }
    else if (
        NT_FUNC_MATCH("ReplyWaitReceivePort") ||
        NT_FUNC_MATCH("RequestWaitReplyPort") ||
        NT_FUNC_MATCH("AlpcSendWaitReceivePort")
        )
    {
        HANDLE handle = StackFrame->Params[0];
        PPH_STRING alpcInfo;

        PhAppendStringBuilder2(
            &context->StringBuilder,
            L"Thread is waiting for an ALPC port:\r\n"
            );
        PhAppendStringBuilder(
            &context->StringBuilder,
            &PhpaGetHandleString(context->ProcessHandle, handle)->sr
            );

        if (alpcInfo = PhpaGetAlpcInformation(context->ThreadId))
        {
            PhAppendStringBuilder2(
                &context->StringBuilder,
                L"\r\n"
                );
            PhAppendStringBuilder(
                &context->StringBuilder,
                &alpcInfo->sr
                );
        }
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
            &PhpaGetHandleString(context->ProcessHandle, handle)->sr
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

        receiverString = PhpaGetSendMessageReceiver(context->ThreadId);

        if (receiverString)
        {
            PhAppendStringBuilder(&context->StringBuilder, &receiverString->sr);
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
            &PhpaGetHandleString(context->ProcessHandle, handle)->sr
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
            &PhpaGetHandleString(context->ProcessHandle, handle)->sr
            );
    }
    else if (
        NT_FUNC_MATCH("WaitForMultipleObjects") ||
        FUNC_MATCH("kernel32.dll!WaitForMultipleObjects")
        )
    {
        ULONG numberOfHandles = PtrToUlong(StackFrame->Params[0]);
        PVOID addressOfHandles = StackFrame->Params[1];
        WAIT_TYPE waitType = (WAIT_TYPE)StackFrame->Params[2];
        BOOLEAN alertable = !!StackFrame->Params[3];

        if (numberOfHandles > MAXIMUM_WAIT_OBJECTS)
        {
            numberOfHandles = PtrToUlong(context->PrevParams[1]);
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
            &PhpaGetHandleString(context->ProcessHandle, handle)->sr
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
            &PhpaGetHandleString(context->ProcessHandle, handle)->sr
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

static VOID PhpAnalyzeWaitFallbacks(
    _In_ PANALYZE_WAIT_CONTEXT Context
    )
{
    PPH_STRING info;

    // We didn't detect NtUserMessageCall, but this may still apply due to another
    // win32k system call (e.g. from EnableWindow).
    if (!Context->Found && (info = PhpaGetSendMessageReceiver(Context->ThreadId)))
    {
        PhAppendStringBuilder2(
            &Context->StringBuilder,
            L"Thread is sending a USER message:\r\n"
            );
        PhAppendStringBuilder(&Context->StringBuilder, &info->sr);
        PhAppendStringBuilder2(&Context->StringBuilder, L"\r\n");

        Context->Found = TRUE;
    }

    // Nt(Alpc)ConnectPort doesn't get detected anywhere else.
    if (!Context->Found && (info = PhpaGetAlpcInformation(Context->ThreadId)))
    {
        PhAppendStringBuilder2(
            &Context->StringBuilder,
            L"Thread is waiting for an ALPC port:\r\n"
            );
        PhAppendStringBuilder(&Context->StringBuilder, &info->sr);
        PhAppendStringBuilder2(&Context->StringBuilder, L"\r\n");

        Context->Found = TRUE;
    }
}

static BOOLEAN PhpWaitUntilThreadIsWaiting(
    _In_ HANDLE ThreadHandle
    )
{
    ULONG attempts;
    BOOLEAN isWaiting = FALSE;
    THREAD_BASIC_INFORMATION basicInfo;

    if (!NT_SUCCESS(PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return FALSE;

    for (attempts = 0; attempts < 20; attempts++)
    {
        PVOID processes;
        PSYSTEM_PROCESS_INFORMATION processInfo;
        ULONG i;

        PhDelayExecution(100);

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
                    (processInfo->Threads[i].WaitReason == UserRequest ||
                    processInfo->Threads[i].WaitReason == Executive)
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

        PhDelayExecution(500);
    }

    return isWaiting;
}

_Success_(return)
static BOOLEAN PhpGetThreadLastSystemCallNumber(
    _In_ HANDLE ThreadHandle,
    _Out_ PUSHORT LastSystemCallNumber
    )
{
    THREAD_LAST_SYSCALL_INFORMATION lastSystemCall;

    if (NT_SUCCESS(PhGetThreadLastSystemCall(ThreadHandle, &lastSystemCall)))
    {
        *LastSystemCallNumber = lastSystemCall.SystemCallNumber;
        return TRUE;
    }

    return FALSE;
}

static NTSTATUS PhpWfsoThreadStart(
    _In_ PVOID Parameter
    )
{
    HANDLE eventHandle;
    LARGE_INTEGER timeout;

    eventHandle = Parameter;

    timeout.QuadPart = -(LONGLONG)UInt32x32To64(5, PH_TIMEOUT_SEC);
    NtWaitForSingleObject(eventHandle, FALSE, &timeout);

    return STATUS_SUCCESS;
}

static NTSTATUS PhpWfmoThreadStart(
    _In_ PVOID Parameter
    )
{
    HANDLE eventHandle;
    LARGE_INTEGER timeout;

    eventHandle = Parameter;

    timeout.QuadPart = -(LONGLONG)UInt32x32To64(5, PH_TIMEOUT_SEC);
    NtWaitForMultipleObjects(1, &eventHandle, WaitAll, FALSE, &timeout);

    return STATUS_SUCCESS;
}

static NTSTATUS PhpRfThreadStart(
    _In_ PVOID Parameter
    )
{
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;
    ULONG data;

    fileHandle = Parameter;

    NtReadFile(fileHandle, NULL, NULL, NULL, &isb, &data, sizeof(ULONG), NULL, NULL);

    return STATUS_SUCCESS;
}

static VOID PhpInitializeServiceNumbers(
    VOID
    )
{
    if (PhBeginInitOnce(&ServiceNumbersInitOnce))
    {
        NTSTATUS status;
        HANDLE eventHandle;
        HANDLE threadHandle;
        HANDLE pipeReadHandle;
        HANDLE pipeWriteHandle;

        // The ThreadLastSystemCall info class only works when the thread is in the Waiting
        // state. We'll create a thread which blocks on an event object we create, then wait
        // until it is in the Waiting state. Only then can we query the thread using
        // ThreadLastSystemCall.

        // NtWaitForSingleObject

        status = NtCreateEvent(&eventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);

        if (NT_SUCCESS(status))
        {
            if (NT_SUCCESS(PhCreateThreadEx(&threadHandle, PhpWfsoThreadStart, eventHandle)))
            {
                if (PhpWaitUntilThreadIsWaiting(threadHandle))
                {
                    PhpGetThreadLastSystemCallNumber(threadHandle, &NumberForWfso);
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
            if (NT_SUCCESS(PhCreateThreadEx(&threadHandle, PhpWfmoThreadStart, eventHandle)))
            {
                if (PhpWaitUntilThreadIsWaiting(threadHandle))
                {
                    PhpGetThreadLastSystemCallNumber(threadHandle, &NumberForWfmo);
                }

                NtSetEvent(eventHandle, NULL);
                NtClose(threadHandle);
            }

            NtClose(eventHandle);
        }

        // NtReadFile

        status = PhCreatePipe(&pipeReadHandle, &pipeWriteHandle);

        if (NT_SUCCESS(status))
        {
            if (NT_SUCCESS(PhCreateThreadEx(&threadHandle, PhpRfThreadStart, pipeReadHandle)))
            {
                ULONG data = 0;
                IO_STATUS_BLOCK isb;

                if (PhpWaitUntilThreadIsWaiting(threadHandle))
                {
                    PhpGetThreadLastSystemCallNumber(threadHandle, &NumberForRf);
                }

                NtWriteFile(pipeWriteHandle, NULL, NULL, NULL, &isb, &data, sizeof(data), NULL, NULL);
                NtClose(threadHandle);
            }

            NtClose(pipeReadHandle);
            NtClose(pipeWriteHandle);
        }

        PhEndInitOnce(&ServiceNumbersInitOnce);
    }
}

static PPH_STRING PhpaGetHandleString(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle
    )
{
    PPH_STRING typeName = NULL;
    PPH_STRING name = NULL;
    PPH_STRING result;

    PhGetHandleInformation(
        ProcessHandle,
        Handle,
        ULONG_MAX,
        NULL,
        &typeName,
        NULL,
        &name
        );

    if (typeName && name)
    {
        result = PhaFormatString(
            L"Handle 0x%lx (%s): %s",
            HandleToUlong(Handle),
            typeName->Buffer,
            !PhIsNullOrEmptyString(name) ? name->Buffer : L"(unnamed object)"
            );
    }
    else
    {
        result = PhaFormatString(
            L"Handle 0x%lx: (error querying handle)",
            HandleToUlong(Handle)
            );
    }

    if (typeName)
        PhDereferenceObject(typeName);
    if (name)
        PhDereferenceObject(name);

    return result;
}

static VOID PhpGetWfmoInformation(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN IsWow64,
    _In_ ULONG NumberOfHandles,
    _In_ PHANDLE AddressOfHandles,
    _In_ WAIT_TYPE WaitType,
    _In_ BOOLEAN Alertable,
    _Inout_ PPH_STRING_BUILDER StringBuilder
    )
{
    NTSTATUS status;
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    ULONG i;

    status = STATUS_SUCCESS;

    if (NumberOfHandles <= MAXIMUM_WAIT_OBJECTS)
    {
#ifdef _WIN64
        if (IsWow64)
        {
            ULONG handles32[MAXIMUM_WAIT_OBJECTS];

            if (NT_SUCCESS(status = NtReadVirtualMemory(
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
            status = NtReadVirtualMemory(
                ProcessHandle,
                AddressOfHandles,
                handles,
                NumberOfHandles * sizeof(HANDLE),
                NULL
                );
#ifdef _WIN64
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
                    &PhpaGetHandleString(ProcessHandle, handles[i])->sr
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

static PPH_STRING PhpaGetSendMessageReceiver(
    _In_ HANDLE ThreadId
    )
{
    HWND windowHandle;
    ULONG threadId;
    ULONG processId;
    CLIENT_ID clientId;
    PPH_STRING clientIdName;
    WCHAR windowClass[64];
    PPH_STRING windowText;

    if (!PhGetSendMessageReceiver(ThreadId, &windowHandle))
        return NULL;

    threadId = GetWindowThreadProcessId(windowHandle, &processId);

    clientId.UniqueProcess = UlongToHandle(processId);
    clientId.UniqueThread = UlongToHandle(threadId);
    clientIdName = PH_AUTO(PhGetClientIdName(&clientId));

    if (!GetClassName(windowHandle, windowClass, sizeof(windowClass) / sizeof(WCHAR)))
        windowClass[0] = UNICODE_NULL;

    windowText = PH_AUTO(PhGetWindowText(windowHandle));

    return PhaFormatString(L"Window 0x%Ix (%s): %s \"%s\"", windowHandle, clientIdName->Buffer, windowClass, PhGetStringOrEmpty(windowText));
}

PPH_STRING PhpaGetAlpcInformation(
    _In_ HANDLE ThreadId
    )
{
    NTSTATUS status;
    PPH_STRING string = NULL;
    HANDLE threadHandle;
    PALPC_SERVER_INFORMATION serverInfo;
    ULONG bufferLength;

    if (!NT_SUCCESS(PhOpenThread(&threadHandle, THREAD_QUERY_INFORMATION, ThreadId)))
        return NULL;

    bufferLength = 0x110;
    serverInfo = PhAllocate(bufferLength);
    serverInfo->In.ThreadHandle = threadHandle;

    status = NtAlpcQueryInformation(NULL, AlpcServerInformation, serverInfo, bufferLength, &bufferLength);

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(serverInfo);
        serverInfo = PhAllocate(bufferLength);
        serverInfo->In.ThreadHandle = threadHandle;

        status = NtAlpcQueryInformation(NULL, AlpcServerInformation, serverInfo, bufferLength, &bufferLength);
    }

    if (NT_SUCCESS(status) && serverInfo->Out.ThreadBlocked)
    {
        CLIENT_ID clientId;
        PPH_STRING clientIdName;

        clientId.UniqueProcess = serverInfo->Out.ConnectedProcessId;
        clientId.UniqueThread = NULL;
        clientIdName = PH_AUTO(PhGetClientIdName(&clientId));

        string = PhaFormatString(L"ALPC Port: %.*s (%s)", serverInfo->Out.ConnectionPortName.Length / sizeof(WCHAR), serverInfo->Out.ConnectionPortName.Buffer, clientIdName->Buffer);
    }

    PhFree(serverInfo);
    NtClose(threadHandle);

    return string;
}
