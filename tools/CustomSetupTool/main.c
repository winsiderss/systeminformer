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

#include "setup.h"

#define SETUP_CMD_INSTALL   1
#define SETUP_CMD_UNINSTALL 2
#define SETUP_CMD_UPDATE    3
#define SETUP_CMD_SILENT    4

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
            LONG dpiValue = PhGetWindowDpi(hwndDlg);

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
            case SetupCommandInstall:
                ShowWelcomePageDialog(context);
                break;
            case SetupCommandUninstall:
                ShowUninstallPageDialog(context);
                break;
            case SetupCommandUpdate:
                ShowUpdatePageDialog(context);
                break;
            }
        }
        break;
    }

    return S_OK;
}

INT SetupShowMessagePromptForLegacyVersion(
    VOID
    )
{
    INT result;
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };

    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
    config.dwCommonButtons = TDCBF_OK_BUTTON;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainIcon = TD_INFORMATION_ICON;
    config.pszMainInstruction = L"Hey there, before we continue...";
    config.pszContent =
        L"- Process Hacker was renamed System Informer.\n"
        L"- Process Hacker does not support Windows 10 or 11.\n"
        L"- Process Hacker will not be updated.\n"
        L"- Process Hacker will not be uninstalled.\n\n"
        L"This update will now install System Informer.\n\nPlease remember to uninstall Process Hacker. Thanks <3";
    config.cxWidth = 200;

    //PhShowInformation2(
    //    NULL,
    //    L"Process Hacker",
    //    L"%s",
    //    L"Process Hacker was renamed System Informer.\n"
    //    L"The legacy version of Process Hacker is no longer maintained and will not receive updates.\r\n\r\n"
    //    L"The updater is now installing System Informer. The Process Hacker installation must be manually uninstalled"
    //    );

    if (SUCCEEDED(TaskDialogIndirect(
        &config,
        &result,
        NULL,
        NULL
        )))
    {
        return result;
    }
    else
    {
        return -1;
    }
}

VOID SetupShowDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    PH_AUTO_POOL autoPool;
    BOOL value = FALSE;
    TASKDIALOGCONFIG config;

    assert(!Context->Silent);

    PhInitializeAutoPool(&autoPool);

    if (Context->SetupIsLegacyUpdate)
    {
        SetupShowMessagePromptForLegacyVersion();
    }

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.hInstance = PhInstanceHandle;
    config.pszContent = L"Initializing...";
    config.pfCallback = SetupTaskDialogBootstrapCallback;
    config.lpCallbackData = (LONG_PTR)Context;

    TaskDialogIndirect(&config, NULL, NULL, &value);

    if (value)
    {
        SetupExecuteApplication(Context);
    }

    PhDeleteAutoPool(&autoPool);
}

VOID SetupSilent(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    NTSTATUS status;

    assert(Context->Silent);

    if (PhGetOwnTokenAttributes().Elevated)
    {
        BOOLEAN start;

        switch (Context->SetupMode)
        {
        default:
        case SetupCommandInstall:
            status = SetupProgressThread(Context);
            start = TRUE;
            break;
        case SetupCommandUninstall:
            status = SetupUninstallBuild(Context);
            start = FALSE;
            break;
        case SetupCommandUpdate:
            status = SetupUpdateBuild(Context);
            start = TRUE;
            break;
        }

        if (start && NT_SUCCESS(status) && Context->ErrorCode == ERROR_SUCCESS)
            SetupExecuteApplication(Context);
    }
    else
    {
        PPH_STRING applicationFileName;
        PH_STRINGREF applicationCommandLine;

        if (!NT_SUCCESS(status = PhGetProcessCommandLineStringRef(&applicationCommandLine)))
        {
            Context->ErrorCode = WIN32_FROM_NTSTATUS(status);
            return;
        }

        if (!(applicationFileName = PhGetApplicationFileNameWin32()))
        {
            Context->ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            return;
        }

        if (!NT_SUCCESS(status = PhShellExecuteEx(
            NULL,
            PhGetString(applicationFileName),
            PhGetStringRefZ(&applicationCommandLine),
            NULL,
            SW_SHOW,
            PH_SHELL_EXECUTE_ADMIN,
            INFINITE,
            &Context->SubProcessHandle
            )))
        {
            Context->ErrorCode = WIN32_FROM_NTSTATUS(status);
            return;
        }
    }

    if (!NT_SUCCESS(status) && Context->ErrorCode == ERROR_SUCCESS)
        Context->ErrorCode = WIN32_FROM_NTSTATUS(status);
}

