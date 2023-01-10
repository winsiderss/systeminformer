/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2022
 *
 */

#include <phapp.h>
#include <phsettings.h>

#include <cpysave.h>
#include <emenu.h>
#include <kphuser.h>
#include <symprv.h>

#include <actions.h>
#include <colmgr.h>
#include <phplug.h>
#include <settings.h>
#include <thrdprv.h>

#include <dbghelp.h>

#define WM_PH_COMPLETED (WM_APP + 301)
//#define WM_PH_STATUS_UPDATE (WM_APP + 302)
#define WM_PH_SHOWSTACKMENU (WM_APP + 303)
#define WM_PH_SHOWSTACKDEFAULT (WM_APP + 304)

static PPH_OBJECT_TYPE PhThreadStackContextType = NULL;
static RECT MinimumSize = { -1, -1, -1, -1 };

typedef struct _PH_THREAD_STACK_CONTEXT
{
    HANDLE ProcessId;
    HANDLE ThreadId;
    HANDLE ThreadHandle;
    PPH_THREAD_PROVIDER ThreadProvider;
    PPH_SYMBOL_PROVIDER SymbolProvider;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG CustomWalk : 1;
            ULONG StopWalk : 1;
            ULONG EnableCloseDialog : 1;
            ULONG HighlightSystemPages : 1;
            ULONG HighlightUserPages : 1;
            ULONG HideSystemPages : 1;
            ULONG HideUserPages : 1;
            ULONG HighlightInlineFrames : 1;
            ULONG HideInlineFrames : 1;
            ULONG Spare : 23;
        };
    };

    PPH_LIST List;
    PPH_LIST NewList;
    PH_TN_FILTER_SUPPORT TreeFilterSupport;
    PPH_TN_FILTER_ENTRY TreeFilterEntry;

    HWND TaskDialogHandle;

    NTSTATUS WalkStatus;
    PPH_STRING StatusMessage;
    PPH_STRING StatusContent;
    PH_QUEUED_LOCK StatusLock;

    BOOLEAN SymbolProgressMarquee;
    BOOLEAN SymbolProgressReset;
    ULONG SymbolProgress;

    PH_LAYOUT_MANAGER LayoutManager;

    HWND WindowHandle;
    HWND ParentHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
    PPH_LIST NodeRootList;
    WNDPROC ThreadStackStatusDefaultWindowProc;
    PH_CALLBACK_REGISTRATION SymbolProviderEventRegistration;
} PH_THREAD_STACK_CONTEXT, *PPH_THREAD_STACK_CONTEXT;

