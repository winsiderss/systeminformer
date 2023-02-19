/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2012-2021
 *
 */

#include "onlnchk.h"
#include "db.h"

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessHighlightingColorCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ModuleMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ServiceMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION TreeNewMessageCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ModulesTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ServiceTreeNewInitializingCallbackRegistration;

BOOLEAN VirusTotalScanningEnabled = FALSE;
ULONG ProcessesUpdatedCount = 0;
LIST_ENTRY ProcessListHead = { &ProcessListHead, &ProcessListHead };
PH_QUEUED_LOCK ProcessesListLock = PH_QUEUED_LOCK_INIT;

VOID ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    static ULONG ProcessesUpdatedCount = 0;
    PLIST_ENTRY listEntry;

    if (!VirusTotalScanningEnabled)
        return;

    if (ProcessesUpdatedCount != 3)
    {
        ProcessesUpdatedCount++;
        return;
    }

    listEntry = ProcessListHead.Flink;

    while (listEntry != &ProcessListHead)
    {
        PPROCESS_EXTENSION extension;
        PPH_STRING filePath = NULL;

        extension = CONTAINING_RECORD(listEntry, PROCESS_EXTENSION, ListEntry);

        if (extension->ProcessItem)
        {
            filePath = extension->ProcessItem->FileNameWin32;
        }
        else if (extension->ModuleItem)
        {
            filePath = extension->ModuleItem->FileNameWin32;
        }
        else if (extension->ServiceItem)
        {
            if (extension->FilePath)
            {
                filePath = extension->FilePath;
            }
            else
            {
                if (!extension->ServiceValid)
                {
                    PPH_STRING serviceFileName = NULL;
                    PPH_STRING serviceBinaryPath = NULL;

                    if (NT_SUCCESS(QueryServiceFileName(
                        &extension->ServiceItem->Name->sr,
                        &serviceFileName,
                        &serviceBinaryPath
                        )))
                    {
                        PhMoveReference(&extension->FilePath, serviceFileName);
                        if (serviceBinaryPath) PhDereferenceObject(serviceBinaryPath);
                    }

                    extension->ServiceValid = TRUE;
                }

                filePath = extension->FilePath;
            }
        }

        if (!PhIsNullOrEmptyString(filePath))
        {
            if (!extension->ResultValid)
            {
                PPROCESS_DB_OBJECT object;

                if (filePath && (object = FindProcessDbObject(&filePath->sr)))
                {
                    extension->Stage1 = TRUE;
                    extension->ResultValid = TRUE;

                    extension->Positives = object->Positives;
                    PhSwapReference(&extension->VirusTotalResult, object->Result);
                }
            }

            if (!extension->Stage1)
            {
                if (filePath && !VirusTotalGetCachedResult(filePath))
                {
                    VirusTotalAddCacheResult(filePath, extension);
                }

                extension->Stage1 = TRUE;
            }
        }

        listEntry = listEntry->Flink;
    }
}

VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (VirusTotalScanningEnabled = !!PhGetIntegerSetting(SETTING_NAME_VIRUSTOTAL_SCAN_ENABLED))
    {
        InitializeProcessDb();
        InitializeVirusTotalProcessMonitor();
    }
}

VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_OPTIONS_POINTERS optionsEntry = (PPH_PLUGIN_OPTIONS_POINTERS)Parameter;

    if (!optionsEntry)
        return;

    optionsEntry->CreateSection(
        L"OnlineChecks",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        OptionsDlgProc,
        NULL
        );
}

VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;

    if (!menuItem)
        return;

    switch (menuItem->Id)
    {
    case ENABLE_SERVICE_VIRUSTOTAL:
        {
            ULONG scanningEnabled = !VirusTotalScanningEnabled;

            PhSetIntegerSetting(SETTING_NAME_VIRUSTOTAL_SCAN_ENABLED, scanningEnabled);

            if (VirusTotalScanningEnabled != scanningEnabled)
            {
                INT result = IDOK;
                TASKDIALOGCONFIG config;

                memset(&config, 0, sizeof(TASKDIALOGCONFIG));
                config.cbSize = sizeof(TASKDIALOGCONFIG);
                config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION;
                config.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
                config.hwndParent = menuItem->OwnerWindow;
                config.hMainIcon = PhGetApplicationIcon(FALSE);
                config.cxWidth = 180;
                config.pszWindowTitle = L"System Informer - VirusTotal";
                config.pszMainInstruction = L"VirusTotal scanning requires a restart of System Informer.";
                config.pszContent = L"Do you want to restart System Informer now?";

                if (SUCCEEDED(TaskDialogIndirect(&config, &result, NULL, NULL)) && result == IDYES)
                {
                    ProcessHacker_PrepareForEarlyShutdown();
                    PhShellProcessHacker(
                        menuItem->OwnerWindow,
                        L"-v",
                        SW_SHOW,
                        PH_SHELL_EXECUTE_NOASYNC,
                        PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                        0,
                        NULL
                        );
                    ProcessHacker_Destroy();
                }
            }
        }
        break;
    case MENUITEM_VIRUSTOTAL_UPLOAD:
        UploadToOnlineService(menuItem->Context, MENUITEM_VIRUSTOTAL_UPLOAD);
        break;
    case MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE:
        UploadServiceToOnlineService(menuItem->Context, MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE);
        break;
    case MENUITEM_JOTTI_UPLOAD:
        UploadToOnlineService(menuItem->Context, MENUITEM_JOTTI_UPLOAD);
        break;
    case MENUITEM_JOTTI_UPLOAD_SERVICE:
        UploadServiceToOnlineService(menuItem->Context, MENUITEM_JOTTI_UPLOAD_SERVICE);
        break;
    case MENUITEM_HYBRIDANALYSIS_UPLOAD:
        UploadToOnlineService(menuItem->Context, MENUITEM_HYBRIDANALYSIS_UPLOAD);
        break;
    case MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE:
        UploadServiceToOnlineService(menuItem->Context, MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE);
        break;
    case MENUITEM_VIRUSTOTAL_UPLOAD_FILE:
    case MENUITEM_HYBRIDANALYSIS_UPLOAD_FILE:
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
                }
            }

            PhFreeFileDialog(fileDialog);
        }
        break;
    }
}

VOID NTAPI MainMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_EMENU_ITEM onlineMenuItem;
    PPH_EMENU_ITEM enableMenuItem;

    if (!menuInfo)
        return;

    if (menuInfo->u.MainMenu.SubMenuIndex != PH_MENU_ITEM_LOCATION_TOOLS)
        return;

    onlineMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"&Online Checks", NULL);
    PhInsertEMenuItem(onlineMenuItem, enableMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, ENABLE_SERVICE_VIRUSTOTAL, L"&Enable VirusTotal scanning", NULL), ULONG_MAX);
    PhInsertEMenuItem(onlineMenuItem, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(onlineMenuItem, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_HYBRIDANALYSIS_UPLOAD_FILE, L"Upload file to &Hybrid-Analysis...", NULL), ULONG_MAX);
    PhInsertEMenuItem(onlineMenuItem, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_VIRUSTOTAL_UPLOAD_FILE, L"&Upload file to VirusTotal...", NULL), ULONG_MAX);
    //PhInsertEMenuItem(onlineMenuItem, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_VIRUSTOTAL_QUEUE, L"Upload unknown files to VirusTotal...", NULL), ULONG_MAX);
    PhInsertEMenuItem(menuInfo->Menu, onlineMenuItem, ULONG_MAX);

    if (VirusTotalScanningEnabled)
        enableMenuItem->Flags |= PH_EMENU_CHECKED;
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

VOID NTAPI ProcessMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_PROCESS_ITEM processItem;
    PPH_EMENU_ITEM sendToMenu;

    if (!menuInfo)
        return;

    if (menuInfo->u.Process.NumberOfProcesses == 1)
        processItem = menuInfo->u.Process.Processes[0];
    else
        processItem = NULL;

    sendToMenu = CreateSendToMenu(TRUE, menuInfo->Menu, processItem ? processItem->FileNameWin32 : NULL);

    // Only enable the Send To menu if there is exactly one process selected and it has a file name.
    if (!processItem || !processItem->FileName)
    {
        sendToMenu->Flags |= PH_EMENU_DISABLED;
    }
}