_Success_(return)
BOOLEAN PhParseKsiSettingsBlob( // copied from ksisup.c (dmex)
    _In_ PPH_STRING KsiSettingsBlob,
    _Out_ PPH_STRING* Directory,
    _Out_ PPH_STRING* ServiceName
    )
{
    BOOLEAN status = FALSE;
    PPH_STRING directory = NULL;
    PPH_STRING serviceName = NULL;
    PSTR string;
    ULONG stringLength;
    PPH_BYTES value;
    PVOID object;

    stringLength = (ULONG)KsiSettingsBlob->Length / sizeof(WCHAR) / 2;
    string = PhAllocateZero(stringLength + sizeof(UNICODE_NULL));

    if (PhHexStringToBufferEx(&KsiSettingsBlob->sr, stringLength, string))
    {
        value = PhCreateBytesEx(string, stringLength);

        if (object = PhCreateJsonParserEx(value, FALSE))
        {
            directory = PhGetJsonValueAsString(object, "KsiDirectory");
            serviceName = PhGetJsonValueAsString(object, "KsiServiceName");
            PhFreeJsonObject(object);
        }
        else
        {
            // legacy nightly
            directory = PhCreateStringEx((PWSTR)string, stringLength);
            serviceName = PhCreateString(L"KSystemInformer");

            if (!PhDoesDirectoryExistWin32(PhGetString(directory)))
            {
                PhClearReference(&directory);
                PhClearReference(&serviceName);
            }
        }

        PhDereferenceObject(value);
    }

    PhFree(string);

    if (!PhIsNullOrEmptyString(directory) &&
        !PhIsNullOrEmptyString(serviceName))
    {
        *Directory = directory;
        *ServiceName = serviceName;
        return TRUE;
    }

    PhClearReference(&serviceName);
    PhClearReference(&directory);
    return FALSE;
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
        switch (Option->Id)
        {
        case SETUP_CMD_INSTALL:
            context->SetupMode = SetupCommandInstall;
            break;
        case SETUP_CMD_UNINSTALL:
            context->SetupMode = SetupCommandUninstall;
            break;
        case SETUP_CMD_UPDATE:
            context->SetupMode = SetupCommandUpdate;
            break;
        case SETUP_CMD_SILENT:
            context->Silent = TRUE;
            break;
        }

        if (Option->Id == SETUP_CMD_UPDATE && Value)
        {
            PPH_STRING directory;
            PPH_STRING serviceName;

            if (PhParseKsiSettingsBlob(Value, &directory, &serviceName))
            {
                PhSwapReference(&context->SetupInstallPath, directory);
                PhSwapReference(&context->SetupServiceName, serviceName);

                if (!PhEndsWithStringRef(&context->SetupInstallPath->sr, &PhNtPathSeperatorString, FALSE))
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
            }
        }
    }

    return TRUE;
}

VOID SetupParseCommandLine(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    static PH_COMMAND_LINE_OPTION options[] =
    {
        { SETUP_CMD_INSTALL,   L"install",   NoArgumentType },
        { SETUP_CMD_UNINSTALL, L"uninstall", NoArgumentType },
        { SETUP_CMD_UPDATE,    L"update",    OptionalArgumentType },
        { SETUP_CMD_SILENT,    L"silent",    NoArgumentType },
    };
    PH_STRINGREF commandLine;

    if (NT_SUCCESS(PhGetProcessCommandLineStringRef(&commandLine)))
    {
        PhParseCommandLine(
            &commandLine,
            options,
            ARRAYSIZE(options),
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
    PPH_SETUP_CONTEXT context;

    if (!NT_SUCCESS(PhInitializePhLib(L"System Informer - Setup", Instance)))
        return EXIT_FAILURE;

    if (!SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
        return EXIT_FAILURE;

    SetupInitializeMutant();

    context = PhAllocateZero(sizeof(PH_SETUP_CONTEXT));

    if (PhIsNullOrEmptyString(context->SetupInstallPath))
    {
        context->SetupInstallPath = SetupFindInstallDirectory();
    }

    SetupParseCommandLine(context);

    if (context->Silent)
    {
        SetupSilent(context);
    }
    else
    {
        PhGuiSupportInitialization();

        SetupShowDialog(context);
    }

    if (context->SubProcessHandle)
    {
        PROCESS_BASIC_INFORMATION processInfo;

        PhWaitForSingleObject(context->SubProcessHandle, 0);
        PhGetProcessBasicInformation(context->SubProcessHandle, &processInfo);

        context->ErrorCode = WIN32_FROM_NTSTATUS(processInfo.ExitStatus);

        NtClose(context->SubProcessHandle);
    }

    return context->ErrorCode;
}
