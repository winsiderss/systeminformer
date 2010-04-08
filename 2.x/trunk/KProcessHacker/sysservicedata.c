/*
 * Process Hacker Driver - 
 *   system service logging (data)
 * 
 * Copyright (C) 2009 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _SYSSERVICEDATA_PRIVATE
#include "include/sysservicedata.h"

PVOID KphpSsCallEntryAllocateRoutine(
    __in PRTL_GENERIC_TABLE Table,
    __in CLONG ByteSize
    );

RTL_GENERIC_COMPARE_RESULTS KphpSsCallEntryCompareRoutine(
    __in PRTL_GENERIC_TABLE Table,
    __in PVOID FirstStruct,
    __in PVOID SecondStruct
    );

VOID KphpSsCallEntryFreeRoutine(
    __in PRTL_GENERIC_TABLE Table,
    __in PVOID Buffer
    );

KPHSS_CALL_ENTRY SsEntries[] =
{
    /* NTSTATUS NtAddAtom(PWSTR String, ULONG StringLength, PUSHORT Atom) */
    { &SsNtAddAtom, "NtAddAtom", 3, { WStringArgument, 0, Int16Argument } },
    /* NTSTATUS NtAlertResumeThread(HANDLE ThreadHandle, PULONG PreviousSuspendCount) */
    { &SsNtAlertResumeThread, "NtAlertResumeThread", 2, { HandleArgument, 0 } },
    /* NTSTATUS NtAlertThread(HANDLE ThreadHandle) */
    { &SsNtAlertThread, "NtAlertThread", 1, { HandleArgument } },
    /* NTSTATUS NtAllocateLocallyUniqueId(PLUID Luid) */
    { &SsNtAllocateLocallyUniqueId, "NtAllocateLocallyUniqueId", 1, { 0 } },
    /* NTSTATUS NtAllocateUserPhysicalPages(HANDLE ProcessHandle, PULONG NumberOfPages, PULONG PageFrameNumbers) */
    { &SsNtAllocateUserPhysicalPages, "NtAllocateUserPhysicalPages", 3, { HandleArgument, Int32Argument, 0 } },
    /* NTSTATUS NtAllocateUuids(PLARGE_INTEGER UuidLastTimeAllocated, PULONG UuidDeltaTime, PULONG UuidSequenceNumber, 
     *      PUCHAR UuidSeed) */
    { &SsNtAllocateUuids, "NtAllocateUuids", 4, { Int64Argument, 0, 0, 0 } },
    /* NTSTATUS NtAllocateVirtualMemory(HANDLE ProcessHandle, PVOID *BaseAddress, ULONG ZeroBits, 
     *      PULONG AllocationSize, ULONG AllocationType, ULONG Protect) */
    { &SsNtAllocateVirtualMemory, "NtAllocateVirtualMemory", 6, { HandleArgument, Int32Argument, 0, Int32Argument, 0, 0 } },
    /* NTSTATUS NtApphelpCacheControl(APPHELPCACHECONTROL ApphelpCacheControl, PUNICODE_STRING ApphelpCacheObject) */
    { &SsNtApphelpCacheControl, "NtApphelpCacheControl", 2, { 0, UnicodeStringArgument } },
    /* NTSTATUS NtAreMappedFilesTheSame(PVOID Address1, PVOID Address2) */
    { &SsNtAreMappedFilesTheSame, "NtAreMappedFilesTheSame", 2, { 0, 0 } },
    /* NTSTATUS NtAssignProcessToJobObject(HANDLE JobHandle, HANDLE ProcessHandle) */
    { &SsNtAssignProcessToJobObject, "NtAssignProcessToJobObject", 2, { HandleArgument, HandleArgument } },
    /* NTSTATUS NtCallbackReturn(PVOID Result, ULONG ResultLength, NTSTATUS Status) */
    { &SsNtCallbackReturn, "NtCallbackReturn", 3, { 0, 0, 0 } },
    /* NTSTATUS NtCancelDeviceWakeupRequest(HANDLE DeviceHandle) */
    { &SsNtCancelDeviceWakeupRequest, "NtCancelDeviceWakeupRequest", 1, { HandleArgument } },
    /* NTSTATUS NtCancelIoFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock) */
    { &SsNtCancelIoFile, "NtCancelIoFile", 2, { HandleArgument, 0 } },
    /* NTSTATUS NtCancelTimer(HANDLE TimerHandle, PBOOLEAN CurrentState) */
    { &SsNtCancelTimer, "NtCancelTimer", 2, { HandleArgument, 0 } },
    /* NTSTATUS NtClearEvent(HANDLE EventHandle) */
    { &SsNtClearEvent, "NtClearEvent", 1, { HandleArgument } },
    /* NTSTATUS NtClose(HANDLE Handle) */
    { &SsNtClose, "NtClose", 1, { HandleArgument } },
    /* NTSTATUS NtContinue(PCONTEXT Context, BOOLEAN TestAlert) */
    { &SsNtContinue, "NtContinue", 2, { ContextArgument, 0 } },
    /* NTSTATUS NtCreateDebugObject(PHANDLE DebugObjectHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      ULONG Flags) */
    { &SsNtCreateDebugObject, "NtCreateDebugObject", 4, { 0, 0, ObjectAttributesArgument, 0 } },
    /* NTSTATUS NtCreateDirectoryObject(PHANDLE DirectoryHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) */
    { &SsNtCreateDirectoryObject, "NtCreateDirectoryObject", 3, { 0, 0, ObjectAttributesArgument } },
    /* NTSTATUS NtCreateEvent(PHANDLE EventHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      EVENT_TYPE EventType, BOOLEAN InitialState) */
    { &SsNtCreateEvent, "NtCreateEvent", 5, { 0, 0, ObjectAttributesArgument, 0, 0 } },
    /* NTSTATUS NtCreateEventPair(PHANDLE EventPairHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) */
    { &SsNtCreateEventPair, "NtCreateEventPair", 3, { 0, 0, ObjectAttributesArgument } },
    /* NTSTATUS NtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, 
     *      ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, 
     *      PVOID EaBuffer, ULONG EaLength) */
    { &SsNtCreateFile, "NtCreateFile", 11, { 0, 0, ObjectAttributesArgument, 0, Int64Argument, 0, 0, 0, 0, 0, 0 } },
    /* NTSTATUS NtCreateIoCompletion(PHANDLE IoCompletionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      ULONG NumberOfConcurrentThreads) */
    { &SsNtCreateIoCompletion, "NtCreateIoCompletion", 4, { 0, 0, ObjectAttributesArgument, 0 } },
    /* NTSTATUS NtCreateJobObject(PHANDLE JobHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) */
    { &SsNtCreateJobObject, "NtCreateJobObject", 3, { 0, 0, ObjectAttributesArgument } },
    /* NTSTATUS NtCreateJobSet(ULONG NumJob, IN PJOB_SET_ARRAY UserJobSet, IN ULONG Flags) */
    { &SsNtCreateJobSet, "NtCreateJobSet", 3, { 0, 0, 0 } },
    /* NTSTATUS NtCreateKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      ULONG TitleIndex, PUNICODE_STRING Class, ULONG CreateOptions, 
     *      PULONG Disposition) */
    { &SsNtCreateKey, "NtCreateKey", 7, { 0, 0, ObjectAttributesArgument, 0, UnicodeStringArgument, 0, 0 } },
    /* NTSTATUS NtCreateKeyedEvent(PHANDLE KeyedEventHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      ULONG Flags) */
    { &SsNtCreateKeyedEvent, "NtCreateKeyedEvent", 4, { 0, 0, ObjectAttributesArgument, 0 } },
    /* NTSTATUS NtCreateMailslotFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      PIO_STATUS_BLOCK IoStatusBlock, ULONG CreateOptions, ULONG MailslotQuota, 
     *      ULONG MaximumMessageSize, PLARGE_INTEGER ReadTimeout) */
    { &SsNtCreateMailslotFile, "NtCreateMailslotFile", 8, { 0, 0, ObjectAttributesArgument, 0, 0, 0, 0, Int64Argument } },
    /* NTSTATUS NtCreateMutant(PHANDLE MutantHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      BOOLEAN InitialOwner) */
    { &SsNtCreateMutant, "NtCreateMutant", 4, { 0, 0, ObjectAttributesArgument, 0 } },
    /* NTSTATUS NtCreateNamedPipeFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG CreateDisposition, 
     *      ULONG CreateOptions, BOOLEAN TypeMessage, BOOLEAN ReadmodeMessage, 
     *      BOOLEAN Nonblocking, ULONG MaxInstances, ULONG InBufferSize, 
     *      ULONG OutBufferSize, PLARGE_INTEGER DefaultTimeout) */
    { &SsNtCreateNamedPipeFile, "NtCreateNamedPipeFile", 14, { 0, 0, ObjectAttributesArgument, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Int64Argument } },
    /* NTSTATUS NtCreatePagingFile(PUNICODE_STRING FileName, PULARGE_INTEGER MinimumSize, PULARGE_INTEGER MaximumSize, 
     *      ULONG Priority) */
    { &SsNtCreatePagingFile, "NtCreatePagingFile", 4, { UnicodeStringArgument, Int64Argument, Int64Argument, 0 } },
    /* NTSTATUS NtCreatePort(PHANDLE PortHandle, POBJECT_ATTRIBUTES ObjectAttributes, ULONG MaxConnectionInfoLength, 
     *      ULONG MaxMessageLength, ULONG MaxPoolUsage) */
    { &SsNtCreatePort, "NtCreatePort", 5, { 0, ObjectAttributesArgument, 0, 0, 0 } },
    /* NTSTATUS NtCreatePrivateNamespace(PHANDLE PrivateNamespaceHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      PBOUNDARY_DESCRIPTOR BoundaryDescriptor) */
    { &SsNtCreatePrivateNamespace, "NtCreatePrivateNamespace", 4, { 0, 0, ObjectAttributesArgument, 0 } },
    /* NTSTATUS NtCreateProcess(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      HANDLE InheritFromProcessHandle, BOOLEAN InheritHandles, HANDLE SectionHandle, 
     *      HANDLE DebugPort, HANDLE ExceptionPort) */
    { &SsNtCreateProcess, "NtCreateProcess", 8, { 0, 0, ObjectAttributesArgument, HandleArgument, 0, HandleArgument, HandleArgument, HandleArgument } },
    /* NTSTATUS NtCreateProcessEx(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      HANDLE ParentProcess, ULONG Flags, HANDLE SectionHandle, 
     *      HANDLE DebugPort, HANDLE ExceptionPort, ULONG JobMemberLevel */
    { &SsNtCreateProcessEx, "NtCreateProcessEx", 9, { 0, 0, ObjectAttributesArgument, HandleArgument, 0, HandleArgument, HandleArgument, HandleArgument, 0 } },
    /* NTSTATUS NtCreateProfile(PHANDLE ProfileHandle, HANDLE ProcessHandle, PVOID Base, 
     *      ULONG Size, ULONG BucketShift, PULONG Buffer, 
     *      ULONG BufferLength, KPROFILE_SOURCE Source, ULONG ProcessorMask) */
    { &SsNtCreateProfile, "NtCreateProfile", 9, { 0, HandleArgument, 0, 0, 0, 0, 0, 0, 0 } },
    /* NTSTATUS NtCreateSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      PLARGE_INTEGER SectionSize, ULONG Protect, ULONG Attributes, 
     *      HANDLE FileHandle) */
    { &SsNtCreateSection, "NtCreateSection", 7, { 0, 0, ObjectAttributesArgument, Int64Argument, 0, 0, HandleArgument } },
    /* NTSTATUS NtCreateSemaphore(PHANDLE SemaphoreHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      LONG InitialCount, LONG MaximumCount) */
    { &SsNtCreateSemaphore, "NtCreateSemaphore", 5, { 0, 0, ObjectAttributesArgument, 0, 0 } },
    /* NTSTATUS NtCreateSymbolicLinkObject(PHANDLE SymbolicLinkHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      PUNICODE_STRING TargetName) */
    { &SsNtCreateSymbolicLinkObject, "NtCreateSymbolicLinkObject", 4, { 0, 0, ObjectAttributesArgument, UnicodeStringArgument } },
    /* NTSTATUS NtCreateThread(PHANDLE ThreadHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      HANDLE ProcessHandle, PCLIENT_ID ClientId, PCONTEXT ThreadContext, 
     *      PINITIAL_TEB UserStack, BOOLEAN CreateSuspended) */
    { &SsNtCreateThread, "NtCreateThread", 8, { 0, 0, ObjectAttributesArgument, HandleArgument, 0, ContextArgument, InitialTebArgument, 0 } },
    /* NTSTATUS NtCreateTimer(PHANDLE TimerHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      TIMER_TYPE TimerType) */
    { &SsNtCreateTimer, "NtCreateTimer", 4, { 0, 0, ObjectAttributesArgument, 0 } },
    /* NTSTATUS NtCreateToken(PHANDLE TokenHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      TOKEN_TYPE Type, PLUID AuthenticationId, PLARGE_INTEGER ExpirationTime, 
     *      PTOKEN_USER User, PTOKEN_GROUPS Groups, PTOKEN_PRIVILEGES Privileges, 
     *      PTOKEN_OWNER Owner, PTOKEN_PRIMARY_GROUP PrimaryGroup, PTOKEN_DEFAULT_DACL DefaultDacl, 
     *      PTOKEN_SOURCE Source) */
    { &SsNtCreateToken, "NtCreateToken", 13, { 0, 0, ObjectAttributesArgument, 0, Int64Argument, Int64Argument, 0, 0, 0, 0, 0, 0, 0 } },
    /* NTSTATUS NtCreateWaitablePort(PHANDLE PortHandle, POBJECT_ATTRIBUTES ObjectAttributes, ULONG MaxConnectionInfoLength, 
     *      ULONG MaxMessageLength, ULONG MaxPoolUsage) */
    { &SsNtCreateWaitablePort, "NtCreateWaitablePort", 5, { 0, ObjectAttributesArgument, 0, 0, 0 } },
    /* NTSTATUS NtDebugActiveProcess(HANDLE ProcessHandle, HANDLE DebugObjectHandle) */
    { &SsNtDebugActiveProcess, "NtDebugActiveProcess", 2, { HandleArgument, HandleArgument } },
    /* NTSTATUS NtDebugContinue(HANDLE DebugObjectHandle, PCLIENT_ID ClientId, NTSTATUS ContinueStatus) */
    { &SsNtDebugContinue, "NtDebugContinue", 3, { HandleArgument, ClientIdArgument, 0 } },
    /* NTSTATUS NtDelayExecution(BOOLEAN Alertable, PLARGE_INTEGER Interval) */
    { &SsNtDelayExecution, "NtDelayExecution", 2, { 0, Int64Argument } },
    /* NTSTATUS NtDeleteAtom(USHORT Atom) */
    { &SsNtDeleteAtom, "NtDeleteAtom", 1, { 0 } },
    /* NTSTATUS NtDeleteBootEntry(ULONG Id) */
    { &SsNtDeleteBootEntry, "NtDeleteBootEntry", 1, { 0 } },
    /* NTSTATUS NtDeleteDriverEntry(ULONG Id) */
    { &SsNtDeleteDriverEntry, "NtDeleteDriverEntry", 1, { 0 } },
    /* NTSTATUS NtDeleteFile(POBJECT_ATTRIBUTES ObjectAttributes) */
    { &SsNtDeleteFile, "NtDeleteFile", 1, { ObjectAttributesArgument } },
    /* NTSTATUS NtDeleteKey(HANDLE KeyHandle) */
    { &SsNtDeleteKey, "NtDeleteKey", 1, { HandleArgument } },
    /* NTSTATUS NtDeleteObjectAuditAlarm(PUNICODE_STRING SubsystemName, PVOID HandleId, BOOLEAN GenerateOnClose) */
    { &SsNtDeleteObjectAuditAlarm, "NtDeleteObjectAuditAlarm", 3, { UnicodeStringArgument, 0, 0 } },
    /* NTSTATUS NtDeletePrivateNamespace(HANDLE PrivateNamespaceHandle) */
    { &SsNtDeletePrivateNamespace, "NtDeletePrivateNamespace", 1, { HandleArgument } },
    /* NTSTATUS NtDeleteValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName) */
    { &SsNtDeleteValueKey, "NtDeleteValueKey", 2, { HandleArgument, UnicodeStringArgument } },
    /* NTSTATUS NtDeviceIoControlFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, 
     *      PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG IoControlCode, 
     *      PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, 
     *      ULONG OutputBufferLength) */
    { &SsNtDeviceIoControlFile, "NtDeviceIoControlFile", 10, { HandleArgument, HandleArgument, 0, 0, 0, 0, 0, 0, 0, 0 } },
    /* NTSTATUS NtDisplayString(PUNICODE_STRING String) */
    { &SsNtDisplayString, "NtDisplayString", 1, { UnicodeStringArgument } },
    /* NTSTATUS NtDuplicateObject(HANDLE SourceProcessHandle, HANDLE SourceHandle, HANDLE TargetProcessHandle, 
     *      PHANDLE TargetHandle, ACCESS_MASK DesiredAccess, ULONG Attributes, 
     *      ULONG Options) */
    { &SsNtDuplicateObject, "NtDuplicateObject", 7, { HandleArgument, HandleArgument, HandleArgument, 0, 0, 0, 0 } },
    /* NTSTATUS NtDuplicateToken(HANDLE ExistingTokenHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      BOOLEAN EffectiveOnly, TOKEN_TYPE TokenType, PHANDLE NewTokenHandle) */
    { &SsNtDuplicateToken, "NtDuplicateToken", 6, { HandleArgument, 0, ObjectAttributesArgument, 0, 0, 0 } },
    /* NTSTATUS NtEnumerateBootEntries(PVOID Buffer, PULONG BufferLength) */
    { &SsNtEnumerateBootEntries, "NtEnumerateBootEntries", 2, { 0, Int32Argument } },
    /* NTSTATUS NtEnumerateDriverEntries(PVOID Buffer, PULONG BufferLength) */
    { &SsNtEnumerateDriverEntries, "NtEnumerateDriverEntries", 2, { 0, Int32Argument } },
    /* NTSTATUS NtEnumerateKey(HANDLE KeyHandle, ULONG Index, KEY_INFORMATION_CLASS KeyInformationClass, 
     *      PVOID KeyInformation, ULONG KeyInformationLength, PULONG ResultLength) */
    { &SsNtEnumerateKey, "NtEnumerateKey", 6, { HandleArgument, 0, 0, 0, 0, 0 } },
    /* NTSTATUS NtEnumerateSystemEnvironmentValuesEx(ULONG InformationClass, PVOID Buffer, PULONG BufferLength) */
    { &SsNtEnumerateSystemEnvironmentValuesEx, "NtEnumerateSystemEnvironmentValuesEx", 3, { 0, 0, Int32Argument } },
    /* NTSTATUS NtEnumerateValueKey(HANDLE KeyHandle, ULONG Index, KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, 
     *      PVOID KeyValueInformation, ULONG KeyValueInformationLength, PULONG ResultLength) */
    { &SsNtEnumerateValueKey, "NtEnumerateValueKey", 6, { HandleArgument, 0, 0, 0, 0, 0 } },
    /* NTSTATUS NtExtendSection(HANDLE SectionHandle, PLARGE_INTEGER SectionSize) */
    { &SsNtExtendSection, "NtExtendSection", 2, { HandleArgument, Int64Argument } },
    /* NTSTATUS NtFilterToken(HANDLE ExistingTokenHandle, ULONG Flags, PTOKEN_GROUPS SidsToDisable, 
     *      PTOKEN_PRIVILEGES PrivilegesToDelete, PTOKEN_GROUPS SidsToRestricted, PHANDLE NewTokenHandle) */
    { &SsNtFilterToken, "NtFilterToken", 6, { HandleArgument, 0, 0, 0, 0, 0 } },
    /* NTSTATUS NtFindAtom(PWSTR String, ULONG StringLength, PUSHORT Atom) */
    { &SsNtFindAtom, "NtFindAtom", 3, { WStringArgument, 0, 0 } },
    /* NTSTATUS NtFlushBuffersFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock) */
    { &SsNtFlushBuffersFile, "NtFlushBuffersFile", 2, { HandleArgument, 0 } },
    /* NTSTATUS NtFlushInstructionCache(HANDLE ProcessHandle, PVOID BaseAddress, ULONG FlushSize) */
    { &SsNtFlushInstructionCache, "NtFlushInstructionCache", 3, { HandleArgument, 0, 0 } },
    /* NTSTATUS NtFlushKey(HANDLE KeyHandle) */
    { &SsNtFlushKey, "NtFlushKey", 1, { HandleArgument } },
    /* NTSTATUS NtFlushProcessWriteBuffers() */
    { &SsNtFlushProcessWriteBuffers, "NtFlushProcessWriteBuffers", 0 },
    /* NTSTATUS NtFlushVirtualMemory(HANDLE ProcessHandle, PVOID *BaseAddress, PULONG FlushSize, 
     *      PIO_STATUS_BLOCK IoStatusBlock) */
    { &SsNtFlushVirtualMemory, "NtFlushVirtualMemory", 4, { HandleArgument, Int32Argument, Int32Argument, 0 } },
    /* NTSTATUS NtFlushWriteBuffer() */
    { &SsNtFlushWriteBuffer, "NtFlushWriteBuffer", 0 },
    /* NTSTATUS NtFreeUserPhysicalPages(HANDLE ProcessHandle, PULONG NumberOfPages, PULONG PageFrameNumbers) */
    { &SsNtFreeUserPhysicalPages, "NtFreeUserPhysicalPages", 3, { HandleArgument, Int32Argument, 0 } },
    /* NTSTATUS NtFreeVirtualMemory(HANDLE ProcessHandle, PVOID *BaseAddress, PULONG FreeSize, 
     *      ULONG FreeType) */
    { &SsNtFreeVirtualMemory, "NtFreeVirtualMemory", 4, { HandleArgument, Int32Argument, Int32Argument, 0 } },
    /* NTSTATUS NtFsControlFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, 
     *      PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG FsControlCode, 
     *      PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, 
     *      ULONG OutputBufferLength) */
    { &SsNtFsControlFile, "NtFsControlFile", 10, { HandleArgument, HandleArgument, 0, 0, 0, 0, 0, 0, 0, 0 } },
    /* NTSTATUS NtGetContextThread(HANDLE ThreadHandle, PCONTEXT Context) */
    { &SsNtGetContextThread, "NtGetContextThread", 2, { HandleArgument, ContextArgument } },
    /* NTSTATUS NtGetCurrentProcessorNumber() */
    { &SsNtGetCurrentProcessorNumber, "NtGetCurrentProcessorNumber", 0 },
    /* NTSTATUS NtGetDevicePowerState(HANDLE DeviceHandle, PDEVICE_POWER_STATE DevicePowerState) */
    { &SsNtGetDevicePowerState, "NtGetDevicePowerState", 2, { HandleArgument, 0 } },
    /* NTSTATUS NtGetNextProcess(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess, ULONG HandleAttributes, 
     *      ULONG Flags, PHANDLE NewProcessHandle) */
    { &SsNtGetNextProcess, "NtGetNextProcess", 5, { HandleArgument, 0, 0, 0, 0 } },
    /* NTSTATUS NtGetNextThread(HANDLE ProcessHandle, HANDLE ThreadHandle, ACCESS_MASK DesiredAccess, 
     *      ULONG HandleAttributes, ULONG Flags, PHANDLE NewThreadHandle) */
    { &SsNtGetNextThread, "NtGetNextThread", 6, { HandleArgument, HandleArgument, 0, 0, 0, 0 } },
    /* NTSTATUS NtGetPlugPlayEvent(HANDLE EventHandle, PVOID Context, PVOID Buffer, 
     *      ULONG BufferLength) */
    { &SsNtGetPlugPlayEvent, "NtGetPlugPlayEvent", 4, { HandleArgument, 0, 0, 0 } },
    /* NTSTATUS NtGetWriteWatch(HANDLE ProcessHandle, ULONG Flags, PVOID BaseAddress, 
     *      ULONG RegionSize, PULONG Buffer, PULONG BufferEntries, 
     *      PULONG Granularity) */
    { &SsNtGetWriteWatch, "NtGetWriteWatch", 7, { HandleArgument, 0, 0, 0, 0, 0, 0 } },
    /* NTSTATUS NtImpersonateAnonymousToken(HANDLE ThreadHandle) */
    { &SsNtImpersonateAnonymousToken, "NtImpersonateAnonymousToken", 1, { HandleArgument } },
    /* NTSTATUS NtImpersonateClientOfPort(HANDLE PortHandle, PPORT_MESSAGE Message) */
    { &SsNtImpersonateClientOfPort, "SsNtImpersonateClientOfPort", 2, { HandleArgument, 0 } },
    /* NTSTATUS NtImpersonateThread(HANDLE ThreadHandle, HANDLE TargetThreadHandle, PSECURITY_QUALITY_OF_SERVICE SecurityQos) */
    { &SsNtImpersonateThread, "NtImpersonateThread", 3, { HandleArgument, HandleArgument, 0 } },
    /* NTSTATUS NtInitiatePowerAction(POWER_ACTION SystemAction, SYSTEM_POWER_STATE MinSystemState, ULONG Flags, 
     *      BOOLEAN Asynchronous) */
    { &SsNtInitiatePowerAction, "NtInitiatePowerAction", 4, { 0, 0, 0, 0 } },
    /* NTSTATUS NtIsProcessInJob(HANDLE ProcessHandle, HANDLE JobHandle) */
    { &SsNtIsProcessInJob, "NtIsProcessInJob", 2, { HandleArgument, HandleArgument } },
    /* NTSTATUS NtIsSystemResumeAutomatic() */
    { &SsNtIsSystemResumeAutomatic, "NtIsSystemResumeAutomatic", 0 },
    /* NTSTATUS NtListenPort(HANDLE PortHandle, PPORT_MESSAGE Message) */
    { &SsNtListenPort, "NtListenPort", 2, { HandleArgument, 0 } },
    /* NTSTATUS NtLoadDriver(PUNICODE_STRING DriverServiceName) */
    { &SsNtLoadDriver, "NtLoadDriver", 1, { UnicodeStringArgument } },
    /* NTSTATUS NtLoadKey(POBJECT_ATTRIBUTES KeyObjectAttributes, POBJECT_ATTRIBUTES FileObjectAttributes) */
    { &SsNtLoadKey, "NtLoadKey", 2, { ObjectAttributesArgument, ObjectAttributesArgument } },
    /* NTSTATUS NtLoadKey2(POBJECT_ATTRIBUTES KeyObjectAttributes, POBJECT_ATTRIBUTES FileObjectAttributes, ULONG Flags) */
    { &SsNtLoadKey2, "NtLoadKey2", 3, { ObjectAttributesArgument, ObjectAttributesArgument, 0 } },
    /* NTSTATUS NtLockFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, 
     *      PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PULARGE_INTEGER LockOffset, 
     *      PULARGE_INTEGER LockLength, ULONG Key, BOOLEAN FailImmediately, 
     *      BOOLEAN ExclusiveLock) */
    { &SsNtLockFile, "NtLockFile", 10, { HandleArgument, HandleArgument, 0, 0, 0, Int64Argument, Int64Argument, 0, 0, 0 } },
    /* NTSTATUS NtLockVirtualMemory(HANDLE ProcessHandle, PVOID *BaseAddress, PULONG LockSize, 
     *      ULONG LockType) */
    { &SsNtLockVirtualMemory, "NtLockVirtualMemory", 4, { HandleArgument, Int32Argument, Int32Argument, 0 } },
    /* NTSTATUS NtMakePermanentObject(HANDLE Handle) */
    { &SsNtMakePermanentObject, "NtMakePermanentObject", 1, { HandleArgument } },
    /* NTSTATUS NtMakeTemporaryObject(HANDLE Handle) */
    { &SsNtMakeTemporaryObject, "NtMakeTemporaryObject", 1, { HandleArgument } },
    /* NTSTATUS NtMapUserPhysicalPages(PVOID BaseAddress, PULONG NumberOfPages, PULONG PageFrameNumbers) */
    { &SsNtMapUserPhysicalPages, "NtMapUserPhysicalPages", 3, { 0, Int32Argument, 0 } },
    /* NTSTATUS NtMapUserPhysicalPagesScatter(PVOID BaseAddress, PULONG NumberOfPages, PULONG PageFrameNumbers) */
    { &SsNtMapUserPhysicalPagesScatter, "NtMapUserPhysicalPagesScatter", 3, { 0, Int32Argument, 0 } },
    /* NTSTATUS NtMapViewOfSection(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID *BaseAddress, 
     *      ULONG ZeroBits, ULONG CommitSize, PLARGE_INTEGER SectionOffset, 
     *      PULONG ViewSize, SECTION_INHERIT InheritDisposition, ULONG AllocationType, 
     *      ULONG Protect) */
    { &SsNtMapViewOfSection, "NtMapViewOfSection", 10, { HandleArgument, HandleArgument, Int32Argument, 0, 0, Int64Argument, Int32Argument, 0, 0, 0 } },
    /* NTSTATUS NtModifyBootEntry(PBOOT_ENTRY BootEntry) */
    { &SsNtModifyBootEntry, "NtModifyBootEntry", 1, { 0 } },
    /* NTSTATUS NtModifyDriverEntry(PEFI_DRIVER_ENTRY DriverEntry) */
    { &SsNtModifyDriverEntry, "NtModifyDriverEntry", 1, { 0 } },
    /* NTSTATUS NtNotifyChangeDirectoryFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, 
     *      PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PFILE_NOTIFY_INFORMATION Buffer, 
     *      ULONG BufferLength, ULONG NotifyFilter, BOOLEAN WatchSubtree) */
    { &SsNtNotifyChangeDirectoryFile, "NtNotifyChangeDirectoryFile", 9, { HandleArgument, HandleArgument, 0, 0, 0, 0, 0, 0, 0 } },
    /* NTSTATUS NtNotifyChangeKey(HANDLE KeyHandle, HANDLE EventHandle, PIO_APC_ROUTINE ApcRoutine, 
     *      PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG NotifyFilter, 
     *      BOOLEAN WatchSubtree, PVOID Buffer, ULONG BufferLength, 
     *      BOOLEAN Asynchronous) */
    { &SsNtNotifyChangeKey, "NtNotifyChangeKey", 10, { HandleArgument, HandleArgument, 0, 0, 0, 0, 0, 0, 0, 0 } },
    /* NTSTATUS NtNotifyChangeMultipleKeys(HANDLE KeyHandle, ULONG Flags, POBJECT_ATTRIBUTES KeyObjectAttributes, 
     *      HANDLE EventHandle, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, 
     *      PIO_STATUS_BLOCK IoStatusBlock, ULONG NotifyFilter, BOOLEAN WatchSubtree, 
     *      PVOID Buffer, ULONG BufferLength, BOOLEAN Asynchronous) */
    { &SsNtNotifyChangeMultipleKeys, "NtNotifyChangeMultipleKeys", 12, { HandleArgument, 0, ObjectAttributesArgument, HandleArgument, 0, 0, 0, 0, 0, 0, 0, 0 } },
    /* NTSTATUS NtOpenDirectoryObject(PHANDLE DirectoryHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) */
    { &SsNtOpenDirectoryObject, "NtOpenDirectoryObject", 3, { 0, 0, ObjectAttributesArgument } },
    /* NTSTATUS NtOpenEvent(PHANDLE EventHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) */
    { &SsNtOpenEvent, "NtOpenEvent", 3, { 0, 0, ObjectAttributesArgument } },
    /* NTSTATUS NtOpenEventPair(PHANDLE EventPairHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) */
    { &SsNtOpenEventPair, "NtOpenEventPair", 3, { 0, 0, ObjectAttributesArgument } },
    /* NTSTATUS NtOpenFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions) */
    { &SsNtOpenFile, "NtOpenFile", 6, { 0, 0, ObjectAttributesArgument, 0, 0, 0 } },
    /* NTSTATUS NtOpenIoCompletion(PHANDLE IoCompletionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) */
    { &SsNtOpenIoCompletion, "NtOpenIoCompletion", 3, { 0, 0, ObjectAttributesArgument } },
    /* NTSTATUS NtOpenJobObject(PHANDLE JobHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) */
    { &SsNtOpenJobObject, "NtOpenJobObject", 3, { 0, 0, ObjectAttributesArgument } },
    /* NTSTATUS NtOpenKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) */
    { &SsNtOpenKey, "NtOpenKey", 3, { 0, 0, ObjectAttributesArgument } },
    /* NTSTATUS NtOpenKeyedEvent(PHANDLE KeyedEventHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) */
    { &SsNtOpenKeyedEvent, "NtOpenKeyedEvent", 3, { 0, 0, ObjectAttributesArgument } },
    /* NTSTATUS NtOpenMutant(PHANDLE MutantHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) */
    { &SsNtOpenMutant, "NtOpenMutant", 3, { 0, 0, ObjectAttributesArgument } },
    /* NTSTATUS NtOpenObjectAuditAlarm(PUNICODE_STRING SubsystemName, PVOID *HandleId, PUNICODE_STRING ObjectTypeName, 
     *      PUNICODE_STRING ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, HANDLE TokenHandle, 
     *      ACCESS_MASK DesiredAccess, ACCESS_MASK GrantedAccess, PPRIVILEGE_SET Privileges, 
     *      BOOLEAN ObjectCreation, BOOLEAN AccessGranted, PBOOLEAN GenerateOnClose) */
    { &SsNtOpenObjectAuditAlarm, "NtOpenObjectAuditAlarm", 12, { UnicodeStringArgument, Int32Argument, UnicodeStringArgument, UnicodeStringArgument, 0, HandleArgument, 0, 0, 0, 0, 0, 0 } },
    /* NTSTATUS NtOpenProcess(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      PCLIENT_ID ClientId) */
    { &SsNtOpenProcess, "NtOpenProcess", 4, { 0, 0, ObjectAttributesArgument, ClientIdArgument } },
    /* NTSTATUS NtOpenProcessToken(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess, PHANDLE TokenHandle) */
    { &SsNtOpenProcessToken, "NtOpenProcessToken", 3, { HandleArgument, 0, 0 } },
    /* NTSTATUS NtOpenProcessTokenEx(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess, ULONG HandleAttributes, 
     *      PHANDLE TokenHandle) */
    { &SsNtOpenProcessTokenEx, "NtOpenProcessTokenEx", 4, { HandleArgument, 0, 0, 0 } },
    /* NTSTATUS NtOpenSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) */
    { &SsNtOpenSection, "NtOpenSection", 3, { 0, 0, ObjectAttributesArgument } },
    /* NTSTATUS NtOpenSemaphore(PHANDLE SemaphoreHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) */
    { &SsNtOpenSemaphore, "NtOpenSemaphore", 3, { 0, 0, ObjectAttributesArgument } },
    /* NTSTATUS NtOpenSymbolicLinkObject(PHANDLE SymbolicLinkHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) */
    { &SsNtOpenSymbolicLinkObject, "NtOpenSymbolicLinkObject", 3, { 0, 0, ObjectAttributesArgument } },
    /* NTSTATUS NtOpenThread(PHANDLE ThreadHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, 
     *      PCLIENT_ID ClientId) */
    { &SsNtOpenThread, "NtOpenThread", 4, { 0, 0, ObjectAttributesArgument, ClientIdArgument } },
    /* NTSTATUS NtOpenThreadToken(HANDLE ThreadHandle, ACCESS_MASK DesiredAccess, BOOLEAN OpenAsSelf, 
     *      PHANDLE TokenHandle) */
    { &SsNtOpenThreadToken, "NtOpenThreadToken", 4, { HandleArgument, 0, 0, 0 } },
    /* NTSTATUS NtOpenThreadTokenEx(HANDLE ThreadHandle, ACCESS_MASK DesiredAccess, BOOLEAN OpenAsSelf, 
     *      ULONG HandleAttributes, PHANDLE TokenHandle) */
    { &SsNtOpenThreadTokenEx, "NtOpenThreadTokenEx", 5, { HandleArgument, 0, 0, 0, 0 } },
    /* NTSTATUS NtOpenTimer(PHANDLE TimerHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes) */
    { &SsNtOpenTimer, "NtOpenTimer", 3, { 0, 0, ObjectAttributesArgument } },
    /* NTSTATUS NtReadFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, 
     *      PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, 
     *      ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key) */
    { &SsNtReadFile, "NtReadFile", 9, { HandleArgument, HandleArgument, 0, 0, 0, 0, 0, Int64Argument, Int32Argument } },
    /* NTSTATUS NtWriteFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, 
     *      PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, 
     *      ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key) */
    { &SsNtWriteFile, "NtWriteFile", 9, { HandleArgument, HandleArgument, 0, 0, 0, 0, 0, Int64Argument, Int32Argument } },
    
    { NULL, "Dummy", 0 }
};

