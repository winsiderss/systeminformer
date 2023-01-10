/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2013-2022
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

    HANDLE ProcessId;
    HANDLE ThreadId;

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

    BOOLEAN IsProcessItem;
    PPH_THREAD_ITEM ThreadItem;
    PPH_PROCESS_ITEM ProcessItem;

    HWCT WctSessionHandle;
    PVOID Ole32ModuleHandle;
} WCT_CONTEXT, *PWCT_CONTEXT;

#define ID_WCT_MENU_GOTOTHREAD (WM_APP + 1)
#define ID_WCT_MENU_GOTOPROCESS (WM_APP + 2)

VOID WctAddChildWaitNode(
    _In_ PWCT_TREE_CONTEXT Context,
    _In_opt_ PWCT_ROOT_NODE ParentNode,
    _In_ PWAITCHAIN_NODE_INFO WctNode,
    _In_ BOOLEAN IsDeadLocked
    );

PWCT_ROOT_NODE WtcAddWaitNode(
    _Inout_ PWCT_TREE_CONTEXT Context
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

BOOLEAN WaitChainRegisterCallbacks(
    _Inout_ PWCT_CONTEXT Context
    )
{
    PCOGETCALLSTATE coGetCallStateCallback = NULL;
    PCOGETACTIVATIONSTATE coGetActivationStateCallback = NULL;

    if (!(Context->Ole32ModuleHandle = PhLoadLibrary(L"ole32.dll")))
        return FALSE;

    if (!(coGetCallStateCallback = PhGetProcedureAddress(Context->Ole32ModuleHandle, "CoGetCallState", 0)))
        return FALSE;

    if (!(coGetActivationStateCallback = PhGetProcedureAddress(Context->Ole32ModuleHandle, "CoGetActivationState", 0)))
        return FALSE;

    RegisterWaitChainCOMCallback(coGetCallStateCallback, coGetActivationStateCallback);
    return TRUE;
}

VOID WaitChainCheckThread(
    _Inout_ PWCT_CONTEXT Context,
    _In_ HANDLE ThreadId
    )
{
    BOOL isDeadLocked = FALSE;
    ULONG nodeInfoLength = WCT_MAX_NODE_COUNT;
    WAITCHAIN_NODE_INFO nodeInfoArray[WCT_MAX_NODE_COUNT];
    PWCT_ROOT_NODE rootNode = NULL;

    memset(nodeInfoArray, 0, sizeof(nodeInfoArray));

    if (!GetThreadWaitChain(
        Context->WctSessionHandle,
        0,
        WCT_GETINFO_ALL_FLAGS,
        HandleToUlong(ThreadId),
        &nodeInfoLength,
        nodeInfoArray,
        &isDeadLocked
        ))
    {
        return;
    }

    if (nodeInfoLength > WCT_MAX_NODE_COUNT)
        nodeInfoLength = WCT_MAX_NODE_COUNT;

    for (ULONG i = 0; i < nodeInfoLength; i++)
    {
        PWAITCHAIN_NODE_INFO wctNode = &nodeInfoArray[i];

        if (wctNode->ObjectType == WctThreadType)
        {
            rootNode = WtcAddWaitNode(&Context->TreeContext);
            rootNode->ObjectType = wctNode->ObjectType;
            rootNode->ObjectStatus = wctNode->ObjectStatus;
            rootNode->Alertable = wctNode->LockObject.Alertable;
            rootNode->ThreadId = UlongToHandle(wctNode->ThreadObject.ThreadId);
            rootNode->ProcessId = UlongToHandle(wctNode->ThreadObject.ProcessId);
            rootNode->ThreadIdString = PhFormatUInt64(wctNode->ThreadObject.ThreadId, TRUE);
            rootNode->ProcessIdString = PhFormatUInt64(wctNode->ThreadObject.ProcessId, TRUE);
            rootNode->WaitTimeString = PhFormatUInt64(wctNode->ThreadObject.WaitTime, TRUE);
            rootNode->ContextSwitchesString = PhFormatUInt64(wctNode->ThreadObject.ContextSwitches, TRUE);
            rootNode->TimeoutString = PhFormatUInt64(wctNode->LockObject.Timeout.QuadPart, TRUE);

            if (wctNode->LockObject.ObjectName[0] != UNICODE_NULL)
            {
                // -- ProcessID --
                //wctNode->LockObject.ObjectName[0]
                // -- ThreadID --
                //wctNode->LockObject.ObjectName[2]
                // -- Unknown --
                //wctNode->LockObject.ObjectName[4]
                // -- ContextSwitches --
                //wctNode->LockObject.ObjectName[6]

                if (PhIsDigitCharacter(wctNode->LockObject.ObjectName[0]))
                {
                    rootNode->ObjectNameString = PhFormatString(L"%s", wctNode->LockObject.ObjectName);
                }
                //else
                //{
                //    rootNode->ObjectNameString = PhFormatString(L"[%lu, %lu]",
                //        wctNode.LockObject.ObjectName[0],
                //        wctNode.LockObject.ObjectName[2]
                //        );
                //}
            }

            rootNode->Node.Expanded = TRUE;
            rootNode->HasChildren = TRUE;

            // This is a root node.
            PhAddItemList(Context->TreeContext.NodeRootList, rootNode);
        }
        else
        {
            WctAddChildWaitNode(&Context->TreeContext, rootNode, wctNode, isDeadLocked);
        }
    }
}

NTSTATUS WaitChainCallbackThread(
    _In_ PVOID Parameter
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWCT_CONTEXT context = (PWCT_CONTEXT)Parameter;

    if (!WaitChainRegisterCallbacks(context))
        return PhDosErrorToNtStatus(GetLastError());

    // Synchronous WCT session
    if (!(context->WctSessionHandle = OpenThreadWaitChainSession(0, NULL)))
        return PhDosErrorToNtStatus(GetLastError());

    //TreeNew_SetRedraw(context->TreeNewHandle, FALSE);

    if (context->IsProcessItem)
    {
        HANDLE threadHandle;
        HANDLE newThreadHandle;
        THREAD_BASIC_INFORMATION basicInfo;

        status = NtGetNextThread(
            context->ProcessItem->QueryHandle,
            NULL,
            THREAD_QUERY_LIMITED_INFORMATION,
            0,
            0,
            &threadHandle
            );

        while (NT_SUCCESS(status))
        {
            if (NT_SUCCESS(PhGetThreadBasicInformation(threadHandle, &basicInfo)))
            {
                WaitChainCheckThread(context, basicInfo.ClientId.UniqueThread);
            }

            status = NtGetNextThread(
                context->ProcessItem->QueryHandle,
                threadHandle,
                THREAD_QUERY_LIMITED_INFORMATION,
                0,
                0,
                &newThreadHandle
                );

            NtClose(threadHandle);
            threadHandle = newThreadHandle;
        }
    }
    else
    {
        WaitChainCheckThread(context, context->ThreadItem->ThreadId);
    }

    if (context->WctSessionHandle)
    {
        CloseThreadWaitChainSession(context->WctSessionHandle);
    }

    if (context->Ole32ModuleHandle)
    {
        FreeLibrary(context->Ole32ModuleHandle);
    }

    //TreeNew_SetRedraw(context->TreeNewHandle, TRUE);
    TreeNew_NodesStructured(context->TreeNewHandle);

    return status;
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
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_WCT_TREE);

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

            PhCreateThread2(WaitChainCallbackThread, context);
        }
        break;
    case WM_DESTROY:
        {
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
                    PWCT_ROOT_NODE selectedNode = NULL;
                    PPH_PROCESS_NODE processNode = NULL;

                    if (selectedNode = WtcGetSelectedWaitNode(&context->TreeContext))
                    {
                        if (processNode = PhFindProcessNode(selectedNode->ProcessId))
                        {
                            ProcessHacker_SelectTabPage(0);
                            PhSelectAndEnsureVisibleProcessNode(processNode);
                        }
                    }
                }
                break;
            case ID_WCT_MENU_GOTOTHREAD:
                {
                    PWCT_ROOT_NODE selectedNode = NULL;
                    PPH_PROCESS_ITEM processItem = NULL;
                    PPH_PROCESS_PROPCONTEXT propContext = NULL;

                    if (selectedNode = WtcGetSelectedWaitNode(&context->TreeContext))
                    {
                        if (processItem = PhReferenceProcessItem(selectedNode->ProcessId))
                        {
                            if (propContext = PhCreateProcessPropContext(NULL, processItem))
                            {
                                if (selectedNode->ThreadId)
                                {
                                    PhSetSelectThreadIdProcessPropContext(propContext, selectedNode->ThreadId);
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
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

static VOID NTAPI EtWctMenuItemDeleteFunction(
    _In_ PPH_PLUGIN_MENU_ITEM Item
    )
{
    PWCT_CONTEXT context = Item->Context;

    PhDereferenceObject(context);
}

VOID NTAPI WctProcessMenuInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = (PPH_PLUGIN_MENU_INFORMATION)Parameter;
    ULONG insertIndex = 0;
    PWCT_CONTEXT context = NULL;
    PPH_PROCESS_ITEM processItem = NULL;
    PPH_EMENU_ITEM menuItem = NULL;
    PPH_EMENU_ITEM miscMenuItem = NULL;
    PPH_EMENU_ITEM wsMenuItem = NULL;

    if (menuInfo->u.Process.NumberOfProcesses == 1)
        processItem = menuInfo->u.Process.Processes[0];
    else
        processItem = NULL;

    context = EtWaitChainContextCreate();
    context->IsProcessItem = TRUE;
    context->ProcessItem = processItem ? PhReferenceObject(processItem) : NULL;

    if (miscMenuItem = PhFindEMenuItem(menuInfo->Menu, 0, L"Miscellaneous", 0))
    {
        menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_WCT_MENUITEM, L"Wait Chain Tra&versal", context);
        ((PPH_PLUGIN_MENU_ITEM)menuItem->Context)->DeleteFunction = EtWctMenuItemDeleteFunction;
        PhInsertEMenuItem(miscMenuItem, menuItem, ULONG_MAX);

        if (!processItem || !processItem->QueryHandle || processItem->ProcessId == NtCurrentProcessId())
            menuItem->Flags |= PH_EMENU_DISABLED;

        if (!PhGetOwnTokenAttributes().Elevated)
            menuItem->Flags |= PH_EMENU_DISABLED;
    }
    else
    {
        PhDereferenceObject(context);
    }
}

VOID NTAPI WctThreadMenuInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = (PPH_PLUGIN_MENU_INFORMATION)Parameter;
    PWCT_CONTEXT context = NULL;
    PPH_THREAD_ITEM threadItem = NULL;
    PPH_EMENU_ITEM menuItem = NULL;
    PPH_EMENU_ITEM miscMenuItem = NULL;
    ULONG indexOfMenuItem;

    if (menuInfo->u.Thread.NumberOfThreads == 1)
        threadItem = menuInfo->u.Thread.Threads[0];
    else
        threadItem = NULL;

    context = EtWaitChainContextCreate();
    context->IsProcessItem = FALSE;
    context->ThreadItem = threadItem ? PhReferenceObject(threadItem) : NULL;

    if (miscMenuItem = PhFindEMenuItem(menuInfo->Menu, 0, L"Analyze", 0))
        indexOfMenuItem = PhIndexOfEMenuItem(menuInfo->Menu, miscMenuItem) + 1;
    else
        indexOfMenuItem = ULONG_MAX;

    menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_WCT_MENUITEM, L"Wait Chain Tra&versal", context);
    ((PPH_PLUGIN_MENU_ITEM)menuItem->Context)->DeleteFunction = EtWctMenuItemDeleteFunction;
    PhInsertEMenuItem(menuInfo->Menu, menuItem, indexOfMenuItem);

    // Disable menu if current process selected.
    if (threadItem == NULL || menuInfo->u.Thread.ProcessId == NtCurrentProcessId())
        menuItem->Flags |= PH_EMENU_DISABLED;
}

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
                    getChildren->Children = (PPH_TREENEW_NODE *)context->NodeList->Items;
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
                    switch (node->ObjectType)
                    {
                    case WctCriticalSectionType:
                        PhInitializeStringRef(&getCellText->Text, L"CriticalSection");
                        break;
                    case WctSendMessageType:
                        PhInitializeStringRef(&getCellText->Text, L"SendMessage");
                        break;
                    case WctMutexType:
                        PhInitializeStringRef(&getCellText->Text, L"Mutex");
                        break;
                    case WctAlpcType:
                        PhInitializeStringRef(&getCellText->Text, L"Alpc");
                        break;
                    case WctComType:
                        PhInitializeStringRef(&getCellText->Text, L"Com");
                        break;
                    case WctComActivationType:
                        PhInitializeStringRef(&getCellText->Text, L"ComActivation");
                        break;
                    case WctProcessWaitType:
                        PhInitializeStringRef(&getCellText->Text, L"ProcWait");
                        break;
                    case WctThreadType:
                        PhInitializeStringRef(&getCellText->Text, L"Thread");
                        break;
                    case WctThreadWaitType:
                        PhInitializeStringRef(&getCellText->Text, L"ThreadWait");
                        break;
                    case WctSocketIoType:
                        PhInitializeStringRef(&getCellText->Text, L"Socket I/O");
                        break;
                    case WctSmbIoType:
                        PhInitializeStringRef(&getCellText->Text, L"SMB I/O");
                        break;
                    case WctUnknownType:
                    case WctMaxType:
                    default:
                        PhInitializeStringRef(&getCellText->Text, L"Unknown");
                        break;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_STATUS:
                {
                    switch (node->ObjectStatus)
                    {
                    case WctStatusNoAccess:
                        PhInitializeStringRef(&getCellText->Text, L"No Access");
                        break;
                    case WctStatusRunning:
                        PhInitializeStringRef(&getCellText->Text, L"Running");
                        break;
                    case WctStatusBlocked:
                        PhInitializeStringRef(&getCellText->Text, L"Blocked");
                        break;
                    case WctStatusPidOnly:
                        PhInitializeStringRef(&getCellText->Text, L"Pid Only");
                        break;
                    case WctStatusPidOnlyRpcss:
                        PhInitializeStringRef(&getCellText->Text, L"Pid Only (Rpcss)");
                        break;
                    case WctStatusOwned:
                        PhInitializeStringRef(&getCellText->Text, L"Owned");
                        break;
                    case WctStatusNotOwned:
                        PhInitializeStringRef(&getCellText->Text, L"Not Owned");
                        break;
                    case WctStatusAbandoned:
                        PhInitializeStringRef(&getCellText->Text, L"Abandoned");
                        break;
                    case WctStatusError:
                        PhInitializeStringRef(&getCellText->Text, L"Error");
                        break;
                    case WctStatusUnknown:
                    case WctStatusMax:
                    default:
                        PhInitializeStringRef(&getCellText->Text, L"Unknown");
                        break;
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
                        PhInitializeStringRef(&getCellText->Text, L"false");
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

    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_TYPE, TRUE, L"Type", 80, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_THREADID, TRUE, L"ThreadId", 50, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_PROCESSID, TRUE, L"ProcessId", 50, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_STATUS, TRUE, L"Status", 80, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_CONTEXTSWITCH, TRUE, L"Context Switches", 70, PH_ALIGN_LEFT, 4, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_WAITTIME, TRUE, L"WaitTime", 60, PH_ALIGN_LEFT, 5, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_TIMEOUT, TRUE, L"Timeout", 60, PH_ALIGN_LEFT, 6, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_ALERTABLE, TRUE, L"Alertable", 50, PH_ALIGN_LEFT, 7, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_NAME, TRUE, L"Name", 100, PH_ALIGN_LEFT, 8, 0);

    TreeNew_SetTriState(hwnd, TRUE);
    TreeNew_SetSort(hwnd, 0, NoSortOrder);

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
    PWCT_ROOT_NODE childNode = NULL;

    childNode = WtcAddWaitNode(Context);
    childNode->IsDeadLocked = TRUE;
    childNode->ObjectType = WctNode->ObjectType;
    childNode->ObjectStatus = WctNode->ObjectStatus;
    childNode->Alertable = WctNode->LockObject.Alertable;
    childNode->ThreadId = UlongToHandle(WctNode->ThreadObject.ThreadId);
    childNode->ProcessIdString = PhFormatUInt64(WctNode->ThreadObject.ProcessId, TRUE);
    childNode->ThreadIdString = PhFormatUInt64(WctNode->ThreadObject.ThreadId, TRUE);
    childNode->WaitTimeString = PhFormatUInt64(WctNode->ThreadObject.WaitTime, TRUE);
    childNode->ContextSwitchesString = PhFormatUInt64(WctNode->ThreadObject.ContextSwitches, TRUE);

    if (WctNode->LockObject.ObjectName[0] != UNICODE_NULL)
        childNode->ObjectNameString = PhFormatString(L"%s", WctNode->LockObject.ObjectName);

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
    PWCT_ROOT_NODE node = NULL;
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        node = Context->NodeList->Items[i];

        if (node->Node.Selected)
            return node;
    }

    return NULL;
}

VOID EtShowWaitChainDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PVOID Context
    )
{
    PhReferenceObject(Context);

    DialogBoxParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_WCT_DIALOG),
        NULL,
        WaitChainDlgProc,
        (LPARAM)Context
        );
}
