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

PPH_STRING PhApplicationDirectory;
PPH_STRING PhApplicationFileName;
HFONT PhApplicationFont;
HANDLE PhHeapHandle;
HINSTANCE PhInstanceHandle;
HANDLE PhKphHandle;
PWSTR PhWindowClassName = L"ProcessHacker";
ULONG WindowsVersion;

PH_PROVIDER_THREAD PhPrimaryProviderThread;
PH_PROVIDER_THREAD PhSecondaryProviderThread;

ACCESS_MASK ProcessQueryAccess;
ACCESS_MASK ProcessAllAccess;
ACCESS_MASK ThreadQueryAccess;
ACCESS_MASK ThreadSetAccess;
ACCESS_MASK ThreadAllAccess;

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

    if (!PhInitializeSystem())
        return 1;

    {
        PhApplicationFileName = PhGetApplicationFileName();
        PhApplicationDirectory = PhGetApplicationDirectory();
    }

    PhInitializeKph();

    PhGuiSupportInitialization();

    if (!PhMainWndInitialization(nCmdShow))
    {
        PhShowError(NULL, L"Unable to initialize the main window.");
        return 1;
    }

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
    }

    return (INT)message.wParam;
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
}

BOOLEAN PhInitializeSystem()
{
    if (!NT_SUCCESS(PhInitializeRef()))
        return FALSE;
    PhFastLockInitialization();
    if (!PhInitializeBase())
        return FALSE;
    PhVerifyInitialization();
    if (!PhInitializeProcessProvider())
        return FALSE;
    if (!PhInitializeServiceProvider())
        return FALSE;
    if (!PhInitializeModuleProvider())
        return FALSE;
    if (!PhProcessPropInitialization())
        return FALSE;

    PhInitializeDosDeviceNames();
    PhRefreshDosDeviceNames();

    return TRUE;
}

ATOM PhRegisterWindowClass()
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = 0;
    wcex.lpfnWndProc = PhMainWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = PhInstanceHandle;
    wcex.hIcon = LoadIcon(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
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

    if (WindowsVersion >= WINDOWS_VISTA)
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
