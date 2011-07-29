/*
 * Process Hacker - 
 *   handle properties
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
#include <kphuser.h>
#include <phplug.h>

typedef struct _HANDLE_PROPERTIES_CONTEXT
{
    HANDLE ProcessId;
    PPH_HANDLE_ITEM HandleItem;
} HANDLE_PROPERTIES_CONTEXT, *PHANDLE_PROPERTIES_CONTEXT;

INT_PTR CALLBACK PhpHandleGeneralDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

static NTSTATUS PhpDuplicateHandleFromProcess(
    __out PHANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt PVOID Context
    )
{
    NTSTATUS status;
    PHANDLE_PROPERTIES_CONTEXT context = Context;
    HANDLE processHandle;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_DUP_HANDLE,
        context->ProcessId
        )))
        return status;

    status = PhDuplicateObject(
        processHandle,
        context->HandleItem->Handle,
        NtCurrentProcess(),
        Handle,
        DesiredAccess,
        0,
        0
        );
    NtClose(processHandle);

    return status;
}

VOID PhShowHandleProperties(
    __in HWND ParentWindowHandle,
    __in HANDLE ProcessId,
    __in PPH_HANDLE_ITEM HandleItem
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[16];
    HANDLE_PROPERTIES_CONTEXT context;
    PH_STD_OBJECT_SECURITY stdObjectSecurity;
    PPH_ACCESS_ENTRY accessEntries;
    ULONG numberOfAccessEntries;

    context.ProcessId = ProcessId;
    context.HandleItem = HandleItem;

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.pszCaption = L"Handle";
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    // General page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_HNDLGENERAL);
    propSheetPage.pfnDlgProc = PhpHandleGeneralDlgProc;
    propSheetPage.lParam = (LPARAM)&context;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // Object-specific page
    if (!HandleItem->TypeName)
    {
        // Dummy
    }
    else if (PhEqualString2(HandleItem->TypeName, L"Event", TRUE))
    {
        pages[propSheetHeader.nPages++] = PhCreateEventPage(
            PhpDuplicateHandleFromProcess,
            &context
            );
    }
    else if (PhEqualString2(HandleItem->TypeName, L"EventPair", TRUE))
    {
        pages[propSheetHeader.nPages++] = PhCreateEventPairPage(
            PhpDuplicateHandleFromProcess,
            &context
            );
    }
    else if (PhEqualString2(HandleItem->TypeName, L"Job", TRUE))
    {
        pages[propSheetHeader.nPages++] = PhCreateJobPage(
            PhpDuplicateHandleFromProcess,
            &context,
            NULL
            );
    }
    else if (PhEqualString2(HandleItem->TypeName, L"Mutant", TRUE))
    {
        pages[propSheetHeader.nPages++] = PhCreateMutantPage(
            PhpDuplicateHandleFromProcess,
            &context
            );
    }
    else if (PhEqualString2(HandleItem->TypeName, L"Section", TRUE))
    {
        pages[propSheetHeader.nPages++] = PhCreateSectionPage(
            PhpDuplicateHandleFromProcess,
            &context
            );
    }
    else if (PhEqualString2(HandleItem->TypeName, L"Semaphore", TRUE))
    {
        pages[propSheetHeader.nPages++] = PhCreateSemaphorePage(
            PhpDuplicateHandleFromProcess,
            &context
            );
    }
    else if (PhEqualString2(HandleItem->TypeName, L"Timer", TRUE))
    {
        pages[propSheetHeader.nPages++] = PhCreateTimerPage(
            PhpDuplicateHandleFromProcess,
            &context
            );
    }
    else if (PhEqualString2(HandleItem->TypeName, L"Token", TRUE))
    {
        pages[propSheetHeader.nPages++] = PhCreateTokenPage(
            PhpDuplicateHandleFromProcess,
            &context,
            NULL
            );
    }

    // Security page
    stdObjectSecurity.OpenObject = PhpDuplicateHandleFromProcess;
    stdObjectSecurity.ObjectType = HandleItem->TypeName->Buffer;
    stdObjectSecurity.Context = &context;

    if (PhGetAccessEntries(HandleItem->TypeName->Buffer, &accessEntries, &numberOfAccessEntries))
    {
        pages[propSheetHeader.nPages++] = PhCreateSecurityPage(
            PhGetStringOrEmpty(HandleItem->BestObjectName),
            PhStdGetObjectSecurity,
            PhStdSetObjectSecurity,
            &stdObjectSecurity,
            accessEntries,
            numberOfAccessEntries
            );
        PhFree(accessEntries);
    }

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_OBJECT_PROPERTIES objectProperties;
        PH_PLUGIN_HANDLE_PROPERTIES_CONTEXT propertiesContext;

        propertiesContext.ProcessId = ProcessId;
        propertiesContext.HandleItem = HandleItem;

        objectProperties.Parameter = &propertiesContext;
        objectProperties.NumberOfPages = propSheetHeader.nPages;
        objectProperties.MaximumNumberOfPages = sizeof(pages) / sizeof(HPROPSHEETPAGE);
        objectProperties.Pages = pages;

        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackHandlePropertiesInitializing), &objectProperties);

        propSheetHeader.nPages = objectProperties.NumberOfPages;
    }

    PropertySheet(&propSheetHeader);
}

static VOID PhpShowProcessPropContext(
    __in PVOID Parameter
    )
{
    PhShowProcessProperties(Parameter);
    PhDereferenceObject(Parameter);
}

INT_PTR CALLBACK PhpHandleGeneralDlgProc(
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
            PHANDLE_PROPERTIES_CONTEXT context = (PHANDLE_PROPERTIES_CONTEXT)propSheetPage->lParam;
            BOOLEAN showPropertiesButton = FALSE;
            PPH_ACCESS_ENTRY accessEntries;
            ULONG numberOfAccessEntries;
            HANDLE processHandle;
            OBJECT_BASIC_INFORMATION basicInfo;
            BOOLEAN haveBasicInfo = FALSE;

            SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)context);

            if (!context)
            {
                // Dummy
            }
            else if (PhEqualString2(context->HandleItem->TypeName, L"File", TRUE))
            {
                showPropertiesButton = TRUE;
            }
            else if (PhEqualString2(context->HandleItem->TypeName, L"Key", TRUE))
            {
                showPropertiesButton = TRUE;
            }
            else if (PhEqualString2(context->HandleItem->TypeName, L"Process", TRUE))
            {
                showPropertiesButton = TRUE;
            }
            else if (PhEqualString2(context->HandleItem->TypeName, L"Thread", TRUE))
            {
                showPropertiesButton = TRUE;
            }

            ShowWindow(GetDlgItem(hwndDlg, IDC_PROPERTIES), showPropertiesButton ? SW_SHOW : SW_HIDE);

            SetDlgItemText(hwndDlg, IDC_NAME, PhGetString(context->HandleItem->BestObjectName));
            SetDlgItemText(hwndDlg, IDC_TYPE, context->HandleItem->TypeName->Buffer);
            SetDlgItemText(hwndDlg, IDC_ADDRESS, context->HandleItem->ObjectString);

            if (PhGetAccessEntries(
                context->HandleItem->TypeName->Buffer,
                &accessEntries,
                &numberOfAccessEntries
                ))
            {
                PPH_STRING accessString;
                PPH_STRING grantedAccessString;

                accessString = PhGetAccessString(
                    context->HandleItem->GrantedAccess,
                    accessEntries,
                    numberOfAccessEntries
                    );

                if (accessString->Length != 0)
                {
                    grantedAccessString = PhFormatString(
                        L"%s (%s)",
                        context->HandleItem->GrantedAccessString,
                        accessString->Buffer
                        );
                    SetDlgItemText(hwndDlg, IDC_GRANTED_ACCESS, grantedAccessString->Buffer);
                    PhDereferenceObject(grantedAccessString);
                }
                else
                {
                    SetDlgItemText(hwndDlg, IDC_GRANTED_ACCESS, context->HandleItem->GrantedAccessString);
                }

                PhDereferenceObject(accessString);

                PhFree(accessEntries);
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_GRANTED_ACCESS, context->HandleItem->GrantedAccessString);
            }

            if (NT_SUCCESS(PhOpenProcess(
                &processHandle,
                PROCESS_DUP_HANDLE,
                context->ProcessId
                )))
            {
                if (NT_SUCCESS(PhGetHandleInformation(
                    processHandle,
                    context->HandleItem->Handle,
                    -1,
                    &basicInfo,
                    NULL,
                    NULL,
                    NULL
                    )))
                {
                    SetDlgItemInt(hwndDlg, IDC_REFERENCES, basicInfo.PointerCount, FALSE);
                    SetDlgItemInt(hwndDlg, IDC_HANDLES, basicInfo.HandleCount, FALSE);
                    SetDlgItemInt(hwndDlg, IDC_PAGED, basicInfo.PagedPoolCharge, FALSE);
                    SetDlgItemInt(hwndDlg, IDC_NONPAGED, basicInfo.NonPagedPoolCharge, FALSE);

                    haveBasicInfo = TRUE;
                }

                NtClose(processHandle);
            }

            if (!haveBasicInfo)
            {
                SetDlgItemText(hwndDlg, IDC_REFERENCES, L"Unknown");
                SetDlgItemText(hwndDlg, IDC_HANDLES, L"Unknown");
                SetDlgItemText(hwndDlg, IDC_PAGED, L"Unknown");
                SetDlgItemText(hwndDlg, IDC_NONPAGED, L"Unknown");
            }
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, PhMakeContextAtom());
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_PROPERTIES:
                {
                    PHANDLE_PROPERTIES_CONTEXT context = (PHANDLE_PROPERTIES_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());

                    if (!context)
                    {
                        // Dummy
                    }
                    else if (PhEqualString2(context->HandleItem->TypeName, L"File", TRUE))
                    {
                        if (context->HandleItem->BestObjectName)
                            PhShellProperties(hwndDlg, context->HandleItem->BestObjectName->Buffer);
                        else
                            PhShowError(hwndDlg, L"Unable to open file properties because the object is unnamed.");
                    }
                    else if (PhEqualString2(context->HandleItem->TypeName, L"Key", TRUE))
                    {
                        PhShellOpenKey(hwndDlg, context->HandleItem->BestObjectName);
                    }
                    else if (PhEqualString2(context->HandleItem->TypeName, L"Process", TRUE))
                    {
                        HANDLE processHandle;
                        HANDLE processId;
                        PPH_PROCESS_ITEM processItem;

                        processId = NULL;

                        if (PhKphHandle)
                        {
                            if (NT_SUCCESS(PhOpenProcess(
                                &processHandle,
                                ProcessQueryAccess,
                                context->ProcessId
                                )))
                            {
                                PROCESS_BASIC_INFORMATION basicInfo;

                                if (NT_SUCCESS(KphQueryInformationObject(
                                    PhKphHandle,
                                    processHandle,
                                    context->HandleItem->Handle,
                                    KphObjectProcessBasicInformation,
                                    &basicInfo,
                                    sizeof(PROCESS_BASIC_INFORMATION),
                                    NULL
                                    )))
                                {
                                    processId = basicInfo.UniqueProcessId;
                                }

                                NtClose(processHandle);
                            }
                        }
                        else
                        {
                            HANDLE handle;
                            PROCESS_BASIC_INFORMATION basicInfo;

                            if (NT_SUCCESS(PhpDuplicateHandleFromProcess(
                                &handle,
                                ProcessQueryAccess,
                                context
                                )))
                            {
                                if (NT_SUCCESS(PhGetProcessBasicInformation(handle, &basicInfo)))
                                    processId = basicInfo.UniqueProcessId;

                                NtClose(handle);
                            }
                        }

                        if (processId)
                        {
                            processItem = PhReferenceProcessItem(processId);

                            if (processItem)
                            {
                                ProcessHacker_ShowProcessProperties(PhMainWndHandle, processItem);
                                PhDereferenceObject(processItem);
                            }
                            else
                            {
                                PhShowError(hwndDlg, L"The process does not exist.");
                            }
                        }
                    }
                    else if (PhEqualString2(context->HandleItem->TypeName, L"Thread", TRUE))
                    {
                        HANDLE processHandle;
                        CLIENT_ID clientId;
                        PPH_PROCESS_ITEM processItem;
                        PPH_PROCESS_PROPCONTEXT propContext;

                        clientId.UniqueProcess = NULL;
                        clientId.UniqueThread = NULL;

                        if (PhKphHandle)
                        {
                            if (NT_SUCCESS(PhOpenProcess(
                                &processHandle,
                                ProcessQueryAccess,
                                context->ProcessId
                                )))
                            {
                                THREAD_BASIC_INFORMATION basicInfo;

                                if (NT_SUCCESS(KphQueryInformationObject(
                                    PhKphHandle,
                                    processHandle,
                                    context->HandleItem->Handle,
                                    KphObjectThreadBasicInformation,
                                    &basicInfo,
                                    sizeof(THREAD_BASIC_INFORMATION),
                                    NULL
                                    )))
                                {
                                    clientId = basicInfo.ClientId;
                                }

                                NtClose(processHandle);
                            }
                        }
                        else
                        {
                            HANDLE handle;
                            THREAD_BASIC_INFORMATION basicInfo;

                            if (NT_SUCCESS(PhpDuplicateHandleFromProcess(
                                &handle,
                                ThreadQueryAccess,
                                context
                                )))
                            {
                                if (NT_SUCCESS(PhGetThreadBasicInformation(handle, &basicInfo)))
                                    clientId = basicInfo.ClientId;

                                NtClose(handle);
                            }
                        }

                        if (clientId.UniqueProcess)
                        {
                            processItem = PhReferenceProcessItem(clientId.UniqueProcess);

                            if (processItem)
                            {
                                propContext = PhCreateProcessPropContext(PhMainWndHandle, processItem);
                                PhDereferenceObject(processItem);
                                PhSetSelectThreadIdProcessPropContext(propContext, clientId.UniqueThread);
                                ProcessHacker_Invoke(PhMainWndHandle, PhpShowProcessPropContext, propContext);
                            }
                            else
                            {
                                PhShowError(hwndDlg, L"The process does not exist.");
                            }
                        }
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