RTL_GENERIC_TABLE KphSsCallTable;
FAST_MUTEX KphSsCallTableMutex;

/* KphSsDataInit
 * 
 * Initializes all data structures so that system service entries 
 * can be looked up.
 */
VOID KphSsDataInit()
{
    ULONG i;
    
    RtlInitializeGenericTable(
        &KphSsCallTable,
        KphpSsCallEntryCompareRoutine,
        KphpSsCallEntryAllocateRoutine,
        KphpSsCallEntryFreeRoutine,
        NULL
        );
    
    for (i = 0; i < sizeof(SsEntries) / sizeof(KPHSS_CALL_ENTRY); i++)
    {
        /* Ignore the dummy entry. */
        if (SsEntries[i].Number)
        {
            RtlInsertElementGenericTable(
                &KphSsCallTable,
                &SsEntries[i],
                /* Save some space... */
                FIELD_OFFSET(KPHSS_CALL_ENTRY, Arguments) + 
                    SsEntries[i].NumberOfArguments * sizeof(KPHSS_ARGUMENT_TYPE),
                NULL
                );
        }
    }
    
    ExInitializeFastMutex(&KphSsCallTableMutex);
}

/* KphSsDataDeinit
 * 
 * Frees all memory associated with system service data.
 */
VOID KphSsDataDeinit()
{
    PKPHSS_CALL_ENTRY callEntry;
    
    while (callEntry = (PKPHSS_CALL_ENTRY)RtlGetElementGenericTable(&KphSsCallTable, 0))
        RtlDeleteElementGenericTable(&KphSsCallTable, callEntry);
}

