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
    _Out_ PPH_CM_MANAGER Manager,
    _In_ HWND Handle,
    _In_ ULONG MinId,
    _In_ PPH_CM_POST_SORT_FUNCTION PostSortFunction
    )
{
    Manager->Handle = Handle;
    Manager->MinId = MinId;
    Manager->NextId = MinId;
    Manager->PostSortFunction = PostSortFunction;
    InitializeListHead(&Manager->ColumnListHead);
    Manager->NotifyList = NULL;
}

VOID PhCmDeleteManager(
    _In_ PPH_CM_MANAGER Manager
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

    if (Manager->NotifyList)
        PhDereferenceObject(Manager->NotifyList);
}

PPH_CM_COLUMN PhCmCreateColumn(
    _Inout_ PPH_CM_MANAGER Manager,
    _In_ PPH_TREENEW_COLUMN Column,
    _In_ struct _PH_PLUGIN *Plugin,
    _In_ ULONG SubId,
    _In_opt_ PVOID Context,
    _In_ PVOID SortFunction
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
    tnColumn.Visible = Column->Visible;
    tnColumn.CustomDraw = Column->CustomDraw;
    tnColumn.SortDescending = Column->SortDescending;
    tnColumn.Text = Column->Text;
    tnColumn.Width = Column->Width;
    tnColumn.Alignment = Column->Alignment;
    tnColumn.DisplayIndex = Column->Visible ? Column->DisplayIndex : -1;
    tnColumn.TextFlags = Column->TextFlags;
    TreeNew_AddColumn(Manager->Handle, &tnColumn);

    return column;
}

PPH_CM_COLUMN PhCmFindColumn(
    _In_ PPH_CM_MANAGER Manager,
    _In_ PPH_STRINGREF PluginName,
    _In_ ULONG SubId
    )
{
    PLIST_ENTRY listEntry;
    PPH_CM_COLUMN column;

    listEntry = Manager->ColumnListHead.Flink;

    while (listEntry != &Manager->ColumnListHead)
    {
        column = CONTAINING_RECORD(listEntry, PH_CM_COLUMN, ListEntry);

        if (column->SubId == SubId && PhEqualStringRef(PluginName, &column->Plugin->AppContext.AppName, FALSE))
            return column;

        listEntry = listEntry->Flink;
    }

    return NULL;
}

VOID PhCmSetNotifyPlugin(
    _In_ PPH_CM_MANAGER Manager,
    _In_ struct _PH_PLUGIN *Plugin
    )
{
    if (!Manager->NotifyList)
    {
        Manager->NotifyList = PhCreateList(8);
    }
    else
    {
        if (PhFindItemList(Manager->NotifyList, Plugin) != -1)
            return;
    }

    PhAddItemList(Manager->NotifyList, Plugin);
}

BOOLEAN PhCmForwardMessage(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_ PPH_CM_MANAGER Manager
    )
{
    PH_PLUGIN_TREENEW_MESSAGE pluginMessage;
    PPH_PLUGIN plugin;

    if (Message == TreeNewDestroying)
        return FALSE;

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
        {
            // Some plugins want to be notified about all messages.
            if (Manager->NotifyList)
            {
                ULONG i;

                for (i = 0; i < Manager->NotifyList->Count; i++)
                {
                    plugin = Manager->NotifyList->Items[i];

                    pluginMessage.TreeNewHandle = hwnd;
                    pluginMessage.Message = Message;
                    pluginMessage.Parameter1 = Parameter1;
                    pluginMessage.Parameter2 = Parameter2;
                    pluginMessage.SubId = 0;
                    pluginMessage.Context = NULL;

                    PhInvokeCallback(PhGetPluginCallback(plugin, PluginCallbackTreeNewMessage), &pluginMessage);
                }
            }
        }
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
    _In_ void *context,
    _In_ const void *elem1,
    _In_ const void *elem2
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
    _In_ PPH_TREENEW_NODE *Nodes,
    _In_ ULONG NumberOfNodes,
    _In_ ULONG SortColumn,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PPH_CM_MANAGER Manager
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
    _In_ HWND TreeNewHandle,
    _In_ PPH_STRINGREF Settings
    )
{
    return PhCmLoadSettingsEx(TreeNewHandle, NULL, 0, Settings, NULL);
}