typedef struct _THREAD_STACK_ITEM
{
    PH_THREAD_STACK_FRAME StackFrame;
    ULONG Index;
    PPH_STRING Symbol;
    PPH_STRING FileName;
    PPH_STRING LineText;
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
    PH_STACK_TREE_COLUMN_FILENAME,
    PH_STACK_TREE_COLUMN_LINETEXT,
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
    PPH_STRING FileNameString;
    PPH_STRING LineTextString;
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

typedef enum _PH_THREAD_STACK_MENUITEM
{
    PH_THREAD_STACK_MENUITEM_INSPECT = 1,
    PH_THREAD_STACK_MENUITEM_OPENFILELOCATION,
} PH_THREAD_STACK_MENUITEM;

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

BEGIN_SORT_FUNCTION(FileName)
{
    sortResult = PhCompareStringWithNull(node1->FileNameString, node2->FileNameString, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LineText)
{
    sortResult = PhCompareStringWithNull(node1->LineTextString, node2->LineTextString, TRUE);
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
    return PhHashInt32((*(PPH_STACK_TREE_ROOT_NODE*)Entry)->Index);
}

VOID DestroyThreadStackNode(
    _In_ PPH_STACK_TREE_ROOT_NODE Node
    )
{
    if (Node->TooltipText)
        PhDereferenceObject(Node->TooltipText);
    if (Node->IndexString)
        PhDereferenceObject(Node->IndexString);

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

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
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
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
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
                    SORT_FUNCTION(FileName),
                    SORT_FUNCTION(LineText),
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
            case PH_STACK_TREE_COLUMN_FILENAME:
                getCellText->Text = PhGetStringRef(node->FileNameString);
                break;
            case PH_STACK_TREE_COLUMN_LINETEXT:
                getCellText->Text = PhGetStringRef(node->LineTextString);
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

            if (context->HighlightInlineFrames && PhIsStackFrameTypeInline(node->StackFrame.InlineFrameContext))
            {
                getNodeColor->BackColor = PhGetIntegerSetting(L"ColorInlineThreadStack");
            }
            else if (context->HighlightSystemPages && (ULONG_PTR)node->StackFrame.PcAddress > PhSystemBasicInformation.MaximumUserModeAddress)
            {
                getNodeColor->BackColor = PhGetIntegerSetting(L"ColorSystemThreadStack");
            }
            else if (context->HighlightUserPages && (ULONG_PTR)node->StackFrame.PcAddress <= PhSystemBasicInformation.MaximumUserModeAddress)
            {
                getNodeColor->BackColor = PhGetIntegerSetting(L"ColorUserThreadStack");
            }

            getNodeColor->Flags = TN_AUTO_FORECOLOR;
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
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case VK_F5:
                SendMessage(context->WindowHandle, WM_COMMAND, IDC_REFRESH, 0);
                break;
            }
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            SendMessage(context->WindowHandle, WM_COMMAND, WM_PH_SHOWSTACKDEFAULT, 0);
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
                    PH_FORMAT format[5];
                    SIZE_T returnLength;
                    WCHAR buffer[0x100];

                    PhMoveReference(&fileName, PhGetFileName(fileName));

                    // File: %s: line %lu\n
                    PhInitFormatS(&format[0], L"File: ");
                    PhInitFormatSR(&format[1], fileName->sr);
                    PhInitFormatS(&format[2], L": line ");
                    PhInitFormatU(&format[3], lineInfo.LineNumber);
                    PhInitFormatS(&format[4], L"\n");

                    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), &returnLength))
                    {
                        PhAppendStringBuilderEx(&stringBuilder, buffer, returnLength - sizeof(UNICODE_NULL));
                    }
                    else
                    {
                        PhAppendFormatStringBuilder(
                            &stringBuilder,
                            L"File: %s: line %lu\n",
                            fileName->Buffer,
                            lineInfo.LineNumber
                            );
                    }

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
    case TreeNewGetDialogCode:
        {
            PULONG code = Parameter2;

            if (PtrToUlong(Parameter1) == VK_F5)
            {
                *code = DLGC_WANTMESSAGE;
                return TRUE;
            }
        }
        return FALSE;
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
    PPH_STACK_TREE_ROOT_NODE stackNode = NULL;

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        stackNode = Context->NodeList->Items[i];

        if (stackNode->Node.Selected)
            return stackNode;
    }

    return NULL;
}

_Success_(return)
BOOLEAN GetSelectedThreadStackNodes(
    _In_ PPH_THREAD_STACK_CONTEXT Context,
    _Out_ PPH_STACK_TREE_ROOT_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_STACK_TREE_ROOT_NODE node = (PPH_STACK_TREE_ROOT_NODE)Context->NodeList->Items[i];

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

BOOLEAN PhpThreadStackTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_ PVOID Context
    )
{
    PPH_THREAD_STACK_CONTEXT stackContext = Context;
    PPH_STACK_TREE_ROOT_NODE stackNode = (PPH_STACK_TREE_ROOT_NODE)Node;

    if (stackContext->HideSystemPages && (ULONG_PTR)stackNode->StackFrame.PcAddress > PhSystemBasicInformation.MaximumUserModeAddress)
        return FALSE;
    if (stackContext->HideUserPages && (ULONG_PTR)stackNode->StackFrame.PcAddress <= PhSystemBasicInformation.MaximumUserModeAddress)
        return FALSE;
    if (stackContext->HideInlineFrames && PhIsStackFrameTypeInline(stackNode->StackFrame.InlineFrameContext))
        return FALSE;

    return TRUE;
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

    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_INDEX, TRUE, L"#", 30, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_SYMBOL, TRUE, L"Name", 250, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_STACKADDRESS, FALSE, L"Stack address", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_FRAMEADDRESS, FALSE, L"Frame address", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_PARAMETER1, FALSE, L"Stack parameter #1", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_PARAMETER2, FALSE, L"Stack parameter #2", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_PARAMETER3, FALSE, L"Stack parameter #3", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_PARAMETER4, FALSE, L"Stack parameter #4", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_CONTROLADDRESS, FALSE, L"Control address", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_RETURNADDRESS, FALSE, L"Return address", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_FILENAME, FALSE, L"File name", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_LINETEXT, FALSE, L"Line number", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);

    TreeNew_SetTriState(Context->TreeNewHandle, FALSE);
    TreeNew_SetSort(Context->TreeNewHandle, PH_STACK_TREE_COLUMN_INDEX, AscendingSortOrder);

    ThreadStackLoadSettingsTreeList(Context);

    PhInitializeTreeNewFilterSupport(&Context->TreeFilterSupport, Context->TreeNewHandle, Context->NodeList);
    Context->TreeFilterEntry = PhAddTreeNewFilter(&Context->TreeFilterSupport, PhpThreadStackTreeFilterCallback, Context);
}

