/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2023-2026
 *
 */

#include <peview.h>
#include <mapclr.h>

typedef struct _PVP_PE_CLR_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
    PVOID PdbMetadataAddress;
    BOOLEAN ClrMetadataInitialized;
    PH_MAPPED_CLR_METADATA ClrMetadata;
} PVP_PE_CLR_CONTEXT, *PPVP_PE_CLR_CONTEXT;

typedef struct _PVP_PE_CLR_TABLE_PREVIEW_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    RECT MinimumSize;
    PPH_MAPPED_CLR_METADATA ClrMetadata;
    ULONG TableIndex;
    ULONG RowCount;
    ULONG ColumnCount;
    PCSTR TableName;
} PVP_PE_CLR_TABLE_PREVIEW_CONTEXT, *PPVP_PE_CLR_TABLE_PREVIEW_CONTEXT;

static PCWSTR PvpClrColumnTypeToString(
    _In_ ULONG Type
    )
{
    switch (Type)
    {
    case PH_CLR_COLUMN_FIXED:
        return L"Fixed";
    case PH_CLR_COLUMN_STRING:
        return L"String";
    case PH_CLR_COLUMN_GUID:
        return L"GUID";
    case PH_CLR_COLUMN_BLOB:
        return L"Blob";
    case PH_CLR_COLUMN_TABLE:
        return L"Table";
    case PH_CLR_COLUMN_CODED:
        return L"Coded";
    default:
        return L"Unknown";
    }
}

static VOID PvpPeClrTablePreviewAddColumns(
    _In_ PPVP_PE_CLR_TABLE_PREVIEW_CONTEXT Context
    )
{
    PhAddListViewColumn(Context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 60, L"RID");

    for (ULONG i = 0; i < Context->ColumnCount; i++)
    {
        ULONG type = 0;
        PCWSTR typeName;
        PCWSTR columnName = NULL;

        if (!NT_SUCCESS(PhGetMappedClrColumnInfo(
            Context->ClrMetadata,
            Context->TableIndex,
            i,
            NULL,
            NULL,
            &type
            )))
        {
            typeName = L"Unknown";
        }
        else
        {
            typeName = PvpClrColumnTypeToString(type);
        }

        // Get the actual column name from the mapping table
        if (Context->TableIndex < PH_CLR_TABLE_MAXIMUM && 
            i < PH_CLR_MAX_COLUMNS && 
            PhClrTableColumnNames[Context->TableIndex][i])
        {
            columnName = PhClrTableColumnNames[Context->TableIndex][i];
        }

        if (columnName)
        {
            PhAddListViewColumn(
                Context->ListViewHandle,
                i + 1,
                i + 1,
                i + 1,
                LVCFMT_LEFT,
                110,
                columnName
                );
        }
        else
        {
            PhAddListViewColumn(
                Context->ListViewHandle,
                i + 1,
                i + 1,
                i + 1,
                LVCFMT_LEFT,
                110,
                PhaFormatString(L"Column %lu (%s)", i, typeName)->Buffer
                );
        }
    }
}

static VOID PvpPeClrTablePreviewAddRows(
    _In_ PPVP_PE_CLR_TABLE_PREVIEW_CONTEXT Context
    )
{
    WCHAR value[PH_INT64_STR_LEN_1];

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);

    for (ULONG row = 0; row < Context->RowCount; row++)
    {
        INT lvItemIndex;
        ULONG rid = row + 1;

        PhPrintUInt32(value, rid);
        lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, value, NULL);

        for (ULONG column = 0; column < Context->ColumnCount; column++)
        {
            ULONG columnValue;

            if (NT_SUCCESS(PhGetMappedClrColumnValue(
                Context->ClrMetadata,
                Context->TableIndex,
                column,
                rid,
                &columnValue
                )))
            {
                PhSetListViewSubItem(
                    Context->ListViewHandle,
                    lvItemIndex,
                    column + 1,
                    PhaFormatString(L"0x%lx", columnValue)->Buffer
                    );
            }
        }
    }

    for (ULONG column = 0; column <= Context->ColumnCount; column++)
        ListView_SetColumnWidth(Context->ListViewHandle, column, LVSCW_AUTOSIZE_USEHEADER);

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvpPeClrTablePreviewDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_CLR_TABLE_PREVIEW_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPVP_PE_CLR_TABLE_PREVIEW_CONTEXT)lParam;
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
            HICON smallIcon;
            HICON largeIcon;
            PPH_STRING tableName;

            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhGetStockApplicationIcon(&smallIcon, &largeIcon, PhGetWindowDpi(hwndDlg));
            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)largeIcon);

            if (tableName = PhConvertUtf8ToUtf16(context->TableName))
            {
                PhSetWindowText(hwndDlg, PhaFormatString(L"CLR Table: %s", tableName->Buffer)->Buffer);
                PhDereferenceObject(tableName);
            }

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhSetExtendedListView(context->ListViewHandle);
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhLayoutManagerLayout(&context->LayoutManager);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 280;
            context->MinimumSize.bottom = 180;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            PvpPeClrTablePreviewAddColumns(context);
            PvpPeClrTablePreviewAddRows(context);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, context->MinimumSize.right, context->MinimumSize.bottom);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                return TRUE;
            }
        }
        break;
    case WM_CLOSE:
        {
            DestroyWindow(hwndDlg);
        }
        return TRUE;
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
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)PhGetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}

