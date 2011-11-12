/*
 * Process Hacker -
 *   memory list information
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

#include <phapp.h>
#include <phsvccl.h>

#define MSG_UPDATE (WM_APP + 1)

INT_PTR CALLBACK PhpMemoryListsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

HWND PhMemoryListsWindowHandle = NULL;
static VOID (NTAPI *UnregisterDialogFunction)(HWND);
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

VOID PhShowMemoryListsDialog(
    __in HWND ParentWindowHandle,
    __in_opt VOID (NTAPI *RegisterDialog)(HWND),
    __in_opt VOID (NTAPI *UnregisterDialog)(HWND)
    )
{
    if (!PhMemoryListsWindowHandle)
    {
        PhMemoryListsWindowHandle = CreateDialog(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_MEMLISTS),
            ParentWindowHandle,
            PhpMemoryListsDlgProc
            );
        RegisterDialog(PhMemoryListsWindowHandle);
        UnregisterDialogFunction = UnregisterDialog;
    }

    if (!IsWindowVisible(PhMemoryListsWindowHandle))
        ShowWindow(PhMemoryListsWindowHandle, SW_SHOW);
    else if (IsIconic(PhMemoryListsWindowHandle))
        ShowWindow(PhMemoryListsWindowHandle, SW_RESTORE);
    else
        SetForegroundWindow(PhMemoryListsWindowHandle);
}

static VOID NTAPI ProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PostMessage(PhMemoryListsWindowHandle, MSG_UPDATE, 0, 0);
}

static VOID PhpUpdateMemoryListInfo(
    __in HWND hwndDlg
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

        SetDlgItemText(hwndDlg, IDC_ZLISTZEROED_V, PhaFormatSize((ULONG64)memoryListInfo.ZeroPageCount * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTFREE_V, PhaFormatSize((ULONG64)memoryListInfo.FreePageCount * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTMODIFIED_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedPageCount * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTMODIFIEDNOWRITE_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedNoWritePageCount * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTBAD_V, PhaFormatSize((ULONG64)memoryListInfo.BadPageCount * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY_V, PhaFormatSize((ULONG64)standbyPageCount * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY0_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[0] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY1_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[1] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY2_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[2] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY3_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[3] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY4_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[4] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY5_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[5] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY6_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[6] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY7_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[7] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED_V, PhaFormatSize((ULONG64)repurposedPageCount * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED0_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[0] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED1_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[1] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED2_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[2] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED3_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[3] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED4_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[4] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED5_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[5] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED6_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[6] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED7_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[7] * PAGE_SIZE, -1)->Buffer);

        if (WindowsVersion >= WINDOWS_8)
            SetDlgItemText(hwndDlg, IDC_ZLISTMODIFIEDPAGEFILE_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedPageCountPageFile * PAGE_SIZE, -1)->Buffer);
        else
            SetDlgItemText(hwndDlg, IDC_ZLISTMODIFIEDPAGEFILE_V, L"N/A");
    }
    else
    {
        SetDlgItemText(hwndDlg, IDC_ZLISTZEROED_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTFREE_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTMODIFIED_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTMODIFIEDNOWRITE_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTMODIFIEDPAGEFILE_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTBAD_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY0_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY1_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY2_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY3_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY4_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY5_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY6_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTSTANDBY7_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED0_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED1_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED2_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED3_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED4_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED5_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED6_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZLISTREPURPOSED7_V, L"Unknown");
    }
}

INT_PTR CALLBACK PhpMemoryListsDlgProc(
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
            PhRegisterCallback(&PhProcessesUpdatedEvent, ProcessesUpdatedCallback, NULL, &ProcessesUpdatedRegistration);
            PhpUpdateMemoryListInfo(hwndDlg);

            PhLoadWindowPlacementFromSetting(L"MemoryListsWindowPosition", NULL, hwndDlg);
            PhRegisterDialog(hwndDlg);
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterDialog(hwndDlg);
            PhSaveWindowPlacementToSetting(L"MemoryListsWindowPosition", NULL, hwndDlg);

            PhUnregisterCallback(&PhProcessesUpdatedEvent, &ProcessesUpdatedRegistration);

            UnregisterDialogFunction(hwndDlg);
            PhMemoryListsWindowHandle = NULL;
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_EMPTY:
                {
                    HMENU menu;
                    HMENU subMenu;
                    RECT buttonRect;
                    POINT point;
                    UINT selectedItem;
                    SYSTEM_MEMORY_LIST_COMMAND command = -1;

                    menu = LoadMenu(PhInstanceHandle, MAKEINTRESOURCE(IDR_EMPTYMEMLISTS));
                    subMenu = GetSubMenu(menu, 0);

                    GetClientRect(GetDlgItem(hwndDlg, IDC_EMPTY), &buttonRect);
                    point.x = 0;
                    point.y = buttonRect.bottom;

                    ClientToScreen(GetDlgItem(hwndDlg, IDC_EMPTY), &point);
                    selectedItem = PhShowContextMenu2(
                        hwndDlg,
                        GetDlgItem(hwndDlg, IDC_EMPTY),
                        subMenu,
                        point
                        );

                    switch (selectedItem)
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
                    }

                    if (command != -1)
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
                            if (!PhElevated)
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
                                    // User cancelled eleavtion.
                                    status = STATUS_SUCCESS;
                                }
                            }
                        }

                        if (!NT_SUCCESS(status))
                        {
                            PhShowStatus(hwndDlg, L"Unable to execute the memory list command", status, 0);
                        }
                    }

                    DestroyMenu(menu);
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
    }

    return FALSE;
}
