/*
 * Process Hacker .NET Tools - 
 *   thread list extensions
 * 
 * Copyright (C) 2015 wj32
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
    _In_ PVOID Context
    );

#define THREAD_TREE_CONTEXT_TYPE 1

typedef struct _THREAD_TREE_CONTEXT
{
    ULONG Type;
    HANDLE ProcessId;
    PH_CALLBACK_REGISTRATION AddedCallbackRegistration;
    PCLR_PROCESS_SUPPORT Support;
} THREAD_TREE_CONTEXT, *PTHREAD_TREE_CONTEXT;

static PPH_HASHTABLE ContextHashtable;
static PH_QUEUED_LOCK ContextHashtableLock = PH_QUEUED_LOCK_INIT;
static PH_INITONCE ContextHashtableInitOnce = PH_INITONCE_INIT;

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
    PTHREAD_TREE_CONTEXT context = Context;

    dnThread = PhPluginGetObjectExtension(PluginInstance, threadItem, EmThreadItemType);
    memset(dnThread, 0, sizeof(DN_THREAD_ITEM));
    dnThread->ThreadItem = threadItem;
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

    PhRegisterCallback(
        &threadsContext->Provider->ThreadAddedEvent,
        ThreadAddedHandler,
        context,
        &context->AddedCallbackRegistration
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

    if (context->Support)
        FreeClrProcessSupport(context->Support);
}

VOID ThreadTreeNewInitializing(
    _In_ PVOID Parameter
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PPH_THREADS_CONTEXT threadsContext;
    PTHREAD_TREE_CONTEXT context;
    BOOLEAN isDotNet;

    threadsContext = info->SystemContext;
    context = PhPluginGetObjectExtension(PluginInstance, threadsContext, EmThreadsContextType);

    if (NT_SUCCESS(PhGetProcessIsDotNet(threadsContext->Provider->ProcessId, &isDotNet)) && isDotNet)
    {
        PCLR_PROCESS_SUPPORT support;

        support = CreateClrProcessSupport(threadsContext->Provider->ProcessId);

        if (!support)
            return;

        context->Support = support;

        AddTreeNewColumn(info, context, DNTHTNC_APPDOMAIN, TRUE, L"AppDomain", 120, PH_ALIGN_LEFT, 0, FALSE, ThreadTreeNewSortFunction);
    }
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
        IXCLRDataProcess *process;
        IXCLRDataTask *task;
        IXCLRDataAppDomain *appDomain;

        if (Context->Support)
            process = Context->Support->DataProcess;
        else
            return;

        if (SUCCEEDED(IXCLRDataProcess_GetTaskByOSThreadID(process, HandleToUlong(DnThread->ThreadItem->ThreadId), &task)))
        {
            if (SUCCEEDED(IXCLRDataTask_GetCurrentAppDomain(task, &appDomain)))
            {
                DnThread->AppDomainText = GetNameXClrDataAppDomain(appDomain);
                IXCLRDataAppDomain_Release(appDomain);
            }

            IXCLRDataTask_Release(task);
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
        result = PhCompareStringWithNull(dnThread1->AppDomainText, dnThread2->AppDomainText, TRUE);
        break;
    }

    return result;
}
