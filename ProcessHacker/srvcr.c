/*
 * Process Hacker -
 *   service creation dialog
 *
 * Copyright (C) 2010-2013 wj32
 * Copyright (C) 2019 dmex
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
#include <phsettings.h>
#include <svcsup.h>

#include <actions.h>
#include <phsvccl.h>

INT_PTR CALLBACK PhpCreateServiceDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhShowCreateServiceDialog(
    _In_ HWND ParentWindowHandle
    )
{
    DialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_CREATESERVICE),
        PhCsForceNoParent ? NULL : ParentWindowHandle,
        PhpCreateServiceDlgProc
        );
}

INT_PTR CALLBACK PhpCreateServiceDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhSetApplicationWindowIcon(hwndDlg);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_TYPE), PhServiceTypeStrings, RTL_NUMBER_OF(PhServiceTypeStrings));
            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_STARTTYPE), PhServiceStartTypeStrings, RTL_NUMBER_OF(PhServiceStartTypeStrings));
            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_ERRORCONTROL), PhServiceErrorControlStrings, RTL_NUMBER_OF(PhServiceErrorControlStrings));

            PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_TYPE), L"Own Process", FALSE);
            PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_STARTTYPE), L"Demand Start", FALSE);
            PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_ERRORCONTROL), L"Ignore", FALSE);

            if (!PhGetOwnTokenAttributes().Elevated)
            {
                Button_SetElevationRequiredState(GetDlgItem(hwndDlg, IDOK), TRUE);
            }

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDC_NAME));
            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                {
                    EndDialog(hwndDlg, IDCANCEL);
                }
                break;
            case IDOK:
                {
                    NTSTATUS status = STATUS_SUCCESS;
                    BOOLEAN success = FALSE;
                    SC_HANDLE scManagerHandle;
                    SC_HANDLE serviceHandle;
                    ULONG win32Result = ERROR_SUCCESS;
                    PPH_STRING serviceName;
                    PPH_STRING serviceDisplayName;
                    PPH_STRING serviceTypeString;
                    PPH_STRING serviceStartTypeString;
                    PPH_STRING serviceErrorControlString;
                    ULONG serviceType;
                    ULONG serviceStartType;
                    ULONG serviceErrorControl;
                    PPH_STRING serviceBinaryPath;

                    serviceName = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_NAME)));
                    serviceDisplayName = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_DISPLAYNAME)));

                    serviceTypeString = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_TYPE)));
                    serviceStartTypeString = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_STARTTYPE)));
                    serviceErrorControlString = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_ERRORCONTROL)));
                    serviceType = PhGetServiceTypeInteger(serviceTypeString->Buffer);
                    serviceStartType = PhGetServiceStartTypeInteger(serviceStartTypeString->Buffer);
                    serviceErrorControl = PhGetServiceErrorControlInteger(serviceErrorControlString->Buffer);

                    serviceBinaryPath = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_BINARYPATH)));

                    if (PhGetOwnTokenAttributes().Elevated)
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

                    fileName = PhGetFileName(PhaGetDlgItemText(hwndDlg, IDC_BINARYPATH));
                    PhSetFileDialogFileName(fileDialog, fileName->Buffer);
                    PhDereferenceObject(fileName);

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        fileName = PhGetFileDialogFileName(fileDialog);
                        PhSetDialogItemText(hwndDlg, IDC_BINARYPATH, fileName->Buffer);
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