VOID DeleteThreadStackTree(
    _In_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    PhRemoveTreeNewFilter(&Context->TreeFilterSupport, Context->TreeFilterEntry);
    PhDeleteTreeNewFilterSupport(&Context->TreeFilterSupport);

    ThreadStackSaveSettingsTreeList(Context);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        DestroyThreadStackNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

VOID NTAPI PhpThreadStackContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_THREAD_STACK_CONTEXT context = (PPH_THREAD_STACK_CONTEXT)Object;

    if (context->StatusMessage) PhDereferenceObject(context->StatusMessage);
    if (context->StatusContent) PhDereferenceObject(context->StatusContent);
    if (context->NewList) PhDereferenceObject(context->NewList);
    if (context->List) PhDereferenceObject(context->List);

    if (context->ThreadHandle)
        NtClose(context->ThreadHandle);
}

VOID PhShowThreadStackDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ PPH_THREAD_PROVIDER ThreadProvider
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    NTSTATUS status;
    HANDLE threadHandle;
    PPH_THREAD_STACK_CONTEXT context;

    // If the user is trying to view a system thread stack
    // but KSystemInformer is not loaded, show an error message.
    if (ProcessId == SYSTEM_PROCESS_ID && (KphLevel() < KphLevelMed))
    {
        PhShowError2(ParentWindowHandle, PH_KPH_ERROR_TITLE, L"%s", PH_KPH_ERROR_MESSAGE);
        return;
    }

    // The idle process has pseudo CIDs (dmex)
    if (ProcessId == SYSTEM_IDLE_PROCESS_ID &&
        HandleToUlong(ThreadId) < PhSystemProcessorInformation.NumberOfProcessors)
    {
        PhShowStatus(ParentWindowHandle, L"Unable to open the thread.", STATUS_UNSUCCESSFUL, 0);
        return;
    }

    if (!NT_SUCCESS(status = PhOpenThread(
        &threadHandle,
        THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME,
        ThreadId
        )))
    {
        if (KphLevel() >= KphLevelMed)
        {
            status = PhOpenThread(
                &threadHandle,
                THREAD_QUERY_LIMITED_INFORMATION,
                ThreadId
                );
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to open the thread.", status, 0);
        return;
    }

    if (PhBeginInitOnce(&initOnce))
    {
        PhThreadStackContextType = PhCreateObjectType(L"ThreadStackContext", 0, PhpThreadStackContextDeleteProcedure);
        PhEndInitOnce(&initOnce);
    }

    context = PhCreateObjectZero(sizeof(PH_THREAD_STACK_CONTEXT), PhThreadStackContextType);
    context->List = PhCreateList(10);
    context->NewList = PhCreateList(10);
    PhInitializeQueuedLock(&context->StatusLock);

    context->ParentHandle = ParentWindowHandle;
    context->ThreadHandle = threadHandle;
    context->ProcessId = ProcessId;
    context->ThreadId = ThreadId;
    context->ThreadProvider = ThreadProvider;
    context->SymbolProvider = ThreadProvider->SymbolProvider;

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_THRDSTACK),
        ParentWindowHandle,
        PhpThreadStackDlgProc,
        (LPARAM)context
        );
}

