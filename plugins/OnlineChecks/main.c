/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2012-2024
 *     jxy-s   2026
 *
 */

#include "onlnchk.h"

#include <trace.h>

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ModuleMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ServiceMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION TreeNewMessageCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ModulesTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ServiceTreeNewInitializingCallbackRegistration;

BOOLEAN ScanningInitialized = FALSE;
BOOLEAN AutoScanningEnabled = FALSE;
LIST_ENTRY ScanExtensionsListHead = { &ScanExtensionsListHead, &ScanExtensionsListHead };
PH_QUEUED_LOCK ScanExtensionsListLock = PH_QUEUED_LOCK_INIT;

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI LoadCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    AutoScanningEnabled = !!PhGetIntegerSetting(SETTING_NAME_AUTO_SCAN_ENABLED);
    ScanningInitialized = InitializeScanning();
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI UnloadCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    if (ScanningInitialized)
        CleanupScanning();
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    static const ULONG delay = 3;
    LARGE_INTEGER systemTime;

    if (!ScanningInitialized)
        return;

    if (PtrToUlong(Parameter) < delay)
        return;

    PhQuerySystemTime(&systemTime);

    PhAcquireQueuedLockExclusive(&ScanExtensionsListLock);

    for (PLIST_ENTRY listEntry = ScanExtensionsListHead.Flink;
         listEntry != &ScanExtensionsListHead;
         listEntry = listEntry->Flink)
    {
        PSCAN_EXTENSION extension;

        extension = CONTAINING_RECORD(listEntry, SCAN_EXTENSION, ListEntry);

        if (!extension->FileName)
        {
            PPH_STRING fileName = NULL;

            switch (extension->Type)
            {
            case SCAN_EXTENSION_PROCESS:
                if (extension->ProcessItem->FileName)
                    fileName = PhGetFileName(extension->ProcessItem->FileName);
                break;
            case SCAN_EXTENSION_MODULE:
                if (extension->ModuleItem->FileName)
                    fileName = PhGetFileName(extension->ModuleItem->FileName);
                break;
            case SCAN_EXTENSION_SERVICE:
                if ((PtrToUlong(Parameter) > delay) && extension->ServiceItem->FileName)
                    fileName = PhGetFileName(extension->ServiceItem->FileName);
                break;
            }

            if (!PhIsNullOrEmptyString(fileName))
                extension->FileName = PhReferenceObject(fileName);

            PhClearReference(&fileName);
        }

        if (!extension->FileName)
            continue;

        EvaluateScanContext(
            &systemTime,
            &extension->ScanContext,
            extension->FileName,
            AutoScanningEnabled ? 0 : SCAN_FLAG_LOCAL_ONLY
            );
    }

    PhReleaseQueuedLockExclusive(&ScanExtensionsListLock);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ShowOptionsCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_OPTIONS_POINTERS optionsEntry = (PPH_PLUGIN_OPTIONS_POINTERS)Parameter;

    optionsEntry->CreateSection(
        L"OnlineChecks",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        OptionsDlgProc,
        NULL
        );
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI MenuItemCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;
    PSCAN_EXTENSION extension = NULL;
    SCAN_TYPE scanType = 0;
    PPH_STRING fileName = NULL;

    switch (menuItem->Id)
    {
    case MENUITEM_VIRUSTOTAL_UPLOAD:
        UploadToOnlineService(menuItem->Context, MENUITEM_VIRUSTOTAL_UPLOAD);
        break;
    case MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE:
        UploadServiceToOnlineService(menuItem->OwnerWindow, menuItem->Context, MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE);
        break;
    case MENUITEM_JOTTI_UPLOAD:
        UploadToOnlineService(menuItem->Context, MENUITEM_JOTTI_UPLOAD);
        break;
    case MENUITEM_JOTTI_UPLOAD_SERVICE:
        UploadServiceToOnlineService(menuItem->OwnerWindow, menuItem->Context, MENUITEM_JOTTI_UPLOAD_SERVICE);
        break;
    case MENUITEM_HYBRIDANALYSIS_UPLOAD:
        UploadToOnlineService(menuItem->Context, MENUITEM_HYBRIDANALYSIS_UPLOAD);
        break;
    case MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE:
        UploadServiceToOnlineService(menuItem->OwnerWindow, menuItem->Context, MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE);
        break;
    case MENUITEM_FILESCANIO_UPLOAD:
        UploadToOnlineService(menuItem->Context, MENUITEM_FILESCANIO_UPLOAD);
        break;
    case MENUITEM_FILESCANIO_UPLOAD_SERVICE:
        UploadServiceToOnlineService(menuItem->OwnerWindow, menuItem->Context, MENUITEM_FILESCANIO_UPLOAD_SERVICE);
        break;
    case MENUITEM_VIRUSTOTAL_UPLOAD_FILE:
    case MENUITEM_HYBRIDANALYSIS_UPLOAD_FILE:
    case MENUITEM_JOTTI_UPLOAD_FILE:
    case MENUITEM_FILESCANIO_UPLOAD_FILE:
        {
            static PH_FILETYPE_FILTER filters[] =
            {
                { L"All files (*.*)", L"*.*" }
            };
            PVOID fileDialog;
            PPH_STRING fileName;

            fileDialog = PhCreateOpenFileDialog();
            PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

            if (PhShowFileDialog(menuItem->Context, fileDialog))
            {
                fileName = PH_AUTO(PhGetFileDialogFileName(fileDialog));

                switch (menuItem->Id)
                {
                case MENUITEM_VIRUSTOTAL_UPLOAD_FILE:
                    UploadToOnlineService(fileName, MENUITEM_VIRUSTOTAL_UPLOAD);
                    break;
                case MENUITEM_HYBRIDANALYSIS_UPLOAD_FILE:
                    UploadToOnlineService(fileName, MENUITEM_HYBRIDANALYSIS_UPLOAD);
                    break;
                case MENUITEM_JOTTI_UPLOAD_FILE:
                    UploadToOnlineService(fileName, MENUITEM_JOTTI_UPLOAD);
                    break;
                case MENUITEM_FILESCANIO_UPLOAD_FILE:
                    UploadToOnlineService(fileName, MENUITEM_FILESCANIO_UPLOAD);
                    break;
                }
            }

            PhFreeFileDialog(fileDialog);
        }
        break;
    case MENUITEM_VIRUSTOTAL_SCAN_PROCESS:
        {
            extension = PhPluginGetObjectExtension(PluginInstance, menuItem->Context, EmProcessItemType);
            scanType = SCAN_TYPE_VIRUSTOTAL;
            if (extension->ProcessItem->FileName)
            {
                fileName = PhGetFileName(extension->ProcessItem->FileName);
                extension->ProcessItem->FileName = fileName;
            }
        }
        break;
    case MENUITEM_VIRUSTOTAL_SCAN_MODULE:
        {
            extension = PhPluginGetObjectExtension(PluginInstance, menuItem->Context, EmModuleItemType);
            scanType = SCAN_TYPE_VIRUSTOTAL;
            if (extension->ModuleItem->FileName)
            {
                fileName = PhGetFileName(extension->ModuleItem->FileName);
                extension->ModuleItem->FileName = fileName;
            }
        }
        break;
    case MENUITEM_VIRUSTOTAL_SCAN_SERVICE:
        {
            extension = PhPluginGetObjectExtension(PluginInstance, menuItem->Context, EmServiceItemType);
            scanType = SCAN_TYPE_VIRUSTOTAL;
            if (extension->ServiceItem->FileName)
            {
                fileName = PhGetFileName(extension->ServiceItem->FileName);
                extension->ServiceItem->FileName = fileName;
            }
        }
        break;
    case MENUITEM_HYBRIDANALYSIS_SCAN_PROCESS:
        {
            extension = PhPluginGetObjectExtension(PluginInstance, menuItem->Context, EmProcessItemType);
            scanType = SCAN_TYPE_HYBRIDANALYSIS;
            if (extension->ProcessItem->FileName)
            {
                fileName = PhGetFileName(extension->ProcessItem->FileName);
                extension->ProcessItem->FileName = fileName;
            }
        }
        break;
    case MENUITEM_HYBRIDANALYSIS_SCAN_MODULE:
        {
            extension = PhPluginGetObjectExtension(PluginInstance, menuItem->Context, EmModuleItemType);
            scanType = SCAN_TYPE_HYBRIDANALYSIS;
            if (extension->ModuleItem->FileName)
            {
                fileName = PhGetFileName(extension->ModuleItem->FileName);
                extension->ModuleItem->FileName = fileName;
            }
        }
        break;
    case MENUITEM_HYBRIDANALYSIS_SCAN_SERVICE:
        {
            extension = PhPluginGetObjectExtension(PluginInstance, menuItem->Context, EmServiceItemType);
            scanType = SCAN_TYPE_HYBRIDANALYSIS;
            if (extension->ServiceItem->FileName)
            {
                fileName = PhGetFileName(extension->ServiceItem->FileName);
                extension->ServiceItem->FileName = fileName;
            }
        }
        break;
    }

    if (fileName)
        EnqueueScan(&extension->ScanContext, scanType, fileName, SCAN_FLAG_RESCAN);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI MainMenuInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_EMENU_ITEM onlineMenuItem;
    //PPH_EMENU_ITEM enableMenuItem;

    if (menuInfo->u.MainMenu.SubMenuIndex != PH_MENU_ITEM_LOCATION_TOOLS)
        return;

    onlineMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"&Online Checks", NULL);
    PhInsertEMenuItem(onlineMenuItem, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_FILESCANIO_UPLOAD_FILE, L"Upload file to &Filescan...", NULL), ULONG_MAX);
    PhInsertEMenuItem(onlineMenuItem, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_HYBRIDANALYSIS_UPLOAD_FILE, L"Upload file to &Hybrid-Analysis...", NULL), ULONG_MAX);
    PhInsertEMenuItem(onlineMenuItem, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_VIRUSTOTAL_UPLOAD_FILE, L"Upload file to &VirusTotal...", NULL), ULONG_MAX);
    PhInsertEMenuItem(onlineMenuItem, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_JOTTI_UPLOAD_FILE, L"Upload file to &Jotti...", NULL), ULONG_MAX);
    PhInsertEMenuItem(menuInfo->Menu, onlineMenuItem, ULONG_MAX);
}

