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

#define _KPH_PRIVATE
#include "include/kph.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, GetSystemRoutineAddress)
#pragma alloc_text(PAGE, KphNtInit)
#pragma alloc_text(PAGE, OpenProcess)
#pragma alloc_text(PAGE, SetProcessToken)
#endif

POBJECT_TYPE ObpDirectoryObjectType;
POBJECT_TYPE ObpTypeObjectType;

/* GetSystemRoutineAddress
 * 
 * Gets the address of a function exported by ntoskrnl or hal.
 */
PVOID GetSystemRoutineAddress(WCHAR *Name)
{
    UNICODE_STRING routineName;
    PVOID routineAddress = NULL;
    
    RtlInitUnicodeString(&routineName, Name);
    
    /* Wrap in SEH because MmGetSystemRoutineAddress is known to cause 
       some BSODs. */
    try
    {
        routineAddress = MmGetSystemRoutineAddress(&routineName);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        routineAddress = NULL;
    }
    
    return routineAddress;
}

/* KphNtInit
 * 
 * Initializes the KProcessHacker NT component.
 */
NTSTATUS KphNtInit()
{
    NTSTATUS status = STATUS_SUCCESS;
    /* Confuse those damn AVs... */
    PWCHAR keService = L"KeService"; // length 9, 18 bytes
    PWCHAR descriptorTable = L"DescriptorTable"; // 15, 30 bytes
    WCHAR keServiceDescriptorTable[9 + 15 + 1];
    
    /* Reconstruct the string. */
    memcpy(keServiceDescriptorTable, keService, 18);
    memcpy(keServiceDescriptorTable + 9, descriptorTable, 30);
    keServiceDescriptorTable[9 + 15] = L'\0';
    
    /* Dynamically get function pointers. */
    __KeServiceDescriptorTable = GetSystemRoutineAddress(keServiceDescriptorTable);
    dfprintf("KeServiceDescriptorTable: %#x\n", __KeServiceDescriptorTable);
    PsGetProcessJob = GetSystemRoutineAddress(L"PsGetProcessJob");
    dfprintf("PsGetProcessJob: %#x\n", PsGetProcessJob);
    PsResumeProcess = GetSystemRoutineAddress(L"PsResumeProcess");
    dfprintf("PsResumeProcess: %#x\n", PsResumeProcess);
    PsSuspendProcess = GetSystemRoutineAddress(L"PsSuspendProcess");
    dfprintf("PsSuspendProcess: %#x\n", PsSuspendProcess);
    
    if (WindowsVersion >= WINDOWS_7)
    {
        ObGetObjectType = GetSystemRoutineAddress(L"ObGetObjectType");
        dfprintf("ObGetObjectType: %#x\n", ObGetObjectType);
    }
    
    /* Scan for functions. */
    if (PsTerminateProcessScan.Initialized)
    {
        __PsTerminateProcess = KvScanProc(&PsTerminateProcessScan);
        dfprintf("PsTerminateProcess: %#x\n", __PsTerminateProcess);
    }
    if (PspTerminateThreadByPointerScan.Initialized)
    {
        __PspTerminateThreadByPointer = KvScanProc(&PspTerminateThreadByPointerScan);
        dfprintf("PspTerminateThreadByPointer: %#x\n", __PspTerminateThreadByPointer);
    }
    
    /* Fill in other global variables. */
    
    /* Directory object type. */
    {
        HANDLE rootDirectoryHandle;
        PVOID rootDirectoryObject;
        UNICODE_STRING rootDirectoryName;
        OBJECT_ATTRIBUTES objectAttributes;
        
        RtlInitUnicodeString(&rootDirectoryName, L"\\");
        InitializeObjectAttributes(
            &objectAttributes,
            &rootDirectoryName,
            OBJ_KERNEL_HANDLE,
            NULL,
            NULL
            );
        
        status = ZwOpenDirectoryObject(&rootDirectoryHandle, DIRECTORY_QUERY, &objectAttributes);
        
        if (!NT_SUCCESS(status))
            return status;
        
        status = ObReferenceObjectByHandle(rootDirectoryHandle, 0, NULL, KernelMode, &rootDirectoryObject, NULL);
        ZwClose(rootDirectoryHandle);
        
        if (!NT_SUCCESS(status))
            return status;
        
        ObpDirectoryObjectType = KphGetObjectTypeNt(rootDirectoryObject);
        ObDirectoryObjectType = &ObpDirectoryObjectType;
        ObDereferenceObject(rootDirectoryObject);
    }
    
    /* Type object type. */
    ObpTypeObjectType = KphGetObjectTypeNt(*PsProcessType);
    ObTypeObjectType = &ObpTypeObjectType;
    
    return status;
}

