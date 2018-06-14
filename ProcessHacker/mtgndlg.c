/*
 * Process Hacker -
 *   process mitigation policy details
 *
 * Copyright (C) 2016 wj32
 * Copyright (C) 2016-2018 dmex
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
#include <procmtgn.h>
#include <windowsx.h>

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

NTSTATUS PhpGetProcessSystemDllInitBlock(
    _In_ HANDLE ProcessHandle,
    _Out_ PPS_SYSTEM_DLL_INIT_BLOCK *SystemDllInitBlock
    )
{
    NTSTATUS status;
    PH_STRINGREF systemRoot;
    PVOID ldrInitBlockBaseAddress = NULL;
    PPH_STRING ntdllFileName;

    PhGetSystemRoot(&systemRoot);
    ntdllFileName = PhConcatStringRefZ(&systemRoot, L"\\System32\\ntdll.dll");

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        ntdllFileName->Buffer,
        "LdrSystemDllInitBlock",
        0,
        &ldrInitBlockBaseAddress,
        NULL
        );

    PhDereferenceObject(ntdllFileName);

    if (NT_SUCCESS(status) && ldrInitBlockBaseAddress)
    {
        PPS_SYSTEM_DLL_INIT_BLOCK ldrInitBlock;

        ldrInitBlock = PhAllocate(sizeof(PS_SYSTEM_DLL_INIT_BLOCK));
        memset(ldrInitBlock, 0, sizeof(PS_SYSTEM_DLL_INIT_BLOCK));

        status = NtReadVirtualMemory(
            ProcessHandle,
            ldrInitBlockBaseAddress,
            ldrInitBlock,
            sizeof(PS_SYSTEM_DLL_INIT_BLOCK),
            NULL
            );

        if (NT_SUCCESS(status))
            *SystemDllInitBlock = ldrInitBlock;
        else
            PhFree(ldrInitBlock);
    }

    return status;
}

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

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        ProcessId
        )))
    {
        PPS_SYSTEM_DLL_INIT_BLOCK dllInitBlock;

        if (NT_SUCCESS(PhpGetProcessSystemDllInitBlock(processHandle, &dllInitBlock)))
        {
            context.SystemDllInitBlock = dllInitBlock;
        }

        NtClose(processHandle);
    }

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION,
        ProcessId
        )))
    {
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
        PhShowStatus(ParentWindowHandle, L"Unable to open the process", status, 0);
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
                if (context->SystemDllInitBlock->MitigationOptionsMap.Map[1] & PROCESS_CREATION_MITIGATION_POLICY2_LOADER_INTEGRITY_CONTINUITY_ALWAYS_ON)
                {
                    PMITIGATION_POLICY_ENTRY entry;

                    entry = PhAllocate(sizeof(MITIGATION_POLICY_ENTRY));
                    entry->NonStandard = TRUE;
                    entry->ShortDescription = PhCreateString(L"Loader Integrity");
                    entry->LongDescription = PhCreateString(L"OS signing levels for depenedent module loads are enabled.");

                    PhAddListViewItem(lvHandle, MAXINT, entry->ShortDescription->Buffer, entry);
                }

                if (context->SystemDllInitBlock->MitigationOptionsMap.Map[1] & PROCESS_CREATION_MITIGATION_POLICY2_MODULE_TAMPERING_PROTECTION_ALWAYS_ON)
                {
                    PMITIGATION_POLICY_ENTRY entry;

                    entry = PhAllocate(sizeof(MITIGATION_POLICY_ENTRY));
                    entry->NonStandard = TRUE;
                    entry->ShortDescription = PhCreateString(L"Module Tampering");
                    entry->LongDescription = PhCreateString(L"Module Tampering protection is enabled.");

                    PhAddListViewItem(lvHandle, MAXINT, entry->ShortDescription->Buffer, entry);
                }

                // Note: This value doesn't appear to be available via the MitigationOptionsMap.
                //if (context->SystemDllInitBlock->MitigationOptionsMap.Map[1] & PROCESS_CREATION_MITIGATION_POLICY2_RESTRICT_INDIRECT_BRANCH_PREDICTION_ALWAYS_ON)
                //{
                //    PMITIGATION_POLICY_ENTRY entry;
                //
                //    entry = PhAllocate(sizeof(MITIGATION_POLICY_ENTRY));
                //    entry->NonStandard = TRUE;
                //    entry->ShortDescription = PhCreateString(L"Indirect branch prediction");
                //    entry->LongDescription = PhCreateString(L"Protects against sibling hardware threads (hyperthreads) from interfering with indirect branch predictions.");
                //
                //    PhAddListViewItem(lvHandle, MAXINT, entry->ShortDescription->Buffer, entry);
                //}
            }

            ExtendedListView_SortItems(lvHandle);
            ExtendedListView_SetColumnWidth(lvHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
            ListView_SetItemState(lvHandle, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

            PhSetDialogFocus(hwndDlg, lvHandle);
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
    }

    return FALSE;
}
