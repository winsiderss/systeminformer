/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2016
 *     dmex    2016-2023
 *
 */

#include "usernotes.h"
#include <toolstatusintf.h>
#include <commdlg.h>
#include <mapimg.h>

static PPH_PLUGIN PluginInstance;
static PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginMenuHookCallbackRegistration;
static PH_CALLBACK_REGISTRATION MainMenuInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION TreeNewMessageCallbackRegistration;
static PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessPropertiesInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION ServicePropertiesInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessMenuInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessTreeNewInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION GetProcessHighlightingColorCallbackRegistration;
static PH_CALLBACK_REGISTRATION ServiceTreeNewInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION MiListSectionMenuInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessModifiedCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
static PH_CALLBACK_REGISTRATION SearchChangedRegistration;

static PTOOLSTATUS_INTERFACE ToolStatusInterface = NULL;
static COLORREF ProcessCustomColors[16] = { 0 };

HWND ProcessTreeNewHandle = NULL;
LIST_ENTRY ProcessListHead = { &ProcessListHead, &ProcessListHead };
PH_QUEUED_LOCK ProcessListLock = PH_QUEUED_LOCK_INIT;

HWND ServiceTreeNewHandle = NULL;
LIST_ENTRY ServiceListHead = { &ServiceListHead, &ServiceListHead };
PH_QUEUED_LOCK ServiceListLock = PH_QUEUED_LOCK_INIT;

BOOLEAN MatchDbObjectIntent(
    _In_ PDB_OBJECT Object,
    _In_ ULONG Intent
    )
{
    return (!(Intent & INTENT_PROCESS_COMMENT) || Object->Comment->Length != 0) &&
        (!(Intent & INTENT_PROCESS_PRIORITY_CLASS) || Object->PriorityClass != 0) &&
        (!(Intent & INTENT_PROCESS_IO_PRIORITY) || Object->IoPriorityPlusOne != 0) &&
        (!(Intent & INTENT_PROCESS_HIGHLIGHT) || Object->BackColor != ULONG_MAX) &&
        (!(Intent & INTENT_PROCESS_COLLAPSE) || Object->Collapse == TRUE) &&
        (!(Intent & INTENT_PROCESS_AFFINITY) || Object->AffinityMask != 0) &&
        (!(Intent & INTENT_PROCESS_PAGEPRIORITY) || Object->PagePriorityPlusOne != 0) &&
        (!(Intent & INTENT_PROCESS_BOOST) || Object->Boost == TRUE);
}

PDB_OBJECT FindDbObjectForProcess(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ ULONG Intent
    )
{
    PDB_OBJECT object;

    if (
        ProcessItem->CommandLine &&
        (object = FindDbObject(COMMAND_LINE_TAG, &ProcessItem->CommandLine->sr)) &&
        MatchDbObjectIntent(object, Intent)
        )
    {
        return object;
    }

    if (
        ProcessItem->ProcessName &&
        (object = FindDbObject(FILE_TAG, &ProcessItem->ProcessName->sr)) &&
        MatchDbObjectIntent(object, Intent)
        )
    {
        return object;
    }

    return NULL;
}

VOID DeleteDbObjectForProcessIfUnused(
    _In_ PDB_OBJECT Object
    )
{
    if (
        Object->Comment->Length == 0 &&
        Object->PriorityClass == 0 &&
        Object->IoPriorityPlusOne == 0 &&
        Object->BackColor == ULONG_MAX &&
        Object->Collapse == FALSE &&
        Object->AffinityMask == 0 &&
        Object->PagePriorityPlusOne == 0 &&
        Object->Boost == FALSE
        )
    {
        DeleteDbObject(Object);
    }
}

NTSTATUS GetProcessAffinity(
    _In_ HANDLE ProcessHandle,
    _Out_ PKAFFINITY Affinity
    )
{
    NTSTATUS status;
    GROUP_AFFINITY groupAffinity = { 0 };

    status = PhGetProcessGroupAffinity(
        ProcessHandle,
        &groupAffinity
        );

    if (status == STATUS_INVALID_PARAMETER || status == STATUS_INVALID_INFO_CLASS) // GH#1317: Required for Windows 7 (jxy-s)
    {
        status = PhGetProcessAffinityMask(ProcessHandle, &groupAffinity.Mask);
    }

    if (NT_SUCCESS(status))
    {
        *Affinity = groupAffinity.Mask;
    }

    return status;
}

NTSTATUS GetProcessIoPriority(
    _In_ HANDLE ProcessHandle,
    _Out_ IO_PRIORITY_HINT* IoPriority
    )
{
    NTSTATUS status;
    IO_PRIORITY_HINT ioPriority = ULONG_MAX;

    status = PhGetProcessIoPriority(
        ProcessHandle,
        &ioPriority
        );

    if (NT_SUCCESS(status))
    {
        *IoPriority = ioPriority;
    }

    return status;
}

NTSTATUS GetProcessPagePriority(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG PagePriority
    )
{
    NTSTATUS status;
    ULONG pagePriority = ULONG_MAX;

    status = PhGetProcessPagePriority(
        ProcessHandle,
        &pagePriority
        );

    if (NT_SUCCESS(status))
    {
        *PagePriority = pagePriority;
    }

    return status;
}

ULONG GetPriorityClassFromId(
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case PHAPP_ID_PRIORITY_REALTIME:
        return PROCESS_PRIORITY_CLASS_REALTIME;
    case PHAPP_ID_PRIORITY_HIGH:
        return PROCESS_PRIORITY_CLASS_HIGH;
    case PHAPP_ID_PRIORITY_ABOVENORMAL:
        return PROCESS_PRIORITY_CLASS_ABOVE_NORMAL;
    case PHAPP_ID_PRIORITY_NORMAL:
        return PROCESS_PRIORITY_CLASS_NORMAL;
    case PHAPP_ID_PRIORITY_BELOWNORMAL:
        return PROCESS_PRIORITY_CLASS_BELOW_NORMAL;
    case PHAPP_ID_PRIORITY_IDLE:
        return PROCESS_PRIORITY_CLASS_IDLE;
    }

    return PROCESS_PRIORITY_CLASS_UNKNOWN;
}

ULONG GetIoPriorityFromId(
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case PHAPP_ID_IOPRIORITY_VERYLOW:
        return IoPriorityVeryLow;
    case PHAPP_ID_IOPRIORITY_LOW:
        return IoPriorityLow;
    case PHAPP_ID_IOPRIORITY_NORMAL:
        return IoPriorityNormal;
    case PHAPP_ID_IOPRIORITY_HIGH:
        return IoPriorityHigh;
    }

    return ULONG_MAX;
}

ULONG GetPagePriorityFromId(
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case PHAPP_ID_PAGEPRIORITY_VERYLOW:
        return MEMORY_PRIORITY_VERY_LOW;
    case PHAPP_ID_PAGEPRIORITY_LOW:
        return MEMORY_PRIORITY_LOW;
    case PHAPP_ID_PAGEPRIORITY_MEDIUM:
        return MEMORY_PRIORITY_MEDIUM;
    case PHAPP_ID_PAGEPRIORITY_BELOWNORMAL:
        return MEMORY_PRIORITY_BELOW_NORMAL;
    case PHAPP_ID_PAGEPRIORITY_NORMAL:
        return MEMORY_PRIORITY_NORMAL;
    }

    return ULONG_MAX;
}

VOID InitializeDbPath(
    VOID
    )
{
    if (ProcessHacker_IsPortableMode())
    {
        PPH_STRING fileName;

        fileName = PhGetApplicationDirectoryFileNameZ(L"usernotesdb.xml", TRUE);
        SetDbPath(fileName);
    }
    else
    {
        PPH_STRING fileName;

        fileName = PhGetKnownLocationZ(PH_FOLDERID_RoamingAppData, L"\\SystemInformer\\usernotesdb.xml", TRUE);
        SetDbPath(fileName);
    }
}

VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(TOOLSTATUS_PLUGIN_NAME))
    {
        ToolStatusInterface = PhGetPluginInformation(toolStatusPlugin)->Interface;

        if (ToolStatusInterface->Version < TOOLSTATUS_INTERFACE_VERSION)
            ToolStatusInterface = NULL;
    }

    InitializeDbPath();
    InitializeDb();
    LoadDb();
}

VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI ShowOptionsCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_OPTIONS_POINTERS optionsEntry = (PPH_PLUGIN_OPTIONS_POINTERS)Parameter;

    optionsEntry->CreateSection(
        L"UserNotes",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        OptionsDlgProc,
        NULL
        );
}

VOID NTAPI MainMenuInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_EMENU_ITEM onlineMenuItem;

    if (menuInfo->u.MainMenu.SubMenuIndex != PH_MENU_ITEM_LOCATION_TOOLS)
        return;

    onlineMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"&User Notes", NULL);
    PhInsertEMenuItem(onlineMenuItem, PhPluginCreateEMenuItem(PluginInstance, 0, FILE_PRIORITY_SAVE_IFEO, L"Configure priority for executable...", NULL), ULONG_MAX);
    PhInsertEMenuItem(onlineMenuItem, PhPluginCreateEMenuItem(PluginInstance, 0, FILE_IO_PRIORITY_SAVE_IFEO, L"Configure IO priority for executable...", NULL), ULONG_MAX);
    PhInsertEMenuItem(onlineMenuItem, PhPluginCreateEMenuItem(PluginInstance, 0, FILE_PAGE_PRIORITY_SAVE_IFEO, L"Configure page priority for executable...", NULL), ULONG_MAX);
    PhInsertEMenuItem(menuInfo->Menu, onlineMenuItem, ULONG_MAX);
}

