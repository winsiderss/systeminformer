#ifndef _NTTMAPI_H
#define _NTTMAPI_H

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateTransactionManager(
    __out PHANDLE TmHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PUNICODE_STRING LogFileName,
    __in_opt ULONG CreateOptions,
    __in_opt ULONG CommitStrength
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenTransactionManager(
    __out PHANDLE TmHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PUNICODE_STRING LogFileName,
    __in_opt LPGUID TmIdentity,
    __in_opt ULONG OpenOptions
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRenameTransactionManager(
    __in PUNICODE_STRING LogFileName,
    __in LPGUID ExistingTransactionManagerGuid
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollforwardTransactionManager(
    __in HANDLE TransactionManagerHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRecoverTransactionManager(
    __in HANDLE TransactionManagerHandle
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationTransactionManager(
    __in HANDLE TransactionManagerHandle,
    __in TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
    __out_bcount(TransactionManagerInformationLength) PVOID TransactionManagerInformation,
    __in ULONG TransactionManagerInformationLength,
    __out_opt PULONG ReturnLength
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationTransactionManager(
    __in_opt HANDLE TmHandle,
    __in TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
    __in_bcount(TransactionManagerInformationLength) PVOID TransactionManagerInformation,
    __in ULONG TransactionManagerInformationLength
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS 
NTAPI
NtEnumerateTransactionObject(
    __in_opt HANDLE RootObjectHandle,
    __in KTMOBJECT_TYPE QueryType,
    __inout_bcount(ObjectCursorLength) PKTMOBJECT_CURSOR ObjectCursor,
    __in ULONG ObjectCursorLength,
    __out PULONG ReturnLength
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateTransaction(
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
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenTransaction(
    __out PHANDLE TransactionHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in LPGUID Uow,
    __in_opt HANDLE TmHandle
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationTransaction(
    __in HANDLE TransactionHandle,
    __in TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
    __out_bcount(TransactionInformationLength) PVOID TransactionInformation,
    __in ULONG TransactionInformationLength,
    __out_opt PULONG ReturnLength
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationTransaction(
    __in HANDLE TransactionHandle,
    __in TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
    __in_bcount(TransactionInformationLength) PVOID TransactionInformation,
    __in ULONG TransactionInformationLength
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCommitTransaction(
    __in HANDLE TransactionHandle,
    __in BOOLEAN Wait
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollbackTransaction(
    __in HANDLE TransactionHandle,
    __in BOOLEAN Wait
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateEnlistment(
    __out PHANDLE EnlistmentHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ResourceManagerHandle,
    __in HANDLE TransactionHandle,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt ULONG CreateOptions,
    __in NOTIFICATION_MASK NotificationMask,
    __in_opt PVOID EnlistmentKey
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenEnlistment(
    __out PHANDLE EnlistmentHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ResourceManagerHandle,
    __in LPGUID EnlistmentGuid,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationEnlistment(
    __in HANDLE EnlistmentHandle,
    __in ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
    __out_bcount(EnlistmentInformationLength) PVOID EnlistmentInformation,
    __in ULONG EnlistmentInformationLength,
    __out_opt PULONG ReturnLength
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationEnlistment(
    __in_opt HANDLE EnlistmentHandle,
    __in ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
    __in_bcount(EnlistmentInformationLength) PVOID EnlistmentInformation,
    __in ULONG EnlistmentInformationLength
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRecoverEnlistment(
    __in HANDLE EnlistmentHandle,
    __in_opt PVOID EnlistmentKey
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrePrepareEnlistment(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrepareEnlistment(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCommitEnlistment(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollbackEnlistment(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI 
NtPrePrepareComplete(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI 
NtPrepareComplete(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI 
NtCommitComplete(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI 
NtReadOnlyEnlistment(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI 
NtRollbackComplete(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI 
NtSinglePhaseReject(
    __in HANDLE EnlistmentHandle,
    __in_opt PLARGE_INTEGER TmVirtualClock
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateResourceManager(
    __out PHANDLE ResourceManagerHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE TmHandle,
    __in LPGUID RmGuid,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt ULONG CreateOptions,
    __in_opt PUNICODE_STRING Description
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenResourceManager(
    __out PHANDLE ResourceManagerHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE TmHandle,
    __in_opt LPGUID ResourceManagerGuid,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRecoverResourceManager(
    __in HANDLE ResourceManagerHandle
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetNotificationResourceManager(
    __in HANDLE ResourceManagerHandle,
    __out PTRANSACTION_NOTIFICATION TransactionNotification,
    __in ULONG NotificationLength,
    __in_opt PLARGE_INTEGER Timeout,
    __out_opt PULONG ReturnLength,
    __in ULONG Asynchronous,
    __in_opt ULONG_PTR AsynchronousContext
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationResourceManager(
    __in HANDLE ResourceManagerHandle,
    __in RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
    __out_bcount(ResourceManagerInformationLength) PVOID ResourceManagerInformation,
    __in ULONG ResourceManagerInformationLength,
    __out_opt PULONG ReturnLength
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationResourceManager(
    __in HANDLE ResourceManagerHandle,
    __in RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
    __in_bcount(ResourceManagerInformationLength) PVOID ResourceManagerInformation,
    __in ULONG ResourceManagerInformationLength
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRegisterProtocolAddressInformation(
    __in HANDLE ResourceManager,
    __in PCRM_PROTOCOL_ID ProtocolId,
    __in ULONG ProtocolInformationSize,
    __in PVOID ProtocolInformation,
    __in_opt ULONG CreateOptions
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPropagationComplete(
    __in HANDLE ResourceManagerHandle,
    __in ULONG RequestCookie,
    __in ULONG BufferLength,
    __in PVOID Buffer
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPropagationFailed(
    __in HANDLE ResourceManagerHandle,
    __in ULONG RequestCookie,
    __in NTSTATUS PropStatus
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
// private
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreezeTransactions(
    __in PLARGE_INTEGER FreezeTimeout,
    __in PLARGE_INTEGER ThawTimeout
    );
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
// private
NTSYSCALLAPI
NTSTATUS
NTAPI
NtThawTransactions(
    VOID
    );
#endif

#endif