PPH_EMENU_ITEM CreateSendToMenu(
    _In_ BOOLEAN ProcessesMenu,
    _In_ PPH_EMENU_ITEM Parent,
    _In_opt_ PPH_STRING FileName
    )
{
    PPH_EMENU_ITEM sendToMenu;
    PPH_EMENU_ITEM menuItem;
    ULONG insertIndex;

    sendToMenu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"Sen&d to", NULL);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_FILESCANIO_UPLOAD, L"&filescan.io", FileName), ULONG_MAX);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_HYBRIDANALYSIS_UPLOAD, L"&hybrid-analysis.com", FileName), ULONG_MAX);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_JOTTI_UPLOAD, L"virusscan.&jotti.org", FileName), ULONG_MAX);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_VIRUSTOTAL_UPLOAD, L"&virustotal.com", FileName), ULONG_MAX);

    if (ProcessesMenu && (menuItem = PhFindEMenuItem(Parent, 0, NULL, PHAPP_ID_PROCESS_SEARCHONLINE)))
    {
        insertIndex = PhIndexOfEMenuItem(Parent, menuItem);

        PhInsertEMenuItem(Parent, PhCreateEMenuSeparator(), insertIndex + 1);
        PhInsertEMenuItem(Parent, sendToMenu, insertIndex + 2);
    }
    else
    {
        PhInsertEMenuItem(Parent, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(Parent, sendToMenu, ULONG_MAX);
    }

    return sendToMenu;
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ProcessMenuInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_PROCESS_ITEM processItem;
    PPH_EMENU_ITEM sendToMenu;
    PPH_EMENU_ITEM scanWithMenu;
    PPH_EMENU_ITEM menuItem;

    if (menuInfo->u.Process.NumberOfProcesses == 1)
        processItem = menuInfo->u.Process.Processes[0];
    else
        processItem = NULL;

    sendToMenu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"Sen&d to", NULL);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_FILESCANIO_UPLOAD, L"&filescan.io", processItem ? processItem->FileName : NULL), ULONG_MAX);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_HYBRIDANALYSIS_UPLOAD, L"&hybrid-analysis.com", processItem ? processItem->FileName : NULL), ULONG_MAX);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_JOTTI_UPLOAD, L"virusscan.&jotti.org", processItem ? processItem->FileName : NULL), ULONG_MAX);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_VIRUSTOTAL_UPLOAD, L"&virustotal.com", processItem ? processItem->FileName : NULL), ULONG_MAX);
    scanWithMenu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"Scan with", NULL);
    PhInsertEMenuItem(scanWithMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_VIRUSTOTAL_SCAN_PROCESS, L"VirusTotal", processItem), ULONG_MAX);
    PhInsertEMenuItem(scanWithMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_HYBRIDANALYSIS_SCAN_PROCESS, L"Hybrid-Analysis", processItem), ULONG_MAX);

    if ((menuItem = PhFindEMenuItem(menuInfo->Menu, 0, NULL, PHAPP_ID_PROCESS_SEARCHONLINE)))
    {
        ULONG insertIndex = PhIndexOfEMenuItem(menuInfo->Menu, menuItem);

        PhInsertEMenuItem(menuInfo->Menu, PhCreateEMenuSeparator(), insertIndex + 1);
        PhInsertEMenuItem(menuInfo->Menu, sendToMenu, insertIndex + 2);
        PhInsertEMenuItem(menuInfo->Menu, scanWithMenu, insertIndex + 3);
    }
    else
    {
        PhInsertEMenuItem(menuInfo->Menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menuInfo->Menu, sendToMenu, ULONG_MAX);
        PhInsertEMenuItem(menuInfo->Menu, scanWithMenu, ULONG_MAX);
    }

    // Only enable the Send To menu if there is exactly one process selected and it has a file name.
    if (!processItem || !processItem->FileName)
    {
        sendToMenu->Flags |= PH_EMENU_DISABLED;
        scanWithMenu->Flags |= PH_EMENU_DISABLED;
    }

    if (!ScanningInitialized)
        scanWithMenu->Flags |= PH_EMENU_DISABLED;
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ModuleMenuInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_MODULE_ITEM moduleItem;
    PPH_EMENU_ITEM sendToMenu;
    PPH_EMENU_ITEM scanWithMenu;

    if (menuInfo->u.Module.NumberOfModules == 1)
        moduleItem = menuInfo->u.Module.Modules[0];
    else
        moduleItem = NULL;

    sendToMenu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"Sen&d to", NULL);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_FILESCANIO_UPLOAD, L"&filescan.io", moduleItem ? moduleItem->FileName : NULL), ULONG_MAX);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_HYBRIDANALYSIS_UPLOAD, L"&hybrid-analysis.com", moduleItem ? moduleItem->FileName : NULL), ULONG_MAX);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_JOTTI_UPLOAD, L"virusscan.&jotti.org", moduleItem ? moduleItem->FileName : NULL), ULONG_MAX);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_VIRUSTOTAL_UPLOAD, L"&virustotal.com", moduleItem ? moduleItem->FileName : NULL), ULONG_MAX);
    scanWithMenu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"Scan with", NULL);
    PhInsertEMenuItem(scanWithMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_VIRUSTOTAL_SCAN_MODULE, L"VirusTotal", moduleItem), ULONG_MAX);
    PhInsertEMenuItem(scanWithMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_HYBRIDANALYSIS_SCAN_MODULE, L"Hybrid-Analysis", moduleItem), ULONG_MAX);
    PhInsertEMenuItem(menuInfo->Menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menuInfo->Menu, sendToMenu, ULONG_MAX);
    PhInsertEMenuItem(menuInfo->Menu, scanWithMenu, ULONG_MAX);

    if (!moduleItem)
    {
        sendToMenu->Flags |= PH_EMENU_DISABLED;
        scanWithMenu->Flags |= PH_EMENU_DISABLED;
    }

    if (!ScanningInitialized)
        scanWithMenu->Flags |= PH_EMENU_DISABLED;
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ServiceMenuInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_SERVICE_ITEM serviceItem;
    PPH_EMENU_ITEM sendToMenu;
    PPH_EMENU_ITEM scanWithMenu;

    if (menuInfo->u.Service.NumberOfServices == 1)
        serviceItem = menuInfo->u.Service.Services[0];
    else
        serviceItem = NULL;

    sendToMenu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"Sen&d to", NULL);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_FILESCANIO_UPLOAD_SERVICE, L"&filescan.io", serviceItem), ULONG_MAX);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE, L"&hybrid-analysis.com", serviceItem), ULONG_MAX);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_JOTTI_UPLOAD_SERVICE, L"virusscan.&jotti.org", serviceItem), ULONG_MAX);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE, L"&virustotal.com", serviceItem), ULONG_MAX);
    scanWithMenu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"Scan with", NULL);
    PhInsertEMenuItem(scanWithMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_VIRUSTOTAL_SCAN_SERVICE, L"VirusTotal", serviceItem), ULONG_MAX);
    PhInsertEMenuItem(scanWithMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_HYBRIDANALYSIS_SCAN_SERVICE, L"Hybrid-Analysis", serviceItem), ULONG_MAX);
    PhInsertEMenuItem(menuInfo->Menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menuInfo->Menu, sendToMenu, ULONG_MAX);
    PhInsertEMenuItem(menuInfo->Menu, scanWithMenu, ULONG_MAX);

    if (!serviceItem)
    {
        sendToMenu->Flags |= PH_EMENU_DISABLED;
        scanWithMenu->Flags |= PH_EMENU_DISABLED;
    }

    if (!ScanningInitialized)
        scanWithMenu->Flags |= PH_EMENU_DISABLED;
}

