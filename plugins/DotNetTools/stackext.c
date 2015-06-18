/*
 * Process Hacker .NET Tools -
 *   thread stack extensions
 *
 * Copyright (C) 2011-2015 wj32
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

#include "dn.h"
#include "clrsup.h"
#include "svcext.h"
#include <symprv.h>

typedef struct _THREAD_STACK_CONTEXT
{
    PCLR_PROCESS_SUPPORT Support;
    HANDLE ProcessId;
    HANDLE ThreadId;
    HANDLE ThreadHandle;

    PVOID PredictedEip;
    PVOID PredictedEbp;
    PVOID PredictedEsp;

#ifdef _WIN64
    BOOLEAN IsWow64;
    BOOLEAN ConnectedToPhSvc;
#endif
} THREAD_STACK_CONTEXT, *PTHREAD_STACK_CONTEXT;

static PPH_HASHTABLE ContextHashtable;
static PH_QUEUED_LOCK ContextHashtableLock = PH_QUEUED_LOCK_INIT;
static PH_INITONCE ContextHashtableInitOnce = PH_INITONCE_INIT;

PTHREAD_STACK_CONTEXT FindThreadStackContext(
    _In_ PVOID UniqueKey
    )
{
    PTHREAD_STACK_CONTEXT context;
    PVOID *item;

    PhAcquireQueuedLockExclusive(&ContextHashtableLock);

    item = PhFindItemSimpleHashtable(ContextHashtable, UniqueKey);

    if (item)
        context = *item;
    else
        context = NULL;

    PhReleaseQueuedLockExclusive(&ContextHashtableLock);

    return context;
}

VOID ProcessThreadStackControl(
    _In_ PPH_PLUGIN_THREAD_STACK_CONTROL Control
    )
{
    if (PhBeginInitOnce(&ContextHashtableInitOnce))
    {
        ContextHashtable = PhCreateSimpleHashtable(4);
        PhEndInitOnce(&ContextHashtableInitOnce);
    }

    switch (Control->Type)
    {
    case PluginThreadStackInitializing:
        {
            PTHREAD_STACK_CONTEXT context;
#if _WIN64
            HANDLE processHandle;
#endif

            context = PhAllocate(sizeof(THREAD_STACK_CONTEXT));
            memset(context, 0, sizeof(THREAD_STACK_CONTEXT));
            context->ProcessId = Control->u.Initializing.ProcessId;
            context->ThreadId = Control->u.Initializing.ThreadId;
            context->ThreadHandle = Control->u.Initializing.ThreadHandle;

#if _WIN64
            if (NT_SUCCESS(PhOpenProcess(&processHandle, ProcessQueryAccess, Control->u.Initializing.ProcessId)))
            {
                PhGetProcessIsWow64(processHandle, &context->IsWow64);
                NtClose(processHandle);
            }
#endif

            PhAcquireQueuedLockExclusive(&ContextHashtableLock);
            PhAddItemSimpleHashtable(ContextHashtable, Control->UniqueKey, context);
            PhReleaseQueuedLockExclusive(&ContextHashtableLock);
        }
        break;
    case PluginThreadStackUninitializing:
        {
            PTHREAD_STACK_CONTEXT context = FindThreadStackContext(Control->UniqueKey);

            if (!context)
                return;

            PhFree(context);

            PhAcquireQueuedLockExclusive(&ContextHashtableLock);
            PhRemoveItemSimpleHashtable(ContextHashtable, Control->UniqueKey);
            PhReleaseQueuedLockExclusive(&ContextHashtableLock);
        }
        break;
    case PluginThreadStackResolveSymbol:
        {
            PTHREAD_STACK_CONTEXT context = FindThreadStackContext(Control->UniqueKey);
            PPH_STRING managedSymbol = NULL;
            ULONG64 displacement;

            if (!context)
                return;

            if (context->Support)
            {
#ifndef _WIN64
                PVOID predictedEip;
                PVOID predictedEbp;
                PVOID predictedEsp;

                predictedEip = context->PredictedEip;
                predictedEbp = context->PredictedEbp;
                predictedEsp = context->PredictedEsp;

                PredictAddressesFromClrData(
                    context->Support,
                    context->ThreadId,
                    Control->u.ResolveSymbol.StackFrame->PcAddress,
                    Control->u.ResolveSymbol.StackFrame->FrameAddress,
                    Control->u.ResolveSymbol.StackFrame->StackAddress,
                    &context->PredictedEip,
                    &context->PredictedEbp,
                    &context->PredictedEsp
                    );

                // Fix up dbghelp EBP with real EBP given by the CLR data routines.
                if (Control->u.ResolveSymbol.StackFrame->PcAddress == predictedEip)
                {
                    Control->u.ResolveSymbol.StackFrame->FrameAddress = predictedEbp;
                    Control->u.ResolveSymbol.StackFrame->StackAddress = predictedEsp;
                }
#endif

                managedSymbol = GetRuntimeNameByAddressClrProcess(
                    context->Support,
                    (ULONG64)Control->u.ResolveSymbol.StackFrame->PcAddress,
                    &displacement
                    );
            }
#ifdef _WIN64
            else if (context->IsWow64 && context->ConnectedToPhSvc)
            {
                PVOID predictedEip;
                PVOID predictedEbp;
                PVOID predictedEsp;

                predictedEip = context->PredictedEip;
                predictedEbp = context->PredictedEbp;
                predictedEsp = context->PredictedEsp;

                CallPredictAddressesFromClrData(
                    context->ProcessId,
                    context->ThreadId,
                    Control->u.ResolveSymbol.StackFrame->PcAddress,
                    Control->u.ResolveSymbol.StackFrame->FrameAddress,
                    Control->u.ResolveSymbol.StackFrame->StackAddress,
                    &context->PredictedEip,
                    &context->PredictedEbp,
                    &context->PredictedEsp
                    );

                // Fix up dbghelp EBP with real EBP given by the CLR data routines.
                if (Control->u.ResolveSymbol.StackFrame->PcAddress == predictedEip)
                {
                    Control->u.ResolveSymbol.StackFrame->FrameAddress = predictedEbp;
                    Control->u.ResolveSymbol.StackFrame->StackAddress = predictedEsp;
                }

                managedSymbol = CallGetRuntimeNameByAddress(
                    context->ProcessId,
                    (ULONG64)Control->u.ResolveSymbol.StackFrame->PcAddress,
                    &displacement
                    );
            }
#endif

            if (managedSymbol)
            {
                if (displacement != 0)
                    PhMoveReference(&managedSymbol, PhFormatString(L"%s + 0x%I64x", managedSymbol->Buffer, displacement));

                if (Control->u.ResolveSymbol.Symbol)
                    PhMoveReference(&managedSymbol, PhFormatString(L"%s <-- %s", managedSymbol->Buffer, Control->u.ResolveSymbol.Symbol->Buffer));

                PhMoveReference(&Control->u.ResolveSymbol.Symbol, managedSymbol);
            }
        }
        break;
    case PluginThreadStackBeginDefaultWalkStack:
        {
            PTHREAD_STACK_CONTEXT context = FindThreadStackContext(Control->UniqueKey);
            BOOLEAN isDotNet;

            if (!context)
                return;

            if (!NT_SUCCESS(PhGetProcessIsDotNet(context->ProcessId, &isDotNet)) || !isDotNet)
                return;

            context->Support = CreateClrProcessSupport(context->ProcessId);

#ifdef _WIN64
            if (context->IsWow64)
                context->ConnectedToPhSvc = PhUiConnectToPhSvcEx(NULL, Wow64PhSvcMode, FALSE);
#endif
        }
        break;
    case PluginThreadStackEndDefaultWalkStack:
        {
            PTHREAD_STACK_CONTEXT context = FindThreadStackContext(Control->UniqueKey);

            if (!context)
                return;

            if (context->Support)
            {
                FreeClrProcessSupport(context->Support);
                context->Support = NULL;
            }

#ifdef _WIN64
            if (context->ConnectedToPhSvc)
            {
                PhUiDisconnectFromPhSvc();
                context->ConnectedToPhSvc = FALSE;
            }
#endif
        }
        break;
    }
}

VOID PredictAddressesFromClrData(
    _In_ PCLR_PROCESS_SUPPORT Support,
    _In_ HANDLE ThreadId,
    _In_ PVOID PcAddress,
    _In_ PVOID FrameAddress,
    _In_ PVOID StackAddress,
    _Out_ PVOID *PredictedEip,
    _Out_ PVOID *PredictedEbp,
    _Out_ PVOID *PredictedEsp
    )
{
#ifdef _WIN64
    *PredictedEip = NULL;
    *PredictedEbp = NULL;
    *PredictedEsp = NULL;
#else
    IXCLRDataTask *task;

    *PredictedEip = NULL;
    *PredictedEbp = NULL;
    *PredictedEsp = NULL;

    if (SUCCEEDED(IXCLRDataProcess_GetTaskByOSThreadID(
        Support->DataProcess,
        HandleToUlong(ThreadId),
        &task
        )))
    {
        IXCLRDataStackWalk *stackWalk;

        if (SUCCEEDED(IXCLRDataTask_CreateStackWalk(task, 0xf, &stackWalk)))
        {
            HRESULT result;
            BOOLEAN firstTime = TRUE;
            CONTEXT context;
            ULONG contextSize;

            memset(&context, 0, sizeof(CONTEXT));
            context.ContextFlags = CONTEXT_CONTROL;
            context.Eip = PtrToUlong(PcAddress);
            context.Ebp = PtrToUlong(FrameAddress);
            context.Esp = PtrToUlong(StackAddress);

            result = IXCLRDataStackWalk_SetContext2(stackWalk, CLRDATA_STACK_SET_CURRENT_CONTEXT, sizeof(CONTEXT), (BYTE *)&context);

            if (SUCCEEDED(result = IXCLRDataStackWalk_Next(stackWalk)) && result != S_FALSE)
            {
                if (SUCCEEDED(IXCLRDataStackWalk_GetContext(stackWalk, CONTEXT_CONTROL, sizeof(CONTEXT), &contextSize, (BYTE *)&context)))
                {
                    *PredictedEip = UlongToPtr(context.Eip);
                    *PredictedEbp = UlongToPtr(context.Ebp);
                    *PredictedEsp = UlongToPtr(context.Esp);
                }
            }

            IXCLRDataStackWalk_Release(stackWalk);
        }

        IXCLRDataTask_Release(task);
    }
#endif
}
