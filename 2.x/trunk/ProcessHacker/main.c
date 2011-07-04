/*
 * Process Hacker - 
 *   main program
 * 
 * Copyright (C) 2009-2011 wj32
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

#define PH_MAIN_PRIVATE
#include <phapp.h>
#include <kphuser.h>
#include <phsvc.h>
#include <settings.h>
#include <hexedit.h>
#include <shlobj.h>

VOID PhActivatePreviousInstance();

VOID PhInitializeCommonControls();

VOID PhInitializeKph();

BOOLEAN PhInitializeAppSystem();

ATOM PhRegisterWindowClass();

VOID PhpInitializeSettings();

VOID PhpProcessStartupParameters();

VOID PhpEnablePrivileges();

PPH_STRING PhApplicationDirectory;
PPH_STRING PhApplicationFileName;
PHAPPAPI HFONT PhApplicationFont;
HFONT PhBoldListViewFont;
HFONT PhBoldMessageFont;
PPH_STRING PhCurrentUserName = NULL;
HFONT PhIconTitleFont;
HINSTANCE PhInstanceHandle;
PPH_STRING PhLocalSystemName = NULL;
BOOLEAN PhPluginsEnabled = FALSE;
PPH_STRING PhProcDbFileName = NULL;
PPH_STRING PhSettingsFileName = NULL;
PH_INTEGER_PAIR PhSmallIconSize = { 16, 16 };
PH_STARTUP_PARAMETERS PhStartupParameters;
PWSTR PhWindowClassName = L"ProcessHacker";

PH_PROVIDER_THREAD PhPrimaryProviderThread;
PH_PROVIDER_THREAD PhSecondaryProviderThread;

COLORREF PhSysWindowColor;

static PPH_LIST DialogList = NULL;
static PH_AUTO_POOL BaseAutoPool;

INT WINAPI WinMain(
    __in HINSTANCE hInstance,
    __in_opt HINSTANCE hPrevInstance,
    __in LPSTR lpCmdLine,
    __in INT nCmdShow
    )
{
    ULONG result;
#ifdef DEBUG
    PHP_BASE_THREAD_DBG dbg;
#endif

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    PhInstanceHandle = (HINSTANCE)NtCurrentPeb()->ImageBaseAddress;

    if (!NT_SUCCESS(PhInitializePhLib()))
        return 1;
    if (!PhInitializeAppSystem())
        return 1;

    PhRegisterWindowClass();
    PhInitializeCommonControls();

    if (PhCurrentTokenQueryHandle)
    {
        PTOKEN_USER tokenUser;

        if (NT_SUCCESS(PhGetTokenUser(PhCurrentTokenQueryHandle, &tokenUser)))
        {
            PhCurrentUserName = PhGetSidFullName(tokenUser->User.Sid, TRUE, NULL);
            PhFree(tokenUser);
        }
    }

    PhLocalSystemName = PhGetSidFullName(&PhSeLocalSystemSid, TRUE, NULL);

    // There has been a report of the above call failing.
    if (!PhLocalSystemName)
        PhLocalSystemName = PhCreateString(L"NT AUTHORITY\\SYSTEM");

    PhApplicationFileName = PhGetApplicationFileName();
    PhApplicationDirectory = PhGetApplicationDirectory();

    // Just in case
    if (!PhApplicationFileName)
        PhApplicationFileName = PhCreateString(L"ProcessHacker.exe");
    if (!PhApplicationDirectory)
        PhApplicationDirectory = PhReferenceEmptyString();

    PhpProcessStartupParameters();

    PhpInitializeSettings();

    PhpEnablePrivileges();

    if (PhStartupParameters.RunAsServiceMode)
    {
        RtlExitUserProcess(PhRunAsServiceStart(PhStartupParameters.RunAsServiceMode));
    }

    // Activate a previous instance if required.
    if (
        PhGetIntegerSetting(L"AllowOnlyOneInstance") &&
        !PhStartupParameters.NewInstance &&
        !PhStartupParameters.ShowOptions &&
        !PhStartupParameters.CommandMode &&
        !PhStartupParameters.PhSvc
        )
        PhActivatePreviousInstance();

    if (PhGetIntegerSetting(L"EnableKph") && !PhStartupParameters.NoKph)
        PhInitializeKph();

    if (
        PhStartupParameters.CommandMode &&
        PhStartupParameters.CommandType &&
        PhStartupParameters.CommandAction
        )
    {
        NTSTATUS status;

        status = PhCommandModeStart();

        if (!NT_SUCCESS(status) && !PhStartupParameters.Silent)
        {
            PhShowStatus(NULL, L"Unable to execute the command", status, 0);
        }

        RtlExitUserProcess(status);
    }

    if (PhStartupParameters.PhSvc)
    {
        RtlExitUserProcess(PhSvcMain(NULL, NULL, NULL));
    }

    // Create a mutant for the installer.
    {
        HANDLE mutantHandle;
        OBJECT_ATTRIBUTES oa;
        UNICODE_STRING mutantName;

        RtlInitUnicodeString(&mutantName, L"\\BaseNamedObjects\\ProcessHacker2Mutant");
        InitializeObjectAttributes(
            &oa,
            &mutantName,
            0,
            NULL,
            NULL
            );

        NtCreateMutant(&mutantHandle, MUTANT_ALL_ACCESS, &oa, FALSE);
    }

    // Set priority to High.
    {
        PROCESS_PRIORITY_CLASS priorityClass;

        priorityClass.Foreground = FALSE;
        priorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_HIGH;
        NtSetInformationProcess(NtCurrentProcess(), ProcessPriorityClass, &priorityClass, sizeof(PROCESS_PRIORITY_CLASS));
    }

#ifdef DEBUG
    dbg.ClientId = NtCurrentTeb()->ClientId;
    dbg.StartAddress = WinMain;
    dbg.Parameter = NULL;
    InsertTailList(&PhDbgThreadListHead, &dbg.ListEntry);
    TlsSetValue(PhDbgThreadDbgTlsIndex, &dbg);
#endif

    PhInitializeAutoPool(&BaseAutoPool);

    PhGuiSupportInitialization();
    PhTreeNewInitialization();
    PhGraphControlInitialization();
    PhHexEditInitialization();

    PhSmallIconSize.X = GetSystemMetrics(SM_CXSMICON);
    PhSmallIconSize.Y = GetSystemMetrics(SM_CYSMICON);

    if (PhStartupParameters.ShowOptions)
    {
        // Elevated options dialog for changing the value of Replace Task Manager with Process Hacker.
        PhShowOptionsDialog(PhStartupParameters.WindowHandle);
        RtlExitUserProcess(STATUS_SUCCESS);
    }

    PhPluginsEnabled = PhGetIntegerSetting(L"EnablePlugins") && !PhStartupParameters.NoPlugins;

    if (PhPluginsEnabled)
    {
        PhPluginsInitialization();
        PhLoadPlugins();
    }

    if (!PhMainWndInitialization(nCmdShow))
    {
        PhShowError(NULL, L"Unable to initialize the main window.");
        return 1;
    }

    PhDrainAutoPool(&BaseAutoPool);

    result = PhMainMessageLoop();
    RtlExitUserProcess(result);
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

        if (DialogList)
        {
            for (i = 0; i < DialogList->Count; i++)
            {
                if (IsDialogMessage((HWND)DialogList->Items[i], &message))
                {
                    processed = TRUE;
                    break;
                }
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
    if (!DialogList)
        DialogList = PhCreateList(2);

    PhAddItemList(DialogList, (PVOID)DialogWindowHandle);
}

VOID PhUnregisterDialog(
    __in HWND DialogWindowHandle
    )
{
    ULONG indexOfDialog;

    if (!DialogList)
        return;

    indexOfDialog = PhFindItemList(DialogList, (PVOID)DialogWindowHandle);

    if (indexOfDialog != -1)
        PhRemoveItemList(DialogList, indexOfDialog);
}

VOID PhApplyUpdateInterval(
    __in ULONG Interval
    )
{
    PhSetIntervalProviderThread(&PhPrimaryProviderThread, Interval);
    PhSetIntervalProviderThread(&PhSecondaryProviderThread, Interval);
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
        !(PhApplicationFont = PhpCreateFont(hWnd, L"Microsoft Sans Serif", 8, FW_NORMAL)) &&
        !(PhApplicationFont = PhpCreateFont(hWnd, L"Tahoma", 8, FW_NORMAL))
        )
    {
        if (success)
            PhApplicationFont = CreateFontIndirect(&metrics.lfMessageFont);
        else
            PhApplicationFont = NULL;
    }

    if (
        !(PhBoldMessageFont = PhpCreateFont(hWnd, L"Microsoft Sans Serif", 8, FW_BOLD)) &&
        !(PhBoldMessageFont = PhpCreateFont(hWnd, L"Tahoma", 8, FW_BOLD))
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
    static PH_STRINGREF kprocesshacker = PH_STRINGREF_INIT(L"kprocesshacker.sys");
    PPH_STRING kprocesshackerFileName;

    // Append kprocesshacker.sys to the application directory.
    kprocesshackerFileName = PhConcatStringRef2(&PhApplicationDirectory->sr, &kprocesshacker);

    KphConnect2(&PhKphHandle, L"KProcessHacker2", kprocesshackerFileName->Buffer);
    PhDereferenceObject(kprocesshackerFileName);

    if (PhKphHandle)
    {
        KphGetFeatures(PhKphHandle, &PhKphFeatures);
    }
}

BOOLEAN PhInitializeAppSystem()
{
    PhApplicationName = L"Process Hacker";

    if (!PhProcessProviderInitialization())
        return FALSE;
    if (!PhServiceProviderInitialization())
        return FALSE;
    if (!PhNetworkProviderInitialization())
        return FALSE;
    if (!PhModuleProviderInitialization())
        return FALSE;
    if (!PhThreadProviderInitialization())
        return FALSE;
    if (!PhHandleProviderInitialization())
        return FALSE;
    if (!PhMemoryProviderInitialization())
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

VOID PhpInitializeSettings()
{
    NTSTATUS status;

    PhSettingsInitialization();

    if (!PhStartupParameters.NoSettings)
    {
        static PH_STRINGREF settingsSuffix = PH_STRINGREF_INIT(L".settings.xml");
        PPH_STRING settingsFileName;

        // There are three possible locations for the settings file:
        // 1. The file name given in the command line.
        // 2. A file named ProcessHacker.exe.settings.xml in the program directory. (This changes 
        //    based on the executable file name.)
        // 3. The default location.

        // 1. File specified in command line
        if (PhStartupParameters.SettingsFileName)
        {
            // Get an absolute path now.
            PhSettingsFileName = PhGetFullPath(PhStartupParameters.SettingsFileName->Buffer, NULL);
        }

        // 2. File in program directory
        if (!PhSettingsFileName)
        {
            settingsFileName = PhConcatStringRef2(&PhApplicationFileName->sr, &settingsSuffix);

            if (RtlDoesFileExists_U(settingsFileName->Buffer))
            {
                PhSettingsFileName = settingsFileName;
            }
            else
            {
                PhDereferenceObject(settingsFileName);
            }
        }

        // 3. Default location
        if (!PhSettingsFileName)
        {
            PhSettingsFileName = PhGetKnownLocation(CSIDL_APPDATA, L"\\Process Hacker 2\\settings.xml");
        }

        if (PhSettingsFileName)
        {
            status = PhLoadSettings(PhSettingsFileName->Buffer);

            // If we didn't find the file, it will be created. Otherwise, 
            // there was probably a parsing error and we don't want to 
            // change anything.
            if (status == STATUS_FILE_CORRUPT_ERROR)
            {
                if (PhShowMessage(
                    NULL,
                    MB_ICONWARNING | MB_YESNO,
                    L"Process Hacker's settings file is corrupt. Do you want to reset it?\n"
                    L"If you select No, the settings system will not function properly."
                    ) == IDYES)
                {
                    HANDLE fileHandle;
                    IO_STATUS_BLOCK isb;
                    CHAR data[] = "<settings></settings>";

                    // This used to delete the file. But it's better to keep the file there 
                    // and overwrite it with some valid XML, especially with case (2) above.
                    if (NT_SUCCESS(PhCreateFileWin32(
                        &fileHandle,
                        PhSettingsFileName->Buffer,
                        FILE_GENERIC_WRITE,
                        0,
                        FILE_SHARE_READ | FILE_SHARE_DELETE,
                        FILE_OVERWRITE,
                        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                        )))
                    {
                        NtWriteFile(fileHandle, NULL, NULL, NULL, &isb, data, sizeof(data) - 1, NULL, NULL);
                        NtClose(fileHandle);
                    }
                }
                else
                {
                    // Pretend we don't have a settings store so bad things 
                    // don't happen.
                    PhDereferenceObject(PhSettingsFileName);
                    PhSettingsFileName = NULL;
                }
            }
        }
    }

    // Apply basic global settings.
    PhMaxSizeUnit = PhGetIntegerSetting(L"MaxSizeUnit");
}

#define PH_ARG_SETTINGS 1
#define PH_ARG_NOSETTINGS 2
#define PH_ARG_SHOWVISIBLE 3
#define PH_ARG_SHOWHIDDEN 4
#define PH_ARG_COMMANDMODE 5
#define PH_ARG_COMMANDTYPE 6
#define PH_ARG_COMMANDOBJECT 7
#define PH_ARG_COMMANDACTION 8
#define PH_ARG_COMMANDVALUE 9
#define PH_ARG_RUNASSERVICEMODE 10
#define PH_ARG_NOKPH 11
#define PH_ARG_INSTALLKPH 12
#define PH_ARG_UNINSTALLKPH 13
#define PH_ARG_DEBUG 14
#define PH_ARG_HWND 15
#define PH_ARG_POINT 16
#define PH_ARG_SHOWOPTIONS 17
#define PH_ARG_PHSVC 18
#define PH_ARG_NOPLUGINS 19
#define PH_ARG_NEWINSTANCE 20
#define PH_ARG_ELEVATE 21
#define PH_ARG_SILENT 22

BOOLEAN NTAPI PhpCommandLineOptionCallback(
    __in_opt PPH_COMMAND_LINE_OPTION Option,
    __in_opt PPH_STRING Value,
    __in_opt PVOID Context
    )
{
    ULONG64 integer;

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
        case PH_ARG_COMMANDVALUE:
            PhSwapReference(&PhStartupParameters.CommandValue, Value);
            break;
        case PH_ARG_RUNASSERVICEMODE:
            PhSwapReference(&PhStartupParameters.RunAsServiceMode, Value);
            break;
        case PH_ARG_NOKPH:
            PhStartupParameters.NoKph = TRUE;
            break;
        case PH_ARG_INSTALLKPH:
            PhStartupParameters.InstallKph = TRUE;
            break;
        case PH_ARG_UNINSTALLKPH:
            PhStartupParameters.UninstallKph = TRUE;
            break;
        case PH_ARG_DEBUG:
            PhStartupParameters.Debug = TRUE;
            break;
        case PH_ARG_HWND:
            if (PhStringToInteger64(&Value->sr, 16, &integer))
                PhStartupParameters.WindowHandle = (HWND)(ULONG_PTR)integer;
            break;
        case PH_ARG_POINT:
            {
                PH_STRINGREF xString;
                PH_STRINGREF yString;

                if (PhSplitStringRefAtChar(&Value->sr, ',', &xString, &yString))
                {
                    LONG64 x;
                    LONG64 y;

                    if (PhStringToInteger64(&xString, 10, &x) && PhStringToInteger64(&yString, 10, &y))
                    {
                        PhStartupParameters.Point.x = (LONG)x;
                        PhStartupParameters.Point.y = (LONG)y;
                    }
                }
            }
            break;
        case PH_ARG_SHOWOPTIONS:
            PhStartupParameters.ShowOptions = TRUE;
            break;
        case PH_ARG_PHSVC:
            PhStartupParameters.PhSvc = TRUE;
            break;
        case PH_ARG_NOPLUGINS:
            PhStartupParameters.NoPlugins = TRUE;
            break;
        case PH_ARG_NEWINSTANCE:
            PhStartupParameters.NewInstance = TRUE;
            break;
        case PH_ARG_ELEVATE:
            PhStartupParameters.Elevate = TRUE;
            break;
        case PH_ARG_SILENT:
            PhStartupParameters.Silent = TRUE;
            break;
        }
    }
    else
    {
        PPH_STRING upperValue;

        upperValue = PhDuplicateString(Value);
        PhUpperString(upperValue);

        if (PhFindStringInString(upperValue, 0, L"TASKMGR.EXE") != -1)
        {
            // User probably has Process Hacker replacing Task Manager. Force 
            // the main window to start visible.
            PhStartupParameters.ShowVisible = TRUE;
        }

        PhDereferenceObject(upperValue);
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
        { PH_ARG_COMMANDVALUE, L"cvalue", MandatoryArgumentType },
        { PH_ARG_RUNASSERVICEMODE, L"ras", MandatoryArgumentType },
        { PH_ARG_NOKPH, L"nokph", NoArgumentType },
        { PH_ARG_INSTALLKPH, L"installkph", NoArgumentType },
        { PH_ARG_UNINSTALLKPH, L"uninstallkph", NoArgumentType },
        { PH_ARG_DEBUG, L"debug", NoArgumentType },
        { PH_ARG_HWND, L"hwnd", MandatoryArgumentType },
        { PH_ARG_POINT, L"point", MandatoryArgumentType },
        { PH_ARG_SHOWOPTIONS, L"showoptions", NoArgumentType },
        { PH_ARG_PHSVC, L"phsvc", NoArgumentType },
        { PH_ARG_NOPLUGINS, L"noplugins", NoArgumentType },
        { PH_ARG_NEWINSTANCE, L"newinstance", NoArgumentType },
        { PH_ARG_ELEVATE, L"elevate", NoArgumentType },
        { PH_ARG_SILENT, L"s", NoArgumentType }
    };
    PH_STRINGREF commandLine;

    commandLine.us = NtCurrentPeb()->ProcessParameters->CommandLine;

    memset(&PhStartupParameters, 0, sizeof(PH_STARTUP_PARAMETERS));

    if (!PhParseCommandLine(
        &commandLine,
        options,
        sizeof(options) / sizeof(PH_COMMAND_LINE_OPTION),
        PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS | PH_COMMAND_LINE_IGNORE_FIRST_PART,
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
            L"-cvalue command-value\n"
            L"-debug\n"
            L"-elevate\n"
            L"-hide\n"
            L"-installkph\n"
            L"-newinstance\n"
            L"-nokph\n"
            L"-noplugins\n"
            L"-nosettings\n"
            L"-phsvc\n"
            L"-ras servicename\n"
            L"-s\n"
            L"-settings filename\n"
            L"-uninstallkph\n"
            L"-v\n"
            );
    }

    if (PhStartupParameters.InstallKph)
    {
        NTSTATUS status;
        PPH_STRING kprocesshackerFileName;

        kprocesshackerFileName = PhConcatStrings2(PhApplicationDirectory->Buffer, L"\\kprocesshacker.sys");

        status = KphInstall(L"KProcessHacker2", kprocesshackerFileName->Buffer);

        if (!NT_SUCCESS(status) && !PhStartupParameters.Silent)
            PhShowStatus(NULL, L"Unable to install KProcessHacker", status, 0);

        RtlExitUserProcess(status);
    }

    if (PhStartupParameters.UninstallKph)
    {
        NTSTATUS status;

        status = KphUninstall(L"KProcessHacker2");

        if (!NT_SUCCESS(status) && !PhStartupParameters.Silent)
            PhShowStatus(NULL, L"Unable to uninstall KProcessHacker", status, 0);

        RtlExitUserProcess(status);
    }

    if (PhStartupParameters.Elevate && !PhElevated)
    {
        PhShellProcessHacker(
            NULL,
            L"-v",
            SW_SHOW,
            PH_SHELL_EXECUTE_ADMIN,
            TRUE,
            0,
            NULL
            );
        RtlExitUserProcess(STATUS_SUCCESS);
    }

    if (PhStartupParameters.Debug)
    {
        // The symbol provider won't work if this is chosen.
        PhShowDebugConsole();
    }
}

VOID PhpEnablePrivileges()
{
    HANDLE tokenHandle;

    if (NT_SUCCESS(PhOpenProcessToken(
        &tokenHandle,
        TOKEN_ADJUST_PRIVILEGES,
        NtCurrentProcess()
        )))
    {
        CHAR privilegesBuffer[FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges) + sizeof(LUID_AND_ATTRIBUTES) * 7];
        PTOKEN_PRIVILEGES privileges;
        ULONG i;

        privileges = (PTOKEN_PRIVILEGES)privilegesBuffer;
        privileges->PrivilegeCount = 7;

        for (i = 0; i < privileges->PrivilegeCount; i++)
        {
            privileges->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
            privileges->Privileges[i].Luid.HighPart = 0;
        }

        privileges->Privileges[0].Luid.LowPart = SE_DEBUG_PRIVILEGE;
        privileges->Privileges[1].Luid.LowPart = SE_INC_BASE_PRIORITY_PRIVILEGE;
        privileges->Privileges[2].Luid.LowPart = SE_INC_WORKING_SET_PRIVILEGE;
        privileges->Privileges[3].Luid.LowPart = SE_LOAD_DRIVER_PRIVILEGE;
        privileges->Privileges[4].Luid.LowPart = SE_RESTORE_PRIVILEGE;
        privileges->Privileges[5].Luid.LowPart = SE_SHUTDOWN_PRIVILEGE;
        privileges->Privileges[6].Luid.LowPart = SE_TAKE_OWNERSHIP_PRIVILEGE;

        NtAdjustPrivilegesToken(
            tokenHandle,
            FALSE,
            privileges,
            0,
            NULL,
            NULL
            );

        NtClose(tokenHandle);
    }
}
