/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2017-2024
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <emenu.h>
#include <settings.h>
#include <actions.h>
#include <phsvccl.h>
#include <kphuser.h>
#include <ksisup.h>

#define MSG_UPDATE (WM_APP + 1)

INT_PTR CALLBACK PhpMemoryListsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

HWND PhMemoryListsWindowHandle = NULL;
static VOID (NTAPI *UnregisterDialogFunction)(HWND);
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

VOID PhShowMemoryListsDialog(
    _In_ HWND ParentWindowHandle,
    _In_opt_ VOID (NTAPI *RegisterDialog)(HWND),
    _In_opt_ VOID (NTAPI *UnregisterDialog)(HWND)
    )
{
    if (!PhMemoryListsWindowHandle)
    {
        PhMemoryListsWindowHandle = PhCreateDialog(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_MEMLISTS),
            ParentWindowHandle,
            PhpMemoryListsDlgProc,
            NULL
            );

        if (RegisterDialog)
            RegisterDialog(PhMemoryListsWindowHandle);
        UnregisterDialogFunction = UnregisterDialog;
    }

    if (!IsWindowVisible(PhMemoryListsWindowHandle))
        ShowWindow(PhMemoryListsWindowHandle, SW_SHOW);
    else if (IsMinimized(PhMemoryListsWindowHandle))
        ShowWindow(PhMemoryListsWindowHandle, SW_RESTORE);
    else
        SetForegroundWindow(PhMemoryListsWindowHandle);
}

static VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (PhMemoryListsWindowHandle)
    {
        PostMessage(PhMemoryListsWindowHandle, MSG_UPDATE, 0, 0);
    }
}

