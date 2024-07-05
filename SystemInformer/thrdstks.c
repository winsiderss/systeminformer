/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023
 *
 */

#include <phapp.h>
#include <settings.h>
#include <colmgr.h>
#include <mainwnd.h>
#include <emenu.h>
#include <symprv.h>
#include <phplug.h>
#include <cpysave.h>
#include <procprv.h>
#include <procprp.h>

#define WM_PH_THREAD_STACKS_PUBLISH    (WM_USER + 2000)

typedef enum _PH_THREAD_STACKS_COLUMN
{
    PH_THREAD_STACKS_COLUMN_SYMBOL,
    PH_THREAD_STACKS_COLUMN_PROCESSID,
    PH_THREAD_STACKS_COLUMN_THREADID,
    PH_THREAD_STACKS_COLUMN_FRAMENUMBER,
    PH_THREAD_STACKS_COLUMN_STACKADDRESS,
    PH_THREAD_STACKS_COLUMN_FRAMEADDRESS,
    PH_THREAD_STACKS_COLUMN_PARAMETER1,
    PH_THREAD_STACKS_COLUMN_PARAMETER2,
    PH_THREAD_STACKS_COLUMN_PARAMETER3,
    PH_THREAD_STACKS_COLUMN_PARAMETER4,
    PH_THREAD_STACKS_COLUMN_CONTROLADDRESS,
    PH_THREAD_STACKS_COLUMN_RETURNADDRESS,
    PH_THREAD_STACKS_COLUMN_FILENAME,
    PH_THREAD_STACKS_COLUMN_LINETEXT,
    PH_THREAD_STACKS_COLUMN_ARCHITECTURE,
    PH_THREAD_STACKS_COLUMN_FRAMEDISTANCE,
    PH_THREAD_STACKS_COLUMN_STARTADDRESS,
    PH_THREAD_STACKS_COLUMN_STARTADDRESSSYMBOL,
    PH_THREAD_STACKS_COLUMN_PROCESSNAME,
    PH_THREAD_STACKS_COLUMN_THREADNAME,
    MAX_PH_THREAD_STACKS_COLUMN,
} PH_THREAD_STACKS_COLUMN;

typedef enum _PH_THREAD_STACKS_NODE_TYPE
{
    PhThreadStacksNodeTypeProcess,
    PhThreadStacksNodeTypeThread,
    PhThreadStacksNodeTypeFrame,
} PH_THREAD_STACKS_NODE_TYPE, *PPH_THREAD_STACKS_NODE_TYPE;

typedef struct _PH_THREAD_STACKS_PROCESS_NODE
{
    HANDLE ProcessId;
    PPH_SYMBOL_PROVIDER SymbolProvider;
    PPH_STRING ProcessName;
    WCHAR ProcessIdString[PH_INT32_STR_LEN_1];
    PPH_STRING FileName;
    PH_STRINGREF Architecture;
    PPH_LIST Threads;
} PH_THREAD_STACKS_PROCESS_NODE, *PPH_THREAD_STACKS_PROCESS_NODE;

typedef struct _PH_THREAD_STACKS_THREAD_NODE
{
    PPH_THREAD_STACKS_PROCESS_NODE Process;

    HANDLE ThreadId;
    HANDLE ThreadHandle;
    ULONG64 StartAddress;
    PPH_STRING ThreadName;
    WCHAR ThreadIdString[PH_INT32_STR_LEN_1];
    WCHAR StartAddressString[PH_PTR_STR_LEN_1];
    PPH_STRING StartAddressSymbol;
    PPH_STRING FileName;
    PPH_LIST Frames;
    BOOLEAN FramesComplete;
} PH_THREAD_STACKS_THREAD_NODE, *PPH_THREAD_STACKS_THREAD_NODE;

typedef struct _PH_THREAD_STACKS_FRAME_NODE
{
    PPH_THREAD_STACKS_THREAD_NODE Thread;

    PH_THREAD_STACK_FRAME StackFrame;
    ULONG FrameNumber;
    ULONG FrameDistance;
    PPH_STRING FileName;
    PPH_STRING LineText;
    WCHAR FrameNumberString[PH_INT32_STR_LEN_1];
    WCHAR StackAddressString[PH_PTR_STR_LEN_1];
    WCHAR FrameAddressString[PH_PTR_STR_LEN_1];
    WCHAR Parameter1String[PH_PTR_STR_LEN_1];
    WCHAR Parameter2String[PH_PTR_STR_LEN_1];
    WCHAR Parameter3String[PH_PTR_STR_LEN_1];
    WCHAR Parameter4String[PH_PTR_STR_LEN_1];
    WCHAR PcAddressString[PH_PTR_STR_LEN_1];
    WCHAR ReturnAddressString[PH_PTR_STR_LEN_1];
    PH_STRINGREF Architecture;
    PPH_STRING FrameDistanceString;
} PH_THREAD_STACKS_FRAME_NODE, *PPH_THREAD_STACKS_FRAME_NODE;

typedef struct _PH_THREAD_STACKS_NODE
{
    PH_TREENEW_NODE Node;

    PH_THREAD_STACKS_NODE_TYPE Type;

    PPH_STRING Symbol;

    union
    {
        PH_THREAD_STACKS_PROCESS_NODE Process;
        PH_THREAD_STACKS_THREAD_NODE Thread;
        PH_THREAD_STACKS_FRAME_NODE Frame;
    };

    PH_STRINGREF TextCache[MAX_PH_THREAD_STACKS_COLUMN];
} PH_THREAD_STACKS_NODE, *PPH_THREAD_STACKS_NODE;

typedef struct _PH_THREAD_STACKS_WORKER_CONTEXT
{
    volatile LONG Stop;
    PVOID Context;
    ULONG TotalThreads;
    ULONG WalkedThreads;
    PPH_LIST NodeList;
    PPH_LIST NodeRootList;
} PH_THREAD_STACKS_WORKER_CONTEXT, *PPH_THREAD_STACKS_WORKER_CONTEXT;

typedef struct _PH_THREAD_STACKS_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND TreeNewHandle;
    HWND MessageHandle;
    HWND SearchWindowHandle;
    ULONG_PTR SearchMatchHandle;

    PH_LAYOUT_MANAGER LayoutManager;
    RECT MinimumSize;

    ULONG MessageCount;
    ULONG LastMessageCount;
    PPH_STRING StatusMessage;
    PPH_STRING SymbolMessage;
    PH_QUEUED_LOCK MessageLock;

    PPH_LIST NodeList;
    PPH_LIST NodeRootList;
    PPH_THREAD_STACKS_WORKER_CONTEXT WorkerContext;
    PH_CALLBACK_REGISTRATION SymbolProviderEventRegistration;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG HideSystemFrames : 1;
            ULONG HideUserFrames : 1;
            ULONG HideInlineFrames : 1;
            ULONG HighlightSystemFrames : 1;
            ULONG HighlightUserFrames : 1;
            ULONG HighlightInlineFrames : 1;
            ULONG Spare : 26;
        };
    };
} PH_THREAD_STACKS_CONTEXT, *PPH_THREAD_STACKS_CONTEXT;

typedef struct _PH_THREAD_STACKS_WALK_CONTEXT
{
    PPH_THREAD_STACKS_WORKER_CONTEXT Context;
    PPH_THREAD_STACKS_THREAD_NODE ThreadNode;
} PH_THREAD_STACKS_WALK_CONTEXT, *PPH_THREAD_STACKS_WALK_CONTEXT;

static PPH_OBJECT_TYPE PhpThreadStacksObjectType = NULL;
static PPH_OBJECT_TYPE PhpThreadStacksWorkerObjectType = NULL;
static PPH_OBJECT_TYPE PhpThreadStacksNodeObjectType = NULL;

const ACCESS_MASK PhpThreadStacksThreadAccessMasks[] =
{
    THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME,
    THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT,
    THREAD_QUERY_LIMITED_INFORMATION,
};

VOID NTAPI PhpThreadStacksNodeDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_THREAD_STACKS_NODE node = Object;

    PhDereferenceObject(node->Symbol);

    switch (node->Type)
    {
    case PhThreadStacksNodeTypeProcess:
        {
            PhClearReference(&node->Process.SymbolProvider);
            PhDereferenceObject(node->Process.ProcessName);
            PhClearReference(&node->Process.FileName);
            PhDereferenceObject(node->Process.Threads);
        }
        break;
    case PhThreadStacksNodeTypeThread:
        {
            if (node->Thread.ThreadHandle)
                NtClose(node->Thread.ThreadHandle);
            PhClearReference(&node->Thread.ThreadName);
            PhDereferenceObject(node->Thread.Frames);
            PhClearReference(&node->Thread.StartAddressSymbol);
            PhClearReference(&node->Thread.FileName);
        }
        break;
    case PhThreadStacksNodeTypeFrame:
        {
            PhClearReference(&node->Frame.FileName);
            PhClearReference(&node->Frame.LineText);
            PhClearReference(&node->Frame.FrameDistanceString);
        }
        break;
    }
}

PPH_THREAD_STACKS_NODE PhpThreadStacksCreateNode(
    _In_ PH_THREAD_STACKS_NODE_TYPE Type
    )
{
    PPH_THREAD_STACKS_NODE node;

    node = PhCreateObjectZero(sizeof(PH_THREAD_STACKS_NODE), PhpThreadStacksNodeObjectType);

    PhInitializeTreeNewNode(&node->Node);

    node->Type = Type;

    memset(node->TextCache, 0, sizeof(PH_STRINGREF) * MAX_PH_THREAD_STACKS_COLUMN);
    node->Node.TextCache = node->TextCache;
    node->Node.TextCacheSize = MAX_PH_THREAD_STACKS_COLUMN;

    return node;
}

VOID NTAPI PhpThreadStacksWorkerContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_THREAD_STACKS_WORKER_CONTEXT context = Object;

    for (ULONG i = 0; i < context->NodeList->Count; i++)
    {
        PhDereferenceObject(context->NodeList->Items[i]);
    }

    PhClearReference(&context->Context);
    PhDereferenceObject(context->NodeList);
    PhDereferenceObject(context->NodeRootList);
}

PPH_THREAD_STACKS_WORKER_CONTEXT PhpThreadStacksCreateWorkerContext(
    _In_ PPH_THREAD_STACKS_CONTEXT Context
    )
{
    PPH_THREAD_STACKS_WORKER_CONTEXT context;

    context = PhCreateObjectZero(sizeof(PH_THREAD_STACKS_WORKER_CONTEXT), PhpThreadStacksWorkerObjectType);

    context->Context = Context;
    PhReferenceObject(context->Context);

    context->NodeList = PhCreateList(500);
    context->NodeRootList = PhCreateList(100);

    return context;
}

VOID NTAPI PhpThreadStacksContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_THREAD_STACKS_CONTEXT context = Object;

    PhClearReference(&context->WorkerContext);

    for (ULONG i = 0; i < context->NodeList->Count; i++)
    {
        PhDereferenceObject(context->NodeList->Items[i]);
    }

    PhDereferenceObject(context->NodeList);
    PhDereferenceObject(context->NodeRootList);
    PhDereferenceObject(context->StatusMessage);
    PhDereferenceObject(context->SymbolMessage);
}

PPH_THREAD_STACKS_CONTEXT PhpThreadStacksCreateContext(
    VOID
    )
{
    PPH_THREAD_STACKS_CONTEXT context;

    context = PhCreateObjectZero(sizeof(PH_THREAD_STACKS_CONTEXT), PhpThreadStacksObjectType);

    context->NodeList = PhCreateList(500);
    context->NodeRootList = PhCreateList(100);

    context->StatusMessage = PhReferenceEmptyString();
    context->SymbolMessage = PhReferenceEmptyString();
    PhInitializeQueuedLock(&context->MessageLock);

    return context;
}

VOID PhpThreadStacksMessage(
    _In_ PPH_THREAD_STACKS_WORKER_CONTEXT Context,
    _In_ __format_string PCWSTR Format,
    ...
    )
{
    PPH_THREAD_STACKS_CONTEXT context = Context->Context;
    PPH_STRING message;
    va_list args;

    if (Context->Stop)
        return;

    va_start(args, Format);
    message = PhFormatString_V((PWSTR)Format, args);
    va_end(args);

    PhAcquireQueuedLockExclusive(&context->MessageLock);
    PhMoveReference(&context->StatusMessage, message);
    context->MessageCount++;
    PhReleaseQueuedLockExclusive(&context->MessageLock);
}

VOID PhpThreadStacksPublish(
    _In_ PPH_THREAD_STACKS_WORKER_CONTEXT Context,
    _In_ BOOLEAN Restructure,
    _In_opt_ PPH_THREAD_STACKS_NODE Node
    )
{
    PPH_THREAD_STACKS_CONTEXT context = Context->Context;

    if (Context->Stop)
        return;

    if (Node)
        PhReferenceObject(Node);

    PostMessage(context->WindowHandle, WM_PH_THREAD_STACKS_PUBLISH, (WPARAM)Restructure, (LPARAM)Node);
}

VOID PhpThreadStacksCreateThreadNode(
    _In_ PPH_THREAD_STACKS_WORKER_CONTEXT Context,
    _In_ PPH_THREAD_STACKS_PROCESS_NODE ProcessNode,
    _In_ PSYSTEM_THREAD_INFORMATION ThreadInfo
    )
{
    PPH_THREAD_STACKS_NODE node;

    node = PhpThreadStacksCreateNode(PhThreadStacksNodeTypeThread);

    PhAddItemList(Context->NodeList, node);
    PhAddItemList(ProcessNode->Threads, node);

    node->Thread.Process = ProcessNode;

    node->Thread.ThreadId = ThreadInfo->ClientId.UniqueThread;
    PhPrintUInt32(node->Thread.ThreadIdString, HandleToUlong(node->Thread.ThreadId));

    for (ULONG i = 0; i < ARRAYSIZE(PhpThreadStacksThreadAccessMasks); i++)
    {
        if (NT_SUCCESS(PhOpenThread(&node->Thread.ThreadHandle, PhpThreadStacksThreadAccessMasks[i], node->Thread.ThreadId)))
            break;
    }

    if (node->Thread.ThreadHandle)
    {
        ULONG_PTR startAddress;

        if (NT_SUCCESS(PhGetThreadStartAddress(node->Thread.ThreadHandle, &startAddress)))
            node->Thread.StartAddress = (ULONG64)startAddress;

        PhGetThreadName(node->Thread.ThreadHandle, &node->Thread.ThreadName);
    }

    if (!node->Thread.StartAddress)
        node->Thread.StartAddress = (ULONG64)ThreadInfo->StartAddress;

    PhPrintPointer(node->Thread.StartAddressString, (PVOID)node->Thread.StartAddress);

    if (!PhIsNullOrEmptyString(node->Thread.ThreadName))
        node->Symbol = PhReferenceObject(node->Thread.ThreadName);
    else
        node->Symbol = PhCreateString(node->Thread.StartAddressString);

    node->Thread.Frames = PhCreateList(10);

    Context->TotalThreads++;

    PhpThreadStacksPublish(Context, FALSE, node);
}

