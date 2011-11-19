/*
 * Process Hacker -
 *   copy/save code for listviews and treelists
 *
 * Copyright (C) 2010-2011 wj32
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
#include <treenew.h>
#include <cpysave.h>

#define TAB_SIZE 8

VOID PhpEscapeStringForCsv(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in PPH_STRING String
    )
{
    SIZE_T i;
    SIZE_T length;
    PWCHAR runStart;
    ULONG runLength;

    length = String->Length / sizeof(WCHAR);
    runStart = NULL;

    for (i = 0; i < length; i++)
    {
        switch (String->Buffer[i])
        {
        case '\"':
            if (runStart)
            {
                PhAppendStringBuilderEx(StringBuilder, runStart, runLength * sizeof(WCHAR));
                runStart = NULL;
            }

            PhAppendStringBuilder2(StringBuilder, L"\"\"");

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
    __out PPH_STRING ***Table,
    __in ULONG Rows,
    __in ULONG Columns
    )
{
    PPH_STRING **table;
    ULONG i;

    PhCreateAlloc((PVOID *)&table, sizeof(PPH_STRING *) * Rows);
    PhaDereferenceObject(table);

    for (i = 0; i < Rows; i++)
    {
        PhCreateAlloc((PVOID *)&table[i], sizeof(PPH_STRING) * Columns);
        PhaDereferenceObject(table[i]);
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
    __in PPH_STRING **Table,
    __in ULONG Rows,
    __in ULONG Columns,
    __in ULONG Mode
    )
{
    PPH_LIST lines;
    // The tab count array contains the number of tabs need to fill the biggest
    // row cell in each column.
    PULONG tabCount;
    ULONG i;
    ULONG j;

    if (Mode == PH_EXPORT_MODE_TABS || Mode == PH_EXPORT_MODE_SPACES)
    {
        // Create the tab count array.

        PhCreateAlloc(&tabCount, sizeof(ULONG) * Columns);
        PhaDereferenceObject(tabCount);
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

        PhInitializeStringBuilder(&stringBuilder, 100);

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

                        PhAppendStringBuilder(&stringBuilder, Table[i][j]);
                    }
                    else
                    {
                        k = tabCount[j] + 1;
                    }

                    PhAppendCharStringBuilder2(&stringBuilder, '\t', k);
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

                        PhAppendStringBuilder(&stringBuilder, Table[i][j]);
                    }
                    else
                    {
                        k = (tabCount[j] + 1) * TAB_SIZE;
                    }

                    PhAppendCharStringBuilder2(&stringBuilder, ' ', k);
                }
            }
            break;
        case PH_EXPORT_MODE_CSV:
            {
                for (j = 0; j < Columns; j++)
                {
                    PhAppendCharStringBuilder(&stringBuilder, '\"');

                    if (Table[i][j])
                    {
                        PhpEscapeStringForCsv(&stringBuilder, Table[i][j]);
                    }

                    PhAppendCharStringBuilder(&stringBuilder, '\"');

                    if (j != Columns - 1)
                        PhAppendCharStringBuilder(&stringBuilder, ',');
                }
            }
            break;
        }

        PhAddItemList(lines, PhFinalStringBuilderString(&stringBuilder));
    }

    return lines;
}

VOID PhMapDisplayIndexTreeNew(
    __in HWND TreeNewHandle,
    __out_opt PULONG *DisplayToId,
    __out_opt PWSTR **DisplayToText,
    __out PULONG NumberOfColumns
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
                displayToText[i] = column.Text;
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
    __in HWND TreeNewHandle,
    __reserved ULONG Reserved
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

            PhAppendStringBuilderEx(&stringBuilder, getCellText.Text.Buffer, getCellText.Text.Length);
            PhAppendStringBuilder2(&stringBuilder, L", ");
        }

        // Remove the trailing comma and space.
        if (stringBuilder.String->Length != 0)
            PhRemoveStringBuilder(&stringBuilder, stringBuilder.String->Length / 2 - 2, 2);

        PhAppendStringBuilder2(&stringBuilder, L"\r\n");
    }

    PhFree(displayToId);

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_LIST PhGetGenericTreeNewLines(
    __in HWND TreeNewHandle,
    __in ULONG Mode
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
                table[i + 1][j] = PHA_DEREFERENCE(PhReferenceEmptyString());
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
    __in HWND ListViewHandle,
    __out_ecount(Count) PULONG DisplayToId,
    __out_ecount_opt(Count) PPH_STRING *DisplayToText,
    __in ULONG Count,
    __out PULONG NumberOfColumns
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

PPH_STRING PhGetListViewText(
    __in HWND ListViewHandle
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG displayToId[100];
    ULONG rows;
    ULONG columns;
    ULONG i;
    ULONG j;

    PhaMapDisplayIndexListView(ListViewHandle, displayToId, NULL, 100, &columns);
    rows = ListView_GetItemCount(ListViewHandle);

    PhInitializeStringBuilder(&stringBuilder, 0x100);

    for (i = 0; i < rows; i++)
    {
        if (!(ListView_GetItemState(ListViewHandle, i, LVIS_SELECTED) & LVIS_SELECTED))
            continue;

        for (j = 0; j < columns; j++)
        {
            WCHAR buffer[512];

            buffer[0] = 0;
            ListView_GetItemText(ListViewHandle, i, j, buffer, sizeof(buffer) / sizeof(WCHAR));
            PhAppendStringBuilder2(&stringBuilder, buffer);

            PhAppendStringBuilder2(&stringBuilder, L", ");
        }

        // Remove the trailing comma and space.
        if (stringBuilder.String->Length != 0)
            PhRemoveStringBuilder(&stringBuilder, stringBuilder.String->Length / 2 - 2, 2);

        PhAppendStringBuilder2(&stringBuilder, L"\r\n");
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_LIST PhGetListViewLines(
    __in HWND ListViewHandle,
    __in ULONG Mode
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
            WCHAR buffer[512];

            // Important: use this to bypass extlv's hooking.
            // extlv only hooks LVM_GETITEM, not LVM_GETITEMTEXT.
            buffer[0] = 0;
            ListView_GetItemText(ListViewHandle, i - 1, j, buffer, sizeof(buffer) / sizeof(WCHAR));
            table[i][j] = PhaCreateString(buffer);
        }
    }

    lines = PhaFormatTextTable(table, rows, columns, Mode);

    PhDeleteAutoPool(&autoPool);

    return lines;
}
