/*
 * Process Hacker - 
 *   handle information
 * 
 * Copyright (C) 2010-2011 wj32
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
#include <kphuser.h>

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

static HANDLE PhQueryObjectThreadHandle = NULL;
static PVOID PhQueryObjectFiber = NULL;
static PH_QUEUED_LOCK PhQueryObjectMutex;
static HANDLE PhQueryObjectStartEvent = NULL;
static HANDLE PhQueryObjectCompletedEvent = NULL;
static PH_QUERY_OBJECT_CONTEXT PhQueryObjectContext;

static PPH_STRING PhObjectTypeNames[MAX_OBJECT_TYPE_NUMBER] = { 0 };
static PPH_GET_CLIENT_ID_NAME PhHandleGetClientIdName = NULL;

VOID PhHandleInfoInitialization()
{
    PhHandleGetClientIdName = PhStdGetClientIdName;
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
        status = KphQueryInformationObject(
            PhKphHandle,
            ProcessHandle,
            Handle,
            KphObjectBasicInformation,
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

    if (ObjectTypeNumber != -1 && ObjectTypeNumber < MAX_OBJECT_TYPE_NUMBER)
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
            status = KphQueryInformationObject(
                PhKphHandle,
                ProcessHandle,
                Handle,
                KphObjectTypeInformation,
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
            status = KphQueryInformationObject(
                PhKphHandle,
                ProcessHandle,
                Handle,
                KphObjectTypeInformation,
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

        if (ObjectTypeNumber != -1 && ObjectTypeNumber < MAX_OBJECT_TYPE_NUMBER)
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
    ULONG bufferSize;
    ULONG attempts = 8;

    bufferSize = 0x200;
    buffer = PhAllocate(bufferSize);

    // A loop is needed because the I/O subsystem likes to give us the wrong return lengths...
    do
    {
        if (PhKphHandle)
        {
            status = KphQueryInformationObject(
                PhKphHandle,
                ProcessHandle,
                Handle,
                KphObjectNameInformation,
                buffer,
                bufferSize,
                &bufferSize
                );
        }
        else
        {
            status = NtQueryObject(
                Handle,
                ObjectNameInformation,
                buffer,
                bufferSize,
                &bufferSize
                );
        }

        if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_INFO_LENGTH_MISMATCH ||
            status == STATUS_BUFFER_TOO_SMALL)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    } while (--attempts);

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
    static PH_STRINGREF hklmPrefix = PH_STRINGREF_INIT(L"\\Registry\\Machine");
    static PH_STRINGREF hkcrPrefix = PH_STRINGREF_INIT(L"\\Registry\\Machine\\Software\\Classes");
    static PH_STRINGREF hkuPrefix = PH_STRINGREF_INIT(L"\\Registry\\User");
    static PPH_STRING hkcuPrefix;
    static PPH_STRING hkcucrPrefix;

    static PH_STRINGREF hklmString = PH_STRINGREF_INIT(L"HKLM");
    static PH_STRINGREF hkcrString = PH_STRINGREF_INIT(L"HKCR");
    static PH_STRINGREF hkuString = PH_STRINGREF_INIT(L"HKU");
    static PH_STRINGREF hkcuString = PH_STRINGREF_INIT(L"HKCU");
    static PH_STRINGREF hkcucrString = PH_STRINGREF_INIT(L"HKCU\\Software\\Classes");

    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    PPH_STRING newName;
    PH_STRINGREF name;

    if (PhBeginInitOnce(&initOnce))
    {
        PTOKEN_USER tokenUser;
        PPH_STRING stringSid = NULL;

        if (PhCurrentTokenQueryHandle)
        {
            if (NT_SUCCESS(PhGetTokenUser(
                PhCurrentTokenQueryHandle,
                &tokenUser
                )))
            {
                stringSid = PhSidToStringSid(tokenUser->User.Sid);
                PhFree(tokenUser);
            }
        }

        if (stringSid)
        {
            static PH_STRINGREF registryUserPrefix = PH_STRINGREF_INIT(L"\\Registry\\User\\");
            static PH_STRINGREF classesString = PH_STRINGREF_INIT(L"_Classes");

            hkcuPrefix = PhConcatStringRef2(&registryUserPrefix, &stringSid->sr);
            hkcucrPrefix = PhConcatStringRef2(&hkcuPrefix->sr, &classesString);

            PhDereferenceObject(stringSid);
        }
        else
        {
            hkcuPrefix = PhCreateString(L"..."); // some random string that won't ever get matched
            hkcucrPrefix = PhCreateString(L"...");
        }

        PhEndInitOnce(&initOnce);
    }

    name = Name->sr;

    if (PhStartsWithStringRef(&name, &hkcrPrefix, TRUE))
    {
        name.Buffer += hkcrPrefix.Length / sizeof(WCHAR);
        name.Length -= hkcrPrefix.Length;
        newName = PhConcatStringRef2(&hkcrString, &name);
    }
    else if (PhStartsWithStringRef(&name, &hklmPrefix, TRUE))
    {
        name.Buffer += hklmPrefix.Length / sizeof(WCHAR);
        name.Length -= hklmPrefix.Length;
        newName = PhConcatStringRef2(&hklmString, &name);
    }
    else if (PhStartsWithStringRef(&name, &hkcucrPrefix->sr, TRUE))
    {
        name.Buffer += hkcucrPrefix->Length / sizeof(WCHAR);
        name.Length -= hkcucrPrefix->Length;
        newName = PhConcatStringRef2(&hkcucrString, &name);
    }
    else if (PhStartsWithStringRef(&name, &hkcuPrefix->sr, TRUE))
    {
        name.Buffer += hkcuPrefix->Length / sizeof(WCHAR);
        name.Length -= hkcuPrefix->Length;
        newName = PhConcatStringRef2(&hkcuString, &name);
    }
    else if (PhStartsWithStringRef(&name, &hkuPrefix, TRUE))
    {
        name.Buffer += hkuPrefix.Length / sizeof(WCHAR);
        name.Length -= hkuPrefix.Length;
        newName = PhConcatStringRef2(&hkuString, &name);
    }
    else
    {
        newName = Name;
        PhReferenceObject(Name);
    }

    return newName;
}

__callback PPH_STRING PhStdGetClientIdName(
    __in PCLIENT_ID ClientId
    )
{
    static PH_QUEUED_LOCK cachedProcessesLock = PH_QUEUED_LOCK_INIT;
    static PVOID processes = NULL;
    static ULONG lastProcessesTickCount = 0;

    PPH_STRING name;
    ULONG tickCount;
    PSYSTEM_PROCESS_INFORMATION processInfo;

    // Get a new process list only if 2 seconds have passed 
    // since the last update.

    tickCount = GetTickCount();

    if (tickCount - lastProcessesTickCount >= 2000)
    {
        PhAcquireQueuedLockExclusive(&cachedProcessesLock);

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

        PhReleaseQueuedLockExclusive(&cachedProcessesLock);
    }

    // Get a lock on the process list and get a name for the client ID.

    PhAcquireQueuedLockShared(&cachedProcessesLock);

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

    PhReleaseQueuedLockShared(&cachedProcessesLock);

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
    PPH_GET_CLIENT_ID_NAME handleGetClientIdName;

    if (PhEqualString2(TypeName, L"EtwRegistration", TRUE))
    {
        if (PhKphHandle)
        {
            ETWREG_BASIC_INFORMATION basicInfo;

            status = KphQueryInformationObject(
                PhKphHandle,
                ProcessHandle,
                Handle,
                KphObjectEtwRegBasicInformation,
                &basicInfo,
                sizeof(ETWREG_BASIC_INFORMATION),
                NULL
                );

            if (NT_SUCCESS(status))
            {
                static PH_STRINGREF publishersKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\WINEVT\\Publishers\\");

                PPH_STRING guidString;
                PPH_STRING keyName;
                HANDLE keyHandle;
                PPH_STRING publisherName = NULL;

                guidString = PhFormatGuid(&basicInfo.Guid);

                // We should perform a lookup on the GUID to get the publisher name.

                keyName = PhConcatStringRef2(&publishersKeyName, &guidString->sr);

                if (NT_SUCCESS(PhOpenKey(
                    &keyHandle,
                    KEY_READ,
                    PH_KEY_LOCAL_MACHINE,
                    &keyName->sr,
                    0
                    )))
                {
                    publisherName = PhQueryRegistryString(keyHandle, NULL);

                    if (publisherName && publisherName->Length == 0)
                    {
                        PhDereferenceObject(publisherName);
                        publisherName = NULL;
                    }

                    NtClose(keyHandle);
                }

                PhDereferenceObject(keyName);

                if (publisherName)
                {
                    bestObjectName = publisherName;
                    PhDereferenceObject(guidString);
                }
                else
                {
                    bestObjectName = guidString;
                }
            }
        }
    }
    else if (PhEqualString2(TypeName, L"File", TRUE))
    {
        // Convert the file name to a DOS file name.
        bestObjectName = PhResolveDevicePrefix(ObjectName);

        if (!bestObjectName)
        {
            bestObjectName = ObjectName;
            PhReferenceObject(ObjectName);
        }
    }
    else if (PhEqualString2(TypeName, L"Key", TRUE))
    {
        bestObjectName = PhFormatNativeKeyName(ObjectName);
    }
    else if (PhEqualString2(TypeName, L"Process", TRUE))
    {
        CLIENT_ID clientId;

        clientId.UniqueThread = NULL;

        if (PhKphHandle)
        {
            PROCESS_BASIC_INFORMATION basicInfo;

            status = KphQueryInformationObject(
                PhKphHandle,
                ProcessHandle,
                Handle,
                KphObjectProcessBasicInformation,
                &basicInfo,
                sizeof(PROCESS_BASIC_INFORMATION),
                NULL
                );

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            clientId.UniqueProcess = basicInfo.UniqueProcessId;
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

        handleGetClientIdName = PhHandleGetClientIdName;

        if (handleGetClientIdName)
            bestObjectName = handleGetClientIdName(&clientId);
    }
    else if (PhEqualString2(TypeName, L"Thread", TRUE))
    {
        CLIENT_ID clientId;

        if (PhKphHandle)
        {
            THREAD_BASIC_INFORMATION basicInfo;

            status = KphQueryInformationObject(
                PhKphHandle,
                ProcessHandle,
                Handle,
                KphObjectThreadBasicInformation,
                &basicInfo,
                sizeof(THREAD_BASIC_INFORMATION),
                NULL
                );

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            clientId = basicInfo.ClientId;
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

        handleGetClientIdName = PhHandleGetClientIdName;

        if (handleGetClientIdName)
            bestObjectName = handleGetClientIdName(&clientId);
    }
    else if (PhEqualString2(TypeName, L"TmEn", TRUE))
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
    else if (PhEqualString2(TypeName, L"TmRm", TRUE))
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
            if (!PhIsNullOrEmptyString(description))
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
    else if (PhEqualString2(TypeName, L"TmTm", TRUE))
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

        if (NT_SUCCESS(status) && !PhIsNullOrEmptyString(logFileName))
        {
            bestObjectName = PhGetFileName(logFileName);
            PhDereferenceObject(logFileName);
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
    else if (PhEqualString2(TypeName, L"TmTx", TRUE))
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

        if (NT_SUCCESS(status) && !PhIsNullOrEmptyString(description))
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
    else if (PhEqualString2(TypeName, L"Token", TRUE))
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
                PH_FORMAT format[3];

                PhInitFormatSR(&format[0], fullName->sr);
                PhInitFormatS(&format[1], L": 0x");
                PhInitFormatX(&format[2], statistics.AuthenticationId.LowPart);

                bestObjectName = PhFormat(format, 3, fullName->Length + 8 + 16);
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
    NTSTATUS subStatus;

    status = PhGetHandleInformationEx(
        ProcessHandle,
        Handle,
        ObjectTypeNumber,
        0,
        &subStatus,
        BasicInformation,
        TypeName,
        ObjectName,
        BestObjectName,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // Fail if any component failed, for compatibility reasons.
    if (!NT_SUCCESS(subStatus))
        return subStatus;

    return status;
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
 * \param Flags Reserved.
 * \param SubStatus A variable which receives the NTSTATUS value of 
 * the last component that fails. If all operations succeed, the 
 * value will be STATUS_SUCCESS. If the function returns an error 
 * status, this variable is not set.
 * \param BasicInformation A variable which receives basic 
 * information about the object.
 * \param TypeName A variable which receives the object type name.
 * \param ObjectName A variable which receives the object name.
 * \param BestObjectName A variable which receives the formatted 
 * object name.
 * \param ExtraInformation Reserved.
 *
 * \retval STATUS_INVALID_HANDLE The handle specified in
 * \c ProcessHandle or \c Handle is invalid.
 * \retval STATUS_INVALID_PARAMETER_3 The value specified in 
 * \c ObjectTypeNumber is invalid.
 *
 * \remarks If \a BasicInformation or \a TypeName are specified, 
 * the function will fail if either cannot be queried. \a ObjectName, 
 * \a BestObjectName and \a ExtraInformation will return NULL if they 
 * cannot be queried.
 */
