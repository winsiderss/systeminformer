/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2017-2022
 *
 */

#include <phapp.h>
#include <emenu.h>
#include <settings.h>
#include <colmgr.h>
#include <phplug.h>
#include <phsettings.h>

#define WM_PH_PLUGINS_SHOWPROPERTIES (WM_APP + 401)

static HANDLE PhPluginsThreadHandle = NULL;
static HWND PhPluginsWindowHandle = NULL;
static PH_EVENT PhPluginsInitializedEvent = PH_EVENT_INIT;

typedef struct _PH_PLUGMAN_CONTEXT
{
    HWND WindowHandle;
    HWND TreeNewHandle;
    HFONT NormalFontHandle;
    HFONT TitleFontHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
    PH_LAYOUT_MANAGER LayoutManager;
} PH_PLUGMAN_CONTEXT, *PPH_PLUGMAN_CONTEXT;

typedef enum _PH_PLUGIN_TREE_ITEM_MENU
{
    PH_PLUGIN_TREE_ITEM_MENU_UNINSTALL,
    PH_PLUGIN_TREE_ITEM_MENU_DISABLE,
    PH_PLUGIN_TREE_ITEM_MENU_PROPERTIES
} PH_PLUGIN_TREE_ITEM_MENU;

typedef enum _PH_PLUGIN_TREE_COLUMN_ITEM
{
    PH_PLUGIN_TREE_COLUMN_ITEM_NAME,
    PH_PLUGIN_TREE_COLUMN_ITEM_VERSION,
    PH_PLUGIN_TREE_COLUMN_ITEM_MAXIMUM
} PH_PLUGIN_TREE_COLUMN_ITEM;

typedef struct _PH_PLUGIN_TREE_ROOT_NODE
{
    PH_TREENEW_NODE Node;

    BOOLEAN PluginOptions;
    PPH_PLUGIN PluginInstance;

    PPH_STRING InternalName;
    PPH_STRING Name;
    PPH_STRING Version;
    PPH_STRING Description;

    PH_STRINGREF TextCache[PH_PLUGIN_TREE_COLUMN_ITEM_MAXIMUM];
} PH_PLUGIN_TREE_ROOT_NODE, *PPH_PLUGIN_TREE_ROOT_NODE;

INT_PTR CALLBACK PhpPluginPropertiesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

ULONG PhpDisabledPluginsCount(
    VOID
    );

INT_PTR CALLBACK PhpPluginsDisabledDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#pragma region Plugin TreeList

#define SORT_FUNCTION(Column) PluginsTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PluginsTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_PLUGIN_TREE_ROOT_NODE node1 = *(PPH_PLUGIN_TREE_ROOT_NODE*)_elem1; \
    PPH_PLUGIN_TREE_ROOT_NODE node2 = *(PPH_PLUGIN_TREE_ROOT_NODE*)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->Node.Index, (ULONG_PTR)node2->Node.Index); \
    \
    return PhModifySort(sortResult, ((PPH_PLUGMAN_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringWithNull(node1->Name, node2->Name, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Version)
{
    sortResult = PhCompareStringWithNull(node1->Version, node2->Version, TRUE);
}
END_SORT_FUNCTION

VOID PluginsLoadSettingsTreeList(
    _Inout_ PPH_PLUGMAN_CONTEXT Context
    )
{
    PPH_STRING settings;
    
    settings = PhGetStringSetting(L"PluginManagerTreeListColumns");
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);
}

VOID PluginsSaveSettingsTreeList(
    _Inout_ PPH_PLUGMAN_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(L"PluginManagerTreeListColumns", &settings->sr);
    PhDereferenceObject(settings);
}

BOOLEAN PluginsNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_PLUGIN_TREE_ROOT_NODE node1 = *(PPH_PLUGIN_TREE_ROOT_NODE *)Entry1;
    PPH_PLUGIN_TREE_ROOT_NODE node2 = *(PPH_PLUGIN_TREE_ROOT_NODE *)Entry2;

    return PhEqualString(node1->InternalName, node2->InternalName, TRUE);
}

ULONG PluginsNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashStringRef(&(*(PPH_PLUGIN_TREE_ROOT_NODE*)Entry)->InternalName->sr, TRUE);
}

