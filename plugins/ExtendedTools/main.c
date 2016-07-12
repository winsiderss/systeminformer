/*
 * Process Hacker Extended Tools -
 *   main program
 *
 * Copyright (C) 2010-2015 wj32
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

#include "exttools.h"

PPH_PLUGIN PluginInstance;
LIST_ENTRY EtProcessBlockListHead;
LIST_ENTRY EtNetworkBlockListHead;
HWND ProcessTreeNewHandle;
HWND NetworkTreeNewHandle;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginTreeNewMessageCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessPropertiesInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION HandlePropertiesInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ThreadMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ModuleMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION NetworkTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION SystemInformationInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION MiniInformationInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
PH_CALLBACK_REGISTRATION NetworkItemsUpdatedCallbackRegistration;

static HANDLE ModuleProcessId;

VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    EtEtwStatisticsInitialization();
    EtGpuMonitorInitialization();

    EtRegisterNotifyIcons();
}

VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    EtSaveSettingsDiskTreeList();
    EtEtwStatisticsUninitialization();
}

VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    EtShowOptionsDialog((HWND)Parameter);
}

VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;

    switch (menuItem->Id)
    {
    case ID_PROCESS_UNLOADEDMODULES:
        {
            EtShowUnloadedDllsDialog(PhMainWndHandle, menuItem->Context);
        }
        break;
    case ID_PROCESS_WSWATCH:
        {
            EtShowWsWatchDialog(PhMainWndHandle, menuItem->Context);
        }
        break;
    case ID_THREAD_CANCELIO:
        {
            EtUiCancelIoThread(menuItem->OwnerWindow, menuItem->Context);
        }
        break;
    case ID_MODULE_SERVICES:
        {
            EtShowModuleServicesDialog(
                menuItem->OwnerWindow,
                ModuleProcessId,
                ((PPH_MODULE_ITEM)menuItem->Context)->Name->Buffer
                );
        }
        break;
    }
}

VOID NTAPI TreeNewMessageCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_MESSAGE message = Parameter;

    if (message->TreeNewHandle == ProcessTreeNewHandle)
        EtProcessTreeNewMessage(Parameter);
    else if (message->TreeNewHandle == NetworkTreeNewHandle)
        EtNetworkTreeNewMessage(Parameter);
}

VOID NTAPI MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    EtInitializeDiskTab();
}

VOID NTAPI ProcessPropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    EtProcessGpuPropertiesInitializing(Parameter);
    EtProcessEtwPropertiesInitializing(Parameter);
}

VOID NTAPI HandlePropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    EtHandlePropertiesInitializing(Parameter);
}

VOID NTAPI ProcessMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_PROCESS_ITEM processItem;
    ULONG flags;
    PPH_EMENU_ITEM miscMenu;

    if (menuInfo->u.Process.NumberOfProcesses == 1)
        processItem = menuInfo->u.Process.Processes[0];
    else
        processItem = NULL;

    flags = 0;

    if (!processItem)
        flags = PH_EMENU_DISABLED;

    miscMenu = PhFindEMenuItem(menuInfo->Menu, 0, L"Miscellaneous", 0);

    if (miscMenu)
    {
        PhInsertEMenuItem(miscMenu, PhPluginCreateEMenuItem(PluginInstance, flags, ID_PROCESS_UNLOADEDMODULES, L"Unloaded modules", processItem), -1);
        PhInsertEMenuItem(miscMenu, PhPluginCreateEMenuItem(PluginInstance, flags, ID_PROCESS_WSWATCH, L"WS watch", processItem), -1);
    }
}

VOID NTAPI ThreadMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_THREAD_ITEM threadItem;
    ULONG insertIndex;
    PPH_EMENU_ITEM menuItem;

    if (menuInfo->u.Thread.NumberOfThreads == 1)
        threadItem = menuInfo->u.Thread.Threads[0];
    else
        threadItem = NULL;

    if (menuItem = PhFindEMenuItem(menuInfo->Menu, 0, L"Resume", 0))
        insertIndex = PhIndexOfEMenuItem(menuInfo->Menu, menuItem) + 1;
    else
        insertIndex = 0;

    PhInsertEMenuItem(menuInfo->Menu, menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_THREAD_CANCELIO,
        L"Cancel I/O", threadItem), insertIndex);

    if (!threadItem) menuItem->Flags |= PH_EMENU_DISABLED;
}

VOID NTAPI ModuleMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_PROCESS_ITEM processItem;
    BOOLEAN addMenuItem;
    PPH_MODULE_ITEM moduleItem;
    ULONG insertIndex;
    PPH_EMENU_ITEM menuItem;

    addMenuItem = FALSE;

    if (processItem = PhReferenceProcessItem(menuInfo->u.Module.ProcessId))
    {
        if (processItem->ServiceList && processItem->ServiceList->Count != 0)
            addMenuItem = TRUE;

        PhDereferenceObject(processItem);
    }

    if (!addMenuItem)
        return;

    if (menuInfo->u.Module.NumberOfModules == 1)
        moduleItem = menuInfo->u.Module.Modules[0];
    else
        moduleItem = NULL;

    if (menuItem = PhFindEMenuItem(menuInfo->Menu, PH_EMENU_FIND_STARTSWITH, L"Inspect", 0))
        insertIndex = PhIndexOfEMenuItem(menuInfo->Menu, menuItem) + 1;
    else
        insertIndex = 0;

    ModuleProcessId = menuInfo->u.Module.ProcessId;

    PhInsertEMenuItem(menuInfo->Menu, menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_MODULE_SERVICES,
        L"Services", moduleItem), insertIndex);

    if (!moduleItem) menuItem->Flags |= PH_EMENU_DISABLED;
}

VOID NTAPI ProcessTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION treeNewInfo = Parameter;

    ProcessTreeNewHandle = treeNewInfo->TreeNewHandle;
    EtProcessTreeNewInitializing(Parameter);
}

VOID NTAPI NetworkTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION treeNewInfo = Parameter;

    NetworkTreeNewHandle = treeNewInfo->TreeNewHandle;
    EtNetworkTreeNewInitializing(Parameter);
}

VOID NTAPI SystemInformationInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (EtGpuEnabled)
        EtGpuSystemInformationInitializing(Parameter);
    if (EtEtwEnabled && !!PhGetIntegerSetting(SETTING_NAME_ENABLE_SYSINFO_GRAPHS))
        EtEtwSystemInformationInitializing(Parameter);
}

VOID NTAPI MiniInformationInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (EtGpuEnabled)
        EtGpuMiniInformationInitializing(Parameter);
    if (EtEtwEnabled)
        EtEtwMiniInformationInitializing(Parameter);
}

VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PLIST_ENTRY listEntry;

    // Note: no lock is needed because we only ever modify the list on this same thread.

    listEntry = EtProcessBlockListHead.Flink;

    while (listEntry != &EtProcessBlockListHead)
    {
        PET_PROCESS_BLOCK block;

        block = CONTAINING_RECORD(listEntry, ET_PROCESS_BLOCK, ListEntry);

        PhUpdateDelta(&block->HardFaultsDelta, block->ProcessItem->HardFaultCount);

        // Invalidate all text.

        PhAcquireQueuedLockExclusive(&block->TextCacheLock);
        memset(block->TextCacheValid, 0, sizeof(block->TextCacheValid));
        PhReleaseQueuedLockExclusive(&block->TextCacheLock);

        listEntry = listEntry->Flink;
    }
}

VOID NTAPI NetworkItemsUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PLIST_ENTRY listEntry;

    // Note: no lock is needed because we only ever modify the list on this same thread.

    listEntry = EtNetworkBlockListHead.Flink;

    while (listEntry != &EtNetworkBlockListHead)
    {
        PET_NETWORK_BLOCK block;

        block = CONTAINING_RECORD(listEntry, ET_NETWORK_BLOCK, ListEntry);

        // Invalidate all text.

        PhAcquireQueuedLockExclusive(&block->TextCacheLock);
        memset(block->TextCacheValid, 0, sizeof(block->TextCacheValid));
        PhReleaseQueuedLockExclusive(&block->TextCacheLock);

        listEntry = listEntry->Flink;
    }
}

PET_PROCESS_BLOCK EtGetProcessBlock(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    return PhPluginGetObjectExtension(PluginInstance, ProcessItem, EmProcessItemType);
}

PET_NETWORK_BLOCK EtGetNetworkBlock(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    return PhPluginGetObjectExtension(PluginInstance, NetworkItem, EmNetworkItemType);
}

VOID EtInitializeProcessBlock(
    _Out_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    memset(Block, 0, sizeof(ET_PROCESS_BLOCK));
    Block->ProcessItem = ProcessItem;
    PhInitializeQueuedLock(&Block->TextCacheLock);
    InsertTailList(&EtProcessBlockListHead, &Block->ListEntry);
}

VOID EtDeleteProcessBlock(
    _In_ PET_PROCESS_BLOCK Block
    )
{
    ULONG i;

    EtProcIconNotifyProcessDelete(Block);

    for (i = 1; i <= ETPRTNC_MAXIMUM; i++)
    {
        PhClearReference(&Block->TextCache[i]);
    }

    RemoveEntryList(&Block->ListEntry);
}

VOID EtInitializeNetworkBlock(
    _Out_ PET_NETWORK_BLOCK Block,
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    memset(Block, 0, sizeof(ET_NETWORK_BLOCK));
    Block->NetworkItem = NetworkItem;
    PhInitializeQueuedLock(&Block->TextCacheLock);
    InsertTailList(&EtNetworkBlockListHead, &Block->ListEntry);
}

VOID EtDeleteNetworkBlock(
    _In_ PET_NETWORK_BLOCK Block
    )
{
    ULONG i;

    for (i = 1; i <= ETNETNC_MAXIMUM; i++)
    {
        PhClearReference(&Block->TextCache[i]);
    }

    RemoveEntryList(&Block->ListEntry);
}

VOID NTAPI ProcessItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    EtInitializeProcessBlock(Extension, Object);
}

VOID NTAPI ProcessItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    EtDeleteProcessBlock(Extension);
}

VOID NTAPI NetworkItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    EtInitializeNetworkBlock(Extension, Object);
}

VOID NTAPI NetworkItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    EtDeleteNetworkBlock(Extension);
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

            info->DisplayName = L"Extended Tools";
            info->Author = L"wj32";
            info->Description = L"Extended functionality for Windows 7 and above, including ETW monitoring, GPU monitoring and a Disk tab.";
            info->Url = L"https://wj32.org/processhacker/forums/viewtopic.php?t=1114";
            info->HasOptions = TRUE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackUnload),
                UnloadCallback,
                NULL,
                &PluginUnloadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
                MenuItemCallback,
                NULL,
                &PluginMenuItemCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackTreeNewMessage),
                TreeNewMessageCallback,
                NULL,
                &PluginTreeNewMessageCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessPropertiesInitializing),
                ProcessPropertiesInitializingCallback,
                NULL,
                &ProcessPropertiesInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackHandlePropertiesInitializing),
                HandlePropertiesInitializingCallback,
                NULL,
                &HandlePropertiesInitializingCallbackRegistration
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
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackModuleMenuInitializing),
                ModuleMenuInitializingCallback,
                NULL,
                &ModuleMenuInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessTreeNewInitializing),
                ProcessTreeNewInitializingCallback,
                NULL,
                &ProcessTreeNewInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackNetworkTreeNewInitializing),
                NetworkTreeNewInitializingCallback,
                NULL,
                &NetworkTreeNewInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackSystemInformationInitializing),
                SystemInformationInitializingCallback,
                NULL,
                &SystemInformationInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMiniInformationInitializing),
                MiniInformationInitializingCallback,
                NULL,
                &MiniInformationInitializingCallbackRegistration
                );

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                ProcessesUpdatedCallback,
                NULL,
                &ProcessesUpdatedCallbackRegistration
                );
            PhRegisterCallback(
                &PhNetworkItemsUpdatedEvent,
                NetworkItemsUpdatedCallback,
                NULL,
                &NetworkItemsUpdatedCallbackRegistration
                );

            InitializeListHead(&EtProcessBlockListHead);
            InitializeListHead(&EtNetworkBlockListHead);

            PhPluginSetObjectExtension(
                PluginInstance,
                EmProcessItemType,
                sizeof(ET_PROCESS_BLOCK),
                ProcessItemCreateCallback,
                ProcessItemDeleteCallback
                );
            PhPluginSetObjectExtension(
                PluginInstance,
                EmNetworkItemType,
                sizeof(ET_NETWORK_BLOCK),
                NetworkItemCreateCallback,
                NetworkItemDeleteCallback
                );

            {
                static PH_SETTING_CREATE settings[] =
                {
                    { StringSettingType, SETTING_NAME_DISK_TREE_LIST_COLUMNS, L"" },
                    { IntegerPairSettingType, SETTING_NAME_DISK_TREE_LIST_SORT, L"4,2" }, // 4, DescendingSortOrder
                    { IntegerSettingType, SETTING_NAME_ENABLE_ETW_MONITOR, L"1" },
                    { IntegerSettingType, SETTING_NAME_ENABLE_GPU_MONITOR, L"1" },
                    { IntegerSettingType, SETTING_NAME_ENABLE_SYSINFO_GRAPHS, L"1" },
                    { StringSettingType, SETTING_NAME_GPU_NODE_BITMAP, L"01000000" },
                    { IntegerSettingType, SETTING_NAME_GPU_LAST_NODE_COUNT, L"0" }
                };

                PhAddSettings(settings, sizeof(settings) / sizeof(PH_SETTING_CREATE));
            }
        }
        break;
    }

    return TRUE;
}