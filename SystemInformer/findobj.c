/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <phsettings.h>
#include <cpysave.h>
#include <emenu.h>
#include <hndlinfo.h>
#include <kphuser.h>
#include <secedit.h>
#include <workqueue.h>

#include <colmgr.h>
#include <hndlmenu.h>
#include <hndlprv.h>
#include <mainwnd.h>
#include <procprv.h>
#include <proctree.h>
#include <settings.h>

#include "..\tools\thirdparty\pcre\pcre2.h"

#define WM_PH_SEARCH_SHOWDIALOG (WM_APP + 801)
#define WM_PH_SEARCH_FINISHED (WM_APP + 802)
#define WM_PH_SEARCH_SHOWMENU (WM_APP + 803)

static PPH_OBJECT_TYPE PhFindObjectsItemType = NULL;
static HANDLE PhFindObjectsThreadHandle = NULL;
static HWND PhFindObjectsWindowHandle = NULL;
static PH_EVENT PhFindObjectsInitializedEvent = PH_EVENT_INIT;

typedef struct _PH_HANDLE_SEARCH_CONTEXT
{
    PH_LAYOUT_MANAGER LayoutManager;
    RECT MinimumSize;

    HWND WindowHandle;
    HWND TreeNewHandle;
    HWND TypeWindowHandle;
    HWND SearchWindowHandle;
    PPH_STRING WindowText;
    HFONT TypeWindowFont;

    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;

    HANDLE SearchThreadHandle;

    BOOLEAN SearchStop;
    PPH_STRING SearchString;
    PPH_STRING SearchTypeString;
    pcre2_code *SearchRegexCompiledExpression;
    pcre2_match_data *SearchRegexMatchData;
    PPH_LIST SearchResults;
    ULONG SearchResultsAddIndex;
    PH_QUEUED_LOCK SearchResultsLock;
    ULONG64 SearchPointer;
    BOOLEAN UseSearchPointer;
} PH_HANDLE_SEARCH_CONTEXT, *PPH_HANDLE_SEARCH_CONTEXT;

typedef enum _PHP_OBJECT_RESULT_TYPE
{
    HandleSearchResult,
    ModuleSearchResult,
    MappedFileSearchResult
} PHP_OBJECT_RESULT_TYPE;

typedef struct _PHP_OBJECT_SEARCH_RESULT
{
    HANDLE ProcessId;
    PHP_OBJECT_RESULT_TYPE ResultType;

    HANDLE Handle;
    PVOID Object;
    PPH_STRING TypeName;
    PPH_STRING ObjectName;
    PPH_STRING BestObjectName;

    SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Info;
} PHP_OBJECT_SEARCH_RESULT, *PPHP_OBJECT_SEARCH_RESULT;

typedef enum _PH_HANDLE_OBJECT_TREE_COLUMN_ITEM_NAME
{
    PH_OBJECT_SEARCH_TREE_COLUMN_PROCESS,
    PH_OBJECT_SEARCH_TREE_COLUMN_TYPE,
    PH_OBJECT_SEARCH_TREE_COLUMN_NAME,
    PH_OBJECT_SEARCH_TREE_COLUMN_HANDLE,
    PH_OBJECT_SEARCH_TREE_COLUMN_OBJECTADDRESS,
    PH_OBJECT_SEARCH_TREE_COLUMN_ORIGINALNAME,
    PH_OBJECT_SEARCH_TREE_COLUMN_GRANTEDACCESS,
    PH_OBJECT_SEARCH_TREE_COLUMN_MAXIMUM
} PH_HANDLE_OBJECT_TREE_COLUMN_ITEM_NAME;

typedef struct _PH_HANDLE_OBJECT_TREE_ROOT_NODE
{
    PH_TREENEW_NODE Node;
    ULONG64 UniqueId; // used to stabilize sorting
    PHP_OBJECT_RESULT_TYPE ResultType;
    PVOID HandleObject;
    HANDLE Handle;
    HANDLE ProcessId;
    PPH_STRING ClientIdName;
    PPH_STRING TypeNameString;
    PPH_STRING ObjectNameString;
    PPH_STRING BestObjectName;
    PPH_STRING GrantedAccessSymbolicText;
    WCHAR HandleString[PH_PTR_STR_LEN_1];
    WCHAR ObjectString[PH_PTR_STR_LEN_1];

    SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HandleInfo;

    PH_STRINGREF TextCache[PH_OBJECT_SEARCH_TREE_COLUMN_MAXIMUM];
} PH_HANDLE_OBJECT_TREE_ROOT_NODE, *PPH_HANDLE_OBJECT_TREE_ROOT_NODE;