VOID DestroyPluginsNode(
    _In_ PPH_PLUGIN_TREE_ROOT_NODE Node
    )
{
    PhClearReference(&Node->InternalName);
    PhClearReference(&Node->Name);
    PhClearReference(&Node->Version);
    PhClearReference(&Node->Description);

    PhFree(Node);
}

PPH_PLUGIN_TREE_ROOT_NODE AddPluginsNode(
    _Inout_ PPH_PLUGMAN_CONTEXT Context,
    _In_ PPH_PLUGIN Plugin
    )
{
    PPH_PLUGIN_TREE_ROOT_NODE pluginNode;
    PH_IMAGE_VERSION_INFO versionInfo;
    PPH_STRING fileName;

    pluginNode = PhAllocate(sizeof(PH_PLUGIN_TREE_ROOT_NODE));
    memset(pluginNode, 0, sizeof(PH_PLUGIN_TREE_ROOT_NODE));

    PhInitializeTreeNewNode(&pluginNode->Node);

    memset(pluginNode->TextCache, 0, sizeof(PH_STRINGREF) * PH_PLUGIN_TREE_COLUMN_ITEM_MAXIMUM);
    pluginNode->Node.TextCache = pluginNode->TextCache;
    pluginNode->Node.TextCacheSize = PH_PLUGIN_TREE_COLUMN_ITEM_MAXIMUM;

    pluginNode->PluginInstance = Plugin;
    pluginNode->PluginOptions = Plugin->Information.HasOptions;
    pluginNode->InternalName = PhCreateString2(&Plugin->Name);
    if (Plugin->Information.DisplayName)
        pluginNode->Name = PhCreateString(Plugin->Information.DisplayName);
    if (Plugin->Information.Description)
        pluginNode->Description = PhCreateString(Plugin->Information.Description);

    if (fileName = PhGetPluginFileName(Plugin))
    {
        if (PhInitializeImageVersionInfoEx(&versionInfo, &fileName->sr, FALSE))
        {
            pluginNode->Version = PhReferenceObject(versionInfo.FileVersion);
            PhDeleteImageVersionInfo(&versionInfo);
        }

        PhDereferenceObject(fileName);
    }

    PhAddEntryHashtable(Context->NodeHashtable, &pluginNode);
    PhAddItemList(Context->NodeList, pluginNode);

    TreeNew_NodesStructured(Context->TreeNewHandle);

    return pluginNode;
}

PPH_PLUGIN_TREE_ROOT_NODE FindPluginsNode(
    _In_ PPH_PLUGMAN_CONTEXT Context,
    _In_ PPH_STRING InternalName
    )
{
    PH_PLUGIN_TREE_ROOT_NODE lookupPluginsNode;
    PPH_PLUGIN_TREE_ROOT_NODE lookupPluginsNodePtr = &lookupPluginsNode;
    PPH_PLUGIN_TREE_ROOT_NODE *pluginsNode;

    lookupPluginsNode.InternalName = InternalName;

    pluginsNode = (PPH_PLUGIN_TREE_ROOT_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupPluginsNodePtr
        );

    if (pluginsNode)
        return *pluginsNode;
    else
        return NULL;
}

