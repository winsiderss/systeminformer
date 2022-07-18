/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2017-2021
 *
 */

#include <phapp.h>

#include <secedit.h>
#include <svcsup.h>
#include <mainwnd.h>
#include <settings.h>

#include <actions.h>
#include <phplug.h>
#include <phsvccl.h>
#include <phsettings.h>
#include <procprv.h>
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
    PPH_SERVICE_ITEM serviceItem;

    if (!(serviceItem = ((PPH_SERVICE_ITEM)Context)))
        return STATUS_UNSUCCESSFUL;

    if (serviceHandle = PhOpenService(
        serviceItem->Name->Buffer,
        DesiredAccess
        ))
    {
        *Handle = serviceHandle;
        return STATUS_SUCCESS;
    }

    *Handle = NULL;
    return PhGetLastWin32ErrorAsNtStatus();
}

NTSTATUS PhpCloseServiceCallback(
    _In_opt_ PVOID Context
    )
{
    PPH_SERVICE_ITEM serviceItem;

    if (!(serviceItem = ((PPH_SERVICE_ITEM)Context)))
        return STATUS_UNSUCCESSFUL;

    PhDereferenceObject(serviceItem);

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

    if (!(stdObjectSecurity = (PPH_STD_OBJECT_SECURITY)Context))
        return STATUS_UNSUCCESSFUL;

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

NTSTATUS PhpShowServicePropertiesThread(
    _In_opt_ PVOID Context
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[32];
    SERVICE_PROPERTIES_CONTEXT context;
    PPH_SERVICE_ITEM serviceItem;

    if (!(serviceItem = ((PPH_SERVICE_ITEM)Context)))
        return STATUS_UNSUCCESSFUL;

    propSheetHeader.dwFlags =
        PSH_MODELESS |
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE |
        //PSH_USECALLBACK |
        PSH_USEHICON;
    propSheetHeader.hInstance = PhInstanceHandle;
    //propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.pszCaption = PhGetString(serviceItem->Name);
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;
 
    {
        if (serviceItem->IconEntry && serviceItem->IconEntry->SmallIconIndex)
        {
            propSheetHeader.hIcon = PhGetImageListIcon(serviceItem->IconEntry->SmallIconIndex, FALSE);
        }
        else
        {
            if (serviceItem->Type == SERVICE_KERNEL_DRIVER || serviceItem->Type == SERVICE_FILE_SYSTEM_DRIVER)
            {
                propSheetHeader.hIcon = PhGetImageListIcon(1, FALSE); // ServiceCogIcon;
            }
            else
            {
                propSheetHeader.hIcon = PhGetImageListIcon(0, FALSE); // ServiceApplicationIcon;
            }
        }
    }

    // General

    memset(&context, 0, sizeof(SERVICE_PROPERTIES_CONTEXT));
    context.ServiceItem = serviceItem;
    context.Ready = FALSE;
    context.Dirty = FALSE;

    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_SRVGENERAL);
    propSheetPage.hInstance = PhInstanceHandle;
    propSheetPage.pfnDlgProc = PhpServiceGeneralDlgProc;
    propSheetPage.lParam = (LPARAM)&context;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_OBJECT_PROPERTIES objectProperties;

        objectProperties.Parameter = serviceItem;
        objectProperties.NumberOfPages = propSheetHeader.nPages;
        objectProperties.MaximumNumberOfPages = sizeof(pages) / sizeof(HPROPSHEETPAGE);
        objectProperties.Pages = pages;

        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackServicePropertiesInitializing), &objectProperties);

        propSheetHeader.nPages = objectProperties.NumberOfPages;
    }

    PhModalPropertySheet(&propSheetHeader);

    PhDereferenceObject(serviceItem);

    return STATUS_SUCCESS;
}

VOID PhShowServiceProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    PhReferenceObject(ServiceItem);
    PhCreateThread2(PhpShowServicePropertiesThread, ServiceItem);
}