PPH_STRING ShowFileDialog(
    _In_ HWND ParentWindowHandle
    )
{
    static PH_FILETYPE_FILTER filters[] =
    {
        { L"Executable files (*.exe)", L"*.exe" },
        { L"All files (*.*)", L"*.*" }
    };
    PPH_STRING fileName = NULL;
    PVOID fileDialog;

    fileDialog = PhCreateOpenFileDialog();
    PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));

    if (PhShowFileDialog(ParentWindowHandle, fileDialog))
    {
        fileName = PhGetFileDialogFileName(fileDialog);
    }

    PhFreeFileDialog(fileDialog);

    if (fileName)
    {
        NTSTATUS status;
        PH_MAPPED_IMAGE mappedImage;

        status = PhLoadMappedImage(
            PhGetString(fileName),
            NULL,
            &mappedImage
            );

        if (NT_SUCCESS(status))
        {
            if (mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            {
                if (!(mappedImage.NtHeaders32->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) || mappedImage.NtHeaders32->FileHeader.Characteristics & IMAGE_FILE_DLL)
                    status = STATUS_INVALID_IMAGE_NOT_MZ;
            }
            else if (mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            {
                if (!(mappedImage.NtHeaders32->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) || mappedImage.NtHeaders32->FileHeader.Characteristics & IMAGE_FILE_DLL)
                    status = STATUS_INVALID_IMAGE_NOT_MZ;
            }

            PhUnloadMappedImage(&mappedImage);
        }

        if (!NT_SUCCESS(status))
        {
            PhClearReference(&fileName);
            PhShowStatus(ParentWindowHandle, L"Unable to configure IFEO priority for this image.", status, 0);
        }
    }

    return fileName;
}

typedef struct _USERNOTES_TASK_IFEO_CONTEXT
{
    HWND WindowHandle;
    PPH_STRING FileName;
    PPH_STRING StatusMessage;
    ULONG RadioButton;
    USERNOTES_COMMAND_ID MenuCommand;
} USERNOTES_TASK_IFEO_CONTEXT, *PUSERNOTES_TASK_IFEO_CONTEXT;

HRESULT CALLBACK TaskDialogBootstrapCallback(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PUSERNOTES_TASK_IFEO_CONTEXT context = (PUSERNOTES_TASK_IFEO_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_CREATED:
        {
            context->WindowHandle = hwndDlg;

            PhSetApplicationWindowIcon(hwndDlg);
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            ULONG buttonId = (ULONG)wParam;

            if (buttonId == IDYES)
            {
                if (context->RadioButton == ULONG_MAX)
                    return S_FALSE;

                if (context->MenuCommand == PROCESS_PRIORITY_SAVE_IFEO || context->MenuCommand == FILE_PRIORITY_SAVE_IFEO)
                {
                    NTSTATUS status;
                    ULONG newPriorityClass;

                    newPriorityClass = GetPriorityClassFromId(context->RadioButton);

                    if (newPriorityClass != PROCESS_PRIORITY_CLASS_UNKNOWN)
                    {
                        status = CreateIfeoObject(
                            &context->FileName->sr,
                            newPriorityClass,
                            ULONG_MAX,
                            ULONG_MAX
                            );

                        if (!NT_SUCCESS(status))
                        {
                            TASKDIALOGCONFIG config;

                            memset(&config, 0, sizeof(TASKDIALOGCONFIG));
                            config.cbSize = sizeof(TASKDIALOGCONFIG);
                            config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
                            config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
                            config.pszMainIcon = TD_ERROR_ICON;
                            config.pszWindowTitle = L"System Informer";
                            config.pszMainInstruction = L"Unable to update the IFEO key for priority.";
                            config.cxWidth = 200;

                            if (context->StatusMessage = PhGetStatusMessage(status, 0))
                            {
                                config.pszContent = PhGetString(context->StatusMessage);
                            }

                            SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
                            return S_FALSE;
                        }
                    }
                    else
                    {
                        return S_FALSE;
                    }
                }
                else if (context->MenuCommand == PROCESS_IO_PRIORITY_SAVE_IFEO || context->MenuCommand == FILE_IO_PRIORITY_SAVE_IFEO)
                {
                    NTSTATUS status;
                    ULONG newIoPriorityClass;

                    newIoPriorityClass = GetIoPriorityFromId(context->RadioButton);

                    if (newIoPriorityClass != ULONG_MAX)
                    {
                        status = CreateIfeoObject(
                            &context->FileName->sr,
                            ULONG_MAX,
                            newIoPriorityClass,
                            ULONG_MAX
                            );

                        if (!NT_SUCCESS(status))
                        {
                            TASKDIALOGCONFIG config;

                            memset(&config, 0, sizeof(TASKDIALOGCONFIG));
                            config.cbSize = sizeof(TASKDIALOGCONFIG);
                            config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
                            config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
                            config.pszMainIcon = TD_ERROR_ICON;
                            config.pszWindowTitle = L"System Informer";
                            config.pszMainInstruction = L"Unable to update the IFEO key for priority.";
                            config.cxWidth = 200;

                            if (context->StatusMessage = PhGetStatusMessage(status, 0))
                            {
                                config.pszContent = PhGetString(context->StatusMessage);
                            }

                            SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
                            return S_FALSE;
                        }
                    }
                    else
                    {
                        return S_FALSE;
                    }
                }
                else if (context->MenuCommand == PROCESS_PAGE_PRIORITY_SAVE_IFEO || context->MenuCommand == FILE_PAGE_PRIORITY_SAVE_IFEO)
                {
                    NTSTATUS status;
                    ULONG newPagePriority;

                    newPagePriority = GetPagePriorityFromId(context->RadioButton);

                    if (newPagePriority != ULONG_MAX)
                    {
                        status = CreateIfeoObject(
                            &context->FileName->sr,
                            ULONG_MAX,
                            ULONG_MAX,
                            newPagePriority
                            );

                        if (!NT_SUCCESS(status))
                        {
                            TASKDIALOGCONFIG config;

                            memset(&config, 0, sizeof(TASKDIALOGCONFIG));
                            config.cbSize = sizeof(TASKDIALOGCONFIG);
                            config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
                            config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
                            config.pszMainIcon = TD_ERROR_ICON;
                            config.pszWindowTitle = L"System Informer";
                            config.pszMainInstruction = L"Unable to update the IFEO key for priority.";
                            config.cxWidth = 200;

                            if (context->StatusMessage = PhGetStatusMessage(status, 0))
                            {
                                config.pszContent = PhGetString(context->StatusMessage);
                            }

                            SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
                            return S_FALSE;
                        }
                    }
                    else
                    {
                        return S_FALSE;
                    }
                }
            }
        }
        break;
    case TDN_RADIO_BUTTON_CLICKED:
        {
            context->RadioButton = (ULONG)wParam;
        }
        break;
    }

    return S_OK;
}

VOID ShowProcessPriorityDialog(
    _In_ PPH_PLUGIN_MENU_ITEM MenuItem,
    _In_ PPH_STRING FileName
    )
{
    static TASKDIALOG_BUTTON TaskDialogRadioButtonArray[] =
    {
        { PHAPP_ID_PRIORITY_REALTIME, L"Realtime" },
        { PHAPP_ID_PRIORITY_HIGH, L"High" },
        { PHAPP_ID_PRIORITY_ABOVENORMAL, L"Above normal" },
        { PHAPP_ID_PRIORITY_NORMAL, L"Normal" },
        { PHAPP_ID_PRIORITY_BELOWNORMAL, L"Below normal" },
        { PHAPP_ID_PRIORITY_IDLE, L"Idle" },
    };
    static TASKDIALOG_BUTTON TaskDialogButtonArray[] =
    {
        { IDYES, L"Save" },
        { IDCANCEL, L"Cancel" },
    };
    PUSERNOTES_TASK_IFEO_CONTEXT context;
    TASKDIALOGCONFIG config;
    ULONG priorityClass;

    context = PhAllocateZero(sizeof(USERNOTES_TASK_IFEO_CONTEXT));
    context->RadioButton = ULONG_MAX;
    context->MenuCommand = MenuItem->Id;
    context->FileName = FileName;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS | TDF_POSITION_RELATIVE_TO_WINDOW;
    config.hMainIcon = PhGetApplicationIcon(FALSE);
    config.pszWindowTitle = PhGetString(FileName);
    config.pszMainInstruction = L"Select the default process priority.";
    config.pszContent = L"The process priority will be applied by Windows even when System Informer isn't currently running. "
    L"Note: Realtime priority requires the User has the SeIncreaseBasePriorityPrivilege or the process running as Administrator.";
    config.nDefaultButton = IDCANCEL;
    config.pRadioButtons = TaskDialogRadioButtonArray;
    config.cRadioButtons = RTL_NUMBER_OF(TaskDialogRadioButtonArray);
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = RTL_NUMBER_OF(TaskDialogButtonArray);
    config.hwndParent = MenuItem->OwnerWindow;
    config.lpCallbackData = (LONG_PTR)context;
    config.pfCallback = TaskDialogBootstrapCallback;
    config.cxWidth = 200;

    if (FindIfeoObject(&FileName->sr, &priorityClass, NULL, NULL))
    {
        switch (priorityClass)
        {
        case PROCESS_PRIORITY_CLASS_REALTIME:
            config.nDefaultRadioButton = PHAPP_ID_PRIORITY_REALTIME;
            break;
        case PROCESS_PRIORITY_CLASS_HIGH:
            config.nDefaultRadioButton = PHAPP_ID_PRIORITY_HIGH;
            break;
        case PROCESS_PRIORITY_CLASS_ABOVE_NORMAL:
            config.nDefaultRadioButton = PHAPP_ID_PRIORITY_ABOVENORMAL;
            break;
        case PROCESS_PRIORITY_CLASS_NORMAL:
            config.nDefaultRadioButton = PHAPP_ID_PRIORITY_NORMAL;
            break;
        case PROCESS_PRIORITY_CLASS_BELOW_NORMAL:
            config.nDefaultRadioButton = PHAPP_ID_PRIORITY_BELOWNORMAL;
            break;
        case PROCESS_PRIORITY_CLASS_IDLE:
            config.nDefaultRadioButton = PHAPP_ID_PRIORITY_IDLE;
            break;
        default:
            //config.dwFlags |= TDF_NO_DEFAULT_RADIO_BUTTON;
            config.nDefaultRadioButton = PHAPP_ID_PRIORITY_NORMAL;
            break;
        }
    }
    else
    {
        //config.dwFlags |= TDF_NO_DEFAULT_RADIO_BUTTON;
        config.nDefaultRadioButton = PHAPP_ID_PRIORITY_NORMAL;
    }

    TaskDialogIndirect(&config, NULL, NULL, NULL);

    PhFree(context);
}

VOID ShowProcessIoPriorityDialog(
    _In_ PPH_PLUGIN_MENU_ITEM MenuItem,
    _In_ PPH_STRING FileName
    )
{
    static TASKDIALOG_BUTTON TaskDialogRadioButtonArray[] =
    {
        { PHAPP_ID_IOPRIORITY_HIGH , L"High" },
        { PHAPP_ID_IOPRIORITY_NORMAL, L"Normal" },
        { PHAPP_ID_IOPRIORITY_LOW , L"Low" },
        { PHAPP_ID_IOPRIORITY_VERYLOW, L"Very low" },
    };
    static TASKDIALOG_BUTTON TaskDialogButtonArray[] =
    {
        { IDYES, L"Save" },
        { IDCANCEL, L"Cancel" },
    };
    PUSERNOTES_TASK_IFEO_CONTEXT context;
    TASKDIALOGCONFIG config;
    ULONG ioPriorityClass;

    context = PhAllocateZero(sizeof(USERNOTES_TASK_IFEO_CONTEXT));
    context->RadioButton = ULONG_MAX;
    context->MenuCommand = MenuItem->Id;
    context->FileName = FileName;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS | TDF_POSITION_RELATIVE_TO_WINDOW;
    config.hMainIcon = PhGetApplicationIcon(FALSE);
    config.pszWindowTitle = PhGetString(FileName);
    config.pszMainInstruction = L"Select the default process IO priority.";
    config.pszContent = L"The IO priority will be applied by Windows even when System Informer isn't currently running. "
    L"Note: High IO priority requires the User has the SeIncreaseBasePriorityPrivilege or the process running as Administrator.";
    config.nDefaultButton = IDCANCEL;
    config.pRadioButtons = TaskDialogRadioButtonArray;
    config.cRadioButtons = RTL_NUMBER_OF(TaskDialogRadioButtonArray);
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = RTL_NUMBER_OF(TaskDialogButtonArray);
    config.hwndParent = MenuItem->OwnerWindow;
    config.lpCallbackData = (LONG_PTR)context;
    config.pfCallback = TaskDialogBootstrapCallback;
    config.cxWidth = 200;

    if (FindIfeoObject(&FileName->sr, NULL, &ioPriorityClass, NULL))
    {
        switch (ioPriorityClass)
        {
        case IoPriorityVeryLow:
            config.nDefaultRadioButton = PHAPP_ID_IOPRIORITY_VERYLOW;
            break;
        case IoPriorityLow:
            config.nDefaultRadioButton = PHAPP_ID_IOPRIORITY_LOW;
            break;
        case IoPriorityNormal:
            config.nDefaultRadioButton = PHAPP_ID_IOPRIORITY_NORMAL;
            break;
        case IoPriorityHigh:
            config.nDefaultRadioButton = PHAPP_ID_IOPRIORITY_HIGH;
            break;
        default:
            //config.dwFlags |= TDF_NO_DEFAULT_RADIO_BUTTON;
            config.nDefaultRadioButton = PHAPP_ID_IOPRIORITY_NORMAL;
            break;
        }
    }
    else
    {
        //config.dwFlags |= TDF_NO_DEFAULT_RADIO_BUTTON;
        config.nDefaultRadioButton = PHAPP_ID_IOPRIORITY_NORMAL;
    }

    TaskDialogIndirect(&config, NULL, NULL, NULL);

    PhFree(context);
}

VOID ShowProcessPagePriorityDialog(
    _In_ PPH_PLUGIN_MENU_ITEM MenuItem,
    _In_ PPH_STRING FileName
    )
{
    static TASKDIALOG_BUTTON TaskDialogRadioButtonArray[] =
    {
        { PHAPP_ID_PAGEPRIORITY_NORMAL, L"Normal" },
        { PHAPP_ID_PAGEPRIORITY_BELOWNORMAL, L"Below normal" },
        { PHAPP_ID_PAGEPRIORITY_MEDIUM, L"Medium" },
        { PHAPP_ID_PAGEPRIORITY_LOW , L"Low" },
        { PHAPP_ID_PAGEPRIORITY_VERYLOW, L"Very low" },
    };
    static TASKDIALOG_BUTTON TaskDialogButtonArray[] =
    {
        { IDYES, L"Save" },
        { IDCANCEL, L"Cancel" },
    };
    PUSERNOTES_TASK_IFEO_CONTEXT context;
    TASKDIALOGCONFIG config;
    ULONG pagePriorityClass;

    context = PhAllocateZero(sizeof(USERNOTES_TASK_IFEO_CONTEXT));
    context->RadioButton = ULONG_MAX;
    context->MenuCommand = MenuItem->Id;
    context->FileName = FileName;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS | TDF_POSITION_RELATIVE_TO_WINDOW;
    config.hMainIcon = PhGetApplicationIcon(FALSE);
    config.pszWindowTitle = PhGetString(FileName);
    config.pszMainInstruction = L"Select the default process page priority.";
    config.pszContent = L"The page priority will be applied by Windows even when System Informer isn't currently running.";
    config.nDefaultButton = IDCANCEL;
    config.pRadioButtons = TaskDialogRadioButtonArray;
    config.cRadioButtons = RTL_NUMBER_OF(TaskDialogRadioButtonArray);
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = RTL_NUMBER_OF(TaskDialogButtonArray);
    config.hwndParent = MenuItem->OwnerWindow;
    config.lpCallbackData = (LONG_PTR)context;
    config.pfCallback = TaskDialogBootstrapCallback;
    config.cxWidth = 200;

    if (FindIfeoObject(&FileName->sr, NULL, NULL, &pagePriorityClass))
    {
        switch (pagePriorityClass)
        {
        case MEMORY_PRIORITY_VERY_LOW:
            config.nDefaultRadioButton = PHAPP_ID_PAGEPRIORITY_VERYLOW;
            break;
        case MEMORY_PRIORITY_LOW:
            config.nDefaultRadioButton = PHAPP_ID_PAGEPRIORITY_LOW;
            break;
        case MEMORY_PRIORITY_MEDIUM:
            config.nDefaultRadioButton = PHAPP_ID_PAGEPRIORITY_MEDIUM;
            break;
        case MEMORY_PRIORITY_BELOW_NORMAL:
            config.nDefaultRadioButton = PHAPP_ID_PAGEPRIORITY_BELOWNORMAL;
            break;
        case MEMORY_PRIORITY_NORMAL:
            config.nDefaultRadioButton = PHAPP_ID_PAGEPRIORITY_NORMAL;
            break;
        default:
            //config.dwFlags |= TDF_NO_DEFAULT_RADIO_BUTTON;
            config.nDefaultRadioButton = PHAPP_ID_PAGEPRIORITY_NORMAL;
            break;
        }
    }
    else
    {
        //config.dwFlags |= TDF_NO_DEFAULT_RADIO_BUTTON;
        config.nDefaultRadioButton = PHAPP_ID_PAGEPRIORITY_NORMAL;
    }

    TaskDialogIndirect(&config, NULL, NULL, NULL);

    PhFree(context);
}

VOID NTAPI MenuItemCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;
    PPH_PROCESS_ITEM processItem;
    PDB_OBJECT object;

    switch (menuItem->Id)
    {
    case FILE_PRIORITY_SAVE_IFEO:
        {
            NTSTATUS status;
            PPH_STRING fileName;
            ULONG priorityClass;

            if (fileName = ShowFileDialog(menuItem->OwnerWindow))
            {
                PhMoveReference(&fileName, PhGetBaseName(fileName));

                if (FindIfeoObject(&fileName->sr, &priorityClass, NULL, NULL))
                {
                    status = DeleteIfeoObject(
                        &fileName->sr,
                        priorityClass,
                        ULONG_MAX,
                        ULONG_MAX
                        );

                    if (NT_SUCCESS(status))
                    {
                        PhShowInformation(menuItem->OwnerWindow, L"Successfully deleted the IFEO key.", status, 0);
                    }
                    else
                    {
                        PhShowStatus(menuItem->OwnerWindow, L"Unable to update the IFEO for priority.", status, 0);
                    }
                }
                else
                {
                    ShowProcessPriorityDialog(menuItem, fileName);
                }

                PhDereferenceObject(fileName);
            }
        }
        return;
    case FILE_IO_PRIORITY_SAVE_IFEO:
        {
            NTSTATUS status;
            PPH_STRING fileName;
            ULONG ioPriorityClass;

            if (fileName = ShowFileDialog(menuItem->OwnerWindow))
            {
                PhMoveReference(&fileName, PhGetBaseName(fileName));

                if (FindIfeoObject(&fileName->sr, NULL, &ioPriorityClass, NULL))
                {
                    status = DeleteIfeoObject(
                        &fileName->sr,
                        ULONG_MAX,
                        ioPriorityClass,
                        ULONG_MAX
                        );

                    if (NT_SUCCESS(status))
                    {
                        PhShowInformation(menuItem->OwnerWindow, L"Successfully deleted the IFEO key.", status, 0);
                    }
                    else
                    {
                        PhShowStatus(menuItem->OwnerWindow, L"Unable to update the IFEO for IO priority.", status, 0);
                    }
                }
                else
                {
                    ShowProcessIoPriorityDialog(menuItem, fileName);
                }

                PhDereferenceObject(fileName);
            }
        }
        return;
    case FILE_PAGE_PRIORITY_SAVE_IFEO:
        {
            NTSTATUS status;
            PPH_STRING fileName;
            ULONG pagePriorityClass;

            if (fileName = ShowFileDialog(menuItem->OwnerWindow))
            {
                PhMoveReference(&fileName, PhGetBaseName(fileName));

                if (FindIfeoObject(&fileName->sr, NULL, NULL, &pagePriorityClass))
                {
                    status = DeleteIfeoObject(
                        &fileName->sr,
                        ULONG_MAX,
                        ULONG_MAX,
                        pagePriorityClass
                        );

                    if (NT_SUCCESS(status))
                    {
                        PhShowInformation(menuItem->OwnerWindow, L"Successfully deleted the IFEO key.", status, 0);
                    }
                    else
                    {
                        PhShowStatus(menuItem->OwnerWindow, L"Unable to update the IFEO for page priority.", status, 0);
                    }
                }
                else
                {
                    ShowProcessPagePriorityDialog(menuItem, fileName);
                }

                PhDereferenceObject(fileName);
            }
        }
        return;
    }

    if (!(processItem = PhGetSelectedProcessItem()))
        return;

    PhReferenceObject(processItem);

    switch (menuItem->Id)
    {
    case PROCESS_PRIORITY_SAVE_ID:
        {
            LockDb();

            if ((object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr)) && object->PriorityClass != 0)
            {
                object->PriorityClass = 0;
                DeleteDbObjectForProcessIfUnused(object);
            }
            else
            {
                NTSTATUS status = STATUS_ACCESS_DENIED;
                UCHAR priorityClass = PROCESS_PRIORITY_CLASS_UNKNOWN;

                if (processItem->QueryHandle)
                {
                    status = PhGetProcessPriority(processItem->QueryHandle, &priorityClass);
                }

                if (NT_SUCCESS(status))
                {
                    object = CreateDbObject(FILE_TAG, &processItem->ProcessName->sr, NULL);
                    object->PriorityClass = priorityClass;
                }
                else
                {
                    PhShowStatus(menuItem->OwnerWindow, L"Unable to query priority.", status, 0);
                }
            }

            UnlockDb();
            SaveDb();
        }
        break;
    case PROCESS_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID:
        {
            if (processItem->CommandLine)
            {
                LockDb();

                if ((object = FindDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr)) && object->PriorityClass != 0)
                {
                    object->PriorityClass = 0;
                    DeleteDbObjectForProcessIfUnused(object);
                }
                else
                {
                    NTSTATUS status = STATUS_ACCESS_DENIED;
                    UCHAR priorityClass = PROCESS_PRIORITY_CLASS_UNKNOWN;

                    if (processItem->QueryHandle)
                    {
                        status = PhGetProcessPriority(processItem->QueryHandle, &priorityClass);
                    }

                    if (NT_SUCCESS(status))
                    {
                        object = CreateDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr, NULL);
                        object->PriorityClass = priorityClass;
                    }
                    else
                    {
                        PhShowStatus(menuItem->OwnerWindow, L"Unable to query priority.", status, 0);
                    }
                }

                UnlockDb();
                SaveDb();
            }
        }
        break;
    case PROCESS_PRIORITY_SAVE_IFEO:
        {
            ULONG priorityClass;
            NTSTATUS status;

            if (FindIfeoObject(&processItem->ProcessName->sr, &priorityClass, NULL, NULL))
            {
                status = DeleteIfeoObject(
                    &processItem->ProcessName->sr,
                    priorityClass,
                    ULONG_MAX,
                    ULONG_MAX
                    );

                if (!NT_SUCCESS(status))
                {
                    PhShowStatus(menuItem->OwnerWindow, L"Unable to update the IFEO for priority.", status, 0);
                }
            }
            else
            {
                ShowProcessPriorityDialog(menuItem, processItem->ProcessName);
                //status = CreateIfeoObject(&processItem->ProcessName->sr, processItem->PriorityClass, ULONG_MAX, ULONG_MAX);
            }
        }
        break;
    case PROCESS_IO_PRIORITY_SAVE_ID:
        {
            LockDb();

            if ((object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr)) && object->IoPriorityPlusOne != 0)
            {
                object->IoPriorityPlusOne = 0;
                DeleteDbObjectForProcessIfUnused(object);
            }
            else
            {
                NTSTATUS status = STATUS_ACCESS_DENIED;
                IO_PRIORITY_HINT ioPriority = IoPriorityNormal;

                if (processItem->QueryHandle)
                {
                    status = GetProcessIoPriority(processItem->QueryHandle, &ioPriority);
                }

                if (NT_SUCCESS(status))
                {
                    object = CreateDbObject(FILE_TAG, &processItem->ProcessName->sr, NULL);
                    object->IoPriorityPlusOne = ioPriority + 1;
                }
                else
                {
                    PhShowStatus(menuItem->OwnerWindow, L"Unable to query IO priority.", status, 0);
                }
            }

            UnlockDb();
            SaveDb();
        }
        break;
    case PROCESS_IO_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID:
        {
            if (processItem->CommandLine)
            {
                LockDb();

                if ((object = FindDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr)) && object->IoPriorityPlusOne != 0)
                {
                    object->IoPriorityPlusOne = 0;
                    DeleteDbObjectForProcessIfUnused(object);
                }
                else
                {
                    NTSTATUS status = STATUS_ACCESS_DENIED;
                    IO_PRIORITY_HINT ioPriority = PHAPP_ID_IOPRIORITY_NORMAL;

                    if (processItem->QueryHandle)
                    {
                        status = GetProcessIoPriority(processItem->QueryHandle, &ioPriority);
                    }

                    if (NT_SUCCESS(status))
                    {
                        object = CreateDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr, NULL);
                        object->IoPriorityPlusOne = ioPriority + 1;
                    }
                    else
                    {
                        PhShowStatus(menuItem->OwnerWindow, L"Unable to query IO priority.", status, 0);
                    }
                }

                UnlockDb();
                SaveDb();
            }
        }
        break;
    case PROCESS_IO_PRIORITY_SAVE_IFEO:
        {
            ULONG ioPriorityClass;

            if (FindIfeoObject(&processItem->ProcessName->sr, NULL, &ioPriorityClass, NULL))
            {
                NTSTATUS status;

                status = DeleteIfeoObject(
                    &processItem->ProcessName->sr,
                    ULONG_MAX,
                    ioPriorityClass,
                    ULONG_MAX
                    );

                if (!NT_SUCCESS(status))
                {
                    PhShowStatus(menuItem->OwnerWindow, L"Unable to update the IFEO for IO priority.", status, 0);
                }
            }
            else
            {
                ShowProcessIoPriorityDialog(menuItem, processItem->ProcessName);
                //IO_PRIORITY_HINT ioPriorityClass;
                //
                //if ((ioPriorityClass = GetProcessIoPriority(processItem->QueryHandle)) != ULONG_MAX)
                //{
                //    status = CreateIfeoObject(&processItem->ProcessName->sr, ULONG_MAX, ioPriorityClass, ULONG_MAX);
                //}
            }
        }
        break;
    case PROCESS_HIGHLIGHT_ID:
        {
            BOOLEAN highlightPresent = (BOOLEAN)menuItem->Context;

            if (!highlightPresent)
            {
                CHOOSECOLOR chooseColor;

                PhLoadCustomColorList(SETTING_NAME_CUSTOM_COLOR_LIST, ProcessCustomColors, RTL_NUMBER_OF(ProcessCustomColors));

                memset(&chooseColor, 0, sizeof(CHOOSECOLOR));
                chooseColor.lStructSize = sizeof(CHOOSECOLOR);
                chooseColor.hwndOwner = menuItem->OwnerWindow;
                chooseColor.lpCustColors = ProcessCustomColors;
                chooseColor.lpfnHook = ColorDlgHookProc;
                chooseColor.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_SOLIDCOLOR | CC_ENABLEHOOK;

                if (ChooseColor(&chooseColor))
                {
                    PhSaveCustomColorList(SETTING_NAME_CUSTOM_COLOR_LIST, ProcessCustomColors, RTL_NUMBER_OF(ProcessCustomColors));

                    LockDb();

                    object = CreateDbObject(FILE_TAG, &processItem->ProcessName->sr, NULL);
                    object->BackColor = chooseColor.rgbResult;

                    UnlockDb();
                    SaveDb();
                }
            }
            else
            {
                LockDb();

                if ((object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr)) && object->BackColor != ULONG_MAX)
                {
                    object->BackColor = ULONG_MAX;
                    DeleteDbObjectForProcessIfUnused(object);
                }

                UnlockDb();
                SaveDb();
            }

            PhInvalidateAllProcessNodes();
        }
        break;
    case PROCESS_COLLAPSE_ID:
        {
            LockDb();

            if ((object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr)) && object->Collapse)
            {
                object->Collapse = FALSE;
                DeleteDbObjectForProcessIfUnused(object);
            }
            else
            {
                object = CreateDbObject(FILE_TAG, &processItem->ProcessName->sr, NULL);
                object->Collapse = TRUE;
            }

            UnlockDb();
            SaveDb();
        }
        break;
    case PROCESS_AFFINITY_ID:
        {
            BOOLEAN changed = FALSE;
            KAFFINITY affinityMask;

            // Show the affinity dialog (with our values).
            if (PhShowProcessAffinityDialog2(menuItem->OwnerWindow, processItem, &affinityMask))
            {
                PDB_OBJECT object;

                LockDb();

                if (object = FindDbObjectForProcess(processItem, INTENT_PROCESS_AFFINITY))
                {
                    // Update the process affinity in our database (if the database values are different).
                    if (object->AffinityMask != affinityMask)
                    {
                        object->AffinityMask = affinityMask;
                        changed = TRUE;
                    }
                }

                UnlockDb();

                if (changed)
                {
                    SaveDb();
                }
            }
        }
        break;
    case PROCESS_AFFINITY_SAVE_ID:
        {
            LockDb();

            if ((object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr)) && object->AffinityMask != 0)
            {
                object->AffinityMask = 0;
                DeleteDbObjectForProcessIfUnused(object);
            }
            else
            {
                NTSTATUS status = STATUS_ACCESS_DENIED;
                KAFFINITY affinityMask = SIZE_MAX;

                if (processItem->QueryHandle)
                {
                    status = GetProcessAffinity(processItem->QueryHandle, &affinityMask);
                }

                if (NT_SUCCESS(status))
                {
                    object = CreateDbObject(FILE_TAG, &processItem->ProcessName->sr, NULL);
                    object->AffinityMask = affinityMask;
                }
                else
                {
                    USHORT groupBuffer[20] = { 0 };
                    USHORT groupCount = RTL_NUMBER_OF(groupBuffer);

                    if (processItem->QueryHandle && NT_SUCCESS(PhGetProcessGroupInformation(
                        processItem->QueryHandle,
                        &groupCount,
                        groupBuffer
                        )) && groupCount > 1)
                    {
                        PhShowInformation2(
                            menuItem->OwnerWindow,
                            L"Unable to query the current affinity.",
                            L"This process has multi-group affinity, %s",
                            L"you can only change affinity for individual threads."
                            );
                    }
                    else
                    {
                        PhShowStatus(menuItem->OwnerWindow, L"Unable to query process affinity.", status, 0);
                    }
                }
            }

            UnlockDb();
            SaveDb();
        }
        break;
    case PROCESS_AFFINITY_SAVE_FOR_THIS_COMMAND_LINE_ID:
        {
            if (processItem->CommandLine)
            {
                LockDb();

                if ((object = FindDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr)) && object->AffinityMask != 0)
                {
                    object->AffinityMask = 0;
                    DeleteDbObjectForProcessIfUnused(object);
                }
                else
                {
                    NTSTATUS status = STATUS_ACCESS_DENIED;
                    KAFFINITY affinityMask = SIZE_MAX;

                    if (processItem->QueryHandle)
                    {
                        status = GetProcessAffinity(processItem->QueryHandle, &affinityMask);
                    }

                    if (NT_SUCCESS(status))
                    {
                        object = CreateDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr, NULL);
                        object->AffinityMask = affinityMask;
                    }
                    else
                    {
                        USHORT groupBuffer[20] = { 0 };
                        USHORT groupCount = RTL_NUMBER_OF(groupBuffer);

                        if (processItem->QueryHandle && NT_SUCCESS(PhGetProcessGroupInformation(
                            processItem->QueryHandle,
                            &groupCount,
                            groupBuffer
                            )) && groupCount > 1)
                        {
                            PhShowInformation2(
                                menuItem->OwnerWindow,
                                L"Unable to query the current affinity.",
                                L"This process has multi-group affinity, %s",
                                L"you can only change affinity for individual threads."
                                );
                        }
                        else
                        {
                            PhShowStatus(menuItem->OwnerWindow, L"Unable to query process affinity.", status, 0);
                        }
                    }
                }

                UnlockDb();
                SaveDb();
            }
        }
        break;
    case PROCESS_PAGE_PRIORITY_SAVE_ID:
        {
            LockDb();

            if ((object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr)) && object->PagePriorityPlusOne != 0)
            {
                object->PagePriorityPlusOne = 0;
                DeleteDbObjectForProcessIfUnused(object);
            }
            else
            {
                NTSTATUS status = STATUS_ACCESS_DENIED;
                ULONG pagePriority = MEMORY_PRIORITY_NORMAL;

                if (processItem->QueryHandle)
                {
                    status = GetProcessPagePriority(processItem->QueryHandle, &pagePriority);
                }

                if (NT_SUCCESS(status))
                {
                    object = CreateDbObject(FILE_TAG, &processItem->ProcessName->sr, NULL);
                    object->PagePriorityPlusOne = pagePriority + 1;
                }
                else
                {
                    PhShowStatus(menuItem->OwnerWindow, L"Unable to query page priority.", status, 0);
                }
            }

            UnlockDb();
            SaveDb();
        }
        break;
    case PROCESS_PAGE_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID:
        {
            if (processItem->CommandLine)
            {
                LockDb();

                if ((object = FindDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr)) && object->PagePriorityPlusOne != 0)
                {
                    object->PagePriorityPlusOne = 0;
                    DeleteDbObjectForProcessIfUnused(object);
                }
                else
                {
                    NTSTATUS status = STATUS_ACCESS_DENIED;
                    ULONG pagePriority = MEMORY_PRIORITY_NORMAL;

                    if (processItem->QueryHandle)
                    {
                        status = GetProcessPagePriority(processItem->QueryHandle, &pagePriority);
                    }

                    if (NT_SUCCESS(status))
                    {
                        object = CreateDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr, NULL);
                        object->PagePriorityPlusOne = pagePriority + 1;
                    }
                    else
                    {
                        PhShowStatus(menuItem->OwnerWindow, L"Unable to query page priority.", status, 0);
                    }
                }

                UnlockDb();
                SaveDb();
            }
        }
        break;
    case PROCESS_PAGE_PRIORITY_SAVE_IFEO:
        {
            ULONG pagePriorityClass;

            if (FindIfeoObject(&processItem->ProcessName->sr, NULL, NULL, &pagePriorityClass))
            {
                NTSTATUS status;

                status = DeleteIfeoObject(
                    &processItem->ProcessName->sr,
                    ULONG_MAX,
                    ULONG_MAX,
                    pagePriorityClass
                    );

                if (!NT_SUCCESS(status))
                {
                    PhShowStatus(menuItem->OwnerWindow, L"Unable to update the IFEO for page priority.", status, 0);
                }
            }
            else
            {
                ShowProcessPagePriorityDialog(menuItem, processItem->ProcessName);
                //if ((pagePriorityClass = GetProcessPagePriority(processItem->QueryHandle)) != ULONG_MAX)
                //{
                //    status = CreateIfeoObject(&processItem->ProcessName->sr, ULONG_MAX, ULONG_MAX, pagePriorityClass);
                //}
            }
        }
        break;
    case PROCESS_BOOST_PRIORITY_ID:
        {
            NTSTATUS status = STATUS_ACCESS_DENIED;
            BOOLEAN priorityBoostDisabled = FALSE;

            if (processItem->QueryHandle)
            {
                status = PhGetProcessPriorityBoost(processItem->QueryHandle, &priorityBoostDisabled);
            }

            if (NT_SUCCESS(status))
            {
                HANDLE processHandle;

                status = PhOpenProcess(
                    &processHandle,
                    PROCESS_SET_INFORMATION,
                    processItem->ProcessId
                    );

                if (NT_SUCCESS(status))
                {
                    status = PhSetProcessPriorityBoost(processHandle, !priorityBoostDisabled);
                    NtClose(processHandle);
                }
            }

            if (!NT_SUCCESS(status))
            {
                PhShowStatus(menuItem->OwnerWindow, L"Unable to query process boost.", status, 0);
            }
        }
        break;
    case PROCESS_BOOST_PRIORITY_SAVE_ID:
        {
            LockDb();

            if ((object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr)) && object->Boost)
            {
                object->Boost = FALSE;
                DeleteDbObjectForProcessIfUnused(object);
            }
            else
            {
                NTSTATUS status = STATUS_ACCESS_DENIED;
                BOOLEAN priorityBoostDisabled = FALSE;

                if (processItem->QueryHandle)
                {
                    status = PhGetProcessPriorityBoost(processItem->QueryHandle, &priorityBoostDisabled);
                }

                if (NT_SUCCESS(status))
                {
                    object = CreateDbObject(FILE_TAG, &processItem->ProcessName->sr, NULL);
                    object->Boost = TRUE; // intentional
                }
                else
                {
                    PhShowStatus(menuItem->OwnerWindow, L"Unable to query process boost.", status, 0);
                }
            }

            UnlockDb();
            SaveDb();
        }
        break;
    case PROCESS_BOOST_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID:
        {
            if (processItem->CommandLine)
            {
                LockDb();

                if ((object = FindDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr)) && object->Boost)
                {
                    object->Boost = FALSE;
                    DeleteDbObjectForProcessIfUnused(object);
                }
                else
                {
                    NTSTATUS status = STATUS_ACCESS_DENIED;
                    BOOLEAN priorityBoostDisabled = FALSE;

                    if (processItem->QueryHandle)
                    {
                        status = PhGetProcessPriorityBoost(processItem->QueryHandle, &priorityBoostDisabled);
                    }

                    if (NT_SUCCESS(status))
                    {
                        object = CreateDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr, NULL);
                        object->Boost = TRUE; // intentional
                    }
                    else
                    {
                        PhShowStatus(menuItem->OwnerWindow, L"Unable to query process boost.", status, 0);
                    }
                }

                UnlockDb();
                SaveDb();
            }
        }
        break;
    }

    PhDereferenceObject(processItem);
}

