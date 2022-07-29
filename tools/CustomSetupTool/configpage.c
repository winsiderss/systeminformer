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

#include <setup.h>

static TASKDIALOG_BUTTON TaskDialogButtonArray[] =
{
    { IDYES, L"Browse" },
    { IDOK, L"Next" },
};

VOID SetupShowBrowseDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    PVOID fileDialog;

    fileDialog = PhCreateOpenFileDialog();
    PhSetFileDialogOptions(fileDialog, PH_FILEDIALOG_PICKFOLDERS);

    if (PhShowFileDialog(Context->DialogHandle, fileDialog))
    {
        PPH_STRING fileDialogFolderPath;

        fileDialogFolderPath = PH_AUTO(PhGetFileDialogFileName(fileDialog));
        PhTrimToNullTerminatorString(fileDialogFolderPath);
        PhSwapReference(&Context->SetupInstallPath, fileDialogFolderPath);

        PhFreeFileDialog(fileDialog);
    }

    if (PhIsNullOrEmptyString(Context->SetupInstallPath))
    {
        Context->SetupInstallPath = SetupFindInstallDirectory();
    }

    if (!PhIsNullOrEmptyString(Context->SetupInstallPath))
    {
        if (!PhEndsWithStringRef(&Context->SetupInstallPath->sr, &PhNtPathSeperatorString, TRUE))
        {
            PhSwapReference(&Context->SetupInstallPath, PhConcatStringRef2(&Context->SetupInstallPath->sr, &PhNtPathSeperatorString));
        }
    }
}

HRESULT CALLBACK SetupConfigPageCallbackProc(
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
    case TDN_NAVIGATED:
        {
            PPH_STRING status;

            status = PhFormatString(
                L"Installation Folder:\r\n\r\n%s",
                PhGetStringOrEmpty(context->SetupInstallPath)
                );
            SendMessage(hwndDlg, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)status->Buffer);
            PhDereferenceObject(status);
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDYES)
            {
                PPH_STRING status;

                SetupShowBrowseDialog(context);

                status = PhFormatString(
                    L"Installation Folder:\r\n\r\n%s",
                    PhGetStringOrEmpty(context->SetupInstallPath)
                    );
                SendMessage(hwndDlg, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)status->Buffer);
                PhDereferenceObject(status);

                return S_FALSE;
            }

            if ((INT)wParam == IDOK)
            {
                if (PhIsNullOrEmptyString(context->SetupInstallPath))
                    return S_FALSE;

#ifdef PH_BUILD_API
                ShowInstallPageDialog(context);
#else
                ShowDownloadPageDialog(context);
#endif
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

VOID ShowConfigPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.cxWidth = 200;
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = RTL_NUMBER_OF(TaskDialogButtonArray);
    config.pfCallback = SetupConfigPageCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Setup Options";
    config.pszContent = L"Installation Folder:\r\n\r\nSelect \"Browse\" to continue.";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}


//INT_PTR CALLBACK SetupPropPage3_WndProc(
//    _In_ HWND hwndDlg,
//    _In_ UINT uMsg,
//    _Inout_ WPARAM wParam,
//    _Inout_ LPARAM lParam
//    )
//{
//    PPH_SETUP_CONTEXT context = NULL;
//
//    if (uMsg == WM_INITDIALOG)
//    {
//        context = PhGetWindowContext(GetParent(hwndDlg), ULONG_MAX);
//        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
//    }
//    else
//    {
//        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
//    }
//
//    if (context == NULL)
//        return FALSE;
//
//    switch (uMsg)
//    {
//    case WM_INITDIALOG:
//        {

//
//            if (PhIsNullOrEmptyString(SetupInstallPath))
//                SetupInstallPath = SetupFindInstallDirectory();

