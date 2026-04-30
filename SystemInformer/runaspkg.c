/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2018-2023
 *
 */

#include <phapp.h>
#include <apiimport.h>
#include <appresolver.h>
#include <actions.h>
#include <lsasup.h>
#include <phconsole.h>
#include <phsvc.h>
#include <phsvccl.h>
#include <phsettings.h>
#include <settings.h>
#include <svcsup.h>
#include <mainwnd.h>

typedef struct _PH_RUNAS_PACKAGE_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND ComboBoxHandle;
    HWND SearchBoxHandle;
    HWND TreeNewHandle;
    HIMAGELIST ImageListHandle;
    LONG WindowDpi;
    PH_LAYOUT_MANAGER LayoutManager;

    PH_TN_FILTER_SUPPORT TreeFilterSupport;
    PPH_TN_FILTER_ENTRY TreeFilterEntry;
    ULONG_PTR SearchMatchHandle;

    HFONT NormalFontHandle;
    HFONT TitleFontHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;

} PH_RUNAS_PACKAGE_CONTEXT, *PPH_RUNAS_PACKAGE_CONTEXT;

typedef enum _PH_RUNASPACKAGE_TREE_COLUMN_ITEM
{
    PH_RUNASPACKAGE_TREE_COLUMN_ITEM_NAME,
    PH_RUNASPACKAGE_TREE_COLUMN_ITEM_APPID,
    PH_RUNASPACKAGE_TREE_COLUMN_ITEM_MAXIMUM
} PH_RUNASPACKAGE_TREE_COLUMN_ITEM;

typedef struct _PH_RUNASPACKAGE_TREE_ROOT_NODE
{
    PH_TREENEW_NODE Node;

    PPH_STRING AppUserModelId;
    PPH_STRING DisplayName;
    PPH_STRING PackageInstallPath;
    PPH_STRING PackageFullName;
    PPH_STRING SmallLogoPath;

    LONG IconIndex;

    PH_STRINGREF TextCache[PH_RUNASPACKAGE_TREE_COLUMN_ITEM_MAXIMUM];
} PH_RUNASPACKAGE_TREE_ROOT_NODE, *PPH_RUNASPACKAGE_TREE_ROOT_NODE;