VOID RemovePluginsNode(
    _In_ PPH_PLUGMAN_CONTEXT Context,
    _In_ PPH_PLUGIN_TREE_ROOT_NODE Node
    )
{
    ULONG index = 0;

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
    {
        PhRemoveItemList(Context->NodeList, index);
    }

    DestroyPluginsNode(Node);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID UpdatePluginsNode(
    _In_ PPH_PLUGMAN_CONTEXT Context,
    _In_ PPH_PLUGIN_TREE_ROOT_NODE Node
    )
{
    memset(Node->TextCache, 0, sizeof(PH_STRINGREF) * PH_PLUGIN_TREE_COLUMN_ITEM_MAXIMUM);

    PhInvalidateTreeNewNode(&Node->Node, TN_CACHE_COLOR);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

BOOLEAN NTAPI PluginsTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGMAN_CONTEXT context = Context;
    PPH_PLUGIN_TREE_ROOT_NODE node;

    if (!context)
        return FALSE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren)
                break;

            node = (PPH_PLUGIN_TREE_ROOT_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Name),
                    //SORT_FUNCTION(Author),
                    SORT_FUNCTION(Version)
                };
                int (__cdecl *sortFunction)(void *, const void *, const void *);

                if (context->TreeNewSortColumn < PH_PLUGIN_TREE_COLUMN_ITEM_MAXIMUM)
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

            if (!isLeaf)
                break;

            node = (PPH_PLUGIN_TREE_ROOT_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;

            if (!getCellText)
                break;

            node = (PPH_PLUGIN_TREE_ROOT_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PH_PLUGIN_TREE_COLUMN_ITEM_NAME:
                getCellText->Text = PhGetStringRef(node->Name);
                break;
            case PH_PLUGIN_TREE_COLUMN_ITEM_VERSION:
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
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;

            if (!getNodeColor)
                break;

            node = (PPH_PLUGIN_TREE_ROOT_NODE)getNodeColor->Node;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &context->TreeNewSortColumn, &context->TreeNewSortOrder);
            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            if (!keyEvent)
                break;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(context->WindowHandle, WM_COMMAND, ID_OBJECT_COPY, 0);
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(context->TreeNewHandle, 0, -1);
                break;
            case VK_DELETE:
                SendMessage(context->WindowHandle, WM_COMMAND, ID_OBJECT_CLOSE, 0);
                break;
            }
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;

            SendMessage(context->WindowHandle, WM_COMMAND, WM_PH_PLUGINS_SHOWPROPERTIES, (LPARAM)mouseEvent);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;
            
            SendMessage(context->WindowHandle, WM_COMMAND, ID_SHOWCONTEXTMENU, (LPARAM)contextMenuEvent);
        }
        return TRUE;
    //case TreeNewHeaderRightClick:
    //    {
    //        PH_TN_COLUMN_MENU_DATA data;
    //
    //        data.TreeNewHandle = hwnd;
    //        data.MouseEvent = Parameter1;
    //        data.DefaultSortColumn = 0;
    //        data.DefaultSortOrder = AscendingSortOrder;
    //        PhInitializeTreeNewColumnMenu(&data);
    //
    //        data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
    //            PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
    //        PhHandleTreeNewColumnMenu(&data);
    //        PhDeleteTreeNewColumnMenu(&data);
    //    }
    //    return TRUE;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = Parameter1;
            RECT rect;

            if (!customDraw)
                break;

            rect = customDraw->CellRect;
            node = (PPH_PLUGIN_TREE_ROOT_NODE)customDraw->Node;

            switch (customDraw->Column->Id)
            {
            case PH_PLUGIN_TREE_COLUMN_ITEM_NAME:
                {
                    PH_STRINGREF text;
                    SIZE nameSize;
                    SIZE textSize;
                    LONG dpiValue;

                    dpiValue = PhGetWindowDpi(hwnd);

                    rect.left += PhGetDpi(15, dpiValue);
                    rect.top += PhGetDpi(5, dpiValue);
                    rect.right -= PhGetDpi(5, dpiValue);
                    rect.bottom -= PhGetDpi(8, dpiValue);

                    // top
                    if (PhEnableThemeSupport)
                        SetTextColor(customDraw->Dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    else
                        SetTextColor(customDraw->Dc, RGB(0x0, 0x0, 0x0));

                    SelectFont(customDraw->Dc, context->TitleFontHandle);
                    text = PhIsNullOrEmptyString(node->Name) ? PhGetStringRef(node->InternalName) : PhGetStringRef(node->Name);
                    GetTextExtentPoint32(customDraw->Dc, text.Buffer, (ULONG)text.Length / sizeof(WCHAR), &nameSize);
                    DrawText(customDraw->Dc, text.Buffer, (ULONG)text.Length / sizeof(WCHAR), &rect, DT_TOP | DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE);

                    // bottom
                    if (PhEnableThemeSupport)
                        SetTextColor(customDraw->Dc, RGB(0x90, 0x90, 0x90));
                    else
                        SetTextColor(customDraw->Dc, RGB(0x64, 0x64, 0x64));

                    SelectFont(customDraw->Dc, context->NormalFontHandle);
                    text = PhGetStringRef(node->Description);
                    GetTextExtentPoint32(customDraw->Dc, text.Buffer, (ULONG)text.Length / sizeof(WCHAR), &textSize);
                    DrawText(
                        customDraw->Dc,
                        text.Buffer,
                        (ULONG)text.Length / sizeof(WCHAR),
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

VOID ClearPluginsTree(
    _In_ PPH_PLUGMAN_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        DestroyPluginsNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

PPH_PLUGIN_TREE_ROOT_NODE GetSelectedPluginsNode(
    _In_ PPH_PLUGMAN_CONTEXT Context
    )
{
    PPH_PLUGIN_TREE_ROOT_NODE pluginNode = NULL;

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        pluginNode = Context->NodeList->Items[i];

        if (pluginNode->Node.Selected)
            return pluginNode;
    }

    return NULL;
}

BOOLEAN GetSelectedPluginsNodes(
    _In_ PPH_PLUGMAN_CONTEXT Context,
    _Out_ PPH_PLUGIN_TREE_ROOT_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_PLUGIN_TREE_ROOT_NODE node = (PPH_PLUGIN_TREE_ROOT_NODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node);
        }
    }

    if (list->Count)
    {
        *Nodes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
        *NumberOfNodes = list->Count;

        PhDereferenceObject(list);
        return TRUE;
    }

    PhDereferenceObject(list);
    return FALSE;
}

VOID InitializePluginsTree(
    _Inout_ PPH_PLUGMAN_CONTEXT Context
    )
{
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(Context->WindowHandle);

    Context->NodeList = PhCreateList(20);
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPH_PLUGIN_TREE_ROOT_NODE),
        PluginsNodeHashtableEqualFunction,
        PluginsNodeHashtableHashFunction,
        20
        );

    Context->NormalFontHandle = PhCreateCommonFont(-10, FW_NORMAL, NULL, dpiValue);
    Context->TitleFontHandle = PhCreateCommonFont(-14, FW_BOLD, NULL, dpiValue);

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");

    TreeNew_SetCallback(Context->TreeNewHandle, PluginsTreeNewCallback, Context);
    TreeNew_SetRowHeight(Context->TreeNewHandle, PhGetDpi(48, dpiValue));

    PhAddTreeNewColumnEx2(Context->TreeNewHandle, PH_PLUGIN_TREE_COLUMN_ITEM_NAME, TRUE, L"Plugin", 80, PH_ALIGN_LEFT, 0, 0, TN_COLUMN_FLAG_CUSTOMDRAW);
    //PhAddTreeNewColumnEx2(Context->TreeNewHandle, PH_PLUGIN_TREE_COLUMN_ITEM_AUTHOR, TRUE, L"Author", 80, PH_ALIGN_LEFT, 1, 0, 0);
    //PhAddTreeNewColumnEx2(Context->TreeNewHandle, PH_PLUGIN_TREE_COLUMN_ITEM_VERSION, TRUE, L"Version", 80, PH_ALIGN_CENTER, 2, DT_CENTER, 0);

    TreeNew_SetTriState(Context->TreeNewHandle, TRUE);

    PluginsLoadSettingsTreeList(Context);
}