//        }
//        break;
//    case WM_COMMAND:
//        {
//            switch (GET_WM_COMMAND_ID(wParam, lParam))
//            {
//            case IDC_FOLDER_BROWSE:
//                {
//                    PVOID fileDialog;
//
//                    fileDialog = PhCreateOpenFileDialog();
//                    PhSetFileDialogOptions(fileDialog, PH_FILEDIALOG_PICKFOLDERS);
//
//                    if (PhShowFileDialog(hwndDlg, fileDialog))
//                    {
//                        PPH_STRING fileDialogFolderPath;
//
//                        fileDialogFolderPath = PH_AUTO(PhGetFileDialogFileName(fileDialog));
//                        PhTrimToNullTerminatorString(fileDialogFolderPath);
//                        PhSwapReference(&SetupInstallPath, fileDialogFolderPath);
//
//                        PhFreeFileDialog(fileDialog);
//                    }
//
//                    if (PhIsNullOrEmptyString(SetupInstallPath))
//                        SetupInstallPath = SetupFindInstallDirectory();
//
//                    SetDlgItemText(hwndDlg, IDC_INSTALL_DIRECTORY, PhGetStringOrEmpty(SetupInstallPath));
//                }
//                break;
//            case IDC_STARTUP_CHECK:
//                {
//                    BOOL enabled = Button_GetCheck(GetDlgItem(hwndDlg, IDC_STARTUP_CHECK)) == BST_CHECKED;
//
//                    // Enable the 'Start minimized on system tray' checkbox if 'Start minimized' has been enabled.
//                    EnableWindow(GetDlgItem(hwndDlg, IDC_STARTMIN_CHECK), enabled);
//
//                    if (!enabled)
//                    {
//                        // Clear the 'Start minimized on system tray' checkbox if 'Start minimized' has been disabled.
//                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_STARTMIN_CHECK), FALSE);
//                    }
//                }
//                break;
//            }
//        }
//        break;
//    case WM_NOTIFY:
//        {
//            LPNMHDR header = (LPNMHDR)lParam;
//            LPPSHNOTIFY pageNotify = (LPPSHNOTIFY)header;
//
//            switch (pageNotify->hdr.code)
//            {
//            case PSN_WIZNEXT:
//                {
//                    context->SetupCreateDesktopShortcut = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SHORTCUT_CHECK)) == BST_CHECKED;
//                    context->SetupCreateDesktopShortcutAllUsers = Button_GetCheck(GetDlgItem(hwndDlg, IDC_SHORTCUT_ALL_CHECK)) == BST_CHECKED;
//                    context->SetupCreateDefaultTaskManager = Button_GetCheck(GetDlgItem(hwndDlg, IDC_TASKMANAGER_CHECK)) == BST_CHECKED;
//                    context->SetupCreateSystemStartup = Button_GetCheck(GetDlgItem(hwndDlg, IDC_STARTUP_CHECK)) == BST_CHECKED;
//                    context->SetupCreateMinimizedSystemStartup = Button_GetCheck(GetDlgItem(hwndDlg, IDC_STARTMIN_CHECK)) == BST_CHECKED;
//                    context->SetupInstallDebuggingTools = Button_GetCheck(GetDlgItem(hwndDlg, IDC_DBGTOOLS_CHECK)) == BST_CHECKED;
//                    context->SetupInstallPeViewAssociations = Button_GetCheck(GetDlgItem(hwndDlg, IDC_PEVIEW_CHECK)) == BST_CHECKED;
//                    context->SetupInstallKphService = Button_GetCheck(GetDlgItem(hwndDlg, IDC_KPH_CHECK)) == BST_CHECKED;
//                    context->SetupResetSettings = Button_GetCheck(GetDlgItem(hwndDlg, IDC_RESET_CHECK)) == BST_CHECKED;
//                    context->SetupStartAppAfterExit = Button_GetCheck(GetDlgItem(hwndDlg, IDC_PHSTART_CHECK)) == BST_CHECKED;
//
//                    SetupInstallPath = PhGetWindowText(GetDlgItem(hwndDlg, IDC_INSTALL_DIRECTORY));
//
//                    if (PhIsNullOrEmptyString(SetupInstallPath))
//                        SetupInstallPath = SetupFindInstallDirectory();
//#ifdef PH_BUILD_API
//                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)IDD_DIALOG4);
//                    return TRUE;
//#endif
//                }
//                break;
//            case PSN_QUERYINITIALFOCUS:
//                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)GetDlgItem(hwndDlg, IDC_FOLDER_BROWSE));
//                return TRUE;
//            }
//        }
//        break;
//    }
//
//    return FALSE;
//}
