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
#include "wndtree.h"

// Wait Chain Traversal Documentation:
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms681622.aspx

#define WCT_GETINFO_ALL_FLAGS (WCT_OUT_OF_PROC_FLAG|WCT_OUT_OF_PROC_COM_FLAG|WCT_OUT_OF_PROC_CS_FLAG|WCT_NETWORK_IO_FLAG)

typedef struct _WCT_CONTEXT
{
    HWND DialogHandle;
    HWND TreeNewHandle;

    HWND HighlightingWindow;
    ULONG HighlightingWindowCount;
    WCT_TREE_CONTEXT TreeContext;
    PH_LAYOUT_MANAGER LayoutManager;

    BOOLEAN IsProcessItem;
    PPH_THREAD_ITEM ThreadItem;
    PPH_PROCESS_ITEM ProcessItem;

    HWCT WctSessionHandle;
    HMODULE Ole32ModuleHandle;
} WCT_CONTEXT, *PWCT_CONTEXT;

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
            rootNode = WeAddWindowNode(&Context->TreeContext);
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

            if (wctNode->LockObject.ObjectName[0] != '\0')
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
            WctAddChildWindowNode(&Context->TreeContext, rootNode, wctNode, isDeadLocked);
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
        return NTSTATUS_FROM_WIN32(GetLastError());

    // Synchronous WCT session
    if (!(context->WctSessionHandle = OpenThreadWaitChainSession(0, NULL)))
        return NTSTATUS_FROM_WIN32(GetLastError());

    //TreeNew_SetRedraw(context->TreeNewHandle, FALSE);

    if (context->IsProcessItem)
    {
        NTSTATUS status;
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

            WtcInitializeWindowTree(hwndDlg, context->TreeNewHandle, &context->TreeContext);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhLoadWindowPlacementFromSetting(SETTING_NAME_WCT_WINDOW_POSITION, SETTING_NAME_WCT_WINDOW_SIZE, hwndDlg);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            PhCreateThread2(WaitChainCallbackThread, context);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveWindowPlacementToSetting(SETTING_NAME_WCT_WINDOW_POSITION, SETTING_NAME_WCT_WINDOW_SIZE, hwndDlg);
            PhDeleteLayoutManager(&context->LayoutManager);

            WtcDeleteWindowTree(&context->TreeContext);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
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
            case ID_WCT_SHOWCONTEXTMENU:
                {
                    POINT point;
                    PPH_EMENU menu;
                    PWCT_ROOT_NODE selectedNode;

                    point.x = (SHORT)LOWORD(lParam);
                    point.y = (SHORT)HIWORD(lParam);

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        menu = PhCreateEMenu();
                        PhLoadResourceEMenuItem(menu, PluginInstance->DllBase, MAKEINTRESOURCE(IDR_WCT_MAIN_MENU), 0);
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

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
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

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
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
    }

    return FALSE;
}

VOID NTAPI WctProcessMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ULONG insertIndex = 0;
    PWCT_CONTEXT context = NULL;
    PPH_PLUGIN_MENU_INFORMATION menuInfo = NULL;
    PPH_PROCESS_ITEM processItem = NULL;
    PPH_EMENU_ITEM menuItem = NULL;
    PPH_EMENU_ITEM miscMenuItem = NULL;
    PPH_EMENU_ITEM wsMenuItem = NULL;

    menuInfo = (PPH_PLUGIN_MENU_INFORMATION)Parameter;

    if (menuInfo->u.Process.NumberOfProcesses == 1)
        processItem = menuInfo->u.Process.Processes[0];
    else
    {
        processItem = NULL;
    }

    if (processItem == NULL)
        return;

    context = PhAllocateZero(sizeof(WCT_CONTEXT));
    context->IsProcessItem = TRUE;
    context->ProcessItem = processItem;

    if (miscMenuItem = PhFindEMenuItem(menuInfo->Menu, 0, L"Miscellaneous", 0))
    {
        PhInsertEMenuItem(miscMenuItem, menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_WCT_MENUITEM, L"Wait Chain Tra&versal", context), -1);

        if (!processItem || !processItem->QueryHandle || processItem->ProcessId == NtCurrentProcessId())
            menuItem->Flags |= PH_EMENU_DISABLED;

        if (!PhGetOwnTokenAttributes().Elevated)
            menuItem->Flags |= PH_EMENU_DISABLED;
    }
    else
    {
        PhFree(context);
    }
}

VOID NTAPI WctThreadMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PWCT_CONTEXT context = NULL;
    PPH_PLUGIN_MENU_INFORMATION menuInfo = NULL;
    PPH_THREAD_ITEM threadItem = NULL;
    PPH_EMENU_ITEM menuItem = NULL;
    PPH_EMENU_ITEM miscMenuItem = NULL;
    ULONG indexOfMenuItem;

    menuInfo = (PPH_PLUGIN_MENU_INFORMATION)Parameter;

    if (menuInfo->u.Thread.NumberOfThreads == 1)
        threadItem = menuInfo->u.Thread.Threads[0];
    else
        threadItem = NULL;

    context = PhAllocateZero(sizeof(WCT_CONTEXT));
    context->IsProcessItem = FALSE;
    context->ThreadItem = threadItem;

    if (miscMenuItem = PhFindEMenuItem(menuInfo->Menu, 0, L"Analyze", 0))
        indexOfMenuItem = PhIndexOfEMenuItem(menuInfo->Menu, miscMenuItem) + 1;
    else
        indexOfMenuItem = ULONG_MAX;

    menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_WCT_MENUITEM, L"Wait Chain Tra&versal", context);
    PhInsertEMenuItem(menuInfo->Menu, menuItem, indexOfMenuItem);

    // Disable menu if current process selected.
    if (threadItem == NULL || menuInfo->u.Thread.ProcessId == NtCurrentProcessId())
        menuItem->Flags |= PH_EMENU_DISABLED;
}
