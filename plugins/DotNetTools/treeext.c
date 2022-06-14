/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015
 *     dmex    2018
 *
 */

#include "dn.h"
#include "clrsup.h"
#include "svcext.h"

VOID NTAPI ThreadsContextCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    );

VOID NTAPI ThreadsContextDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    );

VOID ThreadTreeNewMessage(
    _In_ PVOID Parameter
    );

LONG ThreadTreeNewSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PVOID Context
    );

#define THREAD_TREE_CONTEXT_TYPE 1

typedef struct _THREAD_TREE_CONTEXT
{
    ULONG Type;
    HANDLE ProcessId;
#if _WIN64
    BOOLEAN IsWow64;
    BOOLEAN ConnectedToPhSvc;
#endif
    PH_CALLBACK_REGISTRATION AddedCallbackRegistration;
    PH_CALLBACK_REGISTRATION RemovedCallbackRegistration;
    PCLR_PROCESS_SUPPORT Support;
} THREAD_TREE_CONTEXT, *PTHREAD_TREE_CONTEXT;

VOID InitializeTreeNewObjectExtensions(
    VOID
    )
{
    PhPluginSetObjectExtension(
        PluginInstance,
        EmThreadsContextType,
        sizeof(THREAD_TREE_CONTEXT),
        ThreadsContextCreateCallback,
        ThreadsContextDeleteCallback
        );
}

VOID AddTreeNewColumn(
    _In_ PPH_PLUGIN_TREENEW_INFORMATION TreeNewInfo,
    _In_ PVOID Context,
    _In_ ULONG SubId,
    _In_ BOOLEAN Visible,
    _In_ PWSTR Text,
    _In_ ULONG Width,
    _In_ ULONG Alignment,
    _In_ ULONG TextFlags,
    _In_ BOOLEAN SortDescending,
    _In_ PPH_PLUGIN_TREENEW_SORT_FUNCTION SortFunction
    )
{
    PH_TREENEW_COLUMN column;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Visible = Visible;
    column.SortDescending = SortDescending;
    column.Text = Text;
    column.Width = Width;
    column.Alignment = Alignment;
    column.DisplayIndex = TreeNew_GetVisibleColumnCount(TreeNewInfo->TreeNewHandle);
    column.TextFlags = TextFlags;

    PhPluginAddTreeNewColumn(
        PluginInstance,
        TreeNewInfo->CmData,
        &column,
        SubId,
        Context,
        SortFunction
        );
}

VOID DispatchTreeNewMessage(
    _In_ PVOID Parameter
    )
{
    PPH_PLUGIN_TREENEW_MESSAGE message = Parameter;

    switch (*(PULONG)message->Context)
    {
    case THREAD_TREE_CONTEXT_TYPE:
        ThreadTreeNewMessage(Parameter);
        break;
    }
}

static VOID ThreadAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_THREAD_ITEM threadItem = Parameter;
    PDN_THREAD_ITEM dnThread;

    if (!threadItem)
        return;

    dnThread = PhPluginGetObjectExtension(PluginInstance, threadItem, EmThreadItemType);
    memset(dnThread, 0, sizeof(DN_THREAD_ITEM));
    dnThread->ThreadItem = threadItem;
}

static VOID ThreadRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_THREAD_ITEM threadItem = Parameter;
    PDN_THREAD_ITEM dnThread;
    //PTHREAD_TREE_CONTEXT context = Context;

    if (!threadItem)
        return;

    dnThread = PhPluginGetObjectExtension(PluginInstance, threadItem, EmThreadItemType);

    if (dnThread)
    {
        PhClearReference(&dnThread->AppDomainText);
    }
}

VOID NTAPI ThreadsContextCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_THREADS_CONTEXT threadsContext = Object;
    PTHREAD_TREE_CONTEXT context = Extension;

    memset(context, 0, sizeof(THREAD_TREE_CONTEXT));
    context->Type = THREAD_TREE_CONTEXT_TYPE;
    context->ProcessId = threadsContext->Provider->ProcessId;

#if _WIN64
    if (threadsContext->Provider->ProcessHandle)
    {
        PhGetProcessIsWow64(threadsContext->Provider->ProcessHandle, &context->IsWow64);
    }
#endif

    PhRegisterCallback(
        &threadsContext->Provider->ThreadAddedEvent,
        ThreadAddedHandler,
        context,
        &context->AddedCallbackRegistration
        );

    PhRegisterCallback(
        &threadsContext->Provider->ThreadRemovedEvent,
        ThreadRemovedHandler,
        context,
        &context->RemovedCallbackRegistration
        );
}

VOID NTAPI ThreadsContextDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_THREADS_CONTEXT threadsContext = Object;
    PTHREAD_TREE_CONTEXT context = Extension;

    PhUnregisterCallback(
        &threadsContext->Provider->ThreadAddedEvent,
        &context->AddedCallbackRegistration
        );

    PhUnregisterCallback(
        &threadsContext->Provider->ThreadRemovedEvent,
        &context->RemovedCallbackRegistration
        );

    if (context->Support)
        FreeClrProcessSupport(context->Support);

