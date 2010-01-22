#include <phgui.h>
#include <windowsx.h>

INT_PTR CALLBACK PhpServicePropertiesDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowServicePropertiesDialog(
    __in HWND ParentWindowHandle,
    __in PPH_SERVICE_ITEM Service
    )
{
    PhReferenceObject(Service);
    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_SERVICE),
        ParentWindowHandle,
        PhpServicePropertiesDlgProc,
        (LPARAM)Service
        );
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

INT_PTR CALLBACK PhpServicePropertiesDlgProc(      
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
            PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)lParam;
            SC_HANDLE serviceHandle;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

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
        }
        break;
    case WM_DESTROY:
        {
            PhDereferenceObject((PPH_SERVICE_ITEM)GetProp(hwndDlg, L"ServiceItem"));
            RemoveProp(hwndDlg, L"ServiceItem");
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    }

    return FALSE;
}
