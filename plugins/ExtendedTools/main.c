/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2018-2023
 *
 */

#include "exttools.h"
#include "extension\plugin.h"

PPH_PLUGIN PluginInstance = NULL;
HWND ProcessTreeNewHandle = NULL;
HWND NetworkTreeNewHandle = NULL;
LIST_ENTRY EtProcessBlockListHead = { &EtProcessBlockListHead, &EtProcessBlockListHead };
LIST_ENTRY EtNetworkBlockListHead = { &EtNetworkBlockListHead, &EtNetworkBlockListHead };
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginTreeNewMessageCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginPhSvcRequestCallbackRegistration;
PH_CALLBACK_REGISTRATION MainMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessPropertiesInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION HandlePropertiesInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ThreadMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ModuleMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION NetworkTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION SystemInformationInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION MiniInformationInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION TrayIconsInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessItemsUpdatedCallbackRegistration;
PH_CALLBACK_REGISTRATION NetworkItemsUpdatedCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessStatsEventCallbackRegistration;
PH_CALLBACK_REGISTRATION SettingsUpdatedCallbackRegistration;

EXTENDEDTOOLS_INTERFACE PluginInterface =
{
    EXTENDEDTOOLS_INTERFACE_VERSION,
    EtLookupTotalGpuAdapterUtilization,
    EtLookupTotalGpuAdapterDedicated,
    EtLookupTotalGpuAdapterShared,
    EtLookupTotalGpuAdapterEngineUtilization
};

ULONG EtWindowsVersion = WINDOWS_ANCIENT;
BOOLEAN EtIsExecutingInWow64 = FALSE;
ULONG ProcessesUpdatedCount = 0;
static HANDLE ModuleProcessId = NULL;
ULONG EtUpdateInterval = 0;
USHORT EtMaxPrecisionUnit = 2;
BOOLEAN EtGraphShowText = FALSE;
BOOLEAN EtEnableScaleGraph = FALSE;
BOOLEAN EtEnableScaleText = FALSE;
BOOLEAN EtPropagateCpuUsage = FALSE;
BOOLEAN EtEnableAvxSupport = FALSE;

VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    EtWindowsVersion = PhWindowsVersion;
    EtIsExecutingInWow64 = PhIsExecutingInWow64();

    EtLoadSettings();

    EtEtwStatisticsInitialization();
    EtGpuMonitorInitialization();
    EtFramesMonitorInitialization();
}

VOID NTAPI UnloadCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    BOOLEAN SessionEnding = (BOOLEAN)PtrToUlong(Parameter);

    // Skip ETW when the system is shutting down. (dmex)
    if (SessionEnding)
        return;

    EtEtwStatisticsUninitialization();
    EtFramesMonitorUninitialization();
}

VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_OPTIONS_POINTERS optionsEntry = (PPH_PLUGIN_OPTIONS_POINTERS)Parameter;

    if (optionsEntry)
    {
        optionsEntry->CreateSection(
            L"ExtendedTools",
            PluginInstance->DllBase,
            MAKEINTRESOURCE(IDD_OPTIONS),
            OptionsDlgProc,
            NULL
            );
    }
}

VOID NTAPI MenuItemCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;

    switch (menuItem->Id)
    {
    case ID_PROCESS_UNLOADEDMODULES:
        {
            EtShowUnloadedDllsDialog(menuItem->OwnerWindow, menuItem->Context);
        }
        break;
    case ID_PROCESS_WSWATCH:
        {
            EtShowWsWatchDialog(menuItem->OwnerWindow, menuItem->Context);
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
                ((PPH_MODULE_ITEM)menuItem->Context)->Name
                );
        }
        break;
    case ID_REPARSE_POINTS:
    case ID_REPARSE_OBJID:
    case ID_REPARSE_SDDL:
        {
            EtShowReparseDialog(menuItem->OwnerWindow, UlongToPtr(menuItem->Id));
        }
        break;
    case ID_PROCESS_WAITCHAIN:
        {
            EtShowWaitChainProcessDialog(menuItem->OwnerWindow, menuItem->Context);
        }
        break;
    case ID_THREAD_WAITCHAIN:
        {
            EtShowWaitChainThreadDialog(menuItem->OwnerWindow, menuItem->Context);
        }
        break;
    case ID_PIPE_ENUM:
        {
            EtShowPipeEnumDialog(menuItem->OwnerWindow);
        }
        break;
    case ID_FIRMWARE:
        {
            EtShowFirmwareDialog(menuItem->OwnerWindow);
        }
        break;
    case ID_OBJMGR:
        {
            EtShowObjectManagerDialog(menuItem->OwnerWindow);
        }
        break;
    case ID_POOL_TABLE:
        {
            EtShowPoolTableDialog(menuItem->OwnerWindow);
        }
        break;
    case ID_TPM:
        {
            EtShowTpmDialog(menuItem->OwnerWindow);
        }
        break;
    }
}

