/*
 * Process Hacker -
 *   process mitigation policy details
 *
 * Copyright (C) 2016 wj32
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

typedef struct _MITIGATION_POLICY_ENTRY
{
    PPH_STRING ShortDescription;
    PPH_STRING LongDescription;
} MITIGATION_POLICY_ENTRY, *PMITIGATION_POLICY_ENTRY;

typedef struct _MITIGATION_POLICY_CONTEXT
{
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
    _In_ struct _PH_PROCESS_MITIGATION_POLICY_ALL_INFORMATION *Information
    )
{
    MITIGATION_POLICY_CONTEXT context;
    PROCESS_MITIGATION_POLICY policy;
    PPH_STRING shortDescription;
    PPH_STRING longDescription;

    memset(&context.Entries, 0, sizeof(context.Entries));

    for (policy = 0; policy < MaxProcessMitigationPolicy; policy++)
    {
        if (Information->Pointers[policy] && PhDescribeProcessMitigationPolicy(
            policy,
            Information->Pointers[policy],
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

INT_PTR CALLBACK PhpProcessMitigationPolicyDlgProc(
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
            PMITIGATION_POLICY_CONTEXT context = (PMITIGATION_POLICY_CONTEXT)lParam;
            HWND lvHandle;
            PROCESS_MITIGATION_POLICY policy;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
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

            ExtendedListView_SortItems(lvHandle);
            ListView_SetItemState(lvHandle, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
            SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)lvHandle, TRUE);
        }
        break;
    case WM_DESTROY:
        {
            // Nothing
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
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

                        SetDlgItemText(hwndDlg, IDC_DESCRIPTION, description);
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