INT_PTR CALLBACK PhpThreadStackDlgProc(
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
            NTSTATUS status;

            context->WindowHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_TREELIST);
            context->HighlightUserPages = !!PhGetIntegerSetting(L"UseColorUserThreadStack");
            context->HighlightSystemPages = !!PhGetIntegerSetting(L"UseColorSystemThreadStack");
            context->HighlightInlineFrames = !!PhGetIntegerSetting(L"UseColorInlineThreadStack");
            PhSetWindowExStyle(context->TreeNewHandle, WS_EX_CLIENTEDGE, 0);

            PhSetApplicationWindowIcon(hwndDlg);

            PhSetWindowText(hwndDlg, PhaFormatString(L"Stack - thread %lu", HandleToUlong(context->ThreadId))->Buffer);

            InitializeThreadStackTree(context);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_OPTIONS), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
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
            PhSetDialogFocus(hwndDlg, context->TreeNewHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);

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
            {
                // HACK: Show error dialog on the parent window.
                PhShowStatus(GetParent(hwndDlg), L"Unable to load the stack.", status, 0);
                EndDialog(hwndDlg, IDCANCEL);
            }
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

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhDereferenceObject(context);
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
            case IDC_REFRESH:
                {
                    NTSTATUS status;

                    if (!NT_SUCCESS(status = PhpRefreshThreadStack(hwndDlg, context)))
                    {
                        PhShowStatus(hwndDlg, L"Unable to refresh the stack.", status, 0);
                    }
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
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(PH_EMENU_DEFAULT, PH_THREAD_STACK_MENUITEM_INSPECT, L"&Inspect", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_THREAD_STACK_MENUITEM_OPENFILELOCATION, L"Open &file location", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"Copy", NULL, NULL), ULONG_MAX);
                        PhInsertCopyCellEMenuItem(menu, IDC_COPY, context->TreeNewHandle, contextMenuEvent->Column);

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
                            BOOLEAN handled = FALSE;

                            handled = PhHandleCopyCellEMenuItem(selectedItem);

                            if (handled)
                                break;

                            switch (selectedItem->Id)
                            {
                            case PH_THREAD_STACK_MENUITEM_INSPECT:
                                {
                                    if (!PhIsNullOrEmptyString(selectedNode->FileNameString) && PhDoesFileExistWin32(PhGetString(selectedNode->FileNameString)))
                                    {
                                        PhShellExecuteUserString(
                                            hwndDlg,
                                            L"ProgramInspectExecutables",
                                            PhGetString(selectedNode->FileNameString),
                                            FALSE,
                                            L"Make sure the PE Viewer executable file is present."
                                            );
                                    }
                                }
                                break;
                            case PH_THREAD_STACK_MENUITEM_OPENFILELOCATION:
                                {
                                    if (!PhIsNullOrEmptyString(selectedNode->FileNameString) && PhDoesFileExistWin32(PhGetString(selectedNode->FileNameString)))
                                    {
                                        PhShellExecuteUserString(
                                            hwndDlg,
                                            L"FileBrowseExecutable",
                                            PhGetString(selectedNode->FileNameString),
                                            FALSE,
                                            L"Make sure the Explorer executable file is present."
                                            );
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
                            }
                        }

                        PhDestroyEMenu(menu);
                    }
                }
                break;
            case WM_PH_SHOWSTACKDEFAULT:
                {
                    PPH_STACK_TREE_ROOT_NODE selectedNode;

                    if (selectedNode = GetSelectedThreadStackNode(context))
                    {
                        if (!PhIsNullOrEmptyString(selectedNode->FileNameString) && PhDoesFileExistWin32(PhGetString(selectedNode->FileNameString)))
                        {
                            PhShellExecuteUserString(
                                hwndDlg,
                                L"ProgramInspectExecutables",
                                PhGetString(selectedNode->FileNameString),
                                FALSE,
                                L"Make sure the PE Viewer executable file is present."
                                );
                        }
                    }
                }
                break;
            case IDC_OPTIONS:
                {
                    RECT rect;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM hideUserItem;
                    PPH_EMENU_ITEM hideSystemItem;
                    PPH_EMENU_ITEM hideInlineItem;
                    PPH_EMENU_ITEM userItem;
                    PPH_EMENU_ITEM systemItem;
                    PPH_EMENU_ITEM inlineItem;
                    PPH_EMENU_ITEM selectedItem;

                    if (!GetWindowRect(GET_WM_COMMAND_HWND(wParam, lParam), &rect))
                        break;

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, hideUserItem = PhCreateEMenuItem(0, 1, L"Hide user frames", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, hideSystemItem = PhCreateEMenuItem(0, 2, L"Hide system frames", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, hideInlineItem = PhCreateEMenuItem(0, 3, L"Hide inline frames", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, userItem = PhCreateEMenuItem(0, 4, L"Highlight user frames", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, systemItem = PhCreateEMenuItem(0, 5, L"Highlight system frames", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, inlineItem = PhCreateEMenuItem(0, 6, L"Highlight inline frames", NULL, NULL), ULONG_MAX);

                    if (context->HideUserPages)
                        hideUserItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HideSystemPages)
                        hideSystemItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HideInlineFrames)
                        hideInlineItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightUserPages)
                        userItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightSystemPages)
                        systemItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightInlineFrames)
                        inlineItem->Flags |= PH_EMENU_CHECKED;

                    selectedItem = PhShowEMenu(
                        menu,
                        GET_WM_COMMAND_HWND(wParam, lParam),
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_BOTTOM,
                        rect.left,
                        rect.top
                        );

                    if (selectedItem && selectedItem->Id)
                    {
                        if (selectedItem->Id == 1)
                        {
                            context->HideUserPages = !context->HideUserPages;
                        }
                        else if (selectedItem->Id == 2)
                        {
                            context->HideSystemPages = !context->HideSystemPages;
                        }
                        else if (selectedItem->Id == 3)
                        {
                            context->HideInlineFrames = !context->HideInlineFrames;
                        }
                        else if (selectedItem->Id == 4)
                        {
                            context->HighlightUserPages = !context->HighlightUserPages;
                            PhSetIntegerSetting(L"UseColorUserThreadStack", context->HighlightUserPages);
                        }
                        else if (selectedItem->Id == 5)
                        {
                            context->HighlightSystemPages = !context->HighlightSystemPages;
                            PhSetIntegerSetting(L"UseColorSystemThreadStack", context->HighlightSystemPages);
                        }
                        else if (selectedItem->Id == 6)
                        {
                            context->HighlightInlineFrames = !context->HighlightInlineFrames;
                            PhSetIntegerSetting(L"UseColorInlineThreadStack", context->HighlightInlineFrames);
                        }

                        PhApplyTreeNewFilters(&context->TreeFilterSupport);
                    }

                    PhDestroyEMenu(menu);
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
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID PhpFreeThreadStackItem(
    _In_ PTHREAD_STACK_ITEM StackItem
    )
{
    if (StackItem->Symbol) PhDereferenceObject(StackItem->Symbol);
    if (StackItem->FileName) PhDereferenceObject(StackItem->FileName);
    if (StackItem->LineText) PhDereferenceObject(StackItem->LineText);

    PhFree(StackItem);
}

