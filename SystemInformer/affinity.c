/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2020-2022
 *
 */

/*
 * The affinity dialog was originally created to support the modification
 * of process affinity masks, but now supports modifying thread affinity
 * and generic masks.
 */

#include <phapp.h>
#include <phsettings.h>
#include <procprv.h>
#include <thrdprv.h>

typedef struct _PH_AFFINITY_DIALOG_CONTEXT
{
    HWND WindowHandle;
    HWND GroupComboHandle;

    PPH_PROCESS_ITEM ProcessItem;
    PPH_THREAD_ITEM ThreadItem;
    KAFFINITY NewAffinityMask;

    PPH_LIST CpuControlList;
    USHORT AffinityGroup;
    KAFFINITY AffinityMask;
    KAFFINITY SystemAffinityMask;

    // Multiple selected items (dmex)
    PPH_THREAD_ITEM* Threads;
    ULONG NumberOfThreads;
    PHANDLE ThreadHandles;
} PH_AFFINITY_DIALOG_CONTEXT, *PPH_AFFINITY_DIALOG_CONTEXT;

INT_PTR CALLBACK PhpProcessAffinityDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhShowProcessAffinityDialog(
    _In_ HWND ParentWindowHandle,
    _In_opt_ PPH_PROCESS_ITEM ProcessItem,
    _In_opt_ PPH_THREAD_ITEM ThreadItem
    )
{
    PH_AFFINITY_DIALOG_CONTEXT context;

    assert(!!ProcessItem != !!ThreadItem); // make sure we have one and not the other (wj32)

    memset(&context, 0, sizeof(PH_AFFINITY_DIALOG_CONTEXT));
    context.ProcessItem = ProcessItem;
    context.ThreadItem = ThreadItem;

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_AFFINITY),
        ParentWindowHandle,
        PhpProcessAffinityDlgProc,
        (LPARAM)&context
        );
}

_Success_(return)
BOOLEAN PhShowProcessAffinityDialog2(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _Out_ PKAFFINITY NewAffinityMask
    )
{
    PH_AFFINITY_DIALOG_CONTEXT context;

    memset(&context, 0, sizeof(PH_AFFINITY_DIALOG_CONTEXT));
    context.ProcessItem = ProcessItem;
    context.ThreadItem = NULL;

    if (DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_AFFINITY),
        ParentWindowHandle,
        PhpProcessAffinityDlgProc,
        (LPARAM)&context
        ) == IDOK)
    {
        *NewAffinityMask = context.NewAffinityMask;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

VOID PhShowThreadAffinityDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_THREAD_ITEM* Threads,
    _In_ ULONG NumberOfThreads
    )
{
    PH_AFFINITY_DIALOG_CONTEXT context;

    memset(&context, 0, sizeof(PH_AFFINITY_DIALOG_CONTEXT));
    context.Threads = Threads;
    context.NumberOfThreads = NumberOfThreads;
    context.ThreadHandles = PhAllocateZero(NumberOfThreads * sizeof(HANDLE));

    // Cache handles to each thread since the ThreadId gets 
    // reassigned to a different process after the thread exits. (dmex)
    for (ULONG i = 0; i < NumberOfThreads; i++)
    {
        PhOpenThread(
            &context.ThreadHandles[i],
            THREAD_QUERY_LIMITED_INFORMATION | THREAD_SET_LIMITED_INFORMATION,
            Threads[i]->ThreadId
            );
    }

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_AFFINITY),
        ParentWindowHandle,
        PhpProcessAffinityDlgProc,
        (LPARAM)&context
        );
}

static BOOLEAN PhpShowProcessErrorAffinity(
    _In_ HWND hWnd,
    _In_ PPH_PROCESS_ITEM Process,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    )
{
    return PhShowContinueStatus(
        hWnd,
        PhaFormatString(
        L"Unable to change affinity of process %lu",
        HandleToUlong(Process->ProcessId)
        )->Buffer,
        Status,
        Win32Result
        );
}

static BOOLEAN PhpShowThreadErrorAffinity(
    _In_ HWND hWnd,
    _In_ PPH_THREAD_ITEM Thread,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    )
{
    return PhShowContinueStatus(
        hWnd,
        PhaFormatString(
        L"Unable to change affinity of thread %lu",
        HandleToUlong(Thread->ThreadId)
        )->Buffer,
        Status,
        Win32Result
        );
}

