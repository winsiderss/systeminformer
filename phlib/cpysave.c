/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2012
 *     dmex    2018-2023
 *
 */

#include <ph.h>
#include <cpysave.h>

#include <guisup.h>
#include <treenew.h>

#define TAB_SIZE 8

VOID PhpEscapeStringForCsv(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ PPH_STRING String
    )
{
    SIZE_T i;
    SIZE_T length;
    PWCHAR runStart;
    SIZE_T runLength;

    length = String->Length / sizeof(WCHAR);
    runStart = NULL;
    runLength = 0;

    for (i = 0; i < length; i++)
    {
        switch (String->Buffer[i])
        {
        case L'\"':
            if (runStart)
            {
                PhAppendStringBuilderEx(StringBuilder, runStart, runLength * sizeof(WCHAR));
                runStart = NULL;
            }

            PhAppendStringBuilder2(StringBuilder, L"\"\"");

            break;
        case L',':
            if (runStart)
            {
                PhAppendStringBuilderEx(StringBuilder, runStart, runLength * sizeof(WCHAR));
                runStart = NULL;
            }

            // Note: There doesn't seem to be a proper way to escape
            // commas for some locales in a way that works with all
            // third party software. For now we'll swap commas
            // for full stops. This works but prevents formatting
            // integers with the correct decimal separator. (dmex)

            PhAppendStringBuilder2(StringBuilder, L".");

            break;
        default:
            if (runStart)
            {
                runLength++;
            }
            else
            {
                runStart = &String->Buffer[i];
                runLength = 1;
            }

            break;
        }
    }

    if (runStart)
        PhAppendStringBuilderEx(StringBuilder, runStart, runLength * sizeof(WCHAR));
}

/**
 * Allocates a text table.
 *
 * \param Table A variable which receives a pointer to the text table.
 * \param Rows The number of rows in the table.
 * \param Columns The number of columns in the table.
 */
VOID PhaCreateTextTable(
    _Out_ PPH_STRING ***Table,
    _In_ ULONG Rows,
    _In_ ULONG Columns
    )
{
    PPH_STRING **table;
    ULONG i;

    table = PH_AUTO(PhCreateAlloc(sizeof(PPH_STRING *) * Rows));

    for (i = 0; i < Rows; i++)
    {
        table[i] = PH_AUTO(PhCreateAlloc(sizeof(PPH_STRING) * Columns));
        memset(table[i], 0, sizeof(PPH_STRING) * Columns);
    }

    *Table = table;
}

/**
 * Formats a text table to a list of lines.
 *
 * \param Table A pointer to the text table.
 * \param Rows The number of rows in the table.
 * \param Columns The number of columns in the table.
 * \param Mode The export formatting mode.
 *
 * \return A list of strings for each line in the output. The list object and
 * string objects are not auto-dereferenced.
 */
