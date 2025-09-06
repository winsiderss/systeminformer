/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2018-2024
 *
 */

#include "exttools.h"
#include "extension\plugin.h"
#include <trace.h>

PPH_PLUGIN PluginInstance = NULL;
HWND ProcessTreeNewHandle = NULL;
HWND NetworkTreeNewHandle = NULL;
RTL_STATIC_LIST_HEAD(EtProcessBlockListHead);
RTL_STATIC_LIST_HEAD(EtNetworkBlockListHead);
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
PH_CALLBACK_REGISTRATION HandlePropertiesWindowInitializedCallbackRegistration;
PH_CALLBACK_REGISTRATION HandlePropertiesWindowUninitializingCallbackRegistration;
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
BOOLEAN EtGpuFahrenheitEnabled = FALSE;
BOOLEAN EtNpuFahrenheitEnabled = FALSE;
ULONG EtSampleCount = 0;
ULONG ProcessesUpdatedCount = 0;
static HANDLE ModuleProcessId = NULL;
ULONG EtUpdateInterval = 0;
USHORT EtMaxPrecisionUnit = 2;
BOOLEAN EtGraphShowText = FALSE;
BOOLEAN EtEnableScaleGraph = FALSE;
BOOLEAN EtEnableScaleText = FALSE;
BOOLEAN EtPropagateCpuUsage = FALSE;
BOOLEAN EtEnableAvxSupport = FALSE;

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    EtWindowsVersion = PhWindowsVersion;
    EtIsExecutingInWow64 = PhIsExecutingInWow64();
    EtSampleCount = PhGetIntegerSetting(L"SampleCount");

    EtLoadSettings();

    EtEtwStatisticsInitialization();
    EtGpuMonitorInitialization();
    EtNpuMonitorInitialization();
    EtFramesMonitorInitialization();
}

_Function_class_(PH_CALLBACK_FUNCTION)
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

_Function_class_(PH_CALLBACK_FUNCTION)
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

_Function_class_(PH_CALLBACK_FUNCTION)
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
    case ID_SMBIOS:
        {
            EtShowSMBIOSDialog(menuItem->OwnerWindow);
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

_Function_class_(PH_CALLBACK_FUNCTION)
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

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI PhSvcRequestCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (!Parameter)
        return;

    DispatchPhSvcRequest(Parameter);
}

_Function_class_(PH_CALLBACK_FUNCTION)
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
    PhInsertEMenuItem(systemMenu, PhPluginCreateEMenuItem(PluginInstance, 0, ID_SMBIOS, L"SM&BIOS", NULL), ULONG_MAX);
    PhInsertEMenuItem(systemMenu, bootMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_FIRMWARE, L"Firm&ware Table", NULL), ULONG_MAX);
    PhInsertEMenuItem(systemMenu, tpmMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ID_TPM, L"&Trusted Platform Module", NULL), ULONG_MAX);
    PhInsertEMenuItem(systemMenu, PhPluginCreateEMenuItem(PluginInstance, 0, ID_PIPE_ENUM, L"&Named Pipes", NULL), ULONG_MAX);
    PhInsertEMenuItem(systemMenu, reparsePointsMenu = PhPluginCreateEMenuItem(PluginInstance, 0, ID_REPARSE_POINTS, L"NTFS Reparse Points", NULL), ULONG_MAX);
    PhInsertEMenuItem(systemMenu, reparseObjIdMenu = PhPluginCreateEMenuItem(PluginInstance, 0, ID_REPARSE_OBJID, L"NTFS Object Identifiers", NULL), ULONG_MAX);
    PhInsertEMenuItem(systemMenu, reparseSsdlMenu = PhPluginCreateEMenuItem(PluginInstance, 0, ID_REPARSE_SDDL, L"NTFS Security Descriptors", NULL), ULONG_MAX);

    if (!PhGetOwnTokenAttributes().Elevated)
    {
        bootMenuItem->Flags |= PH_EMENU_DISABLED;
        reparsePointsMenu->Flags |= PH_EMENU_DISABLED;
        reparseObjIdMenu->Flags |= PH_EMENU_DISABLED;
        reparseSsdlMenu->Flags |= PH_EMENU_DISABLED;
    }

    if (EtWindowsVersion < WINDOWS_8 || !EtTpmIsReady())
    {
        tpmMenuItem->Flags |= PH_EMENU_DISABLED;
    }
}