VOID NTAPI TreeNewMessageCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_MESSAGE message = Parameter;

    if (message->TreeNewHandle == ProcessTreeNewHandle)
        EtProcessTreeNewMessage(Parameter);
    else if (message->TreeNewHandle == NetworkTreeNewHandle)
        EtNetworkTreeNewMessage(Parameter);
}

VOID NTAPI PhSvcRequestCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (!Parameter)
        return;

    DispatchPhSvcRequest(Parameter);
}

VOID NTAPI MainMenuInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_EMENU_ITEM systemMenu;
    PPH_EMENU_ITEM bootMenuItem;
    PPH_EMENU_ITEM tpmMenuItem;
    PPH_EMENU_ITEM reparsePointsMenu;
    PPH_EMENU_ITEM reparseObjIdMenu;
    PPH_EMENU_ITEM reparseSsdlMenu;

    if (menuInfo->u.MainMenu.SubMenuIndex != PH_MENU_ITEM_LOCATION_TOOLS)
        return;

    if (!(systemMenu = PhFindEMenuItem(menuInfo->Menu, 0, L"System", 0)))
    {
        PhInsertEMenuItem(menuInfo->Menu, systemMenu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"&System", NULL), ULONG_MAX);
    }

    PhInsertEMenuItem(systemMenu, PhPluginCreateEMenuItem(PluginInstance, 0, ID_POOL_TABLE, L"Poo&l Table", NULL), ULONG_MAX);
    PhInsertEMenuItem(systemMenu, PhPluginCreateEMenuItem(PluginInstance, 0, ID_OBJMGR, L"&Object Manager", NULL), ULONG_MAX);
    PhInsertEMenuItem(systemMenu, bootMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_FIRMWARE, L"Firm&ware Table", NULL), ULONG_MAX);
    PhInsertEMenuItem(systemMenu, tpmMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_TPM, L"&Trusted Platform Module", NULL), ULONG_MAX);
    PhInsertEMenuItem(systemMenu, PhPluginCreateEMenuItem(PluginInstance, 0, ID_PIPE_ENUM, L"&Named Pipes", NULL), ULONG_MAX);
    PhInsertEMenuItem(systemMenu, reparsePointsMenu = PhPluginCreateEMenuItem(PluginInstance, 0, ID_REPARSE_POINTS, L"NTFS Reparse Points", NULL), ULONG_MAX);
    PhInsertEMenuItem(systemMenu, reparseObjIdMenu = PhPluginCreateEMenuItem(PluginInstance, 0, ID_REPARSE_OBJID, L"NTFS Object Identifiers", NULL), ULONG_MAX);
    PhInsertEMenuItem(systemMenu, reparseSsdlMenu = PhPluginCreateEMenuItem(PluginInstance, 0, ID_REPARSE_SDDL, L"NTFS Security Descriptors", NULL), ULONG_MAX);

    if (!PhGetOwnTokenAttributes().Elevated)
    {
        bootMenuItem->Flags |= PH_EMENU_DISABLED;
        tpmMenuItem->Flags |= PH_EMENU_DISABLED;
        reparsePointsMenu->Flags |= PH_EMENU_DISABLED;
        reparseObjIdMenu->Flags |= PH_EMENU_DISABLED;
        reparseSsdlMenu->Flags |= PH_EMENU_DISABLED;
    }

    if (EtWindowsVersion < WINDOWS_8 || !EtTpmIsReady())
        tpmMenuItem->Flags |= PH_EMENU_DISABLED;
}

VOID NTAPI MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    EtInitializeDiskTab();
    EtInitializeFirewallTab();
    EtRegisterToolbarGraphs();

    EtFramesMonitorStart();
}

VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (ProcessesUpdatedCount != 3)
    {
        ProcessesUpdatedCount++;
        return;
    }
}

VOID NTAPI ProcessPropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (Parameter)
    {
        EtProcessGpuPropertiesInitializing(Parameter);
        EtProcessFramesPropertiesInitializing(Parameter);
        EtProcessEtwPropertiesInitializing(Parameter);
    }
}

