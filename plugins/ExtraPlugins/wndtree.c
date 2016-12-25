/*
 * Process Hacker Extra Plugins -
 *   Plugin Manager
 *
 * Copyright (C) 2016 dmex
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

#include "main.h"

BOOLEAN PluginsNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );
ULONG PluginsNodeHashtableHashFunction(
    _In_ PVOID Entry
    );
VOID DestroyPluginsNode(
    _In_ PPLUGIN_NODE Node
    );

VOID DeletePluginsTree(
    _In_ PWCT_CONTEXT Context
    )
{
    PPH_STRING settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(SETTING_NAME_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        DestroyPluginsNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

struct _PH_TN_FILTER_SUPPORT* GetPluginListFilterSupport(
    _In_ PWCT_CONTEXT Context
    )
{
    return &Context->FilterSupport;
}

BOOLEAN PluginsNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPLUGIN_NODE windowNode1 = *(PPLUGIN_NODE *)Entry1;
    PPLUGIN_NODE windowNode2 = *(PPLUGIN_NODE *)Entry2;

    return PhEqualString(windowNode1->InternalName, windowNode2->InternalName, TRUE);
}

ULONG PluginsNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashStringRef(&(*(PPLUGIN_NODE*)Entry)->InternalName->sr, TRUE);
}

VOID PluginsAddTreeNode(
    _In_ PWCT_CONTEXT Context,
    _In_ PPLUGIN_NODE Entry
    )
{
    PhInitializeTreeNewNode(&Entry->Node);

    memset(Entry->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);
    Entry->Node.TextCache = Entry->TextCache;
    Entry->Node.TextCacheSize = TREE_COLUMN_ITEM_MAXIMUM;

    PhAddEntryHashtable(Context->NodeHashtable, &Entry);
    PhAddItemList(Context->NodeList, Entry);

    if (Context->FilterSupport.NodeList)
    {
        Entry->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &Entry->Node);
    }
}

PPLUGIN_NODE FindTreeNode(
    _In_ PWCT_CONTEXT Context,
    _In_ TREE_PLUGIN_STATE State,
    _In_ PPH_STRING InternalName
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPLUGIN_NODE entry = Context->NodeList->Items[i];

        if (entry->State == State && PhEqualString(entry->InternalName, InternalName, TRUE))
            return entry;
    }

    return NULL;
}

VOID WeRemoveWindowNode(
    _In_ PWCT_CONTEXT Context,
    _In_ PPLUGIN_NODE WindowNode
    )
{
    ULONG index = 0;

    // Remove from hashtable/list and cleanup.
    PhRemoveEntryHashtable(Context->NodeHashtable, &WindowNode);

    if ((index = PhFindItemList(Context->NodeList, WindowNode)) != -1)
    {
        PhRemoveItemList(Context->NodeList, index);
    }

    DestroyPluginsNode(WindowNode);
}

VOID DestroyPluginsNode(
    _In_ PPLUGIN_NODE WindowNode
    )
{
    PhDereferenceObject(WindowNode);
}

#define SORT_FUNCTION(Column) PmPoolTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PmPoolTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPLUGIN_NODE node1 = *(PPLUGIN_NODE *)_elem1; \
    PPLUGIN_NODE node2 = *(PPLUGIN_NODE *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->Node.Index, (ULONG_PTR)node2->Node.Index); \
    \
    return PhModifySort(sortResult, ((PWCT_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareString(node1->Name, node2->Name, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Author)
{
    sortResult = PhCompareString(node1->Author, node2->Author, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Version)
{
    sortResult = PhCompareString(node1->Version, node2->Version, FALSE);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PluginsTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    )
{
    PWCT_CONTEXT context;
    PPLUGIN_NODE node;

    context = Context;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPLUGIN_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(Author),
                    SORT_FUNCTION(Version)
                };
                int (__cdecl *sortFunction)(void *, const void *, const void *);

                if (context->TreeNewSortColumn < TREE_COLUMN_ITEM_MAXIMUM)
                    sortFunction = sortFunctions[context->TreeNewSortColumn];
                else
                    sortFunction = NULL;

                if (sortFunction)
                {
                    qsort_s(context->NodeList->Items, context->NodeList->Count, sizeof(PVOID), sortFunction, context);
                }

                getChildren->Children = (PPH_TREENEW_NODE *)context->NodeList->Items;
                getChildren->NumberOfChildren = context->NodeList->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = (PPH_TREENEW_IS_LEAF)Parameter1;
            node = (PPLUGIN_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PPLUGIN_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case TREE_COLUMN_ITEM_NAME:
                getCellText->Text = PhGetStringRef(node->Name);
                break;
            case TREE_COLUMN_ITEM_AUTHOR:
                getCellText->Text = PhGetStringRef(node->Author);
                break;
            case TREE_COLUMN_ITEM_VERSION:
                getCellText->Text = PhGetStringRef(node->Version);
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = (PPH_TREENEW_GET_NODE_COLOR)Parameter1;
            node = (PPLUGIN_NODE)getNodeColor->Node;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &context->TreeNewSortColumn, &context->TreeNewSortOrder);
            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewKeyDown:
    case TreeNewNodeExpanding:
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            SendMessage(context->ParentWindowHandle, WM_COMMAND, WM_ACTION, (LPARAM)context);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = (PPH_TREENEW_MOUSE_EVENT)Parameter1;

            SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_WCTSHOWCONTEXTMENU, MAKELONG(mouseEvent->Location.x, mouseEvent->Location.y));
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = AscendingSortOrder;
            PhInitializeTreeNewColumnMenu(&data);

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = Parameter1;
            RECT rect = customDraw->CellRect;
            node = (PPLUGIN_NODE)customDraw->Node;

            switch (customDraw->Column->Id)
            {
            case TREE_COLUMN_ITEM_NAME:
                {
                    PH_STRINGREF text;
                    SIZE nameSize;
                    SIZE textSize;
                    
                    if (node->PluginOptions)
                    {
                        if (!node->Icon)
                        {
                            HBITMAP bitmapActive;

                            if (bitmapActive = LoadImageFromResources(PluginInstance->DllBase, 17, 17, MAKEINTRESOURCE(IDB_SETTINGS_PNG), TRUE))
                            {
                                node->Icon = CommonBitmapToIcon(bitmapActive, 17, 17);
                                DeleteObject(bitmapActive);
                            }
                        }

                        if (node->Icon)
                        {
                            DrawIconEx(
                                customDraw->Dc,
                                rect.left + 5,
                                rect.top + ((rect.bottom - rect.top) - 17) / 2,
                                node->Icon,
                                17,
                                17,
                                0,
                                NULL,
                                DI_NORMAL
                                );
                        }
                    }

                    rect.left += 19;
                    rect.left += 8;

                    rect.top += 5;
                    rect.right -= 5;
                    rect.bottom -= 8;

                    // top
                    SetTextColor(customDraw->Dc, RGB(0x0, 0x0, 0x0));
                    SelectObject(customDraw->Dc, context->TitleFontHandle);
                    text = PhIsNullOrEmptyString(node->Name) ? PhGetStringRef(node->InternalName) : PhGetStringRef(node->Name);
                    GetTextExtentPoint32(customDraw->Dc, text.Buffer, (ULONG)text.Length / 2, &nameSize);
                    DrawText(customDraw->Dc, text.Buffer, (ULONG)text.Length / 2, &rect, DT_TOP | DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE);
                    
                    // bottom
                    SetTextColor(customDraw->Dc, RGB(0x64, 0x64, 0x64));
                    SelectObject(customDraw->Dc, context->NormalFontHandle);
                    text = PhGetStringRef(node->Description);
                    GetTextExtentPoint32(customDraw->Dc, text.Buffer, (ULONG)text.Length / 2, &textSize);
                    DrawText(
                        customDraw->Dc,
                        text.Buffer, 
                        (ULONG)text.Length / 2, 
                        &rect, 
                        DT_BOTTOM | DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE
                        );
                }
                break;
            }
        }
        return TRUE;
    }

    return FALSE;
}

VOID PluginsClearTree(
    _In_ PWCT_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        DestroyPluginsNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
}

PPLUGIN_NODE WeGetSelectedWindowNode(
    _In_ PWCT_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPLUGIN_NODE windowNode = Context->NodeList->Items[i];

        if (windowNode->Node.Selected)
            return windowNode;
    }

    return NULL;
}

VOID WeGetSelectedWindowNodes(
    _In_ PWCT_CONTEXT Context,
    __out PPLUGIN_NODE **Windows,
    __out PULONG NumberOfWindows
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPLUGIN_NODE node = (PPLUGIN_NODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
            PhAddItemList(list, node);
    }

    *Windows = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfWindows = list->Count;

    PhDereferenceObject(list);
}

VOID InitializePluginsTree(
    _In_ PWCT_CONTEXT Context,
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle
    )
{
    LOGFONT logFont;

    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPLUGIN_NODE),
        PluginsNodeHashtableCompareFunction,
        PluginsNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(100);

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeNewHandle = TreeNewHandle;
    PhSetControlTheme(TreeNewHandle, L"explorer");

    if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, 0))
    {
        Context->TitleFontHandle = CreateFont(
            -PhMultiplyDivideSigned(-14, PhGlobalDpi, 72),
            0,
            0,
            0,
            FW_BOLD,
            FALSE,
            FALSE,
            FALSE,
            ANSI_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY | ANTIALIASED_QUALITY,
            DEFAULT_PITCH,
            logFont.lfFaceName
            );

        Context->NormalFontHandle = CreateFont(
            -PhMultiplyDivideSigned(-10, PhGlobalDpi, 72),
            0,
            0,
            0,
            FW_NORMAL,
            FALSE,
            FALSE,
            FALSE,
            ANSI_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY | ANTIALIASED_QUALITY,
            DEFAULT_PITCH,
            logFont.lfFaceName
            );
    }

    TreeNew_SetCallback(TreeNewHandle, PluginsTreeNewCallback, Context);
    TreeNew_SetRowHeight(TreeNewHandle, 48);

    PhAddTreeNewColumnEx2(TreeNewHandle, TREE_COLUMN_ITEM_NAME, TRUE, L"Plugin", 80, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_NAME, 0, TN_COLUMN_FLAG_CUSTOMDRAW);
    PhAddTreeNewColumnEx2(TreeNewHandle, TREE_COLUMN_ITEM_AUTHOR, TRUE, L"Author", 80, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_AUTHOR, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, TREE_COLUMN_ITEM_VERSION, TRUE, L"Version", 80, PH_ALIGN_CENTER, TREE_COLUMN_ITEM_VERSION, DT_CENTER, 0);

    TreeNew_SetSort(TreeNewHandle, 0, NoSortOrder);

    PPH_STRING settings = PhGetStringSetting(SETTING_NAME_TREE_LIST_COLUMNS);
    PhCmLoadSettings(TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, TreeNewHandle, Context->NodeList);
}