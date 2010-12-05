#ifndef _NTZWAPI_H
#define _NTZWAPI_H

// This file was automatically generated. Do not edit.

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAcceptConnectPort(
    __out PHANDLE PortHandle,
    __in_opt PVOID PortContext,
    __in PPORT_MESSAGE ConnectionRequest,
    __in BOOLEAN AcceptConnection,
    __inout_opt PPORT_VIEW ServerView,
    __out_opt PREMOTE_PORT_VIEW ClientView
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAccessCheck(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in HANDLE ClientToken,
    __in ACCESS_MASK DesiredAccess,
    __in PGENERIC_MAPPING GenericMapping,
    __out_bcount(*PrivilegeSetLength) PPRIVILEGE_SET PrivilegeSet,
    __inout PULONG PrivilegeSetLength,
    __out PACCESS_MASK GrantedAccess,
    __out PNTSTATUS AccessStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAccessCheckAndAuditAlarm(
    __in PUNICODE_STRING SubsystemName,
    __in_opt PVOID HandleId,
    __in PUNICODE_STRING ObjectTypeName,
    __in PUNICODE_STRING ObjectName,
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in ACCESS_MASK DesiredAccess,
    __in PGENERIC_MAPPING GenericMapping,
    __in BOOLEAN ObjectCreation,
    __out PACCESS_MASK GrantedAccess,
    __out PNTSTATUS AccessStatus,
    __out PBOOLEAN GenerateOnClose
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAccessCheckByType(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in_opt PSID PrincipalSelfSid,
    __in HANDLE ClientToken,
    __in ACCESS_MASK DesiredAccess,
    __in_ecount(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    __in ULONG ObjectTypeListLength,
    __in PGENERIC_MAPPING GenericMapping,
    __out_bcount(*PrivilegeSetLength) PPRIVILEGE_SET PrivilegeSet,
    __inout PULONG PrivilegeSetLength,
    __out PACCESS_MASK GrantedAccess,
    __out PNTSTATUS AccessStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAccessCheckByTypeAndAuditAlarm(
    __in PUNICODE_STRING SubsystemName,
    __in_opt PVOID HandleId,
    __in PUNICODE_STRING ObjectTypeName,
    __in PUNICODE_STRING ObjectName,
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in_opt PSID PrincipalSelfSid,
    __in ACCESS_MASK DesiredAccess,
    __in AUDIT_EVENT_TYPE AuditType,
    __in ULONG Flags,
    __in_ecount_opt(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    __in ULONG ObjectTypeListLength,
    __in PGENERIC_MAPPING GenericMapping,
    __in BOOLEAN ObjectCreation,
    __out PACCESS_MASK GrantedAccess,
    __out PNTSTATUS AccessStatus,
    __out PBOOLEAN GenerateOnClose
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAccessCheckByTypeResultList(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in_opt PSID PrincipalSelfSid,
    __in HANDLE ClientToken,
    __in ACCESS_MASK DesiredAccess,
    __in_ecount(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    __in ULONG ObjectTypeListLength,
    __in PGENERIC_MAPPING GenericMapping,
    __out_bcount(*PrivilegeSetLength) PPRIVILEGE_SET PrivilegeSet,
    __inout PULONG PrivilegeSetLength,
    __out_ecount(ObjectTypeListLength) PACCESS_MASK GrantedAccess,
    __out_ecount(ObjectTypeListLength) PNTSTATUS AccessStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAccessCheckByTypeResultListAndAuditAlarm(
    __in PUNICODE_STRING SubsystemName,
    __in_opt PVOID HandleId,
    __in PUNICODE_STRING ObjectTypeName,
    __in PUNICODE_STRING ObjectName,
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in_opt PSID PrincipalSelfSid,
    __in ACCESS_MASK DesiredAccess,
    __in AUDIT_EVENT_TYPE AuditType,
    __in ULONG Flags,
    __in_ecount_opt(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    __in ULONG ObjectTypeListLength,
    __in PGENERIC_MAPPING GenericMapping,
    __in BOOLEAN ObjectCreation,
    __out_ecount(ObjectTypeListLength) PACCESS_MASK GrantedAccess,
    __out_ecount(ObjectTypeListLength) PNTSTATUS AccessStatus,
    __out PBOOLEAN GenerateOnClose
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAccessCheckByTypeResultListAndAuditAlarmByHandle(
    __in PUNICODE_STRING SubsystemName,
    __in_opt PVOID HandleId,
    __in HANDLE ClientToken,
    __in PUNICODE_STRING ObjectTypeName,
    __in PUNICODE_STRING ObjectName,
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in_opt PSID PrincipalSelfSid,
    __in ACCESS_MASK DesiredAccess,
    __in AUDIT_EVENT_TYPE AuditType,
    __in ULONG Flags,
    __in_ecount_opt(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    __in ULONG ObjectTypeListLength,
    __in PGENERIC_MAPPING GenericMapping,
    __in BOOLEAN ObjectCreation,
    __out_ecount(ObjectTypeListLength) PACCESS_MASK GrantedAccess,
    __out_ecount(ObjectTypeListLength) PNTSTATUS AccessStatus,
    __out PBOOLEAN GenerateOnClose
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAddAtom(
    __in_bcount_opt(Length) PWSTR AtomName,
    __in ULONG Length,
    __out_opt PRTL_ATOM Atom
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAdjustGroupsToken(
    __in HANDLE TokenHandle,
    __in BOOLEAN ResetToDefault,
    __in_opt PTOKEN_GROUPS NewState,
    __in_opt ULONG BufferLength,
    __out_bcount_part_opt(BufferLength, *ReturnLength) PTOKEN_GROUPS PreviousState,
    __out PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAdjustPrivilegesToken(
    __in HANDLE TokenHandle,
    __in BOOLEAN DisableAllPrivileges,
    __in_opt PTOKEN_PRIVILEGES NewState,
    __in ULONG BufferLength,
    __out_bcount_part_opt(BufferLength, *ReturnLength) PTOKEN_PRIVILEGES PreviousState,
    __out __drv_when(PreviousState == NULL, __out_opt) PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlertResumeThread(
    __in HANDLE ThreadHandle,
    __out_opt PULONG PreviousSuspendCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlertThread(
    __in HANDLE ThreadHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAllocateLocallyUniqueId(
    __out PLUID Luid
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAllocateReserveObject(
    __out PHANDLE MemoryReserveHandle,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in RESERVE_OBJECT_TYPE Type
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAllocateUserPhysicalPages(
    __in HANDLE ProcessHandle,
    __inout PULONG_PTR NumberOfPages,
    __out_ecount(*NumberOfPages) PULONG_PTR UserPfnArray
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAllocateUuids(
    __out PULARGE_INTEGER Time,
    __out PULONG Range,
    __out PULONG Sequence,
    __out PCHAR Seed
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAllocateVirtualMemory(
    __in HANDLE ProcessHandle,
    __inout PVOID *BaseAddress,
    __in ULONG_PTR ZeroBits,
    __inout PSIZE_T RegionSize,
    __in ULONG AllocationType,
    __in ULONG Protect
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcAcceptConnectPort(
    __out PHANDLE ServerPortHandle,
    __in HANDLE PortHandle,
    __in ULONG Flags,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in PALPC_PORT_ATTRIBUTES PortAttributes,
    __in_opt PVOID PortContext,
    __in PPORT_MESSAGE ConnectMessage,
    __inout_opt PALPC_MESSAGE_ATTRIBUTES ReplyMessageAttributes,
    __in BOOLEAN AcceptConnection
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcCancelMessage(
    __in HANDLE PortHandle,
    __in ULONG Flags,
    __in PALPC_CONTEXT_ATTRIBUTES ContextAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcConnectPort(
    __out PHANDLE ClientPortHandle,
    __in PUNICODE_STRING PortName,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PALPC_PORT_ATTRIBUTES PortAttributes,
    __in ULONG Flags,
    __in_opt PSID RequiredServerSid,
    __inout PPORT_MESSAGE ConnectMessage,
    __inout_opt PULONG ReceiveMessageLength,
    __inout_opt PALPC_MESSAGE_ATTRIBUTES ConnectMessageAttributes,
    __inout_opt PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,
    __in_opt PLARGE_INTEGER ReceiveTimeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcCreatePort(
    __out PHANDLE PortHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PALPC_PORT_ATTRIBUTES PortAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcCreatePortSection(
    __in HANDLE PortHandle,
    __in ULONG Flags,
    __in_opt HANDLE SectionHandle,
    __in SIZE_T SectionSize,
    __out PALPC_HANDLE AlpcSectionHandle,
    __out PSIZE_T AlpcSectionSize
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcCreateResourceReserve(
    __in HANDLE PortHandle,
    __in ULONG Flags, // reserved
    __in SIZE_T MessageReserveSize,
    __out PALPC_HANDLE AlpcReserveHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcCreateSectionView(
    __in HANDLE PortHandle,
    __in ULONG Flags, // reserved
    __inout PALPC_VIEW_ATTRIBUTES ViewAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcCreateSecurityContext(
    __in HANDLE PortHandle,
    __in ULONG Flags, // reserved
    __inout PALPC_SECURITY_ATTRIBUTES SecurityAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcDeletePortSection(
    __in HANDLE PortHandle,
    __in ULONG Flags, // reserved
    __in ALPC_HANDLE AlpcSectionHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcDeleteResourceReserve(
    __in HANDLE PortHandle,
    __in ULONG Flags, // reserved
    __in ALPC_HANDLE AlpcReserveHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcDeleteSectionView(
    __in HANDLE PortHandle,
    __in ULONG Flags, // reserved
    __in PVOID ViewBase
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcDeleteSecurityContext(
    __in HANDLE PortHandle,
    __in ULONG Flags, // reserved
    __in ALPC_HANDLE AlpcSecurityHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcDisconnectPort(
    __in HANDLE PortHandle,
    __in ULONG Flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcImpersonateClientOfPort(
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE Message,
    __in ULONG Flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcOpenSenderProcess(
    __out PHANDLE ProcessHandle,
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE Message,
    __in ULONG Flags,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcOpenSenderThread(
    __out PHANDLE ThreadHandle,
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE Message,
    __in ULONG Flags,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcQueryInformation(
    __in HANDLE PortHandle,
    __in ALPC_PORT_INFORMATION_CLASS PortInformationClass,
    __out_bcount(PortInformationLength) PVOID PortInformation,
    __in ULONG PortInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcQueryInformationMessage(
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE Message,
    __in ALPC_MESSAGE_INFORMATION_CLASS MessageInformationClass,
    __out_bcount(MessageInformationLength) PVOID MessageInformation,
    __in ULONG MessageInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcRevokeSecurityContext(
    __in HANDLE PortHandle,
    __in ULONG Flags, // reserved
    __in ALPC_HANDLE AlpcSecurityHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcSendWaitReceivePort(
    __in HANDLE PortHandle,
    __in ULONG Flags,
    __in_opt PPORT_MESSAGE SendMessage,
    __in_opt PALPC_MESSAGE_ATTRIBUTES SendMessageAttributes,
    __inout_opt PPORT_MESSAGE ReceiveMessage,
    __inout_opt PULONG ReceiveMessageLength,
    __inout_opt PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,
    __in_opt PLARGE_INTEGER ReceiveTimeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcSetInformation(
    __in HANDLE PortHandle,
    __in ALPC_PORT_INFORMATION_CLASS PortInformationClass,
    __in_bcount(PortInformationLength) PVOID PortInformation,
    __in ULONG PortInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAreMappedFilesTheSame(
    __in PVOID File1MappedAsAnImage,
    __in PVOID File2MappedAsFile
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAssignProcessToJobObject(
    __in HANDLE JobHandle,
    __in HANDLE ProcessHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCallbackReturn(
    __in_bcount_opt(OutputLength) PVOID OutputBuffer,
    __in ULONG OutputLength,
    __in NTSTATUS Status
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCancelIoFile(
    __in HANDLE FileHandle,
    __out PIO_STATUS_BLOCK IoStatusBlock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCancelIoFileEx(
    __in HANDLE FileHandle,
    __in_opt PIO_STATUS_BLOCK UserIosb,
    __out PIO_STATUS_BLOCK IoStatusBlock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCancelSynchronousIoFile(
    __in HANDLE ThreadHandle,
    __in_opt PIO_STATUS_BLOCK UserIosb,
    __out PIO_STATUS_BLOCK IoStatusBlock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCancelTimer(
    __in HANDLE TimerHandle,
    __out_opt PBOOLEAN CurrentState
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwClearEvent(
    __in HANDLE EventHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwClose(
    __in HANDLE Handle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCloseObjectAuditAlarm(
    __in PUNICODE_STRING SubsystemName,
    __in_opt PVOID HandleId,
    __in BOOLEAN GenerateOnClose
    );

NTSYSCALLAPI
NTSTATUS
NTAPI 
ZwCommitComplete(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCommitEnlistment(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCommitTransaction(
    __in HANDLE TransactionHandle,
    __in BOOLEAN Wait
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCompactKeys(
    __in ULONG Count,
    __in_ecount(Count) HANDLE KeyArray[]
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCompareTokens(
    __in HANDLE FirstTokenHandle,
    __in HANDLE SecondTokenHandle,
    __out PBOOLEAN Equal
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCompleteConnectPort(
    __in HANDLE PortHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCompressKey(
    __in HANDLE Key
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwConnectPort(
    __out PHANDLE PortHandle,
    __in PUNICODE_STRING PortName,
    __in PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    __inout_opt PPORT_VIEW ClientView,
    __inout_opt PREMOTE_PORT_VIEW ServerView,
    __out_opt PULONG MaxMessageLength,
    __inout_opt PVOID ConnectionInformation,
    __inout_opt PULONG ConnectionInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwContinue(
    __in PCONTEXT ContextRecord,
    __in BOOLEAN TestAlert
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateDebugObject(
    __out PHANDLE DebugObjectHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in ULONG Flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateDirectoryObject(
    __out PHANDLE DirectoryHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateEnlistment(
    __out PHANDLE EnlistmentHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ResourceManagerHandle,
    __in HANDLE TransactionHandle,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt ULONG CreateOptions,
    __in NOTIFICATION_MASK NotificationMask,
    __in_opt PVOID EnlistmentKey
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateEvent(
    __out PHANDLE EventHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in EVENT_TYPE EventType,
    __in BOOLEAN InitialState
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateEventPair(
    __out PHANDLE EventPairHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateFile(
    __out PHANDLE FileHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in_opt PLARGE_INTEGER AllocationSize,
    __in ULONG FileAttributes,
    __in ULONG ShareAccess,
    __in ULONG CreateDisposition,
    __in ULONG CreateOptions,
    __in_bcount_opt(EaLength) PVOID EaBuffer,
    __in ULONG EaLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateIoCompletion(
    __out PHANDLE IoCompletionHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt ULONG Count
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateJobObject(
    __out PHANDLE JobHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateJobSet(
    __in ULONG NumJob,
    __in_ecount(NumJob) PJOB_SET_ARRAY UserJobSet,
    __in ULONG Flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateKey(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __reserved ULONG TitleIndex,
    __in_opt PUNICODE_STRING Class,
    __in ULONG CreateOptions,
    __out_opt PULONG Disposition
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateKeyTransacted(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __reserved ULONG TitleIndex,
    __in_opt PUNICODE_STRING Class,
    __in ULONG CreateOptions,
    __in HANDLE TransactionHandle,
    __out_opt PULONG Disposition
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateKeyedEvent(
    __out PHANDLE KeyedEventHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in ULONG Flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateMailslotFile(
    __out PHANDLE FileHandle,
    __in ULONG DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in ULONG CreateOptions,
    __in ULONG MailslotQuota,
    __in ULONG MaximumMessageSize,
    __in PLARGE_INTEGER ReadTimeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateMutant(
    __out PHANDLE MutantHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in BOOLEAN InitialOwner
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateNamedPipeFile(
    __out PHANDLE FileHandle,
    __in ULONG DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in ULONG ShareAccess,
    __in ULONG CreateDisposition,
    __in ULONG CreateOptions,
    __in ULONG NamedPipeType,
    __in ULONG ReadMode,
    __in ULONG CompletionMode,
    __in ULONG MaximumInstances,
    __in ULONG InboundQuota,
    __in ULONG OutboundQuota,
    __in_opt PLARGE_INTEGER DefaultTimeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreatePagingFile(
    __in PUNICODE_STRING PageFileName,
    __in PLARGE_INTEGER MinimumSize,
    __in PLARGE_INTEGER MaximumSize,
    __in ULONG Priority
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreatePort(
    __out PHANDLE PortHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in ULONG MaxConnectionInfoLength,
    __in ULONG MaxMessageLength,
    __in_opt ULONG MaxPoolUsage
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreatePrivateNamespace(
    __out PHANDLE DirectoryHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in PVOID BoundaryDescriptor
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE ParentProcess,
    __in BOOLEAN InheritObjectTable,
    __in_opt HANDLE SectionHandle,
    __in_opt HANDLE DebugPort,
    __in_opt HANDLE ExceptionPort
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateProcessEx(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE ParentProcess,
    __in ULONG Flags,
    __in_opt HANDLE SectionHandle,
    __in_opt HANDLE DebugPort,
    __in_opt HANDLE ExceptionPort,
    __in ULONG JobMemberLevel
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateProfile(
    __out PHANDLE ProfileHandle,
    __in_opt HANDLE Process,
    __in PVOID ProfileBase,
    __in SIZE_T ProfileSize,
    __in ULONG BucketSize,
    __in PULONG Buffer,
    __in ULONG BufferSize,
    __in KPROFILE_SOURCE ProfileSource,
    __in KAFFINITY Affinity
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateProfileEx(
    __out PHANDLE ProfileHandle,
    __in_opt HANDLE Process,
    __in PVOID ProfileBase,
    __in SIZE_T ProfileSize,
    __in ULONG BucketSize,
    __in PULONG Buffer,
    __in ULONG BufferSize,
    __in KPROFILE_SOURCE ProfileSource,
    __in ULONG GroupAffinityCount,
    __in_opt PGROUP_AFFINITY GroupAffinity
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateResourceManager(
    __out PHANDLE ResourceManagerHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE TmHandle,
    __in LPGUID RmGuid,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt ULONG CreateOptions,
    __in_opt PUNICODE_STRING Description
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateSection(
    __out PHANDLE SectionHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PLARGE_INTEGER MaximumSize,
    __in ULONG SectionPageProtection,
    __in ULONG AllocationAttributes,
    __in_opt HANDLE FileHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateSemaphore(
    __out PHANDLE SemaphoreHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in LONG InitialCount,
    __in LONG MaximumCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateSymbolicLinkObject(
    __out PHANDLE LinkHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in PUNICODE_STRING LinkTarget
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateThread(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE ProcessHandle,
    __out PCLIENT_ID ClientId,
    __in PCONTEXT ThreadContext,
    __in PINITIAL_TEB InitialTeb,
    __in BOOLEAN CreateSuspended
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateThreadEx(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE ProcessHandle,
    __in PVOID StartRoutine,
    __in_opt PVOID StartContext,
    __in ULONG Flags, // THREAD_CREATE_FLAGS_*
    __in_opt ULONG_PTR StackZeroBits,
    __in_opt SIZE_T StackCommit,
    __in_opt SIZE_T StackReserve,
    __in_opt PPROCESS_ATTRIBUTE_LIST AttributeList
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateTimer(
    __out PHANDLE TimerHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in TIMER_TYPE TimerType
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateToken(
    __out PHANDLE TokenHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in TOKEN_TYPE TokenType,
    __in PLUID AuthenticationId,
    __in PLARGE_INTEGER ExpirationTime,
    __in PTOKEN_USER User,
    __in PTOKEN_GROUPS Groups,
    __in PTOKEN_PRIVILEGES Privileges,
    __in_opt PTOKEN_OWNER Owner,
    __in PTOKEN_PRIMARY_GROUP PrimaryGroup,
    __in_opt PTOKEN_DEFAULT_DACL DefaultDacl,
    __in PTOKEN_SOURCE TokenSource
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateTransaction(
    __out PHANDLE TransactionHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt LPGUID Uow,
    __in_opt HANDLE TmHandle,
    __in_opt ULONG CreateOptions,
    __in_opt ULONG IsolationLevel,
    __in_opt ULONG IsolationFlags,
    __in_opt PLARGE_INTEGER Timeout,
    __in_opt PUNICODE_STRING Description
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateTransactionManager(
    __out PHANDLE TmHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PUNICODE_STRING LogFileName,
    __in_opt ULONG CreateOptions,
    __in_opt ULONG CommitStrength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateUserProcess(
    __out PHANDLE ProcessHandle,
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK ProcessDesiredAccess,
    __in ACCESS_MASK ThreadDesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ProcessObjectAttributes,
    __in_opt POBJECT_ATTRIBUTES ThreadObjectAttributes,
    __in ULONG ProcessFlags, // PROCESS_CREATE_FLAGS_*
    __in ULONG ThreadFlags, // THREAD_CREATE_FLAGS_*
    __in_opt PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
    __inout PPROCESS_CREATE_INFO CreateInfo,
    __in_opt PPROCESS_ATTRIBUTE_LIST AttributeList
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateWaitablePort(
    __out PHANDLE PortHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in ULONG MaxConnectionInfoLength,
    __in ULONG MaxMessageLength,
    __in_opt ULONG MaxPoolUsage
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCreateWorkerFactory(
    __out PHANDLE WorkerFactoryHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE IoCompletionHandle,
    __in HANDLE ProcessHandle,
    __in PVOID WorkerThreadStart,
    __in_opt PVOID WorkerThreadContext,
    __in_opt ULONG MaximumCount,
    __in_opt SIZE_T SizeOfStackReserve,
    __in_opt SIZE_T SizeOfStackCommit
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwDebugActiveProcess(
    __in HANDLE ProcessHandle,
    __in HANDLE DebugObjectHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwDebugContinue(
    __in HANDLE DebugObjectHandle,
    __in PCLIENT_ID ClientId,
    __in NTSTATUS ContinueStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwDelayExecution(
    __in BOOLEAN Alertable,
    __in PLARGE_INTEGER DelayInterval
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwDeleteAtom(
    __in RTL_ATOM Atom
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwDeleteFile(
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwDeleteKey(
    __in HANDLE KeyHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwDeleteObjectAuditAlarm(
    __in PUNICODE_STRING SubsystemName,
    __in_opt PVOID HandleId,
    __in BOOLEAN GenerateOnClose
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwDeletePrivateNamespace(
    __in HANDLE DirectoryHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwDeleteValueKey(
    __in HANDLE KeyHandle,
    __in PUNICODE_STRING ValueName
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwDeviceIoControlFile(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in ULONG IoControlCode,
    __in_bcount_opt(InputBufferLength) PVOID InputBuffer,
    __in ULONG InputBufferLength,
    __out_bcount_opt(OutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwDisableLastKnownGood(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwDisplayString(
    __in PUNICODE_STRING String
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwDrawText(
    __in PUNICODE_STRING Text
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwDuplicateObject(
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
ZwDuplicateToken(
    __in HANDLE ExistingTokenHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in BOOLEAN EffectiveOnly,
    __in TOKEN_TYPE TokenType,
    __out PHANDLE NewTokenHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwEnableLastKnownGood(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwEnumerateKey(
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
ZwEnumerateTransactionObject(
    __in_opt HANDLE RootObjectHandle,
    __in KTMOBJECT_TYPE QueryType,
    __inout_bcount(ObjectCursorLength) PKTMOBJECT_CURSOR ObjectCursor,
    __in ULONG ObjectCursorLength,
    __out PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwEnumerateValueKey(
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
ZwExtendSection(
    __in HANDLE SectionHandle,
    __inout PLARGE_INTEGER NewSectionSize
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwFilterToken(
    __in HANDLE ExistingTokenHandle,
    __in ULONG Flags,
    __in_opt PTOKEN_GROUPS SidsToDisable,
    __in_opt PTOKEN_PRIVILEGES PrivilegesToDelete,
    __in_opt PTOKEN_GROUPS RestrictedSids,
    __out PHANDLE NewTokenHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwFindAtom(
    __in_bcount_opt(Length) PWSTR AtomName,
    __in ULONG Length,
    __out_opt PRTL_ATOM Atom
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwFlushBuffersFile(
    __in HANDLE FileHandle,
    __out PIO_STATUS_BLOCK IoStatusBlock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwFlushInstructionCache(
    __in HANDLE ProcessHandle,
    __in_opt PVOID BaseAddress,
    __in SIZE_T Length
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwFlushKey(
    __in HANDLE KeyHandle
    );

NTSYSCALLAPI
VOID
NTAPI
ZwFlushProcessWriteBuffers(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwFlushWriteBuffer(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwFreeUserPhysicalPages(
    __in HANDLE ProcessHandle,
    __inout PULONG_PTR NumberOfPages,
    __in_ecount(*NumberOfPages) PULONG_PTR UserPfnArray
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwFreeVirtualMemory(
    __in HANDLE ProcessHandle,
    __inout PVOID *BaseAddress,
    __inout PSIZE_T RegionSize,
    __in ULONG FreeType
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwFreezeRegistry(
    __in ULONG ThawTimeoutInSeconds
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwFreezeTransactions(
    __in PLARGE_INTEGER Timeout,
    __in PLARGE_INTEGER ThawTimeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwFsControlFile(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in ULONG FsControlCode,
    __in_bcount_opt(InputBufferLength) PVOID InputBuffer,
    __in ULONG InputBufferLength,
    __out_bcount_opt(OutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwGetContextThread(
    __in HANDLE ThreadHandle,
    __inout PCONTEXT ThreadContext
    );

NTSYSCALLAPI
ULONG
NTAPI
ZwGetCurrentProcessorNumber(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwGetDevicePowerState(
    __in HANDLE Device,
    __out PDEVICE_POWER_STATE State
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwGetNextProcess(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Flags,
    __out PHANDLE NewProcessHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwGetNextThread(
    __in HANDLE ProcessHandle,
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Flags,
    __out PHANDLE NewThreadHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwGetNotificationResourceManager(
    __in HANDLE ResourceManagerHandle,
    __out PTRANSACTION_NOTIFICATION TransactionNotification,
    __in ULONG NotificationLength,
    __in_opt PLARGE_INTEGER Timeout,
    __out_opt PULONG ReturnLength,
    __in ULONG Asynchronous,
    __in_opt ULONG_PTR AsynchronousContext
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwGetPlugPlayEvent(
    __in HANDLE EventHandle,
    __in_opt PVOID Context,
    __out_bcount(EventBufferSize) PPLUGPLAY_EVENT_BLOCK EventBlock,
    __in ULONG EventBufferSize
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwGetWriteWatch(
    __in HANDLE ProcessHandle,
    __in ULONG Flags,
    __in PVOID BaseAddress,
    __in SIZE_T RegionSize,
    __out_ecount(*EntriesInUserAddressArray) PVOID *UserAddressArray,
    __inout PULONG_PTR EntriesInUserAddressArray,
    __out PULONG Granularity
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwImpersonateAnonymousToken(
    __in HANDLE ThreadHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwImpersonateClientOfPort(
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE Message
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwImpersonateThread(
    __in HANDLE ServerThreadHandle,
    __in HANDLE ClientThreadHandle,
    __in PSECURITY_QUALITY_OF_SERVICE SecurityQos
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwInitializeRegistry(
    __in USHORT BootCondition
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwInitiatePowerAction(
    __in POWER_ACTION SystemAction,
    __in SYSTEM_POWER_STATE LightestSystemState,
    __in ULONG Flags, // POWER_ACTION_* flags
    __in BOOLEAN Asynchronous
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwIsProcessInJob(
    __in HANDLE ProcessHandle,
    __in_opt HANDLE JobHandle
    );

NTSYSCALLAPI
BOOLEAN
NTAPI
ZwIsSystemResumeAutomatic(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwListenPort(
    __in HANDLE PortHandle,
    __out PPORT_MESSAGE ConnectionRequest
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwLoadDriver(
    __in PUNICODE_STRING DriverServiceName
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwLoadKey(
    __in POBJECT_ATTRIBUTES TargetKey,
    __in POBJECT_ATTRIBUTES SourceFile
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwLoadKey2(
    __in POBJECT_ATTRIBUTES TargetKey,
    __in POBJECT_ATTRIBUTES SourceFile,
    __in ULONG Flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwLoadKeyEx(
    __in POBJECT_ATTRIBUTES TargetKey,
    __in POBJECT_ATTRIBUTES SourceFile,
    __in ULONG Flags,
    __in_opt HANDLE TrustClassKey 
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwLockFile(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in PLARGE_INTEGER ByteOffset,
    __in PLARGE_INTEGER Length,
    __in ULONG Key,
    __in BOOLEAN FailImmediately,
    __in BOOLEAN ExclusiveLock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwLockProductActivationKeys(
    __inout_opt ULONG *pPrivateVer,
    __out_opt ULONG *pSafeMode
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwLockRegistryKey(
    __in HANDLE KeyHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwLockVirtualMemory(
    __in HANDLE ProcessHandle,
    __inout PVOID *BaseAddress,
    __inout PSIZE_T RegionSize,
    __in ULONG MapType
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwMakeTemporaryObject(
    __in HANDLE Handle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwMapUserPhysicalPages(
    __in PVOID VirtualAddress,
    __in ULONG_PTR NumberOfPages,
    __in_ecount_opt(NumberOfPages) PULONG_PTR UserPfnArray
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwMapUserPhysicalPagesScatter(
    __in_ecount(NumberOfPages) PVOID *VirtualAddresses,
    __in ULONG_PTR NumberOfPages,
    __in_ecount_opt(NumberOfPages) PULONG_PTR UserPfnArray
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwMapViewOfSection(
    __in HANDLE SectionHandle,
    __in HANDLE ProcessHandle,
    __inout PVOID *BaseAddress,
    __in ULONG_PTR ZeroBits,
    __in SIZE_T CommitSize,
    __inout_opt PLARGE_INTEGER SectionOffset,
    __inout PSIZE_T ViewSize,
    __in SECTION_INHERIT InheritDisposition,
    __in ULONG AllocationType,
    __in ULONG Win32Protect
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwNotifyChangeDirectoryFile(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __out_bcount(Length) PVOID Buffer,
    __in ULONG Length,
    __in ULONG CompletionFilter,
    __in BOOLEAN WatchTree
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwNotifyChangeKey(
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
ZwNotifyChangeMultipleKeys(
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
ZwOpenDirectoryObject(
    __out PHANDLE DirectoryHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenEnlistment(
    __out PHANDLE EnlistmentHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ResourceManagerHandle,
    __in LPGUID EnlistmentGuid,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenEvent(
    __out PHANDLE EventHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenEventPair(
    __out PHANDLE EventPairHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenFile(
    __out PHANDLE FileHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in ULONG ShareAccess,
    __in ULONG OpenOptions
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenIoCompletion(
    __out PHANDLE IoCompletionHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenJobObject(
    __out PHANDLE JobHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenKey(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenKeyEx(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in ULONG OpenOptions
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenKeyTransacted(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE TransactionHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenKeyTransactedEx(
    __out PHANDLE KeyHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in ULONG OpenOptions,
    __in HANDLE TransactionHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenKeyedEvent(
    __out PHANDLE KeyedEventHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenMutant(
    __out PHANDLE MutantHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenObjectAuditAlarm(
    __in PUNICODE_STRING SubsystemName,
    __in_opt PVOID HandleId,
    __in PUNICODE_STRING ObjectTypeName,
    __in PUNICODE_STRING ObjectName,
    __in_opt PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in HANDLE ClientToken,
    __in ACCESS_MASK DesiredAccess,
    __in ACCESS_MASK GrantedAccess,
    __in_opt PPRIVILEGE_SET Privileges,
    __in BOOLEAN ObjectCreation,
    __in BOOLEAN AccessGranted,
    __out PBOOLEAN GenerateOnClose
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenPrivateNamespace(
    __out PHANDLE DirectoryHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in PVOID BoundaryDescriptor
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PCLIENT_ID ClientId
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenProcessToken(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE TokenHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenProcessTokenEx(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __out PHANDLE TokenHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenResourceManager(
    __out PHANDLE ResourceManagerHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE TmHandle,
    __in_opt LPGUID ResourceManagerGuid,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenSection(
    __out PHANDLE SectionHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenSemaphore(
    __out PHANDLE SemaphoreHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenSession(
    __out PHANDLE SessionHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenSymbolicLinkObject(
    __out PHANDLE LinkHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenThread(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PCLIENT_ID ClientId
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenThreadToken(
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in BOOLEAN OpenAsSelf,
    __out PHANDLE TokenHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenThreadTokenEx(
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in BOOLEAN OpenAsSelf,
    __in ULONG HandleAttributes,
    __out PHANDLE TokenHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenTimer(
    __out PHANDLE TimerHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenTransaction(
    __out PHANDLE TransactionHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in LPGUID Uow,
    __in_opt HANDLE TmHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwOpenTransactionManager(
    __out PHANDLE TmHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PUNICODE_STRING LogFileName,
    __in_opt LPGUID TmIdentity,
    __in_opt ULONG OpenOptions
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPlugPlayControl(
    __in PLUGPLAY_CONTROL_CLASS PnPControlClass,
    __inout_bcount(PnPControlDataLength) PVOID PnPControlData,
    __in ULONG PnPControlDataLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPowerInformation(
    __in POWER_INFORMATION_LEVEL InformationLevel,
    __in_bcount_opt(InputBufferLength) PVOID InputBuffer,
    __in ULONG InputBufferLength,
    __out_bcount_opt(OutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI 
ZwPrePrepareComplete(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPrePrepareEnlistment(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI 
ZwPrepareComplete(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPrepareEnlistment(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPrivilegeCheck(
    __in HANDLE ClientToken,
    __inout PPRIVILEGE_SET RequiredPrivileges,
    __out PBOOLEAN Result
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPrivilegeObjectAuditAlarm(
    __in PUNICODE_STRING SubsystemName,
    __in_opt PVOID HandleId,
    __in HANDLE ClientToken,
    __in ACCESS_MASK DesiredAccess,
    __in PPRIVILEGE_SET Privileges,
    __in BOOLEAN AccessGranted
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPrivilegedServiceAuditAlarm(
    __in PUNICODE_STRING SubsystemName,
    __in PUNICODE_STRING ServiceName,
    __in HANDLE ClientToken,
    __in PPRIVILEGE_SET Privileges,
    __in BOOLEAN AccessGranted
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPropagationComplete(
    __in HANDLE ResourceManagerHandle,
    __in ULONG RequestCookie,
    __in ULONG BufferLength,
    __in PVOID Buffer
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPropagationFailed(
    __in HANDLE ResourceManagerHandle,
    __in ULONG RequestCookie,
    __in NTSTATUS PropStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwProtectVirtualMemory(
    __in HANDLE ProcessHandle,
    __inout PVOID *BaseAddress,
    __inout PSIZE_T RegionSize,
    __in ULONG NewProtect,
    __out PULONG OldProtect
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwPulseEvent(
    __in HANDLE EventHandle,
    __out_opt PLONG PreviousState
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryAttributesFile(
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __out PFILE_BASIC_INFORMATION FileInformation
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryDebugFilterState(
    __in ULONG ComponentId,
    __in ULONG Level
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryDefaultLocale(
    __in BOOLEAN UserProfile,
    __out PLCID DefaultLocaleId
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryDefaultUILanguage(
    __out LANGID *DefaultUILanguageId
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryDirectoryFile(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __out_bcount(Length) PVOID FileInformation,
    __in ULONG Length,
    __in FILE_INFORMATION_CLASS FileInformationClass,
    __in BOOLEAN ReturnSingleEntry,
    __in_opt PUNICODE_STRING FileName,
    __in BOOLEAN RestartScan
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryDirectoryObject(
    __in HANDLE DirectoryHandle,
    __out_bcount_opt(BufferLength) PVOID Buffer,
    __in ULONG Length,
    __in BOOLEAN ReturnSingleEntry,
    __in BOOLEAN RestartScan,
    __inout PULONG Context,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryEaFile(
    __in HANDLE FileHandle,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __out_bcount(Length) PVOID Buffer,
    __in ULONG Length,
    __in BOOLEAN ReturnSingleEntry,
    __in_bcount_opt(EaListLength) PVOID EaList,
    __in ULONG EaListLength,
    __in_opt PULONG EaIndex OPTIONAL,
    __in BOOLEAN RestartScan
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryEvent(
    __in HANDLE EventHandle,
    __in EVENT_INFORMATION_CLASS EventInformationClass,
    __out_bcount(EventInformationLength) PVOID EventInformation,
    __in ULONG EventInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryFullAttributesFile(
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __out PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationAtom(
    __in RTL_ATOM Atom,
    __in ATOM_INFORMATION_CLASS AtomInformationClass,
    __out_bcount(AtomInformationLength) PVOID AtomInformation,
    __in ULONG AtomInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationEnlistment(
    __in HANDLE EnlistmentHandle,
    __in ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
    __out_bcount(EnlistmentInformationLength) PVOID EnlistmentInformation,
    __in ULONG EnlistmentInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationFile(
    __in HANDLE FileHandle,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __out_bcount(Length) PVOID FileInformation,
    __in ULONG Length,
    __in FILE_INFORMATION_CLASS FileInformationClass
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationJobObject(
    __in_opt HANDLE JobHandle,
    __in JOBOBJECTINFOCLASS JobObjectInformationClass,
    __out_bcount(JobObjectInformationLength) PVOID JobObjectInformation,
    __in ULONG JobObjectInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationPort(
    __in HANDLE PortHandle,
    __in PORT_INFORMATION_CLASS PortInformationClass,
    __out_bcount(Length) PVOID PortInformation,
    __in ULONG Length,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess(
    __in HANDLE ProcessHandle,
    __in PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationResourceManager(
    __in HANDLE ResourceManagerHandle,
    __in RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
    __out_bcount(ResourceManagerInformationLength) PVOID ResourceManagerInformation,
    __in ULONG ResourceManagerInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationThread(
    __in HANDLE ThreadHandle,
    __in THREAD_INFORMATION_CLASS ThreadInformationClass,
    __out_bcount(ThreadInformationLength) PVOID ThreadInformation,
    __in ULONG ThreadInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationToken(
    __in HANDLE TokenHandle,
    __in TOKEN_INFORMATION_CLASS TokenInformationClass,
    __out_bcount(TokenInformationLength) PVOID TokenInformation,
    __in ULONG TokenInformationLength,
    __out PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationTransaction(
    __in HANDLE TransactionHandle,
    __in TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
    __out_bcount(TransactionInformationLength) PVOID TransactionInformation,
    __in ULONG TransactionInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationTransactionManager(
    __in HANDLE TransactionManagerHandle,
    __in TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
    __out_bcount(TransactionManagerInformationLength) PVOID TransactionManagerInformation,
    __in ULONG TransactionManagerInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationWorkerFactory(
    __in HANDLE WorkerFactoryHandle,
    __in WORKER_FACTORY_INFORMATION_CLASS WorkerFactoryInformationClass,
    __out_bcount(WorkerFactoryInformationLength) PVOID WorkerFactoryInformation,
    __in ULONG WorkerFactoryInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInstallUILanguage(
    __out LANGID *InstallUILanguageId
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryIntervalProfile(
    __in KPROFILE_SOURCE ProfileSource,
    __out PULONG Interval
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryIoCompletion(
    __in HANDLE IoCompletionHandle,
    __in IO_COMPLETION_INFORMATION_CLASS IoCompletionInformationClass,
    __out_bcount(IoCompletionInformation) PVOID IoCompletionInformation,
    __in ULONG IoCompletionInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryKey(
    __in HANDLE KeyHandle,
    __in KEY_INFORMATION_CLASS KeyInformationClass,
    __out_bcount_opt(Length) PVOID KeyInformation,
    __in ULONG Length,
    __out PULONG ResultLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryMultipleValueKey(
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
ZwQueryMutant(
    __in HANDLE MutantHandle,
    __in MUTANT_INFORMATION_CLASS MutantInformationClass,
    __out_bcount(MutantInformationLength) PVOID MutantInformation,
    __in ULONG MutantInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryObject(
    __in HANDLE Handle,
    __in OBJECT_INFORMATION_CLASS ObjectInformationClass,
    __out_bcount_opt(ObjectInformationLength) PVOID ObjectInformation,
    __in ULONG ObjectInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryOpenSubKeys(
    __in POBJECT_ATTRIBUTES TargetKey,
    __out PULONG HandleCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryOpenSubKeysEx(
    __in POBJECT_ATTRIBUTES TargetKey,
    __in ULONG BufferLength,
    __out_bcount(BufferLength) PVOID Buffer,
    __out PULONG RequiredSize
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryPerformanceCounter(
    __out PLARGE_INTEGER PerformanceCounter,
    __out_opt PLARGE_INTEGER PerformanceFrequency
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryPortInformationProcess(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryQuotaInformationFile(
    __in HANDLE FileHandle,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __out_bcount(Length) PVOID Buffer,
    __in ULONG Length,
    __in BOOLEAN ReturnSingleEntry,
    __in_bcount_opt(SidListLength) PVOID SidList,
    __in ULONG SidListLength,
    __in_opt PSID StartSid,
    __in BOOLEAN RestartScan
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQuerySection(
    __in HANDLE SectionHandle,
    __in SECTION_INFORMATION_CLASS SectionInformationClass,
    __out_bcount(SectionInformationLength) PVOID SectionInformation,
    __in SIZE_T SectionInformationLength,
    __out_opt PSIZE_T ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQuerySecurityObject(
    __in HANDLE Handle,
    __in SECURITY_INFORMATION SecurityInformation,
    __out_bcount_opt(Length) PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in ULONG Length,
    __out PULONG LengthNeeded
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQuerySemaphore(
    __in HANDLE SemaphoreHandle,
    __in SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
    __out_bcount(SemaphoreInformationLength) PVOID SemaphoreInformation,
    __in ULONG SemaphoreInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQuerySymbolicLinkObject(
    __in HANDLE LinkHandle,
    __inout PUNICODE_STRING LinkTarget,
    __out_opt PULONG ReturnedLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQuerySystemEnvironmentValue(
    __in PUNICODE_STRING VariableName,
    __out_bcount(ValueLength) PWSTR VariableValue,
    __in USHORT ValueLength,
    __out_opt PUSHORT ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
    __in SYSTEM_INFORMATION_CLASS SystemInformationClass,
    __out_bcount_opt(SystemInformationLength) PVOID SystemInformation,
    __in ULONG SystemInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQuerySystemInformationEx(
    __in SYSTEM_INFORMATION_CLASS SystemInformationClass,
    __in_bcount(QueryInformationLength) PVOID QueryInformation,
    __in ULONG QueryInformationLength,
    __out_bcount_opt(SystemInformationLength) PVOID SystemInformation,
    __in ULONG SystemInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQuerySystemTime(
    __out PLARGE_INTEGER SystemTime
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryTimer(
    __in HANDLE TimerHandle,
    __in TIMER_INFORMATION_CLASS TimerInformationClass,
    __out_bcount(TimerInformationLength) PVOID TimerInformation,
    __in ULONG TimerInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryTimerResolution(
    __out PULONG MaximumTime,
    __out PULONG MinimumTime,
    __out PULONG CurrentTime
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryValueKey(
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
ZwQueryVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in MEMORY_INFORMATION_CLASS MemoryInformationClass,
    __out_bcount(MemoryInformationLength) PVOID MemoryInformation,
    __in SIZE_T MemoryInformationLength,
    __out_opt PSIZE_T ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryVolumeInformationFile(
    __in HANDLE FileHandle,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __out_bcount(Length) PVOID FsInformation,
    __in ULONG Length,
    __in FS_INFORMATION_CLASS FsInformationClass
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueueApcThread(
    __in HANDLE ThreadHandle,
    __in PPS_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcArgument1,
    __in_opt PVOID ApcArgument2,
    __in_opt PVOID ApcArgument3
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueueApcThreadEx(
    __in HANDLE ThreadHandle,
    __in_opt HANDLE UserApcReserveHandle,
    __in PPS_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcArgument1,
    __in_opt PVOID ApcArgument2,
    __in_opt PVOID ApcArgument3
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRaiseException(
    __in PEXCEPTION_RECORD ExceptionRecord,
    __in PCONTEXT ContextRecord,
    __in BOOLEAN FirstChance
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwReadFile(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __out_bcount(Length) PVOID Buffer,
    __in ULONG Length,
    __in_opt PLARGE_INTEGER ByteOffset,
    __in_opt PULONG Key
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwReadFileScatter(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in PFILE_SEGMENT_ELEMENT SegmentArray,
    __in ULONG Length,
    __in_opt PLARGE_INTEGER ByteOffset,
    __in_opt PULONG Key
    );

NTSYSCALLAPI
NTSTATUS
NTAPI 
ZwReadOnlyEnlistment(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwReadRequestData(
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE Message,
    __in ULONG DataEntryIndex,
    __out_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesRead
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwReadVirtualMemory(
    __in HANDLE ProcessHandle,
    __in_opt PVOID BaseAddress,
    __out_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesRead
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRecoverEnlistment(
    __in HANDLE EnlistmentHandle,
    __in_opt PVOID EnlistmentKey
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRecoverResourceManager(
    __in HANDLE ResourceManagerHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRecoverTransactionManager(
    __in HANDLE TransactionManagerHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRegisterProtocolAddressInformation(
    __in HANDLE ResourceManager,
    __in PCRM_PROTOCOL_ID ProtocolId,
    __in ULONG ProtocolInformationSize,
    __in PVOID ProtocolInformation,
    __in_opt ULONG CreateOptions
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRegisterThreadTerminatePort(
    __in HANDLE PortHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwReleaseKeyedEvent(
    __in HANDLE KeyedEventHandle,
    __in PVOID KeyValue,
    __in BOOLEAN Alertable,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwReleaseMutant(
    __in HANDLE MutantHandle,
    __out_opt PLONG PreviousCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwReleaseSemaphore(
    __in HANDLE SemaphoreHandle,
    __in LONG ReleaseCount,
    __out_opt PLONG PreviousCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwReleaseWorkerFactoryWorker(
    __in HANDLE WorkerFactoryHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRemoveIoCompletion(
    __in HANDLE IoCompletionHandle,
    __out PVOID *KeyContext,
    __out PVOID *ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRemoveProcessDebug(
    __in HANDLE ProcessHandle,
    __in HANDLE DebugObjectHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRenameKey(
    __in HANDLE KeyHandle,
    __in PUNICODE_STRING NewName
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRenameTransactionManager(
    __in PUNICODE_STRING LogFileName,
    __in LPGUID ExistingTransactionManagerGuid
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwReplaceKey(
    __in POBJECT_ATTRIBUTES NewFile,
    __in HANDLE TargetHandle,
    __in POBJECT_ATTRIBUTES OldFile
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwReplyPort(
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE ReplyMessage
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwReplyWaitReceivePort(
    __in HANDLE PortHandle,
    __out_opt PVOID *PortContext,
    __in_opt PPORT_MESSAGE ReplyMessage,
    __out PPORT_MESSAGE ReceiveMessage
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwReplyWaitReceivePortEx(
    __in HANDLE PortHandle,
    __out_opt PVOID *PortContext,
    __in_opt PPORT_MESSAGE ReplyMessage,
    __out PPORT_MESSAGE ReceiveMessage,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwReplyWaitReplyPort(
    __in HANDLE PortHandle,
    __inout PPORT_MESSAGE ReplyMessage
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRequestPort(
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE RequestMessage
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRequestWaitReplyPort(
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE RequestMessage,
    __out PPORT_MESSAGE ReplyMessage
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwResetEvent(
    __in HANDLE EventHandle,
    __out_opt PLONG PreviousState
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwResetWriteWatch(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in SIZE_T RegionSize
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRestoreKey(
    __in HANDLE KeyHandle,
    __in HANDLE FileHandle,
    __in ULONG Flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwResumeProcess(
    __in HANDLE ProcessHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwResumeThread(
    __in HANDLE ThreadHandle,
    __out_opt PULONG PreviousSuspendCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI 
ZwRollbackComplete(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRollbackEnlistment(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRollbackTransaction(
    __in HANDLE TransactionHandle,
    __in BOOLEAN Wait
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwRollforwardTransactionManager(
    __in HANDLE TransactionManagerHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSaveKey(
    __in HANDLE KeyHandle,
    __in HANDLE FileHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSaveKeyEx(
    __in HANDLE KeyHandle,
    __in HANDLE FileHandle,
    __in ULONG Format
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSaveMergedKeys(
    __in HANDLE HighPrecedenceKeyHandle,
    __in HANDLE LowPrecedenceKeyHandle,
    __in HANDLE FileHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSecureConnectPort(
    __out PHANDLE PortHandle,
    __in PUNICODE_STRING PortName,
    __in PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    __inout_opt PPORT_VIEW ClientView,
    __in_opt PSID RequiredServerSid,
    __inout_opt PREMOTE_PORT_VIEW ServerView,
    __out_opt PULONG MaxMessageLength,
    __inout_opt PVOID ConnectionInformation,
    __inout_opt PULONG ConnectionInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSerializeBoot(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetContextThread(
    __in HANDLE ThreadHandle,
    __in PCONTEXT ThreadContext
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetDebugFilterState(
    __in ULONG ComponentId,
    __in ULONG Level,
    __in BOOLEAN State
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetDefaultHardErrorPort(
    __in HANDLE DefaultHardErrorPort
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetDefaultLocale(
    __in BOOLEAN UserProfile,
    __in LCID DefaultLocaleId
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetDefaultUILanguage(
    __in LANGID DefaultUILanguageId
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetEaFile(
    __in HANDLE FileHandle,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in_bcount(Length) PVOID Buffer,
    __in ULONG Length
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetEvent(
    __in HANDLE EventHandle,
    __out_opt PLONG PreviousState
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetEventBoostPriority(
    __in HANDLE EventHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetHighEventPair(
    __in HANDLE EventPairHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetHighWaitLowEventPair(
    __in HANDLE EventPairHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationDebugObject(
    __in HANDLE DebugObjectHandle,
    __in DEBUGOBJECTINFOCLASS DebugObjectInformationClass,
    __in PVOID DebugInformation,
    __in ULONG DebugInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationEnlistment(
    __in_opt HANDLE EnlistmentHandle,
    __in ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
    __in_bcount(EnlistmentInformationLength) PVOID EnlistmentInformation,
    __in ULONG EnlistmentInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationFile(
    __in HANDLE FileHandle,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in_bcount(Length) PVOID FileInformation,
    __in ULONG Length,
    __in FILE_INFORMATION_CLASS FileInformationClass
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationJobObject(
    __in HANDLE JobHandle,
    __in JOBOBJECTINFOCLASS JobObjectInformationClass,
    __in_bcount(JobObjectInformationLength) PVOID JobObjectInformation,
    __in ULONG JobObjectInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationKey(
    __in HANDLE KeyHandle,
    __in KEY_SET_INFORMATION_CLASS KeySetInformationClass,
    __in_bcount(KeySetInformationLength) PVOID KeySetInformation,
    __in ULONG KeySetInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationObject(
    __in HANDLE Handle,
    __in OBJECT_INFORMATION_CLASS ObjectInformationClass,
    __in_bcount(ObjectInformationLength) PVOID ObjectInformation,
    __in ULONG ObjectInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationProcess(
    __in HANDLE ProcessHandle,
    __in PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __in_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationResourceManager(
    __in HANDLE ResourceManagerHandle,
    __in RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
    __in_bcount(ResourceManagerInformationLength) PVOID ResourceManagerInformation,
    __in ULONG ResourceManagerInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationThread(
    __in HANDLE ThreadHandle,
    __in THREAD_INFORMATION_CLASS ThreadInformationClass,
    __in_bcount(ThreadInformationLength) PVOID ThreadInformation,
    __in ULONG ThreadInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationToken(
    __in HANDLE TokenHandle,
    __in TOKEN_INFORMATION_CLASS TokenInformationClass,
    __in_bcount(TokenInformationLength) PVOID TokenInformation,
    __in ULONG TokenInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationTransaction(
    __in HANDLE TransactionHandle,
    __in TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
    __in_bcount(TransactionInformationLength) PVOID TransactionInformation,
    __in ULONG TransactionInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationTransactionManager(
    __in_opt HANDLE TmHandle,
    __in TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
    __in_bcount(TransactionManagerInformationLength) PVOID TransactionManagerInformation,
    __in ULONG TransactionManagerInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationWorkerFactory(
    __in HANDLE WorkerFactoryHandle,
    __in WORKER_FACTORY_INFORMATION_CLASS WorkerFactoryInformationClass,
    __in_bcount(WorkerFactoryInformationLength) PVOID WorkerFactoryInformation,
    __in ULONG WorkerFactoryInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetIntervalProfile(
    __in ULONG Interval,
    __in KPROFILE_SOURCE Source
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetIoCompletion(
    __in HANDLE IoCompletionHandle,
    __in PVOID KeyContext,
    __in_opt PVOID ApcContext,
    __in NTSTATUS IoStatus,
    __in ULONG_PTR IoStatusInformation
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetIoCompletionEx(
    __in HANDLE IoCompletionHandle,
    __in HANDLE IoCompletionReserveHandle,
    __in PVOID KeyContext,
    __in_opt PVOID ApcContext,
    __in NTSTATUS IoStatus,
    __in ULONG_PTR IoStatusInformation
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetLdtEntries(
    __in ULONG Selector0,
    __in ULONG Entry0Low,
    __in ULONG Entry0Hi,
    __in ULONG Selector1,
    __in ULONG Entry1Low,
    __in ULONG Entry1Hi
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetLowEventPair(
    __in HANDLE EventPairHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetLowWaitHighEventPair(
    __in HANDLE EventPairHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetQuotaInformationFile(
    __in HANDLE FileHandle,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in_bcount(Length) PVOID Buffer,
    __in ULONG Length
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetSecurityObject(
    __in HANDLE Handle,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetSystemEnvironmentValue(
    __in PUNICODE_STRING VariableName,
    __in PUNICODE_STRING VariableValue
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetSystemInformation(
    __in SYSTEM_INFORMATION_CLASS SystemInformationClass,
    __in_bcount_opt(SystemInformationLength) PVOID SystemInformation,
    __in ULONG SystemInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetSystemPowerState(
    __in POWER_ACTION SystemAction,
    __in SYSTEM_POWER_STATE LightestSystemState,
    __in ULONG Flags // POWER_ACTION_* flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetSystemTime(
    __in_opt PLARGE_INTEGER SystemTime,
    __out_opt PLARGE_INTEGER PreviousTime
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetThreadExecutionState(
    __in EXECUTION_STATE NewFlags, // ES_* flags
    __out EXECUTION_STATE *PreviousFlags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetTimer(
    __in HANDLE TimerHandle,
    __in PLARGE_INTEGER DueTime,
    __in_opt PTIMER_APC_ROUTINE TimerApcRoutine,
    __in_opt PVOID TimerContext,
    __in BOOLEAN ResumeTimer,
    __in_opt LONG Period,
    __out_opt PBOOLEAN PreviousState
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetTimerEx(
    __in HANDLE TimerHandle,
    __in TIMER_SET_INFORMATION_CLASS TimerSetInformationClass,
    __inout_bcount_opt(TimerSetInformationLength) PVOID TimerSetInformation,
    __in ULONG TimerSetInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetTimerResolution(
    __in ULONG DesiredTime,
    __in BOOLEAN SetResolution,
    __out PULONG ActualTime
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetUuidSeed(
    __in PCHAR Seed
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetValueKey(
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
ZwSetVolumeInformationFile(
    __in HANDLE FileHandle,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in_bcount(Length) PVOID FsInformation,
    __in ULONG Length,
    __in FS_INFORMATION_CLASS FsInformationClass
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwShutdownSystem(
    __in SHUTDOWN_ACTION Action
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwShutdownWorkerFactory(
    __in HANDLE WorkerFactoryHandle,
    __inout PULONG RefCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSignalAndWaitForSingleObject(
    __in HANDLE SignalHandle,
    __in HANDLE WaitHandle,
    __in BOOLEAN Alertable,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI 
ZwSinglePhaseReject(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwStartProfile(
    __in HANDLE ProfileHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwStopProfile(
    __in HANDLE ProfileHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSuspendProcess(
    __in HANDLE ProcessHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSuspendThread(
    __in HANDLE ThreadHandle,
    __out_opt PULONG PreviousSuspendCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwTerminateJobObject(
    __in HANDLE JobHandle,
    __in NTSTATUS ExitStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwTerminateProcess(
    __in_opt HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwTerminateThread(
    __in_opt HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwTestAlert(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwThawRegistry(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwThawTransactions(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwTraceEvent(
    __in HANDLE TraceHandle,
    __in ULONG Flags,
    __in ULONG FieldSize,
    __in PVOID Fields
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwUnloadDriver(
    __in PUNICODE_STRING DriverServiceName
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwUnloadKey(
    __in POBJECT_ATTRIBUTES TargetKey
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwUnloadKey2(
    __in POBJECT_ATTRIBUTES TargetKey,
    __in ULONG Flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwUnloadKeyEx(
    __in POBJECT_ATTRIBUTES TargetKey,
    __in_opt HANDLE Event
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwUnlockFile(
    __in HANDLE FileHandle,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in PLARGE_INTEGER ByteOffset,
    __in PLARGE_INTEGER Length,
    __in ULONG Key
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwUnlockVirtualMemory( 
    __in HANDLE ProcessHandle,
    __inout PVOID *BaseAddress,
    __inout PSIZE_T RegionSize,
    __in ULONG MapType
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwUnmapViewOfSection(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwVdmControl(
    __in VDMSERVICECLASS Service,
    __inout PVOID ServiceData
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwWaitForDebugEvent(
    __in HANDLE DebugObjectHandle,
    __in BOOLEAN Alertable,
    __in_opt PLARGE_INTEGER Timeout,
    __out PVOID WaitStateChange
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwWaitForKeyedEvent(
    __in HANDLE KeyedEventHandle,
    __in PVOID KeyValue,
    __in BOOLEAN Alertable,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwWaitForMultipleObjects(
    __in ULONG Count,
    __in_ecount(Count) PHANDLE Handles,
    __in WAIT_TYPE WaitType,
    __in BOOLEAN Alertable,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwWaitForSingleObject(
    __in HANDLE Handle,
    __in BOOLEAN Alertable,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwWaitForWorkViaWorkerFactory(
    __in HANDLE WorkerFactoryHandle,
    __out struct _IO_COMPLETION_MINIPACKET *MiniPacket
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwWaitHighEventPair(
    __in HANDLE EventPairHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwWaitLowEventPair( 
    __in HANDLE EventPairHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwWorkerFactoryWorkerReady(
    __in HANDLE WorkerFactoryHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwWriteFile(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in_bcount(Length) PVOID Buffer,
    __in ULONG Length,
    __in_opt PLARGE_INTEGER ByteOffset,
    __in_opt PULONG Key
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwWriteFileGather(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __in PFILE_SEGMENT_ELEMENT SegmentArray,
    __in ULONG Length,
    __in_opt PLARGE_INTEGER ByteOffset,
    __in_opt PULONG Key
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwWriteRequestData(
    __in HANDLE PortHandle,
    __in PPORT_MESSAGE Message,
    __in ULONG DataEntryIndex,
    __in_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesWritten
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwWriteVirtualMemory(
    __in HANDLE ProcessHandle,
    __in_opt PVOID BaseAddress,
    __in_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesWritten
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwYieldExecution(
    VOID
    );

#endif
