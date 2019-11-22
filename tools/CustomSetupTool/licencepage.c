/*
 * Process Hacker Toolchain -
 *   project setup
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

#include <setup.h>

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

    config.pszWindowTitle = L"Process Hacker - Setup";
    config.pszMainInstruction = L"License Agreement";
    config.pszContent = L"Process Hacker is distributed under the GNU GPL version 3:\r\n<a href=\"https://raw.githubusercontent.com/processhacker/processhacker/master/LICENSE.txt\">View LICENSE.txt</a>\r\n\r\nSelect \"Next\" to continue.";

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
