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
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Flags,
    __out PHANDLE NewProcessHandle
    );

typedef NTSTATUS (NTAPI *_NtGetNextThread)(
    __in HANDLE ProcessHandle,
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Flags,
    __out PHANDLE NewThreadHandle
    );

EXT _NtGetNextProcess NtGetNextProcess;
EXT _NtGetNextThread NtGetNextThread;
#endif

#if !(PHNT_VERSION >= PHNT_VISTA)

typedef NTSTATUS (NTAPI *_NtQueryInformationEnlistment)(
    __in HANDLE EnlistmentHandle,
    __in ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
    __out_bcount(EnlistmentInformationLength) PVOID EnlistmentInformation,
    __in ULONG EnlistmentInformationLength,
    __out_opt PULONG ReturnLength
    );

typedef NTSTATUS (NTAPI *_NtQueryInformationResourceManager)(
    __in HANDLE ResourceManagerHandle,
    __in RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
    __out_bcount(ResourceManagerInformationLength) PVOID ResourceManagerInformation,
    __in ULONG ResourceManagerInformationLength,
    __out_opt PULONG ReturnLength
    );

typedef NTSTATUS (NTAPI *_NtQueryInformationTransaction)(
    __in HANDLE TransactionHandle,
    __in TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
    __out_bcount(TransactionInformationLength) PVOID TransactionInformation,
    __in ULONG TransactionInformationLength,
    __out_opt PULONG ReturnLength
    );

typedef NTSTATUS (NTAPI *_NtQueryInformationTransactionManager)(
    __in HANDLE TransactionManagerHandle,
    __in TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
    __out_bcount(TransactionManagerInformationLength) PVOID TransactionManagerInformation,
    __in ULONG TransactionManagerInformationLength,
    __out_opt PULONG ReturnLength
    );

EXT _NtQueryInformationEnlistment NtQueryInformationEnlistment;
EXT _NtQueryInformationResourceManager NtQueryInformationResourceManager;
EXT _NtQueryInformationTransaction NtQueryInformationTransaction;
EXT _NtQueryInformationTransactionManager NtQueryInformationTransactionManager;
#endif

BOOLEAN PhInitializeImports();

#endif
