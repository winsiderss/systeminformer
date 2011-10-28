/*
 * Process Hacker Extended Tools -
 *   main program
 *
 * Copyright (C) 2010-2011 wj32
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
#include "resource.h"

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI UnloadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI TreeNewMessageCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ProcessPropertiesInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI HandlePropertiesInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ProcessMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ThreadMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ModuleMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ProcessTreeNewInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI NetworkTreeNewInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI NetworkItemsUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ProcessItemCreateCallback(
    __in PVOID Object,
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in PVOID Extension
    );

VOID NTAPI ProcessItemDeleteCallback(
    __in PVOID Object,
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in PVOID Extension
    );

VOID NTAPI NetworkItemCreateCallback(
    __in PVOID Object,
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in PVOID Extension
    );

VOID NTAPI NetworkItemDeleteCallback(
    __in PVOID Object,
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in PVOID Extension
    );

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
PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
PH_CALLBACK_REGISTRATION NetworkItemsUpdatedCallbackRegistration;

static HANDLE ModuleProcessId;

LOGICAL DllMain(
    __in HINSTANCE Instance,
    __in ULONG Reason,
    __reserved PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.ExtendedTools", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Extended Tools";
            info->Author = L"wj32";
            info->Description = L"Extended functionality for Windows Vista and above, including ETW monitoring and a Disk tab.";
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
                    { IntegerSettingType, SETTING_NAME_ETWSYS_ALWAYS_ON_TOP, L"0" },
                    { IntegerPairSettingType, SETTING_NAME_ETWSYS_WINDOW_POSITION, L"400,400" },
                    { IntegerPairSettingType, SETTING_NAME_ETWSYS_WINDOW_SIZE, L"500,400" },
                    { IntegerPairSettingType, SETTING_NAME_MEMORY_LISTS_WINDOW_POSITION, L"400,400" },
                    { IntegerSettingType, SETTING_NAME_GPUSYS_ALWAYS_ON_TOP, L"0" },
                    { IntegerPairSettingType, SETTING_NAME_GPUSYS_WINDOW_POSITION, L"400,400" },
                    { IntegerPairSettingType, SETTING_NAME_GPUSYS_WINDOW_SIZE, L"500,500" }
                };

                PhAddSettings(settings, sizeof(settings) / sizeof(PH_SETTING_CREATE));
            }
        }
        break;
    }

    return TRUE;
}

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    EtEtwStatisticsInitialization();
    EtGpuMonitorInitialization();
}

VOID NTAPI UnloadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    EtSaveSettingsDiskTreeList();
    EtEtwStatisticsUninitialization();
}

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    EtShowOptionsDialog((HWND)Parameter);
}

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;

    switch (menuItem->Id)
    {
    case ID_VIEW_DISKANDNETWORK:
        {
            EtShowEtwSystemDialog();
        }
        break;
    case ID_VIEW_GPUINFORMATION:
        {
            EtShowGpuSystemDialog();
        }
        break;
    case ID_VIEW_MEMORYLISTS:
        {
            EtShowMemoryListsDialog();
        }
        break;
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
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_MESSAGE message = Parameter;

    if (message->TreeNewHandle == ProcessTreeNewHandle)
        EtProcessTreeNewMessage(Parameter);
    else if (message->TreeNewHandle == NetworkTreeNewHandle)
        EtNetworkTreeNewMessage(Parameter);
}

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_VIEW, L"System Information", ID_VIEW_MEMORYLISTS, L"Memory Lists", NULL);

    if (EtGpuEnabled)
    {
        // This will get inserted before Memory Lists.
        PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_VIEW, L"System Information", ID_VIEW_GPUINFORMATION, L"GPU Information", NULL);
    }

    if (EtEtwEnabled)
    {
        // This will get inserted before GPU Information.
        PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_VIEW, L"System Information", ID_VIEW_DISKANDNETWORK, L"Disk and Network", NULL);

        EtInitializeDiskTab();
    }
}

VOID NTAPI ProcessPropertiesInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    EtProcessGpuPropertiesInitializing(Parameter);
    EtProcessEtwPropertiesInitializing(Parameter);
}

VOID NTAPI HandlePropertiesInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    EtHandlePropertiesInitializing(Parameter);
}

VOID NTAPI ProcessMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
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
        PhInsertEMenuItem(miscMenu, PhPluginCreateEMenuItem(PluginInstance, flags, ID_PROCESS_UNLOADEDMODULES, L"Unloaded Modules", processItem), -1);
        PhInsertEMenuItem(miscMenu, PhPluginCreateEMenuItem(PluginInstance, flags, ID_PROCESS_WSWATCH, L"WS Watch", processItem), -1);
    }
}

VOID NTAPI ThreadMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
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
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
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

    if (menuItem = PhFindEMenuItem(menuInfo->Menu, 0, L"Inspect", 0))
        insertIndex = PhIndexOfEMenuItem(menuInfo->Menu, menuItem) + 1;
    else
        insertIndex = 0;

    ModuleProcessId = menuInfo->u.Module.ProcessId;

    PhInsertEMenuItem(menuInfo->Menu, menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_MODULE_SERVICES,
        L"Services", moduleItem), insertIndex);

    if (!moduleItem) menuItem->Flags |= PH_EMENU_DISABLED;
}

VOID NTAPI ProcessTreeNewInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION treeNewInfo = Parameter;

    ProcessTreeNewHandle = treeNewInfo->TreeNewHandle;
    EtProcessTreeNewInitializing(Parameter);
}

VOID NTAPI NetworkTreeNewInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION treeNewInfo = Parameter;

    NetworkTreeNewHandle = treeNewInfo->TreeNewHandle;
    EtNetworkTreeNewInitializing(Parameter);
}

static VOID NTAPI ProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
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

static VOID NTAPI NetworkItemsUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
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
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    return PhPluginGetObjectExtension(PluginInstance, ProcessItem, EmProcessItemType);
}

PET_NETWORK_BLOCK EtGetNetworkBlock(
    __in PPH_NETWORK_ITEM NetworkItem
    )
{
    return PhPluginGetObjectExtension(PluginInstance, NetworkItem, EmNetworkItemType);
}

VOID EtInitializeProcessBlock(
    __out PET_PROCESS_BLOCK Block,
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    memset(Block, 0, sizeof(ET_PROCESS_BLOCK));
    Block->ProcessItem = ProcessItem;
    PhInitializeQueuedLock(&Block->TextCacheLock);
    InsertTailList(&EtProcessBlockListHead, &Block->ListEntry);
}

VOID EtDeleteProcessBlock(
    __in PET_PROCESS_BLOCK Block
    )
{
    ULONG i;

    EtProcIconNotifyProcessDelete(Block);

    for (i = 1; i <= ETPRTNC_MAXIMUM; i++)
    {
        PhSwapReference(&Block->TextCache[i], NULL);
    }

    RemoveEntryList(&Block->ListEntry);
}

VOID EtInitializeNetworkBlock(
    __out PET_NETWORK_BLOCK Block,
    __in PPH_NETWORK_ITEM NetworkItem
    )
{
    memset(Block, 0, sizeof(ET_NETWORK_BLOCK));
    Block->NetworkItem = NetworkItem;
    PhInitializeQueuedLock(&Block->TextCacheLock);
    InsertTailList(&EtNetworkBlockListHead, &Block->ListEntry);
}

VOID EtDeleteNetworkBlock(
    __in PET_NETWORK_BLOCK Block
    )
{
    ULONG i;

    for (i = 1; i <= ETNETNC_MAXIMUM; i++)
    {
        PhSwapReference(&Block->TextCache[i], NULL);
    }

    RemoveEntryList(&Block->ListEntry);
}

VOID NTAPI ProcessItemCreateCallback(
    __in PVOID Object,
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in PVOID Extension
    )
{
    EtInitializeProcessBlock(Extension, Object);
}

VOID NTAPI ProcessItemDeleteCallback(
    __in PVOID Object,
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in PVOID Extension
    )
{
    EtDeleteProcessBlock(Extension);
}

VOID NTAPI NetworkItemCreateCallback(
    __in PVOID Object,
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in PVOID Extension
    )
{
    EtInitializeNetworkBlock(Extension, Object);
}

VOID NTAPI NetworkItemDeleteCallback(
    __in PVOID Object,
    __in PH_EM_OBJECT_TYPE ObjectType,
    __in PVOID Extension
    )
{
    EtDeleteNetworkBlock(Extension);
}