VOID UpdateScanExtensionResult(
    _In_ PSCAN_EXTENSION Extension,
    _In_ SCAN_TYPE Type
    )
{
    PPH_STRING result = ReferenceScanResult(&Extension->ScanContext, Type);
    if (result != Extension->ScanResults[Type])
        PhMoveReference(&Extension->ScanResults[Type], result);
    else
        PhDereferenceObject(result);
}

PH_STRINGREF GetScanText(
    _In_ PSCAN_EXTENSION Extension,
    _In_ SCAN_TYPE Type
    )
{
    static PH_STRINGREF scanningDisabled = PH_STRINGREF_INIT(L"Scanning disabled");

    if (!ScanningInitialized)
        return scanningDisabled;

    UpdateScanExtensionResult(Extension, Type);
    return Extension->ScanResults[Type]->sr;
}

_Function_class_(PH_PLUGIN_TREENEW_SORT_FUNCTION)
LONG NTAPI VirusTotalProcessNodeSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PVOID Context
    )
{
    PPH_PROCESS_NODE node1 = Node1;
    PPH_PROCESS_NODE node2 = Node2;
    PSCAN_EXTENSION extension1 = PhPluginGetObjectExtension(PluginInstance, node1->ProcessItem, EmProcessItemType);
    PSCAN_EXTENSION extension2 = PhPluginGetObjectExtension(PluginInstance, node2->ProcessItem, EmProcessItemType);
    PH_STRINGREF string1 = GetScanText(extension1, SCAN_TYPE_VIRUSTOTAL);
    PH_STRINGREF string2 = GetScanText(extension2, SCAN_TYPE_VIRUSTOTAL);

    return PhCompareStringRef(&string1, &string2, FALSE);
}

