/*
 * Process Hacker Plugins -
 *   Hardware Devices Plugin
 *
 * Copyright (C) 2015-2016 dmex
 * Copyright (C) 2016 wj32
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

#include "devices.h"

VOID NTAPI DiskDriveProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PDV_DISK_DETAILS_CONTEXT context = Context;

    if (context->WindowHandle)
    {
        //PostMessage(context->WindowHandle, UPDATE_MSG, 0, 0);
    }
}

VOID DiskDriveUpdateDetails(
    _Inout_ PDV_DISK_DETAILS_CONTEXT Context
    )
{
    HANDLE deviceHandle;

    if (NT_SUCCESS(DiskDriveCreateHandle(
        &deviceHandle,
        Context->DiskId.DevicePath
        )))
    {
        PPH_LIST attributes;

        if (DiskDriveQueryImminentFailure(deviceHandle, &attributes))
        {
            for (ULONG i = 0; i < attributes->Count; i++)
            {
                PSMART_ATTRIBUTES attribute = attributes->Items[i];

                INT lvItemIndex = PhAddListViewItem(
                    Context->ListViewHandle, 
                    MAXINT, 
                    SmartAttributeGetText(attribute->AttributeId),
                    (PVOID)attribute->AttributeId
                    );

                if (attribute->RawValue)
                {
                    PhSetListViewSubItem(
                        Context->ListViewHandle,
                        lvItemIndex,
                        1,
                        PhaFormatString(L"%u", attribute->RawValue)->Buffer
                        );
                }
                else
                {
                    PhSetListViewSubItem(
                        Context->ListViewHandle,
                        lvItemIndex,
                        1,
                        PhaFormatString(L"%u", attribute->CurrentValue)->Buffer
                        );
                }

                PhSetListViewSubItem(
                    Context->ListViewHandle,
                    lvItemIndex,
                    2,
                    PhaFormatString(L"%u", attribute->WorstValue)->Buffer
                    );

                PhFree(attribute);
            }

            PhDereferenceObject(attributes);
        }

        NtClose(deviceHandle);
    }
}

INT_PTR CALLBACK DiskDriveDetailsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_DISK_DETAILS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PDV_DISK_DETAILS_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PDV_DISK_DETAILS_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_DETAILS_LIST);

            if (context->DiskName)
                SetWindowText(hwndDlg, context->DiskName->Buffer);
            // BUG

            PhCenterWindow(hwndDlg, context->ParentHandle);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 250, L"Property");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 50, L"Value");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 50, L"Best");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 70, L"Raw");

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_DESCRIPTION), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_EDIT1), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_WARN), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                DiskDriveProcessesUpdatedHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );

            DiskDriveUpdateDetails(context);
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(
                &PhProcessesUpdatedEvent,
                &context->ProcessesUpdatedRegistration
                );

            PhDeleteLayoutManager(&context->LayoutManager);
            PostQuitMessage(0);
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
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SHOWDIALOG:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case UPDATE_MSG:
        {
            DiskDriveUpdateDetails(context);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->code == LVN_ITEMCHANGED)
            {
                LPNM_LISTVIEW listView = (LPNM_LISTVIEW)lParam;
                //if (listView->uChanged & LVIF_STATE && listView->uNewState & LVIS_SELECTED)

                PWSTR description;

                if (ListView_GetSelectedCount(context->ListViewHandle) == 1)
                    description = SmartAttributeGetDescription((SMART_ATTRIBUTE_ID)listView->lParam); // PhGetSelectedListViewItemParam(context->ListViewHandle)
                else
                    description = L"";

                SetDlgItemText(hwndDlg, IDC_EDIT1, description);

            }
        }
        break;
    }

    return FALSE;
}

VOID FreeDiskDriveDetailsContext(
    _In_ PDV_DISK_DETAILS_CONTEXT Context
    )
{
    DeleteDiskId(&Context->DiskId);
    PhClearReference(&Context->DiskName);
    PhFree(Context);
}

NTSTATUS ShowDiskDetailsDialogThread(
    _In_ PVOID Parameter
    )
{
    PDV_DISK_DETAILS_CONTEXT context = (PDV_DISK_DETAILS_CONTEXT)Parameter;
    BOOL result;
    MSG message;
    HWND dialogHandle;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    dialogHandle = CreateDialogParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_DISKDRIVE_DETAILS),
        NULL,
        DiskDriveDetailsDlgProc,
        (LPARAM)context
        );

    PostMessage(dialogHandle, WM_SHOWDIALOG, 0, 0);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(dialogHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    FreeDiskDriveDetailsContext(context);

    return STATUS_SUCCESS;
}

VOID ShowDiskDriveDetailsDialog(
    _In_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    HANDLE dialogThread = NULL;
    PDV_DISK_DETAILS_CONTEXT context;

    context = PhAllocate(sizeof(DV_DISK_DETAILS_CONTEXT));
    memset(context, 0, sizeof(DV_DISK_DETAILS_CONTEXT));

    context->ParentHandle = Context->WindowHandle;
    PhSetReference(&context->DiskName, Context->DiskEntry->DiskName);
    CopyDiskId(&context->DiskId, &Context->DiskEntry->Id);

    if (dialogThread = PhCreateThread(0, ShowDiskDetailsDialogThread, context))
        NtClose(dialogThread);
    else
        FreeDiskDriveDetailsContext(context);
}