_Function_class_(PH_CALLBACK_FUNCTION)
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

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (ProcessesUpdatedCount != 3)
    {
        ProcessesUpdatedCount++;
    }
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ProcessPropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (Parameter)
    {
        EtProcessGpuPropertiesInitializing(Parameter);
        EtProcessNpuPropertiesInitializing(Parameter);
        EtProcessFramesPropertiesInitializing(Parameter);
        EtProcessEtwPropertiesInitializing(Parameter);
    }
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI HandlePropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (Parameter)
        EtHandlePropertiesInitializing(Parameter);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI HandlePropertiesWindowInitializedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (Parameter)
        EtHandlePropertiesWindowInitialized(Parameter);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI HandlePropertiesWindowUninitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (Parameter)
        EtHandlePropertiesWindowUninitializing(Parameter);
}

_Function_class_(PH_CALLBACK_FUNCTION)
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

_Function_class_(PH_CALLBACK_FUNCTION)
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

_Function_class_(PH_CALLBACK_FUNCTION)
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

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ProcessTreeNewInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION treeNewInfo = Parameter;

    ProcessTreeNewHandle = treeNewInfo->TreeNewHandle;
    EtProcessTreeNewInitializing(Parameter);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI NetworkTreeNewInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION treeNewInfo = Parameter;

    NetworkTreeNewHandle = treeNewInfo->TreeNewHandle;
    EtNetworkTreeNewInitializing(Parameter);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI SystemInformationInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (EtGpuEnabled)
        EtGpuSystemInformationInitializing(Parameter);
    if (EtNpuEnabled)
        EtNpuSystemInformationInitializing(Parameter);
    if (EtEtwEnabled && PhGetIntegerSetting(SETTING_NAME_ENABLE_SYSINFO_GRAPHS))
        EtEtwSystemInformationInitializing(Parameter);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI MiniInformationInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (EtGpuEnabled)
        EtGpuMiniInformationInitializing(Parameter);
    if (EtNpuEnabled)
        EtNpuMiniInformationInitializing(Parameter);
    if (EtEtwEnabled)
        EtEtwMiniInformationInitializing(Parameter);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI TrayIconsInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    EtLoadTrayIconGuids();
    EtRegisterNotifyIcons(Parameter);
}

_Function_class_(PH_CALLBACK_FUNCTION)
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

_Function_class_(PH_CALLBACK_FUNCTION)
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

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ProcessStatsEventCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_PROCESS_STATS_EVENT event = Parameter;

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

            block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_NPU] = PhAddListViewGroup(
                listViewHandle, (INT)ListView_GetGroupCount(listViewHandle), L"NPU");
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NPUTOTALDEDICATED] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_NPU], MAXINT, L"Dedicated memory", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NPUTOTALDEDICATED],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NPUTOTALDEDICATED]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NPUTOTALSHARED] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_NPU], MAXINT, L"Shared memory", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NPUTOTALSHARED],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NPUTOTALSHARED]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NPUTOTALCOMMIT] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_NPU], MAXINT, L"Commit memory", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NPUTOTALCOMMIT],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NPUTOTALCOMMIT]));
            block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NPUTOTAL] = PhAddListViewGroupItem(
                listViewHandle, block->ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_NPU], MAXINT, L"Total memory", NULL);
            PhSetListViewItemParam(listViewHandle, block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NPUTOTAL],
                UlongToPtr(block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NPUTOTAL]));
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
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->GpuDedicatedUsage);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTALSHARED])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->GpuSharedUsage);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTALCOMMIT])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->GpuCommitUsage);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_GPUTOTAL])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->GpuDedicatedUsage + block->GpuSharedUsage + block->GpuCommitUsage);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKREADS])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatI64U(&format[0], block->DiskReadCount);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKREADBYTES])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->DiskReadRawDelta.Value);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKREADBYTESDELTA])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->DiskReadRawDelta.Delta);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKWRITES])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatI64U(&format[0], block->DiskWriteCount);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKWRITEBYTES])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->DiskWriteRawDelta.Value);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKWRITEBYTESDELTA])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->DiskWriteRawDelta.Delta);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKTOTAL])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatI64U(&format[0], block->DiskReadCount + block->DiskWriteCount);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKTOTALBYTES])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->DiskReadRawDelta.Value + block->DiskWriteRawDelta.Value);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_DISKTOTALBYTESDELTA])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->DiskReadRawDelta.Delta + block->DiskWriteRawDelta.Delta);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKREADS])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatI64U(&format[0], block->NetworkReceiveCount);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKREADBYTES])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->NetworkReceiveRawDelta.Value);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKREADBYTESDELTA])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->NetworkReceiveRawDelta.Delta);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKWRITES])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatI64U(&format[0], block->NetworkSendCount);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKWRITEBYTES])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->NetworkSendRawDelta.Value);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKWRITEBYTESDELTA])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->NetworkSendRawDelta.Delta);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKTOTAL])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatI64U(&format[0], block->NetworkReceiveCount + block->NetworkSendCount);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKTOTALBYTES])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->NetworkReceiveRawDelta.Value + block->NetworkSendRawDelta.Value);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NETWORKTOTALBYTESDELTA])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->NetworkReceiveRawDelta.Delta + block->NetworkSendRawDelta.Delta);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NPUTOTALDEDICATED])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->NpuDedicatedUsage);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NPUTOTALSHARED])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->NpuSharedUsage);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NPUTOTALCOMMIT])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->NpuCommitUsage);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                    else if (index == block->ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_NPUTOTAL])
                    {
                        PH_FORMAT format[1];
                        WCHAR buffer[PH_INT64_STR_LEN_1];

                        PhInitFormatSize(&format[0], block->NpuDedicatedUsage + block->NpuSharedUsage + block->NpuCommitUsage);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                        {
                            wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                        }
                    }
                }
            }
        }
        break;
    }
}