_Function_class_(PH_PLUGIN_TREENEW_SORT_FUNCTION)
LONG NTAPI VirusTotalModuleNodeSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PVOID Context
    )
{
    PPH_MODULE_NODE node1 = Node1;
    PPH_MODULE_NODE node2 = Node2;
    PSCAN_EXTENSION extension1 = PhPluginGetObjectExtension(PluginInstance, node1->ModuleItem, EmModuleItemType);
    PSCAN_EXTENSION extension2 = PhPluginGetObjectExtension(PluginInstance, node2->ModuleItem, EmModuleItemType);
    PH_STRINGREF string1 = GetScanText(extension1, SCAN_TYPE_VIRUSTOTAL);
    PH_STRINGREF string2 = GetScanText(extension2, SCAN_TYPE_VIRUSTOTAL);

    return PhCompareStringRef(&string1, &string2, FALSE);
}

_Function_class_(PH_PLUGIN_TREENEW_SORT_FUNCTION)
LONG NTAPI VirusTotalServiceNodeSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PVOID Context
    )
{
    PPH_SERVICE_NODE node1 = Node1;
    PPH_SERVICE_NODE node2 = Node2;
    PSCAN_EXTENSION extension1 = PhPluginGetObjectExtension(PluginInstance, node1->ServiceItem, EmServiceItemType);
    PSCAN_EXTENSION extension2 = PhPluginGetObjectExtension(PluginInstance, node2->ServiceItem, EmServiceItemType);
    PH_STRINGREF string1 = GetScanText(extension1, SCAN_TYPE_VIRUSTOTAL);
    PH_STRINGREF string2 = GetScanText(extension2, SCAN_TYPE_VIRUSTOTAL);

    return PhCompareStringRef(&string1, &string2, FALSE);
}