VOID NTAPI MenuHookCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_HOOK_INFORMATION menuHookInfo = Parameter;
    ULONG id;

    id = menuHookInfo->SelectedItem->Id;

    switch (id)
    {
    case PHAPP_ID_PRIORITY_REALTIME:
    case PHAPP_ID_PRIORITY_HIGH:
    case PHAPP_ID_PRIORITY_ABOVENORMAL:
    case PHAPP_ID_PRIORITY_NORMAL:
    case PHAPP_ID_PRIORITY_BELOWNORMAL:
    case PHAPP_ID_PRIORITY_IDLE:
        {
            BOOLEAN changed = FALSE;
            PPH_PROCESS_ITEM *processes;
            ULONG numberOfProcesses;
            ULONG i;

            PhGetSelectedProcessItems(&processes, &numberOfProcesses);
            LockDb();

            for (i = 0; i < numberOfProcesses; i++)
            {
                PDB_OBJECT object;

                if (object = FindDbObjectForProcess(processes[i], INTENT_PROCESS_PRIORITY_CLASS))
                {
                    ULONG newPriorityClass = GetPriorityClassFromId(id);

                    if (object->PriorityClass != newPriorityClass)
                    {
                        object->PriorityClass = newPriorityClass;
                        changed = TRUE;
                    }
                }
            }

            UnlockDb();
            PhFree(processes);

            if (changed)
                SaveDb();
        }
        break;
    case PHAPP_ID_IOPRIORITY_VERYLOW:
    case PHAPP_ID_IOPRIORITY_LOW:
    case PHAPP_ID_IOPRIORITY_NORMAL:
    case PHAPP_ID_IOPRIORITY_HIGH:
        {
            BOOLEAN changed = FALSE;
            PPH_PROCESS_ITEM *processes;
            ULONG numberOfProcesses;
            ULONG i;

            PhGetSelectedProcessItems(&processes, &numberOfProcesses);
            LockDb();

            for (i = 0; i < numberOfProcesses; i++)
            {
                PDB_OBJECT object;

                if (object = FindDbObjectForProcess(processes[i], INTENT_PROCESS_IO_PRIORITY))
                {
                    ULONG newIoPriorityPlusOne = GetIoPriorityFromId(id) + 1;

                    if (object->IoPriorityPlusOne != newIoPriorityPlusOne)
                    {
                        object->IoPriorityPlusOne = newIoPriorityPlusOne;
                        changed = TRUE;
                    }
                }
            }

            UnlockDb();
            PhFree(processes);

            if (changed)
                SaveDb();
        }
        break;
    case PHAPP_ID_PAGEPRIORITY_VERYLOW:
    case PHAPP_ID_PAGEPRIORITY_LOW:
    case PHAPP_ID_PAGEPRIORITY_MEDIUM:
    case PHAPP_ID_PAGEPRIORITY_BELOWNORMAL:
    case PHAPP_ID_PAGEPRIORITY_NORMAL:
        {
            BOOLEAN changed = FALSE;
            PPH_PROCESS_ITEM *processes;
            ULONG numberOfProcesses;
            ULONG i;

            PhGetSelectedProcessItems(&processes, &numberOfProcesses);
            LockDb();

            for (i = 0; i < numberOfProcesses; i++)
            {
                PDB_OBJECT object;

                if (object = FindDbObjectForProcess(processes[i], INTENT_PROCESS_PAGEPRIORITY))
                {
                    ULONG newPagePriorityPlusOne = GetPagePriorityFromId(id) + 1;

                    if (object->PagePriorityPlusOne != newPagePriorityPlusOne)
                    {
                        object->PagePriorityPlusOne = newPagePriorityPlusOne;
                        changed = TRUE;
                    }
                }
            }

            UnlockDb();
            PhFree(processes);

            if (changed)
                SaveDb();
        }
        break;
    }
}

