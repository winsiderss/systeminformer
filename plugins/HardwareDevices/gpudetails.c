/*
* Process Hacker Extra Plugins -
*   Nvidia GPU Plugin
*
* Copyright (C) 2015 dmex
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

static VOID NvUpdateDetails(
    _Inout_ PPH_NVGPU_SYSINFO_CONTEXT Context
    )
{
    //SetDlgItemText(Context->DetailsHandle, IDC_EDIT1, ((PPH_STRING)PH_AUTO(NvGpuQueryName()))->Buffer);
    //SetDlgItemText(Context->DetailsHandle, IDC_EDIT2, ((PPH_STRING)PH_AUTO(NvGpuQueryShortName()))->Buffer);
    //SetDlgItemText(Context->DetailsHandle, IDC_EDIT3, ((PPH_STRING)PH_AUTO(NvGpuQueryVbiosVersionString()))->Buffer);
    //SetDlgItemText(Context->DetailsHandle, IDC_EDIT4, ((PPH_STRING)PH_AUTO(NvGpuQueryRevision()))->Buffer);
    //SetDlgItemText(Context->DetailsHandle, IDC_EDIT5, ((PPH_STRING)PH_AUTO(NvGpuQueryDeviceId()))->Buffer);
    //SetDlgItemText(Context->DetailsHandle, IDC_EDIT6, ((PPH_STRING)PH_AUTO(NvGpuQueryRopsCount()))->Buffer);
    //SetDlgItemText(Context->DetailsHandle, IDC_EDIT7, ((PPH_STRING)PH_AUTO(NvGpuQueryShaderCount()))->Buffer);
    ////SetDlgItemText(Context->DetailsHandle, IDC_EDIT8, ((PPH_STRING)PhAutoDereferenceObject(NvGpuQueryPciInfo()))->Buffer);
    //SetDlgItemText(Context->DetailsHandle, IDC_EDIT9, ((PPH_STRING)PH_AUTO(NvGpuQueryBusWidth()))->Buffer);
    //SetDlgItemText(Context->DetailsHandle, IDC_EDIT10, ((PPH_STRING)PH_AUTO(NvGpuQueryDriverVersion()))->Buffer);
    //SetDlgItemText(Context->DetailsHandle, IDC_EDIT12, ((PPH_STRING)PH_AUTO(NvGpuQueryDriverSettings()))->Buffer);
    //SetDlgItemText(Context->DetailsHandle, IDC_EDIT14, ((PPH_STRING)PH_AUTO(NvGpuQueryRamType()))->Buffer);
    ////SetDlgItemText(Context->DetailsHandle, IDC_EDIT13, ((PPH_STRING)PhAutoDereferenceObject(NvGpuQueryRamMaker()))->Buffer);
    ////SetDlgItemText(Context->DetailsHandle, IDC_EDIT13, NvGpuDriverIsWHQL() ? L"WHQL" : L"");
    //SetDlgItemText(Context->DetailsHandle, IDC_EDIT13, ((PPH_STRING)PH_AUTO(NvGpuQueryFoundry()))->Buffer);
    //SetDlgItemText(Context->DetailsHandle, IDC_EDIT11, ((PPH_STRING)PH_AUTO(NvGpuQueryPcbValue()))->Buffer);
}

static INT_PTR CALLBACK DetailsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_NVGPU_SYSINFO_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_NVGPU_SYSINFO_CONTEXT)lParam;

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PPH_NVGPU_SYSINFO_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_NCDESTROY)
        {
            RemoveProp(hwndDlg, L"Context");
        }
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->DetailsHandle = hwndDlg;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            //HBITMAP bitmapRefresh = NULL;
            //bitmapRefresh = LoadImageFromResources(96, 64, MAKEINTRESOURCE(IDB_NV_LOGO_PNG));
            //SendMessage(GetDlgItem(hwndDlg, IDC_NVIMAGE), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmapRefresh);
            //DeleteObject(bitmapRefresh);

            NvUpdateDetails(context);
        }
        break;
    case WM_DESTROY:
        {
            context->DetailsHandle = NULL;
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case UPDATE_MSG:
        {
            NvUpdateDetails(context);
        }
        break;
    }

    return FALSE;
}

VOID ShowDetailsDialog(
    _In_ HWND ParentHandle,
    _In_ PVOID Context
    )
{
    //DialogBoxParam(
    //    PluginInstance->DllBase,
    //    MAKEINTRESOURCE(IDD_GPU_DETAILS),
    //    ParentHandle,
    //    DetailsDlgProc,
    //    (LPARAM)Context
    //    );
}