VOID NTAPI HandlePropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (Parameter)
        EtHandlePropertiesInitializing(Parameter);
}

VOID NTAPI ProcessMenuInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_PROCESS_ITEM processItem;
    ULONG flags;
    PPH_EMENU_ITEM miscMenu;
    PPH_EMENU_ITEM menuItem;

    if (menuInfo->u.Process.NumberOfProcesses == 1)
        processItem = menuInfo->u.Process.Processes[0];
    else
        processItem = NULL;

    flags = 0;

    if (processItem)
    {
        if (PH_IS_FAKE_PROCESS_ID(processItem->ProcessId) ||
            processItem->ProcessId == SYSTEM_IDLE_PROCESS_ID)
        {
            flags = PH_EMENU_DISABLED;
        }
    }
    else
    {
        flags = PH_EMENU_DISABLED;
    }

    miscMenu = PhFindEMenuItem(menuInfo->Menu, 0, NULL, PHAPP_ID_PROCESS_MISCELLANEOUS);

    if (miscMenu)
    {
        PhInsertEMenuItem(miscMenu, PhPluginCreateEMenuItem(PluginInstance, flags, ID_PROCESS_UNLOADEDMODULES, L"&Unloaded modules", processItem), ULONG_MAX);
        PhInsertEMenuItem(miscMenu, PhPluginCreateEMenuItem(PluginInstance, flags, ID_PROCESS_WSWATCH, L"&WS watch", processItem), ULONG_MAX);
        menuItem = PhPluginCreateEMenuItem(PluginInstance, flags, ID_PROCESS_WAITCHAIN, L"Wait Chain Tra&versal", processItem);
        PhInsertEMenuItem(miscMenu, menuItem, ULONG_MAX);

        if (!processItem || !processItem->QueryHandle || processItem->ProcessId == NtCurrentProcessId())
            menuItem->Flags |= PH_EMENU_DISABLED;

        if (!PhGetOwnTokenAttributes().Elevated)
            menuItem->Flags |= PH_EMENU_DISABLED;
    }
}

VOID NTAPI ThreadMenuInitializingCallback(
    _In_ PVOID Parameter,
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

    if (menuItem = PhFindEMenuItem(menuInfo->Menu, 0, NULL, PHAPP_ID_THREAD_RESUME))
        insertIndex = PhIndexOfEMenuItem(menuInfo->Menu, menuItem) + 1;
    else
        insertIndex = ULONG_MAX;

    menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_THREAD_CANCELIO, L"Ca&ncel I/O", threadItem);
    PhInsertEMenuItem(menuInfo->Menu, menuItem, insertIndex);

    if (!threadItem)
        PhSetDisabledEMenuItem(menuItem);
    if (menuInfo->u.Thread.ProcessId == SYSTEM_IDLE_PROCESS_ID)
        PhSetDisabledEMenuItem(menuItem);

    if (menuItem = PhFindEMenuItem(menuInfo->Menu, 0, NULL, PHAPP_ID_ANALYZE_WAIT))
        insertIndex = PhIndexOfEMenuItem(menuInfo->Menu, menuItem) + 1;
    else
        insertIndex = ULONG_MAX;

    menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_THREAD_WAITCHAIN, L"Wait Chain Tra&versal", threadItem);
    PhInsertEMenuItem(menuInfo->Menu, menuItem, insertIndex);

    if (!threadItem)
        PhSetDisabledEMenuItem(menuItem);
    if (menuInfo->u.Thread.ProcessId == SYSTEM_IDLE_PROCESS_ID || menuInfo->u.Thread.ProcessId == NtCurrentProcessId())
        PhSetDisabledEMenuItem(menuItem);
}

VOID NTAPI ModuleMenuInitializingCallback(
    _In_ PVOID Parameter,
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

    if (menuItem = PhFindEMenuItem(menuInfo->Menu, 0, NULL, PHAPP_ID_MODULE_UNLOAD))
        insertIndex = PhIndexOfEMenuItem(menuInfo->Menu, menuItem) + 1;
    else
        insertIndex = ULONG_MAX;

    ModuleProcessId = menuInfo->u.Module.ProcessId;

    menuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_MODULE_SERVICES, L"Ser&vices", moduleItem);
    PhInsertEMenuItem(menuInfo->Menu, PhCreateEMenuSeparator(), insertIndex);
    PhInsertEMenuItem(menuInfo->Menu, menuItem, insertIndex + 1);

    if (!moduleItem) menuItem->Flags |= PH_EMENU_DISABLED;
}

