/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2016-2022
 *
 */

#include <phapp.h>
#include <phsettings.h>
#include <procmtgn.h>

typedef struct _MITIGATION_POLICY_ENTRY
{
    BOOLEAN NonStandard;
    PPH_STRING ShortDescription;
    PPH_STRING LongDescription;
} MITIGATION_POLICY_ENTRY, *PMITIGATION_POLICY_ENTRY;

typedef struct _MITIGATION_POLICY_CONTEXT
{
    HWND ListViewHandle;
    PPS_SYSTEM_DLL_INIT_BLOCK SystemDllInitBlock;
    MITIGATION_POLICY_ENTRY Entries[MaxProcessMitigationPolicy];
} MITIGATION_POLICY_CONTEXT, *PMITIGATION_POLICY_CONTEXT;

INT_PTR CALLBACK PhpProcessMitigationPolicyDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhShowProcessMitigationPolicyDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    PH_PROCESS_MITIGATION_POLICY_ALL_INFORMATION information;
    MITIGATION_POLICY_CONTEXT context;
    PROCESS_MITIGATION_POLICY policy;
    PPH_STRING shortDescription;
    PPH_STRING longDescription;

    memset(&context, 0, sizeof(MITIGATION_POLICY_CONTEXT));
    memset(&context.Entries, 0, sizeof(context.Entries));

    // Try to get a handle with query information + vm read access.
    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        ProcessId
        )))
    {
        // Try to get a handle with query information.
        status = PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_INFORMATION,
            ProcessId
            );
    }

    if (NT_SUCCESS(status))
    {
        PPS_SYSTEM_DLL_INIT_BLOCK dllInitBlock;

        if (NT_SUCCESS(PhGetProcessSystemDllInitBlock(processHandle, &dllInitBlock)))
        {
            context.SystemDllInitBlock = dllInitBlock;
        }

        if (NT_SUCCESS(PhGetProcessMitigationPolicy(processHandle, &information)))
        {
            for (policy = 0; policy < MaxProcessMitigationPolicy; policy++)
            {
                if (information.Pointers[policy] && PhDescribeProcessMitigationPolicy(
                    policy,
                    information.Pointers[policy],
                    &shortDescription,
                    &longDescription
                    ))
                {
                    context.Entries[policy].ShortDescription = shortDescription;
                    context.Entries[policy].LongDescription = longDescription;
                }
            }

            // HACK: Show System process CET mitigations (dmex)
            if (ProcessId == SYSTEM_PROCESS_ID)
            {
                SYSTEM_SHADOW_STACK_INFORMATION shadowStackInformation;

                if (NT_SUCCESS(PhGetSystemShadowStackInformation(&shadowStackInformation)))
                {
                    PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY policyData;

                    memset(&policyData, 0, sizeof(PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY));
                    policyData.EnableUserShadowStack = shadowStackInformation.KernelCetEnabled;
                    policyData.EnableUserShadowStackStrictMode = shadowStackInformation.KernelCetEnabled;
                    policyData.AuditUserShadowStack = shadowStackInformation.KernelCetAuditModeEnabled;

                    if (PhDescribeProcessMitigationPolicy(
                        ProcessUserShadowStackPolicy,
                        &policyData,
                        &shortDescription,
                        &longDescription
                        ))
                    {
                        context.Entries[ProcessUserShadowStackPolicy].ShortDescription = shortDescription;
                        context.Entries[ProcessUserShadowStackPolicy].LongDescription = longDescription;
                    }
                }
            }

            DialogBoxParam(
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_MITIGATION),
                ParentWindowHandle,
                PhpProcessMitigationPolicyDlgProc,
                (LPARAM)&context
                );

            for (policy = 0; policy < MaxProcessMitigationPolicy; policy++)
            {
                PhClearReference(&context.Entries[policy].ShortDescription);
                PhClearReference(&context.Entries[policy].LongDescription);
            }
        }

        if (context.SystemDllInitBlock)
            PhFree(context.SystemDllInitBlock);

        NtClose(processHandle);
    }
    else
    {
        PhShowStatus(ParentWindowHandle, L"Unable to open the process.", status, 0);
    }
}