BOOLEAN PhCmLoadSettingsEx(
    _In_ HWND TreeNewHandle,
    _In_opt_ PPH_CM_MANAGER Manager,
    _In_ ULONG Flags,
    _In_ PPH_STRINGREF Settings,
    _In_opt_ PPH_STRINGREF SortSettings
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

    if (Settings->Length != 0)
    {
        columnHashtable = PhCreateSimpleHashtable(20);

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

                if (!(Flags & PH_CM_COLUMN_WIDTHS_ONLY))
                {
                    if (valuePart.Length == 0 || !PhStringToInteger64(&valuePart, 10, &integer))
                        goto CleanupExit;

                    displayIndex = (ULONG)integer;
                }
                else
                {
                    if (valuePart.Length != 0)
                        goto CleanupExit;

                    displayIndex = -1;
                }

                // Width

                if (columnPart.Length == 0 || !PhStringToInteger64(&columnPart, 10, &integer))
                    goto CleanupExit;

                width = (ULONG)integer;

                column = PhAllocate(sizeof(PH_TREENEW_COLUMN));
                column->Id = id;
                column->DisplayIndex = displayIndex;
                column->Width = width;
                PhAddItemSimpleHashtable(columnHashtable, UlongToPtr(column->Id), column);
            }
        }

        TreeNew_SetRedraw(TreeNewHandle, FALSE);

        // Set visibility and width.

        i = 0;
        count = 0;
        total = TreeNew_GetColumnCount(TreeNewHandle);
        hasFixedColumn = !!TreeNew_GetFixedColumn(TreeNewHandle);
        memset(orderArray, 0, sizeof(orderArray));
        maxOrder = 0;

        while (count < total)
        {
            PH_TREENEW_COLUMN setColumn;
            PPH_TREENEW_COLUMN *columnPtr;

            if (TreeNew_GetColumn(TreeNewHandle, i, &setColumn))
            {
                columnPtr = (PPH_TREENEW_COLUMN *)PhFindItemSimpleHashtable(columnHashtable, UlongToPtr(i));

                if (!(Flags & PH_CM_COLUMN_WIDTHS_ONLY))
                {
                    if (columnPtr)
                    {
                        setColumn.Visible = TRUE;
                        setColumn.Width = (*columnPtr)->Width;
                        TreeNew_SetColumn(TreeNewHandle, TN_COLUMN_FLAG_VISIBLE | TN_COLUMN_WIDTH, &setColumn);

                        if (!setColumn.Fixed)
                        {
                            // For compatibility reasons, normal columns have their display indicies stored
                            // one higher than usual (so they start from 1, not 0). Fix that here.
                            if (hasFixedColumn && (*columnPtr)->DisplayIndex != 0)
                                (*columnPtr)->DisplayIndex--;

                            if ((*columnPtr)->DisplayIndex < PH_CM_ORDER_LIMIT)
                            {
                                orderArray[(*columnPtr)->DisplayIndex] = i;

                                if ((ULONG)maxOrder < (*columnPtr)->DisplayIndex + 1)
                                    maxOrder = (*columnPtr)->DisplayIndex + 1;
                            }
                        }
                    }
                    else if (!setColumn.Fixed) // never hide the fixed column
                    {
                        setColumn.Visible = FALSE;
                        TreeNew_SetColumn(TreeNewHandle, TN_COLUMN_FLAG_VISIBLE, &setColumn);
                    }
                }
                else
                {
                    if (columnPtr)
                    {
                        setColumn.Width = (*columnPtr)->Width;
                        TreeNew_SetColumn(TreeNewHandle, TN_COLUMN_WIDTH, &setColumn);
                    }
                }

                count++;
            }

            i++;
        }

        if (!(Flags & PH_CM_COLUMN_WIDTHS_ONLY))
        {
            // Set the order array.
            TreeNew_SetColumnOrderArray(TreeNewHandle, maxOrder, orderArray);
        }

        TreeNew_SetRedraw(TreeNewHandle, TRUE);

        result = TRUE;

CleanupExit:
        PhBeginEnumHashtable(columnHashtable, &enumContext);

        while (pair = PhNextEnumHashtable(&enumContext))
            PhFree(pair->Value);

        PhDereferenceObject(columnHashtable);
    }

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

    return result;
}

PPH_STRING PhCmSaveSettings(
    _In_ HWND TreeNewHandle
    )
{
    return PhCmSaveSettingsEx(TreeNewHandle, NULL, 0, NULL);
}

PPH_STRING PhCmSaveSettingsEx(
    _In_ HWND TreeNewHandle,
    _In_opt_ PPH_CM_MANAGER Manager,
    _In_ ULONG Flags,
    _Out_opt_ PPH_STRING *SortSettings
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
            if (!(Flags & PH_CM_COLUMN_WIDTHS_ONLY))
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
                            cmColumn->Plugin->Name.Buffer,
                            cmColumn->SubId,
                            column.DisplayIndex + increment,
                            column.Width
                            );
                    }
                }
            }
            else
            {
                if (!Manager || i < Manager->MinId)
                {
                    PhAppendFormatStringBuilder(
                        &stringBuilder,
                        L"%u,,%u|",
                        i,
                        column.Width
                        );
                }
                else
                {
                    PPH_CM_COLUMN cmColumn;

                    cmColumn = column.Context;
                    PhAppendFormatStringBuilder(
                        &stringBuilder,
                        L"+%s+%u,,%u|",
                        cmColumn->Plugin->Name.Buffer,
                        cmColumn->SubId,
                        column.Width
                        );
                }
            }

            count++;
        }

        i++;
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

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
                        *SortSettings = PhFormatString(L"+%s+%u,%u", cmColumn->Plugin->Name.Buffer, cmColumn->SubId, sortOrder);
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
