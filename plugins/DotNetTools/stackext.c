/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2017-2023
 *
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

            context = PhAllocateZero(sizeof(THREAD_STACK_CONTEXT));
            context->ProcessId = Control->u.Initializing.ProcessId;
            context->ThreadId = Control->u.Initializing.ThreadId;
            context->ThreadHandle = Control->u.Initializing.ThreadHandle;
#if _WIN64
            PhGetProcessIsWow64(Control->u.Initializing.ProcessHandle, &context->IsWow64);
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
            PPH_STRING managedFileName = NULL;
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

                managedFileName = DnGetFileNameByAddressClrProcess(
                    context->Support,
                    (ULONG64)Control->u.ResolveSymbol.StackFrame->PcAddress
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

                managedFileName = CallGetFileNameByAddress(
                    context->ProcessId,
                    (ULONG64)Control->u.ResolveSymbol.StackFrame->PcAddress
                    );
            }
#endif

            if (managedSymbol)
            {
                if (displacement != 0)
                {
                    PH_FORMAT format[3];

                    PhInitFormatSR(&format[0], managedSymbol->sr);
                    PhInitFormatS(&format[1], L" + 0x");
                    PhInitFormatI64X(&format[2], displacement);

                    PhMoveReference(&managedSymbol, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                if (Control->u.ResolveSymbol.Symbol)
                {
                    PH_FORMAT format[3];

                    PhInitFormatSR(&format[0], managedSymbol->sr);
                    PhInitFormatS(&format[1], L" <-- ");
                    PhInitFormatSR(&format[2], Control->u.ResolveSymbol.Symbol->sr);

                    PhMoveReference(&managedSymbol, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                PhMoveReference(&Control->u.ResolveSymbol.Symbol, managedSymbol);
            }

            if (managedFileName)
            {
                PhMoveReference(&Control->u.ResolveSymbol.FileName, managedFileName);
            }
        }
        break;
    case PluginThreadStackBeginDefaultWalkStack:
        {
            PTHREAD_STACK_CONTEXT context = FindThreadStackContext(Control->UniqueKey);
            BOOLEAN isDotNet = FALSE;

            if (!context)
                return;

            if (!NT_SUCCESS(PhGetProcessIsDotNetEx(
                context->ProcessId,
                NULL,
                PH_CLR_USE_SECTION_CHECK,
                &isDotNet,
                NULL
                )) || !isDotNet)
            {
                return;
            }

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
    _In_ struct _CLR_PROCESS_SUPPORT *Support,
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