VOID PhpThreadStacksCreateProcessNode(
    _In_ PPH_THREAD_STACKS_WORKER_CONTEXT Context,
    _In_ PSYSTEM_PROCESS_INFORMATION ProcessInfo
    )
{
    PPH_THREAD_STACKS_NODE node;
    HANDLE processHandle;

    node = PhpThreadStacksCreateNode(PhThreadStacksNodeTypeProcess);

    PhAddItemList(Context->NodeList, node);
    PhAddItemList(Context->NodeRootList, node);

    node->Process.ProcessId = ProcessInfo->UniqueProcessId;
    PhPrintUInt32(node->Process.ProcessIdString, HandleToUlong(node->Process.ProcessId));

    node->Process.SymbolProvider = PhCreateSymbolProvider(node->Process.ProcessId);

    PhLoadSymbolProviderOptions(node->Process.SymbolProvider);

    if (node->Process.ProcessId != SYSTEM_IDLE_PROCESS_ID)
        node->Process.ProcessName = PhCreateStringFromUnicodeString(&ProcessInfo->ImageName);
    else
        node->Process.ProcessName = PhCreateStringFromUnicodeString(&SYSTEM_IDLE_PROCESS_NAME);

    node->Symbol = PhReferenceObject(node->Process.ProcessName);

    if (node->Process.ProcessId == SYSTEM_PROCESS_ID)
        node->Process.FileName = PhGetKernelFileName2();

    if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, node->Process.ProcessId)))
    {
        USHORT machine;

        if (!node->Process.FileName)
            PhGetProcessImageFileName(processHandle, &node->Process.FileName);

        if (NT_SUCCESS(PhGetProcessArchitecture(processHandle, &machine)))
        {
            switch (machine)
            {
            case IMAGE_FILE_MACHINE_I386:
                PhInitializeStringRef(&node->Process.Architecture, L"x86");
                break;
            case IMAGE_FILE_MACHINE_AMD64:
                PhInitializeStringRef(&node->Process.Architecture, L"x64");
                break;
            case IMAGE_FILE_MACHINE_ARMNT:
                PhInitializeStringRef(&node->Process.Architecture, L"ARM");
                break;
            case IMAGE_FILE_MACHINE_ARM64:
                PhInitializeStringRef(&node->Process.Architecture, L"ARM64");
                break;
            default:
                PhInitializeEmptyStringRef(&node->Process.Architecture);
                break;
            }
        }
        else
        {
            PhInitializeEmptyStringRef(&node->Process.Architecture);
        }

        NtClose(processHandle);
    }

    node->Process.Threads = PhCreateList(ProcessInfo->NumberOfThreads);
    for (ULONG i = 0; i < ProcessInfo->NumberOfThreads; i++)
    {
        PSYSTEM_THREAD_INFORMATION threadInfo;

        threadInfo = &ProcessInfo->Threads[i];

        PhpThreadStacksCreateThreadNode(Context, &node->Process, threadInfo);
    }

    PhpThreadStacksPublish(Context, FALSE, node);
}

PPH_STRING PhpThreadStacksInitFrameNode(
    _In_ PPH_THREAD_STACKS_WORKER_CONTEXT Context,
    _In_ PPH_THREAD_STACKS_FRAME_NODE FrameNode,
    _In_ PPH_THREAD_STACK_FRAME StackFrame
    )
{
    PPH_STRING symbol = NULL;
    PPH_STRING fileName = NULL;
    PPH_STRING lineText = NULL;
    ULONG64 baseAddress = 0;
    PPH_STRING lineFileName;
    PH_SYMBOL_LINE_INFORMATION lineInfo;

    if (PhSymbolProviderInlineContextSupported())
    {
        symbol = PhGetSymbolFromInlineContext(
            FrameNode->Thread->Process->SymbolProvider,
            StackFrame,
            NULL,
            &fileName,
            NULL,
            NULL,
            &baseAddress
            );

        if (PhPluginsEnabled)
        {
            PH_PLUGIN_THREAD_STACK_CONTROL control;

            control.Type = PluginThreadStackResolveSymbol;
            control.UniqueKey = Context;
            control.u.ResolveSymbol.StackFrame = StackFrame;
            control.u.ResolveSymbol.Symbol = symbol;
            control.u.ResolveSymbol.FileName = fileName;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

            symbol = control.u.ResolveSymbol.Symbol;
            fileName = control.u.ResolveSymbol.FileName;
        }

        if (PhGetLineFromInlineContext(
            FrameNode->Thread->Process->SymbolProvider,
            StackFrame,
            baseAddress,
            &lineFileName,
            NULL,
            &lineInfo
            ))
        {
            PH_FORMAT format[3];

            PhInitFormatSR(&format[0], lineFileName->sr);
            PhInitFormatS(&format[1], L" @ ");
            PhInitFormatU(&format[2], lineInfo.LineNumber);

            lineText = PhFormat(format, RTL_NUMBER_OF(format), 0);
            PhDereferenceObject(lineFileName);
        }
    }
    else
    {
        symbol = PhGetSymbolFromAddress(
            FrameNode->Thread->Process->SymbolProvider,
            (ULONG64)StackFrame->PcAddress,
            NULL,
            &fileName,
            NULL,
            NULL
            );

        if (PhPluginsEnabled)
        {
            PH_PLUGIN_THREAD_STACK_CONTROL control;

            control.Type = PluginThreadStackResolveSymbol;
            control.UniqueKey = Context;
            control.u.ResolveSymbol.StackFrame = StackFrame;
            control.u.ResolveSymbol.Symbol = symbol;
            control.u.ResolveSymbol.FileName = fileName;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

            symbol = control.u.ResolveSymbol.Symbol;
            fileName = control.u.ResolveSymbol.FileName;
        }

        if (PhGetLineFromAddress(
            FrameNode->Thread->Process->SymbolProvider,
            (ULONG64)StackFrame->PcAddress,
            &lineFileName,
            NULL,
            &lineInfo
            ))
        {
            PH_FORMAT format[3];

            PhInitFormatSR(&format[0], lineFileName->sr);
            PhInitFormatS(&format[1], L" @ ");
            PhInitFormatU(&format[2], lineInfo.LineNumber);

            lineText = PhFormat(format, RTL_NUMBER_OF(format), 0);
            PhDereferenceObject(lineFileName);
        }
    }

    FrameNode->StackFrame = *StackFrame;

    if (symbol &&
        (FrameNode->StackFrame.Machine == IMAGE_FILE_MACHINE_I386) &&
        FlagOn(FrameNode->StackFrame.Flags, PH_THREAD_STACK_FRAME_FPO_DATA_PRESENT))
    {
        PhMoveReference(&symbol, PhConcatStringRefZ(&symbol->sr, L" (No unwind info)"));
    }

    if (PhSymbolProviderInlineContextSupported() &&
        PhIsStackFrameTypeInline(FrameNode->StackFrame.InlineFrameContext))
    {
        if (symbol)
            PhMoveReference(&symbol, PhConcatStringRefZ(&symbol->sr, L" (Inline Function)"));

        // Zero inline frames so the stack matches windbg output.
        FrameNode->StackFrame.PcAddress = NULL;
        FrameNode->StackFrame.ReturnAddress = NULL;
        FrameNode->StackFrame.FrameAddress = NULL;
        FrameNode->StackFrame.StackAddress = NULL;
        memset(FrameNode->StackFrame.Params, 0, sizeof(FrameNode->StackFrame.Params));
    }

    if (FrameNode->StackFrame.StackAddress)
    {
        PhPrintPointer(FrameNode->StackAddressString, (PVOID)FrameNode->StackFrame.StackAddress);

        if (FrameNode->FrameNumber > 0)
        {
            PPH_THREAD_STACKS_NODE prev = FrameNode->Thread->Frames->Items[FrameNode->FrameNumber - 1];

            if (prev->Frame.StackFrame.StackAddress)
            {
                FrameNode->FrameDistance = (ULONG)((ULONG_PTR)FrameNode->StackFrame.StackAddress - (ULONG_PTR)prev->Frame.StackFrame.StackAddress);
                FrameNode->FrameDistanceString = PhFormatSize(FrameNode->FrameDistance, ULONG_MAX);
            }
        }
    }

    // There are no parameters for kernel-mode frames.
    if ((ULONG_PTR)FrameNode->StackFrame.PcAddress <= PhSystemBasicInformation.MaximumUserModeAddress)
    {
        PhPrintPointer(FrameNode->Parameter1String, FrameNode->StackFrame.Params[0]);
        PhPrintPointer(FrameNode->Parameter2String, FrameNode->StackFrame.Params[1]);
        PhPrintPointer(FrameNode->Parameter3String, FrameNode->StackFrame.Params[2]);
        PhPrintPointer(FrameNode->Parameter4String, FrameNode->StackFrame.Params[3]);
    }

    if (FrameNode->StackFrame.PcAddress)
        PhPrintPointer(FrameNode->PcAddressString, (PVOID)FrameNode->StackFrame.PcAddress);

    if (FrameNode->StackFrame.ReturnAddress)
        PhPrintPointer(FrameNode->ReturnAddressString, (PVOID)FrameNode->StackFrame.ReturnAddress);

    switch (FrameNode->StackFrame.Machine)
    {
    case IMAGE_FILE_MACHINE_ARM64EC:
        PhInitializeStringRef(&FrameNode->Architecture, L"ARM64EC");
        break;
    case IMAGE_FILE_MACHINE_CHPE_X86:
        PhInitializeStringRef(&FrameNode->Architecture, L"CHPE");
        break;
    case IMAGE_FILE_MACHINE_ARM64:
        PhInitializeStringRef(&FrameNode->Architecture, L"ARM64");
        break;
    case IMAGE_FILE_MACHINE_ARM:
        PhInitializeStringRef(&FrameNode->Architecture, L"ARM");
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        PhInitializeStringRef(&FrameNode->Architecture, L"x64");
        break;
    case IMAGE_FILE_MACHINE_I386:
        PhInitializeStringRef(&FrameNode->Architecture, L"x86");
        break;
    default:
        PhInitializeStringRef(&FrameNode->Architecture, L"");
        break;
    }

    PhMoveReference(&FrameNode->FileName, fileName);
    PhMoveReference(&FrameNode->LineText, lineText);

    return symbol;
}

