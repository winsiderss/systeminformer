/*
 * Process Hacker Extra Plugins -
 *   Wait Chain Traversal (WCT) Plugin
 *
 * Copyright (C) 2013-2015 dmex
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

// Wait Chain Traversal Documentation:
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms681622.aspx

static PPH_PLUGIN PluginInstance;
static PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessMenuInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION ThreadMenuInitializingCallbackRegistration;

static BOOLEAN WaitChainRegisterCallbacks(
    _Inout_ PWCT_CONTEXT Context
    )
{
    PCOGETCALLSTATE coGetCallStateCallback = NULL;
    PCOGETACTIVATIONSTATE coGetActivationStateCallback = NULL;

    Context->Ole32ModuleHandle = LoadLibrary(L"ole32.dll");
    if (!Context->Ole32ModuleHandle)
        return FALSE;

    coGetCallStateCallback = PhGetProcedureAddress(Context->Ole32ModuleHandle, "CoGetCallState", 0);
    if (!coGetCallStateCallback)
        return FALSE;

    coGetActivationStateCallback = PhGetProcedureAddress(Context->Ole32ModuleHandle, "CoGetActivationState", 0);
    if (!coGetActivationStateCallback)
        return FALSE;

    RegisterWaitChainCOMCallback(coGetCallStateCallback, coGetActivationStateCallback);
    return TRUE;
}

static VOID WaitChainCheckThread(
    _Inout_ PWCT_CONTEXT Context,
    _In_ HANDLE ThreadId
    )
{
    BOOL isDeadLocked = FALSE;
    ULONG nodeInfoLength = WCT_MAX_NODE_COUNT;
    WAITCHAIN_NODE_INFO nodeInfoArray[WCT_MAX_NODE_COUNT];
    PWCT_ROOT_NODE rootNode = NULL;

    memset(nodeInfoArray, 0, sizeof(nodeInfoArray));

    // Retrieve the thread wait chain.
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

    // Check if the wait chain is too big for the array we passed in.
    if (nodeInfoLength > WCT_MAX_NODE_COUNT)
        nodeInfoLength = WCT_MAX_NODE_COUNT;

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);
    TreeNew_NodesStructured(Context->TreeNewHandle);

    for (ULONG i = 0; i < nodeInfoLength; i++)
    {
        PWAITCHAIN_NODE_INFO wctNode = &nodeInfoArray[i];

        if (wctNode->ObjectType == WctThreadType)
        {
            rootNode = WeAddWindowNode(&Context->TreeContext);

            rootNode->WctInfo = *wctNode;
            rootNode->ThreadId = UlongToHandle(wctNode->ThreadObject.ThreadId);
            rootNode->ProcessId = UlongToHandle(wctNode->ThreadObject.ProcessId);
            rootNode->ThreadIdString = PhFormatString(L"%u", wctNode->ThreadObject.ThreadId);
            rootNode->ProcessIdString = PhFormatString(L"%u", wctNode->ThreadObject.ProcessId);
            rootNode->WaitTimeString = PhFormatString(L"%u", wctNode->ThreadObject.WaitTime);
            rootNode->ContextSwitchesString = PhFormatString(L"%u", wctNode->ThreadObject.ContextSwitches);
            rootNode->TimeoutString = PhFormatString(L"%I64d", wctNode->LockObject.Timeout.QuadPart);

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
                //    rootNode->ObjectNameString = PhFormatString(L"[%u, %u]",
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

    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

static NTSTATUS WaitChainCallbackThread(
    _In_ PVOID Parameter
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWCT_CONTEXT context = (PWCT_CONTEXT)Parameter;

    if (!WaitChainRegisterCallbacks(context))
        return NTSTATUS_FROM_WIN32(GetLastError());

    // Synchronous WCT session
    context->WctSessionHandle = OpenThreadWaitChainSession(0, NULL);

    if (context->WctSessionHandle == NULL)
        return NTSTATUS_FROM_WIN32(GetLastError());

    if (context->IsProcessItem)
    {
        PVOID processes = NULL;
        PSYSTEM_PROCESS_INFORMATION process = NULL;

        if (!NT_SUCCESS(status = PhEnumProcesses(&processes)))
            return status;

        process = PH_FIRST_PROCESS(processes);

        do
        {
            if (process->UniqueProcessId == context->ProcessItem->ProcessId)
            {
                for (ULONG i = 0; i < process->NumberOfThreads; i++)
                {
                    WaitChainCheckThread(context, process->Threads[i].ClientId.UniqueThread);
                }
            }
        } while (process = PH_NEXT_PROCESS(process));
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
        LdrUnloadDll(context->Ole32ModuleHandle);
    }

    return status;
}

static INT_PTR CALLBACK WaitChainDlgProc(
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
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PWCT_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            PhUnregisterDialog(hwndDlg);
            PhSaveWindowPlacementToSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);
            PhDeleteLayoutManager(&context->LayoutManager);
            WtcDeleteWindowTree(&context->TreeContext);

            RemoveProp(hwndDlg, L"Context");
            PhFree(context);
        }
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE threadHandle = NULL;

            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_CUSTOM1);

            PhRegisterDialog(hwndDlg);
            WtcInitializeWindowTree(hwndDlg, context->TreeNewHandle, &context->TreeContext);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhLoadWindowPlacementFromSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);

            if (threadHandle = PhCreateThread(0, WaitChainCallbackThread, (PVOID)context))
                NtClose(threadHandle);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case ID_WCTSHOWCONTEXTMENU:
                {
                    POINT point;
                    HMENU menu;
                    HMENU subMenu;
                    PWCT_ROOT_NODE selectedNode;

                    point.x = (SHORT)LOWORD(lParam);
                    point.y = (SHORT)HIWORD(lParam);

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        menu = LoadMenu(PluginInstance->DllBase, MAKEINTRESOURCE(IDR_MAIN_MENU));
                        subMenu = GetSubMenu(menu, 0);
                        SetMenuDefaultItem(subMenu, ID_MENU_PROPERTIES, FALSE);

                        if (selectedNode->ThreadId > 0)
                        {
                            PhEnableMenuItem(subMenu, ID_MENU_GOTOTHREAD, TRUE);
                            PhEnableMenuItem(subMenu, ID_MENU_GOTOPROCESS, TRUE);
                        }
                        else
                        {
                            PhEnableMenuItem(subMenu, ID_MENU_GOTOTHREAD, FALSE);
                            PhEnableMenuItem(subMenu, ID_MENU_GOTOPROCESS, FALSE);
                        }

                        TrackPopupMenu(
                            subMenu,
                            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
                            point.x,
                            point.y,
                            0,
                            hwndDlg,
                            NULL
                            );

                        DestroyMenu(menu);
                    }
                }
                break;
            case ID_MENU_GOTOPROCESS:
                {
                    PWCT_ROOT_NODE selectedNode = NULL;
                    PPH_PROCESS_NODE processNode = NULL;

                    if (selectedNode = WeGetSelectedWindowNode(&context->TreeContext))
                    {
                        if (processNode = PhFindProcessNode(selectedNode->ProcessId))
                        {
                            ProcessHacker_SelectTabPage(PhMainWndHandle, 0);
                            PhSelectAndEnsureVisibleProcessNode(processNode);
                        }
                    }
                }
                break;
            case ID_MENU_GOTOTHREAD:
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
                            PhShowError(hwndDlg, L"The process does not exist.");
                        }
                    }
                }
                break;
            case ID_MENU_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                    PhSetClipboardStringEx(hwndDlg, text->Buffer, text->Length);
                    PhDereferenceObject(text);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

static VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = (PPH_PLUGIN_MENU_ITEM)Parameter;

    switch (menuItem->Id)
    {
    case IDD_WCT_MENUITEM:
        {
            DialogBoxParam(
                PluginInstance->DllBase,
                MAKEINTRESOURCE(IDD_WCT_DIALOG),
                NULL,
                WaitChainDlgProc,
                (LPARAM)menuItem->Context
                );
        }
        break;
    }
}

static VOID NTAPI ProcessMenuInitializingCallback(
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

    context = (PWCT_CONTEXT)PhAllocate(sizeof(WCT_CONTEXT));
    memset(context, 0, sizeof(WCT_CONTEXT));

    context->IsProcessItem = TRUE;
    context->ProcessItem = processItem;

    miscMenuItem = PhFindEMenuItem(menuInfo->Menu, 0, L"Miscellaneous", 0);
    if (miscMenuItem)
    {
        menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, IDD_WCT_MENUITEM, L"Wait Chain Traversal", context);
        PhInsertEMenuItem(miscMenuItem, menuItem, -1);

        // Disable menu if current process selected.
        if (processItem == NULL || processItem->ProcessId == NtCurrentProcessId())
            menuItem->Flags |= PH_EMENU_DISABLED;
    }
}

static VOID NTAPI ThreadMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PWCT_CONTEXT context = NULL;
    PPH_PLUGIN_MENU_INFORMATION menuInfo = NULL;
    PPH_THREAD_ITEM threadItem = NULL;
    PPH_EMENU_ITEM menuItem = NULL;
    PPH_EMENU_ITEM miscMenuItem = NULL;

    menuInfo = (PPH_PLUGIN_MENU_INFORMATION)Parameter;

    if (menuInfo->u.Thread.NumberOfThreads == 1)
        threadItem = menuInfo->u.Thread.Threads[0];
    else
        threadItem = NULL;

    context = (PWCT_CONTEXT)PhAllocate(sizeof(WCT_CONTEXT));
    memset(context, 0, sizeof(WCT_CONTEXT));

    context->IsProcessItem = FALSE;
    context->ThreadItem = threadItem;

    miscMenuItem = PhFindEMenuItem(menuInfo->Menu, 0, L"Analyze", 0);
    if (miscMenuItem)
    {
        menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, IDD_WCT_MENUITEM, L"Wait Chain Traversal", context);
        PhInsertEMenuItem(miscMenuItem, menuItem, -1);

        // Disable menu if current process selected.
        if (threadItem == NULL || menuInfo->u.Thread.ProcessId == NtCurrentProcessId())
            menuItem->Flags |= PH_EMENU_DISABLED;
    }
}

LOGICAL DllMain(
    _In_ HINSTANCE Instance,
    _In_ ULONG Reason,
    _Reserved_ PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Wait Chain Traversal";
            info->Author = L"dmex";
            info->Description = L"Plugin for Wait Chain analysis via right-click Miscellaneous > Wait Chain Traversal or individual threads via Process properties > Threads tab > Analyze > Wait Chain Traversal.";
            info->HasOptions = FALSE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
                MenuItemCallback,
                NULL,
                &PluginMenuItemCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessMenuInitializing),
                ProcessMenuInitializingCallback,
                NULL,
                &ProcessMenuInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackThreadMenuInitializing),
                ThreadMenuInitializingCallback,
                NULL,
                &ThreadMenuInitializingCallbackRegistration
                );

            {
                PH_SETTING_CREATE settings[] =
                {
                    { StringSettingType, SETTING_NAME_TREE_LIST_COLUMNS, L"" },
                    { IntegerPairSettingType, SETTING_NAME_WINDOW_POSITION, L"100,100" },
                    { IntegerPairSettingType, SETTING_NAME_WINDOW_SIZE, L"690,540" }
                };

                PhAddSettings(settings, _countof(settings));
            }
        }
        break;
    }

    return TRUE;
}