VOID DeletePluginsTree(
    _In_ PPH_PLUGMAN_CONTEXT Context
    )
{
    if (Context->TitleFontHandle)
        DeleteFont(Context->TitleFontHandle);
    if (Context->NormalFontHandle)
        DeleteFont(Context->NormalFontHandle);

    PluginsSaveSettingsTreeList(Context);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        DestroyPluginsNode(Context->NodeList->Items[i]);

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

#pragma endregion

PPH_STRING PhpGetPluginBaseName(
    _In_ PPH_PLUGIN Plugin
    )
{
    PPH_STRING fileName;
    PPH_STRING baseName;

    if (fileName = PhGetPluginFileName(Plugin))
    {
        PH_STRINGREF pathNamePart;
        PH_STRINGREF baseNamePart;

        if (PhSplitStringRefAtLastChar(&fileName->sr, OBJ_NAME_PATH_SEPARATOR, &pathNamePart, &baseNamePart))
        {
            baseName = PhCreateString2(&baseNamePart);
            PhDereferenceObject(fileName);
            return baseName;
        }

        baseName = PhCreateString2(&Plugin->Name);
        PhDereferenceObject(fileName);
        return baseName;
    }
    else
    {
        // Fake disabled plugin.
        baseName = PhCreateString2(&Plugin->Name);
        return baseName;
    }
}

BOOLEAN NTAPI PhpEnumeratePluginCallback(
    _In_ PPH_PLUGIN Information,
    _In_opt_ PVOID Context
    )
{
    PPH_STRING pluginBaseName;

    pluginBaseName = PhpGetPluginBaseName(Information);

    if (!PhIsPluginDisabled(&pluginBaseName->sr))
    {
        AddPluginsNode(Context, Information);
    }

    PhDereferenceObject(pluginBaseName);
    return TRUE;
}

INT_PTR CALLBACK PhPluginsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_PLUGMAN_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PH_PLUGMAN_CONTEXT));
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
            context->WindowHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_PLUGINTREE);

            InitializePluginsTree(context);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_INSTRUCTION), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_DISABLED), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            PhEnumeratePlugins(PhpEnumeratePluginCallback, context);
            TreeNew_AutoSizeColumn(context->TreeNewHandle, PH_PLUGIN_TREE_COLUMN_ITEM_NAME, TN_AUTOSIZE_REMAINING_SPACE);
            PhSetWindowText(GetDlgItem(hwndDlg, IDC_DISABLED), PhaFormatString(L"Disabled Plugins (%lu)", PhpDisabledPluginsCount())->Buffer);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            DeletePluginsTree(context);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_DISABLED:
                {
                    DialogBox(
                        PhInstanceHandle,
                        MAKEINTRESOURCE(IDD_PLUGINSDISABLED),
                        hwndDlg,
                        PhpPluginsDisabledDlgProc
                        );

                    ClearPluginsTree(context);
                    PhEnumeratePlugins(PhpEnumeratePluginCallback, context);
                    TreeNew_AutoSizeColumn(context->TreeNewHandle, PH_PLUGIN_TREE_COLUMN_ITEM_NAME, TN_AUTOSIZE_REMAINING_SPACE);
                    PhSetWindowText(GetDlgItem(hwndDlg, IDC_DISABLED), PhaFormatString(L"Disabled Plugins (%lu)", PhpDisabledPluginsCount())->Buffer);
                }
                break;
            case ID_SHOWCONTEXTMENU:
                {
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM selectedItem;
                    //PPH_EMENU_ITEM uninstallItem;
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
                    PPH_PLUGIN_TREE_ROOT_NODE selectedNode = (PPH_PLUGIN_TREE_ROOT_NODE)contextMenuEvent->Node;

                    if (!selectedNode)
                        break;

                    menu = PhCreateEMenu();
                    //PhInsertEMenuItem(menu, uninstallItem = PhCreateEMenuItem(0, PH_PLUGIN_TREE_ITEM_MENU_UNINSTALL, L"Uninstall", NULL, NULL), ULONG_MAX);
                    //PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_PLUGIN_TREE_ITEM_MENU_DISABLE, L"Disable", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_PLUGIN_TREE_ITEM_MENU_PROPERTIES, L"Properties", NULL, NULL), ULONG_MAX);

                    //if (!PhGetOwnTokenAttributes().Elevated)
                    //{
                    //    HBITMAP shieldBitmap;
                    //
                    //    if (shieldBitmap = PhGetShieldBitmap())
                    //        uninstallItem->Bitmap = shieldBitmap;
                    //}

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        contextMenuEvent->Location.x,
                        contextMenuEvent->Location.y
                        );

                    if (selectedItem && selectedItem->Id != ULONG_MAX)
                    {
                        switch (selectedItem->Id)
                        {
                        case PH_PLUGIN_TREE_ITEM_MENU_UNINSTALL:
                            {
                                //if (PhShowConfirmMessage(
                                //    hwndDlg,
                                //    L"Uninstall",
                                //    PhGetString(selectedNode->Name),
                                //    L"Changes may require a restart to take effect...",
                                //    TRUE
                                //    ))
                                //{
                                //
                                //}
                            }
                            break;
                        case PH_PLUGIN_TREE_ITEM_MENU_DISABLE:
                            {
                                PPH_STRING baseName = PhpGetPluginBaseName(selectedNode->PluginInstance);
          
                                PhSetPluginDisabled(&baseName->sr, TRUE);

                                RemovePluginsNode(context, selectedNode);

                                PhSetWindowText(GetDlgItem(hwndDlg, IDC_DISABLED), PhaFormatString(L"Disabled Plugins (%lu)", PhpDisabledPluginsCount())->Buffer);

                                PhDereferenceObject(baseName);
                            }
                            break;
                        case PH_PLUGIN_TREE_ITEM_MENU_PROPERTIES:
                            {
                                DialogBoxParam(
                                    PhInstanceHandle, 
                                    MAKEINTRESOURCE(IDD_PLUGINPROPERTIES), 
                                    hwndDlg, 
                                    PhpPluginPropertiesDlgProc,
                                    (LPARAM)selectedNode->PluginInstance
                                    );
                            }
                            break;
                        }
                    }
                }
                break;
            case WM_PH_PLUGINS_SHOWPROPERTIES:
                {
                    PPH_TREENEW_MOUSE_EVENT mouseEvent = (PPH_TREENEW_MOUSE_EVENT)lParam;
                    PPH_PLUGIN_TREE_ROOT_NODE selectedNode = (PPH_PLUGIN_TREE_ROOT_NODE)mouseEvent->Node;

                    if (!selectedNode)
                        break;

                    DialogBoxParam(
                        PhInstanceHandle,
                        MAKEINTRESOURCE(IDD_PLUGINPROPERTIES),
                        hwndDlg,
                        PhpPluginPropertiesDlgProc,
                        (LPARAM)selectedNode->PluginInstance
                        );
                }
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
            TreeNew_AutoSizeColumn(context->TreeNewHandle, PH_PLUGIN_TREE_COLUMN_ITEM_NAME, TN_AUTOSIZE_REMAINING_SPACE);
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

