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
    ObjectBasicInformation, // q: OBJECT_BASIC_INFORMATION
    ObjectNameInformation, // q: OBJECT_NAME_INFORMATION
    ObjectTypeInformation, // q: OBJECT_TYPE_INFORMATION
    ObjectTypesInformation, // q: OBJECT_TYPES_INFORMATION
    ObjectHandleFlagInformation, // qs: OBJECT_HANDLE_FLAG_INFORMATION
    ObjectSessionInformation, // s: void // change object session // (requires SeTcbPrivilege)
    ObjectSessionObjectInformation, // s: void // change object session // (requires SeTcbPrivilege)
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
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

/**
 * The OBJECT_BASIC_INFORMATION structure contains basic information about an object.
 */
typedef struct _OBJECT_BASIC_INFORMATION
{
    ULONG Attributes;               // The attributes of the object include whether the object is permanent, can be inherited, and other characteristics.
    ACCESS_MASK GrantedAccess;      // Specifies a mask that represents the granted access when the object was created.
    ULONG HandleCount;              // The number of handles that are currently open for the object.
    ULONG PointerCount;             // The number of references to the object from both handles and other references, such as those from the system.
    ULONG PagedPoolCharge;          // The amount of paged pool memory that the object is using.
    ULONG NonPagedPoolCharge;       // The amount of non-paged pool memory that the object is using.
    ULONG Reserved[3];              // Reserved for future use.
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
 * The OBJECT_NAME_INFORMATION structure contains various statistics and properties about an object type.
 */
typedef struct _OBJECT_TYPE_INFORMATION
{
    UNICODE_STRING TypeName;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG TotalPagedPoolUsage;
    ULONG TotalNonPagedPoolUsage;
    ULONG TotalNamePoolUsage;
    ULONG TotalHandleTableUsage;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    ULONG HighWaterPagedPoolUsage;
    ULONG HighWaterNonPagedPoolUsage;
    ULONG HighWaterNamePoolUsage;
    ULONG HighWaterHandleTableUsage;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccessMask;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    UCHAR TypeIndex; // since WINBLUE
    CHAR ReservedByte;
    ULONG PoolType;
    ULONG DefaultPagedPoolCharge;
    ULONG DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

typedef struct _OBJECT_TYPES_INFORMATION
{
    ULONG NumberOfTypes;
} OBJECT_TYPES_INFORMATION, *POBJECT_TYPES_INFORMATION;

typedef struct _OBJECT_HANDLE_FLAG_INFORMATION
{
    BOOLEAN Inherit;
    BOOLEAN ProtectFromClose;
} OBJECT_HANDLE_FLAG_INFORMATION, *POBJECT_HANDLE_FLAG_INFORMATION;

//
// Objects, handles
//

#if (PHNT_MODE != PHNT_MODE_KERNEL)

/**
 * The NtQueryObject routine retrieves various kinds of object information.
 *
 * @param Handle The handle of the object for which information is being queried.
 * @param ObjectInformationClass The information class indicating the kind of object information to be retrieved.
 * @param ObjectInformation An optional pointer to a buffer where the requested information is to be returned.
 * @param ObjectInformationLength The size of the buffer pointed to by the ObjectInformation parameter, in bytes.
 * @param ReturnLength An optional pointer to a location where the function writes the actual size of the information requested.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryobject
 */
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
 * The NtSetInformationObject routine changes various kinds of information about a object.
 *
 * @param Handle The handle of the object for which information is being changed.
 * @param ObjectInformationClass The type of information, supplied in the buffer pointed to by ObjectInformation, to set for the object.
 * @param ObjectInformation Pointer to a buffer that contains the information to set for the object.
 * @param ObjectInformationLength The size of the buffer pointed to by the ObjectInformation parameter, in bytes.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationObject(
    _In_ HANDLE Handle,
    _In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _In_reads_bytes_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength
    );

#define DUPLICATE_CLOSE_SOURCE 0x00000001       // Close the source handle.
#define DUPLICATE_SAME_ACCESS 0x00000002        // Instead of using the DesiredAccess parameter, copy the access rights from the source handle to the target handle.
#define DUPLICATE_SAME_ATTRIBUTES 0x00000004    // Instead of using the HandleAttributes parameter, copy the attributes from the source handle to the target handle.

/**
 * The NtDuplicateObject routine creates a handle that is a duplicate of the specified source handle.
 *
 * @param SourceProcessHandle A handle to the source process for the handle being duplicated.
 * @param SourceHandle The handle to duplicate.
 * @param TargetProcessHandle A handle to the target process that is to receive the new handle. This parameter is optional and can be specified as NULL if the DUPLICATE_CLOSE_SOURCE flag is set in Options.
 * @param TargetHandle A pointer to a HANDLE variable into which the routine writes the new duplicated handle. The duplicated handle is valid in the specified target process. This parameter is optional and can be specified as NULL if no duplicate handle is to be created.
 * @param DesiredAccess An ACCESS_MASK value that specifies the desired access for the new handle.
 * @param HandleAttributes A ULONG that specifies the desired attributes for the new handle.
 * @param Options A set of flags to control the behavior of the duplication operation.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-zwduplicateobject
 */
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
 * @param Handle Handle to an object of any type.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwmaketemporaryobject
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMakeTemporaryObject(
    _In_ HANDLE Handle
    );

/**
 * The NtMakePermanentObject routine changes the attributes of an object to make it permanent.
 *
 * @param Handle Handle to an object of any type.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwmaketemporaryobject
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMakePermanentObject(
    _In_ HANDLE Handle
    );

/**
 * The NtSignalAndWaitForSingleObject routine signals one object and waits on another object as a single operation.
 *
 * @param SignalHandle A handle to the object to be signaled.
 * @param WaitHandle A handle to the object to wait on. The SYNCHRONIZE access right is required.
 * @param Alertable If this parameter is TRUE, the function returns when the system queues an I/O completion routine or APC function, and the thread calls the function.
 * @param Timeout The time-out interval. The function returns if the interval elapses, even if the object's state is nonsignaled and no completion or APC objects are queued.
 * If zero, the function tests the object's state, checks for queued completion routines or APCs, and returns immediately. 
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-signalobjectandwait
 */
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
 * The NtWaitForSingleObject routine waits until the specified object is in the signaled state or the time-out interval elapses.
 *
 * @param Handle The handle to the wait object.
 * @param Alertable The function returns when either the time-out period has elapsed or when the APC function is called.
 * @param Timeout A pointer to an absolute or relative time over which the wait is to occur. Can be null. If a timeout is specified,
 * and the object has not attained a state of signaled when the timeout expires, then the wait is automatically satisfied.
 * If an explicit timeout value of zero is specified, then no wait occurs if the wait cannot be satisfied immediately.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntwaitforsingleobject
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForSingleObject(
    _In_ HANDLE Handle,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );

/**
 * The NtWaitForMultipleObjects routine waits until one or all of the specified objects are in the signaled state, an I/O completion routine or asynchronous procedure call (APC) is queued to the thread, or the time-out interval elapses.
 *
 * @param Count The number of object handles to wait for in the array pointed to by lpHandles. The maximum number of object handles is MAXIMUM_WAIT_OBJECTS. This parameter cannot be zero.
 * @param Handles An array of object handles. The array can contain handles of objects of different types. It may not contain multiple copies of the same handle.
 * @param WaitType If this parameter is WaitAll, the function returns when the state of all objects in the Handles array is set to signaled.
 * @param Alertable f this parameter is TRUE and the thread is in the waiting state, the function returns when the system queues an I/O completion routine or APC, and the thread runs the routine or function.
 * @param Timeout A pointer to an absolute or relative time over which the wait is to occur. Can be null. If a timeout is specified,
 * and the object has not attained a state of signaled when the timeout expires, then the wait is automatically satisfied.
 * If an explicit timeout value of zero is specified, then no wait occurs if the wait cannot be satisfied immediately.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitformultipleobjectsex
 */
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

#if (PHNT_VERSION >= PHNT_WINDOWS_SERVER_2003)
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
#endif // (PHNT_VERSION >= PHNT_WINDOWS_SERVER_2003)

/**
 * The NtSetSecurityObject routine sets an object's security state.
 *
 * @param Handle Handle for the object whose security state is to be set.
 * @param SecurityInformation A SECURITY_INFORMATION value specifying the information to be set.
 * @param SecurityDescriptor Pointer to the security descriptor to be set for the object.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-zwsetsecurityobject
 */
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
 * @param Handle Handle for the object whose security descriptor is to be queried. 
 * @param SecurityInformation A SECURITY_INFORMATION value specifying the information to be queried.
 * @param SecurityDescriptor Caller-allocated buffer that NtQuerySecurityObject fills with a copy of the specified security descriptor.
 * @param Length Size, in bytes, of the buffer pointed to by SecurityDescriptor.
 * @param LengthNeeded Pointer to a caller-allocated variable that receives the number of bytes required to store the copied security descriptor.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntquerysecurityobject
 */
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
 * @param Handle The handle being closed.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwclose
 */
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
 * @param FirstObjectHandle The first object handle to compare.
 * @param SecondObjectHandle The second object handle to compare.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-compareobjecthandles
 */
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
 * @param DirectoryHandle Pointer to a HANDLE variable that receives a handle to the object directory.
 * @param DesiredAccess An ACCESS_MASK that specifies the requested access to the directory object.
 * @param ObjectAttributes The attributes for the directory object.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwcreatedirectoryobject
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateDirectoryObject(
    _Out_ PHANDLE DirectoryHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateDirectoryObjectEx(
    _Out_ PHANDLE DirectoryHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ShadowDirectoryHandle,
    _In_ ULONG Flags
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

/**
 * Opens an existing directory object.
 * 
 * @param DirectoryHandle A handle to the newly opened directory object.
 * @param DesiredAccess An ACCESS_MASK that specifies the requested access to the directory object.
 * @param ObjectAttributes The attributes for the directory object.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/devnotes/ntopendirectoryobject
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenDirectoryObject(
    _Out_ PHANDLE DirectoryHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * The OBJECT_DIRECTORY_INFORMATION structure contains information about the directory object.
 */
typedef struct _OBJECT_DIRECTORY_INFORMATION
{
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

/**
 * Retrieves information about the specified directory object.
 * 
 * @param DirectoryHandle A handle to the directory object. This handle must have been opened with the appropriate access rights.
 * @param Buffer A pointer to a buffer that receives the directory information.
 * @param Length The size, in bytes, of the buffer pointed to by the Buffer parameter.
 * @param ReturnSingleEntry A BOOLEAN value that specifies whether to return a single entry or multiple entries.
 * @param RestartScan A BOOLEAN value that specifies whether to restart the scan from the beginning of the directory.
 * @param Context A pointer to a variable that maintains the context of the directory enumeration.
 * @param ReturnLength An optional pointer to a variable that receives the number of bytes returned in the buffer.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/devnotes/ntquerydirectoryobject
 */
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

// private
typedef enum _BOUNDARY_ENTRY_TYPE
{
    OBNS_Invalid,
    OBNS_Name,
    OBNS_SID,
    OBNS_IL
} BOUNDARY_ENTRY_TYPE;

// private
typedef struct _OBJECT_BOUNDARY_ENTRY
{
    BOUNDARY_ENTRY_TYPE EntryType;
    ULONG EntrySize;
    //union
    //{
    //    WCHAR Name[1];
    //    PSID Sid;
    //    PSID IntegrityLabel;
    //};
} OBJECT_BOUNDARY_ENTRY, *POBJECT_BOUNDARY_ENTRY;

// rev
#define OBJECT_BOUNDARY_DESCRIPTOR_VERSION 1

// private
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

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)

/**
 * Creates a private namespace.
 *
 * @param NamespaceHandle A handle to the newly created private namespace.
 * @param DesiredAccess An ACCESS_MASK that specifies the requested access to the private namespace.
 * @param ObjectAttributes The attributes for the private namespace.
 * @param BoundaryDescriptor A descriptor that defines how the namespace is to be isolated. The RtlCreateBoundaryDescriptor function creates a boundary descriptor.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createprivatenamespacea
 */
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
 * @param NamespaceHandle A handle to the newly opened private namespace.
 * @param DesiredAccess An ACCESS_MASK that specifies the requested access to the private namespace.
 * @param ObjectAttributes The attributes for the private namespace.
 * @param BoundaryDescriptor A descriptor that defines how the namespace is to be isolated. The RtlCreateBoundaryDescriptor function creates a boundary descriptor.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-openprivatenamespacea
 */
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
 * Deletes an open namespace handle.
 *
 * @param NamespaceHandle A handle to the private namespace.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/namespaceapi/nf-namespaceapi-closeprivatenamespace
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeletePrivateNamespace(
    _In_ HANDLE NamespaceHandle
    );

#endif // (PHNT_VERSION >= PHNT_WINDOWS_VISTA)

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

//
// Symbolic links
//

#if (PHNT_MODE != PHNT_MODE_KERNEL)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateSymbolicLinkObject(
    _Out_ PHANDLE LinkHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ PCUNICODE_STRING LinkTarget
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSymbolicLinkObject(
    _Out_ PHANDLE LinkHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySymbolicLinkObject(
    _In_ HANDLE LinkHandle,
    _Inout_ PUNICODE_STRING LinkTarget,
    _Out_opt_ PULONG ReturnedLength
    );

typedef enum _SYMBOLIC_LINK_INFO_CLASS
{
    SymbolicLinkGlobalInformation = 1, // s: ULONG
    SymbolicLinkAccessMask, // s: ACCESS_MASK
    MaxnSymbolicLinkInfoClass
} SYMBOLIC_LINK_INFO_CLASS;

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
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