VOID InvalidateProcessComments(
    VOID
    )
{
    PLIST_ENTRY listEntry;

    PhAcquireQueuedLockExclusive(&ProcessListLock);

    listEntry = ProcessListHead.Flink;

    while (listEntry != &ProcessListHead)
    {
        PPROCESS_EXTENSION extension;

        extension = CONTAINING_RECORD(listEntry, PROCESS_EXTENSION, ListEntry);

        extension->Valid = FALSE;

        listEntry = listEntry->Flink;
    }

    PhReleaseQueuedLockExclusive(&ProcessListLock);
}

VOID UpdateProcessComment(
    _In_ PPH_PROCESS_NODE Node,
    _In_ PPROCESS_EXTENSION Extension
    )
{
    if (!Extension->Valid)
    {
        PDB_OBJECT object;

        LockDb();

        if (object = FindDbObjectForProcess(Node->ProcessItem, INTENT_PROCESS_COMMENT))
        {
            PhSwapReference(&Extension->Comment, object->Comment);
        }
        else
        {
            PhClearReference(&Extension->Comment);
        }

        UnlockDb();

        Extension->Valid = TRUE;
    }
}

VOID InvalidateServiceComments(
    VOID
    )
{
    PLIST_ENTRY listEntry;

    PhAcquireQueuedLockExclusive(&ServiceListLock);

    listEntry = ServiceListHead.Flink;

    while (listEntry != &ServiceListHead)
    {
        PSERVICE_EXTENSION extension;

        extension = CONTAINING_RECORD(listEntry, SERVICE_EXTENSION, ListEntry);

        extension->Valid = FALSE;

        listEntry = listEntry->Flink;
    }

    PhReleaseQueuedLockExclusive(&ServiceListLock);
}