//NTSTATUS PhpPluginsDialogThreadStart(
//    _In_ PVOID Parameter
//    )
//{
//    BOOL result;
//    MSG message;
//    PH_AUTO_POOL autoPool;
//
//    PhInitializeAutoPool(&autoPool);
//
//    PhPluginsWindowHandle = PhCreateDialog(
//        PhInstanceHandle,
//        MAKEINTRESOURCE(IDD_PLUGINS),
//        NULL,
//        PhPluginsDlgProc,
//        NULL
//        );
//
//    PhSetEvent(&PhPluginsInitializedEvent);
//
//    while (result = GetMessage(&message, NULL, 0, 0))
//    {
//        if (result == -1)
//            break;
//
//        if (!IsDialogMessage(PhPluginsWindowHandle, &message))
//        {
//            TranslateMessage(&message);
//            DispatchMessage(&message);
//        }
//
//        PhDrainAutoPool(&autoPool);
//    }
//
//    PhDeleteAutoPool(&autoPool);
//    PhResetEvent(&PhPluginsInitializedEvent);
//
//    if (PhPluginsThreadHandle)
//    {
//        NtClose(PhPluginsThreadHandle);
//        PhPluginsThreadHandle = NULL;
//    }
//
//    return STATUS_SUCCESS;
//}
//
//VOID PhShowPluginsDialog(
//    _In_ HWND ParentWindowHandle
//    )
//{
//    if (PhPluginsEnabled)
//    {
//        if (!PhPluginsThreadHandle)
//        {
//            if (!NT_SUCCESS(PhCreateThreadEx(&PhPluginsThreadHandle, PhpPluginsDialogThreadStart, NULL)))
//            {
//                PhShowError(PhMainWndHandle, L"%s", L"Unable to create the window.");
//                return;
//            }
//
//            PhWaitForEvent(&PhPluginsInitializedEvent, NULL);
//        }
//
//        PostMessage(PhPluginsWindowHandle, WM_PH_PLUGINS_SHOWDIALOG, 0, 0);
//    }
//    else
//    {
//        PhShowInformation2(
//            ParentWindowHandle, 
//            L"Plugins are not enabled.",
//            L"%s",
//            L"To use plugins enable them in Options and restart System Informer."
//            );
//    }
//}

