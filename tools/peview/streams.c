/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2018-2020 dmex
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

VOID PvpPeEnumerateFileStreams(
    _In_ HWND ListViewHandle
    )
{
    HANDLE fileHandle;

    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);

    if (NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        PhGetString(PvFileName),
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        ULONG count = 0;
        PVOID buffer;
        PFILE_STREAM_INFORMATION i;

        if (NT_SUCCESS(PhEnumFileStreams(fileHandle, &buffer)))
        {
            for (i = PH_FIRST_STREAM(buffer); i; i = PH_NEXT_STREAM(i))
            {
                FILE_NETWORK_OPEN_INFORMATION streamInfo;
                INT lvItemIndex;
                PPH_STRING attributeName;
                PPH_STRING attributeFileName;
                WCHAR number[PH_INT32_STR_LEN_1];

                if (i->StreamNameLength && i->StreamName[0])
                    attributeName = PhCreateStringEx(i->StreamName, i->StreamNameLength);
                else
                    attributeName = PhReferenceEmptyString();
                attributeFileName = PhConcatStringRef2(&PvFileName->sr, &attributeName->sr);

                PhPrintUInt32(number, ++count);
                lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, number, attributeName);
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, attributeName->Buffer);
                PhSetListViewSubItem(
                    ListViewHandle,
                    lvItemIndex,
                    2,
                    PhaFormatSize(i->StreamSize.QuadPart, ULONG_MAX)->Buffer
                    );
                //PhSetListViewSubItem(
                //    ListViewHandle,
                //    lvItemIndex,
                //    3,
                //    PhaFormatSize(i->StreamAllocationSize.QuadPart, ULONG_MAX)->Buffer
                //    );

                // Each stream associated with a file has its own allocation size, actual size, valid data length
                // and also maintains its own attributes for compression, encryption, and sparseness. (dmex)
                if (NT_SUCCESS(PhQueryFullAttributesFileWin32(
                    PhGetString(attributeFileName),
                    &streamInfo
                    )))
                {
                    PH_STRING_BUILDER stringBuilder;
                    WCHAR pointer[PH_PTR_STR_LEN_1];

                    PhInitializeStringBuilder(&stringBuilder, 0x100);

                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_READONLY)
                        PhAppendStringBuilder2(&stringBuilder, L"Readonly, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_HIDDEN)
                        PhAppendStringBuilder2(&stringBuilder, L"Hidden, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_SYSTEM)
                        PhAppendStringBuilder2(&stringBuilder, L"System, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                        PhAppendStringBuilder2(&stringBuilder, L"Directory, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_ARCHIVE)
                        PhAppendStringBuilder2(&stringBuilder, L"Archive, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_DEVICE)
                        PhAppendStringBuilder2(&stringBuilder, L"Device, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_NORMAL)
                        PhAppendStringBuilder2(&stringBuilder, L"Normal, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_TEMPORARY)
                        PhAppendStringBuilder2(&stringBuilder, L"Temporary, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE)
                        PhAppendStringBuilder2(&stringBuilder, L"Sparse, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                        PhAppendStringBuilder2(&stringBuilder, L"Reparse point, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_COMPRESSED)
                        PhAppendStringBuilder2(&stringBuilder, L"Compressed, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_OFFLINE)
                        PhAppendStringBuilder2(&stringBuilder, L"Offline, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)
                        PhAppendStringBuilder2(&stringBuilder, L"Not indexed, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
                        PhAppendStringBuilder2(&stringBuilder, L"Encrypted, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM)
                        PhAppendStringBuilder2(&stringBuilder, L"Integiry, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_VIRTUAL)
                        PhAppendStringBuilder2(&stringBuilder, L"Vitual, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA)
                        PhAppendStringBuilder2(&stringBuilder, L"No scrub, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_EA)
                        PhAppendStringBuilder2(&stringBuilder, L"Extended attributes, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_PINNED)
                        PhAppendStringBuilder2(&stringBuilder, L"Pinned, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_UNPINNED)
                        PhAppendStringBuilder2(&stringBuilder, L"Unpinned, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_RECALL_ON_OPEN)
                        PhAppendStringBuilder2(&stringBuilder, L"Recall on opened, ");
                    if (streamInfo.FileAttributes & FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS)
                        PhAppendStringBuilder2(&stringBuilder, L"Recall on data, ");
                    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
                        PhRemoveEndStringBuilder(&stringBuilder, 2);

                    PhPrintPointer(pointer, UlongToPtr(streamInfo.FileAttributes));
                    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, PhFinalStringBuilderString(&stringBuilder)->Buffer);
                    PhDeleteStringBuilder(&stringBuilder);
                }

                PhDereferenceObject(attributeFileName);
                //PhDereferenceObject(attributeName);
            }

            PhFree(buffer);
        }

        NtClose(fileHandle);
    }

    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

typedef struct _PVP_PE_STREAMS_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HIMAGELIST ListViewImageList;
} PVP_PE_STREAMS_CONTEXT, *PPVP_PE_STREAMS_CONTEXT;

INT_PTR CALLBACK PvpPeStreamsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;
    PPVP_PE_STREAMS_CONTEXT context;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    if (uMsg == WM_INITDIALOG)
    {
        context = propPageContext->Context = PhAllocate(sizeof(PVP_PE_STREAMS_CONTEXT));
        memset(context, 0, sizeof(PVP_PE_STREAMS_CONTEXT));
    }
    else
    {
        context = propPageContext->Context;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 150, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 150, L"Size");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Attributes");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageStreamsListViewColumns", context->ListViewHandle);

            if (context->ListViewImageList = ImageList_Create(2, 20, ILC_MASK | ILC_COLOR, 1, 1))
                ListView_SetImageList(context->ListViewHandle, context->ListViewImageList, LVSIL_SMALL);

            PvpPeEnumerateFileStreams(context->ListViewHandle);
            //ExtendedListView_SortItems(context->ListViewHandle);
            
            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageStreamsListViewColumns", context->ListViewHandle);

            if (context->ListViewImageList)
                ImageList_Destroy(context->ListViewImageList);

            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, context->ListViewHandle, dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
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
