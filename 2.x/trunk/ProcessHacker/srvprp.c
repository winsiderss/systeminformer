/*
 * Process Hacker - 
 *   service properties
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
#include <windowsx.h>

typedef struct _SERVICE_PROPERTIES_CONTEXT
{
    PPH_SERVICE_ITEM ServiceItem;
    BOOLEAN Ready;
    BOOLEAN Dirty;
} SERVICE_PROPERTIES_CONTEXT, *PSERVICE_PROPERTIES_CONTEXT;

INT_PTR CALLBACK PhpServiceGeneralDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

static NTSTATUS PhpOpenService(
    __out PHANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in PVOID Context
    )
{
    SC_HANDLE serviceHandle;

    if (!(serviceHandle = PhOpenService(
        ((PPH_SERVICE_ITEM)Context)->Name->Buffer,
        DesiredAccess
        )))
        return NTSTATUS_FROM_WIN32(GetLastError());

    *Handle = serviceHandle;

    return STATUS_SUCCESS;
}

VOID PhShowServiceProperties(
    __in HWND ParentWindowHandle,
    __in PPH_SERVICE_ITEM ServiceItem
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[2];
    SERVICE_PROPERTIES_CONTEXT context;
    PH_STD_OBJECT_SECURITY stdObjectSecurity;
    PPH_ACCESS_ENTRY accessEntries;
    ULONG numberOfAccessEntries;

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.pszCaption = ServiceItem->Name->Buffer;
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    // General

    context.ServiceItem = ServiceItem;
    context.Ready = FALSE;
    context.Dirty = FALSE;

    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_SRVGENERAL);
    propSheetPage.pfnDlgProc = PhpServiceGeneralDlgProc;
    propSheetPage.lParam = (LPARAM)&context;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // Security

    stdObjectSecurity.OpenObject = PhpOpenService;
    stdObjectSecurity.ObjectType = L"Service";
    stdObjectSecurity.Context = ServiceItem;

    if (PhGetAccessEntries(L"Service", &accessEntries, &numberOfAccessEntries))
    {
        pages[propSheetHeader.nPages++] = PhCreateSecurityPage(
            ServiceItem->Name->Buffer,
            PhStdGetObjectSecurity,
            PhStdSetObjectSecurity,
            &stdObjectSecurity,
            accessEntries,
            numberOfAccessEntries
            );
        PhFree(accessEntries);
    }

    PropertySheet(&propSheetHeader);
}

INT_PTR CALLBACK PhpServiceGeneralDlgProc(      
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
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PSERVICE_PROPERTIES_CONTEXT context = (PSERVICE_PROPERTIES_CONTEXT)propSheetPage->lParam;
            PPH_SERVICE_ITEM serviceItem = context->ServiceItem;
            SC_HANDLE serviceHandle;

            // HACK
            PhCenterWindow(GetParent(hwndDlg), GetParent(GetParent(hwndDlg)));

            SetProp(hwndDlg, L"Context", (HANDLE)context);

            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_TYPE), PhServiceTypeStrings,
                sizeof(PhServiceTypeStrings) / sizeof(WCHAR *));
            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_STARTTYPE), PhServiceStartTypeStrings,
                sizeof(PhServiceStartTypeStrings) / sizeof(WCHAR *));
            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_ERRORCONTROL), PhServiceErrorControlStrings,
                sizeof(PhServiceErrorControlStrings) / sizeof(WCHAR *));

            SetDlgItemText(hwndDlg, IDC_DESCRIPTION, serviceItem->DisplayName->Buffer);
            ComboBox_SelectString(GetDlgItem(hwndDlg, IDC_TYPE), -1,
                PhGetServiceTypeString(serviceItem->Type));
            ComboBox_SelectString(GetDlgItem(hwndDlg, IDC_STARTTYPE), -1,
                PhGetServiceStartTypeString(serviceItem->StartType));
            ComboBox_SelectString(GetDlgItem(hwndDlg, IDC_ERRORCONTROL), -1,
                PhGetServiceErrorControlString(serviceItem->ErrorControl));

            serviceHandle = PhOpenService(serviceItem->Name->Buffer, SERVICE_QUERY_CONFIG);

            if (serviceHandle)
            {
                LPQUERY_SERVICE_CONFIG config;
                PPH_STRING description;

                if (config = PhGetServiceConfig(serviceHandle))
                {
                    SetDlgItemText(hwndDlg, IDC_GROUP, config->lpLoadOrderGroup);
                    SetDlgItemText(hwndDlg, IDC_BINARYPATH, config->lpBinaryPathName);
                    SetDlgItemText(hwndDlg, IDC_USERACCOUNT, config->lpServiceStartName);

                    PhFree(config);
                }

                if (description = PhGetServiceDescription(serviceHandle))
                {
                    SetDlgItemText(hwndDlg, IDC_DESCRIPTION, description->Buffer);
                    PhDereferenceObject(description);
                }

                CloseServiceHandle(serviceHandle);
            }

            SetDlgItemText(hwndDlg, IDC_PASSWORD, L"password");
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_PASSWORDCHECK), BST_UNCHECKED);

            SetDlgItemText(hwndDlg, IDC_SERVICEDLL, L"N/A");

            {
                HKEY keyHandle;
                PPH_STRING keyName;

                keyName = PhConcatStrings(
                    3,
                    L"SYSTEM\\CurrentControlSet\\Services\\",
                    serviceItem->Name->Buffer,
                    L"\\Parameters"
                    );

                if (RegOpenKey(
                    HKEY_LOCAL_MACHINE,
                    keyName->Buffer,
                    &keyHandle
                    ) == ERROR_SUCCESS)
                {
                    PPH_STRING serviceDllString;

                    if (serviceDllString = PhQueryRegistryString(keyHandle, L"ServiceDll"))
                    {
                        PPH_STRING expandedString;

                        if (expandedString = PhExpandEnvironmentStrings(serviceDllString->Buffer))
                        {
                            SetDlgItemText(hwndDlg, IDC_SERVICEDLL, expandedString->Buffer);
                            PhDereferenceObject(expandedString);
                        }

                        PhDereferenceObject(serviceDllString);
                    }

                    RegCloseKey(keyHandle);
                }

                PhDereferenceObject(keyName);
            }

            context->Ready = TRUE;
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, L"Context");
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_PASSWORD:
                {
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_PASSWORDCHECK), BST_CHECKED);
                    }
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

                    if (PhShowFileDialog(NULL, fileDialog))
                    {
                        fileName = PhGetFileDialogFileName(fileDialog);
                        SetDlgItemText(hwndDlg, IDC_BINARYPATH, fileName->Buffer);
                        PhDereferenceObject(fileName);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            }

            switch (HIWORD(wParam))
            {
            case EN_CHANGE:
            case CBN_SELCHANGE:
                {
                    PSERVICE_PROPERTIES_CONTEXT context = 
                        (PSERVICE_PROPERTIES_CONTEXT)GetProp(hwndDlg, L"Context");

                    if (context->Ready)
                        context->Dirty = TRUE;
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_KILLACTIVE:
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
                }
                return TRUE;
            case PSN_APPLY:
                {
                    PSERVICE_PROPERTIES_CONTEXT context = 
                        (PSERVICE_PROPERTIES_CONTEXT)GetProp(hwndDlg, L"Context");
                    PPH_SERVICE_ITEM serviceItem = context->ServiceItem;
                    SC_HANDLE serviceHandle;
                    PPH_STRING newServiceTypeString;
                    PPH_STRING newServiceStartTypeString;
                    PPH_STRING newServiceErrorControlString;
                    ULONG newServiceType;
                    ULONG newServiceStartType;
                    ULONG newServiceErrorControl;
                    PPH_STRING newServiceGroup;
                    PPH_STRING newServiceBinaryPath;
                    PPH_STRING newServiceUserAccount;
                    PPH_STRING newServicePassword;

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

                    if (!context->Dirty)
                    {
                        return TRUE;
                    }

                    newServiceTypeString = PHA_DEREFERENCE(PhGetWindowText(GetDlgItem(hwndDlg, IDC_TYPE)));
                    newServiceStartTypeString = PHA_DEREFERENCE(PhGetWindowText(GetDlgItem(hwndDlg, IDC_STARTTYPE)));
                    newServiceErrorControlString = PHA_DEREFERENCE(PhGetWindowText(GetDlgItem(hwndDlg, IDC_ERRORCONTROL)));
                    newServiceType = PhGetServiceTypeInteger(newServiceTypeString->Buffer);
                    newServiceStartType = PhGetServiceStartTypeInteger(newServiceStartTypeString->Buffer);
                    newServiceErrorControl = PhGetServiceErrorControlInteger(newServiceErrorControlString->Buffer);

                    newServiceGroup = PHA_DEREFERENCE(PhGetWindowText(GetDlgItem(hwndDlg, IDC_GROUP)));
                    newServiceBinaryPath = PHA_DEREFERENCE(PhGetWindowText(GetDlgItem(hwndDlg, IDC_BINARYPATH)));
                    newServiceUserAccount = PHA_DEREFERENCE(PhGetWindowText(GetDlgItem(hwndDlg, IDC_USERACCOUNT)));

                    if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_PASSWORDCHECK)) == BST_CHECKED)
                    {
                        newServicePassword = PHA_DEREFERENCE(PhGetWindowText(GetDlgItem(hwndDlg, IDC_PASSWORD)));
                    }
                    else
                    {
                        newServicePassword = NULL;
                    }

                    if (newServiceType == SERVICE_KERNEL_DRIVER && newServiceUserAccount->Length == 0)
                    {
                        newServiceUserAccount = NULL;
                    }

                    serviceHandle = PhOpenService(serviceItem->Name->Buffer, SERVICE_CHANGE_CONFIG);

                    if (serviceHandle)
                    {
                        if (ChangeServiceConfig(
                            serviceHandle,
                            newServiceType,
                            newServiceStartType,
                            newServiceErrorControl,
                            newServiceBinaryPath->Buffer,
                            newServiceGroup->Buffer,
                            NULL,
                            NULL,
                            PhGetString(newServiceUserAccount),
                            PhGetString(newServicePassword),
                            NULL
                            ))
                        {
                            PhMarkNeedsConfigUpdateServiceItem(serviceItem);
                        }
                        else
                        {
                            PhShowStatus(hwndDlg, L"Unable to change service configuration",
                                0, GetLastError());
                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                        }

                        CloseServiceHandle(serviceHandle);
                    }
                    else
                    {
                        PhShowStatus(hwndDlg, L"Unable to change service configuration", 0, GetLastError());
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                    }
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}