PPH_LIST PhaFormatTextTable(
    _In_ PPH_STRING **Table,
    _In_ ULONG Rows,
    _In_ ULONG Columns,
    _In_ ULONG Mode
    )
{
    PPH_LIST lines;
    // The tab count array contains the number of tabs need to fill the biggest
    // row cell in each column.
    PULONG tabCount = NULL;
    ULONG i;
    ULONG j;

    if (Mode == PH_EXPORT_MODE_TABS || Mode == PH_EXPORT_MODE_SPACES)
    {
        // Create the tab count array.

        tabCount = PH_AUTO(PhCreateAlloc(sizeof(ULONG) * Columns));
        memset(tabCount, 0, sizeof(ULONG) * Columns); // zero all values

        for (i = 0; i < Rows; i++)
        {
            for (j = 0; j < Columns; j++)
            {
                ULONG newCount;

                if (Table[i][j])
                    newCount = (ULONG)(Table[i][j]->Length / sizeof(WCHAR) / TAB_SIZE);
                else
                    newCount = 0;

                // Replace the existing count if this tab count is bigger.
                if (tabCount[j] < newCount)
                    tabCount[j] = newCount;
            }
        }
    }

    // Create the final list of lines by going through each cell and appending
    // the proper tab count (if we are using tabs). This will make sure each column
    // is properly aligned.

    lines = PhCreateList(Rows);

    for (i = 0; i < Rows; i++)
    {
        PH_STRING_BUILDER stringBuilder;

        PhInitializeStringBuilder(&stringBuilder, 0x100);

        switch (Mode)
        {
        case PH_EXPORT_MODE_TABS:
            {
                for (j = 0; j < Columns; j++)
                {
                    ULONG k;

                    if (Table[i][j])
                    {
                        // Calculate the number of tabs needed.
                        k = (ULONG)(tabCount[j] + 1 - Table[i][j]->Length / sizeof(WCHAR) / TAB_SIZE);

                        PhAppendStringBuilder(&stringBuilder, &Table[i][j]->sr);
                    }
                    else
                    {
                        k = tabCount[j] + 1;
                    }

                    PhAppendCharStringBuilder2(&stringBuilder, L'\t', k);
                }
            }
            break;
        case PH_EXPORT_MODE_SPACES:
            {
                for (j = 0; j < Columns; j++)
                {
                    ULONG k;

                    if (Table[i][j])
                    {
                        // Calculate the number of spaces needed.
                        k = (ULONG)((tabCount[j] + 1) * TAB_SIZE - Table[i][j]->Length / sizeof(WCHAR));

                        PhAppendStringBuilder(&stringBuilder, &Table[i][j]->sr);
                    }
                    else
                    {
                        k = (tabCount[j] + 1) * TAB_SIZE;
                    }

                    PhAppendCharStringBuilder2(&stringBuilder, L' ', k);
                }
            }
            break;
        case PH_EXPORT_MODE_CSV:
            {
                for (j = 0; j < Columns; j++)
                {
                    PhAppendCharStringBuilder(&stringBuilder, L'\"');

                    if (Table[i][j])
                    {
                        PhpEscapeStringForCsv(&stringBuilder, Table[i][j]);
                    }

                    PhAppendCharStringBuilder(&stringBuilder, L'\"');

                    if (j != Columns - 1)
                        PhAppendCharStringBuilder(&stringBuilder, L',');
                }
            }
            break;
        }

        PhAddItemList(lines, PhFinalStringBuilderString(&stringBuilder));
    }

    return lines;
}

VOID PhMapDisplayIndexTreeNew(
    _In_ HWND TreeNewHandle,
    _Out_opt_ PULONG *DisplayToId,
    _Out_opt_ PWSTR **DisplayToText,
    _Out_ PULONG NumberOfColumns
    )
{
    PPH_TREENEW_COLUMN fixedColumn;
    ULONG numberOfColumns;
    PULONG displayToId;
    PWSTR *displayToText;
    ULONG i;
    PH_TREENEW_COLUMN column;

    fixedColumn = TreeNew_GetFixedColumn(TreeNewHandle);
    numberOfColumns = TreeNew_GetVisibleColumnCount(TreeNewHandle);

    displayToId = PhAllocate(numberOfColumns * sizeof(ULONG));

    if (fixedColumn)
    {
        TreeNew_GetColumnOrderArray(TreeNewHandle, numberOfColumns - 1, displayToId + 1);
        displayToId[0] = fixedColumn->Id;
    }
    else
    {
        TreeNew_GetColumnOrderArray(TreeNewHandle, numberOfColumns, displayToId);
    }

    if (DisplayToText)
    {
        displayToText = PhAllocate(numberOfColumns * sizeof(PWSTR));

        for (i = 0; i < numberOfColumns; i++)
        {
            if (TreeNew_GetColumn(TreeNewHandle, displayToId[i], &column))
            {
                displayToText[i] = (PWSTR)column.Text;
            }
        }

        *DisplayToText = displayToText;
    }

    if (DisplayToId)
        *DisplayToId = displayToId;
    else
        PhFree(displayToId);

    *NumberOfColumns = numberOfColumns;
}

