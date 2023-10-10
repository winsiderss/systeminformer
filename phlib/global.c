/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2017-2023
 *
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
    VOID
    );

BOOLEAN PhInitializeProcessorInformation(
    VOID
    );

PVOID PhInstanceHandle = NULL;
PWSTR PhApplicationName = NULL;
PVOID PhHeapHandle = NULL;
RTL_OSVERSIONINFOEXW PhOsVersion = { 0 };
PHLIBAPI PH_SYSTEM_BASIC_INFORMATION PhSystemBasicInformation = { 0 };
PH_SYSTEM_PROCESSOR_INFORMATION PhSystemProcessorInformation = { 0 };
ULONG WindowsVersion = WINDOWS_NEW;
static WCHAR WindowsVersionStringBuffer[40] = { L'0', L'.', L'0', L'.', L'0', UNICODE_NULL };
PWSTR WindowsVersionString = WindowsVersionStringBuffer;
PWSTR WindowsVersionName = L"Windows";

// Internal data
#ifdef DEBUG
PHLIB_STATISTICS_BLOCK PhLibStatisticsBlock;
#endif

NTSTATUS PhInitializePhLib(
    _In_ PWSTR ApplicationName,
    _In_ PVOID ImageBaseAddress
    )
{
    PhApplicationName = ApplicationName;
    PhInstanceHandle = ImageBaseAddress;

    PhInitializeWindowsVersion();
    PhInitializeSystemInformation();

    if (!PhHeapInitialization())
        return STATUS_UNSUCCESSFUL;

    if (!PhQueuedLockInitialization())
        return STATUS_UNSUCCESSFUL;

    if (!PhRefInitialization())
        return STATUS_UNSUCCESSFUL;

    if (!PhBaseInitialization())
        return STATUS_UNSUCCESSFUL;

    PhInitializeProcessorInformation();

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
    SYSTEM_BASIC_INFORMATION basicInfo;

    memset(&basicInfo, 0, sizeof(SYSTEM_BASIC_INFORMATION));

    if (!NT_SUCCESS(NtQuerySystemInformation(
        SystemBasicInformation,
        &basicInfo,
        sizeof(SYSTEM_BASIC_INFORMATION),
        NULL
        )))
    {
        basicInfo.NumberOfProcessors = 1;
        basicInfo.NumberOfPhysicalPages = ULONG_MAX;
        basicInfo.AllocationGranularity = 0x10000;
        basicInfo.MaximumUserModeAddress = 0x10000;
        basicInfo.ActiveProcessorsAffinityMask = USHRT_MAX;
    }

    PhSystemBasicInformation.NumberOfProcessors = (USHORT)basicInfo.NumberOfProcessors;
    PhSystemBasicInformation.NumberOfPhysicalPages = basicInfo.NumberOfPhysicalPages;
    PhSystemBasicInformation.AllocationGranularity = basicInfo.AllocationGranularity;
    PhSystemBasicInformation.MaximumUserModeAddress = basicInfo.MaximumUserModeAddress;
    PhSystemBasicInformation.ActiveProcessorsAffinityMask = basicInfo.ActiveProcessorsAffinityMask;
}

