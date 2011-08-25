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
} THREAD_STACK_CONTEXT, *PTHREAD_STACK_CONTEXT;

static PPH_HASHTABLE ContextHashtable;
static PH_QUEUED_LOCK ContextHashtableLock = PH_QUEUED_LOCK_INIT;
static PH_INITONCE ContextHashtableInitOnce = PH_INITONCE_INIT;

VOID ProcessThreadStackControl(
    __in PPH_PLUGIN_THREAD_STACK_CONTROL Control
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
            context->Support = CreateClrProcessSupport(Control->u.Initializing.ProcessId);

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
                managedSymbol = GetRuntimeNameByAddressClrProcess(
                    context->Support,
                    (ULONG64)Control->u.ResolveSymbol.StackFrame->PcAddress,
                    &displacement
                    );

                if (managedSymbol)
                {
                    if (displacement != 0)
                    {
                        PhSwapReference2(&managedSymbol, PhFormatString(L"%s + 0x%I64x", managedSymbol->Buffer, displacement));
                    }

                    if (Control->u.ResolveSymbol.Symbol->Buffer)
                        PhSwapReference2(&managedSymbol, PhFormatString(L"%s <-- %s", managedSymbol->Buffer, Control->u.ResolveSymbol.Symbol->Buffer));

                    PhSwapReference2(&Control->u.ResolveSymbol.Symbol, managedSymbol);
                }
            }
        }
        break;
    }
}