VOID PhpThreadStacksCreateFrameNode(
    _In_ PPH_THREAD_STACKS_WORKER_CONTEXT Context,
    _In_ PPH_THREAD_STACKS_THREAD_NODE ThreadNode,
    _In_ PPH_THREAD_STACK_FRAME StackFrame
    )
{
    PPH_THREAD_STACKS_NODE node;

    node = PhpThreadStacksCreateNode(PhThreadStacksNodeTypeFrame);

    // frame nodes will become visible during publishing
    node->Node.Visible = FALSE;

    node->Frame.Thread = ThreadNode;
    node->Frame.FrameNumber = ThreadNode->Frames->Count;

    PhAddItemList(Context->NodeList, node);
    PhAddItemList(ThreadNode->Frames, node);

    PhPrintUInt32(node->Frame.FrameNumberString, node->Frame.FrameNumber);

    node->Symbol = PhpThreadStacksInitFrameNode(Context, &node->Frame, StackFrame);

    PhpThreadStacksPublish(Context, FALSE, node);
}

VOID PhpThreadStacksWorkerPhase1(
    _In_ PPH_THREAD_STACKS_WORKER_CONTEXT Context
    )
{
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;

    if (!NT_SUCCESS(PhEnumProcesses(&processes)))
        return;

    process = PH_FIRST_PROCESS(processes);

    PhpThreadStacksMessage(Context, L"Enumerating processes...");

    do
    {
        PhpThreadStacksCreateProcessNode(Context, process);
    } while (process = PH_NEXT_PROCESS(process));

    PhFree(processes);

    PhpThreadStacksPublish(Context, TRUE, NULL);
}

BOOLEAN NTAPI PhpThreadStacksWalkCallback(
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _In_ PVOID Context
    )
{
    PPH_THREAD_STACKS_WALK_CONTEXT context = Context;

    if (context->Context->Stop)
        return FALSE;

    PhpThreadStacksCreateFrameNode(context->Context, context->ThreadNode, StackFrame);

    return TRUE;
}

VOID PhpThreadStacksThreadPhase2(
    _In_ PPH_THREAD_STACKS_WORKER_CONTEXT Context,
    _In_ PPH_THREAD_STACKS_THREAD_NODE ThreadNode
    )
{
    PH_THREAD_STACKS_WALK_CONTEXT context;
    CLIENT_ID clientId;

    if (!ThreadNode->ThreadHandle)
        return;

    clientId.UniqueProcess = ThreadNode->Process->ProcessId;
    clientId.UniqueThread = ThreadNode->ThreadId;

    context.Context = Context;
    context.ThreadNode = ThreadNode;

    PhWalkThreadStack(
        ThreadNode->ThreadHandle,
        ThreadNode->Process->SymbolProvider->ProcessHandle,
        &clientId,
        ThreadNode->Process->SymbolProvider,
        PH_WALK_USER_WOW64_STACK | PH_WALK_USER_STACK | PH_WALK_KERNEL_STACK,
        PhpThreadStacksWalkCallback,
        &context
        );
}

VOID PhpThreadStacksProcessPhase2(
    _In_ PPH_THREAD_STACKS_WORKER_CONTEXT Context,
    _In_ PPH_THREAD_STACKS_PROCESS_NODE ProcessNode
    )
{
    for (ULONG i = 0; i < ProcessNode->Threads->Count; i++)
    {
        PPH_THREAD_STACKS_NODE node = ProcessNode->Threads->Items[i];

        PhpThreadStacksMessage(
            Context,
            L"Walking stacks... %lu%% - %ls (%lu) %lu%%",
            (ULONG)(((FLOAT)Context->WalkedThreads / Context->TotalThreads) * 100),
            ProcessNode->ProcessName->Buffer,
            HandleToUlong(ProcessNode->ProcessId),
            (ULONG)(((FLOAT)(i + 1) / ProcessNode->Threads->Count) * 100)
            );

        // Resolve the start address symbol.
        node->Thread.StartAddressSymbol = PhGetSymbolFromAddress(
            ProcessNode->SymbolProvider,
            node->Thread.StartAddress,
            NULL,
            &node->Thread.FileName,
            NULL,
            NULL
            );

        if (node->Symbol == node->Thread.ThreadName)
        {
            // Keep the thread name as the symbol.
            NOTHING;
        }
        else if (node->Thread.StartAddressSymbol)
        {
            PhMoveReference(&node->Symbol, PhReferenceObject(node->Thread.StartAddressSymbol));
        }

        PhpThreadStacksThreadPhase2(Context, &node->Thread);

        Context->WalkedThreads++;
        node->Thread.FramesComplete = TRUE;

        PhpThreadStacksPublish(Context, TRUE, NULL);

        if (Context->Stop)
            break;
    }
}

VOID PhpThreadStacksWorkerPhase2(
    _In_ PPH_THREAD_STACKS_WORKER_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeRootList->Count; i++)
    {
        PPH_THREAD_STACKS_NODE node = Context->NodeRootList->Items[i];

        PhpThreadStacksMessage(
            Context,
            L"Walking stacks... %lu%% - %ls (%lu)",
            (ULONG)(((FLOAT)Context->WalkedThreads / Context->TotalThreads) * 100),
            node->Process.ProcessName->Buffer,
            HandleToUlong(node->Process.ProcessId)
            );

        PhLoadSymbolProviderModules(node->Process.SymbolProvider, node->Process.ProcessId);

        PhpThreadStacksProcessPhase2(Context, &node->Process);

        // Clean up the symbol provider as soon as possible.
        PhClearReference(&node->Process.SymbolProvider);

        if (Context->Stop)
            break;
    }
}

