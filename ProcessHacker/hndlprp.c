/*
 * Process Hacker -
 *   handle properties
 *
 * Copyright (C) 2010-2013 wj32
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
#include <secedit.h>
#include <phplug.h>

typedef struct _HANDLE_PROPERTIES_CONTEXT
{
    HANDLE ProcessId;
    PPH_HANDLE_ITEM HandleItem;
} HANDLE_PROPERTIES_CONTEXT, *PHANDLE_PROPERTIES_CONTEXT;

INT_PTR CALLBACK PhpHandleGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

static NTSTATUS PhpDuplicateHandleFromProcess(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
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

    if (KphIsConnected() && PhEqualString2(context->HandleItem->TypeName, L"File", TRUE))
    {
        status = PhCallKphDuplicateObjectWithTimeout(
            processHandle,
            context->HandleItem->Handle,
            NtCurrentProcess(),
            Handle,
            DesiredAccess,
            0,
            0
            );
    }
    else
    {
        status = PhDuplicateObject(
            processHandle,
            context->HandleItem->Handle,
            NtCurrentProcess(),
            Handle,
            DesiredAccess,
            0,
            0
            );
    }

    NtClose(processHandle);

    return status;
}

VOID PhShowHandleProperties(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId,
    _In_ PPH_HANDLE_ITEM HandleItem
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

    PhModalPropertySheet(&propSheetHeader);
}

INT_PTR CALLBACK PhpHandleGeneralDlgProc(
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
            PHANDLE_PROPERTIES_CONTEXT context = (PHANDLE_PROPERTIES_CONTEXT)propSheetPage->lParam;
            PPH_ACCESS_ENTRY accessEntries;
            ULONG numberOfAccessEntries;
            HANDLE processHandle;
            OBJECT_BASIC_INFORMATION basicInfo;
            BOOLEAN haveBasicInfo = FALSE;

            SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)context);

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
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)GetDlgItem(hwndDlg, IDC_BASICINFORMATION));
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}
