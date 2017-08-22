#ifndef NTFILL_H
#define NTFILL_H

extern ULONG KphDynNtVersion;
extern ULONG KphDynObDecodeShift;
extern ULONG KphDynObAttributesShift;

// EX

typedef struct _EX_PUSH_LOCK_WAIT_BLOCK *PEX_PUSH_LOCK_WAIT_BLOCK;

NTKERNELAPI
VOID
FASTCALL
ExfUnblockPushLock(
    _Inout_ PEX_PUSH_LOCK PushLock,
    _Inout_opt_ PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock
    );

typedef struct _HANDLE_TABLE_ENTRY
{
    union
    {
        PVOID Object;
        ULONG ObAttributes;
        ULONG_PTR Value;
    };
    union
    {
        ACCESS_MASK GrantedAccess;
        LONG NextFreeTableEntry;
    };
} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

typedef struct _HANDLE_TABLE HANDLE_TABLE, *PHANDLE_TABLE;

typedef BOOLEAN (NTAPI *PEX_ENUM_HANDLE_CALLBACK_61)(
    _Inout_ PHANDLE_TABLE_ENTRY HandleTableEntry,
    _In_ HANDLE Handle,
    _In_ PVOID Context
    );

// since WIN8
typedef BOOLEAN (NTAPI *PEX_ENUM_HANDLE_CALLBACK)(
    _In_ PHANDLE_TABLE HandleTable,
    _Inout_ PHANDLE_TABLE_ENTRY HandleTableEntry,
    _In_ HANDLE Handle,
    _In_ PVOID Context
    );

