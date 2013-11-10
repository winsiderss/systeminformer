#ifndef _NTIMPORT_H
#define _NTIMPORT_H

#ifdef _PH_NTIMPORT_PRIVATE
#define EXT DECLSPEC_SELECTANY
#else
#define EXT extern
#endif

// Only functions appearing in Windows XP and below may be
// imported normally. The other functions are imported here.

#if !(PHNT_VERSION >= PHNT_WS03)

typedef NTSTATUS (NTAPI *_NtGetNextProcess)(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG HandleAttributes,
    _In_ ULONG Flags,
    _Out_ PHANDLE NewProcessHandle
    );

typedef NTSTATUS (NTAPI *_NtGetNextThread)(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG HandleAttributes,
    _In_ ULONG Flags,
    _Out_ PHANDLE NewThreadHandle
    );

EXT _NtGetNextProcess NtGetNextProcess;
EXT _NtGetNextThread NtGetNextThread;
#endif

#if !(PHNT_VERSION >= PHNT_VISTA)

typedef NTSTATUS (NTAPI *_NtQueryInformationEnlistment)(
    _In_ HANDLE EnlistmentHandle,
    _In_ ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
    _Out_writes_bytes_(EnlistmentInformationLength) PVOID EnlistmentInformation,
    _In_ ULONG EnlistmentInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

typedef NTSTATUS (NTAPI *_NtQueryInformationResourceManager)(
    _In_ HANDLE ResourceManagerHandle,
    _In_ RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
    _Out_writes_bytes_(ResourceManagerInformationLength) PVOID ResourceManagerInformation,
    _In_ ULONG ResourceManagerInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

typedef NTSTATUS (NTAPI *_NtQueryInformationTransaction)(
    _In_ HANDLE TransactionHandle,
    _In_ TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
    _Out_writes_bytes_(TransactionInformationLength) PVOID TransactionInformation,
    _In_ ULONG TransactionInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

typedef NTSTATUS (NTAPI *_NtQueryInformationTransactionManager)(
    _In_ HANDLE TransactionManagerHandle,
    _In_ TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
    _Out_writes_bytes_(TransactionManagerInformationLength) PVOID TransactionManagerInformation,
    _In_ ULONG TransactionManagerInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

EXT _NtQueryInformationEnlistment NtQueryInformationEnlistment;
EXT _NtQueryInformationResourceManager NtQueryInformationResourceManager;
EXT _NtQueryInformationTransaction NtQueryInformationTransaction;
EXT _NtQueryInformationTransactionManager NtQueryInformationTransactionManager;
#endif

BOOLEAN PhInitializeImports();

#endif
