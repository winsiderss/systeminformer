#ifndef _NTOBAPI_H
#define _NTOBAPI_H

#if (PHNT_MODE != PHNT_MODE_KERNEL)
#define OBJECT_TYPE_CREATE 0x0001
#define OBJECT_TYPE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)
#endif

#if (PHNT_MODE != PHNT_MODE_KERNEL)
#define DIRECTORY_QUERY 0x0001
#define DIRECTORY_TRAVERSE 0x0002
#define DIRECTORY_CREATE_OBJECT 0x0004
#define DIRECTORY_CREATE_SUBDIRECTORY 0x0008
#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xf)
#endif

#if (PHNT_MODE != PHNT_MODE_KERNEL)
#define SYMBOLIC_LINK_QUERY 0x0001
#define SYMBOLIC_LINK_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)
#endif

#define OBJ_PROTECT_CLOSE 0x00000001
#ifndef OBJ_INHERIT
#define OBJ_INHERIT 0x00000002
#endif
#define OBJ_AUDIT_OBJECT_CLOSE 0x00000004

#if (PHNT_MODE != PHNT_MODE_KERNEL)
typedef enum _OBJECT_INFORMATION_CLASS
{
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectTypesInformation,
    ObjectHandleFlagInformation,
    ObjectSessionInformation,
    MaxObjectInfoClass
} OBJECT_INFORMATION_CLASS;
#else
#define ObjectNameInformation 1
#define ObjectTypesInformation 3
#define ObjectHandleFlagInformation 4
#define ObjectSessionInformation 5
#endif

typedef struct _OBJECT_BASIC_INFORMATION
{
    ULONG Attributes;
    ACCESS_MASK GrantedAccess;
    ULONG HandleCount;
    ULONG PointerCount;
    ULONG PagedPoolCharge;
    ULONG NonPagedPoolCharge;
    ULONG Reserved[3];
    ULONG NameInfoSize;
    ULONG TypeInfoSize;
    ULONG SecurityDescriptorSize;
    LARGE_INTEGER CreationTime;
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

#if (PHNT_MODE != PHNT_MODE_KERNEL)
typedef struct _OBJECT_NAME_INFORMATION
{
    UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;
#endif

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
    ULONG PoolType;
    ULONG DefaultPagedPoolCharge;
    ULONG DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

typedef struct _OBJECT_TYPES_INFORMATION
{
    ULONG NumberOfTypes;
    OBJECT_TYPE_INFORMATION TypeInformation[1];
} OBJECT_TYPES_INFORMATION, *POBJECT_TYPES_INFORMATION;

typedef struct _OBJECT_HANDLE_FLAG_INFORMATION
{
    BOOLEAN Inherit;
    BOOLEAN ProtectFromClose;
} OBJECT_HANDLE_FLAG_INFORMATION, *POBJECT_HANDLE_FLAG_INFORMATION;

// Objects, handles

#if (PHNT_MODE != PHNT_MODE_KERNEL)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryObject(
    __in HANDLE Handle,
    __in OBJECT_INFORMATION_CLASS ObjectInformationClass,
    __out_bcount_opt(ObjectInformationLength) PVOID ObjectInformation,
    __in ULONG ObjectInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationObject(
    __in HANDLE Handle,
    __in OBJECT_INFORMATION_CLASS ObjectInformationClass,
    __in_bcount(ObjectInformationLength) PVOID ObjectInformation,
    __in ULONG ObjectInformationLength
    );

#define DUPLICATE_CLOSE_SOURCE 0x00000001
#define DUPLICATE_SAME_ACCESS 0x00000002
#define DUPLICATE_SAME_ATTRIBUTES 0x00000004

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDuplicateObject(
    __in HANDLE SourceProcessHandle,
    __in HANDLE SourceHandle,
    __in_opt HANDLE TargetProcessHandle,
    __out_opt PHANDLE TargetHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Options
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtMakeTemporaryObject(
    __in HANDLE Handle
    );

typedef NTSTATUS (NTAPI *_NtMakePermanentObject)(
    __in HANDLE Handle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSignalAndWaitForSingleObject(
    __in HANDLE SignalHandle,
    __in HANDLE WaitHandle,
    __in BOOLEAN Alertable,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForSingleObject(
    __in HANDLE Handle,
    __in BOOLEAN Alertable,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForMultipleObjects(
    __in ULONG Count,
    __in_ecount(Count) PHANDLE Handles,
    __in WAIT_TYPE WaitType,
    __in BOOLEAN Alertable,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSecurityObject(
    __in HANDLE Handle,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySecurityObject(
    __in HANDLE Handle,
    __in SECURITY_INFORMATION SecurityInformation,
    __out_bcount_opt(Length) PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in ULONG Length,
    __out PULONG LengthNeeded
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtClose(
    __in HANDLE Handle
    );

#endif

// Directory objects

#if (PHNT_MODE != PHNT_MODE_KERNEL)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateDirectoryObject(
    __out PHANDLE DirectoryHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenDirectoryObject(
    __out PHANDLE DirectoryHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

typedef struct _OBJECT_DIRECTORY_INFORMATION
{
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDirectoryObject(
    __in HANDLE DirectoryHandle,
    __out_bcount_opt(BufferLength) PVOID Buffer,
    __in ULONG Length,
    __in BOOLEAN ReturnSingleEntry,
    __in BOOLEAN RestartScan,
    __inout PULONG Context,
    __out_opt PULONG ReturnLength
    );

#endif

// Private namespaces

#if (PHNT_MODE != PHNT_MODE_KERNEL)

#if (PHNT_VERSION >= PHNT_VISTA)

// begin_private

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreatePrivateNamespace(
    __out PHANDLE NamespaceHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in PVOID BoundaryDescriptor
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenPrivateNamespace(
    __out PHANDLE NamespaceHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in PVOID BoundaryDescriptor
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeletePrivateNamespace(
    __in HANDLE NamespaceHandle
    );

// end_private

#endif

#endif

// Symbolic links

#if (PHNT_MODE != PHNT_MODE_KERNEL)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateSymbolicLinkObject(
    __out PHANDLE LinkHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in PUNICODE_STRING LinkTarget
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSymbolicLinkObject(
    __out PHANDLE LinkHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySymbolicLinkObject(
    __in HANDLE LinkHandle,
    __inout PUNICODE_STRING LinkTarget,
    __out_opt PULONG ReturnedLength
    );

#endif

#endif