static VOID PvpPeClrShowTablePreview(
    _In_ PPVP_PE_CLR_CONTEXT Context,
    _In_ ULONG TableIndex
    )
{
    NTSTATUS status;
    HWND dialogHandle;
    PPVP_PE_CLR_TABLE_PREVIEW_CONTEXT previewContext;

    if (!Context->ClrMetadataInitialized)
    {
        PhShowError(Context->WindowHandle, L"%s", L"Unable to preview CLR table rows because CLR metadata is unavailable.");
        return;
    }

    previewContext = PhAllocateZero(sizeof(PVP_PE_CLR_TABLE_PREVIEW_CONTEXT));
    previewContext->ClrMetadata = &Context->ClrMetadata;
    previewContext->TableIndex = TableIndex;

    status = PhGetMappedClrTableInfoEx(
        previewContext->ClrMetadata,
        previewContext->TableIndex,
        NULL,
        &previewContext->RowCount,
        &previewContext->ColumnCount,
        &previewContext->TableName,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        PhFree(previewContext);
        PhShowStatus(Context->WindowHandle, L"Unable to preview CLR table rows", status, 0);
        return;
    }

    dialogHandle = PhCreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PECLRTABLEPREVIEW),
        Context->WindowHandle,
        PvpPeClrTablePreviewDlgProc,
        previewContext
        );

    if (!dialogHandle)
    {
        PhFree(previewContext);
        PhShowError(Context->WindowHandle, L"%s", L"Unable to create the CLR table preview window.");
        return;
    }

    ShowWindow(dialogHandle, SW_SHOW);
}