NTSTATUS PhpThreadStacksWorkerThreadStart(
    _In_ PVOID Parameter
    )
{
    PPH_THREAD_STACKS_WORKER_CONTEXT context = Parameter;
    PH_PLUGIN_THREAD_STACK_CONTROL control;

    control.UniqueKey = context;

    if (PhPluginsEnabled)
    {
        control.Type = PluginThreadStackBeginDefaultWalkStack;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
    }

    // Phase 1 enumerates processes and threads
    PhpThreadStacksWorkerPhase1(context);

    // Phase 2 walks stacks, resolves symbols, and creates frame nodes
    PhpThreadStacksWorkerPhase2(context);

    if (PhPluginsEnabled)
    {
        control.Type = PluginThreadStackEndDefaultWalkStack;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
    }

    PhpThreadStacksMessage(context, L"");

    PhClearReference(&context->Context);

    PhDereferenceObject(context);

    return STATUS_SUCCESS;
}

VOID PhpThreadStacksRefresh(
    _In_ PPH_THREAD_STACKS_CONTEXT Context
    )
{
    PPH_THREAD_STACKS_WORKER_CONTEXT context;

    if (Context->WorkerContext)
        InterlockedIncrement(&Context->WorkerContext->Stop);

    context = PhpThreadStacksCreateWorkerContext(Context);

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PhDereferenceObject(Context->NodeList->Items[i]);

    PhClearList(Context->NodeList);
    PhClearList(Context->NodeRootList);

    PhMoveReference(&Context->WorkerContext, PhReferenceObject(context));

    if (!NT_SUCCESS(PhCreateThread2(PhpThreadStacksWorkerThreadStart, context)))
        PhDereferenceObject(context);

    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhpThreadStacksExpandNodes(
    _In_ PPH_THREAD_STACKS_CONTEXT Context,
    _In_ BOOLEAN Expand
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_THREAD_STACKS_NODE node = Context->NodeList->Items[i];

        node->Node.Expanded = Expand;
    }

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhpThreadStacksInvalidateNodes(
    _In_ PPH_THREAD_STACKS_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_THREAD_STACKS_NODE node = Context->NodeList->Items[i];

        PhInvalidateTreeNewNode(&node->Node, TN_CACHE_COLOR);
        TreeNew_InvalidateNode(Context->TreeNewHandle, &node->Node);
    }
}

BOOLEAN PhpThreadStackFrameVisible(
    _In_ PPH_THREAD_STACKS_CONTEXT Context,
    _In_ PPH_THREAD_STACKS_FRAME_NODE FrameNode,
    _In_ BOOLEAN Matched
    )
{
    if (Context->HideSystemFrames && (ULONG_PTR)FrameNode->StackFrame.PcAddress > PhSystemBasicInformation.MaximumUserModeAddress)
        return FALSE;
    if (Context->HideUserFrames && (ULONG_PTR)FrameNode->StackFrame.PcAddress <= PhSystemBasicInformation.MaximumUserModeAddress)
        return FALSE;
    if (Context->HideInlineFrames && PhIsStackFrameTypeInline(FrameNode->StackFrame.InlineFrameContext))
        return FALSE;

    return Matched;
}

VOID PhpThreadStacksRestructureNodesWithSearch(
    _In_ PPH_THREAD_STACKS_CONTEXT Context
    )
{
    if (!Context->SearchMatchHandle)
    {
        for (ULONG i = 0; i < Context->NodeList->Count; i++)
        {
            PPH_THREAD_STACKS_NODE node = Context->NodeList->Items[i];

            if (node->Type == PhThreadStacksNodeTypeFrame)
                node->Node.Visible = PhpThreadStackFrameVisible(Context, &node->Frame, TRUE);
            else
                node->Node.Visible = TRUE;
        }

        TreeNew_NodesStructured(Context->TreeNewHandle);
        return;
    }

    // We only search for symbols in stack frames when searching, if any frame
    // matches the entire ancestry related to the node is made visible.
    for (ULONG i = 0; i < Context->NodeRootList->Count; i++)
    {
        PPH_THREAD_STACKS_NODE processNode = Context->NodeRootList->Items[i];

        processNode->Node.Visible = FALSE;

        for (ULONG j = 0; j < processNode->Process.Threads->Count; j++)
        {
            PPH_THREAD_STACKS_NODE threadNode = processNode->Process.Threads->Items[j];
            BOOLEAN match;

            threadNode->Node.Visible = FALSE;

            if (!threadNode->Thread.FramesComplete)
            {
                continue;
            }

            match = FALSE;

            for (ULONG k = 0; k < threadNode->Thread.Frames->Count; k++)
            {
                PPH_THREAD_STACKS_NODE frameNode = threadNode->Thread.Frames->Items[k];

                frameNode->Node.Visible = PhpThreadStackFrameVisible(Context, &frameNode->Frame, match);

                if (match || !PhSearchControlMatch(Context->SearchMatchHandle, &frameNode->Symbol->sr))
                    continue;

                match = TRUE;

                threadNode->Node.Visible = TRUE;
                processNode->Node.Visible = TRUE;

                for (ULONG l = 0; l <= k; l++)
                {
                    PPH_THREAD_STACKS_NODE prevFrameNode = threadNode->Thread.Frames->Items[l];
                    prevFrameNode->Node.Visible = PhpThreadStackFrameVisible(Context, &frameNode->Frame, TRUE);
                }
            }
        }
    }

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID NTAPI PhpThreadStacksSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPH_THREAD_STACKS_CONTEXT context = Context;

    assert(context);

    context->SearchMatchHandle = MatchHandle;

    PhpThreadStacksRestructureNodesWithSearch(context);
}