#define SORT_FUNCTION(Column) PhRunAsPackageTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhRunAsPackageTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_RUNASPACKAGE_TREE_ROOT_NODE node1 = *(PPH_RUNASPACKAGE_TREE_ROOT_NODE*)_elem1; \
    PPH_RUNASPACKAGE_TREE_ROOT_NODE node2 = *(PPH_RUNASPACKAGE_TREE_ROOT_NODE*)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->Node.Index, (ULONG_PTR)node2->Node.Index); \
    \
    return PhModifySort(sortResult, ((PPH_RUNAS_PACKAGE_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringWithNull(node1->DisplayName, node2->DisplayName, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Version)
{
    sortResult = PhCompareStringWithNull(node1->AppUserModelId, node2->AppUserModelId, TRUE);
}
END_SORT_FUNCTION

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
BOOLEAN PhRunAsPackageNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_RUNASPACKAGE_TREE_ROOT_NODE node1 = *(PPH_RUNASPACKAGE_TREE_ROOT_NODE*)Entry1;
    PPH_RUNASPACKAGE_TREE_ROOT_NODE node2 = *(PPH_RUNASPACKAGE_TREE_ROOT_NODE*)Entry2;

    return PhEqualString(node1->AppUserModelId, node2->AppUserModelId, TRUE);
}

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
ULONG PhRunAsPackageNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashStringRef(&(*(PPH_RUNASPACKAGE_TREE_ROOT_NODE*)Entry)->AppUserModelId->sr, TRUE);
}

/**
 * Destroys a Run As Package tree node and releases its resources.
 *
 * \param Node The node to destroy.
 */
VOID PhRunAsPackageDestroyNode(
    _In_ PPH_RUNASPACKAGE_TREE_ROOT_NODE Node
    )
{
    PhClearReference(&Node->AppUserModelId);
    PhClearReference(&Node->DisplayName);
    PhClearReference(&Node->PackageInstallPath);
    PhClearReference(&Node->PackageFullName);
    PhClearReference(&Node->SmallLogoPath);

    PhFree(Node);
}

/**
 * Adds a new node to the Run As Package tree.
 *
 * \param Context The Run As Package context.
 * \param Package The package entry to add.
 * \return The created tree node.
 */
PPH_RUNASPACKAGE_TREE_ROOT_NODE PhRunAsPackageAddNode(
    _Inout_ PPH_RUNAS_PACKAGE_CONTEXT Context,
    _In_ PPH_APPUSERMODELID_ENUM_ENTRY Package
    )
{
    PPH_RUNASPACKAGE_TREE_ROOT_NODE node;

    node = PhAllocate(sizeof(PH_RUNASPACKAGE_TREE_ROOT_NODE));
    memset(node, 0, sizeof(PH_RUNASPACKAGE_TREE_ROOT_NODE));

    PhInitializeTreeNewNode(&node->Node);

    memset(node->TextCache, 0, sizeof(PH_STRINGREF) * PH_RUNASPACKAGE_TREE_COLUMN_ITEM_MAXIMUM);
    node->Node.TextCache = node->TextCache;
    node->Node.TextCacheSize = PH_RUNASPACKAGE_TREE_COLUMN_ITEM_MAXIMUM;

    PhSetReference(&node->AppUserModelId, Package->AppUserModelId);
    PhSetReference(&node->DisplayName, Package->PackageDisplayName);
    PhSetReference(&node->PackageInstallPath, Package->PackageInstallPath);
    PhSetReference(&node->PackageFullName, Package->PackageFullName);
    PhSetReference(&node->SmallLogoPath, Package->SmallLogoPath);

    if (Package->SmallLogoPath && PhDoesFileExistWin32(Package->SmallLogoPath->Buffer))
    {
        HICON iconHandle = NULL;
        HBITMAP bitmap;
        LONG width;
        LONG height;

        width = PhGetSystemMetrics(SM_CXICON, Context->WindowDpi);
        height = PhGetSystemMetrics(SM_CYICON, Context->WindowDpi);

        if (bitmap = PhLoadImageFromFile(Package->SmallLogoPath->Buffer, width, height))
        {
            iconHandle = PhGdiplusConvertBitmapToIcon(bitmap, width, height, RGB(0, 0, 0));
            DeleteBitmap(bitmap);
        }

        if (iconHandle)
        {
            node->IconIndex = PhImageListAddIcon(Context->ImageListHandle, iconHandle);
            DestroyIcon(iconHandle);
        }
    }

    PhAddEntryHashtable(Context->NodeHashtable, &node);
    PhAddItemList(Context->NodeList, node);

    if (Context->TreeFilterSupport.FilterList)
        node->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->TreeFilterSupport, &node->Node);

    TreeNew_NodesStructured(Context->TreeNewHandle);

    return node;
}

/**
 * Finds a node in the Run As Package tree by AppUserModelId.
 *
 * \param Context The Run As Package context.
 * \param AppUserModelId The AppUserModelId to search for.
 * \return The found node, or NULL if not found.
 */
PPH_RUNASPACKAGE_TREE_ROOT_NODE PhRunAsPackageFindNode(
    _In_ PPH_RUNAS_PACKAGE_CONTEXT Context,
    _In_ PPH_STRING AppUserModelId
    )
{
    PH_RUNASPACKAGE_TREE_ROOT_NODE lookupNode;
    PPH_RUNASPACKAGE_TREE_ROOT_NODE lookupNodePtr = &lookupNode;
    PPH_RUNASPACKAGE_TREE_ROOT_NODE *node;

    lookupNode.AppUserModelId = AppUserModelId;

    node = (PPH_RUNASPACKAGE_TREE_ROOT_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupNodePtr
        );

    if (node)
        return *node;
    else
        return NULL;
}