_Function_class_(PH_CLR_ENUM_TABLES_CALLBACK)
static BOOLEAN NTAPI PvpClrEnumTableCallback(
    _In_opt_ ULONG TableIndex,
    _In_ ULONG RowSize,
    _In_ ULONG RowCount,
    _In_ PCSTR Name,
    _In_opt_ PVOID Rows,
    _In_opt_ PVOID Context
    )
{
    PPVP_PE_CLR_CONTEXT context = Context;
    WCHAR value[PH_INT64_STR_LEN_1];
    WCHAR countValue[PH_INT64_STR_LEN_1];
    WCHAR rvaStartValue[PH_INT64_STR_LEN_1];
    WCHAR rvaEndValue[PH_INT64_STR_LEN_1];
    WCHAR sizeValue[PH_INT64_STR_LEN_1];
    INT lvItemIndex;
    PPH_STRING nameSr;
    ULONG rvaStart = 0;
    ULONG rvaEnd = 0;
    ULONG64 tableSize = 0;
    ULONG hashAlgorithm;
    PH_HASH_CONTEXT hashContext;
    UCHAR hashResult[32];
    ULONG hashResultSize = 0;
    PPH_STRING hashString;

    PhPrintUInt32(value, TableIndex);

    // If this is the initial pass (Name is NULL), add a blank row with TableIndex as lParam
    if (!Name)
    {
        lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, value, NULL);
        PhSetListViewItemParam(context->ListViewHandle, lvItemIndex, UlongToPtr(TableIndex));
        return TRUE;
    }

    // This is the data-population pass; find the existing row by TableIndex
    lvItemIndex = PhFindListViewItemByParam(context->ListViewHandle, -1, UlongToPtr(TableIndex));

    if (lvItemIndex == -1)
    {
        lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, value, NULL);
        PhSetListViewItemParam(context->ListViewHandle, lvItemIndex, UlongToPtr(TableIndex));
    }

    PhPrintUInt32(countValue, RowCount);

    // Column 1: Name
    if (nameSr = PhConvertUtf8ToUtf16(Name))
    {
        PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 1, nameSr->Buffer);
        PhDereferenceObject(nameSr);
    }
    
    // Column 2: Count
    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 2, countValue);

    // Column 3: Size
    PhPrintUInt32(sizeValue, RowSize);
    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 3, sizeValue);

    // Column 4-5: RVA start and end
    if (Rows && PvImageCor20Header)
    {
        PVOID metadataVa;
        
        // Get the VA of the metadata section
        if (NT_SUCCESS(PhMappedImageRvaToVa(&PvMappedImage, PvImageCor20Header->MetaData.VirtualAddress, &metadataVa)))
        {
            // Calculate offset within metadata and compute RVA
            ULONG64 rowsVa = (ULONG64)Rows;
            ULONG64 metadataVa64 = (ULONG64)metadataVa;
            
            if (rowsVa >= metadataVa64)
            {
                rvaStart = PvImageCor20Header->MetaData.VirtualAddress + (ULONG)(rowsVa - metadataVa64);
                PhPrintUInt32(rvaStartValue, rvaStart);
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 4, rvaStartValue);

                tableSize = (ULONG64)RowCount * RowSize;
                rvaEnd = rvaStart + (ULONG)tableSize;
                PhPrintUInt32(rvaEndValue, rvaEnd);
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 5, rvaEndValue);
            }
        }
    }

    // Column 6: Hash
    hashAlgorithm = PhGetIntegerSetting(L"HashAlgorithm");

    if (Rows && RowCount > 0 && NT_SUCCESS(PhInitializeHash(&hashContext, hashAlgorithm)))
    {
        PhUpdateHash(&hashContext, Rows, (ULONG64)RowCount * RowSize);

        hashResultSize = sizeof(hashResult);

        if (NT_SUCCESS(PhFinalHash(&hashContext, hashResult, sizeof(hashResult), &hashResultSize)))
        {
            // Format hash as hex string
            if (hashResultSize > 0)
            {
                hashString = PhBufferToHexString(hashResult, hashResultSize);
                if (hashString)
                {
                    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 6, hashString->Buffer);
                    PhDereferenceObject(hashString);
                }
            }
        }
    }

    return TRUE;
}

INT_PTR CALLBACK PvpPeClrTablesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_CLR_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_CLR_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;

            if (context->PropSheetContext->Context)
            {
                PPV_CLR_PAGECONTEXT pageContext = context->PropSheetContext->Context;
                context->PdbMetadataAddress = pageContext->PdbMetadataAddress;
            }
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
            PhSetExtendedListView(context->ListViewHandle);
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);

            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 50, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"Count");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Size");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 120, L"RVA (start)");
            PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 120, L"RVA (end)");
            PhAddListViewColumn(context->ListViewHandle, 6, 6, 6, LVCFMT_LEFT, 200, L"Hash");

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            context->ClrMetadataInitialized = FALSE;

            {
                NTSTATUS status;

                status = PhInitializeMappedClrMetadata(
                    &context->ClrMetadata,
                    &PvMappedImage
                    );

                if (NT_SUCCESS(status))
                {
                    context->ClrMetadataInitialized = TRUE;

                    // Iterate through all possible table indices
                    for (ULONG tableIndex = 0; tableIndex < PH_CLR_TABLE_MAXIMUM; tableIndex++)
                    {
                        PvpClrEnumTableCallback(
                            tableIndex,
                            0,
                            0,
                            NULL,
                            NULL,
                            context
                            );
                    }

                    // Now populate data for existing tables
                    PhEnumMappedClrTables(
                        &context->ClrMetadata,
                        PvpClrEnumTableCallback,
                        context
                        );
                }
            }

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            if (context->ClrMetadataInitialized)
            {
                PhDeleteMappedClrMetadata(&context->ClrMetadata);
            }

            PhDeleteLayoutManager(&context->LayoutManager);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case NM_DBLCLK:
                {
                    LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)lParam;
                    if (itemActivate->iItem != -1)
                    {
                        PvpPeClrShowTablePreview(context, itemActivate->iItem);
                    }
                }
                break;
            case PSN_QUERYINITIALFOCUS:
                return (INT_PTR)context->ListViewHandle;
            }

            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)PhGetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}