BOOLEAN NTAPI PhpWalkThreadStackCallback(
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _In_opt_ PVOID Context
    )
{
    PPH_THREAD_STACK_CONTEXT threadStackContext = (PPH_THREAD_STACK_CONTEXT)Context;
    PPH_STRING symbol;
    PPH_STRING fileName = NULL;
    PPH_STRING lineText = NULL;
    PTHREAD_STACK_ITEM item;
    ULONG64 baseAddress = 0;
    BOOLEAN enableStackFrameInlineInfo;
    BOOLEAN enableStackFrameLineInfo;

    if (!threadStackContext)
        return FALSE;
    if (threadStackContext->StopWalk)
        return FALSE;

    enableStackFrameInlineInfo = !!PhGetIntegerSetting(L"EnableThreadStackInlineSymbols");
    enableStackFrameLineInfo = !!PhGetIntegerSetting(L"EnableThreadStackLineInformation");

    PhAcquireQueuedLockExclusive(&threadStackContext->StatusLock);
    {
        if (threadStackContext->NewList->Count)
        {
            PH_FORMAT format[3];

            PhInitFormatS(&format[0], L"Processing stack frame ");
            PhInitFormatU(&format[1], threadStackContext->NewList->Count);
            PhInitFormatS(&format[2], L"...");

            PhMoveReference(&threadStackContext->StatusMessage, PhFormat(format, RTL_NUMBER_OF(format), 0));
        }
        else
        {
            PhMoveReference(&threadStackContext->StatusMessage, PhCreateString(L"Processing stack frames..."));
        }
    }
    PhReleaseQueuedLockExclusive(&threadStackContext->StatusLock);

    if (enableStackFrameInlineInfo && PhSymbolProviderInlineContextSupported())
    {
        symbol = PhGetSymbolFromInlineContext(
            threadStackContext->SymbolProvider,
            StackFrame,
            NULL,
            &fileName,
            NULL,
            NULL,
            &baseAddress
            );

        if (symbol &&
            (StackFrame->Flags & PH_THREAD_STACK_FRAME_I386) &&
            !(StackFrame->Flags & PH_THREAD_STACK_FRAME_FPO_DATA_PRESENT))
        {
            PhMoveReference(&symbol, PhConcatStringRefZ(&symbol->sr, L" (No unwind info)"));
        }

        if (PhPluginsEnabled)
        {
            PH_PLUGIN_THREAD_STACK_CONTROL control;

            control.Type = PluginThreadStackResolveSymbol;
            control.UniqueKey = threadStackContext;
            control.u.ResolveSymbol.StackFrame = StackFrame;
            control.u.ResolveSymbol.Symbol = symbol;
            control.u.ResolveSymbol.FileName = fileName;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

            symbol = control.u.ResolveSymbol.Symbol;
            fileName = control.u.ResolveSymbol.FileName;
        }

        if (enableStackFrameLineInfo)
        {
            PPH_STRING lineFileName;
            PH_SYMBOL_LINE_INFORMATION lineInfo;

            if (PhGetLineFromInlineContext(
                threadStackContext->SymbolProvider,
                StackFrame,
                baseAddress,
                &lineFileName,
                NULL,
                &lineInfo
                ))
            {
                PH_FORMAT format[3];

                PhInitFormatSR(&format[0], lineFileName->sr);
                PhInitFormatS(&format[1], L" @ ");
                PhInitFormatU(&format[2], lineInfo.LineNumber);

                lineText = PhFormat(format, RTL_NUMBER_OF(format), 0);
                PhDereferenceObject(lineFileName);
            }
        }

        if (symbol && PhIsStackFrameTypeInline(StackFrame->InlineFrameContext))
        {
            PhMoveReference(&symbol, PhConcatStringRefZ(&symbol->sr, L" (Inline function)"));
        }
    }
    else
    {
        symbol = PhGetSymbolFromAddress(
            threadStackContext->SymbolProvider,
            (ULONG64)StackFrame->PcAddress,
            NULL,
            &fileName,
            NULL,
            NULL
            );

        if (symbol &&
            (StackFrame->Flags & PH_THREAD_STACK_FRAME_I386) &&
            !(StackFrame->Flags & PH_THREAD_STACK_FRAME_FPO_DATA_PRESENT))
        {
            PhMoveReference(&symbol, PhConcatStringRefZ(&symbol->sr, L" (No unwind info)"));
        }

        if (PhPluginsEnabled)
        {
            PH_PLUGIN_THREAD_STACK_CONTROL control;

            control.Type = PluginThreadStackResolveSymbol;
            control.UniqueKey = threadStackContext;
            control.u.ResolveSymbol.StackFrame = StackFrame;
            control.u.ResolveSymbol.Symbol = symbol;
            control.u.ResolveSymbol.FileName = fileName;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

            symbol = control.u.ResolveSymbol.Symbol;
            fileName = control.u.ResolveSymbol.FileName;
        }

        if (enableStackFrameLineInfo)
        {
            PPH_STRING lineFileName;
            PH_SYMBOL_LINE_INFORMATION lineInfo;

            if (PhGetLineFromAddress(
                threadStackContext->SymbolProvider,
                (ULONG64)StackFrame->PcAddress,
                &lineFileName,
                NULL,
                &lineInfo
                ))
            {
                PH_FORMAT format[3];

                PhInitFormatSR(&format[0], lineFileName->sr);
                PhInitFormatS(&format[1], L" @ ");
                PhInitFormatU(&format[2], lineInfo.LineNumber);

                lineText = PhFormat(format, RTL_NUMBER_OF(format), 0);
                PhDereferenceObject(lineFileName);
            }
        }
    }

    item = PhAllocateZero(sizeof(THREAD_STACK_ITEM));
    item->StackFrame = *StackFrame;
    item->Index = threadStackContext->NewList->Count;
    item->Symbol = symbol;
    item->FileName = fileName;
    item->LineText = lineText;
    PhAddItemList(threadStackContext->NewList, item);

    if ( // Zero inline frames so the stack matches windbg output. (dmex)
        PhSymbolProviderInlineContextSupported() &&
        PhIsStackFrameTypeInline(StackFrame->InlineFrameContext)
        )
    {
        // Note: Only zero the item->StackFrame local copy. (dmex)
        item->StackFrame.PcAddress = 0;
        item->StackFrame.ReturnAddress = 0;
        item->StackFrame.FrameAddress = 0;
        item->StackFrame.StackAddress = 0;
        memset(item->StackFrame.Params, 0, sizeof(item->StackFrame.Params));
    }

    return TRUE;
}