BOOLEAN NTAPI PhpThreadStacksTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPH_THREAD_STACKS_CONTEXT context = Context;
    PPH_THREAD_STACKS_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPH_THREAD_STACKS_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                getChildren->Children = (PPH_TREENEW_NODE*)context->NodeRootList->Items;
                getChildren->NumberOfChildren = context->NodeRootList->Count;
            }
            else
            {
                switch (node->Type)
                {
                case PhThreadStacksNodeTypeProcess:
                    getChildren->Children = (PPH_TREENEW_NODE*)node->Process.Threads->Items;
                    getChildren->NumberOfChildren = node->Process.Threads->Count;
                    break;
                case PhThreadStacksNodeTypeThread:
                    getChildren->Children = (PPH_TREENEW_NODE*)node->Thread.Frames->Items;
                    getChildren->NumberOfChildren = node->Thread.Frames->Count;
                    break;
                case PhThreadStacksNodeTypeFrame:
                default:
                    getChildren->Children = NULL;
                    getChildren->NumberOfChildren = 0;
                    break;
                }
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;
            node = (PPH_THREAD_STACKS_NODE)isLeaf->Node;

            isLeaf->IsLeaf = node->Type == PhThreadStacksNodeTypeFrame;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            node = (PPH_THREAD_STACKS_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PH_THREAD_STACKS_COLUMN_SYMBOL:
                getCellText->Text = PhGetStringRef(node->Symbol);
                break;
            case PH_THREAD_STACKS_COLUMN_PROCESSID:
                {
                    switch (node->Type)
                    {
                    case PhThreadStacksNodeTypeProcess:
                        PhInitializeStringRefLongHint(&getCellText->Text, node->Process.ProcessIdString);
                        break;
                    case PhThreadStacksNodeTypeThread:
                        PhInitializeStringRefLongHint(&getCellText->Text, node->Thread.Process->ProcessIdString);
                        break;
                    case PhThreadStacksNodeTypeFrame:
                        PhInitializeStringRefLongHint(&getCellText->Text, node->Frame.Thread->Process->ProcessIdString);
                        break;
                    }
                }
                break;
            case PH_THREAD_STACKS_COLUMN_THREADID:
                {
                    switch (node->Type)
                    {
                    case PhThreadStacksNodeTypeThread:
                        PhInitializeStringRefLongHint(&getCellText->Text, node->Thread.ThreadIdString);
                        break;
                    case PhThreadStacksNodeTypeFrame:
                        PhInitializeStringRefLongHint(&getCellText->Text, node->Frame.Thread->ThreadIdString);
                        break;
                    }
                }
                break;
            case PH_THREAD_STACKS_COLUMN_FRAMENUMBER:
                if (node->Type == PhThreadStacksNodeTypeFrame)
                    PhInitializeStringRefLongHint(&getCellText->Text, node->Frame.FrameNumberString);
                break;
            case PH_THREAD_STACKS_COLUMN_STACKADDRESS:
                if (node->Type == PhThreadStacksNodeTypeFrame)
                    PhInitializeStringRefLongHint(&getCellText->Text, node->Frame.StackAddressString);
                break;
            case PH_THREAD_STACKS_COLUMN_FRAMEADDRESS:
                if (node->Type == PhThreadStacksNodeTypeFrame)
                    PhInitializeStringRefLongHint(&getCellText->Text, node->Frame.FrameAddressString);
                break;
            case PH_THREAD_STACKS_COLUMN_PARAMETER1:
                if (node->Type == PhThreadStacksNodeTypeFrame)
                    PhInitializeStringRefLongHint(&getCellText->Text, node->Frame.Parameter1String);
                break;
            case PH_THREAD_STACKS_COLUMN_PARAMETER2:
                if (node->Type == PhThreadStacksNodeTypeFrame)
                    PhInitializeStringRefLongHint(&getCellText->Text, node->Frame.Parameter2String);
                break;
            case PH_THREAD_STACKS_COLUMN_PARAMETER3:
                if (node->Type == PhThreadStacksNodeTypeFrame)
                    PhInitializeStringRefLongHint(&getCellText->Text, node->Frame.Parameter3String);
                break;
            case PH_THREAD_STACKS_COLUMN_PARAMETER4:
                if (node->Type == PhThreadStacksNodeTypeFrame)
                    PhInitializeStringRefLongHint(&getCellText->Text, node->Frame.Parameter4String);
                break;
            case PH_THREAD_STACKS_COLUMN_CONTROLADDRESS:
                if (node->Type == PhThreadStacksNodeTypeFrame)
                    PhInitializeStringRefLongHint(&getCellText->Text, node->Frame.PcAddressString);
                break;
            case PH_THREAD_STACKS_COLUMN_RETURNADDRESS:
                if (node->Type == PhThreadStacksNodeTypeFrame)
                    PhInitializeStringRefLongHint(&getCellText->Text, node->Frame.ReturnAddressString);
                break;
            case PH_THREAD_STACKS_COLUMN_FILENAME:
                {
                    switch (node->Type)
                    {
                    case PhThreadStacksNodeTypeProcess:
                        getCellText->Text = PhGetStringRef(node->Process.FileName);
                        break;
                    case PhThreadStacksNodeTypeThread:
                        getCellText->Text = PhGetStringRef(node->Thread.FileName);
                        break;
                    case PhThreadStacksNodeTypeFrame:
                        getCellText->Text = PhGetStringRef(node->Frame.FileName);
                        break;
                    }
                }
                break;
            case PH_THREAD_STACKS_COLUMN_LINETEXT:
                if (node->Type == PhThreadStacksNodeTypeFrame)
                    getCellText->Text = PhGetStringRef(node->Frame.LineText);
                break;
            case PH_THREAD_STACKS_COLUMN_ARCHITECTURE:
                {
                    switch (node->Type)
                    {
                    case PhThreadStacksNodeTypeProcess:
                        getCellText->Text = node->Process.Architecture;
                        break;
                    case PhThreadStacksNodeTypeFrame:
                        getCellText->Text = node->Frame.Architecture;
                        break;
                    }
                }
                break;
            case PH_THREAD_STACKS_COLUMN_FRAMEDISTANCE:
                if (node->Type == PhThreadStacksNodeTypeFrame)
                    getCellText->Text = PhGetStringRef(node->Frame.FrameDistanceString);
                break;
            case PH_THREAD_STACKS_COLUMN_STARTADDRESS:
                {
                    switch (node->Type)
                    {
                    case PhThreadStacksNodeTypeThread:
                        PhInitializeStringRefLongHint(&getCellText->Text, node->Thread.StartAddressString);
                        break;
                    case PhThreadStacksNodeTypeFrame:
                        PhInitializeStringRefLongHint(&getCellText->Text, node->Frame.Thread->StartAddressString);
                        break;
                    }
                }
                break;
            case PH_THREAD_STACKS_COLUMN_STARTADDRESSSYMBOL:
                {
                    switch (node->Type)
                    {
                    case PhThreadStacksNodeTypeThread:
                        getCellText->Text = PhGetStringRef(node->Thread.StartAddressSymbol);
                        break;
                    case PhThreadStacksNodeTypeFrame:
                        getCellText->Text = PhGetStringRef(node->Frame.Thread->StartAddressSymbol);
                        break;
                    }
                }
                break;
            case PH_THREAD_STACKS_COLUMN_PROCESSNAME:
                {
                    switch (node->Type)
                    {
                    case PhThreadStacksNodeTypeProcess:
                        getCellText->Text = PhGetStringRef(node->Process.ProcessName);
                        break;
                    case PhThreadStacksNodeTypeThread:
                        getCellText->Text = PhGetStringRef(node->Thread.Process->ProcessName);
                        break;
                    case PhThreadStacksNodeTypeFrame:
                        getCellText->Text = PhGetStringRef(node->Frame.Thread->Process->ProcessName);
                        break;
                    }
                }
                break;
            case PH_THREAD_STACKS_COLUMN_THREADNAME:
                {
                    switch (node->Type)
                    {
                    case PhThreadStacksNodeTypeThread:
                        getCellText->Text = PhGetStringRef(node->Thread.ThreadName);
                        break;
                    case PhThreadStacksNodeTypeFrame:
                        getCellText->Text = PhGetStringRef(node->Frame.Thread->ThreadName);
                        break;
                    }
                }
                break;
            default:
                return FALSE;
            }

            switch (node->Type)
                {
                case PhThreadStacksNodeTypeProcess:
                    getCellText->Flags = TN_CACHE;
                    break;
                case PhThreadStacksNodeTypeThread:
                    if (node->Thread.FramesComplete)
                        getCellText->Flags = TN_CACHE;
                    break;
                case PhThreadStacksNodeTypeFrame:
                    if (node->Frame.Thread->FramesComplete)
                        getCellText->Flags = TN_CACHE;
                    break;
                }
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;
            node = (PPH_THREAD_STACKS_NODE)getNodeColor->Node;

            if (node->Type == PhThreadStacksNodeTypeFrame)
            {
                if (context->HighlightInlineFrames && PhIsStackFrameTypeInline(node->Frame.StackFrame.InlineFrameContext))
                    getNodeColor->BackColor = PhGetIntegerSetting(L"ColorInlineThreadStack");
                else if (context->HighlightSystemFrames && (ULONG_PTR)node->Frame.StackFrame.PcAddress > PhSystemBasicInformation.MaximumUserModeAddress)
                    getNodeColor->BackColor = PhGetIntegerSetting(L"ColorSystemThreadStack");
                else if (context->HighlightUserFrames && (ULONG_PTR)node->Frame.StackFrame.PcAddress <= PhSystemBasicInformation.MaximumUserModeAddress)
                    getNodeColor->BackColor = PhGetIntegerSetting(L"ColorUserThreadStack");

                getNodeColor->Flags = TN_AUTO_FORECOLOR;
            }
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            PhInitializeTreeNewColumnMenuEx(&data, 0);

            data.Selection = PhShowEMenu(
                data.Menu,
                hwnd,
                PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP,
                data.MouseEvent->ScreenLocation.x,
                data.MouseEvent->ScreenLocation.y
                );

            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;
            PPH_EMENU menu;
            PPH_EMENU_ITEM selectedItem;
            PPH_EMENU_ITEM gotoProcess;
            PPH_EMENU_ITEM gotoThread;
            PPH_EMENU_ITEM hideUserFrames;
            PPH_EMENU_ITEM hideSystemFrames;
            PPH_EMENU_ITEM hideInlineFrames;
            PPH_EMENU_ITEM highlightUserFrames;
            PPH_EMENU_ITEM highlightSystemFrames;
            PPH_EMENU_ITEM highlightInlineFrames;

            node = (PPH_THREAD_STACKS_NODE)contextMenuEvent->Node;

            menu = PhCreateEMenu();
            PhInsertEMenuItem(menu, gotoProcess = PhCreateEMenuItem(0, 1, L"Go to process...", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, gotoThread = PhCreateEMenuItem(0, 2, L"Go to thread...", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, hideUserFrames = PhCreateEMenuItem(0, 3, L"Expand all", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, hideUserFrames = PhCreateEMenuItem(0, 4, L"Collapse all", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, hideUserFrames = PhCreateEMenuItem(0, 5, L"Hide user frames", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, hideSystemFrames = PhCreateEMenuItem(0, 6, L"Hide system frames", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, hideInlineFrames = PhCreateEMenuItem(0, 7, L"Hide inline frames", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, highlightUserFrames = PhCreateEMenuItem(0, 8, L"Highlight user frames", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, highlightSystemFrames = PhCreateEMenuItem(0, 9, L"Highlight system frames", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, highlightInlineFrames = PhCreateEMenuItem(0, 10, L"Highlight inline frames", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 100, L"Copy", NULL, NULL), ULONG_MAX);
            PhInsertCopyCellEMenuItem(menu, 100, hwnd, contextMenuEvent->Column);

            if (node)
            {
                if (node->Type == PhThreadStacksNodeTypeProcess)
                    PhSetDisabledEMenuItem(gotoThread);
            }
            else
            {
                PhSetDisabledEMenuItem(gotoProcess);
                PhSetDisabledEMenuItem(gotoThread);
            }

            if (context->HideUserFrames)
                hideUserFrames->Flags |= PH_EMENU_CHECKED;
            if (context->HideSystemFrames)
                hideSystemFrames->Flags |= PH_EMENU_CHECKED;
            if (context->HideInlineFrames)
                hideInlineFrames->Flags |= PH_EMENU_CHECKED;
            if (context->HighlightUserFrames)
                highlightUserFrames->Flags |= PH_EMENU_CHECKED;
            if (context->HighlightSystemFrames)
                highlightSystemFrames->Flags |= PH_EMENU_CHECKED;
            if (context->HighlightInlineFrames)
                highlightInlineFrames->Flags |= PH_EMENU_CHECKED;

            selectedItem = PhShowEMenu(
                menu,
                context->WindowHandle,
                PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP,
                contextMenuEvent->Location.x,
                contextMenuEvent->Location.y
                );

            if (selectedItem && selectedItem->Id != ULONG_MAX && !PhHandleCopyCellEMenuItem(selectedItem))
            {
                switch (selectedItem->Id)
                {
                case 1:
                    {
                        PPH_PROCESS_ITEM processItem;
                        HANDLE processId;

                        assert(node);

                        switch (node->Type)
                        {
                        case PhThreadStacksNodeTypeProcess:
                            processId = node->Process.ProcessId;
                            break;
                        case PhThreadStacksNodeTypeThread:
                            processId = node->Thread.Process->ProcessId;
                            break;
                        case PhThreadStacksNodeTypeFrame:
                            processId = node->Frame.Thread->Process->ProcessId;
                            break;
                        DEFAULT_UNREACHABLE;
                        }

                        if (processItem = PhReferenceProcessItem(processId))
                        {
                            PPH_PROCESS_PROPCONTEXT propContext;

                            if (propContext = PhCreateProcessPropContext(PhMainWndHandle, processItem))
                            {
                                PhShowProcessProperties(propContext);
                                PhDereferenceObject(propContext);
                            }

                            PhDereferenceObject(processItem);
                        }
                    }
                    break;
                case 2:
                    {
                        PPH_PROCESS_ITEM processItem;
                        HANDLE processId;
                        HANDLE threadId;

                        assert(node);
                        assert(node->Type == PhThreadStacksNodeTypeThread ||
                               node->Type == PhThreadStacksNodeTypeFrame);

                        switch (node->Type)
                        {
                        case PhThreadStacksNodeTypeThread:
                            processId = node->Thread.Process->ProcessId;
                            threadId = node->Thread.ThreadId;
                            break;
                        case PhThreadStacksNodeTypeFrame:
                            processId = node->Frame.Thread->Process->ProcessId;
                            threadId = node->Frame.Thread->ThreadId;
                            break;
                        DEFAULT_UNREACHABLE;
                        }

                        if (processItem = PhReferenceProcessItem(processId))
                        {
                            PPH_PROCESS_PROPCONTEXT propContext;

                            if (propContext = PhCreateProcessPropContext(PhMainWndHandle, processItem))
                            {
                                PhSetSelectThreadIdProcessPropContext(propContext, threadId);
                                PhShowProcessProperties(propContext);
                                PhDereferenceObject(propContext);
                            }

                            PhDereferenceObject(processItem);
                        }
                    }
                    break;
                case 3:
                    PhpThreadStacksExpandNodes(context, TRUE);
                    break;
                case 4:
                    PhpThreadStacksExpandNodes(context, FALSE);
                    break;
                case 5:
                    context->HideUserFrames = !context->HideUserFrames;
                    PhpThreadStacksRestructureNodesWithSearch(context);
                    break;
                case 6:
                    context->HideSystemFrames = !context->HideSystemFrames;
                    PhpThreadStacksRestructureNodesWithSearch(context);
                    break;
                case 7:
                    context->HideInlineFrames = !context->HideInlineFrames;
                    PhpThreadStacksRestructureNodesWithSearch(context);
                    break;
                case 8:
                    context->HighlightUserFrames = !context->HighlightUserFrames;
                    PhSetIntegerSetting(L"UseColorUserThreadStack", context->HighlightUserFrames);
                    PhpThreadStacksInvalidateNodes(context);
                    break;
                case 9:
                    context->HighlightSystemFrames = !context->HighlightSystemFrames;
                    PhSetIntegerSetting(L"UseColorSystemThreadStack", context->HighlightSystemFrames);
                    PhpThreadStacksInvalidateNodes(context);
                    break;
                case 10:
                    context->HighlightInlineFrames = !context->HighlightInlineFrames;
                    PhSetIntegerSetting(L"UseColorInlineThreadStack", context->HighlightInlineFrames);
                    PhpThreadStacksInvalidateNodes(context);
                    break;
                case 100:
                    {
                        PPH_STRING text;

                        text = PhGetTreeNewText(hwnd, 0);
                        PhSetClipboardString(hwnd, &text->sr);
                        PhDereferenceObject(text);
                    }
                    break;
                }
            }

            PhDestroyEMenu(menu);
        }
        return TRUE;
    }

    return FALSE;
}

VOID PhpThreadStacksLoadSettingsTreeList(
    _Inout_ PPH_THREAD_STACKS_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhGetStringSetting(L"ThreadStacksTreeListColumns");
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);
}

VOID PhpThreadStacksSaveSettingsTreeList(
    _Inout_ PPH_THREAD_STACKS_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(L"ThreadStacksTreeListColumns", &settings->sr);
    PhDereferenceObject(settings);
}

VOID PhpInitializeThreadStacksTree(
    _In_ PPH_THREAD_STACKS_CONTEXT Context
    )
{
    PhSetControlTheme(Context->TreeNewHandle, L"explorer");

    TreeNew_SetCallback(Context->TreeNewHandle, PhpThreadStacksTreeNewCallback, Context);
    TreeNew_SetExtendedFlags(Context->TreeNewHandle, TN_FLAG_ITEM_DRAG_SELECT, TN_FLAG_ITEM_DRAG_SELECT);

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_SYMBOL, TRUE, L"Symbol", 250, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_PROCESSID, TRUE, L"PID", 80, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_THREADID, TRUE, L"TID", 80, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_FRAMENUMBER, TRUE, L"Frame", 80, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_ARCHITECTURE, TRUE, L"Architecture", 100, PH_ALIGN_LEFT, 4, 0);

    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_STACKADDRESS, FALSE, L"Stack address", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_FRAMEADDRESS, FALSE, L"Frame address", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_PARAMETER1, FALSE, L"Stack parameter #1", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_PARAMETER2, FALSE, L"Stack parameter #2", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_PARAMETER3, FALSE, L"Stack parameter #3", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_PARAMETER4, FALSE, L"Stack parameter #4", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_CONTROLADDRESS, FALSE, L"Control address", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_RETURNADDRESS, FALSE, L"Return address", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_FILENAME, FALSE, L"File name", 250, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_LINETEXT, FALSE, L"Line number", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_FRAMEDISTANCE, FALSE, L"Frame distance", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_STARTADDRESS, FALSE, L"Start address", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_STARTADDRESSSYMBOL, FALSE, L"Start address (symbolic)", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_PROCESSNAME, FALSE, L"Process name", 250, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_THREAD_STACKS_COLUMN_THREADNAME, FALSE, L"Thread name", 250, PH_ALIGN_LEFT, ULONG_MAX, 0);

    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
    //TreeNew_SetTriState(Context->TreeNewHandle, FALSE);

    PhpThreadStacksLoadSettingsTreeList(Context);
}

VOID PhpThreadStacksSymbolProviderEventCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_SYMBOL_EVENT_DATA event = Parameter;
    PPH_THREAD_STACKS_CONTEXT context = Context;
    PPH_STRING statusMessage;

    if (!event || !context)
        return;

    switch (event->EventType)
    {
    case PH_SYMBOL_EVENT_TYPE_LOAD_START:
    case PH_SYMBOL_EVENT_TYPE_PROGRESS:
        statusMessage = PhReferenceObject(event->EventMessage);
        break;
    default:
        statusMessage = PhReferenceEmptyString();
        break;
    }

    PhAcquireQueuedLockExclusive(&context->MessageLock);
    PhMoveReference(&context->SymbolMessage, statusMessage);
    context->MessageCount++;
    PhReleaseQueuedLockExclusive(&context->MessageLock);
}

INT_PTR CALLBACK PhpThreadStacksDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_THREAD_STACKS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_THREAD_STACKS_CONTEXT)lParam;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_TREELIST);
            context->MessageHandle = GetDlgItem(hwndDlg, IDC_MESSAGE);
            context->SearchWindowHandle = GetDlgItem(hwndDlg, IDC_FILTER);

            context->HighlightUserFrames = !!PhGetIntegerSetting(L"UseColorUserThreadStack");
            context->HighlightSystemFrames = !!PhGetIntegerSetting(L"UseColorSystemThreadStack");
            context->HighlightInlineFrames = !!PhGetIntegerSetting(L"UseColorInlineThreadStack");

            PhSetApplicationWindowIcon(hwndDlg);
            PhRegisterDialog(hwndDlg);
            PhCreateSearchControl(
                hwndDlg,
                context->SearchWindowHandle,
                L"Search Thread Stacks",
                PhpThreadStacksSearchControlCallback,
                context
                );

            PhpInitializeThreadStacksTree(context);
   
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchWindowHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REFRESH), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->MessageHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 300;
            context->MinimumSize.bottom = 100;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            if (PhGetIntegerPairSetting(L"ThreadStacksWindowPosition").X)
                PhLoadWindowPlacementFromSetting(L"ThreadStacksWindowPosition", L"ThreadStacksWindowSize", hwndDlg);
            else
                PhCenterWindow(hwndDlg, PhMainWndHandle);

            PhRegisterWindowCallback(hwndDlg, PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST, NULL);

            PhSetTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT, 200, NULL);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);

            PhpThreadStacksRefresh(context);

            PhRegisterCallback(
                &PhSymbolEventCallback,
                PhpThreadStacksSymbolProviderEventCallback,
                context,
                &context->SymbolProviderEventRegistration
                );
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhUnregisterCallback(&PhSymbolEventCallback, &context->SymbolProviderEventRegistration);

            PhKillTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT);

            PhSaveWindowPlacementToSetting(L"ThreadStacksWindowPosition", L"ThreadStacksWindowSize", hwndDlg);

            PhUnregisterWindowCallback(hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhpThreadStacksSaveSettingsTreeList(context);

            if (context->WorkerContext)
                InterlockedIncrement(&context->WorkerContext->Stop);

            PhDereferenceObject(context);

            PostQuitMessage(0);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
                {
                case IDC_REFRESH:
                    PhpThreadStacksRefresh(context);
                    break;
                case IDCANCEL:
                    DestroyWindow(hwndDlg);
                    break;
                }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, context->MinimumSize.right, context->MinimumSize.bottom);
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_TIMER:
        {
            PPH_STRING statusMessage = NULL;
            PPH_STRING symbolMessage = NULL;

            PhAcquireQueuedLockExclusive(&context->MessageLock);
            if (context->MessageCount != context->LastMessageCount)
            {
                context->LastMessageCount = context->MessageCount;
                statusMessage = PhReferenceObject(context->StatusMessage);
                symbolMessage = PhReferenceObject(context->SymbolMessage);
            }
            PhReleaseQueuedLockExclusive(&context->MessageLock);

            if (statusMessage)
            {
                PPH_STRING message;
                PH_FORMAT format[3];
                ULONG count = 0;

                assert(symbolMessage);

                PhInitFormatSR(&format[count++], statusMessage->sr);
                if (symbolMessage->Length)
                {
                    PhInitFormatS(&format[count++], L" - ");
                    PhInitFormatSR(&format[count++], symbolMessage->sr);
                }

                message = PhFormat(format, count, 80);

                SetWindowText(context->MessageHandle, message->Buffer);

                PhDereferenceObject(message);
                PhDereferenceObject(statusMessage);
                PhDereferenceObject(symbolMessage);
            }
        }
        break;
    case WM_PH_THREAD_STACKS_PUBLISH:
        {
            BOOLEAN restructure = (BOOLEAN)wParam;
            PPH_THREAD_STACKS_NODE node = (PPH_THREAD_STACKS_NODE)lParam;

            if (node)
            {
                PhAddItemList(context->NodeList, node);
                if (node->Type == PhThreadStacksNodeTypeProcess)
                    PhAddItemList(context->NodeRootList, node);
            }

            if (restructure)
                PhpThreadStacksRestructureNodesWithSearch(context);
        }
        break;
    }

    return FALSE;
}

