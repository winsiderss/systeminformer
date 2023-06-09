/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2013-2023
 *
 */

#include "exttools.h"
#include <wct.h>

// Wait Chain Traversal Documentation:
// https://learn.microsoft.com/en-us/windows/win32/debug/wait-chain-traversal
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms681622.aspx

#define WCT_GETINFO_ALL_FLAGS (WCT_OUT_OF_PROC_FLAG|WCT_OUT_OF_PROC_COM_FLAG|WCT_OUT_OF_PROC_CS_FLAG|WCT_NETWORK_IO_FLAG)

typedef enum _WCT_TREE_COLUMN_ITEM_NAME
{
    TREE_COLUMN_ITEM_TYPE = 0,
    TREE_COLUMN_ITEM_STATUS = 1,
    TREE_COLUMN_ITEM_NAME = 2,
    TREE_COLUMN_ITEM_TIMEOUT = 3,
    TREE_COLUMN_ITEM_ALERTABLE = 4,
    TREE_COLUMN_ITEM_PROCESSID = 5,
    TREE_COLUMN_ITEM_THREADID = 6,
    TREE_COLUMN_ITEM_WAITTIME = 7,
    TREE_COLUMN_ITEM_CONTEXTSWITCH = 8,
    TREE_COLUMN_ITEM_MAXIMUM
} WCT_TREE_COLUMN_ITEM_NAME;

typedef struct _WCT_ROOT_NODE
{
    PH_TREENEW_NODE Node;
    struct _WCT_ROOT_NODE* Parent;
    PPH_LIST Children;

    ULONG64 Index;

    BOOLEAN HasChildren;
    BOOLEAN Alertable;
    BOOLEAN IsDeadLocked;

    WCT_OBJECT_TYPE ObjectType;
    WCT_OBJECT_STATUS ObjectStatus;
    PPH_STRING TimeoutString;
    PPH_STRING ProcessIdString;
    PPH_STRING ThreadIdString;
    PPH_STRING WaitTimeString;
    PPH_STRING ContextSwitchesString;
    PPH_STRING ObjectNameString;

    ULONG ProcessId;
    ULONG ThreadId;
    ULONG WaitTime;
    ULONG ContextSwitches;
    LARGE_INTEGER Timeout;

    PH_STRINGREF TextCache[TREE_COLUMN_ITEM_MAXIMUM];
    WCHAR WindowHandleString[PH_PTR_STR_LEN_1];
} WCT_ROOT_NODE, *PWCT_ROOT_NODE;

typedef struct _WCT_TREE_CONTEXT
{
    HWND ParentWindowHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;

    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
    PPH_LIST NodeRootList;
} WCT_TREE_CONTEXT, *PWCT_TREE_CONTEXT;

typedef struct _WCT_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND TreeNewHandle;

    WCT_TREE_CONTEXT TreeContext;
    PH_LAYOUT_MANAGER LayoutManager;

    PPH_THREAD_ITEM ThreadItem;
    PPH_PROCESS_ITEM ProcessItem;

    HWCT WctSessionHandle;
    PVOID Ole32ModuleHandle;

    PPH_STRING StatusMessage;
    PPH_LIST WaitChainList;
    PPH_LIST DeadLockList;
} WCT_CONTEXT, *PWCT_CONTEXT;

#define ID_WCT_MENU_GOTOTHREAD (WM_APP + 1)
#define ID_WCT_MENU_GOTOPROCESS (WM_APP + 2)
#define ID_WCT_UPDATE_RESULTS (WM_APP + 3)

PWCT_ROOT_NODE WtcAddWaitNode(
    _Inout_ PWCT_TREE_CONTEXT Context
    );

VOID WctAddChildWaitNode(
    _In_ PWCT_TREE_CONTEXT Context,
    _In_opt_ PWCT_ROOT_NODE ParentNode,
    _In_ PWAITCHAIN_NODE_INFO WctNode,
    _In_ BOOLEAN IsDeadLocked
    );

VOID EtWaitChainSetTreeStatusMessage(
    _Inout_ PWCT_CONTEXT Context,
    _In_ BOOLEAN ShowEmpty
    );

VOID EtWaitChainUpdateResults(
    _Inout_ PWCT_CONTEXT Context
    );

VOID WtcDeleteWaitTree(
    _In_ PWCT_TREE_CONTEXT Context
    );

VOID WtcClearWaitTree(
    _In_ PWCT_TREE_CONTEXT Context
    );