NTSTATUS PhpRefreshThreadStackThreadStart(
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
    PostMessage(threadStackContext->TaskDialogHandle, WM_PH_COMPLETED, 0, 0);

    PhDeleteAutoPool(&autoPool);
    PhDereferenceObject(threadStackContext);

    return STATUS_SUCCESS;
}

LRESULT CALLBACK PhpThreadStackTaskDialogSubclassProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_THREAD_STACK_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(hwndDlg, 0xF)))
        return 0;

    oldWndProc = context->ThreadStackStatusDefaultWindowProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hwndDlg, 0xF);
        }
        break;
    case WM_PH_COMPLETED:
        {
            context->EnableCloseDialog = TRUE;
            SendMessage(hwndDlg, TDM_CLICK_BUTTON, IDOK, 0);
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwndDlg, uMsg, wParam, lParam);
}

VOID PhpSymbolProviderEventCallbackHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_SYMBOL_EVENT_DATA event = Parameter;
    PPH_THREAD_STACK_CONTEXT context = Context;
    PPH_STRING statusMessage = NULL;
    ULONG statusProgress = 0;

    if (!event) return;
    if (!context) return;

    switch (event->EventType)
    {
    case PH_SYMBOL_EVENT_TYPE_LOAD_START:
        statusMessage = PhReferenceObject(event->EventMessage);
        break;
    case PH_SYMBOL_EVENT_TYPE_LOAD_END:
        statusMessage = PhCreateString(L"Loading symbols...");
        break;
    case PH_SYMBOL_EVENT_TYPE_PROGRESS:
        {
            statusMessage = PhReferenceObject(event->EventMessage);
            statusProgress = (ULONG)event->EventProgress;
        }
        break;
    }

    if (statusMessage)
    {
        PhAcquireQueuedLockExclusive(&context->StatusLock);
        PhMoveReference(&context->StatusContent, statusMessage);
        context->SymbolProgress = statusProgress;
        PhReleaseQueuedLockExclusive(&context->StatusLock);
    }
}

