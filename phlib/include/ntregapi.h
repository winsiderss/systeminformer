#ifndef _NTREGAPI_H
#define _NTREGAPI_H

// Boot condition flags (NtInitializeRegistry)

#define REG_INIT_BOOT_SM 0x0000
#define REG_INIT_BOOT_SETUP 0x0001
#define REG_INIT_BOOT_ACCEPTED_BASE 0x0002
#define REG_INIT_BOOT_ACCEPTED_MAX REG_INIT_BOOT_ACCEPTED_BASE + 999

#define REG_MAX_KEY_VALUE_NAME_LENGTH 32767
#define REG_MAX_KEY_NAME_LENGTH 512

typedef enum _KEY_INFORMATION_CLASS
{
    KeyBasicInformation,
    KeyNodeInformation,
    KeyFullInformation,
    KeyNameInformation,
    KeyCachedInformation,
    KeyFlagsInformation,
    KeyVirtualizationInformation,
    KeyHandleTagsInformation,
    MaxKeyInfoClass
} KEY_INFORMATION_CLASS;

typedef struct _KEY_BASIC_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
    ULONG TitleIndex;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

typedef struct _KEY_NODE_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
    ULONG TitleIndex;
    ULONG ClassOffset;
    ULONG ClassLength;
    ULONG NameLength;
    WCHAR Name[1];
    // ...
    // WCHAR Class[1];
} KEY_NODE_INFORMATION, *PKEY_NODE_INFORMATION;

typedef struct _KEY_FULL_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
    ULONG TitleIndex;
    ULONG ClassOffset;
    ULONG ClassLength;
    ULONG SubKeys;
    ULONG MaxNameLen;
    ULONG MaxClassLen;
    ULONG Values;
    ULONG MaxValueNameLen;
    ULONG MaxValueDataLen;
    WCHAR Class[1];
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

typedef struct _KEY_NAME_INFORMATION
{
    ULONG NameLength;
    WCHAR Name[1];
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;

typedef struct _KEY_CACHED_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
    ULONG TitleIndex;
    ULONG SubKeys;
    ULONG MaxNameLen;
    ULONG Values;
    ULONG MaxValueNameLen;
    ULONG MaxValueDataLen;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_CACHED_INFORMATION, *PKEY_CACHED_INFORMATION;

typedef struct _KEY_FLAGS_INFORMATION
{
    ULONG UserFlags;
} KEY_FLAGS_INFORMATION, *PKEY_FLAGS_INFORMATION;

typedef enum _KEY_SET_INFORMATION_CLASS
{
    KeyWriteTimeInformation,
    KeyWow64FlagsInformation,
    KeyControlFlagsInformation,
    KeySetVirtualizationInformation,
    KeySetDebugInformation,
    KeySetHandleTagsInformation,
    MaxKeySetInfoClass
} KEY_SET_INFORMATION_CLASS;

typedef struct _KEY_WRITE_TIME_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
} KEY_WRITE_TIME_INFORMATION, *PKEY_WRITE_TIME_INFORMATION;

typedef struct _KEY_WOW64_FLAGS_INFORMATION
{
    ULONG UserFlags;
} KEY_WOW64_FLAGS_INFORMATION, *PKEY_WOW64_FLAGS_INFORMATION;

typedef struct _KEY_HANDLE_TAGS_INFORMATION
{
    ULONG HandleTags;
} KEY_HANDLE_TAGS_INFORMATION, *PKEY_HANDLE_TAGS_INFORMATION;

typedef struct _KEY_CONTROL_FLAGS_INFORMATION
{
    ULONG ControlFlags;
} KEY_CONTROL_FLAGS_INFORMATION, *PKEY_CONTROL_FLAGS_INFORMATION;

typedef struct _KEY_SET_VIRTUALIZATION_INFORMATION
{
    ULONG VirtualTarget : 1;
    ULONG VirtualStore : 1;
    ULONG VirtualSource : 1; // true if key has been virtualized at least once
    ULONG Reserved : 29;
} KEY_SET_VIRTUALIZATION_INFORMATION, *PKEY_SET_VIRTUALIZATION_INFORMATION;