VOID UpdateServiceComment(
    _In_ PPH_SERVICE_NODE Node,
    _In_ PSERVICE_EXTENSION Extension
    )
{
    if (!Extension->Valid)
    {
        PDB_OBJECT object;

        LockDb();

        if (object = FindDbObject(SERVICE_TAG, &Node->ServiceItem->Name->sr))
        {
            PhSwapReference(&Extension->Comment, object->Comment);
        }
        else
        {
            PhClearReference(&Extension->Comment);
        }

        UnlockDb();

        Extension->Valid = TRUE;
    }
}

VOID TreeNewMessageCallback(
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

            if (message->TreeNewHandle == ProcessTreeNewHandle)
            {
                PPH_PROCESS_NODE node;

                node = (PPH_PROCESS_NODE)getCellText->Node;

                switch (message->SubId)
                {
                case COMMENT_COLUMN_ID:
                    {
                        PPROCESS_EXTENSION extension;

                        extension = PhPluginGetObjectExtension(PluginInstance, node->ProcessItem, EmProcessItemType);
                        UpdateProcessComment(node, extension);
                        getCellText->Text = PhGetStringRef(extension->Comment);
                    }
                    break;
                }
            }
            else if (message->TreeNewHandle == ServiceTreeNewHandle)
            {
                PPH_SERVICE_NODE node;

                node = (PPH_SERVICE_NODE)getCellText->Node;

                switch (message->SubId)
                {
                case COMMENT_COLUMN_ID:
                    {
                        PSERVICE_EXTENSION extension;

                        extension = PhPluginGetObjectExtension(PluginInstance, node->ServiceItem, EmServiceItemType);
                        UpdateServiceComment(node, extension);
                        getCellText->Text = PhGetStringRef(extension->Comment);
                    }
                    break;
                }
            }
        }
        break;
    }
}

