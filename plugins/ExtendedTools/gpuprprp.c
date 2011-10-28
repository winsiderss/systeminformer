/*
 * Process Hacker Extended Tools -
 *   GPU process properties page
 *
 * Copyright (C) 2011 wj32
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
#include "d3dkmt.h"

#define MSG_UPDATE (WM_APP + 1)

typedef struct _ET_GPU_CONTEXT
{
    HWND WindowHandle;
    PET_PROCESS_BLOCK Block;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
    BOOLEAN Enabled;
} ET_GPU_CONTEXT, *PET_GPU_CONTEXT;

VOID NTAPI GpuUpdateHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

INT_PTR CALLBACK EtpGpuPageDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID EtProcessGpuPropertiesInitializing(
    __in PVOID Parameter
    )
{
    PPH_PLUGIN_PROCESS_PROPCONTEXT propContext = Parameter;

    if (EtGpuEnabled)
    {
        PhAddProcessPropPage(
            propContext->PropContext,
            PhCreateProcessPropPageContextEx(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_PROCGPU), EtpGpuPageDlgProc, NULL)
            );
    }
}

VOID NTAPI GpuUpdateHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PET_GPU_CONTEXT context = Context;

    if (context->Enabled)
        PostMessage(context->WindowHandle, MSG_UPDATE, 0, 0);
}

VOID EtpUpdateGpuInfo(
    __in HWND hwndDlg,
    __in PET_GPU_CONTEXT Context
    )
{
    PET_PROCESS_BLOCK block = Context->Block;
    ET_PROCESS_GPU_STATISTICS statistics;
    WCHAR runningTimeString[PH_TIMESPAN_STR_LEN_1] = L"N/A";

    if (Context->Block->ProcessItem->QueryHandle)
        EtQueryProcessGpuStatistics(Context->Block->ProcessItem->QueryHandle, &statistics);

    PhPrintTimeSpan(runningTimeString, statistics.RunningTime * 10, PH_TIMESPAN_HMSM);

    SetDlgItemText(hwndDlg, IDC_ZDEDICATEDCOMMITTED_V, PhaFormatSize(statistics.DedicatedCommitted, -1)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZSHAREDCOMMITTED_V, PhaFormatSize(statistics.SharedCommitted, -1)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZTOTALSEGMENTS_V, PhaFormatUInt64(statistics.SegmentCount, TRUE)->Buffer);

    SetDlgItemText(hwndDlg, IDC_ZRUNNINGTIME_V, runningTimeString);
    SetDlgItemText(hwndDlg, IDC_ZCONTEXTSWITCHES_V, PhaFormatUInt64(statistics.ContextSwitches, TRUE)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZTOTALNODES_V, PhaFormatUInt64(statistics.NodeCount, TRUE)->Buffer);

    SetDlgItemText(hwndDlg, IDC_ZTOTALALLOCATED_V, PhaFormatSize(statistics.BytesAllocated, -1)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZTOTALRESERVED_V, PhaFormatSize(statistics.BytesReserved, -1)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZWRITECOMBINEDALLOCATED_V, PhaFormatSize(statistics.WriteCombinedBytesAllocated, -1)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZWRITECOMBINEDRESERVED_V, PhaFormatSize(statistics.WriteCombinedBytesReserved, -1)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZCACHEDALLOCATED_V, PhaFormatSize(statistics.CachedBytesAllocated, -1)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZCACHEDRESERVED_V, PhaFormatSize(statistics.CachedBytesReserved, -1)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZSECTIONALLOCATED_V, PhaFormatSize(statistics.SectionBytesAllocated, -1)->Buffer);
    SetDlgItemText(hwndDlg, IDC_ZSECTIONRESERVED_V, PhaFormatSize(statistics.SectionBytesReserved, -1)->Buffer);
}

INT_PTR CALLBACK EtpGpuPageDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PET_GPU_CONTEXT context;

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
            context = PhAllocate(sizeof(ET_GPU_CONTEXT));
            propPageContext->Context = context;
            context->WindowHandle = hwndDlg;
            context->Block = EtGetProcessBlock(processItem);
            context->Enabled = TRUE;

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                GpuUpdateHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );

            EtpUpdateGpuInfo(hwndDlg, context);
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
            EtpUpdateGpuInfo(hwndDlg, context);
        }
        break;
    }

    return FALSE;
}
