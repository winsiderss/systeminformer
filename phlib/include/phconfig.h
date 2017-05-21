#ifndef _PH_PHCONFIG_H
#define _PH_PHCONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define _User_set_

PHLIBAPI extern _User_set_ PVOID PhLibImageBase;

PHLIBAPI extern _User_set_ PWSTR PhApplicationName;
PHLIBAPI extern _User_set_ ULONG PhGlobalDpi;
PHLIBAPI extern PVOID PhHeapHandle;
PHLIBAPI extern RTL_OSVERSIONINFOEXW PhOsVersion;
PHLIBAPI extern SYSTEM_BASIC_INFORMATION PhSystemBasicInformation;
PHLIBAPI extern ULONG WindowsVersion;

PHLIBAPI extern ACCESS_MASK ProcessQueryAccess;
PHLIBAPI extern ACCESS_MASK ProcessAllAccess;
PHLIBAPI extern ACCESS_MASK ThreadQueryAccess;
PHLIBAPI extern ACCESS_MASK ThreadSetAccess;
PHLIBAPI extern ACCESS_MASK ThreadAllAccess;

#define WINDOWS_ANCIENT 0
#define WINDOWS_XP 51
#define WINDOWS_SERVER_2003 52
#define WINDOWS_VISTA 60
#define WINDOWS_7 61
#define WINDOWS_8 62
#define WINDOWS_8_1 63
#define WINDOWS_10 100
#define WINDOWS_NEW MAXLONG

#define WINDOWS_HAS_CONSOLE_HOST (WindowsVersion >= WINDOWS_7)
#define WINDOWS_HAS_CYCLE_TIME (WindowsVersion >= WINDOWS_VISTA)
#define WINDOWS_HAS_IFILEDIALOG (WindowsVersion >= WINDOWS_VISTA)
#define WINDOWS_HAS_IMAGE_FILE_NAME_BY_PROCESS_ID (WindowsVersion >= WINDOWS_VISTA)
#define WINDOWS_HAS_IMMERSIVE (WindowsVersion >= WINDOWS_8)
#define WINDOWS_HAS_LIMITED_ACCESS (WindowsVersion >= WINDOWS_VISTA)
#define WINDOWS_HAS_SERVICE_TAGS (WindowsVersion >= WINDOWS_VISTA)
#define WINDOWS_HAS_UAC (WindowsVersion >= WINDOWS_VISTA)

// Debugging

#ifdef DEBUG
#define dprintf(format, ...) DbgPrint(format, __VA_ARGS__)
#else
#define dprintf(format, ...)
#endif

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

#ifdef __cplusplus
}
#endif

#endif
