/*
 * Process Hacker -
 *   thread stack viewer
 *
 * Copyright (C) 2010-2016 wj32
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

#include <phapp.h>

#include <kphuser.h>
#include <symprv.h>

#include <actions.h>
#include <phplug.h>
#include <settings.h>
#include <thrdprv.h>

#define WM_PH_COMPLETED (WM_APP + 301)
#define WM_PH_STATUS_UPDATE (WM_APP + 302)

typedef struct _THREAD_STACK_CONTEXT
{
    HANDLE ProcessId;
    HANDLE ThreadId;
    HANDLE ThreadHandle;
    HWND ListViewHandle;
    PPH_THREAD_PROVIDER ThreadProvider;
    PPH_SYMBOL_PROVIDER SymbolProvider;
    BOOLEAN CustomWalk;

    BOOLEAN StopWalk;
    PPH_LIST List;
    PPH_LIST NewList;
    HWND ProgressWindowHandle;
    NTSTATUS WalkStatus;
    PPH_STRING StatusMessage;
    PH_QUEUED_LOCK StatusLock;
} THREAD_STACK_CONTEXT, *PTHREAD_STACK_CONTEXT;

typedef struct _THREAD_STACK_ITEM
{
    PH_THREAD_STACK_FRAME StackFrame;
    ULONG Index;
    PPH_STRING Symbol;
} THREAD_STACK_ITEM, *PTHREAD_STACK_ITEM;

INT_PTR CALLBACK PhpThreadStackDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhpFreeThreadStackItem(
    _In_ PTHREAD_STACK_ITEM StackItem
    );

NTSTATUS PhpRefreshThreadStack(
    _In_ HWND hwnd,
    _In_ PTHREAD_STACK_CONTEXT ThreadStackContext
    );

INT_PTR CALLBACK PhpThreadStackProgressDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

static RECT MinimumSize = { -1, -1, -1, -1 };

VOID PhShowThreadStackDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ PPH_THREAD_PROVIDER ThreadProvider
    )
{
    NTSTATUS status;
    THREAD_STACK_CONTEXT threadStackContext;
    HANDLE threadHandle = NULL;

    // If the user is trying to view a system thread stack
    // but KProcessHacker is not loaded, show an error message.
    if (ProcessId == SYSTEM_PROCESS_ID && !KphIsConnected())
    {
        PhShowError(ParentWindowHandle, PH_KPH_ERROR_MESSAGE);
        return;
    }

    memset(&threadStackContext, 0, sizeof(THREAD_STACK_CONTEXT));
    threadStackContext.ProcessId = ProcessId;
    threadStackContext.ThreadId = ThreadId;
    threadStackContext.ThreadProvider = ThreadProvider;
    threadStackContext.SymbolProvider = ThreadProvider->SymbolProvider;

    if (!NT_SUCCESS(status = PhOpenThread(
        &threadHandle,
        THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME,
        ThreadId
        )))
    {
        if (KphIsConnected())
        {
            status = PhOpenThread(
                &threadHandle,
                ThreadQueryAccess,
                ThreadId
                );
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to open the thread", status, 0);
        return;
    }

    threadStackContext.ThreadHandle = threadHandle;
    threadStackContext.List = PhCreateList(10);
    threadStackContext.NewList = PhCreateList(10);
    PhInitializeQueuedLock(&threadStackContext.StatusLock);

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_THRDSTACK),
        ParentWindowHandle,
        PhpThreadStackDlgProc,
        (LPARAM)&threadStackContext
        );

    PhClearReference(&threadStackContext.StatusMessage);
    PhDereferenceObject(threadStackContext.NewList);
    PhDereferenceObject(threadStackContext.List);

    if (threadStackContext.ThreadHandle)
        NtClose(threadStackContext.ThreadHandle);
}

static INT_PTR CALLBACK PhpThreadStackDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            NTSTATUS status;
            PTHREAD_STACK_CONTEXT threadStackContext;
            PPH_STRING title;
            HWND lvHandle;
            PPH_LAYOUT_MANAGER layoutManager;

            threadStackContext = (PTHREAD_STACK_CONTEXT)lParam;
            SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)threadStackContext);

            title = PhFormatString(L"Stack - thread %u", HandleToUlong(threadStackContext->ThreadId));
            SetWindowText(hwndDlg, title->Buffer);
            PhDereferenceObject(title);

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 30, L" ");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 300, L"Name");
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhLoadListViewColumnsFromSetting(L"ThreadStackListViewColumns", lvHandle);

            threadStackContext->ListViewHandle = lvHandle;

            layoutManager = PhAllocate(sizeof(PH_LAYOUT_MANAGER));
            PhInitializeLayoutManager(layoutManager, hwndDlg);
            SetProp(hwndDlg, L"LayoutManager", (HANDLE)layoutManager);

            PhAddLayoutItem(layoutManager, lvHandle, NULL,
                PH_ANCHOR_ALL);
            PhAddLayoutItem(layoutManager, GetDlgItem(hwndDlg, IDC_COPY),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(layoutManager, GetDlgItem(hwndDlg, IDC_REFRESH),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(layoutManager, GetDlgItem(hwndDlg, IDOK),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (MinimumSize.left == -1)
            {
                RECT rect;

                rect.left = 0;
                rect.top = 0;
                rect.right = 190;
                rect.bottom = 120;
                MapDialogRect(hwndDlg, &rect);
                MinimumSize = rect;
                MinimumSize.left = 0;
            }

            PhLoadWindowPlacementFromSetting(NULL, L"ThreadStackWindowSize", hwndDlg);
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_THREAD_STACK_CONTROL control;

                control.Type = PluginThreadStackInitializing;
                control.UniqueKey = threadStackContext;
                control.u.Initializing.ProcessId = threadStackContext->ProcessId;
                control.u.Initializing.ThreadId = threadStackContext->ThreadId;
                control.u.Initializing.ThreadHandle = threadStackContext->ThreadHandle;
                control.u.Initializing.SymbolProvider = threadStackContext->SymbolProvider;
                control.u.Initializing.CustomWalk = FALSE;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

                threadStackContext->CustomWalk = control.u.Initializing.CustomWalk;
            }

            status = PhpRefreshThreadStack(hwndDlg, threadStackContext);

            if (status == STATUS_ABANDONED)
                EndDialog(hwndDlg, IDCANCEL);
            else if (!NT_SUCCESS(status))
                PhShowStatus(hwndDlg, L"Unable to load the stack", status, 0);
        }
        break;
    case WM_DESTROY:
        {
            PPH_LAYOUT_MANAGER layoutManager;
            PTHREAD_STACK_CONTEXT threadStackContext;
            ULONG i;

            layoutManager = (PPH_LAYOUT_MANAGER)GetProp(hwndDlg, L"LayoutManager");
            PhDeleteLayoutManager(layoutManager);
            PhFree(layoutManager);

            threadStackContext = (PTHREAD_STACK_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_THREAD_STACK_CONTROL control;

                control.Type = PluginThreadStackUninitializing;
                control.UniqueKey = threadStackContext;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
            }

            for (i = 0; i < threadStackContext->List->Count; i++)
                PhpFreeThreadStackItem(threadStackContext->List->Items[i]);

            PhSaveListViewColumnsToSetting(L"ThreadStackListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
            PhSaveWindowPlacementToSetting(NULL, L"ThreadStackWindowSize", hwndDlg);

            RemoveProp(hwndDlg, PhMakeContextAtom());
            RemoveProp(hwndDlg, L"LayoutManager");
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case IDCANCEL: // Esc and X button to close
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_REFRESH:
                {
                    NTSTATUS status;

                    if (!NT_SUCCESS(status = PhpRefreshThreadStack(
                        hwndDlg,
                        (PTHREAD_STACK_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom())
                        )))
                    {
                        PhShowStatus(hwndDlg, L"Unable to load the stack", status, 0);
                    }
                }
                break;
            case IDC_COPY:
                {
                    HWND lvHandle;

                    lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

                    if (ListView_GetSelectedCount(lvHandle) == 0)
                        PhSetStateAllListViewItems(lvHandle, LVIS_SELECTED, LVIS_SELECTED);

                    PhCopyListView(lvHandle);
                    SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)lvHandle, TRUE);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case LVN_GETINFOTIP:
                {
                    LPNMLVGETINFOTIP getInfoTip = (LPNMLVGETINFOTIP)header;
                    HWND lvHandle;
                    PTHREAD_STACK_CONTEXT threadStackContext;

                    lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
                    threadStackContext = (PTHREAD_STACK_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());

                    if (header->hwndFrom == lvHandle)
                    {
                        PTHREAD_STACK_ITEM stackItem;
                        PPH_THREAD_STACK_FRAME stackFrame;

                        if (PhGetListViewItemParam(lvHandle, getInfoTip->iItem, &stackItem))
                        {
                            PH_STRING_BUILDER stringBuilder;
                            PPH_STRING fileName;
                            PH_SYMBOL_LINE_INFORMATION lineInfo;

                            stackFrame = &stackItem->StackFrame;
                            PhInitializeStringBuilder(&stringBuilder, 40);

                            PhAppendFormatStringBuilder(
                                &stringBuilder,
                                L"Stack: 0x%Ix, Frame: 0x%Ix\n",
                                stackFrame->StackAddress,
                                stackFrame->FrameAddress
                                );

                            // There are no params for kernel-mode stack traces.
                            if ((ULONG_PTR)stackFrame->PcAddress <= PhSystemBasicInformation.MaximumUserModeAddress)
                            {
                                PhAppendFormatStringBuilder(
                                    &stringBuilder,
                                    L"Parameters: 0x%Ix, 0x%Ix, 0x%Ix, 0x%Ix\n",
                                    stackFrame->Params[0],
                                    stackFrame->Params[1],
                                    stackFrame->Params[2],
                                    stackFrame->Params[3]
                                    );
                            }

                            if (PhGetLineFromAddress(
                                threadStackContext->SymbolProvider,
                                (ULONG64)stackFrame->PcAddress,
                                &fileName,
                                NULL,
                                &lineInfo
                                ))
                            {
                                PhAppendFormatStringBuilder(
                                    &stringBuilder,
                                    L"File: %s: line %u\n",
                                    fileName->Buffer,
                                    lineInfo.LineNumber
                                    );
                                PhDereferenceObject(fileName);
                            }

                            if (stringBuilder.String->Length != 0)
                                PhRemoveEndStringBuilder(&stringBuilder, 1);

                            if (PhPluginsEnabled)
                            {
                                PH_PLUGIN_THREAD_STACK_CONTROL control;

                                control.Type = PluginThreadStackGetTooltip;
                                control.UniqueKey = threadStackContext;
                                control.u.GetTooltip.StackFrame = stackFrame;
                                control.u.GetTooltip.StringBuilder = &stringBuilder;
                                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
                            }

                            PhCopyListViewInfoTip(getInfoTip, &stringBuilder.String->sr);
                            PhDeleteStringBuilder(&stringBuilder);
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            PPH_LAYOUT_MANAGER layoutManager;

            layoutManager = (PPH_LAYOUT_MANAGER)GetProp(hwndDlg, L"LayoutManager");
            PhLayoutManagerLayout(layoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, MinimumSize.right, MinimumSize.bottom);
        }
        break;
    }

    return FALSE;
}

static VOID PhpFreeThreadStackItem(
    _In_ PTHREAD_STACK_ITEM StackItem
    )
{
    PhClearReference(&StackItem->Symbol);
    PhFree(StackItem);
}

static NTSTATUS PhpRefreshThreadStack(
    _In_ HWND hwnd,
    _In_ PTHREAD_STACK_CONTEXT ThreadStackContext
    )
{
    ULONG i;

    ThreadStackContext->StopWalk = FALSE;
    PhMoveReference(&ThreadStackContext->StatusMessage, PhCreateString(L"Loading stack..."));

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PROGRESS),
        hwnd,
        PhpThreadStackProgressDlgProc,
        (LPARAM)ThreadStackContext
        );

    if (!ThreadStackContext->StopWalk && NT_SUCCESS(ThreadStackContext->WalkStatus))
    {
        for (i = 0; i < ThreadStackContext->List->Count; i++)
            PhpFreeThreadStackItem(ThreadStackContext->List->Items[i]);

        PhDereferenceObject(ThreadStackContext->List);
        ThreadStackContext->List = ThreadStackContext->NewList;
        ThreadStackContext->NewList = PhCreateList(10);

        SendMessage(ThreadStackContext->ListViewHandle, WM_SETREDRAW, FALSE, 0);
        ListView_DeleteAllItems(ThreadStackContext->ListViewHandle);

        for (i = 0; i < ThreadStackContext->List->Count; i++)
        {
            PTHREAD_STACK_ITEM item = ThreadStackContext->List->Items[i];
            INT lvItemIndex;
            WCHAR integerString[PH_INT32_STR_LEN_1];

            PhPrintUInt32(integerString, item->Index);
            lvItemIndex = PhAddListViewItem(ThreadStackContext->ListViewHandle, MAXINT, integerString, item);
            PhSetListViewSubItem(ThreadStackContext->ListViewHandle, lvItemIndex, 1, PhGetStringOrDefault(item->Symbol, L"???"));
        }

        SendMessage(ThreadStackContext->ListViewHandle, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(ThreadStackContext->ListViewHandle, NULL, FALSE);
    }
    else
    {
        for (i = 0; i < ThreadStackContext->NewList->Count; i++)
            PhpFreeThreadStackItem(ThreadStackContext->NewList->Items[i]);

        PhClearList(ThreadStackContext->NewList);
    }

    if (ThreadStackContext->StopWalk)
        return STATUS_ABANDONED;

    return ThreadStackContext->WalkStatus;
}

static BOOLEAN NTAPI PhpWalkThreadStackCallback(
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _In_opt_ PVOID Context
    )
{
    PTHREAD_STACK_CONTEXT threadStackContext = (PTHREAD_STACK_CONTEXT)Context;
    PPH_STRING symbol;
    PTHREAD_STACK_ITEM item;

    if (threadStackContext->StopWalk)
        return FALSE;

    PhAcquireQueuedLockExclusive(&threadStackContext->StatusLock);
    PhMoveReference(&threadStackContext->StatusMessage,
        PhFormatString(L"Processing frame %u...", threadStackContext->NewList->Count));
    PhReleaseQueuedLockExclusive(&threadStackContext->StatusLock);
    PostMessage(threadStackContext->ProgressWindowHandle, WM_PH_STATUS_UPDATE, 0, 0);

    symbol = PhGetSymbolFromAddress(
        threadStackContext->SymbolProvider,
        (ULONG64)StackFrame->PcAddress,
        NULL,
        NULL,
        NULL,
        NULL
        );

    if (symbol &&
        (StackFrame->Flags & PH_THREAD_STACK_FRAME_I386) &&
        !(StackFrame->Flags & PH_THREAD_STACK_FRAME_FPO_DATA_PRESENT))
    {
        PhMoveReference(&symbol, PhConcatStrings2(symbol->Buffer, L" (No unwind info)"));
    }

    item = PhAllocate(sizeof(THREAD_STACK_ITEM));
    item->StackFrame = *StackFrame;
    item->Index = threadStackContext->NewList->Count;

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_THREAD_STACK_CONTROL control;

        control.Type = PluginThreadStackResolveSymbol;
        control.UniqueKey = threadStackContext;
        control.u.ResolveSymbol.StackFrame = StackFrame;
        control.u.ResolveSymbol.Symbol = symbol;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

        symbol = control.u.ResolveSymbol.Symbol;
    }

    item->Symbol = symbol;
    PhAddItemList(threadStackContext->NewList, item);

    return TRUE;
}

static NTSTATUS PhpRefreshThreadStackThreadStart(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    NTSTATUS status;
    PTHREAD_STACK_CONTEXT threadStackContext = Parameter;
    CLIENT_ID clientId;
    BOOLEAN defaultWalk;

    PhInitializeAutoPool(&autoPool);

    PhLoadSymbolsThreadProvider(threadStackContext->ThreadProvider);

    clientId.UniqueProcess = threadStackContext->ProcessId;
    clientId.UniqueThread = threadStackContext->ThreadId;
    defaultWalk = TRUE;

    if (threadStackContext->CustomWalk)
    {
        PH_PLUGIN_THREAD_STACK_CONTROL control;

        control.Type = PluginThreadStackWalkStack;
        control.UniqueKey = threadStackContext;
        control.u.WalkStack.Status = STATUS_UNSUCCESSFUL;
        control.u.WalkStack.ThreadHandle = threadStackContext->ThreadHandle;
        control.u.WalkStack.ProcessHandle = threadStackContext->SymbolProvider->ProcessHandle;
        control.u.WalkStack.ClientId = &clientId;
        control.u.WalkStack.Flags = PH_WALK_I386_STACK | PH_WALK_AMD64_STACK | PH_WALK_KERNEL_STACK;
        control.u.WalkStack.Callback = PhpWalkThreadStackCallback;
        control.u.WalkStack.CallbackContext = threadStackContext;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
        status = control.u.WalkStack.Status;

        if (NT_SUCCESS(status))
            defaultWalk = FALSE;
    }

    if (defaultWalk)
    {
        PH_PLUGIN_THREAD_STACK_CONTROL control;

        control.UniqueKey = threadStackContext;

        if (PhPluginsEnabled)
        {
            control.Type = PluginThreadStackBeginDefaultWalkStack;
            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
        }

        status = PhWalkThreadStack(
            threadStackContext->ThreadHandle,
            threadStackContext->SymbolProvider->ProcessHandle,
            &clientId,
            threadStackContext->SymbolProvider,
            PH_WALK_I386_STACK | PH_WALK_AMD64_STACK | PH_WALK_KERNEL_STACK,
            PhpWalkThreadStackCallback,
            threadStackContext
            );

        if (PhPluginsEnabled)
        {
            control.Type = PluginThreadStackEndDefaultWalkStack;
            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
        }
    }

    if (threadStackContext->NewList->Count != 0)
        status = STATUS_SUCCESS;

    threadStackContext->WalkStatus = status;
    PostMessage(threadStackContext->ProgressWindowHandle, WM_PH_COMPLETED, 0, 0);

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

static INT_PTR CALLBACK PhpThreadStackProgressDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PTHREAD_STACK_CONTEXT threadStackContext;
            HANDLE threadHandle;

            threadStackContext = (PTHREAD_STACK_CONTEXT)lParam;
            SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)threadStackContext);
            threadStackContext->ProgressWindowHandle = hwndDlg;

            if (threadHandle = PhCreateThread(0, PhpRefreshThreadStackThreadStart, threadStackContext))
            {
                NtClose(threadHandle);
            }
            else
            {
                threadStackContext->WalkStatus = STATUS_UNSUCCESSFUL;
                EndDialog(hwndDlg, IDOK);
                break;
            }

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhSetWindowStyle(GetDlgItem(hwndDlg, IDC_PROGRESS), PBS_MARQUEE, PBS_MARQUEE);
            SendMessage(GetDlgItem(hwndDlg, IDC_PROGRESS), PBM_SETMARQUEE, TRUE, 75);
            SetWindowText(hwndDlg, L"Loading stack...");
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, PhMakeContextAtom());
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                {
                    PTHREAD_STACK_CONTEXT threadStackContext = (PTHREAD_STACK_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());

                    EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);
                    threadStackContext->StopWalk = TRUE;
                }
                break;
            }
        }
        break;
    case WM_PH_COMPLETED:
        {
            EndDialog(hwndDlg, IDOK);
        }
        break;
    case WM_PH_STATUS_UPDATE:
        {
            PTHREAD_STACK_CONTEXT threadStackContext = (PTHREAD_STACK_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());
            PPH_STRING message;

            PhAcquireQueuedLockExclusive(&threadStackContext->StatusLock);
            message = threadStackContext->StatusMessage;
            PhReferenceObject(message);
            PhReleaseQueuedLockExclusive(&threadStackContext->StatusLock);

            SetDlgItemText(hwndDlg, IDC_PROGRESSTEXT, message->Buffer);
            PhDereferenceObject(message);
        }
        break;
    }

    return 0;
}
