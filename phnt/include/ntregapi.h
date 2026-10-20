/*
 * Registry support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTREGAPI_H
#define _NTREGAPI_H

//
// Registry
//

#define REG_MAX_KEY_VALUE_NAME_LENGTH 32767
#define REG_MAX_KEY_NAME_LENGTH 512

/**
 * Selects metadata for NtQueryKey or NtEnumerateKey; use basic/node information
 * to identify a key and full/cached information to size later enumeration buffers.
 * Counts and maxima are observations, not reservations or stable snapshots.
 * \remarks The native x64 and WoW64 paths accept query
 * classes 0 through 8 and enumeration classes 0 through 2. Other classes return
 * STATUS_INVALID_PARAMETER; MaxKeyInfoClass is a sentinel, not a request class.
 * Enumeration requires KEY_ENUMERATE_SUB_KEYS. Queries require KEY_QUERY_VALUE
 * except KeyNameInformation and KeyHandleTagsInformation: these require a valid
 * key handle with any nonzero granted access mask, not a zero-access handle.
 *
 * Names, class strings, lengths and offsets use bytes, not WCHAR counts. Variable
 * records can be partial on STATUS_BUFFER_OVERFLOW while retaining the full
 * NameLength/ClassLength. ResultLength is then a required size, not a safe copy
 * length. Fixed query records require at least 40 bytes for cached information,
 * 12 for flags, and 4 for virtualization, handle tags and trust information.
 * Prefer naturally aligned buffers; see each API for WoW64 realignment behavior.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ne-wdm-_key_information_class
 */
typedef enum _KEY_INFORMATION_CLASS
{
    KeyBasicInformation, // q: KEY_BASIC_INFORMATION
    KeyNodeInformation, // q: KEY_NODE_INFORMATION
    KeyFullInformation, // q: KEY_FULL_INFORMATION
    KeyNameInformation, // q: KEY_NAME_INFORMATION
    KeyCachedInformation, // q: KEY_CACHED_INFORMATION
    KeyFlagsInformation, // q: KEY_FLAGS_INFORMATION
    KeyVirtualizationInformation, // q: KEY_VIRTUALIZATION_INFORMATION
    KeyHandleTagsInformation, // q: KEY_HANDLE_TAGS_INFORMATION
    KeyTrustInformation, // q: KEY_TRUST_INFORMATION
    KeyLayerInformation, // q: KEY_LAYER_INFORMATION; rejected by NtQueryKey on build 26100
    MaxKeyInfoClass
} KEY_INFORMATION_CLASS;

/**
 * Identifies a queried key or an enumerated child by its final path component
 * and last-write time. Name is not a full registry path and is not NUL-terminated.
 * The fixed prefix is 16 bytes; the complete record requires 16 + NameLength.
 * NameLength remains the full byte length on overflow, even when less is copied.
 * Do not use sizeof(KEY_BASIC_INFORMATION) as the variable-record prefix size.
 * The ordinary output sets TitleIndex to 0; ignore it as legacy metadata.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_key_basic_information
 */