/**
 * Removes a node from the Run As Package tree and destroys it.
 *
 * \param Context The Run As Package context.
 * \param Node The node to remove.
 */
VOID PhRunAsPackageRemoveNode(
    _In_ PPH_RUNAS_PACKAGE_CONTEXT Context,
    _In_ PPH_RUNASPACKAGE_TREE_ROOT_NODE Node
    )
{
    ULONG index = 0;

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
    {
        PhRemoveItemList(Context->NodeList, index);
    }

    PhRunAsPackageDestroyNode(Node);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

/**
 * Updates a node in the Run As Package tree, invalidating its cache and refreshing the view.
 *
 * \param Context The Run As Package context.
 * \param Node The node to update.
 */
VOID PhRunAsPackageUpdateNode(
    _In_ PPH_RUNAS_PACKAGE_CONTEXT Context,
    _In_ PPH_RUNASPACKAGE_TREE_ROOT_NODE Node
    )
{
    memset(Node->TextCache, 0, sizeof(PH_STRINGREF) * PH_RUNASPACKAGE_TREE_COLUMN_ITEM_MAXIMUM);

    PhInvalidateTreeNewNode(&Node->Node, TN_CACHE_COLOR);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

BOOLEAN NTAPI PhRunAsPackageTreeNewCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPH_RUNAS_PACKAGE_CONTEXT context = Context;
    PPH_RUNASPACKAGE_TREE_ROOT_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPH_RUNASPACKAGE_TREE_ROOT_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static CONST _CoreCrtSecureSearchSortCompareFunction sortFunctions[] =
                {
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(Version)
                };
                _CoreCrtSecureSearchSortCompareFunction sortFunction;

                static_assert(RTL_NUMBER_OF(sortFunctions) == PH_RUNASPACKAGE_TREE_COLUMN_ITEM_MAXIMUM, "SortFunctions must equal maximum.");

                if (context->TreeNewSortColumn < PH_RUNASPACKAGE_TREE_COLUMN_ITEM_MAXIMUM)
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
            node = (PPH_RUNASPACKAGE_TREE_ROOT_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PPH_RUNASPACKAGE_TREE_ROOT_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PH_RUNASPACKAGE_TREE_COLUMN_ITEM_NAME:
                getCellText->Text = PhGetStringRef(node->DisplayName);
                break;
            case PH_RUNASPACKAGE_TREE_COLUMN_ITEM_APPID:
                getCellText->Text = PhGetStringRef(node->AppUserModelId);
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
            node = (PPH_RUNASPACKAGE_TREE_ROOT_NODE)getNodeColor->Node;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            PPH_TREENEW_SORT_CHANGED_EVENT sorting = Parameter1;

            context->TreeNewSortColumn = sorting->SortColumn;
            context->TreeNewSortOrder = sorting->SortOrder;

            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(WindowHandle);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(context->WindowHandle, WM_COMMAND, ID_OBJECT_COPY, 0);
                break;
            case VK_DELETE:
                SendMessage(context->WindowHandle, WM_COMMAND, ID_OBJECT_CLOSE, 0);
                break;
            }
        }
        return TRUE;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = Parameter1;
            RECT rect;

            rect = customDraw->CellRect;
            node = (PPH_RUNASPACKAGE_TREE_ROOT_NODE)customDraw->Node;

            switch (customDraw->Column->Id)
            {
            case PH_RUNASPACKAGE_TREE_COLUMN_ITEM_NAME:
                {
                    PH_STRINGREF text;
                    SIZE nameSize;
                    SIZE textSize;
                    LONG dpiValue;

                    dpiValue = PhGetWindowDpi(WindowHandle);

                    rect.left += PhScaleToDisplay(15, dpiValue);
                    rect.top += PhScaleToDisplay(5, dpiValue);
                    rect.right -= PhScaleToDisplay(5, dpiValue);
                    rect.bottom -= PhScaleToDisplay(8, dpiValue);

                    rect.left += 32;

                    // top
                    if (PhEnableThemeSupport)
                        SetTextColor(customDraw->Dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    else
                        SetTextColor(customDraw->Dc, RGB(0x0, 0x0, 0x0));

                    SelectFont(customDraw->Dc, context->TitleFontHandle);
                    text = PhIsNullOrEmptyString(node->DisplayName) ? PhGetStringRef(node->AppUserModelId) : PhGetStringRef(node->DisplayName);
                    GetTextExtentPoint32(customDraw->Dc, text.Buffer, (ULONG)text.Length / sizeof(WCHAR), &nameSize);
                    DrawText(customDraw->Dc, text.Buffer, (ULONG)text.Length / sizeof(WCHAR), &rect, DT_TOP | DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE);

                    // bottom
                    if (PhEnableThemeSupport)
                        SetTextColor(customDraw->Dc, RGB(0x90, 0x90, 0x90));
                    else
                        SetTextColor(customDraw->Dc, RGB(0x64, 0x64, 0x64));

                    SelectFont(customDraw->Dc, context->NormalFontHandle);
                    text = PhGetStringRef(node->AppUserModelId);
                    GetTextExtentPoint32(customDraw->Dc, text.Buffer, (ULONG)text.Length / sizeof(WCHAR), &textSize);
                    DrawText(
                        customDraw->Dc,
                        text.Buffer,
                        (ULONG)text.Length / sizeof(WCHAR),
                        &rect,
                        DT_BOTTOM | DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE
                        );

                    if (context->ImageListHandle)
                    {
                        LONG width;
                        LONG height;

                        width = PhGetSystemMetrics(SM_CXICON, context->WindowDpi);
                        height = PhGetSystemMetrics(SM_CYICON, context->WindowDpi);

                        PhImageListDrawEx(
                            context->ImageListHandle,
                            (ULONG)(ULONG_PTR)node->IconIndex,
                            customDraw->Dc,
                            customDraw->CellRect.left + 5,//((customDraw->CellRect.right - customDraw->CellRect.left) - width) / 2,
                            customDraw->CellRect.top + ((customDraw->CellRect.bottom - customDraw->CellRect.top) - height) / 2,
                            width,
                            height,
                            CLR_DEFAULT,
                            CLR_NONE,
                            ILD_TRANSPARENT,
                            ILS_NORMAL
                            );
                    }

                }
                break;
            }
        }
        return TRUE;
    }

    return FALSE;
}

