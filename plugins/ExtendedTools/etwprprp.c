/*
 * Process Hacker Extended Tools -
 *   ETW process properties page
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

typedef struct _ET_DISKNET_CONTEXT
{
    HWND WindowHandle;
    PET_PROCESS_BLOCK Block;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
    BOOLEAN Enabled;
} ET_DISKNET_CONTEXT, *PET_DISKNET_CONTEXT;

VOID NTAPI DiskNetworkUpdateHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

INT_PTR CALLBACK EtpDiskNetworkPageDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID EtProcessEtwPropertiesInitializing(
    __in PVOID Parameter
    )
{
    PPH_PLUGIN_PROCESS_PROPCONTEXT propContext = Parameter;

    if (EtEtwEnabled)
    {
        PhAddProcessPropPage(
            propContext->PropContext,
            PhCreateProcessPropPageContextEx(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_PROCDISKNET), EtpDiskNetworkPageDlgProc, NULL)
            );
    }
}

VOID NTAPI DiskNetworkUpdateHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PET_DISKNET_CONTEXT context = Context;

    if (context->Enabled)
        PostMessage(context->WindowHandle, MSG_UPDATE, 0, 0);
}

VOID EtpUpdateDiskNetworkInfo(
    __in HWND hwndDlg,
    __in PET_DISKNET_CONTEXT Context
    )
{
    PET_PROCESS_BLOCK block = Context->Block;

    SetDlgItemText(hwndDlg, IDC_ZREADS_V, PhaFormatUInt64(block->DiskReadCount, TRUE)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZREADBYTES_V, PhaFormatSize(block->DiskReadRawDelta.Value, -1)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZREADBYTESDELTA_V, PhaFormatSize(block->DiskReadRawDelta.Delta, -1)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZWRITES_V, PhaFormatUInt64(block->DiskWriteCount, TRUE)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZWRITEBYTES_V, PhaFormatSize(block->DiskWriteRawDelta.Value, -1)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZWRITEBYTESDELTA_V, PhaFormatSize(block->DiskWriteRawDelta.Delta, -1)->Buffer);

    SetDlgItemText(hwndDlg, IDC_ZRECEIVES_V, PhaFormatUInt64(block->NetworkReceiveCount, TRUE)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZRECEIVEBYTES_V, PhaFormatSize(block->NetworkReceiveRawDelta.Value, -1)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZRECEIVEBYTESDELTA_V, PhaFormatSize(block->NetworkReceiveRawDelta.Delta, -1)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZSENDS_V, PhaFormatUInt64(block->NetworkSendCount, TRUE)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZSENDBYTES_V, PhaFormatSize(block->NetworkSendRawDelta.Value, -1)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZSENDBYTESDELTA_V, PhaFormatSize(block->NetworkSendRawDelta.Delta, -1)->Buffer);
}

INT_PTR CALLBACK EtpDiskNetworkPageDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PET_DISKNET_CONTEXT context;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext, &processItem))
    {
        context = propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context = PhAllocate(sizeof(ET_DISKNET_CONTEXT));
            propPageContext->Context = context;
            context->WindowHandle = hwndDlg;
            context->Block = EtGetProcessBlock(processItem);
            context->Enabled = TRUE;

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                DiskNetworkUpdateHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );

            EtpUpdateDiskNetworkInfo(hwndDlg, context);
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(&PhProcessesUpdatedEvent, &context->ProcessesUpdatedRegistration);
            PhFree(context);

            PhPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (PhBeginPropPageLayout(hwndDlg, propPageContext))
                PhEndPropPageLayout(hwndDlg, propPageContext);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_SETACTIVE:
                context->Enabled = TRUE;
                break;
            case PSN_KILLACTIVE:
                context->Enabled = FALSE;
                break;
            }
        }
        break;
    case MSG_UPDATE:
        {
            EtpUpdateDiskNetworkInfo(hwndDlg, context);
        }
        break;
    }

    return FALSE;
}
