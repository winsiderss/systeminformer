/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2024
 *
 */

#include <ph.h>
#include <apiimport.h>
#include <kphuser.h>
#include <lsasup.h>
#include <mapldr.h>

/**
 * Opens a thread.
 *
 * \param ThreadHandle A variable which receives a handle to the thread.
 * \param DesiredAccess The desired access to the thread.
 * \param ThreadId The ID of the thread.
 */
NTSTATUS PhOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ThreadId
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    CLIENT_ID clientId;
    KPH_LEVEL level;

    clientId.UniqueProcess = NULL;
    clientId.UniqueThread = ThreadId;

    level = KsiLevel();

    if ((level >= KphLevelMed) && (DesiredAccess & KPH_THREAD_READ_ACCESS) == DesiredAccess)
    {
        status = KphOpenThread(
            ThreadHandle,
            DesiredAccess,
            &clientId
            );
    }
    else
    {
        InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);
        status = NtOpenThread(
            ThreadHandle,
            DesiredAccess,
            &objectAttributes,
            &clientId
            );

        if (status == STATUS_ACCESS_DENIED && (level == KphLevelMax))
        {
            status = KphOpenThread(
                ThreadHandle,
                DesiredAccess,
                &clientId
                );
        }
    }

    return status;
}

NTSTATUS PhOpenThreadClientId(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    KPH_LEVEL level;

    level = KsiLevel();

    if ((level >= KphLevelMed) && (DesiredAccess & KPH_THREAD_READ_ACCESS) == DesiredAccess)
    {
        status = KphOpenThread(
            ThreadHandle,
            DesiredAccess,
            ClientId
            );
    }
    else
    {
        InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);
        status = NtOpenThread(
            ThreadHandle,
            DesiredAccess,
            &objectAttributes,
            ClientId
            );

        if (status == STATUS_ACCESS_DENIED && (level == KphLevelMax))
        {
            status = KphOpenThread(
                ThreadHandle,
                DesiredAccess,
                ClientId
                );
        }
    }

    return status;
}

/** Limited API for untrusted/external code. */
NTSTATUS PhOpenThreadPublic(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ThreadId
    )
{
    OBJECT_ATTRIBUTES objectAttributes;
    CLIENT_ID clientId;

    clientId.UniqueProcess = NULL;
    clientId.UniqueThread = ThreadId;

    InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);

    return NtOpenThread(
        ThreadHandle,
        DesiredAccess,
        &objectAttributes,
        &clientId
        );
}

NTSTATUS PhOpenThreadProcess(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE ProcessHandle
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
    KPH_LEVEL level;

    level = KsiLevel();

    if (level == KphLevelMax || (level >= KphLevelMed && FlagOn(DesiredAccess, KPH_PROCESS_READ_ACCESS) == DesiredAccess))
    {
        status = KphOpenThreadProcess(
            ThreadHandle,
            DesiredAccess,
            ProcessHandle
            );

        if (NT_SUCCESS(status))
            return status;
    }

    status = PhGetThreadBasicInformation(
        ThreadHandle,
        &basicInfo
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhOpenProcessClientId(
        ProcessHandle,
        DesiredAccess,
        &basicInfo.ClientId
        );

    return status;
}

NTSTATUS PhTerminateThread(
    _In_ HANDLE ThreadHandle,
    _In_ NTSTATUS ExitStatus
    )
{
    NTSTATUS status;

    status = NtTerminateThread(
        ThreadHandle,
        ExitStatus
        );

    return status;
}


/*
 * Retrieves the context of a thread.
 *
 * \param ThreadHandle The handle to the thread.
 * \param ThreadContext A pointer to the CONTEXT structure that receives the thread context.
 *
 * \return The status of the operation.
 */
NTSTATUS PhGetContextThread(
    _In_ HANDLE ThreadHandle,
    _Inout_ PCONTEXT ThreadContext
    )
{
    NTSTATUS status;

    status = NtGetContextThread(
        ThreadHandle,
        ThreadContext
        );

    return status;
}

NTSTATUS PhGetThreadName(
    _In_ HANDLE ThreadHandle,
    _Out_ PPH_STRING *ThreadName
    )
{
    NTSTATUS status;
    PTHREAD_NAME_INFORMATION buffer;
    ULONG bufferSize;
    ULONG returnLength;

    if (WindowsVersion < WINDOWS_10)
        return STATUS_NOT_SUPPORTED;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQueryInformationThread(
        ThreadHandle,
        ThreadNameInformation,
        buffer,
        bufferSize,
        &returnLength
        );

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize);

        status = NtQueryInformationThread(
            ThreadHandle,
            ThreadNameInformation,
            buffer,
            bufferSize,
            &returnLength
            );
    }

    if (NT_SUCCESS(status))
    {
        // Note: Some threads have UNICODE_NULL as their name. (dmex)
        if (RtlIsNullOrEmptyUnicodeString(&buffer->ThreadName))
        {
            PhFree(buffer);
            return STATUS_UNSUCCESSFUL;
        }

        *ThreadName = PhCreateStringFromUnicodeString(&buffer->ThreadName);
    }

    PhFree(buffer);

    return status;
}

