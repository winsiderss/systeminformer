/*
 * Process Hacker - 
 *   pagefiles viewer
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

#include <phgui.h>

INT_PTR CALLBACK PhpPagefilesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowPagefilesDialog(
    __in HWND ParentWindowHandle
    )
{
    NTSTATUS status;
    PSYSTEM_PAGEFILE_INFORMATION pagefiles;

    if (!NT_SUCCESS(status = PhEnumPagefiles(&pagefiles)))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to get pagefile information", status, 0);
        return;
    }

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PAGEFILES),
        ParentWindowHandle,
        PhpPagefilesDlgProc,
        (LPARAM)pagefiles
        );

    PhFree(pagefiles);
}

INT_PTR CALLBACK PhpPagefilesDlgProc(
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
            PVOID pagefiles;
            PSYSTEM_PAGEFILE_INFORMATION pagefile;
            HWND lvHandle;

            pagefiles = (PSYSTEM_PAGEFILE_INFORMATION)lParam;
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 120, L"File name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Usage");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"Peak usage");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Total");
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");

            pagefile = PH_FIRST_PAGEFILE(pagefiles);

            while (pagefile)
            {
                INT lvItemIndex;
                PPH_STRING fileName;
                PPH_STRING newFileName;
                PPH_STRING usage;

                fileName = PhCreateStringEx(pagefile->PageFileName.Buffer, pagefile->PageFileName.Length);
                newFileName = PhGetFileName(fileName);
                PhDereferenceObject(fileName);

                lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, newFileName->Buffer, pagefile);

                // Usage
                usage = PhFormatSize(UInt32x32To64(pagefile->TotalInUse, PAGE_SIZE), -1);
                PhSetListViewSubItem(lvHandle, lvItemIndex, 1, usage->Buffer);
                PhDereferenceObject(usage);

                // Peak usage
                usage = PhFormatSize(UInt32x32To64(pagefile->PeakUsage, PAGE_SIZE), -1);
                PhSetListViewSubItem(lvHandle, lvItemIndex, 2, usage->Buffer);
                PhDereferenceObject(usage);

                // Total
                usage = PhFormatSize(UInt32x32To64(pagefile->TotalSize, PAGE_SIZE), -1);
                PhSetListViewSubItem(lvHandle, lvItemIndex, 3, usage->Buffer);
                PhDereferenceObject(usage);

                PhDereferenceObject(newFileName);

                pagefile = PH_NEXT_PAGEFILE(pagefile);
            }

            SetFocus(GetDlgItem(hwndDlg, IDOK));
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
    }

    return FALSE;
}
