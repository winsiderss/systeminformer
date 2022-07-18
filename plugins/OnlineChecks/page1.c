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

HRESULT CALLBACK TaskDialogProcessingCallbackProc(
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
            PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), UploadCheckThreadStart, context);
        }
        break;
    }

    return S_OK;
}

VOID ShowVirusTotalUploadDialog(
    _In_ PUPLOAD_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);

    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_SHOW_MARQUEE_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE);

    config.pszWindowTitle = PhaFormatString(L"Scanning %s...", PhGetStringOrEmpty(Context->BaseFileName))->Buffer;
    config.pszMainInstruction = PhaFormatString(L"Scanning %s...", PhGetStringOrEmpty(Context->BaseFileName))->Buffer;

    config.cxWidth = 200;
    config.pfCallback = TaskDialogProcessingCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}