/**
 * Clears all nodes from the Run As Package tree.
 *
 * \param Context The Run As Package context.
 */
VOID PhRunAsPackageClearTree(
    _In_ PPH_RUNAS_PACKAGE_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PhRunAsPackageDestroyNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

/**
 * Gets the currently selected node in the Run As Package tree.
 *
 * \param Context The Run As Package context.
 * \return The selected node, or NULL if none is selected.
 */
PPH_RUNASPACKAGE_TREE_ROOT_NODE PhRunAsPackageGetSelectedNode(
    _In_ PPH_RUNAS_PACKAGE_CONTEXT Context
    )
{
    PPH_RUNASPACKAGE_TREE_ROOT_NODE node = NULL;

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        node = Context->NodeList->Items[i];

        if (node->Node.Selected)
            return node;
    }

    return NULL;
}

/**
 * Gets all selected nodes in the Run As Package tree.
 *
 * \param Context The Run As Package context.
 * \param Nodes Receives an array of selected nodes.
 * \param NumberOfNodes Receives the number of selected nodes.
 * \return TRUE if any nodes are selected, FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhRunAsPackageGetSelectedNodes(
    _In_ PPH_RUNAS_PACKAGE_CONTEXT Context,
    _Out_ PPH_RUNASPACKAGE_TREE_ROOT_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_RUNASPACKAGE_TREE_ROOT_NODE node = (PPH_RUNASPACKAGE_TREE_ROOT_NODE)Context->NodeList->Items[i];

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

_Function_class_(PH_TN_FILTER_FUNCTION)
BOOLEAN PhRunAsPackageTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_ PVOID Context
    )
{
    PPH_RUNASPACKAGE_TREE_ROOT_NODE node = (PPH_RUNASPACKAGE_TREE_ROOT_NODE)Node;
    PPH_RUNAS_PACKAGE_CONTEXT context = Context;

    if (!context->SearchMatchHandle)
        return TRUE;

    if (node->AppUserModelId && PhSearchControlMatch(context->SearchMatchHandle, &node->AppUserModelId->sr))
        return TRUE;
    if (node->DisplayName && PhSearchControlMatch(context->SearchMatchHandle, &node->DisplayName->sr))
        return TRUE;
    if (node->PackageInstallPath && PhSearchControlMatch(context->SearchMatchHandle, &node->PackageInstallPath->sr))
        return TRUE;
    if (node->PackageFullName && PhSearchControlMatch(context->SearchMatchHandle, &node->PackageFullName->sr))
        return TRUE;

    return FALSE;
}

/**
 * Initializes the Run As Package tree and its resources.
 *
 * \param Context The Run As Package context.
 */
