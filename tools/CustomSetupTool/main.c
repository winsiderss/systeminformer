/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex
 *
 */

#include <setup.h>

VOID PhAddDefaultSettings(
    VOID
    )
{
    NOTHING;
}

VOID PhUpdateCachedSettings(
    VOID
    )
{
    NOTHING;
}

LRESULT CALLBACK SetupTaskDialogSubclassProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_SETUP_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(hwndDlg, UCHAR_MAX)))
        return 0;

    oldWndProc = context->TaskDialogWndProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hwndDlg, UCHAR_MAX);
        }
        break;
    case SETUP_SHOWINSTALL:
        {
            ShowInstallPageDialog(context);
        }
        break;
    case SETUP_SHOWFINAL:
        {
            ShowCompletedPageDialog(context);
        }
        break;
    case SETUP_SHOWERROR:
        {
            ShowErrorPageDialog(context);
        }
        break;
    case SETUP_SHOWUNINSTALL:
        {
            ShowUninstallPageDialog(context);
        }
        break;
    case SETUP_SHOWUNINSTALLFINAL:
        {
            ShowUninstallCompletedPageDialog(context);
        }
        break;
    case SETUP_SHOWUNINSTALLERROR:
        {
            ShowUninstallErrorPageDialog(context);
        }
        break;

    case SETUP_SHOWUPDATE:
        {
            ShowUpdatePageDialog(context);
        }
        break;
    case SETUP_SHOWUPDATEFINAL:
        {
            //ShowUpdateCompletedPageDialog(context);

            SetupExecuteApplication(context);
            PostMessage(context->DialogHandle, TDM_CLICK_BUTTON, IDOK, 0);
        }
        break;
    case SETUP_SHOWUPDATEERROR:
        {
            ShowUpdateErrorPageDialog(context);
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwndDlg, uMsg, wParam, lParam);
}

HRESULT CALLBACK SetupTaskDialogBootstrapCallback(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_SETUP_CONTEXT context = (PPH_SETUP_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_CREATED:
        {
            LONG dpiValue;

            dpiValue = PhGetWindowDpi(hwndDlg);

            context->DialogHandle = hwndDlg;

            context->IconLargeHandle = PhLoadIcon(
                PhInstanceHandle,
                MAKEINTRESOURCE(IDI_ICON),
                PH_LOAD_ICON_SIZE_LARGE,
                PhGetSystemMetrics(SM_CXICON, dpiValue),
                PhGetSystemMetrics(SM_CYICON, dpiValue),
                dpiValue
                );
            context->IconSmallHandle = PhLoadIcon(
                PhInstanceHandle,
                MAKEINTRESOURCE(IDI_ICON),
                PH_LOAD_ICON_SIZE_LARGE,
                PhGetSystemMetrics(SM_CXSMICON, dpiValue),
                PhGetSystemMetrics(SM_CYSMICON, dpiValue),
                dpiValue
                );

            SendMessage(context->DialogHandle, WM_SETICON, ICON_SMALL, (LPARAM)context->IconSmallHandle);
            SendMessage(context->DialogHandle, WM_SETICON, ICON_BIG, (LPARAM)context->IconLargeHandle);

            context->TaskDialogWndProc = (WNDPROC)GetWindowLongPtr(hwndDlg, GWLP_WNDPROC);
            PhSetWindowContext(hwndDlg, UCHAR_MAX, context);
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)SetupTaskDialogSubclassProc);

            switch (context->SetupMode)
            {
            default:
            case SETUP_COMMAND_INSTALL:
                ShowWelcomePageDialog(context);
                break;
            case SETUP_COMMAND_UNINSTALL:
                ShowUninstallPageDialog(context);
                break;
            case SETUP_COMMAND_UPDATE:
                ShowUpdatePageDialog(context);
                break;
            }
        }
        break;
    }

    return S_OK;
}

VOID SetupShowDialog(
    VOID
    )
{
    PPH_SETUP_CONTEXT context;
    TASKDIALOGCONFIG config;
    PH_AUTO_POOL autoPool;
    BOOL value = FALSE;

    PhInitializeAutoPool(&autoPool);

    context = PhCreateAlloc(sizeof(PH_SETUP_CONTEXT));
    memset(context, 0, sizeof(PH_SETUP_CONTEXT));

    SetupParseCommandLine(context);

    if (PhIsNullOrEmptyString(context->SetupInstallPath))
    {
        context->SetupInstallPath = SetupFindInstallDirectory();
    }

    if (context->SetupIsLegacyUpdate)
    {
        PhShowInformation2(
            NULL,
            L"Process Hacker",
            L"%s",
            L"Process Hacker was renamed System Informer.\n"
            L"The legacy version of Process Hacker is no longer maintained and will not receive updates."
            );
    }

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.hInstance = PhInstanceHandle;
    config.pszContent = L"Initializing...";
    config.pfCallback = SetupTaskDialogBootstrapCallback;
    config.lpCallbackData = (LONG_PTR)context;

    TaskDialogIndirect(&config, NULL, NULL, &value);

    if (value)
    {
        SetupExecuteApplication(context);
    }

    PhDeleteAutoPool(&autoPool);
}

