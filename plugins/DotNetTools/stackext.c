/*
 * Process Hacker .NET Tools -
 *   thread stack extensions
 *
 * Copyright (C) 2011 wj32
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

typedef struct _THREAD_STACK_CONTEXT
{
    PCLR_PROCESS_SUPPORT Support;
    HANDLE ThreadId;
    HANDLE ThreadHandle;

#ifndef _WIN64
    PVOID PredictedEip;
    PVOID PredictedEbp;
    PVOID PredictedEsp;
#endif
} THREAD_STACK_CONTEXT, *PTHREAD_STACK_CONTEXT;

VOID PredictAddressesFromClrData(
    _In_ PTHREAD_STACK_CONTEXT Context,
    _In_ PPH_THREAD_STACK_FRAME Frame,
    _Out_ PVOID *PredictedEip,
    _Out_ PVOID *PredictedEbp,
    _Out_ PVOID *PredictedEsp
    );

static PPH_HASHTABLE ContextHashtable;
static PH_QUEUED_LOCK ContextHashtableLock = PH_QUEUED_LOCK_INIT;
static PH_INITONCE ContextHashtableInitOnce = PH_INITONCE_INIT;

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
            BOOLEAN isDotNet;

            if (!NT_SUCCESS(PhGetProcessIsDotNet(Control->u.Initializing.ProcessId, &isDotNet)) || !isDotNet)
                return;

            context = PhAllocate(sizeof(THREAD_STACK_CONTEXT));
            memset(context, 0, sizeof(THREAD_STACK_CONTEXT));
            context->Support = CreateClrProcessSupport(Control->u.Initializing.ProcessId);
            context->ThreadId = Control->u.Initializing.ThreadId;
            context->ThreadHandle = Control->u.Initializing.ThreadHandle;

            PhAcquireQueuedLockExclusive(&ContextHashtableLock);
            PhAddItemSimpleHashtable(ContextHashtable, Control->UniqueKey, context);
            PhReleaseQueuedLockExclusive(&ContextHashtableLock);
        }
        break;
    case PluginThreadStackUninitializing:
        {
            PTHREAD_STACK_CONTEXT context;
            PVOID *item;

            PhAcquireQueuedLockExclusive(&ContextHashtableLock);

            item = PhFindItemSimpleHashtable(ContextHashtable, Control->UniqueKey);

            if (item)
                context = *item;
            else
                context = NULL;

            PhReleaseQueuedLockExclusive(&ContextHashtableLock);

            if (!context)
                return;

            if (context->Support)
                FreeClrProcessSupport(context->Support);

            PhFree(context);

            PhAcquireQueuedLockExclusive(&ContextHashtableLock);
            PhRemoveItemSimpleHashtable(ContextHashtable, Control->UniqueKey);
            PhReleaseQueuedLockExclusive(&ContextHashtableLock);
        }
        break;
    case PluginThreadStackResolveSymbol:
        {
            PTHREAD_STACK_CONTEXT context;
            PVOID *item;
            PPH_STRING managedSymbol;
            ULONG64 displacement;

            PhAcquireQueuedLockExclusive(&ContextHashtableLock);

            item = PhFindItemSimpleHashtable(ContextHashtable, Control->UniqueKey);

            if (item)
                context = *item;
            else
                context = NULL;

            PhReleaseQueuedLockExclusive(&ContextHashtableLock);

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
                    context,
                    Control->u.ResolveSymbol.StackFrame,
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

                if (managedSymbol)
                {
                    if (displacement != 0)
                    {
                        PhMoveReference(&managedSymbol, PhFormatString(L"%s + 0x%I64x", managedSymbol->Buffer, displacement));
                    }

                    if (Control->u.ResolveSymbol.Symbol->Buffer)
                        PhMoveReference(&managedSymbol, PhFormatString(L"%s <-- %s", managedSymbol->Buffer, Control->u.ResolveSymbol.Symbol->Buffer));

                    PhMoveReference(&Control->u.ResolveSymbol.Symbol, managedSymbol);
                }
            }
        }
        break;
    }
}

VOID PredictAddressesFromClrData(
    _In_ PTHREAD_STACK_CONTEXT Context,
    _In_ PPH_THREAD_STACK_FRAME Frame,
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
        Context->Support->DataProcess,
        HandleToUlong(Context->ThreadId),
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
            context.Eip = PtrToUlong(Frame->PcAddress);
            context.Ebp = PtrToUlong(Frame->FrameAddress);
            context.Esp = PtrToUlong(Frame->StackAddress);

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
