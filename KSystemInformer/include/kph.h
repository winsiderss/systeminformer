/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#pragma once
#include <ntifs.h>
#include <ntintsafe.h>
#include <minwindef.h>
#include <ntstrsafe.h>
#include <fltKernel.h>
#include <ntimage.h>
#include <bcrypt.h>
#include <wsk.h>
#include <pooltags.h>
#define PHNT_MODE PHNT_MODE_KERNEL
#include <phnt.h>
#include <ntfill.h>
#include <ntpebteb.h>
#include <ntldr.h>
#include <ntwow64.h>
#include <kphapi.h>

#define KSIAPI NTAPI

#define PAGED_CODE_PASSIVE()\
    PAGED_CODE()\
    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL)
#define NPAGED_CODE_PASSIVE()\
    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL)
#define NPAGED_CODE_DISPATCH_MAX()\
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL)
#define NPAGED_CODE_DISPATCH_MIN()\
    NT_ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL)

#define PAGED_FILE() \
    __pragma(bss_seg("PAGEBBS"))\
    __pragma(code_seg("PAGE"))\
    __pragma(data_seg("PAGEDATA"))\
    __pragma(const_seg("PAGERO"))

#define KPH_PROTECTED_DATA_SECTION_PUSH() \
    __pragma(data_seg(push))\
    __pragma(data_seg("KSIDATA"))
#define KPH_PROTECTED_DATA_SECTION_POP() \
    __pragma(data_seg(pop))
#define KPH_PROTECTED_DATA_SECTION_RO_PUSH() \
    __pragma(const_seg(push))\
    __pragma(const_seg("KSIRO"))
#define KPH_PROTECTED_DATA_SECTION_RO_POP() \
    __pragma(const_seg(pop))

#define _Outptr_allocatesMem_ _Outptr_result_nullonfailure_ __drv_allocatesMem(Mem)
#define _Out_allocatesMem_ _Out_ __drv_allocatesMem(Mem)
#define _Out_allocatesMem_size_(size) _Out_allocatesMem_ _Post_writable_byte_size_(size)
#define _FreesMem_ _Pre_notnull_ _Post_ptr_invalid_ __drv_freesMem(Mem)
#define _In_freesMem_ _In_ _FreesMem_
#define _In_aliasesMem_ _In_ _Pre_notnull_ _Post_ptr_invalid_ __drv_aliasesMem
#define _Return_allocatesMem_ __drv_allocatesMem(Mem) _Post_maybenull_ _Must_inspect_result_
#define _Return_allocatesMem_size_(size) _Return_allocatesMem_ _Post_writable_byte_size_(size)

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#define InterlockedExchangeAddULongPtr(target, value) (ULONG_PTR)InterlockedExchangeAddSizeT((SIZE_T*)target, (SIZE_T)value)
#define InterlockedIncrementSSizeT(target) (SSIZE_T)InterlockedIncrementSizeT((SIZE_T*)target)
#define InterlockedDecrementSSizeT(target) (SSIZE_T)InterlockedDecrementSizeT((SIZE_T*)target)
#define InterlockedCompareExchangeSizeT(target, value, expected)\
    (SIZE_T)InterlockedCompareExchangePointer((PVOID*)Target, (PVOID)Value, (PVOID)expected)

FORCEINLINE
SIZE_T InterlockedExchangeIfGreaterSizeT(
    _In_ volatile SIZE_T* Target,
    _In_ SIZE_T Value
    )
{
    SIZE_T expected;

    for (;;)
    {
        expected = *Target;

        if (Value <= expected)
        {
            break;
        }

        if (InterlockedCompareExchangeSizeT(Target,
                                            Value,
                                            expected) == expected)
        {
            break;
        }
    }

    return expected;
}

#define ProbeOutputType(pointer, type)\
_Pragma("warning(suppress : 6001)")\
ProbeForWrite(pointer, sizeof(type), TYPE_ALIGNMENT(type))

#define ProbeInputType(pointer, type)\
_Pragma("warning(suppress : 6001)")\
ProbeForRead(pointer, sizeof(type), TYPE_ALIGNMENT(type))

#define C_2sTo4(x) ((unsigned int)(signed short)(x))

#define RebaseUnicodeString(string, oldBase, newBase)\
if ((string)->Buffer)\
{\
    (string)->Buffer = Add2Ptr(newBase, PtrOffset(oldBase, (string)->Buffer));\
}

typedef struct _KPH_SIZED_BUFFER
{
    ULONG Size;
    PBYTE Buffer;
} KPH_SIZED_BUFFER, *PKPH_SIZED_BUFFER;

typedef struct _KPH_FILE_VERSION
{
    USHORT MajorVersion;
    USHORT MinorVersion;
    USHORT BuildNumber;
    USHORT Revision;
} KPH_FILE_VERSION, *PKPH_FILE_VERSION;

// main

extern PDRIVER_OBJECT KphDriverObject;
extern RTL_OSVERSIONINFOEXW KphOsVersionInfo;
extern KPH_FILE_VERSION KphKernelVersion;
extern KPH_INFORMER_SETTINGS KphInformerSettings;
extern BOOLEAN KphIgnoreProtectionSuppression;
extern SYSTEM_SECUREBOOT_INFORMATION KphSecureBootInfo;
extern SYSTEM_CODEINTEGRITY_INFORMATION KphCodeIntegrityInfo;