PPH_STRING PhGetTreeNewText(
    _In_ HWND TreeNewHandle,
    _Reserved_ ULONG Reserved
    )
{
    PH_STRING_BUILDER stringBuilder;
    PULONG displayToId;
    ULONG rows;
    ULONG columns;
    ULONG i;
    ULONG j;

    PhMapDisplayIndexTreeNew(TreeNewHandle, &displayToId, NULL, &columns);
    rows = TreeNew_GetFlatNodeCount(TreeNewHandle);

    PhInitializeStringBuilder(&stringBuilder, 0x100);

    for (i = 0; i < rows; i++)
    {
        PH_TREENEW_GET_CELL_TEXT getCellText;

        getCellText.Node = TreeNew_GetFlatNode(TreeNewHandle, i);
        assert(getCellText.Node);

        if (!getCellText.Node->Selected)
            continue;

        for (j = 0; j < columns; j++)
        {
            getCellText.Id = displayToId[j];
            PhInitializeEmptyStringRef(&getCellText.Text);
            TreeNew_GetCellText(TreeNewHandle, &getCellText);

            // Ignore empty columns. (dmex)
            if (getCellText.Text.Length != 0)
            {
                PhAppendStringBuilder(&stringBuilder, &getCellText.Text);
            }

            PhAppendStringBuilder2(&stringBuilder, L", ");
        }

        // Remove the trailing comma and space.
        if (stringBuilder.String->Length != 0)
            PhRemoveEndStringBuilder(&stringBuilder, 2);

        PhAppendStringBuilder2(&stringBuilder, L"\r\n");
    }

    PhFree(displayToId);

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_LIST PhGetGenericTreeNewLines(
    _In_ HWND TreeNewHandle,
    _In_ ULONG Mode
    )
{
    PH_AUTO_POOL autoPool;
    PPH_LIST lines;
    ULONG rows;
    ULONG columns;
    ULONG numberOfNodes;
    PULONG displayToId;
    PWSTR *displayToText;
    PPH_STRING **table;
    ULONG i;
    ULONG j;

    PhInitializeAutoPool(&autoPool);

    numberOfNodes = TreeNew_GetFlatNodeCount(TreeNewHandle);

    rows = numberOfNodes + 1;
    PhMapDisplayIndexTreeNew(TreeNewHandle, &displayToId, &displayToText, &columns);

    PhaCreateTextTable(&table, rows, columns);

    for (i = 0; i < columns; i++)
        table[0][i] = PhaCreateString(displayToText[i]);

    for (i = 0; i < numberOfNodes; i++)
    {
        PPH_TREENEW_NODE node;

        node = TreeNew_GetFlatNode(TreeNewHandle, i);

        if (node)
        {
            for (j = 0; j < columns; j++)
            {
                PH_TREENEW_GET_CELL_TEXT getCellText;

                getCellText.Node = node;
                getCellText.Id = displayToId[j];
                PhInitializeEmptyStringRef(&getCellText.Text);
                TreeNew_GetCellText(TreeNewHandle, &getCellText);

                table[i + 1][j] = PhaCreateStringEx(getCellText.Text.Buffer, getCellText.Text.Length);
            }
        }
        else
        {
            for (j = 0; j < columns; j++)
            {
                table[i + 1][j] = PH_AUTO(PhReferenceEmptyString());
            }
        }
    }

    PhFree(displayToText);
    PhFree(displayToId);

    lines = PhaFormatTextTable(table, rows, columns, Mode);

    PhDeleteAutoPool(&autoPool);

    return lines;
}

VOID PhaMapDisplayIndexListView(
    _In_ HWND ListViewHandle,
    _Out_writes_(Count) PULONG DisplayToId,
    _Out_writes_opt_(Count) PPH_STRING *DisplayToText,
    _In_ ULONG Count,
    _Out_ PULONG NumberOfColumns
    )
{
    LVCOLUMN lvColumn;
    ULONG i;
    ULONG count;
    WCHAR buffer[128];

    count = 0;
    lvColumn.mask = LVCF_ORDER | LVCF_TEXT;
    lvColumn.pszText = buffer;
    lvColumn.cchTextMax = sizeof(buffer) / sizeof(WCHAR);

    for (i = 0; i < Count; i++)
    {
        if (ListView_GetColumn(ListViewHandle, i, &lvColumn))
        {
            ULONG displayIndex;

            displayIndex = (ULONG)lvColumn.iOrder;
            assert(displayIndex < Count);
            DisplayToId[displayIndex] = i;

            if (DisplayToText)
                DisplayToText[displayIndex] = PhaCreateString(buffer);

            count++;
        }
        else
        {
            break;
        }
    }

    *NumberOfColumns = count;
}

VOID PhaMapDisplayIndexIListView(
    _In_ struct IListView* ListView,
    _Out_writes_(Count) PULONG DisplayToId,
    _Out_writes_opt_(Count) PPH_STRING *DisplayToText,
    _In_ ULONG Count,
    _Out_ PULONG NumberOfColumns
    )
{
    LVCOLUMN lvColumn;
    ULONG i;
    ULONG count;
    WCHAR buffer[128];

    count = 0;
    lvColumn.mask = LVCF_ORDER | LVCF_TEXT;
    lvColumn.pszText = buffer;
    lvColumn.cchTextMax = sizeof(buffer) / sizeof(WCHAR);

    for (i = 0; i < Count; i++)
    {
        if (SUCCEEDED(IListView_GetColumn(ListView, i, &lvColumn)))
        {
            ULONG displayIndex;

            displayIndex = (ULONG)lvColumn.iOrder;
            assert(displayIndex < Count);
            DisplayToId[displayIndex] = i;

            if (DisplayToText)
                DisplayToText[displayIndex] = PhaCreateString(buffer);

            count++;
        }
        else
        {
            break;
        }
    }

    *NumberOfColumns = count;
}

PPH_STRING PhGetListViewItemText(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ LONG SubItemIndex
    )
{
    PPH_STRING buffer;
    SIZE_T allocatedCount;
    SIZE_T count;
    LVITEM lvItem;

    // Unfortunately LVM_GETITEMTEXT doesn't want to return the actual length of the text.
    // Keep doubling the buffer size until we get a return count that is strictly less than
    // the amount we allocated.

    buffer = NULL;
    allocatedCount = 256;
    count = allocatedCount;

    while (count >= allocatedCount)
    {
        allocatedCount *= 2;
        if (buffer) PhDereferenceObject(buffer);
        buffer = PhCreateStringEx(NULL, (allocatedCount + 1) * sizeof(WCHAR));

        lvItem.iSubItem = SubItemIndex;
        lvItem.cchTextMax = (LONG)allocatedCount;
        lvItem.pszText = buffer->Buffer;
        count = SendMessage(ListViewHandle, LVM_GETITEMTEXT, Index, (LPARAM)&lvItem);
    }

    PhTrimToNullTerminatorString(buffer);

    return buffer;
}

PPH_STRING PhGetIListViewItemText(
    _In_ struct IListView* ListView,
    _In_ LONG Index,
    _In_ LONG SubItemIndex
    )
{
    PPH_STRING buffer;
    SIZE_T allocatedCount;
    SIZE_T count;
    LVITEM lvItem;

    // Unfortunately LVM_GETITEMTEXT doesn't want to return the actual length of the text.
    // Keep doubling the buffer size until we get a return count that is strictly less than
    // the amount we allocated.

    buffer = NULL;
    allocatedCount = 256;
    count = allocatedCount;

    while (count >= allocatedCount)
    {
        allocatedCount *= 2;
        if (buffer) PhDereferenceObject(buffer);
        buffer = PhCreateStringEx(NULL, (allocatedCount + 1) * sizeof(WCHAR));

        memset(&lvItem, 0, sizeof(LVITEM));
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = Index;
        lvItem.iSubItem = SubItemIndex;
        lvItem.pszText = buffer->Buffer;
        lvItem.cchTextMax = (LONG)allocatedCount;

        if (SUCCEEDED(IListView_GetItem(ListView, &lvItem)))
            break;

        count = PhCountStringZ(lvItem.pszText);

        //if (SUCCEEDED(IListView_GetItemText(ListView, Index, 0, buffer->Buffer, allocatedCount)))
        //    break;
    }

    PhTrimToNullTerminatorString(buffer);

    return buffer;
}
//PPH_STRING PhGetListViewItemText(
//    _In_ HWND ListViewHandle,
//    _In_ INT Index,
//    _In_ INT SubIndex
//    )
//{
//    WCHAR buffer[DOS_MAX_PATH_LENGTH] = L"";
//    LVITEM item;
//
//    item.mask = LVIF_TEXT;
//    item.iItem = Index;
//    item.iSubItem = SubIndex;
//    item.pszText = buffer;
//    item.cchTextMax = RTL_NUMBER_OF(buffer);
//
//    if (ListView_GetItem(ListViewHandle, &item))
//    {
//        return PhCreateString(buffer);
//    }
//
//    return NULL;
//}

PPH_STRING PhGetListViewSelectedItemText(
    _In_ HWND ListViewHandle
    )
{
    LONG index;

    index = PhFindListViewItemByFlags(
        ListViewHandle,
        INT_ERROR,
        LVNI_SELECTED
        );

    if (index != INT_ERROR)
    {
        return PhGetListViewItemText(ListViewHandle, index, 0);
    }

    return NULL;
}

PPH_STRING PhGetIListViewSelectedItemText(
    _In_ struct IListView* ListView
    )
{
    LONG index;

    index = PhFindIListViewItemByFlags(
        ListView,
        INT_ERROR,
        LVNI_SELECTED
        );

    if (index != INT_ERROR)
    {
        return PhGetIListViewItemText(ListView, index, 0);
    }

    return NULL;
}

PPH_STRING PhaGetListViewItemText(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ LONG SubItemIndex
    )
{
    PPH_STRING value;

    if (value = PhGetListViewItemText(ListViewHandle, Index, SubItemIndex))
    {
        PH_AUTO(value);

        return value;
    }

    return NULL;
}

PPH_STRING PhaGetIListViewItemText(
    _In_ struct IListView* ListView,
    _In_ LONG Index,
    _In_ LONG SubItemIndex
    )
{
    PPH_STRING value;

    if (value = PhGetIListViewItemText(ListView, Index, SubItemIndex))
    {
        PH_AUTO(value);

        return value;
    }

    return NULL;
}

PPH_STRING PhGetListViewText(
    _In_ HWND ListViewHandle
    )
{
    PH_AUTO_POOL autoPool;
    PH_STRING_BUILDER stringBuilder;
    ULONG displayToId[100];
    ULONG rows;
    ULONG columns;
    ULONG i;
    ULONG j;

    PhInitializeAutoPool(&autoPool);

    PhaMapDisplayIndexListView(ListViewHandle, displayToId, NULL, 100, &columns);
    rows = ListView_GetItemCount(ListViewHandle);

    PhInitializeStringBuilder(&stringBuilder, 0x100);

    for (i = 0; i < rows; i++)
    {
        if (!(ListView_GetItemState(ListViewHandle, i, LVIS_SELECTED) & LVIS_SELECTED))
            continue;

        for (j = 0; j < columns; j++)
        {
            PhAppendStringBuilder(&stringBuilder, &PhaGetListViewItemText(ListViewHandle, i, j)->sr);
            PhAppendStringBuilder2(&stringBuilder, L", ");
        }

        // Remove the trailing comma and space.
        if (stringBuilder.String->Length != 0)
            PhRemoveEndStringBuilder(&stringBuilder, 2);

        PhAppendStringBuilder2(&stringBuilder, L"\r\n");
    }

    PhDeleteAutoPool(&autoPool);

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_STRING PhGetIListViewText(
    _In_ struct IListView* ListView
    )
{
    PH_AUTO_POOL autoPool;
    PH_STRING_BUILDER stringBuilder;
    ULONG displayToId[100];
    ULONG state = 0;
    ULONG rows = 0;
    ULONG columns;
    ULONG i;
    ULONG j;

    PhInitializeAutoPool(&autoPool);

    PhaMapDisplayIndexIListView(ListView, displayToId, NULL, 100, &columns);
    IListView_GetItemCount(ListView, &rows);

    PhInitializeStringBuilder(&stringBuilder, 0x100);

    for (i = 0; i < rows; i++)
    {
        if (!SUCCEEDED(IListView_GetItemState(ListView, i, 0, LVIS_SELECTED, &state)))
            continue;

        if (!FlagOn(state, LVIS_SELECTED))
            continue;

        for (j = 0; j < columns; j++)
        {
            PhAppendStringBuilder(&stringBuilder, &PhaGetIListViewItemText(ListView, i, j)->sr);
            PhAppendStringBuilder2(&stringBuilder, L", ");
        }

        // Remove the trailing comma and space.
        if (stringBuilder.String->Length != 0)
            PhRemoveEndStringBuilder(&stringBuilder, 2);

        PhAppendStringBuilder2(&stringBuilder, L"\r\n");
    }

    PhDeleteAutoPool(&autoPool);

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_LIST PhGetListViewLines(
    _In_ HWND ListViewHandle,
    _In_ ULONG Mode
    )
{
    PH_AUTO_POOL autoPool;
    PPH_LIST lines;
    ULONG rows;
    ULONG columns;
    ULONG displayToId[100];
    PPH_STRING displayToText[100];
    PPH_STRING **table;
    ULONG i;
    ULONG j;

    PhInitializeAutoPool(&autoPool);

    rows = ListView_GetItemCount(ListViewHandle) + 1; // +1 for column headers

    // Create the display index/text to ID map.
    PhaMapDisplayIndexListView(ListViewHandle, displayToId, displayToText, 100, &columns);

    PhaCreateTextTable(&table, rows, columns);

    // Populate the first row with the column headers.
    for (i = 0; i < columns; i++)
        table[0][i] = displayToText[i];

    // Populate the other rows with text.
    for (i = 1; i < rows; i++)
    {
        for (j = 0; j < columns; j++)
        {
            // Important: use this to bypass extlv's hooking.
            // extlv only hooks LVM_GETITEM, not LVM_GETITEMTEXT.
            table[i][j] = PhaGetListViewItemText(ListViewHandle, i - 1, j);
        }
    }

    lines = PhaFormatTextTable(table, rows, columns, Mode);

    PhDeleteAutoPool(&autoPool);

    return lines;
}