VOID NTAPI ProcessTreeNewInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION treeNewInfo = Parameter;

    ProcessTreeNewHandle = treeNewInfo->TreeNewHandle;
    EtProcessTreeNewInitializing(Parameter);
}

VOID NTAPI NetworkTreeNewInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION treeNewInfo = Parameter;

    NetworkTreeNewHandle = treeNewInfo->TreeNewHandle;
    EtNetworkTreeNewInitializing(Parameter);
}

VOID NTAPI SystemInformationInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (EtGpuEnabled)
        EtGpuSystemInformationInitializing(Parameter);
    if (EtEtwEnabled && !!PhGetIntegerSetting(SETTING_NAME_ENABLE_SYSINFO_GRAPHS))
        EtEtwSystemInformationInitializing(Parameter);
}

VOID NTAPI MiniInformationInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (EtGpuEnabled)
        EtGpuMiniInformationInitializing(Parameter);
    if (EtEtwEnabled)
        EtEtwMiniInformationInitializing(Parameter);
}

VOID NTAPI TrayIconsInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    EtLoadTrayIconGuids();
    EtRegisterNotifyIcons(Parameter);
}

VOID NTAPI ProcessItemsUpdatedCallback(
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

        // Update the frame stats for the process (dmex)
        if (EtFramesEnabled)
        {
            if (!(block->ProcessItem->State & PH_PROCESS_ITEM_REMOVED))
            {
                EtProcessFramesUpdateProcessBlock(block);
            }
        }

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

VOID NTAPI ProcessStatsEventCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_PROCESS_STATS_EVENT event = Parameter;

    if (!event)
        return;
    if (event->Version)
        return;

    switch (event->Type)
    {
    case 1:
        {
            HWND listViewHandle = event->Parameter;
            PET_PROCESS_BLOCK block;

            if (!(block = EtGetProcessBlock(event->ProcessItem)))
                break;

            block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_GPU] = PhAddListViewGroup(
                listViewHandle, (INT)ListView_GetGroupCount(listViewHandle), L"GPU");
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTALDEDICATED] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_GPU], MAXINT, L"Dedicated memory", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTALDEDICATED],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTALDEDICATED]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTALSHARED] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_GPU], MAXINT, L"Shared memory", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTALSHARED],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTALSHARED]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTALCOMMIT] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_GPU], MAXINT, L"Commit memory", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTALCOMMIT],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTALCOMMIT]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTAL] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_GPU], MAXINT, L"Total memory", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTAL],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTAL]));

            block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_DISK] = PhAddListViewGroup(
                listViewHandle, (INT)ListView_GetGroupCount(listViewHandle), L"Disk I/O");
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKREADS] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_DISK], MAXINT, L"Reads", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKREADS],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKREADS]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKREADBYTES] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_DISK], MAXINT, L"Read bytes", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKREADBYTES],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKREADBYTES]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKREADBYTESDELTA] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_DISK], MAXINT, L"Read bytes delta", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKREADBYTESDELTA],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKREADBYTESDELTA]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKWRITES] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_DISK], MAXINT, L"Writes", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKWRITES],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKWRITES]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKWRITEBYTES] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_DISK], MAXINT, L"Write bytes", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKWRITEBYTES],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKWRITEBYTES]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKWRITEBYTESDELTA] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_DISK], MAXINT, L"Write bytes delta", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKWRITEBYTESDELTA],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKWRITEBYTESDELTA]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKTOTAL] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_DISK], MAXINT, L"Total", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKTOTAL],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKTOTAL]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKTOTALBYTES] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_DISK], MAXINT, L"Total bytes", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKTOTALBYTES],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKTOTALBYTES]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKTOTALBYTESDELTA] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_DISK], MAXINT, L"Total bytes delta", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKTOTALBYTESDELTA],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKTOTALBYTESDELTA]));

            block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_NETWORK] = PhAddListViewGroup(
                listViewHandle, (INT)ListView_GetGroupCount(listViewHandle), L"Network I/O");
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKREADS] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_NETWORK], MAXINT, L"Receives", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKREADS],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKREADS]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKREADBYTES] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_NETWORK], MAXINT, L"Receive bytes", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKREADBYTES],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKREADBYTES]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKREADBYTESDELTA] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_NETWORK], MAXINT, L"Receive bytes delta", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKREADBYTESDELTA],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKREADBYTESDELTA]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKWRITES] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_NETWORK], MAXINT, L"Sends", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKWRITES],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKWRITES]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKWRITEBYTES] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_NETWORK], MAXINT, L"Send bytes", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKWRITEBYTES],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKWRITEBYTES]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKWRITEBYTESDELTA] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_NETWORK], MAXINT, L"Send bytes delta", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKWRITEBYTESDELTA],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKWRITEBYTESDELTA]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKTOTAL] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_NETWORK], MAXINT, L"Total", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKTOTAL],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKTOTAL]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKTOTALBYTES] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_NETWORK], MAXINT, L"Total bytes", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKTOTALBYTES],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKTOTALBYTES]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKTOTALBYTESDELTA] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_NETWORK], MAXINT, L"Total bytes delta", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKTOTALBYTESDELTA],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKTOTALBYTESDELTA]));
        }
        break;
    case 2:
        {
            NMLVDISPINFO* dispInfo = (NMLVDISPINFO*)event->Parameter;
            PET_PROCESS_BLOCK block;

            if (!(block = EtGetProcessBlock(event->ProcessItem)))
                break;

            if (dispInfo->item.iSubItem == 1)
            {
                if (dispInfo->item.mask & LVIF_TEXT)
                {
                    ULONG index = PtrToUlong((PVOID)dispInfo->item.lParam);

                    if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTALDEDICATED])
                    {
                        PPH_STRING value;

                        value = PhFormatSize(block->GpuDedicatedUsage, ULONG_MAX);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTALSHARED])
                    {
                        PPH_STRING value;

                        value = PhFormatSize(block->GpuSharedUsage, ULONG_MAX);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTALCOMMIT])
                    {
                        PPH_STRING value;

                        value = PhFormatSize(block->GpuCommitUsage, ULONG_MAX);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTAL])
                    {
                        PPH_STRING value;

                        value = PhFormatSize(block->GpuDedicatedUsage + block->GpuSharedUsage + block->GpuCommitUsage, ULONG_MAX);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKREADS])
                    {
                        PPH_STRING value;

                        value = PhFormatUInt64(block->DiskReadCount, TRUE);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKREADBYTES])
                    {
                        PPH_STRING value;

                        value = PhFormatSize(block->DiskReadRawDelta.Value, ULONG_MAX);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKREADBYTESDELTA])
                    {
                        PPH_STRING value;

                        value = PhFormatSize(block->DiskReadRawDelta.Delta, ULONG_MAX);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKWRITES])
                    {
                        PPH_STRING value;

                        value = PhFormatUInt64(block->DiskWriteCount, TRUE);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKWRITEBYTES])
                    {
                        PPH_STRING value;

                        value = PhFormatSize(block->DiskWriteRawDelta.Value, ULONG_MAX);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKWRITEBYTESDELTA])
                    {
                        PPH_STRING value;

                        value = PhFormatSize(block->DiskWriteRawDelta.Delta, ULONG_MAX);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKTOTAL])
                    {
                        PPH_STRING value;

                        value = PhFormatUInt64(block->DiskReadCount + block->DiskWriteCount, TRUE);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKTOTALBYTES])
                    {
                        PPH_STRING value;

                        value = PhFormatSize(block->DiskReadRawDelta.Value + block->DiskWriteRawDelta.Value, ULONG_MAX);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKTOTALBYTESDELTA])
                    {
                        PPH_STRING value;

                        value = PhFormatSize(block->DiskReadRawDelta.Delta + block->DiskWriteRawDelta.Delta, ULONG_MAX);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKREADS])
                    {
                        PPH_STRING value;

                        value = PhFormatUInt64(block->NetworkReceiveCount, TRUE);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKREADBYTES])
                    {
                        PPH_STRING value;

                        value = PhFormatSize(block->NetworkReceiveRawDelta.Value, ULONG_MAX);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKREADBYTESDELTA])
                    {
                        PPH_STRING value;

                        value = PhFormatSize(block->NetworkReceiveRawDelta.Delta, ULONG_MAX);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKWRITES])
                    {
                        PPH_STRING value;

                        value = PhFormatUInt64(block->NetworkSendCount, TRUE);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKWRITEBYTES])
                    {
                        PPH_STRING value;

                        value = PhFormatSize(block->NetworkSendRawDelta.Value, ULONG_MAX);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKWRITEBYTESDELTA])
                    {
                        PPH_STRING value;

                        value = PhFormatSize(block->NetworkSendRawDelta.Delta, ULONG_MAX);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKTOTAL])
                    {
                        PPH_STRING value;

                        value = PhFormatUInt64(block->NetworkReceiveCount + block->NetworkSendCount, TRUE);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKTOTALBYTES])
                    {
                        PPH_STRING value;

                        value = PhFormatSize(block->NetworkReceiveRawDelta.Value + block->NetworkSendRawDelta.Value, ULONG_MAX);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKTOTALBYTESDELTA])
                    {
                        PPH_STRING value;

                        value = PhFormatSize(block->NetworkReceiveRawDelta.Delta + block->NetworkSendRawDelta.Delta, ULONG_MAX);
                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                        PhDereferenceObject(value);
                    }
                }
            }
        }
        break;
    }
}

