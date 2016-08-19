#include <setup.h>
#include <appsup.h>

VOID LoadInstallDirectory(
    _In_ HWND hwndDlg
    )
{
    // Load the path used by previous installations.
    SetupInstallPath = GetProcessHackerInstallPath();

    // If the string is null or empty, use the default installation path.
    if (PhIsNullOrEmptyString(SetupInstallPath))
    {
        PPH_STRING defaultInstallPath;
        PPH_STRING expandedString;

        // TODO: Does ProgramW6432 work on 32bit?
        defaultInstallPath = PhCreateString(L"%ProgramW6432%\\Process Hacker\\");

        if (expandedString = PhExpandEnvironmentStrings(&defaultInstallPath->sr))
        {
            SetupInstallPath = expandedString;
        }

        PhDereferenceObject(defaultInstallPath);
    }

    // If the string is null or empty, fallback to a hard-coded default path.
    if (PhIsNullOrEmptyString(SetupInstallPath))
    {
        SetupInstallPath = PhCreateString(L"C:\\Program Files\\Process Hacker\\");
    }
    
#ifdef _DEBUG
    PPH_STRING setupDirectory = PhGetApplicationDirectory();
    PhSwapReference(&SetupInstallPath, PhConcatStrings2(setupDirectory->Buffer, L"ProcessHacker_Test\\"));
    PhDereferenceObject(setupDirectory);
#endif

    // We must make sure the install path ends with a backslash since
    // the string is wcscat' with our zip extraction paths.
    if (PathAddBackslash(SetupInstallPath->Buffer))
    {
        //PathSearchAndQualify()
    }

    SetDlgItemText(hwndDlg, IDC_INSTALL_DIRECTORY, SetupInstallPath->Buffer);
}

BOOL PropSheetPage3_OnInitDialog(
    _In_ HWND hwndDlg,
    _In_ HWND hwndFocus,
    _Inout_ LPARAM lParam
    )
{
    // Set the fonts.
    InitializeFont(GetDlgItem(hwndDlg, IDC_MAINHEADER), -17, FW_SEMIBOLD);

    // Set the default checkboxes.
    Button_SetCheck(GetDlgItem(hwndDlg, IDC_CHECK1), TRUE);
    Button_SetCheck(GetDlgItem(hwndDlg, IDC_CHECK6), TRUE);

    // Fix the text margins.
    SendMessage(
        GetDlgItem(hwndDlg, IDC_INSTALL_DIRECTORY),
        EM_SETMARGINS,
        EC_LEFTMARGIN | EC_RIGHTMARGIN,
        MAKELPARAM(0, 0));

    // Query the default installation path
    LoadInstallDirectory(hwndDlg);

    // Enable the themed dialog background texture.
    EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);

    return TRUE;
}

BOOL PropSheetPage3_OnNotify(
    _In_ HWND hwndDlg,
    _In_ INT idCtrl,
    _Inout_ LPNMHDR lpNmh
    )
{
    LPPSHNOTIFY pageNotify = (LPPSHNOTIFY)lpNmh;

    switch (pageNotify->hdr.code)
    {
    case PSN_QUERYINITIALFOCUS:
        {
            // Set the default control as the browse button.
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)GetDlgItem(hwndDlg, IDC_FOLDER_BROWSE));
        }
        return TRUE;
    }

    return FALSE;
}

BOOL PropSheetPage3_OnCommand(
    _In_ HWND hwndDlg,
    _In_ INT id,
    _In_ HWND hwndCtl,
    _In_ UINT codeNotify
    )
{
    switch (id)
    {
    case IDC_FOLDER_BROWSE:
        {
            PPH_STRING installFolder;

            installFolder = BrowseForFolder(hwndDlg, L"Select installation folder");

            if (installFolder)
            {
                PhSwapReference(&SetupInstallPath, installFolder);

                SetDlgItemText(hwndDlg, IDC_INSTALL_DIRECTORY, SetupInstallPath->Buffer);
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PropSheetPage3_WndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _Inout_ WPARAM wParam,
    _Inout_ LPARAM lParam
    )
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, PropSheetPage3_OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, PropSheetPage3_OnNotify);
        HANDLE_MSG(hwndDlg, WM_COMMAND, PropSheetPage3_OnCommand);
    }

    return FALSE;
}