NTSTATUS PhSetThreadName(
    _In_ HANDLE ThreadHandle,
    _In_ PCWSTR ThreadName
    )
{
    NTSTATUS status;
    THREAD_NAME_INFORMATION threadNameInfo;

    if (WindowsVersion < WINDOWS_10)
        return STATUS_NOT_SUPPORTED;

    memset(&threadNameInfo, 0, sizeof(THREAD_NAME_INFORMATION));

    status = RtlInitUnicodeStringEx(
        &threadNameInfo.ThreadName,
        ThreadName
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtSetInformationThread(
        ThreadHandle,
        ThreadNameInformation,
        &threadNameInfo,
        sizeof(THREAD_NAME_INFORMATION)
        );

    return status;
}

/**
 * Sets a thread's affinity mask.
 *
 * \param ThreadHandle A handle to a thread. The handle must have THREAD_SET_LIMITED_INFORMATION
 * access.
 * \param AffinityMask The new affinity mask.
 */
NTSTATUS PhSetThreadAffinityMask(
    _In_ HANDLE ThreadHandle,
    _In_ KAFFINITY AffinityMask
    )
{
    NTSTATUS status;

    status = NtSetInformationThread(
        ThreadHandle,
        ThreadAffinityMask,
        &AffinityMask,
        sizeof(KAFFINITY)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationThread(
            ThreadHandle,
            KphThreadAffinityMask,
            &AffinityMask,
            sizeof(KAFFINITY)
            );
    }

    return status;
}

NTSTATUS PhSetThreadBasePriorityClientId(
    _In_ CLIENT_ID ClientId,
    _In_ KPRIORITY Increment
    )
{
    NTSTATUS status;
    SYSTEM_THREAD_CID_PRIORITY_INFORMATION threadInfo;

    threadInfo.ClientId = ClientId;
    threadInfo.Priority = Increment;

    status = NtSetSystemInformation(
        SystemThreadPriorityClientIdInformation,
        &threadInfo,
        sizeof(SYSTEM_THREAD_CID_PRIORITY_INFORMATION)
        );

    if (status == STATUS_PENDING)
        status = STATUS_SUCCESS;

    return status;
}

NTSTATUS PhSetThreadBasePriority(
    _In_ HANDLE ThreadHandle,
    _In_ KPRIORITY Increment
    )
{
    NTSTATUS status;

    status = NtSetInformationThread(
        ThreadHandle,
        ThreadBasePriority,
        &Increment,
        sizeof(KPRIORITY)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationThread(
            ThreadHandle,
            KphThreadBasePriority,
            &Increment,
            sizeof(KPRIORITY)
            );
    }

    return status;
}

/**
 * Sets a thread's I/O priority.
 *
 * \param ThreadHandle A handle to a thread. The handle must have THREAD_SET_LIMITED_INFORMATION
 * access.
 * \param IoPriority The new I/O priority.
 */
NTSTATUS PhSetThreadIoPriority(
    _In_ HANDLE ThreadHandle,
    _In_ IO_PRIORITY_HINT IoPriority
    )
{
    NTSTATUS status;

    status = NtSetInformationThread(
        ThreadHandle,
        ThreadIoPriority,
        &IoPriority,
        sizeof(IO_PRIORITY_HINT)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationThread(
            ThreadHandle,
            KphThreadIoPriority,
            &IoPriority,
            sizeof(IO_PRIORITY_HINT)
            );
    }

    return status;
}

NTSTATUS PhSetThreadPagePriority(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG PagePriority
    )
{
    NTSTATUS status;
    PAGE_PRIORITY_INFORMATION pagePriorityInfo;

    pagePriorityInfo.PagePriority = PagePriority;

    status = NtSetInformationThread(
        ThreadHandle,
        ThreadPagePriority,
        &pagePriorityInfo,
        sizeof(PAGE_PRIORITY_INFORMATION)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationThread(
            ThreadHandle,
            KphThreadPagePriority,
            &pagePriorityInfo,
            sizeof(PAGE_PRIORITY_INFORMATION)
            );
    }

    return status;
}

NTSTATUS PhSetThreadPriorityBoost(
    _In_ HANDLE ThreadHandle,
    _In_ BOOLEAN DisablePriorityBoost
    )
{
    NTSTATUS status;
    ULONG priorityBoost;

    priorityBoost = DisablePriorityBoost ? 1 : 0;

    status = NtSetInformationThread(
        ThreadHandle,
        ThreadPriorityBoost,
        &priorityBoost,
        sizeof(ULONG)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationThread(
            ThreadHandle,
            KphThreadPriorityBoost,
            &priorityBoost,
            sizeof(ULONG)
            );
    }

    return status;
}

NTSTATUS PhSetThreadIdealProcessor(
    _In_ HANDLE ThreadHandle,
    _In_ PPROCESSOR_NUMBER ProcessorNumber,
    _Out_opt_ PPROCESSOR_NUMBER PreviousIdealProcessor
    )
{
    NTSTATUS status;
    PROCESSOR_NUMBER processorNumber;

    processorNumber = *ProcessorNumber;
    status = NtSetInformationThread(
        ThreadHandle,
        ThreadIdealProcessorEx,
        &processorNumber,
        sizeof(PROCESSOR_NUMBER)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationThread(
            ThreadHandle,
            KphThreadIdealProcessorEx,
            &processorNumber,
            sizeof(PROCESSOR_NUMBER)
            );
    }

    if (PreviousIdealProcessor)
        *PreviousIdealProcessor = processorNumber;

    return status;
}

NTSTATUS PhSetThreadGroupAffinity(
    _In_ HANDLE ThreadHandle,
    _In_ GROUP_AFFINITY GroupAffinity
    )
{
    NTSTATUS status;

    status = NtSetInformationThread(
        ThreadHandle,
        ThreadGroupInformation,
        &GroupAffinity,
        sizeof(GROUP_AFFINITY)
        );

    if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
    {
        status = KphSetInformationThread(
            ThreadHandle,
            KphThreadGroupInformation,
            &GroupAffinity,
            sizeof(GROUP_AFFINITY)
            );
    }

    return status;
}

NTSTATUS PhGetThreadLastSystemCall(
    _In_ HANDLE ThreadHandle,
    _Out_ PTHREAD_LAST_SYSCALL_INFORMATION LastSystemCall
    )
{
    if (WindowsVersion < WINDOWS_8)
    {
        return NtQueryInformationThread(
            ThreadHandle,
            ThreadLastSystemCall,
            LastSystemCall,
            RTL_SIZEOF_THROUGH_FIELD(THREAD_LAST_SYSCALL_INFORMATION, Pad),
            NULL
            );
    }
    else
    {
        return NtQueryInformationThread(
            ThreadHandle,
            ThreadLastSystemCall,
            LastSystemCall,
            sizeof(THREAD_LAST_SYSCALL_INFORMATION),
            NULL
            );
    }
}

NTSTATUS PhCreateImpersonationToken(
    _In_ HANDLE ThreadHandle,
    _Out_ PHANDLE TokenHandle
    )
{
    NTSTATUS status;
    HANDLE tokenHandle;
    SECURITY_QUALITY_OF_SERVICE securityService;

    status = PhRevertImpersonationToken(ThreadHandle);

    if (!NT_SUCCESS(status))
        return status;

    securityService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    securityService.ImpersonationLevel = SecurityImpersonation;
    securityService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    securityService.EffectiveOnly = FALSE;

    status = NtImpersonateThread(
        ThreadHandle,
        ThreadHandle,
        &securityService
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhOpenThreadToken(
        ThreadHandle,
        TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
        FALSE,
        &tokenHandle
        );

    if (NT_SUCCESS(status))
    {
        *TokenHandle = tokenHandle;
    }

    return status;
}

NTSTATUS PhImpersonateToken(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE TokenHandle
    )
{
    NTSTATUS status;
    TOKEN_TYPE tokenType;
    ULONG returnLength;

    status = NtQueryInformationToken(
        TokenHandle,
        TokenType,
        &tokenType,
        sizeof(TOKEN_TYPE),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    if (tokenType == TokenPrimary)
    {
        SECURITY_QUALITY_OF_SERVICE securityService;
        OBJECT_ATTRIBUTES objectAttributes;
        HANDLE tokenHandle;

        InitializeObjectAttributes(
            &objectAttributes,
            NULL,
            OBJ_EXCLUSIVE,
            NULL,
            NULL
            );

        securityService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        securityService.ImpersonationLevel = SecurityImpersonation;
        securityService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        securityService.EffectiveOnly = FALSE;
        objectAttributes.SecurityQualityOfService = &securityService;

        status = NtDuplicateToken(
            TokenHandle,
            TOKEN_IMPERSONATE | TOKEN_QUERY,
            &objectAttributes,
            FALSE,
            TokenImpersonation,
            &tokenHandle
            );

        if (!NT_SUCCESS(status))
            return status;

        status = NtSetInformationThread(
            ThreadHandle,
            ThreadImpersonationToken,
            &tokenHandle,
            sizeof(HANDLE)
            );

        NtClose(tokenHandle);
    }
    else
    {
        status = NtSetInformationThread(
            ThreadHandle,
            ThreadImpersonationToken,
            &TokenHandle,
            sizeof(HANDLE)
            );
    }

    return status;
}

NTSTATUS PhRevertImpersonationToken(
    _In_ HANDLE ThreadHandle
    )
{
    HANDLE tokenHandle = NULL;

    return NtSetInformationThread(
        ThreadHandle,
        ThreadImpersonationToken,
        &tokenHandle,
        sizeof(HANDLE)
        );
}

NTSTATUS PhGetThreadLastStatusValue(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PNTSTATUS LastStatusValue
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, LastStatusValue)),
            LastStatusValue,
            sizeof(NTSTATUS),
            NULL
            );
    }
    else