VOID PhInitializeWindowsVersion(
    VOID
    )
{
    RTL_OSVERSIONINFOEXW versionInfo;
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG buildVersion;
    PH_FORMAT format[5];

    memset(&versionInfo, 0, sizeof(RTL_OSVERSIONINFOEXW));
    versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

    if (!NT_SUCCESS(RtlGetVersion(&versionInfo)))
    {
        WindowsVersion = WINDOWS_ANCIENT;
        WindowsVersionName = L"Windows";
        return;
    }

    PhInitFormatU(&format[0], versionInfo.dwMajorVersion);
    PhInitFormatC(&format[1], L'.');
    PhInitFormatU(&format[2], versionInfo.dwMinorVersion);
    PhInitFormatC(&format[3], L'.');
    PhInitFormatU(&format[4], versionInfo.dwBuildNumber);
    PhFormatToBuffer(
        format,
        RTL_NUMBER_OF(format),
        WindowsVersionStringBuffer,
        sizeof(WindowsVersionStringBuffer),
        NULL
        );

    memcpy(&PhOsVersion, &versionInfo, sizeof(RTL_OSVERSIONINFOEXW));
    majorVersion = versionInfo.dwMajorVersion;
    minorVersion = versionInfo.dwMinorVersion;
    buildVersion = versionInfo.dwBuildNumber;

    if (majorVersion == 6 && minorVersion < 1 || majorVersion < 6)
    {
        WindowsVersion = WINDOWS_ANCIENT;
        WindowsVersionName = L"Windows";
    }
    // Windows 7, Windows Server 2008 R2
    else if (majorVersion == 6 && minorVersion == 1)
    {
        WindowsVersion = WINDOWS_7;
        WindowsVersionName = L"Windows 7";
    }
    // Windows 8, Windows Server 2012
    else if (majorVersion == 6 && minorVersion == 2)
    {
        WindowsVersion = WINDOWS_8;
        WindowsVersionName = L"Windows 8";
    }
    // Windows 8.1, Windows Server 2012 R2
    else if (majorVersion == 6 && minorVersion == 3)
    {
        WindowsVersion = WINDOWS_8_1;
        WindowsVersionName = L"Windows 8.1";
    }
    // Windows 10, Windows Server 2016
    else if (majorVersion == 10 && minorVersion == 0)
    {
        if (buildVersion > 22631)
        {
            WindowsVersion = WINDOWS_NEW;
            WindowsVersionName = L"Windows";
        }
        else if (buildVersion >= 22631)
        {
            WindowsVersion = WINDOWS_11_23H2;
            WindowsVersionName = L"Windows 11 23H2";
        }
        else if (buildVersion >= 22621)
        {
            WindowsVersion = WINDOWS_11_22H2;
            WindowsVersionName = L"Windows 11 22H2";
        }
        else if (buildVersion >= 22000)
        {
            WindowsVersion = WINDOWS_11;
            WindowsVersionName = L"Windows 11";
        }
        else if (buildVersion >= 19045)
        {
            WindowsVersion = WINDOWS_10_22H2;
            WindowsVersionName = L"Windows 10 22H2";
        }
        else if (buildVersion >= 19044)
        {
            WindowsVersion = WINDOWS_10_21H2;
            WindowsVersionName = L"Windows 10 21H2";
        }
        else if (buildVersion >= 19043)
        {
            WindowsVersion = WINDOWS_10_21H1;
            WindowsVersionName = L"Windows 10 21H1";
        }
        else if (buildVersion >= 19042)
        {
            WindowsVersion = WINDOWS_10_20H2;
            WindowsVersionName = L"Windows 10 20H2";
        }
        else if (buildVersion >= 19041)
        {
            WindowsVersion = WINDOWS_10_20H1;
            WindowsVersionName = L"Windows 10 20H1";
        }
        else if (buildVersion >= 18363)
        {
            WindowsVersion = WINDOWS_10_19H2;
            WindowsVersionName = L"Windows 10 19H2";
        }
        else if (buildVersion >= 18362)
        {
            WindowsVersion = WINDOWS_10_19H1;
            WindowsVersionName = L"Windows 10 19H1";
        }
        else if (buildVersion >= 17763)
        {
            WindowsVersion = WINDOWS_10_RS5;
            WindowsVersionName = L"Windows 10 RS5";
        }
        else if (buildVersion >= 17134)
        {
            WindowsVersion = WINDOWS_10_RS4;
            WindowsVersionName = L"Windows 10 RS4";
        }
        else if (buildVersion >= 16299)
        {
            WindowsVersion = WINDOWS_10_RS3;
            WindowsVersionName = L"Windows 10 RS3";
        }
        else if (buildVersion >= 15063)
        {
            WindowsVersion = WINDOWS_10_RS2;
            WindowsVersionName = L"Windows 10 RS2";
        }
        else if (buildVersion >= 14393)
        {
            WindowsVersion = WINDOWS_10_RS1;
            WindowsVersionName = L"Windows 10 RS1";
        }
        else if (buildVersion >= 10586)
        {
            WindowsVersion = WINDOWS_10_TH2;
            WindowsVersionName = L"Windows 10 TH2";
        }
        else if (buildVersion >= 10240)
        {
            WindowsVersion = WINDOWS_10;
            WindowsVersionName = L"Windows 10";
        }
        else
        {
            WindowsVersion = WINDOWS_10;
            WindowsVersionName = L"Windows 10";
        }
    }
    else
    {
        WindowsVersion = WINDOWS_NEW;
        WindowsVersionName = L"Windows";
    }
}