VOID PhRunAsPackageInitializeTree(
    _Inout_ PPH_RUNAS_PACKAGE_CONTEXT Context
    )
{
    static CONST PH_STRINGREF PhRunAsPackageLoadingText = PH_STRINGREF_INIT(L"Loading package information...");

    Context->NodeList = PhCreateList(20);
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPH_RUNASPACKAGE_TREE_ROOT_NODE),
        PhRunAsPackageNodeHashtableEqualFunction,
        PhRunAsPackageNodeHashtableHashFunction,
        20
        );

    Context->NormalFontHandle = PhCreateCommonFont(-10, FW_NORMAL, NULL, Context->WindowDpi);
    Context->TitleFontHandle = PhCreateCommonFont(-14, FW_BOLD, NULL, Context->WindowDpi);

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");
    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);
    TreeNew_SetCallback(Context->TreeNewHandle, PhRunAsPackageTreeNewCallback, Context);
    TreeNew_SetRowHeight(Context->TreeNewHandle, PhScaleToDisplay(48, Context->WindowDpi));

    PhAddTreeNewColumnEx2(Context->TreeNewHandle, PH_RUNASPACKAGE_TREE_COLUMN_ITEM_NAME, TRUE, L"Package", 80, PH_ALIGN_LEFT, 0, 0, TN_COLUMN_FLAG_CUSTOMDRAW);
    //PhAddTreeNewColumnEx2(Context->TreeNewHandle, PH_PLUGIN_TREE_COLUMN_ITEM_VERSION, TRUE, L"Version", 80, PH_ALIGN_CENTER, 1, DT_CENTER, 0);

    //PhRunAsPackageLoadSettingsTreeList(Context);

    PhInitializeTreeNewFilterSupport(&Context->TreeFilterSupport, Context->TreeNewHandle, Context->NodeList);
    Context->TreeFilterEntry = PhAddTreeNewFilter(&Context->TreeFilterSupport, PhRunAsPackageTreeFilterCallback, Context);

    TreeNew_SetEmptyText(Context->TreeNewHandle, &PhRunAsPackageLoadingText, 0);
    TreeNew_SetTriState(Context->TreeNewHandle, TRUE);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
}

/**
 * Deletes the Run As Package tree and releases its resources.
 *
 * \param Context The Run As Package context.
 */
