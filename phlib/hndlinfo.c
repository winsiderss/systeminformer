/*
 * Process Hacker - 
 *   handle information
 * 
 * Copyright (C) 2010 wj32
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

#include <ph.h>
#include <kph.h>

typedef enum _PH_QUERY_OBJECT_WORK
{
    QueryNameHack,
    QuerySecurityHack,
    SetSecurityHack
} PH_QUERY_OBJECT_WORK;

typedef struct _PH_QUERY_OBJECT_CONTEXT
{
    LOGICAL Initialized;
    PH_QUERY_OBJECT_WORK Work;

    HANDLE Handle;
    SECURITY_INFORMATION SecurityInformation;
    PVOID Buffer;
    ULONG Length;

    NTSTATUS Status;
    ULONG ReturnLength;
} PH_QUERY_OBJECT_CONTEXT, *PPH_QUERY_OBJECT_CONTEXT;

NTSTATUS PhpQueryObjectThreadStart(
    __in PVOID Parameter
    );

HANDLE PhQueryObjectThreadHandle = NULL;
PVOID PhQueryObjectFiber = NULL;
PH_MUTEX PhQueryObjectMutex;
HANDLE PhQueryObjectStartEvent = NULL;
HANDLE PhQueryObjectCompletedEvent = NULL;
PH_QUERY_OBJECT_CONTEXT PhQueryObjectContext;

#define MAX_OBJECT_TYPE_NUMBER 256
PPH_STRING PhObjectTypeNames[MAX_OBJECT_TYPE_NUMBER + 1];
PPH_GET_CLIENT_ID_NAME PhHandleGetClientIdName = PhStdGetClientIdName;

static PPH_STRING HkcuPrefix;
static PPH_STRING HkcucrPrefix;

VOID PhHandleInfoInitialization()
{
    PhInitializeMutex(&PhQueryObjectMutex);

    memset(PhObjectTypeNames, 0, sizeof(PhObjectTypeNames));

    {
        HANDLE tokenHandle;
        PTOKEN_USER tokenUser;
        PPH_STRING stringSid = NULL;

        if (NT_SUCCESS(PhOpenProcessToken(
            &tokenHandle,
            TOKEN_QUERY,
            NtCurrentProcess()
            )))
        {
            if (NT_SUCCESS(PhGetTokenUser(
                tokenHandle,
                &tokenUser
                )))
            {
                stringSid = PhSidToStringSid(tokenUser->User.Sid);

                PhFree(tokenUser);
            }

            NtClose(tokenHandle);
        }

        if (stringSid)
        {
            HkcuPrefix = PhConcatStrings2(L"\\REGISTRY\\USER\\", stringSid->Buffer);
            HkcucrPrefix = PhConcatStrings2(HkcuPrefix->Buffer, L"_Classes");
        }
        else
        {
            HkcuPrefix = PhCreateString(L"..."); // some random string that won't ever get matched
            HkcucrPrefix = PhCreateString(L"...");
        }
    }
}

PPH_GET_CLIENT_ID_NAME PhSetHandleClientIdFunction(
    __in PPH_GET_CLIENT_ID_NAME GetClientIdName
    )
{
    return _InterlockedExchangePointer(
        (PPVOID)&PhHandleGetClientIdName,
        GetClientIdName
        );
}

NTSTATUS PhpGetObjectBasicInformation(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __out POBJECT_BASIC_INFORMATION BasicInformation
    )
{
    NTSTATUS status;

    if (PhKphHandle)
    {
        status = KphZwQueryObject(
            PhKphHandle,
            ProcessHandle,
            Handle,
            ObjectBasicInformation,
            BasicInformation,
            sizeof(OBJECT_BASIC_INFORMATION),
            NULL
            );

        if (NT_SUCCESS(status))
        {
            // The object was referenced in KProcessHacker, so 
            // we need to subtract 1 from the pointer count.
            BasicInformation->PointerCount -= 1;
        }
    }
    else
    {
        status = NtQueryObject(
            Handle,
            ObjectBasicInformation,
            BasicInformation,
            sizeof(OBJECT_BASIC_INFORMATION),
            NULL
            );

        if (NT_SUCCESS(status))
        {
            // The object was referenced in NtQueryObject and 
            // a handle was opened to the object. We need to 
            // subtract 1 from the pointer count, then subtract 
            // 1 from both counts.
            BasicInformation->HandleCount -= 1;
            BasicInformation->PointerCount -= 2;
        }
    }

    return status;
}

NTSTATUS PhpGetObjectTypeName(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in ULONG ObjectTypeNumber,
    __out PPH_STRING *TypeName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PPH_STRING typeName = NULL;

    // If the cache contains the object type name, use it. Otherwise, 
    // query the type name.

    if (ObjectTypeNumber != -1 && ObjectTypeNumber <= MAX_OBJECT_TYPE_NUMBER)
        typeName = PhObjectTypeNames[ObjectTypeNumber];

    if (typeName)
    {
        PhReferenceObject(typeName);
    }
    else
    {
        POBJECT_TYPE_INFORMATION buffer;
        ULONG returnLength = 0;
        PPH_STRING oldTypeName;

        // Get the needed buffer size.
        if (PhKphHandle)
        {
            status = KphZwQueryObject(
                PhKphHandle,
                ProcessHandle,
                Handle,
                ObjectTypeInformation,
                NULL,
                0,
                &returnLength
                );
        }
        else
        {
            status = NtQueryObject(
                Handle,
                ObjectTypeInformation,
                NULL,
                0,
                &returnLength
                );
        }

        if (returnLength == 0)
            return status;

        buffer = PhAllocate(returnLength);

        if (PhKphHandle)
        {
            status = KphZwQueryObject(
                PhKphHandle,
                ProcessHandle,
                Handle,
                ObjectTypeInformation,
                buffer,
                returnLength,
                &returnLength
                );
        }
        else
        {
            status = NtQueryObject(
                Handle,
                ObjectTypeInformation,
                buffer,
                returnLength,
                &returnLength
                );
        }

        if (!NT_SUCCESS(status))
        {
            PhFree(buffer);
            return status;
        }

        // Create a copy of the type name.
        typeName = PhCreateStringEx(buffer->TypeName.Buffer, buffer->TypeName.Length);

        if (ObjectTypeNumber != -1)
        {
            // Try to store the type name in the cache.
            oldTypeName = _InterlockedCompareExchangePointer(
                &PhObjectTypeNames[ObjectTypeNumber],
                typeName,
                NULL
                );

            // Add a reference if we stored the type name 
            // successfully.
            if (!oldTypeName)
                PhReferenceObject(typeName);
        }

        PhFree(buffer);
    }

    // At this point typeName should contain a type name 
    // with one additional reference.

    *TypeName = typeName;

    return status;
}

NTSTATUS PhpGetObjectName(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __out PPH_STRING *ObjectName
    )
{
    NTSTATUS status;
    POBJECT_NAME_INFORMATION buffer;
    ULONG returnLength = 0;

    // Get the needed buffer size.
    if (PhKphHandle)
    {
        status = KphZwQueryObject(
            PhKphHandle,
            ProcessHandle,
            Handle,
            ObjectNameInformation,
            NULL,
            0,
            &returnLength
            );
    }
    else
    {
        status = NtQueryObject(
            Handle,
            ObjectNameInformation,
            NULL,
            0,
            &returnLength
            );
    }

    if (returnLength == 0)
        return status;

    buffer = PhAllocate(returnLength);

    if (PhKphHandle)
    {
        status = KphZwQueryObject(
            PhKphHandle,
            ProcessHandle,
            Handle,
            ObjectNameInformation,
            buffer,
            returnLength,
            &returnLength
            );
    }
    else
    {
        status = NtQueryObject(
            Handle,
            ObjectNameInformation,
            buffer,
            returnLength,
            &returnLength
            );
    }

    if (NT_SUCCESS(status))
    {
        *ObjectName = PhCreateStringEx(buffer->Name.Buffer, buffer->Name.Length);
    }

    PhFree(buffer);

    return status;
}

PPH_STRING PhFormatNativeKeyName(
    __in PPH_STRING Name
    )
{
#define HKLM_PREFIX L"\\REGISTRY\\MACHINE"
#define HKLM_PREFIX_LENGTH 17
#define HKCR_PREFIX L"\\REGISTRY\\MACHINE\\SOFTWARE\\CLASSES"
#define HKCR_PREFIX_LENGTH 34
#define HKU_PREFIX L"\\REGISTRY\\USER"
#define HKU_PREFIX_LENGTH 14

    PPH_STRING newName;

    if (PhStringStartsWith2(Name, HKCR_PREFIX, TRUE))
    {
        newName = PhConcatStrings2(L"HKCR", &Name->Buffer[HKCR_PREFIX_LENGTH]);
    }
    else if (PhStringStartsWith2(Name, HKLM_PREFIX, TRUE))
    {
        newName = PhConcatStrings2(L"HKLM", &Name->Buffer[HKLM_PREFIX_LENGTH]);
    }
    else if (PhStringStartsWith(Name, HkcucrPrefix, TRUE))
    {
        newName = PhConcatStrings2(
            L"HKCU\\Software\\Classes",
            &Name->Buffer[HkcucrPrefix->Length / 2]
            );
    }
    else if (PhStringStartsWith(Name, HkcuPrefix, TRUE))
    {
        newName = PhConcatStrings2(L"HKCU", &Name->Buffer[HkcuPrefix->Length / 2]);
    }
    else if (PhStringStartsWith2(Name, HKU_PREFIX, TRUE))
    {
        newName = PhConcatStrings2(L"HKU", &Name->Buffer[HKU_PREFIX_LENGTH]);
    }
    else
    {
        newName = Name;
        PhReferenceObject(Name);
    }

    return newName;
}

PPH_STRING PhStdGetClientIdName(
    __in PCLIENT_ID ClientId
    )
{
    static PH_QUEUED_LOCK cachedProcessesLock = PH_QUEUED_LOCK_INIT;
    static PVOID processes = NULL;
    static ULONG64 lastProcessesTickCount = 0;

    PPH_STRING name;
    ULONG64 tickCount;
    PSYSTEM_PROCESS_INFORMATION processInfo;

    // Get a new process list only if 2 seconds have passed 
    // since the last update.

    tickCount = NtGetTickCount64();

    if (tickCount - lastProcessesTickCount >= 2000)
    {
        PhAcquireQueuedLockExclusiveFast(&cachedProcessesLock);

        // Re-check the tick count.
        if (tickCount - lastProcessesTickCount >= 2000)
        {
            if (processes)
            {
                PhFree(processes);
                processes = NULL;
            }

            if (!NT_SUCCESS(PhEnumProcesses(&processes)))
            {
                PhReleaseQueuedLockExclusive(&cachedProcessesLock);
                return PhCreateString(L"(Error querying processes)");
            }

            lastProcessesTickCount = tickCount;
        }

        PhReleaseQueuedLockExclusiveFast(&cachedProcessesLock);
    }

    // Get a lock on the process list and get a name for the client ID.

    PhAcquireQueuedLockSharedFast(&cachedProcessesLock);

    if (!processes)
    {
        PhReleaseQueuedLockShared(&cachedProcessesLock);
        return NULL;
    }

    processInfo = PhFindProcessInformation(processes, ClientId->UniqueProcess);

    if (ClientId->UniqueThread)
    {
        if (processInfo)
        {
            name = PhFormatString(
                L"%.*s (%u): %u",
                processInfo->ImageName.Length / 2,
                processInfo->ImageName.Buffer,
                (ULONG)ClientId->UniqueProcess,
                (ULONG)ClientId->UniqueThread
                );
        }
        else
        {
            name = PhFormatString(L"Non-existent process (%u): %u",
                (ULONG)ClientId->UniqueProcess, (ULONG)ClientId->UniqueThread);
        }
    }
    else
    {
        if (processInfo)
        {
            name = PhFormatString(
                L"%.*s (%u)",
                processInfo->ImageName.Length / 2,
                processInfo->ImageName.Buffer,
                (ULONG)ClientId->UniqueProcess
                );
        }
        else
        {
            name = PhFormatString(L"Non-existent process (%u)", (ULONG)ClientId->UniqueProcess);
        }
    }

    PhReleaseQueuedLockSharedFast(&cachedProcessesLock);

    return name;
}

NTSTATUS PhpGetBestObjectName(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in PPH_STRING ObjectName,
    __in PPH_STRING TypeName,
    __out PPH_STRING *BestObjectName
    )
{
    NTSTATUS status;
    PPH_STRING bestObjectName = NULL;

    if (PhStringEquals2(TypeName, L"File", TRUE))
    {
        // Convert the file name to a DOS file name.
        bestObjectName = PhResolveDevicePrefix(ObjectName);

        if (!bestObjectName)
        {
            bestObjectName = ObjectName;
            PhReferenceObject(ObjectName);
        }
    }
    else if (PhStringEquals2(TypeName, L"Key", TRUE))
    {
        bestObjectName = PhFormatNativeKeyName(ObjectName);
    }
    else if (PhStringEquals2(TypeName, L"Process", TRUE))
    {
        CLIENT_ID clientId;

        clientId.UniqueThread = NULL;

        if (PhKphHandle)
        {
            status = KphGetProcessId(
                PhKphHandle,
                ProcessHandle,
                Handle,
                &clientId.UniqueProcess
                );

            if (!NT_SUCCESS(status) || !clientId.UniqueProcess)
                goto CleanupExit;
        }
        else
        {
            HANDLE dupHandle;
            PROCESS_BASIC_INFORMATION basicInfo;

            status = NtDuplicateObject(
                ProcessHandle,
                Handle,
                NtCurrentProcess(),
                &dupHandle,
                ProcessQueryAccess,
                0,
                0
                );

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            status = PhGetProcessBasicInformation(dupHandle, &basicInfo);
            NtClose(dupHandle);

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            clientId.UniqueProcess = basicInfo.UniqueProcessId;
        }

        if (PhHandleGetClientIdName)
        {
            bestObjectName = PhHandleGetClientIdName(&clientId);
        }
    }
    else if (PhStringEquals2(TypeName, L"Thread", TRUE))
    {
        CLIENT_ID clientId;

        if (PhKphHandle)
        {
            status = KphGetThreadId(
                PhKphHandle,
                ProcessHandle,
                Handle,
                &clientId.UniqueThread,
                &clientId.UniqueProcess
                );

            if (!NT_SUCCESS(status) || !clientId.UniqueThread || !clientId.UniqueProcess)
                goto CleanupExit;
        }
        else
        {
            HANDLE dupHandle;
            THREAD_BASIC_INFORMATION basicInfo;

            status = NtDuplicateObject(
                ProcessHandle,
                Handle,
                NtCurrentProcess(),
                &dupHandle,
                ThreadQueryAccess,
                0,
                0
                );

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            status = PhGetThreadBasicInformation(dupHandle, &basicInfo);
            NtClose(dupHandle);

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            clientId = basicInfo.ClientId;
        }

        if (PhHandleGetClientIdName)
        {
            bestObjectName = PhHandleGetClientIdName(&clientId);
        }
    }
    else if (PhStringEquals2(TypeName, L"TmEn", TRUE))
    {
        HANDLE dupHandle;
        ENLISTMENT_BASIC_INFORMATION basicInfo;

        status = NtDuplicateObject(
            ProcessHandle,
            Handle,
            NtCurrentProcess(),
            &dupHandle,
            ENLISTMENT_QUERY_INFORMATION,
            0,
            0
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhGetEnlistmentBasicInformation(dupHandle, &basicInfo);
        NtClose(dupHandle);

        if (NT_SUCCESS(status))
        {
            bestObjectName = PhFormatGuid(&basicInfo.EnlistmentId);
        }
    }
    else if (PhStringEquals2(TypeName, L"TmRm", TRUE))
    {
        HANDLE dupHandle;
        GUID guid;
        PPH_STRING description;

        status = NtDuplicateObject(
            ProcessHandle,
            Handle,
            NtCurrentProcess(),
            &dupHandle,
            RESOURCEMANAGER_QUERY_INFORMATION,
            0,
            0
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhGetResourceManagerBasicInformation(
            dupHandle,
            &guid,
            &description
            );
        NtClose(dupHandle);

        if (NT_SUCCESS(status))
        {
            if (!PhIsStringNullOrEmpty(description))
            {
                bestObjectName = description;
            }
            else
            {
                bestObjectName = PhFormatGuid(&guid);

                if (description)
                    PhDereferenceObject(description);
            }
        }
    }
    else if (PhStringEquals2(TypeName, L"TmTm", TRUE))
    {
        HANDLE dupHandle;
        PPH_STRING logFileName = NULL;
        TRANSACTIONMANAGER_BASIC_INFORMATION basicInfo;

        status = NtDuplicateObject(
            ProcessHandle,
            Handle,
            NtCurrentProcess(),
            &dupHandle,
            TRANSACTIONMANAGER_QUERY_INFORMATION,
            0,
            0
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhGetTransactionManagerLogFileName(
            dupHandle,
            &logFileName
            );

        if (NT_SUCCESS(status) && !PhIsStringNullOrEmpty(logFileName))
        {
            bestObjectName = PhResolveDevicePrefix(logFileName);

            if (bestObjectName)
                PhDereferenceObject(logFileName);
            else
                bestObjectName = logFileName;
        }
        else
        {
            if (logFileName)
                PhDereferenceObject(logFileName);

            status = PhGetTransactionManagerBasicInformation(
                dupHandle,
                &basicInfo
                );

            if (NT_SUCCESS(status))
            {
                bestObjectName = PhFormatGuid(&basicInfo.TmIdentity);
            }
        }

        NtClose(dupHandle);
    }
    else if (PhStringEquals2(TypeName, L"TmTx", TRUE))
    {
        HANDLE dupHandle;
        PPH_STRING description = NULL;
        TRANSACTION_BASIC_INFORMATION basicInfo;

        status = NtDuplicateObject(
            ProcessHandle,
            Handle,
            NtCurrentProcess(),
            &dupHandle,
            TRANSACTION_QUERY_INFORMATION,
            0,
            0
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhGetTransactionPropertiesInformation(
            dupHandle,
            NULL,
            NULL,
            &description
            );

        if (NT_SUCCESS(status) && !PhIsStringNullOrEmpty(description))
        {
            bestObjectName = description;
        }
        else
        {
            if (description)
                PhDereferenceObject(description);

            status = PhGetTransactionBasicInformation(
                dupHandle,
                &basicInfo
                );

            if (NT_SUCCESS(status))
            {
                bestObjectName = PhFormatGuid(&basicInfo.TransactionId);
            }
        }

        NtClose(dupHandle);
    }
    else if (PhStringEquals2(TypeName, L"Token", TRUE))
    {
        HANDLE dupHandle;
        PTOKEN_USER tokenUser = NULL;
        TOKEN_STATISTICS statistics = { 0 };

        status = NtDuplicateObject(
            ProcessHandle,
            Handle,
            NtCurrentProcess(),
            &dupHandle,
            TOKEN_QUERY,
            0,
            0
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhGetTokenUser(dupHandle, &tokenUser);
        PhGetTokenStatistics(dupHandle, &statistics);

        if (NT_SUCCESS(status))
        {
            PPH_STRING fullName;

            fullName = PhGetSidFullName(tokenUser->User.Sid, TRUE, NULL);

            if (fullName)
            {
                bestObjectName = PhFormatString(L"%s: 0x%x", fullName->Buffer, statistics.AuthenticationId.LowPart);
                PhDereferenceObject(fullName);
            }

            PhFree(tokenUser);
        }

        NtClose(dupHandle);
    }

CleanupExit:

    if (!bestObjectName)
    {
        bestObjectName = ObjectName;
        PhReferenceObject(ObjectName);
    }

    *BestObjectName = bestObjectName;

    return STATUS_SUCCESS;
}

/**
 * Gets information for a handle.
 *
 * \param ProcessHandle A handle to the process in which the 
 * handle resides.
 * \param Handle The handle value.
 * \param ObjectTypeNumber The object type number of the handle. 
 * You can specify -1 for this parameter if the object type number 
 * is not known.
 * \param BasicInformation A variable which receives basic 
 * information about the object.
 * \param TypeName A variable which receives the object type name.
 * \param ObjectName A variable which receives the object name.
 * \param BestObjectName A variable which receives the formatted 
 * object name.
 *
 * \retval STATUS_INVALID_HANDLE The handle specified in
 * \c ProcessHandle or \c Handle is invalid.
 * \retval STATUS_INVALID_PARAMETER_3 The value specified in 
 * \c ObjectTypeNumber is invalid.
 */
