#include <phdk.h>
#include "resource.h"
#include "sbiedll.h"

typedef struct _BOXED_PROCESS
{
    HANDLE ProcessId;
    WCHAR BoxName[34];
} BOXED_PROCESS, *PBOXED_PROCESS;

VOID NTAPI LoadCallback(
    __in PVOID Parameter,
    __in PVOID Context
    );

VOID NTAPI ShowOptionsCallback(
    __in PVOID Parameter,
    __in PVOID Context
    );

VOID NTAPI MenuItemCallback(
    __in PVOID Parameter,
    __in PVOID Context
    );

VOID NTAPI MainWindowShowingCallback(
    __in PVOID Parameter,
    __in PVOID Context
    );

VOID NTAPI ProcessesUpdatedCallback(
    __in PVOID Parameter,
    __in PVOID Context
    );

VOID NTAPI GetProcessHighlightingColorCallback(
    __in PVOID Parameter,
    __in PVOID Context
    );

VOID NTAPI GetProcessTooltipTextCallback(
    __in PVOID Parameter,
    __in PVOID Context
    );

VOID NTAPI RefreshBoxedPids(
    __in PVOID Context,
    __in BOOLEAN TimerOrWaitFired
    );

INT_PTR CALLBACK OptionsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
PH_CALLBACK_REGISTRATION GetProcessHighlightingColorCallbackRegistration;
PH_CALLBACK_REGISTRATION GetProcessTooltipTextCallbackRegistration;

P_SbieApi_EnumBoxes SbieApi_EnumBoxes;
P_SbieApi_EnumProcessEx SbieApi_EnumProcessEx;
P_SbieDll_KillAll SbieDll_KillAll;

PPH_HASHTABLE BoxedProcessesHashtable;
PH_QUEUED_LOCK BoxedProcessesLock = PH_QUEUED_LOCK_INIT;
BOOLEAN BoxedProcessesUpdated = FALSE;

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
            PH_PLUGIN_INFORMATION info;

            info.DisplayName = L"Sandboxie Support";
            info.Author = L"wj32";
            info.Description = L"Provides functionality for sandboxed processes.";
            info.HasOptions = TRUE;

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.SbieSupport", Instance, &info);
            
            if (!PluginInstance)
                return FALSE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
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
                PhGetGeneralCallback(GeneralCallbackGetProcessHighlightingColor),
                GetProcessHighlightingColorCallback,
                NULL,
                &GetProcessHighlightingColorCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackGetProcessTooltipText),
                GetProcessTooltipTextCallback,
                NULL,
                &GetProcessTooltipTextCallbackRegistration
                );

            {
                static PH_SETTING_CREATE settings[] =
                {
                    { StringSettingType, L"ProcessHacker.SbieSupport.SbieDllPath", L"C:\\Program Files\\Sandboxie\\SbieDll.dll" }
                };

                PhAddSettings(settings, sizeof(settings) / sizeof(PH_SETTING_CREATE));
            }
        }
        break;
    }

    return TRUE;
}

BOOLEAN NTAPI BoxedProcessesCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    return ((PBOXED_PROCESS)Entry1)->ProcessId == ((PBOXED_PROCESS)Entry2)->ProcessId;
}

ULONG NTAPI BoxedProcessesHashFunction(
    __in PVOID Entry
    )
{
    return (ULONG)((PBOXED_PROCESS)Entry)->ProcessId / 4;
}

VOID NTAPI LoadCallback(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_STRING sbieDllPath;
    HMODULE module;
    HANDLE timerQueueHandle;
    HANDLE timerHandle;

    BoxedProcessesHashtable = PhCreateHashtable(
        sizeof(BOXED_PROCESS),
        BoxedProcessesCompareFunction,
        BoxedProcessesHashFunction,
        32
        );

    sbieDllPath = PhGetStringSetting(L"ProcessHacker.SbieSupport.SbieDllPath");
    module = LoadLibrary(sbieDllPath->Buffer);
    PhDereferenceObject(sbieDllPath);

    SbieApi_EnumBoxes = (PVOID)GetProcAddress(module, "_SbieApi_EnumBoxes@8");
    SbieApi_EnumProcessEx = (PVOID)GetProcAddress(module, "_SbieApi_EnumProcessEx@16");
    SbieDll_KillAll = (PVOID)GetProcAddress(module, "_SbieDll_KillAll@8");

    if (NT_SUCCESS(RtlCreateTimerQueue(&timerQueueHandle)))
    {
        RtlCreateTimer(timerQueueHandle, &timerHandle, RefreshBoxedPids, NULL, 0, 5000, 0);
    }
}

VOID NTAPI ShowOptionsCallback(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    DialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        (HWND)Parameter,
        OptionsDlgProc
        );
}

VOID NTAPI MenuItemCallback(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;

    switch (menuItem->Id)
    {
    case 1:
        {
            if (PhShowConfirmMessage(
                PhMainWndHandle,
                L"terminate",
                L"all sandboxed processes",
                NULL,
                FALSE
                ))
            {
                PBOXED_PROCESS boxedProcess;
                ULONG enumerationKey = 0;

                // Make sure we have an update-to-date list.
                RefreshBoxedPids(NULL, FALSE);

                PhAcquireQueuedLockShared(&BoxedProcessesLock);

                while (PhEnumHashtable(BoxedProcessesHashtable, &boxedProcess, &enumerationKey))
                {
                    HANDLE processHandle;

                    if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_TERMINATE, boxedProcess->ProcessId)))
                    {
                        PhTerminateProcess(processHandle, STATUS_SUCCESS);
                        NtClose(processHandle);
                    }
                }

                PhReleaseQueuedLockShared(&BoxedProcessesLock);
            }
        }
        break;
    }
}

