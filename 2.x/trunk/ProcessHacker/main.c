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
#include <kph.h>
#include <settings.h>

PPH_STRING PhApplicationDirectory;
PPH_STRING PhApplicationFileName;
HFONT PhApplicationFont;
HANDLE PhHeapHandle;
HINSTANCE PhInstanceHandle;
HANDLE PhKphHandle;
ULONG PhKphFeatures;
PPH_STRING PhSettingsFileName;
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

static PPH_AUTO_POOL BaseAutoPool;

INT WINAPI WinMain(
    __in HINSTANCE hInstance,
    __in HINSTANCE hPrevInstance,
    __in LPSTR lpCmdLine,
    __in INT nCmdShow
    )
{
    PhInstanceHandle = hInstance;

    PhHeapHandle = HeapCreate(0, 0, 0);

    if (!PhHeapHandle)
        return 1;

    PhInitializeWindowsVersion();
    PhRegisterWindowClass();
    PhInitializeCommonControls();

    if (!PhInitializeImports())
        return 1;

    PhInitializeSystemInformation();

    if (!NT_SUCCESS(PhInitializeRef()))
        return 1;
    PhFastLockInitialization();
    if (!PhInitializeBase())
        return 1;

    if (!PhInitializeSystem())
        return 1;

    PhApplicationFileName = PhGetApplicationFileName();
    PhApplicationDirectory = PhGetApplicationDirectory();

    // Load settings.
    {
        PhSettingsInitialization();

        PhSettingsFileName = PhGetKnownLocation(CSIDL_APPDATA, L"\\Process Hacker 2\\settings.xml");

        if (PhSettingsFileName)
            PhLoadSettings(PhSettingsFileName->Buffer);
    }

    // Activate a previous instance if required.
    if (!PhGetIntegerSetting(L"AllowMultipleInstances"))
        PhActivatePreviousInstance();

    PhInitializeKph();

    BaseAutoPool = PhCreateAutoPool();

    PhGuiSupportInitialization();

    if (!PhMainWndInitialization(nCmdShow))
    {
        PhShowError(NULL, L"Unable to initialize the main window.");
        return 1;
    }

    PhDrainAutoPool(BaseAutoPool);

    return PhMainMessageLoop();
}

INT PhMainMessageLoop()
{
    BOOL result;
    MSG message;
    HACCEL acceleratorTable;

    acceleratorTable = LoadAccelerators(PhInstanceHandle, MAKEINTRESOURCE(IDR_MAINWND));

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            return 1;

        if (!TranslateAccelerator(message.hwnd, acceleratorTable, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(BaseAutoPool);
    }

    return (INT)message.wParam;
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
            ExitProcess(0);
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

VOID PhInitializeFont(
    __in HWND hWnd
    )
{
    if (!(PhApplicationFont = CreateFont(
        -MulDiv(8, GetDeviceCaps(GetDC(hWnd), LOGPIXELSY), 72),
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH,
        L"Tahoma"
        )))
    {
        NONCLIENTMETRICS metrics;

        metrics.cbSize = sizeof(NONCLIENTMETRICS);

        if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0))
        {
            PhApplicationFont = CreateFontIndirect(&metrics.lfMessageFont);
        }
        else
        {
            PhApplicationFont = NULL;
        }
    }
}

VOID PhInitializeKph()
{
    // KProcessHacker doesn't support 64-bit systems.
#ifdef _M_IX86
    static WCHAR kprocesshacker[] = L"kprocesshacker.sys";
    PPH_STRING kprocesshackerFileName;

    // Append kprocesshacker.sys to the application directory.
    kprocesshackerFileName = PhConcatStrings2(PhApplicationDirectory->Buffer, kprocesshacker);

    PhKphHandle = NULL;
    KphConnect2(&PhKphHandle, L"KProcessHacker", kprocesshackerFileName->Buffer);
    PhDereferenceObject(kprocesshackerFileName);
#else
    PhKphHandle = NULL;
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