_Function_class_(PH_PLUGIN_TREENEW_SORT_FUNCTION)
LONG NTAPI HybridAnalysisProcessNodeSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PVOID Context
    )
{
    PPH_PROCESS_NODE node1 = Node1;
    PPH_PROCESS_NODE node2 = Node2;
    PSCAN_EXTENSION extension1 = PhPluginGetObjectExtension(PluginInstance, node1->ProcessItem, EmProcessItemType);
    PSCAN_EXTENSION extension2 = PhPluginGetObjectExtension(PluginInstance, node2->ProcessItem, EmProcessItemType);
    PH_STRINGREF string1 = GetScanText(extension1, SCAN_TYPE_HYBRIDANALYSIS);
    PH_STRINGREF string2 = GetScanText(extension2, SCAN_TYPE_HYBRIDANALYSIS);

    return PhCompareStringRef(&string1, &string2, FALSE);
}

_Function_class_(PH_PLUGIN_TREENEW_SORT_FUNCTION)
LONG NTAPI HybridAnalysisModuleNodeSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PVOID Context
    )
{
    PPH_MODULE_NODE node1 = Node1;
    PPH_MODULE_NODE node2 = Node2;
    PSCAN_EXTENSION extension1 = PhPluginGetObjectExtension(PluginInstance, node1->ModuleItem, EmModuleItemType);
    PSCAN_EXTENSION extension2 = PhPluginGetObjectExtension(PluginInstance, node2->ModuleItem, EmModuleItemType);
    PH_STRINGREF string1 = GetScanText(extension1, SCAN_TYPE_HYBRIDANALYSIS);
    PH_STRINGREF string2 = GetScanText(extension2, SCAN_TYPE_HYBRIDANALYSIS);

    return PhCompareStringRef(&string1, &string2, FALSE);
}

