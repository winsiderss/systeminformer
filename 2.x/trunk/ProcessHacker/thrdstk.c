/*
 * Process Hacker - 
 *   thread stack viewer
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

#include <phapp.h>
#include <kphuser.h>
#include <settings.h>

typedef struct THREAD_STACK_CONTEXT
{
    HANDLE ProcessId;
    HANDLE ThreadId;
    HANDLE ThreadHandle;
    HWND ListViewHandle;
    PPH_SYMBOL_PROVIDER SymbolProvider;

    ULONG Index;
    PPH_LIST List;
} THREAD_STACK_CONTEXT, *PTHREAD_STACK_CONTEXT;

INT_PTR CALLBACK PhpThreadStackDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

NTSTATUS PhpRefreshThreadStack(
    __in PTHREAD_STACK_CONTEXT ThreadStackContext
    );

static RECT MinimumSize = { -1, -1, -1, -1 };

VOID PhShowThreadStackDialog(
    __in HWND ParentWindowHandle,
    __in HANDLE ProcessId,
    __in HANDLE ThreadId,
    __in PPH_SYMBOL_PROVIDER SymbolProvider
    )
{
    NTSTATUS status;
    THREAD_STACK_CONTEXT threadStackContext;
    HANDLE threadHandle = NULL;

    // If the user is trying to view a system thread stack 
    // but KProcessHacker is not loaded, show an error message.
    if (ProcessId == SYSTEM_PROCESS_ID && !KphIsConnected())
    {
        PhShowError(ParentWindowHandle, KPH_ERROR_MESSAGE);
        return;
    }

    threadStackContext.ProcessId = ProcessId;
    threadStackContext.ThreadId = ThreadId;
    threadStackContext.SymbolProvider = SymbolProvider;

    if (!NT_SUCCESS(status = PhOpenThread(
        &threadHandle,
        THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME,
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

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_THRDSTACK),
        ParentWindowHandle,
        PhpThreadStackDlgProc,
        (LPARAM)&threadStackContext
        );

    PhDereferenceObject(threadStackContext.List);

    if (threadStackContext.ThreadHandle)
        NtClose(threadStackContext.ThreadHandle);
} 

static INT_PTR CALLBACK PhpThreadStackDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PTHREAD_STACK_CONTEXT threadStackContext;
            PPH_STRING title;
            HWND lvHandle;
            PPH_LAYOUT_MANAGER layoutManager;

            threadStackContext = (PTHREAD_STACK_CONTEXT)lParam;
            SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)threadStackContext);

            title = PhFormatString(L"Stack - thread %u", (ULONG)threadStackContext->ThreadId);
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

            PhpRefreshThreadStack(threadStackContext);
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

            for (i = 0; i < threadStackContext->List->Count; i++)
                PhFree(threadStackContext->List->Items[i]);

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
                    PhpRefreshThreadStack(
                        (PTHREAD_STACK_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom())
                        );
                }
                break;
            case IDC_COPY:
                {
                    HWND lvHandle;

                    lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

                    if (ListView_GetSelectedCount(lvHandle) == 0)
                        PhSetStateAllListViewItems(lvHandle, LVIS_SELECTED, LVIS_SELECTED);

                    PhCopyListView(lvHandle);
                    SetFocus(lvHandle);
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
                        PPH_THREAD_STACK_FRAME stackFrame;

                        if (PhGetListViewItemParam(lvHandle, getInfoTip->iItem, &stackFrame))
                        {
                            PH_STRING_BUILDER stringBuilder;
                            PPH_STRING fileName;
                            PH_SYMBOL_LINE_INFORMATION lineInfo;

                            PhInitializeStringBuilder(&stringBuilder, 40);

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
                                PhRemoveStringBuilder(&stringBuilder, stringBuilder.String->Length / 2 - 1, 1);

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

static BOOLEAN NTAPI PhpWalkThreadStackCallback(
    __in PPH_THREAD_STACK_FRAME StackFrame,
    __in_opt PVOID Context
    )
{
    PTHREAD_STACK_CONTEXT threadStackContext = (PTHREAD_STACK_CONTEXT)Context;
    PPH_STRING symbol;
    INT lvItemIndex;
    WCHAR integerString[PH_INT32_STR_LEN_1];
    PPH_THREAD_STACK_FRAME stackFrame;

    symbol = PhGetSymbolFromAddress(
        threadStackContext->SymbolProvider,
        (ULONG64)StackFrame->PcAddress,
        NULL,
        NULL,
        NULL,
        NULL
        );

    PhPrintUInt32(integerString, threadStackContext->Index++);

    stackFrame = PhAllocateCopy(StackFrame, sizeof(PH_THREAD_STACK_FRAME));
    PhAddItemList(threadStackContext->List, stackFrame);
    lvItemIndex = PhAddListViewItem(threadStackContext->ListViewHandle, MAXINT,
        integerString, stackFrame);

    if (symbol)
    {
        PhSetListViewSubItem(threadStackContext->ListViewHandle, lvItemIndex, 1,
            symbol->Buffer);
        PhDereferenceObject(symbol);
    }
    else
    {
        PhSetListViewSubItem(threadStackContext->ListViewHandle, lvItemIndex, 1,
            L"???");
    }

    return TRUE;
}

static NTSTATUS PhpRefreshThreadStack(
    __in PTHREAD_STACK_CONTEXT ThreadStackContext
    )
{
    NTSTATUS status;
    ULONG i;
    CLIENT_ID clientId;

    clientId.UniqueProcess = ThreadStackContext->ProcessId;
    clientId.UniqueThread = ThreadStackContext->ThreadId;

    ListView_DeleteAllItems(ThreadStackContext->ListViewHandle);

    for (i = 0; i < ThreadStackContext->List->Count; i++)
        PhFree(ThreadStackContext->List->Items[i]);

    PhClearList(ThreadStackContext->List);

    SendMessage(ThreadStackContext->ListViewHandle, WM_SETREDRAW, FALSE, 0);
    ThreadStackContext->Index = 0;
    status = PhWalkThreadStack(
        ThreadStackContext->ThreadHandle,
        ThreadStackContext->SymbolProvider->ProcessHandle,
        &clientId,
        PH_WALK_I386_STACK | PH_WALK_AMD64_STACK | PH_WALK_KERNEL_STACK,
        PhpWalkThreadStackCallback,
        ThreadStackContext
        );
    SendMessage(ThreadStackContext->ListViewHandle, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(ThreadStackContext->ListViewHandle, NULL, FALSE);

    return status;
}