VOID EtLoadSettings(
    VOID
    )
{
    EtUpdateInterval = PhGetIntegerSetting(L"UpdateInterval");
    EtMaxPrecisionUnit = (USHORT)PhGetIntegerSetting(L"MaxPrecisionUnit");
    EtGraphShowText = !!PhGetIntegerSetting(L"GraphShowText");
    EtEnableScaleGraph = !!PhGetIntegerSetting(L"EnableGraphMaxScale");
    EtEnableScaleText = !!PhGetIntegerSetting(L"EnableGraphMaxText");
    EtPropagateCpuUsage = !!PhGetIntegerSetting(L"PropagateCpuUsage");
    EtEnableAvxSupport = !!PhGetIntegerSetting(L"EnableAvxSupport");
    EtTrayIconTransparencyEnabled = !!PhGetIntegerSetting(L"IconTransparencyEnabled");
}

PPH_STRING PhGetSelectedListViewItemText(
    _In_ HWND hWnd
    )
{
    INT index = PhFindListViewItemByFlags(
        hWnd,
        INT_ERROR,
        LVNI_SELECTED
        );

    if (index != INT_ERROR)
    {
        WCHAR buffer[DOS_MAX_PATH_LENGTH] = L"";

        LVITEM item;
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 0;
        item.pszText = buffer;
        item.cchTextMax = ARRAYSIZE(buffer);

        if (ListView_GetItem(hWnd, &item))
            return PhCreateString(buffer);
    }

    return NULL;
}

