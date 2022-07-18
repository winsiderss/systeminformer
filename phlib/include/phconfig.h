#ifndef _PH_PHCONFIG_H
#define _PH_PHCONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define _User_set_

extern _User_set_ PVOID PhInstanceHandle;
extern _User_set_ PWSTR PhApplicationName;
PHLIBAPI extern _User_set_ ULONG PhGlobalDpi;
extern PVOID PhHeapHandle;
extern RTL_OSVERSIONINFOEXW PhOsVersion;
PHLIBAPI extern SYSTEM_BASIC_INFORMATION PhSystemBasicInformation;
extern ULONG WindowsVersion;

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
#define WINDOWS_11 113 // October, 2021
#define WINDOWS_11_22H1 114 // February, 2022
#define WINDOWS_NEW ULONG_MAX

// Debugging

#ifdef DEBUG
#define dprintf(format, ...) DbgPrint(format, ##__VA_ARGS__)
#else
#define dprintf(format, ...)
#endif

// global

// Initialization flags

// Features

// Imports

#define PHLIB_INIT_MODULE_RESERVED1 0x1
#define PHLIB_INIT_MODULE_RESERVED2 0x2
#define PHLIB_INIT_MODULE_RESERVED3 0x4
#define PHLIB_INIT_MODULE_RESERVED4 0x8
#define PHLIB_INIT_MODULE_RESERVED5 0x10
#define PHLIB_INIT_MODULE_RESERVED6 0x20
#define PHLIB_INIT_MODULE_RESERVED7 0x40

PHLIBAPI
NTSTATUS
NTAPI
PhInitializePhLib(
    VOID
    );

PHLIBAPI
NTSTATUS
NTAPI
PhInitializePhLibEx(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_ PVOID ImageBaseAddress,
    _In_opt_ SIZE_T HeapReserveSize,
    _In_opt_ SIZE_T HeapCommitSize
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsExecutingInWow64(
    VOID
    );

// 
DECLSPEC_NORETURN
FORCEINLINE
VOID
PhExitApplication(
    _In_opt_ NTSTATUS Status
    )
{
#if (PHNT_VERSION >= PHNT_WIN7)
    RtlExitUserProcess(Status);
#else
    ExitProcess(Status);
#endif
}

// Processor group support (dmex)

typedef struct _PH_SYSTEM_PROCESSOR_INFORMATION
{
    BOOLEAN SingleProcessorGroup;
    USHORT NumberOfProcessors;
    USHORT NumberOfProcessorGroups;
    PUSHORT ActiveProcessorCount;
} PH_SYSTEM_PROCESSOR_INFORMATION, *PPH_SYSTEM_PROCESSOR_INFORMATION;

PHLIBAPI extern PH_SYSTEM_PROCESSOR_INFORMATION PhSystemProcessorInformation;

#ifdef __cplusplus
}
#endif

#endif
