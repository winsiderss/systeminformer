#include <setup.h>
#include <appsup.h>

VOID LoadEulaText(
    _In_ HWND hwndDlg
    )
{
    HRSRC resourceHandle;
    HGLOBAL resourceData;
    PVOID resourceBuffer;

    if (resourceHandle = FindResource(PhLibImageBase, MAKEINTRESOURCE(IDR_TXT1), L"TXT"))
    {
        if (resourceData = LoadResource(PhLibImageBase, resourceHandle))
        {
            if (resourceBuffer = LockResource(resourceData))
            {
                PPH_STRING eulaTextString;
                
                if (eulaTextString = PhConvertMultiByteToUtf16(resourceBuffer))
                {
                    SetWindowText(GetDlgItem(hwndDlg, IDC_EDIT1), eulaTextString->Buffer);
                    PhDereferenceObject(eulaTextString);
                }
            }
        }

        FreeResource(resourceHandle);
    }
}

BOOL PropSheetPage2_OnInitDialog(
    _In_ HWND hwndDlg,
    _In_ HWND hwndFocus,
    _Inout_ LPARAM lParam
    )
{
    // Set the fonts.
    InitializeFont(GetDlgItem(hwndDlg, IDC_MAINHEADER), -17, FW_SEMIBOLD);

    // Set the default radio button state to 'do not accept'.
    Button_SetCheck(GetDlgItem(hwndDlg, IDC_RADIO2), BST_CHECKED);

    // Enable the themed dialog background texture.
    EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);

    // Load the EULA text into the window.
    LoadEulaText(hwndDlg);

    return TRUE;
}

BOOL PropSheetPage2_OnNotify(
    _In_ HWND hwndDlg,
    _In_ INT idCtrl,
    _Inout_ LPNMHDR lpNmh
    )
{
    LPPSHNOTIFY pageNotify = (LPPSHNOTIFY)lpNmh;

    switch (pageNotify->hdr.code)
    {
    case PSN_SETACTIVE:
        {
            HWND hwPropSheet = pageNotify->hdr.hwndFrom;

#ifndef DEBUG
            // Disable the property sheet Next button, the user must accept the EULA to continue.
            PropSheet_SetWizButtons(hwPropSheet, PSWIZB_BACK);
#endif
        }
        break;
    case PSN_QUERYINITIALFOCUS:
        {
            // Set the default control as the 'do not accept' radio button.
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)GetDlgItem(hwndDlg, IDC_RADIO2));
        }
        return TRUE;
    }

    return FALSE;
}

BOOL PropSheetPage2_OnCommand(
    _In_ HWND hwndDlg,
    _In_ INT id,
    _In_ HWND hwndCtl,
    _In_ UINT codeNotify
    )
{
    switch (id)
    {
    case IDC_RADIO1:
    case IDC_RADIO2:
        {
            if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_RADIO1)) == BST_CHECKED)
            {
                // The user has agreed to the EULA, enable the Next button.
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
            }
            else
            {
                // The user did not agree, disable the next button.
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PropSheetPage2_WndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _Inout_ WPARAM wParam,
    _Inout_ LPARAM lParam
    )
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, PropSheetPage2_OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, PropSheetPage2_OnNotify);
        HANDLE_MSG(hwndDlg, WM_COMMAND, PropSheetPage2_OnCommand);
    }

    return FALSE;
}