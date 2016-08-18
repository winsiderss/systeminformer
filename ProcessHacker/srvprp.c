/*
 * Process Hacker -
 *   service properties
 *
 * Copyright (C) 2010-2015 wj32
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

#include <secedit.h>
#include <svcsup.h>

#include <actions.h>
#include <phplug.h>
#include <phsvccl.h>
#include <srvprv.h>

typedef struct _SERVICE_PROPERTIES_CONTEXT
{
    PPH_SERVICE_ITEM ServiceItem;
    BOOLEAN Ready;
    BOOLEAN Dirty;

    BOOLEAN OldDelayedStart;
} SERVICE_PROPERTIES_CONTEXT, *PSERVICE_PROPERTIES_CONTEXT;

INT_PTR CALLBACK PhpServiceGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

static NTSTATUS PhpOpenService(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    SC_HANDLE serviceHandle;

    if (!(serviceHandle = PhOpenService(
        ((PPH_SERVICE_ITEM)Context)->Name->Buffer,
        DesiredAccess
        )))
        return PhGetLastWin32ErrorAsNtStatus();

    *Handle = serviceHandle;

    return STATUS_SUCCESS;
}

static _Callback_ NTSTATUS PhpSetServiceSecurity(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PPH_STD_OBJECT_SECURITY stdObjectSecurity;

    stdObjectSecurity = (PPH_STD_OBJECT_SECURITY)Context;

    status = PhStdSetObjectSecurity(SecurityDescriptor, SecurityInformation, Context);

    if ((status == STATUS_ACCESS_DENIED || status == NTSTATUS_FROM_WIN32(ERROR_ACCESS_DENIED)) && !PhGetOwnTokenAttributes().Elevated)
    {
        // Elevate using phsvc.
        if (PhUiConnectToPhSvc(NULL, FALSE))
        {
            status = PhSvcCallSetServiceSecurity(
                ((PPH_SERVICE_ITEM)stdObjectSecurity->Context)->Name->Buffer,
                SecurityInformation,
                SecurityDescriptor
                );
            PhUiDisconnectFromPhSvc();
        }
    }

    return status;
}

VOID PhShowServiceProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[32];
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

    memset(&context, 0, sizeof(SERVICE_PROPERTIES_CONTEXT));
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
            PhpSetServiceSecurity,
            &stdObjectSecurity,
            accessEntries,
            numberOfAccessEntries
            );
        PhFree(accessEntries);
    }

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_OBJECT_PROPERTIES objectProperties;

        objectProperties.Parameter = ServiceItem;
        objectProperties.NumberOfPages = propSheetHeader.nPages;
        objectProperties.MaximumNumberOfPages = sizeof(pages) / sizeof(HPROPSHEETPAGE);
        objectProperties.Pages = pages;

        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackServicePropertiesInitializing), &objectProperties);

        propSheetHeader.nPages = objectProperties.NumberOfPages;
    }

    PhModalPropertySheet(&propSheetHeader);
}

static VOID PhpRefreshControls(
    _In_ HWND hwndDlg
    )
{
    if (
        WindowsVersion >= WINDOWS_VISTA &&
        PhEqualString2(PhaGetDlgItemText(hwndDlg, IDC_STARTTYPE), L"Auto start", FALSE)
        )
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_DELAYEDSTART), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_DELAYEDSTART), FALSE);
    }
}

INT_PTR CALLBACK PhpServiceGeneralDlgProc(
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
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PSERVICE_PROPERTIES_CONTEXT context = (PSERVICE_PROPERTIES_CONTEXT)propSheetPage->lParam;
            PPH_SERVICE_ITEM serviceItem = context->ServiceItem;
            SC_HANDLE serviceHandle;
            ULONG startType;
            ULONG errorControl;
            PPH_STRING serviceDll;

            // HACK
            PhCenterWindow(GetParent(hwndDlg), GetParent(GetParent(hwndDlg)));

            SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)context);

            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_TYPE), PhServiceTypeStrings,
                sizeof(PhServiceTypeStrings) / sizeof(WCHAR *));
            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_STARTTYPE), PhServiceStartTypeStrings,
                sizeof(PhServiceStartTypeStrings) / sizeof(WCHAR *));
            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_ERRORCONTROL), PhServiceErrorControlStrings,
                sizeof(PhServiceErrorControlStrings) / sizeof(WCHAR *));

            SetDlgItemText(hwndDlg, IDC_DESCRIPTION, serviceItem->DisplayName->Buffer);
            PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_TYPE),
                PhGetServiceTypeString(serviceItem->Type), FALSE);

            startType = serviceItem->StartType;
            errorControl = serviceItem->ErrorControl;
            serviceHandle = PhOpenService(serviceItem->Name->Buffer, SERVICE_QUERY_CONFIG);

            if (serviceHandle)
            {
                LPQUERY_SERVICE_CONFIG config;
                PPH_STRING description;
                BOOLEAN delayedStart;

                if (config = PhGetServiceConfig(serviceHandle))
                {
                    SetDlgItemText(hwndDlg, IDC_GROUP, config->lpLoadOrderGroup);
                    SetDlgItemText(hwndDlg, IDC_BINARYPATH, config->lpBinaryPathName);
                    SetDlgItemText(hwndDlg, IDC_USERACCOUNT, config->lpServiceStartName);

                    if (startType != config->dwStartType || errorControl != config->dwErrorControl)
                    {
                        startType = config->dwStartType;
                        errorControl = config->dwErrorControl;
                        PhMarkNeedsConfigUpdateServiceItem(serviceItem);
                    }

                    PhFree(config);
                }

                if (description = PhGetServiceDescription(serviceHandle))
                {
                    SetDlgItemText(hwndDlg, IDC_DESCRIPTION, description->Buffer);
                    PhDereferenceObject(description);
                }

                if (
                    WindowsVersion >= WINDOWS_VISTA &&
                    PhGetServiceDelayedAutoStart(serviceHandle, &delayedStart)
                    )
                {
                    context->OldDelayedStart = delayedStart;

                    if (delayedStart)
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_DELAYEDSTART), BST_CHECKED);
                }

                CloseServiceHandle(serviceHandle);
            }

            PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_STARTTYPE),
                PhGetServiceStartTypeString(startType), FALSE);
            PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_ERRORCONTROL),
                PhGetServiceErrorControlString(errorControl), FALSE);

            SetDlgItemText(hwndDlg, IDC_PASSWORD, L"password");
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_PASSWORDCHECK), BST_UNCHECKED);

            if (NT_SUCCESS(PhGetServiceDllParameter(&serviceItem->Name->sr, &serviceDll)))
            {
                SetDlgItemText(hwndDlg, IDC_SERVICEDLL, serviceDll->Buffer);
                PhDereferenceObject(serviceDll);
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_SERVICEDLL, L"N/A");
            }

            PhpRefreshControls(hwndDlg);

            context->Ready = TRUE;
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, PhMakeContextAtom());
        }
        break;
    case WM_COMMAND:
        {
            PSERVICE_PROPERTIES_CONTEXT context =
                (PSERVICE_PROPERTIES_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());

            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                {
                    // Workaround for property sheet + multiline edit: http://support.microsoft.com/kb/130765

                    SendMessage(GetParent(hwndDlg), uMsg, wParam, lParam);
                }
                break;
            case IDC_PASSWORD:
                {
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_PASSWORDCHECK), BST_CHECKED);
                    }
                }
                break;
            case IDC_DELAYEDSTART:
                {
                    context->Dirty = TRUE;
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
                    PPH_STRING commandLine;
                    PPH_STRING fileName;

                    fileDialog = PhCreateOpenFileDialog();
                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

                    commandLine = PhaGetDlgItemText(hwndDlg, IDC_BINARYPATH);

                    if (context->ServiceItem->Type & SERVICE_WIN32)
                    {
                        PH_STRINGREF dummyFileName;
                        PH_STRINGREF dummyArguments;

                        if (!PhParseCommandLineFuzzy(&commandLine->sr, &dummyFileName, &dummyArguments, &fileName))
                            fileName = NULL;

                        if (!fileName)
                            PhSwapReference(&fileName, commandLine);
                    }
                    else
                    {
                        fileName = PhGetFileName(commandLine);
                    }

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

            switch (HIWORD(wParam))
            {
            case EN_CHANGE:
            case CBN_SELCHANGE:
                {
                    PhpRefreshControls(hwndDlg);

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
            case PSN_QUERYINITIALFOCUS:
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)GetDlgItem(hwndDlg, IDC_STARTTYPE));
                }
                return TRUE;
            case PSN_KILLACTIVE:
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
                }
                return TRUE;
            case PSN_APPLY:
                {
                    NTSTATUS status;
                    PSERVICE_PROPERTIES_CONTEXT context =
                        (PSERVICE_PROPERTIES_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());
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

                    newServiceTypeString = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_TYPE)));
                    newServiceStartTypeString = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_STARTTYPE)));
                    newServiceErrorControlString = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_ERRORCONTROL)));
                    newServiceType = PhGetServiceTypeInteger(newServiceTypeString->Buffer);
                    newServiceStartType = PhGetServiceStartTypeInteger(newServiceStartTypeString->Buffer);
                    newServiceErrorControl = PhGetServiceErrorControlInteger(newServiceErrorControlString->Buffer);

                    newServiceGroup = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_GROUP)));
                    newServiceBinaryPath = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_BINARYPATH)));
                    newServiceUserAccount = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_USERACCOUNT)));

                    if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_PASSWORDCHECK)) == BST_CHECKED)
                    {
                        newServicePassword = PhGetWindowText(GetDlgItem(hwndDlg, IDC_PASSWORD));
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
                            if (WindowsVersion >= WINDOWS_VISTA)
                            {
                                BOOLEAN newDelayedStart;

                                newDelayedStart = Button_GetCheck(GetDlgItem(hwndDlg, IDC_DELAYEDSTART)) == BST_CHECKED;

                                if (newDelayedStart != context->OldDelayedStart)
                                {
                                    PhSetServiceDelayedAutoStart(serviceHandle, newDelayedStart);
                                }
                            }

                            PhMarkNeedsConfigUpdateServiceItem(serviceItem);

                            CloseServiceHandle(serviceHandle);
                        }
                        else
                        {
                            CloseServiceHandle(serviceHandle);
                            goto ErrorCase;
                        }
                    }
                    else
                    {
                        if (GetLastError() == ERROR_ACCESS_DENIED && !PhGetOwnTokenAttributes().Elevated)
                        {
                            // Elevate using phsvc.
                            if (PhUiConnectToPhSvc(hwndDlg, FALSE))
                            {
                                if (NT_SUCCESS(status = PhSvcCallChangeServiceConfig(
                                    serviceItem->Name->Buffer,
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
                                    )))
                                {
                                    if (WindowsVersion >= WINDOWS_VISTA)
                                    {
                                        BOOLEAN newDelayedStart;

                                        newDelayedStart = Button_GetCheck(GetDlgItem(hwndDlg, IDC_DELAYEDSTART)) == BST_CHECKED;

                                        if (newDelayedStart != context->OldDelayedStart)
                                        {
                                            SERVICE_DELAYED_AUTO_START_INFO info;

                                            info.fDelayedAutostart = newDelayedStart;
                                            PhSvcCallChangeServiceConfig2(
                                                serviceItem->Name->Buffer,
                                                SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
                                                &info
                                                );
                                        }
                                    }

                                    PhMarkNeedsConfigUpdateServiceItem(serviceItem);
                                }

                                PhUiDisconnectFromPhSvc();

                                if (!NT_SUCCESS(status))
                                {
                                    SetLastError(PhNtStatusToDosError(status));
                                    goto ErrorCase;
                                }
                            }
                            else
                            {
                                // User cancelled elevation.
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                            }
                        }
                        else
                        {
                            goto ErrorCase;
                        }
                    }

                    goto Cleanup;
ErrorCase:
                    if (PhShowMessage(
                        hwndDlg,
                        MB_ICONERROR | MB_RETRYCANCEL,
                        L"Unable to change service configuration: %s",
                        PH_AUTO_T(PH_STRING, PhGetWin32Message(GetLastError()))->Buffer
                        ) == IDRETRY)
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                    }

Cleanup:
                    if (newServicePassword)
                    {
                        RtlSecureZeroMemory(newServicePassword->Buffer, newServicePassword->Length);
                        PhDereferenceObject(newServicePassword);
                    }
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}