NTSTATUS PhGetHandleInformation(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in ULONG ObjectTypeNumber,
    __out_opt POBJECT_BASIC_INFORMATION BasicInformation,
    __out_opt PPH_STRING *TypeName,
    __out_opt PPH_STRING *ObjectName,
    __out_opt PPH_STRING *BestObjectName
    )
{
    NTSTATUS status;
    HANDLE dupHandle = NULL;
    PPH_STRING typeName = NULL;
    PPH_STRING objectName = NULL;
    PPH_STRING bestObjectName = NULL;

    if (Handle == NULL || Handle == NtCurrentProcess() || Handle == NtCurrentThread())
        return STATUS_INVALID_HANDLE;
    if (ObjectTypeNumber != -1 && ObjectTypeNumber > MAX_OBJECT_TYPE_NUMBER)
        return STATUS_INVALID_PARAMETER_3;

    // Duplicate the handle if we're not using KPH.
    if (!PhKphHandle)
    {
        // However, we obviously don't need to duplicate it 
        // if the handle is in the current process.
        if (ProcessHandle != NtCurrentProcess())
        {
            status = NtDuplicateObject(
                ProcessHandle,
                Handle,
                NtCurrentProcess(),
                &dupHandle,
                0,
                0,
                0
                );

            if (!NT_SUCCESS(status))
                return status;
        }
        else
        {
            dupHandle = Handle;
        }
    }

    // Get basic information.
    if (BasicInformation)
    {
        status = PhpGetObjectBasicInformation(
            ProcessHandle,
            PhKphHandle ? Handle : dupHandle,
            BasicInformation
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    // Exit early if we don't need to get any other information.
    if (!TypeName && !ObjectName && !BestObjectName)
        goto CleanupExit;

    // Get the type name.
    status = PhpGetObjectTypeName(
        ProcessHandle,
        PhKphHandle ? Handle : dupHandle,
        ObjectTypeNumber,
        &typeName
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Exit early if we don't need to get the object name.
    if (!ObjectName && !BestObjectName)
        goto CleanupExit;

    // Get the object name.
    // If we're dealing with a file handle we must take 
    // special precautions so we don't hang.
    if (PhStringEquals2(typeName, L"File", TRUE))
    {
        // Use KPH if we can.
        if (PhKphHandle)
        {
            PUNICODE_STRING buffer;

            buffer = PhAllocate(0x800);

            status = KphGetHandleObjectName(
                PhKphHandle,
                ProcessHandle,
                Handle,
                buffer,
                0x800,
                NULL
                );

            if (NT_SUCCESS(status))
                objectName = PhCreateStringEx(buffer->Buffer, buffer->Length);

            PhFree(buffer);
        }
        else
        {
            // 0: Query normally.
            // 1: Hack.
            // 2: Fail.
            ULONG hackLevel = 1;

            // We can't use the hack on XP because hanging threads 
            // can't even be terminated!
            if (WindowsVersion <= WINDOWS_XP)
                hackLevel = 2;

            if (hackLevel == 0)
            {
                status = PhpGetObjectName(
                    ProcessHandle,
                    PhKphHandle ? Handle : dupHandle,
                    &objectName
                    );
            }
            else if (hackLevel == 1)
            {
                POBJECT_NAME_INFORMATION buffer;

                buffer = PhAllocate(0x800);

                status = PhQueryObjectNameHack(
                    dupHandle,
                    buffer,
                    0x800,
                    NULL
                    );

                if (NT_SUCCESS(status))
                    objectName = PhCreateStringEx(buffer->Name.Buffer, buffer->Name.Length);

                PhFree(buffer);
            }
            else
            {
                status = STATUS_UNSUCCESSFUL;
            }
        }
    }
    else
    {
        // Query the object normally.
        status = PhpGetObjectName(
            ProcessHandle,
            PhKphHandle ? Handle : dupHandle,
            &objectName
            );
    }

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhpGetBestObjectName(
        ProcessHandle,
        Handle,
        objectName,
        typeName,
        &bestObjectName
        );

CleanupExit:

    if (NT_SUCCESS(status))
    {
        if (TypeName && typeName)
        {
            *TypeName = typeName;
            PhReferenceObject(typeName);
        }

        if (ObjectName && objectName)
        {
            *ObjectName = objectName;
            PhReferenceObject(objectName);
        }

        if (BestObjectName && bestObjectName)
        {
            *BestObjectName = bestObjectName;
            PhReferenceObject(bestObjectName);
        }
    }

    if (dupHandle && ProcessHandle != NtCurrentProcess())
        NtClose(dupHandle);
    if (typeName)
        PhDereferenceObject(typeName);
    if (objectName)
        PhDereferenceObject(objectName);
    if (bestObjectName)
        PhDereferenceObject(bestObjectName);

    return status;
}

BOOLEAN PhpHeadQueryObjectHack()
{
    PhAcquireMutex(&PhQueryObjectMutex);

    // Create a query thread if we don't have one.
    if (!PhQueryObjectThreadHandle)
    {
        PhQueryObjectThreadHandle = CreateThread(NULL, 0, PhpQueryObjectThreadStart, NULL, 0, NULL);

        if (!PhQueryObjectThreadHandle)
            return FALSE;
    }

    // Create the events if they don't exist.

    if (!PhQueryObjectStartEvent)
    {
        if (!(PhQueryObjectStartEvent = CreateEvent(NULL, FALSE, FALSE, NULL)))
            return FALSE;
    }

    if (!PhQueryObjectCompletedEvent)
    {
        if (!(PhQueryObjectCompletedEvent = CreateEvent(NULL, FALSE, FALSE, NULL)))
            return FALSE;
    }

    return TRUE;
}

NTSTATUS PhpTailQueryObjectHack(
    __out_opt PULONG ReturnLength
    )
{
    ULONG waitResult;

    PhQueryObjectContext.Initialized = TRUE;
    // Allow the worker thread to start.
    SetEvent(PhQueryObjectStartEvent);
    // Wait for the work to complete, with a timeout of 1 second.
    waitResult = WaitForSingleObject(PhQueryObjectCompletedEvent, 1000);
    PhQueryObjectContext.Initialized = FALSE;

    // Return normally if the work was completed.
    if (waitResult == WAIT_OBJECT_0)
    {
        NTSTATUS status;
        ULONG returnLength;

        status = PhQueryObjectContext.Status;
        returnLength = PhQueryObjectContext.ReturnLength;

        PhReleaseMutex(&PhQueryObjectMutex);

        if (ReturnLength)
            *ReturnLength = returnLength;

        return status;
    }
    // Kill the worker thread if it took too long.
    // else if (waitResult == WAIT_TIMEOUT)
    else
    {
        // Kill the thread.
        if (TerminateThread(PhQueryObjectThreadHandle, 1))
        {
            PhQueryObjectThreadHandle = NULL;

            // Delete the fiber (and free the thread stack).
            DeleteFiber(PhQueryObjectFiber);
            PhQueryObjectFiber = NULL;
        }

        PhReleaseMutex(&PhQueryObjectMutex);

        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS PhQueryObjectNameHack(
    __in HANDLE Handle,
    __out_bcount(ObjectNameInformationLength) POBJECT_NAME_INFORMATION ObjectNameInformation,
    __in ULONG ObjectNameInformationLength,
    __out_opt PULONG ReturnLength
    )
{
    if (!PhpHeadQueryObjectHack())
        return STATUS_UNSUCCESSFUL;

    PhQueryObjectContext.Work = QueryNameHack;
    PhQueryObjectContext.Handle = Handle;
    PhQueryObjectContext.Buffer = ObjectNameInformation;
    PhQueryObjectContext.Length = ObjectNameInformationLength;

    return PhpTailQueryObjectHack(ReturnLength);
}

NTSTATUS PhQueryObjectSecurityHack(
    __in HANDLE Handle,
    __in SECURITY_INFORMATION SecurityInformation,
    __out_bcount(Length) PVOID Buffer,
    __in ULONG Length,
    __out_opt PULONG ReturnLength
    )
{
    if (!PhpHeadQueryObjectHack())
        return STATUS_UNSUCCESSFUL;

    PhQueryObjectContext.Work = QuerySecurityHack;
    PhQueryObjectContext.Handle = Handle;
    PhQueryObjectContext.SecurityInformation = SecurityInformation;
    PhQueryObjectContext.Buffer = Buffer;
    PhQueryObjectContext.Length = Length;

    return PhpTailQueryObjectHack(ReturnLength);
}

NTSTATUS PhSetObjectSecurityHack(
    __in HANDLE Handle,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PVOID Buffer
    )
{
    if (!PhpHeadQueryObjectHack())
        return STATUS_UNSUCCESSFUL;

    PhQueryObjectContext.Work = SetSecurityHack;
    PhQueryObjectContext.Handle = Handle;
    PhQueryObjectContext.SecurityInformation = SecurityInformation;
    PhQueryObjectContext.Buffer = Buffer;

    return PhpTailQueryObjectHack(NULL);
}

NTSTATUS PhpQueryObjectThreadStart(
    __in PVOID Parameter
    )
{
    PhQueryObjectFiber = ConvertThreadToFiber(Parameter);

    while (TRUE)
    {
        // Wait for work.
        if (WaitForSingleObject(PhQueryObjectStartEvent, INFINITE) != WAIT_OBJECT_0)
            continue;

        // Make sure we actually have work.
        if (PhQueryObjectContext.Initialized)
        {
            switch (PhQueryObjectContext.Work)
            {
            case QueryNameHack:
                PhQueryObjectContext.Status = NtQueryObject(
                    PhQueryObjectContext.Handle,
                    ObjectNameInformation,
                    PhQueryObjectContext.Buffer,
                    PhQueryObjectContext.Length,
                    &PhQueryObjectContext.ReturnLength
                    );
                break;
            case QuerySecurityHack:
                PhQueryObjectContext.Status = NtQuerySecurityObject(
                    PhQueryObjectContext.Handle,
                    PhQueryObjectContext.SecurityInformation,
                    (PSECURITY_DESCRIPTOR)PhQueryObjectContext.Buffer,
                    PhQueryObjectContext.Length,
                    &PhQueryObjectContext.ReturnLength
                    );
                break;
            case SetSecurityHack:
                PhQueryObjectContext.Status = NtSetSecurityObject(
                    PhQueryObjectContext.Handle,
                    PhQueryObjectContext.SecurityInformation,
                    (PSECURITY_DESCRIPTOR)PhQueryObjectContext.Buffer
                    );
                break;
            }

            // Work done.
            SetEvent(PhQueryObjectCompletedEvent);
        }
    }

    return STATUS_SUCCESS;
}
