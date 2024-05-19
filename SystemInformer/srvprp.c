/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2017-2023
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

    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN Ready : 1;
            BOOLEAN Dirty : 1;
            BOOLEAN OldDelayedStart : 1;
            BOOLEAN Spare : 5;
        };
    };

    HWND WindowHandle;
    HWND TypeWindowHandle;
    HWND StartTypeWindowHandle;
    HWND ErrorControlWindowHandle;
    HWND DelayedStartWindowHandle;
    HWND DescriptionWindowHandle;
    HWND GroupWindowHandle;
    HWND BinaryPathWindowHandle;
    HWND UserNameWindowHandle;
    HWND PassBoxWindowHandle;
    HWND PassCheckBoxWindowHandle;
    HWND ServiceDllWindowHandle;
} SERVICE_PROPERTIES_CONTEXT, *PSERVICE_PROPERTIES_CONTEXT;

INT_PTR CALLBACK PhpServiceGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

static NTSTATUS PhpOpenServiceCallback(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PVOID Context
    )
{
    PPH_SERVICE_ITEM serviceItem = ((PPH_SERVICE_ITEM)Context);
    NTSTATUS status;
    SC_HANDLE serviceHandle;

    status = PhOpenService(
        &serviceHandle,
        DesiredAccess,
        PhGetString(serviceItem->Name)
        );

    if (NT_SUCCESS(status))
    {
        *Handle = serviceHandle;
        return STATUS_SUCCESS;
    }

    *Handle = NULL;
    return status;
}

NTSTATUS PhpCloseServiceCallback(
    _In_ PVOID Context
    )
{
    PhDereferenceObject(((PPH_SERVICE_ITEM)Context));
    return STATUS_SUCCESS;
}

static _Callback_ NTSTATUS PhpSetServiceSecurityCallback(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_opt_ PVOID Context
    )
{
    PPH_STD_OBJECT_SECURITY stdObjectSecurity = ((PPH_STD_OBJECT_SECURITY)Context);
    PPH_SERVICE_ITEM serviceItem = ((PPH_SERVICE_ITEM)stdObjectSecurity->Context);
    NTSTATUS status;

    status = PhStdSetObjectSecurity(
        SecurityDescriptor,
        SecurityInformation,
        stdObjectSecurity->ObjectContext
        );

    if (status == STATUS_ACCESS_DENIED && !PhGetOwnTokenAttributes().Elevated)
    {
        // Elevate using phsvc.
        if (PhUiConnectToPhSvc(NULL, FALSE))
        {
            status = PhSvcCallSetServiceSecurity(
                PhGetString(serviceItem->Name),
                SecurityInformation,
                SecurityDescriptor
                );
            PhUiDisconnectFromPhSvc();
        }
    }

    return status;
}

