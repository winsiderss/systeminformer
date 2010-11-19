#ifndef _NTLDR_H
#define _NTLDR_H

// DLLs

#define LDRP_STATIC_LINK 0x00000002
#define LDRP_IMAGE_DLL 0x00000004
#define LDRP_LOAD_IN_PROGRESS 0x00001000
#define LDRP_UNLOAD_IN_PROGRESS 0x00002000
#define LDRP_ENTRY_PROCESSED 0x00004000
#define LDRP_ENTRY_INSERTED 0x00008000
#define LDRP_CURRENT_LOAD 0x00010000
#define LDRP_FAILED_BUILTIN_LOAD 0x00020000
#define LDRP_DONT_CALL_FOR_THREADS 0x00040000
#define LDRP_PROCESS_ATTACH_CALLED 0x00080000
#define LDRP_DEBUG_SYMBOLS_LOADED 0x00100000
#define LDRP_IMAGE_NOT_AT_BASE 0x00200000
#define LDRP_COR_IMAGE 0x00400000
#define LDRP_COR_OWNS_UNMAP 0x00800000
#define LDRP_SYSTEM_MAPPED 0x01000000
#define LDRP_IMAGE_VERIFYING 0x02000000
#define LDRP_DRIVER_DEPENDENT_DLL 0x04000000
#define LDRP_ENTRY_NATIVE 0x08000000
#define LDRP_REDIRECTED 0x10000000
#define LDRP_NON_PAGED_DEBUG_INFO 0x20000000
#define LDRP_MM_LOADED 0x40000000
#define LDRP_COMPAT_DATABASE_PROCESSED 0x80000000

// Use the size of the structure as it was in 
// Windows XP.
#define LDR_DATA_TABLE_ENTRY_SIZE_WINXP FIELD_OFFSET(LDR_DATA_TABLE_ENTRY, ForwarderLinks)

typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    union
    {
        LIST_ENTRY HashLinks;
        struct
        {
            PVOID SectionPointer;
            ULONG CheckSum;
        };
    };
    union
    {
        ULONG TimeDateStamp;
        PVOID LoadedImports;
    };
    PVOID EntryPointActivationContext;
    PVOID PatchInformation;
    LIST_ENTRY ForwarderLinks;
    LIST_ENTRY ServiceTagLinks;
    LIST_ENTRY StaticLinks;
    PVOID ContextInformation;
    ULONG_PTR OriginalBase;
    LARGE_INTEGER LoadTime;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef BOOLEAN (NTAPI *PDLL_INIT_ROUTINE)(
    __in PVOID DllHandle,
    __in ULONG Reason,
    __in_opt PCONTEXT Context
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrLoadDll(
    __in_opt PWSTR DllPath,
    __in_opt PULONG DllCharacteristics,
    __in PUNICODE_STRING DllName,
    __out PVOID *DllHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrUnloadDll(
    __in PVOID DllHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandle(
    __in_opt PWSTR DllPath,
    __in_opt PULONG DllCharacteristics,
    __in PUNICODE_STRING DllName,
    __out PVOID *DllHandle
    );

#define LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT 0x00000001
#define LDR_GET_DLL_HANDLE_EX_PIN 0x00000002

NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandleEx(
    __in ULONG Flags,
    __in_opt PCWSTR DllPath,
    __in_opt PULONG DllCharacteristics,
    __in PUNICODE_STRING DllName,
    __out_opt PVOID *DllHandle
    );

#if (PHNT_VERSION >= PHNT_WIN7)
// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandleByMapping(
    __in PVOID Base,
    __out PVOID *DllHandle
    );
#endif

#if (PHNT_VERSION >= PHNT_WIN7)
// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandleByName(
    __in_opt PUNICODE_STRING BaseDllName,
    __in_opt PUNICODE_STRING FullDllName,
    __out PVOID *DllHandle
    );
#endif

#define LDR_ADDREF_DLL_PIN 0x00000001

NTSYSAPI
NTSTATUS
NTAPI
LdrAddRefDll(
    __in ULONG Flags,
    __in PVOID DllHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrGetProcedureAddress(
    __in PVOID DllHandle,
    __in_opt PANSI_STRING ProcedureName,
    __in_opt ULONG ProcedureNumber,
    __out PVOID *ProcedureAddress
    );

// begin_rev

#define LDR_GET_PROCEDURE_ADDRESS_DONT_RECORD_FORWARDER 0x00000001

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSAPI
NTSTATUS
NTAPI
LdrGetProcedureAddressEx(
    __in PVOID DllHandle,
    __in_opt PANSI_STRING ProcedureName,
    __in_opt ULONG ProcedureNumber,
    __out PVOID *ProcedureAddress,
    __in ULONG Flags
    );
#endif

// end_rev

#define LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS 0x00000001
#define LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY 0x00000002

#define LDR_LOCK_LOADER_LOCK_DISPOSITION_INVALID 0
#define LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED 1
#define LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_NOT_ACQUIRED 2

NTSYSAPI
NTSTATUS
NTAPI
LdrLockLoaderLock(
    __in ULONG Flags,
    __out_opt ULONG *Disposition,
    __out PVOID *Cookie
    );

#define LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS 0x00000001

NTSYSAPI
NTSTATUS
NTAPI
LdrUnlockLoaderLock(
    __in ULONG Flags,
    __inout PVOID Cookie
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrRelocateImage(
    __in PVOID NewBase,
    __in PSTR LoaderName,
    __in NTSTATUS Success,
    __in NTSTATUS Conflict,
    __in NTSTATUS Invalid
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrRelocateImageWithBias(
    __in PVOID NewBase,
    __in LONGLONG Bias,
    __in PSTR LoaderName,
    __in NTSTATUS Success,
    __in NTSTATUS Conflict,
    __in NTSTATUS Invalid
    );

NTSYSAPI
PIMAGE_BASE_RELOCATION
NTAPI
LdrProcessRelocationBlock(
    __in ULONG_PTR VA,
    __in ULONG SizeOfBlock,
    __in PUSHORT NextOffset,
    __in LONG_PTR Diff
    );

NTSYSAPI
BOOLEAN
NTAPI
LdrVerifyMappedImageMatchesChecksum(
    __in PVOID BaseAddress,
    __in SIZE_T NumberOfBytes,
    __in ULONG FileLength
    );

typedef VOID (NTAPI *PLDR_IMPORT_MODULE_CALLBACK)(
    __in PVOID Parameter,
    __in PSTR ModuleName
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrVerifyImageMatchesChecksum(
    __in HANDLE ImageFileHandle,
    __in_opt PLDR_IMPORT_MODULE_CALLBACK ImportCallbackRoutine,
    __in PVOID ImportCallbackParameter,
    __out_opt PUSHORT ImageCharacteristics
    );

#if (PHNT_VERSION >= PHNT_VISTA)
// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrQueryModuleServiceTags(
    __in PVOID DllHandle,
    __out_ecount(*NumberOfServiceTags) PULONG ServiceTags,
    __inout PULONG NumberOfServiceTags
    );
#endif

// begin_msdn:"DLL Load Notification"

#define LDR_DLL_NOTIFICATION_REASON_LOADED 1
#define LDR_DLL_NOTIFICATION_REASON_UNLOADED 2

typedef struct _LDR_DLL_LOADED_NOTIFICATION_DATA
{
    ULONG Flags;
    PUNICODE_STRING FullDllName;
    PUNICODE_STRING BaseDllName;
    PVOID DllBase;
    ULONG SizeOfImage;
} LDR_DLL_LOADED_NOTIFICATION_DATA, *PLDR_DLL_LOADED_NOTIFICATION_DATA;

typedef struct _LDR_DLL_UNLOADED_NOTIFICATION_DATA
{
    ULONG Flags;
    PCUNICODE_STRING FullDllName;
    PCUNICODE_STRING BaseDllName;
    PVOID DllBase;
    ULONG SizeOfImage;
} LDR_DLL_UNLOADED_NOTIFICATION_DATA, *PLDR_DLL_UNLOADED_NOTIFICATION_DATA;

typedef union _LDR_DLL_NOTIFICATION_DATA
{
    LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
    LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
} LDR_DLL_NOTIFICATION_DATA, *PLDR_DLL_NOTIFICATION_DATA;

typedef VOID (NTAPI *PLDR_DLL_NOTIFICATION_FUNCTION)(
    __in ULONG NotificationReason,
    __in PLDR_DLL_NOTIFICATION_DATA NotificationData,
    __in_opt PVOID Context
    );

#if (PHNT_VERSION >= PHNT_VISTA)

NTSYSAPI
NTSTATUS
NTAPI
LdrRegisterDllNotification(
    __in ULONG Flags,
    __in PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction,
    __in PVOID Context,
    __out PVOID *Cookie
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrUnregisterDllNotification(
    __in PVOID Cookie
    );

#endif

// end_msdn

// Module information

typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
    HANDLE Section;
    PVOID MappedBase;
    PVOID ImageBase;
    ULONG ImageSize;
    ULONG Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    UCHAR FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES
{
    ULONG NumberOfModules;
    RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

// rev
typedef struct _RTL_PROCESS_MODULE_INFORMATION_EX
{
    USHORT NextEntryOffset;
    RTL_PROCESS_MODULE_INFORMATION ModuleInfo;
    ULONG CheckSum;
    ULONG Reserved1;
    ULONG Reserved2;
} RTL_PROCESS_MODULE_INFORMATION_EX, *PRTL_PROCESS_MODULE_INFORMATION_EX;

#endif
