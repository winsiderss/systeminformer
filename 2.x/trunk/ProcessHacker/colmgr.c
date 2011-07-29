/*
 * Process Hacker - 
 *   tree new column manager
 * 
 * Copyright (C) 2011 wj32
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
#include <extmgri.h>
#include <phplug.h>
#include <colmgr.h>

typedef struct _PH_CM_SORT_CONTEXT
{
    PPH_PLUGIN_TREENEW_SORT_FUNCTION SortFunction;
    ULONG SubId;
    PVOID Context;
    PPH_CM_POST_SORT_FUNCTION PostSortFunction;
    PH_SORT_ORDER SortOrder;
} PH_CM_SORT_CONTEXT, *PPH_CM_SORT_CONTEXT;

VOID PhCmInitializeManager(
    __out PPH_CM_MANAGER Manager,
    __in HWND Handle,
    __in ULONG MinId,
    __in PPH_CM_POST_SORT_FUNCTION PostSortFunction
    )
{
    Manager->Handle = Handle;
    Manager->MinId = MinId;
    Manager->NextId = MinId;
    Manager->PostSortFunction = PostSortFunction;
    InitializeListHead(&Manager->ColumnListHead);
}

VOID PhCmDeleteManager(
    __in PPH_CM_MANAGER Manager
    )
{
    PLIST_ENTRY listEntry;
    PPH_CM_COLUMN column;

    listEntry = Manager->ColumnListHead.Flink;

    while (listEntry != &Manager->ColumnListHead)
    {
        column = CONTAINING_RECORD(listEntry, PH_CM_COLUMN, ListEntry);
        listEntry = listEntry->Flink;

        PhFree(column);
    }
}

PPH_CM_COLUMN PhCmCreateColumn(
    __inout PPH_CM_MANAGER Manager,
    __in PPH_TREENEW_COLUMN Column,
    __in struct _PH_PLUGIN *Plugin,
    __in ULONG SubId,
    __in_opt PVOID Context,
    __in PVOID SortFunction
    )
{
    PPH_CM_COLUMN column;
    PH_TREENEW_COLUMN tnColumn;

    column = PhAllocate(sizeof(PH_CM_COLUMN));
    memset(column, 0, sizeof(PH_CM_COLUMN));
    column->Id = Manager->NextId++;
    column->Plugin = Plugin;
    column->SubId = SubId;
    column->Context = Context;
    column->SortFunction = SortFunction;
    InsertTailList(&Manager->ColumnListHead, &column->ListEntry);

    memset(&tnColumn, 0, sizeof(PH_TREENEW_COLUMN));
    tnColumn.Id = column->Id;
    tnColumn.Context = column;
    tnColumn.Visible = FALSE;
    tnColumn.CustomDraw = Column->CustomDraw;
    tnColumn.Text = Column->Text;
    tnColumn.Width = Column->Width;
    tnColumn.Alignment = Column->Alignment;
    tnColumn.TextFlags = Column->TextFlags;
    TreeNew_AddColumn(Manager->Handle, &tnColumn);

    return column;
}

PPH_CM_COLUMN PhCmFindColumn(
    __in PPH_CM_MANAGER Manager,
    __in PPH_STRINGREF PluginName,
    __in ULONG SubId
    )
{
    PLIST_ENTRY listEntry;
    PPH_CM_COLUMN column;

    listEntry = Manager->ColumnListHead.Flink;

    while (listEntry != &Manager->ColumnListHead)
    {
        column = CONTAINING_RECORD(listEntry, PH_CM_COLUMN, ListEntry);

        if (column->SubId == SubId && PhEqualStringRef2(PluginName, column->Plugin->Name, FALSE))
            return column;

        listEntry = listEntry->Flink;
    }

    return NULL;
}

BOOLEAN PhCmForwardMessage(
    __in HWND hwnd,
    __in PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in PPH_CM_MANAGER Manager
    )
{
    PH_PLUGIN_TREENEW_MESSAGE pluginMessage;
    PPH_PLUGIN plugin;

    switch (Message)
    {
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            PH_TREENEW_COLUMN tnColumn;
            PPH_CM_COLUMN column;

            if (getCellText->Id < Manager->MinId)
                return FALSE;

            if (!TreeNew_GetColumn(hwnd, getCellText->Id, &tnColumn))
                return FALSE;

            column = tnColumn.Context;
            pluginMessage.SubId = column->SubId;
            pluginMessage.Context = column->Context;
            plugin = column->Plugin;
        }
        break;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = Parameter1;
            PPH_CM_COLUMN column;

            if (customDraw->Column->Id < Manager->MinId)
                return FALSE;

            column = customDraw->Column->Context;
            pluginMessage.SubId = column->SubId;
            pluginMessage.Context = column->Context;
            plugin = column->Plugin;
        }
        break;
    case TreeNewColumnResized:
        {
            PPH_TREENEW_COLUMN tlColumn = Parameter1;
            PPH_CM_COLUMN column;

            if (tlColumn->Id < Manager->MinId)
                return FALSE;

            column = tlColumn->Context;
            pluginMessage.SubId = column->SubId;
            pluginMessage.Context = column->Context;
            plugin = column->Plugin;
        }
        break;
    default:
        return FALSE;
    }

    pluginMessage.TreeNewHandle = hwnd;
    pluginMessage.Message = Message;
    pluginMessage.Parameter1 = Parameter1;
    pluginMessage.Parameter2 = Parameter2;

    PhInvokeCallback(PhGetPluginCallback(plugin, PluginCallbackTreeNewMessage), &pluginMessage);

    return TRUE;
}

static int __cdecl PhCmpSortFunction(
    __in void *context,
    __in const void *elem1,
    __in const void *elem2
    )
{
    PPH_CM_SORT_CONTEXT sortContext = context;
    PVOID node1 = *(PVOID *)elem1;
    PVOID node2 = *(PVOID *)elem2;
    LONG result;

    result = sortContext->SortFunction(node1, node2, sortContext->SubId, sortContext->Context);

    return sortContext->PostSortFunction(result, node1, node2, sortContext->SortOrder);
}

BOOLEAN PhCmForwardSort(
    __in PPH_TREENEW_NODE *Nodes,
    __in ULONG NumberOfNodes,
    __in ULONG SortColumn,
    __in PH_SORT_ORDER SortOrder,
    __in PPH_CM_MANAGER Manager
    )
{
    PH_TREENEW_COLUMN tnColumn;
    PPH_CM_COLUMN column;
    PH_CM_SORT_CONTEXT sortContext;

    if (SortColumn < Manager->MinId)
        return FALSE;

    if (!TreeNew_GetColumn(Manager->Handle, SortColumn, &tnColumn))
        return TRUE;

    column = tnColumn.Context;

    if (!column->SortFunction)
        return TRUE;

    sortContext.SortFunction = column->SortFunction;
    sortContext.SubId = column->SubId;
    sortContext.Context = column->Context;
    sortContext.PostSortFunction = Manager->PostSortFunction;
    sortContext.SortOrder = SortOrder;
    qsort_s(Nodes, NumberOfNodes, sizeof(PVOID), PhCmpSortFunction, &sortContext);

    return TRUE;
}

BOOLEAN PhCmLoadSettings(
    __in HWND TreeNewHandle,
    __in PPH_STRINGREF Settings
    )
{
    return PhCmLoadSettingsEx(TreeNewHandle, NULL, Settings, NULL);
}

BOOLEAN PhCmLoadSettingsEx(
    __in HWND TreeNewHandle,
    __in_opt PPH_CM_MANAGER Manager,
    __in PPH_STRINGREF Settings,
    __in_opt PPH_STRINGREF SortSettings
    )
{
    BOOLEAN result = FALSE;
    PH_STRINGREF columnPart;
    PH_STRINGREF remainingColumnPart;
    PH_STRINGREF valuePart;
    PH_STRINGREF subPart;
    ULONG64 integer;
    ULONG total;
    BOOLEAN hasFixedColumn;
    ULONG count;
    ULONG i;
    PPH_HASHTABLE columnHashtable;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_KEY_VALUE_PAIR pair;
    LONG orderArray[PH_CM_ORDER_LIMIT];
    LONG maxOrder;

    if (Settings->Length == 0)
        return FALSE;

    columnHashtable = PhCreateSimpleHashtable(20);

    memset(orderArray, 0, sizeof(orderArray));

    remainingColumnPart = *Settings;

    while (remainingColumnPart.Length != 0)
    {
        PPH_TREENEW_COLUMN column;
        ULONG id;
        ULONG displayIndex;
        ULONG width;

        PhSplitStringRefAtChar(&remainingColumnPart, '|', &columnPart, &remainingColumnPart);

        if (columnPart.Length != 0)
        {
            // Id

            PhSplitStringRefAtChar(&columnPart, ',', &valuePart, &columnPart);

            if (valuePart.Length == 0)
                goto CleanupExit;

            if (valuePart.Buffer[0] == '+')
            {
                PH_STRINGREF pluginName;
                ULONG subId;
                PPH_CM_COLUMN cmColumn;

                // This is a plugin-owned column.

                if (!Manager)
                    continue;
                if (!PhEmParseCompoundId(&valuePart, &pluginName, &subId))
                    continue;

                cmColumn = PhCmFindColumn(Manager, &pluginName, subId);

                if (!cmColumn)
                    continue; // can't find the column, skip this part

                id = cmColumn->Id;
            }
            else
            {
                if (!PhStringToInteger64(&valuePart, 10, &integer))
                    goto CleanupExit;

                id = (ULONG)integer;
            }

            // Display Index

            PhSplitStringRefAtChar(&columnPart, ',', &valuePart, &columnPart);

            if (valuePart.Length == 0 || !PhStringToInteger64(&valuePart, 10, &integer))
                goto CleanupExit;

            displayIndex = (ULONG)integer;

            // Width

            if (columnPart.Length == 0 || !PhStringToInteger64(&columnPart, 10, &integer))
                goto CleanupExit;

            width = (ULONG)integer;

            column = PhAllocate(sizeof(PH_TREENEW_COLUMN));
            column->Id = id;
            column->DisplayIndex = displayIndex;
            column->Width = width;
            PhAddItemSimpleHashtable(columnHashtable, (PVOID)column->Id, column);
        }
    }

    TreeNew_SetRedraw(TreeNewHandle, FALSE);

    // Set visibility and width.

    i = 0;
    count = 0;
    total = TreeNew_GetColumnCount(TreeNewHandle);
    hasFixedColumn = !!TreeNew_GetFixedColumn(TreeNewHandle);

    while (count < total)
    {
        PH_TREENEW_COLUMN setColumn;
        PPH_TREENEW_COLUMN *columnPtr;

        if (TreeNew_GetColumn(TreeNewHandle, i, &setColumn))
        {
            columnPtr = (PPH_TREENEW_COLUMN *)PhFindItemSimpleHashtable(columnHashtable, (PVOID)i);

            if (columnPtr)
            {
                setColumn.Width = (*columnPtr)->Width;
                setColumn.Visible = TRUE;
                TreeNew_SetColumn(TreeNewHandle, TN_COLUMN_FLAG_VISIBLE | TN_COLUMN_WIDTH, &setColumn);

                TreeNew_GetColumn(TreeNewHandle, i, &setColumn); // get the Fixed and ViewIndex for use in the second pass
                (*columnPtr)->Fixed = setColumn.Fixed;
                (*columnPtr)->s.ViewIndex = setColumn.s.ViewIndex;

                // For compatibility reasons, normal columns have their display indicies stored 
                // one higher than usual (so they start from 1, not 0). Fix that here.
                if (hasFixedColumn && !(*columnPtr)->Fixed && (*columnPtr)->DisplayIndex != 0)
                    (*columnPtr)->DisplayIndex--;
            }
            else if (!setColumn.Fixed) // never hide the fixed column
            {
                setColumn.Visible = FALSE;
                TreeNew_SetColumn(TreeNewHandle, TN_COLUMN_FLAG_VISIBLE, &setColumn);
            }

            count++;
        }

        i++;
    }

    // Do a second pass to create the order array. This is because the ViewIndex of each column 
    // were unstable in the previous pass since we were both adding and removing columns.

    PhBeginEnumHashtable(columnHashtable, &enumContext);
    maxOrder = 0;

    while (pair = PhNextEnumHashtable(&enumContext))
    {
        PPH_TREENEW_COLUMN column;

        column = pair->Value;

        if (column->Fixed)
            continue; // fixed column cannot be re-ordered

        if (column->DisplayIndex < PH_CM_ORDER_LIMIT)
        {
            orderArray[column->DisplayIndex] = column->s.ViewIndex;

            if ((ULONG)maxOrder < column->DisplayIndex + 1)
                maxOrder = column->DisplayIndex + 1;
        }
    }

    // Set the order array.

    TreeNew_SetColumnOrderArray(TreeNewHandle, maxOrder, orderArray);

    TreeNew_SetRedraw(TreeNewHandle, TRUE);

    // Load sort settings.

    if (SortSettings && SortSettings->Length != 0)
    {
        PhSplitStringRefAtChar(SortSettings, ',', &valuePart, &subPart);

        if (valuePart.Length != 0 && subPart.Length != 0)
        {
            ULONG sortColumn;
            PH_SORT_ORDER sortOrder;

            sortColumn = -1;

            if (valuePart.Buffer[0] == '+')
            {
                PH_STRINGREF pluginName;
                ULONG subId;
                PPH_CM_COLUMN cmColumn;

                if (
                    Manager &&
                    PhEmParseCompoundId(&valuePart, &pluginName, &subId) &&
                    (cmColumn = PhCmFindColumn(Manager, &pluginName, subId))
                    )
                {
                    sortColumn = cmColumn->Id;
                }
            }
            else
            {
                PhStringToInteger64(&valuePart, 10, &integer);
                sortColumn = (ULONG)integer;
            }

            PhStringToInteger64(&subPart, 10, &integer);
            sortOrder = (PH_SORT_ORDER)integer;

            if (sortColumn != -1 && sortOrder <= DescendingSortOrder)
            {
                TreeNew_SetSort(TreeNewHandle, sortColumn, sortOrder);
            }
        }
    }

    result = TRUE;

CleanupExit:

    PhBeginEnumHashtable(columnHashtable, &enumContext);

    while (pair = PhNextEnumHashtable(&enumContext))
        PhFree(pair->Value);

    PhDereferenceObject(columnHashtable);

    return result;
}

PPH_STRING PhCmSaveSettings(
    __in HWND TreeNewHandle
    )
{
    return PhCmSaveSettingsEx(TreeNewHandle, NULL, NULL);
}

PPH_STRING PhCmSaveSettingsEx(
    __in HWND TreeNewHandle,
    __in_opt PPH_CM_MANAGER Manager,
    __out_opt PPH_STRING *SortSettings
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG i = 0;
    ULONG count = 0;
    ULONG total;
    ULONG increment;
    PH_TREENEW_COLUMN column;

    total = TreeNew_GetColumnCount(TreeNewHandle);

    if (TreeNew_GetFixedColumn(TreeNewHandle))
        increment = 1; // the first normal column should have a display index that starts with 1, for compatibility
    else
        increment = 0;

    PhInitializeStringBuilder(&stringBuilder, 100);

    while (count < total)
    {
        if (TreeNew_GetColumn(TreeNewHandle, i, &column))
        {
            if (column.Visible)
            {
                if (!Manager || i < Manager->MinId)
                {
                    PhAppendFormatStringBuilder(
                        &stringBuilder,
                        L"%u,%u,%u|",
                        i,
                        column.Fixed ? 0 : column.DisplayIndex + increment,
                        column.Width
                        );
                }
                else
                {
                    PPH_CM_COLUMN cmColumn;

                    cmColumn = column.Context;
                    PhAppendFormatStringBuilder(
                        &stringBuilder,
                        L"+%s+%u,%u,%u|",
                        cmColumn->Plugin->Name,
                        cmColumn->SubId,
                        column.DisplayIndex + increment,
                        column.Width
                        );
                }
            }

            count++;
        }

        i++;
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveStringBuilder(&stringBuilder, stringBuilder.String->Length / 2 - 1, 1);

    if (SortSettings)
    {
        ULONG sortColumn;
        PH_SORT_ORDER sortOrder;

        if (TreeNew_GetSort(TreeNewHandle, &sortColumn, &sortOrder))
        {
            if (sortOrder != NoSortOrder)
            {
                if (!Manager || sortColumn < Manager->MinId)
                {
                    *SortSettings = PhFormatString(L"%u,%u", sortColumn, sortOrder);
                }
                else
                {
                    PH_TREENEW_COLUMN column;
                    PPH_CM_COLUMN cmColumn;

                    if (TreeNew_GetColumn(TreeNewHandle, sortColumn, &column))
                    {
                        cmColumn = column.Context;
                        *SortSettings = PhFormatString(L"+%s+%u,%u", cmColumn->Plugin->Name, cmColumn->SubId, sortOrder);
                    }
                    else
                    {
                        *SortSettings = PhReferenceEmptyString();
                    }
                }
            }
            else
            {
                *SortSettings = PhCreateString(L"0,0");
            }
        }
        else
        {
            *SortSettings = PhReferenceEmptyString();
        }
    }

    return PhFinalStringBuilderString(&stringBuilder);
}
