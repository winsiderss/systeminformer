/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2019 dmex
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

VOID PvpPeEnumerateFileLinks(
    _In_ HWND ListViewHandle
    )
{
    HANDLE fileHandle;
    ULONG count = 0;

    if (NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        PhGetString(PvFileName),
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT
        )))
    {
        PFILE_LINKS_INFORMATION hardlinks;
        PFILE_LINK_ENTRY_INFORMATION i;

        if (NT_SUCCESS(PhEnumFileHardLinks(
            fileHandle,
            &hardlinks
            )))
        {
            for (i = PH_FIRST_LINK(&hardlinks->Entry); i; i = PH_NEXT_LINK(i))
            {
                NTSTATUS status;
                HANDLE linkHandle;
                UNICODE_STRING fileNameUs;
                OBJECT_ATTRIBUTES oa;
                IO_STATUS_BLOCK isb;

                fileNameUs.Length = sizeof(LONGLONG);
                fileNameUs.MaximumLength = sizeof(LONGLONG);
                fileNameUs.Buffer = (PWSTR)&i->ParentFileId;

                InitializeObjectAttributes(
                    &oa,
                    &fileNameUs,
                    OBJ_CASE_INSENSITIVE,
                    fileHandle,
                    NULL
                    );

                status = NtCreateFile(
                    &linkHandle,
                    FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                    &oa,
                    &isb,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    0,
                    FILE_OPEN,
                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_BY_FILE_ID,
                    NULL,
                    0
                    );

                if (NT_SUCCESS(status))
                {
                    INT lvItemIndex;
                    PPH_STRING parentName;
                    WCHAR number[PH_PTR_STR_LEN_1];

                    PhPrintUInt32(number, ++count);
                    lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, number, NULL);

                    if (NT_SUCCESS(PhGetFileHandleName(linkHandle, &parentName)))
                    {
                        PPH_STRING fileName = NULL;
                        PPH_STRING baseName;
                        PH_STRINGREF pathPart;
                        PH_STRINGREF baseNamePart;

                        baseName = PhGetBaseName(PvFileName);

                        if (PhSplitStringRefAtChar(&PvFileName->sr, OBJ_NAME_PATH_SEPARATOR, &pathPart, &baseNamePart))
                        {
                            PhMoveReference(&fileName, PhCreateString2(&pathPart));
                            PhMoveReference(&fileName, PhConcatStringRef2(&fileName->sr, &parentName->sr));
                            PhMoveReference(&fileName, PhConcatStrings(3, fileName->Buffer, L"\\", baseName->Buffer));
                        }

                        //if (!PhIsNullOrEmptyString(fileName))
                        //{
                        //    HANDLE filePathHandle;
                        //
                        //    if (NT_SUCCESS(PhCreateFileWin32(
                        //        &filePathHandle,
                        //        PhGetString(fileName),
                        //        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        //        FILE_ATTRIBUTE_NORMAL,
                        //        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        //        FILE_OPEN,
                        //        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                        //        )))
                        //    {
                        //        PFILE_ALL_INFORMATION fileAllInformation;
                        //        //PFILE_ID_INFORMATION fileIdInformation;
                        //
                        //        if (NT_SUCCESS(PhGetFileAllInformation(filePathHandle, &fileAllInformation)))
                        //        {
                        //            PhPrintPointer(number, (PVOID)fileAllInformation->InternalInformation.IndexNumber.QuadPart);
                        //            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, number);
                        //        }
                        //
                        //        //if (NT_SUCCESS(PhGetFileId(filePathHandle, &fileIdInformation)))
                        //        //{
                        //        //    PPH_STRING fileIdString = PhBufferToHexString(
                        //        //        fileIdInformation->FileId.Identifier,
                        //        //        sizeof(fileIdInformation->FileId.Identifier)
                        //        //        );
                        //        //
                        //        //    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, fileIdString->Buffer);
                        //        //    PhDereferenceObject(fileIdString);
                        //        //}
                        //
                        //        NtClose(filePathHandle);
                        //    }
                        //}

                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, PhGetStringOrEmpty(fileName));

                        if (fileName) PhDereferenceObject(fileName);
                        PhDereferenceObject(baseName);
                        PhDereferenceObject(parentName);
                    }

                    NtClose(linkHandle);
                }
            }

            PhFree(hardlinks);
        }

        NtClose(fileHandle);
    }
}

INT_PTR CALLBACK PvpPeLinksDlgProc(
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
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 250, L"Name");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"ImageHardLinksListViewColumns", lvHandle);

            PvpPeEnumerateFileLinks(lvHandle);
            //ExtendedListView_SortItems(lvHandle);
            
            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageHardLinksListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
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