NTSTATUS PhpShowServicePropertiesThread(
    _In_ PVOID Context
    )
{
    PPH_SERVICE_ITEM serviceItem = ((PPH_SERVICE_ITEM)Context);
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[32];
    SERVICE_PROPERTIES_CONTEXT context;

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

static VOID PhServicePropertiesRefreshIcon(
    _In_ PSERVICE_PROPERTIES_CONTEXT Context
    )
{
    HICON serviceIcon;

    if (Context->ServiceItem->IconEntry && Context->ServiceItem->IconEntry->SmallIconIndex)
        serviceIcon = PhGetImageListIcon(Context->ServiceItem->IconEntry->SmallIconIndex, FALSE);
    else
    {
        if (Context->ServiceItem->Type == SERVICE_KERNEL_DRIVER || Context->ServiceItem->Type == SERVICE_FILE_SYSTEM_DRIVER)
            serviceIcon = PhGetImageListIcon(1, FALSE); // ServiceCogIcon;
        else
            serviceIcon = PhGetImageListIcon(0, FALSE); // ServiceApplicationIcon;
    }

    if (serviceIcon)
    {
        PhSetWindowIcon(GetParent(Context->WindowHandle), serviceIcon, NULL, TRUE);
    }
}

static VOID PhServicePropertiesRefreshControls(
    _In_ PSERVICE_PROPERTIES_CONTEXT Context
    )
{
    PPH_STRING serviceStartTypeString;
    PPH_STRING serviceTypeString;

    serviceStartTypeString = PH_AUTO(PhGetWindowText(Context->StartTypeWindowHandle));
    serviceTypeString = PH_AUTO(PhGetWindowText(Context->TypeWindowHandle));

    if (PhEqualString2(serviceStartTypeString, L"Auto start", FALSE))
        EnableWindow(Context->DelayedStartWindowHandle, TRUE);
    else
        EnableWindow(Context->DelayedStartWindowHandle, FALSE);

    if (PhEqualString2(serviceTypeString, L"Driver", FALSE) ||
        PhEqualString2(serviceTypeString, L"FS driver", FALSE))
    {
        EnableWindow(Context->UserNameWindowHandle, FALSE);
        EnableWindow(Context->PassBoxWindowHandle, FALSE);
        EnableWindow(Context->PassCheckBoxWindowHandle, FALSE);
        EnableWindow(Context->ServiceDllWindowHandle, FALSE);
    }
    else
    {
        EnableWindow(Context->UserNameWindowHandle, TRUE);
        EnableWindow(Context->PassBoxWindowHandle, TRUE);
        EnableWindow(Context->PassCheckBoxWindowHandle, TRUE);
        EnableWindow(Context->ServiceDllWindowHandle, TRUE);
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

            context->WindowHandle = hwndDlg;
            context->TypeWindowHandle = GetDlgItem(hwndDlg, IDC_TYPE);
            context->StartTypeWindowHandle = GetDlgItem(hwndDlg, IDC_STARTTYPE);
            context->ErrorControlWindowHandle = GetDlgItem(hwndDlg, IDC_ERRORCONTROL);
            context->DelayedStartWindowHandle = GetDlgItem(hwndDlg, IDC_DELAYEDSTART);
            context->DescriptionWindowHandle = GetDlgItem(hwndDlg, IDC_DESCRIPTION);
            context->GroupWindowHandle = GetDlgItem(hwndDlg, IDC_GROUP);
            context->BinaryPathWindowHandle = GetDlgItem(hwndDlg, IDC_BINARYPATH);
            context->UserNameWindowHandle = GetDlgItem(hwndDlg, IDC_USERACCOUNT);
            context->PassBoxWindowHandle = GetDlgItem(hwndDlg, IDC_PASSWORD);
            context->PassCheckBoxWindowHandle = GetDlgItem(hwndDlg, IDC_PASSWORDCHECK);
            context->ServiceDllWindowHandle = GetDlgItem(hwndDlg, IDC_SERVICEDLL);

            // HACK
            if (PhGetIntegerPairSetting(L"ServiceWindowPosition").X == 0)
                PhCenterWindow(GetParent(hwndDlg), PhMainWndHandle);
            else
                PhLoadWindowPlacementFromSetting(L"ServiceWindowPosition", NULL, GetParent(hwndDlg));

            PhAddComboBoxStringRefs(context->TypeWindowHandle, PhServiceTypeStrings, RTL_NUMBER_OF(PhServiceTypeStrings));
            PhAddComboBoxStringRefs(context->StartTypeWindowHandle, PhServiceStartTypeStrings, RTL_NUMBER_OF(PhServiceStartTypeStrings));
            PhAddComboBoxStringRefs(context->ErrorControlWindowHandle, PhServiceErrorControlStrings, RTL_NUMBER_OF(PhServiceErrorControlStrings));

            PhSetWindowText(context->DescriptionWindowHandle, PhGetStringOrEmpty(serviceItem->DisplayName));
            PhSelectComboBoxString(context->TypeWindowHandle, PhGetServiceTypeString(serviceItem->Type)->Buffer, FALSE);

            startType = serviceItem->StartType;
            errorControl = serviceItem->ErrorControl;

            if (NT_SUCCESS(PhOpenService(&serviceHandle, SERVICE_QUERY_CONFIG, PhGetString(serviceItem->Name))))
            {
                LPQUERY_SERVICE_CONFIG config;
                PPH_STRING description;
                BOOLEAN delayedStart;

                if (NT_SUCCESS(PhGetServiceConfig(serviceHandle, &config)))
                {
                    PhSetWindowText(context->GroupWindowHandle, config->lpLoadOrderGroup);
                    PhSetWindowText(context->BinaryPathWindowHandle, config->lpBinaryPathName);
                    PhSetWindowText(context->UserNameWindowHandle, config->lpServiceStartName);

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
                    PhSetWindowText(context->DescriptionWindowHandle, PhGetString(description));
                    PhDereferenceObject(description);
                }

                if (PhGetServiceDelayedAutoStart(serviceHandle, &delayedStart))
                {
                    context->OldDelayedStart = delayedStart;

                    if (delayedStart)
                        Button_SetCheck(context->DelayedStartWindowHandle, BST_CHECKED);
                }

                PhCloseServiceHandle(serviceHandle);
            }

            PhSelectComboBoxString(context->StartTypeWindowHandle, PhGetServiceStartTypeString(startType)->Buffer, FALSE);
            PhSelectComboBoxString(context->ErrorControlWindowHandle, PhGetServiceErrorControlString(errorControl)->Buffer, FALSE);

            PhSetWindowText(context->PassBoxWindowHandle, L"password");
            Button_SetCheck(context->PassCheckBoxWindowHandle, BST_UNCHECKED);

            if (NT_SUCCESS(PhGetServiceDllParameter(serviceItem->Type, &serviceItem->Name->sr, &serviceDll)))
            {
                PhSetDialogItemText(hwndDlg, IDC_SERVICEDLL, PhGetString(serviceDll));
                PhDereferenceObject(serviceDll);
            }
            else
            {
                PhSetDialogItemText(hwndDlg, IDC_SERVICEDLL, L"N/A");
            }

            PhServicePropertiesRefreshIcon(context);
            PhServicePropertiesRefreshControls(context);

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
                        Button_SetCheck(context->PassCheckBoxWindowHandle, BST_CHECKED);
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
                    PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));

                    commandLine = PH_AUTO_T(PH_STRING, PhGetWindowText(context->BinaryPathWindowHandle));

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

                    PhSetFileDialogFileName(fileDialog, PhGetString(fileName));
                    PhDereferenceObject(fileName);

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        fileName = PhGetFileDialogFileName(fileDialog);
                        PhSetWindowText(context->BinaryPathWindowHandle, PhGetString(fileName));
                        PhDereferenceObject(fileName);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            case IDC_PERMISSIONS:
                {
                    PhReferenceObject(context->ServiceItem);

                    PhEditSecurityEx(
                        PhCsForceNoParent ? NULL : hwndDlg,
                        PhGetStringOrEmpty(context->ServiceItem->DisplayName),
                        L"Service",
                        PhpOpenServiceCallback,
                        PhpCloseServiceCallback,
                        NULL,
                        PhpSetServiceSecurityCallback,
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
                    PhServicePropertiesRefreshControls(context);

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
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)context->StartTypeWindowHandle);
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

                    newServiceTypeString = PH_AUTO(PhGetWindowText(context->TypeWindowHandle));
                    newServiceStartTypeString = PH_AUTO(PhGetWindowText(context->StartTypeWindowHandle));
                    newServiceErrorControlString = PH_AUTO(PhGetWindowText(context->ErrorControlWindowHandle));
                    newServiceType = PhGetServiceTypeInteger(&newServiceTypeString->sr);
                    newServiceStartType = PhGetServiceStartTypeInteger(&newServiceStartTypeString->sr);
                    newServiceErrorControl = PhGetServiceErrorControlInteger(&newServiceErrorControlString->sr);

                    if (PhEqualStringRef(PhGetServiceTypeString(serviceItem->Type), &newServiceTypeString->sr, TRUE))
                        newServiceType = SERVICE_NO_CHANGE;
                    if (PhEqualStringRef(PhGetServiceStartTypeString(serviceItem->StartType), &newServiceStartTypeString->sr, TRUE))
                        newServiceStartType = SERVICE_NO_CHANGE;
                    if (PhEqualStringRef(PhGetServiceErrorControlString(serviceItem->ErrorControl), &newServiceErrorControlString->sr, TRUE))
                        newServiceErrorControl = SERVICE_NO_CHANGE;

                    if (PhGetWindowTextLength(context->GroupWindowHandle))
                        newServiceGroup = PH_AUTO(PhGetWindowText(context->GroupWindowHandle));
                    if (PhGetWindowTextLength(context->BinaryPathWindowHandle))
                        newServiceBinaryPath = PH_AUTO(PhGetWindowText(context->BinaryPathWindowHandle));
                    if (PhGetWindowTextLength(context->UserNameWindowHandle))
                        newServiceUserAccount = PH_AUTO(PhGetWindowText(context->UserNameWindowHandle));

                    if (Button_GetCheck(context->PassCheckBoxWindowHandle) == BST_CHECKED)
                        newServicePassword = PhGetWindowText(context->PassBoxWindowHandle);

                    status = PhOpenService(&serviceHandle, SERVICE_CHANGE_CONFIG, PhGetString(serviceItem->Name));

                    if (NT_SUCCESS(status))
                    {
                        status = PhChangeServiceConfig(
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
                            );

                        if (NT_SUCCESS(status))
                        {
                            BOOLEAN newDelayedStart;

                            newDelayedStart = Button_GetCheck(context->DelayedStartWindowHandle) == BST_CHECKED;

                            if (newDelayedStart != context->OldDelayedStart)
                            {
                                PhSetServiceDelayedAutoStart(serviceHandle, newDelayedStart);
                            }

                            PhMarkNeedsConfigUpdateServiceItem(serviceItem);

                            PhCloseServiceHandle(serviceHandle);
                        }
                        else
                        {
                            PhCloseServiceHandle(serviceHandle);
                            goto ErrorCase;
                        }
                    }
                    else
                    {
                        if (status == STATUS_ACCESS_DENIED && !PhGetOwnTokenAttributes().Elevated)
                        {
                            // Elevate using phsvc.
                            if (PhUiConnectToPhSvc(hwndDlg, FALSE))
                            {
                                status = PhSvcCallChangeServiceConfig(
                                    PhGetString(serviceItem->Name),
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
                                    );

                                if (NT_SUCCESS(status))
                                {
                                    BOOLEAN newDelayedStart;

                                    newDelayedStart = Button_GetCheck(context->DelayedStartWindowHandle) == BST_CHECKED;

                                    if (newDelayedStart != context->OldDelayedStart)
                                    {
                                        SERVICE_DELAYED_AUTO_START_INFO info;

                                        info.fDelayedAutostart = newDelayedStart;
                                        PhSvcCallChangeServiceConfig2(
                                            PhGetString(serviceItem->Name),
                                            SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
                                            &info
                                            );
                                    }

                                    PhMarkNeedsConfigUpdateServiceItem(serviceItem);
                                }

                                PhUiDisconnectFromPhSvc();

                                if (!NT_SUCCESS(status))
                                {
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

                    PhShowStatus(hwndDlg, L"Unable to change service configuration.", status, 0);
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