VOID NTAPI ModuleMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_MODULE_ITEM moduleItem;
    PPH_EMENU_ITEM sendToMenu;

    if (!menuInfo)
        return;

    if (menuInfo->u.Module.NumberOfModules == 1)
        moduleItem = menuInfo->u.Module.Modules[0];
    else
        moduleItem = NULL;

    sendToMenu = CreateSendToMenu(FALSE, menuInfo->Menu, moduleItem ? moduleItem->FileNameWin32 : NULL);

    if (!moduleItem)
    {
        sendToMenu->Flags |= PH_EMENU_DISABLED;
    }
}

VOID NTAPI ServiceMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_SERVICE_ITEM serviceItem;
    PPH_EMENU_ITEM sendToMenu;

    if (!menuInfo)
        return;

    if (menuInfo->u.Service.NumberOfServices == 1)
        serviceItem = menuInfo->u.Service.Services[0];
    else
        serviceItem = NULL;

    sendToMenu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"Sen&d to", NULL);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE, L"&hybrid-analysis.com", serviceItem ? serviceItem : NULL), ULONG_MAX);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_JOTTI_UPLOAD_SERVICE, L"virusscan.&jotti.org", serviceItem ? serviceItem : NULL), ULONG_MAX);
    PhInsertEMenuItem(sendToMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE, L"&virustotal.com", serviceItem ? serviceItem : NULL), ULONG_MAX);
    PhInsertEMenuItem(menuInfo->Menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menuInfo->Menu, sendToMenu, ULONG_MAX);

    if (!serviceItem)
    {
        sendToMenu->Flags |= PH_EMENU_DISABLED;
    }
}

VOID ProcessHighlightingColorCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    //PPH_PLUGIN_GET_HIGHLIGHTING_COLOR getHighlightingColor = Parameter;
    //PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)getHighlightingColor->Parameter;
    //PPROCESS_DB_OBJECT object;

    //if (getHighlightingColor->Handled)
    //    return;

    //if (!PhGetIntegerSetting(SETTING_NAME_VIRUSTOTAL_HIGHLIGHT_DETECTIONS))
    //    return;

    //LockProcessDb();

    //if (PhIsNullOrEmptyString(processItem->FileNameWin32))
    //    return;

    //if ((object = FindProcessDbObject(&processItem->FileNameWin32->sr)) && object->Positives)
    //{
    //    getHighlightingColor->BackColor = RGB(255, 0, 0);
    //    getHighlightingColor->Cache = TRUE;
    //    getHighlightingColor->Handled = TRUE;
    //}

    //UnlockProcessDb();
}

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
    PPROCESS_EXTENSION extension1 = PhPluginGetObjectExtension(PluginInstance, node1->ProcessItem, EmProcessItemType);
    PPROCESS_EXTENSION extension2 = PhPluginGetObjectExtension(PluginInstance, node2->ProcessItem, EmProcessItemType);

    return PhCompareStringWithNullSortOrder(extension1->VirusTotalResult, extension2->VirusTotalResult, SortOrder, TRUE);
}

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
    PPROCESS_EXTENSION extension1 = PhPluginGetObjectExtension(PluginInstance, node1->ModuleItem, EmModuleItemType);
    PPROCESS_EXTENSION extension2 = PhPluginGetObjectExtension(PluginInstance, node2->ModuleItem, EmModuleItemType);

    return PhCompareStringWithNullSortOrder(extension1->VirusTotalResult, extension2->VirusTotalResult, SortOrder, TRUE);
}

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
    PPROCESS_EXTENSION extension1 = PhPluginGetObjectExtension(PluginInstance, node1->ServiceItem, EmServiceItemType);
    PPROCESS_EXTENSION extension2 = PhPluginGetObjectExtension(PluginInstance, node2->ServiceItem, EmServiceItemType);

    return PhCompareStringWithNullSortOrder(extension1->VirusTotalResult, extension2->VirusTotalResult, SortOrder, TRUE);
}