typedef enum _KEY_VALUE_INFORMATION_CLASS
{
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64,
    MaxKeyValueInfoClass
} KEY_VALUE_INFORMATION_CLASS;

typedef struct _KEY_VALUE_BASIC_INFORMATION
{
    ULONG TitleIndex;
    ULONG Type;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION
{
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataOffset;
    ULONG DataLength;
    ULONG NameLength;
    WCHAR Name[1];
    // ...
    // UCHAR Data[1];
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION
{
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataLength;
    UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION_ALIGN64
{
    ULONG Type;
    ULONG DataLength;
    UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, *PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64;

typedef struct _KEY_VALUE_ENTRY
{
    PUNICODE_STRING ValueName;
    ULONG DataLength;
    ULONG DataOffset;
    ULONG Type;
} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY;

typedef enum _REG_ACTION
{
    KeyAdded,
    KeyRemoved,
    KeyModified
} REG_ACTION;

typedef struct _REG_NOTIFY_INFORMATION
{
    ULONG NextEntryOffset;
    REG_ACTION Action;
    ULONG KeyLength;
    WCHAR Key[1];
} REG_NOTIFY_INFORMATION, *PREG_NOTIFY_INFORMATION;

typedef struct _KEY_PID_ARRAY
{
    HANDLE PID;
    UNICODE_STRING KeyName;
} KEY_PID_ARRAY, *PKEY_PID_ARRAY;

typedef struct _KEY_OPEN_SUBKEYS_INFORMATION
{
    ULONG Count;
    KEY_PID_ARRAY KeyArray[1];
} KEY_OPEN_SUBKEYS_INFORMATION, *PKEY_OPEN_SUBKEYS_INFORMATION;

// System calls

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateKey(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __reserved ULONG TitleIndex,
    __in_opt PUNICODE_STRING Class,
    __in ULONG CreateOptions,
    __out_opt PULONG Disposition
    );

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateKeyTransacted(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __reserved ULONG TitleIndex,
    __in_opt PUNICODE_STRING Class,
    __in ULONG CreateOptions,
    __in HANDLE TransactionHandle,
    __out_opt PULONG Disposition
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenKey(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenKeyTransacted(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE TransactionHandle
    );
#endif

#if (PHNT_VERSION >= PHNT_WIN7)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenKeyEx(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in ULONG OpenOptions
    );
#endif

#if (PHNT_VERSION >= PHNT_WIN7)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenKeyTransactedEx(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in ULONG OpenOptions,
    __in HANDLE TransactionHandle
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteKey(
    __in HANDLE KeyHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRenameKey(
    __in HANDLE KeyHandle,
    __in PUNICODE_STRING NewName
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteValueKey(
    __in HANDLE KeyHandle,
    __in PUNICODE_STRING ValueName
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryKey(
    __in HANDLE KeyHandle,
    __in KEY_INFORMATION_CLASS KeyInformationClass,
    __out_bcount_opt(Length) PVOID KeyInformation,
    __in ULONG Length,
    __out PULONG ResultLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationKey(
    __in HANDLE KeyHandle,
    __in KEY_SET_INFORMATION_CLASS KeySetInformationClass,
    __in_bcount(KeySetInformationLength) PVOID KeySetInformation,
    __in ULONG KeySetInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryValueKey(
    __in HANDLE KeyHandle,
    __in PUNICODE_STRING ValueName,
    __in KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    __out_bcount_opt(Length) PVOID KeyValueInformation,
    __in ULONG Length,
    __out PULONG ResultLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetValueKey(
    __in HANDLE KeyHandle,
    __in PUNICODE_STRING ValueName,
    __in_opt ULONG TitleIndex,
    __in ULONG Type,
    __in_bcount_opt(DataSize) PVOID Data,
    __in ULONG DataSize
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryMultipleValueKey(
    __in HANDLE KeyHandle,
    __inout_ecount(EntryCount) PKEY_VALUE_ENTRY ValueEntries,
    __in ULONG EntryCount,
    __out_bcount(*BufferLength) PVOID ValueBuffer,
    __inout PULONG BufferLength,
    __out_opt PULONG RequiredBufferLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnumerateKey(
    __in HANDLE KeyHandle,
    __in ULONG Index,
    __in KEY_INFORMATION_CLASS KeyInformationClass,
    __out_bcount_opt(Length) PVOID KeyInformation,
    __in ULONG Length,
    __out PULONG ResultLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnumerateValueKey(
    __in HANDLE KeyHandle,
    __in ULONG Index,
    __in KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    __out_bcount_opt(Length) PVOID KeyValueInformation,
    __in ULONG Length,
    __out PULONG ResultLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushKey(
    __in HANDLE KeyHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCompactKeys(
    __in ULONG Count,
    __in_ecount(Count) HANDLE KeyArray[]
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCompressKey(
    __in HANDLE Key
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLoadKey(
    __in POBJECT_ATTRIBUTES TargetKey,
    __in POBJECT_ATTRIBUTES SourceFile
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLoadKey2(
    __in POBJECT_ATTRIBUTES TargetKey,
    __in POBJECT_ATTRIBUTES SourceFile,
    __in ULONG Flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLoadKeyEx(
    __in POBJECT_ATTRIBUTES TargetKey,
    __in POBJECT_ATTRIBUTES SourceFile,
    __in ULONG Flags,
    __in_opt HANDLE TrustClassKey
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplaceKey(
    __in POBJECT_ATTRIBUTES NewFile,
    __in HANDLE TargetHandle,
    __in POBJECT_ATTRIBUTES OldFile
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSaveKey(
    __in HANDLE KeyHandle,
    __in HANDLE FileHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSaveKeyEx(
    __in HANDLE KeyHandle,
    __in HANDLE FileHandle,
    __in ULONG Format
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSaveMergedKeys(
    __in HANDLE HighPrecedenceKeyHandle,
    __in HANDLE LowPrecedenceKeyHandle,
    __in HANDLE FileHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRestoreKey(
    __in HANDLE KeyHandle,
    __in HANDLE FileHandle,
    __in ULONG Flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnloadKey(
    __in POBJECT_ATTRIBUTES TargetKey
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnloadKey2(
    __in POBJECT_ATTRIBUTES TargetKey,
    __in ULONG Flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnloadKeyEx(
    __in POBJECT_ATTRIBUTES TargetKey,
    __in_opt HANDLE Event
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtNotifyChangeKey(
    __in HANDLE KeyHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in ULONG CompletionFilter,
    __in BOOLEAN WatchTree,
    __out_bcount_opt(BufferSize) PVOID Buffer,
    __in ULONG BufferSize,
    __in BOOLEAN Asynchronous
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtNotifyChangeMultipleKeys(
    __in HANDLE MasterKeyHandle,
    __in_opt ULONG Count,
    __in_ecount_opt(Count) OBJECT_ATTRIBUTES SlaveObjects[],
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in ULONG CompletionFilter,
    __in BOOLEAN WatchTree,
    __out_bcount_opt(BufferSize) PVOID Buffer,
    __in ULONG BufferSize,
    __in BOOLEAN Asynchronous
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryOpenSubKeys(
    __in POBJECT_ATTRIBUTES TargetKey,
    __out PULONG HandleCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryOpenSubKeysEx(
    __in POBJECT_ATTRIBUTES TargetKey,
    __in ULONG BufferLength,
    __out_bcount(BufferLength) PVOID Buffer,
    __out PULONG RequiredSize
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitializeRegistry(
    __in USHORT BootCondition
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLockRegistryKey(
    __in HANDLE KeyHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLockProductActivationKeys(
    __inout_opt ULONG *pPrivateVer,
    __out_opt ULONG *pSafeMode
    );

#if (PHNT_VERSION >= PHNT_VISTA)
// private
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreezeRegistry(
    __in ULONG TimeOutInSeconds
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
// private
NTSYSCALLAPI
NTSTATUS
NTAPI
NtThawRegistry(
    VOID
    );
#endif

#endif
