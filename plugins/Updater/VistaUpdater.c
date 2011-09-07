/*
* Process Hacker Update Checker -
*   main window
*
* Copyright (C) 2011 wj32
* Copyright (C) 2011 dmex
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

#include "updater.h"

TASKDIALOGCONFIG UpdateAvailablePage = { 0 };

void Test()
{
    HRESULT hr;
    
    int nButton;
    int nRadioButton;
    BOOL fVerificationFlagChecked;

    const TASKDIALOG_BUTTON cb[] =
    { 
        { 1000, L"&Download the update now" },
        { 1001, L"Do &not download the update" },
    };

    UpdateAvailablePage.cbSize = sizeof(UpdateAvailablePage);

    UpdateAvailablePage.hwndParent = PhMainWndHandle;
    UpdateAvailablePage.hInstance = PhLibImageBase;
    UpdateAvailablePage.dwFlags = TDF_USE_HICON_FOOTER | TDF_USE_HICON_MAIN
        | TDF_ALLOW_DIALOG_CANCELLATION 
        | TDF_EXPAND_FOOTER_AREA  
        | TDF_USE_COMMAND_LINKS
        | TDF_ENABLE_HYPERLINKS
        //| TDIF_SIZE_TO_CONTENT 
        | TDF_POSITION_RELATIVE_TO_WINDOW;

    UpdateAvailablePage.dwCommonButtons = TDCBF_CLOSE_BUTTON;

    // TaskDialog icons
    LoadIconWithScaleDown(NULL, IDI_INFORMATION, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), &UpdateAvailablePage.hMainIcon);
    LoadIconWithScaleDown(NULL, IDI_INFORMATION, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), &UpdateAvailablePage.hFooterIcon);
  
    // TaskDialog strings
    UpdateAvailablePage.pszWindowTitle = L"Process Hacker 2.21";
    UpdateAvailablePage.pszMainInstruction = L"An update for Process Hacker is available.";
    UpdateAvailablePage.pszContent = L"Version: 2.21 \r\nReleased: September 10, 2011";
    UpdateAvailablePage.pszExpandedInformation = L"<A HREF=\"executablestring\">View Changelog</A>";

    // TaskDialog buttons
    UpdateAvailablePage.cButtons = _countof(cb);
    UpdateAvailablePage.pButtons = cb;

    UpdateAvailablePage.pfCallback = TaskDlgWndProc;

    hr = TaskDialogIndirect(&UpdateAvailablePage, &nButton, &nRadioButton, &fVerificationFlagChecked);
    
    if (SUCCEEDED(hr))
    {
        if (fVerificationFlagChecked)
        {
            // the user checked the verification flag...
            // do any additional work here
        }
    }
    else
    {
        // some error occurred...check hr to see what it is
    }
}

HRESULT CALLBACK TaskDlgWndProc(
    __in HWND hwnd, 
    __in UINT uMsg, 
    __in WPARAM wParam, 
    __in LPARAM lParam, 
    __in LONG_PTR lpRefData
    )
{
    switch (uMsg)
    {
    case TDN_CREATED:
        {

        }
        break;
    case TDN_DESTROYED:
        {

        }
        break; 
    case TDN_BUTTON_CLICKED:
        {
            switch(wParam)
            {
                //default:
            case 1000:
                {
                    TASKDIALOGCONFIG tc = { 0 };
                    int nButton;

                    tc.cbSize = sizeof(tc);

                    tc.hwndParent = PhMainWndHandle;
                    tc.hInstance = PhLibImageBase;
                    tc.dwFlags = TDF_USE_HICON_FOOTER | TDF_USE_HICON_MAIN
                        | TDF_ALLOW_DIALOG_CANCELLATION 
                        | TDF_EXPAND_FOOTER_AREA  
                        | TDF_USE_COMMAND_LINKS
                        | TDF_ENABLE_HYPERLINKS
                        //| TDIF_SIZE_TO_CONTENT 
                        | TDF_POSITION_RELATIVE_TO_WINDOW 
                        | TDF_SHOW_PROGRESS_BAR 
                        | TDF_CALLBACK_TIMER;

                    tc.dwCommonButtons = TDCBF_CLOSE_BUTTON;

                    // TaskDialog icons
                    LoadIconWithScaleDown(NULL, IDI_INFORMATION, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), &tc.hMainIcon);
                    LoadIconWithScaleDown(NULL, IDI_INFORMATION, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), &tc.hFooterIcon);
                    
                    // TaskDialog strings
                    tc.pszWindowTitle = L"Process Hacker Updater";
                    tc.pszMainInstruction = L"Downloading Process Hacker 2.21";
                    tc.pszContent = L"Version: 2.21 \r\nRemaning: 10.21 MB / 12.23MB \r\nSpeed: 100.00 kB/s";

                    tc.pfCallback = TaskDlgWndProc;

                    SendMessage(hwnd, TDM_NAVIGATE_PAGE, NULL, &tc);

                    return S_FALSE;
                }
                break;
            }
        }
        break;
    case TDN_NAVIGATED:
        {
            return S_FALSE;
        }
        break;
    case TDN_TIMER:
        {
            SendMessage(hwnd, TDM_SET_PROGRESS_BAR_POS, 50, NULL);
            return S_FALSE;
        }
        break;
    case TDN_HYPERLINK_CLICKED:        
        {

        }
        break;
    }

    return S_OK;
}