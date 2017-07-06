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

#include <filestream.h>
#include <phintrnl.h>
#include <symprv.h>

BOOLEAN PhInitializeSystem(
    _In_ ULONG Flags
    );

VOID PhInitializeSystemInformation(
    VOID
    );

VOID PhInitializeWindowsVersion(
    VOID
    );

PHLIBAPI PVOID PhInstanceHandle;
PHLIBAPI PWSTR PhApplicationName = L"Application";
PHLIBAPI ULONG PhGlobalDpi = 96;
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
        NtCurrentPeb()->ImageBaseAddress,
        0,
        0
        );
}

NTSTATUS PhInitializePhLibEx(
    _In_ ULONG Flags,
    _In_ PVOID ImageBaseAddress,
    _In_opt_ SIZE_T HeapReserveSize,
    _In_opt_ SIZE_T HeapCommitSize
    )
{
    PhInstanceHandle = ImageBaseAddress;
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

    PhInitializeWindowsVersion();
    PhInitializeSystemInformation();

    if (!PhQueuedLockInitialization())
        return STATUS_UNSUCCESSFUL;

    if (!NT_SUCCESS(PhRefInitialization()))
        return STATUS_UNSUCCESSFUL;
    if (!PhBaseInitialization())
        return STATUS_UNSUCCESSFUL;

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

static BOOLEAN PhInitializeSystem(
    _In_ ULONG Flags
    )
{
    if (Flags & PHLIB_INIT_MODULE_FILE_STREAM)
    {
        if (!PhFileStreamInitialization())
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
    NtQuerySystemInformation(
        SystemBasicInformation,
        &PhSystemBasicInformation,
        sizeof(SYSTEM_BASIC_INFORMATION),
        NULL
        );
}

static VOID PhInitializeWindowsVersion(
    VOID
    )
{
    RTL_OSVERSIONINFOEXW versionInfo;
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG buildVersion;

    versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

    if (!NT_SUCCESS(RtlGetVersion((PRTL_OSVERSIONINFOW)&versionInfo)))
    {
        WindowsVersion = WINDOWS_NEW;
        return;
    }

    memcpy(&PhOsVersion, &versionInfo, sizeof(RTL_OSVERSIONINFOEXW));
    majorVersion = versionInfo.dwMajorVersion;
    minorVersion = versionInfo.dwMinorVersion;
    buildVersion = versionInfo.dwBuildNumber;

    if (majorVersion == 6 && minorVersion < 1 || majorVersion < 6)
    {
        WindowsVersion = WINDOWS_ANCIENT;
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
        switch (buildVersion)
        {
        case 10240:
            WindowsVersion = WINDOWS_10;
            break;
        case 10586:
            WindowsVersion = WINDOWS_10_TH2;
            break;
        case 14393:
            WindowsVersion = WINDOWS_10_RS1;
            break;
        case 15063:
            WindowsVersion = WINDOWS_10_RS2;
            break;
        default:
            WindowsVersion = WINDOWS_NEW;
            break;
        }
    }
    else if (majorVersion == 10 && minorVersion > 0 || majorVersion > 10)
    {
        WindowsVersion = WINDOWS_NEW;
    }

    ProcessQueryAccess = PROCESS_QUERY_LIMITED_INFORMATION;
    ProcessAllAccess = STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1fff;
    ThreadQueryAccess = THREAD_QUERY_LIMITED_INFORMATION;
    ThreadSetAccess = THREAD_SET_LIMITED_INFORMATION;
    ThreadAllAccess = STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xfff;
}