INT_PTR CALLBACK PhpProcessMitigationPolicyDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PMITIGATION_POLICY_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PMITIGATION_POLICY_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;
            PROCESS_MITIGATION_POLICY policy;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            context->ListViewHandle = lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 350, L"Policy");
            PhSetExtendedListView(lvHandle);

            for (policy = 0; policy < MaxProcessMitigationPolicy; policy++)
            {
                PMITIGATION_POLICY_ENTRY entry = &context->Entries[policy];

                if (!entry->ShortDescription)
                    continue;

                PhAddListViewItem(lvHandle, MAXINT, entry->ShortDescription->Buffer, entry);
            }

            if (context->SystemDllInitBlock && RTL_CONTAINS_FIELD(context->SystemDllInitBlock, context->SystemDllInitBlock->Size, MitigationOptionsMap))
            {
                // TODO: Windows doesn't propagate these flags into the MitigationOptionsMap array. (dmex)
                //if (context->SystemDllInitBlock->MitigationOptionsMap.Map[0] & PROCESS_CREATION_MITIGATION_POLICY2_LOADER_INTEGRITY_CONTINUITY_ALWAYS_ON)
                //{
                //    PMITIGATION_POLICY_ENTRY entry;
                //
                //    entry = PhAllocate(sizeof(MITIGATION_POLICY_ENTRY));
                //    entry->NonStandard = TRUE;
                //    entry->ShortDescription = PhCreateString(L"Loader Integrity");
                //    entry->LongDescription = PhCreateString(L"OS signing levels for dependent module loads are enabled.");
                //
                //    PhAddListViewItem(lvHandle, MAXINT, entry->ShortDescription->Buffer, entry);
                //}

                //if (context->SystemDllInitBlock->MitigationOptionsMap.Map[0] & PROCESS_CREATION_MITIGATION_POLICY2_MODULE_TAMPERING_PROTECTION_ALWAYS_ON)
                //{
                //    PMITIGATION_POLICY_ENTRY entry;
                //
                //    entry = PhAllocate(sizeof(MITIGATION_POLICY_ENTRY));
                //    entry->NonStandard = TRUE;
                //    entry->ShortDescription = PhCreateString(L"Module Tampering");
                //    entry->LongDescription = PhCreateString(L"Module Tampering protection is enabled.");
                //
                //    PhAddListViewItem(lvHandle, MAXINT, entry->ShortDescription->Buffer, entry);
                //}

                if (context->SystemDllInitBlock->MitigationOptionsMap.Map[0] & PROCESS_CREATION_MITIGATION_POLICY2_RESTRICT_INDIRECT_BRANCH_PREDICTION_ALWAYS_ON)
                {
                    PMITIGATION_POLICY_ENTRY entry;

                    entry = PhAllocate(sizeof(MITIGATION_POLICY_ENTRY));
                    entry->NonStandard = TRUE;
                    entry->ShortDescription = PhCreateString(L"Indirect branch prediction");
                    entry->LongDescription = PhCreateString(L"Protects against sibling hardware threads (hyperthreads) from interfering with indirect branch predictions.");

                    PhAddListViewItem(lvHandle, MAXINT, entry->ShortDescription->Buffer, entry);
                }

                if (context->SystemDllInitBlock->MitigationOptionsMap.Map[0] & PROCESS_CREATION_MITIGATION_POLICY2_ALLOW_DOWNGRADE_DYNAMIC_CODE_POLICY_ALWAYS_ON)
                {
                    PMITIGATION_POLICY_ENTRY entry;

                    entry = PhAllocate(sizeof(MITIGATION_POLICY_ENTRY));
                    entry->NonStandard = TRUE;
                    entry->ShortDescription = PhCreateString(L"Dynamic code (downgrade)");
                    entry->LongDescription = PhCreateString(L"Allows a broker to downgrade the dynamic code policy for a process.");

                    PhAddListViewItem(lvHandle, MAXINT, entry->ShortDescription->Buffer, entry);
                }

                if (context->SystemDllInitBlock->MitigationOptionsMap.Map[0] & PROCESS_CREATION_MITIGATION_POLICY2_SPECULATIVE_STORE_BYPASS_DISABLE_ALWAYS_ON)
                {
                    PMITIGATION_POLICY_ENTRY entry;

                    entry = PhAllocate(sizeof(MITIGATION_POLICY_ENTRY));
                    entry->NonStandard = TRUE;
                    entry->ShortDescription = PhCreateString(L"Speculative store bypass");
                    entry->LongDescription = PhCreateString(L"Disables spectre mitigations for the process.");

                    PhAddListViewItem(lvHandle, MAXINT, entry->ShortDescription->Buffer, entry);
                }
            }

            ExtendedListView_SortItems(lvHandle);
            ExtendedListView_SetColumnWidth(lvHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
            ListView_SetItemState(lvHandle, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

            PhSetDialogFocus(hwndDlg, lvHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            ULONG index = -1;

            while ((index = PhFindListViewItemByFlags(
                context->ListViewHandle,
                index,
                LVNI_ALL
                )) != -1)
            {
                PMITIGATION_POLICY_ENTRY entry;

                if (PhGetListViewItemParam(context->ListViewHandle, index, &entry))
                {
                    if (entry->NonStandard)
                    {
                        PhClearReference(&entry->ShortDescription);
                        PhClearReference(&entry->LongDescription);
                        PhFree(entry);
                    }
                }
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;
            HWND lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

            switch (header->code)
            {
            case LVN_ITEMCHANGED:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        PWSTR description;

                        if (ListView_GetSelectedCount(lvHandle) == 1)
                            description = ((PMITIGATION_POLICY_ENTRY)PhGetSelectedListViewItemParam(lvHandle))->LongDescription->Buffer;
                        else
                            description = L"";

                        PhSetDialogItemText(hwndDlg, IDC_DESCRIPTION, description);
                    }
                }
                break;
            }

            PhHandleListViewNotifyForCopy(lParam, lvHandle);
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