static VOID PhpRefreshControls(
    _In_ HWND hwndDlg
    )
{
    if (PhEqualString2(PhaGetDlgItemText(hwndDlg, IDC_STARTTYPE), L"Auto start", FALSE))
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_DELAYEDSTART), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_DELAYEDSTART), FALSE);
    }

    if (PhEqualString2(PhaGetDlgItemText(hwndDlg, IDC_TYPE), L"Driver", FALSE) ||
        PhEqualString2(PhaGetDlgItemText(hwndDlg, IDC_TYPE), L"FS driver", FALSE))
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_USERACCOUNT), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORD), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORDCHECK), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SERVICEDLL), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_USERACCOUNT), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORD), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_PASSWORDCHECK), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SERVICEDLL), TRUE);
    }
}

INT_PTR CALLBACK PhpServiceGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PSERVICE_PROPERTIES_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
        context = (PSERVICE_PROPERTIES_CONTEXT)propSheetPage->lParam;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_SERVICE_ITEM serviceItem = context->ServiceItem;
            SC_HANDLE serviceHandle;
            ULONG startType;
            ULONG errorControl;
            PPH_STRING serviceDll;

            // HACK
            if (PhGetIntegerPairSetting(L"ServiceWindowPosition").X == 0)
                PhCenterWindow(GetParent(hwndDlg), PhMainWndHandle);
            else
                PhLoadWindowPlacementFromSetting(L"ServiceWindowPosition", NULL, GetParent(hwndDlg));

            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_TYPE), PhServiceTypeStrings, RTL_NUMBER_OF(PhServiceTypeStrings));
            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_STARTTYPE), PhServiceStartTypeStrings, RTL_NUMBER_OF(PhServiceStartTypeStrings));
            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_ERRORCONTROL), PhServiceErrorControlStrings, RTL_NUMBER_OF(PhServiceErrorControlStrings));

            PhSetDialogItemText(hwndDlg, IDC_DESCRIPTION, PhGetStringOrEmpty(serviceItem->DisplayName));
            PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_TYPE), PhGetServiceTypeString(serviceItem->Type)->Buffer, FALSE);

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
                    PhSetDialogItemText(hwndDlg, IDC_GROUP, config->lpLoadOrderGroup);
                    PhSetDialogItemText(hwndDlg, IDC_BINARYPATH, config->lpBinaryPathName);
                    PhSetDialogItemText(hwndDlg, IDC_USERACCOUNT, config->lpServiceStartName);

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
                    PhSetDialogItemText(hwndDlg, IDC_DESCRIPTION, description->Buffer);
                    PhDereferenceObject(description);
                }

                if (PhGetServiceDelayedAutoStart(serviceHandle, &delayedStart))
                {
                    context->OldDelayedStart = delayedStart;

                    if (delayedStart)
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_DELAYEDSTART), BST_CHECKED);
                }

                CloseServiceHandle(serviceHandle);
            }

            PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_STARTTYPE), PhGetServiceStartTypeString(startType)->Buffer, FALSE);
            PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_ERRORCONTROL), PhGetServiceErrorControlString(errorControl)->Buffer, FALSE);

            PhSetDialogItemText(hwndDlg, IDC_PASSWORD, L"password");
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_PASSWORDCHECK), BST_UNCHECKED);

            if (NT_SUCCESS(PhGetServiceDllParameter(serviceItem->Type, &serviceItem->Name->sr, &serviceDll)))
            {
                PhSetDialogItemText(hwndDlg, IDC_SERVICEDLL, serviceDll->Buffer);
                PhDereferenceObject(serviceDll);
            }
            else
            {
                PhSetDialogItemText(hwndDlg, IDC_SERVICEDLL, L"N/A");
            }

            PhpRefreshControls(hwndDlg);

            context->Ready = TRUE;

            if (PhEnableThemeSupport) // TODO: Required for compat (dmex)
                PhInitializeWindowTheme(GetParent(hwndDlg), PhEnableThemeSupport);  // HACK (GetParent)
            else
                PhInitializeWindowTheme(hwndDlg, FALSE);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveWindowPlacementToSetting(L"ServiceWindowPosition", NULL, GetParent(hwndDlg));
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
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
                        PhSetDialogItemText(hwndDlg, IDC_BINARYPATH, fileName->Buffer);
                        PhDereferenceObject(fileName);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            case IDC_PERMISSIONS:
                {
                    PhReferenceObject(context->ServiceItem);

                    PhEditSecurity(
                        PhCsForceNoParent ? NULL : hwndDlg,
                        PhGetStringOrEmpty(context->ServiceItem->DisplayName),
                        L"Service",
                        PhpOpenService,
                        PhpCloseServiceCallback,
                        context->ServiceItem
                        );
                }
                break;
            }

            switch (GET_WM_COMMAND_CMD(wParam, lParam))
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
            //case PSN_KILLACTIVE:
            //    {
            //        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
            //    }
            //    return TRUE;
            case PSN_APPLY:
                {
                    NTSTATUS status;
                    PPH_SERVICE_ITEM serviceItem = context->ServiceItem;
                    SC_HANDLE serviceHandle;
                    PPH_STRING newServiceTypeString;
                    PPH_STRING newServiceStartTypeString;
                    PPH_STRING newServiceErrorControlString;
                    ULONG newServiceType;
                    ULONG newServiceStartType;
                    ULONG newServiceErrorControl;
                    PPH_STRING newServiceGroup = NULL;
                    PPH_STRING newServiceBinaryPath = NULL;
                    PPH_STRING newServiceUserAccount = NULL;
                    PPH_STRING newServicePassword = NULL;

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

                    if (!context->Dirty)
                    {
                        return TRUE;
                    }

                    newServiceTypeString = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_TYPE)));
                    newServiceStartTypeString = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_STARTTYPE)));
                    newServiceErrorControlString = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_ERRORCONTROL)));
                    newServiceType = PhGetServiceTypeInteger(&newServiceTypeString->sr);
                    newServiceStartType = PhGetServiceStartTypeInteger(&newServiceStartTypeString->sr);
                    newServiceErrorControl = PhGetServiceErrorControlInteger(&newServiceErrorControlString->sr);

                    if (PhGetWindowTextLength(GetDlgItem(hwndDlg, IDC_GROUP)))
                        newServiceGroup = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_GROUP)));
                    if (PhGetWindowTextLength(GetDlgItem(hwndDlg, IDC_BINARYPATH)))
                        newServiceBinaryPath = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_BINARYPATH)));
                    if (PhGetWindowTextLength(GetDlgItem(hwndDlg, IDC_USERACCOUNT)))
                        newServiceUserAccount = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_USERACCOUNT)));
                    
                    if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_PASSWORDCHECK)) == BST_CHECKED)
                        newServicePassword = PhGetWindowText(GetDlgItem(hwndDlg, IDC_PASSWORD));

                    serviceHandle = PhOpenService(serviceItem->Name->Buffer, SERVICE_CHANGE_CONFIG);

                    if (serviceHandle)
                    {
                        if (ChangeServiceConfig(
                            serviceHandle,
                            newServiceType,
                            newServiceStartType,
                            newServiceErrorControl,
                            PhGetString(newServiceBinaryPath),
                            PhGetString(newServiceGroup),
                            NULL,
                            NULL,
                            PhGetString(newServiceUserAccount),
                            PhGetString(newServicePassword),
                            NULL
                            ))
                        {
                            BOOLEAN newDelayedStart;

                            newDelayedStart = Button_GetCheck(GetDlgItem(hwndDlg, IDC_DELAYEDSTART)) == BST_CHECKED;

                            if (newDelayedStart != context->OldDelayedStart)
                            {
                                PhSetServiceDelayedAutoStart(serviceHandle, newDelayedStart);
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
                                    PhGetString(newServiceBinaryPath),
                                    PhGetString(newServiceGroup),
                                    NULL,
                                    NULL,
                                    PhGetString(newServiceUserAccount),
                                    PhGetString(newServicePassword),
                                    NULL
                                    )))
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

                    PhShowStatus(hwndDlg, L"Unable to change service configuration.", 0, GetLastError());
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID);

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
