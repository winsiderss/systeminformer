/*
 * Process Hacker Toolchain -
 *   project setup
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

#include <setup.h>

HANDLE MutantHandle = NULL;
SETUP_COMMAND_TYPE SetupMode = SETUP_COMMAND_INSTALL;
PH_COMMAND_LINE_OPTION options[] =
{
    { SETUP_COMMAND_INSTALL, L"install", NoArgumentType },
    { SETUP_COMMAND_UNINSTALL, L"uninstall", NoArgumentType },
    { SETUP_COMMAND_UPDATE, L"update", NoArgumentType },
    { SETUP_COMMAND_REPAIR, L"repair", NoArgumentType },
};

static NTSTATUS CreateSetupMutant(
    VOID
    )
{
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING mutantName;

    RtlInitUnicodeString(&mutantName, L"\\BaseNamedObjects\\ProcessHackerSetup");
    InitializeObjectAttributes(
        &oa,
        &mutantName,
        0,
        NULL,
        NULL
        );

    return NtCreateMutant(&MutantHandle, MUTANT_ALL_ACCESS, &oa, FALSE);
}

static VOID SetupInitializeDpi(
    VOID
    )
{
    HDC hdc;

    if (hdc = CreateIC(L"DISPLAY", NULL, NULL, NULL))
    {
        PhGlobalDpi = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(NULL, hdc);
    }
}

static BOOLEAN NTAPI MainPropSheetCommandLineCallback(
    _In_opt_ PPH_COMMAND_LINE_OPTION Option,
    _In_opt_ PPH_STRING Value,
    _In_opt_ PVOID Context
    )
{
    if (Option)
        SetupMode = Option->Id;

    return TRUE;
}

INT CALLBACK MainPropSheet_Callback(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _Inout_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case PSCB_PRECREATE:
        {
            if (lParam)
            {
                if (((DLGTEMPLATEEX *)lParam)->signature == USHORT_MAX)
                    ((DLGTEMPLATEEX *)lParam)->style |= WS_MINIMIZEBOX;
                else
                    ((DLGTEMPLATE *)lParam)->style |= WS_MINIMIZEBOX;
            }
        }
        break;
    case PSCB_INITIALIZED:
        {
            PPH_SETUP_CONTEXT context;

            context = PhAllocate(sizeof(PH_SETUP_CONTEXT));
            memset(context, 0, sizeof(PH_SETUP_CONTEXT));
       
            context->CurrentMajorVersion = PHAPP_VERSION_MAJOR;
            context->CurrentMinorVersion = PHAPP_VERSION_MINOR;
            context->CurrentRevisionVersion = PHAPP_VERSION_REVISION;

            context->DialogHandle = hwndDlg;
            context->PropSheetBackHandle = GetDlgItem(hwndDlg, IDC_PROPSHEET_BACK);
            context->PropSheetForwardHandle = GetDlgItem(hwndDlg, IDC_PROPSHEET_NEXT);
            context->PropSheetCancelHandle = GetDlgItem(hwndDlg, IDC_PROPSHEET_CANCEL);

            SetupInitializeFont(context->PropSheetBackHandle, -12, FW_NORMAL);
            SetupInitializeFont(context->PropSheetForwardHandle, -12, FW_NORMAL);
            SetupInitializeFont(context->PropSheetCancelHandle, -12, FW_NORMAL);

            SetProp(hwndDlg, L"SetupContext", (HANDLE)context);
        }
        break;
    }

    return FALSE;
}

INT WINAPI wWinMain(
    _In_ HINSTANCE Instance,
    _In_opt_ HINSTANCE PrevInstance,
    _In_ PWSTR CmdLine,
    _In_ INT CmdShow
    )
{
    if (!NT_SUCCESS(CreateSetupMutant()))
        return 1;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (!NT_SUCCESS(PhInitializePhLibEx(ULONG_MAX, Instance, 0, 0)))
        return 1;

    PhApplicationName = L"Process Hacker - Setup";
    PhGuiSupportInitialization();
    SetupInitializeDpi();

    {
        PH_STRINGREF commandLine;

        PhUnicodeStringToStringRef(&NtCurrentPeb()->ProcessParameters->CommandLine, &commandLine);
        if (!PhParseCommandLine(
            &commandLine,
            options,
            ARRAYSIZE(options),
            PH_COMMAND_LINE_IGNORE_FIRST_PART,
            MainPropSheetCommandLineCallback,
            NULL
            ))
        {
            return 1;
        }
    }

#ifdef _DEBUG
    if (CheckProcessHackerInstalled())
        SetupMode = SETUP_COMMAND_UNINSTALL;
#endif

    switch (SetupMode)
    {
    case SETUP_COMMAND_INSTALL:
    default:
        {
            PROPSHEETPAGE propSheetPage = { sizeof(PROPSHEETPAGE) };
            PROPSHEETHEADER propSheetHeader = { sizeof(PROPSHEETHEADER) };
            HPROPSHEETPAGE pages[6];

            propSheetHeader.dwFlags = PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP | PSH_USECALLBACK | PSH_WIZARD_LITE;
            propSheetHeader.pszIcon = MAKEINTRESOURCE(IDI_ICON1);
            propSheetHeader.pfnCallback = MainPropSheet_Callback;
            propSheetHeader.hInstance = PhInstanceHandle;
            propSheetHeader.phpage = pages;

            // welcome page
            memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
            propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
            propSheetPage.dwFlags = PSP_USETITLE;
            propSheetPage.pszTitle = PhApplicationName;
            propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG1);
            propSheetPage.hInstance = PhInstanceHandle;
            propSheetPage.pfnDlgProc = SetupPropPage1_WndProc;
            pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

            // eula page
            memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
            propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
            propSheetPage.dwFlags = PSP_USETITLE;
            propSheetPage.pszTitle = PhApplicationName;
            propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG2);
            propSheetPage.hInstance = PhInstanceHandle;
            propSheetPage.pfnDlgProc = SetupPropPage2_WndProc;
            pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

            // config page
            memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
            propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
            propSheetPage.dwFlags = PSP_USETITLE;
            propSheetPage.pszTitle = PhApplicationName;
            propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG3);
            propSheetPage.hInstance = PhInstanceHandle;
            propSheetPage.pfnDlgProc = SetupPropPage3_WndProc;
            pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

            // download page
            memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
            propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
            propSheetPage.dwFlags = PSP_USETITLE;
            propSheetPage.pszTitle = PhApplicationName;
            propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG5);
            propSheetPage.hInstance = PhInstanceHandle;
            propSheetPage.pfnDlgProc = SetupPropPage5_WndProc;
            pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

            // extract page
            memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
            propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
            propSheetPage.dwFlags = PSP_USETITLE;
            propSheetPage.pszTitle = PhApplicationName;
            propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG4);
            propSheetPage.hInstance = PhInstanceHandle;
            propSheetPage.pfnDlgProc = SetupInstallPropPage_WndProc;
            pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

            // error page
            memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
            propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
            propSheetPage.dwFlags = PSP_USETITLE;
            propSheetPage.pszTitle = PhApplicationName;
            propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_ERROR);
            propSheetPage.hInstance = PhInstanceHandle;
            propSheetPage.pfnDlgProc = SetupErrorPage_WndProc;
            pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

            PhModalPropertySheet(&propSheetHeader);
        }
        break;
    case SETUP_COMMAND_UNINSTALL:
        SetupShowUninstallDialog();
        break;
    case SETUP_COMMAND_UPDATE:
        SetupShowUpdateDialog();
        break;
    case SETUP_COMMAND_REPAIR:
        break;
    }

    return 0;
}