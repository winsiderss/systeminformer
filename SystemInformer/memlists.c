/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2017-2022
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <phsettings.h>
#include <emenu.h>
#include <settings.h>
#include <actions.h>
#include <phsvccl.h>
#include <kphuser.h>

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
            HANDLE tokenHandle;

            if (NT_SUCCESS(PhOpenProcessToken(NtCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &tokenHandle)))
            {
                PhSetTokenPrivilege2(tokenHandle, SE_PROF_SINGLE_PROCESS_PRIVILEGE, SE_PRIVILEGE_ENABLED);
                NtClose(tokenHandle);
            }

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
                    PPH_EMENU menu;
                    RECT buttonRect;
                    POINT point;
                    PPH_EMENU_ITEM selectedItem;
                    SYSTEM_MEMORY_LIST_COMMAND command = ULONG_MAX;

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_EMPTY_COMBINEMEMORYLISTS, L"&Combine memory lists", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_EMPTY_EMPTYWORKINGSETS, L"Empty &working sets", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_EMPTY_COMPRESSIONSTORE, L"Empty &compression working sets", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_EMPTY_EMPTYMODIFIEDPAGELIST, L"Empty &modified page list", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_EMPTY_EMPTYSTANDBYLIST, L"Empty &standby list", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_EMPTY_EMPTYPRIORITY0STANDBYLIST, L"Empty &priority 0 standby list", NULL, NULL), ULONG_MAX);

                    GetClientRect(GetDlgItem(hwndDlg, IDC_EMPTY), &buttonRect);
                    point.x = 0;
                    point.y = buttonRect.bottom;

                    ClientToScreen(GetDlgItem(hwndDlg, IDC_EMPTY), &point);
                    selectedItem = PhShowEMenu(menu, hwndDlg, PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP, point.x, point.y);

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

                                status = NtSetSystemInformation(
                                    SystemCombinePhysicalMemoryInformation,
                                    &combineInfo,
                                    sizeof(MEMORY_COMBINE_INFORMATION_EX)
                                    );

                                if (NT_SUCCESS(status))
                                {
                                    PhShowInformation2(
                                        hwndDlg,
                                        L"Memory pages combined",
                                        L"%s (%llu pages)",
                                        PhaFormatSize(combineInfo.PagesCombined * PAGE_SIZE, ULONG_MAX)->Buffer,
                                        combineInfo.PagesCombined
                                        );
                                }
                                else
                                {
                                    PhShowStatus(hwndDlg, L"Unable to combine memory pages", status, 0);
                                }
                            }
                            break;
                        case ID_EMPTY_COMPRESSIONSTORE:
                            {
                                NTSTATUS status;

                                if (KphLevel() < KphLevelMax)
                                {
                                    PhShowError2(hwndDlg, PH_KPH_ERROR_TITLE, L"%s", PH_KPH_ERROR_MESSAGE);
                                    break;
                                }

                                status = KphSystemControl(KphSystemControlEmptyCompressionStore,
                                                          NULL,
                                                          0);
                                if (!NT_SUCCESS(status))
                                {
                                    PhShowStatus(hwndDlg, L"Unable to empty compression stores", status, 0);
                                }
                            }
                            break;
                        }
                    }

                    if (command != ULONG_MAX)
                    {
                        NTSTATUS status;

                        SetCursor(LoadCursor(NULL, IDC_WAIT));
                        status = NtSetSystemInformation(
                            SystemMemoryListInformation,
                            &command,
                            sizeof(SYSTEM_MEMORY_LIST_COMMAND)
                            );
                        SetCursor(LoadCursor(NULL, IDC_ARROW));

                        if (status == STATUS_PRIVILEGE_NOT_HELD)
                        {
                            if (!PhGetOwnTokenAttributes().Elevated)
                            {
                                if (PhUiConnectToPhSvc(hwndDlg, FALSE))
                                {
                                    SetCursor(LoadCursor(NULL, IDC_WAIT));
                                    status = PhSvcCallIssueMemoryListCommand(command);
                                    SetCursor(LoadCursor(NULL, IDC_ARROW));
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
                            PhShowStatus(hwndDlg, L"Unable to execute the memory list command", status, 0);
                        }
                    }

                    PhDestroyEMenu(menu);
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
