#ifndef _PH_APIIMPORT_H
#define _PH_APIIMPORT_H

// ntdll

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

// shell32

#if defined(_M_IX86)
#define __unaligned
#endif

typedef HRESULT (WINAPI *_SHCreateShellItem)(
    _In_opt_ const struct _ITEMIDLIST __unaligned *pidlParent,
    _In_opt_ struct IShellFolder *psfParent,
    _In_ const struct _ITEMIDLIST __unaligned *pidl,
    _Out_ struct IShellItem **ppsi
    );

typedef HRESULT (WINAPI *_SHOpenFolderAndSelectItems)(
    _In_ const struct _ITEMIDLIST __unaligned *pidlFolder,
    _In_ UINT cidl,
    _In_reads_opt_(cidl) const struct _ITEMIDLIST __unaligned **apidl,
    _In_ ULONG dwFlags
    );

typedef HRESULT (WINAPI *_SHParseDisplayName)(
    _In_ LPCWSTR pszName,
    _In_opt_ struct IBindCtx *pbc,
    _Out_ const struct _ITEMIDLIST __unaligned **ppidl,
    _In_ ULONG sfgaoIn,
    _Out_ ULONG *psfgaoOut
    );

#define PH_DECLARE_IMPORT(Name) _##Name Name##_Import(VOID)

PH_DECLARE_IMPORT(NtQueryInformationEnlistment);
PH_DECLARE_IMPORT(NtQueryInformationResourceManager);
PH_DECLARE_IMPORT(NtQueryInformationTransaction);
PH_DECLARE_IMPORT(NtQueryInformationTransactionManager);
PH_DECLARE_IMPORT(SHCreateShellItem);
PH_DECLARE_IMPORT(SHOpenFolderAndSelectItems);
PH_DECLARE_IMPORT(SHParseDisplayName);

#endif