FORCEINLINE
BOOLEAN KphSuppressProtections(
    VOID
    )
{
    if (KphIgnoreProtectionSuppression)
    {
        return FALSE;
    }

    if (KphSecureBootInfo.SecureBootCapable &&
        KphSecureBootInfo.SecureBootEnabled)
    {
        return FALSE;
    }

    if (!KD_DEBUGGER_ENABLED)
    {
        return FALSE;
    }

    if (!FlagOn(KphCodeIntegrityInfo.CodeIntegrityOptions,
                CODEINTEGRITY_OPTION_TESTSIGN))
    {
        return FALSE;
    }

    return TRUE;
}

// alloc

//
// Always use wrapped allocators, unless explicitly necessary.
//
// This helps catch programmer error. Deprecate the original ones here, the
// allocation infrastructure will internally suppress the deprecated warnings.
//
#pragma deprecated(ExAllocateCacheAwareRundownProtection)
#pragma deprecated(ExAllocateFromLookasideListEx)
#pragma deprecated(ExAllocateFromNPagedLookasideList)
#pragma deprecated(ExAllocateFromPagedLookasideList)
#pragma deprecated(ExAllocatePool)
#pragma deprecated(ExAllocatePool2)
#pragma deprecated(ExAllocatePool3)
#pragma deprecated(ExAllocatePoolPriorityUninitialized)
#pragma deprecated(ExAllocatePoolPriorityZero)
#pragma deprecated(ExAllocatePoolQuotaUninitialized)
#pragma deprecated(ExAllocatePoolQuotaZero)
#pragma deprecated(ExAllocatePoolUninitialized)
#pragma deprecated(ExAllocatePoolWithQuota)
#pragma deprecated(ExAllocatePoolWithQuotaTag)
#pragma deprecated(ExAllocatePoolWithTag)
#pragma deprecated(ExAllocatePoolWithTagPriority)
#pragma deprecated(ExAllocatePoolZero)
#pragma deprecated(ExFreePool)
#pragma deprecated(ExFreePool2)
#pragma deprecated(ExFreePoolWithTag)
#pragma deprecated(ExFreeToLookasideListEx)
#pragma deprecated(ExFreeToNPagedLookasideList)
#pragma deprecated(ExFreeToPagedLookasideList)
#pragma deprecated(ExDeleteLookasideListEx)
#pragma deprecated(ExDeleteNPagedLookasideList)
#pragma deprecated(ExDeletePagedLookasideList)
#pragma deprecated(ExInitializeLookasideListEx)
#pragma deprecated(ExInitializeNPagedLookasideList)
#pragma deprecated(ExInitializePagedLookasideList)

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphInitializeAlloc(
    _In_ PUNICODE_STRING RegistryPath
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Return_allocatesMem_size_(NumberOfBytes)
PVOID KphAllocateNPaged(
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag
    );

_IRQL_requires_max_(APC_LEVEL)
_Return_allocatesMem_size_(NumberOfBytes)
PVOID KphAllocatePaged(
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphFree(
    _FreesMem_ PVOID Memory,
    _In_ ULONG Tag
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphInitializeNPagedLookaside(
    _Out_ PNPAGED_LOOKASIDE_LIST Lookaside,
    _In_ SIZE_T Size,
    _In_ ULONG Tag
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphDeleteNPagedLookaside(
    _Inout_ PNPAGED_LOOKASIDE_LIST Lookaside
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Return_allocatesMem_size_(Lookaside->L.Size)
PVOID KphAllocateFromNPagedLookaside(
    _Inout_ PNPAGED_LOOKASIDE_LIST Lookaside
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphFreeToNPagedLookaside(
    _Inout_ PNPAGED_LOOKASIDE_LIST Lookaside,
    _In_freesMem_ PVOID Memory
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphInitializePagedLookaside(
    _Out_ PPAGED_LOOKASIDE_LIST Lookaside,
    _In_ SIZE_T Size,
    _In_ ULONG Tag
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphDeletePagedLookaside(
    _Inout_ PPAGED_LOOKASIDE_LIST Lookaside
    );

_IRQL_requires_max_(APC_LEVEL)
_Return_allocatesMem_size_(Lookaside->L.Size)
PVOID KphAllocateFromPagedLookaside(
    _Inout_ PPAGED_LOOKASIDE_LIST Lookaside
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphFreeToPagedLookaside(
    _Inout_ PPAGED_LOOKASIDE_LIST Lookaside,
    _In_freesMem_ PVOID Memory
    );

typedef PAGED_LOOKASIDE_LIST KPH_PAGED_LOOKASIDE_OBJECT;
typedef KPH_PAGED_LOOKASIDE_OBJECT* PKPH_PAGED_LOOKASIDE_OBJECT;

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphCreatePagedLookasideObject(
    _Outptr_result_nullonfailure_ PKPH_PAGED_LOOKASIDE_OBJECT* Lookaside,
    _In_ SIZE_T Size,
    _In_ ULONG Tag
    );

_IRQL_requires_max_(APC_LEVEL)
_Return_allocatesMem_size_(Lookaside->L.Size)
PVOID KphAllocateFromPagedLookasideObject(
    _Inout_ PKPH_PAGED_LOOKASIDE_OBJECT Lookaside
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphFreeToPagedLookasideObject(
    _Inout_ PKPH_PAGED_LOOKASIDE_OBJECT Lookaside,
    _In_freesMem_ PVOID Memory
    );

typedef NPAGED_LOOKASIDE_LIST KPH_NPAGED_LOOKASIDE_OBJECT;
typedef KPH_NPAGED_LOOKASIDE_OBJECT* PKPH_NPAGED_LOOKASIDE_OBJECT;

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS KphCreateNPagedLookasideObject(
    _Outptr_result_nullonfailure_ PKPH_NPAGED_LOOKASIDE_OBJECT* Lookaside,
    _In_ SIZE_T Size,
    _In_ ULONG Tag
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Return_allocatesMem_size_(Lookaside->L.Size)
PVOID KphAllocateFromNPagedLookasideObject(
    _Inout_ PKPH_NPAGED_LOOKASIDE_OBJECT Lookaside
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphFreeToNPagedLookasideObject(
    _Inout_ PKPH_NPAGED_LOOKASIDE_OBJECT Lookaside,
    _In_freesMem_ PVOID Memory
    );

// dynimp

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphDynamicImport(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
PVOID KphGetSystemRoutineAddress(
    _In_z_ PCWSTR SystemRoutineName
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
PVOID KphGetRoutineAddress(
    _In_z_ PCWSTR ModuleName,
    _In_z_ PCSTR RoutineName
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphLocateKernelImage(
    _Out_ PVOID* ImageBase,
    _Out_ PSIZE_T ImageSize
    );

// object

#ifdef _X86_
#define KERNEL_HANDLE_BIT (0x80000000)
#else
#define KERNEL_HANDLE_BIT (0xffffffff80000000)
#endif

#define IsKernelHandle(Handle) ((LONG_PTR)(Handle) < 0)
#define MakeKernelHandle(Handle) ((HANDLE)((ULONG_PTR)(Handle) | KERNEL_HANDLE_BIT))
#define IsPseudoHandle(Handle) (((ULONG_PTR)(Handle) <= (ULONG_PTR)-1) &&\
                                ((ULONG_PTR)(Handle) >= (ULONG_PTR)-6))

_Acquires_lock_(Process)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphReferenceProcessHandleTable(
    _In_ PEPROCESS Process,
    _Outptr_result_nullonfailure_ PHANDLE_TABLE* HandleTable
    );

_Releases_lock_(Process)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphDereferenceProcessHandleTable(
    _In_ PEPROCESS Process
    );

typedef
_Function_class_(KPH_ENUM_PROCESS_HANDLES_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN
KSIAPI
KPH_ENUM_PROCESS_HANDLES_CALLBACK(
    _Inout_ PHANDLE_TABLE_ENTRY HandleTableEntry,
    _In_ HANDLE Handle,
    _In_opt_ PVOID Parameter
    );
typedef KPH_ENUM_PROCESS_HANDLES_CALLBACK* PKPH_ENUM_PROCESS_HANDLES_CALLBACK;

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphEnumerateProcessHandlesEx(
    _In_ PEPROCESS Process,
    _In_ PKPH_ENUM_PROCESS_HANDLES_CALLBACK Callback,
    _In_opt_ PVOID Parameter
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphEnumerateProcessHandles(
    _In_ HANDLE ProcessHandle,
    _Out_writes_bytes_(BufferLength) PVOID Buffer,
    _In_opt_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryNameObject(
    _In_ PVOID Object,
    _Out_writes_bytes_(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryNameFileObject(
    _In_ PFILE_OBJECT FileObject,
    _Out_writes_bytes_(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryInformationObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSetInformationObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _In_reads_bytes_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenNamedObject(
    _Out_ PHANDLE ObjectHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ POBJECT_TYPE ObjectType,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphDuplicateObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE SourceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TargetHandle,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCompareObjects(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE FirstObjectHandle,
    _In_ HANDLE SecondObjectHandle,
    _In_ KPROCESSOR_MODE AccessMode
    );

// process

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenProcessToken(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenProcessJob(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE JobHandle,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphTerminateProcess(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _Out_writes_bytes_opt_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSetInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _In_reads_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

// qrydrv

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenDriver(
    _Out_ PHANDLE DriverHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryInformationDriver(
    _In_ HANDLE DriverHandle,
    _In_ DRIVER_INFORMATION_CLASS DriverInformationClass,
    _Out_writes_bytes_opt_(DriverInformationLength) PVOID DriverInformation,
    _In_ ULONG DriverInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

// thread

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenThreadToken(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN OpenAsSelf,
    _Out_ PHANDLE TokenHandle,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenThreadProcess(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE ProcessHandle,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeStackBackTrace(
    VOID
    );

_IRQL_requires_max_(APC_LEVEL)
_Success_(return != 0)
ULONG KphCaptureStackBackTrace(
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _In_opt_ ULONG Flags,
    _Out_writes_(FramesToCapture) PVOID *BackTrace,
    _Out_opt_ PULONG BackTraceHash
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCaptureStackBackTraceThread(
    _In_ PETHREAD Thread,
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID *BackTrace,
    _Out_opt_ PULONG CapturedFrames,
    _Out_opt_ PULONG BackTraceHash,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ ULONG Flags,
    _In_opt_ PLARGE_INTEGER Timeout
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCaptureStackBackTraceThreadByHandle(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID *BackTrace,
    _Out_opt_ PULONG CapturedFrames,
    _Out_opt_ PULONG BackTraceHash,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ ULONG Flags,
    _In_opt_ PLARGE_INTEGER Timeout
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _Out_writes_bytes_opt_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

// util

typedef EX_RUNDOWN_REF KPH_RUNDOWN;
typedef KPH_RUNDOWN* PKPH_RUNDOWN;

_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG KphCaptureStack(
    _Out_ PVOID* Frames,
    _In_ ULONG Count
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
BOOLEAN KphAcquireRundown(
    _Inout_ PKPH_RUNDOWN Rundown
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphReleaseRundown(
    _Inout_ PKPH_RUNDOWN Rundown
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphInitializeRundown(
    _Out_ PKPH_RUNDOWN Rundown
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphWaitForRundown(
    _Inout_ PKPH_RUNDOWN Rundown
    );

typedef EX_PUSH_LOCK KPH_RWLOCK;
typedef KPH_RWLOCK* PKPH_RWLOCK;

_IRQL_requires_max_(APC_LEVEL)
VOID KphInitializeRWLock(
    _Out_ PKPH_RWLOCK Lock
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphDeleteRWLock(
    _In_ PKPH_RWLOCK Lock
    );

_IRQL_requires_max_(APC_LEVEL)
_Acquires_lock_(_Global_critical_region_)
VOID KphAcquireRWLockExclusive(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) PKPH_RWLOCK Lock
    );

_IRQL_requires_max_(APC_LEVEL)
_Acquires_lock_(_Global_critical_region_)
VOID KphAcquireRWLockShared(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) PKPH_RWLOCK Lock
    );

_IRQL_requires_max_(APC_LEVEL)
_Releases_lock_(_Global_critical_region_)
VOID KphReleaseRWLock(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) PKPH_RWLOCK Lock
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphValidateAddressForSystemModules(
    _In_ PVOID Address,
    _In_ SIZE_T Length
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryRegistryString(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _Outptr_allocatesMem_ PUNICODE_STRING* String
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphFreeRegistryString(
    _In_freesMem_ PUNICODE_STRING String
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryRegistryBinary(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _Outptr_allocatesMem_ PBYTE* Buffer,
    _Out_ PULONG Length
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphFreeRegistryBinary(
    _In_freesMem_ PBYTE Buffer
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryRegistryULong(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _Out_ PULONG Value
    );

#define KPH_MAP_IMAGE    0x00000001ul

_IRQL_always_function_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphMapViewInSystem(
    _In_ HANDLE FileHandle,
    _In_ ULONG Flags,
    _Outptr_result_bytebuffer_(*ViewSize) PVOID *MappedBase,
    _Inout_ PSIZE_T ViewSize
    );

_IRQL_always_function_max_(PASSIVE_LEVEL)
VOID KphUnmapViewInSystem(
    _In_ PVOID MappedBase
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetProcessModules(
    _In_ PEPROCESS Process,
    _Outptr_allocatesMem_ PRTL_PROCESS_MODULES *Modules
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphFreeProcessModules(
    _In_freesMem_ PRTL_PROCESS_MODULES Modules
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetNameFileObject(
    _In_ PFILE_OBJECT FileObject,
    _Outptr_allocatesMem_ PUNICODE_STRING* FileName
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphFreeNameFileObject(
    _In_freesMem_ PUNICODE_STRING FileName
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphSinglePrivilegeCheckEx(
    _In_ LUID PrivilegeValue,
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphSinglePrivilegeCheck(
    _In_ LUID PrivilegeValue,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphProcessIsLsass(
    _In_ PEPROCESS Process
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetFileVersion(
    _In_ PUNICODE_STRING FileName,
    _Out_ PKPH_FILE_VERSION Version
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetKernelVersion(
    _Out_ PKPH_FILE_VERSION Version
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGuardGrantSuppressedCallAccess(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress,
    _In_ ULONG Flags
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetFileNameFinalComponent(
    _In_ PUNICODE_STRING FileName,
    _Out_ PUNICODE_STRING FinalComponent
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetProcessImageName(
    _In_ PEPROCESS Process,
    _Out_allocatesMem_ PUNICODE_STRING ImageName
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphFreeProcessImageName(
    _In_freesMem_ PUNICODE_STRING ImageName
    );

// vm

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCopyVirtualMemory(
    _In_ PEPROCESS FromProcess,
    _In_ PVOID FromAddress,
    _In_ PEPROCESS ToProcess,
    _Out_writes_bytes_(BufferLength) PVOID ToAddress,
    _In_ SIZE_T BufferLength,
    _In_ KPROCESSOR_MODE AccessMode,
    _Out_ PSIZE_T ReturnLength
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphReadVirtualMemoryUnsafe(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesRead,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQuerySection(
    _In_ HANDLE SectionHandle,
    _In_ KPH_SECTION_INFORMATION_CLASS SectionInformationClass,
    _Out_writes_bytes_(SectionInformationLength) PVOID SectionInformation,
    _In_ ULONG SectionInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

// hash

typedef struct _KPH_HASH
{
    ALG_ID AlgorithmId;
    ULONG Size;
    BYTE Buffer[MINCRYPT_MAX_HASH_LEN];
} KPH_HASH, *PKPH_HASH;

typedef struct _KPH_AUTHENTICODE_INFO
{
    BYTE SHA1[MINCRYPT_SHA1_HASH_LEN];
    BYTE SHA256[MINCRYPT_SHA256_HASH_LEN];
    PBYTE Signature;
    ULONG SignatureSize;
} KPH_AUTHENTICODE_INFO, *PKPH_AUTHENTICODE_INFO;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphInitializeHashing(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupHashing(
    VOID
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphReferenceHashingInfrastructure(
    VOID
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphDereferenceHashingInfrastructure(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHashBuffer(
    _In_ PBYTE Buffer,
    _In_ ULONG BufferLength,
    _In_ ALG_ID AlgorithmId,
    _Out_ PKPH_HASH Hash
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHashFile(
    _In_ HANDLE FileHandle,
    _In_ ALG_ID AlgorithmId,
    _Out_ PKPH_HASH Hash
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHashFileByName(
    _In_ PUNICODE_STRING FileName,
    _In_ ALG_ID AlgorithmId,
    _Out_ PKPH_HASH Hash
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetAuthenticodeInfo(
    _In_ HANDLE FileHandle,
    _Out_allocatesMem_ PKPH_AUTHENTICODE_INFO Info
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetAuthenticodeInfoByFileName(
    _In_ PUNICODE_STRING FileName,
    _Out_allocatesMem_ PKPH_AUTHENTICODE_INFO Info
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphFreeAuthenticodeInfo(
    _In_freesMem_ PKPH_AUTHENTICODE_INFO Info
    );

// sign

typedef struct _KPH_SIGNING_INFO
{
    KPH_AUTHENTICODE_INFO Authenticode;
    MINCRYPT_POLICY_INFO PolicyInfo;
    LARGE_INTEGER SigningTime;
    MINCRYPT_POLICY_INFO TimeStampPolicyInfo;
    UNICODE_STRING CatalogName;
} KPH_SIGNING_INFO, *PKPH_SIGNING_INFO;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphInitializeSigning(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupSigning(
    VOID
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphReferenceSigningInfrastructure(
    VOID
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphDereferenceSigningInfrastructure(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetSigningInfo(
    _In_ HANDLE FileHandle,
    _Out_allocatesMem_ PKPH_SIGNING_INFO Info
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetSigningInfoByFileName(
    _In_ PUNICODE_STRING FileName,
    _Out_allocatesMem_ PKPH_SIGNING_INFO Info
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphFreeSigningInfo(
    _In_freesMem_ PKPH_SIGNING_INFO Info
    );

// kphobject

typedef struct _KPH_OBJECT_HEADER
{
    volatile SSIZE_T PointerCount;
    UCHAR TypeIndex;

    QUAD Body;
} KPH_OBJECT_HEADER, *PKPH_OBJECT_HEADER;

#define KphObjectToObjectHeader(x) ((PKPH_OBJECT_HEADER)CONTAINING_RECORD((PCHAR)Object, KPH_OBJECT_HEADER, Body))
#define KphObjectHeaderToObject(x) ((PVOID)&((PKPH_OBJECT_HEADER)(x))->Body)
#define KphAddObjectHeaderSize(x) ((SIZE_T)(x) + FIELD_OFFSET(KPH_OBJECT_HEADER, Body))

typedef
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_Return_allocatesMem_size_(Size)
PVOID
KSIAPI
KPH_TYPE_ALLOCATE_PROCEDURE(
    _In_ SIZE_T Size
    );
typedef KPH_TYPE_ALLOCATE_PROCEDURE* PKPH_TYPE_ALLOCATE_PROCEDURE;

typedef
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_Must_inspect_result_
NTSTATUS
KSIAPI
KPH_TYPE_INITIALIZE_PROCEDURE(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    );
typedef KPH_TYPE_INITIALIZE_PROCEDURE* PKPH_TYPE_INITIALIZE_PROCEDURE;

typedef
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
VOID
KSIAPI
KPH_TYPE_DELETE_PROCEDURE(
    _Inout_ PVOID Object
    );
typedef KPH_TYPE_DELETE_PROCEDURE* PKPH_TYPE_DELETE_PROCEDURE;

typedef
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
VOID
KSIAPI
KPH_TYPE_FREE_PROCEDURE(
    _In_freesMem_ PVOID Object
    );
typedef KPH_TYPE_FREE_PROCEDURE* PKPH_TYPE_FREE_PROCEDURE;

typedef struct _KPH_OBJECT_TYPE_INFO
{
    PKPH_TYPE_ALLOCATE_PROCEDURE Allocate;
    PKPH_TYPE_INITIALIZE_PROCEDURE Initialize;
    PKPH_TYPE_DELETE_PROCEDURE Delete;
    PKPH_TYPE_FREE_PROCEDURE Free;
} KPH_OBJECT_TYPE_INFO, *PKPH_OBJECT_TYPE_INFO;

typedef struct _KPH_OBJECT_TYPE
{
    UNICODE_STRING Name;
    UCHAR Index;
    volatile SIZE_T TotalNumberOfObjects;
    volatile SIZE_T HighWaterNumberOfObjects;
    KPH_OBJECT_TYPE_INFO TypeInfo;
} KPH_OBJECT_TYPE, *PKPH_OBJECT_TYPE;

VOID
KphCreateObjectType(
    _In_ PUNICODE_STRING TypeName,
    _In_ PKPH_OBJECT_TYPE_INFO TypeInfo,
    _Outptr_ PKPH_OBJECT_TYPE* ObjectType
    );

_Must_inspect_result_
NTSTATUS
KphCreateObject(
    _In_ PKPH_OBJECT_TYPE ObjectType,
    _In_ ULONG ObjectBodySize,
    _Outptr_result_nullonfailure_ PVOID* Object,
    _In_opt_ PVOID Parameter
    );

VOID
KphReferenceObject(
    _In_ PVOID Object
    );

VOID
KphDereferenceObject(
    _In_ PVOID Object
    );

_Must_inspect_result_
PKPH_OBJECT_TYPE
KphGetObjectType(
    _In_ PVOID Object
    );

// cid_tracking

#define KPH_PROTECTED_PROCESS_MASK (KPH_PROCESS_READ_ACCESS |\
                                    PROCESS_TERMINATE |\
                                    PROCESS_SUSPEND_RESUME)
#define KPH_PROTECTED_THREAD_MASK (KPH_THREAD_READ_ACCESS |\
                                   THREAD_TERMINATE |\
                                   THREAD_SUSPEND_RESUME |\
                                   THREAD_RESUME)

typedef struct _KPH_PROCESS_CONTEXT
{
    PEPROCESS EProcess;

    HANDLE ProcessId;
    CLIENT_ID CreatorClientId;

    PUNICODE_STRING ImageFileName;
    UNICODE_STRING ImageName;
    PFILE_OBJECT FileObject;

    volatile SIZE_T NumberOfImageLoads;

    union
    {
        volatile ULONG Flags;
        struct
        {
            ULONG CreateNotification : 1;
            ULONG ExitNotification : 1;
            ULONG VerifiedProcess : 1;
            ULONG SecurelyCreated : 1;
            ULONG Protected : 1;
            ULONG IsLsass : 1;
            ULONG IsWow64 : 1;
            ULONG IsSubsystemProcess : 1;
            ULONG AllocatedImageName : 1;
            ULONG Reserved : 23;
        };
    };

    KPH_RWLOCK ThreadListLock;
    struct _KPH_THREAD_CONTEXT* InitialThread;
    SIZE_T NumberOfThreads;
    LIST_ENTRY ThreadListHead;
    SIZE_T NumberOfUnlinkedThreads;

    //
    // Masks are only valid if Protected flag is set.
    //
    KPH_RWLOCK ProtectionLock;
    ACCESS_MASK ProcessAllowedMask;
    ACCESS_MASK ThreadAllowedMask;

    //
    // Only tracked for verified processes.
    //
    volatile SIZE_T NumberOfMicrosoftImageLoads;
    volatile SIZE_T NumberOfAntimalwareImageLoads;
    volatile SIZE_T NumberOfVerifiedImageLoads;
    volatile SIZE_T NumberOfUntrustedImageLoads;

    PKNORMAL_ROUTINE ApcNoopRoutine;

    SUBSYSTEM_INFORMATION_TYPE SubsystemType;

    //
    // SubsystemInformationTypeWSL information
    //
    struct
    {
        BOOLEAN ValidProcessId;
        ULONG ProcessId;

    } WSL;
} KPH_PROCESS_CONTEXT, *PKPH_PROCESS_CONTEXT;

extern PKPH_OBJECT_TYPE KphProcessContextType;

typedef struct _KPH_THREAD_CONTEXT
{
    PETHREAD EThread;

    PKPH_PROCESS_CONTEXT ProcessContext;

    CLIENT_ID ClientId;
    CLIENT_ID CreatorClientId;

    union
    {
        volatile ULONG Flags;
        struct
        {
            ULONG CreateNotification : 1;
            ULONG ExecuteNotification : 1;
            ULONG ExitNotification : 1;
            ULONG InThreadList : 1;
            ULONG IsCreatingProcess : 1;
            ULONG InitApcQueued : 1;
            ULONG InitApcExecuted : 1;
            ULONG Reserved : 25;
        };
    };

    LIST_ENTRY ThreadListEntry;

    //
    // Only valid if IsCreatingProcess flag is set.
    //
    HANDLE IsCreatingProcessId;

    SUBSYSTEM_INFORMATION_TYPE SubsystemType;

    //
    // SubsystemInformationTypeWSL information
    //
    struct
    {
        BOOLEAN ValidThreadId;
        ULONG ThreadId;

    } WSL;
} KPH_THREAD_CONTEXT, *PKPH_THREAD_CONTEXT;

extern PKPH_OBJECT_TYPE KphThreadContextType;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCidInitialize(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCidPopulate(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCidCleanup(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCidMarkPopulated(
    VOID
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
PKPH_PROCESS_CONTEXT KphGetProcessContext(
    _In_ HANDLE ProcessId
    );

#define KphGetCurrentProcessContext() KphGetProcessContext(PsGetCurrentProcessId())

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
PKPH_THREAD_CONTEXT KphGetThreadContext(
    _In_ HANDLE ThreadId
    );

#define KphGetCurrentThreadContext() KphGetThreadContext(PsGetCurrentThreadId())

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PKPH_PROCESS_CONTEXT KphTrackProcessContext(
    _In_ PEPROCESS Process
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PKPH_PROCESS_CONTEXT KphUntrackProcessContext(
    _In_ HANDLE ProcessId
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PKPH_THREAD_CONTEXT KphTrackThreadContext(
    _In_ PETHREAD Thread
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PKPH_THREAD_CONTEXT KphUntrackThreadContext(
    _In_ HANDLE ThreadId
    );

typedef
_Function_class_(KPH_ENUM_PROCESS_CONTEXTS_CALLBACK)
_Must_inspect_result_
BOOLEAN
KSIAPI
KPH_ENUM_PROCESS_CONTEXTS_CALLBACK(
    _In_ PKPH_PROCESS_CONTEXT Process,
    _In_opt_ PVOID Parameter
    );
typedef KPH_ENUM_PROCESS_CONTEXTS_CALLBACK* PKPH_ENUM_PROCESS_CONTEXTS_CALLBACK;

_IRQL_requires_max_(APC_LEVEL)
VOID KphEnumerateProcessContexts(
    _In_ PKPH_ENUM_PROCESS_CONTEXTS_CALLBACK Callback,
    _In_opt_ PVOID Parameter
    );

typedef
_Function_class_(KPH_ENUM_THREAD_CONTEXTS_CALLBACK)
_Must_inspect_result_
BOOLEAN
KSIAPI
KPH_ENUM_THREAD_CONTEXTS_CALLBACK(
    _In_ PKPH_PROCESS_CONTEXT Thread,
    _In_opt_ PVOID Parameter
    );
typedef KPH_ENUM_THREAD_CONTEXTS_CALLBACK* PKPH_ENUM_THREAD_CONTEXTS_CALLBACK;

_IRQL_requires_max_(APC_LEVEL)
VOID KphEnumerateThreadContexts(
    _In_ PKPH_ENUM_THREAD_CONTEXTS_CALLBACK Callback,
    _In_opt_ PVOID Parameter
    );

typedef
_Function_class_(KPH_ENUM_CID_CONTEXTS_CALLBACK)
_Must_inspect_result_
BOOLEAN
KSIAPI
KPH_ENUM_CID_CONTEXTS_CALLBACK(
    _In_ PVOID Context,
    _In_opt_ PVOID Parameter
    );
typedef KPH_ENUM_CID_CONTEXTS_CALLBACK* PKPH_ENUM_CID_CONTEXTS_CALLBACK;

_IRQL_requires_max_(APC_LEVEL)
VOID KphEnumerateCidContexts(
    _In_ KPH_ENUM_CID_CONTEXTS_CALLBACK Callback,
    _In_opt_ PVOID Parameter
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCheckProcessApcNoopRoutine(
    _In_ PKPH_PROCESS_CONTEXT ProcessContext
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphVerifyProcessAndProtectIfAppropriate(
    _In_ PKPH_PROCESS_CONTEXT Process
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
_Maybenull_
PUNICODE_STRING KphGetThreadImageName(
    _In_ PKPH_THREAD_CONTEXT Thread
    );

// protection

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeProtection(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphStartProtectingProcess(
    _In_ PKPH_PROCESS_CONTEXT Process,
    _In_ ACCESS_MASK ProcessAllowedMask,
    _In_ ACCESS_MASK ThreadAllowedMask
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphStopProtectingProcess(
    _In_ PKPH_PROCESS_CONTEXT Process
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphIsProtectedProcess(
    _In_ PKPH_PROCESS_CONTEXT Process
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphApplyObProtections(
    _Inout_ POB_PRE_OPERATION_INFORMATION Info
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphApplyImageProtections(
    _Inout_ PKPH_PROCESS_CONTEXT Process,
    _In_ PIMAGE_INFO_EX ImageInfo
    );

// verify

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphInitializeVerify(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupVerify(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphVerifyBuffer(
    _In_ PBYTE Buffer,
    _In_ ULONG BufferLength,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphVerifyFile(
    _In_ PUNICODE_STRING FileName
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphDominationCheck(
    _In_ PEPROCESS Process,
    _In_ PEPROCESS ProcessTarget,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(APC_LEVEL)
KPH_PROCESS_STATE KphGetProcessState(
    _In_ PKPH_PROCESS_CONTEXT Process
    );

// ksi.dll

#if !defined(_KSIDLL_)
#define KSISYSAPI DECLSPEC_IMPORT
#else
#define KSISYSAPI
#endif

typedef struct _KSI_KAPC
{
    KAPC Apc;
    PDRIVER_OBJECT DriverObject;
    PVOID InternalRoutine;
    PVOID InternalCleanup;
    PVOID InternalContext;
} KSI_KAPC, *PKSI_KAPC;

typedef enum _KSI_KAPC_CLEANUP_REASON
{
    KsiApcCleanupKernel,
    KsiApcCleanupNormal,
    KsiApcCleanupRundown
} KSI_KAPC_CLEANUP_REASON;

typedef
_Function_class_(KSI_KCLEANUP_ROUTINE)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_same_
VOID
KSIAPI
KSI_KCLEANUP_ROUTINE(
    _In_ PKSI_KAPC Apc,
    _In_ KSI_KAPC_CLEANUP_REASON Reason
    );
typedef KSI_KCLEANUP_ROUTINE *PKSI_KCLEANUP_ROUTINE;

typedef
_Function_class_(KSI_KKERNEL_ROUTINE)
_IRQL_requires_(APC_LEVEL)
_IRQL_requires_same_
VOID
KSIAPI
KSI_KKERNEL_ROUTINE(
    _In_ PKSI_KAPC Apc,
    _Inout_ _Deref_pre_maybenull_ PKNORMAL_ROUTINE* NormalRoutine,
    _Inout_ _Deref_pre_maybenull_ PVOID* NormalContext,
    _Inout_ _Deref_pre_maybenull_ PVOID* SystemArgument1,
    _Inout_ _Deref_pre_maybenull_ PVOID* SystemArgument2
    );
typedef KSI_KKERNEL_ROUTINE *PKSI_KKERNEL_ROUTINE;

C_ASSERT(FIELD_OFFSET(KSI_KAPC, Apc) == 0);

KSISYSAPI
VOID
KSIAPI
KsiInitializeApc(
    _Out_ PKSI_KAPC Apc,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PRKTHREAD Thread,
    _In_ KAPC_ENVIRONMENT Environment,
    _In_ PKSI_KKERNEL_ROUTINE KernelRoutine,
    _In_ PKSI_KCLEANUP_ROUTINE CleanupRoutine,
    _In_opt_ PKNORMAL_ROUTINE NormalRoutine,
    _In_ KPROCESSOR_MODE ApcMode,
    _In_opt_ PVOID NormalContext
    );

KSISYSAPI
BOOLEAN
KSIAPI
KsiInsertQueueApc(
    _In_ PKSI_KAPC Apc,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2,
    _In_ KPRIORITY PriorityBoost
    );

// system

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSystemControl(
    _In_ KPH_SYSTEM_CONTROL_CLASS SystemControlClass,
    _In_reads_bytes_(SystemControlInfoLength) PVOID SystemControlInfo,
    _In_ ULONG SystemControlInfoLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

// alpc

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphAlpcQueryInformation(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE PortHandle,
    _In_ KPH_ALPC_INFORMATION_CLASS AlpcInformationClass,
    _Out_writes_bytes_opt_(AlpcInformationLength) PVOID AlpcInformation,
    _In_ ULONG AlpcInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

// file

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryInformationFile(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE FileHandle,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _Out_writes_bytes_(FileInformationLength) PVOID FileInformation,
    _In_ ULONG FileInformationLength,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryVolumeInformationFile(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE FileHandle,
    _In_ FS_INFORMATION_CLASS FsInformationClass,
    _Out_writes_bytes_(FsInformationLength) PVOID FsInformation,
    _In_ ULONG FsInformationLength,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCreateFile(
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength,
    _In_ ULONG Options,
    _In_ KPROCESSOR_MODE AccessMode
    );

// knowndll

extern PVOID KphNtDllBaseAddress;
extern PVOID KphNtDllRtlSetBits;

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphInitializeKnownDll(
    VOID
    );

// socket

typedef PVOID KPH_SOCKET_HANDLE;
typedef PVOID* PKPH_SOCKET_HANDLE;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphInitializeSocket(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupSocket(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetAddressInfo(
    _In_ PUNICODE_STRING NodeName,
    _In_opt_ PUNICODE_STRING ServiceName,
    _In_opt_ PADDRINFOEXW Hints,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Outptr_allocatesMem_ PADDRINFOEXW* AddressInfo
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphFreeAddressInfo(
    _In_freesMem_ PADDRINFOEXW AddressInfo
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphSocketClose(
    _In_freesMem_ KPH_SOCKET_HANDLE Socket
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphSocketConnect(
    _In_ USHORT SocketType,
    _In_ ULONG Protocol,
    _In_ PSOCKADDR LocalAddress,
    _In_ PSOCKADDR RemoteAddress,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Outptr_allocatesMem_ PKPH_SOCKET_HANDLE Socket
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphSocketSend(
    _In_ KPH_SOCKET_HANDLE Socket,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphSocketRecv(
    _In_ KPH_SOCKET_HANDLE Socket,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_writes_bytes_to_(*Length, *Length) PVOID Buffer,
    _Inout_ PULONG Length
    );