VOID PhRunAsPackageDeleteTree(
    _In_ PPH_RUNAS_PACKAGE_CONTEXT Context
    )
{
    PhRemoveTreeNewFilter(&Context->TreeFilterSupport, Context->TreeFilterEntry);
    PhDeleteTreeNewFilterSupport(&Context->TreeFilterSupport);

    if (Context->TitleFontHandle)
        DeleteFont(Context->TitleFontHandle);
    if (Context->NormalFontHandle)
        DeleteFont(Context->NormalFontHandle);

    //PhRunAsPackageSaveSettingsTreeList(Context);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PhRunAsPackageDestroyNode(Context->NodeList->Items[i]);

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

_Function_class_(USER_THREAD_START_ROUTINE)
/**
 * Thread callback to enumerate package application user model IDs and post results to the dialog.
 *
 * \param Context Pointer to the run-as package context.
 * \return NTSTATUS status code.
 */
static NTSTATUS PhEnumPackageThreadCallback(
    _In_ PVOID Context
    )
{
    PPH_RUNAS_PACKAGE_CONTEXT context = Context;

    PPH_LIST apps = PhEnumPackageApplicationUserModelIds();
    PostMessage(context->WindowHandle, WM_PH_UPDATE_DIALOG, 0, (LPARAM)apps);

    PhDereferenceObject(context);
    return STATUS_SUCCESS;
}

/**
 * Sets the image list for the run-as package dialog.
 *
 * \param Context The run-as package context.
 */
static VOID PhRunAsPackageSetImagelist(
    _Inout_ PPH_RUNAS_PACKAGE_CONTEXT Context
    )
{
    if (Context->ImageListHandle)
    {
        PhImageListDestroy(Context->ImageListHandle);
        Context->ImageListHandle = NULL;
    }

    Context->ImageListHandle = PhImageListCreate(
        PhGetSystemMetrics(SM_CXICON, Context->WindowDpi),
        PhGetSystemMetrics(SM_CYICON, Context->WindowDpi),
        ILC_MASK | ILC_COLOR32,
        20,
        10
        );
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PhPackageWindowContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    //PPH_RUNAS_PACKAGE_CONTEXT context = (PPH_RUNAS_PACKAGE_CONTEXT)Object;
    NOTHING;
}

PPH_RUNAS_PACKAGE_CONTEXT PhCreatePackageWindowContext(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PPH_OBJECT_TYPE PhPackageWindowContextObjectType;
    PPH_RUNAS_PACKAGE_CONTEXT context;

    if (PhBeginInitOnce(&initOnce))
    {
        PhPackageWindowContextObjectType = PhCreateObjectType(L"RunAsPackageWindowContext", 0, PhPackageWindowContextDeleteProcedure);
        PhEndInitOnce(&initOnce);
    }

    context = PhCreateObject(sizeof(PH_RUNAS_PACKAGE_CONTEXT), PhPackageWindowContextObjectType);
    memset(context, 0, sizeof(PH_RUNAS_PACKAGE_CONTEXT));

    return context;
}

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI PhpRunAsPackageSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPH_RUNAS_PACKAGE_CONTEXT context = Context;

    assert(context);

    context->SearchMatchHandle = MatchHandle;

    PhApplyTreeNewFilters(&context->TreeFilterSupport);
}

/**
 * Callback to add recent programs to a combo box.
 *
 * \param Command The command string.
 * \param Context The combo box handle.
 * \return TRUE to continue enumeration.
 */
_Function_class_(PH_ENUM_MRULIST_CALLBACK)
static BOOLEAN PhpEnumerateRecentProgramsToComboBox(
    _In_ PPH_STRINGREF Command,
    _In_ PVOID Context
    )
{
    ComboBox_AddString(Context, PhGetStringRefZ(Command));
    return TRUE;
}

/**
 * Adds recent programs to the specified combo box.
 *
 * \param ComboBoxHandle The combo box handle.
 */
static VOID PhpAddProgramsToComboBox(
    _In_ HWND ComboBoxHandle
    )
{
    PhDeleteComboBoxStrings(ComboBoxHandle, TRUE);

    PhEnumerateRecentList(PhpEnumerateRecentProgramsToComboBox, ComboBoxHandle);
}

