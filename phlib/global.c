/*
 * Process Hacker -
 *   global variables and initialization functions
 *
 * Copyright (C) 2010-2013 wj32
 * Copyright (C) 2017-2021 dmex
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

VOID PhInitializeSystemInformation(
    VOID
    );

VOID PhInitializeWindowsVersion(
    VOID
    );

BOOLEAN PhHeapInitialization(
    _In_opt_ SIZE_T HeapReserveSize,
    _In_opt_ SIZE_T HeapCommitSize
    );

PVOID PhInstanceHandle = NULL;
PWSTR PhApplicationName = NULL;
PHLIBAPI ULONG PhGlobalDpi = 96;
PVOID PhHeapHandle = NULL;
RTL_OSVERSIONINFOEXW PhOsVersion = { 0 };
PHLIBAPI SYSTEM_BASIC_INFORMATION PhSystemBasicInformation = { 0 };
ULONG WindowsVersion = WINDOWS_NEW;

// Internal data
#ifdef DEBUG
PHLIB_STATISTICS_BLOCK PhLibStatisticsBlock;
#endif

NTSTATUS PhInitializePhLib(
    VOID
    )
{
    return PhInitializePhLibEx(
        L"Application",
        ULONG_MAX, // all possible features
        NtCurrentPeb()->ImageBaseAddress,
        0,
        0
        );
}

NTSTATUS PhInitializePhLibEx(
    _In_ PWSTR ApplicationName,
    _In_ ULONG Flags,
    _In_ PVOID ImageBaseAddress,
    _In_opt_ SIZE_T HeapReserveSize,
    _In_opt_ SIZE_T HeapCommitSize
    )
{
    PhApplicationName = ApplicationName;
    PhInstanceHandle = ImageBaseAddress;

    PhInitializeWindowsVersion();
    PhInitializeSystemInformation();

    if (!PhHeapInitialization(HeapReserveSize, HeapCommitSize))
        return STATUS_UNSUCCESSFUL;

    if (!PhQueuedLockInitialization())
        return STATUS_UNSUCCESSFUL;

    if (!PhRefInitialization())
        return STATUS_UNSUCCESSFUL;

    if (!PhBaseInitialization())
        return STATUS_UNSUCCESSFUL;

    return STATUS_SUCCESS;
}

BOOLEAN PhIsExecutingInWow64(
    VOID
    )
{
#ifndef _WIN64
    static BOOLEAN valid = FALSE;
    static BOOLEAN isWow64 = FALSE;

    if (!valid)
    {
        PhGetProcessIsWow64(NtCurrentProcess(), &isWow64);
        MemoryBarrier();
        valid = TRUE;
    }

    return isWow64;
#else
    return FALSE;
#endif
}

VOID PhInitializeSystemInformation(
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

VOID PhInitializeWindowsVersion(
    VOID
    )
{
    RTL_OSVERSIONINFOEXW versionInfo;
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG buildVersion;

    memset(&versionInfo, 0, sizeof(RTL_OSVERSIONINFOEXW));
    versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

    if (!NT_SUCCESS(RtlGetVersion(&versionInfo)))
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
    // Windows 7, Windows Server 2008 R2
    else if (majorVersion == 6 && minorVersion == 1)
    {
        WindowsVersion = WINDOWS_7;
    }
    // Windows 8, Windows Server 2012
    else if (majorVersion == 6 && minorVersion == 2)
    {
        WindowsVersion = WINDOWS_8;
    }
    // Windows 8.1, Windows Server 2012 R2
    else if (majorVersion == 6 && minorVersion == 3)
    {
        WindowsVersion = WINDOWS_8_1;
    }
    // Windows 10, Windows Server 2016
    else if (majorVersion == 10 && minorVersion == 0)
    {
        if (buildVersion >= 22500)
        {
            WindowsVersion = WINDOWS_11_22H1;
        }
        else if (buildVersion >= 22000)
        {
            WindowsVersion = WINDOWS_11;
        }
        else if (buildVersion >= 19044)
        {
            WindowsVersion = WINDOWS_10_21H2;
        }
        else if (buildVersion >= 19043)
        {
            WindowsVersion = WINDOWS_10_21H1;
        }
        else if (buildVersion >= 19042)
        {
            WindowsVersion = WINDOWS_10_20H2;
        }
        else if (buildVersion >= 19041)
        {
            WindowsVersion = WINDOWS_10_20H1;
        }
        else if (buildVersion >= 18363)
        {
            WindowsVersion = WINDOWS_10_19H2;
        }
        else if (buildVersion >= 18362)
        {
            WindowsVersion = WINDOWS_10_19H1;
        }
        else if (buildVersion >= 17763)
        {
            WindowsVersion = WINDOWS_10_RS5;
        }
        else if (buildVersion >= 17134)
        {
            WindowsVersion = WINDOWS_10_RS4;
        }
        else if (buildVersion >= 16299)
        {
            WindowsVersion = WINDOWS_10_RS3;
        }
        else if (buildVersion >= 15063)
        {
            WindowsVersion = WINDOWS_10_RS2;
        }
        else if (buildVersion >= 14393)
        {
            WindowsVersion = WINDOWS_10_RS1;
        }
        else if (buildVersion >= 10586)
        {
            WindowsVersion = WINDOWS_10_TH2;
        }
        else if (buildVersion >= 10240)
        {
            WindowsVersion = WINDOWS_10;
        }
        else
        {
            WindowsVersion = WINDOWS_10;
        }
    }
    else
    {
        WindowsVersion = WINDOWS_NEW;
    }
}

BOOLEAN PhHeapInitialization(
    _In_opt_ SIZE_T HeapReserveSize,
    _In_opt_ SIZE_T HeapCommitSize
    )
{
    //if (WindowsVersion >= WINDOWS_8)
    //{
    //    PhHeapHandle = RtlCreateHeap(
    //        HEAP_GROWABLE | HEAP_CREATE_SEGMENT_HEAP | HEAP_CLASS_1,
    //        NULL,
    //        0,
    //        0,
    //        NULL,
    //        NULL
    //        );
    //}
    //
    //if (!PhHeapHandle)
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
            return FALSE;

#if (PHNT_VERSION >= PHNT_VISTA)
        RtlSetHeapInformation(
            PhHeapHandle,
            HeapCompatibilityInformation,
            &(ULONG){ HEAP_COMPATIBILITY_LFH },
            sizeof(ULONG)
            );
#endif
    }

    return TRUE;
}