/* KphAttachProcess
 * 
 * Attaches to a process represented by the specified EPROCESS.
 */
VOID KphAttachProcess(
    __in PEPROCESS Process,
    __out PKPH_ATTACH_STATE AttachState
    )
{
    AttachState->Attached = FALSE;
    
    /* Don't attach if we are already attached to the target. */
    if (Process != PsGetCurrentProcess())
    {
        KeStackAttachProcess(Process, &AttachState->ApcState);
        AttachState->Attached = TRUE;
        AttachState->Process = Process;
    }
}

/* KphAttachProcessHandle
 * 
 * Attaches to a process represented by the specified handle.
 */
NTSTATUS KphAttachProcessHandle(
    __in HANDLE ProcessHandle,
    __out PKPH_ATTACH_STATE AttachState
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS processObject;
    
    AttachState->Attached = FALSE;
    
    status = ObReferenceObjectByHandle(
        ProcessHandle,
        0,
        *PsProcessType,
        KernelMode,
        &processObject,
        NULL
        );
    
    if (!NT_SUCCESS(status))
        return status;
    
    KphAttachProcess(processObject, AttachState);
    ObDereferenceObject(processObject);
    
    return status;
}

/* KphAttachProcessId
 * 
 * Attaches to a process represented by the specified process ID.
 */
NTSTATUS KphAttachProcessId(
    __in HANDLE ProcessId,
    __out PKPH_ATTACH_STATE AttachState
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS processObject;
    
    AttachState->Attached = FALSE;
    
    status = PsLookupProcessByProcessId(ProcessId, &processObject);
    
    if (!NT_SUCCESS(status))
        return status;
    
    KphAttachProcess(processObject, AttachState);
    ObDereferenceObject(processObject);
    
    return status;
}

/* KphCaptureUnicodeString
 * 
 * Captures a UNICODE_STRING. This function will not throw exceptions.
 */
