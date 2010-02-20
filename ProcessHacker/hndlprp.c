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

#include <phgui.h>
#include <kph.h>

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
    __in PVOID Context
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
    HPROPSHEETPAGE pages[3];
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
    if (PhStringEquals2(HandleItem->TypeName, L"Token", TRUE))
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

    PropertySheet(&propSheetHeader);
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

            SetProp(hwndDlg, L"Context", (HANDLE)context);

            if (!context)
            {
                // Dummy
            }
            else if (PhStringEquals2(context->HandleItem->TypeName, L"File", TRUE))
            {
                showPropertiesButton = TRUE;
            }
            else if (PhStringEquals2(context->HandleItem->TypeName, L"Key", TRUE))
            {
                showPropertiesButton = TRUE;
            }
            else if (PhStringEquals2(context->HandleItem->TypeName, L"Process", TRUE))
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
                }

                NtClose(processHandle);
            }
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
            case IDC_PROPERTIES:
                {
                    PHANDLE_PROPERTIES_CONTEXT context = (PHANDLE_PROPERTIES_CONTEXT)GetProp(hwndDlg, L"Context");

                    if (!context)
                    {
                        // Dummy
                    }
                    else if (PhStringEquals2(context->HandleItem->TypeName, L"File", TRUE))
                    {
                        PhShellProperties(hwndDlg, context->HandleItem->BestObjectName->Buffer);
                    }
                    else if (PhStringEquals2(context->HandleItem->TypeName, L"Key", TRUE))
                    {
                        PhShellOpenKey(hwndDlg, context->HandleItem->BestObjectName);
                    }
                    else if (PhStringEquals2(context->HandleItem->TypeName, L"Process", TRUE))
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
                                KphGetProcessId(
                                    PhKphHandle,
                                    processHandle,
                                    context->HandleItem->Handle,
                                    &processId
                                    );
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
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