#endif
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, LastStatusValue)), // LastErrorValue/ExceptionCode
            LastStatusValue,
            sizeof(NTSTATUS),
            NULL
            );
    }

    return status;
}

NTSTATUS PhGetThreadApartmentState(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ POLETLSFLAGS ApartmentState
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif
    ULONG_PTR oletlsDataAddress = 0;

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, ReservedForOle)),
            &oletlsDataAddress,
            sizeof(ULONG),
            NULL
            );
    }
    else
#endif
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, ReservedForOle)),
            &oletlsDataAddress,
            sizeof(ULONG_PTR),
            NULL
            );
    }

    if (NT_SUCCESS(status) && oletlsDataAddress)
    {
        PVOID apartmentStateOffset;

        // Note: Teb->ReservedForOle is the SOleTlsData structure
        // and ApartmentState is the dwFlags field. (dmex)

#ifdef _WIN64
        if (isWow64)
            apartmentStateOffset = PTR_ADD_OFFSET(oletlsDataAddress, 0xC);
        else
            apartmentStateOffset = PTR_ADD_OFFSET(oletlsDataAddress, 0x14);
#else
        apartmentStateOffset = PTR_ADD_OFFSET(oletlsDataAddress, 0xC);
#endif

        status = NtReadVirtualMemory(
            ProcessHandle,
            apartmentStateOffset,
            ApartmentState,
            sizeof(ULONG),
            NULL
            );
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

// rev from advapi32!WctGetCOMInfo (dmex)
/**
 * If a thread is blocked on a COM call, we can retrieve COM ownership information using these functions. Retrieves COM information when a thread is blocked on a COM call.
 *
 * \param ThreadHandle A handle to the thread.
 * \param ProcessHandle A handle to a process.
 * \param ApartmentCallState The COM call information.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhGetThreadApartmentCallState(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_COM_CALLSTATE ApartmentCallState
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif
    ULONG_PTR oletlsDataAddress = 0;

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, ReservedForOle)),
            &oletlsDataAddress,
            sizeof(ULONG),
            NULL
            );
    }
    else
#endif
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, ReservedForOle)),
            &oletlsDataAddress,
            sizeof(ULONG_PTR),
            NULL
            );
    }

    if (NT_SUCCESS(status) && oletlsDataAddress)
    {
        typedef enum _CALL_STATE_TYPE
        {
            CALL_STATE_TYPE_OUTGOING, // tagOutgoingCallData
            CALL_STATE_TYPE_INCOMING, // tagIncomingCallData
            CALL_STATE_TYPE_ACTIVATION // tagOutgoingActivationData
        } CALL_STATE_TYPE;
        typedef struct tagOutgoingCallData // private
        {
            ULONG dwServerPID;
            ULONG dwServerTID;
        } tagOutgoingCallData, *PtagOutgoingCallData;
        typedef struct tagIncomingCallData // private
        {
            ULONG dwClientPID;
        } tagIncomingCallData, *PtagIncomingCallData;
        typedef struct tagOutgoingActivationData // private
        {
            GUID guidServer;
        } tagOutgoingActivationData, *PtagOutgoingActivationData;
        static HRESULT (WINAPI* CoGetCallState_I)( // rev
            _In_ CALL_STATE_TYPE Type,
            _Out_ PULONG OffSet
            ) = NULL;
        //static HRESULT (WINAPI* CoGetActivationState_I)( // rev
        //    _In_ LPCLSID Clsid,
        //    _In_ ULONG ClientTid,
        //    _Out_ PULONG ServerPid
        //    ) = NULL;
        static PH_INITONCE initOnce = PH_INITONCE_INIT;
        ULONG outgoingCallDataOffset = 0;
        ULONG incomingCallDataOffset = 0;
        ULONG outgoingActivationDataOffset = 0;
        tagOutgoingCallData outgoingCallData;
        tagIncomingCallData incomingCallData;
        tagOutgoingActivationData outgoingActivationData;

        if (PhBeginInitOnce(&initOnce))
        {
            PVOID baseAddress;

            if (baseAddress = PhGetLoaderEntryDllBaseZ(L"combase.dll"))
            {
                CoGetCallState_I = PhGetDllBaseProcedureAddress(baseAddress, "CoGetCallState", 0);
                //CoGetActivationState_I = PhGetDllBaseProcedureAddress(baseAddress, "CoGetActivationState", 0);
            }

            PhEndInitOnce(&initOnce);
        }

        memset(&outgoingCallData, 0, sizeof(tagOutgoingCallData));
        memset(&incomingCallData, 0, sizeof(tagIncomingCallData));
        memset(&outgoingActivationData, 0, sizeof(tagOutgoingActivationData));

        if (HR_SUCCESS(CoGetCallState_I(CALL_STATE_TYPE_OUTGOING, &outgoingCallDataOffset)) && outgoingCallDataOffset)
        {
            NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(oletlsDataAddress, outgoingCallDataOffset),
                &outgoingCallData,
                sizeof(tagOutgoingCallData),
                NULL
                );
        }

        if (HR_SUCCESS(CoGetCallState_I(CALL_STATE_TYPE_INCOMING, &incomingCallDataOffset)) && incomingCallDataOffset)
        {
            NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(oletlsDataAddress, incomingCallDataOffset),
                &incomingCallData,
                sizeof(tagIncomingCallData),
                NULL
                );
        }

        if (HR_SUCCESS(CoGetCallState_I(CALL_STATE_TYPE_ACTIVATION, &outgoingActivationDataOffset)) && outgoingActivationDataOffset)
        {
            NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(oletlsDataAddress, outgoingActivationDataOffset),
                &outgoingActivationData,
                sizeof(tagOutgoingActivationData),
                NULL
                );
        }

        memset(ApartmentCallState, 0, sizeof(PH_COM_CALLSTATE));
        ApartmentCallState->ServerPID = outgoingCallData.dwServerPID != 0 ? outgoingCallData.dwServerPID : ULONG_MAX;
        ApartmentCallState->ServerTID = outgoingCallData.dwServerTID != 0 ? outgoingCallData.dwServerTID : ULONG_MAX;
        ApartmentCallState->ClientPID = incomingCallData.dwClientPID != 0 ? incomingCallData.dwClientPID : ULONG_MAX;
        memcpy(&ApartmentCallState->ServerGuid, &outgoingActivationData.guidServer, sizeof(GUID));
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

// rev from advapi32!WctGetCritSecInfo (dmex)
/**
 * Retrieves the thread identifier when a thread is blocked on a critical section.
 *
 * \param ThreadHandle A handle to the thread.
 * \param ProcessId The ID of a process.
 * \param ThreadId The ID of the thread owning the critical section.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhGetThreadCriticalSectionOwnerThread(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessId,
    _Out_ PULONG ThreadId
    )
{
    NTSTATUS status;
    PRTL_DEBUG_INFORMATION debugBuffer;

    if (WindowsVersion < WINDOWS_11)
        return STATUS_UNSUCCESSFUL;

    if (!(debugBuffer = RtlCreateQueryDebugBuffer(0, FALSE)))
        return STATUS_UNSUCCESSFUL;

    debugBuffer->CriticalSectionOwnerThread = ThreadHandle;

    status = RtlQueryProcessDebugInformation(
        ProcessId,
        RTL_QUERY_PROCESS_NONINVASIVE_CS_OWNER, // TODO: RTL_QUERY_PROCESS_CS_OWNER (dmex)
        debugBuffer
        );

    if (!NT_SUCCESS(status))
    {
        RtlDestroyQueryDebugBuffer(debugBuffer);
        return status;
    }

    if (!debugBuffer->Reserved[0])
    {
        RtlDestroyQueryDebugBuffer(debugBuffer);
        return STATUS_UNSUCCESSFUL;
    }

    *ThreadId = PtrToUlong(debugBuffer->Reserved[0]);

    RtlDestroyQueryDebugBuffer(debugBuffer);

    return STATUS_SUCCESS;
}

// rev from advapi32!WctGetSocketInfo (dmex)
/**
 * Retrieves the connection state when a thread is blocked on a socket.
 *
 * \param ThreadHandle A handle to the thread.
 * \param ProcessHandle A handle to a process.
 * \param ThreadSocketState The state of the socket.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhGetThreadSocketState(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_THREAD_SOCKET_STATE ThreadSocketState
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
    BOOLEAN openedProcessHandle = FALSE;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif
    HANDLE winsockHandleAddress;

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        ULONG winsockDataAddress = 0;

        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, WinSockData)),
            &winsockDataAddress,
            sizeof(ULONG),
            NULL
            );

        winsockHandleAddress = UlongToHandle(winsockDataAddress);
    }
    else
#endif
    {
        ULONG_PTR winsockDataAddress = 0;

        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, WinSockData)),
            &winsockDataAddress,
            sizeof(ULONG_PTR),
            NULL
            );

        winsockHandleAddress = (HANDLE)winsockDataAddress;
    }

    if (NT_SUCCESS(status) && winsockHandleAddress)
    {
        static LONG (WINAPI* LPFN_WSASTARTUP)(
            _In_ WORD wVersionRequested,
            _Out_ PVOID* lpWSAData
            );
        static LONG (WINAPI* LPFN_GETSOCKOPT)(
            _In_ UINT_PTR s,
            _In_ LONG level,
            _In_ LONG optname,
            _Out_writes_bytes_(*optlen) char FAR* optval,
            _Inout_ LONG FAR* optlen
            );
        static LONG (WINAPI* LPFN_CLOSESOCKET)(
            _In_ ULONG_PTR s
            );
        static LONG (WINAPI* LPFN_WSACLEANUP)(
            void
            );
        static PH_INITONCE initOnce = PH_INITONCE_INIT;
        #ifndef WINSOCK_VERSION
        #define WINSOCK_VERSION MAKEWORD(2,2)
        #endif
        #ifndef SOCKET_ERROR
        #define SOCKET_ERROR (-1)
        #endif
        #ifndef SOL_SOCKET
        #define SOL_SOCKET 0xffff
        #endif
        #ifndef SO_BSP_STATE
        #define SO_BSP_STATE 0x1009
        #endif
        typedef struct _SOCKET_ADDRESS
        {
            _Field_size_bytes_(iSockaddrLength) PVOID lpSockaddr;
            // _When_(lpSockaddr->sa_family == AF_INET, _Field_range_(>=, sizeof(SOCKADDR_IN)))
            // _When_(lpSockaddr->sa_family == AF_INET6, _Field_range_(>=, sizeof(SOCKADDR_IN6)))
            LONG iSockaddrLength;
        } SOCKET_ADDRESS, *PSOCKET_ADDRESS, *LPSOCKET_ADDRESS;
        typedef struct _CSADDR_INFO
        {
            SOCKET_ADDRESS LocalAddr;
            SOCKET_ADDRESS RemoteAddr;
            LONG iSocketType;
            LONG iProtocol;
        } CSADDR_INFO, *PCSADDR_INFO, FAR* LPCSADDR_INFO;
        PVOID wsaStartupData;
        HANDLE winsockTargetHandle;

        if (PhBeginInitOnce(&initOnce))
        {
            PVOID baseAddress;

            if (baseAddress = PhLoadLibrary(L"ws2_32.dll"))
            {
                LPFN_WSASTARTUP = PhGetDllBaseProcedureAddress(baseAddress, "WSAStartup", 0);
                LPFN_GETSOCKOPT = PhGetDllBaseProcedureAddress(baseAddress, "getsockopt", 0);
                //LPFN_GETSOCKNAME = PhGetDllBaseProcedureAddress(baseAddress, "getsockname", 0);
                //LPFN_GETPEERNAME = PhGetDllBaseProcedureAddress(baseAddress, "getpeername", 0);
                LPFN_CLOSESOCKET = PhGetDllBaseProcedureAddress(baseAddress, "closesocket", 0);
                LPFN_WSACLEANUP = PhGetDllBaseProcedureAddress(baseAddress, "WSACleanup", 0);
            }

            PhEndInitOnce(&initOnce);
        }

        if (LPFN_WSASTARTUP(WINSOCK_VERSION, &wsaStartupData) != 0)
        {
            status = STATUS_UNSUCCESSFUL;
            goto CleanupExit;
        }

        status = NtDuplicateObject(
            ProcessHandle,
            winsockHandleAddress,
            NtCurrentProcess(),
            &winsockTargetHandle,
            0,
            0,
            DUPLICATE_SAME_ACCESS
            );

        if (NT_SUCCESS(status))
        {
            ULONG returnLength;
            OBJECT_BASIC_INFORMATION winsockTargetBasicInfo;
            INT winsockAddressInfoLength = sizeof(CSADDR_INFO);
            CSADDR_INFO winsockAddressInfo;

            memset(&winsockTargetBasicInfo, 0, sizeof(OBJECT_BASIC_INFORMATION));
            NtQueryObject(
                winsockTargetHandle,
                ObjectBasicInformation,
                &winsockTargetBasicInfo,
                sizeof(OBJECT_BASIC_INFORMATION),
                &returnLength
                );

            if (winsockTargetBasicInfo.HandleCount > 2)
            {
                if (LPFN_GETSOCKOPT((UINT_PTR)winsockTargetHandle, SOL_SOCKET, SO_BSP_STATE, (PCHAR)&winsockAddressInfo, &winsockAddressInfoLength) != SOCKET_ERROR)
                {
                    if (winsockAddressInfo.iProtocol == 6)
                    {
                        if (winsockAddressInfo.LocalAddr.lpSockaddr && winsockAddressInfo.RemoteAddr.lpSockaddr)
                            *ThreadSocketState = PH_THREAD_SOCKET_STATE_SHARED;
                        else
                            *ThreadSocketState = PH_THREAD_SOCKET_STATE_DISCONNECTED;
                    }
                    else
                        *ThreadSocketState = PH_THREAD_SOCKET_STATE_NOT_TCPIP;
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL; // WSAGetLastError();
                }
            }
            else
            {
                status = STATUS_UNSUCCESSFUL;
            }

            LPFN_CLOSESOCKET((UINT_PTR)winsockTargetHandle);

            NtClose(winsockTargetHandle);
        }

        LPFN_WSACLEANUP();
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }

CleanupExit:
    if (openedProcessHandle)
        NtClose(ProcessHandle);

    return status;
}

NTSTATUS PhGetThreadStackLimits(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG_PTR LowPart,
    _Out_ PULONG_PTR HighPart
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
    NT_TIB ntTib;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

    memset(&ntTib, 0, sizeof(NT_TIB));

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, NtTib)),
            &ntTib,
            sizeof(NT_TIB32),
            NULL
            );
    }
    else
#endif
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, NtTib)),
            &ntTib,
            sizeof(NT_TIB),
            NULL
            );
    }

    if (NT_SUCCESS(status))
    {
#ifdef _WIN64
        if (isWow64)
        {
            PNT_TIB32 ntTib32 = (PNT_TIB32)&ntTib;
            *LowPart = (ULONG_PTR)UlongToPtr(ntTib32->StackLimit);
            *HighPart = (ULONG_PTR)UlongToPtr(ntTib32->StackBase);
        }
        else
        {
            *LowPart = (ULONG_PTR)ntTib.StackLimit;
            *HighPart = (ULONG_PTR)ntTib.StackBase;
        }
#else
        *LowPart = (ULONG_PTR)ntTib.StackLimit;
        *HighPart = (ULONG_PTR)ntTib.StackBase;
#endif
    }

    return status;
}

NTSTATUS PhGetThreadStackSize(
    _In_ HANDLE ThreadHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG_PTR StackUsage,
    _Out_ PULONG_PTR StackLimit
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
    NT_TIB ntTib;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

    memset(&ntTib, 0, sizeof(NT_TIB));

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, NtTib)),
            &ntTib,
            sizeof(NT_TIB32),
            NULL
            );
    }
    else
#endif
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, NtTib)),
            &ntTib,
            sizeof(NT_TIB),
            NULL
            );
    }

    if (NT_SUCCESS(status))
    {
        MEMORY_BASIC_INFORMATION memoryBasicInformation;
        PVOID stackBaseAddress = NULL;
        PVOID stackLimitAddress = NULL;

#ifdef _WIN64
        if (isWow64)
        {
            PNT_TIB32 ntTib32 = (PNT_TIB32)&ntTib;
            stackBaseAddress = UlongToPtr(ntTib32->StackBase);
            stackLimitAddress = UlongToPtr(ntTib32->StackLimit);
        }
        else
        {
            stackBaseAddress = ntTib.StackBase;
            stackLimitAddress = ntTib.StackLimit;
        }
#else
        stackBaseAddress = ntTib.StackBase;
        stackLimitAddress = ntTib.StackLimit;
#endif
        memset(&memoryBasicInformation, 0, sizeof(MEMORY_BASIC_INFORMATION));

        status = NtQueryVirtualMemory(
            ProcessHandle,
            stackLimitAddress,
            MemoryBasicInformation,
            &memoryBasicInformation,
            sizeof(MEMORY_BASIC_INFORMATION),
            NULL
            );

        if (NT_SUCCESS(status))
        {
            // TEB->DeallocationStack == memoryBasicInfo.AllocationBase
            *StackUsage = (ULONG_PTR)PTR_SUB_OFFSET(stackBaseAddress, stackLimitAddress);
            *StackLimit = (ULONG_PTR)PTR_SUB_OFFSET(stackBaseAddress, memoryBasicInformation.AllocationBase);
        }
    }

    return status;
}

NTSTATUS PhGetThreadIsFiber(
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE ProcessHandle,
    _Out_ PBOOLEAN ThreadIsFiber
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInfo;
    BOOLEAN openedProcessHandle = FALSE;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif
    LONG flags = 0;

    if (!NT_SUCCESS(status = PhGetThreadBasicInformation(ThreadHandle, &basicInfo)))
        return status;

    if (!ProcessHandle)
    {
        if (!NT_SUCCESS(status = PhOpenProcess(
            &ProcessHandle,
            PROCESS_VM_READ | (WindowsVersion > WINDOWS_7 ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_QUERY_INFORMATION),
            basicInfo.ClientId.UniqueProcess
            )))
            return status;

        openedProcessHandle = TRUE;
    }

#ifdef _WIN64
    PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (isWow64)
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(WOW64_GET_TEB32(basicInfo.TebBaseAddress), UFIELD_OFFSET(TEB32, SameTebFlags)),
            &flags,
            sizeof(USHORT),
            NULL
            );
    }
    else
#endif
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(basicInfo.TebBaseAddress, UFIELD_OFFSET(TEB, SameTebFlags)),
            &flags,
            sizeof(USHORT),
            NULL
            );
    }

    if (NT_SUCCESS(status))
    {
        *ThreadIsFiber = _bittest(&flags, 2); // HasFiberData offset (dmex)
    }

    if (openedProcessHandle)
        NtClose(ProcessHandle);

    return status;
}

// rev from SwitchToThread (dmex)
/**
 * Causes the calling thread to yield execution to another thread that is ready to run on the current processor. The operating system selects the next thread to be executed.
 *
 * \remarks The operating system will not switch execution to another processor, even if that processor is idle or is running a thread of lower priority.
 *
 * \return If calling the SwitchToThread function caused the operating system to switch execution to another thread, the return value is nonzero.
 * \rthere are no other threads ready to execute, the operating system does not switch execution to another thread, and the return value is zero.
 */
BOOLEAN PhSwitchToThread(
    VOID
    )
{
    LARGE_INTEGER interval = { 0 };

    return PhDelayExecutionEx(FALSE, &interval) != STATUS_NO_YIELD_PERFORMED;
}

NTSTATUS PhGetProcessRuntimeLibrary(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_PROCESS_RUNTIME_LIBRARY* RuntimeLibrary,
    _Out_opt_ PBOOLEAN IsWow64Process
    )
{
    static PH_PROCESS_RUNTIME_LIBRARY NativeRuntime =
    {
        PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntdll.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\kernel32.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\user32.dll"),
    };
#ifdef _WIN64
    static PH_PROCESS_RUNTIME_LIBRARY Wow64Runtime =
    {
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysWOW64\\ntdll.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysWOW64\\kernel32.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysWOW64\\user32.dll"),
    };
#ifdef _M_ARM64
    static PH_PROCESS_RUNTIME_LIBRARY Arm32Runtime =
    {
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysArm32\\ntdll.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysArm32\\kernel32.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SysArm32\\user32.dll"),
    };
    static PH_PROCESS_RUNTIME_LIBRARY Chpe32Runtime =
    {
        PH_STRINGREF_INIT(L"\\SystemRoot\\SyChpe32\\ntdll.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SyChpe32\\kernel32.dll"),
        PH_STRINGREF_INIT(L"\\SystemRoot\\SyChpe32\\user32.dll"),
    };
#endif
#endif

    *RuntimeLibrary = &NativeRuntime;

    if (IsWow64Process)
        *IsWow64Process = FALSE;

#ifdef _WIN64
    NTSTATUS status;
#ifdef _M_ARM64
    USHORT machine;

    status = PhGetProcessArchitecture(ProcessHandle, &machine);

    if (!NT_SUCCESS(status))
        return status;

    if (machine != IMAGE_FILE_MACHINE_TARGET_HOST)
    {
        switch (machine)
        {
        case IMAGE_FILE_MACHINE_I386:
        case IMAGE_FILE_MACHINE_CHPE_X86:
            {
                *RuntimeLibrary = &Chpe32Runtime;

                if (IsWow64Process)
                    *IsWow64Process = TRUE;
            }
            break;
        case IMAGE_FILE_MACHINE_ARMNT:
            {
                *RuntimeLibrary = &Arm32Runtime;

                if (IsWow64Process)
                    *IsWow64Process = TRUE;
            }
            break;
        case IMAGE_FILE_MACHINE_AMD64:
        case IMAGE_FILE_MACHINE_ARM64:
            break;
        default:
            return STATUS_INVALID_PARAMETER;
        }
    }
#else
    BOOLEAN isWow64 = FALSE;

    status = PhGetProcessIsWow64(ProcessHandle, &isWow64);

    if (!NT_SUCCESS(status))
        return status;

    if (isWow64)
    {
        *RuntimeLibrary = &Wow64Runtime;

        if (IsWow64Process)
            *IsWow64Process = TRUE;
    }
#endif
#endif

    return STATUS_SUCCESS;
}

/**
 * Causes a process to load a DLL.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_CREATE_THREAD, PROCESS_VM_OPERATION, PROCESS_VM_READ
 * and PROCESS_VM_WRITE access.
 * \param FileName The file name of the DLL to inject.
 * \param LoadDllUsingApcThread Queues an APC (Asynchronous Procedure Call) when calling LoadLibraryW.
 * \param Timeout The timeout, in milliseconds, for the process to load the DLL.
 *
 * \remarks If the process does not load the DLL before the timeout expires it may crash. Choose the
 * timeout value carefully.
 */
NTSTATUS PhLoadDllProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_STRINGREF FileName,
    _In_ BOOLEAN LoadDllUsingApcThread,
    _In_opt_ ULONG Timeout
    )
{
    NTSTATUS status;
    SIZE_T fileNameAllocationSize;
    PVOID fileNameBaseAddress = NULL;
    PVOID loadLibraryW = NULL;
    PVOID rtlExitUserThread = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    if (KphProcessLevel(ProcessHandle) > KphLevelMed)
    {
        return STATUS_ACCESS_DENIED;
    }

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        "LoadLibraryW",
        &loadLibraryW,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (LoadDllUsingApcThread)
    {
        status = PhGetProcedureAddressRemote(
            ProcessHandle,
            &runtimeLibrary->NtdllFileName,
            "RtlExitUserThread",
            &rtlExitUserThread,
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    fileNameAllocationSize = FileName->Length + sizeof(UNICODE_NULL);
    status = NtAllocateVirtualMemory(
        ProcessHandle,
        &fileNameBaseAddress,
        0,
        &fileNameAllocationSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtWriteVirtualMemory(
        ProcessHandle,
        fileNameBaseAddress,
        FileName->Buffer,
        FileName->Length + sizeof(UNICODE_NULL),
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    if (LoadDllUsingApcThread)
    {
        status = PhCreateUserThread(
            ProcessHandle,
            NULL,
            THREAD_ALL_ACCESS,
            THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
            0,
            0,
            0,
            rtlExitUserThread,
            LongToPtr(STATUS_SUCCESS),
            &threadHandle,
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = NtQueueApcThread(
            threadHandle,
            loadLibraryW,
            fileNameBaseAddress,
            NULL,
            NULL
            );
    }
    else
    {
        status = PhCreateUserThread(
            ProcessHandle,
            NULL,
            THREAD_ALL_ACCESS,
            0,
            0,
            0,
            0,
            loadLibraryW,
            fileNameBaseAddress,
            &threadHandle,
            NULL
            );
    }

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWaitForSingleObject(threadHandle, Timeout);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

CleanupExit:
    if (threadHandle)
        NtClose(threadHandle);

    if (powerRequestHandle)
        PhDestroyExecutionRequiredRequest(powerRequestHandle);

    if (fileNameBaseAddress)
        PhFreeVirtualMemory(ProcessHandle, fileNameBaseAddress, MEM_RELEASE);

    return status;
}

/**
 * Causes a process to unload a DLL.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_CREATE_THREAD, PROCESS_VM_OPERATION, PROCESS_VM_READ
 * and PROCESS_VM_WRITE access.
 * \param BaseAddress The base address of the DLL to unload.
 * \param Timeout The timeout, in milliseconds, for the process to unload the DLL.
 */
NTSTATUS PhUnloadDllProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_opt_ ULONG Timeout
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    HANDLE powerRequestHandle = NULL;
    THREAD_BASIC_INFORMATION basicInfo;
    PVOID freeLibrary = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // No point trying to set the load count on Windows 8 and higher, because NT now uses a DAG of
    // loader nodes. (wj32)
    if (WindowsVersion < WINDOWS_8)
    {
#ifdef _WIN64
        BOOLEAN isWow64 = FALSE;
#endif
        status = PhSetProcessModuleLoadCount(
            ProcessHandle,
            BaseAddress,
            1
            );

#ifdef _WIN64
        PhGetProcessIsWow64(ProcessHandle, &isWow64);

        if (isWow64 && status == STATUS_DLL_NOT_FOUND)
        {
            // The DLL might be 32-bit.
            status = PhSetProcessModuleLoadCount32(
                ProcessHandle,
                BaseAddress,
                1
                );
        }
#endif
        if (!NT_SUCCESS(status))
            return status;
    }

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        "FreeLibrary",
        &freeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            return status;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        0,
        0,
        0,
        0,
        freeLibrary,
        BaseAddress,
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhWaitForSingleObject(threadHandle, Timeout);

    if (status == STATUS_WAIT_0)
    {
        status = PhGetThreadBasicInformation(threadHandle, &basicInfo);

        if (NT_SUCCESS(status))
            status = basicInfo.ExitStatus;
    }

    NtClose(threadHandle);

    if (powerRequestHandle)
        PhDestroyExecutionRequiredRequest(powerRequestHandle);

    return status;
}

/**
 * Sets an environment variable in a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION, PROCESS_CREATE_THREAD, PROCESS_VM_OPERATION, PROCESS_VM_READ
 * and PROCESS_VM_WRITE access.
 * \param Name The name of the environment variable to set.
 * \param Value The new value of the environment variable. If this parameter is NULL, the
 * environment variable is deleted.
 * \param Timeout The timeout, in milliseconds, for the process to set the environment variable.
 */
NTSTATUS PhSetEnvironmentVariableRemote(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_STRINGREF Name,
    _In_opt_ PPH_STRINGREF Value,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInformation;
    PVOID nameBaseAddress = NULL;
    PVOID valueBaseAddress = NULL;
    SIZE_T nameAllocationSize = 0;
    SIZE_T valueAllocationSize = 0;
    PVOID rtlExitUserThread = NULL;
    PVOID setEnvironmentVariableW = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif

    nameAllocationSize = Name->Length + sizeof(UNICODE_NULL);

    if (Value)
        valueAllocationSize = Value->Length + sizeof(UNICODE_NULL);

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
#ifdef _WIN64
        &isWow64
#else
        NULL
#endif
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlExitUserThread",
        &rtlExitUserThread,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        "SetEnvironmentVariableW",
        &setEnvironmentVariableW,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtAllocateVirtualMemory(
        ProcessHandle,
        &nameBaseAddress,
        0,
        &nameAllocationSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtWriteVirtualMemory(
        ProcessHandle,
        nameBaseAddress,
        Name->Buffer,
        Name->Length,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (Value)
    {
        status = NtAllocateVirtualMemory(
            ProcessHandle,
            &valueBaseAddress,
            0,
            &valueAllocationSize,
            MEM_COMMIT,
            PAGE_READWRITE
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = NtWriteVirtualMemory(
            ProcessHandle,
            valueBaseAddress,
            Value->Buffer,
            Value->Length,
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
        0,
        0,
        0,
        rtlExitUserThread,
        LongToPtr(STATUS_SUCCESS),
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

#ifdef _WIN64
    if (isWow64)
    {
        status = RtlQueueApcWow64Thread(
            threadHandle,
            setEnvironmentVariableW,
            nameBaseAddress,
            valueBaseAddress,
            NULL
            );
    }
    else
    {
#endif
        status = NtQueueApcThread(
            threadHandle,
            setEnvironmentVariableW,
            nameBaseAddress,
            valueBaseAddress,
            NULL
            );
#ifdef _WIN64
    }
#endif
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtResumeThread(threadHandle, NULL); // Execute the pending APC (dmex)

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtWaitForSingleObject(threadHandle, FALSE, Timeout);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetThreadBasicInformation(threadHandle, &basicInformation);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = basicInformation.ExitStatus;

CleanupExit:

    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    if (nameBaseAddress)
    {
        nameAllocationSize = 0;
        NtFreeVirtualMemory(
            ProcessHandle,
            &nameBaseAddress,
            &nameAllocationSize,
            MEM_RELEASE
            );
    }

    if (valueBaseAddress)
    {
        valueAllocationSize = 0;
        NtFreeVirtualMemory(
            ProcessHandle,
            &valueBaseAddress,
            &valueAllocationSize,
            MEM_RELEASE
            );
    }

    return status;
}

// based on https://www.drdobbs.com/a-safer-alternative-to-terminateprocess/184416547 (dmex)
NTSTATUS PhTerminateProcessAlternative(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus,
    _In_opt_ ULONG Timeout
    )
{
    NTSTATUS status;
    PVOID rtlExitUserProcess = NULL;
    HANDLE powerRequestHandle = NULL;
    HANDLE threadHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlExitUserProcess",
        &rtlExitUserProcess,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        0,
        0,
        0,
        0,
        rtlExitUserProcess,
        LongToPtr(ExitStatus),
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWaitForSingleObject(threadHandle, Timeout);

CleanupExit:

    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
}

NTSTATUS PhGetProcessSystemDllInitBlock(
    _In_ HANDLE ProcessHandle,
    _Out_ PPS_SYSTEM_DLL_INIT_BLOCK SystemDllInitBlock
    )
{
    NTSTATUS status;
    PS_SYSTEM_DLL_INIT_BLOCK ldrSystemDllInitBlock = { 0 };
    PVOID ldrSystemDllInitBlockAddress;

    status = PhGetProcedureAddressRemoteZ(
        ProcessHandle,
        L"\\SystemRoot\\System32\\ntdll.dll",
        "LdrSystemDllInitBlock",
        &ldrSystemDllInitBlockAddress,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtReadVirtualMemory(
        ProcessHandle,
        ldrSystemDllInitBlockAddress,
        &ldrSystemDllInitBlock,
        PS_SYSTEM_DLL_INIT_BLOCK_SIZE_V1,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (RTL_CONTAINS_FIELD(&ldrSystemDllInitBlock, ldrSystemDllInitBlock.Size, MitigationAuditOptionsMap))
    {
        RtlMoveMemory(SystemDllInitBlock, &ldrSystemDllInitBlock, sizeof(ldrSystemDllInitBlock));
    }
    else
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }

    return status;
}

NTSTATUS PhGetProcessCodePage(
    _In_ HANDLE ProcessHandle,
    _Out_ PUSHORT ProcessCodePage
    )
{
    NTSTATUS status;
    USHORT codePage = 0;

    if (WindowsVersion >= WINDOWS_11)
    {
        PVOID pebBaseAddress;
#ifdef _WIN64
        BOOLEAN isWow64 = FALSE;

        PhGetProcessIsWow64(ProcessHandle, &isWow64);

        if (isWow64)
        {
            status = PhGetProcessPeb32(ProcessHandle, &pebBaseAddress);

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            status = NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB32, ActiveCodePage)),
                &codePage,
                sizeof(USHORT),
                NULL
                );
        }
        else
#endif
        {
            status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress);

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            status = NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(pebBaseAddress, UFIELD_OFFSET(PEB, ActiveCodePage)),
                &codePage,
                sizeof(USHORT),
                NULL
                );
        }
    }
    else
    {
        PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;
        PVOID nlsAnsiCodePage;

        status = PhGetProcessRuntimeLibrary(
            ProcessHandle,
            &runtimeLibrary,
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhGetProcedureAddressRemote(
            ProcessHandle,
            &runtimeLibrary->NtdllFileName,
            "NlsAnsiCodePage",
            &nlsAnsiCodePage,
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = NtReadVirtualMemory(
            ProcessHandle,
            nlsAnsiCodePage,
            &codePage,
            sizeof(USHORT),
            NULL
            );
    }

    if (NT_SUCCESS(status))
    {
        *ProcessCodePage = codePage;
    }

CleanupExit:
    return status;
}

NTSTATUS PhGetProcessConsoleCodePage(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN ConsoleOutputCP,
    _Out_ PUSHORT ConsoleCodePage
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInformation;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PVOID getConsoleCP = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        ConsoleOutputCP ? "GetConsoleOutputCP" : "GetConsoleCP",
        &getConsoleCP,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            return status;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        0,
        0,
        0,
        0,
        getConsoleCP,
        NULL,
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWaitForSingleObject(threadHandle, 5000);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetThreadBasicInformation(threadHandle, &basicInformation);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    *ConsoleCodePage = (USHORT)basicInformation.ExitStatus;

CleanupExit:
    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
}

NTSTATUS PhFlushProcessHeapsRemote(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;
    THREAD_BASIC_INFORMATION basicInformation;
    PVOID rtlExitUserThread = NULL;
    PVOID rtlFlushHeaps = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
#ifdef _WIN64
        &isWow64
#else
        NULL
#endif
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlExitUserThread",
        &rtlExitUserThread,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlFlushHeaps",
        &rtlFlushHeaps,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
        0,
        0,
        0,
        rtlExitUserThread,
        LongToPtr(STATUS_SUCCESS),
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

#ifdef _WIN64
    if (isWow64)
    {
        status = RtlQueueApcWow64Thread(
            threadHandle,
            rtlFlushHeaps,
            NULL,
            NULL,
            NULL
            );
    }
    else
    {
#endif
        status = NtQueueApcThreadEx(
            threadHandle,
            QUEUE_USER_APC_SPECIAL_USER_APC,
            rtlFlushHeaps,
            NULL,
            NULL,
            NULL
            );
#ifdef _WIN64
    }
#endif
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtResumeThread(threadHandle, NULL); // Execute the pending APC (dmex)

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtWaitForSingleObject(threadHandle, FALSE, Timeout);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetThreadBasicInformation(threadHandle, &basicInformation);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = basicInformation.ExitStatus;

CleanupExit:

    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
}

/*
 * Invokes a procedure in the context of the owning thread for the window.
 *
 * \param WindowHandle The handle of the window.
 * \param ApcRoutine The procedure to be invoked.
 * \param ApcArgument1 The first argument to be passed to the procedure.
 * \param ApcArgument2 The second argument to be passed to the procedure.
 * \param ApcArgument3 The third argument to be passed to the procedure.
 *
 * \return The status of the operation.
 */
NTSTATUS PhInvokeWindowProcedureRemote(
    _In_ HWND WindowHandle,
    _In_ PVOID ApcRoutine,
    _In_opt_ PVOID ApcArgument1,
    _In_opt_ PVOID ApcArgument2,
    _In_opt_ PVOID ApcArgument3
    )
{
    NTSTATUS status;
    HANDLE processHande = NULL;
    HANDLE threadHande = NULL;
    HANDLE powerHandle = NULL;
    CLIENT_ID clientId;

    // Get the client ID of the window.

    status = PhGetWindowClientId(WindowHandle, &clientId);

    if (!NT_SUCCESS(status))
        return status;

    // Open the process associated with the window.

    status = PhOpenProcessClientId(
        &processHande,
        PROCESS_ALL_ACCESS,
        &clientId
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Open the thread associated with the window.

    status = PhOpenThreadClientId(
        &threadHande,
        THREAD_ALL_ACCESS, // THREAD_ALERT
        &clientId
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Create an execution required request for the process (Windows 8 and above)

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(processHande, &powerHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    // Queue a Special user-mode APC to execute within the context of the message loop.

    status = NtQueueApcThreadEx(
        threadHande,
        QUEUE_USER_APC_SPECIAL_USER_APC,
        ApcRoutine,
        ApcArgument1,
        ApcArgument2,
        ApcArgument3
        );

CleanupExit:
    if (threadHande)
        NtClose(threadHande);
    if (processHande)
        NtClose(processHande);
    if (powerHandle)
        PhDestroyExecutionRequiredRequest(powerHandle);

    return status;
}

/**
 * Destroys the specified window in a process.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_SET_LIMITED_INFORMATION access.
 * \param WindowHandle A handle to the window to be destroyed.
 *
 * \return Successful or errant status.
 *
 * \remarks A thread cannot call DestroyWindow for a window created by a different thread,
 * unless we queue a special APC to the owner thread.
 */
NTSTATUS PhDestroyWindowRemote(
    _In_ HANDLE ProcessHandle,
    _In_ HWND WindowHandle
    )
{
    NTSTATUS status;
    PVOID destroyWindow = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->User32FileName,
        "DestroyWindow",
        &destroyWindow,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhInvokeWindowProcedureRemote(
        WindowHandle,
        destroyWindow,
        (PVOID)WindowHandle,
        NULL,
        NULL
        );

CleanupExit:
    return status;
}

NTSTATUS PhPostWindowQuitMessageRemote(
    _In_ HANDLE ProcessHandle,
    _In_ HWND WindowHandle
    )
{
    NTSTATUS status;
    PVOID postQuitMessage = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->User32FileName,
        "PostQuitMessage",
        &postQuitMessage,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhInvokeWindowProcedureRemote(
        WindowHandle,
        postQuitMessage,
        UlongToPtr(EXIT_SUCCESS),
        NULL,
        NULL
        );

CleanupExit:
    return status;
}

/// https://learn.microsoft.com/en-us/windows/win32/multimedia/obtaining-and-setting-timer-resolution
NTSTATUS PhSetProcessTimerResolutionRemote(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Period
    )
{
    NTSTATUS status;
    PVOID rtlExitUserThread = NULL;
    PVOID timeBeginPeriod = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;
    LARGE_INTEGER timeout;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
#ifdef _WIN64
        & isWow64
#else
        NULL
#endif
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlExitUserThread",
        &rtlExitUserThread,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        "TimeBeginPeriod",
        &timeBeginPeriod,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
        0,
        0,
        0,
        rtlExitUserThread,
        LongToPtr(STATUS_SUCCESS),
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

#ifdef _WIN64
    if (isWow64)
    {
        status = RtlQueueApcWow64Thread(
            threadHandle,
            timeBeginPeriod,
            UlongToPtr(Period),
            NULL,
            NULL
            );
    }
    else
    {
#endif
        status = NtQueueApcThread(
            threadHandle,
            timeBeginPeriod,
            UlongToPtr(Period),
            NULL,
            NULL
            );
#ifdef _WIN64
    }
#endif
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtResumeThread(threadHandle, NULL);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtWaitForSingleObject(threadHandle, FALSE, PhTimeoutFromMilliseconds(&timeout, 5000));

    if (!NT_SUCCESS(status))
        goto CleanupExit;

CleanupExit:

    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
}

NTSTATUS PhSetProcessTimerResolutionRemote2(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Period
    )
{
    NTSTATUS status;
    PVOID rtlExitUserThread = NULL;
    PVOID timeBeginPeriod = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
#ifdef _WIN64
        & isWow64
#else
        NULL
#endif
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlExitUserThread",
        &rtlExitUserThread,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        "TimeBeginPeriod",
        &timeBeginPeriod,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
        0,
        0,
        0,
        rtlExitUserThread,
        LongToPtr(STATUS_SUCCESS),
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

#ifdef _WIN64
    if (isWow64)
    {
        status = RtlQueueApcWow64Thread(
            threadHandle,
            timeBeginPeriod,
            UlongToPtr(Period),
            NULL,
            NULL
            );
    }
    else
    {
#endif
        status = NtQueueApcThread(
            threadHandle,
            timeBeginPeriod,
            UlongToPtr(Period),
            NULL,
            NULL
            );
#ifdef _WIN64
    }
#endif
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtResumeThread(threadHandle, NULL);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWaitForSingleObject(threadHandle, 5000);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

CleanupExit:

    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
}

NTSTATUS PhSetHandleInformationRemote(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE RemoteHandle,
    _In_ ULONG Mask,
    _In_ ULONG Flags
    )
{
    NTSTATUS status;
    PVOID rtlExitUserThread = NULL;
    PVOID setHandleInformation = NULL;
    HANDLE threadHandle = NULL;
    HANDLE powerRequestHandle = NULL;
    PPH_PROCESS_RUNTIME_LIBRARY runtimeLibrary;
    THREAD_BASIC_INFORMATION basicInformation;
#ifdef _WIN64
    BOOLEAN isWow64;
#endif

    status = PhGetProcessRuntimeLibrary(
        ProcessHandle,
        &runtimeLibrary,
#ifdef _WIN64
        & isWow64
#else
        NULL
#endif
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->NtdllFileName,
        "RtlExitUserThread",
        &rtlExitUserThread,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcedureAddressRemote(
        ProcessHandle,
        &runtimeLibrary->Kernel32FileName,
        "SetHandleInformation",
        &setHandleInformation,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (WindowsVersion >= WINDOWS_8)
    {
        status = PhCreateExecutionRequiredRequest(ProcessHandle, &powerRequestHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = PhCreateUserThread(
        ProcessHandle,
        NULL,
        THREAD_ALL_ACCESS,
        THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
        0,
        0,
        0,
        rtlExitUserThread,
        LongToPtr(STATUS_SUCCESS),
        &threadHandle,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

#ifdef _WIN64
    if (isWow64)
    {
        status = RtlQueueApcWow64Thread(
            threadHandle,
            setHandleInformation,
            RemoteHandle,
            UlongToPtr(Mask),
            UlongToPtr(Flags)
            );
    }
    else
    {
#endif
        status = NtQueueApcThread(
            threadHandle,
            setHandleInformation,
            RemoteHandle,
            UlongToPtr(Mask),
            UlongToPtr(Flags)
            );
#ifdef _WIN64
    }
#endif
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtResumeThread(threadHandle, NULL);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhWaitForSingleObject(threadHandle, 5000);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetThreadBasicInformation(threadHandle, &basicInformation);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if ((BOOL)basicInformation.ExitStatus) // Read TEB->LastErrorStatus?
        status = STATUS_SUCCESS;
    else
        status = STATUS_UNSUCCESSFUL;

CleanupExit:

    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (powerRequestHandle)
    {
        PhDestroyExecutionRequiredRequest(powerRequestHandle);
    }

    return status;
}