VOID EtLoadSettingsFirstRun(
    VOID
    )
{
    if (PhGetIntegerSetting(SETTING_NAME_FIRST_RUN))
    {
        if (PhGetUserLocaleInfoBool(LOCALE_IMEASURE))
        {
            PhSetIntegerSetting(SETTING_NAME_ENABLE_FAHRENHEIT, TRUE);
        }

        PhSetIntegerSetting(SETTING_NAME_FIRST_RUN, FALSE);
    }
}

VOID EtLoadSettings(
    VOID
    )
{
    EtLoadSettingsFirstRun();

    EtUpdateInterval = PhGetIntegerSetting(L"UpdateInterval");
    EtMaxPrecisionUnit = (USHORT)PhGetIntegerSetting(L"MaxPrecisionUnit");
    EtGraphShowText = !!PhGetIntegerSetting(L"GraphShowText");
    EtEnableScaleGraph = !!PhGetIntegerSetting(L"EnableGraphMaxScale");
    EtEnableScaleText = !!PhGetIntegerSetting(L"EnableGraphMaxText");
    EtPropagateCpuUsage = !!PhGetIntegerSetting(L"PropagateCpuUsage");
    EtEnableAvxSupport = !!PhGetIntegerSetting(L"EnableAvxSupport");
    EtTrayIconTransparencyEnabled = !!PhGetIntegerSetting(L"IconTransparencyEnabled");
    EtGpuFahrenheitEnabled = !!PhGetIntegerSetting(SETTING_NAME_ENABLE_FAHRENHEIT);
    EtNpuFahrenheitEnabled = EtGpuFahrenheitEnabled;
}

