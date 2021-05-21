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

typedef struct _PVP_PE_LINKS_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HIMAGELIST ListViewImageList;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_LINKS_CONTEXT, *PPVP_PE_LINKS_CONTEXT;

VOID PvpPeEnumerateFileLinks(
    _In_ HWND ListViewHandle
    )
{
    HANDLE fileHandle;
    ULONG count = 0;

    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);

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
                PH_FILE_ID_DESCRIPTOR fileId;

                memset(&fileId, 0, sizeof(PH_FILE_ID_DESCRIPTOR));
                fileId.Type = FileIdType;
                fileId.FileId.QuadPart = i->ParentFileId;

                status = PhOpenFileById(
                    &linkHandle,
                    fileHandle,
                    &fileId,
                    FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_SYNCHRONOUS_IO_NONALERT
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

    //ExtendedListView_SortItems(ListViewHandle);
    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvpPeLinksDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_LINKS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_LINKS_CONTEXT));
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
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 250, L"Name");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageHardLinksListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvpPeEnumerateFileLinks(context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageHardLinksListViewColumns", context->ListViewHandle);

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