NTKERNELAPI
BOOLEAN
NTAPI
ExEnumHandleTable(
    _In_ PHANDLE_TABLE HandleTable,
    _In_ PEX_ENUM_HANDLE_CALLBACK EnumHandleProcedure,
    _Inout_ PVOID Context,
    _Out_opt_ PHANDLE Handle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

// IO

extern POBJECT_TYPE *IoDriverObjectType;

// KE

typedef enum _KAPC_ENVIRONMENT
{
    OriginalApcEnvironment,
    AttachedApcEnvironment,
    CurrentApcEnvironment,
    InsertApcEnvironment
} KAPC_ENVIRONMENT, *PKAPC_ENVIRONMENT;

typedef VOID (NTAPI *PKNORMAL_ROUTINE)(
    _In_ PVOID NormalContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2
    );

typedef VOID KKERNEL_ROUTINE(
    _In_ PRKAPC Apc,
    _Inout_ PKNORMAL_ROUTINE *NormalRoutine,
    _Inout_ PVOID *NormalContext,
    _Inout_ PVOID *SystemArgument1,
    _Inout_ PVOID *SystemArgument2
    );

typedef KKERNEL_ROUTINE (NTAPI *PKKERNEL_ROUTINE);

typedef VOID (NTAPI *PKRUNDOWN_ROUTINE)(
    _In_ PRKAPC Apc
    );

NTKERNELAPI
VOID
NTAPI
KeInitializeApc(
    _Out_ PRKAPC Apc,
    _In_ PRKTHREAD Thread,
    _In_ KAPC_ENVIRONMENT Environment,
    _In_ PKKERNEL_ROUTINE KernelRoutine,
    _In_opt_ PKRUNDOWN_ROUTINE RundownRoutine,
    _In_opt_ PKNORMAL_ROUTINE NormalRoutine,
    _In_opt_ KPROCESSOR_MODE ProcessorMode,
    _In_opt_ PVOID NormalContext
    );

NTKERNELAPI
BOOLEAN
NTAPI
KeInsertQueueApc(
    _Inout_ PRKAPC Apc,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2,
    _In_ KPRIORITY Increment
    );

// MM

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ MEMORY_INFORMATION_CLASS MemoryInformationClass,
    _Out_writes_bytes_(MemoryInformationLength) PVOID MemoryInformation,
    _In_ SIZE_T MemoryInformationLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

// OB

// These definitions are no longer correct, but they produce correct results.

#define OBJ_PROTECT_CLOSE 0x00000001
#define OBJ_HANDLE_ATTRIBUTES (OBJ_PROTECT_CLOSE | OBJ_INHERIT | OBJ_AUDIT_OBJECT_CLOSE)

// This attribute is now stored in the GrantedAccess field.
#define ObpAccessProtectCloseBit 0x2000000

#define ObpDecodeGrantedAccess(Access) \
    ((Access) & ~ObpAccessProtectCloseBit)

FORCEINLINE PVOID ObpDecodeObject(PVOID Object)
{
#ifdef _M_X64
    if (KphDynNtVersion >= PHNT_WIN8)
    {
        if (KphDynObDecodeShift != -1)
            return (PVOID)(((LONG_PTR)Object >> KphDynObDecodeShift) & ~(ULONG_PTR)0xf);
        else
            return NULL;
    }
    else
    {
        return (PVOID)((ULONG_PTR)Object & ~OBJ_HANDLE_ATTRIBUTES);
    }
#else
    return (PVOID)((ULONG_PTR)Object & ~OBJ_HANDLE_ATTRIBUTES);
#endif
}

FORCEINLINE ULONG ObpGetHandleAttributes(PHANDLE_TABLE_ENTRY HandleTableEntry)
{
#ifdef _M_X64
    if (KphDynNtVersion >= PHNT_WIN8)
    {
        if (KphDynObAttributesShift != -1)
            return (ULONG)(HandleTableEntry->Value >> KphDynObAttributesShift) & 0x3;
        else
            return 0;
    }
    else
    {
        return (HandleTableEntry->ObAttributes & (OBJ_INHERIT | OBJ_AUDIT_OBJECT_CLOSE)) |
            ((HandleTableEntry->GrantedAccess & ObpAccessProtectCloseBit) ? OBJ_PROTECT_CLOSE : 0);
    }
#else
    return (HandleTableEntry->ObAttributes & (OBJ_INHERIT | OBJ_AUDIT_OBJECT_CLOSE)) |
        ((HandleTableEntry->GrantedAccess & ObpAccessProtectCloseBit) ? OBJ_PROTECT_CLOSE : 0);
#endif
}

typedef struct _OBJECT_CREATE_INFORMATION OBJECT_CREATE_INFORMATION, *POBJECT_CREATE_INFORMATION;

// This is incorrect as of Windows 8.1, but the size of the structure is still correct.
typedef struct _OBJECT_HEADER
{
    LONG PointerCount;
    union
    {
        LONG HandleCount;
        PVOID NextToFree;
    };
    POBJECT_TYPE Type;
    UCHAR NameInfoOffset;
    UCHAR HandleInfoOffset;
    UCHAR QuotaInfoOffset;
    UCHAR Flags;
    union
    {
        POBJECT_CREATE_INFORMATION ObjectCreateInfo;
        PVOID QuotaBlockCharged;
    };
    PVOID SecurityDescriptor;
    QUAD Body;
} OBJECT_HEADER, *POBJECT_HEADER;

#define OBJECT_TO_OBJECT_HEADER(Object) CONTAINING_RECORD((Object), OBJECT_HEADER, Body)

NTKERNELAPI
POBJECT_TYPE
NTAPI
ObGetObjectType(
    _In_ PVOID Object
    );

NTKERNELAPI
NTSTATUS
NTAPI
ObOpenObjectByName(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ POBJECT_TYPE ObjectType,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_opt_ PACCESS_STATE AccessState,
    _In_opt_ ACCESS_MASK DesiredAccess,
    _In_ PVOID ParseContext,
    _Out_ PHANDLE Handle
    );

NTKERNELAPI
NTSTATUS
NTAPI
ObSetHandleAttributes(
    _In_ HANDLE Handle,
    _In_ POBJECT_HANDLE_FLAG_INFORMATION HandleFlags,
    _In_ KPROCESSOR_MODE PreviousMode
    );

NTKERNELAPI
NTSTATUS
ObCloseHandle(
    _In_ HANDLE Handle,
    _In_ KPROCESSOR_MODE PreviousMode
    );

// PS

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _In_reads_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _Out_writes_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

NTKERNELAPI
NTSTATUS
NTAPI
PsLookupProcessThreadByCid(
    _In_ PCLIENT_ID ClientId,
    _Out_opt_ PEPROCESS *Process,
    _Out_ PETHREAD *Thread
    );

NTKERNELAPI
PVOID
NTAPI
PsGetThreadWin32Thread(
    _In_ PETHREAD Thread
    );

typedef struct _EJOB *PEJOB;

extern POBJECT_TYPE *PsJobType;

NTKERNELAPI
PEJOB
NTAPI
PsGetProcessJob(
    _In_ PEPROCESS Process
    );

NTKERNELAPI
NTSTATUS
NTAPI
PsAcquireProcessExitSynchronization(
    _In_ PEPROCESS Process
    );

NTKERNELAPI
VOID
NTAPI
PsReleaseProcessExitSynchronization(
    _In_ PEPROCESS Process
    );

// RTL

// Sensible limit that may or may not correspond to the actual Windows value.
#define MAX_STACK_DEPTH 256

#define RTL_WALK_USER_MODE_STACK 0x00000001
#define RTL_WALK_VALID_FLAGS 0x00000001

NTSYSAPI
ULONG
NTAPI
RtlWalkFrameChain(
    _Out_ PVOID *Callers,
    _In_ ULONG Count,
    _In_ ULONG Flags
    );

#endif