/* KphSsLookupCallEntry
 * 
 * Lookups up a system service entry by system service number.
 */
PKPHSS_CALL_ENTRY KphSsLookupCallEntry(
    __in ULONG Number
    )
{
    KPHSS_CALL_ENTRY callEntry;
    PKPHSS_CALL_ENTRY foundEntry;
    
    callEntry.Number = &Number;
    
    ExAcquireFastMutex(&KphSsCallTableMutex);
    foundEntry = (PKPHSS_CALL_ENTRY)RtlLookupElementGenericTable(
        &KphSsCallTable,
        &callEntry
        );
    ExReleaseFastMutex(&KphSsCallTableMutex);
    
    return foundEntry;
}

/* KphpSsCallEntryAllocateRoutine
 * 
 * Allocates storage for a system service entry.
 */
PVOID KphpSsCallEntryAllocateRoutine(
    __in PRTL_GENERIC_TABLE Table,
    __in CLONG ByteSize
    )
{
    return ExAllocatePoolWithTag(
        PagedPool,
        ByteSize,
        TAG_CALL_ENTRY
        );
}

/* KphpSsCallEntryCompareRoutine
 * 
 * Compares two system service entries.
 */
RTL_GENERIC_COMPARE_RESULTS KphpSsCallEntryCompareRoutine(
    __in PRTL_GENERIC_TABLE Table,
    __in PVOID FirstStruct,
    __in PVOID SecondStruct
    )
{
    PKPHSS_CALL_ENTRY callEntry1, callEntry2;
    
    callEntry1 = (PKPHSS_CALL_ENTRY)FirstStruct;
    callEntry2 = (PKPHSS_CALL_ENTRY)SecondStruct;
    
    if (*(callEntry1->Number) < *(callEntry2->Number))
        return GenericLessThan;
    else if (*(callEntry1->Number) > *(callEntry2->Number))
        return GenericGreaterThan;
    else
        return GenericEqual;
}

/* KphpSsCallEntryFreeRoutine
 * 
 * Frees storage for a system service entry.
 */
VOID KphpSsCallEntryFreeRoutine(
    __in PRTL_GENERIC_TABLE Table,
    __in PVOID Buffer
    )
{
    ExFreePoolWithTag(
        Buffer,
        TAG_CALL_ENTRY
        );
}