NTSTATUS PhpThreadStacksDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    HWND windowHandle;
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    windowHandle = PhCreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_THRDSTACKS),
        NULL,
        PhpThreadStacksDlgProc,
        Parameter
        );

    ShowWindow(windowHandle, SW_SHOW);
    SetForegroundWindow(windowHandle);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == INT_ERROR)
            break;

        if (!IsDialogMessage(windowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

VOID PhShowThreadStacksDialog(
    _In_ HWND ParentWindowHandle
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PPH_THREAD_STACKS_CONTEXT context;

    if (PhBeginInitOnce(&initOnce))
    {
        PhpThreadStacksObjectType = PhCreateObjectType(L"ThreadStacks", 0, PhpThreadStacksContextDeleteProcedure);
        PhpThreadStacksWorkerObjectType = PhCreateObjectType(L"ThreadStacksWorker", 0, PhpThreadStacksWorkerContextDeleteProcedure);
        PhpThreadStacksNodeObjectType = PhCreateObjectType(L"ThreadStacksNode", 0, PhpThreadStacksNodeDeleteProcedure);

        PhEndInitOnce(&initOnce);
    }

    context = PhpThreadStacksCreateContext();
    context->ParentWindowHandle = ParentWindowHandle;

    if (!NT_SUCCESS(PhCreateThread2(PhpThreadStacksDialogThreadStart, context)))
    {
        PhShowError(ParentWindowHandle, L"%s", L"Unable to create the window.");
        PhDereferenceObject(context);
    }
}