NTSTATUS PhGetHandleInformationEx(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in ULONG ObjectTypeNumber,
    __reserved ULONG Flags,
    __out_opt PNTSTATUS SubStatus,
    __out_opt POBJECT_BASIC_INFORMATION BasicInformation,
    __out_opt PPH_STRING *TypeName,
    __out_opt PPH_STRING *ObjectName,
    __out_opt PPH_STRING *BestObjectName,
    __reserved PVOID *ExtraInformation
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS subStatus = STATUS_SUCCESS;
    HANDLE dupHandle = NULL;
    PPH_STRING typeName = NULL;
    PPH_STRING objectName = NULL;
    PPH_STRING bestObjectName = NULL;

    if (Handle == NULL || Handle == NtCurrentProcess() || Handle == NtCurrentThread())
        return STATUS_INVALID_HANDLE;
    if (ObjectTypeNumber != -1 && ObjectTypeNumber >= MAX_OBJECT_TYPE_NUMBER)
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
    if (PhEqualString2(typeName, L"File", TRUE) && !PhKphHandle)
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
            // Pretend the file object has no name.
            objectName = PhReferenceEmptyString();
            status = STATUS_SUCCESS;
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
    {
        subStatus = status;
        status = STATUS_SUCCESS;
        goto CleanupExit;
    }

    // Exit early if we don't need to get the best object name.
    if (!BestObjectName)
        goto CleanupExit;

    status = PhpGetBestObjectName(
        ProcessHandle,
        Handle,
        objectName,
        typeName,
        &bestObjectName
        );

    if (!NT_SUCCESS(status))
    {
        subStatus = status;
        status = STATUS_SUCCESS;
        goto CleanupExit;
    }

CleanupExit:

    if (NT_SUCCESS(status))
    {
        if (SubStatus)
        {
            *SubStatus = subStatus;
        }

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
    PhAcquireQueuedLockExclusive(&PhQueryObjectMutex);

    // Create a query thread if we don't have one.
    if (!PhQueryObjectThreadHandle)
    {
        PhQueryObjectThreadHandle = CreateThread(NULL, 0, PhpQueryObjectThreadStart, NULL, 0, NULL);

        if (!PhQueryObjectThreadHandle)
        {
            PhReleaseQueuedLockExclusive(&PhQueryObjectMutex);
            return FALSE;
        }
    }

    // Create the events if they don't exist.

    if (!PhQueryObjectStartEvent)
    {
        if (!NT_SUCCESS(NtCreateEvent(
            &PhQueryObjectStartEvent,
            EVENT_ALL_ACCESS,
            NULL,
            SynchronizationEvent,
            FALSE
            )))
        {
            PhReleaseQueuedLockExclusive(&PhQueryObjectMutex);
            return FALSE;
        }
    }

    if (!PhQueryObjectCompletedEvent)
    {
        if (!NT_SUCCESS(NtCreateEvent(
            &PhQueryObjectCompletedEvent,
            EVENT_ALL_ACCESS,
            NULL,
            SynchronizationEvent,
            FALSE
            )))
        {
            PhReleaseQueuedLockExclusive(&PhQueryObjectMutex);
            return FALSE;
        }
    }

    return TRUE;
}

NTSTATUS PhpTailQueryObjectHack(
    __out_opt PULONG ReturnLength
    )
{
    NTSTATUS status;
    LARGE_INTEGER timeout;

    PhQueryObjectContext.Initialized = TRUE;

    // Allow the worker thread to start.
    NtSetEvent(PhQueryObjectStartEvent, NULL);
    // Wait for the work to complete, with a timeout of 1 second.
    timeout.QuadPart = -1000 * PH_TIMEOUT_MS;
    status = NtWaitForSingleObject(PhQueryObjectCompletedEvent, FALSE, &timeout);

    PhQueryObjectContext.Initialized = FALSE;

    // Return normally if the work was completed.
    if (status == STATUS_WAIT_0)
    {
        ULONG returnLength;

        status = PhQueryObjectContext.Status;
        returnLength = PhQueryObjectContext.ReturnLength;

        PhReleaseQueuedLockExclusive(&PhQueryObjectMutex);

        if (ReturnLength)
            *ReturnLength = returnLength;

        return status;
    }
    // Kill the worker thread if it took too long.
    // else if (status == STATUS_TIMEOUT)
    else
    {
        // Kill the thread.
        if (NT_SUCCESS(NtTerminateThread(PhQueryObjectThreadHandle, STATUS_TIMEOUT)))
        {
            PhQueryObjectThreadHandle = NULL;

            // Delete the fiber (and free the thread stack).
            DeleteFiber(PhQueryObjectFiber);
            PhQueryObjectFiber = NULL;
        }

        PhReleaseQueuedLockExclusive(&PhQueryObjectMutex);

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
        if (NtWaitForSingleObject(PhQueryObjectStartEvent, FALSE, NULL) != STATUS_WAIT_0)
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
            NtSetEvent(PhQueryObjectCompletedEvent, NULL);
        }
    }

    return STATUS_SUCCESS;
}
