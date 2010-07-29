/*
 * Process Hacker - 
 *   copy/save code for listviews and treelists
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

#include <phapp.h>

VOID PhpMapDisplayIndexTreeList(
    __in HWND TreeListHandle,
    __out_ecount(PHTLC_MAXIMUM) PULONG DisplayToId,
    __out_ecount_opt(PHTLC_MAXIMUM) PWSTR *DisplayToText,
    __out PULONG NumberOfColumns
    )
{
    PH_TREELIST_COLUMN column;
    ULONG i;
    ULONG count;

    count = 0;

    for (i = 0; i < PHTLC_MAXIMUM; i++)
    {
        column.Id = i;

        if (TreeList_GetColumn(TreeListHandle, &column))
        {
            if (column.Visible)
            {
                assert(column.DisplayIndex < PHTLC_MAXIMUM);

                DisplayToId[column.DisplayIndex] = i;

                if (DisplayToText)
                    DisplayToText[column.DisplayIndex] = column.Text;

                count++;
            }
        }
    }

    *NumberOfColumns = count;
}

PPH_FULL_STRING PhGetProcessTreeListText(
    __in HWND TreeListHandle
    )
{
    PPH_FULL_STRING string;
    ULONG displayToId[PHTLC_MAXIMUM];
    ULONG rows;
    ULONG columns;
    ULONG i;
    ULONG j;

    PhpMapDisplayIndexTreeList(TreeListHandle, displayToId, NULL, &columns);
    rows = TreeList_GetVisibleNodeCount(TreeListHandle);

    string = PhCreateFullString2(0x100);

    for (i = 0; i < rows; i++)
    {
        PH_TL_GETNODETEXT getNodeText;

        getNodeText.Node = TreeList_GetVisibleNode(TreeListHandle, i);
        assert(getNodeText.Node);

        if (!getNodeText.Node->Selected)
            continue;

        for (j = 0; j < columns; j++)
        {
            getNodeText.Id = displayToId[j];
            PhInitializeEmptyStringRef(&getNodeText.Text);
            TreeList_GetNodeText(TreeListHandle, &getNodeText);

            PhFullStringAppendEx(string, getNodeText.Text.Buffer, getNodeText.Text.Length);
            PhFullStringAppend2(string, L", ");
        }

        // Remove the trailing comma and space.
        if (string->Length != 0)
            PhFullStringRemove(string, string->Length / 2 - 2, 2);

        PhFullStringAppend2(string, L"\r\n");
    }

    return string;
}

VOID PhpPopulateTableWithProcessNodes(
    __in HWND TreeListHandle,
    __in PPH_PROCESS_NODE Node,
    __in ULONG Level,
    __in PPH_STRING **Table,
    __inout PULONG Index,
    __in PULONG DisplayToId,
    __in ULONG Columns
    )
{
    ULONG i;

    for (i = 0; i < Columns; i++)
    {
        PH_TL_GETNODETEXT getNodeText;
        PPH_STRING text;

        getNodeText.Node = &Node->Node;
        getNodeText.Id = DisplayToId[i];
        PhInitializeEmptyStringRef(&getNodeText.Text);
        TreeList_GetNodeText(TreeListHandle, &getNodeText);

        if (i != 0)
        {
            text = PhaCreateStringEx(getNodeText.Text.Buffer, getNodeText.Text.Length);
        }
        else
        {
            // If this is the first column in the row, add some indentation.
            text = PhaCreateStringEx(
                NULL,
                getNodeText.Text.Length + Level * 2 * sizeof(WCHAR)
                );
            wmemset(text->Buffer, ' ', Level * 2);
            memcpy(&text->Buffer[Level * 2], getNodeText.Text.Buffer, getNodeText.Text.Length);
        }

        Table[*Index][i] = text;
    }

    (*Index)++;

    // Process the children.
    for (i = 0; i < Node->Children->Count; i++)
    {
        PhpPopulateTableWithProcessNodes(
            TreeListHandle,
            Node->Children->Items[i],
            Level + 1,
            Table,
            Index,
            DisplayToId,
            Columns
            );
    }
}

VOID PhpEscapeStringForCsv(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in PPH_STRING String
    )
{
    ULONG i;
    ULONG length;
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
                PhStringBuilderAppendEx(StringBuilder, runStart, runLength * sizeof(WCHAR));
                runStart = NULL;
            }

            PhStringBuilderAppend2(StringBuilder, L"\"\"");

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
        PhStringBuilderAppendEx(StringBuilder, runStart, runLength * sizeof(WCHAR));
}

PPH_LIST PhGetProcessTreeListLines(
    __in HWND TreeListHandle,
    __in ULONG NumberOfNodes,
    __in PPH_LIST RootNodes,
    __in ULONG Mode
    )
{
#define TAB_SIZE 8

    PH_AUTO_POOL autoPool;
    PPH_LIST lines;
    // The number of rows in the table (including +1 for the column headers).
    ULONG rows;
    // The number of columns.
    ULONG columns;
    // A column display index to ID map.
    ULONG displayToId[PHTLC_MAXIMUM];
    // A column display index to text map.
    PWSTR displayToText[PHTLC_MAXIMUM];
    // The actual string table.
    PPH_STRING **table;
    // The tab count array contains the number of tabs need to fill the biggest 
    // row cell in each column.
    PULONG tabCount;
    ULONG i;
    ULONG j;

    // Use a local auto-pool to make memory mangement a bit less painful.
    PhInitializeAutoPool(&autoPool);

    rows = NumberOfNodes + 1;

    // Create the display index to ID map.
    PhpMapDisplayIndexTreeList(TreeListHandle, displayToId, displayToText, &columns);

    // Create the rows.

    PhCreateAlloc((PVOID *)&table, sizeof(PPH_STRING *) * rows);
    PhaDereferenceObject(table);

    for (i = 0; i < rows; i++)
    {
        PhCreateAlloc((PVOID *)&table[i], sizeof(PPH_STRING) * columns);
        PhaDereferenceObject(table[i]);
    }

    // Populate the first row with the column headers.
    for (i = 0; i < columns; i++)
    {
        table[0][i] = PhaCreateString(displayToText[i]);
    }

    // Go through the nodes in the process tree and populate each cell of the table.

    j = 1; // index starts at one because the first row contains the column headers.

    for (i = 0; i < RootNodes->Count; i++)
    {
        PhpPopulateTableWithProcessNodes(
            TreeListHandle,
            RootNodes->Items[i],
            0,
            table,
            &j,
            displayToId,
            columns
            );
    }

    if (Mode == PH_EXPORT_MODE_TABS || Mode == PH_EXPORT_MODE_SPACES)
    {
        // Create the tab count array.

        PhCreateAlloc(&tabCount, sizeof(ULONG) * columns);
        PhaDereferenceObject(tabCount);
        memset(tabCount, 0, sizeof(ULONG) * columns); // zero all values

        for (i = 0; i < rows; i++)
        {
            for (j = 0; j < columns; j++)
            {
                ULONG newCount;

                newCount = table[i][j]->Length / sizeof(WCHAR) / TAB_SIZE;

                // Replace the existing count if this tab count is bigger.
                if (tabCount[j] < newCount)
                    tabCount[j] = newCount;
            }
        }
    }

    // Create the final list of lines by going through each cell and appending 
    // the proper tab count (if we are using tabs). This will make sure each column 
    // is properly aligned.

    lines = PhCreateList(rows);

    for (i = 0; i < rows; i++)
    {
        PPH_STRING line;
        PH_STRING_BUILDER stringBuilder;

        PhInitializeStringBuilder(&stringBuilder, 100);

        switch (Mode)
        {
        case PH_EXPORT_MODE_TABS:
            {
                for (j = 0; j < columns; j++)
                {
                    ULONG k;

                    // Calculate the number of tabs needed.
                    k = tabCount[j] + 1 - table[i][j]->Length / sizeof(WCHAR) / TAB_SIZE;

                    PhStringBuilderAppend(&stringBuilder, table[i][j]);
                    PhStringBuilderAppendChar2(&stringBuilder, '\t', k);
                }
            }
            break;
        case PH_EXPORT_MODE_SPACES:
            {
                for (j = 0; j < columns; j++)
                {
                    ULONG k;

                    // Calculate the number of spaces needed.
                    k = (tabCount[j] + 1) * TAB_SIZE - table[i][j]->Length / sizeof(WCHAR);

                    PhStringBuilderAppend(&stringBuilder, table[i][j]);
                    PhStringBuilderAppendChar2(&stringBuilder, ' ', k);
                }
            }
            break;
        case PH_EXPORT_MODE_CSV:
            {
                for (j = 0; j < columns; j++)
                {
                    PhStringBuilderAppendChar(&stringBuilder, '\"');
                    PhpEscapeStringForCsv(&stringBuilder, table[i][j]);
                    PhStringBuilderAppendChar(&stringBuilder, '\"');

                    if (j != columns - 1)
                        PhStringBuilderAppendChar(&stringBuilder, ',');
                }
            }
            break;
        }

        line = PhReferenceStringBuilderString(&stringBuilder);
        PhDeleteStringBuilder(&stringBuilder);
        PhAddListItem(lines, line);
    }

    PhDeleteAutoPool(&autoPool);

    return lines;
}

VOID PhpMapDisplayIndexListView(
    __in HWND ListViewHandle,
    __out_ecount(Count) PULONG DisplayToId,
    __in ULONG Count,
    __out PULONG NumberOfColumns
    )
{
    LVCOLUMN lvColumn;
    ULONG i;
    ULONG count;

    count = 0;
    lvColumn.mask = LVCF_ORDER;

    for (i = 0; i < Count; i++)
    {
        if (ListView_GetColumn(ListViewHandle, i, &lvColumn))
        {
            ULONG displayIndex;

            displayIndex = (ULONG)lvColumn.iOrder;
            assert(displayIndex < Count);
            DisplayToId[displayIndex] = i;

            count++;
        }
        else
        {
            break;
        }
    }

    *NumberOfColumns = count;
}

PPH_FULL_STRING PhGetListViewText(
    __in HWND ListViewHandle
    )
{
    PPH_FULL_STRING string;
    ULONG displayToId[100];
    ULONG rows;
    ULONG columns;
    ULONG i;
    ULONG j;

    PhpMapDisplayIndexListView(ListViewHandle, displayToId, 100, &columns);
    rows = ListView_GetItemCount(ListViewHandle);

    string = PhCreateFullString2(0x100);

    for (i = 0; i < rows; i++)
    {
        if (!(ListView_GetItemState(ListViewHandle, i, LVIS_SELECTED) & LVIS_SELECTED))
            continue;

        for (j = 0; j < columns; j++)
        {
            LVITEM lvItem;
            WCHAR buffer[512];

            lvItem.mask = LVIF_TEXT;
            lvItem.iItem = i;
            lvItem.iSubItem = j;
            lvItem.cchTextMax = sizeof(buffer) / sizeof(WCHAR);
            lvItem.pszText = buffer;

            if (ListView_GetItem(ListViewHandle, &lvItem))
                PhFullStringAppend2(string, buffer);

            PhFullStringAppend2(string, L", ");
        }

        // Remove the trailing comma and space.
        if (string->Length != 0)
            PhFullStringRemove(string, string->Length / 2 - 2, 2);

        PhFullStringAppend2(string, L"\r\n");
    }

    return string;
}
