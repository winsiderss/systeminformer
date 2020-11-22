/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2019-2020 dmex
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

#include <peview.h>

VOID PvpPeEnumerateProcessIds(
    _In_ HWND ListViewHandle
    )
{
    HANDLE fileHandle;

    if (NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        PhGetString(PvFileName),
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        PFILE_PROCESS_IDS_USING_FILE_INFORMATION processIds;
        ULONG count = 0;
        ULONG i;

        if (NT_SUCCESS(PhGetProcessIdsUsingFile(
            fileHandle,
            &processIds
            )))
        {
            for (i = 0; i < processIds->NumberOfProcessIdsInList; i++)
            {
                INT lvItemIndex;
                HANDLE processId;
                PPH_STRING fileName = NULL;
                WCHAR number[PH_INT32_STR_LEN_1];

                processId = (HANDLE)processIds->ProcessIdList[i];

                if (processId == SYSTEM_PROCESS_ID)
                    fileName = PhGetKernelFileName();
                else
                    PhGetProcessImageFileNameByProcessId(processId, &fileName);

                if (fileName)
                    PhMoveReference(&fileName, PhGetFileName(fileName));

                PhPrintUInt32(number, ++count);
                lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, number, NULL);
                PhPrintUInt32(number, HandleToUlong(processId));
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, number);
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetStringOrEmpty(fileName));

                if (fileName)
                    PhDereferenceObject(fileName);
            }

            PhFree(processIds);
        }

        NtClose(fileHandle);
    }
}

INT_PTR CALLBACK PvpPeProcessesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 150, L"PID");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 150, L"Name");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"ImagePidsListViewColumns", lvHandle);

            PvpPeEnumerateProcessIds(lvHandle);
            //ExtendedListView_SortItems(lvHandle);
            
            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImagePidsListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST), dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    }

    return FALSE;
}
