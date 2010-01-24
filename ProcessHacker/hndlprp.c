#include <phgui.h>

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
    CloseHandle(processHandle);

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
    PhReferenceObject(HandleItem);
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

    // General page.
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_HNDLGENERAL);
    propSheetPage.pfnDlgProc = PhpHandleGeneralDlgProc;
    propSheetPage.lParam = (LPARAM)&context;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // Security page.
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

    PhDereferenceObject(HandleItem);
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
            HANDLE processHandle;
            OBJECT_BASIC_INFORMATION basicInfo;

            SetDlgItemText(hwndDlg, IDC_NAME, PhGetString(context->HandleItem->BestObjectName));
            SetDlgItemText(hwndDlg, IDC_TYPE, context->HandleItem->TypeName->Buffer);
            SetDlgItemText(hwndDlg, IDC_ADDRESS, context->HandleItem->ObjectString);
            SetDlgItemText(hwndDlg, IDC_GRANTED_ACCESS, context->HandleItem->GrantedAccessString);

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

                CloseHandle(processHandle);
            }
        }
        break;
    }

    return FALSE;
}