HRESULT CALLBACK PhpThreadStackTaskDialogCallback(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_THREAD_STACK_CONTEXT context = (PPH_THREAD_STACK_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_CREATED:
        {
            context->TaskDialogHandle = hwndDlg;

            PhSetApplicationWindowIcon(hwndDlg);
            SendMessage(hwndDlg, TDM_UPDATE_ICON, TDIE_ICON_MAIN, (LPARAM)PhGetApplicationIcon(FALSE));

            SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);
            context->SymbolProgressMarquee = TRUE;

            context->ThreadStackStatusDefaultWindowProc = (WNDPROC)GetWindowLongPtr(hwndDlg, GWLP_WNDPROC);
            PhSetWindowContext(hwndDlg, 0xF, context);
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)PhpThreadStackTaskDialogSubclassProc);

            PhRegisterCallback(
                &PhSymbolEventCallback,
                PhpSymbolProviderEventCallbackHandler,
                context,
                &context->SymbolProviderEventRegistration
                );

            PhReferenceObject(context);
            PhCreateThread2(PhpRefreshThreadStackThreadStart, context);
        }
        break;
    case TDN_DESTROYED:
        {
            PhUnregisterCallback(&PhSymbolEventCallback, &context->SymbolProviderEventRegistration);
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDCANCEL)
            {
                context->StopWalk = TRUE;
                context->SymbolProvider->Terminating = TRUE;
            }

            //if (!context->EnableCloseDialog)
            //    return S_FALSE;
        }
        break;
    case TDN_TIMER:
        {
            PPH_STRING message;
            PPH_STRING content;
            ULONG progress = 0;

            PhAcquireQueuedLockExclusive(&context->StatusLock);

            message = context->StatusMessage;
            content = context->StatusContent;
            progress = context->SymbolProgress;

            if (message) PhReferenceObject(message);
            if (content) PhReferenceObject(content);

            PhReleaseQueuedLockExclusive(&context->StatusLock);

            SendMessage(context->TaskDialogHandle, TDM_SET_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)PhGetStringOrDefault(message, L" "));
            SendMessage(context->TaskDialogHandle, TDM_SET_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)PhGetStringOrDefault(content, L" "));

            if (message) PhDereferenceObject(message);
            if (content) PhDereferenceObject(content);

            if (context->SymbolProgressReset)
            {
                SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
                SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);
                context->SymbolProgressMarquee = TRUE;
                context->SymbolProgressReset = FALSE;
            }

            if (progress)
            {
                if (context->SymbolProgressMarquee)
                {
                    SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);
                    SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, FALSE, 0);
                    context->SymbolProgressMarquee = FALSE;
                }

                SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_POS, (WPARAM)progress, 0);
            }
            else
            {
                if (!context->SymbolProgressMarquee)
                {
                    SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
                    SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);
                    context->SymbolProgressMarquee = TRUE;
                }
            }
        }
        break;
    }

    return S_OK;
}