#define SORT_FUNCTION(Column) PhpHandleObjectTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpHandleObjectTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_HANDLE_OBJECT_TREE_ROOT_NODE node1 = *(PPH_HANDLE_OBJECT_TREE_ROOT_NODE*)_elem1; \
    PPH_HANDLE_OBJECT_TREE_ROOT_NODE node2 = *(PPH_HANDLE_OBJECT_TREE_ROOT_NODE*)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->UniqueId, (ULONG_PTR)node2->UniqueId); \
    \
    return PhModifySort(sortResult, ((PPH_HANDLE_SEARCH_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Process)
{
    sortResult = PhCompareString(node1->ClientIdName, node2->ClientIdName, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Type)
{
    sortResult = PhCompareString(node1->TypeNameString, node2->TypeNameString, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareString(node1->BestObjectName, node2->BestObjectName, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Handle)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->Handle, (ULONG_PTR)node2->Handle);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ObjectAddress)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->HandleObject, (ULONG_PTR)node2->HandleObject);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(OriginalName)
{
    sortResult = PhCompareString(node1->ObjectNameString, node2->ObjectNameString, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(GrantedAccess)
{
    sortResult = PhCompareStringWithNull(node1->GrantedAccessSymbolicText, node2->GrantedAccessSymbolicText, TRUE);
}
END_SORT_FUNCTION

VOID PhpHandleObjectLoadSettingsTreeList(
    _Inout_ PPH_HANDLE_SEARCH_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhGetStringSetting(L"FindObjTreeListColumns");
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);
}

VOID PhpHandleObjectSaveSettingsTreeList(
    _Inout_ PPH_HANDLE_SEARCH_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(L"FindObjTreeListColumns", &settings->sr);
    PhDereferenceObject(settings);
}

BOOLEAN PhpHandleObjectNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_HANDLE_OBJECT_TREE_ROOT_NODE node1 = *(PPH_HANDLE_OBJECT_TREE_ROOT_NODE *)Entry1;
    PPH_HANDLE_OBJECT_TREE_ROOT_NODE node2 = *(PPH_HANDLE_OBJECT_TREE_ROOT_NODE *)Entry2;

    return node1->HandleObject == node2->HandleObject;
}

ULONG PhpHandleObjectNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashIntPtr((ULONG_PTR)(*(PPH_HANDLE_OBJECT_TREE_ROOT_NODE*)Entry)->Handle);
}

VOID PhpDestroyHandleObjectNode(
    _In_ PPH_HANDLE_OBJECT_TREE_ROOT_NODE Node
    )
{
    PhClearReference(&Node->ClientIdName);
    PhClearReference(&Node->TypeNameString);
    PhClearReference(&Node->ObjectNameString);
    PhClearReference(&Node->BestObjectName);
    PhClearReference(&Node->GrantedAccessSymbolicText);

    PhFree(Node);
}

PPH_HANDLE_OBJECT_TREE_ROOT_NODE PhpAddHandleObjectNode(
    _Inout_ PPH_HANDLE_SEARCH_CONTEXT Context,
    _In_ HANDLE Handle
    )
{
    static ULONG64 NextUniqueId = 0;
    PPH_HANDLE_OBJECT_TREE_ROOT_NODE handleObjectNode;

    handleObjectNode = PhAllocate(sizeof(PH_HANDLE_OBJECT_TREE_ROOT_NODE));
    memset(handleObjectNode, 0, sizeof(PH_HANDLE_OBJECT_TREE_ROOT_NODE));
    PhInitializeTreeNewNode(&handleObjectNode->Node);

    memset(handleObjectNode->TextCache, 0, sizeof(PH_STRINGREF) * PH_OBJECT_SEARCH_TREE_COLUMN_MAXIMUM);
    handleObjectNode->Node.TextCache = handleObjectNode->TextCache;
    handleObjectNode->Node.TextCacheSize = PH_OBJECT_SEARCH_TREE_COLUMN_MAXIMUM;

    handleObjectNode->Handle = Handle;
    handleObjectNode->UniqueId = ++NextUniqueId;

    PhAddEntryHashtable(Context->NodeHashtable, &handleObjectNode);
    PhAddItemList(Context->NodeList, handleObjectNode);

    //TreeNew_NodesStructured(Context->TreeNewHandle);

    return handleObjectNode;
}

PPH_HANDLE_OBJECT_TREE_ROOT_NODE PhpFindHandleObjectNode(
    _In_ PPH_HANDLE_SEARCH_CONTEXT Context,
    _In_ HANDLE Handle
    )
{
    PH_HANDLE_OBJECT_TREE_ROOT_NODE lookupHandleObjectNode;
    PPH_HANDLE_OBJECT_TREE_ROOT_NODE lookupHandleObjectNodePtr = &lookupHandleObjectNode;
    PPH_HANDLE_OBJECT_TREE_ROOT_NODE *handleObjectNode;

    lookupHandleObjectNode.Handle = Handle;

    handleObjectNode = (PPH_HANDLE_OBJECT_TREE_ROOT_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupHandleObjectNodePtr
        );

    if (handleObjectNode)
        return *handleObjectNode;
    else
        return NULL;
}

VOID PhpRemoveHandleObjectNode(
    _In_ PPH_HANDLE_SEARCH_CONTEXT Context,
    _In_ PPH_HANDLE_OBJECT_TREE_ROOT_NODE Node
    )
{
    ULONG index = 0;

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
    {
        PhRemoveItemList(Context->NodeList, index);
    }

    PhpDestroyHandleObjectNode(Node);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhpUpdateHandleObjectNode(
    _In_ PPH_HANDLE_SEARCH_CONTEXT Context,
    _In_ PPH_HANDLE_OBJECT_TREE_ROOT_NODE Node
    )
{
    memset(Node->TextCache, 0, sizeof(PH_STRINGREF) * PH_OBJECT_SEARCH_TREE_COLUMN_MAXIMUM);

    PhInvalidateTreeNewNode(&Node->Node, TN_CACHE_COLOR);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

BOOLEAN NTAPI PhpHandleObjectTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPH_HANDLE_SEARCH_CONTEXT context = Context;
    PPH_HANDLE_OBJECT_TREE_ROOT_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPH_HANDLE_OBJECT_TREE_ROOT_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Process),
                    SORT_FUNCTION(Type),
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(Handle),
                    SORT_FUNCTION(ObjectAddress),
                    SORT_FUNCTION(OriginalName),
                    SORT_FUNCTION(GrantedAccess),
                };
                int (__cdecl *sortFunction)(void *, const void *, const void *);

                if (context->TreeNewSortColumn < PH_OBJECT_SEARCH_TREE_COLUMN_MAXIMUM)
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
            node = (PPH_HANDLE_OBJECT_TREE_ROOT_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PPH_HANDLE_OBJECT_TREE_ROOT_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PH_OBJECT_SEARCH_TREE_COLUMN_PROCESS:
                getCellText->Text = PhGetStringRef(node->ClientIdName);
                break;
            case PH_OBJECT_SEARCH_TREE_COLUMN_TYPE:
                getCellText->Text = PhGetStringRef(node->TypeNameString);
                break;
            case PH_OBJECT_SEARCH_TREE_COLUMN_NAME:
                getCellText->Text = PhGetStringRef(node->BestObjectName);
                break;
            case PH_OBJECT_SEARCH_TREE_COLUMN_HANDLE:
                PhInitializeStringRefLongHint(&getCellText->Text, node->HandleString);
                break;
            case PH_OBJECT_SEARCH_TREE_COLUMN_OBJECTADDRESS:
                PhInitializeStringRefLongHint(&getCellText->Text, node->ObjectString);
                break;
            case PH_OBJECT_SEARCH_TREE_COLUMN_ORIGINALNAME:
                getCellText->Text = PhGetStringRef(node->ObjectNameString);
                break;
            case PH_OBJECT_SEARCH_TREE_COLUMN_GRANTEDACCESS:
                getCellText->Text = PhGetStringRef(node->GrantedAccessSymbolicText);
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
            node = (PPH_HANDLE_OBJECT_TREE_ROOT_NODE)getNodeColor->Node;

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
            SendMessage(context->WindowHandle, WM_COMMAND, ID_OBJECT_PROPERTIES, 0);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            SendMessage(
                context->WindowHandle,
                WM_COMMAND,
                WM_PH_SEARCH_SHOWMENU,
                (LPARAM)contextMenuEvent
                );
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = AscendingSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    }

    return FALSE;
}

