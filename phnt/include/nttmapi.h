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
 * \remarks For an owned volatile manager, use TRANSACTION_MANAGER_VOLATILE (0x1) with
 * no log filename. The entry rejects a nonempty log name for that option; durable managers have
 * separate logging and recovery requirements. This handle is not the same object type as a transaction or RM handle.
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
 * \remarks Opens an existing manager by the supplied identity/name inputs and requested
 * access; it does not create a recovery log. An owned volatile manager can be reopened by the TmIdentity
 * returned by TransactionManagerBasicInformation. Close each opened handle independently.
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
 * \remarks Recovery is a durable-log operation, not a mandatory initialization step
 * for every manager. The helper rejects a volatile manager with status 0xC019003B. Do not infer
 * successful recovery from successful volatile manager creation or substitute global transaction thawing.
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
 * \remarks TransactionManagerBasicInformation requires exactly 24 bytes on
 * the tm.sys entry. Its identity can be used for a subsequent manager open; a short basic query is
 * not a supported sizing-success result. Other classes have independent sizes and durable-log requirements.
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
 * \remarks The entry accepts class 3 only with input length zero and
 * asks the manager helper to test log writability. This is not a general setter for basic identity or virtual-clock
 * fields returned by manager queries; unrelated information classes are rejected.
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
 * Creates a KTM transaction to group operations by participating resource managers. This is not
 * NtCreateRegistryTransaction's registry-only object type. The ntoskrnl entry forwards to tm.sys;
 * ownership/outcome must be managed separately.
 *
 * \param TransactionHandle Required output receiving a caller-owned handle on success. Close with NtClose.
 * Last transaction-handle close before commit rolls back the active transaction; closing a participating key
 * handle is not a commit.
 * \param DesiredAccess Nonzero requested rights. Query uses 0x1, set uses 0x2, enlistment uses 0x4, commit
 * uses 0x8 and rollback uses 0x10. The synchronous commit/rollback entries additionally require SYNCHRONIZE
 * (0x100000).
 * \param ObjectAttributes Optional Object Manager attributes/security, or NULL.
 * \param Uow Optional GUID; NULL requests an automatically generated UOW identifier. Query
 * TransactionBasicInformation to obtain it for NtOpenTransaction. The native user-mode capture checks a
 * supplied GUID for 4-byte alignment.
 * \param TmHandle Optional manager handle. Use NULL for ordinary user-mode creation: the tm.sys entry rejects
 * any non-NULL manager for a user-mode caller. Kernel-mode manager association follows a separate
 * access-checked path.
 * \param CreateOptions Creation flags. The entry admits only 0 or TRANSACTION_DO_NOT_PROMOTE (0x1); other
 * bits return STATUS_INVALID_PARAMETER. Accepted creation options do not imply support for every promotion or
 * resource-manager operation.
 * \param IsolationLevel Reserved by the public contract; pass 0.
 * \param IsolationFlags Reserved by the public contract; pass 0.
 * \param Timeout Optional transaction expiration time, not a timeout on this creation call. Units are 100 ns:
 * negative is relative, positive is absolute. NULL or 0 requests no deadline at creation. If the transaction
 * is still active when its deadline expires, KTM initiates rollback.
 * \param Description Optional counted Unicode description copied by KTM. The create path accepts at most 128
 * bytes (64 WCHARs) in Length and rejects 130 bytes. Exclude any terminator from the counted byte length.
 * \return STATUS_SUCCESS or an NTSTATUS failure. Initialize outputs and check status; native capture need not
 * clear a failed result and WoW64 copyback differs.
 * \remarks Keep key/transaction handles and transaction completion separate. KTM commit/rollback with Wait =
 * FALSE can return STATUS_PENDING; that is not proof of a completed outcome. Resource-manager enlistment,
 * recovery, and concurrent completion have separate contracts.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwcreatetransaction
 * \remarks On the tm.sys entry, a user-mode caller must pass NULL for TmHandle; a non-NULL handle returns
 * STATUS_INVALID_PARAMETER even when it refers to a valid owned transaction manager. The kernel-mode path has
 * separate support for an explicit manager. Resource-manager ownership is selected separately when creating
 * an enlistment.
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
 * Opens an existing KTM transaction by its unit-of-work GUID. Use the identifier
 * returned by TransactionBasicInformation; this does not create or reset it.
 * Unlike NtOpenRegistryTransaction, lookup is not by an Object Manager name.
 *
 * \param TransactionHandle Required output receiving a separately owned handle
 * on success. Close it with NtClose. Initialize before calling and check status.
 * \param DesiredAccess Nonzero requested transaction rights. Query, set, commit,
 * rollback and synchronous-wait requirements are independent rights.
 * \param ObjectAttributes Optional handle attributes, not a transaction-name
 * selector or a replacement security descriptor for an existing object.
 * \param Uow Required readable GUID identifying the existing transaction; NULL
 * returns STATUS_INVALID_PARAMETER. Native user-mode capture requires 4-byte
 * GUID alignment.
 * \param TmHandle Optional manager to scope lookup; NULL selects the global
 * unbound-transaction namespace. Explicit-manager
 * lookup has separate manager access/state checks.
 * \return STATUS_SUCCESS or failure, including STATUS_TRANSACTION_NOT_FOUND for
 * an unknown UOW. DesiredAccess = 0 returns STATUS_INVALID_PARAMETER.
 * \remarks A second handle shares the transaction and outcome. A failed native
 * lookup can preserve the caller's handle sentinel; the WoW64 wrapper
 * copies back its NULL temporary after the native call. Do not use output without
 * checking status. Registry-only transaction handles have no KTM UOW query here.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwopentransaction
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
 * Queries KTM identity, outcome, properties or enlistment metadata. It is not a query interface for
 * registry-only lightweight transaction objects.
 *
 * \param TransactionHandle KTM handle with TRANSACTION_QUERY_INFORMATION (0x1).
 * \param TransactionInformationClass Selector. Native tm.sys accepts 0 (basic), 1 (properties), 2
 * (enlistments), 3 (superior enlistment) and 5 (private DTC data); 4 (bind) is not a query class there. WoW64
 * dispatches only 0, 1 and 4, so its forwarded class 4 is rejected by native code and native classes 2/3/5
 * are unavailable through that thunk. Do not remove SDK selectors based on this.
 * \param TransactionInformation Caller-owned output. Use naturally aligned storage; the native public records
 * are probed at 4-byte alignment.
 * \param TransactionInformationLength Capacity in bytes. Basic requires exactly 24 bytes, not merely at least
 * 24. Superior information requires exactly 32. Properties requires at least 32; its description begins at
 * offset 24, and the complete size is max(32, 24 + DescriptionLength). Enlistments requires at least 36 bytes
 * even for zero entries; each returned pair occupies 32 bytes after the 4-byte count. Private DTC data has a
 * separate opaque byte-buffer contract.
 * \param ReturnLength Optional returned/required size, only meaningful for the actual completed path. Native
 * too-small properties/enlistments below the fixed minimum return 0 here, not a usable allocation size. Early
 * length/access/class failures can leave it unchanged. WoW64 properties has separate early checks.
 * \return STATUS_SUCCESS, STATUS_INFO_LENGTH_MISMATCH for exact-size failures, STATUS_BUFFER_TOO_SMALL below
 * native variable-record minima, STATUS_BUFFER_OVERFLOW for native partial variable records, or another
 * status.
 * \remarks Basic State was 1 for active, committed and rolled-back transactions Use Outcome (1 undetermined,
 * 2 committed, 3 aborted), not State alone, to determine completion in these paths. Properties
 * DescriptionLength is bytes, not WCHARs, and remains full on overflow. No general NUL-termination or
 * padding-content guarantee should be inferred.
 *
 * WoW64 properties rejects NULL output or capacity below 32 with STATUS_INFO_LENGTH_MISMATCH before native
 * query, and copies fields/description back only on NT_SUCCESS: overflow can update ReturnLength without
 * returning the native partial record. Its temporary buffer permits unaligned properties output, but basic
 * output is forwarded and still requires native alignment. Private DTC selectors are deprecated internal
 * interfaces. Query only complete, capacity-bounded returned fields.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwqueryinformationtransaction
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
 * Updates KTM transaction properties. Use TransactionPropertiesInformation to change its
 * description/deadline; do not use the Outcome field to commit/abort.
 *
 * \param TransactionHandle KTM handle with TRANSACTION_SET_INFORMATION (0x2).
 * \param TransactionInformationClass Public property selector 1. The native implementation additionally
 * accepts private/deprecated bind (4) and DTC opaque data (5), not basic (0), enlistments (2) or superior
 * (3).
 * \param TransactionInformation Readable caller-owned input copied before normal handle access validation.
 * Native properties/bind input requires 4-byte alignment; the WoW64 properties helper copies to aligned
 * temporary storage and accepts unaligned source input. Prefer natural alignment and do not rely on copyback.
 * \param TransactionInformationLength Properties needs at least 32 bytes and enough space for
 * DescriptionLength bytes beginning at offset 24. Description may be at most 128 bytes. Bind needs exactly 8
 * native bytes (one manager handle); WoW64 accepts at least 4 source bytes and widens the handle. Private DTC
 * data accepts 1 through 1024 native bytes; these private operations are not general application interfaces.
 * \return STATUS_SUCCESS or failure. Properties shorter than 32 returns STATUS_INFO_LENGTH_MISMATCH. A
 * declared description beyond the supplied input returns STATUS_INVALID_PARAMETER natively but
 * STATUS_INFO_LENGTH_MISMATCH from the WoW64 precheck; description length above 128 is rejected by native
 * code.
 * \remarks Properties is not a wholesale replacement of every field. Outcome is ignored by the setter.
 * DescriptionLength = 0 clears description, but Timeout = 0 preserves an existing timeout rather than
 * disabling it. Nonzero isolation fields overwrite existing fields on the internal path; public callers
 * should keep reserved isolation fields 0. The helper can update the description before a later timer
 * operation fails, so do not assume universal all-or-nothing mutation on failure. Use the explicit outcome
 * APIs.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwsetinformationtransaction
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
 * Initiates commit of a KTM transaction. This is distinct from the registry-only NtCommitRegistryTransaction
 * API and its reserved Flags parameter.
 *
 * \param TransactionHandle KTM transaction handle with TRANSACTION_COMMIT (0x8). The native entry
 * additionally requires SYNCHRONIZE (0x100000) when Wait is nonzero: the synchronous access mask is 0x100008.
 * A commit-only handle can request asynchronous commit but is denied for synchronous commit.
 * \param Wait TRUE waits for outcome completion; FALSE permits asynchronous completion. Empty transactions
 * can complete immediately even with FALSE.
 * \return STATUS_SUCCESS, STATUS_PENDING for an operation still in progress, or an error. STATUS_PENDING
 * satisfies NT_SUCCESS but is not completed commit. Terminal paths return
 * STATUS_TRANSACTION_ALREADY_COMMITTED after commit and STATUS_TRANSACTION_ALREADY_ABORTED after rollback.
 * \remarks For asynchronous calls, wait/query outcome using appropriately authorized handles before treating
 * changes as committed. Registry-only handles are rejected with STATUS_OBJECT_TYPE_MISMATCH. Close every
 * handle independently; completion does not close it. Superior, coordinator, resource-manager, and recovery
 * paths can return additional failures.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwcommittransaction
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
 * Initiates rollback of a KTM transaction, not a registry-only lightweight one.
 *
 * \param TransactionHandle KTM handle with TRANSACTION_ROLLBACK (0x10). The native entry additionally
 * requires SYNCHRONIZE (0x100000) for nonzero Wait, giving synchronous access mask 0x100010. A rollback-only
 * handle succeeds for asynchronous rollback but is denied with Wait = TRUE.
 * \param Wait TRUE waits for completion; FALSE permits asynchronous rollback.
 * \return STATUS_SUCCESS, STATUS_PENDING when completion remains outstanding, or an NTSTATUS failure.
 * Repeated rollback of an already aborted empty KTM transaction succeeds; rollback after commit returns
 * STATUS_TRANSACTION_ALREADY_COMMITTED. Registry-only repeated rollback differs.
 * \remarks STATUS_PENDING is not completed rollback even though NT_SUCCESS is true. Query/wait for the
 * outcome when needed. Deadline expiration and last- transaction-handle close can also initiate rollback, but
 * closing an individual participating key handle does not complete the transaction. The caller still owns and
 * must close any transaction handles retained after explicit rollback.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwrollbacktransaction
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
 * \remarks Binds an RM to a transaction: native access checks require RM enlist access 0x8 and transaction
 * enlist access 0x4. NotificationMask combinations are validated as a protocol, not independent
 * subscriptions. A normal enlistment requesting rollback alone is rejected; prepare/commit/rollback (0xE) is
 * accepted for asynchronous rollback.
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
 * \remarks Reopens an enlistment by its GUID within an RM. The entry rejects a NULL GUID
 * or zero DesiredAccess before lookup. EnlistmentBasicInformation supplies the identifier; it does not grant
 * rights to advance the transaction protocol or replace the ownership check.
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
 * \remarks The entry requires exactly 48 bytes for basic or CRM information.
 * Recovery-information queries instead return the stored byte length; a short buffer returns
 * STATUS_BUFFER_TOO_SMALL with the required length and no partial recovery blob. Query access is 0x1.
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
 * \remarks The setter accepts only EnlistmentRecoveryInformation, requires
 * nonzero length no greater than 0x10000 bytes, and requires enlistment access 0x2. This stores participant
 * recovery data; it does not prepare, commit or roll back the transaction. Transaction state is checked separately.
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
 * \remarks Acknowledges completion of rollback for an enlistment; it is not interchangeable
 * with initiating rollback on the transaction. The entry requires enlistment access 0x8 and captures
 * the optional virtual clock before forwarding to the state-machine helper.
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
 * \remarks Creates an RM associated with a transaction manager. The entry requires
 * manager access 0x10, accepts CreateOptions within 0x3 and limits a supplied description to 128 bytes.
 * A volatile RM is useful for an in-memory participant without creating durable recovery files.
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
 * \param ResourceManagerGuid Required GUID of the resource manager object to open.
 * \param ObjectAttributes Optional object attributes for the resource manager object name and security settings.
 * \return NTSTATUS Successful or errant status.
 * \remarks ResourceManagerGuid is required on the entry: NULL or zero DesiredAccess
 * returns STATUS_INVALID_PARAMETER before handle lookup. ObjectAttributes is not an alternative to supplying
 * the GUID. Reopen only the intended manager's RM and close the returned ordinary handle with NtClose.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenResourceManager(
    _Out_ PHANDLE ResourceManagerHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE TmHandle,
    _In_ LPGUID ResourceManagerGuid,
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
 * \remarks Retrieves participant protocol notifications, not generic completion
 * records. Use a bounded timeout and inspect status before reading TransactionNotification or TransactionKey.
 * Completing an owned rollback notification requires the corresponding NtRollbackComplete acknowledgement;
 * dequeuing the notification alone does not complete that participant's protocol.
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
 * \remarks Only ResourceManagerBasicInformation is accepted by the entry. It requires at least 24 bytes; shorter buffers return STATUS_BUFFER_TOO_SMALL and ReturnLength zero.
 * With room for the fixed record, required length is max(24, 20 + DescriptionLength), and a truncated description
 * returns STATUS_BUFFER_OVERFLOW. DescriptionLength counts bytes, not WCHARs or a terminating NUL.
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
