/*
 * Registry support functions
 *
 * This file is part of System Informer.
 */

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
    KeyBasicInformation, // KEY_BASIC_INFORMATION
    KeyNodeInformation, // KEY_NODE_INFORMATION
    KeyFullInformation, // KEY_FULL_INFORMATION
    KeyNameInformation, // KEY_NAME_INFORMATION
    KeyCachedInformation, // KEY_CACHED_INFORMATION
    KeyFlagsInformation, // KEY_FLAGS_INFORMATION
    KeyVirtualizationInformation, // KEY_VIRTUALIZATION_INFORMATION
    KeyHandleTagsInformation, // KEY_HANDLE_TAGS_INFORMATION
    KeyTrustInformation, // KEY_TRUST_INFORMATION
    KeyLayerInformation, // KEY_LAYER_INFORMATION
    MaxKeyInfoClass
} KEY_INFORMATION_CLASS;

typedef struct _KEY_BASIC_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
    ULONG TitleIndex;
    ULONG NameLength;
    _Field_size_bytes_(NameLength) WCHAR Name[1];
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

typedef struct _KEY_NODE_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
    ULONG TitleIndex;
    ULONG ClassOffset;
    ULONG ClassLength;
    ULONG NameLength;
    _Field_size_bytes_(NameLength) WCHAR Name[1];
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
    ULONG MaxNameLength;
    ULONG MaxClassLength;
    ULONG Values;
    ULONG MaxValueNameLength;
    ULONG MaxValueDataLength;
    WCHAR Class[1];
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

typedef struct _KEY_NAME_INFORMATION
{
    ULONG NameLength;
    _Field_size_bytes_(NameLength) WCHAR Name[1];
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;

typedef struct _KEY_CACHED_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
    ULONG TitleIndex;
    ULONG SubKeys;
    ULONG MaxNameLength;
    ULONG Values;
    ULONG MaxValueNameLength;
    ULONG MaxValueDataLength;
    ULONG NameLength;
    _Field_size_bytes_(NameLength) WCHAR Name[1];
} KEY_CACHED_INFORMATION, *PKEY_CACHED_INFORMATION;

// rev
#define REG_FLAG_VOLATILE 0x0001
#define REG_FLAG_LINK 0x0002

// msdn
#define REG_KEY_DONT_VIRTUALIZE 0x0002
#define REG_KEY_DONT_SILENT_FAIL 0x0004
#define REG_KEY_RECURSE_FLAG 0x0008

// private
typedef struct _KEY_FLAGS_INFORMATION
{
    ULONG Wow64Flags;
    ULONG KeyFlags; // REG_FLAG_*
    ULONG ControlFlags; // REG_KEY_*
} KEY_FLAGS_INFORMATION, *PKEY_FLAGS_INFORMATION;

/**
 * The KEY_VIRTUALIZATION_INFORMATION structure contains information about the virtualization state of a key.
 *
 * The flags include:
 * - VirtualizationCandidate: The key is part of the virtualization namespace scope (only HKLM\Software for now).
 * - VirtualizationEnabled: Virtualization is enabled on this key. Can be 1 only if VirtualizationCandidate is 1.
 * - VirtualTarget: The key is a virtual key. Can be 1 only if VirtualizationCandidate and VirtualizationEnabled are 0. Valid only on the virtual store key handles.
 * - VirtualStore: The key is a part of the virtual store path. Valid only on the virtual store key handles.
 * - VirtualSource: The key has ever been virtualized, can be 1 only if VirtualizationCandidate is 1.
 * - Reserved: Reserved bits.
 */
typedef struct _KEY_VIRTUALIZATION_INFORMATION
{
    ULONG VirtualizationCandidate : 1;
    ULONG VirtualizationEnabled : 1;
    ULONG VirtualTarget : 1;
    ULONG VirtualStore : 1;
    ULONG VirtualSource : 1;
    ULONG Reserved : 27;
} KEY_VIRTUALIZATION_INFORMATION, *PKEY_VIRTUALIZATION_INFORMATION;

// private
/**
 * The KEY_TRUST_INFORMATION structure contains information about the trust status of a key.
 *
 * The flags include:
 * - TrustedKey: Indicates whether the key is trusted. When set, this flag signifies that the key is considered
 *   to be secure and reliable.
 * - Reserved: Reserved bits.
 */
typedef struct _KEY_TRUST_INFORMATION
{
    ULONG TrustedKey : 1;
    ULONG Reserved : 31;
} KEY_TRUST_INFORMATION, *PKEY_TRUST_INFORMATION;

// private
/**
 * The KEY_LAYER_INFORMATION structure contains information about a key layer.
 *
 * The flags include:
 * - IsTombstone: Indicates whether the key layer is a tombstone. A tombstone is a marker that indicates
 *   that the key has been deleted but not yet purged from the registry. It is used to maintain the
 *   integrity of the registry and ensure that deleted keys are not immediately reused.
 * - IsSupersedeLocal: Indicates whether the key layer supersedes the local key. When set, this flag
 *   indicates that the key layer should replace the local key's information, effectively overriding
 *   any local changes or settings.
 * - IsSupersedeTree: Indicates whether the key layer supersedes the entire key tree. When set, this flag
 *   indicates that the key layer should replace the entire subtree of keys, overriding any changes or
 *   settings in the subtree.
 * - ClassIsInherited: Indicates whether the key layer's class is inherited. When set, this flag indicates
 *   that the class information of the key layer is inherited from its parent key, rather than being
 *   explicitly defined.
 * - Reserved: Reserved bits.
 */
typedef struct _KEY_LAYER_INFORMATION
{
    ULONG IsTombstone : 1;
    ULONG IsSupersedeLocal : 1;
    ULONG IsSupersedeTree : 1;
    ULONG ClassIsInherited : 1;
    ULONG Reserved : 28;
} KEY_LAYER_INFORMATION, *PKEY_LAYER_INFORMATION;