VOID PhpClearHandleObjectTree(
    _In_ PPH_HANDLE_SEARCH_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PhpDestroyHandleObjectNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

PPH_HANDLE_OBJECT_TREE_ROOT_NODE PhpGetSelectedHandleObjectNode(
    _In_ PPH_HANDLE_SEARCH_CONTEXT Context
    )
{
    PPH_HANDLE_OBJECT_TREE_ROOT_NODE handleNode = NULL;

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        handleNode = Context->NodeList->Items[i];

        if (handleNode->Node.Selected)
            return handleNode;
    }

    return NULL;
}

_Success_(return)
BOOLEAN PhpGetSelectedHandleObjectNodes(
    _In_ PPH_HANDLE_SEARCH_CONTEXT Context,
    _Out_ PPH_HANDLE_OBJECT_TREE_ROOT_NODE **HandleObjectNodes,
    _Out_ PULONG NumberOfHandleObjectNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_HANDLE_OBJECT_TREE_ROOT_NODE node = (PPH_HANDLE_OBJECT_TREE_ROOT_NODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node);
        }
    }

    if (list->Count)
    {
        *HandleObjectNodes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
        *NumberOfHandleObjectNodes = list->Count;

        PhDereferenceObject(list);
        return TRUE;
    }

    PhDereferenceObject(list);
    return FALSE;
}

VOID PhpInitializeHandleObjectTree(
    _Inout_ PPH_HANDLE_SEARCH_CONTEXT Context
    )
{
    Context->NodeList = PhCreateList(100);
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPH_HANDLE_OBJECT_TREE_ROOT_NODE),
        PhpHandleObjectNodeHashtableEqualFunction,
        PhpHandleObjectNodeHashtableHashFunction,
        100
        );

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");

    TreeNew_SetCallback(Context->TreeNewHandle, PhpHandleObjectTreeNewCallback, Context);

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    // Default columns
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_OBJECT_SEARCH_TREE_COLUMN_PROCESS, TRUE, L"Process", 100, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_OBJECT_SEARCH_TREE_COLUMN_TYPE, TRUE, L"Type", 100, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_OBJECT_SEARCH_TREE_COLUMN_NAME, TRUE, L"Name", 200, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_OBJECT_SEARCH_TREE_COLUMN_HANDLE, TRUE, L"Handle", 80, PH_ALIGN_LEFT, 3, 0);

    PhAddTreeNewColumn(Context->TreeNewHandle, PH_OBJECT_SEARCH_TREE_COLUMN_OBJECTADDRESS, FALSE, L"Object address", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_OBJECT_SEARCH_TREE_COLUMN_ORIGINALNAME, FALSE, L"Original name", 200, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_OBJECT_SEARCH_TREE_COLUMN_GRANTEDACCESS, FALSE, L"Granted access", 200, PH_ALIGN_LEFT, ULONG_MAX, 0);

    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);

    TreeNew_SetTriState(Context->TreeNewHandle, TRUE);

    PhpHandleObjectLoadSettingsTreeList(Context);
}

VOID PhpDeleteHandleObjectTree(
    _In_ PPH_HANDLE_SEARCH_CONTEXT Context
    )
{
    PhpHandleObjectSaveSettingsTreeList(Context);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PhpDestroyHandleObjectNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

VOID PhpInitializeFindObjMenu(
    _In_ PPH_EMENU Menu,
    _In_ PPH_HANDLE_OBJECT_TREE_ROOT_NODE *Results,
    _In_ ULONG NumberOfResults
    )
{
    BOOLEAN allCanBeClosed = TRUE;
    ULONG i;

    if (NumberOfResults == 1)
    {
        PH_HANDLE_ITEM_INFO info;

        info.ProcessId = Results[0]->ProcessId;
        info.Handle = Results[0]->Handle;
        info.TypeName = Results[0]->TypeNameString;
        info.BestObjectName = Results[0]->BestObjectName;
        PhInsertHandleObjectPropertiesEMenuItems(Menu, ID_OBJECT_PROPERTIES, FALSE, &info);
    }
    else
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        PhEnableEMenuItem(Menu, ID_OBJECT_COPY, TRUE);
    }

    for (i = 0; i < NumberOfResults; i++)
    {
        if (Results[i]->ResultType != HandleSearchResult)
        {
            allCanBeClosed = FALSE;
            break;
        }
    }

    PhEnableEMenuItem(Menu, ID_OBJECT_CLOSE, allCanBeClosed);
}

static int __cdecl PhpStringObjectTypeCompare(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PPH_STRING entry1 = *(PPH_STRING *)elem1;
    PPH_STRING entry2 = *(PPH_STRING *)elem2;

    return PhCompareString(entry1, entry2, TRUE);
}

VOID PhpPopulateObjectTypes(
    _In_ PPH_HANDLE_SEARCH_CONTEXT Context
    )
{
    POBJECT_TYPES_INFORMATION objectTypes;
    POBJECT_TYPE_INFORMATION objectType;
    PPH_LIST objectTypeList;

    objectTypeList = PhCreateList(100);

    // Add a custom object type for searching all objects.
    ComboBox_AddString(Context->TypeWindowHandle, L"Everything");
    ComboBox_SetCurSel(Context->TypeWindowHandle, 0);

    // Enumerate the available object types.
    if (NT_SUCCESS(PhEnumObjectTypes(&objectTypes)))
    {
        objectType = PH_FIRST_OBJECT_TYPE(objectTypes);

        for (ULONG i = 0; i < objectTypes->NumberOfTypes; i++)
        {
            PhAddItemList(objectTypeList, PhCreateStringFromUnicodeString(&objectType->TypeName));
            objectType = PH_NEXT_OBJECT_TYPE(objectType);
        }

        PhFree(objectTypes);
    }

    // Sort the object types.
    qsort(objectTypeList->Items, objectTypeList->Count, sizeof(PVOID), PhpStringObjectTypeCompare);

    // Set the font, add the dropdown entries and adjust the dropdown width.
    {
        LONG maxLength;
        HDC comboDc;

        maxLength = 0;
        comboDc = GetDC(Context->TypeWindowHandle);

        SetWindowFont(Context->TypeWindowHandle, Context->TypeWindowFont, TRUE);

        for (ULONG i = 0; i < objectTypeList->Count; i++)
        {
            PPH_STRING entry = objectTypeList->Items[i];
            SIZE textSize;

            if (GetTextExtentPoint32(comboDc, entry->Buffer, (ULONG)entry->Length / sizeof(WCHAR), &textSize))
            {
                if (textSize.cx > maxLength)
                    maxLength = textSize.cx;
            }

            ComboBox_AddString(Context->TypeWindowHandle, PhGetString(objectTypeList->Items[i]));
            PhDereferenceObject(objectTypeList->Items[i]);
        }

        ReleaseDC(Context->TypeWindowHandle, comboDc);

        if (maxLength)
        {
            SendMessage(Context->TypeWindowHandle, CB_SETDROPPEDWIDTH, maxLength, 0);
        }
    }

    PhDereferenceObject(objectTypeList);
}

