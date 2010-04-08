/*
 * Process Hacker Driver - 
 *   custom APIs
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

#ifndef _KPH_H
#define _KPH_H

#include <ntifs.h>

#include "debug.h"
#include "ref.h"
#include "util.h"
#include "version.h"

#include "ex.h"
#include "io.h"
#include "ke.h"
#include "mm.h"
#include "ob.h"
#include "ps.h"
#include "se.h"
#include "zw.h"
 
#include "trace.h"

#define MAX_UINTEGER(Bits) ((1 << (Bits)) - 1)
#define BITS_UCHAR 8
#define MAX_UCHAR MAX_UINTEGER(BITS_UCHAR)
#define BITS_USHORT 16
#define MAX_USHORT MAX_UINTEGER(BITS_USHORT)
#define BITS_ULONG 32
#define MAX_ULONG MAX_UINTEGER(BITS_ULONG)

#define SYSTEM_PROCESS_ID ((HANDLE)4)
#define KERNEL_HANDLE_BIT ((ULONG_PTR)1 << (sizeof(HANDLE) * 8 - 1))
#define IsKernelHandle(Handle) ((LONG_PTR)(Handle) < 0)
#define MakeKernelHandle(Handle) ((ULONG_PTR)(Handle) |= KERNEL_HANDLE_BIT)

#define PTR_ADD_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) + (ULONG_PTR)(Offset)))

#define GET_BIT(Integer, Bit) (((Integer) >> (Bit)) & 0x1)
#define SET_BIT(Integer, Bit) ((Integer) |= 1 << (Bit))
#define CLEAR_BIT(Integer, Bit) ((Integer) &= ~(1 << (Bit)))

#define KPH_TIMEOUT_TO_SEC ((LONGLONG) 1 * 10 * 1000 * 1000)
#define KPH_REL_TIMEOUT_IN_SEC(Time) (Time * -1 * KPH_TIMEOUT_TO_SEC)

#define TAG_CAPTURED_UNICODE_STRING ('UChP')

#ifdef EXT
#undef EXT
#endif

#ifdef _KPH_PRIVATE
#define EXT
#define EQNULL = NULL
#else
#define EXT extern
#define EQNULL
#endif

EXT POBJECT_TYPE *ObDirectoryObjectType EQNULL;
EXT POBJECT_TYPE *ObTypeObjectType EQNULL;

EXT PKSERVICE_TABLE_DESCRIPTOR __KeServiceDescriptorTable EQNULL;
EXT _NtClose __NtClose EQNULL;
EXT _ObGetObjectType ObGetObjectType EQNULL;
EXT _PsGetProcessJob PsGetProcessJob EQNULL;
EXT _PsResumeProcess PsResumeProcess EQNULL;
EXT _PsSuspendProcess PsSuspendProcess EQNULL;
EXT _PsTerminateProcess __PsTerminateProcess EQNULL;
EXT PVOID __PspTerminateThreadByPointer EQNULL;
EXT _NtClose __ZwClose EQNULL;

/* Driver information */

typedef enum _DRIVER_INFORMATION_CLASS
{
    DriverBasicInformation,
    DriverNameInformation,
    DriverServiceKeyNameInformation,
    MaxDriverInfoClass
} DRIVER_INFORMATION_CLASS;

typedef struct _DRIVER_BASIC_INFORMATION
{
    ULONG Flags;
    PVOID DriverStart;
    ULONG DriverSize;
} DRIVER_BASIC_INFORMATION, *PDRIVER_BASIC_INFORMATION;

typedef struct _KPH_ATTACH_STATE
{
    BOOLEAN Attached;
    PEPROCESS Process;
    KAPC_STATE ApcState;
} KPH_ATTACH_STATE, *PKPH_ATTACH_STATE;

typedef struct _MAPPED_MDL
{
    PMDL Mdl;
    PVOID Address;
} MAPPED_MDL, *PMAPPED_MDL;

typedef struct _PROCESS_HANDLE
{
    HANDLE Handle;
    PVOID Object;
    ACCESS_MASK GrantedAccess;
    ULONG HandleAttributes;
} PROCESS_HANDLE, *PPROCESS_HANDLE;

