/*
 * Object Manager support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTOBAPI_H
#define _NTOBAPI_H

//
// Object Specific Access Rights
//

#ifndef OBJECT_TYPE_CREATE
#define OBJECT_TYPE_CREATE 0x0001
#endif

#ifndef OBJECT_TYPE_ALL_ACCESS
#define OBJECT_TYPE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | OBJECT_TYPE_CREATE)
#endif

//
// Directory Object Specific Access Rights
//

#ifndef DIRECTORY_QUERY
#define DIRECTORY_QUERY 0x0001
#endif

#ifndef DIRECTORY_TRAVERSE
#define DIRECTORY_TRAVERSE 0x0002
#endif

#ifndef DIRECTORY_CREATE_OBJECT
#define DIRECTORY_CREATE_OBJECT 0x0004
#endif

#ifndef DIRECTORY_CREATE_SUBDIRECTORY
#define DIRECTORY_CREATE_SUBDIRECTORY 0x0008
#endif

#ifndef DIRECTORY_ALL_ACCESS
#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | DIRECTORY_QUERY | DIRECTORY_TRAVERSE | DIRECTORY_CREATE_OBJECT | DIRECTORY_CREATE_SUBDIRECTORY)
#endif

//
// Symbolic Link Specific Access Rights
//

#ifndef SYMBOLIC_LINK_QUERY
#define SYMBOLIC_LINK_QUERY 0x0001
#endif

#ifndef SYMBOLIC_LINK_SET
#define SYMBOLIC_LINK_SET 0x0002
#endif

#ifndef SYMBOLIC_LINK_ALL_ACCESS
#define SYMBOLIC_LINK_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYMBOLIC_LINK_QUERY)
#endif

#ifndef SYMBOLIC_LINK_ALL_ACCESS_EX
#define SYMBOLIC_LINK_ALL_ACCESS_EX (STANDARD_RIGHTS_REQUIRED | SPECIFIC_RIGHTS_ALL)
#endif

//
// Object Attribute Flags
//

#ifndef OBJ_PROTECT_CLOSE
#define OBJ_PROTECT_CLOSE 0x00000001
#endif

#ifndef OBJ_INHERIT
#define OBJ_INHERIT 0x00000002
#endif

#ifndef OBJ_AUDIT_OBJECT_CLOSE
#define OBJ_AUDIT_OBJECT_CLOSE 0x00000004
#endif

//
// Object Information
//

#if (PHNT_MODE != PHNT_MODE_KERNEL)
typedef enum _OBJECT_INFORMATION_CLASS
{
    ObjectBasicInformation,         // q: OBJECT_BASIC_INFORMATION
    ObjectNameInformation,          // q: OBJECT_NAME_INFORMATION
    ObjectTypeInformation,          // q: OBJECT_TYPE_INFORMATION
    ObjectTypesInformation,         // q: OBJECT_TYPES_INFORMATION
    ObjectHandleFlagInformation,    // qs: OBJECT_HANDLE_FLAG_INFORMATION
    ObjectSessionInformation,       // s: void; directory object; set current process session ID (requires SeTcbPrivilege)
    ObjectSessionObjectInformation, // s: void; directory object; associate current session object (requires SeTcbPrivilege)
    ObjectSetRefTraceInformation,   // newer-kernel class; support in NtQueryObject/NtSetInformationObject is implementation-dependent
    MaxObjectInfoClass
} OBJECT_INFORMATION_CLASS;
#else
#define ObjectBasicInformation 0
#define ObjectNameInformation 1
#define ObjectTypeInformation 2
#define ObjectTypesInformation 3
#define ObjectHandleFlagInformation 4
#define ObjectSessionInformation 5
#define ObjectSessionObjectInformation 6
#define ObjectSetRefTraceInformation 7
#define MaxObjectInfoClass 8
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

/**
 * The OBJECT_BASIC_INFORMATION structure contains basic information about an object and its queried handle.
 * \remarks NtQueryObject(ObjectBasicInformation) requires exactly sizeof(OBJECT_BASIC_INFORMATION) (56 bytes).
 * The handle need not grant a specific access mask. SecurityDescriptorSize is obtained only when the
 * queried handle grants READ_CONTROL and the object has a security descriptor.
 * An incorrect buffer length returns STATUS_INFO_LENGTH_MISMATCH without updating ReturnLength
 * for native x64 calls; the WoW64 thunk instead writes 0.
 * Do not use a zero-length call to discover this fixed size.
 */
