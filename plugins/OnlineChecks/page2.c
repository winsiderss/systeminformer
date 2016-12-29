/*
 * Process Hacker Plugins -
 *   Update Checker Plugin
 *
 * Copyright (C) 2016 dmex
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

#include "onlnchk.h"

static TASKDIALOG_BUTTON TaskDialogButtonArray[] =
{
    { IDOK, L"Upload file" },
    { IDRETRY, L"Reanalyse file" },
    { IDYES, L"View last analysis" },
};

HRESULT CALLBACK ShowAvailableCallbackProc(
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
                if (!PhIsNullOrEmptyString(context->reAnalyseUrl))
                    PhShellExecute(hwndDlg, PhGetString(context->reAnalyseUrl), NULL);
            }
            else if (buttonID == IDYES)
            {
                if (!PhIsNullOrEmptyString(context->LaunchCommand))
                    PhShellExecute(hwndDlg, PhGetString(context->LaunchCommand), NULL);
            }
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
    config.hMainIcon = Context->IconLargeHandle;

    if (Context->FileExists)
    {    
        //was last analysed by VirusTotal on 2016-12-28 05:26:50 UTC (1 hour ago) it was first analysed by VirusTotal on 2016-12-12 17:08:19 UTC.
        config.pszMainInstruction = PhaFormatString(
            L"%s was last analysed %s ago", 
            PhGetStringOrEmpty(Context->BaseFileName), 
            PhGetStringOrEmpty(Context->LastAnalysisAgo)
            )->Buffer;

        config.pszContent = PhaFormatString(
            L"Detection ratio: %s/%s\r\nFirst analysed: %s\r\nLast analysed: %s\r\n\r\nYou can take a look at the last analysis or upload it again now.",
            PhGetStringOrEmpty(Context->Detected),
            PhGetStringOrEmpty(Context->MaxDetected),
            PhGetStringOrEmpty(Context->FirstAnalysisDate),
            PhGetStringOrEmpty(Context->LastAnalysisDate)
            )->Buffer;
    }
    else
    {
        config.pszMainInstruction = PhaFormatString(L"%s existing", PhGetStringOrEmpty(Context->BaseFileName))->Buffer;
        config.pszContent = PhaFormatString(L"existing")->Buffer;
    }

    //TaskDialogButtonArray[0].pszButtonText = PhaFormatString(L"Upload %s", PhGetStringOrEmpty(Context->BaseFileName))->Buffer;

    config.cxWidth = 200;
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = ARRAYSIZE(TaskDialogButtonArray);

    config.lpCallbackData = (LONG_PTR)Context;
    config.pfCallback = ShowAvailableCallbackProc;

    SendMessage(Context->DialogHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)&config);
}