#if _WIN64
    if (context->ConnectedToPhSvc)
    {
        PhUiDisconnectFromPhSvc();
        context->ConnectedToPhSvc = FALSE;
    }
#endif
}

VOID ThreadTreeNewInitializing(
    _In_ PVOID Parameter
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PPH_THREADS_CONTEXT threadsContext;
    PTHREAD_TREE_CONTEXT context;
    BOOLEAN isDotNet = FALSE;
    ULONG flags = 0;
  
    threadsContext = info->SystemContext;
    context = PhPluginGetObjectExtension(PluginInstance, threadsContext, EmThreadsContextType);

    PhGetProcessIsDotNetEx(
        threadsContext->Provider->ProcessId,
        threadsContext->Provider->ProcessHandle,
#ifdef _WIN64
        PH_CLR_USE_SECTION_CHECK | PH_CLR_NO_WOW64_CHECK | (context->IsWow64 ? PH_CLR_KNOWN_IS_WOW64 : 0),
#else
        PH_CLR_USE_SECTION_CHECK,
#endif
        &isDotNet,
        NULL
        );

    if (!isDotNet && (flags & PH_CLR_CORELIB_PRESENT | PH_CLR_CORE_3_0_ABOVE))
        isDotNet = TRUE;

    if (isDotNet)
    {
#if _WIN64
        if (context->IsWow64)
        {
            context->ConnectedToPhSvc = PhUiConnectToPhSvcEx(NULL, Wow64PhSvcMode, FALSE);
        }
        else
#endif
        {
            PCLR_PROCESS_SUPPORT support;

            if (support = CreateClrProcessSupport(context->ProcessId))
            {
                context->Support = support;
            }
        }
    }

    AddTreeNewColumn(info, context, DNTHTNC_APPDOMAIN, FALSE, L"AppDomain", 120, PH_ALIGN_LEFT, 0, FALSE, ThreadTreeNewSortFunction);
}

VOID ThreadTreeNewUninitializing(
    _In_ PVOID Parameter
    )
{
    NOTHING;
}

VOID UpdateThreadClrData(
    _In_ PTHREAD_TREE_CONTEXT Context,
    _Inout_ PDN_THREAD_ITEM DnThread
    )
{
    if (!DnThread->ClrDataValid)
    {
        PhClearReference(&DnThread->AppDomainText);

#if _WIN64
        if (Context->IsWow64)
        {
            if (Context->ConnectedToPhSvc)
            {
                DnThread->AppDomainText = CallGetClrThreadAppDomain(Context->ProcessId, DnThread->ThreadItem->ThreadId);
            }
        }
        else
#endif
        {
            if (Context->Support)
            {
                DnThread->AppDomainText = DnGetClrThreadAppDomain(Context->Support, DnThread->ThreadItem->ThreadId);
            }
        }

        DnThread->ClrDataValid = TRUE;
    }
}

VOID ThreadTreeNewMessage(
    _In_ PVOID Parameter
    )
{
    PPH_PLUGIN_TREENEW_MESSAGE message = Parameter;
    PTHREAD_TREE_CONTEXT context = message->Context;

    if (message->Message == TreeNewGetCellText)
    {
        PPH_TREENEW_GET_CELL_TEXT getCellText = message->Parameter1;
        PPH_THREAD_NODE threadNode = (PPH_THREAD_NODE)getCellText->Node;
        PDN_THREAD_ITEM dnThread;

        dnThread = PhPluginGetObjectExtension(PluginInstance, threadNode->ThreadItem, EmThreadItemType);

        switch (message->SubId)
        {
        case DNTHTNC_APPDOMAIN:
            UpdateThreadClrData(context, dnThread);
            getCellText->Text = PhGetStringRef(dnThread->AppDomainText);
            break;
        }
    }
}

LONG ThreadTreeNewSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PVOID Context
    )
{
    PTHREAD_TREE_CONTEXT context = Context;
    LONG result;
    PPH_THREAD_NODE node1 = Node1;
    PPH_THREAD_NODE node2 = Node2;
    PDN_THREAD_ITEM dnThread1;
    PDN_THREAD_ITEM dnThread2;

    dnThread1 = PhPluginGetObjectExtension(PluginInstance, node1->ThreadItem, EmThreadItemType);
    dnThread2 = PhPluginGetObjectExtension(PluginInstance, node2->ThreadItem, EmThreadItemType);

    result = 0;

    switch (SubId)
    {
    case DNTHTNC_APPDOMAIN:
        UpdateThreadClrData(context, dnThread1);
        UpdateThreadClrData(context, dnThread2);
        result = PhCompareStringWithNullSortOrder(dnThread1->AppDomainText, dnThread2->AppDomainText, SortOrder, TRUE);
        break;
    }

    return result;
}