VOID PhpFindObjectAddResultEntries(
    _In_ PPH_HANDLE_SEARCH_CONTEXT Context
    )
{
    ULONG i;

    PhAcquireQueuedLockExclusive(&Context->SearchResultsLock);

    if (Context->SearchResults->Count == 0 || Context->SearchResultsAddIndex == Context->SearchResults->Count)
    {
        PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);
        return;
    }

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    for (i = Context->SearchResultsAddIndex; i < Context->SearchResults->Count; i++)
    {
        PPHP_OBJECT_SEARCH_RESULT searchResult = Context->SearchResults->Items[i];
        CLIENT_ID clientId;
        PPH_PROCESS_ITEM processItem;
        PPH_HANDLE_OBJECT_TREE_ROOT_NODE objectNode;

        clientId.UniqueProcess = searchResult->ProcessId;
        clientId.UniqueThread = NULL;

        processItem = PhReferenceProcessItem(clientId.UniqueProcess);

        objectNode = PhpAddHandleObjectNode(Context, searchResult->Handle);
        objectNode->ProcessId = searchResult->ProcessId;
        objectNode->ResultType = searchResult->ResultType;
        objectNode->ClientIdName = PhGetClientIdNameEx(&clientId, processItem ? processItem->ProcessName : NULL);
        objectNode->TypeNameString = searchResult->TypeName;
        objectNode->ObjectNameString = searchResult->ObjectName;
        objectNode->BestObjectName = searchResult->BestObjectName;
        objectNode->HandleInfo = searchResult->Info;
        PhPrintPointer(objectNode->HandleString, searchResult->Handle);

        if (searchResult->Object)
        {
            objectNode->HandleObject = searchResult->Object;
            PhPrintPointer(objectNode->ObjectString, searchResult->Object);
        }

        if (searchResult->Info.GrantedAccess != 0)
        {
            PPH_ACCESS_ENTRY accessEntries;
            ULONG numberOfAccessEntries;

            if (PhGetAccessEntries(PhGetStringOrEmpty(searchResult->TypeName), &accessEntries, &numberOfAccessEntries))
            {
                objectNode->GrantedAccessSymbolicText = PhGetAccessString(searchResult->Info.GrantedAccess, accessEntries, numberOfAccessEntries);
                PhFree(accessEntries);
            }

            if (objectNode->GrantedAccessSymbolicText)
            {
                WCHAR grantedAccessString[PH_PTR_STR_LEN_1];

                PhPrintPointer(grantedAccessString, UlongToPtr(searchResult->Info.GrantedAccess));
                PhMoveReference(&objectNode->GrantedAccessSymbolicText, PhFormatString(
                    L"%s (%s)",
                    PhGetString(objectNode->GrantedAccessSymbolicText),
                    grantedAccessString
                    ));
            }
        }

        if (processItem)
        {
            PhDereferenceObject(processItem);
        }
    }

    TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);

    Context->SearchResultsAddIndex = i;

    PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);
}

VOID PhpFindObjectClearResultEntries(
    _In_ PPH_HANDLE_SEARCH_CONTEXT Context
    )
{
    PhpClearHandleObjectTree(Context);

    Context->SearchResultsAddIndex = 0;

    for (ULONG i = 0; i < Context->SearchResults->Count; i++)
        PhFree(Context->SearchResults->Items[i]);

    PhClearList(Context->SearchResults);
}

static BOOLEAN MatchSearchString(
    _In_ PPH_HANDLE_SEARCH_CONTEXT Context,
    _In_ PPH_STRINGREF Input
    )
{
    if (Context->SearchRegexCompiledExpression && Context->SearchRegexMatchData)
    {
        return pcre2_match(
            Context->SearchRegexCompiledExpression,
            Input->Buffer,
            Input->Length / sizeof(WCHAR),
            0,
            0,
            Context->SearchRegexMatchData,
            NULL
            ) >= 0;
    }
    else
    {
        return PhFindStringInStringRef(Input, &Context->SearchString->sr, TRUE) != SIZE_MAX;
    }
}

static BOOLEAN MatchTypeString(
    _In_ PPH_HANDLE_SEARCH_CONTEXT Context,
    _In_ PPH_STRINGREF Input
    )
{
    if (PhEqualString2(Context->SearchTypeString, L"Everything", FALSE))
        return TRUE;

    return PhEqualStringRef(Input, &Context->SearchTypeString->sr, TRUE);
}

typedef struct _SEARCH_HANDLE_CONTEXT
{
    PPH_HANDLE_SEARCH_CONTEXT WindowContext;
    BOOLEAN NeedToFree;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HandleInfo;
    HANDLE ProcessHandle;
} SEARCH_HANDLE_CONTEXT, *PSEARCH_HANDLE_CONTEXT;

static NTSTATUS NTAPI SearchHandleFunction(
    _In_ PVOID Parameter
    )
{
    PSEARCH_HANDLE_CONTEXT handleContext = Parameter;
    PPH_HANDLE_SEARCH_CONTEXT context = handleContext->WindowContext;
    PPH_STRING typeName;
    PPH_STRING objectName;
    PPH_STRING bestObjectName;

    if (NT_SUCCESS(PhGetHandleInformation(
        handleContext->ProcessHandle,
        (HANDLE)handleContext->HandleInfo->HandleValue,
        handleContext->HandleInfo->ObjectTypeIndex,
        NULL,
        &typeName,
        &objectName,
        &bestObjectName
        )))
    {
        PPH_STRING upperObjectName;
        PPH_STRING upperBestObjectName;
        PPH_STRING upperTypeName;

        upperObjectName = PhUpperString(objectName);
        upperBestObjectName = PhUpperString(bestObjectName);
        upperTypeName = PhUpperString(typeName);

        if (((MatchSearchString(context, &upperObjectName->sr) || MatchSearchString(context, &upperBestObjectName->sr)) && MatchTypeString(context, &upperTypeName->sr)) ||
            (context->UseSearchPointer && (handleContext->HandleInfo->Object == (PVOID)context->SearchPointer || handleContext->HandleInfo->HandleValue == context->SearchPointer)))
        {
            PPHP_OBJECT_SEARCH_RESULT searchResult;

            searchResult = PhAllocateZero(sizeof(PHP_OBJECT_SEARCH_RESULT));
            searchResult->ProcessId = (HANDLE)handleContext->HandleInfo->UniqueProcessId;
            searchResult->ResultType = HandleSearchResult;
            searchResult->Object = handleContext->HandleInfo->Object;
            searchResult->Handle = (HANDLE)handleContext->HandleInfo->HandleValue;
            searchResult->TypeName = typeName;
            searchResult->ObjectName = objectName;
            searchResult->BestObjectName = bestObjectName;
            searchResult->Info = *handleContext->HandleInfo;

            PhAcquireQueuedLockExclusive(&context->SearchResultsLock);
            PhAddItemList(context->SearchResults, searchResult);
            PhReleaseQueuedLockExclusive(&context->SearchResultsLock);
        }
        else
        {
            PhDereferenceObject(typeName);
            PhDereferenceObject(objectName);
            PhDereferenceObject(bestObjectName);
        }

        PhDereferenceObject(upperTypeName);
        PhDereferenceObject(upperBestObjectName);
        PhDereferenceObject(upperObjectName);
    }

    if (handleContext->NeedToFree)
        PhFree(handleContext);

    return STATUS_SUCCESS;
}