typedef struct _PROCESS_HANDLE_INFORMATION
{
    ULONG HandleCount;
    PROCESS_HANDLE Handles[1];
} PROCESS_HANDLE_INFORMATION, *PPROCESS_HANDLE_INFORMATION;

/* Support routines */

NTSTATUS KphNtInit();

PVOID GetSystemRoutineAddress(
    WCHAR *Name
    );

VOID KphAttachProcess(
    __in PEPROCESS Process,
    __out PKPH_ATTACH_STATE AttachState
    );

NTSTATUS KphAttachProcessHandle(
    __in HANDLE ProcessHandle,
    __out PKPH_ATTACH_STATE AttachState
    );

NTSTATUS KphAttachProcessId(
    __in HANDLE ProcessId,
    __out PKPH_ATTACH_STATE AttachState
    );

NTSTATUS KphCaptureUnicodeString(
    __in PUNICODE_STRING UnicodeString,
    __out PUNICODE_STRING CapturedUnicodeString
    );

VOID KphDetachProcess(
    __in PKPH_ATTACH_STATE AttachState
    );

VOID KphFreeCapturedUnicodeString(
    __in PUNICODE_STRING CapturedUnicodeString
    );

VOID KphProbeForReadUnicodeString(
    __in PUNICODE_STRING UnicodeString
    );

VOID KphProbeSystemAddressRange(
    __in PVOID BaseAddress,
    __in ULONG Length
    );

NTSTATUS OpenProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ProcessId
    );

NTSTATUS SetProcessToken(
    __in HANDLE SourcePid,
    __in HANDLE TargetPid
    );

/* KProcessHacker */

BOOLEAN KphAcquireProcessRundownProtection(
    __in PEPROCESS Process
    );

NTSTATUS KphAssignImpersonationToken(
    __in HANDLE ThreadHandle,
    __in HANDLE TokenHandle
    );