typedef struct _OBJECT_BASIC_INFORMATION
{
    ULONG Attributes;               // Handle attributes combined with OBJ_PERMANENT and OBJ_EXCLUSIVE object attributes.
    ACCESS_MASK GrantedAccess;      // Access rights granted to the queried handle.
    ULONG HandleCount;              // The number of handles that are currently open for the object.
    ULONG PointerCount;             // The number of references to the object from both handles and other references, such as those from the system.
    ULONG PagedPoolCharge;          // The amount of paged pool memory that the object is using.
    ULONG NonPagedPoolCharge;       // The amount of non-paged pool memory that the object is using.
    ULONG Reserved[3];              // Zero-filled reserved slots; do not interpret as usage counters.
    ULONG NameInfoSize;             // The size of the name information for the object.
    ULONG TypeInfoSize;             // The size of the type information for the object.
    ULONG SecurityDescriptorSize;   // The size of the security descriptor for the object.
    LARGE_INTEGER CreationTime;     // The time when a symbolic link was created. Not supported for other types of objects.
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

#if (PHNT_MODE != PHNT_MODE_KERNEL)
/**
 * The OBJECT_NAME_INFORMATION structure contains the name, if there is one, of a given object.
 */
typedef struct _OBJECT_NAME_INFORMATION
{
    UNICODE_STRING Name; // The object name (when present) includes a NULL-terminator and all path separators "\" in the name.
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

/**
 * The OBJECT_TYPE_INFORMATION structure contains various statistics and properties about an object type.
 * \remarks The fixed structure occupies 0x68 bytes on x64 and 0x60 bytes on x86, followed by the type name.
 * TypeName.Length excludes the terminating WCHAR; TypeName.MaximumLength includes it.
 * Each structure and its trailing name are rounded up to pointer alignment in ObjectTypesInformation output.
 * On x64, ObQueryTypeInfo does not write the four Total*Usage fields
 * or the four HighWater*Usage fields. Zero-initialize native output buffers, but do not
 * interpret those compatibility fields as measured pool/name/handle-table usage. The
 * WoW64 thunk copies them from an intermediate buffer, so even pre-zeroing the x86
 * caller buffer does not establish meaningful values for these fields.
 */
typedef struct _OBJECT_TYPE_INFORMATION
{
    UNICODE_STRING TypeName;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG TotalPagedPoolUsage; // Compatibility field; may be left unwritten by ObQueryTypeInfo.
    ULONG TotalNonPagedPoolUsage; // Compatibility field; may be left unwritten by ObQueryTypeInfo.
    ULONG TotalNamePoolUsage; // Compatibility field; may be left unwritten by ObQueryTypeInfo.
    ULONG TotalHandleTableUsage; // Compatibility field; may be left unwritten by ObQueryTypeInfo.
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    ULONG HighWaterPagedPoolUsage; // Compatibility field; may be left unwritten by ObQueryTypeInfo.
    ULONG HighWaterNonPagedPoolUsage; // Compatibility field; may be left unwritten by ObQueryTypeInfo.
    ULONG HighWaterNamePoolUsage; // Compatibility field; may be left unwritten by ObQueryTypeInfo.
    ULONG HighWaterHandleTableUsage; // Compatibility field; may be left unwritten by ObQueryTypeInfo.
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccessMask;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    UCHAR TypeIndex; // since WINBLUE
    CHAR ReservedByte; // Alignment padding, explicitly zeroed by ObQueryTypeInfo.
    ULONG PoolType;
    ULONG DefaultPagedPoolCharge;
    ULONG DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

/**
 * The OBJECT_TYPES_INFORMATION structure contains the number of object types defined in the system.
 * \remarks This 4-byte header is followed by pointer-aligned, variable-sized OBJECT_TYPE_INFORMATION records.
 * The first record begins at offset 0x08 on x64 or 0x04 on x86. Advance past each record's fixed structure
 * and TypeName.MaximumLength bytes, then round up to pointer alignment; do not index a fixed-size array.
 * NtQueryObject(ObjectTypesInformation) ignores Handle. A buffer shorter than 4 bytes
 * reports only the pointer-aligned header size (8 on x64), not the complete required
 * size. Supply at least 4 bytes to obtain the complete required length; retry if the
 * type list grows. Only parse complete records after a successful query.
 */
typedef struct _OBJECT_TYPES_INFORMATION
{
    ULONG NumberOfTypes;
} OBJECT_TYPES_INFORMATION, *POBJECT_TYPES_INFORMATION;

/**
 * The OBJECT_HANDLE_FLAG_INFORMATION structure contains flag information for an object handle.
 * \remarks The structure occupies 2 bytes: Inherit at offset 0 and ProtectFromClose at offset 1.
 * NtQueryObject accepts a buffer of at least 2 bytes; NtSetInformationObject requires exactly 2 bytes.
 */
typedef struct _OBJECT_HANDLE_FLAG_INFORMATION
{
    BOOLEAN Inherit;
    BOOLEAN ProtectFromClose;
} OBJECT_HANDLE_FLAG_INFORMATION, *POBJECT_HANDLE_FLAG_INFORMATION;

#if (PHNT_VERSION >= PHNT_WINDOWS_11_25H2)
/**
 * Reference-tracing buffer declaration retained for compatibility with newer object-information classes.
 * \remarks The fixed layout is 40 bytes on x64 and 24 bytes on x86, with ProcessName at offset 0x08.
 * PoolTags is at offset 0x18 on x64 and 0x10 on x86.
 * Do not assume NtQueryObject or NtSetInformationObject implements
 * ObjectSetRefTraceInformation. Their runtime reference-tracing helpers are instead reached through
 * NtQuerySystemInformation/NtSetSystemInformation with SystemRefTraceInformation (86), using
 * SYSTEM_REF_TRACE_INFORMATION. Do not assume this declaration establishes the newer class's contract.
 */
typedef struct _OBJECT_SET_REF_TRACE_INFORMATION
{
    BOOLEAN Enable;
    BOOLEAN EtwMode;
    UCHAR Reserved[6];
    UNICODE_STRING ProcessName;
    UNICODE_STRING PoolTags;
} OBJECT_SET_REF_TRACE_INFORMATION, *POBJECT_SET_REF_TRACE_INFORMATION;
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11_25H2)

//
// Objects, handles
//

#if (PHNT_MODE != PHNT_MODE_KERNEL)

/**
 * The NtQueryObject routine retrieves various kinds of object information.
 *
 * \param Handle The handle of the object for which information is being queried; ignored for ObjectTypesInformation.
 * \param ObjectInformationClass The information class indicating the kind of object information to be retrieved.
 * \param ObjectInformation An optional pointer to a buffer where the requested information is to be returned.
 * \param ObjectInformationLength The size of the buffer pointed to by the ObjectInformation parameter, in bytes.
 * \param ReturnLength An optional pointer to a location where the function writes the actual size of the information requested.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryobject
 * \remarks The implementation queries classes 0-4 only. Except for
 * ObjectTypesInformation, the handle is referenced before class validation with no
 * required access mask. Name/type buffers are variable sized; basic information is
 * exactly 56 bytes and handle flags require at least 2 bytes. Name and type strings
 * point into the caller-owned output buffer and become invalid when it is freed.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryObject(
    _In_opt_ HANDLE Handle,
    _In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

/**
 * The NtSetInformationObject routine changes various kinds of information about an object.
 *
 * \param Handle The handle of the object for which information is being changed.
 * \param ObjectInformationClass The type of information, supplied in the buffer pointed to by ObjectInformation, to set for the object.
 * \param ObjectInformation Pointer to the class-specific input. Ignored for ObjectSessionInformation and ObjectSessionObjectInformation.
 * \param ObjectInformationLength The input size in bytes. Exactly 2 for ObjectHandleFlagInformation; ignored for the session classes.
 * \return NTSTATUS Successful or errant status.
 * \remarks ObjectHandleFlagInformation changes this handle, not the object or other handles.
 * Nonzero BOOLEAN values request TRUE. Query again if the object type forbids inheritance:
 * ObSetHandleAttributes can return success without applying either requested flag.
 * For the two session classes, pass NULL/0; neither buffer argument is consumed.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationObject(
    _In_ HANDLE Handle,
    _In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _When_(ObjectInformationClass == ObjectSessionInformation || ObjectInformationClass == ObjectSessionObjectInformation, _In_opt_)
    _When_(ObjectInformationClass != ObjectSessionInformation && ObjectInformationClass != ObjectSessionObjectInformation, _In_reads_bytes_(ObjectInformationLength)) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength
    );

/**
 * Options flags for NtDuplicateObject
 */
#define DUPLICATE_CLOSE_SOURCE 0x00000001       // Close the source handle.
#define DUPLICATE_SAME_ACCESS 0x00000002        // Instead of using the DesiredAccess parameter, copy the access rights from the source handle to the target handle.
#define DUPLICATE_SAME_ATTRIBUTES 0x00000004    // Instead of using the HandleAttributes parameter, copy the attributes from the source handle to the target handle.

/**
 * The NtDuplicateObject routine creates a handle that is a duplicate of the specified source handle.
 *
 * \param SourceProcessHandle Source process handle with PROCESS_DUP_HANDLE access (0x40).
 * \param SourceHandle The handle to duplicate.
 * \param TargetProcessHandle Target process handle with PROCESS_DUP_HANDLE access (0x40).
 * Specify NULL with DUPLICATE_CLOSE_SOURCE for a close-only operation.
 * \param TargetHandle Optional pointer receiving the new handle, which is valid in the target process.
 * NULL suppresses only the returned value: if a target process is supplied, duplication
 * still occurs and discarding the value can leak a handle in that process.
 * \param DesiredAccess An ACCESS_MASK value that specifies the desired access for the new handle.
 * \param HandleAttributes A ULONG that specifies the desired attributes for the new handle.
 * \param Options A set of flags to control the behavior of the duplication operation.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-zwduplicateobject
 * \remarks Use NULL TargetProcessHandle with DUPLICATE_CLOSE_SOURCE to close without duplicating.
 * Do not assume a failed duplication preserved the source when DUPLICATE_CLOSE_SOURCE was
 * requested: several failure paths close it. A returned duplicate is owned by the target
 * process and must be closed there. Closing the original does not close the duplicate.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDuplicateObject(
    _In_ HANDLE SourceProcessHandle,
    _In_ HANDLE SourceHandle,
    _In_opt_ HANDLE TargetProcessHandle,
    _Out_opt_ PHANDLE TargetHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG HandleAttributes,
    _In_ ULONG Options
    );

/**
 * The NtMakeTemporaryObject routine changes the attributes of an object to make it temporary.
 *
 * \param Handle Handle to an object of any type, with DELETE access.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwmaketemporaryobject
 * \remarks Clears permanent-object lifetime semantics; it does not close Handle or immediately
 * delete an object that still has references. The caller remains responsible for NtClose.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMakeTemporaryObject(
    _In_ HANDLE Handle
    );

/**
 * The NtMakePermanentObject routine makes an object permanent. User-mode callers require SeCreatePermanentPrivilege.
 *
 * \param Handle Handle to an object of any type.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwmaketemporaryobject
 * \remarks No particular handle access mask is requested after the privilege check.
 * Permanent-object changes are not a substitute for retaining a handle; use
 * NtMakeTemporaryObject with DELETE access when removing permanent lifetime semantics.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMakePermanentObject(
    _In_ HANDLE Handle
    );

/**
 * Signals or releases one object, then waits on another in one native call.
 * This is not a transaction or an atomic handoff guarantee to other threads:
 * a wait timeout or APC/alert does not roll back a completed signal operation.
 *
 * \param SignalHandle Event with EVENT_MODIFY_STATE (0x2), semaphore with
 * SEMAPHORE_MODIFY_STATE (0x2), or a mutant owned by the calling thread. No specific
 * granted access bit is required for the mutant's release, but ownership is required.
 * \param WaitHandle Waitable-object handle requiring SYNCHRONIZE (0x00100000).
 * \param Alertable TRUE permits an alert or ordinary user APC to interrupt the wait.
 * \param Timeout Optional 100-nanosecond deadline: NULL for no deadline, pointed-to
 * zero for polling, negative for a relative interval, positive for absolute NT system time.
 * \return On a completed signal, the subsequent wait's status is returned. A signal
 * failure such as STATUS_MUTANT_NOT_OWNED or STATUS_SEMAPHORE_LIMIT_EXCEEDED ends
 * the call without waiting. Other signal-object types return STATUS_OBJECT_TYPE_MISMATCH.
 * \remarks Both handles are referenced, and WaitHandle's SYNCHRONIZE access is
 * checked, before signaling on x64. An invalid or inaccessible wait
 * handle therefore does not signal the first object. Event signaling sets the event;
 * semaphore signaling increments by 1; mutant signaling releases one acquisition.
 *
 * IMPORTANT: STATUS_ABANDONED (0x80) is ambiguous in this combined call. If the
 * signal object is an abandoned mutant not owned by the caller, its release returns
 * 0x80 and the wait is never entered: no ownership of the wait object is acquired.
 * Alternatively, after a successful signal, waiting on an abandoned mutant returns
 * 0x80 and DOES acquire that wait mutant. Do not infer wait-object acquisition from
 * 0x80 without distinguishing these paths. STATUS_TIMEOUT (0x102), STATUS_USER_APC
 * (0xC0) and STATUS_ALERTED (0x101) also do not prove wait-object acquisition.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-signalobjectandwait
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSignalAndWaitForSingleObject(
    _In_ HANDLE SignalHandle,
    _In_ HANDLE WaitHandle,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );

/**
 * Waits for a waitable object's dispatcher condition, optionally with a deadline.
 * A successful wait applies that object's wait semantics: for example, it consumes
 * a synchronization-event signal or semaphore count, or acquires mutant ownership.
 *
 * \param Handle Waitable-object handle with SYNCHRONIZE access (0x00100000).
 * \param Alertable TRUE permits interruption by an alert or ordinary user APC;
 * the APC executes on the waiting thread, not on a new worker thread.
 * \param Timeout Optional deadline in 100-nanosecond units. NULL means no deadline,
 * a pointed-to zero polls, negative values are relative intervals, and positive
 * values are absolute NT system times since January 1, 1601 (UTC).
 * \return STATUS_WAIT_0 (0) for a satisfied ordinary wait, STATUS_ABANDONED (0x80)
 * when acquiring an abandoned mutant, STATUS_USER_APC (0xC0) or STATUS_ALERTED
 * (0x101) for interruption, STATUS_TIMEOUT (0x102) for expiration, or an error.
 * \remarks Timeout ends the wait without satisfying the object's condition.
 * NT_SUCCESS is not sufficient to establish acquisition: timeout and interruption
 * are nonnegative too. Abandoned-mutant acquisition does grant ownership; validate
 * the protected state before using it and release the mutant when finished.
 * An already-satisfied object can complete even with an expired deadline.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntwaitforsingleobject
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForSingleObject(
    _In_ HANDLE Handle,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );

/**
 * Waits for one or all of an array of waitable objects. Use WaitAny to identify
 * the satisfied object, or WaitAll to acquire the complete set without partially
 * consuming available signals while other objects are not ready.
 *
 * \param Count Number of handles, from 1 through MAXIMUM_WAIT_OBJECTS (64).
 * \param Handles Array of Count pointer-sized handles, each requiring SYNCHRONIZE
 * (0x00100000). WaitAll rejects duplicate underlying dispatcher objects, including
 * distinct handle values obtained by duplicating a handle to the same object.
 * \param WaitType WaitAll (0) or WaitAny (1). WaitAny selects the lowest array
 * index among objects simultaneously eligible when the wait is evaluated.
 * \param Alertable TRUE permits an alert or ordinary user APC to interrupt the wait.
 * \param Timeout Optional 100-nanosecond deadline: NULL for no deadline, pointed-to
 * zero for polling, negative for a relative interval, positive for absolute NT system time.
 * \return WaitAny returns STATUS_WAIT_0 plus the selected zero-based index, or
 * STATUS_ABANDONED_WAIT_0 (0x80) plus that index when acquiring an abandoned mutant.
 * WaitAll returns STATUS_SUCCESS (0), or STATUS_ABANDONED (0x80) when the set is
 * acquired and includes an abandoned mutant; the latter does not identify an index.
 * STATUS_TIMEOUT (0x102), STATUS_USER_APC (0xC0) and STATUS_ALERTED (0x101) do not
 * establish acquisition. Test the status category, not just NT_SUCCESS.
 * \remarks On the native x64 and WoW64 paths, invalid Count returns
 * STATUS_INVALID_PARAMETER_1, invalid WaitType returns STATUS_INVALID_PARAMETER_3,
 * and WaitAll with duplicate objects returns STATUS_INVALID_PARAMETER_MIX.
 * All handles are referenced and access-checked before entering the dispatcher wait;
 * an inaccessible later handle fails even if an earlier object is already signaled.
 * A WaitAll timeout does not consume signals or acquire an abandoned mutant from
 * an incomplete set. A completion-port handle is rejected for WaitAll with
 * STATUS_INVALID_HANDLE. Cross-VM and special object paths
 * have additional restrictions; this is not a promise that any handle is waitable.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitformultipleobjectsex
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForMultipleObjects(
    _In_ ULONG Count,
    _In_reads_(Count) HANDLE Handles[],
    _In_ WAIT_TYPE WaitType,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );

/**
 * Waits on an array of signed 32-bit handle values. The x64 kernel sign-extends
 * each LONG into its native handle array and uses the same dispatcher path as
 * NtWaitForMultipleObjects. The WoW64 NtWaitForMultipleObjects thunk calls this
 * native entry; its name does not mean that the array contains 64-bit HANDLEs.
 *
 * \param Count Number of entries, from 1 through MAXIMUM_WAIT_OBJECTS (64).
 * \param Handles Array of Count signed 32-bit handles, each with SYNCHRONIZE
 * (0x00100000). On a 64-bit caller use LONG storage, not a cast of a HANDLE array.
 * \param WaitType WaitAll (0) or WaitAny (1).
 * \param Alertable TRUE permits an alert or ordinary user APC to interrupt the wait.
 * \param Timeout Optional 100-nanosecond deadline: NULL for no deadline, pointed-to
 * zero for polling, negative for a relative interval, positive for absolute NT system time.
 * \return The same status categories as NtWaitForMultipleObjects: WaitAny returns
 * the selected index added to STATUS_WAIT_0 or STATUS_ABANDONED_WAIT_0 (0x80);
 * WaitAll returns 0 or STATUS_ABANDONED (0x80) without an index. Timeout (0x102)
 * and APC/alert interruption (0xC0/0x101) are not evidence of acquisition.
 * \remarks Invalid Count/WaitType return STATUS_INVALID_PARAMETER_1/_3 on x64. WaitAll rejects duplicate underlying objects
 * with STATUS_INVALID_PARAMETER_MIX and does not partially consume signals on timeout.
 * See NtWaitForMultipleObjects for access checks and object restrictions. Native x64
 * invocation and the WoW64 forwarding path were checked separately; do not assume
 * every architecture exposes a separate user-mode export with this name.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForMultipleObjects32(
    _In_ ULONG Count,
    _In_reads_(Count) LONG Handles[],
    _In_ WAIT_TYPE WaitType,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );

/**
 * The NtSetSecurityObject routine sets an object's security state.
 *
 * \param Handle Handle for the object whose security state is to be set.
 * \param SecurityInformation A SECURITY_INFORMATION value specifying the information to be set.
 * \param SecurityDescriptor Pointer to the security descriptor to be set for the object.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-zwsetsecurityobject
 * \remarks For user-mode handles, OWNER/GROUP/LABEL selections require WRITE_OWNER (0x00080000);
 * DACL/ATTRIBUTE/PROCESS_TRUST_LABEL/ACCESS_FILTER require WRITE_DAC (0x00040000);
 * SACL/SCOPE require ACCESS_SYSTEM_SECURITY (0x01000000). BACKUP and SACL-protection
 * flags can add further combined requirements. The descriptor is captured during the
 * call; ownership is not transferred. Access rights alone do not ensure success:
 * the object security method can reject the update, including STATUS_NO_SECURITY_ON_OBJECT
 * for an object with no associated security descriptor.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSecurityObject(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

/**
 * The NtQuerySecurityObject routine retrieves a copy of an object's security descriptor.
 *
 * \param Handle Handle for the object whose security descriptor is to be queried.
 * \param SecurityInformation A SECURITY_INFORMATION value specifying the information to be queried.
 * \param SecurityDescriptor Caller-allocated buffer that NtQuerySecurityObject fills with a copy of the specified security descriptor.
 * \param Length Size, in bytes, of the buffer pointed to by SecurityDescriptor.
 * \param LengthNeeded Pointer to a caller-allocated variable that receives the number of bytes required to store the copied security descriptor.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntquerysecurityobject
 * \remarks For user-mode handles, OWNER/GROUP/DACL/LABEL/ATTRIBUTE/SCOPE/PROCESS_TRUST_LABEL/
 * ACCESS_FILTER queries require READ_CONTROL (0x00020000). SACL requires
 * ACCESS_SYSTEM_SECURITY (0x01000000); BACKUP requests both. Requirements combine when
 * multiple bits are selected. LengthNeeded is mandatory. Use NULL/0 to size the output
 * and retry STATUS_BUFFER_TOO_SMALL; a handle-reference failure need not update it.
 * The returned self-relative descriptor is a caller-owned byte buffer.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySecurityObject(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_writes_bytes_to_opt_(Length, *LengthNeeded) PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ULONG Length,
    _Out_ PULONG LengthNeeded
    );

/**
 * The NtClose routine closes the specified handle.
 *
 * \param Handle The handle being closed.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwclose
 * \remarks Handle is invalidated only on successful closure. A protected handle is not closed.
 * Invalid handles can raise a user exception under a debugger or handle-exception policy;
 * callers must not use double-close behavior as a validity test. Pseudo-handle behavior is
 * build-specific and does not release the pseudo-handle's underlying process or thread.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtClose(
    _In_ _Post_ptr_invalid_ HANDLE Handle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
/**
 * Compares two object handles to determine if they refer to the same underlying kernel object.
 *
 * \param FirstObjectHandle The first object handle to compare.
 * \param SecondObjectHandle The second object handle to compare.
 * \return STATUS_SUCCESS for the same object, STATUS_NOT_SAME_OBJECT for different objects, or a handle-reference error.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-compareobjecthandles
 * \remarks Neither handle needs a particular access mask. The comparison uses object identity,
 * not handle values, names, or matching object types; distinct objects return
 * STATUS_NOT_SAME_OBJECT (0xC00001AC).
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCompareObjects(
    _In_ HANDLE FirstObjectHandle,
    _In_ HANDLE SecondObjectHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_10)

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

//
// Directory objects
//

#if (PHNT_MODE != PHNT_MODE_KERNEL)

/**
 * The NtCreateDirectoryObject routine creates or opens an object-directory object.
 *
 * \param DirectoryHandle Pointer to a HANDLE variable that receives a handle to the object directory.
 * \param DesiredAccess An ACCESS_MASK that specifies the requested access to the directory object.
 * \param ObjectAttributes The attributes for the directory object.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwcreatedirectoryobject
 * \remarks ObjectAttributes may be NULL to create an unnamed directory. This is useful as a
 * private RootDirectory for subsequent relative object creation; the returned handle
 * is owned by the caller and must be closed with NtClose.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateDirectoryObject(
    _Out_ PHANDLE DirectoryHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
/**
 * The NtCreateDirectoryObjectEx routine creates or opens an object-directory object with extended options, such as shadow directories.
 *
 * \param DirectoryHandle A pointer to a HANDLE variable that receives the directory object handle.
 * \param DesiredAccess The access mask that specifies the requested access to the directory object.
 * \param ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the object name and other attributes.
 * \param ShadowDirectoryHandle An optional shadow directory handle with DIRECTORY_QUERY | DIRECTORY_TRAVERSE access.
 * \param Flags Creation flags. The kernel implementation accepts only bits 0x1 and 0x2; bit 0x2 is kernel-mode only.
 * User-mode callers can pass 0 or 0x1. Other values return STATUS_INVALID_PARAMETER_5.
 * \return NTSTATUS Successful or errant status.
 * \remarks ObjectAttributes may be NULL to create an unnamed directory. This is useful as a
 * private RootDirectory for subsequent relative object creation; the returned handle
 * is owned by the caller and must be closed with NtClose.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateDirectoryObjectEx(
    _Out_ PHANDLE DirectoryHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ HANDLE ShadowDirectoryHandle,
    _In_ ULONG Flags
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

/**
 * Opens an existing directory object.
 *
 * \param DirectoryHandle A handle to the newly opened directory object.
 * \param DesiredAccess An ACCESS_MASK that specifies the requested access to the directory object.
 * \param ObjectAttributes The attributes for the directory object.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/devnotes/ntopendirectoryobject
 * \remarks Opens by ObjectAttributes.ObjectName, relative to RootDirectory when supplied, in
 * the current silo's object namespace. The returned handle is caller-owned. No object
 * is created if the name does not exist.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenDirectoryObject(
    _Out_ PHANDLE DirectoryHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * The OBJECT_DIRECTORY_INFORMATION structure describes a named entry in an object directory.
 * \remarks NtQueryDirectoryObject returns an array of these records, followed by a zero-filled terminator
 * and the name strings. Name.Buffer and TypeName.Buffer point into the caller's output buffer.
 * The fixed record is 32 bytes on x64 and 16 bytes on x86; string lengths are measured in bytes.
 */
typedef struct _OBJECT_DIRECTORY_INFORMATION
{
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

/**
 * Retrieves information about the specified directory object.
 *
 * \param DirectoryHandle A handle to the directory object with DIRECTORY_QUERY access.
 * \param Buffer A pointer to a buffer that receives the directory information.
 * \param Length The size, in bytes, of the buffer pointed to by the Buffer parameter.
 * \param ReturnSingleEntry A BOOLEAN value that specifies whether to return a single entry or multiple entries.
 * \param RestartScan A BOOLEAN value that specifies whether to restart the scan from the beginning of the directory.
 * \param Context A pointer to a variable that maintains the context of the directory enumeration.
 * \param ReturnLength An optional pointer to a variable that receives the number of bytes returned in the buffer.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/devnotes/ntquerydirectoryobject
 * \remarks Context is required even when RestartScan is TRUE. It is an enumeration position,
 * not a handle, and directory modifications can change what a later position denotes.
 * On the native x64 path, a single-entry short buffer returns STATUS_BUFFER_TOO_SMALL
 * and ReturnLength includes the required entry, terminator record and both strings.
 * A multi-entry short buffer returns STATUS_MORE_ENTRIES, possibly with zero entries
 * and an unchanged Context. In that case grow the buffer rather than retrying forever
 * at the same size. WoW64 instead returns STATUS_BUFFER_TOO_SMALL when no converted
 * entry fits, including multi-entry requests; its ReturnLength is the required converted
 * batch size. STATUS_NO_MORE_ENTRIES ends enumeration; do not parse its buffer.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDirectoryObject(
    _In_ HANDLE DirectoryHandle,
    _Out_writes_bytes_opt_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_ BOOLEAN RestartScan,
    _Inout_ PULONG Context,
    _Out_opt_ PULONG ReturnLength
    );

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

//
// Private namespaces
//

#if (PHNT_MODE != PHNT_MODE_KERNEL)

/**
 * The BOUNDARY_ENTRY_TYPE enumeration identifies the kind of value stored in an object boundary entry.
 */
typedef enum _BOUNDARY_ENTRY_TYPE
{
    BOUNDARY_ENTRY_TYPE_INVALID,
    BOUNDARY_ENTRY_TYPE_NAME,
    BOUNDARY_ENTRY_TYPE_SID,
    BOUNDARY_ENTRY_TYPE_IL
} BOUNDARY_ENTRY_TYPE;

/**
 * Provides views of the inline payload immediately following OBJECT_BOUNDARY_ENTRY.
 * Name is variable-length WCHAR data; Sid and IntegrityLabel begin with an inline SID,
 * not a stored pointer. SID.SubAuthority is variable length: obtain its actual byte
 * size from the SID header, and use the entry Size for bounds and iteration.
 * sizeof(OBJECT_BOUNDARY_VALUE) is not a serialized entry size.
 */
typedef union _OBJECT_BOUNDARY_VALUE
{
    WCHAR Name[ANYSIZE_ARRAY];
    SID Sid;
    SID IntegrityLabel;
} OBJECT_BOUNDARY_VALUE, *POBJECT_BOUNDARY_VALUE;

/**
 * The OBJECT_BOUNDARY_ENTRY structure is the 8-byte header of a variable-sized boundary entry.
 * \remarks Size includes this header and its inline payload. NAME entries contain WCHAR data;
 * SID and IL entries contain a SID, not a PSID. Advance by Size and round up to an 8-byte boundary.
 */
typedef struct _OBJECT_BOUNDARY_ENTRY
{
    BOUNDARY_ENTRY_TYPE Type;
    ULONG Size;
} OBJECT_BOUNDARY_ENTRY, *POBJECT_BOUNDARY_ENTRY;

// rev
#define OBJECT_BOUNDARY_DESCRIPTOR_VERSION 1

/**
 * The OBJECT_BOUNDARY_DESCRIPTOR structure describes the boundary conditions for a private namespace.
 * \remarks The fixed header is 16 bytes and Version must be OBJECT_BOUNDARY_DESCRIPTOR_VERSION (1).
 * Items counts the trailing variable-sized entries; TotalSize covers the complete descriptor.
 * Entry types 1 (name), 2 (SID), and 3 (integrity label) are accepted. At most one name and one integrity
 * label entry are allowed; successive entries are aligned to 8-byte boundaries.
 * ObpCaptureBoundaryDescriptor on x64 requires a user-mode TotalSize
 * from 16 through 32767 (0x7FFF) bytes and a 4-byte-aligned descriptor. Flag bit 0
 * requests adding the effective token's AppContainer SID when applicable. The caller
 * retains ownership; the kernel captures a copy for each create/open request.
 */
typedef struct _OBJECT_BOUNDARY_DESCRIPTOR
{
    ULONG Version;
    ULONG Items;
    ULONG TotalSize;
    union
    {
        ULONG Flags;
        struct
        {
            ULONG AddAppContainerSid : 1;
            ULONG Reserved : 31;
        };
    };
    //OBJECT_BOUNDARY_ENTRY Entries[1];
} OBJECT_BOUNDARY_DESCRIPTOR, *POBJECT_BOUNDARY_DESCRIPTOR;

/**
 * Creates a private namespace.
 *
 * \param NamespaceHandle A handle to the newly created private namespace.
 * \param DesiredAccess An ACCESS_MASK that specifies the requested access to the private namespace.
 * \param ObjectAttributes The attributes for the private namespace.
 * \param BoundaryDescriptor A descriptor that defines how the namespace is to be isolated. The RtlCreateBoundaryDescriptor function creates a boundary descriptor.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createprivatenamespacea
 * \remarks The namespace is identified by the boundary descriptor, not an object-directory
 * path. ObjectAttributes may be NULL; for creation, ObjectName must be NULL (a named
 * object returns STATUS_OBJECT_NAME_INVALID). The boundary is captured, not retained
 * as a user pointer. Release an Rtl-created descriptor with RtlDeleteBoundaryDescriptor
 * and close the returned directory handle with NtClose.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreatePrivateNamespace(
    _Out_ PHANDLE NamespaceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ POBJECT_BOUNDARY_DESCRIPTOR BoundaryDescriptor
    );

/**
 * Opens a private namespace.
 *
 * \param NamespaceHandle A handle to the newly opened private namespace.
 * \param DesiredAccess An ACCESS_MASK that specifies the requested access to the private namespace.
 * \param ObjectAttributes The attributes for the private namespace.
 * \param BoundaryDescriptor A descriptor that defines how the namespace is to be isolated. The RtlCreateBoundaryDescriptor function creates a boundary descriptor.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-openprivatenamespacea
 * \remarks Looks up the namespace by its captured boundary descriptor in the current silo.
 * ObjectAttributes is optional and only its Attributes member is used by this entry
 * point; ObjectName is not a path selector. No matching namespace returns
 * STATUS_OBJECT_PATH_NOT_FOUND. The returned directory handle must be closed with NtClose.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenPrivateNamespace(
    _Out_ PHANDLE NamespaceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ POBJECT_BOUNDARY_DESCRIPTOR BoundaryDescriptor
    );

/**
 * Removes a private namespace from the namespace table without closing the supplied handle.
 *
 * \param NamespaceHandle A private namespace handle with DELETE access. The caller must also pass the namespace creator check.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/namespaceapi/nf-namespaceapi-closeprivatenamespace
 * \remarks The creator check validates the effective token against the boundary (including
 * SID/integrity constraints); it is not a comparison with the original creator PID.
 * After successful removal, new opens by boundary fail, while existing handles remain
 * valid and must still be closed. An ordinary directory handle is not a namespace handle.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeletePrivateNamespace(
    _In_ HANDLE NamespaceHandle
    );

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

//
// Symbolic links
//

#if (PHNT_MODE != PHNT_MODE_KERNEL)

/**
 * The NtCreateSymbolicLinkObject routine creates a symbolic link object.
 *
 * \param LinkHandle Pointer to a HANDLE variable that receives a handle to the symbolic link object.
 * \param DesiredAccess An ACCESS_MASK that specifies the requested access to the symbolic link object.
 * \param ObjectAttributes The attributes for the symbolic link object.
 * \param LinkTarget A pointer to a UNICODE_STRING that specifies the target name.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwcreatesymboliclinkobject
 * \remarks ObjectAttributes and LinkTarget are required pointers.
 * LinkTarget.Length must be even and no greater than MaximumLength after MaximumLength
 * is rounded down to an even value; that rounded maximum must be nonzero. For user-mode
 * callers the kernel copies MaximumLength bytes, not just Length: every advertised byte
 * must be readable. A terminator is preserved only if supplied in those copied bytes.
 * This creates an Object Manager link, not a file-system reparse point.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateSymbolicLinkObject(
    _Out_ PHANDLE LinkHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ PCUNICODE_STRING LinkTarget
    );

/**
 * The NtOpenSymbolicLinkObject routine opens an existing symbolic link object.
 *
 * \param LinkHandle Pointer to a HANDLE variable that receives a handle to the symbolic link object.
 * \param DesiredAccess An ACCESS_MASK that specifies the requested access to the symbolic link object.
 * \param ObjectAttributes The attributes for the symbolic link object.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwopensymboliclinkobject
 * \remarks Opens by ObjectAttributes.ObjectName, relative to RootDirectory when supplied, in
 * the current silo's object namespace. The returned handle is caller-owned. No object
 * is created if the name does not exist.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSymbolicLinkObject(
    _Out_ PHANDLE LinkHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * The NtQuerySymbolicLinkObject routine queries the target of a symbolic link object.
 *
 * \param LinkHandle Handle to the symbolic link object with SYMBOLIC_LINK_QUERY access.
 * \param LinkTarget Caller-allocated UNICODE_STRING that receives the target name.
 * \param ReturnedLength Optional pointer receiving the target MaximumLength in bytes. When supplied, the output buffer
 * must accommodate MaximumLength; otherwise, only Length bytes are required and copied, without a guaranteed terminator.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwquerysymboliclinkobject
 * \remarks Only LinkTarget.Length and its caller-provided Buffer contents are updated; Buffer
 * and MaximumLength remain caller-owned. Supplying ReturnedLength requests copying the
 * stored MaximumLength bytes and reports that size on STATUS_BUFFER_TOO_SMALL. A copied
 * terminator exists only if the original target data included one.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySymbolicLinkObject(
    _In_ HANDLE LinkHandle,
    _Inout_ PUNICODE_STRING LinkTarget,
    _Out_opt_ PULONG ReturnedLength
    );

/**
 * The SYMBOLIC_LINK_INFO_CLASS enumeration specifies symbolic link information classes.
 * \remarks NtSetInformationSymbolicLink requires SYMBOLIC_LINK_SET access and SeTcbPrivilege.
 * Both classes require exactly 4 bytes, aligned to 4 bytes for user-mode callers. Server-silo callers
 * are rejected with STATUS_PRIVILEGE_NOT_HELD. An unsupported class returns STATUS_INVALID_INFO_CLASS;
 * an incorrect length returns STATUS_INFO_LENGTH_MISMATCH after the handle has been referenced.
 */
typedef enum _SYMBOLIC_LINK_INFO_CLASS
{
    SymbolicLinkGlobalInformation = 1, // s: ULONG; marks global, nonzero also sets an additional lookup flag (sticky).
    SymbolicLinkAccessMask, // s: ACCESS_MASK used when checking traversal of this link, not a new handle GrantedAccess.
    MaxSymbolicLinkInfoClass
} SYMBOLIC_LINK_INFO_CLASS;

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
/**
 * The NtSetInformationSymbolicLink routine sets information for a symbolic link object.
 *
 * \param LinkHandle Handle to the symbolic link object with SYMBOLIC_LINK_SET access.
 * \param SymbolicLinkInformationClass The information class indicating the kind of symbolic link information to be set.
 * \param SymbolicLinkInformation Pointer to a buffer that contains the information to set for the symbolic link object.
 * \param SymbolicLinkInformationLength The size of the buffer pointed to by the SymbolicLinkInformation parameter, in bytes.
 * \return NTSTATUS Successful or errant status.
 * \remarks The two implemented classes each consume 4 bytes. GlobalInformation always sets
 * the global flag; a nonzero input additionally sets an internal lookup flag and zero
 * does not clear a previously set flag. AccessMask stores a traversal-check mask; it
 * does not rewrite the access rights already granted to LinkHandle.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationSymbolicLink(
    _In_ HANDLE LinkHandle,
    _In_ SYMBOLIC_LINK_INFO_CLASS SymbolicLinkInformationClass,
    _In_reads_bytes_(SymbolicLinkInformationLength) PVOID SymbolicLinkInformation,
    _In_ ULONG SymbolicLinkInformationLength
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_10)

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

#endif // _NTOBAPI_H