typedef enum _KEY_SET_INFORMATION_CLASS
{
    KeyWriteTimeInformation, // KEY_WRITE_TIME_INFORMATION
    KeyWow64FlagsInformation, // KEY_WOW64_FLAGS_INFORMATION
    KeyControlFlagsInformation, // KEY_CONTROL_FLAGS_INFORMATION
    KeySetVirtualizationInformation, // KEY_SET_VIRTUALIZATION_INFORMATION
    KeySetDebugInformation,
    KeySetHandleTagsInformation, // KEY_HANDLE_TAGS_INFORMATION
    KeySetLayerInformation, // KEY_SET_LAYER_INFORMATION
    MaxKeySetInfoClass
} KEY_SET_INFORMATION_CLASS;

/**
 * Structure representing the last write time of a registry key.
 *
 * The values include:
 * - LastWriteTime: Contains the timestamp of the last write operation performed on a registry key.
 */
typedef struct _KEY_WRITE_TIME_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
} KEY_WRITE_TIME_INFORMATION, *PKEY_WRITE_TIME_INFORMATION;

/**
 * The KEY_WOW64_FLAGS_INFORMATION structure contains information about the WOW64 flags for a key.
 *
 * The fields include:
 * - UserFlags: A set of user-defined flags associated with the key. These flags are used to store
 *   additional information about the key in the context of WOW64 (Windows 32-bit on Windows 64-bit).
 */
typedef struct _KEY_WOW64_FLAGS_INFORMATION
{
    ULONG UserFlags;
} KEY_WOW64_FLAGS_INFORMATION, *PKEY_WOW64_FLAGS_INFORMATION;

/**
 * The KEY_HANDLE_TAGS_INFORMATION structure contains information about the handle tags for a key.
 *
 * The fields include:
 * - HandleTags: A set of tags associated with the key handle. These tags are used to store additional
 *   metadata or state information about the key handle.
 */
typedef struct _KEY_HANDLE_TAGS_INFORMATION
{
    ULONG HandleTags;
} KEY_HANDLE_TAGS_INFORMATION, *PKEY_HANDLE_TAGS_INFORMATION;

/**
 * The KEY_SET_LAYER_INFORMATION structure contains information about a key layer.
 *
 * The flags include:
 * - IsTombstone: Indicates whether the key layer is a tombstone. A tombstone is a marker that indicates
 *   that the key has been deleted but not yet purged from the registry. It is used to maintain the
 *   integrity of the registry and ensure that deleted keys are not immediately reused.
 * - IsSupersedeLocal: Indicates whether the key layer supersedes the local key. When set, this flag
 *   indicates that the key layer should replace the local key's information, effectively overriding
 *   any local changes or settings.
 * - IsSupersedeTree: Indicates whether the key layer supersedes the entire key tree. When set, this flag
 *   indicates that the key layer should replace the entire subtree of keys, overriding any changes or
 *   settings in the subtree.
 * - ClassIsInherited: Indicates whether the key layer's class is inherited. When set, this flag indicates
 *   that the class information of the key layer is inherited from its parent key, rather than being
 *   explicitly defined.
 * - Reserved: Reserved bits.
 */
typedef struct _KEY_SET_LAYER_INFORMATION
{
    ULONG IsTombstone : 1;
    ULONG IsSupersedeLocal : 1;
    ULONG IsSupersedeTree : 1;
    ULONG ClassIsInherited : 1;
    ULONG Reserved : 28;
} KEY_SET_LAYER_INFORMATION, *PKEY_SET_LAYER_INFORMATION;

/**
 * The KEY_CONTROL_FLAGS_INFORMATION structure contains control flags for a key.
 *
 * The fields include:
 * - ControlFlags: A set of control flags associated with the key. These flags are used to store
 *   additional control information about the key, which can affect its behavior or state.
 */
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
    KeyValueBasicInformation, // KEY_VALUE_BASIC_INFORMATION
    KeyValueFullInformation, // KEY_VALUE_FULL_INFORMATION
    KeyValuePartialInformation, // KEY_VALUE_PARTIAL_INFORMATION
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64,  // KEY_VALUE_PARTIAL_INFORMATION_ALIGN64
    KeyValueLayerInformation, // KEY_VALUE_LAYER_INFORMATION
    MaxKeyValueInfoClass
} KEY_VALUE_INFORMATION_CLASS;

