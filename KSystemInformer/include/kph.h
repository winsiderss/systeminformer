/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2025
 *
 */

#pragma once
#pragma warning(push)
#pragma warning(disable: 5103) // invalid preprocessing token (/Zc:preprocessor)
#include <ntifs.h>
#include <ntintsafe.h>
#include <minwindef.h>
#include <ntstrsafe.h>
#include <fltKernel.h>
#include <ntimage.h>
#include <bcrypt.h>
#pragma warning(pop)
#include <pooltags.h>
#define PHNT_MODE PHNT_MODE_KERNEL
#include <phnt.h>
#include <ntfill.h>
#include <ntpebteb.h>
#include <ntldr.h>
#include <ntwow64.h>
#include <kphapi.h>
#include <kphringbuff.h>

#define KSIAPI NTAPI

#define KPH_PAGED_CODE()                                                       \
    PAGED_CODE();                                                              \
    NT_ANALYSIS_ASSUME((KeGetCurrentIrql() == APC_LEVEL) ||                    \
                       (KeGetCurrentIrql() == PASSIVE_LEVEL))
#define KPH_PAGED_CODE_PASSIVE()                                               \
    PAGED_CODE();                                                              \
    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);                            \
    NT_ANALYSIS_ASSUME(KeGetCurrentIrql() == PASSIVE_LEVEL)
#define KPH_NPAGED_CODE_PASSIVE()                                              \
    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);                            \
    NT_ANALYSIS_ASSUME(KeGetCurrentIrql() == PASSIVE_LEVEL)
#define KPH_NPAGED_CODE_APC_MAX()                                              \
    NT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);                                \
    NT_ANALYSIS_ASSUME((KeGetCurrentIrql() == APC_LEVEL) ||                    \
                       (KeGetCurrentIrql() == PASSIVE_LEVEL))
#define KPH_NPAGED_CODE_DISPATCH_MAX()                                         \
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);                           \
    NT_ANALYSIS_ASSUME((KeGetCurrentIrql() == DISPATCH_LEVEL) ||               \
                       (KeGetCurrentIrql() == APC_LEVEL) ||                    \
                       (KeGetCurrentIrql() == PASSIVE_LEVEL))
#define KPH_NPAGED_CODE_DISPATCH_MIN()                                         \
    NT_ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);                           \
    NT_ANALYSIS_ASSUME((KeGetCurrentIrql() == DISPATCH_LEVEL) ||               \
                       (KeGetCurrentIrql() == HIGH_LEVEL))
#define KPH_NPAGED_CODE_DISPATCH()                                             \
    NT_ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);                           \
    NT_ANALYSIS_ASSUME(KeGetCurrentIrql() == DISPATCH_LEVEL)

//
// N.B. This decorates code to indicate that the code supports up to APC_LEVEL
// but is in a non-paged segment since it can be called in a paging I/O path.
// Code in this path should also only allocate from non-paged pool to avoid
// deadlocks.
//
#define KPH_NPAGED_CODE_APC_MAX_FOR_PAGING_IO() KPH_NPAGED_CODE_APC_MAX()

#define KPH_PAGED_FILE()                                                       \
    __pragma(bss_seg("PAGEBBS"))                                               \
    __pragma(code_seg("PAGE"))                                                 \
    __pragma(data_seg("PAGEDATA"))                                             \
    __pragma(const_seg("PAGERO"))

#define KPH_PROTECTED_DATA_SECTION_PUSH()                                      \
    __pragma(data_seg(push))                                                   \
    __pragma(data_seg("KSIDATA"))
#define KPH_PROTECTED_DATA_SECTION_POP()                                       \
    __pragma(data_seg(pop))
#define KPH_PROTECTED_DATA_SECTION_RO_PUSH()                                   \
    __pragma(const_seg(push))                                                  \
    __pragma(const_seg("KSIRO"))
#define KPH_PROTECTED_DATA_SECTION_RO_POP()                                    \
    __pragma(const_seg(pop))

#define _Outptr_allocatesMem_ _Outptr_result_nullonfailure_ __drv_allocatesMem(Mem)
#define _Out_allocatesMem_ _Out_ __drv_allocatesMem(Mem)
#define _Out_allocatesMem_size_(size) _Out_allocatesMem_ _Post_writable_byte_size_(size)
#define _FreesMem_ _Pre_notnull_ _Post_ptr_invalid_ __drv_freesMem(Mem)
#define _In_freesMem_ _In_ _FreesMem_
#define _In_aliasesMem_ _In_ _Pre_notnull_ _Post_ptr_invalid_ __drv_aliasesMem
#define _Return_allocatesMem_ __drv_allocatesMem(Mem) _Post_maybenull_ _Must_inspect_result_
#define _Return_allocatesMem_size_(size) _Return_allocatesMem_ _Post_writable_byte_size_(size)
#define _IRQL_requires_for_wait_(timeout)                                      \
    _When_(((timeout == NULL) || (timeout->QuadPart != 0)),                    \
           _IRQL_requires_max_(APC_LEVEL))                                     \
    _When_(((timeout != NULL) && (timeout->QuadPart == 0)),                    \
           _IRQL_requires_max_(DISPATCH_LEVEL))

typedef const void* PCVOID;
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

FORCEINLINE
ULONG64 InterlockedExchangeU64(
    _Inout_ _Interlocked_operand_ volatile ULONG64* Target,
    _In_ ULONG64 Value
    )
{
    return (ULONG64)InterlockedExchange64((LONG64*)Target, (LONG64)Value);
}


FORCEINLINE
ULONG64 InterlockedCompareExchangeU64(
    _Inout_ _Interlocked_operand_ volatile ULONG64* Target,
    _In_ ULONG64 Value,
    _In_ ULONG64 Expected
    )
{
    return (ULONG64)InterlockedCompareExchange64((LONG64*)Target,
                                                 (LONG64)Value,
                                                 (LONG64)Expected);
}

FORCEINLINE
ULONG64 InterlockedIncrementU64(
    _Inout_ _Interlocked_operand_ volatile ULONG64* Target
    )
{
    return (ULONG64)InterlockedIncrement64((LONG64*)Target);
}

FORCEINLINE
ULONG_PTR InterlockedExchangeULongPtr(
    _Inout_ _Interlocked_operand_ volatile ULONG_PTR* Target,
    _In_ ULONG_PTR Value
    )
{
#ifdef _WIN64
    return (ULONG_PTR)InterlockedExchange64((LONG64*)Target, (LONG64)Value);
#else
    return (ULONG_PTR)InterlockedExchange((LONG*)Target, (LONG)Value);
#endif
}