_Function_class_(PH_PLUGIN_TREENEW_SORT_FUNCTION)
LONG NTAPI HybridAnalysisServiceNodeSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PVOID Context
    )
{
    PPH_SERVICE_NODE node1 = Node1;
    PPH_SERVICE_NODE node2 = Node2;
    PSCAN_EXTENSION extension1 = PhPluginGetObjectExtension(PluginInstance, node1->ServiceItem, EmServiceItemType);
    PSCAN_EXTENSION extension2 = PhPluginGetObjectExtension(PluginInstance, node2->ServiceItem, EmServiceItemType);
    PH_STRINGREF string1 = GetScanText(extension1, SCAN_TYPE_HYBRIDANALYSIS);
    PH_STRINGREF string2 = GetScanText(extension2, SCAN_TYPE_HYBRIDANALYSIS);

    return PhCompareStringRef(&string1, &string2, FALSE);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ProcessTreeNewInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PH_TREENEW_COLUMN column;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"VirusTotal";
    column.Width = 140;
    column.Alignment = PH_ALIGN_CENTER;
    column.CustomDraw = TRUE;
    column.Context = info->TreeNewHandle; // Context

    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, COLUMN_ID_VIUSTOTAL_PROCESS, NULL, VirusTotalProcessNodeSortFunction);

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Hybrid-Analysis";
    column.Width = 140;
    column.Alignment = PH_ALIGN_CENTER;
    column.CustomDraw = TRUE;
    column.Context = info->TreeNewHandle; // Context

    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, COLUMN_ID_HYBRIDANALYSIS_PROCESS, NULL, HybridAnalysisProcessNodeSortFunction);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ModuleTreeNewInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PH_TREENEW_COLUMN column;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"VirusTotal";
    column.Width = 140;
    column.Alignment = PH_ALIGN_CENTER;
    column.CustomDraw = TRUE;
    column.Context = info->TreeNewHandle; // Context

    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, COLUMN_ID_VIUSTOTAL_MODULE, NULL, VirusTotalModuleNodeSortFunction);

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Hybrid-Analysis";
    column.Width = 140;
    column.Alignment = PH_ALIGN_CENTER;
    column.CustomDraw = TRUE;
    column.Context = info->TreeNewHandle; // Context

    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, COLUMN_ID_HYBRIDANALYSIS_MODULE, NULL, HybridAnalysisModuleNodeSortFunction);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ServiceTreeNewInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PH_TREENEW_COLUMN column;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"VirusTotal";
    column.Width = 140;
    column.Alignment = PH_ALIGN_CENTER;
    column.CustomDraw = TRUE;
    column.Context = info->TreeNewHandle; // Context

    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, COLUMN_ID_VIUSTOTAL_SERVICE, NULL, VirusTotalServiceNodeSortFunction);

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Hybrid-Analysis";
    column.Width = 140;
    column.Alignment = PH_ALIGN_CENTER;
    column.CustomDraw = TRUE;
    column.Context = info->TreeNewHandle; // Context

    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, COLUMN_ID_HYBRIDANALYSIS_SERVICE, NULL, HybridAnalysisServiceNodeSortFunction);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI TreeNewMessageCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_MESSAGE message = Parameter;

    switch (message->Message)
    {
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = message->Parameter1;
            PSCAN_EXTENSION extension = NULL;
            SCAN_TYPE scanType = SCAN_TYPE_VIRUSTOTAL;

            switch (message->SubId)
            {
            case COLUMN_ID_VIUSTOTAL_PROCESS:
                {
                    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)getCellText->Node;
                    extension = PhPluginGetObjectExtension(PluginInstance, processNode->ProcessItem, EmProcessItemType);
                    scanType = SCAN_TYPE_VIRUSTOTAL;
                }
                break;
            case COLUMN_ID_VIUSTOTAL_MODULE:
                {
                    PPH_MODULE_NODE moduleNode = (PPH_MODULE_NODE)getCellText->Node;
                    extension = PhPluginGetObjectExtension(PluginInstance, moduleNode->ModuleItem, EmModuleItemType);
                    scanType = SCAN_TYPE_VIRUSTOTAL;
                }
                break;
            case COLUMN_ID_VIUSTOTAL_SERVICE:
                {
                    PPH_SERVICE_NODE serviceNode = (PPH_SERVICE_NODE)getCellText->Node;
                    extension = PhPluginGetObjectExtension(PluginInstance, serviceNode->ServiceItem, EmServiceItemType);
                    scanType = SCAN_TYPE_VIRUSTOTAL;
                }
                break;
            case COLUMN_ID_HYBRIDANALYSIS_PROCESS:
                {
                    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)getCellText->Node;
                    extension = PhPluginGetObjectExtension(PluginInstance, processNode->ProcessItem, EmProcessItemType);
                    scanType = SCAN_TYPE_HYBRIDANALYSIS;
                }
                break;
            case COLUMN_ID_HYBRIDANALYSIS_MODULE:
                {
                    PPH_MODULE_NODE moduleNode = (PPH_MODULE_NODE)getCellText->Node;
                    extension = PhPluginGetObjectExtension(PluginInstance, moduleNode->ModuleItem, EmModuleItemType);
                    scanType = SCAN_TYPE_HYBRIDANALYSIS;
                }
                break;
            case COLUMN_ID_HYBRIDANALYSIS_SERVICE:
                {
                    PPH_SERVICE_NODE serviceNode = (PPH_SERVICE_NODE)getCellText->Node;
                    extension = PhPluginGetObjectExtension(PluginInstance, serviceNode->ServiceItem, EmServiceItemType);
                    scanType = SCAN_TYPE_HYBRIDANALYSIS;
                }
                break;
            }

            if (extension)
                getCellText->Text = GetScanText(extension, scanType);
        }
        break;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = message->Parameter1;
            PSCAN_EXTENSION extension = NULL;
            SCAN_TYPE scanType = SCAN_TYPE_VIRUSTOTAL;

            switch (message->SubId)
            {
            case COLUMN_ID_VIUSTOTAL_PROCESS:
                {
                    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)customDraw->Node;
                    extension = PhPluginGetObjectExtension(PluginInstance, processNode->ProcessItem, EmProcessItemType);
                    scanType = SCAN_TYPE_VIRUSTOTAL;
                }
                break;
            case COLUMN_ID_VIUSTOTAL_MODULE:
                {
                    PPH_MODULE_NODE moduleNode = (PPH_MODULE_NODE)customDraw->Node;
                    extension = PhPluginGetObjectExtension(PluginInstance, moduleNode->ModuleItem, EmModuleItemType);
                    scanType = SCAN_TYPE_VIRUSTOTAL;
                }
                break;
            case COLUMN_ID_VIUSTOTAL_SERVICE:
                {
                    PPH_SERVICE_NODE serviceNode = (PPH_SERVICE_NODE)customDraw->Node;
                    extension = PhPluginGetObjectExtension(PluginInstance, serviceNode->ServiceItem, EmServiceItemType);
                    scanType = SCAN_TYPE_VIRUSTOTAL;
                }
                break;
            case COLUMN_ID_HYBRIDANALYSIS_PROCESS:
                {
                    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)customDraw->Node;
                    extension = PhPluginGetObjectExtension(PluginInstance, processNode->ProcessItem, EmProcessItemType);
                    scanType = SCAN_TYPE_HYBRIDANALYSIS;
                }
                break;
            case COLUMN_ID_HYBRIDANALYSIS_MODULE:
                {
                    PPH_MODULE_NODE moduleNode = (PPH_MODULE_NODE)customDraw->Node;
                    extension = PhPluginGetObjectExtension(PluginInstance, moduleNode->ModuleItem, EmModuleItemType);
                    scanType = SCAN_TYPE_HYBRIDANALYSIS;
                }
                break;
            case COLUMN_ID_HYBRIDANALYSIS_SERVICE:
                {
                    PPH_SERVICE_NODE serviceNode = (PPH_SERVICE_NODE)customDraw->Node;
                    extension = PhPluginGetObjectExtension(PluginInstance, serviceNode->ServiceItem, EmServiceItemType);
                    scanType = SCAN_TYPE_HYBRIDANALYSIS;
                }
                break;
            }

            if (extension)
            {
                PH_STRINGREF text;

                text = GetScanText(extension, scanType);

                DrawText(
                    customDraw->Dc,
                    text.Buffer,
                    (ULONG)text.Length / sizeof(WCHAR),
                    &customDraw->CellRect,
                    DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE
                    );
            }
        }
        break;
    }
}

