/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2020-2026
 *
 */

/*
 * The affinity dialog was originally created to support the modification
 * of process affinity masks, but now supports modifying thread affinity
 * and generic masks.
 */

#include <phapp.h>
#include <procprv.h>
#include <thrdprv.h>
#include <emenu.h>
#include <phsettings.h>
#include <settings.h>

KAFFINITY PhGetAffinityPresetMaskEx(
    _In_ USHORT Group,
    _In_ ULONG PresetId,
    _In_opt_ KAFFINITY CurrentMaskForSave
    );

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

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_AFFINITY),
        ParentWindowHandle,
        PhpProcessAffinityDlgProc,
        &context
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

    if (PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_AFFINITY),
        ParentWindowHandle,
        PhpProcessAffinityDlgProc,
        &context
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
        if (!NT_SUCCESS(PhOpenThread(
            &context.ThreadHandles[i],
            THREAD_QUERY_LIMITED_INFORMATION | THREAD_SET_LIMITED_INFORMATION | THREAD_SET_INFORMATION,
            Threads[i]->ThreadId
            )))
        {
            if (!NT_SUCCESS(PhOpenThread(
                &context.ThreadHandles[i],
                THREAD_QUERY_LIMITED_INFORMATION | THREAD_SET_LIMITED_INFORMATION,
                Threads[i]->ThreadId
                )))
            {
                context.ThreadHandles[i] = INVALID_HANDLE_VALUE;
            }
        }
    }

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_AFFINITY),
        ParentWindowHandle,
        PhpProcessAffinityDlgProc,
        &context
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
    _In_ PPH_LIST AffinityErrorsList
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 100);

    for (ULONG i = 0; i < AffinityErrorsList->Count; i++)
    {
        PhAppendFormatStringBuilder(
            &stringBuilder,
            L"%s\n",
            PhGetStringOrDefault(AffinityErrorsList->Items[i], L"An unknown error occurred.")
            );
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

    if (Context->ThreadHandles[0] != INVALID_HANDLE_VALUE)
    {
        if (NT_SUCCESS(PhGetThreadGroupAffinity(Context->ThreadHandles[0], &groupAffinity)))
        {
            lastAffinityMask = groupAffinity.Mask;
        }
    }

    for (ULONG i = 0; i < Context->NumberOfThreads; i++)
    {
        if (Context->ThreadHandles[i] == INVALID_HANDLE_VALUE)
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

                if (context->ThreadHandles[0] != INVALID_HANDLE_VALUE)
                {
                    // Use affinity from the first thread when all threads are identical (dmex)
                    status = PhGetThreadGroupAffinity(
                        context->ThreadHandles[0],
                        &groupAffinity
                        );
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL;
                }

                if (NT_SUCCESS(status))
                {
                    context->AffinityMask = groupAffinity.Mask;
                    context->AffinityGroup = groupAffinity.Group;

                    if (context->GroupComboHandle)
                    {
                        ComboBox_SetCurSel(context->GroupComboHandle, groupAffinity.Group);
                    }

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
                    if (context->ThreadHandles[i] != INVALID_HANDLE_VALUE)
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
                            affinityMask |= AFFINITY_MASK(i);
                    }

                    if (context->GroupComboHandle)
                    {
                         LONG affinityGroupSelection = ComboBox_GetCurSel(context->GroupComboHandle);

                         if (affinityGroupSelection == CB_ERR)
                             affinityGroup = context->AffinityGroup;
                         else
                             affinityGroup = (USHORT)affinityGroupSelection;
                    }

                    if (affinityMask == 0)
                    {
                        PhShowError2(hwndDlg, L"Unable to change affinity settings.", L"%s", L"You must select at least one CPU.");
                        break;
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
                        if (PhSystemProcessorInformation.SingleProcessorGroup)
                        {
                            HANDLE threadHandle;

                            status = PhOpenThread(
                                &threadHandle,
                                THREAD_SET_LIMITED_INFORMATION,
                                context->ThreadItem->ThreadId
                                );

                            if (NT_SUCCESS(status))
                            {
                                status = PhSetThreadAffinityMask(threadHandle, affinityMask);
                                NtClose(threadHandle);
                            }

                            if (!NT_SUCCESS(status))
                            {
                                PhpShowThreadErrorAffinity(hwndDlg, context->ThreadItem, status, 0);
                            }
                        }
                        else
                        {
                            HANDLE threadHandle;

                            status = PhOpenThread(
                                &threadHandle,
                                THREAD_SET_INFORMATION,
                                context->ThreadItem->ThreadId
                                );

                            if (NT_SUCCESS(status))
                            {
                                GROUP_AFFINITY groupAffinity;

                                memset(&groupAffinity, 0, sizeof(GROUP_AFFINITY));
                                groupAffinity.Group = affinityGroup;
                                groupAffinity.Mask = affinityMask;

                                status = PhSetThreadGroupAffinity(threadHandle, groupAffinity);
                                NtClose(threadHandle);
                            }

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
                            if (context->ThreadHandles[i] == INVALID_HANDLE_VALUE)
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
            case IDC_PRESETS:
                {
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM selectedItem;
                    POINT point;
                    RECT rect;

                    menu = PhCreateEMenu();
                    PhAddAffinityPresetsToEMenu(menu, context->AffinityGroup, TRUE);

                    GetWindowRect(GetDlgItem(hwndDlg, IDC_PRESETS), &rect);
                    point.x = rect.left;
                    point.y = rect.bottom;

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (selectedItem && selectedItem->Id)
                    {
                        if (selectedItem->Id >= ID_AFFINITY_SAVE_PRESET_1 && selectedItem->Id <= ID_AFFINITY_SAVE_PRESET_4)
                        {
                            KAFFINITY currentMask = 0;
                            for (ULONG i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
                            {
                                if (Button_GetCheck(context->CpuControlList->Items[i]) == BST_CHECKED)
                                    currentMask |= AFFINITY_MASK(i);
                            }
                            PhGetAffinityPresetMaskEx(context->AffinityGroup, selectedItem->Id, currentMask);
                        }
                        else
                        {
                            KAFFINITY mask = PhGetAffinityPresetMaskEx(context->AffinityGroup, selectedItem->Id, 0);

                            if (mask != 0)
                            {
                                for (ULONG i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
                                {
                                    HWND checkBox = context->CpuControlList->Items[i];

                                    if (IsWindowEnabled(checkBox))
                                    {
                                        if ((mask >> i) & 1)
                                            Button_SetCheck(checkBox, BST_CHECKED);
                                        else
                                            Button_SetCheck(checkBox, BST_UNCHECKED);
                                    }
                                }
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }
                break;
            case IDC_GROUPCPU:
                {
                    LONG index;

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
        status = PhSetProcessPriorityClass(processHandle, PriorityClass);
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

// Note: Workaround for UserNotes plugin dialog overrides (dmex)
NTSTATUS PhSetProcessItemThrottlingState(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ BOOLEAN ClearThrottlingState
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
        if (ClearThrottlingState)
        {
            PhSetProcessPriorityClass(processHandle, PROCESS_PRIORITY_CLASS_NORMAL);

            status = PhSetProcessPowerThrottlingState(processHandle, 0, 0);
        }
        else
        {
            // Taskmgr sets the process priority to idle before enabling 'Eco mode'. (dmex)
            PhSetProcessPriorityClass(processHandle, PROCESS_PRIORITY_CLASS_IDLE);

            status = PhSetProcessPowerThrottlingState(
                processHandle,
                POWER_THROTTLING_PROCESS_EXECUTION_SPEED,
                POWER_THROTTLING_PROCESS_EXECUTION_SPEED
                );
        }

        NtClose(processHandle);
    }

    return status;
}

BOOLEAN PhGetSystemProcessorEfficiencyMasks(
    _In_ USHORT Group,
    _Out_ PKAFFINITY PerformanceMask,
    _Out_ PKAFFINITY EfficiencyMask
    )
{
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX logicalInformation = NULL;
    ULONG bufferLength = 0;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processorInfo = NULL;
    UCHAR maxEfficiencyClass = 0;
    UCHAR minEfficiencyClass = 0xFF;
    BOOLEAN hasEfficiencyClasses = FALSE;
    KAFFINITY systemAffinityMask = 0;
    KAFFINITY perfMask = 0;
    KAFFINITY effMask = 0;

    *PerformanceMask = 0;
    *EfficiencyMask = 0;

    if (!NT_SUCCESS(PhGetProcessorGroupActiveAffinityMask(Group, &systemAffinityMask)))
    {
        if (!NT_SUCCESS(PhGetProcessorSystemAffinityMask(&systemAffinityMask)))
            return FALSE;
    }

    if (!NT_SUCCESS(PhGetSystemLogicalProcessorInformation(RelationProcessorCore, &logicalInformation, &bufferLength)))
    {
        *PerformanceMask = systemAffinityMask;
        return TRUE;
    }

    for (
        processorInfo = logicalInformation;
        (ULONG_PTR)processorInfo < (ULONG_PTR)PTR_ADD_OFFSET(logicalInformation, bufferLength);
        processorInfo = PTR_ADD_OFFSET(processorInfo, processorInfo->Size)
        )
    {
        if (processorInfo->Relationship == RelationProcessorCore)
        {
            if (processorInfo->Processor.EfficiencyClass > maxEfficiencyClass)
                maxEfficiencyClass = processorInfo->Processor.EfficiencyClass;
            if (processorInfo->Processor.EfficiencyClass < minEfficiencyClass)
                minEfficiencyClass = processorInfo->Processor.EfficiencyClass;
        }
    }

    if (maxEfficiencyClass > minEfficiencyClass)
        hasEfficiencyClasses = TRUE;

    for (
        processorInfo = logicalInformation;
        (ULONG_PTR)processorInfo < (ULONG_PTR)PTR_ADD_OFFSET(logicalInformation, bufferLength);
        processorInfo = PTR_ADD_OFFSET(processorInfo, processorInfo->Size)
        )
    {
        if (processorInfo->Relationship == RelationProcessorCore)
        {
            for (USHORT j = 0; j < processorInfo->Processor.GroupCount; j++)
            {
                if (processorInfo->Processor.GroupMask[j].Group == Group)
                {
                    if (hasEfficiencyClasses && processorInfo->Processor.EfficiencyClass < maxEfficiencyClass)
                    {
                        effMask |= processorInfo->Processor.GroupMask[j].Mask;
                    }
                    else
                    {
                        perfMask |= processorInfo->Processor.GroupMask[j].Mask;
                    }
                }
            }
        }
    }

    PhFree(logicalInformation);

    perfMask &= systemAffinityMask;
    effMask &= systemAffinityMask;

    if (perfMask == 0)
        perfMask = systemAffinityMask;

    *PerformanceMask = perfMask;
    *EfficiencyMask = effMask;
    return TRUE;
}

KAFFINITY PhGetAffinityPresetMaskEx(
    _In_ USHORT Group,
    _In_ ULONG PresetId,
    _In_opt_ KAFFINITY CurrentMaskForSave
    )
{
    KAFFINITY systemAffinityMask = 0;
    KAFFINITY perfMask = 0;
    KAFFINITY effMask = 0;

    if (!NT_SUCCESS(PhGetProcessorGroupActiveAffinityMask(Group, &systemAffinityMask)))
    {
        if (!NT_SUCCESS(PhGetProcessorSystemAffinityMask(&systemAffinityMask)))
            return 0;
    }

    if (PresetId >= ID_AFFINITY_CCD_FIRST && PresetId <= ID_AFFINITY_CCD_LAST)
    {
        ULONG ccdIndex = PresetId - ID_AFFINITY_CCD_FIRST;
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX logicalInformation = NULL;
        ULONG bufferLength = 0;
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processorInfo = NULL;
        ULONG currentIndex = 0;
        KAFFINITY ccdMask = 0;

        if (NT_SUCCESS(PhGetSystemLogicalProcessorInformation(RelationCache, &logicalInformation, &bufferLength)))
        {
            for (
                processorInfo = logicalInformation;
                (ULONG_PTR)processorInfo < (ULONG_PTR)PTR_ADD_OFFSET(logicalInformation, bufferLength);
                processorInfo = PTR_ADD_OFFSET(processorInfo, processorInfo->Size)
                )
            {
                if (processorInfo->Relationship == RelationCache && processorInfo->Cache.Level == 3)
                {
                    if (currentIndex == ccdIndex)
                    {
                        if (processorInfo->Cache.GroupMask.Group == Group)
                        {
                            ccdMask |= processorInfo->Cache.GroupMask.Mask;
                        }
                        break;
                    }
                    currentIndex++;
                }
            }
            PhFree(logicalInformation);
        }
        return ccdMask;
    }
    else if (PresetId >= ID_AFFINITY_NUMA_FIRST && PresetId <= ID_AFFINITY_NUMA_LAST)
    {
        ULONG numaIndex = PresetId - ID_AFFINITY_NUMA_FIRST;
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX logicalInformation = NULL;
        ULONG bufferLength = 0;
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processorInfo = NULL;
        ULONG currentIndex = 0;
        KAFFINITY numaMask = 0;

        if (NT_SUCCESS(PhGetSystemLogicalProcessorInformation(RelationNumaNode, &logicalInformation, &bufferLength)))
        {
            for (
                processorInfo = logicalInformation;
                (ULONG_PTR)processorInfo < (ULONG_PTR)PTR_ADD_OFFSET(logicalInformation, bufferLength);
                processorInfo = PTR_ADD_OFFSET(processorInfo, processorInfo->Size)
                )
            {
                if (processorInfo->Relationship == RelationNumaNode)
                {
                    if (currentIndex == numaIndex)
                    {
                        if (processorInfo->NumaNode.GroupMask.Group == Group)
                        {
                            numaMask |= processorInfo->NumaNode.GroupMask.Mask;
                        }
                        break;
                    }
                    currentIndex++;
                }
            }
            PhFree(logicalInformation);
        }
        return numaMask;
    }
    else if (PresetId >= ID_AFFINITY_CUSTOM_1 && PresetId <= ID_AFFINITY_CUSTOM_4)
    {
        ULONG index = PresetId - ID_AFFINITY_CUSTOM_1 + 1;
        PH_FORMAT format[2];
        WCHAR settingName[64];
        PhInitFormatS(&format[0], L"AffinityPresetMask");
        PhInitFormatU(&format[1], index);
        PhFormatToBuffer(format, RTL_NUMBER_OF(format), settingName, sizeof(settingName), NULL);
        return (KAFFINITY)PhGetIntegerSetting(settingName);
    }
    else if (PresetId >= ID_AFFINITY_SAVE_PRESET_1 && PresetId <= ID_AFFINITY_SAVE_PRESET_4)
    {
        ULONG index = PresetId - ID_AFFINITY_SAVE_PRESET_1 + 1;
        PH_FORMAT format[2];
        WCHAR settingName[64];
        PhInitFormatS(&format[0], L"AffinityPresetMask");
        PhInitFormatU(&format[1], index);
        PhFormatToBuffer(format, RTL_NUMBER_OF(format), settingName, sizeof(settingName), NULL);
        PhSetIntegerSetting(settingName, (ULONG)CurrentMaskForSave);
        return 0;
    }
    else if (PresetId >= ID_AFFINITY_CLEAR_PRESET_1 && PresetId <= ID_AFFINITY_CLEAR_PRESET_4)
    {
        ULONG index = PresetId - ID_AFFINITY_CLEAR_PRESET_1 + 1;
        PH_FORMAT format[2];
        WCHAR settingName[64];
        PhInitFormatS(&format[0], L"AffinityPresetMask");
        PhInitFormatU(&format[1], index);
        PhFormatToBuffer(format, RTL_NUMBER_OF(format), settingName, sizeof(settingName), NULL);
        PhSetIntegerSetting(settingName, 0);
        return 0;
    }
    else if (PresetId == ID_AFFINITY_CLEAR_ALL_PRESETS)
    {
        for (ULONG i = 1; i <= 4; i++)
        {
            PH_FORMAT format[2];
            WCHAR settingName[64];
            PhInitFormatS(&format[0], L"AffinityPresetMask");
            PhInitFormatU(&format[1], i);
            PhFormatToBuffer(format, RTL_NUMBER_OF(format), settingName, sizeof(settingName), NULL);
            PhSetIntegerSetting(settingName, 0);
        }
        return 0;
    }

    switch (PresetId)
    {
    case ID_AFFINITY_ALL:
        return systemAffinityMask;
    case ID_AFFINITY_PCORES:
        PhGetSystemProcessorEfficiencyMasks(Group, &perfMask, &effMask);
        return perfMask != 0 ? perfMask : systemAffinityMask;
    case ID_AFFINITY_ECORES:
        PhGetSystemProcessorEfficiencyMasks(Group, &perfMask, &effMask);
        return effMask != 0 ? effMask : systemAffinityMask;
    case ID_AFFINITY_EVEN:
        return (KAFFINITY)0x5555555555555555ULL & systemAffinityMask;
    case ID_AFFINITY_ODD:
        return (KAFFINITY)0xAAAAAAAAAAAAAAAAULL & systemAffinityMask;
    case ID_AFFINITY_FIRSTHALF:
        {
            ULONG activeCount = PhCountBitsUlongPtr(systemAffinityMask);
            ULONG halfCount = activeCount / 2;
            KAFFINITY mask = 0;
            ULONG count = 0;
            for (ULONG i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
            {
                if ((systemAffinityMask >> i) & 1)
                {
                    mask |= AFFINITY_MASK(i);
                    count++;
                    if (count >= halfCount && halfCount > 0)
                        break;
                }
            }
            return mask;
        }
    case ID_AFFINITY_SECONDHALF:
        {
            ULONG activeCount = PhCountBitsUlongPtr(systemAffinityMask);
            ULONG halfCount = activeCount / 2;
            KAFFINITY mask = 0;
            ULONG count = 0;
            for (ULONG i = 0; i < MAXIMUM_PROC_PER_GROUP; i++)
            {
                if ((systemAffinityMask >> i) & 1)
                {
                    count++;
                    if (count > halfCount)
                    {
                        mask |= AFFINITY_MASK(i);
                    }
                }
            }
            return mask;
        }
    }

    return systemAffinityMask;
}

VOID PhAddAffinityPresetsToEMenu(
    _In_ PPH_EMENU_ITEM Menu,
    _In_ USHORT Group,
    _In_ BOOLEAN IncludeSaveItems
    )
{
    KAFFINITY perfMask = 0, effMask = 0;
    BOOLEAN hasHybrid = PhGetSystemProcessorEfficiencyMasks(Group, &perfMask, &effMask) && effMask != 0;

    PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_AFFINITY_ALL, L"&All cores", NULL, NULL), ULONG_MAX);

    if (hasHybrid)
    {
        PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_AFFINITY_PCORES, L"&Performance cores (P-Cores)", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_AFFINITY_ECORES, L"&Efficiency cores (E-Cores)", NULL, NULL), ULONG_MAX);
    }

    // CCDs (Core Complex Dies / L3 Caches)
    {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX logicalInformation = NULL;
        ULONG bufferLength = 0;
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processorInfo = NULL;
        ULONG ccdCount = 0;

        if (NT_SUCCESS(PhGetSystemLogicalProcessorInformation(RelationCache, &logicalInformation, &bufferLength)))
        {
            for (
                processorInfo = logicalInformation;
                (ULONG_PTR)processorInfo < (ULONG_PTR)PTR_ADD_OFFSET(logicalInformation, bufferLength);
                processorInfo = PTR_ADD_OFFSET(processorInfo, processorInfo->Size)
                )
            {
                if (processorInfo->Relationship == RelationCache && processorInfo->Cache.Level == 3)
                {
                    ccdCount++;
                }
            }

            if (ccdCount > 1)
            {
                PhInsertEMenuItem(Menu, PhCreateEMenuSeparator(), ULONG_MAX);

                ULONG index = 0;
                for (
                    processorInfo = logicalInformation;
                    (ULONG_PTR)processorInfo < (ULONG_PTR)PTR_ADD_OFFSET(logicalInformation, bufferLength);
                    processorInfo = PTR_ADD_OFFSET(processorInfo, processorInfo->Size)
                    )
                {
                    if (processorInfo->Relationship == RelationCache && processorInfo->Cache.Level == 3)
                    {
                        if (index <= (ID_AFFINITY_CCD_LAST - ID_AFFINITY_CCD_FIRST))
                        {
                            PH_FORMAT format[5];
                            WCHAR label[64];
                            PhInitFormatS(&format[0], L"CCD &");
                            PhInitFormatU(&format[1], index);
                            PhInitFormatS(&format[2], L" (L3 Cluster ");
                            PhInitFormatU(&format[3], index);
                            PhInitFormatS(&format[4], L")");
                            PhFormatToBuffer(format, RTL_NUMBER_OF(format), label, sizeof(label), NULL);
                            PhInsertEMenuItem(Menu, PhCreateEMenuItem(PH_EMENU_TEXT_OWNED, ID_AFFINITY_CCD_FIRST + index, PhAllocateCopy(label, sizeof(label)), NULL, NULL), ULONG_MAX);
                        }
                        index++;
                    }
                }
            }

            PhFree(logicalInformation);
        }
    }

    // NUMA Nodes
    {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX logicalInformation = NULL;
        ULONG bufferLength = 0;
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processorInfo = NULL;
        ULONG numaCount = 0;

        if (NT_SUCCESS(PhGetSystemLogicalProcessorInformation(RelationNumaNode, &logicalInformation, &bufferLength)))
        {
            for (
                processorInfo = logicalInformation;
                (ULONG_PTR)processorInfo < (ULONG_PTR)PTR_ADD_OFFSET(logicalInformation, bufferLength);
                processorInfo = PTR_ADD_OFFSET(processorInfo, processorInfo->Size)
                )
            {
                if (processorInfo->Relationship == RelationNumaNode)
                {
                    numaCount++;
                }
            }

            if (numaCount > 1)
            {
                PhInsertEMenuItem(Menu, PhCreateEMenuSeparator(), ULONG_MAX);

                ULONG index = 0;
                for (
                    processorInfo = logicalInformation;
                    (ULONG_PTR)processorInfo < (ULONG_PTR)PTR_ADD_OFFSET(logicalInformation, bufferLength);
                    processorInfo = PTR_ADD_OFFSET(processorInfo, processorInfo->Size)
                    )
                {
                    if (processorInfo->Relationship == RelationNumaNode)
                    {
                        if (index <= (ID_AFFINITY_NUMA_LAST - ID_AFFINITY_NUMA_FIRST))
                        {
                            PH_FORMAT format[2];
                            WCHAR label[64];
                            PhInitFormatS(&format[0], L"NUMA Node &");
                            PhInitFormatU(&format[1], (ULONG)processorInfo->NumaNode.NodeNumber);
                            PhFormatToBuffer(format, RTL_NUMBER_OF(format), label, sizeof(label), NULL);
                            PhInsertEMenuItem(Menu, PhCreateEMenuItem(PH_EMENU_TEXT_OWNED, ID_AFFINITY_NUMA_FIRST + index, PhAllocateCopy(label, sizeof(label)), NULL, NULL), ULONG_MAX);
                        }
                        index++;
                    }
                }
            }

            PhFree(logicalInformation);
        }
    }

    PhInsertEMenuItem(Menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_AFFINITY_EVEN, L"E&ven cores", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_AFFINITY_ODD, L"O&dd cores", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_AFFINITY_FIRSTHALF, L"&First half of cores", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(Menu, PhCreateEMenuItem(0, ID_AFFINITY_SECONDHALF, L"&Second half of cores", NULL, NULL), ULONG_MAX);

    // Custom Presets
    {
        BOOLEAN hasCustomPresets = FALSE;
        for (ULONG i = 1; i <= 4; i++)
        {
            PH_FORMAT formatMask[2];
            PH_FORMAT formatName[2];
            WCHAR settingMaskName[64];
            WCHAR settingName[64];
            PhInitFormatS(&formatMask[0], L"AffinityPresetMask");
            PhInitFormatU(&formatMask[1], i);
            PhFormatToBuffer(formatMask, RTL_NUMBER_OF(formatMask), settingMaskName, sizeof(settingMaskName), NULL);

            PhInitFormatS(&formatName[0], L"AffinityPresetName");
            PhInitFormatU(&formatName[1], i);
            PhFormatToBuffer(formatName, RTL_NUMBER_OF(formatName), settingName, sizeof(settingName), NULL);

            KAFFINITY customMask = (KAFFINITY)PhGetIntegerSetting(settingMaskName);
            if (customMask != 0)
            {
                if (!hasCustomPresets)
                {
                    PhInsertEMenuItem(Menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    hasCustomPresets = TRUE;
                }
                PPH_STRING nameString = PhGetStringSetting(settingName);
                PH_FORMAT formatLabel[4];
                WCHAR label[128];
                PhInitFormatS(&formatLabel[0], L"Preset ");
                PhInitFormatU(&formatLabel[1], i);
                PhInitFormatS(&formatLabel[2], L": ");
                PhInitFormatSR(&formatLabel[3], nameString->sr);
                PhFormatToBuffer(formatLabel, RTL_NUMBER_OF(formatLabel), label, sizeof(label), NULL);
                PhDereferenceObject(nameString);
                PhInsertEMenuItem(Menu, PhCreateEMenuItem(PH_EMENU_TEXT_OWNED, ID_AFFINITY_CUSTOM_1 + (i - 1), PhAllocateCopy(label, sizeof(label)), NULL, NULL), ULONG_MAX);
            }
        }

        if (IncludeSaveItems)
        {
            PhInsertEMenuItem(Menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PPH_EMENU_ITEM saveMenu = PhCreateEMenuItem(0, 0, L"Save selection as preset...", NULL, NULL);
            for (ULONG i = 1; i <= 4; i++)
            {
                PH_FORMAT formatSave[2];
                WCHAR label[64];
                PhInitFormatS(&formatSave[0], L"Save as Preset ");
                PhInitFormatU(&formatSave[1], i);
                PhFormatToBuffer(formatSave, RTL_NUMBER_OF(formatSave), label, sizeof(label), NULL);
                PhInsertEMenuItem(saveMenu, PhCreateEMenuItem(PH_EMENU_TEXT_OWNED, ID_AFFINITY_SAVE_PRESET_1 + (i - 1), PhAllocateCopy(label, sizeof(label)), NULL, NULL), ULONG_MAX);
            }
            PhInsertEMenuItem(Menu, saveMenu, ULONG_MAX);

            if (hasCustomPresets)
            {
                PPH_EMENU_ITEM clearMenu = PhCreateEMenuItem(0, 0, L"Clear preset...", NULL, NULL);
                for (ULONG i = 1; i <= 4; i++)
                {
                    PH_FORMAT formatMask[2];
                    WCHAR settingMaskName[64];
                    PhInitFormatS(&formatMask[0], L"AffinityPresetMask");
                    PhInitFormatU(&formatMask[1], i);
                    PhFormatToBuffer(formatMask, RTL_NUMBER_OF(formatMask), settingMaskName, sizeof(settingMaskName), NULL);

                    if ((KAFFINITY)PhGetIntegerSetting(settingMaskName) != 0)
                    {
                        PH_FORMAT formatClear[2];
                        WCHAR label[64];
                        PhInitFormatS(&formatClear[0], L"Clear Preset ");
                        PhInitFormatU(&formatClear[1], i);
                        PhFormatToBuffer(formatClear, RTL_NUMBER_OF(formatClear), label, sizeof(label), NULL);
                        PhInsertEMenuItem(clearMenu, PhCreateEMenuItem(PH_EMENU_TEXT_OWNED, ID_AFFINITY_CLEAR_PRESET_1 + (i - 1), PhAllocateCopy(label, sizeof(label)), NULL, NULL), ULONG_MAX);
                    }
                }
                PhInsertEMenuItem(clearMenu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(clearMenu, PhCreateEMenuItem(0, ID_AFFINITY_CLEAR_ALL_PRESETS, L"Clear all presets", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(Menu, clearMenu, ULONG_MAX);
            }
        }
    }
}

VOID PhUiSetAffinityPresetProcesses(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses,
    _In_ ULONG PresetId
    )
{
    for (ULONG i = 0; i < NumberOfProcesses; i++)
    {
        PPH_PROCESS_ITEM processItem = Processes[i];
        HANDLE processHandle;
        GROUP_AFFINITY groupAffinity = { 0 };
        USHORT group = 0;

        if (NT_SUCCESS(PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_INFORMATION,
            processItem->ProcessId
            )))
        {
            if (NT_SUCCESS(PhGetProcessGroupAffinity(processHandle, &groupAffinity)))
            {
                group = groupAffinity.Group;
            }

            KAFFINITY currentProcessMask = 0;
            if (PresetId >= ID_AFFINITY_SAVE_PRESET_1 && PresetId <= ID_AFFINITY_SAVE_PRESET_4)
            {
                currentProcessMask = groupAffinity.Mask;
            }

            KAFFINITY presetMask = PhGetAffinityPresetMaskEx(group, PresetId, currentProcessMask);

            if (presetMask != 0)
            {
                if (PhSystemProcessorInformation.SingleProcessorGroup)
                {
                    PhSetProcessAffinityMask(processHandle, presetMask);
                }
                else
                {
                    groupAffinity.Group = group;
                    groupAffinity.Mask = presetMask;
                    PhSetProcessGroupAffinity(processHandle, groupAffinity);
                }
            }

            NtClose(processHandle);
        }
    }
}
