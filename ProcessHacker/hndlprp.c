/*
 * Process Hacker -
 *   handle properties
 *
 * Copyright (C) 2010-2013 wj32
 * Copyright (C) 2018 dmex
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
#include <hndlinfo.h>
#include <secedit.h>

#include <hndlprv.h>
#include <phplug.h>

#include <uxtheme.h>

typedef struct _HANDLE_PROPERTIES_CONTEXT
{
    HWND ListViewHandle;
    HANDLE ProcessId;
    PPH_HANDLE_ITEM HandleItem;
    PH_LAYOUT_MANAGER LayoutManager;
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

    status = NtDuplicateObject(
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
    propSheetHeader.hInstance = PhInstanceHandle;
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.pszCaption = L"Handle";
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    // General page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_HNDLGENERAL);
    propSheetPage.hInstance = PhInstanceHandle;
    propSheetPage.pfnDlgProc = PhpHandleGeneralDlgProc;
    propSheetPage.lParam = (LPARAM)&context;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // Object-specific page
    if (PhIsNullOrEmptyString(HandleItem->TypeName))
    {
        NOTHING;
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
    else if (PhEqualString2(HandleItem->TypeName, L"File", TRUE))
    {
        pages[propSheetHeader.nPages++] = PhCreateFilePage(
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
    stdObjectSecurity.ObjectType = PhGetStringOrEmpty(HandleItem->TypeName);
    stdObjectSecurity.Context = &context;

    if (PhGetAccessEntries(PhGetStringOrEmpty(HandleItem->TypeName), &accessEntries, &numberOfAccessEntries))
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

typedef enum _PHP_HANDLE_GENERAL_CATEGORY
{
    PH_HANDLE_GENERAL_CATEGORY_BASICINFO,
    PH_HANDLE_GENERAL_CATEGORY_REFERENCES,
    PH_HANDLE_GENERAL_CATEGORY_QUOTA
} PHP_HANDLE_GENERAL_CATEGORY;

typedef enum _PHP_HANDLE_GENERAL_INDEX
{
    PH_HANDLE_GENERAL_INDEX_NAME,
    PH_HANDLE_GENERAL_INDEX_TYPE,
    PH_HANDLE_GENERAL_INDEX_OBJECT,
    PH_HANDLE_GENERAL_INDEX_ACCESSMASK,
    PH_HANDLE_GENERAL_INDEX_REFERENCES,
    PH_HANDLE_GENERAL_INDEX_HANDLES,
    PH_HANDLE_GENERAL_INDEX_PAGED,
    PH_HANDLE_GENERAL_INDEX_NONPAGED
} PHP_PROCESS_STATISTICS_INDEX;

VOID PhpUpdateHandleGeneralListViewGroups(
    _In_ HWND ListViewHandle
    )
{
    ListView_EnableGroupView(ListViewHandle, TRUE);
    PhAddListViewGroup(ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_BASICINFO, L"Basic information");
    PhAddListViewGroup(ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_REFERENCES, L"References");
    PhAddListViewGroup(ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_QUOTA, L"Quota charges");
    PhAddListViewGroupItem(ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_BASICINFO, PH_HANDLE_GENERAL_INDEX_NAME, L"Name", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_BASICINFO, PH_HANDLE_GENERAL_INDEX_TYPE, L"Type", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_BASICINFO, PH_HANDLE_GENERAL_INDEX_OBJECT, L"Object address", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_BASICINFO, PH_HANDLE_GENERAL_INDEX_ACCESSMASK, L"Granted access", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_REFERENCES, PH_HANDLE_GENERAL_INDEX_REFERENCES, L"References", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_REFERENCES, PH_HANDLE_GENERAL_INDEX_HANDLES, L"Handles", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_QUOTA, PH_HANDLE_GENERAL_INDEX_PAGED, L"Paged", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_QUOTA, PH_HANDLE_GENERAL_INDEX_NONPAGED, L"Virtual size", NULL);
}

VOID PhpUpdateHandleGeneral(
    _In_ PHANDLE_PROPERTIES_CONTEXT Context
    )
{
    HANDLE processHandle;
    PPH_ACCESS_ENTRY accessEntries;
    ULONG numberOfAccessEntries;
    OBJECT_BASIC_INFORMATION basicInfo;
    WCHAR string[PH_PTR_STR_LEN];

    PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_NAME, 1, PhGetStringOrEmpty(Context->HandleItem->BestObjectName));
    PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_TYPE, 1, PhGetStringOrEmpty(Context->HandleItem->TypeName));
    PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_OBJECT, 1, Context->HandleItem->ObjectString);

    if (PhGetAccessEntries(
        PhGetStringOrEmpty(Context->HandleItem->TypeName),
        &accessEntries,
        &numberOfAccessEntries
        ))
    {
        PPH_STRING accessString;
        PPH_STRING grantedAccessString;

        accessString = PH_AUTO(PhGetAccessString(
            Context->HandleItem->GrantedAccess,
            accessEntries,
            numberOfAccessEntries
            ));

        if (accessString->Length != 0)
        {
            grantedAccessString = PH_AUTO(PhFormatString(
                L"0x%x (%s)",
                Context->HandleItem->GrantedAccess,
                accessString->Buffer
                ));

            PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_ACCESSMASK, 1, grantedAccessString->Buffer);
        }
        else
        {
            PhPrintPointer(string, UlongToPtr(Context->HandleItem->GrantedAccess));
            PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_ACCESSMASK, 1, string);
        }

        PhFree(accessEntries);
    }
    else
    {
        PhPrintPointer(string, UlongToPtr(Context->HandleItem->GrantedAccess));
        PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_ACCESSMASK, 1, string);
    }

    if (NT_SUCCESS(PhOpenProcess(
        &processHandle,
        PROCESS_DUP_HANDLE,
        Context->ProcessId
        )))
    {
        if (NT_SUCCESS(PhGetHandleInformation(
            processHandle,
            Context->HandleItem->Handle,
            ULONG_MAX,
            &basicInfo,
            NULL,
            NULL,
            NULL
            )))
        {
            PhPrintUInt32(string, basicInfo.PointerCount);
            PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_REFERENCES, 1, string);

            PhPrintUInt32(string, basicInfo.HandleCount);
            PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_HANDLES, 1, string);

            PhPrintUInt32(string, basicInfo.PagedPoolCharge);
            PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_PAGED, 1, string);

            PhPrintUInt32(string, basicInfo.NonPagedPoolCharge);
            PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_NONPAGED, 1, string);
        }

        NtClose(processHandle);
    }
}

INT_PTR CALLBACK PhpHandleGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PHANDLE_PROPERTIES_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
        context = (PHANDLE_PROPERTIES_CONTEXT)propSheetPage->lParam;

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
            PPH_ACCESS_ENTRY accessEntries;
            ULONG numberOfAccessEntries;
            HANDLE processHandle;
            OBJECT_BASIC_INFORMATION basicInfo;
            BOOLEAN haveBasicInfo = FALSE;

            // HACK
            PhCenterWindow(GetParent(hwndDlg), GetParent(GetParent(hwndDlg)));

            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 120, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 250, L"Value");
            PhSetExtendedListView(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PhpUpdateHandleGeneralListViewGroups(context->ListViewHandle);
            PhpUpdateHandleGeneral(context);

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
            ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
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

    REFLECT_MESSAGE_DLG(hwndDlg, context->ListViewHandle, uMsg, wParam, lParam);

    return FALSE;
}
