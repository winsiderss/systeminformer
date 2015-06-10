/*
 * Process Hacker -
 *   global variables and initialization functions
 *
 * Copyright (C) 2010-2013 wj32
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

#include <ph.h>
#include <phintrnl.h>
#include <symprv.h>

VOID PhInitializeSecurity(
    _In_ ULONG Flags
    );

BOOLEAN PhInitializeSystem(
    _In_ ULONG Flags
    );

VOID PhInitializeSystemInformation(
    VOID
    );

VOID PhInitializeWindowsVersion(
    VOID
    );

PHLIBAPI PVOID PhLibImageBase;

PHLIBAPI PWSTR PhApplicationName = L"Application";
PHLIBAPI ULONG PhCurrentSessionId;
PHLIBAPI HANDLE PhCurrentTokenQueryHandle = NULL;
PHLIBAPI BOOLEAN PhElevated;
PHLIBAPI TOKEN_ELEVATION_TYPE PhElevationType;
PHLIBAPI PVOID PhHeapHandle;
PHLIBAPI RTL_OSVERSIONINFOEXW PhOsVersion;
PHLIBAPI SYSTEM_BASIC_INFORMATION PhSystemBasicInformation;
PHLIBAPI ULONG WindowsVersion;

PHLIBAPI ACCESS_MASK ProcessQueryAccess;
PHLIBAPI ACCESS_MASK ProcessAllAccess;
PHLIBAPI ACCESS_MASK ThreadQueryAccess;
PHLIBAPI ACCESS_MASK ThreadSetAccess;
PHLIBAPI ACCESS_MASK ThreadAllAccess;

// Internal data
#ifdef DEBUG
PHLIB_STATISTICS_BLOCK PhLibStatisticsBlock;
#endif

NTSTATUS PhInitializePhLib(
    VOID
    )
{
    return PhInitializePhLibEx(
        0xffffffff, // all possible features
        0,
        0
        );
}

NTSTATUS PhInitializePhLibEx(
    _In_ ULONG Flags,
    _In_opt_ SIZE_T HeapReserveSize,
    _In_opt_ SIZE_T HeapCommitSize
    )
{
    PhHeapHandle = RtlCreateHeap(
        HEAP_GROWABLE | HEAP_CLASS_1,
        NULL,
        HeapReserveSize ? HeapReserveSize : 2 * 1024 * 1024, // 2 MB
        HeapCommitSize ? HeapCommitSize : 1024 * 1024, // 1 MB
        NULL,
        NULL
        );

    if (!PhHeapHandle)
        return STATUS_INSUFFICIENT_RESOURCES;

    PhLibImageBase = NtCurrentPeb()->ImageBaseAddress;

    PhInitializeWindowsVersion();
    PhInitializeSystemInformation();

    if (!PhQueuedLockInitialization())
        return STATUS_UNSUCCESSFUL;

    if (!NT_SUCCESS(PhInitializeRef()))
        return STATUS_UNSUCCESSFUL;
    if (!PhInitializeBase(Flags))
        return STATUS_UNSUCCESSFUL;

    PhInitializeSecurity(Flags);

    if (!PhInitializeSystem(Flags))
        return STATUS_UNSUCCESSFUL;

    return STATUS_SUCCESS;
}

#ifndef _WIN64
BOOLEAN PhIsExecutingInWow64(
    VOID
    )
{
    static BOOLEAN valid = FALSE;
    static BOOLEAN isWow64;

    if (!valid)
    {
        PhGetProcessIsWow64(NtCurrentProcess(), &isWow64);
        MemoryBarrier();
        valid = TRUE;
    }

    return isWow64;
}
#endif

static VOID PhInitializeSecurity(
    _In_ ULONG Flags
    )
{
    HANDLE tokenHandle;

    PhElevated = TRUE;
    PhElevationType = TokenElevationTypeDefault;
    PhCurrentSessionId = NtCurrentPeb()->SessionId;

    if (Flags & PHLIB_INIT_TOKEN_INFO)
    {
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

            PhCurrentTokenQueryHandle = tokenHandle;
        }
    }
}

static BOOLEAN PhInitializeSystem(
    _In_ ULONG Flags
    )
{
    if (Flags & PHLIB_INIT_MODULE_IO_SUPPORT)
    {
        if (!PhIoSupportInitialization())
            return FALSE;
    }

    if (Flags & PHLIB_INIT_MODULE_SYMBOL_PROVIDER)
    {
        if (!PhSymbolProviderInitialization())
            return FALSE;
    }

    return TRUE;
}

static VOID PhInitializeSystemInformation(
    VOID
    )
{
    if (!NT_SUCCESS(NtQuerySystemInformation(
        SystemBasicInformation,
        &PhSystemBasicInformation,
        sizeof(SYSTEM_BASIC_INFORMATION),
        NULL
        )))
    {
        // Disabled message because it's not appropriate at this abstraction layer.
        //PhShowWarning(
        //    NULL,
        //    L"Unable to query basic system information. "
        //    L"Some functionality may not work as expected."
        //    );
    }
}

static VOID PhInitializeWindowsVersion(
    VOID
    )
{
    RTL_OSVERSIONINFOEXW versionInfo;
    ULONG majorVersion;
    ULONG minorVersion;

    versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

    if (!NT_SUCCESS(RtlGetVersion((PRTL_OSVERSIONINFOW)&versionInfo)))
    {
        //PhShowWarning(
        //    NULL,
        //    L"Unable to determine the Windows version. "
        //    L"Some functionality may not work as expected."
        //    );
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
    /* Windows 8 */
    else if (majorVersion == 6 && minorVersion == 2)
    {
        WindowsVersion = WINDOWS_8;
    }
    /* Windows 8.1 */
    else if (majorVersion == 6 && minorVersion == 3)
    {
        WindowsVersion = WINDOWS_8_1;
    }
    /* Windows 10 */
    else if (majorVersion == 10 && minorVersion == 0)
    {
        WindowsVersion = WINDOWS_10;
    }
    else if (majorVersion == 10 && minorVersion > 0 || majorVersion > 10)
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