VOID PhpRefreshPluginDetails(
    _In_ HWND hwndDlg,
    _In_ PPH_PLUGIN SelectedPlugin
    )
{
    PPH_STRING fileName = NULL;
    PPH_STRING baseName = NULL;
    PH_IMAGE_VERSION_INFO versionInfo;

    if (fileName = PhGetPluginFileName(SelectedPlugin))
        baseName = PH_AUTO(PhGetBaseName(fileName));

    PhSetDialogItemText(hwndDlg, IDC_NAME, SelectedPlugin->Information.DisplayName ? SelectedPlugin->Information.DisplayName : L"(unnamed)");
    PhSetDialogItemText(hwndDlg, IDC_INTERNALNAME, SelectedPlugin->Name.Buffer);
    PhSetDialogItemText(hwndDlg, IDC_AUTHOR, SelectedPlugin->Information.Author);
    PhSetDialogItemText(hwndDlg, IDC_FILENAME, PhGetStringOrEmpty(baseName));
    PhSetDialogItemText(hwndDlg, IDC_DESCRIPTION, SelectedPlugin->Information.Description);
    PhSetDialogItemText(hwndDlg, IDC_URL, SelectedPlugin->Information.Url);

    if (fileName && PhInitializeImageVersionInfoEx(&versionInfo, &fileName->sr, FALSE))
    {
        PhSetDialogItemText(hwndDlg, IDC_VERSION, PhGetStringOrDefault(versionInfo.FileVersion, L"Unknown"));
        PhDeleteImageVersionInfo(&versionInfo);
    }
    else
    {
        PhSetDialogItemText(hwndDlg, IDC_VERSION, L"Unknown");
    }

    ShowWindow(GetDlgItem(hwndDlg, IDC_OPENURL), SelectedPlugin->Information.Url ? SW_SHOW : SW_HIDE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS), SelectedPlugin->Information.HasOptions);

    PhClearReference(&fileName);
}