_Function_class_(PH_CALLBACK_FUNCTION)
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
    memset(Block, 0, sizeof(ET_PROCESS_BLOCK));
    Block->ProcessItem = ProcessItem;
    PhInitializeQueuedLock(&Block->TextCacheLock);

    PhInitializeCircularBuffer_ULONG64(&Block->DiskReadHistory, EtSampleCount);
    PhInitializeCircularBuffer_ULONG64(&Block->DiskWriteHistory, EtSampleCount);
    PhInitializeCircularBuffer_ULONG64(&Block->NetworkSendHistory, EtSampleCount);
    PhInitializeCircularBuffer_ULONG64(&Block->NetworkReceiveHistory, EtSampleCount);

    PhInitializeCircularBuffer_FLOAT(&Block->GpuHistory, EtSampleCount);
    PhInitializeCircularBuffer_ULONG(&Block->GpuMemoryHistory, EtSampleCount);
    PhInitializeCircularBuffer_ULONG(&Block->GpuMemorySharedHistory, EtSampleCount);
    PhInitializeCircularBuffer_ULONG(&Block->GpuCommittedHistory, EtSampleCount);

    //Block->GpuTotalRunningTimeDelta = PhAllocate(sizeof(PH_UINT64_DELTA) * EtGpuTotalNodeCount);
    //memset(Block->GpuTotalRunningTimeDelta, 0, sizeof(PH_UINT64_DELTA) * EtGpuTotalNodeCount);
    //Block->GpuTotalNodesHistory = PhAllocate(sizeof(PH_CIRCULAR_BUFFER_FLOAT) * EtGpuTotalNodeCount);

    PhInitializeCircularBuffer_FLOAT(&Block->NpuHistory, EtSampleCount);
    PhInitializeCircularBuffer_ULONG(&Block->NpuMemoryHistory, EtSampleCount);
    PhInitializeCircularBuffer_ULONG(&Block->NpuMemorySharedHistory, EtSampleCount);
    PhInitializeCircularBuffer_ULONG(&Block->NpuCommittedHistory, EtSampleCount);

    //Block->GpuTotalRunningTimeDelta = PhAllocate(sizeof(PH_UINT64_DELTA) * EtNpuTotalNodeCount);
    //memset(Block->GpuTotalRunningTimeDelta, 0, sizeof(PH_UINT64_DELTA) * EtNpuTotalNodeCount);
    //Block->GpuTotalNodesHistory = PhAllocate(sizeof(PH_CIRCULAR_BUFFER_FLOAT) * EtNpuTotalNodeCount);

    if (EtFramesEnabled)
    {
        PhInitializeCircularBuffer_FLOAT(&Block->FramesPerSecondHistory, EtSampleCount);
        PhInitializeCircularBuffer_FLOAT(&Block->FramesLatencyHistory, EtSampleCount);
        PhInitializeCircularBuffer_FLOAT(&Block->FramesDisplayLatencyHistory, EtSampleCount);
        PhInitializeCircularBuffer_FLOAT(&Block->FramesMsBetweenPresentsHistory, EtSampleCount);
        PhInitializeCircularBuffer_FLOAT(&Block->FramesMsInPresentApiHistory, EtSampleCount);
        PhInitializeCircularBuffer_FLOAT(&Block->FramesMsUntilRenderCompleteHistory, EtSampleCount);
        PhInitializeCircularBuffer_FLOAT(&Block->FramesMsUntilDisplayedHistory, EtSampleCount);
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
    PhDeleteCircularBuffer_ULONG(&Block->GpuMemorySharedHistory);
    PhDeleteCircularBuffer_ULONG(&Block->GpuMemoryHistory);
    PhDeleteCircularBuffer_FLOAT(&Block->GpuHistory);

    PhDeleteCircularBuffer_ULONG(&Block->NpuCommittedHistory);
    PhDeleteCircularBuffer_ULONG(&Block->NpuMemorySharedHistory);
    PhDeleteCircularBuffer_ULONG(&Block->NpuMemoryHistory);
    PhDeleteCircularBuffer_FLOAT(&Block->NpuHistory);

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
                { IntegerSettingType, SETTING_NAME_FIRST_RUN, L"1" },
                { StringSettingType, SETTING_NAME_DISK_TREE_LIST_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_DISK_TREE_LIST_SORT, L"4,2" }, // 4, DescendingSortOrder
                { IntegerSettingType, SETTING_NAME_ENABLE_GPUPERFCOUNTERS, L"1" },
                { IntegerSettingType, SETTING_NAME_ENABLE_NPUPERFCOUNTERS, L"1" },
                { IntegerSettingType, SETTING_NAME_ENABLE_DISKPERFCOUNTERS, L"1" },
                { IntegerSettingType, SETTING_NAME_ENABLE_ETW_MONITOR, L"1" },
                { IntegerSettingType, SETTING_NAME_ENABLE_GPU_MONITOR, L"1" },
                { IntegerSettingType, SETTING_NAME_ENABLE_NPU_MONITOR, L"1" },
                { IntegerSettingType, SETTING_NAME_ENABLE_FPS_MONITOR, L"0" },
                { IntegerSettingType, SETTING_NAME_ENABLE_SYSINFO_GRAPHS, L"1" },
                { IntegerPairSettingType, SETTING_NAME_UNLOADED_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_UNLOADED_WINDOW_SIZE, L"@96|670,470" },
                { StringSettingType, SETTING_NAME_UNLOADED_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_MODULE_SERVICES_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_MODULE_SERVICES_WINDOW_SIZE, L"@96|850,490" },
                { StringSettingType, SETTING_NAME_MODULE_SERVICES_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_GPU_DETAILS_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_GPU_DETAILS_WINDOW_SIZE, L"@96|850,490" },
                { IntegerPairSettingType, SETTING_NAME_GPU_NODES_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_GPU_NODES_WINDOW_SIZE, L"@96|850,490" },
                { IntegerPairSettingType, SETTING_NAME_NPU_NODES_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_NPU_NODES_WINDOW_SIZE, L"@96|850,490" },
                { IntegerPairSettingType, SETTING_NAME_WSWATCH_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_WSWATCH_WINDOW_SIZE, L"@96|325,266" },
                { StringSettingType, SETTING_NAME_WSWATCH_COLUMNS, L"" },
                { StringSettingType, SETTING_NAME_TRAYICON_GUIDS, L"" },
                { IntegerSettingType, SETTING_NAME_ENABLE_FAHRENHEIT, L"0" },
                { StringSettingType, SETTING_NAME_FW_TREE_LIST_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_FW_TREE_LIST_SORT, L"12,2" },
                { IntegerSettingType, SETTING_NAME_FW_IGNORE_PORTSCAN, L"0" },
                { IntegerSettingType, SETTING_NAME_FW_IGNORE_LOOPBACK, L"1" },
                { IntegerSettingType, SETTING_NAME_FW_IGNORE_ALLOW, L"1" },
                { StringSettingType, SETTING_NAME_FW_SESSION_GUID, L"" },
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
                { StringSettingType, SETTING_NAME_PIPE_ENUM_LISTVIEW_COLUMNS_WITH_KSI, L"" },
                { IntegerPairSettingType, SETTING_NAME_FIRMWARE_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_FIRMWARE_WINDOW_SIZE, L"@96|490,340" },
                { StringSettingType, SETTING_NAME_FIRMWARE_LISTVIEW_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_OBJMGR_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_OBJMGR_WINDOW_SIZE, L"@96|1237,627" },
                { StringSettingType, SETTING_NAME_OBJMGR_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_OBJMGR_LIST_SORT, L"0,0" },
                { IntegerPairSettingType, SETTING_NAME_OBJMGR_PROPERTIES_WINDOW_POSITION, L"0,0" },
                { StringSettingType, SETTING_NAME_OBJMGR_LAST_PATH, L"\\" },
                { StringSettingType, SETTING_NAME_OBJMGR_HISTORY,
                    L"\\" ET_OBJMGR_HISTORY_SEPARATOR
                    L"\\BaseNamedObjects" ET_OBJMGR_HISTORY_SEPARATOR
                    L"\\Device" ET_OBJMGR_HISTORY_SEPARATOR
                    L"\\KernelObjects" ET_OBJMGR_HISTORY_SEPARATOR
                    L"\\ObjectTypes" },
                { IntegerPairSettingType, SETTING_NAME_POOL_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_POOL_WINDOW_SIZE, L"@96|510,380" },
                { StringSettingType, SETTING_NAME_POOL_TREE_LIST_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_POOL_TREE_LIST_SORT, L"0,0" },
                { IntegerPairSettingType, SETTING_NAME_BIGPOOL_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_BIGPOOL_WINDOW_SIZE, L"@96|510,380" },
                { IntegerPairSettingType, SETTING_NAME_TPM_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_TPM_WINDOW_SIZE, L"@96|490,340" },
                { StringSettingType, SETTING_NAME_TPM_LISTVIEW_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_SMBIOS_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_SMBIOS_WINDOW_SIZE, L"@96|490,340" },
                { StringSettingType, SETTING_NAME_SMBIOS_INFO_COLUMNS, L"" },
                { IntegerSettingType, SETTING_NAME_SMBIOS_SHOW_UNDEFINED_TYPES, L"0" },
            };

            WPP_INIT_TRACING(PLUGIN_NAME);

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Extended Tools";
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
                PhGetGeneralCallback(GeneralCallbackHandlePropertiesWindowInitialized),
                HandlePropertiesWindowInitializedCallback,
                NULL,
                &HandlePropertiesWindowInitializedCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackHandlePropertiesWindowUninitializing),
                HandlePropertiesWindowUninitializingCallback,
                NULL,
                &HandlePropertiesWindowUninitializingCallbackRegistration
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
