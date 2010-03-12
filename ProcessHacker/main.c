/*
 * Process Hacker - 
 *   main program
 * 
 * Copyright (C) 2009-2010 wj32
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

#define MAIN_PRIVATE
#include <phapp.h>
#include <treelist.h>
#include <kph.h>
#include <settings.h>

VOID PhpProcessStartupParameters();

PPH_STRING PhApplicationDirectory;
PPH_STRING PhApplicationFileName;
PPH_STRING PhSettingsFileName = NULL;
PH_STARTUP_PARAMETERS PhStartupParameters;
PWSTR PhWindowClassName = L"ProcessHacker";

PH_PROVIDER_THREAD PhPrimaryProviderThread;
PH_PROVIDER_THREAD PhSecondaryProviderThread;

COLORREF PhSysWindowColor;

static PPH_LIST DialogList;
static PH_AUTO_POOL BaseAutoPool;

INT WINAPI WinMain(
    __in HINSTANCE hInstance,
    __in_opt HINSTANCE hPrevInstance,
    __in LPSTR lpCmdLine,
    __in INT nCmdShow
    )
{
#ifdef DEBUG
    PHP_BASE_THREAD_DBG dbg;
#endif

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if (!NT_SUCCESS(PhInitializePhLib()))
        return 1;
    if (!PhInitializeAppSystem())
        return 1;

    PhRegisterWindowClass();
    PhInitializeCommonControls();

    PhApplicationFileName = PhGetApplicationFileName();
    PhApplicationDirectory = PhGetApplicationDirectory();

    // Just in case
    if (!PhApplicationFileName)
        PhApplicationFileName = PhCreateString(L"ProcessHacker.exe");
    if (!PhApplicationDirectory)
        PhApplicationDirectory = PhCreateString(L"");

    PhpProcessStartupParameters();

    // Load settings.
    {
        PhSettingsInitialization();

        if (!PhStartupParameters.NoSettings)
        {
            // Use the settings file name given in the command line, 
            // otherwise use the default location.

            if (PhStartupParameters.SettingsFileName)
            {
                // Get an absolute path now.
                PhSettingsFileName = PhGetFullPath(PhStartupParameters.SettingsFileName->Buffer, NULL);
            }

            if (!PhSettingsFileName)
            {
                PhSettingsFileName = PhGetKnownLocation(CSIDL_APPDATA, L"\\Process Hacker 2\\settings.xml");
            }

            if (PhSettingsFileName)
            {
                NTSTATUS status;

                status = PhLoadSettings(PhSettingsFileName->Buffer);

                // If we didn't find the file, it will be created. Otherwise, 
                // there was probably a parsing error and we don't want to 
                // change anything.
                if (!NT_SUCCESS(status) && status != STATUS_NO_SUCH_FILE)
                {
                    // Pretend we don't have a settings store so bad things 
                    // don't happen.
                    PhDereferenceObject(PhSettingsFileName);
                    PhSettingsFileName = NULL;
                }
            }
        }
    }

    if (
        PhStartupParameters.CommandMode &&
        PhStartupParameters.CommandType &&
        PhStartupParameters.CommandAction
        )
    {
        RtlExitUserProcess(PhCommandModeStart());
    }

    if (PhStartupParameters.RunAsServiceMode)
    {
        PhRunAsServiceStart();
        RtlExitUserProcess(STATUS_SUCCESS);
    }

    // Set priority to High.
    SetPriorityClass(NtCurrentProcess(), HIGH_PRIORITY_CLASS);

    // Activate a previous instance if required.
    if (!PhGetIntegerSetting(L"AllowMultipleInstances"))
        PhActivatePreviousInstance();

    if (PhGetIntegerSetting(L"EnableKph") && !PhStartupParameters.NoKph)
        PhInitializeKph();

#ifdef DEBUG
    dbg.ClientId.UniqueProcess = NtCurrentProcessId();
    dbg.ClientId.UniqueThread = NtCurrentThreadId();
    dbg.StartAddress = WinMain;
    dbg.Parameter = NULL;
    InsertTailList(&PhDbgThreadListHead, &dbg.ListEntry);
    TlsSetValue(PhDbgThreadDbgTlsIndex, &dbg);
#endif

    PhInitializeAutoPool(&BaseAutoPool);

    PhGuiSupportInitialization();
    PhTreeListInitialization();
    PhSecurityEditorInitialization();

    DialogList = PhCreateList(1);

    if (!PhMainWndInitialization(nCmdShow))
    {
        PhShowError(NULL, L"Unable to initialize the main window.");
        return 1;
    }

    PhDrainAutoPool(&BaseAutoPool);

    return PhMainMessageLoop();
}

INT PhMainMessageLoop()
{
    BOOL result;
    MSG message;
    HACCEL acceleratorTable;

    acceleratorTable = LoadAccelerators(PhInstanceHandle, MAKEINTRESOURCE(IDR_MAINWND_ACCEL));

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        BOOLEAN processed = FALSE;
        ULONG i;

        if (result == -1)
            return 1;

        if (
            message.hwnd == PhMainWndHandle ||
            IsChild(PhMainWndHandle, message.hwnd)
            )
        {
            if (TranslateAccelerator(PhMainWndHandle, acceleratorTable, &message))
                processed = TRUE;
        }

        for (i = 0; i < DialogList->Count; i++)
        {
            if (IsDialogMessage((HWND)DialogList->Items[i], &message))
            {
                processed = TRUE;
                break;
            }
        }

        if (!processed)
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&BaseAutoPool);
    }

    return (INT)message.wParam;
}

VOID PhRegisterDialog(
    __in HWND DialogWindowHandle
    )
{
    PhAddListItem(DialogList, (PVOID)DialogWindowHandle);
}

VOID PhUnregisterDialog(
    __in HWND DialogWindowHandle
    )
{
    ULONG indexOfDialog;

    indexOfDialog = PhIndexOfListItem(DialogList, (PVOID)DialogWindowHandle);

    if (indexOfDialog != -1)
        PhRemoveListItem(DialogList, indexOfDialog);
}

VOID PhActivatePreviousInstance()
{
    HWND hwnd;

    hwnd = FindWindow(PhWindowClassName, NULL);

    if (hwnd)
    {
        ULONG_PTR result;

        SendMessageTimeout(hwnd, WM_PH_ACTIVATE, 0, 0, SMTO_BLOCK, 5000, &result);

        if (result == PH_ACTIVATE_REPLY)
        {
            SetForegroundWindow(hwnd);
            RtlExitUserProcess(STATUS_SUCCESS);
        }
    }
}

VOID PhInitializeCommonControls()
{
    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC =
        ICC_LINK_CLASS |
        ICC_LISTVIEW_CLASSES |
        ICC_PROGRESS_CLASS |
        ICC_TAB_CLASSES
        ;

    InitCommonControlsEx(&icex);
}

HFONT PhpCreateFont(
    __in HWND hWnd,
    __in PWSTR Name,
    __in ULONG Size,
    __in ULONG Weight
    )
{
    return CreateFont(
        -MulDiv(Size, GetDeviceCaps(GetDC(hWnd), LOGPIXELSY), 72),
        0,
        0,
        0,
        Weight,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH,
        Name
        );
}

VOID PhInitializeFont(
    __in HWND hWnd
    )
{
    LOGFONT iconTitleFont;
    NONCLIENTMETRICS metrics = { sizeof(metrics) };
    BOOLEAN success;

    success = !!SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0);

    if (
        !(PhApplicationFont = PhpCreateFont(hWnd, L"Tahoma", 8, FW_NORMAL)) &&
        !(PhApplicationFont = PhpCreateFont(hWnd, L"Microsoft Sans Serif", 8, FW_NORMAL))
        )
    {
        if (success)
            PhApplicationFont = CreateFontIndirect(&metrics.lfMessageFont);
        else
            PhApplicationFont = NULL;
    }

    if (
        !(PhBoldMessageFont = PhpCreateFont(hWnd, L"Tahoma", 8, FW_BOLD)) &&
        !(PhBoldMessageFont = PhpCreateFont(hWnd, L"Microsoft Sans Serif", 8, FW_BOLD))
        )
    {
        if (success)
        {
            metrics.lfMessageFont.lfWeight = FW_BOLD;
            PhBoldMessageFont = CreateFontIndirect(&metrics.lfMessageFont);
        }
        else
        {
            PhBoldMessageFont = NULL;
        }
    }

    success = !!SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &iconTitleFont, 0);

    if (success)
        PhIconTitleFont = CreateFontIndirect(&iconTitleFont);
}

VOID PhInitializeKph()
{
    // KProcessHacker doesn't support 64-bit systems.
#ifdef _M_IX86
    static WCHAR kprocesshacker[] = L"kprocesshacker.sys";
    PPH_STRING kprocesshackerFileName;

    // Append kprocesshacker.sys to the application directory.
    kprocesshackerFileName = PhConcatStrings2(PhApplicationDirectory->Buffer, kprocesshacker);

    KphConnect2(&PhKphHandle, L"KProcessHacker", kprocesshackerFileName->Buffer);
    PhDereferenceObject(kprocesshackerFileName);
#endif

    if (PhKphHandle)
    {
        KphGetFeatures(PhKphHandle, &PhKphFeatures);
    }
}

BOOLEAN PhInitializeAppSystem()
{
    PhApplicationName = L"Process Hacker";

    if (!PhInitializeProcessProvider())
        return FALSE;
    if (!PhInitializeServiceProvider())
        return FALSE;
    if (!PhInitializeModuleProvider())
        return FALSE;
    if (!PhInitializeThreadProvider())
        return FALSE;
    if (!PhInitializeHandleProvider())
        return FALSE;
    if (!PhProcessPropInitialization())
        return FALSE;

    PhSetHandleClientIdFunction(PhGetClientIdName);

    return TRUE;
}

ATOM PhRegisterWindowClass()
{
    WNDCLASSEX wcex;

    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = 0;
    wcex.lpfnWndProc = PhMainWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = PhInstanceHandle;
    wcex.hIcon = LoadIcon(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    //wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MAINWND);
    wcex.lpszClassName = PhWindowClassName;
    wcex.hIconSm = (HICON)LoadImage(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER), IMAGE_ICON, 16, 16, 0);

    return RegisterClassEx(&wcex);
}

#define PH_ARG_SETTINGS 1
#define PH_ARG_NOSETTINGS 2
#define PH_ARG_SHOWVISIBLE 3
#define PH_ARG_SHOWHIDDEN 4
#define PH_ARG_COMMANDMODE 5
#define PH_ARG_COMMANDTYPE 6
#define PH_ARG_COMMANDOBJECT 7
#define PH_ARG_COMMANDACTION 8
#define PH_ARG_RUNASSERVICEMODE 9
#define PH_ARG_NOKPH 10

BOOLEAN NTAPI PhpCommandLineOptionCallback(
    __in_opt PPH_COMMAND_LINE_OPTION Option,
    __in_opt PPH_STRING Value,
    __in PVOID Context
    )
{
    if (Option)
    {
        switch (Option->Id)
        {
        case PH_ARG_SETTINGS:
            PhSwapReference(&PhStartupParameters.SettingsFileName, Value);
            break;
        case PH_ARG_NOSETTINGS:
            PhStartupParameters.NoSettings = TRUE;
            break;
        case PH_ARG_SHOWVISIBLE:
            PhStartupParameters.ShowVisible = TRUE;
            break;
        case PH_ARG_SHOWHIDDEN:
            PhStartupParameters.ShowHidden = TRUE;
            break;
        case PH_ARG_COMMANDMODE:
            PhStartupParameters.CommandMode = TRUE;
            break;
        case PH_ARG_COMMANDTYPE:
            PhSwapReference(&PhStartupParameters.CommandType, Value);
            break;
        case PH_ARG_COMMANDOBJECT:
            PhSwapReference(&PhStartupParameters.CommandObject, Value);
            break;
        case PH_ARG_COMMANDACTION:
            PhSwapReference(&PhStartupParameters.CommandAction, Value);
            break;
        case PH_ARG_RUNASSERVICEMODE:
            PhStartupParameters.RunAsServiceMode = TRUE;
            break;
        case PH_ARG_NOKPH:
            PhStartupParameters.NoKph = TRUE;
            break;
        }
    }

    return TRUE;
}

VOID PhpProcessStartupParameters()
{
    static PH_COMMAND_LINE_OPTION options[] =
    {
        { PH_ARG_SETTINGS, L"settings", MandatoryArgumentType },
        { PH_ARG_NOSETTINGS, L"nosettings", NoArgumentType },
        { PH_ARG_SHOWVISIBLE, L"v", NoArgumentType },
        { PH_ARG_SHOWHIDDEN, L"hide", NoArgumentType },
        { PH_ARG_COMMANDMODE, L"c", NoArgumentType },
        { PH_ARG_COMMANDTYPE, L"ctype", MandatoryArgumentType },
        { PH_ARG_COMMANDOBJECT, L"cobject", MandatoryArgumentType },
        { PH_ARG_COMMANDACTION, L"caction", MandatoryArgumentType },
        { PH_ARG_RUNASSERVICEMODE, L"ras", NoArgumentType },
        { PH_ARG_NOKPH, L"nokph", NoArgumentType }
    };
    PH_STRINGREF commandLine;

    commandLine.us = NtCurrentPeb()->ProcessParameters->CommandLine;

    memset(&PhStartupParameters, 0, sizeof(PH_STARTUP_PARAMETERS));

    if (!PhParseCommandLine(
        &commandLine,
        options,
        sizeof(options) / sizeof(PH_COMMAND_LINE_OPTION),
        PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS,
        PhpCommandLineOptionCallback,
        NULL
        ))
    {
        PhShowInformation(
            NULL,
            L"Command line options:\n\n"
            L"-c\n"
            L"-ctype command-type\n"
            L"-cobject command-object\n"
            L"-caction command-action\n"
            L"-hide\n"
            L"-nokph\n"
            L"-nosettings\n"
            L"-ras\n"
            L"-settings filename\n"
            L"-v\n"
            );
    }
}