VOID NTAPI SettingsUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    EtLoadSettings();
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
    ULONG sampleCount;

    memset(Block, 0, sizeof(ET_PROCESS_BLOCK));
    Block->ProcessItem = ProcessItem;
    PhInitializeQueuedLock(&Block->TextCacheLock);

    sampleCount = PhGetIntegerSetting(L"SampleCount");
    PhInitializeCircularBuffer_ULONG64(&Block->DiskReadHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG64(&Block->DiskWriteHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG64(&Block->NetworkSendHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG64(&Block->NetworkReceiveHistory, sampleCount);

    PhInitializeCircularBuffer_FLOAT(&Block->GpuHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG(&Block->MemoryHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG(&Block->MemorySharedHistory, sampleCount);
    PhInitializeCircularBuffer_ULONG(&Block->GpuCommittedHistory, sampleCount);

    //Block->GpuTotalRunningTimeDelta = PhAllocate(sizeof(PH_UINT64_DELTA) * EtGpuTotalNodeCount);
    //memset(Block->GpuTotalRunningTimeDelta, 0, sizeof(PH_UINT64_DELTA) * EtGpuTotalNodeCount);
    //Block->GpuTotalNodesHistory = PhAllocate(sizeof(PH_CIRCULAR_BUFFER_FLOAT) * EtGpuTotalNodeCount);

    if (EtFramesEnabled)
    {
        PhInitializeCircularBuffer_FLOAT(&Block->FramesPerSecondHistory, sampleCount);
        PhInitializeCircularBuffer_FLOAT(&Block->FramesLatencyHistory, sampleCount);
        PhInitializeCircularBuffer_FLOAT(&Block->FramesDisplayLatencyHistory, sampleCount);
        PhInitializeCircularBuffer_FLOAT(&Block->FramesMsBetweenPresentsHistory, sampleCount);
        PhInitializeCircularBuffer_FLOAT(&Block->FramesMsInPresentApiHistory, sampleCount);
        PhInitializeCircularBuffer_FLOAT(&Block->FramesMsUntilRenderCompleteHistory, sampleCount);
        PhInitializeCircularBuffer_FLOAT(&Block->FramesMsUntilDisplayedHistory, sampleCount);
    }

    InsertTailList(&EtProcessBlockListHead, &Block->ListEntry);
}

VOID EtDeleteProcessBlock(
    _In_ PET_PROCESS_BLOCK Block
    )
{
    PhDeleteCircularBuffer_ULONG64(&Block->DiskReadHistory);
    PhDeleteCircularBuffer_ULONG64(&Block->DiskWriteHistory);
    PhDeleteCircularBuffer_ULONG64(&Block->NetworkSendHistory);
    PhDeleteCircularBuffer_ULONG64(&Block->NetworkReceiveHistory);

    PhDeleteCircularBuffer_ULONG(&Block->GpuCommittedHistory);
    PhDeleteCircularBuffer_ULONG(&Block->MemorySharedHistory);
    PhDeleteCircularBuffer_ULONG(&Block->MemoryHistory);
    PhDeleteCircularBuffer_FLOAT(&Block->GpuHistory);

    if (EtFramesEnabled)
    {
        PhDeleteCircularBuffer_FLOAT(&Block->FramesPerSecondHistory);
        PhDeleteCircularBuffer_FLOAT(&Block->FramesLatencyHistory);
        PhDeleteCircularBuffer_FLOAT(&Block->FramesDisplayLatencyHistory);
        PhDeleteCircularBuffer_FLOAT(&Block->FramesMsBetweenPresentsHistory);
        PhDeleteCircularBuffer_FLOAT(&Block->FramesMsInPresentApiHistory);
        PhDeleteCircularBuffer_FLOAT(&Block->FramesMsUntilRenderCompleteHistory);
        PhDeleteCircularBuffer_FLOAT(&Block->FramesMsUntilDisplayedHistory);
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

VOID NTAPI ProcessNodeCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_PROCESS_NODE processNode = Object;
    PET_PROCESS_BLOCK block;

    if (block = EtGetProcessBlock(processNode->ProcessItem))
    {
        block->ProcessNode = processNode;
    }
}

VOID NTAPI ProcessNodeDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_PROCESS_NODE processNode = Object;
    PET_PROCESS_BLOCK block;

    if (block = EtGetProcessBlock(processNode->ProcessItem))
    {
        block->ProcessNode = NULL;
    }
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
            PH_SETTING_CREATE settings[] =
            {
                { StringSettingType, SETTING_NAME_DISK_TREE_LIST_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_DISK_TREE_LIST_SORT, L"4,2" }, // 4, DescendingSortOrder
                { IntegerSettingType, SETTING_NAME_ENABLE_GPUPERFCOUNTERS, L"1" },
                { IntegerSettingType, SETTING_NAME_ENABLE_DISKPERFCOUNTERS, L"1" },
                { IntegerSettingType, SETTING_NAME_ENABLE_ETW_MONITOR, L"1" },
                { IntegerSettingType, SETTING_NAME_ENABLE_GPU_MONITOR, L"1" },
                { IntegerSettingType, SETTING_NAME_ENABLE_FPS_MONITOR, L"0" },
                { IntegerSettingType, SETTING_NAME_ENABLE_SYSINFO_GRAPHS, L"1" },
                { IntegerPairSettingType, SETTING_NAME_UNLOADED_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_UNLOADED_WINDOW_SIZE, L"@96|350,270" },
                { StringSettingType, SETTING_NAME_UNLOADED_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_MODULE_SERVICES_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_MODULE_SERVICES_WINDOW_SIZE, L"@96|850,490" },
                { StringSettingType, SETTING_NAME_MODULE_SERVICES_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_GPU_NODES_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_GPU_NODES_WINDOW_SIZE, L"@96|850,490" },
                { IntegerPairSettingType, SETTING_NAME_WSWATCH_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_WSWATCH_WINDOW_SIZE, L"@96|325,266" },
                { StringSettingType, SETTING_NAME_WSWATCH_COLUMNS, L"" },
                { StringSettingType, SETTING_NAME_TRAYICON_GUIDS, L"" },
                { IntegerSettingType, SETTING_NAME_ENABLE_FAHRENHEIT, L"0" },
                { StringSettingType, SETTING_NAME_FW_TREE_LIST_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_FW_TREE_LIST_SORT, L"12,2" },
                { IntegerSettingType, SETTING_NAME_FW_IGNORE_PORTSCAN, L"0" },
                { IntegerSettingType, SETTING_NAME_FW_IGNORE_LOOPBACK, L"1" },
                { IntegerSettingType, SETTING_NAME_FW_IGNORE_ALLOW, L"0" },
                { IntegerSettingType, SETTING_NAME_SHOWSYSINFOGRAPH, L"1" },
                { StringSettingType, SETTING_NAME_WCT_TREE_LIST_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_WCT_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_WCT_WINDOW_SIZE, L"@96|690,540" },
                { IntegerPairSettingType, SETTING_NAME_REPARSE_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_REPARSE_WINDOW_SIZE, L"@96|510,380" },
                { StringSettingType, SETTING_NAME_REPARSE_LISTVIEW_COLUMNS, L"" },
                { StringSettingType, SETTING_NAME_REPARSE_OBJECTID_LISTVIEW_COLUMNS, L"" },
                { StringSettingType, SETTING_NAME_REPARSE_SD_LISTVIEW_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_PIPE_ENUM_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_PIPE_ENUM_WINDOW_SIZE, L"@96|510,380" },
                { StringSettingType, SETTING_NAME_PIPE_ENUM_LISTVIEW_COLUMNS, L"" },
                { StringSettingType, SETTING_NAME_PIPE_ENUM_LISTVIEW_COLUMNS_WITH_KPH, L"" },
                { IntegerPairSettingType, SETTING_NAME_FIRMWARE_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_FIRMWARE_WINDOW_SIZE, L"@96|490,340" },
                { StringSettingType, SETTING_NAME_FIRMWARE_LISTVIEW_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_OBJMGR_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_OBJMGR_WINDOW_SIZE, L"@96|1065,627" },
                { StringSettingType, SETTING_NAME_OBJMGR_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_POOL_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_POOL_WINDOW_SIZE, L"@96|510,380" },
                { StringSettingType, SETTING_NAME_POOL_TREE_LIST_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_POOL_TREE_LIST_SORT, L"0,0" },
                { IntegerPairSettingType, SETTING_NAME_BIGPOOL_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_BIGPOOL_WINDOW_SIZE, L"@96|510,380" },
                { IntegerPairSettingType, SETTING_NAME_TPM_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_TPM_WINDOW_SIZE, L"@96|490,340" },
                { StringSettingType, SETTING_NAME_TPM_LISTVIEW_COLUMNS, L"" },
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Extended Tools";
            info->Author = L"dmex, wj32";
            info->Description = L"Extended functionality for Windows 7 and above, including ETW, GPU, Disk and Firewall monitoring tabs.";
            info->Interface = &PluginInterface;

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
                PhGetGeneralCallback(GeneralCallbackOptionsWindowInitializing),
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
                PhGetPluginCallback(PluginInstance, PluginCallbackPhSvcRequest),
                PhSvcRequestCallback,
                NULL,
                &PluginPhSvcRequestCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainMenuInitializing),
                MainMenuInitializingCallback,
                NULL,
                &MainMenuInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessesUpdated),
                ProcessesUpdatedCallback,
                NULL,
                &ProcessesUpdatedCallbackRegistration
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
                PhGetGeneralCallback(GeneralCallbackTrayIconsInitializing),
                TrayIconsInitializingCallback,
                NULL,
                &TrayIconsInitializingCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                ProcessItemsUpdatedCallback,
                NULL,
                &ProcessItemsUpdatedCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackNetworkProviderUpdatedEvent),
                NetworkItemsUpdatedCallback,
                NULL,
                &NetworkItemsUpdatedCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessStatsNotifyEvent),
                ProcessStatsEventCallback,
                NULL,
                &ProcessStatsEventCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackSettingsUpdated),
                SettingsUpdatedCallback,
                NULL,
                &SettingsUpdatedCallbackRegistration
                );

            PhPluginSetObjectExtension(
                PluginInstance,
                EmProcessItemType,
                sizeof(ET_PROCESS_BLOCK),
                ProcessItemCreateCallback,
                ProcessItemDeleteCallback
                );
            PhPluginSetObjectExtension(
                PluginInstance,
                EmProcessNodeType,
                0,
                ProcessNodeCreateCallback,
                ProcessNodeDeleteCallback
                );
            PhPluginSetObjectExtension(
                PluginInstance,
                EmNetworkItemType,
                sizeof(ET_NETWORK_BLOCK),
                NetworkItemCreateCallback,
                NetworkItemDeleteCallback
                );

            PhAddSettings(settings, RTL_NUMBER_OF(settings));
        }
        break;
    }

    return TRUE;
}