/**
 * Dialog procedure for the Run As Package dialog.
 *
 * \param WindowHandle Handle to the dialog window.
 * \param WindowMessage The message.
 * \param wParam Message parameter.
 * \param lParam Message parameter.
 * \return INT_PTR result.
 */
INT_PTR CALLBACK PhRunAsPackageWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_RUNAS_PACKAGE_CONTEXT context = NULL;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhCreatePackageWindowContext();

        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = WindowHandle;
            context->ParentWindowHandle = (HWND)lParam;
            context->ComboBoxHandle = GetDlgItem(WindowHandle, IDC_PROGRAMCOMBO);
            context->SearchBoxHandle = GetDlgItem(WindowHandle, IDC_SEARCH);
            context->TreeNewHandle = GetDlgItem(WindowHandle, IDC_LIST);
            context->WindowDpi = PhGetWindowDpi(WindowHandle);

            PhSetApplicationWindowIconEx(WindowHandle, context->WindowDpi);

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, context->ComboBoxHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_BROWSE), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->SearchBoxHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_REFRESH), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDCANCEL), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (PhValidWindowPlacementFromSetting(SETTING_RUN_AS_PACKAGE_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_RUN_AS_PACKAGE_WINDOW_POSITION, SETTING_RUN_AS_PACKAGE_WINDOW_SIZE, WindowHandle);
            else
                PhCenterWindow(WindowHandle, GetParent(WindowHandle));

            TreeNew_AutoSizeColumn(context->TreeNewHandle, PH_RUNASPACKAGE_TREE_COLUMN_ITEM_NAME, TN_AUTOSIZE_REMAINING_SPACE);

            PhpAddProgramsToComboBox(context->ComboBoxHandle);
            ComboBox_SetCurSel(context->ComboBoxHandle, 0);

            PhRunAsPackageSetImagelist(context);
            PhRunAsPackageInitializeTree(context);

            PhCreateSearchControl(
                WindowHandle,
                context->SearchBoxHandle,
                L"Search Packages",
                PhpRunAsPackageSearchControlCallback,
                context
                );

            PhInitializeWindowTheme(WindowHandle, PhEnableThemeSupport);

            PhSetDialogFocus(WindowHandle, GetDlgItem(WindowHandle, IDCANCEL));

            EnableWindow(GetDlgItem(WindowHandle, IDC_REFRESH), FALSE);
            PhReferenceObject(context);
            PhCreateThread2(PhEnumPackageThreadCallback, context);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);

            PhSaveWindowPlacementToSetting(SETTING_RUN_AS_PACKAGE_WINDOW_POSITION, SETTING_RUN_AS_PACKAGE_WINDOW_SIZE, WindowHandle);

            PhRunAsPackageDeleteTree(context);

            PhImageListDestroy(context->ImageListHandle);

            PhDestroyWindowIcon(WindowHandle);

            PhDereferenceObject(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);

            TreeNew_AutoSizeColumn(context->TreeNewHandle, PH_RUNASPACKAGE_TREE_COLUMN_ITEM_NAME, TN_AUTOSIZE_REMAINING_SPACE);
        }
        break;
    case WM_DPICHANGED:
        {
            HFONT normalFontHandle;
            HFONT titleFontHandle;

            context->WindowDpi = LOWORD(wParam);

            if (normalFontHandle = PhCreateCommonFont(-10, FW_NORMAL, NULL, context->WindowDpi))
                PhReplaceWindowFont(&context->NormalFontHandle, NULL, normalFontHandle, FALSE);
            if (titleFontHandle = PhCreateCommonFont(-14, FW_BOLD, NULL, context->WindowDpi))
                PhReplaceWindowFont(&context->TitleFontHandle, NULL, titleFontHandle, FALSE);

            TreeNew_SetRowHeight(context->TreeNewHandle, PhScaleToDisplay(48, context->WindowDpi));
            TreeNew_NodesStructured(context->TreeNewHandle);

            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_PH_UPDATE_DIALOG:
        {
            PPH_LIST apps = (PPH_LIST)lParam;

            EnableWindow(GetDlgItem(WindowHandle, IDC_REFRESH), TRUE);

            if (apps)
            {
                TreeNew_SetRedraw(context->TreeNewHandle, FALSE);

                for (ULONG i = 0; i < apps->Count; i++)
                {
                    PhRunAsPackageAddNode(context, apps->Items[i]);
                }

                TreeNew_SetRedraw(context->TreeNewHandle, TRUE);
                TreeNew_AutoSizeColumn(context->TreeNewHandle, PH_RUNASPACKAGE_TREE_COLUMN_ITEM_NAME, TN_AUTOSIZE_REMAINING_SPACE);

                PhDestroyEnumPackageApplicationUserModelIds(apps);
            }
            else
            {
                // Error
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(WindowHandle, IDCANCEL);
                break;
            case IDC_BROWSE:
                {
                    PH_FILETYPE_FILTER filters[] =
                    {
                        { L"Executable files (*.exe;*.pif;*.com;*.bat;*.cmd)", L"*.exe;*.pif;*.com;*.bat;*.cmd" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog = PhCreateOpenFileDialog();

                    PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));

                    if (PhShowFileDialog(WindowHandle, fileDialog))
                    {
                        PPH_STRING fileName;

                        if (fileName = PhGetFileDialogFileName(fileDialog))
                        {
                            ComboBox_SetText(context->ComboBoxHandle, PhGetString(fileName));
                            PhDereferenceObject(fileName);
                        }
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            case IDOK:
                {
                    PPH_RUNASPACKAGE_TREE_ROOT_NODE node;
                    HRESULT status;
                    PPH_STRING windowText;

                    if (node = PhRunAsPackageGetSelectedNode(context))
                    {
                        if (windowText = PhGetWindowText(context->ComboBoxHandle))
                        {
                            PPH_STRING argumentsString = NULL;
                            PPH_STRING commandString = NULL;
                            PPH_STRING directoryString = NULL;
                            PPH_STRING fullFileName = NULL;
                            PH_STRINGREF fileName;
                            PH_STRINGREF arguments;

                            if (PhIsNullOrEmptyString(windowText))
                                break;

                            if (!(commandString = PhExpandEnvironmentStrings(&windowText->sr)))
                                commandString = PhCreateString2(&windowText->sr);

                            PhParseCommandLineFuzzy(&commandString->sr, &fileName, &arguments, &fullFileName);

                            if (PhIsNullOrEmptyString(fullFileName))
                                PhMoveReference(&fullFileName, PhCreateString2(&fileName));

                            if (arguments.Length)
                            {
                                argumentsString = PhCreateString2(&arguments);
                            }

                            directoryString = PhGetHomeDrivePath(!!PhGetOwnTokenAttributes().Elevated);

                            status = PhCreateProcessDesktopPackage(
                                PhGetString(node->AppUserModelId),
                                PhGetString(fullFileName),
                                PhGetString(argumentsString),
                                PhGetString(directoryString),
                                FALSE,
                                NULL,
                                NULL
                                );

                            if (HR_SUCCESS(status))
                            {
                                PhRecentListAddCommand(&commandString->sr);

                                EndDialog(WindowHandle, IDOK);
                            }
                            else
                            {
                                PhShowStatus(WindowHandle, L"Unable to execute the command.", 0, status);
                            }

                            PhClearReference(&directoryString);
                            PhClearReference(&argumentsString);
                            PhClearReference(&fullFileName);
                            PhClearReference(&commandString);
                        }
                    }
                }
                break;
            case IDC_REFRESH:
                {
                    EnableWindow(GetDlgItem(WindowHandle, IDC_REFRESH), FALSE);

                    PhRunAsPackageClearTree(context);
                    PhRunAsPackageSetImagelist(context);

                    PhReferenceObject(context);
                    PhCreateThread2(PhEnumPackageThreadCallback, context);
                }
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}