typedef struct _SEARCH_MODULE_CONTEXT
{
    PPH_HANDLE_SEARCH_CONTEXT WindowContext;
    HANDLE ProcessId;
} SEARCH_MODULE_CONTEXT, *PSEARCH_MODULE_CONTEXT;

static BOOLEAN NTAPI EnumModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    )
{
    PSEARCH_MODULE_CONTEXT moduleContext = Context;
    PPH_HANDLE_SEARCH_CONTEXT context;
    PPH_STRING upperFileName;
    PPH_STRING upperOriginalFileName;

    if (!moduleContext)
        return TRUE;

    context = moduleContext->WindowContext;

    upperFileName = PhUpperString(Module->FileNameWin32);
    upperOriginalFileName = PhUpperString(Module->FileName);

    if ((MatchSearchString(context, &upperFileName->sr) || MatchSearchString(context, &upperOriginalFileName->sr)) ||
        (context->UseSearchPointer && Module->BaseAddress == (PVOID)context->SearchPointer))
    {
        PPHP_OBJECT_SEARCH_RESULT searchResult;
        PWSTR typeName;

        switch (Module->Type)
        {
        case PH_MODULE_TYPE_MAPPED_FILE:
            typeName = L"Mapped file";
            break;
        case PH_MODULE_TYPE_MAPPED_IMAGE:
            typeName = L"Mapped image";
            break;
        default:
            typeName = L"DLL";
            break;
        }

        searchResult = PhAllocateZero(sizeof(PHP_OBJECT_SEARCH_RESULT));
        searchResult->ProcessId = moduleContext->ProcessId;
        searchResult->ResultType = (Module->Type == PH_MODULE_TYPE_MAPPED_FILE || Module->Type == PH_MODULE_TYPE_MAPPED_IMAGE) ? MappedFileSearchResult : ModuleSearchResult;
        searchResult->Handle = (HANDLE)Module->BaseAddress;
        searchResult->TypeName = PhCreateString(typeName);
        PhSetReference(&searchResult->BestObjectName, Module->FileNameWin32);
        PhSetReference(&searchResult->ObjectName, Module->FileName);

        PhAcquireQueuedLockExclusive(&context->SearchResultsLock);
        PhAddItemList(context->SearchResults, searchResult);
        PhReleaseQueuedLockExclusive(&context->SearchResultsLock);
    }

    PhDereferenceObject(upperOriginalFileName);
    PhDereferenceObject(upperFileName);

    return TRUE;
}

NTSTATUS PhpFindObjectsThreadStart(
    _In_ PVOID Parameter
    )
{
    PPH_HANDLE_SEARCH_CONTEXT context = Parameter;
    NTSTATUS status = STATUS_SUCCESS;
    PSYSTEM_HANDLE_INFORMATION_EX handles;
    PPH_HASHTABLE processHandleHashtable;
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;
    ULONG i;

    // Refuse to search with no filter.
    if (context->SearchString->Length == 0)
        goto Exit;

    // Try to get a search pointer from the search string.
    context->UseSearchPointer = PhStringToInteger64(&context->SearchString->sr, 0, &context->SearchPointer);

    PhMoveReference(&context->SearchString, PhUpperString(context->SearchString));

    if (NT_SUCCESS(status = PhEnumHandlesEx(&handles)))
    {
        static PH_INITONCE initOnce = PH_INITONCE_INIT;
        static ULONG fileObjectTypeIndex = ULONG_MAX;

        BOOLEAN useWorkQueue = FALSE;
        PH_WORK_QUEUE workQueue;
        processHandleHashtable = PhCreateSimpleHashtable(8);

        if (KphLevel() < KphLevelMed)
        {
            useWorkQueue = TRUE;
            PhInitializeWorkQueue(&workQueue, 1, 20, 1000);

            if (PhBeginInitOnce(&initOnce))
            {
                fileObjectTypeIndex = PhGetObjectTypeNumberZ(L"File");
                PhEndInitOnce(&initOnce);
            }
        }

        for (i = 0; i < handles->NumberOfHandles; i++)
        {
            PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handleInfo = &handles->Handles[i];
            PVOID *processHandlePtr;
            HANDLE processHandle;

            // Don't continue if the user requested cancellation.
            if (context->SearchStop)
                break;

            // Open a handle to the process if we don't already have one.

            processHandlePtr = PhFindItemSimpleHashtable(
                processHandleHashtable,
                (PVOID)handleInfo->UniqueProcessId
                );

            if (processHandlePtr)
            {
                processHandle = (HANDLE)*processHandlePtr;
            }
            else
            {
                if (NT_SUCCESS(PhOpenProcess(
                    &processHandle,
                    PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION,
                    (HANDLE)handleInfo->UniqueProcessId
                    )))
                {
                    PhAddItemSimpleHashtable(
                        processHandleHashtable,
                        (PVOID)handleInfo->UniqueProcessId,
                        processHandle
                        );
                }
                else
                {
                    if (NT_SUCCESS(PhOpenProcess(
                        &processHandle,
                        PROCESS_QUERY_INFORMATION,
                        (HANDLE)handleInfo->UniqueProcessId
                        )))
                    {
                        PhAddItemSimpleHashtable(
                            processHandleHashtable,
                            (PVOID)handleInfo->UniqueProcessId,
                            processHandle
                            );
                    }
                    else
                    {
                        continue;
                    }
                }
            }

            if (useWorkQueue && handleInfo->ObjectTypeIndex == (USHORT)fileObjectTypeIndex)
            {
                PSEARCH_HANDLE_CONTEXT searchHandleContext;

                searchHandleContext = PhAllocate(sizeof(SEARCH_HANDLE_CONTEXT));
                searchHandleContext->WindowContext = context;
                searchHandleContext->NeedToFree = TRUE;
                searchHandleContext->HandleInfo = handleInfo;
                searchHandleContext->ProcessHandle = processHandle;

                PhQueueItemWorkQueue(&workQueue, SearchHandleFunction, searchHandleContext);
            }
            else
            {
                SEARCH_HANDLE_CONTEXT searchHandleContext;

                searchHandleContext.WindowContext = context;
                searchHandleContext.NeedToFree = FALSE;
                searchHandleContext.HandleInfo = handleInfo;
                searchHandleContext.ProcessHandle = processHandle;

                SearchHandleFunction(&searchHandleContext);
            }
        }

        if (useWorkQueue)
        {
            PhWaitForWorkQueue(&workQueue);
            PhDeleteWorkQueue(&workQueue);
        }

        {
            PPH_KEY_VALUE_PAIR entry;

            i = 0;

            while (PhEnumHashtable(processHandleHashtable, &entry, &i))
                NtClose((HANDLE)entry->Value);
        }

        PhDereferenceObject(processHandleHashtable);
        PhFree(handles);
    }

    if (context->SearchStop)
        goto Exit;

    if (PhEqualString2(context->SearchTypeString, L"File", TRUE) ||
        PhEqualString2(context->SearchTypeString, L"Everything", FALSE))
    {
        if (NT_SUCCESS(PhEnumProcesses(&processes)))
        {
            process = PH_FIRST_PROCESS(processes);

            do
            {
                SEARCH_MODULE_CONTEXT searchModuleContext;

                if (context->SearchStop)
                    break;

                searchModuleContext.WindowContext = context;
                searchModuleContext.ProcessId = process->UniqueProcessId;

                PhEnumGenericModules(
                    process->UniqueProcessId,
                    NULL,
                    PH_ENUM_GENERIC_MAPPED_FILES | PH_ENUM_GENERIC_MAPPED_IMAGES,
                    EnumModulesCallback,
                    &searchModuleContext
                    );
            } while (process = PH_NEXT_PROCESS(process));

            PhFree(processes);
        }
    }

Exit:
    PostMessage(context->WindowHandle, WM_PH_SEARCH_FINISHED, status, 0);

    PhDereferenceObject(context);
    return STATUS_SUCCESS;
}