VOID NTAPI ProcessTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PH_TREENEW_COLUMN column;

    if (!info)
        return;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"VirusTotal";
    column.Width = 140;
    column.Alignment = PH_ALIGN_CENTER;
    column.CustomDraw = TRUE;
    column.Context = info->TreeNewHandle; // Context

    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, COLUMN_ID_VIUSTOTAL_PROCESS, NULL, VirusTotalProcessNodeSortFunction);
}

VOID NTAPI ModuleTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PH_TREENEW_COLUMN column;

    if (!info)
        return;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"VirusTotal";
    column.Width = 140;
    column.Alignment = PH_ALIGN_CENTER;
    column.CustomDraw = TRUE;
    column.Context = info->TreeNewHandle; // Context

    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, COLUMN_ID_VIUSTOTAL_MODULE, NULL, VirusTotalModuleNodeSortFunction);
}

VOID NTAPI ServiceTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PH_TREENEW_COLUMN column;

    if (!info)
        return;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"VirusTotal";
    column.Width = 140;
    column.Alignment = PH_ALIGN_CENTER;
    column.CustomDraw = TRUE;
    column.Context = info->TreeNewHandle; // Context

    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, COLUMN_ID_VIUSTOTAL_SERVICE, NULL, VirusTotalServiceNodeSortFunction);
}

VOID NTAPI TreeNewMessageCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_MESSAGE message = Parameter;

    if (!message)
        return;

    switch (message->Message)
    {
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = message->Parameter1;

            switch (message->SubId)
            {
            case COLUMN_ID_VIUSTOTAL_PROCESS:
                {
                    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)getCellText->Node;
                    PPROCESS_EXTENSION extension = PhPluginGetObjectExtension(PluginInstance, processNode->ProcessItem, EmProcessItemType);

                    getCellText->Text = PhGetStringRef(extension->VirusTotalResult);
                }
                break;
            case COLUMN_ID_VIUSTOTAL_MODULE:
                {
                    PPH_MODULE_NODE moduleNode = (PPH_MODULE_NODE)getCellText->Node;
                    PPROCESS_EXTENSION extension = PhPluginGetObjectExtension(PluginInstance, moduleNode->ModuleItem, EmModuleItemType);

                    getCellText->Text = PhGetStringRef(extension->VirusTotalResult);
                }
                break;
            case COLUMN_ID_VIUSTOTAL_SERVICE:
                {
                    PPH_SERVICE_NODE serviceNode = (PPH_SERVICE_NODE)getCellText->Node;
                    PPROCESS_EXTENSION extension = PhPluginGetObjectExtension(PluginInstance, serviceNode->ServiceItem, EmServiceItemType);

                    getCellText->Text = PhGetStringRef(extension->VirusTotalResult);
                }
                break;
            }
        }
        break;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = message->Parameter1;
            PPROCESS_EXTENSION extension = NULL;
            PH_STRINGREF text;

            if (!VirusTotalScanningEnabled)
            {
                static PH_STRINGREF disabledText = PH_STRINGREF_INIT(L"Scanning disabled");

                DrawText(
                    customDraw->Dc,
                    disabledText.Buffer,
                    (ULONG)disabledText.Length / 2,
                    &customDraw->CellRect,
                    DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE
                    );

                return;
            }

            switch (message->SubId)
            {
            case COLUMN_ID_VIUSTOTAL_PROCESS:
                {
                    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)customDraw->Node;

                    extension = PhPluginGetObjectExtension(PluginInstance, processNode->ProcessItem, EmProcessItemType);
                }
                break;
            case COLUMN_ID_VIUSTOTAL_MODULE:
                {
                    PPH_MODULE_NODE moduleNode = (PPH_MODULE_NODE)customDraw->Node;

                    extension = PhPluginGetObjectExtension(PluginInstance, moduleNode->ModuleItem, EmModuleItemType);
                }
                break;
            case COLUMN_ID_VIUSTOTAL_SERVICE:
                {
                    PPH_SERVICE_NODE serviceNode = (PPH_SERVICE_NODE)customDraw->Node;

                    extension = PhPluginGetObjectExtension(PluginInstance, serviceNode->ServiceItem, EmServiceItemType);
                }
                break;
            }

            if (!extension)
                break;

            //if (extension->Positives > 0)
            //    SetTextColor(customDraw->Dc, RGB(0xff, 0x0, 0x0));

            text = PhGetStringRef(extension->VirusTotalResult);
            DrawText(
                customDraw->Dc,
                text.Buffer,
                (ULONG)text.Length / sizeof(WCHAR),
                &customDraw->CellRect,
                DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE
                );
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
    PPROCESS_EXTENSION extension = Extension;

    memset(extension, 0, sizeof(PROCESS_EXTENSION));

    extension->ProcessItem = processItem;

    PhAcquireQueuedLockExclusive(&ProcessesListLock);
    InsertTailList(&ProcessListHead, &extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ProcessesListLock);
}