VOID NTAPI ProcessItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_PROCESS_ITEM processItem = Object;
    PSCAN_EXTENSION extension = Extension;

    memset(extension, 0, sizeof(SCAN_EXTENSION));
    extension->Type = SCAN_EXTENSION_PROCESS;
    extension->ProcessItem = processItem;

    InitializeScanContext(&extension->ScanContext);

    PhAcquireQueuedLockExclusive(&ScanExtensionsListLock);
    InsertTailList(&ScanExtensionsListHead, &extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ScanExtensionsListLock);
}

VOID NTAPI ProcessItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_PROCESS_ITEM processItem = Object;
    PSCAN_EXTENSION extension = Extension;

    PhAcquireQueuedLockExclusive(&ScanExtensionsListLock);
    RemoveEntryList(&extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ScanExtensionsListLock);

    DeleteScanContext(&extension->ScanContext);
    PhClearReference(&extension->FileName);
    for (ULONG i = 0; i < RTL_NUMBER_OF(extension->ScanResults); i++)
        PhClearReference(&extension->ScanResults[i]);
}

VOID NTAPI ModuleItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_MODULE_ITEM moduleItem = Object;
    PSCAN_EXTENSION extension = Extension;

    memset(extension, 0, sizeof(SCAN_EXTENSION));
    extension->Type = SCAN_EXTENSION_MODULE;
    extension->ModuleItem = moduleItem;

    InitializeScanContext(&extension->ScanContext);

    PhAcquireQueuedLockExclusive(&ScanExtensionsListLock);
    InsertTailList(&ScanExtensionsListHead, &extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ScanExtensionsListLock);
}

