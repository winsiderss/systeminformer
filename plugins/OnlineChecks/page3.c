/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016
 *
 */

#include "onlnchk.h"

HRESULT CALLBACK TaskDialogProgressCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_NAVIGATED:
        {
            SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

            if (context->TaskbarListClass)
                ITaskbarList3_SetProgressState(context->TaskbarListClass, PhMainWndHandle, TBPF_INDETERMINATE);

            PhReferenceObject(context);
            PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), UploadFileThreadStart, context);
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDCANCEL)
            {
                context->Cancel = TRUE;
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

HRESULT CALLBACK TaskDialogReScanProgressCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_NAVIGATED:
        {
            SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

            PhReferenceObject(context);
            PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), UploadRecheckThreadStart, context);
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDCANCEL)
            {
                context->Cancel = TRUE;
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

HRESULT CALLBACK TaskDialogViewReportProgressCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PUPLOAD_CONTEXT context = (PUPLOAD_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_NAVIGATED:
        {
            SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

            PhReferenceObject(context);
            PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), ViewReportThreadStart, context);
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDCANCEL)
            {
                context->Cancel = TRUE;
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

VOID ShowVirusTotalProgressDialog(
    _In_ PUPLOAD_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_EXPAND_FOOTER_AREA | TDF_ENABLE_HYPERLINKS | TDF_SHOW_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE);

    config.pszWindowTitle = PhaFormatString(L"Uploading %s...", PhGetStringOrEmpty(Context->BaseFileName))->Buffer;
    config.pszMainInstruction = PhaFormatString(L"Uploading %s...", PhGetStringOrEmpty(Context->BaseFileName))->Buffer;
    config.pszContent = L"Uploaded: ~ of ~ (0%)\r\nSpeed: ~ KB/s";

    config.cxWidth = 200;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pfCallback = TaskDialogProgressCallbackProc;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}

VOID ShowVirusTotalReScanProgressDialog(
    _In_ PUPLOAD_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_EXPAND_FOOTER_AREA | TDF_ENABLE_HYPERLINKS | TDF_SHOW_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE);

    config.pszWindowTitle = PhaFormatString(L"Rescanning %s...", PhGetStringOrEmpty(Context->BaseFileName))->Buffer;
    config.pszMainInstruction = PhaFormatString(L"Rescanning %s...", PhGetStringOrEmpty(Context->BaseFileName))->Buffer;

    config.cxWidth = 200;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pfCallback = TaskDialogReScanProgressCallbackProc;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}

VOID ShowVirusTotalViewReportProgressDialog(
    _In_ PUPLOAD_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_EXPAND_FOOTER_AREA | TDF_ENABLE_HYPERLINKS | TDF_SHOW_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE);

    config.pszWindowTitle = PhaFormatString(L"Locating analysis for %s...", PhGetStringOrEmpty(Context->BaseFileName))->Buffer;
    config.pszMainInstruction = PhaFormatString(L"Locating analysis for %s...", PhGetStringOrEmpty(Context->BaseFileName))->Buffer;

    config.cxWidth = 200;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pfCallback = TaskDialogViewReportProgressCallbackProc;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}
