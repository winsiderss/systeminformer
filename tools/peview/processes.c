/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2019-2021 dmex
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

typedef struct _PVP_PE_PROCESSES_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HIMAGELIST ListViewImageList;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_PROCESSES_CONTEXT, *PPVP_PE_PROCESSES_CONTEXT;

VOID PvpPeEnumerateProcessIds(
    _In_ HWND ListViewHandle
    )
{
    HANDLE fileHandle;

    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);

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

    //ExtendedListView_SortItems(ListViewHandle);
    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvpPeProcessesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_PROCESSES_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_PROCESSES_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;
        }
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
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 150, L"PID");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 150, L"Name");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImagePidsListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvpPeEnumerateProcessIds(context->ListViewHandle);
            //ExtendedListView_SortItems(context->ListViewHandle);
            
            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImagePidsListViewColumns", context->ListViewHandle);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (context->PropSheetContext && !context->PropSheetContext->LayoutInitialized)
            {
                PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                context->PropSheetContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
        }
        break;
    }

    return FALSE;
}