BOOLEAN PhpShowThreadStackWindow(
    _In_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;
    INT result;

    {
        PPH_STRING windowText;
        HWND control;

        if (control = GetDlgItem(Context->ParentHandle, IDC_STATE))
        {
            if (windowText = PhGetWindowText(control))
            {
                PhMoveReference(&Context->StatusContent, windowText);
            }
        }
    }

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags =
        TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION |
        TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SHOW_MARQUEE_PROGRESS_BAR |
        TDF_CALLBACK_TIMER;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.pfCallback = PhpThreadStackTaskDialogCallback;
    config.lpCallbackData = (LONG_PTR)Context;
    config.hwndParent = Context->WindowHandle;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Processing stack frames...";
    config.pszContent = PhGetStringOrDefault(Context->StatusContent, L"Loading symbols for image...");
    config.cxWidth = 200;

    return SUCCEEDED(TaskDialogIndirect(&config, &result, NULL, NULL)) && result != IDCANCEL;
}

static NTSTATUS PhpRefreshThreadStack(
    _In_ HWND hwnd,
    _In_ PPH_THREAD_STACK_CONTEXT Context
    )
{
    ULONG i;

    Context->StopWalk = FALSE;
    PhMoveReference(&Context->StatusMessage, PhCreateString(L"Processing stack frames..."));

    if (!PhpShowThreadStackWindow(Context))
    {
        return STATUS_ABANDONED;
    }

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
            stackNode->SymbolString = item->Symbol;
            stackNode->FileNameString = item->FileName;
            stackNode->LineTextString = item->LineText;

            if (stackNode->FileNameString)
                stackNode->FileNameString = PhGetFileName(stackNode->FileNameString);

            if (item->StackFrame.StackAddress)
                PhPrintPointer(stackNode->StackAddressString, item->StackFrame.StackAddress);
            if (item->StackFrame.FrameAddress)
                PhPrintPointer(stackNode->FrameAddressString, item->StackFrame.FrameAddress);

            // There are no params for kernel-mode stack traces.
            if ((ULONG_PTR)item->StackFrame.PcAddress <= PhSystemBasicInformation.MaximumUserModeAddress)
            {
                if (item->StackFrame.Params[0])
                    PhPrintPointer(stackNode->Parameter1String, item->StackFrame.Params[0]);
                if (item->StackFrame.Params[1])
                    PhPrintPointer(stackNode->Parameter2String, item->StackFrame.Params[1]);
                if (item->StackFrame.Params[2])
                    PhPrintPointer(stackNode->Parameter3String, item->StackFrame.Params[2]);
                if (item->StackFrame.Params[3])
                    PhPrintPointer(stackNode->Parameter4String, item->StackFrame.Params[3]);
            }

            if (item->StackFrame.PcAddress)
                PhPrintPointer(stackNode->PcAddressString, item->StackFrame.PcAddress);
            if (item->StackFrame.ReturnAddress)
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