INT_PTR CALLBACK PhpPluginPropertiesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_PLUGIN selectedPlugin = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        selectedPlugin = (PPH_PLUGIN)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, selectedPlugin);
    }
    else
    {
        selectedPlugin = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
    }

    if (selectedPlugin == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhpRefreshPluginDetails(hwndDlg, selectedPlugin);

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDOK));

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_OPTIONS:
                {
                    PhInvokeCallback(PhGetPluginCallback(selectedPlugin, PluginCallbackShowOptions), hwndDlg);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case NM_CLICK:
                {
                    if (header->hwndFrom == GetDlgItem(hwndDlg, IDC_OPENURL))
                    {
                        PhShellExecute(hwndDlg, selectedPlugin->Information.Url, NULL);
                    }
                }
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

typedef struct _PLUGIN_DISABLED_CONTEXT
{
    PH_QUEUED_LOCK ListLock;
    HWND DialogHandle;
    HWND ListViewHandle;
} PLUGIN_DISABLED_CONTEXT, *PPLUGIN_DISABLED_CONTEXT;

VOID PhpAddDisabledPlugins(
    _In_ PPLUGIN_DISABLED_CONTEXT Context
    )
{
    PPH_STRING disabled;
    PH_STRINGREF remainingPart;
    PH_STRINGREF part;
    PPH_STRING displayText;
    INT lvItemIndex;

    disabled = PhGetStringSetting(L"DisabledPlugins");
    remainingPart = disabled->sr;

    while (remainingPart.Length)
    {
        PhSplitStringRefAtChar(&remainingPart, L'|', &part, &remainingPart);

        if (part.Length)
        {
            displayText = PhCreateString2(&part);

            PhAcquireQueuedLockExclusive(&Context->ListLock);
            lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, PhGetString(displayText), displayText);
            PhReleaseQueuedLockExclusive(&Context->ListLock);

            ListView_SetCheckState(Context->ListViewHandle, lvItemIndex, TRUE);
        }
    }

    PhDereferenceObject(disabled);
}

