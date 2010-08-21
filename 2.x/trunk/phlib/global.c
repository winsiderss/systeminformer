/*
 * Process Hacker - 
 *   global variables and initialization functions
 * 
 * Copyright (C) 2010 wj32
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

#define PHLIB_GLOBAL_PRIVATE
#include <ph.h>

VOID PhInitializeSecurity();
BOOLEAN PhInitializeSystem();
VOID PhInitializeSystemInformation();
VOID PhInitializeWindowsVersion();

HFONT PhApplicationFont;
PWSTR PhApplicationName = L"Application";
HFONT PhBoldListViewFont;
HFONT PhBoldMessageFont;
ULONG PhCurrentSessionId;
HANDLE PhCurrentTokenQueryHandle = NULL;
BOOLEAN PhElevated;
TOKEN_ELEVATION_TYPE PhElevationType;
PVOID PhHeapHandle;
HFONT PhIconTitleFont;
HINSTANCE PhInstanceHandle;
HANDLE PhKphHandle = NULL;
ULONG PhKphFeatures;
RTL_OSVERSIONINFOEXW PhOsVersion;
SYSTEM_BASIC_INFORMATION PhSystemBasicInformation;
ULONG WindowsVersion;

ACCESS_MASK ProcessQueryAccess;
ACCESS_MASK ProcessAllAccess;
ACCESS_MASK ThreadQueryAccess;
ACCESS_MASK ThreadSetAccess;
ACCESS_MASK ThreadAllAccess;

NTSTATUS PhInitializePhLib()
{
    PhHeapHandle = RtlCreateHeap(
        HEAP_GROWABLE | HEAP_CLASS_1,
        NULL,
        1024 * 1024, // 2 MB
        1024 * 1024, // 1 MB
        NULL,
        NULL
        );

    if (!PhHeapHandle)
        return STATUS_INSUFFICIENT_RESOURCES;

    PhInstanceHandle = (HINSTANCE)NtCurrentPeb()->ImageBaseAddress;

    PhInitializeWindowsVersion();

    if (!PhInitializeImports())
        return STATUS_UNSUCCESSFUL;

    PhInitializeSystemInformation();

    PhFastLockInitialization();
    if (!PhQueuedLockInitialization())
        return STATUS_UNSUCCESSFUL;
    if (!NT_SUCCESS(PhInitializeRef()))
        return STATUS_UNSUCCESSFUL;
    if (!PhInitializeBase())
        return STATUS_UNSUCCESSFUL;

    PhInitializeSecurity();

    if (!PhInitializeSystem())
        return STATUS_UNSUCCESSFUL;

    return STATUS_SUCCESS;
}

static VOID PhInitializeSecurity()
{
    HANDLE tokenHandle;

    PhElevated = TRUE;
    PhElevationType = TokenElevationTypeDefault;
    PhCurrentSessionId = NtCurrentPeb()->SessionId;

    if (NT_SUCCESS(PhOpenProcessToken(
        &tokenHandle,
        TOKEN_QUERY,
        NtCurrentProcess()
        )))
    {
        if (WINDOWS_HAS_UAC)
        {
            PhGetTokenIsElevated(tokenHandle, &PhElevated);
            PhGetTokenElevationType(tokenHandle, &PhElevationType);
        }

        // Just use the PEB.
        //PhGetTokenSessionId(tokenHandle, &PhCurrentSessionId);

        PhCurrentTokenQueryHandle = tokenHandle;
    }
}

BOOLEAN PhInitializeSystem()
{
    if (!PhIoSupportInitialization())
        return FALSE;
    if (!PhSymbolProviderInitialization())
        return FALSE;
    PhHandleInfoInitialization();

#ifdef DEBUG
    InitializeListHead(&PhDbgProviderListHead);
#endif

    return TRUE;
}

static VOID PhInitializeSystemInformation()
{
    if (!NT_SUCCESS(NtQuerySystemInformation(
        SystemBasicInformation,
        &PhSystemBasicInformation,
        sizeof(SYSTEM_BASIC_INFORMATION),
        NULL
        )))
    {
        PhShowWarning(
            NULL,
            L"Unable to query basic system information. "
            L"Some functionality may not work as expected."
            );
    }
}

static VOID PhInitializeWindowsVersion()
{
    RTL_OSVERSIONINFOEXW versionInfo;
    ULONG majorVersion;
    ULONG minorVersion;

    versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

    if (!NT_SUCCESS(RtlGetVersion((PRTL_OSVERSIONINFOW)&versionInfo)))
    {
        PhShowWarning(
            NULL,
            L"Unable to determine the Windows version. "
            L"Some functionality may not work as expected."
            );
        WindowsVersion = WINDOWS_NEW;
        return;
    }

    memcpy(&PhOsVersion, &versionInfo, sizeof(RTL_OSVERSIONINFOEXW));
    majorVersion = versionInfo.dwMajorVersion;
    minorVersion = versionInfo.dwMinorVersion;

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