VOID PhpFindObjectsDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_HANDLE_SEARCH_CONTEXT context = Object;

    PhClearReference(&context->SearchString);
    PhClearReference(&context->SearchTypeString);
}

PPH_HANDLE_SEARCH_CONTEXT PhCreateFindObjectContext(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PPH_HANDLE_SEARCH_CONTEXT context;

    if (PhBeginInitOnce(&initOnce))
    {
        PhFindObjectsItemType = PhCreateObjectType(L"FindObjectsItem", 0, PhpFindObjectsDeleteProcedure);
        PhEndInitOnce(&initOnce);
    }

    context = PhCreateObject(sizeof(PH_HANDLE_SEARCH_CONTEXT), PhFindObjectsItemType);
    memset(context, 0, sizeof(PH_HANDLE_SEARCH_CONTEXT));

    return context;
}

INT_PTR CALLBACK PhpFindObjectsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_HANDLE_SEARCH_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhCreateFindObjectContext();

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
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_TREELIST);
            context->TypeWindowHandle = GetDlgItem(hwndDlg, IDC_FILTERTYPE);
            context->SearchWindowHandle = GetDlgItem(hwndDlg, IDC_FILTER);
            context->TypeWindowFont = PhDuplicateFont(PhApplicationFont);
            context->WindowText = PhGetWindowText(hwndDlg);

            PhSetApplicationWindowIcon(hwndDlg);

            PhRegisterDialog(hwndDlg);
            PhCreateSearchControl(hwndDlg, context->SearchWindowHandle, L"Find Handles or DLLs");
            PhpPopulateObjectTypes(context);
            PhpInitializeHandleObjectTree(context);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->TypeWindowHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP);
            PhAddLayoutItem(&context->LayoutManager, context->SearchWindowHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REGEX), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 300;
            context->MinimumSize.bottom = 100;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            if (PhGetIntegerPairSetting(L"FindObjWindowPosition").X)
                PhLoadWindowPlacementFromSetting(L"FindObjWindowPosition", L"FindObjWindowSize", hwndDlg);
            else
                PhCenterWindow(hwndDlg, PhMainWndHandle);

            PhRegisterWindowCallback(hwndDlg, PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST, NULL);

            context->SearchResults = PhCreateList(128);
            context->SearchResultsAddIndex = 0;

            PhSetTimer(hwndDlg, 1, 1000, NULL);

            Edit_SetSel(context->SearchWindowHandle, 0, -1);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_REGEX), PhGetIntegerSetting(L"FindObjRegex") ? BST_CHECKED : BST_UNCHECKED);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            context->SearchStop = TRUE;

            PhKillTimer(hwndDlg, 1);

            if (context->SearchThreadHandle)
            {
                NtWaitForSingleObject(context->SearchThreadHandle, FALSE, NULL);
                NtClose(context->SearchThreadHandle);
                context->SearchThreadHandle = NULL;
            }

            if (context->SearchRegexCompiledExpression)
            {
                pcre2_code_free(context->SearchRegexCompiledExpression);
                context->SearchRegexCompiledExpression = NULL;
            }

            if (context->SearchRegexMatchData)
            {
                pcre2_match_data_free(context->SearchRegexMatchData);
                context->SearchRegexMatchData = NULL;
            }

            PhSetIntegerSetting(L"FindObjRegex", Button_GetCheck(GetDlgItem(hwndDlg, IDC_REGEX)) == BST_CHECKED);
            PhSaveWindowPlacementToSetting(L"FindObjWindowPosition", L"FindObjWindowSize", hwndDlg);

            PhUnregisterWindowCallback(hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhpDeleteHandleObjectTree(context);

            if (context->SearchResults)
            {
                for (ULONG i = 0; i < context->SearchResults->Count; i++)
                    PhFree(context->SearchResults->Items[i]);

                PhClearList(context->SearchResults);
            }

            if (context->WindowText)
                PhDereferenceObject(context->WindowText);
            if (context->TypeWindowFont)
                DeleteFont(context->TypeWindowFont);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhDereferenceObject(context);

            PostQuitMessage(0);
        }
        break;
    case WM_PH_SEARCH_SHOWDIALOG:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_SETCURSOR:
        {
            if (context->SearchThreadHandle)
            {
                SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                return TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            if (GET_WM_COMMAND_HWND(wParam, lParam) == context->TypeWindowHandle)
            {
                switch (GET_WM_COMMAND_CMD(wParam, lParam))
                {
                case CBN_SELCHANGE:
                    {
                        // Change focus from the dropdown list to the searchbox.
                        SetFocus(context->SearchWindowHandle);
                    }
                    break;
                }
            }

            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDOK:
                {
                    // Don't continue if the user requested cancellation.
                    if (context->SearchStop)
                        break;

                    // Restore the original window title. (dmex)
                    PhSetWindowText(hwndDlg, PhGetStringOrEmpty(context->WindowText));

                    if (!context->SearchThreadHandle)
                    {
                        PhMoveReference(&context->SearchString, PhGetWindowText(context->SearchWindowHandle));
                        PhMoveReference(&context->SearchTypeString, PhGetWindowText(context->TypeWindowHandle));

                        if (context->SearchRegexCompiledExpression)
                        {
                            pcre2_code_free(context->SearchRegexCompiledExpression);
                            context->SearchRegexCompiledExpression = NULL;
                        }

                        if (context->SearchRegexMatchData)
                        {
                            pcre2_match_data_free(context->SearchRegexMatchData);
                            context->SearchRegexMatchData = NULL;
                        }

                        if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_REGEX)) == BST_CHECKED)
                        {
                            int errorCode;
                            PCRE2_SIZE errorOffset;

                            context->SearchRegexCompiledExpression = pcre2_compile(
                                context->SearchString->Buffer,
                                context->SearchString->Length / sizeof(WCHAR),
                                PCRE2_CASELESS | PCRE2_DOTALL,
                                &errorCode,
                                &errorOffset,
                                NULL
                                );

                            if (!context->SearchRegexCompiledExpression)
                            {
                                PhShowError2(hwndDlg, L"Unable to compile the regular expression.", L"\"%s\" at position %zu.",
                                    PhGetStringOrDefault(PH_AUTO(PhPcre2GetErrorMessage(errorCode)), L"Unknown error"),
                                    errorOffset
                                    );
                                break;
                            }

                            context->SearchRegexMatchData = pcre2_match_data_create_from_pattern(context->SearchRegexCompiledExpression, NULL);
                        }

                        // Clean up previous results.

                        PhpFindObjectClearResultEntries(context);

                        // Start the search.

                        PhReferenceObject(context);

                        if (!NT_SUCCESS(PhCreateThreadEx(&context->SearchThreadHandle, PhpFindObjectsThreadStart, context)))
                        {
                            PhDereferenceObject(context);
                            break;
                        }

                        PhSetDialogItemText(hwndDlg, IDOK, L"Cancel");

                        SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
                    }
                    else
                    {
                        context->SearchStop = TRUE;
                        EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
                    }
                }
                break;
            case IDCANCEL:
                {
                    DestroyWindow(hwndDlg);
                }
                break;
            case WM_PH_SEARCH_SHOWMENU:
                {
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM selectedItem;
                    PPH_HANDLE_OBJECT_TREE_ROOT_NODE *handleObjectNodes = NULL;
                    ULONG numberOfHandleObjectNodes = 0;

                    if (!PhpGetSelectedHandleObjectNodes(context, &handleObjectNodes, &numberOfHandleObjectNodes))
                        break;

                    if (numberOfHandleObjectNodes != 0)
                    {
                        menu = PhCreateEMenu();
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_OBJECT_CLOSE, L"C&lose\bDel", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_OBJECT_GOTOOWNINGPROCESS, L"Go to owning &process", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_OBJECT_PROPERTIES, L"Prope&rties", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_OBJECT_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
                        PhInsertCopyCellEMenuItem(menu, ID_OBJECT_COPY, context->TreeNewHandle, contextMenuEvent->Column);
                        PhSetFlagsEMenuItem(menu, ID_OBJECT_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);
                        PhpInitializeFindObjMenu(menu, handleObjectNodes, numberOfHandleObjectNodes);

                        selectedItem = PhShowEMenu(
                            menu,
                            hwndDlg,
                            PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            contextMenuEvent->Location.x,
                            contextMenuEvent->Location.y
                            );

                        if (selectedItem && selectedItem->Id != ULONG_MAX)
                        {
                            PhHandleCopyCellEMenuItem(selectedItem);
                        }

                        PhDestroyEMenu(menu);
                    }

                    PhFree(handleObjectNodes);
                }
                break;
            case ID_OBJECT_CLOSE:
                {
                    PPH_HANDLE_OBJECT_TREE_ROOT_NODE *handleObjectNodes = NULL;
                    ULONG numberOfHandleObjectNodes = 0;

                    if (!PhpGetSelectedHandleObjectNodes(context, &handleObjectNodes, &numberOfHandleObjectNodes))
                        break;

                    if (numberOfHandleObjectNodes != 0 && PhShowConfirmMessage(
                        hwndDlg,
                        L"close",
                        numberOfHandleObjectNodes == 1 ? L"the selected handle" : L"the selected handles",
                        L"Closing handles may cause system instability and data corruption.",
                        FALSE
                        ))
                    {
                        for (ULONG i = 0; i < numberOfHandleObjectNodes; i++)
                        {
                            NTSTATUS status;
                            HANDLE processHandle;

                            if (handleObjectNodes[i]->ResultType != HandleSearchResult)
                                continue;

                            if (WindowsVersion >= WINDOWS_10)
                            {
                                if (NT_SUCCESS(PhOpenProcess(
                                    &processHandle,
                                    PROCESS_QUERY_INFORMATION,
                                    handleObjectNodes[i]->ProcessId
                                    )))
                                {
                                    BOOLEAN critical = FALSE;
                                    BOOLEAN strict = FALSE;
                                    BOOLEAN breakOnTermination;
                                    PROCESS_MITIGATION_POLICY_INFORMATION policyInfo;

                                    if (NT_SUCCESS(PhGetProcessBreakOnTermination(
                                        processHandle,
                                        &breakOnTermination
                                        )))
                                    {
                                        if (breakOnTermination)
                                        {
                                            critical = TRUE;
                                        }
                                    }

                                    policyInfo.Policy = ProcessStrictHandleCheckPolicy;
                                    policyInfo.StrictHandleCheckPolicy.Flags = 0;

                                    if (NT_SUCCESS(NtQueryInformationProcess(
                                        processHandle,
                                        ProcessMitigationPolicy,
                                        &policyInfo,
                                        sizeof(PROCESS_MITIGATION_POLICY_INFORMATION),
                                        NULL
                                        )))
                                    {
                                        if (policyInfo.StrictHandleCheckPolicy.Flags != 0)
                                        {
                                            strict = TRUE;
                                        }
                                    }

                                    if (critical && strict)
                                    {
                                        if (!PhShowConfirmMessage(
                                            hwndDlg,
                                            L"close",
                                            L"critical handle(s)",
                                            L"You are about to close one or more handles for a critical process with strict handle checks enabled. This will shut down the operating system immediately.\r\n\r\n",
                                            TRUE
                                            ))
                                        {
                                            NtClose(processHandle);
                                            continue;
                                        }
                                    }

                                    NtClose(processHandle);
                                }
                            }

                            if (NT_SUCCESS(status = PhOpenProcess(
                                &processHandle,
                                PROCESS_DUP_HANDLE,
                                handleObjectNodes[i]->ProcessId
                                )))
                            {
                                if (NT_SUCCESS(status = NtDuplicateObject(
                                    processHandle,
                                    handleObjectNodes[i]->Handle,
                                    NULL,
                                    NULL,
                                    0,
                                    0,
                                    DUPLICATE_CLOSE_SOURCE
                                    )))
                                {
                                    PhpRemoveHandleObjectNode(context, handleObjectNodes[i]);
                                }

                                NtClose(processHandle);
                            }

                            if (!NT_SUCCESS(status))
                            {
                                if (!PhShowContinueStatus(hwndDlg,
                                    PhaFormatString(L"Unable to close \"%s\"", PhGetStringOrDefault(handleObjectNodes[i]->BestObjectName, L"??"))->Buffer,
                                    status,
                                    0
                                    ))
                                    break;
                            }
                        }
                    }

                    PhFree(handleObjectNodes);
                }
                break;
            case ID_HANDLE_OBJECTPROPERTIES1:
            case ID_HANDLE_OBJECTPROPERTIES2:
                {
                    PPH_HANDLE_OBJECT_TREE_ROOT_NODE handleObjectNode;

                    if (handleObjectNode = PhpGetSelectedHandleObjectNode(context))
                    {
                        PH_HANDLE_ITEM_INFO info;

                        info.ProcessId = handleObjectNode->ProcessId;
                        info.Handle = handleObjectNode->Handle;
                        info.TypeName = handleObjectNode->TypeNameString;
                        info.BestObjectName = handleObjectNode->BestObjectName;

                        if (GET_WM_COMMAND_ID(wParam, lParam) == ID_HANDLE_OBJECTPROPERTIES1)
                            PhShowHandleObjectProperties1(hwndDlg, &info);
                        else
                            PhShowHandleObjectProperties2(hwndDlg, &info);
                    }
                }
                break;
            case ID_OBJECT_GOTOOWNINGPROCESS:
                {
                    PPH_HANDLE_OBJECT_TREE_ROOT_NODE handleObjectNode;

                    if (handleObjectNode = PhpGetSelectedHandleObjectNode(context))
                    {
                        PPH_PROCESS_NODE processNode;

                        if (processNode = PhFindProcessNode(handleObjectNode->ProcessId))
                        {
                            ProcessHacker_SelectTabPage(0);
                            ProcessHacker_SelectProcessNode(processNode);
                            ProcessHacker_ToggleVisible(TRUE);
                        }
                    }
                }
                break;
            case ID_OBJECT_PROPERTIES:
                {
                    PPH_HANDLE_OBJECT_TREE_ROOT_NODE handleObjectNode;

                    if (handleObjectNode = PhpGetSelectedHandleObjectNode(context))
                    {
                        if (handleObjectNode->ResultType == HandleSearchResult)
                        {
                            PPH_HANDLE_ITEM handleItem;

                            handleItem = PhCreateHandleItem(&handleObjectNode->HandleInfo);

                            if (!PhIsNullOrEmptyString(handleObjectNode->BestObjectName))
                            {
                                handleItem->BestObjectName = handleItem->ObjectName = handleObjectNode->BestObjectName;
                                PhReferenceObjectEx(handleObjectNode->BestObjectName, 2);
                            }

                            if (!PhIsNullOrEmptyString(handleObjectNode->TypeNameString))
                            {
                                handleItem->TypeName = handleObjectNode->TypeNameString;
                                PhReferenceObject(handleObjectNode->TypeNameString);
                            }

                            PhShowHandleProperties(
                                hwndDlg,
                                handleObjectNode->ProcessId,
                                handleItem
                                );
                            PhDereferenceObject(handleItem);
                        }
                        else
                        {
                            // DLL or Mapped File. Just show file properties.
                            PhShellProperties(hwndDlg, handleObjectNode->BestObjectName->Buffer);
                        }
                    }
                }
                break;
            case ID_OBJECT_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                    PhSetClipboardString(context->TreeNewHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            }
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
    case WM_TIMER:
        {
            if (!context->SearchThreadHandle)
                break;

            // Update the search results.
            PhpFindObjectAddResultEntries(context);
        }
        break;
    case WM_PH_SEARCH_FINISHED:
        {
            // Add any un-added items.
            PhpFindObjectAddResultEntries(context);

            // Add the result count to the window title. (dmex)
            PhSetWindowText(hwndDlg, PhaFormatString(
                L"%s (%lu results)",
                PhGetStringOrEmpty(context->WindowText),
                context->SearchResultsAddIndex
                )->Buffer);

            NtWaitForSingleObject(context->SearchThreadHandle, FALSE, NULL);
            NtClose(context->SearchThreadHandle);
            context->SearchThreadHandle = NULL;
            context->SearchStop = FALSE;

            PhSetDialogItemText(hwndDlg, IDOK, L"Find");
            EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);

            SetCursor(LoadCursor(NULL, IDC_ARROW));

            if ((NTSTATUS)wParam == STATUS_INSUFFICIENT_RESOURCES)
            {
                PhShowWarning(
                    hwndDlg,
                    L"Unable to search for handles because the total number of handles on the system is too large.\r\n%s",
                    L"Please check if there are any processes with an extremely large number of handles open."
                    );
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

NTSTATUS PhpFindObjectsDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    PhFindObjectsWindowHandle = PhCreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_FINDOBJECTS),
        NULL,
        PhpFindObjectsDlgProc,
        NULL
        );

    PhSetEvent(&PhFindObjectsInitializedEvent);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(PhFindObjectsWindowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
    PhResetEvent(&PhFindObjectsInitializedEvent);

    if (PhFindObjectsThreadHandle)
    {
        NtClose(PhFindObjectsThreadHandle);
        PhFindObjectsThreadHandle = NULL;
    }

    return STATUS_SUCCESS;
}

VOID PhShowFindObjectsDialog(
    VOID
    )
{
    if (!PhFindObjectsThreadHandle)
    {
        if (!NT_SUCCESS(PhCreateThreadEx(&PhFindObjectsThreadHandle, PhpFindObjectsDialogThreadStart, NULL)))
        {
            PhShowError(PhMainWndHandle, L"%s", L"Unable to create the window.");
            return;
        }

        PhWaitForEvent(&PhFindObjectsInitializedEvent, NULL);
    }

    PostMessage(PhFindObjectsWindowHandle, WM_PH_SEARCH_SHOWDIALOG, 0, 0);
}
