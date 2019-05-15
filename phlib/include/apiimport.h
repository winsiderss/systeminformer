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

typedef NTSTATUS (NTAPI *_NtQueryDefaultLocale)(
    _In_ BOOLEAN UserProfile,
    _Out_ PLCID DefaultLocaleId
    );

typedef NTSTATUS (NTAPI *_NtQueryDefaultUILanguage)(
    _Out_ LANGID* DefaultUILanguageId
    );

typedef NTSTATUS (NTAPI* _RtlGetTokenNamedObjectPath)(
    _In_ HANDLE Token,
    _In_opt_ PSID Sid,
    _Out_ PUNICODE_STRING ObjectPath
    );

typedef NTSTATUS (NTAPI* _RtlGetAppContainerNamedObjectPath)(
    _In_opt_ HANDLE Token,
    _In_opt_ PSID AppContainerSid,
    _In_ BOOLEAN RelativePath,
    _Out_ PUNICODE_STRING ObjectPath
    );

typedef NTSTATUS (NTAPI* _RtlGetAppContainerSidType)(
    _In_ PSID AppContainerSid,
    _Out_ PAPPCONTAINER_SID_TYPE AppContainerSidType
    );

typedef NTSTATUS (NTAPI* _RtlGetAppContainerParent)(
    _In_ PSID AppContainerSid,
    _Out_ PSID* AppContainerSidParent
    );

typedef NTSTATUS (NTAPI* _RtlDeriveCapabilitySidsFromName)(
    _Inout_ PUNICODE_STRING UnicodeString,
    _Out_ PSID CapabilityGroupSid,
    _Out_ PSID CapabilitySid
    );

typedef HRESULT (WINAPI* _GetAppContainerRegistryLocation)(
    _In_ REGSAM desiredAccess,
    _Outptr_ PHKEY phAppContainerKey
    );

typedef HRESULT (WINAPI* _GetAppContainerFolderPath)(
    _In_ PCWSTR pszAppContainerSid,
    _Outptr_ PWSTR* ppszPath
    );

typedef BOOL (WINAPI* _ConvertSecurityDescriptorToStringSecurityDescriptorW)(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ DWORD RequestedStringSDRevision,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Outptr_ LPWSTR* StringSecurityDescriptor,
    _Out_opt_ PULONG StringSecurityDescriptorLen
    );

#define PH_DECLARE_IMPORT(Name) _##Name Name##_Import(VOID)

PH_DECLARE_IMPORT(NtQueryInformationEnlistment);
PH_DECLARE_IMPORT(NtQueryInformationResourceManager);
PH_DECLARE_IMPORT(NtQueryInformationTransaction);
PH_DECLARE_IMPORT(NtQueryInformationTransactionManager);
PH_DECLARE_IMPORT(NtQueryDefaultLocale);
PH_DECLARE_IMPORT(NtQueryDefaultUILanguage);

PH_DECLARE_IMPORT(RtlGetTokenNamedObjectPath);
PH_DECLARE_IMPORT(RtlGetAppContainerNamedObjectPath);
PH_DECLARE_IMPORT(RtlGetAppContainerSidType);
PH_DECLARE_IMPORT(RtlGetAppContainerParent);
PH_DECLARE_IMPORT(RtlDeriveCapabilitySidsFromName);

PH_DECLARE_IMPORT(GetAppContainerRegistryLocation);
PH_DECLARE_IMPORT(GetAppContainerFolderPath);

PH_DECLARE_IMPORT(ConvertSecurityDescriptorToStringSecurityDescriptorW);

#endif