VOID MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (ToolStatusInterface)
    {
        PhRegisterCallback(ToolStatusInterface->SearchChangedEvent, SearchChangedHandler, NULL, &SearchChangedRegistration);
    }
}

VOID ProcessPropertiesInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_PROCESS_PROPCONTEXT propContext = Parameter;

    PhAddProcessPropPage(
        propContext->PropContext,
        PhCreateProcessPropPageContextEx(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_PROCCOMMENT), ProcessCommentPageDlgProc, NULL)
        );
}

VOID AddSavePriorityMenuItemsAndHook(
    _In_ PPH_PLUGIN_MENU_INFORMATION MenuInfo,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ BOOLEAN UseSelectionForHook
    )
{
    PPH_EMENU_ITEM affinityMenuItem;
    PPH_EMENU_ITEM boostMenuItem;
    PPH_EMENU_ITEM boostPluginMenuItem;
    PPH_EMENU_ITEM priorityMenuItem;
    PPH_EMENU_ITEM ioPriorityMenuItem;
    PPH_EMENU_ITEM pagePriorityMenuItem;
    PPH_EMENU_ITEM saveMenuItem;
    PPH_EMENU_ITEM saveForCommandLineMenuItem;
    PPH_EMENU_ITEM saveIfeoMenuItem;
    PDB_OBJECT object;
    ULONG objectIfeo;

    if (affinityMenuItem = PhFindEMenuItem(MenuInfo->Menu, 0, NULL, PHAPP_ID_PROCESS_AFFINITY))
    {
        // HACK: Change the affinity menu into a drop-down list.
        ULONG index = PhIndexOfEMenuItem(MenuInfo->Menu, affinityMenuItem);
        PhRemoveEMenuItem(MenuInfo->Menu, affinityMenuItem, 0);

        affinityMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"&Affinity", NULL);
        PhInsertEMenuItem(affinityMenuItem, PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_AFFINITY_ID, L"Set &affinity", NULL), ULONG_MAX);
        PhInsertEMenuItem(affinityMenuItem, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(affinityMenuItem, saveMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_AFFINITY_SAVE_ID, PhaFormatString(L"&Save for %s", ProcessItem->ProcessName->Buffer)->Buffer, NULL), ULONG_MAX);
        PhInsertEMenuItem(affinityMenuItem, saveForCommandLineMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_AFFINITY_SAVE_FOR_THIS_COMMAND_LINE_ID, L"Save &for this command line", NULL), ULONG_MAX);
        PhInsertEMenuItem(MenuInfo->Menu, affinityMenuItem, index);

        if (!ProcessItem->CommandLine)
            saveForCommandLineMenuItem->Flags |= PH_EMENU_DISABLED;

        LockDb();

        if ((object = FindDbObject(FILE_TAG, &ProcessItem->ProcessName->sr)) && object->AffinityMask != 0)
            saveMenuItem->Flags |= PH_EMENU_CHECKED;
        if (ProcessItem->CommandLine && (object = FindDbObject(COMMAND_LINE_TAG, &ProcessItem->CommandLine->sr)) && object->AffinityMask != 0)
            saveForCommandLineMenuItem->Flags |= PH_EMENU_CHECKED;

        UnlockDb();
    }

    // Boost
    if (affinityMenuItem)
    {
        boostMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"&Boost", NULL);
        PhInsertEMenuItem(boostMenuItem, boostPluginMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_BOOST_PRIORITY_ID, L"Set &boost", NULL), ULONG_MAX);
        PhInsertEMenuItem(boostMenuItem, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(boostMenuItem, saveMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_BOOST_PRIORITY_SAVE_ID, PhaFormatString(L"&Save for %s", ProcessItem->ProcessName->Buffer)->Buffer, NULL), ULONG_MAX);
        PhInsertEMenuItem(boostMenuItem, saveForCommandLineMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_BOOST_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID, L"Save &for this command line", NULL), ULONG_MAX);
        PhInsertEMenuItem(MenuInfo->Menu, boostMenuItem, PhIndexOfEMenuItem(MenuInfo->Menu, affinityMenuItem) + 1);

        if (!ProcessItem->CommandLine)
            saveForCommandLineMenuItem->Flags |= PH_EMENU_DISABLED;

        LockDb();

        if ((object = FindDbObject(FILE_TAG, &ProcessItem->ProcessName->sr)) && object->Boost)
            saveMenuItem->Flags |= PH_EMENU_CHECKED;
        if (ProcessItem->CommandLine && (object = FindDbObject(COMMAND_LINE_TAG, &ProcessItem->CommandLine->sr)) && object->Boost)
            saveForCommandLineMenuItem->Flags |= PH_EMENU_CHECKED;

        UnlockDb();

        if (ProcessItem->QueryHandle)
        {
            BOOLEAN priorityBoostDisabled = FALSE;

            if (NT_SUCCESS(PhGetProcessPriorityBoost(ProcessItem->QueryHandle, &priorityBoostDisabled)) && !priorityBoostDisabled)
            {
                boostPluginMenuItem->Flags |= PH_EMENU_CHECKED;
            }
        }
    }

    // Priority
    if (priorityMenuItem = PhFindEMenuItem(MenuInfo->Menu, 0, NULL, PHAPP_ID_PROCESS_PRIORITY))
    {
        PhInsertEMenuItem(priorityMenuItem, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(priorityMenuItem, saveMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_PRIORITY_SAVE_ID, PhaFormatString(L"&Save for %s", ProcessItem->ProcessName->Buffer)->Buffer, NULL), ULONG_MAX);
        PhInsertEMenuItem(priorityMenuItem, saveForCommandLineMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID, L"Save &for this command line", NULL), ULONG_MAX);
        PhInsertEMenuItem(priorityMenuItem, saveIfeoMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_PRIORITY_SAVE_IFEO, PhaFormatString(L"&Save for %s (IFEO)", ProcessItem->ProcessName->Buffer)->Buffer, NULL), ULONG_MAX);

        if (!ProcessItem->CommandLine)
            saveForCommandLineMenuItem->Flags |= PH_EMENU_DISABLED;

        LockDb();

        if ((object = FindDbObject(FILE_TAG, &ProcessItem->ProcessName->sr)) && object->PriorityClass != 0)
            saveMenuItem->Flags |= PH_EMENU_CHECKED;
        if (ProcessItem->CommandLine && (object = FindDbObject(COMMAND_LINE_TAG, &ProcessItem->CommandLine->sr)) && object->PriorityClass != 0)
            saveForCommandLineMenuItem->Flags |= PH_EMENU_CHECKED;

        UnlockDb();

        if (FindIfeoObject(&ProcessItem->ProcessName->sr, &objectIfeo, NULL, NULL))
        {
            saveIfeoMenuItem->Flags |= PH_EMENU_CHECKED;
        }
    }

    // I/O Priority
    if (ioPriorityMenuItem = PhFindEMenuItem(MenuInfo->Menu, 0, NULL, PHAPP_ID_PROCESS_IOPRIORITY))
    {
        PhInsertEMenuItem(ioPriorityMenuItem, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(ioPriorityMenuItem, saveMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_IO_PRIORITY_SAVE_ID, PhaFormatString(L"&Save for %s", ProcessItem->ProcessName->Buffer)->Buffer, NULL), ULONG_MAX);
        PhInsertEMenuItem(ioPriorityMenuItem, saveForCommandLineMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_IO_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID, L"Save &for this command line", NULL), ULONG_MAX);
        PhInsertEMenuItem(ioPriorityMenuItem, saveIfeoMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_IO_PRIORITY_SAVE_IFEO, PhaFormatString(L"&Save for %s (IFEO)", ProcessItem->ProcessName->Buffer)->Buffer, NULL), ULONG_MAX);

        if (!ProcessItem->CommandLine)
            saveForCommandLineMenuItem->Flags |= PH_EMENU_DISABLED;

        LockDb();

        if ((object = FindDbObject(FILE_TAG, &ProcessItem->ProcessName->sr)) && object->IoPriorityPlusOne != 0)
            saveMenuItem->Flags |= PH_EMENU_CHECKED;
        if (ProcessItem->CommandLine && (object = FindDbObject(COMMAND_LINE_TAG, &ProcessItem->CommandLine->sr)) && object->IoPriorityPlusOne != 0)
            saveForCommandLineMenuItem->Flags |= PH_EMENU_CHECKED;

        UnlockDb();

        if (FindIfeoObject(&ProcessItem->ProcessName->sr, NULL, &objectIfeo, NULL))
        {
            saveIfeoMenuItem->Flags |= PH_EMENU_CHECKED;
        }
    }

    // Page Priority
    if (pagePriorityMenuItem = PhFindEMenuItem(MenuInfo->Menu, 0, NULL, PHAPP_ID_PROCESS_PAGEPRIORITY))
    {
        PhInsertEMenuItem(pagePriorityMenuItem, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(pagePriorityMenuItem, saveMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_PAGE_PRIORITY_SAVE_ID, PhaFormatString(L"&Save for %s", ProcessItem->ProcessName->Buffer)->Buffer, NULL), ULONG_MAX);
        PhInsertEMenuItem(pagePriorityMenuItem, saveForCommandLineMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_PAGE_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID, L"Save &for this command line", NULL), ULONG_MAX);
        PhInsertEMenuItem(pagePriorityMenuItem, saveIfeoMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_PAGE_PRIORITY_SAVE_IFEO, PhaFormatString(L"&Save for %s (IFEO)", ProcessItem->ProcessName->Buffer)->Buffer, NULL), ULONG_MAX);

        if (!ProcessItem->CommandLine)
            saveForCommandLineMenuItem->Flags |= PH_EMENU_DISABLED;

        LockDb();

        if ((object = FindDbObject(FILE_TAG, &ProcessItem->ProcessName->sr)) && object->PagePriorityPlusOne != 0)
            saveMenuItem->Flags |= PH_EMENU_CHECKED;
        if (ProcessItem->CommandLine && (object = FindDbObject(COMMAND_LINE_TAG, &ProcessItem->CommandLine->sr)) && object->PagePriorityPlusOne != 0)
            saveForCommandLineMenuItem->Flags |= PH_EMENU_CHECKED;

        UnlockDb();

        if (FindIfeoObject(&ProcessItem->ProcessName->sr, NULL, NULL, &objectIfeo))
        {
            saveIfeoMenuItem->Flags |= PH_EMENU_CHECKED;
        }
    }

    PhPluginAddMenuHook(MenuInfo, PluginInstance, UseSelectionForHook ? NULL : ProcessItem->ProcessId);
}

