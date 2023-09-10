/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015-2016
 *     dmex    2017-2023
 *
 */

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

typedef NTSTATUS (NTAPI *_NtTraceControl)(
    _In_ TRACE_CONTROL_INFORMATION_CLASS TraceInformationClass,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(TraceInformationLength) PVOID TraceInformation,
    _In_ ULONG TraceInformationLength,
    _Out_ PULONG ReturnLength
    );

typedef NTSTATUS (NTAPI *_NtQueryOpenSubKeysEx)(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_ ULONG BufferLength,
    _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
    _Out_ PULONG RequiredSize
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
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
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

typedef BOOL (WINAPI* _CreateEnvironmentBlock)(
    _At_((PZZWSTR*)Environment, _Outptr_) PVOID* Environment,
    _In_opt_ HANDLE TokenHandle,
    _In_ BOOL Inherit
    );

typedef BOOL (WINAPI* _DestroyEnvironmentBlock)(
    _In_ PVOID Environment
    );

typedef BOOL (WINAPI* _MessageBeep)(
    _In_ UINT uType
    );

typedef BOOL (WINAPI* _SetWindowDisplayAffinity)(
    _In_ HWND WindowHandle,
    _In_ ULONG Affinity
    );

typedef BOOLEAN (WINAPI* _WinStationQueryInformationW)(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ ULONG WinStationInformationClass,
    _Out_writes_bytes_(WinStationInformationLength) PVOID pWinStationInformation,
    _In_ ULONG WinStationInformationLength,
    _Out_ PULONG pReturnLength
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

PH_DECLARE_IMPORT(NtQueryInformationEnlistment);
PH_DECLARE_IMPORT(NtQueryInformationResourceManager);
PH_DECLARE_IMPORT(NtQueryInformationTransaction);
PH_DECLARE_IMPORT(NtQueryInformationTransactionManager);
PH_DECLARE_IMPORT(NtQueryDefaultLocale);
PH_DECLARE_IMPORT(NtQueryDefaultUILanguage);
PH_DECLARE_IMPORT(NtTraceControl);
PH_DECLARE_IMPORT(NtQueryOpenSubKeysEx);
PH_DECLARE_IMPORT(NtSetInformationVirtualMemory);
PH_DECLARE_IMPORT(NtCreateProcessStateChange);
PH_DECLARE_IMPORT(NtChangeProcessState);

PH_DECLARE_IMPORT(RtlDefaultNpAcl);
PH_DECLARE_IMPORT(RtlGetTokenNamedObjectPath);
PH_DECLARE_IMPORT(RtlGetAppContainerNamedObjectPath);
PH_DECLARE_IMPORT(RtlGetAppContainerSidType);
PH_DECLARE_IMPORT(RtlGetAppContainerParent);
PH_DECLARE_IMPORT(RtlDeriveCapabilitySidsFromName);

PH_DECLARE_IMPORT(ConvertSecurityDescriptorToStringSecurityDescriptorW);
PH_DECLARE_IMPORT(ConvertStringSecurityDescriptorToSecurityDescriptorW);

PH_DECLARE_IMPORT(DnsQuery_W);
PH_DECLARE_IMPORT(DnsExtractRecordsFromMessage_W);
PH_DECLARE_IMPORT(DnsWriteQuestionToBuffer_W);
PH_DECLARE_IMPORT(DnsFree);

PH_DECLARE_IMPORT(SHAutoComplete);

PH_DECLARE_IMPORT(PssCaptureSnapshot);
PH_DECLARE_IMPORT(PssQuerySnapshot);
PH_DECLARE_IMPORT(PssFreeSnapshot);

PH_DECLARE_IMPORT(CreateEnvironmentBlock);
PH_DECLARE_IMPORT(DestroyEnvironmentBlock);
PH_DECLARE_IMPORT(GetAppContainerRegistryLocation);
PH_DECLARE_IMPORT(GetAppContainerFolderPath);

PH_DECLARE_IMPORT(MessageBeep);
PH_DECLARE_IMPORT(SetWindowDisplayAffinity);

PH_DECLARE_IMPORT(WinStationQueryInformationW);

#endif
