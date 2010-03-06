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
#include <phgui.h>
#include <treelist.h>
#include <kph.h>
#include <settings.h>

VOID PhpProcessStartupParameters();

PPH_STRING PhApplicationDirectory;
PPH_STRING PhApplicationFileName;
HFONT PhApplicationFont;
HFONT PhBoldListViewFont;
HFONT PhBoldMessageFont;
BOOLEAN PhElevated;
HANDLE PhHeapHandle;
HFONT PhIconTitleFont;
HINSTANCE PhInstanceHandle;
HANDLE PhKphHandle = NULL;
ULONG PhKphFeatures;
PPH_STRING PhSettingsFileName = NULL;
PH_STARTUP_PARAMETERS PhStartupParameters;
SYSTEM_BASIC_INFORMATION PhSystemBasicInformation;
PWSTR PhWindowClassName = L"ProcessHacker";
ULONG WindowsVersion;

PH_PROVIDER_THREAD PhPrimaryProviderThread;
PH_PROVIDER_THREAD PhSecondaryProviderThread;

ACCESS_MASK ProcessQueryAccess;
ACCESS_MASK ProcessAllAccess;
ACCESS_MASK ThreadQueryAccess;
ACCESS_MASK ThreadSetAccess;
ACCESS_MASK ThreadAllAccess;

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

    PhInstanceHandle = hInstance;

    PhHeapHandle = HeapCreate(0, 0, 0);

    if (!PhHeapHandle)
        return 1;

    PhInitializeWindowsVersion();
    PhRegisterWindowClass();
    PhInitializeCommonControls();
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if (!PhInitializeImports())
        return 1;

    PhInitializeSystemInformation();

    PhFastLockInitialization();
    if (!PhQueuedLockInitialization())
        return 1;
    if (!NT_SUCCESS(PhInitializeRef()))
        return 1;
    if (!PhInitializeBase())
        return 1;

    if (!PhInitializeSystem())
        return 1;

    PhApplicationFileName = PhGetApplicationFileName();
    PhApplicationDirectory = PhGetApplicationDirectory();

    // Just in case
    if (!PhApplicationFileName)
        PhApplicationFileName = PhCreateString(L"ProcessHacker.exe");
    if (!PhApplicationDirectory)
        PhApplicationDirectory = PhCreateString(L"");

    {
        HANDLE tokenHandle;

        PhElevated = TRUE;

        if (WINDOWS_HAS_UAC &&
            NT_SUCCESS(PhOpenProcessToken(
            &tokenHandle,
            TOKEN_QUERY,
            NtCurrentProcess()
            )))
        {
            PhGetTokenIsElevated(tokenHandle, &PhElevated);
            NtClose(tokenHandle);
        }
    }

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

#ifdef DEBUG
    InitializeListHead(&PhDbgProviderListHead);
    PhInitializeFastLock(&PhDbgProviderListLock);
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

BOOLEAN PhInitializeSystem()
{
    PhVerifyInitialization();
    if (!PhSymbolProviderInitialization())
        return FALSE;
    if (!PhInitializeProcessProvider())
        return FALSE;
    if (!PhInitializeServiceProvider())
        return FALSE;
    if (!PhInitializeModuleProvider())
        return FALSE;
    if (!PhInitializeThreadProvider())
        return FALSE;
    PhHandleInfoInitialization();
    if (!PhInitializeHandleProvider())
        return FALSE;
    if (!PhProcessPropInitialization())
        return FALSE;

    PhInitializeDosDeviceNames();
    PhRefreshDosDeviceNames();

    return TRUE;
}

VOID PhInitializeSystemInformation()
{
    ULONG returnLength;

    if (!NT_SUCCESS(NtQuerySystemInformation(
        SystemBasicInformation,
        &PhSystemBasicInformation,
        sizeof(SYSTEM_BASIC_INFORMATION),
        &returnLength
        )))
    {
        SYSTEM_INFO systemInfo;

        GetSystemInfo(&systemInfo);

        PhSystemBasicInformation.Reserved = systemInfo.dwOemId;
        PhSystemBasicInformation.PageSize = systemInfo.dwPageSize;
        PhSystemBasicInformation.MinimumUserModeAddress = (ULONG_PTR)systemInfo.lpMinimumApplicationAddress;
        PhSystemBasicInformation.MaximumUserModeAddress = (ULONG_PTR)systemInfo.lpMaximumApplicationAddress;
        PhSystemBasicInformation.ActiveProcessorsAffinityMask = systemInfo.dwActiveProcessorMask;
        PhSystemBasicInformation.NumberOfProcessors = (CCHAR)systemInfo.dwNumberOfProcessors;
        PhSystemBasicInformation.AllocationGranularity = systemInfo.dwAllocationGranularity;
    }
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

VOID PhInitializeWindowsVersion()
{
    OSVERSIONINFOEX version;
    ULONG majorVersion;
    ULONG minorVersion;

    version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((POSVERSIONINFO)&version);
    majorVersion = version.dwMajorVersion;
    minorVersion = version.dwMinorVersion;

    if (majorVersion == 5 && minorVersion < 1 || majorVersion < 5)
    {
        WindowsVersion = WINDOWS_ANCIENT;
    }
    /* Windows XP */
    else if (majorVersion == 5 && minorVersion == 1)
    {
        WindowsVersion = WINDOWS_XP;
    }
    /* Windows Server 2003 */
    else if (majorVersion == 5 && minorVersion == 2)
    {
        WindowsVersion = WINDOWS_SERVER_2003;
    }
    /* Windows Vista, Windows Server 2008 */
    else if (majorVersion == 6 && minorVersion == 0)
    {
        WindowsVersion = WINDOWS_VISTA;
    }
    /* Windows 7, Windows Server 2008 R2 */
    else if (majorVersion == 6 && minorVersion == 1)
    {
        WindowsVersion = WINDOWS_7;
    }
    else if (majorVersion == 6 && minorVersion > 1 || majorVersion > 6)
    {
        WindowsVersion = WINDOWS_NEW;
    }

    if (WINDOWS_HAS_LIMITED_ACCESS)
    {
        ProcessQueryAccess = PROCESS_QUERY_LIMITED_INFORMATION;
        ProcessAllAccess = STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1fff;
        ThreadQueryAccess = THREAD_QUERY_LIMITED_INFORMATION;
        ThreadSetAccess = THREAD_SET_LIMITED_INFORMATION;
        ThreadAllAccess = STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xfff;
    }
    else
    {
        ProcessQueryAccess = PROCESS_QUERY_INFORMATION;
        ProcessAllAccess = STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xfff;
        ThreadQueryAccess = THREAD_QUERY_INFORMATION;
        ThreadSetAccess = THREAD_SET_INFORMATION;
        ThreadAllAccess = STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3ff;
    }
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
