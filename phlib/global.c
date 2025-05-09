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
#include <phconsole.h>
#include <phintrnl.h>

#include <trace.h>

VOID PhInitializeRuntimeInformation(
    VOID
    );

VOID PhInitializeSystemInformation(
    VOID
    );

VOID PhInitializeWindowsInformation(
    VOID
    );

BOOLEAN PhHeapInitialization(
    VOID
    );

BOOLEAN PhInitializeProcessorInformation(
    VOID
    );

PVOID PhInstanceHandle = NULL;
PCWSTR PhApplicationName = NULL;
HANDLE PhHeapHandle = NULL;
RTL_OSVERSIONINFOEX PhOsVersion = { 0 };
PHLIBAPI PH_SYSTEM_BASIC_INFORMATION PhSystemBasicInformation = { 0 };
PH_SYSTEM_PROCESSOR_INFORMATION PhSystemProcessorInformation = { 0 };
ULONG WindowsVersion = WINDOWS_NEW;

// Internal data
#ifdef DEBUG
PHLIB_STATISTICS_BLOCK PhLibStatisticsBlock;
#endif

NTSTATUS PhInitializePhLib(
    _In_ PCWSTR ApplicationName,
    _In_ PVOID ImageBaseAddress
    )
{
    WPP_INIT_TRACING(ApplicationName);

    PhTraceInfo("%ls initializing", ApplicationName);

    PhApplicationName = ApplicationName;
    PhInstanceHandle = ImageBaseAddress;

    PhInitializeRuntimeInformation();
    PhInitializeWindowsInformation();
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
    static volatile BOOLEAN valid = FALSE;
    static BOOLEAN isWow64 = FALSE;

    if (!ReadBooleanAcquire(&valid))
    {
        PhGetProcessIsWow64(NtCurrentProcess(), &isWow64);
        WriteBooleanRelease(&valid, TRUE);
    }

    return isWow64;
#else
    return FALSE;
#endif
}

VOID PhInitializeRuntimeInformation(
    VOID
    )
{
#if defined _M_IX86
    // Enable SSE2 CRT support.
    _set_SSE2_enable(1);
#endif

    // Enable UTF8 CRT support.
    //_wsetlocale(LC_ALL, L".UTF8");
}

VOID PhInitializeSystemInformation(
    VOID
    )
{
    SYSTEM_BASIC_INFORMATION basicInfo = { 0 };

    PhSystemBasicInformation.PageSize = PAGE_SIZE;
    PhSystemBasicInformation.NumberOfProcessors = 1;
    PhSystemBasicInformation.NumberOfPhysicalPages = ULONG_MAX;
    PhSystemBasicInformation.MaximumTimerResolution = 0x2625A;
    PhSystemBasicInformation.AllocationGranularity = 0x10000;
    PhSystemBasicInformation.MaximumUserModeAddress = 0x10000;
    PhSystemBasicInformation.ActiveProcessorsAffinityMask = USHRT_MAX;

    if (NT_SUCCESS(NtQuerySystemInformation(
        SystemBasicInformation,
        &basicInfo,
        sizeof(SYSTEM_BASIC_INFORMATION),
        NULL
        )))
    {
        PhSystemBasicInformation.PageSize = (USHORT)basicInfo.PageSize;
        PhSystemBasicInformation.NumberOfProcessors = (USHORT)basicInfo.NumberOfProcessors;
        PhSystemBasicInformation.NumberOfPhysicalPages = basicInfo.NumberOfPhysicalPages;
        PhSystemBasicInformation.MaximumTimerResolution = basicInfo.TimerResolution;
        PhSystemBasicInformation.AllocationGranularity = basicInfo.AllocationGranularity;
        PhSystemBasicInformation.MaximumUserModeAddress = basicInfo.MaximumUserModeAddress;
        PhSystemBasicInformation.ActiveProcessorsAffinityMask = basicInfo.ActiveProcessorsAffinityMask;
    }
}

VOID PhInitializeWindowsInformation(
    VOID
    )
{
    RTL_OSVERSIONINFOEX versionInfo;
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG buildVersion;

    memset(&versionInfo, 0, sizeof(RTL_OSVERSIONINFOEX));
    versionInfo.OSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEX);

    if (!NT_SUCCESS(RtlGetVersion(&versionInfo)))
    {
        WindowsVersion = WINDOWS_ANCIENT;
        return;
    }

    memcpy(&PhOsVersion, &versionInfo, sizeof(RTL_OSVERSIONINFOEX));
    majorVersion = versionInfo.MajorVersion;
    minorVersion = versionInfo.MinorVersion;
    buildVersion = versionInfo.BuildNumber;

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
        if (buildVersion > 26100)
        {
            WindowsVersion = WINDOWS_NEW;
        }
        else if (buildVersion >= 26100)
        {
            WindowsVersion = WINDOWS_11_24H2;
        }
        else if (buildVersion >= 22631)
        {
            WindowsVersion = WINDOWS_11_23H2;
        }
        else if (buildVersion >= 22621)
        {
            WindowsVersion = WINDOWS_11_22H2;
        }
        else if (buildVersion >= 22000)
        {
            WindowsVersion = WINDOWS_11;
        }
        else if (buildVersion >= 19045)
        {
            WindowsVersion = WINDOWS_10_22H2;
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

        const ULONG defaultHeapCompatibilityMode = HEAP_COMPATIBILITY_MODE_LFH;
        RtlSetHeapInformation(
            PhHeapHandle,
            HeapCompatibilityInformation,
            &defaultHeapCompatibilityMode,
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
        PhSystemProcessorInformation.ActiveProcessorsAffinityMasks = &PhSystemBasicInformation.ActiveProcessorsAffinityMask;
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
                PhSystemProcessorInformation.ActiveProcessorsAffinityMasks = PhAllocate(numberOfProcessorGroups * sizeof(KAFFINITY));

                for (i = 0; i < numberOfProcessorGroups; i++)
                {
                    PhSystemProcessorInformation.ActiveProcessorCount[i] = processorInformation->Group.GroupInfo[i].ActiveProcessorCount;
                    PhSystemProcessorInformation.ActiveProcessorsAffinityMasks[i] = processorInformation->Group.GroupInfo[i].ActiveProcessorMask;
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
            PhSystemProcessorInformation.ActiveProcessorsAffinityMasks = PhAllocate(sizeof(KAFFINITY));
            PhSystemProcessorInformation.ActiveProcessorsAffinityMasks[0] = PhSystemBasicInformation.ActiveProcessorsAffinityMask;
        }
    }

    return TRUE;
}

_Use_decl_annotations_
VOID PhExitApplication(
    _In_opt_ NTSTATUS Status
    )
{
    PhTraceInfo("%ls exiting: %!STATUS!", PhApplicationName, Status);

    WPP_CLEANUP();

#define WORKAROUND_CRTBUG_EXITPROCESS
#ifdef WORKAROUND_CRTBUG_EXITPROCESS
    //HANDLE standardHandle;
    //
    //if (standardHandle = PhGetStdHandle(STD_OUTPUT_HANDLE))
    //{
    //    DEVICE_TYPE deviceType;
    //
    //    if (NT_SUCCESS(PhGetDeviceType(NtCurrentProcess(), standardHandle, &deviceType)))
    //    {
    //        if (deviceType == FILE_DEVICE_CONSOLE)
    //        {
    //            FlushFileBuffers(standardHandle);
    //        }
    //    }
    //}

    NtTerminateProcess(NtCurrentProcess(), Status);
#else
    RtlExitUserProcess(Status);
#endif
}
