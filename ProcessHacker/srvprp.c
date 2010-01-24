#include <phgui.h>
#include <windowsx.h>

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
    PH_STD_OBJECT_SECURITY stdObjectSecurity;
    PPH_ACCESS_ENTRY accessEntries;
    ULONG numberOfAccessEntries;

    PhReferenceObject(ServiceItem);

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.pszCaption = ServiceItem->Name->Buffer;
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    // General page.
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_SRVGENERAL);
    propSheetPage.pfnDlgProc = PhpServiceGeneralDlgProc;
    propSheetPage.lParam = (LPARAM)ServiceItem;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // Security page.
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

    PhDereferenceObject(ServiceItem);
}

VOID PhpAddComboBoxItems(
    __in HWND hWnd,
    __in PWSTR *Items,
    __in ULONG NumberOfItems
    )
{
    ULONG i;

    for (i = 0; i < NumberOfItems; i++)
        ComboBox_AddString(hWnd, Items[i]);
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
            WCHAR *serviceTypeItems[] = { L"Driver", L"FS Driver", L"Own Process", L"Share Process",
                L"Own Interactive Process", L"Share Interactive Process" };
            WCHAR *serviceStartTypeItems[] = { L"Disabled", L"Boot Start", L"System Start",
                L"Auto Start", L"Demand Start" };
            WCHAR *serviceErrorControlItems[] = { L"Ignore", L"Normal", L"Severe", L"Critical" };
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)propSheetPage->lParam;
            SC_HANDLE serviceHandle;

            SetProp(hwndDlg, L"ServiceItem", (HANDLE)serviceItem);

            PhpAddComboBoxItems(GetDlgItem(hwndDlg, IDC_TYPE), serviceTypeItems,
                sizeof(serviceTypeItems) / sizeof(WCHAR *));
            PhpAddComboBoxItems(GetDlgItem(hwndDlg, IDC_STARTTYPE), serviceStartTypeItems,
                sizeof(serviceStartTypeItems) / sizeof(WCHAR *));
            PhpAddComboBoxItems(GetDlgItem(hwndDlg, IDC_ERRORCONTROL), serviceErrorControlItems,
                sizeof(serviceErrorControlItems) / sizeof(WCHAR *));

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
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, L"ServiceItem");
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
            }
        }
        break;
    }

    return FALSE;
}