VOID ProcessMenuInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    BOOLEAN highlightPresent = FALSE;
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_PROCESS_ITEM processItem;
    PPH_EMENU_ITEM miscMenuItem;
    PPH_EMENU_ITEM collapseMenuItem;
    PPH_EMENU_ITEM highlightMenuItem;
    PDB_OBJECT object;

    if (menuInfo->u.Process.NumberOfProcesses != 1)
        return;

    processItem = menuInfo->u.Process.Processes[0];

    if (!PH_IS_FAKE_PROCESS_ID(processItem->ProcessId) && processItem->ProcessId != SYSTEM_IDLE_PROCESS_ID && processItem->ProcessId != SYSTEM_PROCESS_ID)
        AddSavePriorityMenuItemsAndHook(menuInfo, processItem, TRUE);

    if (!(miscMenuItem = PhFindEMenuItem(menuInfo->Menu, 0, NULL, PHAPP_ID_PROCESS_MISCELLANEOUS)))
        return;

    LockDb();
    if ((object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr)) && object->BackColor != ULONG_MAX)
        highlightPresent = TRUE;
    UnlockDb();

    PhInsertEMenuItem(miscMenuItem, collapseMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_COLLAPSE_ID, L"Col&lapse by default", NULL), 0);
    PhInsertEMenuItem(miscMenuItem, highlightMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_HIGHLIGHT_ID, L"Highligh&t", UlongToPtr(highlightPresent)), 1);
    PhInsertEMenuItem(miscMenuItem, PhCreateEMenuSeparator(), 2);

    LockDb();

    if ((object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr)) && object->Collapse)
        collapseMenuItem->Flags |= PH_EMENU_CHECKED;
    if (highlightPresent)
        highlightMenuItem->Flags |= PH_EMENU_CHECKED;

    UnlockDb();
}

static LONG NTAPI ProcessCommentSortFunction(
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

    UpdateProcessComment(node1, extension1);
    UpdateProcessComment(node2, extension2);

    return PhCompareStringWithNull(extension1->Comment, extension2->Comment, TRUE);
}

VOID ProcessTreeNewInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PH_TREENEW_COLUMN column;

    ProcessTreeNewHandle = info->TreeNewHandle;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Comment";
    column.Width = 120;
    column.Alignment = PH_ALIGN_LEFT;

    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, COMMENT_COLUMN_ID, NULL, ProcessCommentSortFunction);
}

VOID GetProcessHighlightingColorCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_GET_HIGHLIGHTING_COLOR getHighlightingColor = Parameter;
    PPH_PROCESS_ITEM processItem;
    PDB_OBJECT object;

    processItem = (PPH_PROCESS_ITEM)getHighlightingColor->Parameter;

    if (getHighlightingColor->Handled)
        return;

    LockDb();

    if ((object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr)) && object->BackColor != ULONG_MAX)
    {
        getHighlightingColor->BackColor = object->BackColor;
        getHighlightingColor->Cache = TRUE;
        getHighlightingColor->Handled = TRUE;
    }

    UnlockDb();
}

VOID ServicePropertiesInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_OBJECT_PROPERTIES objectProperties = Parameter;
    PROPSHEETPAGE propSheetPage;

    if (objectProperties->NumberOfPages < objectProperties->MaximumNumberOfPages)
    {
        memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
        propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
        propSheetPage.dwFlags = PSP_USETITLE;
        propSheetPage.hInstance = PluginInstance->DllBase;
        propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_SRVCOMMENT);
        propSheetPage.pszTitle = L"Comment";
        propSheetPage.pfnDlgProc = ServiceCommentPageDlgProc;
        propSheetPage.lParam = (LPARAM)objectProperties->Parameter;
        objectProperties->Pages[objectProperties->NumberOfPages++] = CreatePropertySheetPage(&propSheetPage);
    }
}

LONG NTAPI ServiceCommentSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PVOID Context
    )
{
    PPH_SERVICE_NODE node1 = Node1;
    PPH_SERVICE_NODE node2 = Node2;
    PSERVICE_EXTENSION extension1 = PhPluginGetObjectExtension(PluginInstance, node1->ServiceItem, EmServiceItemType);
    PSERVICE_EXTENSION extension2 = PhPluginGetObjectExtension(PluginInstance, node2->ServiceItem, EmServiceItemType);

    UpdateServiceComment(node1, extension1);
    UpdateServiceComment(node2, extension2);

    return PhCompareStringWithNull(extension1->Comment, extension2->Comment, TRUE);
}

VOID ServiceTreeNewInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PH_TREENEW_COLUMN column;

    ServiceTreeNewHandle = info->TreeNewHandle;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Comment";
    column.Width = 120;
    column.Alignment = PH_ALIGN_LEFT;

    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, COMMENT_COLUMN_ID, NULL, ServiceCommentSortFunction);
}

VOID MiListSectionMenuInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_PROCESS_ITEM processItem;

    processItem = menuInfo->u.MiListSection.ProcessGroup->Representative;

    if (PH_IS_FAKE_PROCESS_ID(processItem->ProcessId) || processItem->ProcessId == SYSTEM_IDLE_PROCESS_ID || processItem->ProcessId == SYSTEM_PROCESS_ID)
        return;

    AddSavePriorityMenuItemsAndHook(menuInfo, processItem, FALSE);
}

VOID ProcessModifiedCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = Parameter;
    PPROCESS_EXTENSION extension;

    extension = PhPluginGetObjectExtension(PluginInstance, processItem, EmProcessItemType);
    extension->Valid = FALSE;
}

