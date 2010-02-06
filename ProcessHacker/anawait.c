/*
 * Process Hacker - 
 *   thread wait analysis
 * 
 * Copyright (C) 2010 wj32
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

#include <phgui.h>

typedef struct _ANALYZE_WAIT_CONTEXT
{
    BOOLEAN Found;
    HANDLE ProcessHandle;
    PPH_SYMBOL_PROVIDER SymbolProvider;
    PPH_STRING_BUILDER StringBuilder;

    PVOID PrevParams[4];
} ANALYZE_WAIT_CONTEXT, *PANALYZE_WAIT_CONTEXT;

BOOLEAN NTAPI PhpWalkThreadStackAnalyzeCallback(
    __in PPH_THREAD_STACK_FRAME StackFrame,
    __in PVOID Context
    );

VOID PhUiAnalyzeWaitThread(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in HANDLE ThreadId,
    __in PPH_SYMBOL_PROVIDER SymbolProvider
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    ANALYZE_WAIT_CONTEXT context;
    PPH_STRING string;

    if (!NT_SUCCESS(status = PhOpenThread(
        &threadHandle,
        THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME,
        ThreadId
        )))
    {
        PhShowStatus(hWnd, L"Unable to open the thread", status, 0);
        return;
    }

    context.ProcessHandle = SymbolProvider->ProcessHandle;
    context.SymbolProvider = SymbolProvider;
    context.StringBuilder = PhCreateStringBuilder(10);

    PhWalkThreadStack(
        threadHandle,
        SymbolProvider->ProcessHandle,
        PH_WALK_I386_STACK,
        PhpWalkThreadStackAnalyzeCallback,
        &context
        );
    CloseHandle(threadHandle);

    if (context.Found)
    {
        string = PhReferenceStringBuilderString(context.StringBuilder);
        PhShowInformationDialog(hWnd, string->Buffer);
        PhDereferenceObject(string);
    }
    else
    {
        PhShowInformation(hWnd, L"The thread does not appear to be waiting.");
    }

    PhDereferenceObject(context.StringBuilder);
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
            !PhIsStringNullOrEmpty(name) ? name->Buffer : L"(unnamed object)"
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

static BOOLEAN NTAPI PhpWalkThreadStackAnalyzeCallback(
    __in PPH_THREAD_STACK_FRAME StackFrame,
    __in PVOID Context
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

#define FUNC_MATCH(Name) PhStringStartsWith2(name, L##Name, TRUE)
#define NT_FUNC_MATCH(Name) ( \
    PhStringStartsWith2(name, L"ntdll.dll!Nt" L##Name, TRUE) || \
    PhStringStartsWith2(name, L"ntdll.dll!Zw" L##Name, TRUE) \
    )

    if (!name)
    {
        // Dummy
    }
    else if (FUNC_MATCH("kernel32.dll!Sleep"))
    {
        PhStringBuilderAppendFormat(
            context->StringBuilder,
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
                PhStringBuilderAppendFormat(
                    context->StringBuilder,
                    L"Thread is sleeping. Timeout: %I64u milliseconds.",
                    -timeout.QuadPart / PH_TIMEOUT_TO_MS
                    );
            }
            else
            {
                // TODO
            }
        }
        else
        {
            PhStringBuilderAppend2(
                context->StringBuilder,
                L"Thread is sleeping."
                );
        }
    }
    else if (NT_FUNC_MATCH("DeviceIoControlFile"))
    {
        HANDLE handle = (HANDLE)StackFrame->Params[0];

        PhStringBuilderAppend2(
            context->StringBuilder,
            L"Thread is waiting for an I/O control request:\r\n"
            );
        PhStringBuilderAppend(
            context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else if (NT_FUNC_MATCH("FsControlFile"))
    {
        HANDLE handle = StackFrame->Params[0];

        PhStringBuilderAppend2(
            context->StringBuilder,
            L"Thread is waiting for a FS control request:\r\n"
            );
        PhStringBuilderAppend(
            context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else if (NT_FUNC_MATCH("QueryObject"))
    {
        HANDLE handle = StackFrame->Params[0];

        // Use the KiFastSystemCall args if the handle we have is wrong.
        if ((ULONG_PTR)handle % 4 != 0 || !handle)
            handle = context->PrevParams[1];

        PhStringBuilderAppend2(
            context->StringBuilder,
            L"Thread is querying an object:\r\n"
            );
        PhStringBuilderAppend(
            context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else if (NT_FUNC_MATCH("ReadFile") || NT_FUNC_MATCH("WriteFile"))
    {
        HANDLE handle = StackFrame->Params[0];

        PhStringBuilderAppend2(
            context->StringBuilder,
            L"Thread is waiting for file I/O:\r\n"
            );
        PhStringBuilderAppend(
            context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else if (NT_FUNC_MATCH("RemoveIoCompletion"))
    {
        HANDLE handle = StackFrame->Params[0];

        PhStringBuilderAppend2(
            context->StringBuilder,
            L"Thread is waiting for an I/O completion port:\r\n"
            );
        PhStringBuilderAppend(
            context->StringBuilder,
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

        PhStringBuilderAppend2(
            context->StringBuilder,
            L"Thread is waiting for a LPC port:\r\n"
            );
        PhStringBuilderAppend(
            context->StringBuilder,
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

        PhStringBuilderAppendFormat(
            context->StringBuilder,
            L"Thread is waiting (%s) for an event pair:\r\n",
            name->Buffer
            );
        PhStringBuilderAppend(
            context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else if (
        FUNC_MATCH("user32.dll!NtUserGetMessage") ||
        FUNC_MATCH("user32.dll!NtUserWaitMessage")
        )
    {
        PhStringBuilderAppend2(
            context->StringBuilder,
            L"Thread is waiting for a USER message.\r\n"
            );
    }
    else if (NT_FUNC_MATCH("WaitForDebugEvent"))
    {
        HANDLE handle = StackFrame->Params[0];

        PhStringBuilderAppend2(
            context->StringBuilder,
            L"Thread is waiting for a debug event:\r\n"
            );
        PhStringBuilderAppend(
            context->StringBuilder,
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

        PhStringBuilderAppendFormat(
            context->StringBuilder,
            L"Thread is waiting (%s) for a keyed event (key 0x%Ix):\r\n",
            name->Buffer,
            key
            );
        PhStringBuilderAppend(
            context->StringBuilder,
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
        HANDLE handles[MAXIMUM_WAIT_OBJECTS];
        ULONG i;

        if (numberOfHandles > MAXIMUM_WAIT_OBJECTS)
        {
            numberOfHandles = (ULONG)context->PrevParams[1];
            addressOfHandles = context->PrevParams[2];
            waitType = (WAIT_TYPE)context->PrevParams[3];
        }

        if (numberOfHandles <= MAXIMUM_WAIT_OBJECTS)
        {
            if (NT_SUCCESS(PhReadVirtualMemory(
                context->ProcessHandle,
                addressOfHandles,
                handles,
                numberOfHandles * sizeof(HANDLE),
                NULL
                )))
            {
                PhStringBuilderAppendFormat(
                    context->StringBuilder,
                    L"Thread is waiting (%s, %s) for:\r\n",
                    alertable ? L"alertable" : L"non-alertable",
                    waitType == WaitAll ? L"wait all" : L"wait any"
                    );

                for (i = 0; i < numberOfHandles; i++)
                {
                    PhStringBuilderAppend(
                        context->StringBuilder,
                        PhapGetHandleString(context->ProcessHandle, handles[i])
                        );
                    PhStringBuilderAppend2(
                        context->StringBuilder,
                        L"\r\n"
                        );
                }
            }
            else
            {
                // Indicate that we failed.
                numberOfHandles = MAXIMUM_WAIT_OBJECTS + 1;
            }
        }

        if (numberOfHandles > MAXIMUM_WAIT_OBJECTS)
        {
            PhStringBuilderAppend2(
                context->StringBuilder,
                L"Thread is waiting for multiple objects."
                );
        }
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

        PhStringBuilderAppendFormat(
            context->StringBuilder,
            L"Thread is waiting (%s) for:\r\n",
            alertable ? L"alertable" : L"non-alertable"
            );
        PhStringBuilderAppend(
            context->StringBuilder,
            PhapGetHandleString(context->ProcessHandle, handle)
            );
    }
    else if (NT_FUNC_MATCH("WaitForWorkViaWorkerFactory"))
    {
        HANDLE handle = StackFrame->Params[0];

        PhStringBuilderAppend2(
            context->StringBuilder,
            L"Thread is waiting for work from a worker factory:\r\n"
            );
        PhStringBuilderAppend(
            context->StringBuilder,
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