VOID PhpShowThreadErrorAffinityList(
    _In_ PPH_AFFINITY_DIALOG_CONTEXT Context,
    _Inout_ PPH_LIST AffinityErrorsList
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 100);

    for (ULONG i = 0; i < AffinityErrorsList->Count; i++)
    {
        PPH_STRING statusMessage = AffinityErrorsList->Items[i];

        PhAppendFormatStringBuilder(
            &stringBuilder,
            L"%s\n",
            PhGetStringOrDefault(statusMessage, L"An unknown error occurred.")
            );

        PhDereferenceObject(statusMessage);
    }

    if (PhEndsWithStringRef2(&stringBuilder.String->sr, L"\n", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhShowInformation2(
        Context->WindowHandle,
        L"Unable to update affinity for thread(s)",
        L"Unable to update affinity for thread(s):\r\n%s",
        PhGetString(PhFinalStringBuilderString(&stringBuilder))
        );

    PhDeleteStringBuilder(&stringBuilder);
    PhDereferenceObjects(AffinityErrorsList->Items, AffinityErrorsList->Count);
    PhDereferenceObject(AffinityErrorsList);
}

BOOLEAN PhpCheckThreadsHaveSameAffinity(
    _In_ PPH_AFFINITY_DIALOG_CONTEXT Context
    )
{
    BOOLEAN result = TRUE;
    GROUP_AFFINITY groupAffinity;
    KAFFINITY lastAffinityMask = 0;
    KAFFINITY affinityMask = 0;

    if (NT_SUCCESS(PhGetThreadGroupAffinity(Context->ThreadHandles[0], &groupAffinity)))
    {
        lastAffinityMask = groupAffinity.Mask;
    }

    for (ULONG i = 0; i < Context->NumberOfThreads; i++)
    {
        if (!Context->ThreadHandles[i])
            continue;

        if (NT_SUCCESS(PhGetThreadGroupAffinity(Context->ThreadHandles[i], &groupAffinity)))
        {
            affinityMask = groupAffinity.Mask;
        }

        if (lastAffinityMask != affinityMask)
        {
            result = FALSE;
            break;
        }
    }

    return result;
}

INT_PTR CALLBACK PhpProcessAffinityDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_AFFINITY_DIALOG_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_AFFINITY_DIALOG_CONTEXT)lParam;
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
            NTSTATUS status = STATUS_UNSUCCESSFUL;
            BOOLEAN differentAffinity = FALSE;
            ULONG i;

            context->WindowHandle = hwndDlg;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            PhSetApplicationWindowIcon(hwndDlg);

            {
                context->CpuControlList = PhCreateList(MAXIMUM_PROC_PER_GROUP);

                for (i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
                {
                    PhAddItemList(context->CpuControlList, GetDlgItem(hwndDlg, IDC_CPU0 + i));
                }
            }

            if (!PhSystemProcessorInformation.SingleProcessorGroup)
            {
                context->GroupComboHandle = GetDlgItem(hwndDlg, IDC_GROUPCPU);

                for (USHORT processorGroup = 0; processorGroup < PhSystemProcessorInformation.NumberOfProcessorGroups; processorGroup++)
                {
                    ComboBox_AddString(context->GroupComboHandle, PhaFormatString(L"Group %hu", processorGroup)->Buffer);
                }

                ShowWindow(context->GroupComboHandle, SW_SHOW);
            }

            if (context->ProcessItem)
            {
                HANDLE processHandle;
                GROUP_AFFINITY processGroupAffinity = { 0 };

                if (NT_SUCCESS(status = PhOpenProcess(
                    &processHandle,
                    PROCESS_QUERY_LIMITED_INFORMATION,
                    context->ProcessItem->ProcessId
                    )))
                {
                    status = PhGetProcessGroupAffinity(processHandle, &processGroupAffinity);

                    if (NT_SUCCESS(status))
                    {
                        context->AffinityMask = processGroupAffinity.Mask;
                        context->AffinityGroup = processGroupAffinity.Group;

                        if (context->GroupComboHandle)
                        {
                            ComboBox_SetCurSel(context->GroupComboHandle, processGroupAffinity.Group);
                        }
                    }

                    NtClose(processHandle);
                }
            }
            else if (context->ThreadItem)
            {
                HANDLE threadHandle;
                THREAD_BASIC_INFORMATION basicInfo;
                GROUP_AFFINITY groupAffinity = { 0 };
                HANDLE processHandle;

                if (NT_SUCCESS(status = PhOpenThread(
                    &threadHandle,
                    THREAD_QUERY_LIMITED_INFORMATION,
                    context->ThreadItem->ThreadId
                    )))
                {
                    status = PhGetThreadGroupAffinity(threadHandle, &groupAffinity);

                    if (NT_SUCCESS(status))
                    {
                        context->AffinityMask = groupAffinity.Mask;
                        context->AffinityGroup = groupAffinity.Group;

                        if (context->GroupComboHandle)
                        {
                            ComboBox_SetCurSel(context->GroupComboHandle, groupAffinity.Group);
                        }

                        if (NT_SUCCESS(PhGetThreadBasicInformation(
                            threadHandle,
                            &basicInfo
                            )))
                        {
                            // A thread's affinity mask is restricted by the process affinity mask,
                            // so use that as the system affinity mask. (wj32)

                            if (NT_SUCCESS(PhOpenProcess(
                                &processHandle,
                                PROCESS_QUERY_LIMITED_INFORMATION,
                                basicInfo.ClientId.UniqueProcess
                                )))
                            {
                                GROUP_AFFINITY processGroupAffinity = { 0 };

                                if (NT_SUCCESS(PhGetProcessGroupAffinity(processHandle, &processGroupAffinity)))
                                {
                                    context->SystemAffinityMask = processGroupAffinity.Mask;
                                }

                                NtClose(processHandle);
                            }
                        }
                    }

                    NtClose(threadHandle);
                }
            }
            else if (context->Threads)
            {
                HANDLE processHandle;
                THREAD_BASIC_INFORMATION basicInfo;
                GROUP_AFFINITY groupAffinity = { 0 };
                PPH_STRING windowText;

                windowText = PH_AUTO(PhGetWindowText(hwndDlg));
                PhSetWindowText(hwndDlg, PhaFormatString(
                    L"%s (%lu threads)",
                    windowText->Buffer,
                    context->NumberOfThreads
                    )->Buffer);

                differentAffinity = !PhpCheckThreadsHaveSameAffinity(context);

                // Use affinity from the first thread when all threads are identical (dmex)
                status = PhGetThreadGroupAffinity(
                    context->ThreadHandles[0],
                    &groupAffinity
                    );

                if (NT_SUCCESS(status))
                {
                    context->AffinityMask = groupAffinity.Mask;
                    context->AffinityGroup = groupAffinity.Group;

                    if (NT_SUCCESS(PhGetThreadBasicInformation(
                        context->ThreadHandles[0],
                        &basicInfo
                        )))
                    {
                        if (NT_SUCCESS(PhOpenProcess(
                            &processHandle,
                            PROCESS_QUERY_LIMITED_INFORMATION,
                            basicInfo.ClientId.UniqueProcess
                            )))
                        {
                            GROUP_AFFINITY processGroupAffinity = { 0 };

                            if (NT_SUCCESS(PhGetProcessGroupAffinity(processHandle, &processGroupAffinity)))
                            {
                                context->SystemAffinityMask = processGroupAffinity.Mask;
                            }

                            NtClose(processHandle);
                        }
                    }
                }
            }

            if (NT_SUCCESS(status) && context->SystemAffinityMask == 0)
            {
                KAFFINITY systemAffinityMask;

                if (PhSystemProcessorInformation.SingleProcessorGroup)
                {
                    status = PhGetProcessorSystemAffinityMask(&systemAffinityMask);
                }
                else
                {
                    status = PhGetProcessorGroupActiveAffinityMask(context->AffinityGroup, &systemAffinityMask);
                }

                if (NT_SUCCESS(status))
                {
                    context->SystemAffinityMask = systemAffinityMask;
                }
            }

            if (!NT_SUCCESS(status))
            {
                PhShowStatus(hwndDlg, L"Unable to query the current affinity.", status, 0);
                EndDialog(hwndDlg, IDCANCEL);
                break;
            }

            // Disable the CPU checkboxes which aren't part of the system affinity mask,
            // and check the CPU checkboxes which are part of the affinity mask. (wj32)

            for (i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
            {
                if ((context->SystemAffinityMask >> i) & 0x1)
                {
                    if (differentAffinity) // Skip for multiple selection (dmex)
                        continue;

                    if ((context->AffinityMask >> i) & 0x1)
                    {
                        Button_SetCheck(context->CpuControlList->Items[i], BST_CHECKED);
                    }
                }
                else
                {
                    EnableWindow(context->CpuControlList->Items[i], FALSE);
                }
            }

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            if (context->ThreadHandles)
            {
                for (ULONG i = 0; i < context->NumberOfThreads; i++)
                {
                    if (context->ThreadHandles[i])
                    {
                        NtClose(context->ThreadHandles[i]);
                        context->ThreadHandles[i] = NULL;
                    }
                }

                PhFree(context->ThreadHandles);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    NTSTATUS status = STATUS_SUCCESS;
                    KAFFINITY affinityMask = 0;
                    USHORT affinityGroup = 0;

                    // Work out the affinity mask.

                    for (ULONG i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
                    {
                        if (Button_GetCheck(context->CpuControlList->Items[i]) == BST_CHECKED)
                            affinityMask |= (KAFFINITY)1 << i;
                    }

                    if (context->GroupComboHandle)
                    {
                        affinityGroup = ComboBox_GetCurSel(context->GroupComboHandle);

                        if (affinityGroup == CB_ERR)
                            affinityGroup = context->AffinityGroup;
                    }

                    if (context->ProcessItem)
                    {
                        HANDLE processHandle;

                        if (NT_SUCCESS(status = PhOpenProcess(
                            &processHandle,
                            PROCESS_SET_INFORMATION,
                            context->ProcessItem->ProcessId
                            )))
                        {
                            if (PhSystemProcessorInformation.SingleProcessorGroup)
                            {
                                status = PhSetProcessAffinityMask(processHandle, affinityMask);
                            }
                            else
                            {
                                GROUP_AFFINITY groupAffinity;

                                memset(&groupAffinity, 0, sizeof(GROUP_AFFINITY));
                                groupAffinity.Group = affinityGroup;
                                groupAffinity.Mask = affinityMask;

                                status = PhSetProcessGroupAffinity(processHandle, groupAffinity);
                            }

                            NtClose(processHandle);
                        }

                        if (NT_SUCCESS(status))
                        {
                            context->NewAffinityMask = affinityMask;
                        }
                        else
                        {
                            PhpShowProcessErrorAffinity(hwndDlg, context->ProcessItem, status, 0);
                        }
                    }
                    else if (context->ThreadItem)
                    {
                        HANDLE threadHandle;

                        if (NT_SUCCESS(status = PhOpenThread(
                            &threadHandle,
                            THREAD_SET_LIMITED_INFORMATION,
                            context->ThreadItem->ThreadId
                            )))
                        {
                            if (PhSystemProcessorInformation.SingleProcessorGroup)
                            {
                                status = PhSetThreadAffinityMask(threadHandle, affinityMask);
                            }
                            else
                            {
                                GROUP_AFFINITY groupAffinity;

                                memset(&groupAffinity, 0, sizeof(GROUP_AFFINITY));
                                groupAffinity.Group = affinityGroup;
                                groupAffinity.Mask = affinityMask;

                                status = PhSetThreadGroupAffinity(threadHandle, groupAffinity);
                            }

                            NtClose(threadHandle);

                            if (!NT_SUCCESS(status))
                            {
                                PhpShowThreadErrorAffinity(hwndDlg, context->ThreadItem, status, 0);
                            }
                        }
                    }
                    else if (context->Threads)
                    {
                        PPH_LIST threadAffinityErrors = PhCreateList(1);

                        for (ULONG i = 0; i < context->NumberOfThreads; i++)
                        {
                            if (!context->ThreadHandles[i])
                                continue;

                            if (PhSystemProcessorInformation.SingleProcessorGroup)
                            {
                                status = PhSetThreadAffinityMask(context->ThreadHandles[i], affinityMask);
                            }
                            else
                            {
                                GROUP_AFFINITY groupAffinity;

                                memset(&groupAffinity, 0, sizeof(GROUP_AFFINITY));
                                groupAffinity.Group = affinityGroup;
                                groupAffinity.Mask = affinityMask;

                                status = PhSetThreadGroupAffinity(context->ThreadHandles[i], groupAffinity);
                            }

                            if (!NT_SUCCESS(status))
                            {
                                PPH_STRING errorMessage;

                                if (errorMessage = PhGetNtMessage(status))
                                {
                                    PhAddItemList(threadAffinityErrors, errorMessage);
                                }
                            }
                        }

                        if (threadAffinityErrors->Count > 0)
                            PhpShowThreadErrorAffinityList(context, threadAffinityErrors);
                        else
                            PhDereferenceObject(threadAffinityErrors);
                    }

                    if (NT_SUCCESS(status))
                    {
                        EndDialog(hwndDlg, IDOK);
                    }
                }
                break;
            case IDC_SELECTALL:
            case IDC_DESELECTALL:
                {
                    for (ULONG i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
                    {
                        HWND checkBox = context->CpuControlList->Items[i];

                        if (IsWindowEnabled(checkBox))
                            Button_SetCheck(checkBox, GET_WM_COMMAND_ID(wParam, lParam) == IDC_SELECTALL ? BST_CHECKED : BST_UNCHECKED);
                    }
                }
                break;
            case IDC_GROUPCPU:
                {
                    INT index;

                    if (!context->GroupComboHandle)
                        break;

                    index = ComboBox_GetCurSel(context->GroupComboHandle);

                    if (index != CB_ERR)
                    {
                        if (index != context->AffinityGroup)
                        {
                            for (ULONG i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
                            {
                                Button_SetCheck(context->CpuControlList->Items[i], BST_UNCHECKED);
                            }
                        }
                        else
                        {
                            BOOLEAN differentAffinity = FALSE;

                            if (context->Threads)
                            {
                                differentAffinity = !PhpCheckThreadsHaveSameAffinity(context);
                            }

                            for (ULONG i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
                            {
                                if ((context->SystemAffinityMask >> i) & 0x1)
                                {
                                    if (differentAffinity) // Skip for multiple selection (dmex)
                                        continue;

                                    if ((context->AffinityMask >> i) & 0x1)
                                    {
                                        Button_SetCheck(context->CpuControlList->Items[i], BST_CHECKED);
                                    }
                                }
                            }
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

// Note: Workaround for UserNotes plugin dialog overrides (dmex)
NTSTATUS PhSetProcessItemAffinityMask(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ KAFFINITY AffinityMask
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION,
        ProcessItem->ProcessId
        );

    if (NT_SUCCESS(status))
    {
        status = PhSetProcessAffinityMask(processHandle, AffinityMask);
        NtClose(processHandle);
    }

    return status;
}

// Note: Workaround for UserNotes plugin dialog overrides (dmex)
NTSTATUS PhSetProcessItemPagePriority(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ ULONG PagePriority
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION,
        ProcessItem->ProcessId
        );

    if (NT_SUCCESS(status))
    {
        status = PhSetProcessPagePriority(processHandle, PagePriority);
        NtClose(processHandle);
    }

    return status;
}

// Note: Workaround for UserNotes plugin dialog overrides (dmex)
NTSTATUS PhSetProcessItemIoPriority(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ IO_PRIORITY_HINT IoPriority
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION,
        ProcessItem->ProcessId
        );

    if (NT_SUCCESS(status))
    {
        status = PhSetProcessIoPriority(processHandle, IoPriority);
        NtClose(processHandle);
    }

    return status;
}

// Note: Workaround for UserNotes plugin dialog overrides (dmex)
NTSTATUS PhSetProcessItemPriority(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ UCHAR PriorityClass
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION,
        ProcessItem->ProcessId
        );

    if (NT_SUCCESS(status))
    {
        status = PhSetProcessPriority(processHandle, PriorityClass);
        NtClose(processHandle);
    }

    return status;
}

// Note: Workaround for UserNotes plugin dialog overrides (dmex)
NTSTATUS PhSetProcessItemPriorityBoost(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ BOOLEAN PriorityBoost
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION,
        ProcessItem->ProcessId
        );

    if (NT_SUCCESS(status))
    {
        status = PhSetProcessPriorityBoost(processHandle, PriorityBoost);
        NtClose(processHandle);
    }

    return status;
}