VOID NTAPI ModuleItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_MODULE_ITEM processItem = Object;
    PSCAN_EXTENSION extension = Extension;

    PhAcquireQueuedLockExclusive(&ScanExtensionsListLock);
    RemoveEntryList(&extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ScanExtensionsListLock);

    DeleteScanContext(&extension->ScanContext);
    PhClearReference(&extension->FileName);
    for (ULONG i = 0; i < RTL_NUMBER_OF(extension->ScanResults); i++)
        PhClearReference(&extension->ScanResults[i]);
}

VOID NTAPI ServiceItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_SERVICE_ITEM serviceItem = Object;
    PSCAN_EXTENSION extension = Extension;

    memset(extension, 0, sizeof(SCAN_EXTENSION));
    extension->Type = SCAN_EXTENSION_SERVICE;
    extension->ServiceItem = serviceItem;

    InitializeScanContext(&extension->ScanContext);

    PhAcquireQueuedLockExclusive(&ScanExtensionsListLock);
    InsertTailList(&ScanExtensionsListHead, &extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ScanExtensionsListLock);
}

VOID NTAPI ServiceItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_SERVICE_ITEM serviceItem = Object;
    PSCAN_EXTENSION extension = Extension;

    PhAcquireQueuedLockExclusive(&ScanExtensionsListLock);
    RemoveEntryList(&extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ScanExtensionsListLock);

    DeleteScanContext(&extension->ScanContext);
    PhClearReference(&extension->FileName);
    for (ULONG i = 0; i < RTL_NUMBER_OF(extension->ScanResults); i++)
        PhClearReference(&extension->ScanResults[i]);
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
                { IntegerSettingType, SETTING_NAME_AUTO_SCAN_ENABLED, L"0" },
                { IntegerSettingType, SETTING_NAME_VIRUSTOTAL_DEFAULT_ACTION, L"0" },
                { StringSettingType, SETTING_NAME_VIRUSTOTAL_DEFAULT_PAT, L"" },
                { StringSettingType, SETTING_NAME_HYBRIDANAL_DEFAULT_PAT, L"" },
                { StringSettingType, SETTING_NAME_FILESCAN_DEFAULT_PAT, L"" },
            };

            WPP_INIT_TRACING(PLUGIN_NAME);

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Online Checks";
            info->Description = L"Allows files to be checked with online services.";

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
                PhGetGeneralCallback(GeneralCallbackMainMenuInitializing),
                MainMenuInitializingCallback,
                NULL,
                &MainMenuInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessMenuInitializing),
                ProcessMenuInitializingCallback,
                NULL,
                &ProcessMenuInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackModuleMenuInitializing),
                ModuleMenuInitializingCallback,
                NULL,
                &ModuleMenuInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackServiceMenuInitializing),
                ServiceMenuInitializingCallback,
                NULL,
                &ServiceMenuInitializingCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessesUpdated),
                ProcessesUpdatedCallback,
                NULL,
                &ProcessesUpdatedCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessTreeNewInitializing),
                ProcessTreeNewInitializingCallback,
                NULL,
                &ProcessTreeNewInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackModuleTreeNewInitializing),
                ModuleTreeNewInitializingCallback,
                NULL,
                &ModulesTreeNewInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackServiceTreeNewInitializing),
                ServiceTreeNewInitializingCallback,
                NULL,
                &ServiceTreeNewInitializingCallbackRegistration
                );

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackTreeNewMessage),
                TreeNewMessageCallback,
                NULL,
                &TreeNewMessageCallbackRegistration
                );

            PhPluginSetObjectExtension(
                PluginInstance,
                EmProcessItemType,
                sizeof(SCAN_EXTENSION),
                ProcessItemCreateCallback,
                ProcessItemDeleteCallback
                );

            PhPluginSetObjectExtension(
                PluginInstance,
                EmModuleItemType,
                sizeof(SCAN_EXTENSION),
                ModuleItemCreateCallback,
                ModuleItemDeleteCallback
                );

            PhPluginSetObjectExtension(
                PluginInstance,
                EmServiceItemType,
                sizeof(SCAN_EXTENSION),
                ServiceItemCreateCallback,
                ServiceItemDeleteCallback
                );

            PhAddSettings(settings, RTL_NUMBER_OF(settings));
        }
        break;
    }

    return TRUE;
}
