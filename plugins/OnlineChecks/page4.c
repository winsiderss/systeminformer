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

HRESULT CALLBACK TaskDialogErrorProc(
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
            if (context->TaskbarListClass)
            {
                ITaskbarList3_SetProgressValue(context->TaskbarListClass, PhMainWndHandle, 1, 1);
                ITaskbarList3_SetProgressState(context->TaskbarListClass, PhMainWndHandle, TBPF_ERROR);
            }
        }
        break;
    }

    return S_OK;
}

VOID VirusTotalShowErrorDialog(
    _In_ PUPLOAD_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE);

    config.pszWindowTitle = PhaFormatString(L"Uploading %s...", PhGetStringOrEmpty(Context->BaseFileName))->Buffer;
    config.pszMainInstruction = PhaFormatString(L"Error uploading %s...", PhGetStringOrEmpty(Context->BaseFileName))->Buffer;
    config.pszContent = PhGetStringOrEmpty(Context->ErrorString);

    config.cxWidth = 200;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pfCallback = TaskDialogErrorProc;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}