typedef struct _KEY_VALUE_BASIC_INFORMATION
{
    ULONG TitleIndex;
    ULONG Type;
    ULONG NameLength;
    _Field_size_bytes_(NameLength) WCHAR Name[1];
} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION
{
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataOffset;
    ULONG DataLength;
    ULONG NameLength;
    _Field_size_bytes_(NameLength) WCHAR Name[1];
    // ...
    // UCHAR Data[1];
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION
{
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataLength;
    _Field_size_bytes_(DataLength) UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION_ALIGN64
{
    ULONG Type;
    ULONG DataLength;
    _Field_size_bytes_(DataLength) UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, *PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64;

// private
typedef struct _KEY_VALUE_LAYER_INFORMATION
{
    ULONG IsTombstone : 1;
    ULONG Reserved : 31;
} KEY_VALUE_LAYER_INFORMATION, *PKEY_VALUE_LAYER_INFORMATION;

// private
typedef enum _CM_EXTENDED_PARAMETER_TYPE
{
  CmExtendedParameterInvalidType,
  CmExtendedParameterTrustClassKey,
  CmExtendedParameterEvent,
  CmExtendedParameterFileAccessToken,
  CmExtendedParameterMax,
} CM_EXTENDED_PARAMETER_TYPE;

#define CM_EXTENDED_PARAMETER_TYPE_BITS 8

// private
typedef struct DECLSPEC_ALIGN(8) _CM_EXTENDED_PARAMETER
{
    struct
    {
        ULONG64 Type : CM_EXTENDED_PARAMETER_TYPE_BITS;
        ULONG64 Reserved : 64 - CM_EXTENDED_PARAMETER_TYPE_BITS;
    };

    union
    {
        ULONG64 ULong64;
        PVOID Pointer;
        SIZE_T Size;
        HANDLE Handle;
        ULONG ULong;
        ACCESS_MASK AccessMask;
    };
} CM_EXTENDED_PARAMETER, *PCM_EXTENDED_PARAMETER;

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
    _Field_size_bytes_(KeyLength) WCHAR Key[1];
} REG_NOTIFY_INFORMATION, *PREG_NOTIFY_INFORMATION;

typedef struct _KEY_PID_ARRAY
{
    HANDLE ProcessId;
    UNICODE_STRING KeyName;
} KEY_PID_ARRAY, *PKEY_PID_ARRAY;

typedef struct _KEY_OPEN_SUBKEYS_INFORMATION
{
    ULONG Count;
    KEY_PID_ARRAY KeyArray[1];
} KEY_OPEN_SUBKEYS_INFORMATION, *PKEY_OPEN_SUBKEYS_INFORMATION;

//
// Virtualization // since REDSTONE
//

// rev
#define VR_DEVICE_NAME L"\\Device\\VRegDriver"

// rev
#define IOCTL_VR_INITIALIZE_JOB_FOR_VREG            CTL_CODE(FILE_DEVICE_UNKNOWN, 1, METHOD_BUFFERED, FILE_ANY_ACCESS) // in: VR_INITIALIZE_JOB_FOR_VREG
#define IOCTL_VR_LOAD_DIFFERENCING_HIVE             CTL_CODE(FILE_DEVICE_UNKNOWN, 2, METHOD_BUFFERED, FILE_ANY_ACCESS) // in: VR_LOAD_DIFFERENCING_HIVE
#define IOCTL_VR_CREATE_NAMESPACE_NODE              CTL_CODE(FILE_DEVICE_UNKNOWN, 3, METHOD_BUFFERED, FILE_ANY_ACCESS) // in: VR_CREATE_NAMESPACE_NODE
#define IOCTL_VR_MODIFY_FLAGS                       CTL_CODE(FILE_DEVICE_UNKNOWN, 4, METHOD_BUFFERED, FILE_ANY_ACCESS) // in: VR_MODIFY_FLAGS
#define IOCTL_VR_CREATE_MULTIPLE_NAMESPACE_NODES    CTL_CODE(FILE_DEVICE_UNKNOWN, 5, METHOD_BUFFERED, FILE_ANY_ACCESS) // in: VR_CREATE_MULTIPLE_NAMESPACE_NODES
#define IOCTL_VR_UNLOAD_DYNAMICALLY_LOADED_HIVES    CTL_CODE(FILE_DEVICE_UNKNOWN, 6, METHOD_BUFFERED, FILE_ANY_ACCESS) // in: VR_UNLOAD_DYNAMICALLY_LOADED_HIVES
#define IOCTL_VR_GET_VIRTUAL_ROOT_KEY               CTL_CODE(FILE_DEVICE_UNKNOWN, 7, METHOD_BUFFERED, FILE_ANY_ACCESS) // in: VR_GET_VIRTUAL_ROOT; out: VR_GET_VIRTUAL_ROOT_RESULT
#define IOCTL_VR_LOAD_DIFFERENCING_HIVE_FOR_HOST    CTL_CODE(FILE_DEVICE_UNKNOWN, 8, METHOD_BUFFERED, FILE_ANY_ACCESS) // in: VR_LOAD_DIFFERENCING_HIVE_FOR_HOST
#define IOCTL_VR_UNLOAD_DIFFERENCING_HIVE_FOR_HOST  CTL_CODE(FILE_DEVICE_UNKNOWN, 9, METHOD_BUFFERED, FILE_ANY_ACCESS) // in: VR_UNLOAD_DIFFERENCING_HIVE_FOR_HOST

// private
typedef struct _VR_INITIALIZE_JOB_FOR_VREG
{
    HANDLE Job;
} VR_INITIALIZE_JOB_FOR_VREG, *PVR_INITIALIZE_JOB_FOR_VREG;

// rev
#define VR_FLAG_INHERIT_TRUST_CLASS 0x00000001
#define VR_FLAG_WRITE_THROUGH_HIVE 0x00000002 // since REDSTONE2
#define VR_FLAG_LOCAL_MACHINE_TRUST_CLASS 0x00000004 // since 21H1

// rev + private
typedef struct _VR_LOAD_DIFFERENCING_HIVE
{
    HANDLE Job;
    ULONG NextLayerIsHost;
    ULONG Flags; // VR_FLAG_*
    ULONG LoadFlags; // NtLoadKeyEx flags
    USHORT KeyPathLength;
    USHORT HivePathLength;
    USHORT NextLayerKeyPathLength;
    HANDLE FileAccessToken; // since 20H1
    WCHAR Strings[ANYSIZE_ARRAY];
    // ...
    // WCHAR KeyPath[1];
    // WCHAR HivePath[1];
    // WCHAR NextLayerKeyPath[1];
} VR_LOAD_DIFFERENCING_HIVE, *PVR_LOAD_DIFFERENCING_HIVE;

// rev + private
typedef struct _VR_CREATE_NAMESPACE_NODE
{
    HANDLE Job;
    USHORT ContainerPathLength;
    USHORT HostPathLength;
    ULONG Flags;
    ACCESS_MASK AccessMask; // since 20H1
    WCHAR Strings[ANYSIZE_ARRAY];
    // ...
    // WCHAR ContainerPath[1];
    // WCHAR HostPath[1];
} VR_CREATE_NAMESPACE_NODE, *PVR_CREATE_NAMESPACE_NODE;

// private
typedef struct _VR_MODIFY_FLAGS
{
    HANDLE Job;
    ULONG AddFlags;
    ULONG RemoveFlags;
} VR_MODIFY_FLAGS, *PVR_MODIFY_FLAGS;

// private
typedef struct _NAMESPACE_NODE_DATA
{
    ACCESS_MASK AccessMask;
    USHORT ContainerPathLength;
    USHORT HostPathLength;
    ULONG Flags;
    WCHAR Strings[ANYSIZE_ARRAY];
    // ...
    // WCHAR ContainerPath[1];
    // WCHAR HostPath[1];
} NAMESPACE_NODE_DATA, *PNAMESPACE_NODE_DATA;

// private
typedef struct _VR_CREATE_MULTIPLE_NAMESPACE_NODES
{
    HANDLE Job;
    ULONG NumNewKeys;
    NAMESPACE_NODE_DATA Keys[1];
} VR_CREATE_MULTIPLE_NAMESPACE_NODES, *PVR_CREATE_MULTIPLE_NAMESPACE_NODES;

// private
typedef struct _VR_UNLOAD_DYNAMICALLY_LOADED_HIVES
{
    HANDLE Job;
} VR_UNLOAD_DYNAMICALLY_LOADED_HIVES, *PVR_UNLOAD_DYNAMICALLY_LOADED_HIVES;

// rev
#define VR_KEY_COMROOT 0          // \Registry\ComRoot\Classes
#define VR_KEY_MACHINE_SOFTWARE 1 // \Registry\Machine\Software // since REDSTONE2
#define VR_KEY_CONTROL_SET 2      // \Registry\Machine\System\ControlSet001 // since REDSTONE2

// rev
typedef struct _VR_GET_VIRTUAL_ROOT
{
    HANDLE Job;
    ULONG Index; // VR_KEY_* // since REDSTONE2
} VR_GET_VIRTUAL_ROOT, *PVR_GET_VIRTUAL_ROOT;

// rev
typedef struct _VR_GET_VIRTUAL_ROOT_RESULT
{
    HANDLE Key;
} VR_GET_VIRTUAL_ROOT_RESULT, *PVR_GET_VIRTUAL_ROOT_RESULT;

// rev
typedef struct _VR_LOAD_DIFFERENCING_HIVE_FOR_HOST
{
    ULONG LoadFlags; // NtLoadKeyEx flags
    ULONG Flags; // VR_FLAG_* // since REDSTONE2
    USHORT KeyPathLength;
    USHORT HivePathLength;
    USHORT NextLayerKeyPathLength;
    HANDLE FileAccessToken; // since 20H1
    WCHAR Strings[ANYSIZE_ARRAY];
    // ...
    // WCHAR KeyPath[1];
    // WCHAR HivePath[1];
    // WCHAR NextLayerKeyPath[1];
} VR_LOAD_DIFFERENCING_HIVE_FOR_HOST, *PVR_LOAD_DIFFERENCING_HIVE_FOR_HOST;

// rev
typedef struct _VR_UNLOAD_DIFFERENCING_HIVE_FOR_HOST
{
    ULONG Reserved;
    USHORT TargetKeyPathLength;
    WCHAR TargetKeyPath[ANYSIZE_ARRAY];
} VR_UNLOAD_DIFFERENCING_HIVE_FOR_HOST, *PVR_UNLOAD_DIFFERENCING_HIVE_FOR_HOST;

//
// Key Open/Create Options
//
#define REG_OPTION_RESERVED             (0x00000000L)   // Parameter is reserved.
#define REG_OPTION_NON_VOLATILE         (0x00000000L)   // Key is preserved when system is rebooted.
#define REG_OPTION_VOLATILE             (0x00000001L)   // Key is not preserved when system is rebooted
#define REG_OPTION_CREATE_LINK          (0x00000002L)   // Created key is a symbolic link
#define REG_OPTION_BACKUP_RESTORE       (0x00000004L)   // open for backup or restore special access rules privilege required
#define REG_OPTION_OPEN_LINK            (0x00000008L)   // Open symbolic link
#define REG_OPTION_DONT_VIRTUALIZE      (0x00000010L)   // Disable Open/Read/Write virtualization for this open and the resulting handle.

#ifndef REG_LEGAL_OPTION
#define REG_LEGAL_OPTION \
    (REG_OPTION_RESERVED | REG_OPTION_NON_VOLATILE |\
     REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK |\
     REG_OPTION_BACKUP_RESTORE | REG_OPTION_OPEN_LINK |\
     REG_OPTION_DONT_VIRTUALIZE)
#endif

#ifndef REG_OPEN_LEGAL_OPTION
#define REG_OPEN_LEGAL_OPTION \
    (REG_OPTION_RESERVED | REG_OPTION_BACKUP_RESTORE | \
     REG_OPTION_OPEN_LINK | REG_OPTION_DONT_VIRTUALIZE)
#endif

//
// Key creation/open disposition
//
#define REG_CREATED_NEW_KEY         (0x00000001L)   // New Registry Key created
#define REG_OPENED_EXISTING_KEY     (0x00000002L)   // Existing Key opened

//
// hive format to be used by Reg(Nt)SaveKeyEx
//
#define REG_STANDARD_FORMAT     1
#define REG_LATEST_FORMAT       2
#define REG_NO_COMPRESSION      4

//
// Key restore & hive load flags
//
#define REG_WHOLE_HIVE_VOLATILE         (0x00000001L)   // Restore whole hive volatile
#define REG_REFRESH_HIVE                (0x00000002L)   // Unwind changes to last flush
#define REG_NO_LAZY_FLUSH               (0x00000004L)   // Never lazy flush this hive
#define REG_FORCE_RESTORE               (0x00000008L)   // Force the restore process even when we have open handles on subkeys
#define REG_APP_HIVE                    (0x00000010L)   // Loads the hive visible to the calling process
#define REG_PROCESS_PRIVATE             (0x00000020L)   // Hive cannot be mounted by any other process while in use
#define REG_START_JOURNAL               (0x00000040L)   // Starts Hive Journal
#define REG_HIVE_EXACT_FILE_GROWTH      (0x00000080L)   // Grow hive file in exact 4k increments
#define REG_HIVE_NO_RM                  (0x00000100L)   // No RM is started for this hive (no transactions)
#define REG_HIVE_SINGLE_LOG             (0x00000200L)   // Legacy single logging is used for this hive
#define REG_BOOT_HIVE                   (0x00000400L)   // This hive might be used by the OS loader
#define REG_LOAD_HIVE_OPEN_HANDLE       (0x00000800L)   // Load the hive and return a handle to its root kcb
#define REG_FLUSH_HIVE_FILE_GROWTH      (0x00001000L)   // Flush changes to primary hive file size as part of all flushes
#define REG_OPEN_READ_ONLY              (0x00002000L)   // Open a hive's files in read-only mode
#define REG_IMMUTABLE                   (0x00004000L)   // Load the hive, but don't allow any modification of it
#define REG_NO_IMPERSONATION_FALLBACK   (0x00008000L)   // Do not fall back to impersonating the caller if hive file access fails
#define REG_APP_HIVE_OPEN_READ_ONLY     (REG_OPEN_READ_ONLY)   // Open an app hive's files in read-only mode (if the hive was not previously loaded)

//
// Unload Flags
//
#define REG_FORCE_UNLOAD            1
#define REG_UNLOAD_LEGAL_FLAGS      (REG_FORCE_UNLOAD)

/**
 * Creates a new registry key routine or opens an existing one.
 *
 * @param[out] KeyHandle A pointer to a handle that receives the key handle.
 * @param[in] DesiredAccess The access mask that specifies the desired access rights.
 * @param[in] ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * @param[in] TitleIndex Reserved.
 * @param[in, optional] Class A pointer to a UNICODE_STRING structure that specifies the class of the key.
 * @param[in] CreateOptions The options to use when creating the key.
 * @param[out, optional] Disposition A pointer to a variable that receives the disposition value.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Reserved_ ULONG TitleIndex,
    _In_opt_ PCUNICODE_STRING Class,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG Disposition
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
/**
 * Creates a new registry key or opens an existing one, and it associates the key with a transaction.
 *
 * @param[out] KeyHandle A pointer to a handle that receives the key handle.
 * @param[in] DesiredAccess The access mask that specifies the desired access rights.
 * @param[in] ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * @param[in] TitleIndex Reserved.
 * @param[in, optional] Class A pointer to a UNICODE_STRING structure that specifies the class of the key.
 * @param[in] CreateOptions The options to use when creating the key.
 * @param[in] TransactionHandle A handle to the transaction.
 * @param[out, optional] Disposition A pointer to a variable that receives the disposition value.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateKeyTransacted(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Reserved_ ULONG TitleIndex,
    _In_opt_ PCUNICODE_STRING Class,
    _In_ ULONG CreateOptions,
    _In_ HANDLE TransactionHandle,
    _Out_opt_ PULONG Disposition
    );
#endif

/**
 * Opens an existing registry key.
 *
 * @param[out] KeyHandle A pointer to a handle that receives the key handle.
 * @param[in] DesiredAccess The access mask that specifies the desired access rights.
 * @param[in] ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * @return NTSTATUS Successful or errant status.
 * @remarks NtOpenKey ignores the security information in the ObjectAttributes structure.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
/**
 * Opens an existing registry key and associates the key with a transaction.
 *
 * @param[out] KeyHandle A pointer to a handle that receives the key handle.
 * @param[in] DesiredAccess The access mask that specifies the desired access rights.
 * @param[in] ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * @param[in] TransactionHandle A handle to the transaction.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenKeyTransacted(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE TransactionHandle
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_7)
/**
 * Opens an existing registry key with extended options.
 *
 * @param[out] KeyHandle A pointer to a handle that receives the key handle.
 * @param[in] DesiredAccess The access mask that specifies the desired access rights.
 * @param[in] ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * @param[in] OpenOptions The options to use when opening the key.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenKeyEx(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG OpenOptions
    );

/**
 * Opens an existing registry key in a transaction with extended options.
 *
 * @param[out] KeyHandle A pointer to a handle that receives the key handle.
 * @param[in] DesiredAccess The access mask that specifies the desired access rights.
 * @param[in] ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * @param[in] OpenOptions The options to use when opening the key.
 * @param[in] TransactionHandle A handle to the transaction.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenKeyTransactedEx(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG OpenOptions,
    _In_ HANDLE TransactionHandle
    );
#endif

/**
 * Deletes a registry key.
 *
 * @param[in] KeyHandle A handle to the key to be deleted.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteKey(
    _In_ HANDLE KeyHandle
    );

/**
 * Renames a registry key.
 *
 * @param[in] KeyHandle A handle to the key to be renamed.
 * @param[in] NewName A pointer to a UNICODE_STRING structure that specifies the new name of the key.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRenameKey(
    _In_ HANDLE KeyHandle,
    _In_ PCUNICODE_STRING NewName
    );

/**
 * Deletes a value from a registry key.
 *
 * @param[in] KeyHandle A handle to the key that contains the value to be deleted.
 * @param[in] ValueName A pointer to a UNICODE_STRING structure that specifies the name of the value to be deleted.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteValueKey(
    _In_ HANDLE KeyHandle,
    _In_ PCUNICODE_STRING ValueName
    );

/**
 * Queries information about a registry key.
 *
 * @param[in] KeyHandle A handle to the key to be queried.
 * @param[in] KeyInformationClass The type of information to be queried.
 * @param[out] KeyInformation A pointer to a buffer that receives the key information.
 * @param[in] Length The size of the buffer.
 * @param[out] ResultLength A pointer to a variable that receives the size of the data returned.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_INFORMATION_CLASS KeyInformationClass,
    _Out_writes_bytes_to_opt_(Length, *ResultLength) PVOID KeyInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
    );

/**
 * Sets information for a registry key.
 *
 * @param[in] KeyHandle A handle to the key to be modified.
 * @param[in] KeySetInformationClass The type of information to be set.
 * @param[in] KeySetInformation A pointer to a buffer that contains the key information.
 * @param[in] KeySetInformationLength The size of the buffer.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_SET_INFORMATION_CLASS KeySetInformationClass,
    _In_reads_bytes_(KeySetInformationLength) PVOID KeySetInformation,
    _In_ ULONG KeySetInformationLength
    );

/**
 * Queries the value of a registry key.
 *
 * @param[in] KeyHandle A handle to the key to be queried.
 * @param[in] ValueName A pointer to a UNICODE_STRING structure that specifies the name of the value to be queried.
 * @param[in] KeyValueInformationClass The type of information to be queried.
 * @param[out] KeyValueInformation A pointer to a buffer that receives the value information.
 * @param[in] Length The size of the buffer.
 * @param[out] ResultLength A pointer to a variable that receives the size of the data returned.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryValueKey(
    _In_ HANDLE KeyHandle,
    _In_ PCUNICODE_STRING ValueName,
    _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    _Out_writes_bytes_to_opt_(Length, *ResultLength) PVOID KeyValueInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
    );

/**
 * Sets the value of a registry key.
 *
 * @param[in] KeyHandle A handle to the key to be modified.
 * @param[in] ValueName A pointer to a UNICODE_STRING structure that specifies the name of the value to be set.
 * @param[in, optional] TitleIndex Reserved.
 * @param[in] Type The type of the value.
 * @param[in] Data A pointer to a buffer that contains the value data.
 * @param[in] DataSize The size of the buffer.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetValueKey(
    _In_ HANDLE KeyHandle,
    _In_ PCUNICODE_STRING ValueName,
    _In_opt_ ULONG TitleIndex,
    _In_ ULONG Type,
    _In_reads_bytes_opt_(DataSize) PVOID Data,
    _In_ ULONG DataSize
    );

/**
 * Queries multiple values of a registry key.
 *
 * @param[in] KeyHandle A handle to the key to be queried.
 * @param[in, out] ValueEntries A pointer to an array of KEY_VALUE_ENTRY structures that specify the values to be queried.
 * @param[in] EntryCount The number of entries in the array.
 * @param[out] ValueBuffer A pointer to a buffer that receives the value data.
 * @param[in, out] BufferLength A pointer to a variable that specifies the size of the buffer and receives the size of the data returned.
 * @param[out, optional] RequiredBufferLength A pointer to a variable that receives the size of the buffer required to hold the data.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryMultipleValueKey(
    _In_ HANDLE KeyHandle,
    _Inout_updates_(EntryCount) PKEY_VALUE_ENTRY ValueEntries,
    _In_ ULONG EntryCount,
    _Out_writes_bytes_(*BufferLength) PVOID ValueBuffer,
    _Inout_ PULONG BufferLength,
    _Out_opt_ PULONG RequiredBufferLength
    );

/**
 * Enumerates the subkeys of a registry key.
 *
 * @param[in] KeyHandle A handle to the key to be enumerated.
 * @param[in] Index The index of the subkey to be enumerated.
 * @param[in] KeyInformationClass The type of information to be queried.
 * @param[out] KeyInformation A pointer to a buffer that receives the key information.
 * @param[in] Length The size of the buffer.
 * @param[out] ResultLength A pointer to a variable that receives the size of the data returned.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnumerateKey(
    _In_ HANDLE KeyHandle,
    _In_ ULONG Index,
    _In_ KEY_INFORMATION_CLASS KeyInformationClass,
    _Out_writes_bytes_to_opt_(Length, *ResultLength) PVOID KeyInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
    );

/**
 * Enumerates the values of a registry key.
 *
 * @param[in] KeyHandle A handle to the key to be enumerated.
 * @param[in] Index The index of the value to be enumerated.
 * @param[in] KeyValueInformationClass The type of information to be queried.
 * @param[out] KeyValueInformation A pointer to a buffer that receives the value information.
 * @param[in] Length The size of the buffer.
 * @param[out] ResultLength A pointer to a variable that receives the size of the data returned.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnumerateValueKey(
    _In_ HANDLE KeyHandle,
    _In_ ULONG Index,
    _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    _Out_writes_bytes_to_opt_(Length, *ResultLength) PVOID KeyValueInformation,
    _In_ ULONG Length,
    _Out_ PULONG ResultLength
    );

/**
 * Flushes the changes to a registry key.
 *
 * @param[in] KeyHandle A handle to the key to be flushed.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushKey(
    _In_ HANDLE KeyHandle
    );

/**
 * Compacts the specified registry keys.
 *
 * @param[in] Count The number of keys to be compacted.
 * @param[in] KeyArray An array of handles to the keys to be compacted.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCompactKeys(
    _In_ ULONG Count,
    _In_reads_(Count) HANDLE KeyArray[]
    );

/**
 * Compresses a registry key.
 *
 * @param[in] KeyHandle A handle to the key to be compressed.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCompressKey(
    _In_ HANDLE KeyHandle
    );

/**
 * Loads a registry key from a file.
 *
 * @param[in] TargetKey A pointer to an OBJECT_ATTRIBUTES structure that specifies the target key.
 * @param[in] SourceFile A pointer to an OBJECT_ATTRIBUTES structure that specifies the source file.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtLoadKey(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_ POBJECT_ATTRIBUTES SourceFile
    );

/**
 * Loads a registry key from a file with additional options.
 *
 * @param[in] TargetKey A pointer to an OBJECT_ATTRIBUTES structure that specifies the target key.
 * @param[in] SourceFile A pointer to an OBJECT_ATTRIBUTES structure that specifies the source file.
 * @param[in] Flags The options to use when loading the key.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtLoadKey2(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_ POBJECT_ATTRIBUTES SourceFile,
    _In_ ULONG Flags
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_SERVER_2003)
/**
 * Loads a registry key from a file with extended options.
 *
 * @param[in] TargetKey A pointer to an OBJECT_ATTRIBUTES structure that specifies the target key.
 * @param[in] SourceFile A pointer to an OBJECT_ATTRIBUTES structure that specifies the source file.
 * @param[in] Flags The options to use when loading the key.
 * @param[in, optional] TrustClassKey A handle to the trust class key.
 * @param[in, optional] Event A handle to an event.
 * @param[in, optional] DesiredAccess The access mask that specifies the desired access rights.
 * @param[out, optional] RootHandle A pointer to a handle that receives the root handle.
 * @param[in, reserved] Reserved Reserved.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtLoadKeyEx(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_ POBJECT_ATTRIBUTES SourceFile,
    _In_ ULONG Flags,
    _In_opt_ HANDLE TrustClassKey,
    _In_opt_ HANDLE Event,
    _In_opt_ ACCESS_MASK DesiredAccess,
    _Out_opt_ PHANDLE RootHandle,
    _Reserved_ PVOID Reserved // previously PIO_STATUS_BLOCK
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
// rev by tyranid
/**
 * Loads a registry key from a file with extended parameters.
 *
 * @param[in] TargetKey A pointer to an OBJECT_ATTRIBUTES structure that specifies the target key.
 * @param[in] SourceFile A pointer to an OBJECT_ATTRIBUTES structure that specifies the source file.
 * @param[in] Flags The options to use when loading the key.
 * @param[in] ExtendedParameters A pointer to an array of extended parameters.
 * @param[in] ExtendedParameterCount The number of extended parameters.
 * @param[in, optional] DesiredAccess The access mask that specifies the desired access rights.
 * @param[out, optional] RootHandle A pointer to a handle that receives the root handle.
 * @param[in, reserved] Reserved Reserved.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtLoadKey3(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_ POBJECT_ATTRIBUTES SourceFile,
    _In_ ULONG Flags,
    _In_reads_(ExtendedParameterCount) PCM_EXTENDED_PARAMETER ExtendedParameters,
    _In_ ULONG ExtendedParameterCount,
    _In_opt_ ACCESS_MASK DesiredAccess,
    _Out_opt_ PHANDLE RootHandle,
    _Reserved_ PVOID Reserved
    );
#endif

/**
 * Replaces a registry key.
 *
 * @param[in] NewFile A pointer to an OBJECT_ATTRIBUTES structure that specifies the new file.
 * @param[in] TargetHandle A handle to the target key.
 * @param[in] OldFile A pointer to an OBJECT_ATTRIBUTES structure that specifies the old file.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplaceKey(
    _In_ POBJECT_ATTRIBUTES NewFile,
    _In_ HANDLE TargetHandle,
    _In_ POBJECT_ATTRIBUTES OldFile
    );

/**
 * Saves the specified registry key to a file.
 *
 * @param KeyHandle Handle to the registry key.
 * @param FileHandle Handle to the file where the key will be saved.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSaveKey(
    _In_ HANDLE KeyHandle,
    _In_ HANDLE FileHandle
    );

/**
 * Saves the specified registry key to a file with a specified format.
 *
 * @param KeyHandle Handle to the registry key.
 * @param FileHandle Handle to the file where the key will be saved.
 * @param Format Format in which the key will be saved.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSaveKeyEx(
    _In_ HANDLE KeyHandle,
    _In_ HANDLE FileHandle,
    _In_ ULONG Format
    );

/**
 * Merges two registry keys and saves the result to a file.
 *
 * @param HighPrecedenceKeyHandle Handle to the high precedence registry key.
 * @param LowPrecedenceKeyHandle Handle to the low precedence registry key.
 * @param FileHandle Handle to the file where the merged key will be saved.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSaveMergedKeys(
    _In_ HANDLE HighPrecedenceKeyHandle,
    _In_ HANDLE LowPrecedenceKeyHandle,
    _In_ HANDLE FileHandle
    );

/**
 * Restores a registry key from a file.
 *
 * @param KeyHandle Handle to the registry key.
 * @param FileHandle Handle to the file from which the key will be restored.
 * @param Flags Flags for the restore operation.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRestoreKey(
    _In_ HANDLE KeyHandle,
    _In_ HANDLE FileHandle,
    _In_ ULONG Flags
    );

/**
 * Unloads a registry key.
 *
 * @param TargetKey Pointer to the object attributes of the target key.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnloadKey(
    _In_ POBJECT_ATTRIBUTES TargetKey
    );

#if PHNT_VERSION >= PHNT_WINDOWS_SERVER_2003
/**
 * Unloads a registry key with additional flags.
 *
 * @param TargetKey Pointer to the object attributes of the target key.
 * @param Flags Flags for the unload operation.
 * @return NTSTATUS Successful or errant status.
 * @remarks Valid flags are REG_FORCE_UNLOAD and REG_UNLOAD_LEGAL_FLAGS.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnloadKey2(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_ ULONG Flags
    );
#endif

/**
 * Unloads a registry key and optionally signals an event.
 *
 * @param TargetKey Pointer to the object attributes of the target key.
 * @param Event Optional handle to an event to be signaled.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnloadKeyEx(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_opt_ HANDLE Event
    );

/**
 * Notifies of changes to a registry key.
 *
 * @param KeyHandle Handle to the registry key.
 * @param Event Optional handle to an event to be signaled.
 * @param ApcRoutine Optional APC routine to be called.
 * @param ApcContext Optional context for the APC routine.
 * @param IoStatusBlock Pointer to an IO status block.
 * @param CompletionFilter Filter for the types of changes to notify.
 * @param WatchTree Whether to watch the entire tree.
 * @param Buffer Optional buffer for change data.
 * @param BufferSize Size of the buffer.
 * @param Asynchronous Whether the operation is asynchronous.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtNotifyChangeKey(
    _In_ HANDLE KeyHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG CompletionFilter,
    _In_ BOOLEAN WatchTree,
    _Out_writes_bytes_opt_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _In_ BOOLEAN Asynchronous
    );

/**
 * Requests notification when a registry key or any of its subkeys changes.
 *
 * @param MasterKeyHandle A handle to an open key. The handle must be opened with the KEY_NOTIFY access right.
 * @param Count The number of subkeys under the key specified by the MasterKeyHandle parameter.
 * @param SubordinateObjects Pointer to an array of OBJECT_ATTRIBUTES structures, one for each subkey. This array can contain one OBJECT_ATTRIBUTES structure.
 * @param Event A handle to an event created by the caller. If Event is not NULL, the caller waits until the operation succeeds, at which time the event is signaled.
 * @param ApcRoutine A pointer to an asynchronous procedure call (APC) function supplied by the caller. If ApcRoutine is not NULL, the specified APC function executes after the operation completes.
 * @param ApcContext A pointer to a context supplied by the caller for its APC function. This value is passed to the APC function when it is executed. The Asynchronous parameter must be TRUE. If ApcContext is specified, the Event parameter must be NULL.
 * @param IoStatusBlock A pointer to an IO_STATUS_BLOCK structure that contains the final status and information about the operation. For successful calls that return data, the number of bytes written to the Buffer parameter is supplied in the Information member of the IO_STATUS_BLOCK structure.
 * @param CompletionFilter A bitmap of operations that trigger notification. This parameter can be one or more of the following flags. REG_NOTIFY_CHANGE_NAME, REG_NOTIFY_CHANGE_ATTRIBUTES, REG_NOTIFY_CHANGE_LAST_SET, REG_NOTIFY_CHANGE_SECURITY.
 * @param WatchTree If this parameter is TRUE, the caller is notified about changes to all subkeys of the specified key. If this parameter is FALSE, the caller is notified only about changes to the specified key.
 * @param Buffer Reserved for system use. This parameter must be NULL.
 * @param BufferSize Reserved for system use. This parameter must be zero.
 * @param Asynchronous Whether the operation is asynchronous.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtNotifyChangeMultipleKeys(
    _In_ HANDLE MasterKeyHandle,
    _In_opt_ ULONG Count,
    _In_reads_opt_(Count) OBJECT_ATTRIBUTES SubordinateObjects[],
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG CompletionFilter,
    _In_ BOOLEAN WatchTree,
    _Out_writes_bytes_opt_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _In_ BOOLEAN Asynchronous
    );

/**
 * Queries the number of open subkeys of a registry key.
 *
 * @param TargetKey Pointer to the object attributes of the target key.
 * @param HandleCount Pointer to a variable to receive the handle count.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryOpenSubKeys(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _Out_ PULONG HandleCount
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_SERVER_2003)
/**
 * Queries the open subkeys of a registry key with additional information.
 *
 * @param TargetKey Pointer to the object attributes of the target key.
 * @param BufferLength Length of the buffer.
 * @param Buffer Optional buffer to receive the subkey information.
 * @param RequiredSize Pointer to a variable to receive the required size.
 * @return NTSTATUS Successful or errant status.
 * @remarks Returns an array of KEY_OPEN_SUBKEYS_INFORMATION structures.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryOpenSubKeysEx(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_ ULONG BufferLength,
    _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
    _Out_ PULONG RequiredSize
    );
#endif

/**
 * Initializes the registry.
 *
 * @param BootCondition Condition for the boot.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitializeRegistry(
    _In_ USHORT BootCondition
    );

/**
 * Locks the registry key and prevents changes from being written to disk.
 *
 * @param KeyHandle Handle to the registry key.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtLockRegistryKey(
    _In_ HANDLE KeyHandle
    );

/**
 * Locks the product activation keys.
 *
 * @param pPrivateVer Optional pointer to a private version variable.
 * @param pSafeMode Optional pointer to a safe mode variable.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtLockProductActivationKeys(
    _Inout_opt_ ULONG *pPrivateVer,
    _Out_opt_ ULONG *pSafeMode
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
/**
 * Freezes the registry and prevents changes from being flushed to disk.
 *
 * @param TimeOutInSeconds Timeout in seconds.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreezeRegistry(
    _In_ ULONG TimeOutInSeconds
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
/**
 * Thaws the registry and enables flushing changes to disk.
 *
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtThawRegistry(
    VOID
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
/**
 * Creates a registry transaction.
 *
 * @param RegistryTransactionHandle Pointer to a variable to receive the handle.
 * @param DesiredAccess Desired access mask.
 * @param ObjAttributes Optional pointer to object attributes.
 * @param CreateOptions Reserved for future use.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateRegistryTransaction(
    _Out_ HANDLE *RegistryTransactionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjAttributes,
    _Reserved_ ULONG CreateOptions
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
/**
 * Opens a registry transaction.
 *
 * @param RegistryTransactionHandle Pointer to a variable to receive the handle.
 * @param DesiredAccess Desired access mask.
 * @param ObjAttributes Pointer to object attributes.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenRegistryTransaction(
    _Out_ HANDLE *RegistryTransactionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjAttributes
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
/**
 * Commits a registry transaction.
 *
 * @param RegistryTransactionHandle Handle to the registry transaction.
 * @param Flags Reserved for future use.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCommitRegistryTransaction(
    _In_ HANDLE RegistryTransactionHandle,
    _Reserved_ ULONG Flags
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
/**
 * Rolls back a registry transaction.
 *
 * @param RegistryTransactionHandle Handle to the registry transaction.
 * @param Flags Reserved for future use.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollbackRegistryTransaction(
    _In_ HANDLE RegistryTransactionHandle,
    _Reserved_ ULONG Flags
    );
#endif

#endif
