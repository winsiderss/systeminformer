#include "toolstatus.h"

INT_PTR CALLBACK OptionsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND comboHandle = GetDlgItem(hwndDlg, IDC_DISPLAYSTYLECOMBO);

            ComboBox_AddString(comboHandle, L"Images only");
            ComboBox_AddString(comboHandle, L"Selective text");
            ComboBox_AddString(comboHandle, L"All text");
            ComboBox_SetCurSel(comboHandle, PhGetIntegerSetting(L"ProcessHacker.ToolStatus.ToolbarDisplayStyle"));

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLETOOLBAR), EnableToolBar ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLESTATUSBAR), EnableStatusBar ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_RESOLVEGHOSTWINDOWS), 
                PhGetIntegerSetting(L"ProcessHacker.ToolStatus.ResolveGhostWindows") ? BST_CHECKED : BST_UNCHECKED);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    PhSetIntegerSetting(L"ProcessHacker.ToolStatus.ToolbarDisplayStyle",
                        (DisplayStyle = ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_DISPLAYSTYLECOMBO))));
                    PhSetIntegerSetting(L"ProcessHacker.ToolStatus.EnableToolBar",
                        (EnableToolBar = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLETOOLBAR)) == BST_CHECKED));
                    PhSetIntegerSetting(L"ProcessHacker.ToolStatus.EnableStatusBar",
                        (EnableStatusBar = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLESTATUSBAR)) == BST_CHECKED));
                    PhSetIntegerSetting(L"ProcessHacker.ToolStatus.ResolveGhostWindows",
                        Button_GetCheck(GetDlgItem(hwndDlg, IDC_RESOLVEGHOSTWINDOWS)) == BST_CHECKED);

                    ApplyToolbarSettings();

                    SendMessage(PhMainWndHandle, WM_SIZE, 0, 0);

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}