BOOLEAN NTAPI MainPropSheetCommandLineCallback(
    _In_opt_ PPH_COMMAND_LINE_OPTION Option,
    _In_opt_ PPH_STRING Value,
    _In_opt_ PVOID Context
    )
{
    PPH_SETUP_CONTEXT context = Context;

    if (!context)
        return FALSE;

    if (Option)
    {
        context->SetupMode = Option->Id;

        if (context->SetupMode == SETUP_COMMAND_UPDATE && Value)
        {
            PPH_STRING directory;
            PWSTR string;
            ULONG stringLength;

            stringLength = (ULONG)Value->Length / sizeof(WCHAR) / 2;
            string = PhAllocateZero(stringLength + sizeof(UNICODE_NULL));

            if (PhHexStringToBufferEx(&Value->sr, stringLength, string))
            {
                if (directory = PhGetFullPath(string, NULL))
                {
                    PhSwapReference(&context->SetupInstallPath, directory);

                    if (!PhEndsWithStringRef(&directory->sr, &PhNtPathSeperatorString, FALSE))
                    {
                        PhMoveReference(&context->SetupInstallPath, PhConcatStringRef2(&directory->sr, &PhNtPathSeperatorString));
                    }

                    // Check the path for the legacy directory name.
                    if (CheckApplicationInstallPathLegacy(context->SetupInstallPath))
                    {
                        // Update the directory path to the new directory.
                        PhMoveReference(&context->SetupInstallPath, SetupFindInstallDirectory());
                        context->SetupIsLegacyUpdate = TRUE;
                    }

                    PhDereferenceObject(directory);
                }
            }

            PhFree(string);
        }
    }
    else
    {
        // HACK: PhParseCommandLine requires the - symbol for commandline parameters
        // and we already support the -silent parameter however we need to maintain
        // compatibility with the legacy Inno Setup.
        //if (!PhIsNullOrEmptyString(Value))
        //{
        //    if (Value && PhEqualString2(Value, L"/silent", TRUE))
        //    {
        //        context->SetupMode = SETUP_COMMAND_SILENTINSTALL;
        //    }
        //}
    }

    if (PhIsNullOrEmptyString(context->SetupInstallPath))
    {
        context->SetupInstallPath = SetupFindInstallDirectory();
    }

    return TRUE;
}

VOID SetupParseCommandLine(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    static PH_COMMAND_LINE_OPTION options[] =
    {
        { SETUP_COMMAND_INSTALL, L"install", NoArgumentType },
        { SETUP_COMMAND_UNINSTALL, L"uninstall", NoArgumentType },
        { SETUP_COMMAND_UPDATE, L"update", OptionalArgumentType },
        { SETUP_COMMAND_UPDATE, L"silent", NoArgumentType },
        //{ SETUP_COMMAND_REPAIR, L"repair", NoArgumentType },
    };
    PH_STRINGREF commandLine;

    if (NT_SUCCESS(PhGetProcessCommandLineStringRef(&commandLine)))
    {
        PhParseCommandLine(
            &commandLine,
            options,
            RTL_NUMBER_OF(options),
            PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS | PH_COMMAND_LINE_IGNORE_FIRST_PART,
            MainPropSheetCommandLineCallback,
            Context
            );
    }
}

VOID SetupInitializeMutant(
    VOID
    )
{
    HANDLE mutantHandle;
    PPH_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectNameUs;
    PH_FORMAT format[2];

    PhInitFormatS(&format[0], L"SiSetupMutant_");
    PhInitFormatU(&format[1], HandleToUlong(NtCurrentProcessId()));

    objectName = PhFormat(format, 2, 16);
    PhStringRefToUnicodeString(&objectName->sr, &objectNameUs);

    InitializeObjectAttributes(
        &objectAttributes,
        &objectNameUs,
        OBJ_CASE_INSENSITIVE,
        PhGetNamespaceHandle(),
        NULL
        );

    NtCreateMutant(
        &mutantHandle,
        MUTANT_QUERY_STATE,
        &objectAttributes,
        TRUE
        );

    PhDereferenceObject(objectName);
}

INT WINAPI wWinMain(
    _In_ HINSTANCE Instance,
    _In_opt_ HINSTANCE PrevInstance,
    _In_ PWSTR CmdLine,
    _In_ INT CmdShow
    )
{
    if (!NT_SUCCESS(PhInitializePhLib(L"System Informer - Setup", Instance)))
        return EXIT_FAILURE;

    if (!SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
        return EXIT_FAILURE;

    SetupInitializeMutant();

    PhGuiSupportInitialization();

    SetupShowDialog();

    return EXIT_SUCCESS;
}