typedef struct _KEY_BASIC_INFORMATION
{
    LARGE_INTEGER LastWriteTime;                    // Absolute last-write time, in 100-nanosecond intervals since January 1, 1601 (UTC).
    ULONG TitleIndex;                               // Legacy localization metadata; ordinary query/enumeration output sets this to 0.
    ULONG NameLength;                               // The size, in bytes, of the key name string in the Name array.
    _Field_size_bytes_(NameLength) WCHAR Name[1];   // The name of the registry key. This string is not null-terminated.
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

/**
 * Returns a key's final name component and optional class string. Both strings
 * are counted bytes without NUL termination. ClassOffset is relative to the
 * record start, not relative to Name. The fixed prefix before Name is 24 bytes.
 *
 * In the ordinary path, a nonempty class follows the name at
 * align_up(24 + NameLength, 4); total size is ClassOffset + ClassLength.
 * With no class, ClassLength = 0, ClassOffset = 0xFFFFFFFF, and total size is
 * 24 + NameLength. Never dereference the absent-class sentinel.
 * NameLength/ClassLength remain full lengths on overflow; bound both ranges by
 * the supplied capacity before reading them. TitleIndex is 0 in ordinary output.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_key_node_information
 */
typedef struct _KEY_NODE_INFORMATION
{
    LARGE_INTEGER LastWriteTime;                    // Absolute last-write time, in 100-nanosecond intervals since January 1, 1601 (UTC).
    ULONG TitleIndex;                               // Legacy localization metadata; ordinary query/enumeration output sets this to 0.
    ULONG ClassOffset;                              // The byte offset from the start of this structure to the class name string. This string is not null-terminated.
    ULONG ClassLength;                              // The size, in bytes, in the class name string.
    ULONG NameLength;                               // The size, in bytes, of the key name string contained in the Name array.
    _Field_size_bytes_(NameLength) WCHAR Name[1];   // The name of the registry key. This string is not null-terminated.
    // ...
    // WCHAR Class[1];
} KEY_NODE_INFORMATION, *PKEY_NODE_INFORMATION;

/**
 * Returns the selected key's class, direct-subkey count, value count and maximum
 * name/class/data byte lengths. Use these maxima to plan buffers, but still handle
 * changes between calls. MaxNameLength/MaxClassLength describe direct subkeys;
 * MaxValueNameLength/MaxValueDataLength describe values of this key.
 * NtEnumerateKey returns these statistics for the selected child, not its parent.
 *
 * The fixed prefix before Class is 44 bytes. The ordinary path needs
 * 44 + ClassLength bytes, with ClassOffset = 44 for a nonempty class and
 * 0xFFFFFFFF when ClassLength = 0. Class is not NUL-terminated. No key name is
 * included. Overflow retains the full ClassLength, so it is not a copied count.
 * TitleIndex is 0 in ordinary output. LastWriteTime uses the absolute system-time
 * format (100-nanosecond intervals since January 1, 1601 UTC).
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_key_full_information
 */
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

/**
 * Returns a counted native registry path, rather than the final component used by KEY_BASIC_INFORMATION. The
 * path is not an HKEY_* alias or a NUL-terminated string. The prefix is 4 bytes; the complete record needs 4
 * + NameLength bytes. NameLength remains the full required name length when only a prefix is returned.
 * Querying this class does not require KEY_QUERY_VALUE, but a zero-access handle is rejected. Virtualization
 * can affect the path shown.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/ns-ntddk-_key_name_information
 */
typedef struct _KEY_NAME_INFORMATION
{
    ULONG NameLength;                               // The size, in bytes, of the key name string in the Name array.
    _Field_size_bytes_(NameLength) WCHAR Name[1];   // The name of the registry key. This string is not null-terminated.
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;

/**
 * Returns fixed-size cached key statistics without appending a name or class.
 * Use the counts/maxima to plan value and direct-subkey enumeration, not as a
 * guarantee that later calls see the same state. Lengths and maxima are bytes.
 * \remarks The native/WoW64 query requires at least 40 bytes and
 * returns ResultLength = 40. A shorter buffer returns STATUS_BUFFER_TOO_SMALL,
 * not a partial cached record. NameLength at offset 32 describes the final key
 * component, but no name follows it; query basic/name information for the text.
 * Bytes 36 through 39 are trailing padding, not another field. Ordinary output
 * sets TitleIndex to 0. Do not infer semantics from padding contents.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/ns-ntddk-_key_cached_information
 */
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
} KEY_CACHED_INFORMATION, *PKEY_CACHED_INFORMATION;

//
// REG_FLAG_*
//

// rev
/**
 * \def REG_FLAG_VOLATILE
 * \brief The registry key is volatile.
 *
 * When this flag is set, the key is not preserved when the system is rebooted.
 * Volatile keys exist only in memory and are lost when the system shuts down.
 */
#define REG_FLAG_VOLATILE 0x0001
// rev
/**
 * \def REG_FLAG_LINK
 * \brief The registry key is a symbolic link.
 *
 * When this flag is set, the key acts as a symbolic link to another registry key.
 * This allows redirection of registry operations to a different key location.
 */
#define REG_FLAG_LINK 0x0002

//
// REG_KEY_*
//

// rev
#define REG_KEY_RESERVED_FLAG 0x0001
// msdn
/**
 * \def REG_KEY_DONT_VIRTUALIZE
 * \brief The REG_KEY_DONT_VIRTUALIZE flag disables write registry virtualization.
 *
 * If this flag is set and a create key or set value operation fails because the caller
 * does not have sufficient access right to the parent key, the registry fails the operation.
 * If this flag is clear, the registry attempts to write the key or value in the virtual store.
 * The caller must have the KEY_READ right on the parent key.
 */
#define REG_KEY_DONT_VIRTUALIZE 0x0002
/**
 * \def REG_KEY_DONT_SILENT_FAIL
 * \brief The REG_KEY_DONT_SILENT_FAIL flag disables open registry virtualization.
 *
 * If this flag is set and an open operation fails on a key that has virtualization enabled,
 * the registry does not attempt to reopen the key. If this flag is clear, the registry attempts
 * to reopen the key with MAXIMUM_ALLOWED access instead of the requested access.
 */
#define REG_KEY_DONT_SILENT_FAIL 0x0004
/**
 * \def REG_KEY_RECURSE_FLAG
 * \brief If REG_KEY_RECURSE_FLAG is set, registry virtualization flags are propagated from the parent key.
 *
 * If this flag is clear, registry virtualization flags are not propagated.
 * Changing this flag affects only new descendent keys created after the flag is changed.
 * It does not set or clear these flags for existing descendent keys.
 */
#define REG_KEY_RECURSE_FLAG 0x0008

/**
 * Returns three ULONGs of key metadata for KeyFlagsInformation (12 bytes).
 * Wow64Flags is key metadata, not the KEY_WOW64_32KEY/KEY_WOW64_64KEY access mask;
 * the ordinary nontransactional getter reads its low 4 bits. KeyFlags
 * reports REG_FLAG_VOLATILE/REG_FLAG_LINK. ControlFlags contains REG_KEY_* bits
 * from a separate low-4-bit control field. This query does not change key state.
 * A short buffer returns STATUS_BUFFER_TOO_SMALL and required length 12.
 */
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
 *
 * KeyVirtualizationInformation is a 4-byte fixed query record. A shorter buffer
 * returns STATUS_BUFFER_TOO_SMALL with required length 4. The getter
 * derives candidate/enabled state using the caller's security subject and the
 * virtualization environment; it is not solely a persistent per-key property.
 * Ignore reserved bits. Querying this record does not enable virtualization.
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

/**
 * Reports the registry trust classification associated with the containing hive.
 * KeyTrustInformation requires at least 4 bytes and returns ResultLength = 4.
 * TrustedKey occupies bit 0. The ordinary getter sets it when bit 0 of
 * CMHIVE.Flags is clear and clears it when that hive flag is set. This is registry
 * metadata, not a general statement that the key's contents are secure, authentic
 * or safe to execute. Ignore the remaining 31 bits; this query does not grant
 * access or change the hive's trust classification.
 */
typedef struct _KEY_TRUST_INFORMATION
{
    ULONG TrustedKey : 1;
    ULONG Reserved : 31;
} KEY_TRUST_INFORMATION, *PKEY_TRUST_INFORMATION;

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

/**
 * Selects metadata changed by NtSetInformationKey, not a value's REG_* data.
 * Use write-time information to assign a key timestamp and the internal flag/tag
 * classes only when their build-specific metadata semantics are required.
 * \remarks The native x64 and WoW64 paths dispatch classes
 * 0 through 5. KeyWriteTimeInformation (0) requires exactly 8 bytes; supported
 * flag/tag classes (1 through 5) require exactly 4. Both shorter and longer inputs
 * return STATUS_INFO_LENGTH_MISMATCH. Unsupported infoclasses, including
 * KeySetLayerInformation (6) and MaxKeySetInfoClass (7), return
 * STATUS_INVALID_INFO_CLASS, unlike the query APIs' invalid-infoclass status.
 *
 * The ordinary handle check requires KEY_SET_VALUE for classes 0 through 4.
 * KeySetHandleTagsInformation requests no specific access and accepts a valid
 * zero-access key handle in this implementation. Registry callbacks, read-only keys/hives,
 * transactions and virtualization can affect the final outcome. Classes 0, 1 and
 * 4 have an access-denied virtualization fallback; this is not a blanket exemption
 * from the normal access requirement. MaxKeySetInfoClass is not a request class.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ne-wdm-_key_set_information_class
 */
typedef enum _KEY_SET_INFORMATION_CLASS
{
    KeyWriteTimeInformation, // s: KEY_WRITE_TIME_INFORMATION
    KeyWow64FlagsInformation, // s: KEY_WOW64_FLAGS_INFORMATION
    KeyControlFlagsInformation, // s: KEY_CONTROL_FLAGS_INFORMATION
    KeySetVirtualizationInformation, // s: KEY_SET_VIRTUALIZATION_INFORMATION
    KeySetDebugInformation, // s: KEY_SET_DEBUG_INFORMATION
    KeySetHandleTagsInformation, // s: KEY_HANDLE_TAGS_INFORMATION
    KeySetLayerInformation, // s: KEY_SET_LAYER_INFORMATION; rejected by NtSetInformationKey on build 26100
    MaxKeySetInfoClass
} KEY_SET_INFORMATION_CLASS;

/**
 * Assigns a key's last-write timestamp through KeyWriteTimeInformation (0).
 * Supply exactly 8 bytes. LastWriteTime is an absolute count of 100-nanosecond
 * intervals since January 1, 1601 UTC, not a relative timeout or time-zone value.
 * The ordinary setter stores the supplied value, including 0; zero is
 * not a request to use the current time. It updates the key node and cached time.
 *
 * Later key/value changes can replace this timestamp. In particular, successful
 * ordinary flag updates (classes 1 through 4) refresh last-write time even when
 * the flag value is unchanged. A successful call does not imply a physical hive
 * flush, and transactional/read-only/layered-key paths have additional rules.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_key_write_time_information
 */
typedef struct _KEY_WRITE_TIME_INFORMATION
{
    LARGE_INTEGER LastWriteTime; // Absolute last-write time, in 100-nanosecond intervals since January 1, 1601 (UTC).
} KEY_WRITE_TIME_INFORMATION, *PKEY_WRITE_TIME_INFORMATION;

/**
 * Supplies internal per-key metadata for KeyWow64FlagsInformation (1).
 * The request is exactly 4 bytes. The ordinary handler accepts only
 * UserFlags bits 0 through 3 (mask 0x0F); any higher bit returns
 * STATUS_INVALID_PARAMETER rather than being silently discarded. Accepted bits
 * replace the key's low-4-bit metadata field and can be queried using
 * KEY_FLAGS_INFORMATION.Wow64Flags. They are not the KEY_WOW64_32KEY or
 * KEY_WOW64_64KEY access flags and do not reopen the handle in another view.
 * The successful ordinary update also refreshes the key's last-write time.
 */
typedef struct _KEY_WOW64_FLAGS_INFORMATION
{
    ULONG UserFlags;
} KEY_WOW64_FLAGS_INFORMATION, *PKEY_WOW64_FLAGS_INFORMATION;

/**
 * Supplies internal per-key diagnostic metadata for KeySetDebugInformation (4). The request is exactly 4
 * bytes. The ordinary handler copies only Flags bits 0 through 7 into the key node and cached debug field;
 * high bits are discarded. Individual debug-bit meanings are not established here, so do not treat this as a
 * public logging configuration or enable arbitrary bits. The successful ordinary path refreshes last-write
 * time. Nonzero diagnostic flags change registry diagnostic behavior.
 */
typedef struct _KEY_SET_DEBUG_INFORMATION
{
    ULONG Flags;
} KEY_SET_DEBUG_INFORMATION, *PKEY_SET_DEBUG_INFORMATION;

/**
 * Supplies or retrieves metadata stored in the opened key body, not the key's persistent value data or access
 * mask. KeySetHandleTagsInformation requires exactly 4 input bytes, but the setter stores only the low 16
 * bits. KeyHandleTagsInformation query requires at least 4 bytes and zero-extends those stored bits into
 * HandleTags; its ResultLength is 4.
 *
 * The setter accepts a valid zero-access key handle. The query is deliberately different: it requires any
 * nonzero granted access, though not specifically KEY_QUERY_VALUE. Duplicated handles referencing the same
 * key body share tag changes; a separately opened handle has its own tag state. Initial tags need not be
 * zero, especially through WoW64.
 *
 * Unlike ordinary key-flag updates, this setter does not refresh the key's last-write time. It remains
 * subject to registry callbacks. Preserve existing tags unless intentionally changing their internal
 * semantics.
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
 * Supplies registry control metadata for KeyControlFlagsInformation (2).
 * The request is exactly 4 bytes. The ordinary handler stores only
 * the low 4 ControlFlags bits (mask 0x0F), unlike class 1 which rejects high bits.
 * Query the resulting field using KEY_FLAGS_INFORMATION.ControlFlags.
 *
 * The REG_KEY_DONT_VIRTUALIZE bit (0x2) also clears the stored VirtualSource
 * marker on this path; replacing the low control bits is not its only effect.
 * These internal controls are not access rights or a request to change a key's
 * security descriptor. A successful ordinary update refreshes last-write time.
 */
typedef struct _KEY_CONTROL_FLAGS_INFORMATION
{
    ULONG ControlFlags;
} KEY_CONTROL_FLAGS_INFORMATION, *PKEY_CONTROL_FLAGS_INFORMATION;

/**
 * Supplies internal virtualization-state commands for KeySetVirtualizationInformation (3). The request is
 * exactly 4 bytes; the input fields are not positioned like KEY_VIRTUALIZATION_INFORMATION's query bits.
 *
 * The ordinary handler is not a simple replacement of three flags:
 * - VirtualSource (input bit 2, mask 0x4) sets the stored source marker and takes
 * precedence: target/store inputs are not processed by that branch.
 * - Otherwise VirtualTarget (bit 0, mask 0x1) sets both target and store markers;
 * a clear target input clears the target marker but does not clear store.
 * - VirtualStore (bit 1, mask 0x2) sets store; a clear input does not clear it.
 * - Clearing VirtualSource in this request does not clear the stored source.
 * The control-flags setter clears source when DONT_VIRTUALIZE is set. Thus a zero input is not a reset of all
 * virtualization state. High input bits have no observed effect in this handler. These flags control
 * virtualization metadata; they do not independently establish access to the key.
 *
 * This request does not create a virtual-store hierarchy or grant access. Query results also depend on the
 * caller/environment. The successful ordinary update refreshes last-write time. Do not apply these internal
 * controls as a general registry redirection switch.
 */
typedef struct _KEY_SET_VIRTUALIZATION_INFORMATION
{
    ULONG VirtualTarget : 1;
    ULONG VirtualStore : 1;
    ULONG VirtualSource : 1; // true if key has been virtualized at least once
    ULONG Reserved : 29;
} KEY_SET_VIRTUALIZATION_INFORMATION, *PKEY_SET_VIRTUALIZATION_INFORMATION;

/**
 * The KEY_VALUE_INFORMATION_CLASS enumeration selects information returned for a registry value.
 * \remarks The kernel implementation accepts classes 0 through 4 in NtQueryValueKey and only classes 0 through 2
 * in NtEnumerateValueKey. Other classes return STATUS_INVALID_PARAMETER. Both calls require KEY_QUERY_VALUE.
 * Use 8-byte-aligned buffers for the documented portable Align64 contract. The x64
 * entry checks only 4-byte alignment for all query classes; its WoW64 query thunk
 * realigns non-8-byte-aligned output through temporary storage. Enumeration checks 4-byte alignment
 * on both paths. These build-specific observations do not relax the portable Align64 recommendation.
 * Names and data lengths are byte counts.
 * STATUS_BUFFER_TOO_SMALL means the fixed header does not fit; STATUS_BUFFER_OVERFLOW can return a partial
 * record. ResultLength reports the required total size, which can exceed the supplied Length.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ne-wdm-_key_value_information_class
 */
typedef enum _KEY_VALUE_INFORMATION_CLASS
{
    KeyValueBasicInformation, // q: KEY_VALUE_BASIC_INFORMATION
    KeyValueFullInformation, // q: KEY_VALUE_FULL_INFORMATION
    KeyValuePartialInformation, // q: KEY_VALUE_PARTIAL_INFORMATION
    KeyValueFullInformationAlign64, // q: KEY_VALUE_FULL_INFORMATION_ALIGN64
    KeyValuePartialInformationAlign64,  // q: KEY_VALUE_PARTIAL_INFORMATION_ALIGN64
    KeyValueLayerInformation, // q: KEY_VALUE_LAYER_INFORMATION; rejected by the query/enumeration syscalls on build 26100
    MaxKeyValueInfoClass
} KEY_VALUE_INFORMATION_CLASS;

/**
 * The KEY_VALUE_BASIC_INFORMATION structure defines a subset of the full information available for a value entry of a registry key.
 * \remarks The fixed header ends at offset 0x0c. Required capacity is 12 + NameLength bytes, not
 * sizeof(KEY_VALUE_BASIC_INFORMATION) + NameLength. Name is not null-terminated and no value data is included.
 * TitleIndex is zero on the ordinary value path. NameLength is the full byte length even
 * when STATUS_BUFFER_OVERFLOW returns only a prefix of the name; bound reads by caller capacity.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_key_value_basic_information
 */
typedef struct _KEY_VALUE_BASIC_INFORMATION
{
    ULONG TitleIndex;
    ULONG Type;
    ULONG NameLength;
    _Field_size_bytes_(NameLength) WCHAR Name[1];
} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION;

/**
 * The KEY_VALUE_FULL_INFORMATION structure defines information available for a value entry of a registry key.
 * \remarks Name begins at offset 0x14 and is not null-terminated. DataOffset is relative to the start
 * of the returned record, not to Name. Use DataOffset and DataLength to locate data after name padding.
 * On the native x64 and WoW64 paths, nonempty data is placed at an
 * 8-byte-aligned offset for both Full classes. For empty data,
 * DataLength is zero and DataOffset is 0xffffffff; do not dereference that offset.
 * Required size is DataOffset + DataLength for nonempty data, or 20 + NameLength for empty data.
 * On STATUS_BUFFER_OVERFLOW these offsets/lengths describe the complete record, not necessarily
 * bytes present in the caller buffer. Check each range against the supplied capacity before reading.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_key_value_full_information
 */
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

/**
 * The KEY_VALUE_FULL_INFORMATION_ALIGN64 structure describes the 8-byte-aligned full-value query format.
 * \remarks The field offsets match KEY_VALUE_FULL_INFORMATION, including Name at offset 0x14.
 * KeyValueFullInformationAlign64 is supported by NtQueryValueKey, not NtEnumerateValueKey on build 26100.
 * DataOffset is relative to the complete record; it is not a pointer and need not immediately follow Name.
 */
typedef DECLSPEC_ALIGN(8) struct _KEY_VALUE_FULL_INFORMATION_ALIGN64
{
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataOffset;
    ULONG DataLength;
    ULONG NameLength;
    _Field_size_bytes_(NameLength) WCHAR Name[1];
    // ...
    // UCHAR Data[1];
} KEY_VALUE_FULL_INFORMATION_ALIGN64, *PKEY_VALUE_FULL_INFORMATION_ALIGN64;

/**
 * The KEY_VALUE_PARTIAL_INFORMATION structure contains a value's type and data without its name.
 * \remarks Data begins at offset 0x0c. Required capacity is 12 + DataLength bytes; the declaration's
 * trailing array element and compiler padding are not an additional wire-format header.
 * DataLength remains the full size on STATUS_BUFFER_OVERFLOW even when only a data prefix fits.
 * Data is returned as stored; REG_SZ/REG_MULTI_SZ tags do not guarantee valid termination.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_key_value_partial_information
 */
typedef struct _KEY_VALUE_PARTIAL_INFORMATION
{
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataLength;
    _Field_size_bytes_(DataLength) UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

/**
 * The KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 structure contains the 8-byte-aligned partial-value query format.
 * \remarks Unlike KEY_VALUE_PARTIAL_INFORMATION, this format has no TitleIndex: Type is at offset 0,
 * DataLength at offset 4, and Data at offset 8. Required capacity is 8 + DataLength bytes.
 * KeyValuePartialInformationAlign64 is query-only. DataLength remains
 * the complete data size on overflow; it is not a safe copy length without a capacity check.
 */
typedef DECLSPEC_ALIGN(8) struct _KEY_VALUE_PARTIAL_INFORMATION_ALIGN64
{
    ULONG Type;
    ULONG DataLength;
    _Field_size_bytes_(DataLength) UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, *PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64;

/**
 * The KEY_VALUE_LAYER_INFORMATION structure defines the flags for a value entry of a registry key.
 */
typedef struct _KEY_VALUE_LAYER_INFORMATION
{
    ULONG IsTombstone : 1;
    ULONG Reserved : 31;
} KEY_VALUE_LAYER_INFORMATION, *PKEY_VALUE_LAYER_INFORMATION;

/**
 * The CM_EXTENDED_PARAMETER_TYPE enumeration specifies the kind of extended
 * parameter contained in a CM_EXTENDED_PARAMETER structure.
 *
 * NtLoadKey3 uses the Handle member for types 1 (trust-class key), 2 (event), and 3 (file access token).
 * Type 0 and the maximum sentinel are invalid. Each supported type may occur at most once.
 */
typedef enum _CM_EXTENDED_PARAMETER_TYPE
{
    CmExtendedParameterInvalidType,
    CmExtendedParameterTrustClassKey,
    CmExtendedParameterEvent,
    CmExtendedParameterFileAccessToken,
    CmExtendedParameterMax,
} CM_EXTENDED_PARAMETER_TYPE;

#define CM_EXTENDED_PARAMETER_TYPE_BITS 8

/**
 * The CM_EXTENDED_PARAMETER structure represents an extended parameter passed to registry APIs.
 *
 * The structure occupies 16 bytes and is aligned to 8 bytes on x64 and x86. Type occupies the low
 * 8 bits of the first ULONG64; the value union begins at offset 8. NtLoadKey3 consumes handles for
 * its supported types. Duplicate or unsupported types return STATUS_INVALID_PARAMETER_4.
 */
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

/**
 * Describes one named value requested by NtQueryMultipleValueKey. Initialize
 * ValueName to a readable counted UNICODE_STRING; its empty name selects the
 * default value. ValueName remains caller-owned and is not replaced on return.
 * DataLength, DataOffset and Type describe the returned data, not the name.
 *
 * DataOffset is a byte offset from ValueBuffer, not a pointer or an offset from
 * this structure. Data starts at 4-byte boundaries even
 * for REG_QWORD; copy such values to aligned storage instead of assuming 8-byte
 * alignment. No string terminator is appended to the stored data.
 *
 * Native x64 entries occupy 24 bytes (field offsets 0, 8, 12, 16); x86 entries
 * occupy 16 bytes (offsets 0, 4, 8, 12). WoW64 converts the entry array, not the
 * packed data format. These are not interchangeable serialized layouts.
 * On STATUS_BUFFER_OVERFLOW, entries after the fitting prefix are incomplete;
 * zero output fields do not distinguish an omitted entry from an empty REG_NONE
 * value. Retry with sufficient capacity before consuming the complete array.
 */
typedef struct _KEY_VALUE_ENTRY
{
    PUNICODE_STRING ValueName;
    ULONG DataLength;
    ULONG DataOffset;
    ULONG Type;
} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY;

/**
 * The REG_ACTION enumeration describes the type of change reported by
 * registry change notifications.
 *
 * - KeyAdded: A key was created.
 * - KeyRemoved: A key was deleted.
 * - KeyModified: A key was modified.
 */
typedef enum _REG_ACTION
{
    KeyAdded,
    KeyRemoved,
    KeyModified
} REG_ACTION;

/**
 * The REG_NOTIFY_INFORMATION structure is returned by registry change
 * notifications and describes a single change event.
 *
 * Fields:
 * - NextEntryOffset: Offset, in bytes, to the next REG_NOTIFY_INFORMATION entry in the buffer (0 for last).
 * - Action: The type of change (REG_ACTION).
 * - KeyLength: Length, in bytes, of the Key name string.
 * - Key: Wide-character buffer containing the name of the key affected (not necessarily null-terminated).
 */
_Struct_size_bytes_(NextEntryOffset)
typedef struct _REG_NOTIFY_INFORMATION
{
    ULONG NextEntryOffset;
    REG_ACTION Action;
    ULONG KeyLength;
    _Field_size_bytes_(KeyLength) WCHAR Key[1];
} REG_NOTIFY_INFORMATION, *PREG_NOTIFY_INFORMATION;

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
// Hive format to be used by Reg(Nt)SaveKeyEx
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

//
// Notify filter values
//
#define REG_NOTIFY_CHANGE_NAME          (0x00000001L) // Create or delete (child)
#define REG_NOTIFY_CHANGE_ATTRIBUTES    (0x00000002L)
#define REG_NOTIFY_CHANGE_LAST_SET      (0x00000004L) // time stamp
#define REG_NOTIFY_CHANGE_SECURITY      (0x00000008L)
#define REG_NOTIFY_THREAD_AGNOSTIC      (0x10000000L) // Not associated with a calling thread, can only be used
                                                      // for async user event based notification
#ifndef REG_LEGAL_CHANGE_FILTER
#define REG_LEGAL_CHANGE_FILTER \
    (REG_NOTIFY_CHANGE_NAME |\
     REG_NOTIFY_CHANGE_ATTRIBUTES |\
     REG_NOTIFY_CHANGE_LAST_SET |\
     REG_NOTIFY_CHANGE_SECURITY |\
     REG_NOTIFY_THREAD_AGNOSTIC)
#endif

//
// Predefined Value Types.
//
#define REG_NONE                        ( 0ul ) // No value type
#define REG_SZ                          ( 1ul ) // Unicode nul terminated string
#define REG_EXPAND_SZ                   ( 2ul ) // Unicode nul terminated string (with environment variable references)
#define REG_BINARY                      ( 3ul ) // Free form binary
#define REG_DWORD                       ( 4ul ) // 32-bit number
#define REG_DWORD_LITTLE_ENDIAN         ( 4ul ) // 32-bit number (same as REG_DWORD)
#define REG_DWORD_BIG_ENDIAN            ( 5ul ) // 32-bit number
#define REG_LINK                        ( 6ul ) // Symbolic Link (unicode)
#define REG_MULTI_SZ                    ( 7ul ) // Multiple Unicode strings
#define REG_RESOURCE_LIST               ( 8ul ) // Resource list in the resource map
#define REG_FULL_RESOURCE_DESCRIPTOR    ( 9ul ) // Resource list in the hardware description
#define REG_RESOURCE_REQUIREMENTS_LIST  ( 10ul )
#define REG_QWORD                       ( 11ul ) // 64-bit number
#define REG_QWORD_LITTLE_ENDIAN         ( 11ul ) // 64-bit number (same as REG_QWORD)

/**
 * Creates one registry key or opens it if it already exists. Use Disposition to
 * distinguish those cases before initializing data or assuming ownership.
 * This is not a recursive directory-creation routine for missing path components.
 *
 * \param[out] KeyHandle Required writable location receiving a caller-owned key
 * handle on success. Close it with NtClose. Initialize it to NULL and check status;
 * early failures need not initialize every output, and WoW64 copyback can differ.
 * \param[in] DesiredAccess Access requested on the returned key. KEY_WOW64_32KEY
 * (0x200) and KEY_WOW64_64KEY (0x100) select views, not granted key operation rights.
 * Creation also requires appropriate access to the existing parent; requesting
 * access on the child does not bypass parent or child security checks.
 * \param[in] ObjectAttributes Initialized attributes containing a counted native
 * path (normally beginning with \Registry) or a path relative to a key handle in
 * RootDirectory. HKEY_* predefined aliases are not native RootDirectory handles.
 * Request OBJ_CASE_INSENSITIVE when case-insensitive object lookup is intended.
 * \param[in] TitleIndex Reserved; pass 0.
 * \param[in, optional] Class Counted class string for a new key, or NULL. The
 * user-mode path rejects odd byte lengths and requires 2-byte alignment
 * for nonempty class data. Creating an already-existing key does not replace its
 * class or values with the supplied creation metadata.
 * \param[in] CreateOptions REG_OPTION_* creation options. REG_OPTION_NON_VOLATILE
 * is 0; REG_OPTION_VOLATILE is 0x1 and does not convert an existing key's lifetime.
 * Link and backup/restore options have separate semantics and security checks.
 * \param[out, optional] Disposition Receives REG_CREATED_NEW_KEY (1) or
 * REG_OPENED_EXISTING_KEY (2) on success. May be NULL. Do not consume a stale
 * disposition after failure or delete an existing key as if this call created it.
 * \return STATUS_SUCCESS when a handle is returned, or an NTSTATUS failure.
 * Invalid creation-option bits return STATUS_INVALID_PARAMETER on the native
 * path; missing parents are not silently created by ordinary native lookup.
 * \remarks The native path captures Class before clearing KeyHandle:
 * an invalid class can leave the output handle unchanged. Most later failures
 * leave it NULL. WoW64 captures 32-bit descriptors, constructs a view-sensitive
 * path and may set key-body tags; it is not just pointer-width conversion.
 * Volatile keys still require explicit cleanup when used as temporary objects.
 * Registry callbacks, layers, virtualization and backup privileges can alter
 * ordinary behavior. Success is not a physical hive-flush guarantee.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwcreatekey
 */
_Kernel_entry_
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

/**
 * Creates a registry key or opens an existing one in an explicit transaction. Use it to group registry
 * changes for a later commit or rollback; closing the returned key handle is not itself a commit.
 * NtSetValueKey/NtDeleteKey operations through the transacted handle participate in its transaction.
 *
 * \param[out] KeyHandle Required output receiving a caller-owned handle on success. Close every successful
 * key handle with NtClose, independently of the transaction's outcome and transaction handle lifetime.
 * \param[in] DesiredAccess Rights requested on the key, distinct from the rights on TransactionHandle.
 * Creation still obeys parent and key security checks.
 * \param[in] ObjectAttributes Initialized counted native/relative key name and attributes, as for
 * NtCreateKey. Intermediate missing parents are not created by the ordinary native path.
 * \param[in] TitleIndex Reserved; pass 0.
 * \param[in, optional] Class Optional counted class string for a newly created key; ignored as replacement
 * metadata when an existing key is opened.
 * \param[in] CreateOptions Creation options as for NtCreateKey; options do not decide whether changes commit.
 * REG_OPTION_VOLATILE (0x1) is supported by the ordinary KTM transaction path.
 * \param[in] TransactionHandle Required active transaction handle. The entry requests access 0x4
 * (TRANSACTION_ENLIST for a KTM transaction), first trying a registry-transaction object and then KTM on type
 * mismatch. NULL is not a request for nontransactional creation; use NtCreateKey for that.
 * \param[out, optional] Disposition REG_CREATED_NEW_KEY (1) or REG_OPENED_EXISTING_KEY (2) on success; this
 * is not a transaction-outcome result.
 * \return NTSTATUS for this create/open operation. A completed transaction cannot be reused for further key
 * opens; commit/rollback errors need separate handling.
 * \remarks Newly created keys are visible through matching transacted handles; ordinary opens do not see them
 * until commit. Rollback removed them and restored staged changes to existing values/deletions. These
 * observations are not a general concurrency or crash-recovery guarantee. Transaction validation precedes the
 * native create helper, so early errors may preserve KeyHandle/Disposition. Initialize outputs and check
 * status. WoW64 has its own NULL-transaction rejection and path/tag handling.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwcreatekeytransacted
 */
_Kernel_entry_
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

/**
 * Opens an existing registry key without creating missing keys. The native implementation forwards to NtOpenKeyEx with OpenOptions = 0.
 *
 * \param[out] KeyHandle Required writable output receiving a caller-owned key
 * handle on success; close it with NtClose. Initialize it to NULL and check the
 * return status rather than assuming every failure clears the output.
 * \param[in] DesiredAccess Requested rights on the opened key. View infoclasses
 * KEY_WOW64_32KEY (0x200) / KEY_WOW64_64KEY (0x100) are not key operation rights.
 * The ordinary open with DesiredAccess = 0 returns STATUS_ACCESS_DENIED;
 * this differs from reducing an existing handle to zero access by duplication.
 * \param[in] ObjectAttributes Initialized native absolute path or a counted name
 * relative to the key handle in RootDirectory. Use OBJ_CASE_INSENSITIVE as needed.
 * \return STATUS_SUCCESS or a failure such as STATUS_OBJECT_NAME_NOT_FOUND for
 * an absent final component. Opening a key does not reserve its name/lifetime.
 * \remarks Security information supplied in ObjectAttributes does not replace
 * the existing key's security descriptor. WoW64 constructs the appropriate path
 * and may attach key-body tags; don't assume the same numeric handle or tags
 * across opens, views or architectures. See NtOpenKeyEx for option/probing details.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwopenkeyex
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * Opens an existing registry key in an explicit active transaction. It cannot create a missing key; use
 * NtCreateKeyTransacted for create-or-open behavior. The native implementation forwards to
 * NtOpenKeyTransactedEx with OpenOptions = 0, preserving the explicit transaction association.
 *
 * \param[out] KeyHandle Required output receiving a caller-owned key handle on success. Close it with
 * NtClose; handle closure does not commit the transaction.
 * \param[in] DesiredAccess Access requested on the key, separately checked from access to the transaction
 * object and parent/name lookup requirements.
 * \param[in] ObjectAttributes Initialized counted native or relative key name. Security information does not
 * replace an existing key's security descriptor.
 * \param[in] TransactionHandle Required active transaction. The KTM path requires TRANSACTION_ENLIST (0x4); a
 * query-only transaction handle is insufficient. NULL returns STATUS_INVALID_HANDLE, not an ordinary open.
 * \return NTSTATUS for the open. A missing key returns STATUS_OBJECT_NAME_NOT_FOUND in the transaction view;
 * a completed transaction returns STATUS_TRANSACTION_NOT_ACTIVE in the KTM path.
 * \remarks Operations through the returned handle participate in its transaction. The same key opened
 * separately without that transaction is a different view. Initialize output and check status: transaction
 * validation can fail before native output initialization. WoW64 adds descriptor/path/tag processing.
 * Commit/rollback and closing the transaction handle are separate responsibilities.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwopenkeytransacted
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenKeyTransacted(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE TransactionHandle
    );

/**
 * Opens an existing key with explicit open options; it does not create a missing key. OpenOptions = 0 is
 * equivalent to NtOpenKey on the native path.
 *
 * \param[out] KeyHandle Required caller-owned output; close successful handles with NtClose. Initialize it to
 * NULL before calling and consume it only on success.
 * \param[in] DesiredAccess Requested key rights and optional WOW64 view infoclass, not a creation-option mask.
 * Existing key security still controls granted access.
 * \param[in] ObjectAttributes Initialized counted native or relative key path. The native user-mode path
 * requires 4-byte alignment of this structure and 2-byte alignment of a nonempty name buffer. Prefer natural
 * alignment. Security information here does not replace the existing key's security.
 * \param[in] OpenOptions 0 or supported REG_OPTION_* open flags. The entry admits only bits 0x4
 * (BACKUP_RESTORE), 0x8 (OPEN_LINK) and 0x10 (DONT_VIRTUALIZE), mask 0x1C. VOLATILE (0x1) and CREATE_LINK
 * (0x2) are creation flags, not valid open options. Backup/restore requires its special privilege checks;
 * admission by the bit mask does not guarantee success.
 * \return STATUS_SUCCESS or failure. Invalid option bits return STATUS_INVALID_PARAMETER_4 before the native
 * helper clears KeyHandle.
 * \remarks Consequently native invalid-option failures can preserve the handle output, while the WoW64
 * wrapper copies back its NULL temporary result. Missing-name failures with valid inputs leave NULL on
 * ordinary paths. Do not generalize one failure's output behavior to all earlier capture errors. WoW64 path
 * construction/tag updates and registry filters/layers/virtualization can affect lookup; view selection is
 * not established solely by structure width.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwopenkeyex
 */
_Kernel_entry_
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
 * Opens an existing registry key in an explicit transaction with open options. Use it for transactional
 * access when OPEN_LINK, BACKUP_RESTORE or DONT_VIRTUALIZE behavior is specifically needed; it does not
 * create missing keys.
 *
 * \param[out] KeyHandle Required output. A successful handle belongs to the caller and must be closed with
 * NtClose independently of transaction outcome.
 * \param[in] DesiredAccess Requested rights on the key and optional view infoclass.
 * \param[in] ObjectAttributes Initialized counted native/relative key path; security information does not
 * replace the existing key's security descriptor.
 * \param[in] OpenOptions Same entry mask 0x1C as NtOpenKeyEx: BACKUP_RESTORE (0x4), OPEN_LINK (0x8),
 * DONT_VIRTUALIZE (0x10), or 0. Other bits return STATUS_INVALID_PARAMETER_4 after a valid transaction is
 * referenced. Special options remain subject to their own restrictions.
 * \param[in] TransactionHandle Required active transaction with access 0x4; for KTM this is
 * TRANSACTION_ENLIST. The entry also recognizes native registry-transaction objects before trying KTM on
 * object-type mismatch.
 * \return NTSTATUS. Invalid/wrong-type/insufficient-access transaction handles fail before the native open
 * helper; do not expect a universal output reset.
 * \remarks OpenOptions = 0 matches NtOpenKeyTransacted. Explicitly commit or roll back the transaction;
 * closing a key does not commit. KTM transactions support volatile-key creation, queries, and deletion.
 * NtRenameKey returns STATUS_NOT_IMPLEMENTED for an active transacted key handle. WoW64 converts descriptors
 * and performs path/tag handling, not just a direct width-adjusted syscall. Filters/layers and
 * recovery/concurrency paths differ.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwopenkeytransactedex
 */
_Kernel_entry_
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

/**
 * Deletes an opened registry key, not merely a value and not a recursive subtree. Remove subkeys from the
 * leaves upward before deleting their parent.
 *
 * \param[in] KeyHandle Handle with DELETE (0x10000) access on the ordinary path. The handle remains
 * caller-owned and must still be closed with NtClose, even after a successful deletion. Other open handles to
 * the same key must also close.
 * \return STATUS_SUCCESS, STATUS_ACCESS_DENIED, STATUS_INVALID_HANDLE, STATUS_CANNOT_DELETE or another
 * registry error. In the ordinary path, a key with a live subkey returns STATUS_CANNOT_DELETE; a leaf
 * containing values can be deleted without first deleting those values individually.
 * \remarks Successful ordinary deletion removes the key from name lookup. Existing handles do not become
 * usable handles to a replacement key: subsequent basic queries on them return STATUS_KEY_DELETED, yet
 * NtClose remains required. Do not assume every operation on a deleted key has the same result, or that
 * success implies a synchronous physical hive flush. Through an active transacted handle, deletion is staged:
 * rollback preserves the original key and value; commit removes them. Registry filters, virtualization,
 * read-only keys or hives, and protected special keys can affect deletion.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwdeletekey
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteKey(
    _In_ HANDLE KeyHandle
    );

/**
 * Renames a key's final name component under its current parent. Existing values and subkeys are retained;
 * this is not a move to another parent or a request to overwrite an existing sibling. Existing open handles
 * continue referring to the renamed key/subtree on the ordinary path.
 *
 * \param[in] KeyHandle Normally requires KEY_WRITE (0x20006) on the opened key. The core additionally checks
 * KEY_CREATE_SUB_KEY on its parent and DELETE on the subtree's security descriptors. DELETE-only handle
 * access is not a substitute for KEY_WRITE. Virtualization/layered paths have additional checks.
 * \param[in] NewName Readable counted UNICODE_STRING containing one component, not a full path. The entry
 * requires an even Length from 2 through 512 bytes inclusive (1 through 256 WCHARs), a non-NUL first WCHAR,
 * no backslash anywhere in the counted name, and 2-byte alignment for nonempty name storage. Do not include a
 * terminator in Length. The input remains caller-owned.
 * \return STATUS_SUCCESS or an error. Empty/odd/oversized/backslash-containing names return
 * STATUS_INVALID_PARAMETER. Collision with an existing live sibling returns STATUS_CANNOT_DELETE, not
 * STATUS_OBJECT_NAME_COLLISION. The previous name remains intact on failure.
 * \remarks Native and WoW64 paths accept a component name of at most 512 bytes; a 514-byte name is rejected.
 * This is not a claim about every Win32 registry interface's documented name limit. The current core rejects
 * rename through an active transacted key handle with STATUS_NOT_IMPLEMENTED. WoW64 widens the name
 * descriptor but does not realign the underlying name data. Subtree security denial, callback overrides,
 * read-only/layered-key behavior and concurrent changes can prevent the operation. Success is not a flush
 * guarantee.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntrenamekey
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRenameKey(
    _In_ HANDLE KeyHandle,
    _In_ PCUNICODE_STRING NewName
    );

/**
 * Removes a named value entry and its data, leaving the containing key and other
 * values intact. This is different from setting a value to zero-length data.
 *
 * \param[in] KeyHandle Key handle with KEY_SET_VALUE (0x2) in the ordinary path;
 * DELETE access to the key is not required merely to delete one of its values.
 * \param[in] ValueName Required readable counted UNICODE_STRING. An empty descriptor
 * selects the default value. Matching is case-insensitive. The native
 * path requires even Length and 2-byte-aligned nonempty user input, trims trailing
 * WCHAR NULs and preserves embedded NULs as characters in the counted name.
 * \return STATUS_SUCCESS if removed, STATUS_OBJECT_NAME_NOT_FOUND if absent,
 * or an error such as STATUS_ACCESS_DENIED. Repeating deletion is not a successful no-op.
 * \remarks Registry filters, transactions, virtualization and layered views can
 * affect the result. This contract does not promise that a physical hive write is
 * flushed to disk when the call returns.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwdeletevaluekey
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteValueKey(
    _In_ HANDLE KeyHandle,
    _In_ PCUNICODE_STRING ValueName
    );

/**
 * Queries metadata for the supplied key itself. Use basic/node/name information
 * to identify it, full/cached information for counts and buffer-planning maxima,
 * and the fixed flag records for registry metadata. This does not read value data.
 *
 * \param[in] KeyHandle Normally requires KEY_QUERY_VALUE. KeyNameInformation
 * and KeyHandleTagsInformation instead require any nonzero granted access mask
 * on the ordinary path; a valid zero-access handle is still denied.
 * \param[in] KeyInformationClass A KEY_INFORMATION_CLASS infoclass. The native/WoW64 paths accept 0 through 8; unsupported classes return
 * STATUS_INVALID_PARAMETER, not STATUS_INVALID_INFO_CLASS.
 * \param[out] KeyInformation Caller-owned output storage. NULL with Length = 0
 * supports size discovery. Prefer natural alignment; the native entry probes
 * nonempty buffers at 4-byte alignment, including records containing LARGE_INTEGER.
 * \param[in] Length Output capacity in bytes. Preserve this for bounds checks.
 * \param[out] ResultLength Required pointer receiving the complete required size
 * on success, STATUS_BUFFER_TOO_SMALL or STATUS_BUFFER_OVERFLOW. On overflow it
 * can exceed Length; embedded name/class lengths also remain full lengths.
 * Do not use an unchanged ResultLength after unrelated failures as a valid size.
 * \return STATUS_SUCCESS for a complete record; STATUS_BUFFER_TOO_SMALL when
 * the fixed prefix/record cannot fit; STATUS_BUFFER_OVERFLOW for a partial
 * variable record; or another NTSTATUS describing the failure.
 * \remarks Returned names/class strings are not NUL-terminated. Counts/maxima
 * can change before a later query; sizing does not reserve a registry snapshot.
 * The WoW64 thunk realigns output not aligned to 8 bytes through
 * temporary storage and copies the requested Length back regardless of status.
 * Unlike its enumeration counterpart, it does not first copy the caller's buffer
 * into that storage. Do not rely on failure-buffer or spare-byte preservation in
 * that path, and consume only fields covered by a valid returned record.
 * It still supports zero-length sizing with an unaligned non-NULL pointer.
 * Registry filters, layered keys, transactions and virtualization have separate
 * paths and can affect ordinary behavior.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwquerykey
 */
_Kernel_entry_
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
 * Changes metadata of an opened registry key: timestamp, internal flags or key- body tags. It does not
 * create/replace a named value; use NtSetValueKey for that.
 *
 * \param[in] KeyHandle Ordinarily requires KEY_SET_VALUE (0x2) for classes 0 through 4.
 * KeySetHandleTagsInformation accepts a valid zero-access key handle; querying those tags through that handle
 * is still denied.
 * \param[in] KeySetInformationClass A KEY_SET_INFORMATION_CLASS infoclass. The native/WoW64 paths support 0
 * through 5, returning STATUS_INVALID_INFO_CLASS for unsupported infoclasses, including class 6.
 * \param[in] KeySetInformation Readable caller-owned input, captured before the ordinary handle access check.
 * The caller's bytes are not modified or retained. NULL is not valid for a supported exact-sized request. The
 * x64/WoW64 paths accept unaligned readable input, including the 8-byte time; naturally aligned structures
 * remain preferable for portable callers.
 * \param[in] KeySetInformationLength Exact byte count: 8 for class 0 and 4 for classes 1 through 5. Extra
 * capacity is not accepted as a larger valid record.
 * \return STATUS_SUCCESS, STATUS_INFO_LENGTH_MISMATCH for an incorrect size, STATUS_INVALID_INFO_CLASS for an
 * unsupported infoclass, or another failure. Class 1 returns STATUS_INVALID_PARAMETER when UserFlags contains
 * bits above 0x0F; other flag classes have different masking/transition rules.
 * \remarks InfoClass and exact-length checks precede input capture and normal handle validation. A bad input
 * pointer can therefore fail before access is checked; do not infer permissions from error precedence. WoW64
 * forwards supported input pointers/lengths directly, with no record conversion or realignment. Input is
 * logically read-only even for unaligned data. Ordinary classes 1 through 4 also refresh last-write time;
 * class 5 does not. Tag changes are shared by duplicated handles, not independently stored in each
 * handle-table entry. Registry filters, read-only keys/hives, transactions and access-denied virtualization
 * fallback paths can change ordinary behavior. A successful update is not a durability/flush guarantee.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntsetinformationkey
 */
_Kernel_entry_
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
 * Retrieves one named value without changing its data. Supports size discovery by passing KeyValueInformation
 * = NULL and Length = 0 with a valid ResultLength.
 *
 * \param[in] KeyHandle Key handle with KEY_QUERY_VALUE (0x1).
 * \param[in] ValueName Required readable counted UNICODE_STRING, matched without case sensitivity. A
 * zero-length descriptor selects the default value. Trailing WCHAR NULs are trimmed; embedded NULs remain
 * name characters.
 * \param[in] KeyValueInformationClass Classes 0 through 4 are accepted; class 5 (KeyValueLayerInformation)
 * and other infoclasses return STATUS_INVALID_PARAMETER. Preserve newer class definitions for other OS targets.
 * \param[out] KeyValueInformation Optional caller buffer, required when Length is nonzero. Native x64 checks
 * 4-byte alignment for all accepted classes on this build; portable Align64 callers should still supply an
 * 8-byte-aligned buffer.
 * \param[in] Length Output capacity in bytes, not a number of records or WCHARs.
 * \param[out] ResultLength Required ULONG receiving the total size needed on STATUS_SUCCESS,
 * STATUS_BUFFER_OVERFLOW or STATUS_BUFFER_TOO_SMALL. It can exceed Length and must not be treated as the
 * number of initialized bytes on overflow.
 * \return STATUS_BUFFER_TOO_SMALL if the fixed prefix does not fit (no record copied), STATUS_BUFFER_OVERFLOW
 * if the prefix fits but the complete record does not, STATUS_OBJECT_NAME_NOT_FOUND if absent, or another
 * NTSTATUS.
 * \remarks Returned NameLength/DataLength describe the full value even on overflow. Validate the available
 * prefix and clamp all reads to Length before using offsets or lengths. Missing-value, invalid-class and
 * access failures leave ResultLength unchanged on ordinary paths; do not use a stale output size. WoW64
 * copies non-8-byte-aligned query output through a temporary native buffer, even for ordinary classes. That
 * thunk copies the requested Length back, so do not rely on untouched buffer contents on failure or beyond
 * the valid returned record. Size discovery and retrieval are separate snapshots; the value may change
 * between them.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwqueryvaluekey
 */
_Kernel_entry_
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
 * Creates or replaces one value entry under a registry key, including its type
 * and data. This does not create or rename the containing key.
 *
 * \param[in] KeyHandle Key handle with KEY_SET_VALUE (0x2) in the ordinary path.
 * Registry virtualization, layered keys and filter callbacks can affect the operation.
 * \param[in] ValueName Required readable counted UNICODE_STRING. Use an empty
 * descriptor (Length = 0, Buffer may be NULL) for the default value, not a NULL
 * descriptor pointer. Names are case-insensitive. On x64, the name
 * length must be even and at most 32766 bytes (16383 WCHARs); a nonempty user-mode
 * name buffer must be 2-byte aligned. Captured trailing WCHAR NULs are trimmed,
 * whereas embedded NULs are part of the counted name.
 * \param[in, optional] TitleIndex Reserved; pass 0. The native path
 * exposes it to registry callbacks but does not pass it to the core value writer.
 * Basic/full/partial query records in the ordinary path return TitleIndex = 0.
 * \param[in] Type REG_* data type. The caller is responsible for the type's format.
 * \param[in, optional] Data Readable DataSize bytes; NULL is valid when DataSize = 0.
 * The native operation copies the counted bytes, not a string inferred from Type.
 * \param[in] DataSize Byte count, including any required string terminators.
 * The native entry rejects values above 0x7FFFF000; this is an entry
 * validation bound, not a guarantee that a value that large can be allocated/stored.
 * \return NTSTATUS describing the update. Odd or oversized name lengths return
 * STATUS_INVALID_PARAMETER in the ordinary path.
 * \remarks A zero-length data value still exists; use NtDeleteValueKey to remove it.
 * A REG_SZ input without its NUL can be stored as supplied, so do not assume the
 * kernel repairs malformed strings. The WoW64 REG_SZ/REG_EXPAND_SZ path can rewrite
 * recognized paths according to key tags; byte-for-byte passthrough is not universal.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwsetvaluekey
 */
_Kernel_entry_
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
 * Retrieves selected values from one registry key into a shared data buffer.
 * Use this to read a known group of settings without issuing one query per name;
 * it does not enumerate subkeys or values and does not reserve them for later use.
 *
 * \param[in] KeyHandle A key handle with KEY_QUERY_VALUE access.
 * \param[in, out] ValueEntries EntryCount KEY_VALUE_ENTRY records. Initialize each
 * ValueName to a readable UNICODE_STRING. Empty names select the default value;
 * duplicate names produce separate data slots. The native path rejects
 * odd name lengths and trims trailing WCHAR NULs in its captured copy, without
 * changing the caller's descriptor. Output fields are meaningful for a complete
 * result only after success; ValueName pointers are preserved.
 * \param[in] EntryCount Number of entries. The native user-mode entry
 * rejects counts above 65536 with STATUS_INSUFFICIENT_RESOURCES. WoW64 allocates
 * and converts the array before calling native code; this limit is not a promise
 * of early validation or successful allocation in every calling environment.
 * \param[out] ValueBuffer Receives value data at the returned byte offsets. Use
 * 4-byte-aligned storage. NULL with an input BufferLength of 0 supports size
 * discovery. Values are placed in request order at 4-byte-aligned offsets; string data is returned as stored, without an added NUL.
 * \param[in, out] BufferLength Required pointer. On input, the available byte
 * capacity; on successful nonempty queries, the packed span used, including
 * alignment gaps. Preserve the original capacity for bounds checks. A short-buffer
 * result is not a complete initialized array or an unrestricted copy length.
 * \param[out, optional] RequiredBufferLength Receives the required packed byte
 * span on success or STATUS_BUFFER_OVERFLOW; it is not an entry count. May be NULL.
 * Do not use its value as a required size after unrelated failures.
 * \return STATUS_SUCCESS for a complete result, STATUS_BUFFER_OVERFLOW if not
 * all requested data fits, or an error such as STATUS_OBJECT_NAME_NOT_FOUND when
 * any requested name is missing. A missing later name can supersede overflow.
 * \remarks In the ordinary path, overflow copies only complete values
 * from a fitting prefix, then stops copying further entries while computing the
 * required size. Uncopied entry output fields are zero, so retry the whole query
 * rather than interpreting zero fields as valid empty values. A zero-capacity
 * query of existing nonempty values returns STATUS_BUFFER_OVERFLOW, not
 * STATUS_BUFFER_TOO_SMALL. Empty data can succeed with zero capacity.
 *
 * With EntryCount = 0, the native/WoW64 paths accept a NULL entry array,
 * still check key access, return RequiredBufferLength = 0, and leave BufferLength
 * unchanged. Do not interpret that unchanged capacity as bytes initialized.
 * WoW64 converts 16-byte entries to native 24-byte entries and copies output
 * fields back even on errors; it forwards ValueBuffer without realigning it.
 * Native requires 4-byte entry-array alignment; WoW64's conversion also accepts
 * unaligned source entries. Prefer naturally aligned arrays.
 * Registry callbacks, layered keys and virtualization can alter ordinary behavior.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntquerymultiplevaluekey
 */
_Kernel_entry_
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
 * Retrieves one direct subkey by index. This enumerates keys, not value entries;
 * use NtEnumerateValueKey for values. Returned metadata belongs to the selected
 * child, not the parent represented by KeyHandle.
 *
 * \param[in] KeyHandle Parent key handle with KEY_ENUMERATE_SUB_KEYS access.
 * \param[in] Index Zero-based index. STATUS_NO_MORE_ENTRIES ends enumeration.
 * Indices are not stable identifiers: mutations can change which child an index
 * selects, and repeated calls need not return the same child.
 * \param[in] KeyInformationClass KeyBasicInformation, KeyNodeInformation or
 * KeyFullInformation (0 through 2). Other infoclasses return STATUS_INVALID_PARAMETER
 * on the native/WoW64 paths, including classes accepted by NtQueryKey.
 * \param[out] KeyInformation Caller-owned output buffer. NULL with Length = 0
 * supports sizing for an existing index. Prefer naturally aligned storage;
 * the native entry requires 4-byte alignment for a nonempty output buffer.
 * \param[in] Length Output capacity in bytes, not WCHARs.
 * \param[out] ResultLength Required pointer. Receives the complete required byte
 * size on success or either short-buffer status. It can exceed Length, and
 * NameLength/ClassLength remain full lengths on overflow. Bound every read by
 * capacity; no terminator is appended. Enumeration end is not a sizing result.
 * \return STATUS_SUCCESS, STATUS_BUFFER_TOO_SMALL if the fixed prefix cannot
 * fit, STATUS_BUFFER_OVERFLOW for a partial record, STATUS_NO_MORE_ENTRIES at
 * the end, or another error status. No key handle is returned or retained.
 * \remarks The WoW64 thunk realigns output not aligned to 8 bytes by
 * copying Length bytes from the caller into temporary storage before the native
 * call, then copying Length bytes back regardless of status. This path therefore
 * reads the caller's storage as well as writing it; use readable/writable storage.
 * Unlike NtQueryKey's thunk, an unaligned non-NULL buffer with Length = 0 returns
 * STATUS_NO_MEMORY before the syscall. Use NULL for zero-length size discovery.
 * Do not generalize this behavior to the separate value-enumeration thunk.
 * Registry filters, transactions, layered keys and virtualization can affect the
 * enumeration; there is no stable multi-call snapshot guarantee.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwenumeratekey
 */
_Kernel_entry_
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
 * Retrieves one value by index under a registry key. This enumerates values, not subkeys, and does not create
 * a stable snapshot or reserve the returned value.
 *
 * \param[in] KeyHandle Key handle with KEY_QUERY_VALUE (0x1); KEY_ENUMERATE_SUB_KEYS alone is not the access
 * right for this operation.
 * \param[in] Index Zero-based value index. Retry the same index after a short-buffer result;
 * STATUS_NO_MORE_ENTRIES indicates the end. Do not assume a stable order or index-to-name mapping while
 * another caller is adding or deleting values.
 * \param[in] KeyValueInformationClass Classes 0, 1 and 2 (Basic, Full, Partial) on native x64 and WoW64.
 * Classes 3 and 4 (Align64) and class 5 (Layer) return STATUS_INVALID_PARAMETER, even though query accepts
 * the Align64 classes.
 * \param[out] KeyValueInformation Optional caller output; NULL with Length = 0 requests the size. Nonempty
 * output requires 4-byte alignment on the native and WoW64 paths; unlike query, enumeration does not realign
 * the x86 buffer.
 * \param[in] Length Output capacity in bytes.
 * \param[out] ResultLength Required ULONG receiving total required bytes on success or either short-buffer
 * status; it can exceed the supplied Length.
 * \return STATUS_SUCCESS, STATUS_BUFFER_TOO_SMALL when the fixed prefix does not fit, STATUS_BUFFER_OVERFLOW
 * for a partial record, STATUS_NO_MORE_ENTRIES, or an error.
 * \remarks ResultLength is unchanged for end-of-enumeration, invalid-class, and access failures. On overflow,
 * the returned name/data lengths remain full lengths: never read that many bytes without checking the
 * caller-buffer capacity. The record formats and data-offset rules are shared with NtQueryValueKey.
 */
_Kernel_entry_
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
 * \param[in] KeyHandle A handle to the key to be flushed.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwflushkey
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushKey(
    _In_ HANDLE KeyHandle
    );

/**
 * Compacts the specified registry keys.
 *
 * \param[in] Count The number of keys to be compacted.
 * \param[in] KeyArray An array of handles to the keys to be compacted.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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
 * \param[in] KeyHandle A handle to the key to be compressed.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCompressKey(
    _In_ HANDLE KeyHandle
    );

/**
 * Loads a registry key from a file.
 *
 * \param[in] TargetKey A pointer to an OBJECT_ATTRIBUTES structure that specifies the target key.
 * \param[in] SourceFile A pointer to an OBJECT_ATTRIBUTES structure that specifies the source file.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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
 * \param[in] TargetKey A pointer to an OBJECT_ATTRIBUTES structure that specifies the target key.
 * \param[in] SourceFile A pointer to an OBJECT_ATTRIBUTES structure that specifies the source file.
 * \param[in] Flags The options to use when loading the key.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtLoadKey2(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_ POBJECT_ATTRIBUTES SourceFile,
    _In_ ULONG Flags
    );

/**
 * Loads a registry key from a file with extended options.
 *
 * \param[in] TargetKey A pointer to an OBJECT_ATTRIBUTES structure that specifies the target key.
 * \param[in] SourceFile A pointer to an OBJECT_ATTRIBUTES structure that specifies the source file.
 * \param[in] Flags The options to use when loading the key.
 * \param[in, optional] TrustClassKey A handle to the trust class key.
 * \param[in, optional] Event A handle to an event.
 * \param[in, optional] DesiredAccess The access mask that specifies the desired access rights.
 * \param[out, optional] RootHandle A pointer to a handle that receives the root handle.
 * \param[in, reserved] Reserved Reserved.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
// rev
/**
 * Loads a registry key from a file with extended parameters.
 *
 * \param[in] TargetKey A pointer to an OBJECT_ATTRIBUTES structure that specifies the target key.
 * \param[in] SourceFile A pointer to an OBJECT_ATTRIBUTES structure that specifies the source file.
 * \param[in] Flags The options to use when loading the key.
 * \param[in, optional] ExtendedParameters An 8-byte-aligned array of 16-byte CM_EXTENDED_PARAMETER entries; ignored when the count is zero.
 * \param[in] ExtendedParameterCount Number of entries, 0 through 3. Each supported parameter type may appear only once.
 * \param[in, optional] DesiredAccess The access mask that specifies the desired access rights.
 * \param[out, optional] RootHandle A pointer to a handle that receives the root handle.
 * \param[in, reserved] Reserved Reserved.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtLoadKey3(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_ POBJECT_ATTRIBUTES SourceFile,
    _In_ ULONG Flags,
    _In_reads_opt_(ExtendedParameterCount) PCM_EXTENDED_PARAMETER ExtendedParameters,
    _In_ ULONG ExtendedParameterCount,
    _In_opt_ ACCESS_MASK DesiredAccess,
    _Out_opt_ PHANDLE RootHandle,
    _Reserved_ PVOID Reserved
    );
#endif

/**
 * Replaces a registry key.
 *
 * \param[in] NewFile A pointer to an OBJECT_ATTRIBUTES structure that specifies the new file.
 * \param[in] TargetHandle A handle to the target key.
 * \param[in] OldFile A pointer to an OBJECT_ATTRIBUTES structure that specifies the old file.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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
 * \param KeyHandle Handle to the registry key.
 * \param FileHandle Handle to the file where the key will be saved.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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
 * \param KeyHandle Handle to the registry key.
 * \param FileHandle Handle to the file where the key will be saved.
 * \param Format Format in which the key will be saved.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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
 * \param HighPrecedenceKeyHandle Handle to the high precedence registry key.
 * \param LowPrecedenceKeyHandle Handle to the low precedence registry key.
 * \param FileHandle Handle to the file where the merged key will be saved.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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
 * \param KeyHandle Handle to the registry key.
 * \param FileHandle Handle to the file from which the key will be restored.
 * \param Flags Flags for the restore operation.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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
 * \param TargetKey Pointer to the object attributes of the target key.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnloadKey(
    _In_ POBJECT_ATTRIBUTES TargetKey
    );

/**
 * Unloads a registry key with additional flags.
 *
 * \param TargetKey Pointer to the object attributes of the target key.
 * \param Flags Flags for the unload operation.
 * \return NTSTATUS Successful or errant status.
 * \remarks Valid flags are REG_FORCE_UNLOAD and REG_UNLOAD_LEGAL_FLAGS.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnloadKey2(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_ ULONG Flags
    );

/**
 * Unloads a registry key and optionally signals an event.
 *
 * \param TargetKey Pointer to the object attributes of the target key.
 * \param Event Optional handle to an event to be signaled.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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
 * \param KeyHandle Handle to the registry key with KEY_NOTIFY access.
 * \param Event Optional handle to an event to be signaled.
 * \param ApcRoutine Optional APC routine to be called.
 * \param ApcContext Optional context for the APC routine.
 * \param IoStatusBlock Pointer to an IO status block.
 * \param CompletionFilter REG_NOTIFY_CHANGE_* flags and optional REG_NOTIFY_THREAD_AGNOSTIC; valid mask 0x1000000f.
 * \param WatchTree Whether to watch the entire tree.
 * \param Buffer Reserved for system use; pass NULL.
 * \param BufferSize Reserved for system use; pass zero.
 * \param Asynchronous Whether the operation is asynchronous.
 * \return NTSTATUS Successful or errant status.
 * \remarks For an asynchronous event-based watch, STATUS_PENDING means the I/O status block and
 * notification resources remain live. A successful wait on the event does not make IoStatusBlock.Status equal
 * to STATUS_SUCCESS: an owned NULL/zero-buffer last-set watch completes with STATUS_NOTIFY_ENUM_DIR (0x10C).
 * Re-query the state of interest and rearm as needed; do not treat the event as a complete change record.
 */
_Kernel_entry_
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
 * \remarks REG_NOTIFY_THREAD_AGNOSTIC (0x10000000) requires a user-mode asynchronous event-based
 * request: Asynchronous must be TRUE, Event non-NULL, and ApcRoutine NULL. The valid filter mask is
 * 0x1000000f. Count greater than 1, a same-hive subordinate key, or an invalid filter returns STATUS_INVALID_PARAMETER.
 *
 * \param MasterKeyHandle A handle to an open key. The handle must be opened with the KEY_NOTIFY access right.
 * \param Count Number of entries in SubordinateObjects, 0 or 1. Kernel-mode asynchronous requests require 0.
 * \param SubordinateObjects Optional attributes for one additional key, which must be in a different hive from MasterKeyHandle.
 * \param Event A handle to an event created by the caller. If Event is not NULL, the caller waits until the operation succeeds, at which time the event is signaled.
 * \param ApcRoutine A pointer to an asynchronous procedure call (APC) function supplied by the caller. If ApcRoutine is not NULL, the specified APC function executes after the operation completes.
 * \param ApcContext A pointer to a context supplied by the caller for its APC function. This value is passed to the APC function when it is executed. The Asynchronous parameter must be TRUE. If ApcContext is specified, the Event parameter must be NULL.
 * \param IoStatusBlock A pointer to an IO_STATUS_BLOCK structure that contains the final status and information about the operation. For successful calls that return data, the number of bytes written to the Buffer parameter is supplied in the Information member of the IO_STATUS_BLOCK structure.
 * \param CompletionFilter A bitmap of operations that trigger notification. This parameter can be one or more of the following flags. REG_NOTIFY_CHANGE_NAME, REG_NOTIFY_CHANGE_ATTRIBUTES, REG_NOTIFY_CHANGE_LAST_SET, REG_NOTIFY_CHANGE_SECURITY.
 * \param WatchTree If this parameter is TRUE, the caller is notified about changes to all subkeys of the specified key. If this parameter is FALSE, the caller is notified only about changes to the specified key.
 * \param Buffer Reserved for system use. This parameter must be NULL.
 * \param BufferSize Reserved for system use. This parameter must be zero.
 * \param Asynchronous Whether the operation is asynchronous.
 * \return NTSTATUS Successful or errant status.
 * \remarks The native entry supports at most 1 subordinate key in addition to
 * the master. Count zero is the single-master form used by NtNotifyChangeKey. Asynchronous completion uses
 * the supplied event/APC and live I/O status storage; a NULL/zero-buffer last-set watch completes with
 * STATUS_NOTIFY_ENUM_DIR (0x10C), requiring the caller to query the changed state.
 */
_Kernel_entry_
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
 * \param TargetKey Pointer to the object attributes of the target key.
 * \param HandleCount Pointer to a variable to receive the handle count.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryOpenSubKeys(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _Out_ PULONG HandleCount
    );

/**
 * The KEY_PID_ARRAY structure associates a process id with a registry key name.
 * It is used when reporting open subkeys and the process that holds them.
 */
typedef struct _KEY_PID_ARRAY
{
    HANDLE ProcessId;
    UNICODE_STRING KeyName;
} KEY_PID_ARRAY, *PKEY_PID_ARRAY;

/**
 * The KEY_OPEN_SUBKEYS_INFORMATION structure contains an array of KEY_PID_ARRAY entries
 * describing subkeys that are currently open.
 *
 * The fields include:
 * - Count: Number of entries in KeyArray.
 * - KeyArray: Array of KEY_PID_ARRAY items.
 */
typedef struct _KEY_OPEN_SUBKEYS_INFORMATION
{
    ULONG Count;
    KEY_PID_ARRAY KeyArray[1];
} KEY_OPEN_SUBKEYS_INFORMATION, *PKEY_OPEN_SUBKEYS_INFORMATION;

/**
 * Queries the open subkeys of a registry key with additional information.
 *
 * \param TargetKey Pointer to the object attributes of the target key.
 * \param BufferLength Length of the buffer.
 * \param Buffer Optional buffer to receive the subkey information.
 * \param RequiredSize Pointer to a variable to receive the required size.
 * \return NTSTATUS Successful or errant status.
 * \remarks Requires SeRestorePrivilege. Returns one KEY_OPEN_SUBKEYS_INFORMATION header containing
 * Count KEY_PID_ARRAY entries, followed by name storage, not an array of headers. On x64 KeyArray
 * begins at offset 8 and each entry occupies 24 bytes; KeyName.Buffer points into the returned buffer.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryOpenSubKeysEx(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_ ULONG BufferLength,
    _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
    _Out_ PULONG RequiredSize
    );

//
// Boot condition flags (NtInitializeRegistry)
//
#define REG_INIT_BOOT_SM 0x0000
#define REG_INIT_BOOT_SETUP 0x0001
#define REG_INIT_BOOT_ACCEPTED_BASE 0x0002
#define REG_INIT_BOOT_ACCEPTED_MAX (REG_INIT_BOOT_ACCEPTED_BASE + 999)

/**
 * Initializes the registry.
 *
 * \param BootCondition Condition for the boot.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitializeRegistry(
    _In_ USHORT BootCondition
    );

/**
 * Locks the registry key and prevents changes from being written to disk.
 *
 * \param KeyHandle Handle to the registry key.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtLockRegistryKey(
    _In_ HANDLE KeyHandle
    );

/**
 * Locks the product activation keys.
 *
 * \param PrivateVersion Optional pointer to a private version variable.
 * \param SafeMode Optional pointer to a safe mode variable.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtLockProductActivationKeys(
    _Inout_opt_ PULONG PrivateVersion,
    _Out_opt_ PULONG SafeMode
    );

/**
 * Freezes the registry and prevents changes from being flushed to disk.
 *
 * \param TimeOutInSeconds Timeout in seconds.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreezeRegistry(
    _In_ ULONG TimeOutInSeconds
    );

/**
 * Thaws the registry and enables flushing changes to disk.
 *
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtThawRegistry(
    VOID
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
/**
 * Creates a registry-only lightweight transaction object, distinct from a KTM transaction created by
 * NtCreateTransaction. Pass it to the transacted key create/open APIs to group registry changes for explicit
 * commit or rollback. Do not pass it to KTM query/set/commit/rollback routines: object types differ.
 *
 * \param RegistryTransactionHandle Required writable output receiving a caller- owned handle on success.
 * Close it with NtClose. Last-handle close of an active transaction rolls back staged changes; closing a key
 * handle alone does not end the transaction.
 * \param DesiredAccess Requested transaction rights. The registry APIs use 0x4 for enlistment through
 * transacted key operations, 0x8 for commit and 0x10 for rollback. These checks do not grant access to the
 * keys themselves.
 * \param ObjAttributes Optional object attributes, or NULL for an unnamed object. A named transaction lives
 * in the Object Manager namespace, not the registry key namespace; NtOpenRegistryTransaction opens it by that
 * object name.
 * \param CreateOptions Reserved; must be 0. Every nonzero bit is rejected with STATUS_INVALID_PARAMETER on
 * the native/WoW64 paths.
 * \return STATUS_SUCCESS or an NTSTATUS failure. Initialize the output and consume it only on success: native
 * nonzero-option rejection occurs before clearing it, while the WoW64 wrapper copies back a NULL temporary.
 * \remarks The implementation is in ntoskrnl, not tm.sys. These APIs do not take KTM UOW identifiers,
 * descriptions or timeouts. Named opens and registry transactions do not imply support for arbitrary
 * resource-manager enlistments or KTM recovery semantics.
 */
_Kernel_entry_
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
 * Opens an existing named registry-only lightweight transaction. This is object- name lookup, not lookup of a
 * registry key or a KTM unit-of-work GUID.
 *
 * \param RegistryTransactionHandle Required output receiving a separately owned handle on success; close it
 * with NtClose. Other open/duplicate handles keep the same transaction object alive. Initialize this output
 * and check status.
 * \param DesiredAccess Requested rights, including 0x4 for transacted key association, 0x8 for commit or 0x10
 * for rollback as needed.
 * \param ObjAttributes Initialized attributes identifying the named transaction in the Object Manager
 * namespace. An unnamed transaction cannot be reopened by inventing a registry path or by passing its handle
 * as a UOW identifier.
 * \return STATUS_SUCCESS or an error such as STATUS_OBJECT_NAME_NOT_FOUND. After the last transaction handle
 * closes, the rolled-back key is no longer available under its former name.
 * \remarks Registry and KTM transactions are different object types. To open a KTM transaction by its GUID
 * use NtOpenTransaction instead. Opening another handle does not clone the transaction or reset a completed
 * transaction.
 */
_Kernel_entry_
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
 * Commits staged changes of a registry-only lightweight transaction. It is not the KTM NtCommitTransaction
 * entry and does not accept a Boolean Wait argument.
 *
 * \param RegistryTransactionHandle Registry-transaction object handle with commit access 0x8. A KTM
 * transaction handle returns STATUS_OBJECT_TYPE_MISMATCH even though the corresponding right has the same
 * value.
 * \param Flags Reserved; must be 0. Nonzero flags return STATUS_INVALID_PARAMETER before handle validation; 1
 * is not a request for synchronous completion.
 * \return STATUS_SUCCESS for completed ordinary commit, or an NTSTATUS failure. For sequential calls,
 * committing again after commit returns STATUS_TRANSACTION_NOT_ACTIVE; committing after rollback returns
 * STATUS_TRANSACTION_ALREADY_ABORTED. Do not substitute KTM's repeat-call rules.
 * \remarks The handle remains caller-owned and must close. A completed transaction cannot be reused as a new
 * active transaction. Bounded registry visibility and last-close cleanup do not establish crash-recovery or
 * concurrent commit/rollback guarantees.
 */
_Kernel_entry_
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
 * Rolls back staged changes of a registry-only lightweight transaction.
 *
 * \param RegistryTransactionHandle Registry-transaction object handle with rollback access 0x10. KTM handles
 * are a different type and are rejected here.
 * \param Flags Reserved; must be 0, not KTM's Boolean Wait. Nonzero flags return STATUS_INVALID_PARAMETER
 * before ordinary handle validation.
 * \return STATUS_SUCCESS for ordinary rollback. The sequential path returns
 * STATUS_TRANSACTION_ALREADY_ABORTED on repeated rollback and STATUS_TRANSACTION_NOT_ACTIVE after commit. KTM
 * repeated rollback differs.
 * \remarks Close the transaction handle separately with NtClose. All opens and duplicates refer to the same
 * outcome. Last-handle close of an active registry transaction also triggers rollback, but explicit outcome
 * handling is preferable when the caller needs to detect errors. No general durability, multi-resource or
 * concurrent-outcome guarantee is implied.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollbackRegistryTransaction(
    _In_ HANDLE RegistryTransactionHandle,
    _Reserved_ ULONG Flags
    );
#endif

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

/**
 * VR_INITIALIZE_JOB_FOR_VREG is used with the VReg driver to associate a job object
 * with the virtualization driver instance.
 */
typedef struct _VR_INITIALIZE_JOB_FOR_VREG
{
    HANDLE Job;
} VR_INITIALIZE_JOB_FOR_VREG, *PVR_INITIALIZE_JOB_FOR_VREG;

// rev
#define VR_FLAG_INHERIT_TRUST_CLASS 0x00000001
#define VR_FLAG_WRITE_THROUGH_HIVE 0x00000002 // since REDSTONE2
#define VR_FLAG_LOCAL_MACHINE_TRUST_CLASS 0x00000004 // since 21H1

// rev
/**
 * VR_LOAD_DIFFERENCING_HIVE describes parameters for loading a differencing hive
 * for the virtualization driver.
 *
 * The structure contains job handle, path length fields, flags and an inline
 * buffer with wide-character strings for the paths.
 */
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

// rev
/**
 * VR_CREATE_NAMESPACE_NODE contains parameters for creating a namespace node
 * that maps a container path to a host path for virtualized registry access.
 */
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

/**
 * VR_MODIFY_FLAGS specifies flags to add or remove for a given job in the
 * virtualization driver.
 */
typedef struct _VR_MODIFY_FLAGS
{
    HANDLE Job;
    ULONG AddFlags;
    ULONG RemoveFlags;
} VR_MODIFY_FLAGS, *PVR_MODIFY_FLAGS;

/**
 * NAMESPACE_NODE_DATA describes a single namespace node entry containing access
 * mask, path lengths, flags and inline strings for paths.
 */
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

/**
 * VR_CREATE_MULTIPLE_NAMESPACE_NODES is used to create multiple namespace nodes
 * in a single operation. It contains the job handle, number of keys and an
 * inline array of NAMESPACE_NODE_DATA items.
 */
typedef struct _VR_CREATE_MULTIPLE_NAMESPACE_NODES
{
    HANDLE Job;
    ULONG NumNewKeys;
    NAMESPACE_NODE_DATA Keys[1];
} VR_CREATE_MULTIPLE_NAMESPACE_NODES, *PVR_CREATE_MULTIPLE_NAMESPACE_NODES;

/**
 * VR_UNLOAD_DYNAMICALLY_LOADED_HIVES requests unloading of dynamically loaded
 * differencing hives for a job.
 */
typedef struct _VR_UNLOAD_DYNAMICALLY_LOADED_HIVES
{
    HANDLE Job;
} VR_UNLOAD_DYNAMICALLY_LOADED_HIVES, *PVR_UNLOAD_DYNAMICALLY_LOADED_HIVES;

// rev
#define VR_KEY_COMROOT 0          // \Registry\ComRoot\Classes
#define VR_KEY_MACHINE_SOFTWARE 1 // \Registry\Machine\Software // since REDSTONE2
#define VR_KEY_CONTROL_SET 2      // \Registry\Machine\System\ControlSet001 // since REDSTONE2

// rev
/**
 * VR_GET_VIRTUAL_ROOT is used to request a virtual root key for a given job
 * and index (VR_KEY_*).
 */
typedef struct _VR_GET_VIRTUAL_ROOT
{
    HANDLE Job;
    ULONG Index; // VR_KEY_* // since REDSTONE2
} VR_GET_VIRTUAL_ROOT, *PVR_GET_VIRTUAL_ROOT;

// rev
/**
 * VR_GET_VIRTUAL_ROOT_RESULT contains the resulting key handle returned by
 * a VR_GET_VIRTUAL_ROOT request.
 */
typedef struct _VR_GET_VIRTUAL_ROOT_RESULT
{
    HANDLE Key;
} VR_GET_VIRTUAL_ROOT_RESULT, *PVR_GET_VIRTUAL_ROOT_RESULT;

// rev
/**
 * VR_LOAD_DIFFERENCING_HIVE_FOR_HOST describes parameters to load a differencing
 * hive on behalf of a host. It contains flags, path lengths and inline string
 * buffers for the key and hive paths.
 */
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
/**
 * VR_UNLOAD_DIFFERENCING_HIVE_FOR_HOST is used to request unloading of a
 * differencing hive for the host. It contains the target key path length and
 * path buffer.
 */
typedef struct _VR_UNLOAD_DIFFERENCING_HIVE_FOR_HOST
{
    ULONG Reserved;
    USHORT TargetKeyPathLength;
    WCHAR TargetKeyPath[ANYSIZE_ARRAY];
} VR_UNLOAD_DIFFERENCING_HIVE_FOR_HOST, *PVR_UNLOAD_DIFFERENCING_HIVE_FOR_HOST;

#endif // _NTREGAPI_H
