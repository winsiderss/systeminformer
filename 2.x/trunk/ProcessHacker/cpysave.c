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

PPH_STRING PhGetProcessTreeListText(
    __in HWND TreeListHandle
    )
{
    PPH_STRING string;
    PH_STRING_BUILDER stringBuilder;
    ULONG displayToId[PHTLC_MAXIMUM];
    ULONG rows;
    ULONG columns;
    ULONG i;
    ULONG j;

    PhpMapDisplayIndexTreeList(TreeListHandle, displayToId, NULL, &columns);
    rows = TreeList_GetVisibleNodeCount(TreeListHandle);

    PhInitializeStringBuilder(&stringBuilder, 200);

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

            PhStringBuilderAppendEx(&stringBuilder, getNodeText.Text.Buffer, getNodeText.Text.Length);
            PhStringBuilderAppend2(&stringBuilder, L", ");
        }

        // Remove the trailing comma and space.
        if (stringBuilder.String->Length != 0)
            PhStringBuilderRemove(&stringBuilder, stringBuilder.String->Length / 2 - 2, 2);

        PhStringBuilderAppend2(&stringBuilder, L"\r\n");
    }

    string = PhReferenceStringBuilderString(&stringBuilder);
    PhDeleteStringBuilder(&stringBuilder);

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

PPH_LIST PhGetProcessTreeListLines(
    __in HWND TreeListHandle,
    __in ULONG NumberOfNodes,
    __in PPH_LIST RootNodes,
    __in BOOLEAN UseTabs
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

    // Create the tab count array.

    PhCreateAlloc(&tabCount, sizeof(ULONG) * columns);
    PhaDereferenceObject(tabCount);
    memset(tabCount, 0, sizeof(ULONG) * columns); // zero all values

    for (i = 0; i < rows; i++)
    {
        for (j = 0; j < columns; j++)
        {
            ULONG newCount;

            newCount = (table[i][j]->Length / sizeof(WCHAR)) / TAB_SIZE;

            // Replace the existing count if this tab count is bigger.
            if (tabCount[j] < newCount)
                tabCount[j] = newCount;
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

        for (j = 0; j < columns; j++)
        {
            if (UseTabs)
            {
                ULONG k;
                ULONG m;

                // Calculate the number of tabs needed.
                k = tabCount[j] - (table[i][j]->Length / sizeof(WCHAR)) / TAB_SIZE + 1;

                PhStringBuilderAppend(&stringBuilder, table[i][j]);

                for (m = 0; m < k; m++)
                    PhStringBuilderAppendChar(&stringBuilder, '\t');
            }
            else
            {
                // TODO
                PhRaiseStatus(STATUS_NOT_IMPLEMENTED);
            }
        }

        line = PhReferenceStringBuilderString(&stringBuilder);
        PhDeleteStringBuilder(&stringBuilder);
        PhAddListItem(lines, line);
    }

    PhDeleteAutoPool(&autoPool);

    return lines;
}
