/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2025
 *
 */

#ifndef _PH_PHCONFIG_H
#define _PH_PHCONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

EXTERN_C PVOID PhInstanceHandle;
EXTERN_C PCWSTR PhApplicationName;
EXTERN_C PVOID PhHeapHandle;
EXTERN_C RTL_OSVERSIONINFOEXW PhOsVersion;
EXTERN_C ULONG WindowsVersion;
EXTERN_C PCWSTR WindowsVersionString;
EXTERN_C PCWSTR WindowsVersionName;

#define WINDOWS_ANCIENT 0
#define WINDOWS_XP 51 // August, 2001
#define WINDOWS_SERVER_2003 52 // April, 2003
#define WINDOWS_VISTA 60 // November, 2006
#define WINDOWS_7 61 // July, 2009
#define WINDOWS_8 62 // August, 2012
#define WINDOWS_8_1 63 // August, 2013
#define WINDOWS_10 100 // TH1 // July, 2015
#define WINDOWS_10_TH2 101 // November, 2015
#define WINDOWS_10_RS1 102 // August, 2016
#define WINDOWS_10_RS2 103 // April, 2017
#define WINDOWS_10_RS3 104 // October, 2017
#define WINDOWS_10_RS4 105 // April, 2018
#define WINDOWS_10_RS5 106 // November, 2018
#define WINDOWS_10_19H1 107 // May, 2019
#define WINDOWS_10_19H2 108 // November, 2019
#define WINDOWS_10_20H1 109 // May, 2020
#define WINDOWS_10_20H2 110 // October, 2020
#define WINDOWS_10_21H1 111 // May, 2021
#define WINDOWS_10_21H2 112 // November, 2021
#define WINDOWS_10_22H2 113 // October, 2022
#define WINDOWS_11 114 // October, 2021
#define WINDOWS_11_22H2 115 // September, 2022
#define WINDOWS_11_23H2 116 // October, 2023
#define WINDOWS_11_24H2 117 // October, 2024
#define WINDOWS_NEW ULONG_MAX

#ifdef DEBUG
#define dprintf(format, ...) DbgPrint(format __VA_OPT__(,) __VA_ARGS__)
#else
#define dprintf(format, ...)
#endif

PHLIBAPI
NTSTATUS
NTAPI
PhInitializePhLib(
    _In_ PCWSTR ApplicationName,
    _In_ PVOID ImageBaseAddress
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsExecutingInWow64(
    VOID
    );

_Analysis_noreturn_
DECLSPEC_NORETURN
VOID
NTAPI
PhExitApplication(
    _In_opt_ NTSTATUS Status
    );

// Processor group support (dmex)

typedef struct _PH_SYSTEM_BASIC_INFORMATION
{
    USHORT PageSize;
    USHORT NumberOfProcessors;
    ULONG MaximumTimerResolution;
    ULONG NumberOfPhysicalPages;
    ULONG AllocationGranularity;
    ULONG_PTR MaximumUserModeAddress;
    KAFFINITY ActiveProcessorsAffinityMask;
} PH_SYSTEM_BASIC_INFORMATION, *PPH_SYSTEM_BASIC_INFORMATION;

PHLIBAPI extern PH_SYSTEM_BASIC_INFORMATION PhSystemBasicInformation;

typedef struct _PH_SYSTEM_PROCESSOR_INFORMATION
{
    BOOLEAN SingleProcessorGroup;
    USHORT NumberOfProcessors;
    USHORT NumberOfProcessorGroups;
    PUSHORT ActiveProcessorCount;
} PH_SYSTEM_PROCESSOR_INFORMATION, *PPH_SYSTEM_PROCESSOR_INFORMATION;

extern PH_SYSTEM_PROCESSOR_INFORMATION PhSystemProcessorInformation;

#ifdef __cplusplus
}
#endif

#endif
