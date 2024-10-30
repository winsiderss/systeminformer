/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015-2016
 *     dmex    2017-2024
 *
 */

#ifndef _PH_APIIMPORT_H
#define _PH_APIIMPORT_H

EXTERN_C_START

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

typedef NTSTATUS (NTAPI* _NtSetInformationVirtualMemory)(
    _In_ HANDLE ProcessHandle,
    _In_ VIRTUAL_MEMORY_INFORMATION_CLASS VmInformationClass,
    _In_ ULONG_PTR NumberOfEntries,
    _In_reads_(NumberOfEntries) PMEMORY_RANGE_ENTRY VirtualAddresses,
    _In_reads_bytes_(VmInformationLength) PVOID VmInformation,
    _In_ ULONG VmInformationLength
    );

typedef NTSTATUS (NTAPI* _NtCreateProcessStateChange)(
    _Out_ PHANDLE ProcessStateChangeHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ProcessHandle,
    _In_opt_ ULONG64 Reserved
    );

typedef NTSTATUS (NTAPI* _NtChangeProcessState)(
    _In_ HANDLE ProcessStateChangeHandle,
    _In_ HANDLE ProcessHandle,
    _In_ PROCESS_STATE_CHANGE_TYPE StateChangeType,
    _In_opt_ PVOID ExtendedInformation,
    _In_opt_ SIZE_T ExtendedInformationLength,
    _In_opt_ ULONG64 Reserved
    );

typedef NTSTATUS (NTAPI* _NtCopyFileChunk)(
    _In_ HANDLE SourceHandle,
    _In_ HANDLE DestinationHandle,
    _In_opt_ HANDLE EventHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG Length,
    _In_ PLARGE_INTEGER SourceOffset,
    _In_ PLARGE_INTEGER DestOffset,
    _In_opt_ PULONG SourceKey,
    _In_opt_ PULONG DestKey,
    _In_ ULONG Flags
    );

typedef NTSTATUS (NTAPI* _RtlDefaultNpAcl)(
    _Out_ PACL* Acl
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
    _Out_ PWSTR* ppszPath
    );

typedef NTSTATUS (WINAPI* _PssNtCaptureSnapshot)(
    _Out_ PHANDLE SnapshotHandle,
    _In_ HANDLE ProcessHandle,
    _In_ PSSNT_CAPTURE_FLAGS CaptureFlags,
    _In_opt_ ULONG ThreadContextFlags
    );

typedef NTSTATUS (WINAPI* _PssNtFreeSnapshot)(
    _In_ HANDLE SnapshotHandle
    );

typedef NTSTATUS (WINAPI* _PssNtFreeRemoteSnapshot)(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE SnapshotHandle
    );

typedef NTSTATUS (WINAPI* _PssNtQuerySnapshot)(
    _In_ HANDLE SnapshotHandle,
    _In_ PSSNT_QUERY_INFORMATION_CLASS InformationClass,
    _Out_writes_bytes_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength
    );

typedef NTSTATUS (WINAPI* _NtPssCaptureVaSpaceBulk)(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ PNTPSS_MEMORY_BULK_INFORMATION BulkInformation,
    _In_ SIZE_T BulkInformationLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

typedef BOOLEAN (WINAPI* _LdrControlFlowGuardEnforcedWithExportSuppression)(
    VOID
    );

// Advapi32

typedef BOOL (WINAPI* _ConvertSecurityDescriptorToStringSecurityDescriptorW)(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ DWORD RequestedStringSDRevision,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Outptr_ LPWSTR* StringSecurityDescriptor,
    _Out_opt_ PULONG StringSecurityDescriptorLen
    );

typedef BOOL (WINAPI* _ConvertStringSecurityDescriptorToSecurityDescriptorW)(
    _In_ LPCWSTR StringSecurityDescriptor,
    _In_ DWORD StringSDRevision,
    _Outptr_ PSECURITY_DESCRIPTOR *SecurityDescriptor,
    _Out_opt_ PULONG SecurityDescriptorSize
    );

// Shlwapi

typedef HRESULT (WINAPI* _SHAutoComplete)(
    _In_ HWND hwndEdit,
    _In_ ULONG Flags
    );

typedef ULONG (WINAPI* _PssCaptureSnapshot)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG CaptureFlags,
    _In_opt_ ULONG ThreadContextFlags,
    _Out_ HANDLE* SnapshotHandle
    );

typedef ULONG (WINAPI* _PssFreeSnapshot)(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE SnapshotHandle
    );

typedef ULONG (WINAPI* _PssQuerySnapshot)(
    _In_ HANDLE SnapshotHandle,
    _In_ ULONG InformationClass,
    _Out_writes_bytes_(BufferLength) void* Buffer,
    _In_ DWORD BufferLength
    );

typedef LONG (WINAPI* _DnsQuery_W)(
    _In_ PWSTR Name,
    _In_ USHORT Type,
    _In_ ULONG Options,
    _Inout_opt_ PVOID Extra,
    _Outptr_result_maybenull_ PVOID* DnsRecordList,
    _Outptr_opt_result_maybenull_ PVOID* Reserved
    );

typedef LONG (WINAPI* _DnsQueryEx)(
    _In_ PVOID pQueryRequest,
    _Inout_ PVOID pQueryResults,
    _Inout_opt_ PVOID pCancelHandle
    );

