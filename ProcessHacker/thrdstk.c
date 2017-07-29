/*
 * Process Hacker -
 *   thread stack viewer
 *
 * Copyright (C) 2010-2016 wj32
 * Copyright (C) 2017 dmex
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

#include <cpysave.h>
#include <emenu.h>
#include <kphuser.h>
#include <symprv.h>

#include <actions.h>
#include <colmgr.h>
#include <phplug.h>
#include <settings.h>
#include <thrdprv.h>

#define WM_PH_COMPLETED (WM_APP + 301)
#define WM_PH_STATUS_UPDATE (WM_APP + 302)
#define WM_PH_SHOWSTACKMENU (WM_APP + 303)

static RECT MinimumSize = { -1, -1, -1, -1 };

typedef struct _PH_THREAD_STACK_CONTEXT
{
    HANDLE ProcessId;
    HANDLE ThreadId;
    HANDLE ThreadHandle;
    PPH_THREAD_PROVIDER ThreadProvider;
    PPH_SYMBOL_PROVIDER SymbolProvider;
    BOOLEAN CustomWalk;

    BOOLEAN StopWalk;
    PPH_LIST List;
    PPH_LIST NewList;
    HWND ProgressWindowHandle;
    NTSTATUS WalkStatus;
    PPH_STRING StatusMessage;
    PH_QUEUED_LOCK StatusLock;

    PH_LAYOUT_MANAGER LayoutManager;

    HWND WindowHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
    PPH_LIST NodeRootList;
} PH_THREAD_STACK_CONTEXT, *PPH_THREAD_STACK_CONTEXT;

typedef struct _THREAD_STACK_ITEM
{
    PH_THREAD_STACK_FRAME StackFrame;
    ULONG Index;
    PPH_STRING Symbol;
} THREAD_STACK_ITEM, *PTHREAD_STACK_ITEM;

typedef enum _PH_STACK_TREE_COLUMN_ITEM_NAME
{
    PH_STACK_TREE_COLUMN_INDEX,
    PH_STACK_TREE_COLUMN_SYMBOL,
    PH_STACK_TREE_COLUMN_STACKADDRESS,
    PH_STACK_TREE_COLUMN_FRAMEADDRESS,
    PH_STACK_TREE_COLUMN_PARAMETER1,
    PH_STACK_TREE_COLUMN_PARAMETER2,
    PH_STACK_TREE_COLUMN_PARAMETER3,
    PH_STACK_TREE_COLUMN_PARAMETER4,
    PH_STACK_TREE_COLUMN_CONTROLADDRESS,
    PH_STACK_TREE_COLUMN_RETURNADDRESS,
    TREE_COLUMN_ITEM_MAXIMUM
} PH_STACK_TREE_COLUMN_ITEM_NAME;

typedef struct _PH_STACK_TREE_ROOT_NODE
{
    PH_TREENEW_NODE Node;

    PH_THREAD_STACK_FRAME StackFrame;

    ULONG Index;
    PPH_STRING TooltipText;
    PPH_STRING IndexString;
    PPH_STRING SymbolString;
    WCHAR StackAddressString[PH_PTR_STR_LEN_1];
    WCHAR FrameAddressString[PH_PTR_STR_LEN_1];
    WCHAR Parameter1String[PH_PTR_STR_LEN_1];
    WCHAR Parameter2String[PH_PTR_STR_LEN_1];
    WCHAR Parameter3String[PH_PTR_STR_LEN_1];
    WCHAR Parameter4String[PH_PTR_STR_LEN_1];
    WCHAR PcAddressString[PH_PTR_STR_LEN_1];
    WCHAR ReturnAddressString[PH_PTR_STR_LEN_1];

    PH_STRINGREF TextCache[TREE_COLUMN_ITEM_MAXIMUM];
} PH_STACK_TREE_ROOT_NODE, *PPH_STACK_TREE_ROOT_NODE;

INT_PTR CALLBACK PhpThreadStackDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhpFreeThreadStackItem(
    _In_ PTHREAD_STACK_ITEM StackItem
    );

NTSTATUS PhpRefreshThreadStack(
    _In_ HWND hwnd,
    _In_ PPH_THREAD_STACK_CONTEXT ThreadStackContext
    );

INT_PTR CALLBACK PhpThreadStackProgressDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#define SORT_FUNCTION(Column) ThreadStackTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl ThreadStackTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_STACK_TREE_ROOT_NODE node1 = *(PPH_STACK_TREE_ROOT_NODE*)_elem1; \
    PPH_STACK_TREE_ROOT_NODE node2 = *(PPH_STACK_TREE_ROOT_NODE*)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->Node.Index, (ULONG_PTR)node2->Node.Index); \
    \
    return PhModifySort(sortResult, ((PPH_THREAD_STACK_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Index)
{
    sortResult = uint64cmp(node1->Index, node2->Index);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Symbol)
{
    sortResult = PhCompareString(node1->SymbolString, node2->SymbolString, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(StackAddress)
{
    sortResult = PhCompareStringZ(node1->StackAddressString, node2->StackAddressString, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(FrameAddress)
{
    sortResult = PhCompareStringZ(node1->FrameAddressString, node2->FrameAddressString, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(StackParameter1)
{
    sortResult = PhCompareStringZ(node1->Parameter1String, node2->Parameter1String, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(StackParameter2)
{
    sortResult = PhCompareStringZ(node1->Parameter2String, node2->Parameter2String, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(StackParameter3)
{
    sortResult = PhCompareStringZ(node1->Parameter3String, node2->Parameter3String, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(StackParameter4)
{
    sortResult = PhCompareStringZ(node1->Parameter4String, node2->Parameter4String, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ControlAddress)
{
    sortResult = PhCompareStringZ(node1->PcAddressString, node2->PcAddressString, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ReturnAddress)
{
    sortResult = PhCompareStringZ(node1->ReturnAddressString, node2->ReturnAddressString, TRUE);
}
END_SORT_FUNCTION

VOID ThreadStackLoadSettingsTreeList(
    _Inout_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhGetStringSetting(L"ThreadStackTreeListColumns");
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);
}

VOID ThreadStackSaveSettingsTreeList(
    _Inout_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(L"ThreadStackTreeListColumns", &settings->sr);
    PhDereferenceObject(settings);
}

BOOLEAN ThreadStackNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_STACK_TREE_ROOT_NODE node1 = *(PPH_STACK_TREE_ROOT_NODE *)Entry1;
    PPH_STACK_TREE_ROOT_NODE node2 = *(PPH_STACK_TREE_ROOT_NODE *)Entry2;

    return node1->Index == node2->Index;
}

ULONG ThreadStackNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return (*(PPH_STACK_TREE_ROOT_NODE*)Entry)->Index;
}

VOID DestroyThreadStackNode(
    _In_ PPH_STACK_TREE_ROOT_NODE Node
    )
{
    PhClearReference(&Node->SymbolString);

    PhDereferenceObject(Node);
}

PPH_STACK_TREE_ROOT_NODE AddThreadStackNode(
    _Inout_ PPH_THREAD_STACK_CONTEXT Context,
    _In_ ULONG Index
    )
{
    PPH_STACK_TREE_ROOT_NODE threadStackNode;

    threadStackNode = PhCreateAlloc(sizeof(PH_STACK_TREE_ROOT_NODE));
    memset(threadStackNode, 0, sizeof(PH_STACK_TREE_ROOT_NODE));

    PhInitializeTreeNewNode(&threadStackNode->Node);

    memset(threadStackNode->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);
    threadStackNode->Node.TextCache = threadStackNode->TextCache;
    threadStackNode->Node.TextCacheSize = TREE_COLUMN_ITEM_MAXIMUM;

    threadStackNode->Index = Index;

    PhAddEntryHashtable(Context->NodeHashtable, &threadStackNode);
    PhAddItemList(Context->NodeList, threadStackNode);

    // TreeNew_NodesStructured(Context->TreeNewHandle);

    return threadStackNode;
}

PPH_STACK_TREE_ROOT_NODE FindThreadStackNode(
    _In_ PPH_THREAD_STACK_CONTEXT Context,
    _In_ ULONG Index
    )
{
    PH_STACK_TREE_ROOT_NODE lookupThreadStackNode;
    PPH_STACK_TREE_ROOT_NODE lookupThreadStackNodePtr = &lookupThreadStackNode;
    PPH_STACK_TREE_ROOT_NODE *threadStackNode;

    lookupThreadStackNode.Index = Index;

    threadStackNode = (PPH_STACK_TREE_ROOT_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupThreadStackNodePtr
        );

    if (threadStackNode)
        return *threadStackNode;
    else
        return NULL;
}

VOID RemoveThreadStackNode(
    _In_ PPH_THREAD_STACK_CONTEXT Context,
    _In_ PPH_STACK_TREE_ROOT_NODE Node
)
{
    ULONG index = 0;

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != -1)
    {
        PhRemoveItemList(Context->NodeList, index);
    }

    DestroyThreadStackNode(Node);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID UpdateThreadStackNode(
    _In_ PPH_THREAD_STACK_CONTEXT Context,
    _In_ PPH_STACK_TREE_ROOT_NODE Node
    )
{
    memset(Node->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);

    PhInvalidateTreeNewNode(&Node->Node, TN_CACHE_COLOR);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

BOOLEAN NTAPI ThreadStackTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PPH_THREAD_STACK_CONTEXT context = Context;
    PPH_STACK_TREE_ROOT_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPH_STACK_TREE_ROOT_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Index),
                    SORT_FUNCTION(Symbol),
                    SORT_FUNCTION(StackAddress),
                    SORT_FUNCTION(FrameAddress),
                    SORT_FUNCTION(StackParameter1),
                    SORT_FUNCTION(StackParameter2),
                    SORT_FUNCTION(StackParameter3),
                    SORT_FUNCTION(StackParameter4),
                    SORT_FUNCTION(ControlAddress),
                    SORT_FUNCTION(ReturnAddress),
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
            node = (PPH_STACK_TREE_ROOT_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PPH_STACK_TREE_ROOT_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PH_STACK_TREE_COLUMN_INDEX:
                {
                    PhMoveReference(&node->IndexString, PhFormatUInt64(node->Index, TRUE));
                    getCellText->Text = PhGetStringRef(node->IndexString);
                }
                break;
            case PH_STACK_TREE_COLUMN_SYMBOL:
                getCellText->Text = PhGetStringRef(node->SymbolString);
                break;
            case PH_STACK_TREE_COLUMN_STACKADDRESS:
                PhInitializeStringRefLongHint(&getCellText->Text, node->StackAddressString);
                break;
            case PH_STACK_TREE_COLUMN_FRAMEADDRESS:
                PhInitializeStringRefLongHint(&getCellText->Text, node->FrameAddressString);
                break;
            case PH_STACK_TREE_COLUMN_PARAMETER1:
                PhInitializeStringRefLongHint(&getCellText->Text, node->Parameter1String);
                break;
            case PH_STACK_TREE_COLUMN_PARAMETER2:
                PhInitializeStringRefLongHint(&getCellText->Text, node->Parameter2String);
                break;
            case PH_STACK_TREE_COLUMN_PARAMETER3:
                PhInitializeStringRefLongHint(&getCellText->Text, node->Parameter3String);
                break;
            case PH_STACK_TREE_COLUMN_PARAMETER4:
                PhInitializeStringRefLongHint(&getCellText->Text, node->Parameter4String);
                break;
            case PH_STACK_TREE_COLUMN_CONTROLADDRESS:
                PhInitializeStringRefLongHint(&getCellText->Text, node->PcAddressString);
                break;
            case PH_STACK_TREE_COLUMN_RETURNADDRESS:
                PhInitializeStringRefLongHint(&getCellText->Text, node->ReturnAddressString);
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
            node = (PPH_STACK_TREE_ROOT_NODE)getNodeColor->Node;

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
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;
            
            SendMessage(
                context->WindowHandle,
                WM_COMMAND,
                WM_PH_SHOWSTACKMENU,
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
            PhInitializeTreeNewColumnMenu(&data);

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    case TreeNewGetCellTooltip:
        {
            PPH_TREENEW_GET_CELL_TOOLTIP getCellTooltip = Parameter1;
            node = (PPH_STACK_TREE_ROOT_NODE)getCellTooltip->Node;

            if (getCellTooltip->Column->Id != 0)
                return FALSE;

            if (!node->TooltipText)
            {
                PH_STRING_BUILDER stringBuilder;
                PPH_STRING fileName;
                PH_SYMBOL_LINE_INFORMATION lineInfo;

                PhInitializeStringBuilder(&stringBuilder, 40);

                if (PhGetLineFromAddress(
                    context->SymbolProvider,
                    (ULONG64)node->StackFrame.PcAddress,
                    &fileName,
                    NULL,
                    &lineInfo
                    ))
                {
                    PhAppendFormatStringBuilder(
                        &stringBuilder,
                        L"File: %s: line %u\n",
                        fileName->Buffer,
                        lineInfo.LineNumber
                        );
                    PhDereferenceObject(fileName);
                }

                if (stringBuilder.String->Length != 0)
                    PhRemoveEndStringBuilder(&stringBuilder, 1);

                if (PhPluginsEnabled)
                {
                    PH_PLUGIN_THREAD_STACK_CONTROL control;

                    control.Type = PluginThreadStackGetTooltip;
                    control.UniqueKey = context;
                    control.u.GetTooltip.StackFrame = &node->StackFrame;
                    control.u.GetTooltip.StringBuilder = &stringBuilder;
                    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
                }

                node->TooltipText = PhFinalStringBuilderString(&stringBuilder);
            }

            if (!PhIsNullOrEmptyString(node->TooltipText))
            {
                getCellTooltip->Text = node->TooltipText->sr;
                getCellTooltip->Unfolding = FALSE;
                getCellTooltip->Font = NULL; // use default font
                getCellTooltip->MaximumWidth = 550;
            }
            else
            {
                return FALSE;
            }
        }
        return TRUE;
    }

    return FALSE;
}

VOID ClearThreadStackTree(
    _In_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        DestroyThreadStackNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
}

PPH_STACK_TREE_ROOT_NODE GetSelectedThreadStackNode(
    _In_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    PPH_STACK_TREE_ROOT_NODE windowNode = NULL;

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        windowNode = Context->NodeList->Items[i];

        if (windowNode->Node.Selected)
            return windowNode;
    }

    return NULL;
}

VOID GetSelectedThreadStackNodes(
    _In_ PPH_THREAD_STACK_CONTEXT Context,
    _Out_ PPH_STACK_TREE_ROOT_NODE **ThreadStackNodes,
    _Out_ PULONG NumberOfThreadStackNodes
    )
{
    PPH_LIST list;

    list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_STACK_TREE_ROOT_NODE node = (PPH_STACK_TREE_ROOT_NODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node);
        }
    }

    *ThreadStackNodes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfThreadStackNodes = list->Count;

    PhDereferenceObject(list);
}

VOID InitializeThreadStackTree(
    _Inout_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    Context->NodeList = PhCreateList(100);
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPH_STACK_TREE_ROOT_NODE),
        ThreadStackNodeHashtableEqualFunction,
        ThreadStackNodeHashtableHashFunction,
        100
        );

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");

    TreeNew_SetCallback(Context->TreeNewHandle, ThreadStackTreeNewCallback, Context);

    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_INDEX, TRUE, L"#", 30, PH_ALIGN_LEFT, PH_STACK_TREE_COLUMN_INDEX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_SYMBOL, TRUE, L"Name", 250, PH_ALIGN_LEFT, PH_STACK_TREE_COLUMN_SYMBOL, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_STACKADDRESS, FALSE, L"Stack address", 100, PH_ALIGN_LEFT, PH_STACK_TREE_COLUMN_STACKADDRESS, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_FRAMEADDRESS, FALSE, L"Frame address", 100, PH_ALIGN_LEFT, PH_STACK_TREE_COLUMN_FRAMEADDRESS, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_PARAMETER1, FALSE, L"Stack parameter #1", 100, PH_ALIGN_LEFT, PH_STACK_TREE_COLUMN_PARAMETER1, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_PARAMETER2, FALSE, L"Stack parameter #2", 100, PH_ALIGN_LEFT, PH_STACK_TREE_COLUMN_PARAMETER2, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_PARAMETER3, FALSE, L"Stack parameter #3", 100, PH_ALIGN_LEFT, PH_STACK_TREE_COLUMN_PARAMETER3, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_PARAMETER4, FALSE, L"Stack parameter #4", 100, PH_ALIGN_LEFT, PH_STACK_TREE_COLUMN_PARAMETER4, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_CONTROLADDRESS, TRUE, L"Control address", 100, PH_ALIGN_LEFT, PH_STACK_TREE_COLUMN_CONTROLADDRESS, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_RETURNADDRESS, FALSE, L"Return address", 100, PH_ALIGN_LEFT, PH_STACK_TREE_COLUMN_RETURNADDRESS, 0);

    TreeNew_SetTriState(Context->TreeNewHandle, TRUE);

    ThreadStackLoadSettingsTreeList(Context);
}

VOID DeleteThreadStackTree(
    _In_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    ThreadStackSaveSettingsTreeList(Context);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        DestroyThreadStackNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

VOID PhShowThreadStackDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ PPH_THREAD_PROVIDER ThreadProvider
    )
{
    NTSTATUS status;
    PH_THREAD_STACK_CONTEXT threadStackContext;
    HANDLE threadHandle = NULL;

    // If the user is trying to view a system thread stack
    // but KProcessHacker is not loaded, show an error message.
    if (ProcessId == SYSTEM_PROCESS_ID && !KphIsConnected())
    {
        PhShowError(ParentWindowHandle, PH_KPH_ERROR_MESSAGE);
        return;
    }

    memset(&threadStackContext, 0, sizeof(PH_THREAD_STACK_CONTEXT));
    threadStackContext.ProcessId = ProcessId;
    threadStackContext.ThreadId = ThreadId;
    threadStackContext.ThreadProvider = ThreadProvider;
    threadStackContext.SymbolProvider = ThreadProvider->SymbolProvider;

    if (!NT_SUCCESS(status = PhOpenThread(
        &threadHandle,
        THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME,
        ThreadId
        )))
    {
        if (KphIsConnected())
        {
            status = PhOpenThread(
                &threadHandle,
                ThreadQueryAccess,
                ThreadId
                );
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to open the thread", status, 0);
        return;
    }

    threadStackContext.ThreadHandle = threadHandle;
    threadStackContext.List = PhCreateList(10);
    threadStackContext.NewList = PhCreateList(10);
    PhInitializeQueuedLock(&threadStackContext.StatusLock);

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_THRDSTACK),
        ParentWindowHandle,
        PhpThreadStackDlgProc,
        (LPARAM)&threadStackContext
        );

    PhClearReference(&threadStackContext.StatusMessage);
    PhDereferenceObject(threadStackContext.NewList);
    PhDereferenceObject(threadStackContext.List);

    if (threadStackContext.ThreadHandle)
        NtClose(threadStackContext.ThreadHandle);
}

static INT_PTR CALLBACK PhpThreadStackDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_THREAD_STACK_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_THREAD_STACK_CONTEXT)lParam;
        SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)context);
    }
    else
    {
        context = (PPH_THREAD_STACK_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            NTSTATUS status;
            
            context->WindowHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_TREELIST);

            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER)));
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)PH_LOAD_SHARED_ICON_LARGE(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER)));

            SetWindowText(hwndDlg, PhaFormatString(L"Stack - thread %u", HandleToUlong(context->ThreadId))->Buffer);

            InitializeThreadStackTree(context);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_COPY), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REFRESH), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (MinimumSize.left == -1)
            {
                RECT rect;

                rect.left = 0;
                rect.top = 0;
                rect.right = 190;
                rect.bottom = 120;
                MapDialogRect(hwndDlg, &rect);
                MinimumSize = rect;
                MinimumSize.left = 0;
            }

            PhLoadWindowPlacementFromSetting(NULL, L"ThreadStackWindowSize", hwndDlg);
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_THREAD_STACK_CONTROL control;

                control.Type = PluginThreadStackInitializing;
                control.UniqueKey = context;
                control.u.Initializing.ProcessId = context->ProcessId;
                control.u.Initializing.ThreadId = context->ThreadId;
                control.u.Initializing.ThreadHandle = context->ThreadHandle;
                control.u.Initializing.SymbolProvider = context->SymbolProvider;
                control.u.Initializing.CustomWalk = FALSE;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

                context->CustomWalk = control.u.Initializing.CustomWalk;
            }

            status = PhpRefreshThreadStack(hwndDlg, context);

            if (status == STATUS_ABANDONED)
                EndDialog(hwndDlg, IDCANCEL);
            else if (!NT_SUCCESS(status))
                PhShowStatus(hwndDlg, L"Unable to load the stack", status, 0);
        }
        break;
    case WM_DESTROY:
        {
            context->StopWalk = TRUE;

            DeleteThreadStackTree(context);

            PhDeleteLayoutManager(&context->LayoutManager);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_THREAD_STACK_CONTROL control;
                control.Type = PluginThreadStackUninitializing;
                control.UniqueKey = context;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
            }

            for (ULONG i = 0; i < context->List->Count; i++)
                PhpFreeThreadStackItem(context->List->Items[i]);

            PhSaveWindowPlacementToSetting(NULL, L"ThreadStackWindowSize", hwndDlg);

            RemoveProp(hwndDlg, PhMakeContextAtom());
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case IDCANCEL: // Esc and X button to close
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_REFRESH:
                {
                    NTSTATUS status;

                    if (!NT_SUCCESS(status = PhpRefreshThreadStack(hwndDlg, context)))
                    {
                        PhShowStatus(hwndDlg, L"Unable to load the stack", status, 0);
                    }
                }
                break;
            case IDC_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                    PhSetClipboardString(context->TreeNewHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            case WM_PH_SHOWSTACKMENU:
                {
                    PPH_EMENU menu;
                    PPH_STACK_TREE_ROOT_NODE selectedNode;
                    PPH_EMENU_ITEM selectedItem;
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;

                    if (selectedNode = GetSelectedThreadStackNode(context))
                    {
                        menu = PhCreateEMenu();
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"Copy", NULL, NULL), -1);
                        PhInsertCopyCellEMenuItem(menu, IDC_COPY, context->TreeNewHandle, contextMenuEvent->Column);

                        selectedItem = PhShowEMenu(
                            menu,
                            hwndDlg,
                            PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            contextMenuEvent->Location.x,
                            contextMenuEvent->Location.y
                            );

                        if (selectedItem && selectedItem->Id != -1)
                        {
                            BOOLEAN handled = FALSE;

                            handled = PhHandleCopyCellEMenuItem(selectedItem);
                        }

                        PhDestroyEMenu(menu);
                    }
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
            PhResizingMinimumSize((PRECT)lParam, wParam, MinimumSize.right, MinimumSize.bottom);
        }
        break;
    }

    return FALSE;
}

static VOID PhpFreeThreadStackItem(
    _In_ PTHREAD_STACK_ITEM StackItem
    )
{
    PhClearReference(&StackItem->Symbol);
    PhFree(StackItem);
}

static NTSTATUS PhpRefreshThreadStack(
    _In_ HWND hwnd,
    _In_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    ULONG i;

    Context->StopWalk = FALSE;
    PhMoveReference(&Context->StatusMessage, PhCreateString(L"Loading stack..."));

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PROGRESS),
        hwnd,
        PhpThreadStackProgressDlgProc,
        (LPARAM)Context
        );

    if (!Context->StopWalk && NT_SUCCESS(Context->WalkStatus))
    {
        for (i = 0; i < Context->List->Count; i++)
            PhpFreeThreadStackItem(Context->List->Items[i]);

        PhDereferenceObject(Context->List);
        Context->List = Context->NewList;
        Context->NewList = PhCreateList(10);

        ClearThreadStackTree(Context);

        for (i = 0; i < Context->List->Count; i++)
        {
            PTHREAD_STACK_ITEM item = Context->List->Items[i];
            PPH_STACK_TREE_ROOT_NODE stackNode;

            stackNode = AddThreadStackNode(Context, item->Index);
            stackNode->StackFrame = item->StackFrame;

            if (!PhIsNullOrEmptyString(item->Symbol))
                stackNode->SymbolString = PhReferenceObject(item->Symbol);

            PhPrintPointer(stackNode->StackAddressString, item->StackFrame.StackAddress);
            PhPrintPointer(stackNode->FrameAddressString, item->StackFrame.FrameAddress);

            // There are no params for kernel-mode stack traces.
            if ((ULONG_PTR)item->StackFrame.PcAddress <= PhSystemBasicInformation.MaximumUserModeAddress)
            {
                PhPrintPointer(stackNode->Parameter1String, item->StackFrame.Params[0]);
                PhPrintPointer(stackNode->Parameter2String, item->StackFrame.Params[1]);
                PhPrintPointer(stackNode->Parameter3String, item->StackFrame.Params[2]);
                PhPrintPointer(stackNode->Parameter4String, item->StackFrame.Params[3]);
            }

            PhPrintPointer(stackNode->PcAddressString, item->StackFrame.PcAddress);
            PhPrintPointer(stackNode->ReturnAddressString, item->StackFrame.ReturnAddress);

            UpdateThreadStackNode(Context, stackNode);
        }

        TreeNew_NodesStructured(Context->TreeNewHandle);
    }
    else
    {
        for (i = 0; i < Context->NewList->Count; i++)
            PhpFreeThreadStackItem(Context->NewList->Items[i]);

        PhClearList(Context->NewList);
    }

    if (Context->StopWalk)
        return STATUS_ABANDONED;

    return Context->WalkStatus;
}

static BOOLEAN NTAPI PhpWalkThreadStackCallback(
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _In_opt_ PVOID Context
    )
{
    PPH_THREAD_STACK_CONTEXT threadStackContext = (PPH_THREAD_STACK_CONTEXT)Context;
    PPH_STRING symbol;
    PTHREAD_STACK_ITEM item;

    if (threadStackContext->StopWalk)
        return FALSE;

    PhAcquireQueuedLockExclusive(&threadStackContext->StatusLock);
    PhMoveReference(&threadStackContext->StatusMessage,
        PhFormatString(L"Processing frame %u...", threadStackContext->NewList->Count));
    PhReleaseQueuedLockExclusive(&threadStackContext->StatusLock);
    PostMessage(threadStackContext->ProgressWindowHandle, WM_PH_STATUS_UPDATE, 0, 0);

    symbol = PhGetSymbolFromAddress(
        threadStackContext->SymbolProvider,
        (ULONG64)StackFrame->PcAddress,
        NULL,
        NULL,
        NULL,
        NULL
        );

    if (symbol &&
        (StackFrame->Flags & PH_THREAD_STACK_FRAME_I386) &&
        !(StackFrame->Flags & PH_THREAD_STACK_FRAME_FPO_DATA_PRESENT))
    {
        PhMoveReference(&symbol, PhConcatStrings2(symbol->Buffer, L" (No unwind info)"));
    }

    item = PhAllocate(sizeof(THREAD_STACK_ITEM));
    item->StackFrame = *StackFrame;
    item->Index = threadStackContext->NewList->Count;

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_THREAD_STACK_CONTROL control;

        control.Type = PluginThreadStackResolveSymbol;
        control.UniqueKey = threadStackContext;
        control.u.ResolveSymbol.StackFrame = StackFrame;
        control.u.ResolveSymbol.Symbol = symbol;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

        symbol = control.u.ResolveSymbol.Symbol;
    }

    item->Symbol = symbol;
    PhAddItemList(threadStackContext->NewList, item);

    return TRUE;
}

static NTSTATUS PhpRefreshThreadStackThreadStart(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    NTSTATUS status;
    PPH_THREAD_STACK_CONTEXT threadStackContext = Parameter;
    CLIENT_ID clientId;
    BOOLEAN defaultWalk;

    PhInitializeAutoPool(&autoPool);

    PhLoadSymbolsThreadProvider(threadStackContext->ThreadProvider);

    clientId.UniqueProcess = threadStackContext->ProcessId;
    clientId.UniqueThread = threadStackContext->ThreadId;
    defaultWalk = TRUE;

    if (threadStackContext->CustomWalk)
    {
        PH_PLUGIN_THREAD_STACK_CONTROL control;

        control.Type = PluginThreadStackWalkStack;
        control.UniqueKey = threadStackContext;
        control.u.WalkStack.Status = STATUS_UNSUCCESSFUL;
        control.u.WalkStack.ThreadHandle = threadStackContext->ThreadHandle;
        control.u.WalkStack.ProcessHandle = threadStackContext->SymbolProvider->ProcessHandle;
        control.u.WalkStack.ClientId = &clientId;
        control.u.WalkStack.Flags = PH_WALK_I386_STACK | PH_WALK_AMD64_STACK | PH_WALK_KERNEL_STACK;
        control.u.WalkStack.Callback = PhpWalkThreadStackCallback;
        control.u.WalkStack.CallbackContext = threadStackContext;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
        status = control.u.WalkStack.Status;

        if (NT_SUCCESS(status))
            defaultWalk = FALSE;
    }

    if (defaultWalk)
    {
        PH_PLUGIN_THREAD_STACK_CONTROL control;

        control.UniqueKey = threadStackContext;

        if (PhPluginsEnabled)
        {
            control.Type = PluginThreadStackBeginDefaultWalkStack;
            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
        }

        status = PhWalkThreadStack(
            threadStackContext->ThreadHandle,
            threadStackContext->SymbolProvider->ProcessHandle,
            &clientId,
            threadStackContext->SymbolProvider,
            PH_WALK_I386_STACK | PH_WALK_AMD64_STACK | PH_WALK_KERNEL_STACK,
            PhpWalkThreadStackCallback,
            threadStackContext
            );

        if (PhPluginsEnabled)
        {
            control.Type = PluginThreadStackEndDefaultWalkStack;
            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
        }
    }

    if (threadStackContext->NewList->Count != 0)
        status = STATUS_SUCCESS;

    threadStackContext->WalkStatus = status;
    PostMessage(threadStackContext->ProgressWindowHandle, WM_PH_COMPLETED, 0, 0);

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

static INT_PTR CALLBACK PhpThreadStackProgressDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_THREAD_STACK_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_THREAD_STACK_CONTEXT)lParam;
        SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)context);
    }
    else
    {
        context = (PPH_THREAD_STACK_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE threadHandle;

            context->ProgressWindowHandle = hwndDlg;
            
            if (threadHandle = PhCreateThread(0, PhpRefreshThreadStackThreadStart, context))
            {
                NtClose(threadHandle);
            }
            else
            {
                context->WalkStatus = STATUS_UNSUCCESSFUL;
                EndDialog(hwndDlg, IDOK);
                break;
            }

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhSetWindowStyle(GetDlgItem(hwndDlg, IDC_PROGRESS), PBS_MARQUEE, PBS_MARQUEE);
            SendMessage(GetDlgItem(hwndDlg, IDC_PROGRESS), PBM_SETMARQUEE, TRUE, 75);
            SetWindowText(hwndDlg, L"Loading stack...");
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, PhMakeContextAtom());
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);
                    context->StopWalk = TRUE;
                }
                break;
            }
        }
        break;
    case WM_PH_COMPLETED:
        {
            EndDialog(hwndDlg, IDOK);
        }
        break;
    case WM_PH_STATUS_UPDATE:
        {
            PPH_STRING message;

            PhAcquireQueuedLockExclusive(&context->StatusLock);
            message = context->StatusMessage;
            PhReferenceObject(message);
            PhReleaseQueuedLockExclusive(&context->StatusLock);

            SetDlgItemText(hwndDlg, IDC_PROGRESSTEXT, message->Buffer);
            PhDereferenceObject(message);
        }
        break;
    }

    return FALSE;
}