FORCEINLINE
ULONG_PTR InterlockedExchangeAddULongPtr(
    _Inout_ _Interlocked_operand_ volatile ULONG_PTR* Target,
    _In_ ULONG_PTR Value
    )
{
    return (ULONG_PTR)InterlockedExchangeAddSizeT((SIZE_T*)Target, (SIZE_T)Value);
}

FORCEINLINE
BOOLEAN InterlockedBitTestAndResetULongPtr(
    _Inout_ _Interlocked_operand_ volatile ULONG_PTR* Target,
    _In_ ULONG_PTR Bit
    )
{
#ifdef _WIN64
    return InterlockedBitTestAndReset64((LONG64*)Target, (LONG64)Bit);
#else
    return InterlockedBitTestAndReset((LONG*)Target, (LONG)Bit);
#endif
}

FORCEINLINE
ULONG_PTR InterlockedCompareExchangeULongPtr(
    _Inout_ _Interlocked_operand_ volatile ULONG_PTR* Target,
    _In_ ULONG_PTR Value,
    _In_ ULONG_PTR Expected
    )
{
#ifdef _WIN64
    return (ULONG_PTR)InterlockedCompareExchange64((LONG64*)Target,
                                                   (LONG64)Value,
                                                   (LONG64)Expected);
#else
    return (ULONG_PTR)InterlockedCompareExchange((LONG*)Target,
                                                 (LONG)Value,
                                                 (LONG)Expected);
#endif
}

FORCEINLINE
ULONG_PTR InterlockedDecrementULongPtr(
    _Inout_ _Interlocked_operand_ volatile ULONG_PTR* Target
    )
{
    return (ULONG_PTR)InterlockedDecrementSizeT((SIZE_T*)Target);
}

FORCEINLINE
SSIZE_T InterlockedIncrementSSizeT(
    _Inout_ _Interlocked_operand_ volatile SSIZE_T* Target
    )
{
    return (SSIZE_T)InterlockedIncrementSizeT((SIZE_T*)Target);
}

FORCEINLINE
SSIZE_T InterlockedDecrementSSizeT(
    _Inout_ _Interlocked_operand_ volatile SSIZE_T* Target
    )
{
    return (SSIZE_T)InterlockedDecrementSizeT((SIZE_T*)Target);
}

FORCEINLINE
SIZE_T InterlockedCompareExchangeSizeT(
    _Inout_ _Interlocked_operand_ volatile SIZE_T* Target,
    _In_ SIZE_T Value,
    _In_ SIZE_T Expected
    )
{
    return (SIZE_T)InterlockedCompareExchangePointer((PVOID*)Target,
                                                     (PVOID)Value,
                                                     (PVOID)Expected);
}