PWCT_ROOT_NODE WtcGetSelectedWaitNode(
    _In_ PWCT_TREE_CONTEXT Context
    );

VOID WtcInitializeWaitTree(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PWCT_TREE_CONTEXT Context
    );

VOID NTAPI EtWaitChainContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PWCT_CONTEXT context = Object;

    if (context->ProcessItem)
    {
        PhDereferenceObject(context->ProcessItem);
    }

    if (context->ThreadItem)
    {
        PhDereferenceObject(context->ThreadItem);
    }

    if (context->StatusMessage)
    {
        PhDereferenceObject(context->StatusMessage);
    }

    if (context->WaitChainList)
    {
        for (ULONG i = 0; i < context->WaitChainList->Count; i++)
        {
            PhFree(context->WaitChainList->Items[i]);
        }

        PhDereferenceObject(context->WaitChainList);
    }

    if (context->DeadLockList)
    {
        for (ULONG i = 0; i < context->DeadLockList->Count; i++)
        {
            PhFree(context->DeadLockList->Items[i]);
        }

        PhDereferenceObject(context->DeadLockList);
    }
}

PVOID EtWaitChainContextCreate(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PPH_OBJECT_TYPE EtWaitChainContextType = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        EtWaitChainContextType = PhCreateObjectType(L"EtWaitChainContextType", 0, EtWaitChainContextDeleteProcedure);
        PhEndInitOnce(&initOnce);
    }

    return PhCreateObjectZero(sizeof(WCT_CONTEXT), EtWaitChainContextType);
}

#define SIP(String, Integer) { (String), (PVOID)((Integer)) }
#define SREF(String) ((PVOID)&(PH_STRINGREF)PH_STRINGREF_INIT((String)))
static PH_STRINGREF WaitChainUnknownString = PH_STRINGREF_INIT(L"Unknown");

static PH_KEY_VALUE_PAIR WaitChainObjectTypePairs[] =
{
    SIP(SREF(L"CriticalSection"), WctCriticalSectionType),
    SIP(SREF(L"SendMessage"), WctSendMessageType),
    SIP(SREF(L"Mutex"), WctMutexType),
    SIP(SREF(L"APLC"), WctAlpcType),
    SIP(SREF(L"COM"), WctComType),
    SIP(SREF(L"ThreadWait"), WctThreadWaitType),
    SIP(SREF(L"ProcessWait"), WctProcessWaitType),
    SIP(SREF(L"Thread"), WctThreadType),
    SIP(SREF(L"COM Activation"), WctComActivationType),
    SIP(SREF(L"Unknown"), WctUnknownType),
    SIP(SREF(L"Socket I/O"), WctSocketIoType),
    SIP(SREF(L"SMB I/O"), WctSmbIoType)
};

static PH_KEY_VALUE_PAIR WaitChainObjectStatusPairs[] =
{
    SIP(SREF(L"No Access"), WctStatusNoAccess),
    SIP(SREF(L"Running"), WctStatusRunning),
    SIP(SREF(L"Blocked"), WctStatusBlocked),
    SIP(SREF(L"Pid Only"), WctStatusPidOnly),
    SIP(SREF(L"Pid Only (RPCSS)"), WctStatusPidOnlyRpcss),
    SIP(SREF(L"Owned"), WctStatusOwned),
    SIP(SREF(L"Not Owned"), WctStatusNotOwned),
    SIP(SREF(L"Abandoned"), WctStatusAbandoned),
    SIP(SREF(L"Unknown"), WctStatusUnknown),
    SIP(SREF(L"Error"), WctStatusError),
};

PPH_STRINGREF WaitChainObjectTypeToString(
    _In_ ULONG ObjectType
    )
{
    PPH_STRINGREF string;

    if (PhFindStringSiKeyValuePairs(
        WaitChainObjectTypePairs,
        sizeof(WaitChainObjectTypePairs),
        ObjectType,
        (PWSTR*)&string
        ))
    {
        return string;
    }

    return &WaitChainUnknownString;
}

PPH_STRINGREF WaitChainObjectStatusToString(
    _In_ ULONG ObjectStatus
    )
{
    PPH_STRINGREF string;

    if (PhFindStringSiKeyValuePairs(
        WaitChainObjectStatusPairs,
        sizeof(WaitChainObjectStatusPairs),
        ObjectStatus,
        (PWSTR*)&string
        ))
    {
        return string;
    }

    return &WaitChainUnknownString;
}

