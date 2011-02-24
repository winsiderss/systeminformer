/*
 * Process Hacker - 
 *   service creation dialog
 * 
 * Copyright (C) 2010 wj32
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

#include <phapp.h>
#include <phsvccl.h>
#include <windowsx.h>

INT_PTR CALLBACK PhpCreateServiceDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowCreateServiceDialog(
    __in HWND ParentWindowHandle
    )
{
    DialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_CREATESERVICE),
        ParentWindowHandle,
        PhpCreateServiceDlgProc
        );
}

INT_PTR CALLBACK PhpCreateServiceDlgProc(      
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
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_TYPE), PhServiceTypeStrings,
                sizeof(PhServiceTypeStrings) / sizeof(WCHAR *));
            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_STARTTYPE), PhServiceStartTypeStrings,
                sizeof(PhServiceStartTypeStrings) / sizeof(WCHAR *));
            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_ERRORCONTROL), PhServiceErrorControlStrings,
                sizeof(PhServiceErrorControlStrings) / sizeof(WCHAR *));

            ComboBox_SelectString(GetDlgItem(hwndDlg, IDC_TYPE), -1, L"Own Process");
            ComboBox_SelectString(GetDlgItem(hwndDlg, IDC_STARTTYPE), -1, L"Demand Start");
            ComboBox_SelectString(GetDlgItem(hwndDlg, IDC_ERRORCONTROL), -1, L"Ignore");

            if (!PhElevated)
            {
                SendMessage(GetDlgItem(hwndDlg, IDOK), BCM_SETSHIELD, 0, TRUE);
            }

            SetFocus(GetDlgItem(hwndDlg, IDC_NAME));
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                {
                    EndDialog(hwndDlg, IDCANCEL);
                }
                break;
            case IDOK:
                {
                    NTSTATUS status = 0;
                    BOOLEAN success = FALSE;
                    SC_HANDLE scManagerHandle;
                    SC_HANDLE serviceHandle;
                    ULONG win32Result = 0;
                    PPH_STRING serviceName;
                    PPH_STRING serviceDisplayName;
                    PPH_STRING serviceTypeString;
                    PPH_STRING serviceStartTypeString;
                    PPH_STRING serviceErrorControlString;
                    ULONG serviceType;
                    ULONG serviceStartType;
                    ULONG serviceErrorControl;
                    PPH_STRING serviceBinaryPath;

                    serviceName = PHA_DEREFERENCE(PhGetWindowText(GetDlgItem(hwndDlg, IDC_NAME)));
                    serviceDisplayName = PHA_DEREFERENCE(PhGetWindowText(GetDlgItem(hwndDlg, IDC_DISPLAYNAME)));

                    serviceTypeString = PHA_DEREFERENCE(PhGetWindowText(GetDlgItem(hwndDlg, IDC_TYPE)));
                    serviceStartTypeString = PHA_DEREFERENCE(PhGetWindowText(GetDlgItem(hwndDlg, IDC_STARTTYPE)));
                    serviceErrorControlString = PHA_DEREFERENCE(PhGetWindowText(GetDlgItem(hwndDlg, IDC_ERRORCONTROL)));
                    serviceType = PhGetServiceTypeInteger(serviceTypeString->Buffer);
                    serviceStartType = PhGetServiceStartTypeInteger(serviceStartTypeString->Buffer);
                    serviceErrorControl = PhGetServiceErrorControlInteger(serviceErrorControlString->Buffer);

                    serviceBinaryPath = PHA_DEREFERENCE(PhGetWindowText(GetDlgItem(hwndDlg, IDC_BINARYPATH)));

                    if (PhElevated)
                    {
                        if (scManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE))
                        {
                            if (serviceHandle = CreateService(
                                scManagerHandle,
                                serviceName->Buffer,
                                serviceDisplayName->Buffer,
                                SERVICE_CHANGE_CONFIG,
                                serviceType,
                                serviceStartType,
                                serviceErrorControl,
                                serviceBinaryPath->Buffer,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                L""
                                ))
                            {
                                EndDialog(hwndDlg, IDOK);
                                CloseServiceHandle(serviceHandle);
                                success = TRUE;
                            }
                            else
                            {
                                win32Result = GetLastError();
                            }

                            CloseServiceHandle(scManagerHandle);
                        }
                        else
                        {
                            win32Result = GetLastError();
                        }
                    }
                    else
                    {
                        if (PhUiConnectToPhSvc(hwndDlg, FALSE))
                        {
                            status = PhSvcCallCreateService(
                                serviceName->Buffer,
                                serviceDisplayName->Buffer,
                                serviceType,
                                serviceStartType,
                                serviceErrorControl,
                                serviceBinaryPath->Buffer,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                L""
                                );
                            PhUiDisconnectFromPhSvc();

                            if (NT_SUCCESS(status))
                            {
                                EndDialog(hwndDlg, IDOK);
                                success = TRUE;
                            }
                        }
                        else
                        {
                            // User cancelled elevation.
                            success = TRUE;
                        }
                    }

                    if (!success)
                        PhShowStatus(hwndDlg, L"Unable to create the service", status, win32Result);
                }
                break;
            case IDC_BROWSE:
                {
                    static PH_FILETYPE_FILTER filters[] =
                    {
                        { L"Executable files (*.exe;*.sys)", L"*.exe;*.sys" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog;
                    PPH_STRING fileName;

                    fileDialog = PhCreateOpenFileDialog();
                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

                    fileName = PhGetFileName(PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_BINARYPATH));
                    PhSetFileDialogFileName(fileDialog, fileName->Buffer);
                    PhDereferenceObject(fileName);

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        fileName = PhGetFileDialogFileName(fileDialog);
                        SetDlgItemText(hwndDlg, IDC_BINARYPATH, fileName->Buffer);
                        PhDereferenceObject(fileName);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