ULONG PhpDisabledPluginsCount(
    VOID
    )
{
    PPH_STRING disabled;
    PH_STRINGREF remainingPart;
    PH_STRINGREF part;
    ULONG count = 0;

    disabled = PhGetStringSetting(L"DisabledPlugins");
    remainingPart = disabled->sr;

    while (remainingPart.Length)
    {
        PhSplitStringRefAtChar(&remainingPart, L'|', &part, &remainingPart);

        if (part.Length)
            count++;
    }

    PhDereferenceObject(disabled);

    return count;
}

INT_PTR CALLBACK PhpPluginsDisabledDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPLUGIN_DISABLED_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(PLUGIN_DISABLED_CONTEXT));
        memset(context, 0, sizeof(PLUGIN_DISABLED_CONTEXT));

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
            context = NULL;
        }
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->DialogHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST_DISABLED);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            ListView_SetExtendedListViewStyleEx(context->ListViewHandle,
                LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER,
                LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 400, L"Property");
            PhSetExtendedListView(context->ListViewHandle);

            PhpAddDisabledPlugins(context);
            ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDOK));

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->code == LVN_ITEMCHANGED)
            {
                LPNM_LISTVIEW listView = (LPNM_LISTVIEW)lParam;

                if (!PhTryAcquireReleaseQueuedLockExclusive(&context->ListLock))
                    break;

                if (listView->uChanged & LVIF_STATE)
                {
                    switch (listView->uNewState & LVIS_STATEIMAGEMASK)
                    {
                    case INDEXTOSTATEIMAGEMASK(2): // checked
                        {
                            PPH_STRING param = (PPH_STRING)listView->lParam;

                            PhSetPluginDisabled(&param->sr, TRUE);
                        }
                        break;
                    case INDEXTOSTATEIMAGEMASK(1): // unchecked
                        {
                            PPH_STRING param = (PPH_STRING)listView->lParam;

                            PhSetPluginDisabled(&param->sr, FALSE);
                        }
                        break;
                    }
                }
            }
        }
        break;
    }

    return FALSE;
}
