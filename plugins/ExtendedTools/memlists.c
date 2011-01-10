/*
 * Process Hacker Extended Tools - 
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

#include "exttools.h"
#include "resource.h"

#define MSG_UPDATE (WM_APP + 1)

INT_PTR CALLBACK EtpMemoryListsDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

HWND EtpMemoryListsWindowHandle = NULL;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

VOID EtShowMemoryListsDialog()
{
    if (!EtpMemoryListsWindowHandle)
    {
        EtpMemoryListsWindowHandle = CreateDialog(
            PluginInstance->DllBase,
            MAKEINTRESOURCE(IDD_MEMLISTS),
            PhMainWndHandle,
            EtpMemoryListsDlgProc
            );
    }

    if (!IsWindowVisible(EtpMemoryListsWindowHandle))
        ShowWindow(EtpMemoryListsWindowHandle, SW_SHOW);
    else if (IsIconic(EtpMemoryListsWindowHandle))
        ShowWindow(EtpMemoryListsWindowHandle, SW_RESTORE);
    else
        SetForegroundWindow(EtpMemoryListsWindowHandle);
}

VOID NTAPI EtpProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PostMessage(EtpMemoryListsWindowHandle, MSG_UPDATE, 0, 0);
}

static VOID EtpUpdateMemoryListInfo(
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

        SetDlgItemText(hwndDlg, IDC_ZZEROED_V, PhaFormatSize((ULONG64)memoryListInfo.ZeroPageCount * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZFREE_V, PhaFormatSize((ULONG64)memoryListInfo.FreePageCount * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZMODIFIED_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedPageCount * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZMODIFIEDNOWRITE_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedNoWritePageCount * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZBAD_V, PhaFormatSize((ULONG64)memoryListInfo.BadPageCount * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY_V, PhaFormatSize((ULONG64)standbyPageCount * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY0_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[0] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY1_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[1] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY2_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[2] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY3_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[3] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY4_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[4] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY5_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[5] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY6_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[6] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY7_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[7] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED_V, PhaFormatSize((ULONG64)repurposedPageCount * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED0_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[0] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED1_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[1] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED2_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[2] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED3_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[3] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED4_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[4] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED5_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[5] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED6_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[6] * PAGE_SIZE, -1)->Buffer);
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED7_V, PhaFormatSize((ULONG64)memoryListInfo.RepurposedPagesByPriority[7] * PAGE_SIZE, -1)->Buffer);
    }
    else
    {
        SetDlgItemText(hwndDlg, IDC_ZZEROED_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZFREE_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZMODIFIED_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZMODIFIEDNOWRITE_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZBAD_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY0_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY1_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY2_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY3_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY4_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY5_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY6_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZSTANDBY7_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED0_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED1_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED2_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED3_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED4_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED5_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED6_V, L"Unknown");
        SetDlgItemText(hwndDlg, IDC_ZREPURPOSED7_V, L"Unknown");
    }
}

INT_PTR CALLBACK EtpMemoryListsDlgProc(      
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
            PhRegisterCallback(&PhProcessesUpdatedEvent, EtpProcessesUpdatedCallback, NULL, &ProcessesUpdatedRegistration);
            EtpUpdateMemoryListInfo(hwndDlg);

            PhLoadWindowPlacementFromSetting(SETTING_NAME_MEMORY_LISTS_WINDOW_POSITION, NULL, hwndDlg);
            PhRegisterDialog(hwndDlg);
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterDialog(hwndDlg);
            PhSaveWindowPlacementToSetting(SETTING_NAME_MEMORY_LISTS_WINDOW_POSITION, NULL, hwndDlg);

            PhUnregisterCallback(&PhProcessesUpdatedEvent, &ProcessesUpdatedRegistration);

            EtpMemoryListsWindowHandle = NULL;
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
            }
        }
        break;
    case MSG_UPDATE:
        {
            EtpUpdateMemoryListInfo(hwndDlg);
        }
        break;
    }

    return FALSE;
}