static VOID PhpUpdateMemoryListInfo(
    _In_ HWND hwndDlg
    )
{
    SYSTEM_MEMORY_LIST_INFORMATION memoryListInfo;

    if (NT_SUCCESS(NtQuerySystemInformation(
        SystemMemoryListInformation,
        &memoryListInfo,
        sizeof(SYSTEM_MEMORY_LIST_INFORMATION),
        NULL
        )))
    {
        ULONG_PTR standbyPageCount;
        ULONG_PTR repurposedPageCount;
        ULONG i;

        standbyPageCount = 0;
        repurposedPageCount = 0;

        for (i = 0; i < 8; i++)
        {
            standbyPageCount += memoryListInfo.PageCountByPriority[i];
            repurposedPageCount += memoryListInfo.RepurposedPagesByPriority[i];
        }

        PhSetDialogItemText(hwndDlg, IDC_ZLISTZEROED_V, PhaFormatSize((ULONG64)memoryListInfo.ZeroPageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTFREE_V, PhaFormatSize((ULONG64)memoryListInfo.FreePageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTMODIFIED_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedPageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTMODIFIEDNOWRITE_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedNoWritePageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTBAD_V, PhaFormatSize((ULONG64)memoryListInfo.BadPageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY_V, PhaFormatSize((ULONG64)standbyPageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY0_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[0] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY1_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[1] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY2_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[2] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY3_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[3] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY4_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[4] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY5_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[5] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY6_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[6] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY7_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[7] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED_V, PhaFormatSize((ULONG64)repurposedPageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED0_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[0] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED1_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[1] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED2_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[2] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED3_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[3] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED4_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[4] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED5_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[5] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED6_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[6] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED7_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[7] * PAGE_SIZE, ULONG_MAX)->Buffer);

        if (WindowsVersion >= WINDOWS_8)
            PhSetDialogItemText(hwndDlg, IDC_ZLISTMODIFIEDPAGEFILE_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedPageCountPageFile * PAGE_SIZE, ULONG_MAX)->Buffer);
        else
            PhSetDialogItemText(hwndDlg, IDC_ZLISTMODIFIEDPAGEFILE_V, L"N/A");
    }
    else
    {
        PhSetDialogItemText(hwndDlg, IDC_ZLISTZEROED_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTFREE_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTMODIFIED_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTMODIFIEDNOWRITE_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTMODIFIEDPAGEFILE_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTBAD_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY0_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY1_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY2_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY3_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY4_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY5_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY6_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTSTANDBY7_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED0_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED1_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED2_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED3_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED4_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED5_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED6_V, L"Unknown");
        PhSetDialogItemText(hwndDlg, IDC_ZLISTREPURPOSED7_V, L"Unknown");
    }
}

typedef
_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID
NTAPI
PH_MEMORY_LIST_COMMAND_CALLBACK(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message,
    _Out_opt_ PBOOLEAN ShowError
    );
typedef PH_MEMORY_LIST_COMMAND_CALLBACK *PPH_MEMORY_LIST_COMMAND_CALLBACK;

PPH_STRING PhpCreateCommandStatusString(
    _In_ PWSTR Message,
    _In_ NTSTATUS Status
    )
{
    PPH_STRING string;
    PPH_STRING statusString;

    if (statusString = PhGetStatusMessage(Status, 0))
    {
        PH_FORMAT format[3];

        PhInitFormatS(&format[0], Message);
        PhInitFormatC(&format[1], L' ');
        PhInitFormatSR(&format[2], statusString->sr);

        string = PhFormat(format, RTL_NUMBER_OF(format), 10);

        PhDereferenceObject(statusString);
    }
    else
    {
        string = PhCreateString(Message);
    }

    return string;
}

NTSTATUS PhpMemoryListCommandCommon(
    _In_ HWND ParentWindow,
    _In_ SYSTEM_MEMORY_LIST_COMMAND Command
    )
{
    NTSTATUS status;

    PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
    status = NtSetSystemInformation(
        SystemMemoryListInformation,
        &Command,
        sizeof(SYSTEM_MEMORY_LIST_COMMAND)
        );
    PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

    if (status == STATUS_PRIVILEGE_NOT_HELD)
    {
        if (!PhGetOwnTokenAttributes().Elevated)
        {
            if (PhUiConnectToPhSvc(ParentWindow, FALSE))
            {
                PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
                status = PhSvcCallIssueMemoryListCommand(Command);
                PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));
                PhUiDisconnectFromPhSvc();
            }
            else
            {
                // User cancelled elevation.
                status = STATUS_SUCCESS;
            }
        }
    }

    return status;
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpEmptyWorkingSetsCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message,
    _Out_opt_ PBOOLEAN ShowError
    )
{
    NTSTATUS status;

    status = PhpMemoryListCommandCommon(ParentWindow, MemoryEmptyWorkingSets);

    if (NT_SUCCESS(status))
        *Message = PhCreateString(L"Working sets emptied.");
    else
        *Message = PhpCreateCommandStatusString(L"Unable to empty working sets.", status);

    if (ShowError)
        *ShowError = !NT_SUCCESS(status);
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpFlushModifiedListCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message,
    _Out_opt_ PBOOLEAN ShowError
    )
{
    NTSTATUS status;

    status = PhpMemoryListCommandCommon(ParentWindow, MemoryFlushModifiedList);

    if (NT_SUCCESS(status))
        *Message = PhCreateString(L"Modified page lists emptied.");
    else
        *Message = PhpCreateCommandStatusString(L"Unable to empty modified page lists.", status);

    if (ShowError)
        *ShowError = !NT_SUCCESS(status);
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpPurgeStandbyListCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message,
    _Out_opt_ PBOOLEAN ShowError
    )
{
    NTSTATUS status;

    status = PhpMemoryListCommandCommon(ParentWindow, MemoryPurgeStandbyList);

    if (NT_SUCCESS(status))
        *Message = PhCreateString(L"Standby lists emptied.");
    else
        *Message = PhpCreateCommandStatusString(L"Unable to empty standby lists.", status);

    if (ShowError)
        *ShowError = !NT_SUCCESS(status);
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpPurgeLowPriorityStandbyListCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message,
    _Out_opt_ PBOOLEAN ShowError
    )
{
    NTSTATUS status;

    status = PhpMemoryListCommandCommon(ParentWindow, MemoryPurgeLowPriorityStandbyList);

    if (NT_SUCCESS(status))
        *Message = PhCreateString(L"Priority 0 standby list emptied.");
    else
        *Message = PhpCreateCommandStatusString(L"Unable to empty priority 0 standby list.", status);

    if (ShowError)
        *ShowError = !NT_SUCCESS(status);
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpCombinMemoryListsCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message,
    _Out_opt_ PBOOLEAN ShowError
    )
{
    NTSTATUS status;
    MEMORY_COMBINE_INFORMATION_EX combineInfo = { 0 };

    PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
    status = NtSetSystemInformation(
        SystemCombinePhysicalMemoryInformation,
        &combineInfo,
        sizeof(MEMORY_COMBINE_INFORMATION_EX)
        );
    PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

    if (NT_SUCCESS(status))
    {
        PH_FORMAT format[5];

        // Memory pages combined: %s (%llu pages)
        PhInitFormatS(&format[0], L"Memory pages combined: ");
        PhInitFormatSize(&format[1], combineInfo.PagesCombined * PAGE_SIZE);
        PhInitFormatS(&format[2], L" (");
        PhInitFormatI64U(&format[3], combineInfo.PagesCombined);
        PhInitFormatS(&format[4], L" pages)");

        *Message = PhFormat(format, RTL_NUMBER_OF(format), 0);
    }
    else
    {
        *Message = PhpCreateCommandStatusString(L"Unable to combine memory pages.", status);
    }

    if (ShowError)
        *ShowError = !NT_SUCCESS(status);
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpEmptyCompressionStoreCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message,
    _Out_opt_ PBOOLEAN ShowError
    )
{
    NTSTATUS status;

    if (KsiLevel() < KphLevelMax)
    {
        // hack(jxy-s): show error presence we're being invoked directly instead of "empty all"
        // if show error is passed, show that the driver is not connected
        if (ShowError)
        {
            PhShowKsiNotConnected(
                ParentWindow,
                L"Emptying the compression store requires a connection to the kernel driver."
                );
            *ShowError = FALSE;
        }

        *Message = NULL;
        return;
    }

    PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
    status = KphSystemControl(
        KphSystemControlEmptyCompressionStore,
        NULL,
        0
        );
    PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

    if (NT_SUCCESS(status))
        *Message = PhCreateString(L"Compression stores emptied.");
    else
        *Message = PhpCreateCommandStatusString(L"Unable to empty compression stores.", status);

    if (ShowError)
        *ShowError = !NT_SUCCESS(status);
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpEmptyRegistryCacheCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message,
    _Out_opt_ PBOOLEAN ShowError
    )
{
    NTSTATUS status;

    PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
    status = NtSetSystemInformation(SystemRegistryReconciliationInformation, NULL, 0);
    PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

    if (NT_SUCCESS(status))
        *Message = PhCreateString(L"Registry cache emptied.");
    else
        *Message = PhpCreateCommandStatusString(L"Unable to empty registry cache.", status);

    if (ShowError)
        *ShowError = !NT_SUCCESS(status);
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpEmptySystemFileCacheCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message,
    _Out_opt_ PBOOLEAN ShowError
    )
{
    NTSTATUS status;
    SYSTEM_FILECACHE_INFORMATION cacheInfo = { 0 };

    PhAdjustPrivilege(NULL, SE_INCREASE_QUOTA_PRIVILEGE, TRUE);

    PhGetSystemFileCacheSize(&cacheInfo);

    PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
    status = PhSetSystemFileCacheSize(
        MAXSIZE_T,
        MAXSIZE_T,
        0
        );
    PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

    if (NT_SUCCESS(status))
    {
        PH_FORMAT format[5];

        // System file cache emptied: %s (%llu pages)
        PhInitFormatS(&format[0], L"System file cache emptied: ");
        PhInitFormatSize(&format[1], cacheInfo.CurrentSize);
        PhInitFormatS(&format[2], L" (");
        PhInitFormatI64U(&format[3], cacheInfo.CurrentSize / PAGE_SIZE);
        PhInitFormatS(&format[4], L" pages)");

        *Message = PhFormat(format, RTL_NUMBER_OF(format), 0);
    }
    else
    {
        *Message = PhpCreateCommandStatusString(L"Unable to empty system file cache.", status);
    }

    if (ShowError)
        *ShowError = !NT_SUCCESS(status);
}

_Function_class_(PH_MEMORY_LIST_COMMAND_CALLBACK)
VOID NTAPI PhpFlushModifiedFileCacheCommand(
    _In_ HWND ParentWindow,
    _Out_ PPH_STRING* Message,
    _Out_opt_ PBOOLEAN ShowError
    )
{
    NTSTATUS status;

    PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
    status = PhFlushVolumeCache();
    PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

    if (NT_SUCCESS(status))
        *Message = PhCreateString(L"Volume file cache flushed to disk.");
    else
        *Message = PhpCreateCommandStatusString(L"Unable to flush volume file cache.", status);

    if (ShowError)
        *ShowError = !NT_SUCCESS(status);
}

VOID PhShowMemoryListCommand(
    _In_ HWND ParentWindow,
    _In_ HWND ButtonWindow,
    _In_ BOOLEAN ShowTopAlign
    )
{
    static PPH_MEMORY_LIST_COMMAND_CALLBACK commands[] =
    {
        PhpCombinMemoryListsCommand,
        PhpEmptyCompressionStoreCommand,
        PhpEmptySystemFileCacheCommand,
        PhpEmptyRegistryCacheCommand,
        PhpEmptyWorkingSetsCommand,
        PhpFlushModifiedListCommand,
        PhpFlushModifiedFileCacheCommand,
        PhpPurgeStandbyListCommand,
        PhpPurgeLowPriorityStandbyListCommand,
    };
    PPH_EMENU menu;
    RECT buttonRect;
    PPH_EMENU_ITEM selectedItem;

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 0, L"&Combine memory pages", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Empty &compression cache", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2, L"Empty system &file cache", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 3, L"Empty &registry cache", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 4, L"Empty &working sets", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 5, L"Empty &modified page list", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 6, L"Empty &modified file cache", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 7, L"Empty &standby list", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 8, L"Empty &priority 0 standby list", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, RTL_NUMBER_OF(commands), L"Empty &all", NULL, NULL), ULONG_MAX);

    if (ShowTopAlign)
    {
        GetWindowRect(ButtonWindow, &buttonRect);
        selectedItem = PhShowEMenu(
            menu,
            ParentWindow,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_BOTTOM,
            buttonRect.left,
            buttonRect.top
            );
    }
    else
    {
        POINT point;

        GetClientRect(ButtonWindow, &buttonRect);
        point.x = 0;
        point.y = buttonRect.bottom;
        ClientToScreen(ButtonWindow, &point);

        selectedItem = PhShowEMenu(
            menu,
            ParentWindow,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            point.x,
            point.y
            );
    }

    if (selectedItem)
    {
        if (selectedItem->Id < RTL_NUMBER_OF(commands))
        {
            PPH_STRING message = NULL;
            BOOLEAN showError = TRUE;

            commands[selectedItem->Id](ParentWindow, &message, &showError);

            if (!PhIsNullOrEmptyString(message))
            {
                if (showError)
                    PhShowError2(ParentWindow, L"Memory Command", PhGetString(message));
                else
                    PhShowInformation2(ParentWindow, L"Memory Command", PhGetString(message));
            }

            PhClearReference(&message);
        }
        else if (!PhGetOwnTokenAttributes().Elevated)
        {
            PhShowError2(ParentWindow, L"Memory Command", L"System Informer must run elevated to empty all.");
        }
        else
        {
            PH_STRING_BUILDER stringBuilder;
            PPH_STRING message;

            PhInitializeStringBuilder(&stringBuilder, 100);

            for (ULONG i = 0; i < RTL_NUMBER_OF(commands); i++)
            {
                message = NULL;

                commands[i](ParentWindow, &message, NULL);

                if (!PhIsNullOrEmptyString(message))
                {
                    PhAppendStringBuilder(&stringBuilder, &message->sr);
                    PhAppendStringBuilder2(&stringBuilder, L"\r\n");
                    PhClearReference(&message);
                }
            }

            if (stringBuilder.String->Length > 2)
                PhRemoveEndStringBuilder(&stringBuilder, 2);

            message = PhFinalStringBuilderString(&stringBuilder);
            if (!PhIsNullOrEmptyString(message))
                PhShowInformation2(ParentWindow, L"Memory Command", PhGetString(message));
            PhClearReference(&message);
        }
    }

    PhDestroyEMenu(menu);
}

INT_PTR CALLBACK PhpMemoryListsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhAdjustPrivilege(NULL, SE_PROF_SINGLE_PROCESS_PRIVILEGE, TRUE);

            PhRegisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), ProcessesUpdatedCallback, NULL, &ProcessesUpdatedRegistration);
            PhpUpdateMemoryListInfo(hwndDlg);

            PhLoadWindowPlacementFromSetting(L"MemoryListsWindowPosition", NULL, hwndDlg);
            PhRegisterDialog(hwndDlg);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterDialog(hwndDlg);
            PhSaveWindowPlacementToSetting(L"MemoryListsWindowPosition", NULL, hwndDlg);

            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), &ProcessesUpdatedRegistration);

            if (UnregisterDialogFunction)
                UnregisterDialogFunction(hwndDlg);
            PhMemoryListsWindowHandle = NULL;
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_EMPTY:
                {
                    PhShowMemoryListCommand(hwndDlg, GET_WM_COMMAND_HWND(wParam, lParam), FALSE);
                }
                break;
            }
        }
        break;
    case MSG_UPDATE:
        {
            PhpUpdateMemoryListInfo(hwndDlg);
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}