typedef LONG (WINAPI* _DnsCancelQuery)(
    _In_ PVOID pCancelHandle
    );

typedef struct _DNS_MESSAGE_BUFFER* PDNS_MESSAGE_BUFFER;

typedef LONG (WINAPI* _DnsExtractRecordsFromMessage_W)(
    _In_ PDNS_MESSAGE_BUFFER DnsBuffer,
    _In_ USHORT MessageLength,
    _Out_ PVOID* DnsRecordList
    );

typedef BOOL (WINAPI* _DnsWriteQuestionToBuffer_W)(
    _Inout_ PDNS_MESSAGE_BUFFER DnsBuffer,
    _Inout_ PULONG BufferSize,
    _In_ PWSTR Name,
    _In_ USHORT Type,
    _In_ USHORT Xid,
    _In_ BOOL RecursionDesired
    );

typedef VOID (WINAPI* _DnsFree)(
    _Pre_opt_valid_ _Frees_ptr_opt_ PVOID Data,
    _In_ ULONG FreeType
    );

// Userenv

typedef BOOL (WINAPI* _CreateEnvironmentBlock)(
    _At_((PZZWSTR*)Environment, _Outptr_) PVOID* Environment,
    _In_opt_ HANDLE TokenHandle,
    _In_ BOOL Inherit
    );

typedef BOOL (WINAPI* _DestroyEnvironmentBlock)(
    _In_ PVOID Environment
    );

// User32

typedef BOOL (WINAPI* _SetWindowDisplayAffinity)(
    _In_ HWND WindowHandle,
    _In_ ULONG Affinity
    );

typedef enum _CONSOLECONTROL CONSOLECONTROL;

typedef NTSTATUS (NTAPI* _ConsoleControl)(
    _In_ CONSOLECONTROL Command,
    _In_reads_bytes_(ConsoleInformationLength) PVOID ConsoleInformation,
    _In_ ULONG ConsoleInformationLength
    );

typedef ULONG (WINAPI *_NotifyServiceStatusChangeW)(
    _In_ SC_HANDLE hService,
    _In_ DWORD dwNotifyMask,
    _In_ PSERVICE_NOTIFYW pNotifyBuffer
    );

typedef ULONG (WINAPI* _SubscribeServiceChangeNotifications)(
    _In_ SC_HANDLE hService,
    _In_ SC_EVENT_TYPE eEventType,
    _In_ PSC_NOTIFICATION_CALLBACK pCallback,
    _In_opt_ PVOID pCallbackContext,
    _Out_ PSC_NOTIFICATION_REGISTRATION* pSubscription
    );

typedef VOID (WINAPI* _UnsubscribeServiceChangeNotifications)(
    _In_ PSC_NOTIFICATION_REGISTRATION pSubscription
    );

#define PH_DECLARE_IMPORT(Name) _##Name Name##_Import(VOID)

// Ntdll

PH_DECLARE_IMPORT(NtQueryInformationEnlistment);
PH_DECLARE_IMPORT(NtQueryInformationResourceManager);
PH_DECLARE_IMPORT(NtQueryInformationTransaction);
PH_DECLARE_IMPORT(NtQueryInformationTransactionManager);
PH_DECLARE_IMPORT(NtSetInformationVirtualMemory);
PH_DECLARE_IMPORT(NtCreateProcessStateChange);
PH_DECLARE_IMPORT(NtChangeProcessState);
PH_DECLARE_IMPORT(NtCopyFileChunk);
PH_DECLARE_IMPORT(LdrControlFlowGuardEnforcedWithExportSuppression);

PH_DECLARE_IMPORT(RtlDefaultNpAcl);
PH_DECLARE_IMPORT(RtlGetTokenNamedObjectPath);
PH_DECLARE_IMPORT(RtlGetAppContainerNamedObjectPath);
PH_DECLARE_IMPORT(RtlGetAppContainerSidType);
PH_DECLARE_IMPORT(RtlGetAppContainerParent);
PH_DECLARE_IMPORT(RtlDeriveCapabilitySidsFromName);

PH_DECLARE_IMPORT(PssNtCaptureSnapshot);
PH_DECLARE_IMPORT(PssNtQuerySnapshot);
PH_DECLARE_IMPORT(PssNtFreeSnapshot);
PH_DECLARE_IMPORT(PssNtFreeRemoteSnapshot);
PH_DECLARE_IMPORT(NtPssCaptureVaSpaceBulk);

// Advapi32

PH_DECLARE_IMPORT(ConvertSecurityDescriptorToStringSecurityDescriptorW);
PH_DECLARE_IMPORT(ConvertStringSecurityDescriptorToSecurityDescriptorW);

// Shlwapi

PH_DECLARE_IMPORT(SHAutoComplete);

PH_DECLARE_IMPORT(CreateEnvironmentBlock);
PH_DECLARE_IMPORT(DestroyEnvironmentBlock);
PH_DECLARE_IMPORT(GetAppContainerRegistryLocation);
PH_DECLARE_IMPORT(GetAppContainerFolderPath);

// User32

PH_DECLARE_IMPORT(SetWindowDisplayAffinity);
PH_DECLARE_IMPORT(ConsoleControl);

EXTERN_C_END

#endif
