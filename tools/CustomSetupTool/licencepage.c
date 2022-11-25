/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex
 *
 */

#include "setup.h"

static TASKDIALOG_BUTTON TaskDialogButtonArray[] =
{
    { IDCONTINUE, L"Next" },
};

HRESULT CALLBACK SetupLicencePageCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_SETUP_CONTEXT context = (PPH_SETUP_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDCONTINUE)
            {
                ShowConfigPageDialog(context);
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

VOID ShowLicencePageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = RTL_NUMBER_OF(TaskDialogButtonArray);
    config.pfCallback = SetupLicencePageCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.cxWidth = 200;

    config.pszWindowTitle = L"System Informer - Setup";
    config.pszMainInstruction = L"License Agreement";
    config.pszContent = L"System Informer is distributed by Winsider Seminars & Solutions, Inc.:\r\n<a href=\"https://raw.githubusercontent.com/winsiderss/systeminformer/blob/master/LICENSE.txt\">View LICENSE.txt</a>\r\n\r\nSelect \"Next\" to continue.";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}

//    case WM_COMMAND:
//        {
//            switch (GET_WM_COMMAND_CMD(wParam, lParam))
//            {
//            case EN_SETFOCUS:
//                {
//                    HWND handle = (HWND)lParam;
//
//                    // Prevent the edit control text from being selected.
//                    SendMessage(handle, EM_SETSEL, -1, 0);
//
//                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)TRUE);
//                    return TRUE;
//                }
//            }
//        }
//        break;
//    }