NTSTATUS KphCaptureStackBackTraceThread(
    __in HANDLE ThreadHandle,
    __in ULONG FramesToSkip,
    __in ULONG FramesToCapture,
    __out_ecount(FramesToCapture) PVOID *BackTrace,
    __out_opt PULONG CapturedFrames,
    __out_opt PULONG BackTraceHash,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphDangerousTerminateThread(
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    );

NTSTATUS KphDuplicateObject(
    __in HANDLE SourceProcessHandle,
    __in HANDLE SourceHandle,
    __in_opt HANDLE TargetProcessHandle,
    __out_opt PHANDLE TargetHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Options,
    __in KPROCESSOR_MODE AccessMode
    );

BOOLEAN KphEnumProcessHandleTable(
    __in PEPROCESS Process,
    __in PEX_ENUM_HANDLE_CALLBACK EnumHandleProcedure,
    __inout PVOID Context,
    __out_opt PHANDLE Handle
    );

NTSTATUS KphGetContextThread(
    __in HANDLE ThreadHandle,
    __inout PCONTEXT ThreadContext,
    __in KPROCESSOR_MODE AccessMode
    );

POBJECT_TYPE KphGetObjectTypeNt(
    __in PVOID Object
    );

HANDLE KphGetProcessId(
    __in HANDLE ProcessHandle
    );

HANDLE KphGetThreadId(
    __in HANDLE ThreadHandle,
    __out_opt PHANDLE ProcessId
    );

NTSTATUS KphGetThreadWin32Thread(
    __in HANDLE ThreadHandle,
    __out PVOID *Win32Thread,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphOpenDirectoryObject(
    __out PHANDLE DirectoryObjectHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphOpenDriver(
    __out PHANDLE DriverHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphOpenNamedObject(
    __out PHANDLE ObjectHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in POBJECT_TYPE ObjectType,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphOpenProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PCLIENT_ID ClientId,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphOpenProcessJob(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE JobHandle,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphOpenProcessTokenEx(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG ObjectAttributes,
    __out PHANDLE TokenHandle,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphOpenThread(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PCLIENT_ID ClientId,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphOpenThreadProcess(
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE ProcessHandle,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphOpenType(
    __out PHANDLE TypeHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphQueryInformationDriver(
    __in HANDLE DriverHandle,
    __in DRIVER_INFORMATION_CLASS DriverInformationClass,
    __out_bcount_opt(DriverInformationLength) PVOID DriverInformation,
    __in ULONG DriverInformationLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphQueryNameFileObject(
    __in PFILE_OBJECT FileObject,
    __inout_bcount(BufferLength) PUNICODE_STRING Buffer,
    __in ULONG BufferLength,
    __out PULONG ReturnLength
    );

NTSTATUS KphQueryNameObject(
    __in PVOID Object,
    __inout_bcount(BufferLength) PUNICODE_STRING Buffer,
    __in ULONG BufferLength,
    __out PULONG ReturnLength
    );

NTSTATUS KphQueryProcessHandles(
    __in HANDLE ProcessHandle,
    __out_bcount_opt(BufferLength) PPROCESS_HANDLE_INFORMATION Buffer,
    __in_opt ULONG BufferLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphReadVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    );

VOID KphReleaseProcessRundownProtection(
    __in PEPROCESS Process
    );

NTSTATUS KphResumeProcess(
    __in HANDLE ProcessHandle
    );

NTSTATUS KphSetContextThread(
    __in HANDLE ThreadHandle,
    __in PCONTEXT ThreadContext,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphSetHandleGrantedAccess(
    __in PEPROCESS Process,
    __in HANDLE Handle,
    __in ACCESS_MASK GrantedAccess
    );

NTSTATUS KphSuspendProcess(
    __in HANDLE ProcessHandle
    );

NTSTATUS KphTerminateProcess(
    __in HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus
    );

NTSTATUS KphTerminateThread(
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    );

NTSTATUS KphUnsafeReadVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphWriteVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    );

/* MM */

NTSTATUS MiDoMappedCopy(
    __in PEPROCESS FromProcess,
    __in PVOID FromAddress,
    __in PEPROCESS ToProcess,
    __in PVOID ToAddress,
    __in ULONG BufferLength,
    __in KPROCESSOR_MODE AccessMode,
    __out PULONG ReturnLength
    );

NTSTATUS MiDoPoolCopy(
    __in PEPROCESS FromProcess,
    __in PVOID FromAddress,
    __in PEPROCESS ToProcess,
    __in PVOID ToAddress,
    __in ULONG BufferLength,
    __in KPROCESSOR_MODE AccessMode,
    __out PULONG ReturnLength
    );

ULONG MiGetExceptionInfo(
    __in PEXCEPTION_POINTERS ExceptionInfo,
    __out PBOOLEAN HaveBadAddress,
    __out PULONG_PTR BadAddress
    );

NTSTATUS MmCopyVirtualMemory(
    __in PEPROCESS FromProcess,
    __in PVOID FromAddress,
    __in PEPROCESS ToProcess,
    __in PVOID ToAddress,
    __in ULONG BufferLength,
    __in KPROCESSOR_MODE AccessMode,
    __out PULONG ReturnLength
    );

/* KProcessHacker private */

NTSTATUS KphpCaptureStackBackTraceThread(
    __in PETHREAD Thread,
    __in ULONG FramesToSkip,
    __in ULONG FramesToCapture,
    __out_ecount(FramesToCapture) PVOID *BackTrace,
    __out_opt PULONG CapturedFrames,
    __out_opt PULONG BackTraceHash,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphpCreateMappedMdl(
    __in PVOID Address,
    __in ULONG Length,
    __out PMAPPED_MDL MappedMdl
    );

VOID KphpFreeMappedMdl(
    __in PMAPPED_MDL MappedMdl
    );

/* OB */

NTSTATUS ObDuplicateObject(
    __in PEPROCESS SourceProcess,
    __in_opt PEPROCESS TargetProcess,
    __in HANDLE SourceHandle,
    __out_opt PHANDLE TargetHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Options,
    __in KPROCESSOR_MODE AccessMode
    );

PHANDLE_TABLE ObReferenceProcessHandleTable(
    __in PEPROCESS Process
    );

VOID ObDereferenceProcessHandleTable(
    __in PEPROCESS Process
    );

/* PS */

NTSTATUS PsTerminateProcess(
    __in PEPROCESS Process,
    __in NTSTATUS ExitStatus
    );

NTSTATUS PspTerminateThreadByPointer(
    __in PETHREAD Thread,
    __in NTSTATUS ExitStatus
    );

#endif