NTSTATUS KphCaptureUnicodeString(
    __in PUNICODE_STRING UnicodeString,
    __out PUNICODE_STRING CapturedUnicodeString
    )
{
    __try
    {
        CapturedUnicodeString->Length = UnicodeString->Length;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }
    
    CapturedUnicodeString->MaximumLength = CapturedUnicodeString->Length;
    CapturedUnicodeString->Buffer = ExAllocatePoolWithTag(
        PagedPool,
        CapturedUnicodeString->Length,
        TAG_CAPTURED_UNICODE_STRING
        );
    
    if (!CapturedUnicodeString->Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    __try
    {
        memcpy(
            CapturedUnicodeString->Buffer,
            UnicodeString->Buffer,
            CapturedUnicodeString->Length
            );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KphFreeCapturedUnicodeString(CapturedUnicodeString);
        return GetExceptionCode();
    }
    
    return STATUS_SUCCESS;
}

/* KphDetachProcess
 * 
 * Detaches from the currently attached process.
 */
VOID KphDetachProcess(
    __in PKPH_ATTACH_STATE AttachState
    )
{
    if (AttachState->Attached)
        KeUnstackDetachProcess(&AttachState->ApcState);
}

/* KphFreeCapturedUnicodeString
 * 
 * Frees a UNICODE_STRING captured by KphCaptureUnicodeString.
 */
VOID KphFreeCapturedUnicodeString(
    __in PUNICODE_STRING CapturedUnicodeString
    )
{
    ExFreePoolWithTag(
        CapturedUnicodeString->Buffer,
        TAG_CAPTURED_UNICODE_STRING
        );
}

/* KphProbeForReadUnicodeString
 * 
 * Probes a UNICODE_STRING structure for reading.
 */
VOID KphProbeForReadUnicodeString(
    __in PUNICODE_STRING UnicodeString
    )
{
    ProbeForRead(UnicodeString, sizeof(UNICODE_STRING), 1);
    ProbeForRead(UnicodeString->Buffer, UnicodeString->Length, 1);
}

/* KphProbeSystemAddressRange
 * 
 * Probes an address range in kernel-mode memory for reading.
 */
VOID KphProbeSystemAddressRange(
    __in PVOID BaseAddress,
    __in ULONG Length
    )
{
    ULONG_PTR page, pageEnd;
    
    /* HACK HACK HACK HACK HACK HACK */
    /* Check the address range by checking each page. */
    /* Round down the base address to the page size. Note: please make sure you are 
     * not using a dumbass compiler which optimizes the following line by removing 
     * the divide and multiply.
     */
    page = (ULONG_PTR)BaseAddress / PAGE_SIZE * PAGE_SIZE;
    /* BaseAddress + Length - 1 is the last address we will be reading. */
    pageEnd = ((ULONG_PTR)BaseAddress + Length - 1) / PAGE_SIZE * PAGE_SIZE;
    
    for (; page <= pageEnd; page += PAGE_SIZE)
    {
        /* Check the page. */
        if (!MmIsAddressValid((PVOID)page))
            ExRaiseStatus(STATUS_ACCESS_VIOLATION);
    }
}

/* OpenProcess
 * 
 * Opens the process with the specified PID.
 */
NTSTATUS OpenProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in HANDLE ProcessId
    )
{
    OBJECT_ATTRIBUTES oa;
    CLIENT_ID clientId;
    
    InitializeObjectAttributes(
        &oa,
        NULL,
        OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );
    clientId.UniqueThread = 0;
    clientId.UniqueProcess = ProcessId;
    
    return KphOpenProcess(ProcessHandle, DesiredAccess, &oa, &clientId, KernelMode);
}

/* SetProcessToken
 * 
 * Assigns the primary token of the target process from the 
 * primary token of source process.
 */
NTSTATUS SetProcessToken(
    __in HANDLE SourcePid,
    __in HANDLE TargetPid
    )
{
    NTSTATUS status;
    HANDLE source;
    
    if (NT_SUCCESS(status = OpenProcess(&source, PROCESS_QUERY_INFORMATION, SourcePid)))
    {
        HANDLE target;
        
        if (NT_SUCCESS(status = OpenProcess(&target, PROCESS_QUERY_INFORMATION | 
            PROCESS_SET_INFORMATION, TargetPid)))
        {
            HANDLE sourceToken;
            
            if (NT_SUCCESS(status = KphOpenProcessTokenEx(source, TOKEN_DUPLICATE, 0, 
                &sourceToken, UserMode)))
            {
                HANDLE dupSourceToken;
                OBJECT_ATTRIBUTES oa;
                
                InitializeObjectAttributes(
                    &oa,
                    NULL,
                    OBJ_KERNEL_HANDLE,
                    NULL,
                    NULL
                    );
                
                if (NT_SUCCESS(status = ZwDuplicateToken(sourceToken, TOKEN_ASSIGN_PRIMARY, &oa,
                    FALSE, TokenPrimary, &dupSourceToken)))
                {
                    PROCESS_ACCESS_TOKEN token;
                    
                    token.Token = dupSourceToken;
                    token.Thread = 0;
                    
                    status = ZwSetInformationProcess(target, ProcessAccessToken, &token, sizeof(token));
                }
                
                ZwClose(dupSourceToken);
            }
            
            ZwClose(sourceToken);
        }
        
        ZwClose(target);
    }
    
    ZwClose(source);
    
    return status;
}