BOOLEAN PhHeapInitialization(
    VOID
    )
{
#if defined(PH_DEBUG_HEAP)
    PhHeapHandle = RtlProcessHeap();
#else
    if (WindowsVersion >= WINDOWS_8)
    {
        PhHeapHandle = RtlCreateHeap(
            HEAP_GROWABLE | HEAP_CREATE_SEGMENT_HEAP | HEAP_CLASS_1,
            NULL,
            0,
            0,
            NULL,
            NULL
            );
    }

    if (!PhHeapHandle)
    {
        PhHeapHandle = RtlCreateHeap(
            HEAP_GROWABLE | HEAP_CLASS_1,
            NULL,
            2 * 1024 * 1024, // 2 MB
            1024 * 1024, // 1 MB
            NULL,
            NULL
            );

        if (!PhHeapHandle)
            return FALSE;

        RtlSetHeapInformation(
            PhHeapHandle,
            HeapCompatibilityInformation,
            &(ULONG){ HEAP_COMPATIBILITY_LFH },
            sizeof(ULONG)
            );
    }
#endif
    return TRUE;
}

BOOLEAN PhInitializeProcessorInformation(
    VOID
    )
{
    if (
        USER_SHARED_DATA->ActiveGroupCount == 1 && // optimization (dmex)
        USER_SHARED_DATA->ActiveProcessorCount > 0 &&
        USER_SHARED_DATA->ActiveProcessorCount <= MAXIMUM_PROC_PER_GROUP
        )
    {
        PhSystemProcessorInformation.SingleProcessorGroup = TRUE;
        PhSystemProcessorInformation.NumberOfProcessors = PhSystemBasicInformation.NumberOfProcessors;
        PhSystemProcessorInformation.NumberOfProcessorGroups = 1;
    }
    else
    {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processorInformation;
        ULONG processorInformationLength;
        USHORT numberOfProcessorGroups = 0;
        USHORT numberOfProcessors = 0;
        USHORT i;

        if (NT_SUCCESS(PhGetSystemLogicalProcessorInformation(
            RelationGroup,
            &processorInformation,
            &processorInformationLength
            )))
        {
            numberOfProcessorGroups = processorInformation->Group.ActiveGroupCount;

            for (i = 0; i < numberOfProcessorGroups; i++)
            {
                numberOfProcessors += processorInformation->Group.GroupInfo[i].ActiveProcessorCount;
            }

            if (numberOfProcessorGroups > 1)
            {
                PhSystemProcessorInformation.ActiveProcessorCount = PhAllocate(numberOfProcessorGroups * sizeof(USHORT));

                for (i = 0; i < numberOfProcessorGroups; i++)
                {
                    PhSystemProcessorInformation.ActiveProcessorCount[i] = processorInformation->Group.GroupInfo[i].ActiveProcessorCount;
                }
            }

            PhFree(processorInformation);
        }

        assert(numberOfProcessorGroups == USER_SHARED_DATA->ActiveGroupCount);
        assert(numberOfProcessors == USER_SHARED_DATA->ActiveProcessorCount);

        if (numberOfProcessorGroups > 1 && numberOfProcessors)
        {
            PhSystemProcessorInformation.SingleProcessorGroup = FALSE;
            PhSystemProcessorInformation.NumberOfProcessors = numberOfProcessors;
            PhSystemProcessorInformation.NumberOfProcessorGroups = numberOfProcessorGroups;
        }
        else
        {
            PhSystemProcessorInformation.SingleProcessorGroup = TRUE;
            PhSystemProcessorInformation.NumberOfProcessors = PhSystemBasicInformation.NumberOfProcessors;
            PhSystemProcessorInformation.NumberOfProcessorGroups = 1;
        }
    }

    return TRUE;
}
