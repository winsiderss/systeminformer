/*
 * Transaction Manager support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTTMAPI_H
#define _NTTMAPI_H

/**
 * The NtCreateTransactionManager routine creates a new transaction manager object.
 *
 * \param TmHandle A pointer to a caller-allocated variable that receives a handle to the new transaction manager object.
 * \param DesiredAccess An ACCESS_MASK value that specifies the requested access to the transaction manager object.
 * \param ObjectAttributes Optional object attributes for the transaction manager object name and security settings.
 * \param LogFileName Optional CLFS log file stream path to associate with the transaction manager. This must be NULL if CreateOptions contains TRANSACTION_MANAGER_VOLATILE.
 * \param CreateOptions Optional transaction manager creation flags.
 * \param CommitStrength Reserved for future use. This value must be zero.
 * \return NTSTATUS code indicating success or failure.
 * \remarks If the specified log stream does not exist, KTM creates it; otherwise, KTM opens the existing stream.
 * Callers are expected to call NtRecoverTransactionManager after creating the transaction manager, and must eventually close the returned handle.
 * \sa https://learn.microsoft.com/en-us/windows/win32/secauthz/ntcreatelowboxtoken
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateTransactionManager(
    _Out_ PHANDLE TmHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PCUNICODE_STRING LogFileName,
    _In_opt_ ULONG CreateOptions,
    _In_opt_ ULONG CommitStrength
    );

/**
 * The NtOpenTransactionManager routine opens an existing transaction manager object.
 *
 * \param TmHandle A pointer to a HANDLE variable that receives a handle to the transaction manager object.
 * \param DesiredAccess An ACCESS_MASK value that specifies the requested access to the transaction manager object.
 * \param ObjectAttributes Optional object attributes for the transaction manager object name and security settings.
 * \param LogFileName Optional CLFS log file stream path associated with the transaction manager.
 * \param TmIdentity Optional identifier for the transaction manager object.
 * \param OpenOptions Optional transaction manager open flags.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenTransactionManager(
    _Out_ PHANDLE TmHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PCUNICODE_STRING LogFileName,
    _In_opt_ LPGUID TmIdentity,
    _In_opt_ ULONG OpenOptions
    );

/**
 * The NtRenameTransactionManager routine renames a transaction manager log file.
 *
 * \param LogFileName The new CLFS log file stream path.
 * \param ExistingTransactionManagerGuid The GUID of the transaction manager to rename.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRenameTransactionManager(
    _In_ PCUNICODE_STRING LogFileName,
    _In_ LPGUID ExistingTransactionManagerGuid
    );

/**
 * The NtRollforwardTransactionManager routine rolls a transaction manager forward to the specified virtual clock.
 *
 * \param TransactionManagerHandle Handle to the transaction manager object.
 * \param TmVirtualClock Optional virtual clock value.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollforwardTransactionManager(
    _In_ HANDLE TransactionManagerHandle,
    _In_opt_ PLARGE_INTEGER TmVirtualClock
    );

/**
 * The NtRecoverTransactionManager routine recovers a transaction manager object.
 *
 * \param TransactionManagerHandle Handle to the transaction manager object.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRecoverTransactionManager(
    _In_ HANDLE TransactionManagerHandle
    );

/**
 * The NtQueryInformationTransactionManager routine queries information about a transaction manager object.
 *
 * \param TransactionManagerHandle Handle to the transaction manager object.
 * \param TransactionManagerInformationClass The type of transaction manager information to query.
 * \param TransactionManagerInformation Caller-allocated buffer that receives the requested information.
 * \param TransactionManagerInformationLength The size of the buffer pointed to by TransactionManagerInformation, in bytes.
 * \param ReturnLength Optional pointer to a variable that receives the number of bytes returned or required.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationTransactionManager(
    _In_ HANDLE TransactionManagerHandle,
    _In_ TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
    _Out_writes_bytes_(TransactionManagerInformationLength) PVOID TransactionManagerInformation,
    _In_ ULONG TransactionManagerInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

/**
 * The NtSetInformationTransactionManager routine sets information for a transaction manager object.
 *
 * \param TmHandle Optional handle to the transaction manager object.
 * \param TransactionManagerInformationClass The type of transaction manager information to set.
 * \param TransactionManagerInformation Pointer to the input buffer containing the information to set.
 * \param TransactionManagerInformationLength The size of the input buffer, in bytes.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationTransactionManager(
    _In_opt_ HANDLE TmHandle,
    _In_ TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
    _In_reads_bytes_(TransactionManagerInformationLength) PVOID TransactionManagerInformation,
    _In_ ULONG TransactionManagerInformationLength
    );

/**
 * The NtEnumerateTransactionObject routine enumerates KTM objects associated with a root handle.
 *
 * \param RootObjectHandle Optional root object handle.
 * \param QueryType The KTM object type to enumerate.
 * \param ObjectCursor Caller-supplied cursor used to track enumeration state.
 * \param ObjectCursorLength The size of the cursor buffer, in bytes.
 * \param ReturnLength Optional pointer to a variable that receives the number of bytes returned or required.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnumerateTransactionObject(
    _In_opt_ HANDLE RootObjectHandle,
    _In_ KTMOBJECT_TYPE QueryType,
    _Inout_updates_bytes_(ObjectCursorLength) PKTMOBJECT_CURSOR ObjectCursor,
    _In_ ULONG ObjectCursorLength,
    _Out_ PULONG ReturnLength
    );

/**
 * The NtCreateTransaction routine creates a transaction object.
 *
 * \param TransactionHandle A pointer to a HANDLE variable that receives a handle to the new transaction object.
 * \param DesiredAccess An ACCESS_MASK value that specifies the requested access to the transaction object.
 * \param ObjectAttributes Optional object attributes for the transaction object name and security settings.
 * \param Uow Optional unit-of-work identifier.
 * \param TmHandle Optional handle to the transaction manager object.
 * \param CreateOptions Optional transaction creation flags.
 * \param IsolationLevel Optional transaction isolation level.
 * \param IsolationFlags Optional transaction isolation flags.
 * \param Timeout Optional timeout for transaction creation.
 * \param Description Optional transaction description.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateTransaction(
    _Out_ PHANDLE TransactionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ LPGUID Uow,
    _In_opt_ HANDLE TmHandle,
    _In_opt_ ULONG CreateOptions,
    _In_opt_ ULONG IsolationLevel,
    _In_opt_ ULONG IsolationFlags,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_opt_ PCUNICODE_STRING Description
    );

/**
 * The NtOpenTransaction routine opens an existing transaction object.
 *
 * \param TransactionHandle A pointer to a HANDLE variable that receives a handle to the transaction object.
 * \param DesiredAccess An ACCESS_MASK value that specifies the requested access to the transaction object.
 * \param ObjectAttributes Optional object attributes for the transaction object name and security settings.
 * \param Uow The unit-of-work identifier of the transaction.
 * \param TmHandle Optional handle to the transaction manager object.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenTransaction(
    _Out_ PHANDLE TransactionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ LPGUID Uow,
    _In_opt_ HANDLE TmHandle
    );

/**
 * The NtQueryInformationTransaction routine queries information about a transaction object.
 *
 * \param TransactionHandle Handle to the transaction object.
 * \param TransactionInformationClass The type of transaction information to query.
 * \param TransactionInformation Caller-allocated buffer that receives the requested information.
 * \param TransactionInformationLength The size of the buffer pointed to by TransactionInformation, in bytes.
 * \param ReturnLength Optional pointer to a variable that receives the number of bytes returned or required.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationTransaction(
    _In_ HANDLE TransactionHandle,
    _In_ TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
    _Out_writes_bytes_(TransactionInformationLength) PVOID TransactionInformation,
    _In_ ULONG TransactionInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

/**
 * The NtSetInformationTransaction routine sets information for a transaction object.
 *
 * \param TransactionHandle Handle to the transaction object.
 * \param TransactionInformationClass The type of transaction information to set.
 * \param TransactionInformation Pointer to the input buffer containing the information to set.
 * \param TransactionInformationLength The size of the input buffer, in bytes.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationTransaction(
    _In_ HANDLE TransactionHandle,
    _In_ TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
    _In_reads_bytes_(TransactionInformationLength) PVOID TransactionInformation,
    _In_ ULONG TransactionInformationLength
    );

/**
 * The NtCommitTransaction routine commits a transaction object.
 *
 * \param TransactionHandle Handle to the transaction object.
 * \param Wait If TRUE, the call waits for the commit to complete.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCommitTransaction(
    _In_ HANDLE TransactionHandle,
    _In_ BOOLEAN Wait
    );

/**
 * The NtRollbackTransaction routine rolls back a transaction object.
 *
 * \param TransactionHandle Handle to the transaction object.
 * \param Wait If TRUE, the call waits for the rollback to complete.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollbackTransaction(
    _In_ HANDLE TransactionHandle,
    _In_ BOOLEAN Wait
    );

/**
 * The NtCreateEnlistment routine creates an enlistment object.
 *
 * \param EnlistmentHandle A pointer to a HANDLE variable that receives a handle to the enlistment object.
 * \param DesiredAccess An ACCESS_MASK value that specifies the requested access to the enlistment object.
 * \param ResourceManagerHandle Handle to the resource manager object.
 * \param TransactionHandle Handle to the transaction object.
 * \param ObjectAttributes Optional object attributes for the enlistment object name and security settings.
 * \param CreateOptions Optional enlistment creation flags.
 * \param NotificationMask Notification flags for enlistment events.
 * \param EnlistmentKey Optional enlistment key.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateEnlistment(
    _Out_ PHANDLE EnlistmentHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ResourceManagerHandle,
    _In_ HANDLE TransactionHandle,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ ULONG CreateOptions,
    _In_ NOTIFICATION_MASK NotificationMask,
    _In_opt_ PVOID EnlistmentKey
    );

/**
 * The NtOpenEnlistment routine opens an existing enlistment object.
 *
 * \param EnlistmentHandle A pointer to a HANDLE variable that receives a handle to the enlistment object.
 * \param DesiredAccess An ACCESS_MASK value that specifies the requested access to the enlistment object.
 * \param ResourceManagerHandle Handle to the resource manager object.
 * \param EnlistmentGuid The GUID of the enlistment object to open.
 * \param ObjectAttributes Optional object attributes for the enlistment object name and security settings.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenEnlistment(
    _Out_ PHANDLE EnlistmentHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ResourceManagerHandle,
    _In_ LPGUID EnlistmentGuid,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * The NtQueryInformationEnlistment routine queries information about an enlistment object.
 *
 * \param EnlistmentHandle Handle to the enlistment object.
 * \param EnlistmentInformationClass The type of enlistment information to query.
 * \param EnlistmentInformation Caller-allocated buffer that receives the requested information.
 * \param EnlistmentInformationLength The size of the buffer pointed to by EnlistmentInformation, in bytes.
 * \param ReturnLength Optional pointer to a variable that receives the number of bytes returned or required.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationEnlistment(
    _In_ HANDLE EnlistmentHandle,
    _In_ ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
    _Out_writes_bytes_(EnlistmentInformationLength) PVOID EnlistmentInformation,
    _In_ ULONG EnlistmentInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

/**
 * The NtSetInformationEnlistment routine sets information for an enlistment object.
 *
 * \param EnlistmentHandle Optional handle to the enlistment object.
 * \param EnlistmentInformationClass The type of enlistment information to set.
 * \param EnlistmentInformation Pointer to the input buffer containing the information to set.
 * \param EnlistmentInformationLength The size of the input buffer, in bytes.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationEnlistment(
    _In_opt_ HANDLE EnlistmentHandle,
    _In_ ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
    _In_reads_bytes_(EnlistmentInformationLength) PVOID EnlistmentInformation,
    _In_ ULONG EnlistmentInformationLength
    );

/**
 * The NtRecoverEnlistment routine recovers an enlistment object.
 *
 * \param EnlistmentHandle Handle to the enlistment object.
 * \param EnlistmentKey Optional enlistment key.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRecoverEnlistment(
    _In_ HANDLE EnlistmentHandle,
    _In_opt_ PVOID EnlistmentKey
    );

/**
 * The NtPrePrepareEnlistment routine starts the pre-prepare phase for an enlistment object.
 *
 * \param EnlistmentHandle Handle to the enlistment object.
 * \param TmVirtualClock Optional virtual clock value.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrePrepareEnlistment(
    _In_ HANDLE EnlistmentHandle,
    _In_opt_ PLARGE_INTEGER TmVirtualClock
    );

/**
 * The NtPrepareEnlistment routine starts the prepare phase for an enlistment object.
 *
 * \param EnlistmentHandle Handle to the enlistment object.
 * \param TmVirtualClock Optional virtual clock value.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrepareEnlistment(
    _In_ HANDLE EnlistmentHandle,
    _In_opt_ PLARGE_INTEGER TmVirtualClock
    );

/**
 * The NtCommitEnlistment routine commits an enlistment object.
 *
 * \param EnlistmentHandle Handle to the enlistment object.
 * \param TmVirtualClock Optional virtual clock value.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCommitEnlistment(
    _In_ HANDLE EnlistmentHandle,
    _In_opt_ PLARGE_INTEGER TmVirtualClock
    );

/**
 * The NtRollbackEnlistment routine rolls back an enlistment object.
 *
 * \param EnlistmentHandle Handle to the enlistment object.
 * \param TmVirtualClock Optional virtual clock value.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollbackEnlistment(
    _In_ HANDLE EnlistmentHandle,
    _In_opt_ PLARGE_INTEGER TmVirtualClock
    );

/**
 * The NtPrePrepareComplete routine completes the pre-prepare phase for an enlistment object.
 *
 * \param EnlistmentHandle Handle to the enlistment object.
 * \param TmVirtualClock Optional virtual clock value.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrePrepareComplete(
    _In_ HANDLE EnlistmentHandle,
    _In_opt_ PLARGE_INTEGER TmVirtualClock
    );

/**
 * The NtPrepareComplete routine completes the prepare phase for an enlistment object.
 *
 * \param EnlistmentHandle Handle to the enlistment object.
 * \param TmVirtualClock Optional virtual clock value.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrepareComplete(
    _In_ HANDLE EnlistmentHandle,
    _In_opt_ PLARGE_INTEGER TmVirtualClock
    );

/**
 * The NtCommitComplete routine completes the commit phase for an enlistment object.
 *
 * \param EnlistmentHandle Handle to the enlistment object.
 * \param TmVirtualClock Optional virtual clock value.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCommitComplete(
    _In_ HANDLE EnlistmentHandle,
    _In_opt_ PLARGE_INTEGER TmVirtualClock
    );

/**
 * The NtReadOnlyEnlistment routine marks an enlistment object as read-only.
 *
 * \param EnlistmentHandle Handle to the enlistment object.
 * \param TmVirtualClock Optional virtual clock value.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadOnlyEnlistment(
    _In_ HANDLE EnlistmentHandle,
    _In_opt_ PLARGE_INTEGER TmVirtualClock
    );

/**
 * The NtRollbackComplete routine completes the rollback phase for an enlistment object.
 *
 * \param EnlistmentHandle Handle to the enlistment object.
 * \param TmVirtualClock Optional virtual clock value.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollbackComplete(
    _In_ HANDLE EnlistmentHandle,
    _In_opt_ PLARGE_INTEGER TmVirtualClock
    );

/**
 * The NtSinglePhaseReject routine rejects a single-phase enlistment object.
 *
 * \param EnlistmentHandle Handle to the enlistment object.
 * \param TmVirtualClock Optional virtual clock value.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSinglePhaseReject(
    _In_ HANDLE EnlistmentHandle,
    _In_opt_ PLARGE_INTEGER TmVirtualClock
    );

/**
 * The NtCreateResourceManager routine creates a resource manager object.
 *
 * \param ResourceManagerHandle A pointer to a HANDLE variable that receives a handle to the resource manager object.
 * \param DesiredAccess An ACCESS_MASK value that specifies the requested access to the resource manager object.
 * \param TmHandle Handle to the transaction manager object.
 * \param RmGuid GUID for the new resource manager object.
 * \param ObjectAttributes Optional object attributes for the resource manager object name and security settings.
 * \param CreateOptions Optional resource manager creation flags.
 * \param Description Optional description string for the resource manager.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateResourceManager(
    _Out_ PHANDLE ResourceManagerHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE TmHandle,
    _In_ LPGUID RmGuid,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ ULONG CreateOptions,
    _In_opt_ PCUNICODE_STRING Description
    );

/**
 * The NtOpenResourceManager routine opens an existing resource manager object.
 *
 * \param ResourceManagerHandle A pointer to a HANDLE variable that receives a handle to the resource manager object.
 * \param DesiredAccess An ACCESS_MASK value that specifies the requested access to the resource manager object.
 * \param TmHandle Handle to the transaction manager object.
 * \param ResourceManagerGuid Optional GUID of the resource manager object to open.
 * \param ObjectAttributes Optional object attributes for the resource manager object name and security settings.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenResourceManager(
    _Out_ PHANDLE ResourceManagerHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE TmHandle,
    _In_opt_ LPGUID ResourceManagerGuid,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * The NtRecoverResourceManager routine recovers a resource manager object.
 *
 * \param ResourceManagerHandle Handle to the resource manager object.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRecoverResourceManager(
    _In_ HANDLE ResourceManagerHandle
    );

/**
 * The NtGetNotificationResourceManager routine retrieves a notification from a resource manager object.
 *
 * \param ResourceManagerHandle Handle to the resource manager object.
 * \param TransactionNotification Caller-allocated buffer that receives the transaction notification.
 * \param NotificationLength Size of the TransactionNotification buffer, in bytes.
 * \param Timeout Optional timeout.
 * \param ReturnLength Optional pointer to a variable that receives the number of bytes returned or required.
 * \param Asynchronous Indicates whether the request is asynchronous.
 * \param AsynchronousContext Optional asynchronous context value.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetNotificationResourceManager(
    _In_ HANDLE ResourceManagerHandle,
    _Out_ PTRANSACTION_NOTIFICATION TransactionNotification,
    _In_ ULONG NotificationLength,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_opt_ PULONG ReturnLength,
    _In_ ULONG Asynchronous,
    _In_opt_ ULONG_PTR AsynchronousContext
    );

/**
 * The NtQueryInformationResourceManager routine queries information about a resource manager object.
 *
 * \param ResourceManagerHandle Handle to the resource manager object.
 * \param ResourceManagerInformationClass The type of resource manager information to query.
 * \param ResourceManagerInformation Caller-allocated buffer that receives the requested information.
 * \param ResourceManagerInformationLength The size of the buffer pointed to by ResourceManagerInformation, in bytes.
 * \param ReturnLength Optional pointer to a variable that receives the number of bytes returned or required.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationResourceManager(
    _In_ HANDLE ResourceManagerHandle,
    _In_ RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
    _Out_writes_bytes_(ResourceManagerInformationLength) PVOID ResourceManagerInformation,
    _In_ ULONG ResourceManagerInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

/**
 * The NtSetInformationResourceManager routine sets information for a resource manager object.
 *
 * \param ResourceManagerHandle Handle to the resource manager object.
 * \param ResourceManagerInformationClass The type of resource manager information to set.
 * \param ResourceManagerInformation Pointer to the input buffer containing the information to set.
 * \param ResourceManagerInformationLength The size of the input buffer, in bytes.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationResourceManager(
    _In_ HANDLE ResourceManagerHandle,
    _In_ RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
    _In_reads_bytes_(ResourceManagerInformationLength) PVOID ResourceManagerInformation,
    _In_ ULONG ResourceManagerInformationLength
    );

/**
 * The NtRegisterProtocolAddressInformation routine registers protocol address information with a resource manager.
 *
 * \param ResourceManager Handle to the resource manager object.
 * \param ProtocolId The protocol identifier.
 * \param ProtocolInformationSize Size of the protocol information buffer, in bytes.
 * \param ProtocolInformation Pointer to the protocol information buffer.
 * \param CreateOptions Optional registration flags.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRegisterProtocolAddressInformation(
    _In_ HANDLE ResourceManager,
    _In_ PCRM_PROTOCOL_ID ProtocolId,
    _In_ ULONG ProtocolInformationSize,
    _In_ PVOID ProtocolInformation,
    _In_opt_ ULONG CreateOptions
    );

/**
 * The NtPropagationComplete routine marks a propagation request as completed.
 *
 * \param ResourceManagerHandle Handle to the resource manager object.
 * \param RequestCookie The propagation request cookie.
 * \param BufferLength Size of the completion buffer, in bytes.
 * \param Buffer Pointer to the completion buffer.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPropagationComplete(
    _In_ HANDLE ResourceManagerHandle,
    _In_ ULONG RequestCookie,
    _In_ ULONG BufferLength,
    _In_ PVOID Buffer
    );

/**
 * The NtPropagationFailed routine marks a propagation request as failed.
 *
 * \param ResourceManagerHandle Handle to the resource manager object.
 * \param RequestCookie The propagation request cookie.
 * \param PropStatus The failure status to record.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPropagationFailed(
    _In_ HANDLE ResourceManagerHandle,
    _In_ ULONG RequestCookie,
    _In_ NTSTATUS PropStatus
    );

/**
 * The NtFreezeTransactions routine freezes KTM transactions.
 *
 * \param FreezeTimeout Optional timeout for freezing transactions.
 * \param ThawTimeout Optional timeout for thawing transactions.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreezeTransactions(
    _In_ PLARGE_INTEGER FreezeTimeout,
    _In_ PLARGE_INTEGER ThawTimeout
    );

/**
 * The NtThawTransactions routine thaws KTM transactions.
 *
 * \param Void No parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtThawTransactions(
    VOID
    );

#endif // _NTTMAPI_H
