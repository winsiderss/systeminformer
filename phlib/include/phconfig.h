#ifndef _PH_PHCONFIG_H
#define _PH_PHCONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define _User_set_

extern _User_set_ PVOID PhLibImageBase;
extern _User_set_ PWSTR PhApplicationName;
extern _User_set_ ULONG PhGlobalDpi;
extern PVOID PhHeapHandle;
extern RTL_OSVERSIONINFOEXW PhOsVersion;
extern SYSTEM_BASIC_INFORMATION PhSystemBasicInformation;
extern ULONG WindowsVersion;

extern ACCESS_MASK ProcessQueryAccess;
extern ACCESS_MASK ProcessAllAccess;
extern ACCESS_MASK ThreadQueryAccess;
extern ACCESS_MASK ThreadSetAccess;
extern ACCESS_MASK ThreadAllAccess;

// global

// Initialization flags

// Features

// Imports

#define PHLIB_INIT_MODULE_RESERVED1 0x1
#define PHLIB_INIT_MODULE_RESERVED2 0x2
/** Needed to use work queues. */
#define PHLIB_INIT_MODULE_RESERVED3 0x4
#define PHLIB_INIT_MODULE_RESERVED4 0x8
/** Needed to use file streams. */
#define PHLIB_INIT_MODULE_FILE_STREAM 0x10
/** Needed to use symbol providers. */
#define PHLIB_INIT_MODULE_SYMBOL_PROVIDER 0x20
#define PHLIB_INIT_MODULE_RESERVED5 0x40

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
    _In_ ULONG Flags,
    _In_opt_ SIZE_T HeapReserveSize,
    _In_opt_ SIZE_T HeapCommitSize
    );

#ifdef _WIN64
FORCEINLINE
BOOLEAN
PhIsExecutingInWow64(
    VOID
    )
{
    return FALSE;
}
#else
PHLIBAPI
BOOLEAN
NTAPI
PhIsExecutingInWow64(
    VOID
    );
#endif


PHLIBAPI
ULONG
NTAPI
PhWindowsVersion(
    VOID
    );

#define WINDOWS_HAS_CONSOLE_HOST (PhWindowsVersion() >= WINDOWS_7)
#define WINDOWS_HAS_CYCLE_TIME (PhWindowsVersion() >= WINDOWS_VISTA)
#define WINDOWS_HAS_IFILEDIALOG (PhWindowsVersion() >= WINDOWS_VISTA)
#define WINDOWS_HAS_IMAGE_FILE_NAME_BY_PROCESS_ID (PhWindowsVersion() >= WINDOWS_VISTA)
#define WINDOWS_HAS_IMMERSIVE (PhWindowsVersion() >= WINDOWS_8)
#define WINDOWS_HAS_LIMITED_ACCESS (PhWindowsVersion() >= WINDOWS_VISTA)
#define WINDOWS_HAS_SERVICE_TAGS (PhWindowsVersion() >= WINDOWS_VISTA)
#define WINDOWS_HAS_UAC (PhWindowsVersion() >= WINDOWS_VISTA)

#ifdef __cplusplus
}
#endif

#endif