FORCEINLINE
SIZE_T InterlockedExchangeIfGreaterSizeT(
    _Inout_ _Interlocked_operand_ volatile SIZE_T* Target,
    _In_ SIZE_T Value
    )
{
    SIZE_T expected;

    for (;;)
    {
        expected = ReadSizeTAcquire(Target);

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

#define ProbeOutputType(pointer, type)                                         \
_Pragma("warning(suppress : 6001 4116)")                                       \
ProbeForRead(pointer, sizeof(type), 1)

#define ProbeInputType(pointer, type)                                          \
_Pragma("warning(suppress : 6001 4116)")                                       \
ProbeForRead(pointer, sizeof(type), 1)

#define ProbeOutputBytes(pointer, size)                                        \
_Pragma("warning(suppress : 6001 4116)")                                       \
ProbeForRead(pointer, size, 1)

#define ProbeInputBytes(pointer, size)                                         \
_Pragma("warning(suppress : 6001 4116)")                                       \
ProbeForRead(pointer, size, 1)

#define C_2sTo4(x) ((unsigned int)(signed short)(x))

#define RebasePtr(pointer, oldBase, newBase)                                   \
Add2Ptr(newBase, PtrOffset(oldBase, pointer))

#define RebaseUnicodeString(string, oldBase, newBase)                          \
if ((string)->Buffer)                                                          \
{                                                                              \
    (string)->Buffer = Add2Ptr(newBase, PtrOffset(oldBase, (string)->Buffer)); \
}

#define KPH_TIMEOUT(ms) { .QuadPart = (-10000ll * (ms)) }

#define KPH_KERNEL_PURGE_EA "$KERNEL.PURGE.SI."

typedef struct _KPH_FILE_VERSION
{
    USHORT MajorVersion;
    USHORT MinorVersion;
    USHORT BuildNumber;
    USHORT Revision;
} KPH_FILE_VERSION, *PKPH_FILE_VERSION;

typedef struct _KPH_MEMORY_REGION
{
    PVOID Start;
    PVOID End;
} KPH_MEMORY_REGION, *PKPH_MEMORY_REGION;

_Must_inspect_result_
FORCEINLINE
BOOLEAN KphIsValidMemoryRegion(
    _In_ PKPH_MEMORY_REGION Region,
    _In_ PVOID Start,
    _In_ PVOID End
    )
{
    return ((Region->Start <= Region->End) &&
            (Region->Start >= Start) &&
            (Region->End <= End));
}

//
// C30030 - Probably allocating executable memory via specifying a
// MM_PAGE_PRIORITY type without a bitwise OR with MdlMappingNoExecute
//
#pragma deprecated("MmGetSystemAddressForMdl")
#pragma deprecated("MmGetSystemAddressForMdlSafe")
_IRQL_requires_max_(DISPATCH_LEVEL)
_Post_writable_byte_size_(Mdl->ByteCount)
_At_(Mdl->MappedSystemVa, _Post_writable_byte_size_(Mdl->ByteCount))
_Check_return_
_Success_(return != NULL)
FORCEINLINE
PVOID KphpGetSystemAddressForMdl(
    _Inout_ PMDL Mdl,
    _In_ MM_PAGE_PRIORITY Priority
    )
{
#pragma warning(suppress: 4995) // suppress deprecation warning
    return MmGetSystemAddressForMdlSafe(Mdl, Priority | MdlMappingNoExecute);
}
#define KphGetSystemAddressForMdl(mdl, priority)                               \
_Pragma("warning(push)")                                                       \
_Pragma("warning(disable : 30030)")                                            \
KphpGetSystemAddressForMdl(mdl, priority)                                      \
_Pragma("warning(pop)")

#pragma deprecated("ObReferenceObjectByHandle")
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
FORCEINLINE
NTSTATUS KphpObReferenceObjectByHandle(
    _In_ HANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_TYPE ObjectType,
    _In_ KPROCESSOR_MODE AccessMode,
    _Out_ PVOID *Object,
    _Out_opt_ POBJECT_HANDLE_INFORMATION HandleInformation
    )
{
#pragma warning(disable: 4995)   // suppress deprecation warning
#pragma prefast(suppress: 28118) // allow at APC_LEVEL
    return ObReferenceObjectByHandle(Handle,
                                     DesiredAccess,
                                     ObjectType,
                                     AccessMode,
                                     Object,
                                     HandleInformation);
}
#define ObReferenceObjectByHandle KphpObReferenceObjectByHandle

// main

extern PDRIVER_OBJECT KphDriverObject;
extern RTL_OSVERSIONINFOEXW KphOsVersionInfo;
extern KPH_FILE_VERSION KphKernelVersion;
extern BOOLEAN KphIgnoreProtectionsSuppressed;
extern BOOLEAN KphIgnoreTestSigningEnabled;
extern SYSTEM_SECUREBOOT_INFORMATION KphSecureBootInfo;
extern SYSTEM_CODEINTEGRITY_INFORMATION KphCodeIntegrityInfo;

FORCEINLINE
BOOLEAN KphInDeveloperMode(
    VOID
    )
{
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

FORCEINLINE
BOOLEAN KphProtectionsSuppressed(
    VOID
    )
{
    if (KphIgnoreProtectionsSuppressed)
    {
        return FALSE;
    }

    return KphInDeveloperMode();
}

FORCEINLINE
BOOLEAN KphTestSigningEnabled(
    VOID
    )
{
    if (KphIgnoreTestSigningEnabled)
    {
        return FALSE;
    }

    return KphInDeveloperMode();
}

// parameters

extern PUNICODE_STRING KphAltitude;
extern PUNICODE_STRING KphPortName;
extern KPH_PARAMETER_FLAGS KphParameterFlags;
extern PUNICODE_STRING KphSystemProcessName;

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupParameters(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeParameters(
    _In_ PCUNICODE_STRING RegistryPath
    );

// alloc

//
// Always use wrapped allocators, unless explicitly necessary.
//
// This helps catch programmer error. Deprecate the original ones here, the
// allocation infrastructure will internally suppress the deprecated warnings.
//
#pragma deprecated("ExAllocateCacheAwareRundownProtection")
#pragma deprecated("ExAllocateFromLookasideListEx")
#pragma deprecated("ExAllocateFromNPagedLookasideList")
#pragma deprecated("ExAllocateFromPagedLookasideList")
#pragma deprecated("ExAllocatePool")
#pragma deprecated("ExAllocatePool2")
#pragma deprecated("ExAllocatePool3")
#pragma deprecated("ExAllocatePoolPriorityUninitialized")
#pragma deprecated("ExAllocatePoolPriorityZero")
#pragma deprecated("ExAllocatePoolQuotaUninitialized")
#pragma deprecated("ExAllocatePoolQuotaZero")
#pragma deprecated("ExAllocatePoolUninitialized")
#pragma deprecated("ExAllocatePoolWithQuota")
#pragma deprecated("ExAllocatePoolWithQuotaTag")
#pragma deprecated("ExAllocatePoolWithTag")
#pragma deprecated("ExAllocatePoolWithTagPriority")
#pragma deprecated("ExAllocatePoolZero")
#pragma deprecated("ExFreePool")
#pragma deprecated("ExFreePool2")
#pragma deprecated("ExFreePoolWithTag")
#pragma deprecated("ExFreeToLookasideListEx")
#pragma deprecated("ExFreeToNPagedLookasideList")
#pragma deprecated("ExFreeToPagedLookasideList")
#pragma deprecated("ExDeleteLookasideListEx")
#pragma deprecated("ExDeleteNPagedLookasideList")
#pragma deprecated("ExDeletePagedLookasideList")
#pragma deprecated("ExInitializeLookasideListEx")
#pragma deprecated("ExInitializeNPagedLookasideList")
#pragma deprecated("ExInitializePagedLookasideList")

FORCEINLINE
VOID KphFreePool(
    _FreesMem_ PVOID Memory
    )
{
#pragma warning(suppress: 4995) // intentional use of ExFreePool
    ExFreePoolWithTag(Memory, 0);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeAlloc(
    VOID
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

#pragma prefast(push)
#pragma prefast(disable: 28195) // acquire memory without doing so
_IRQL_requires_max_(DISPATCH_LEVEL)
_Return_allocatesMem_size_(NumberOfBytes)
FORCEINLINE
PVOID KphpAllocateNPagedA(
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag,
    _Out_writes_bytes_(SizeOfStack) PBYTE Stack,
    _In_ ULONG SizeOfStack
    )
{
    if (NumberOfBytes <= SizeOfStack)
    {
        RtlZeroMemory(Stack, SizeOfStack);
        return Stack;
    }

    return KphAllocateNPaged(NumberOfBytes, Tag);
}
#pragma prefast(pop)

#define KphAllocateNPagedA(NumberOfBytes, Tag, Stack)                          \
    KphpAllocateNPagedA(NumberOfBytes, Tag, Stack, sizeof(Stack))

#pragma prefast(push)
#pragma prefast(disable: 28195) // acquire memory without doing so
_IRQL_requires_max_(APC_LEVEL)
_Return_allocatesMem_size_(NumberOfBytes)
FORCEINLINE
PVOID KphpAllocatePagedA(
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag,
    _Out_writes_bytes_(SizeOfStack) PBYTE Stack,
    _In_ ULONG SizeOfStack
    )
{
    if (NumberOfBytes <= SizeOfStack)
    {
        RtlZeroMemory(Stack, SizeOfStack);
        return Stack;
    }

    return KphAllocatePaged(NumberOfBytes, Tag);
}
#pragma prefast(pop)

#define KphAllocatePagedA(NumberOfBytes, Tag, Stack)                           \
    KphpAllocatePagedA(NumberOfBytes, Tag, Stack, sizeof(Stack))

#pragma prefast(push)
#pragma prefast(disable: 6014) // leaking memory
_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
VOID KphpFreeA(
    _FreesMem_ PVOID Memory,
    _In_ ULONG Tag,
    _In_ PBYTE Stack
    )
{
    if (Memory != Stack)
    {
        KphFree(Memory, Tag);
    }
}
#pragma prefast(pop)

#define KphFreeA(Memory, Tag, Stack) KphpFreeA(Memory, Tag, Stack)

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

extern PPS_SET_LOAD_IMAGE_NOTIFY_ROUTINE_EX KphDynPsSetLoadImageNotifyRoutineEx;
extern PPS_SET_CREATE_PROCESS_NOTIFY_ROUTINE_EX2 KphDynPsSetCreateProcessNotifyRoutineEx2;
extern PMM_PROTECT_DRIVER_SECTION KphDynMmProtectDriverSection;
extern PPS_GET_PROCESS_SEQUENCE_NUMBER KphDynPsGetProcessSequenceNumber;
extern PPS_GET_PROCESS_START_KEY KphDynPsGetProcessStartKey;
extern PSE_REGISTER_IMAGE_VERIFICATION_CALLBACK KphSeRegisterImageVerificationCallback;
extern PSE_UNREGISTER_IMAGE_VERIFICATION_CALLBACK KphSeUnregisterImageVerificationCallback;
extern PCI_VALIDATE_FILE_OBJECT KphDynCiValidateFileObject;
extern PCI_FREE_POLICY_INFO KphDynCiFreePolicyInfo;
extern PLXP_THREAD_GET_CURRENT KphDynLxpThreadGetCurrent;
extern PIO_CHECK_FILE_OBJECT_OPENED_AS_COPY_SOURCE KphDynIoCheckFileObjectOpenedAsCopySource;
extern PIO_CHECK_FILE_OBJECT_OPENED_AS_COPY_DESTINATION KphDynIoCheckFileObjectOpenedAsCopyDestination;
extern PFLT_GET_COPY_INFORMATION_FROM_CALLBACK_DATA KphDynFltGetCopyInformationFromCallbackData;

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

// dyndata

typedef struct _KPH_DYN
{
    ULONG EgeGuid;
    ULONG EpObjectTable;
    ULONG EreGuidEntry;
    ULONG HtHandleContentionEvent;
    ULONG OtName;
    ULONG OtIndex;
    ULONG ObDecodeShift;
    ULONG ObAttributesShift;
    ULONG AlpcCommunicationInfo;
    ULONG AlpcOwnerProcess;
    ULONG AlpcConnectionPort;
    ULONG AlpcServerCommunicationPort;
    ULONG AlpcClientCommunicationPort;
    ULONG AlpcHandleTable;
    ULONG AlpcHandleTableLock;
    ULONG AlpcAttributes;
    ULONG AlpcAttributesFlags;
    ULONG AlpcPortContext;
    ULONG AlpcPortObjectLock;
    ULONG AlpcSequenceNo;
    ULONG AlpcState;
    ULONG KtInitialStack;
    ULONG KtStackLimit;
    ULONG KtStackBase;
    ULONG KtKernelStack;
    ULONG KtReadOperationCount;
    ULONG KtWriteOperationCount;
    ULONG KtOtherOperationCount;
    ULONG KtReadTransferCount;
    ULONG KtWriteTransferCount;
    ULONG KtOtherTransferCount;
    ULONG MmSectionControlArea;
    ULONG MmControlAreaListHead;
    ULONG MmControlAreaLock;
    ULONG EpSectionObject;

    ULONG LxPicoProc;
    ULONG LxPicoProcInfo;
    ULONG LxPicoProcInfoPID;
    ULONG LxPicoThrdInfo;
    ULONG LxPicoThrdInfoTID;

    BCRYPT_KEY_HANDLE SessionTokenPublicKeyHandle;
} KPH_DYN, *PKPH_DYN;

_Must_inspect_result_
PKPH_DYN KphReferenceDynData(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphActivateDynData(
    _In_ PBYTE DynData,
    _In_ ULONG DynDataLength,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeDynData(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupDynData(
    VOID
    );

// object

#ifdef _X86_
#define KERNEL_HANDLE_BIT (0x80000000)
#else
#define KERNEL_HANDLE_BIT (0xffffffff80000000)
#endif

#define IsKernelHandle(Handle) ((LONG_PTR)(Handle) < 0)
#define MakeKernelHandle(Handle) ((HANDLE)((ULONG_PTR)(Handle) | KERNEL_HANDLE_BIT))
#define IsPseudoHandle(Handle) (((ULONG_PTR)(Handle) <= (ULONG_PTR)-1) && \
                                ((ULONG_PTR)(Handle) >= (ULONG_PTR)-6))

_Must_inspect_result_
PVOID KphObpDecodeObject(
    _In_ PKPH_DYN Dyn,
    _In_ PHANDLE_TABLE_ENTRY HandleTableEntry
    );

ULONG KphObpGetHandleAttributes(
    _In_ PKPH_DYN Dyn,
    _In_ PHANDLE_TABLE_ENTRY HandleTableEntry
    );

_When_(NT_SUCCESS(return), _Acquires_lock_(Process))
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphReferenceProcessHandleTable(
    _In_ PKPH_DYN Dyn,
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
    _In_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryNameObject(
    _In_ PVOID Object,
    _Out_writes_bytes_opt_(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryNameFileObject(
    _In_ PFILE_OBJECT FileObject,
    _Out_writes_bytes_opt_(BufferLength) POBJECT_NAME_INFORMATION Buffer,
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

// driver

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
    _In_ KPH_DRIVER_INFORMATION_CLASS DriverInformationClass,
    _Out_writes_bytes_opt_(DriverInformationLength) PVOID DriverInformation,
    _In_ ULONG DriverInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

// device

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenDevice(
    _Out_ PHANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenDeviceDriver(
    _In_ HANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE DriverHandle,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphOpenDeviceBaseDevice(
    _In_ HANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE BaseDeviceHandle,
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
NTSTATUS KphOpenThreadProcess(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE ProcessHandle,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCaptureStackBackTraceThreadByHandle(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID* BackTrace,
    _Out_ PULONG CapturedFrames,
    _Out_opt_ PULONG BackTraceHash,
    _In_ ULONG Flags,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPROCESSOR_MODE AccessMode
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

#pragma deprecated("memcmp")
#pragma deprecated("RtlCompareMemory")
#pragma deprecated("RtlEqualMemory")

_Must_inspect_result_
FORCEINLINE
INT KphCompareMemory(
    _In_reads_bytes_(Length) PVOID Buffer1,
    _In_reads_bytes_(Length) PVOID Buffer2,
    _In_ SIZE_T Length
    )
{
#pragma warning(suppress: 4995) // suppress deprecation warning
    return memcmp(Buffer1, Buffer2, Length);
}

_Must_inspect_result_
FORCEINLINE
BOOLEAN KphEqualMemory(
    _In_reads_bytes_(Length) PVOID Buffer1,
    _In_reads_bytes_(Length) PVOID Buffer2,
    _In_ SIZE_T Length
    )
{
#pragma warning(suppress: 4995) // suppress deprecation warning
    return (memcmp(Buffer1, Buffer2, Length) == 0);
}

typedef
_Function_class_(KPH_BINARY_SEARCH_CALLBACK)
INT
KSIAPI
KPH_BINARY_SEARCH_CALLBACK(
    _In_opt_ PVOID Context,
    _In_ PCVOID Key,
    _In_ PCVOID Element
    );
typedef KPH_BINARY_SEARCH_CALLBACK* PKPH_BINARY_SEARCH_CALLBACK;

_Must_inspect_result_
_Success_(return != NULL)
PVOID KphBinarySearch(
    _In_ PCVOID Key,
    _In_reads_bytes_(NumberOfElements * SizeOfElement) PCVOID Base,
    _In_ ULONG NumberOfElements,
    _In_ ULONG SizeOfElement,
    _In_ PKPH_BINARY_SEARCH_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

typedef
_Function_class_(KPH_QUICK_SORT_CALLBACK)
INT
KSIAPI
KPH_QUICK_SORT_CALLBACK(
    _In_opt_ PVOID Context,
    _In_ PCVOID LeftElement,
    _In_ PCVOID RightElement
    );
typedef KPH_QUICK_SORT_CALLBACK* PKPH_QUICK_SORT_CALLBACK;

VOID KphQuickSort(
    _Inout_updates_bytes_(NumberOfElements * SizeOfElement) PVOID Base,
    _In_ ULONG NumberOfElements,
    _In_ ULONG SizeOfElement,
    _In_ PKPH_QUICK_SORT_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

_Must_inspect_result_
_Success_(return != NULL)
PVOID KphSearchMemory(
    _In_reads_bytes_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _In_reads_bytes_(PatternLength) PVOID Pattern,
    _In_ ULONG PatternLength
    );

typedef EX_RUNDOWN_REF KPH_RUNDOWN;
typedef KPH_RUNDOWN* PKPH_RUNDOWN;

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

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
BOOLEAN KphIsSameFile(
    _In_ PFILE_OBJECT FirstFileObject,
    _In_ PFILE_OBJECT SecondFileObject
    );

typedef struct _KPH_REFERENCE
{
    volatile LONG Count;
} KPH_REFERENCE, *PKPH_REFERENCE;

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphAcquireReference(
    _Inout_ PKPH_REFERENCE Reference,
    _Out_opt_ PLONG PreviousCount
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphReleaseReference(
    _Inout_ PKPH_REFERENCE Reference,
    _Out_opt_ PLONG PreviousCount
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
    _In_ PCUNICODE_STRING ValueName,
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
    _In_ PCUNICODE_STRING ValueName,
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
    _In_ PCUNICODE_STRING ValueName,
    _Out_ PULONG Value
    );

#define KPH_MAP_DATA     0x00000000ul
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
NTSTATUS KphGetNameFileObject(
    _In_ PFILE_OBJECT FileObject,
    _Outptr_allocatesMem_ PUNICODE_STRING* FileName
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphFreeNameFileObject(
    _In_freesMem_ PUNICODE_STRING FileName
    );

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN KphSinglePrivilegeCheckEx(
    _In_ LUID PrivilegeValue,
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN KphSinglePrivilegeCheck(
    _In_ LUID PrivilegeValue,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetFileVersion(
    _In_ PCUNICODE_STRING FileName,
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
    _In_ PVOID VirtualAddress
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphDisableXfgOnTarget(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetFileNameFinalComponent(
    _In_ PCUNICODE_STRING FileName,
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

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenParametersKey(
    _In_ PCUNICODE_STRING RegistryPath,
    _Out_ PHANDLE KeyHandle
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphDominationCheck(
    _In_ PEPROCESS Process,
    _In_ PEPROCESS ProcessTarget,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphDominationAndPrivilegeCheck(
    _In_ ULONG Privileges,
    _In_ PETHREAD Thread,
    _In_ PEPROCESS ProcessTarget,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG64 KphGetProcessSequenceNumber(
    _In_ PEPROCESS Process
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG64 KphGetProcessStartKey(
    _In_ PEPROCESS Process
    );

#define KphGetCurrentProcessStartKey() KphGetProcessStartKey(PsGetCurrentProcess())
#define KphGetThreadProcessStartKey(thread) KphGetProcessStartKey(PsGetThreadProcess(thread))

_IRQL_requires_max_(DISPATCH_LEVEL)
PVOID KphGetCurrentThreadSubProcessTag(
    VOID
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
PVOID KphGetThreadSubProcessTagEx(
    _In_ PETHREAD Thread,
    _In_ BOOLEAN CacheOnly
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
PVOID KphGetThreadSubProcessTag(
    _In_ PETHREAD Thread
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetSigningLevel(
    _In_ PFILE_OBJECT FileObject,
    _Out_ PSE_SIGNING_LEVEL SigningLevel
    );

typedef union _KPH_IMAGE_NT_HEADERS
{
    PIMAGE_NT_HEADERS Headers;
    PIMAGE_NT_HEADERS32 Headers32;
    PIMAGE_NT_HEADERS64 Headers64;
} KPH_IMAGE_NT_HEADERS, *PKPH_IMAGE_NT_HEADERS;

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphImageNtHeader(
    _In_ PVOID Base,
    _In_ ULONG64 Size,
    _Out_ PKPH_IMAGE_NT_HEADERS Headers
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCaptureUnicodeString(
    _In_ PUNICODE_STRING UnicodeString,
    _Out_ PUNICODE_STRING* CapturedUnicodeString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphReleaseUnicodeString(
    _In_ PUNICODE_STRING UnicodeString
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphCopyToMode(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_reads_bytes_(Length) PVOID Source,
    _In_ SIZE_T Length,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphWriteUCharToMode(
    _Out_ PUCHAR Destination,
    _In_ UCHAR Source,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphWriteULongToMode(
    _Out_ PULONG Destination,
    _In_ ULONG Source,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphWriteULong64ToMode(
    _Out_ PULONG64 Destination,
    _In_ ULONG64 Source,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphWriteHandleToMode(
    _Out_ PHANDLE Destination,
    _In_ _Post_ptr_invalid_ HANDLE Source,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphCopyUnicodeStringToMode(
    _Out_writes_bytes_opt_(Length) PVOID Destination,
    _In_ SIZE_T Length,
    _In_opt_ PCUNICODE_STRING String,
    _Out_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

// lsa

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphProcessIsLsass(
    _In_ PEPROCESS Process,
    _Out_ PBOOLEAN IsLsass
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInvalidateLsass(
    _In_ HANDLE ProcessId
    );

// vm

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphReadVirtualMemory(
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
    _Out_writes_bytes_opt_(SectionInformationLength) PVOID SectionInformation,
    _In_ ULONG SectionInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

typedef struct _KPH_VM_TLS_CREATE_DATA_SECTION
{
    NTSTATUS Status;
    HANDLE SectionHandle;
    LARGE_INTEGER SectionFileSize;
    KPROCESSOR_MODE AccessMode;
} KPH_VM_TLS_CREATE_DATA_SECTION, *PKPH_VM_TLS_CREATE_DATA_SECTION;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ KPH_MEMORY_INFORMATION_CLASS MemoryInformationClass,
    _Out_writes_bytes_opt_(MemoryInformationLength) PVOID MemoryInformation,
    _In_ ULONG MemoryInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

// hash

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
    _In_ KPH_HASH_ALGORITHM HashAlgorithm,
    _Out_ PKPH_HASH_INFORMATION HashInformation
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryHashInformationFile(
    _In_ HANDLE FileHandle,
    _Inout_ PKPH_HASH_INFORMATION HashInformation,
    _In_ ULONG HashInformationLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryHashInformationFileObject(
    _In_ PFILE_OBJECT FileObject,
    _Inout_ PKPH_HASH_INFORMATION HashInformation,
    _In_ ULONG HashInformationLength
    );

// kphobject

typedef struct _KPH_OBJECT_HEADER
{
    volatile SSIZE_T PointerCount;
    UCHAR TypeIndex;
    SLIST_ENTRY ListEntry;
    QUAD Body;
} KPH_OBJECT_HEADER, *PKPH_OBJECT_HEADER;

C_ASSERT((FIELD_OFFSET(KPH_OBJECT_HEADER, Body) % MEMORY_ALLOCATION_ALIGNMENT) == 0);

#define KphObjectToObjectHeader(x) ((PKPH_OBJECT_HEADER)CONTAINING_RECORD((PCHAR)x, KPH_OBJECT_HEADER, Body))
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

    union
    {
        struct
        {
            ULONG DeferDelete : 1;
            ULONG Spare : 31;
        };

        ULONG Flags;
    };
} KPH_OBJECT_TYPE_INFO, *PKPH_OBJECT_TYPE_INFO;

typedef struct _KPH_OBJECT_TYPE
{
    UNICODE_STRING Name;
    UCHAR Index;
    volatile SIZE_T TotalNumberOfObjects;
    volatile SIZE_T HighWaterNumberOfObjects;
    KPH_OBJECT_TYPE_INFO TypeInfo;
} KPH_OBJECT_TYPE, *PKPH_OBJECT_TYPE;

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphObjectInitialize(
    VOID
    );

VOID KphCreateObjectType(
    _In_ PCUNICODE_STRING TypeName,
    _In_ PKPH_OBJECT_TYPE_INFO TypeInfo,
    _Outptr_ PKPH_OBJECT_TYPE* ObjectType
    );

_Must_inspect_result_
NTSTATUS KphCreateObject(
    _In_ PKPH_OBJECT_TYPE ObjectType,
    _In_ ULONG ObjectBodySize,
    _Outptr_result_nullonfailure_ PVOID* Object,
    _In_opt_ PVOID Parameter
    );

VOID KphReferenceObject(
    _In_ PVOID Object
    );

VOID KphDereferenceObject(
    _In_ PVOID Object
    );

VOID KphDereferenceObjectDeferDelete(
    _In_ PVOID Object
    );

_Must_inspect_result_
PKPH_OBJECT_TYPE KphGetObjectType(
    _In_ PVOID Object
    );

typedef struct _KPH_ATOMIC_OBJECT_REF
{
    volatile ULONG_PTR Object;
} KPH_ATOMIC_OBJECT_REF, *PKPH_ATOMIC_OBJECT_REF;

#define KPH_ATOMIC_OBJECT_REF_INIT { .Object = 0 }

_Must_inspect_result_
PVOID KphAtomicReferenceObject(
    _In_ PKPH_ATOMIC_OBJECT_REF ObjectRef
    );

VOID KphAtomicAssignObjectReference(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef,
    _In_opt_ PVOID Object
    );

_Must_inspect_result_
PVOID KphAtomicMoveObjectReference(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef,
    _In_opt_ PVOID Object
    );

// cid_table

typedef struct _KPH_CID_TABLE_ENTRY
{
    KPH_ATOMIC_OBJECT_REF ObjectRef;
} KPH_CID_TABLE_ENTRY, *PKPH_CID_TABLE_ENTRY;

typedef struct _KPH_CID_TABLE
{
    KSPIN_LOCK Lock;
    volatile ULONG_PTR Table;
} KPH_CID_TABLE, *PKPH_CID_TABLE;

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphCidTableCreate(
    _Out_ PKPH_CID_TABLE Table
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphCidTableDelete(
    _In_ PKPH_CID_TABLE Table
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphCidAssignObject(
    _Inout_ PKPH_CID_TABLE_ENTRY Entry,
    _In_opt_ PVOID Object
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
PVOID KphCidReferenceObject(
    _In_ PKPH_CID_TABLE_ENTRY Entry
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
PKPH_CID_TABLE_ENTRY KphCidGetEntry(
    _In_ HANDLE Cid,
    _Inout_ PKPH_CID_TABLE Table
    );

typedef
_Function_class_(KPH_CID_ENUMERATE_CALLBACK)
BOOLEAN
KSIAPI
KPH_CID_ENUMERATE_CALLBACK(
    _In_ PVOID Object,
    _In_opt_ PVOID Parameter
    );
typedef KPH_CID_ENUMERATE_CALLBACK* PKPH_CID_ENUMERATE_CALLBACK;

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphCidEnumerate(
    _In_ PKPH_CID_TABLE Table,
    _In_ PKPH_CID_ENUMERATE_CALLBACK Callback,
    _In_opt_ PVOID Parameter
    );

typedef
_Function_class_(KPH_CID_RUNDOWN_CALLBACK)
VOID
KSIAPI
KPH_CID_RUNDOWN_CALLBACK(
    _In_ PVOID Object,
    _In_opt_ PVOID Parameter
    );
typedef KPH_CID_RUNDOWN_CALLBACK* PKPH_CID_RUNDOWN_CALLBACK;

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphCidRundown(
    _In_ PKPH_CID_TABLE Table,
    _In_ PKPH_CID_RUNDOWN_CALLBACK Callback,
    _In_opt_ PVOID Parameter
    );

// cid_tracking

#define KPH_PROTECTED_PROCESS_MASK (KPH_PROCESS_READ_ACCESS                   |\
                                    PROCESS_TERMINATE                         |\
                                    PROCESS_SUSPEND_RESUME)
#define KPH_PROTECTED_THREAD_MASK  (KPH_THREAD_READ_ACCESS                    |\
                                    THREAD_TERMINATE                          |\
                                    THREAD_SUSPEND_RESUME                     |\
                                    THREAD_RESUME)

typedef union _KPH_SESSION_TOKEN_ATOMIC
{
    struct _KPH_SESSION_TOKEN* Token;
    KPH_ATOMIC_OBJECT_REF Atomic;
} KPH_SESSION_TOKEN_ATOMIC, *PKPH_SESSION_TOKEN_ATOMIC;

typedef struct _KPH_PROCESS_CONTEXT
{
    PEPROCESS EProcess;

    HANDLE ProcessId;
    ULONG64 SequenceNumber;
    ULONG64 ProcessStartKey;
    CLIENT_ID CreatorClientId;

    PUNICODE_STRING ImageFileName;
    UNICODE_STRING ImageName;
    PFILE_OBJECT FileObject;

    volatile SIZE_T NumberOfImageLoads;

    KPH_SESSION_TOKEN_ATOMIC SessionToken;
    KPH_INFORMER_SETTINGS InformerFilter;

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
            ULONG IsWow64 : 1;
            ULONG IsSubsystemProcess : 1;
            ULONG AllocatedImageName : 1;
            ULONG SystemAllocatedImageFileName : 1;
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

    union
    {
        volatile ULONG Flags;
        struct
        {
            ULONG Debugged : 1;
            ULONG FileObjectWritable : 1;
            ULONG FileObjectTransaction : 1;
            ULONG UserWritableReferences : 1;
            ULONG Reserved : 28;
        };
    } StateTracking;

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

typedef enum _KPH_PROCESS_CONTEXT_INFORMATION_CLASS
{
    KphProcessContextWSLProcessId, // q: ULONG
} KPH_PROCESS_CONTEXT_INFORMATION_CLASS, *PKPH_PROCESS_CONTEXT_INFORMATION_CLASS;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryInformationProcessContext(
    _In_ PKPH_PROCESS_CONTEXT Process,
    _In_ KPH_PROCESS_CONTEXT_INFORMATION_CLASS InformationClass,
    _Out_writes_bytes_opt_(InformationLength) PVOID Information,
    _In_ ULONG InformationLength,
    _Out_opt_ PULONG ReturnLength
    );

typedef struct _KPH_THREAD_CONTEXT
{
    PETHREAD EThread;

    PKPH_PROCESS_CONTEXT ProcessContext;

    CLIENT_ID ClientId;
    CLIENT_ID CreatorClientId;

    PVOID SubProcessTag;

    KPH_SESSION_TOKEN_ATOMIC SessionToken;
    KPH_SESSION_TOKEN_ATOMIC RequestSessionToken;

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
            ULONG CapturingUserModeStack : 1;
            ULONG Reserved : 26;
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

    PVOID VmTlsCreateDataSection;
    PVOID VmTlsMappedInformation;
} KPH_THREAD_CONTEXT, *PKPH_THREAD_CONTEXT;

extern PKPH_OBJECT_TYPE KphThreadContextType;

typedef enum _KPH_THREAD_CONTEXT_INFORMATION_CLASS
{
    KphThreadContextWSLThreadId,  // q: ULONG
} KPH_THREAD_CONTEXT_INFORMATION_CLASS, *PKPH_THREAD_CONTEXT_INFORMATION_CLASS;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryInformationThreadContext(
    _In_ PKPH_THREAD_CONTEXT Thread,
    _In_ KPH_THREAD_CONTEXT_INFORMATION_CLASS InformationClass,
    _Out_writes_bytes_opt_(InformationLength) PVOID Information,
    _In_ ULONG InformationLength,
    _Out_opt_ PULONG ReturnLength
    );

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

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
PKPH_PROCESS_CONTEXT KphGetSystemProcessContext(
    VOID
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
PKPH_PROCESS_CONTEXT KphGetProcessContext(
    _In_ HANDLE ProcessId
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
PKPH_PROCESS_CONTEXT KphGetEProcessContext(
    _In_ PEPROCESS Process
    );

#define KphGetCurrentProcessContext() KphGetEProcessContext(PsGetCurrentProcess())

_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
PKPH_THREAD_CONTEXT KphGetThreadContext(
    _In_ HANDLE ThreadId
    );

#define KphGetEThreadContext(thread) KphGetThreadContext(PsGetThreadId(thread))

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

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
PKPH_THREAD_CONTEXT KphTrackThreadContext(
    _In_ PETHREAD Thread
    );

_IRQL_requires_max_(APC_LEVEL)
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

_IRQL_requires_max_(APC_LEVEL)
KPH_PROCESS_STATE KphGetProcessState(
    _In_ PKPH_PROCESS_CONTEXT Process
    );

_IRQL_requires_max_(APC_LEVEL)
FORCEINLINE
BOOLEAN KphTestProcessState(
    _In_ KPH_PROCESS_STATE ProcessState,
    _In_ KPH_PROCESS_STATE State
    )
{
    return ((ProcessState & State) == State);
}

_IRQL_requires_max_(APC_LEVEL)
FORCEINLINE
BOOLEAN KphTestProcessContextState(
    _In_ PKPH_PROCESS_CONTEXT Process,
    _In_ KPH_PROCESS_STATE State
    )
{
    return KphTestProcessState(KphGetProcessState(Process), State);
}

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

_IRQL_requires_max_(APC_LEVEL)
VOID KphStopProtectingProcess(
    _In_ PKPH_PROCESS_CONTEXT Process
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphIsProtectedProcess(
    _In_ PKPH_PROCESS_CONTEXT Process
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphApplyObProtections(
    _Inout_ POB_PRE_OPERATION_INFORMATION Info
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphApplyImageProtections(
    _Inout_ PKPH_PROCESS_CONTEXT Process,
    _In_ PIMAGE_INFO_EX ImageInfo
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphAcquireDriverUnloadProtection(
    _Out_opt_ PLONG PreviousCount
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphReleaseDriverUnloadProtection(
    _Out_opt_ PLONG PreviousCount
    );

_IRQL_requires_max_(APC_LEVEL)
LONG KphGetDriverUnloadProtectionCount(
    VOID
    );

FORCEINLINE
BOOLEAN KphIsDriverUnloadProtected(
    VOID
    )
{
    return (KphGetDriverUnloadProtectionCount() > 0);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphStripProtectedProcessMasks(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK ProcessAllowedMask,
    _In_ ACCESS_MASK ThreadAllowedMask,
    _In_ KPROCESSOR_MODE AccessMode
    );

// imgcoherency

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCheckImageCoherency(
    _In_ PVOID ImageBase,
    _In_ SIZE_T ImageSize,
    _In_ PVOID DataBase,
    _In_ SIZE_T DataSize
    );

// verify

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphVerifyCloseKey(
    _In_ BCRYPT_KEY_HANDLE KeyHandle
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphVerifyCreateKey(
    _Out_ BCRYPT_KEY_HANDLE* KeyHandle,
    _In_ PBYTE KeyMaterial,
    _In_ ULONG KeyMaterialLength
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphVerifyBufferEx(
    _In_opt_ BCRYPT_KEY_HANDLE KeyHandle,
    _In_ PBYTE Buffer,
    _In_ ULONG BufferLength,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength
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
NTSTATUS KphVerifyFileObject(
    _In_ PFILE_OBJECT FileObject,
    _In_opt_ PCUNICODE_STRING FileName
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphVerifyFile(
    _In_ PCUNICODE_STRING FileName,
    _In_opt_ PFILE_OBJECT FileObject
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphInitializeVerify(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupVerify(
    VOID
    );

// ksi.dll

#if !defined(_KSIDLL_)
#define KSISYSAPI DECLSPEC_IMPORT
#else
#define KSISYSAPI
#endif

#define KSIDLL_CURRENT_VERSION 1

KSISYSAPI
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
KSIAPI
KsiInitialize(
    _In_ ULONG Version,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_opt_ PVOID Reserved
    );

KSISYSAPI
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
KSIAPI
KsiUninitialize(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ ULONG Reserved
    );

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
    KsiApcCleanupRundown,
    KsiApcCleanupRemoved
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
    _Inout_ PKSI_KAPC Apc,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2,
    _In_ KPRIORITY PriorityBoost
    );

KSISYSAPI
NTSTATUS
KSIAPI
KsiRemoveQueueApc(
    _Inout_ PKSI_KAPC Apc
    );

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(KSI_WORK_QUEUE_ROUTINE)
VOID
KSIAPI
KSI_WORK_QUEUE_ROUTINE(
    _In_opt_ PVOID Parameter
    );
typedef KSI_WORK_QUEUE_ROUTINE *PKSI_WORK_QUEUE_ROUTINE;

typedef struct _KSI_WORK_QUEUE_ITEM
{
    WORK_QUEUE_ITEM WorkItem;
    PDRIVER_OBJECT DriverObject;
    PKSI_WORK_QUEUE_ROUTINE Routine;
    PVOID Parameter;
} KSI_WORK_QUEUE_ITEM, *PKSI_WORK_QUEUE_ITEM;

_IRQL_requires_max_(DISPATCH_LEVEL)
KSISYSAPI
VOID
KSIAPI
KsiInitializeWorkItem(
    _Out_ PKSI_WORK_QUEUE_ITEM WorkItem,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PKSI_WORK_QUEUE_ROUTINE Routine,
    _In_opt_ PVOID Parameter
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
KSISYSAPI
VOID
KSIAPI
KsiQueueWorkItem(
    _Inout_ PKSI_WORK_QUEUE_ITEM WorkItem,
    _In_ WORK_QUEUE_TYPE QueueType
    );

extern KSISYSAPI PEPROCESS KsiSystemProcess;

_IRQL_requires_max_(PASSIVE_LEVEL)
KSISYSAPI
NTSTATUS
KSIAPI
KsiInitializeSystemProcess(
    _In_ PCUNICODE_STRING ProcessName
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

// back_trace

#define KPH_STACK_BACK_TRACE_USER_MODE   0x00000001ul
#define KPH_STACK_BACK_TRACE_SKIP_KPH    0x00000002ul
#define KPH_STACK_BACK_TRACE_NO_SENTINEL 0x00000004ul

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphInitializeStackBackTrace(
    VOID
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Success_(return != 0)
ULONG KphCaptureStackBackTrace(
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID* BackTrace,
    _Out_opt_ PULONG BackTraceHash,
    _In_ ULONG Flags
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphCaptureStackBackTraceThread(
    _In_ PETHREAD Thread,
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID* BackTrace,
    _Out_ PULONG CapturedFrames,
    _Out_opt_ PULONG BackTraceHash,
    _In_ ULONG Flags,
    _In_opt_ PLARGE_INTEGER Timeout
    );

// session_token

typedef struct _KPH_SESSION_TOKEN
{
    KPH_SESSION_ACCESS_TOKEN AccessToken;
    volatile LONG UseCount;
} KPH_SESSION_TOKEN, *PKPH_SESSION_TOKEN;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphRequestSessionAccessToken(
    _Out_ PKPH_SESSION_ACCESS_TOKEN AccessToken,
    _In_ PLARGE_INTEGER Expiry,
    _In_ ULONG Privileges,
    _In_ LONG Uses
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphAssignProcessSessionToken(
    _In_ HANDLE ProcessHandle,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphAssignThreadSessionToken(
    _In_ HANDLE ThreadHandle,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
BOOLEAN KphSessionTokenPrivilegeCheck(
    _In_ PKPH_THREAD_CONTEXT Thread,
    _In_ ULONG Privileges
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeSessionToken(
    VOID
    );

// ringbuff

typedef struct _KPH_RING_SECTION
{
    PVOID SectionObject;
    PMDL Mdl;
    PVOID KernelBase;
} KPH_RING_SECTION, *PKPH_RING_SECTION;

typedef struct _KPH_RING_BUFFER
{
    KSPIN_LOCK ProducerLock;
    volatile ULONG* ProducerPos;
    volatile ULONG* ConsumerPos;
    ULONG Length;
    PVOID Buffer;
    PKEVENT Event;
    KPH_RING_SECTION ProducerSection;
    KPH_RING_SECTION ConsumerSection;
    PEPROCESS Process;
    PVOID UserProducerBase;
    PVOID UserConsumerBase;
} KPH_RING_BUFFER, *PKPH_RING_BUFFER;

_Return_allocatesMem_size_(Length)
PVOID KphReserveRingBuffer(
    _In_ PKPH_RING_BUFFER Ring,
    _In_ ULONG Length
    );

VOID KphCommitRingBuffer(
    _In_ PKPH_RING_BUFFER Ring,
    _In_freesMem_ PVOID Buffer
    );

VOID KphDiscardRingBuffer(
    _In_ PKPH_RING_BUFFER Ring,
    _In_freesMem_ PVOID Buffer
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCreateRingBuffer(
    _Out_ PKPH_RING_BUFFER* Ring,
    _Out_ PKPH_RING_BUFFER_USER User,
    _In_ ULONG Length,
    _In_opt_ PKEVENT Event,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeRingBuffer(
    VOID
    );

// kphthread

typedef
_Function_class_(KPH_THREAD_START_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
KPH_THREAD_START_ROUTINE(
    _In_opt_ PVOID Parameter
    );
typedef KPH_THREAD_START_ROUTINE *PKPH_THREAD_START_ROUTINE;

#define KPH_CREATE_SYSTEM_THREAD_IN_KSI_PROCESS 0x00000001ul

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphCreateSystemThread(
    _Out_opt_ PHANDLE ThreadHandle,
    _Out_opt_ PETHREAD* ThreadObject,
    _In_ PKPH_THREAD_START_ROUTINE StartRoutine,
    _In_opt_ _When_(return >= 0, __drv_aliasesMem) PVOID Parameter,
    _In_opt_ PCUNICODE_STRING ThreadName,
    _In_ ULONG Flags
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupThreading(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeThreading(
    VOID
    );