VOID NTAPI ProcessItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_PROCESS_ITEM processItem = Object;
    PPROCESS_EXTENSION extension = Extension;

    PhClearReference(&extension->VirusTotalResult);

    PhAcquireQueuedLockExclusive(&ProcessesListLock);
    RemoveEntryList(&extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ProcessesListLock);
}

VOID NTAPI ModuleItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_MODULE_ITEM moduleItem = Object;
    PPROCESS_EXTENSION extension = Extension;

    memset(extension, 0, sizeof(PROCESS_EXTENSION));

    extension->ModuleItem = moduleItem;

    PhAcquireQueuedLockExclusive(&ProcessesListLock);
    InsertTailList(&ProcessListHead, &extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ProcessesListLock);
}

VOID NTAPI ModuleItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_MODULE_ITEM processItem = Object;
    PPROCESS_EXTENSION extension = Extension;

    PhClearReference(&extension->VirusTotalResult);

    PhAcquireQueuedLockExclusive(&ProcessesListLock);
    RemoveEntryList(&extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ProcessesListLock);
}

VOID NTAPI ServiceItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_SERVICE_ITEM serviceItem = Object;
    PPROCESS_EXTENSION extension = Extension;

    memset(extension, 0, sizeof(PROCESS_EXTENSION));

    extension->ServiceItem = serviceItem;

    PhAcquireQueuedLockExclusive(&ProcessesListLock);
    InsertTailList(&ProcessListHead, &extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ProcessesListLock);
}

VOID NTAPI ServiceItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_SERVICE_ITEM serviceItem = Object;
    PPROCESS_EXTENSION extension = Extension;

    PhClearReference(&extension->VirusTotalResult);

    PhAcquireQueuedLockExclusive(&ProcessesListLock);
    RemoveEntryList(&extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ProcessesListLock);
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
                { IntegerSettingType, SETTING_NAME_VIRUSTOTAL_SCAN_ENABLED, L"0" },
                { IntegerSettingType, SETTING_NAME_VIRUSTOTAL_HIGHLIGHT_DETECTIONS, L"0" },
                { IntegerSettingType, SETTING_NAME_VIRUSTOTAL_DEFAULT_ACTION, L"0" }
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Online Checks";
            info->Author = L"dmex, wj32";
            info->Description = L"Allows files to be checked with online services.";

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
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
                PhGetGeneralCallback(GeneralCallbackGetProcessHighlightingColor),
                ProcessHighlightingColorCallback,
                NULL,
                &ProcessHighlightingColorCallbackRegistration
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
                sizeof(PROCESS_EXTENSION),
                ProcessItemCreateCallback,
                ProcessItemDeleteCallback
                );

            PhPluginSetObjectExtension(
                PluginInstance,
                EmModuleItemType,
                sizeof(PROCESS_EXTENSION),
                ModuleItemCreateCallback,
                ModuleItemDeleteCallback
                );

            PhPluginSetObjectExtension(
                PluginInstance,
                EmServiceItemType,
                sizeof(PROCESS_EXTENSION),
                ServiceItemCreateCallback,
                ServiceItemDeleteCallback
                );

            PhAddSettings(settings, ARRAYSIZE(settings));
        }
        break;
    }

    return TRUE;
}