VOID WaitChainCheckThread(
    _Inout_ PWCT_CONTEXT Context,
    _In_ HANDLE ThreadId
    )
{
    BOOL isDeadLocked = FALSE;
    ULONG nodeInfoCount = WCT_MAX_NODE_COUNT;
    WAITCHAIN_NODE_INFO nodeInfoArray[WCT_MAX_NODE_COUNT];

    memset(nodeInfoArray, 0, sizeof(nodeInfoArray));

    if (GetThreadWaitChain(
        Context->WctSessionHandle,
        0,
        WCT_GETINFO_ALL_FLAGS,
        HandleToUlong(ThreadId),
        &nodeInfoCount,
        nodeInfoArray,
        &isDeadLocked
        ))
    {
        if (nodeInfoCount > WCT_MAX_NODE_COUNT)
            nodeInfoCount = WCT_MAX_NODE_COUNT;

        PhAddItemList(Context->WaitChainList, PhAllocateCopy(nodeInfoArray, nodeInfoCount * sizeof(WAITCHAIN_NODE_INFO)));
        PhAddItemList(Context->DeadLockList, UlongToPtr(!!isDeadLocked));
    }
}

BOOLEAN WaitChainEnumNextThread(
    _In_ HANDLE ThreadHandle,
    _In_ PWCT_CONTEXT Context
    )
{
    THREAD_BASIC_INFORMATION basicInfo;

    if (NT_SUCCESS(PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
    {
        WaitChainCheckThread(Context, basicInfo.ClientId.UniqueThread);
    }

    return TRUE;
}

NTSTATUS WaitChainCallbackThread(
    _In_ PWCT_CONTEXT Context
    )
{
    if (Context->Ole32ModuleHandle = PhLoadLibrary(L"ole32.dll"))
    {
        PCOGETCALLSTATE coGetCallStateCallback;
        PCOGETACTIVATIONSTATE coGetActivationStateCallback;

        coGetCallStateCallback = PhGetProcedureAddress(Context->Ole32ModuleHandle, "CoGetCallState", 0);
        coGetActivationStateCallback = PhGetProcedureAddress(Context->Ole32ModuleHandle, "CoGetActivationState", 0);

        if (coGetCallStateCallback && coGetActivationStateCallback)
        {
            RegisterWaitChainCOMCallback(coGetCallStateCallback, coGetActivationStateCallback);
        }
    }

    if (Context->WctSessionHandle = OpenThreadWaitChainSession(0, NULL)) // Synchronous WCT Session
    {
        if (Context->ProcessItem)
        {
            PhEnumNextThread(
                Context->ProcessItem->QueryHandle,
                NULL,
                THREAD_QUERY_LIMITED_INFORMATION,
                WaitChainEnumNextThread,
                Context
                );
        }
        else if (Context->ThreadItem)
        {
            WaitChainCheckThread(Context, Context->ThreadItem->ThreadId);
        }

        CloseThreadWaitChainSession(Context->WctSessionHandle);
    }

    if (Context->Ole32ModuleHandle)
    {
        PhFreeLibrary(Context->Ole32ModuleHandle);
    }

    if (Context->WindowHandle)
    {
        SendMessage(Context->WindowHandle, ID_WCT_UPDATE_RESULTS, 0, 0);
    }

    PhDereferenceObject(Context);
    return STATUS_SUCCESS;
}

INT_PTR CALLBACK WaitChainDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWCT_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PWCT_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_WCT_TREE);
            context->WaitChainList = PhCreateList(1);
            context->DeadLockList = PhCreateList(1);

            PhSetApplicationWindowIcon(hwndDlg);

            WtcInitializeWaitTree(hwndDlg, context->TreeNewHandle, &context->TreeContext);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhGetIntegerPairSetting(SETTING_NAME_WCT_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_WCT_WINDOW_POSITION, SETTING_NAME_WCT_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            EtWaitChainSetTreeStatusMessage(context, FALSE);

            PhReferenceObject(context);
            PhCreateThread2(WaitChainCallbackThread, context);
        }
        break;
    case WM_DESTROY:
        {
            context->WindowHandle = NULL;

            PhSaveWindowPlacementToSetting(SETTING_NAME_WCT_WINDOW_POSITION, SETTING_NAME_WCT_WINDOW_SIZE, hwndDlg);
            PhDeleteLayoutManager(&context->LayoutManager);

            WtcDeleteWaitTree(&context->TreeContext);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhDereferenceObject(context);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
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
                    TreeNew_SetRedraw(context->TreeNewHandle, FALSE);
                    WtcClearWaitTree(&context->TreeContext);
                    TreeNew_SetRedraw(context->TreeNewHandle, TRUE);

                    EtWaitChainSetTreeStatusMessage(context, FALSE);

                    PhReferenceObject(context);
                    PhCreateThread2(WaitChainCallbackThread, context);
                }
                break;
            case ID_WCT_SHOWCONTEXTMENU:
                {
                    POINT point;
                    PPH_EMENU menu;
                    PWCT_ROOT_NODE selectedNode;

                    point.x = GET_X_LPARAM(lParam);
                    point.y = GET_Y_LPARAM(lParam);

                    if (selectedNode = WtcGetSelectedWaitNode(&context->TreeContext))
                    {
                        menu = PhCreateEMenu();
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_WCT_MENU_GOTOPROCESS, L"Go to Process...", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_WCT_MENU_GOTOTHREAD, L"Go to Thread...", NULL, NULL), ULONG_MAX);
                        PhSetFlagsEMenuItem(menu, ID_WCT_MENU_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

                        if (selectedNode->ThreadId > 0)
                        {
                            PhSetFlagsEMenuItem(menu, ID_WCT_MENU_GOTOTHREAD, PH_EMENU_DISABLED, 0);
                            PhSetFlagsEMenuItem(menu, ID_WCT_MENU_GOTOPROCESS, PH_EMENU_DISABLED, 0);
                        }
                        else
                        {
                            PhSetFlagsEMenuItem(menu, ID_WCT_MENU_GOTOTHREAD, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                            PhSetFlagsEMenuItem(menu, ID_WCT_MENU_GOTOPROCESS, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                        }

                        PhShowEMenu(menu, hwndDlg, PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT, PH_ALIGN_LEFT | PH_ALIGN_TOP, point.x, point.y);
                        PhDestroyEMenu(menu);
                    }
                }
                break;
            case ID_WCT_MENU_GOTOPROCESS:
                {
                    PWCT_ROOT_NODE selectedNode;
                    PPH_PROCESS_NODE processNode;

                    if (selectedNode = WtcGetSelectedWaitNode(&context->TreeContext))
                    {
                        if (processNode = PhFindProcessNode(UlongToHandle(selectedNode->ProcessId)))
                        {
                            ProcessHacker_SelectTabPage(0);
                            PhSelectAndEnsureVisibleProcessNode(processNode);
                        }
                    }
                }
                break;
            case ID_WCT_MENU_GOTOTHREAD:
                {
                    PWCT_ROOT_NODE selectedNode;
                    PPH_PROCESS_ITEM processItem;
                    PPH_PROCESS_PROPCONTEXT propContext;

                    if (selectedNode = WtcGetSelectedWaitNode(&context->TreeContext))
                    {
                        if (processItem = PhReferenceProcessItem(UlongToHandle(selectedNode->ProcessId)))
                        {
                            if (propContext = PhCreateProcessPropContext(NULL, processItem))
                            {
                                if (selectedNode->ThreadId)
                                {
                                    PhSetSelectThreadIdProcessPropContext(propContext, UlongToHandle(selectedNode->ThreadId));
                                }

                                PhShowProcessProperties(propContext);
                                PhDereferenceObject(propContext);
                            }

                            PhDereferenceObject(processItem);
                        }
                        else
                        {
                            PhShowError(hwndDlg, L"%s", L"The process does not exist.");
                        }
                    }
                }
                break;
            case ID_WCT_MENU_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                    PhSetClipboardString(hwndDlg, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            }
        }
        break;
    case ID_WCT_UPDATE_RESULTS:
        {
            EtWaitChainSetTreeStatusMessage(context, TRUE);

            TreeNew_SetRedraw(context->TreeNewHandle, FALSE);
            EtWaitChainUpdateResults(context);
            TreeNew_SetRedraw(context->TreeNewHandle, TRUE);
            TreeNew_NodesStructured(context->TreeNewHandle);
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

#define SORT_FUNCTION(Column) WctTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl WctTreeNewCompare##Column( \
    _In_ void* _context, \
    _In_ void const* _elem1, \
    _In_ void const* _elem2 \
    ) \
{ \
    PWCT_TREE_CONTEXT context = _context; \
    PWCT_ROOT_NODE node1 = *(PWCT_ROOT_NODE *)_elem1; \
    PWCT_ROOT_NODE node2 = *(PWCT_ROOT_NODE *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uint64cmp(node1->Node.Index, node2->Node.Index); \
    \
    return PhModifySort(sortResult, context->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Type)
{
    sortResult = PhCompareStringRef(WaitChainObjectTypeToString(node1->ObjectType), WaitChainObjectTypeToString(node2->ObjectType), TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Status)
{
    sortResult = PhCompareStringRef(WaitChainObjectStatusToString(node1->ObjectStatus), WaitChainObjectStatusToString(node2->ObjectStatus), TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->ObjectNameString, node2->ObjectNameString, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Timeout)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->TimeoutString, node2->TimeoutString, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Alertable)
{
    sortResult = ucharcmp(node1->Alertable, node2->Alertable);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ProcessId)
{
    sortResult = uintcmp(node1->ProcessId, node2->ProcessId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ThreadId)
{
    sortResult = uintcmp(node1->ThreadId, node2->ThreadId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(WaitTime)
{
    sortResult = uintcmp(node1->WaitTime, node2->WaitTime);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ContextSwitch)
{
    sortResult = uintcmp(node1->ContextSwitches, node2->ContextSwitches);
}
END_SORT_FUNCTION

BOOLEAN NTAPI WtcWaitTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PWCT_TREE_CONTEXT context = Context;
    PWCT_ROOT_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            node = (PWCT_ROOT_NODE)getChildren->Node;

            if (context->TreeNewSortOrder == NoSortOrder)
            {
                if (!node)
                {
                    getChildren->Children = (PPH_TREENEW_NODE *)context->NodeRootList->Items;
                    getChildren->NumberOfChildren = context->NodeRootList->Count;
                }
                else
                {
                    getChildren->Children = (PPH_TREENEW_NODE *)node->Children->Items;
                    getChildren->NumberOfChildren = node->Children->Count;
                }
            }
            else
            {
                if (!node)
                {
                    static PVOID sortFunctions[] =
                    {
                        SORT_FUNCTION(Type),
                        SORT_FUNCTION(Status),
                        SORT_FUNCTION(Name),
                        SORT_FUNCTION(Timeout),
                        SORT_FUNCTION(Alertable),
                        SORT_FUNCTION(ProcessId),
                        SORT_FUNCTION(ThreadId),
                        SORT_FUNCTION(WaitTime),
                        SORT_FUNCTION(ContextSwitch),
                    };
                    int (__cdecl* sortFunction)(void*, void const*, void const*);

                    if (context->TreeNewSortColumn < TREE_COLUMN_ITEM_MAXIMUM)
                        sortFunction = sortFunctions[context->TreeNewSortColumn];
                    else
                        sortFunction = NULL;

                    if (sortFunction)
                    {
                        qsort_s(context->NodeList->Items, context->NodeList->Count, sizeof(PVOID), sortFunction, context);
                    }

                    getChildren->Children = (PPH_TREENEW_NODE*)context->NodeList->Items;
                    getChildren->NumberOfChildren = context->NodeList->Count;
                }
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = (PPH_TREENEW_IS_LEAF)Parameter1;
            node = (PWCT_ROOT_NODE)isLeaf->Node;

            if (context->TreeNewSortOrder == NoSortOrder)
                isLeaf->IsLeaf = !node->HasChildren;
            else
                isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PWCT_ROOT_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case TREE_COLUMN_ITEM_TYPE:
                {
                    PPH_STRINGREF text;

                    if (text = WaitChainObjectTypeToString(node->ObjectType))
                    {
                        getCellText->Text.Buffer = text->Buffer;
                        getCellText->Text.Length = text->Length;
                    }
                    else
                    {
                        PhInitializeEmptyStringRef(&getCellText->Text);
                    }
                }
                break;
            case TREE_COLUMN_ITEM_STATUS:
                {
                    PPH_STRINGREF text;

                    if (text = WaitChainObjectStatusToString(node->ObjectStatus))
                    {
                        getCellText->Text.Buffer = text->Buffer;
                        getCellText->Text.Length = text->Length;
                    }
                    else
                    {
                        PhInitializeEmptyStringRef(&getCellText->Text);
                    }
                }
                break;
            case TREE_COLUMN_ITEM_NAME:
                getCellText->Text = PhGetStringRef(node->ObjectNameString);
                break;
            case TREE_COLUMN_ITEM_TIMEOUT:
                getCellText->Text = PhGetStringRef(node->TimeoutString);
                break;
            case TREE_COLUMN_ITEM_ALERTABLE:
                {
                    if (node->Alertable)
                    {
                        PhInitializeStringRef(&getCellText->Text, L"true");
                    }
                    else
                    {
                        PhInitializeEmptyStringRef(&getCellText->Text);
                    }
                }
                break;
            case TREE_COLUMN_ITEM_PROCESSID:
                {
                    if (node->ObjectType == WctThreadType)
                    {
                        getCellText->Text = PhGetStringRef(node->ProcessIdString);
                    }
                }
                break;
            case TREE_COLUMN_ITEM_THREADID:
                {
                    if (node->ObjectType == WctThreadType)
                    {
                        getCellText->Text = PhGetStringRef(node->ThreadIdString);
                    }
                }
                break;
            case TREE_COLUMN_ITEM_WAITTIME:
                {
                    if (node->ObjectType == WctThreadType)
                    {
                        getCellText->Text = PhGetStringRef(node->WaitTimeString);
                    }
                }
                break;
            case TREE_COLUMN_ITEM_CONTEXTSWITCH:
                {
                    if (node->ObjectType == WctThreadType)
                    {
                        getCellText->Text = PhGetStringRef(node->ContextSwitchesString);
                    }
                }
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
            node = (PWCT_ROOT_NODE)getNodeColor->Node;

            if (node->IsDeadLocked)
            {
                getNodeColor->ForeColor = RGB(255, 0, 0);
            }

            getNodeColor->Flags = TN_CACHE;
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
    case TreeNewLeftDoubleClick:
    case TreeNewNodeExpanding:
        return TRUE;
    case TreeNewContextMenu:
        {
            //PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;
            //SendMessage(context->ParentWindowHandle, WM_COMMAND, WM_CONTEXTMENU, MAKELONG(contextMenuEvent->Location.x, contextMenuEvent->Location.y));
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = TREE_COLUMN_ITEM_TYPE;
            data.DefaultSortOrder = NoSortOrder;
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

static BOOLEAN WtcWaitNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PWCT_ROOT_NODE node1 = *(PWCT_ROOT_NODE*)Entry1;
    PWCT_ROOT_NODE node2 = *(PWCT_ROOT_NODE*)Entry2;

    return node1->Index == node2->Index;
}

static ULONG WtcWaitNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashInt64((*(PWCT_ROOT_NODE*)Entry)->Index);
}

VOID WtcInitializeWaitTree(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PWCT_TREE_CONTEXT Context
    )
{
    HWND hwnd;
    PPH_STRING settings;

    memset(Context, 0, sizeof(WCT_TREE_CONTEXT));

    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PWCT_ROOT_NODE),
        WtcWaitNodeHashtableEqualFunction,
        WtcWaitNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(30);
    Context->NodeRootList = PhCreateList(30);

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeNewHandle = TreeNewHandle;
    hwnd = TreeNewHandle;
    PhSetControlTheme(hwnd, L"explorer");

    TreeNew_SetCallback(hwnd, WtcWaitTreeNewCallback, Context);
    TreeNew_SetRedraw(hwnd, FALSE);

    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_TYPE, TRUE, L"Type", 80, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_THREADID, TRUE, L"ThreadId", 50, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_PROCESSID, TRUE, L"ProcessId", 50, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_STATUS, TRUE, L"Status", 80, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_CONTEXTSWITCH, TRUE, L"Context Switches", 70, PH_ALIGN_LEFT, 4, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_WAITTIME, TRUE, L"WaitTime", 60, PH_ALIGN_LEFT, 5, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_TIMEOUT, TRUE, L"Timeout", 60, PH_ALIGN_LEFT, 6, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_ALERTABLE, TRUE, L"Alertable", 50, PH_ALIGN_LEFT, 7, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_NAME, TRUE, L"Name", 100, PH_ALIGN_LEFT, 8, 0);

    TreeNew_SetRedraw(hwnd, TRUE);
    TreeNew_SetTriState(hwnd, TRUE);
    TreeNew_SetSort(hwnd, TREE_COLUMN_ITEM_TYPE, NoSortOrder);

    settings = PhGetStringSetting(SETTING_NAME_WCT_TREE_LIST_COLUMNS);
    PhCmLoadSettings(hwnd, &settings->sr);
    PhDereferenceObject(settings);
}

VOID WtcDestroyWaitNode(
    _In_ PWCT_ROOT_NODE Node
    )
{
    PhDereferenceObject(Node->Children);

    if (Node->TimeoutString)
        PhDereferenceObject(Node->TimeoutString);
    if (Node->ProcessIdString)
        PhDereferenceObject(Node->ProcessIdString);
    if (Node->ThreadIdString)
        PhDereferenceObject(Node->ThreadIdString);
    if (Node->WaitTimeString)
        PhDereferenceObject(Node->WaitTimeString);
    if (Node->ContextSwitchesString)
        PhDereferenceObject(Node->ContextSwitchesString);
    if (Node->ObjectNameString)
        PhDereferenceObject(Node->ObjectNameString);

    PhFree(Node);
}

VOID WtcDeleteWaitTree(
    _In_ PWCT_TREE_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(SETTING_NAME_WCT_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        WtcDestroyWaitNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
    PhDereferenceObject(Context->NodeRootList);
}

PWCT_ROOT_NODE WtcAddWaitNode(
    _Inout_ PWCT_TREE_CONTEXT Context
    )
{
    static ULONG64 index = 0;
    PWCT_ROOT_NODE node;

    node = PhAllocateZero(sizeof(WCT_ROOT_NODE));
    PhInitializeTreeNewNode(&node->Node);
    node->Index = ++index;

    memset(node->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);
    node->Node.TextCache = node->TextCache;
    node->Node.TextCacheSize = TREE_COLUMN_ITEM_MAXIMUM;
    node->Children = PhCreateList(1);

    PhAddEntryHashtable(Context->NodeHashtable, &node);
    PhAddItemList(Context->NodeList, node);

    //TreeNew_NodesStructured(Context->TreeNewHandle);

    return node;
}

VOID WctAddChildWaitNode(
    _In_ PWCT_TREE_CONTEXT Context,
    _In_opt_ PWCT_ROOT_NODE ParentNode,
    _In_ PWAITCHAIN_NODE_INFO WctNode,
    _In_ BOOLEAN IsDeadLocked
    )
{
    PWCT_ROOT_NODE childNode;

    childNode = WtcAddWaitNode(Context);
    childNode->IsDeadLocked = IsDeadLocked;
    childNode->ObjectType = WctNode->ObjectType;
    childNode->ObjectStatus = WctNode->ObjectStatus;
    childNode->Alertable = !!WctNode->LockObject.Alertable;
    childNode->ThreadId = WctNode->ThreadObject.ThreadId;
    childNode->ProcessId = WctNode->ThreadObject.ProcessId;
    childNode->WaitTime = WctNode->ThreadObject.WaitTime;
    childNode->ContextSwitches = WctNode->ThreadObject.ContextSwitches;
    childNode->Timeout = WctNode->LockObject.Timeout;
    childNode->ProcessIdString = PhFormatUInt64(childNode->ProcessId, TRUE);
    childNode->ThreadIdString = PhFormatUInt64(childNode->ThreadId, TRUE);
    childNode->WaitTimeString = PhFormatUInt64(childNode->WaitTime, TRUE);
    childNode->ContextSwitchesString = PhFormatUInt64(childNode->ContextSwitches, TRUE);

    if (WctNode->LockObject.ObjectName[0] != UNICODE_NULL)
    {
        childNode->ObjectNameString = PhCreateString(WctNode->LockObject.ObjectName);
    }

    if (WctNode->LockObject.Timeout.QuadPart > 0)
    {
        SYSTEMTIME systemTime;
        PPH_STRING dateString;
        PPH_STRING timeString;

        PhLargeIntegerToLocalSystemTime(&systemTime, &WctNode->LockObject.Timeout);
        dateString = PhFormatDate(&systemTime, NULL);
        timeString = PhFormatTime(&systemTime, NULL);

        childNode->TimeoutString = PhFormatString(
            L"%s %s",
            dateString->Buffer,
            timeString->Buffer
            );

        PhDereferenceObject(dateString);
        PhDereferenceObject(timeString);
    }

    if (ParentNode)
    {
        childNode->HasChildren = FALSE;

        // This is a child node.
        childNode->Parent = ParentNode;
        PhAddItemList(ParentNode->Children, childNode);
    }
    else
    {
        childNode->HasChildren = TRUE;
        childNode->Node.Expanded = TRUE;

        // This is a root node.
        PhAddItemList(Context->NodeRootList, childNode);
    }
}

VOID EtWaitChainSetTreeStatusMessage(
    _Inout_ PWCT_CONTEXT Context,
    _In_ BOOLEAN ShowEmpty
    )
{
    if (ShowEmpty)
    {
        PhMoveReference(&Context->StatusMessage, PhCreateString(L"There are no threads to display."));
        TreeNew_SetEmptyText(Context->TreeNewHandle, &Context->StatusMessage->sr, 0);
    }
    else
    {
        PhMoveReference(&Context->StatusMessage, PhCreateString(L"Querying thread wait chain sessions..."));
        TreeNew_SetEmptyText(Context->TreeNewHandle, &Context->StatusMessage->sr, 0);
    }
}

VOID EtWaitChainUpdateResults(
    _Inout_ PWCT_CONTEXT Context
    )
{
    PWCT_ROOT_NODE rootNode = NULL;

    for (ULONG i = 0; i < Context->WaitChainList->Count; i++)
    {
        PWAITCHAIN_NODE_INFO wctNode = Context->WaitChainList->Items[i];

        if (wctNode->ObjectType == WctThreadType)
        {
            rootNode = WtcAddWaitNode(&Context->TreeContext);
            rootNode->ObjectType = wctNode->ObjectType;
            rootNode->ObjectStatus = wctNode->ObjectStatus;
            rootNode->Alertable = !!wctNode->LockObject.Alertable;
            rootNode->ThreadId = wctNode->ThreadObject.ThreadId;
            rootNode->ProcessId = wctNode->ThreadObject.ProcessId;
            rootNode->WaitTime = wctNode->ThreadObject.WaitTime;
            rootNode->ContextSwitches = wctNode->ThreadObject.ContextSwitches;
            rootNode->Timeout = wctNode->LockObject.Timeout;

            rootNode->ThreadIdString = PhFormatUInt64(rootNode->ThreadId, TRUE);
            rootNode->ProcessIdString = PhFormatUInt64(rootNode->ProcessId, TRUE);
            rootNode->WaitTimeString = PhFormatUInt64(rootNode->WaitTime, TRUE);
            rootNode->ContextSwitchesString = PhFormatUInt64(rootNode->ContextSwitches, TRUE);

            if (rootNode->Timeout.QuadPart > 0)
            {
                SYSTEMTIME systemTime;
                PPH_STRING dateString;
                PPH_STRING timeString;

                PhLargeIntegerToLocalSystemTime(&systemTime, &rootNode->Timeout);
                dateString = PhFormatDate(&systemTime, NULL);
                timeString = PhFormatTime(&systemTime, NULL);

                rootNode->TimeoutString = PhFormatString(
                    L"%s %s",
                    dateString->Buffer,
                    timeString->Buffer
                    );

                PhDereferenceObject(dateString);
                PhDereferenceObject(timeString);
            }

            if (wctNode->LockObject.ObjectName[0] != UNICODE_NULL)
            {
                if (PhIsDigitCharacter(wctNode->LockObject.ObjectName[0]))
                {
                    rootNode->ObjectNameString = PhCreateString(wctNode->LockObject.ObjectName);
                }
            }

            rootNode->Node.Expanded = TRUE;
            rootNode->HasChildren = TRUE;

            // This is a root node.
            PhAddItemList(Context->TreeContext.NodeRootList, rootNode);
        }
        else
        {
            WctAddChildWaitNode(&Context->TreeContext, rootNode, wctNode, !!PtrToUlong(Context->DeadLockList->Items[i]));
        }

        PhFree(wctNode);
    }

    PhClearList(Context->WaitChainList);
    PhClearList(Context->DeadLockList);
}

VOID WtcClearWaitTree(
    _In_ PWCT_TREE_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        WtcDestroyWaitNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
    PhClearList(Context->NodeRootList);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

PWCT_ROOT_NODE WtcGetSelectedWaitNode(
    _In_ PWCT_TREE_CONTEXT Context
    )
{
    PWCT_ROOT_NODE node;
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        node = Context->NodeList->Items[i];

        if (node->Node.Selected)
            return node;
    }

    return NULL;
}

VOID EtShowWaitChainProcessDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PWCT_CONTEXT context;

    context = EtWaitChainContextCreate();
    context->ParentWindowHandle = ParentWindowHandle;
    context->ProcessItem = PhReferenceObject(ProcessItem);

    PhDialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_WCT_DIALOG),
        NULL,
        WaitChainDlgProc,
        context
        );
}

VOID EtShowWaitChainThreadDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_THREAD_ITEM ThreadItem
    )
{
    PWCT_CONTEXT context;

    context = EtWaitChainContextCreate();
    context->ParentWindowHandle = ParentWindowHandle;
    context->ThreadItem = PhReferenceObject(ThreadItem);

    PhDialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_WCT_DIALOG),
        NULL,
        WaitChainDlgProc,
        context
        );
}
