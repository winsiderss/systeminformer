/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2017-2023
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
    PostMessage(PhMemoryListsWindowHandle, MSG_UPDATE, 0, 0);
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

VOID PhShowMemoryListCommand(
    _In_ HWND ParentWindow,
    _In_ HWND ButtonWindow,
    _In_ BOOLEAN ShowTopAlign
    )
{
    SYSTEM_MEMORY_LIST_COMMAND command = ULONG_MAX;
    PPH_EMENU menu;
    RECT buttonRect;
    PPH_EMENU_ITEM selectedItem;

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_EMPTY_COMBINEMEMORYLISTS, L"&Combine memory pages", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_EMPTY_COMPRESSIONSTORE, L"Empty &compression cache", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_EMPTY_SYSTEMFILECACHE, L"Empty system &file cache", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_EMPTY_EMPTYREGISTRYCACHE, L"Empty &registry cache", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_EMPTY_EMPTYWORKINGSETS, L"Empty &working sets", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_EMPTY_EMPTYMODIFIEDPAGELIST, L"Empty &modified page list", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_EMPTY_EMPTYSTANDBYLIST, L"Empty &standby list", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_EMPTY_EMPTYPRIORITY0STANDBYLIST, L"Empty &priority 0 standby list", NULL, NULL), ULONG_MAX);

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
        switch (selectedItem->Id)
        {
        case ID_EMPTY_EMPTYWORKINGSETS:
            command = MemoryEmptyWorkingSets;
            break;
        case ID_EMPTY_EMPTYMODIFIEDPAGELIST:
            command = MemoryFlushModifiedList;
            break;
        case ID_EMPTY_EMPTYSTANDBYLIST:
            command = MemoryPurgeStandbyList;
            break;
        case ID_EMPTY_EMPTYPRIORITY0STANDBYLIST:
            command = MemoryPurgeLowPriorityStandbyList;
            break;
        case ID_EMPTY_COMBINEMEMORYLISTS:
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
                    PhShowInformation2(
                        ParentWindow,
                        L"Memory pages combined",
                        L"%s (%llu pages)",
                        PhaFormatSize(combineInfo.PagesCombined * PAGE_SIZE, ULONG_MAX)->Buffer,
                        combineInfo.PagesCombined
                        );
                }
                else
                {
                    PhShowStatus(ParentWindow, L"Unable to combine memory pages", status, 0);
                }
            }
            break;
        case ID_EMPTY_COMPRESSIONSTORE:
            {
                NTSTATUS status;

                if (KphLevel() < KphLevelMax)
                {
                    PhShowKsiNotConnected(
                        ParentWindow,
                        L"Emptying the compression store requires a connection to the kernel driver."
                        );
                    break;
                }

                PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
                status = KphSystemControl(
                    KphSystemControlEmptyCompressionStore,
                    NULL,
                    0
                    );
                PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

                if (!NT_SUCCESS(status))
                {
                    PhShowStatus(ParentWindow, L"Unable to empty compression stores", status, 0);
                }
            }
            break;
        case ID_EMPTY_EMPTYREGISTRYCACHE:
            {
                NTSTATUS status;

                PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
                status = NtSetSystemInformation(SystemRegistryReconciliationInformation, NULL, 0);
                PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

                if (NT_SUCCESS(status))
                {
                    PhShowInformation2(ParentWindow, L"Registry cache flushed", L"", L"");
                }
                else
                {
                    PhShowStatus(ParentWindow, L"Unable to flush registry cache.", status, 0);
                }
            }
            break;
        case ID_EMPTY_SYSTEMFILECACHE:
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
                    PhShowInformation2(
                        ParentWindow,
                        L"System file cache flushed",
                        L"%s (%llu pages)",
                        PhaFormatSize(cacheInfo.CurrentSize, ULONG_MAX)->Buffer,
                        cacheInfo.CurrentSize / PAGE_SIZE
                        );
                }
                else
                {
                    PhShowStatus(ParentWindow, L"Unable to empty system file cache.", status, 0);
                }
            }
            break;
        }
    }

    if (command != ULONG_MAX)
    {
        NTSTATUS status;

        PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));
        status = NtSetSystemInformation(
            SystemMemoryListInformation,
            &command,
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
                    status = PhSvcCallIssueMemoryListCommand(command);
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

        if (!NT_SUCCESS(status))
        {
            PhShowStatus(ParentWindow, L"Unable to execute the memory list command", status, 0);
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
