/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2021
 *
 */

#include "onlnchk.h"

static TASKDIALOG_BUTTON TaskDialogButtonArray[] =
{
    { IDYES, L"View last analysis\nOpen the last VirusTotal analysis page" },
    //{ IDRETRY, L"Reanalyze file\nRescan the existing sample on VirusTotal" },
    { IDOK, L"Upload file\nUpload fresh sample to VirusTotal for analysis" },
};

HRESULT CALLBACK TaskDialogResultFoundProc(
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
                ITaskbarList3_SetProgressState(context->TaskbarListClass, PhMainWndHandle, TBPF_NOPROGRESS);
            }
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            INT buttonID = (INT)wParam;

            if (buttonID == IDOK)
            {
                ShowVirusTotalProgressDialog(context);
                return S_FALSE;
            }
            else if (buttonID == IDRETRY)
            {
//#ifdef PH_BUILD_API
                ShowVirusTotalReScanProgressDialog(context);
                return S_FALSE;
//#else
//                if (!PhIsNullOrEmptyString(context->ReAnalyseUrl))
//                {
//                    PhShellExecute(hwndDlg, PhGetString(context->ReAnalyseUrl), NULL);
//                }
//#endif
            }
            else if (buttonID == IDYES)
            {
//#ifdef PH_BUILD_API
                ShowVirusTotalViewReportProgressDialog(context);
                return S_FALSE;
//#else
//                if (!PhIsNullOrEmptyString(context->LaunchCommand))
//                {
//                    PhShellExecute(hwndDlg, PhGetString(context->LaunchCommand), NULL);
//                }
//#endif
            }
        }
        break;
    case TDN_VERIFICATION_CLICKED:
        {
            BOOL verification = (BOOL)wParam;
        }
        break;
    }

    return S_OK;
}

VOID ShowFileFoundDialog(
    _In_ PUPLOAD_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE);
    config.pszMainInstruction = PhaFormatString(
        L"%s was last analyzed %s ago",
        PhGetStringOrEmpty(Context->BaseFileName),
        PhGetStringOrEmpty(Context->LastAnalysisAgo)
        )->Buffer;
    // was last analyzed by VirusTotal on 2016-12-28 05:26:50 UTC (1 hour ago) it was first analyzed by VirusTotal on 2016-12-12 17:08:19 UTC.
    config.pszContent = PhaFormatString(
        L"Detection ratio: %s/%s\r\nFirst analyzed: %s\r\nLast analyzed: %s\r\nUpload size: %s\r\n\r\nYou can take a look at the last analysis or upload it again now.",
        PhGetStringOrEmpty(Context->Detected),
        PhGetStringOrEmpty(Context->MaxDetected),
        PhGetStringOrEmpty(Context->FirstAnalysisDate),
        PhGetStringOrEmpty(Context->LastAnalysisDate),
        PhGetStringOrEmpty(Context->FileSize)
        )->Buffer;
    config.pszVerificationText = L"Remember this selection...";
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = ARRAYSIZE(TaskDialogButtonArray);
    config.lpCallbackData = (LONG_PTR)Context;
    config.pfCallback = TaskDialogResultFoundProc;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}