VOID ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PLIST_ENTRY listEntry;

    if (GetNumberOfDbObjects() == 0)
        return;

    PhAcquireQueuedLockExclusive(&ProcessListLock);
    LockDb();

    listEntry = ProcessListHead.Flink;

    while (listEntry != &ProcessListHead)
    {
        PPROCESS_EXTENSION extension;
        PPH_PROCESS_ITEM processItem;
        PDB_OBJECT object;

        extension = CONTAINING_RECORD(listEntry, PROCESS_EXTENSION, ListEntry);
        processItem = extension->ProcessItem;

        if (object = FindDbObjectForProcess(processItem, INTENT_PROCESS_PRIORITY_CLASS))
        {
            if (processItem->PriorityClass != object->PriorityClass && !extension->SkipPriority)
            {
                if (!NT_SUCCESS(PhSetProcessItemPriority(processItem, (UCHAR)object->PriorityClass)))
                {
                    extension->SkipPriority = TRUE;
                }
            }
        }

        if (object = FindDbObjectForProcess(processItem, INTENT_PROCESS_IO_PRIORITY))
        {
            if (object->IoPriorityPlusOne != 0 && !extension->SkipIoPriority)
            {
                IO_PRIORITY_HINT ioPriority;

                if (processItem->QueryHandle && NT_SUCCESS(GetProcessIoPriority(processItem->QueryHandle, &ioPriority)))
                {
                    if (ioPriority != object->IoPriorityPlusOne - 1)
                    {
                        if (!NT_SUCCESS(PhSetProcessItemIoPriority(processItem, object->IoPriorityPlusOne - 1)))
                        {
                            extension->SkipIoPriority = TRUE;
                        }
                    }
                }
            }
        }

        if (object = FindDbObjectForProcess(processItem, INTENT_PROCESS_AFFINITY))
        {
            if (object->AffinityMask != 0 && !extension->SkipAffinity)
            {
                KAFFINITY affinityMask;

                if (processItem->QueryHandle && NT_SUCCESS(GetProcessAffinity(processItem->QueryHandle, &affinityMask)))
                {
                    if (affinityMask != object->AffinityMask)
                    {
                        if (!NT_SUCCESS(PhSetProcessItemAffinityMask(processItem, object->AffinityMask)))
                        {
                            extension->SkipAffinity = TRUE;
                        }
                    }
                }
            }
        }

        if (object = FindDbObjectForProcess(processItem, INTENT_PROCESS_PAGEPRIORITY))
        {
            if (object->PagePriorityPlusOne != 0 && !extension->SkipPagePriority)
            {
                ULONG pagePriority;

                if (processItem->QueryHandle && NT_SUCCESS(GetProcessPagePriority(processItem->QueryHandle, &pagePriority)))
                {
                    if (pagePriority != object->PagePriorityPlusOne - 1)
                    {
                        if (!NT_SUCCESS(PhSetProcessItemPagePriority(processItem, object->PagePriorityPlusOne - 1)))
                        {
                            extension->SkipPagePriority = TRUE;
                        }
                    }
                }
            }
        }

        if (object = FindDbObjectForProcess(processItem, INTENT_PROCESS_BOOST))
        {
            if (object->Boost && !extension->SkipBoostPriority)
            {
                BOOLEAN priorityBoostDisabled = FALSE;

                if (processItem->QueryHandle && NT_SUCCESS(PhGetProcessPriorityBoost(processItem->QueryHandle, &priorityBoostDisabled)))
                {
                    if (priorityBoostDisabled != object->Boost)
                    {
                        if (!NT_SUCCESS(PhSetProcessItemPriorityBoost(processItem, object->Boost)))
                        {
                            extension->SkipBoostPriority = TRUE;
                        }
                    }
                }
            }
        }

        listEntry = listEntry->Flink;
    }

    UnlockDb();
    PhReleaseQueuedLockExclusive(&ProcessListLock);
}

VOID SearchChangedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_STRING searchText = Parameter;

    if (PhIsNullOrEmptyString(searchText))
    {
        // ToolStatus expanded all nodes for searching, but the search text just became empty. We
        // should re-collapse processes.

        PPH_LIST nodes = PH_AUTO(PhDuplicateProcessNodeList());
        ULONG i;
        BOOLEAN changed = FALSE;

        LockDb();

        for (i = 0; i < nodes->Count; i++)
        {
            PPH_PROCESS_NODE node = nodes->Items[i];
            PDB_OBJECT object;

            if ((object = FindDbObjectForProcess(node->ProcessItem, INTENT_PROCESS_COLLAPSE)) && object->Collapse)
            {
                node->Node.Expanded = FALSE;
                changed = TRUE;
            }
        }

        UnlockDb();

        if (changed)
            TreeNew_NodesStructured(ProcessTreeNewHandle);
    }
}

VOID ProcessItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_PROCESS_ITEM processItem = Object;
    PPROCESS_EXTENSION extension = Extension;

    memset(extension, 0, sizeof(PROCESS_EXTENSION));
    extension->ProcessItem = processItem;

    PhAcquireQueuedLockExclusive(&ProcessListLock);
    InsertTailList(&ProcessListHead, &extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ProcessListLock);
}

VOID ProcessItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_PROCESS_ITEM processItem = Object;
    PPROCESS_EXTENSION extension = Extension;

    PhClearReference(&extension->Comment);
    PhAcquireQueuedLockExclusive(&ProcessListLock);
    RemoveEntryList(&extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ProcessListLock);
}

VOID ProcessNodeCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_PROCESS_NODE processNode = Object;
    PDB_OBJECT object;

    LockDb();

    if ((object = FindDbObjectForProcess(processNode->ProcessItem, INTENT_PROCESS_COLLAPSE)) && object->Collapse)
    {
        BOOLEAN visible = TRUE;

        for (PPH_PROCESS_NODE parent = processNode->Parent; parent; parent = parent->Parent)
        {
            if (!parent->Node.Visible)
            {
                // Don't change the Expanded state when the node is already hidden by
                // main window -> View -> Hide Processes from other users menu (dmex)
                visible = FALSE;
                break;
            }
        }

        if (visible)
        {
            processNode->Node.Expanded = FALSE;
        }
    }

    UnlockDb();
}

VOID ServiceItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_SERVICE_ITEM serviceItem = Object;
    PSERVICE_EXTENSION extension = Extension;

    memset(extension, 0, sizeof(SERVICE_EXTENSION));
    PhAcquireQueuedLockExclusive(&ServiceListLock);
    InsertTailList(&ServiceListHead, &extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ServiceListLock);
}

VOID ServiceItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_SERVICE_ITEM serviceItem = Object;
    PSERVICE_EXTENSION extension = Extension;

    PhClearReference(&extension->Comment);
    PhAcquireQueuedLockExclusive(&ServiceListLock);
    RemoveEntryList(&extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ServiceListLock);
}

LOGICAL DllMain(
    _In_ HINSTANCE Instance,
    _In_ ULONG Reason,
    _Reserved_ PVOID Reserved
    )
{
    if (Reason == DLL_PROCESS_ATTACH)
    {
        PPH_PLUGIN_INFORMATION info;
        PH_SETTING_CREATE settings[] =
        {
            { StringSettingType, SETTING_NAME_CUSTOM_COLOR_LIST, L"" },
            { StringSettingType, SETTING_NAME_OPTIONS_DB_COLUMNS, L"" },
        };

        PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

        if (!PluginInstance)
            return FALSE;

        info->DisplayName = L"User Notes";
        info->Author = L"dmex, wj32";
        info->Description = L"Allows the user to add comments for processes and services, save process priority and affinity, highlight individual processes and show processes collapsed by default.";

        PhRegisterCallback(
            PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
            LoadCallback,
            NULL,
            &PluginLoadCallbackRegistration
            );
        //PhRegisterCallback(
        //    PhGetPluginCallback(PluginInstance, PluginCallbackUnload),
        //    UnloadCallback,
        //    NULL,
        //    &PluginUnloadCallbackRegistration
        //    );
        PhRegisterCallback(
            PhGetGeneralCallback(GeneralCallbackMainMenuInitializing),
            MainMenuInitializingCallback,
            NULL,
            &MainMenuInitializingCallbackRegistration
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
            PhGetPluginCallback(PluginInstance, PluginCallbackMenuHook),
            MenuHookCallback,
            NULL,
            &PluginMenuHookCallbackRegistration
            );
        PhRegisterCallback(
            PhGetPluginCallback(PluginInstance, PluginCallbackTreeNewMessage),
            TreeNewMessageCallback,
            NULL,
            &TreeNewMessageCallbackRegistration
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
            PhGetGeneralCallback(GeneralCallbackServicePropertiesInitializing),
            ServicePropertiesInitializingCallback,
            NULL,
            &ServicePropertiesInitializingCallbackRegistration
            );
        PhRegisterCallback(
            PhGetGeneralCallback(GeneralCallbackProcessMenuInitializing),
            ProcessMenuInitializingCallback,
            NULL,
            &ProcessMenuInitializingCallbackRegistration
            );
        PhRegisterCallback(
            PhGetGeneralCallback(GeneralCallbackProcessTreeNewInitializing),
            ProcessTreeNewInitializingCallback,
            NULL,
            &ProcessTreeNewInitializingCallbackRegistration
            );
        PhRegisterCallback(
            PhGetGeneralCallback(GeneralCallbackGetProcessHighlightingColor),
            GetProcessHighlightingColorCallback,
            NULL,
            &GetProcessHighlightingColorCallbackRegistration
            );
        PhRegisterCallback(
            PhGetGeneralCallback(GeneralCallbackServiceTreeNewInitializing),
            ServiceTreeNewInitializingCallback,
            NULL,
            &ServiceTreeNewInitializingCallbackRegistration
            );
        PhRegisterCallback(
            PhGetGeneralCallback(GeneralCallbackMiListSectionMenuInitializing),
            MiListSectionMenuInitializingCallback,
            NULL,
            &MiListSectionMenuInitializingCallbackRegistration
            );
        PhRegisterCallback(
            PhGetGeneralCallback(GeneralCallbackProcessProviderModifiedEvent),
            ProcessModifiedCallback,
            NULL,
            &ProcessModifiedCallbackRegistration
            );
        PhRegisterCallback(
            PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
            ProcessesUpdatedCallback,
            NULL,
            &ProcessesUpdatedCallbackRegistration
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
            EmProcessNodeType,
            sizeof(PROCESS_EXTENSION),
            ProcessNodeCreateCallback,
            NULL
            );
        PhPluginSetObjectExtension(
            PluginInstance,
            EmServiceItemType,
            sizeof(SERVICE_EXTENSION),
            ServiceItemCreateCallback,
            ServiceItemDeleteCallback
            );

        PhAddSettings(settings, RTL_NUMBER_OF(settings));
    }

    return TRUE;
}