VOID NTAPI MainWindowShowingCallback(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_TOOLS, L"$",
        0, L"-", NULL);
    PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_TOOLS, L"$",
        1, L"Terminate Sandboxed Processes", NULL);
}

VOID NTAPI ProcessesUpdatedCallback(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PBOXED_PROCESS boxedProcess;
    ULONG enumerationKey = 0;

    if (BoxedProcessesUpdated)
    {
        // Invalidate the nodes of boxed processes (so they use the correct highlighting color).

        PhAcquireQueuedLockShared(&BoxedProcessesLock);

        if (BoxedProcessesUpdated)
        {
            while (PhEnumHashtable(BoxedProcessesHashtable, &boxedProcess, &enumerationKey))
            {
                PPH_PROCESS_NODE processNode;

                if (processNode = PhFindProcessNode(boxedProcess->ProcessId))
                    PhUpdateProcessNode(processNode);
            }

            BoxedProcessesUpdated = FALSE;
        }

        PhReleaseQueuedLockShared(&BoxedProcessesLock);
    }
}

VOID NTAPI GetProcessHighlightingColorCallback(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_PLUGIN_GET_HIGHLIGHTING_COLOR getHighlightingColor = Parameter;
    BOXED_PROCESS lookupBoxedProcess;
    PBOXED_PROCESS boxedProcess;

    PhAcquireQueuedLockShared(&BoxedProcessesLock);

    lookupBoxedProcess.ProcessId = ((PPH_PROCESS_ITEM)getHighlightingColor->Parameter)->ProcessId;

    if (boxedProcess = PhGetHashtableEntry(BoxedProcessesHashtable, &lookupBoxedProcess))
    {
        getHighlightingColor->BackColor = RGB(0x33, 0x33, 0x00);
        getHighlightingColor->Cache = TRUE;
        getHighlightingColor->Handled = TRUE;
    }

    PhReleaseQueuedLockShared(&BoxedProcessesLock);
}

VOID NTAPI GetProcessTooltipTextCallback(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_PLUGIN_GET_TOOLTIP_TEXT getTooltipText = Parameter;
    BOXED_PROCESS lookupBoxedProcess;
    PBOXED_PROCESS boxedProcess;

    PhAcquireQueuedLockShared(&BoxedProcessesLock);

    lookupBoxedProcess.ProcessId = ((PPH_PROCESS_ITEM)getTooltipText->Parameter)->ProcessId;

    if (boxedProcess = PhGetHashtableEntry(BoxedProcessesHashtable, &lookupBoxedProcess))
    {
        PhStringBuilderAppendFormat(getTooltipText->StringBuilder, L"Sandboxie:\n    Box name: %s\n", boxedProcess->BoxName);
    }

    PhReleaseQueuedLockShared(&BoxedProcessesLock);
}

VOID NTAPI RefreshBoxedPids(
    __in PVOID Context,
    __in BOOLEAN TimerOrWaitFired
    )
{
    LONG index;
    WCHAR boxName[34];
    ULONG pids[512];

    if (!SbieApi_EnumBoxes || !SbieApi_EnumProcessEx)
        return;

    PhAcquireQueuedLockExclusive(&BoxedProcessesLock);

    PhClearHashtable(BoxedProcessesHashtable);

    index = -1;

    while ((index = SbieApi_EnumBoxes(index, boxName)) != -1)
    {
        if (SbieApi_EnumProcessEx(boxName, TRUE, 0, pids) == 0)
        {
            ULONG count;
            PULONG pid;

            count = pids[0];
            pid = &pids[1];

            while (count != 0)
            {
                BOXED_PROCESS boxedProcess;

                boxedProcess.ProcessId = ULongToHandle(*pid);
                memcpy(boxedProcess.BoxName, boxName, sizeof(boxName));

                PhAddHashtableEntry(BoxedProcessesHashtable, &boxedProcess);

                count--;
                pid++;
            }
        }
    }

    BoxedProcessesUpdated = TRUE;

    PhReleaseQueuedLockExclusive(&BoxedProcessesLock);
}

INT_PTR CALLBACK OptionsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_STRING sbieDllPath;

            sbieDllPath = PhGetStringSetting(L"ProcessHacker.SbieSupport.SbieDllPath");
            SetDlgItemText(hwndDlg, IDC_SBIEDLLPATH, sbieDllPath->Buffer);
            PhDereferenceObject(sbieDllPath);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    PhSetStringSetting2(L"ProcessHacker.SbieSupport.SbieDllPath",
                        &PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_SBIEDLLPATH)->sr);

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            case IDC_BROWSE:
                {
                    static PH_FILETYPE_FILTER filters[] =
                    {
                        { L"SbieDll.dll", L"SbieDll.dll" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog;
                    PPH_STRING fileName;

                    fileDialog = PhCreateOpenFileDialog();
                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

                    fileName = PhGetFileName(PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_SBIEDLLPATH));
                    PhSetFileDialogFileName(fileDialog, fileName->Buffer);
                    PhDereferenceObject(fileName);

                    if (PhShowFileDialog(NULL, fileDialog))
                    {
                        fileName = PhGetFileDialogFileName(fileDialog);
                        SetDlgItemText(hwndDlg, IDC_SBIEDLLPATH, fileName->Buffer);
                        PhDereferenceObject(fileName);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
