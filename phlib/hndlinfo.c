/*
 * Process Hacker -
 *   handle information
 *
 * Copyright (C) 2010-2015 wj32
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

#define PH_QUERY_HACK_MAX_THREADS 20

typedef struct _PHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT
{
    SLIST_ENTRY ListEntry;

    PUSER_THREAD_START_ROUTINE Routine;
    PVOID Context;

    HANDLE StartEventHandle;
    HANDLE CompletedEventHandle;
    HANDLE ThreadHandle;
} PHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT, *PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT;

typedef enum _PHP_QUERY_OBJECT_WORK
{
    NtQueryObjectWork,
    NtQuerySecurityObjectWork,
    NtSetSecurityObjectWork,
    KphDuplicateObjectWork
} PHP_QUERY_OBJECT_WORK;

typedef struct _PHP_QUERY_OBJECT_COMMON_CONTEXT
{
    PHP_QUERY_OBJECT_WORK Work;
    NTSTATUS Status;

    union
    {
        struct
        {
            HANDLE Handle;
            OBJECT_INFORMATION_CLASS ObjectInformationClass;
            PVOID ObjectInformation;
            ULONG ObjectInformationLength;
            PULONG ReturnLength;
        } NtQueryObject;
        struct
        {
            HANDLE Handle;
            SECURITY_INFORMATION SecurityInformation;
            PSECURITY_DESCRIPTOR SecurityDescriptor;
            ULONG Length;
            PULONG LengthNeeded;
        } NtQuerySecurityObject;
        struct
        {
            HANDLE Handle;
            SECURITY_INFORMATION SecurityInformation;
            PSECURITY_DESCRIPTOR SecurityDescriptor;
        } NtSetSecurityObject;
        struct
        {
            HANDLE SourceProcessHandle;
            HANDLE SourceHandle;
            HANDLE TargetProcessHandle;
            PHANDLE TargetHandle;
            ACCESS_MASK DesiredAccess;
            ULONG HandleAttributes;
            ULONG Options;
        } KphDuplicateObject;
    } u;
} PHP_QUERY_OBJECT_COMMON_CONTEXT, *PPHP_QUERY_OBJECT_COMMON_CONTEXT;

PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT PhpAcquireCallWithTimeoutThread(
    _In_opt_ PLARGE_INTEGER Timeout
    );

VOID PhpReleaseCallWithTimeoutThread(
    _Inout_ PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT ThreadContext
    );

NTSTATUS PhpCallWithTimeout(
    _Inout_ PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT ThreadContext,
    _In_ PUSER_THREAD_START_ROUTINE Routine,
    _In_opt_ PVOID Context,
    _In_ PLARGE_INTEGER Timeout
    );

NTSTATUS PhpCallWithTimeoutThreadStart(
    _In_ PVOID Parameter
    );

static PPH_STRING PhObjectTypeNames[MAX_OBJECT_TYPE_NUMBER] = { 0 };
static PPH_GET_CLIENT_ID_NAME PhHandleGetClientIdName = PhStdGetClientIdName;

static SLIST_HEADER PhpCallWithTimeoutThreadListHead;
static PH_QUEUED_LOCK PhpCallWithTimeoutThreadReleaseEvent = PH_QUEUED_LOCK_INIT;

PPH_GET_CLIENT_ID_NAME PhSetHandleClientIdFunction(
    _In_ PPH_GET_CLIENT_ID_NAME GetClientIdName
    )
{
    return _InterlockedExchangePointer(
        (PVOID *)&PhHandleGetClientIdName,
        GetClientIdName
        );
}

NTSTATUS PhpGetObjectBasicInformation(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _Out_ POBJECT_BASIC_INFORMATION BasicInformation
    )
{
    NTSTATUS status;

    if (KphIsConnected())
    {
        status = KphQueryInformationObject(
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
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ ULONG ObjectTypeNumber,
    _Out_ PPH_STRING *TypeName
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
        if (KphIsConnected())
        {
            status = KphQueryInformationObject(
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

        if (KphIsConnected())
        {
            status = KphQueryInformationObject(
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
        typeName = PhCreateStringFromUnicodeString(&buffer->TypeName);

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
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ BOOLEAN WithTimeout,
    _Out_ PPH_STRING *ObjectName
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
        if (KphIsConnected())
        {
            status = KphQueryInformationObject(
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
            if (WithTimeout)
            {
                status = PhCallNtQueryObjectWithTimeout(
                    Handle,
                    ObjectNameInformation,
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
        *ObjectName = PhCreateStringFromUnicodeString(&buffer->Name);
    }

    PhFree(buffer);

    return status;
}

PPH_STRING PhFormatNativeKeyName(
    _In_ PPH_STRING Name
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
        PhSkipStringRef(&name, hkcrPrefix.Length);
        newName = PhConcatStringRef2(&hkcrString, &name);
    }
    else if (PhStartsWithStringRef(&name, &hklmPrefix, TRUE))
    {
        PhSkipStringRef(&name, hklmPrefix.Length);
        newName = PhConcatStringRef2(&hklmString, &name);
    }
    else if (PhStartsWithStringRef(&name, &hkcucrPrefix->sr, TRUE))
    {
        PhSkipStringRef(&name, hkcucrPrefix->Length);
        newName = PhConcatStringRef2(&hkcucrString, &name);
    }
    else if (PhStartsWithStringRef(&name, &hkcuPrefix->sr, TRUE))
    {
        PhSkipStringRef(&name, hkcuPrefix->Length);
        newName = PhConcatStringRef2(&hkcuString, &name);
    }
    else if (PhStartsWithStringRef(&name, &hkuPrefix, TRUE))
    {
        PhSkipStringRef(&name, hkuPrefix.Length);
        newName = PhConcatStringRef2(&hkuString, &name);
    }
    else
    {
        PhSetReference(&newName, Name);
    }

    return newName;
}

NTSTATUS PhGetSectionFileName(
    _In_ HANDLE SectionHandle,
    _Out_ PPH_STRING *FileName
    )
{
    NTSTATUS status;
    SIZE_T viewSize;
    PVOID viewBase;

    viewSize = 1;
    viewBase = NULL;

    status = NtMapViewOfSection(
        SectionHandle,
        NtCurrentProcess(),
        &viewBase,
        0,
        0,
        NULL,
        &viewSize,
        ViewShare,
        0,
        PAGE_READONLY
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcessMappedFileName(NtCurrentProcess(), viewBase, FileName);
    NtUnmapViewOfSection(NtCurrentProcess(), viewBase);

    return status;
}

_Callback_ PPH_STRING PhStdGetClientIdName(
    _In_ PCLIENT_ID ClientId
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
                HandleToUlong(ClientId->UniqueProcess),
                HandleToUlong(ClientId->UniqueThread)
                );
        }
        else
        {
            name = PhFormatString(
                L"Non-existent process (%u): %u",
                HandleToUlong(ClientId->UniqueProcess), 
                HandleToUlong(ClientId->UniqueThread)
                );
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
                HandleToUlong(ClientId->UniqueProcess)
                );
        }
        else
        {
            name = PhFormatString(L"Non-existent process (%u)", HandleToUlong(ClientId->UniqueProcess));
        }
    }

    PhReleaseQueuedLockShared(&cachedProcessesLock);

    return name;
}

NTSTATUS PhpGetBestObjectName(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ PPH_STRING ObjectName,
    _In_ PPH_STRING TypeName,
    _Out_ PPH_STRING *BestObjectName
    )
{
    NTSTATUS status;
    PPH_STRING bestObjectName = NULL;
    PPH_GET_CLIENT_ID_NAME handleGetClientIdName = PhHandleGetClientIdName;

    if (PhEqualString2(TypeName, L"EtwRegistration", TRUE))
    {
        if (KphIsConnected())
        {
            ETWREG_BASIC_INFORMATION basicInfo;

            status = KphQueryInformationObject(
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
            // The file doesn't have a DOS name.
            PhSetReference(&bestObjectName, ObjectName);
        }

        if (PhIsNullOrEmptyString(bestObjectName) && KphIsConnected())
        {
            KPH_FILE_OBJECT_DRIVER fileObjectDriver;
            PPH_STRING driverName;

            status = KphQueryInformationObject(
                ProcessHandle,
                Handle,
                KphObjectFileObjectDriver,
                &fileObjectDriver,
                sizeof(KPH_FILE_OBJECT_DRIVER),
                NULL
                );

            if (NT_SUCCESS(status) && fileObjectDriver.DriverHandle)
            {
                if (NT_SUCCESS(PhGetDriverName(fileObjectDriver.DriverHandle, &driverName)))
                {
                    static PH_STRINGREF prefix = PH_STRINGREF_INIT(L"Unnamed file: ");

                    PhMoveReference(&bestObjectName, PhConcatStringRef2(&prefix, &driverName->sr));
                    PhDereferenceObject(driverName);
                }

                NtClose(fileObjectDriver.DriverHandle);
            }
        }
    }
    else if (PhEqualString2(TypeName, L"Job", TRUE))
    {
        HANDLE dupHandle;
        PJOBOBJECT_BASIC_PROCESS_ID_LIST processIdList;

        status = NtDuplicateObject(
            ProcessHandle,
            Handle,
            NtCurrentProcess(),
            &dupHandle,
            JOB_OBJECT_QUERY,
            0,
            0
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        if (handleGetClientIdName && NT_SUCCESS(PhGetJobProcessIdList(dupHandle, &processIdList)))
        {
            PH_STRING_BUILDER sb;
            ULONG i;
            CLIENT_ID clientId;
            PPH_STRING name;

            PhInitializeStringBuilder(&sb, 40);
            clientId.UniqueThread = NULL;

            for (i = 0; i < processIdList->NumberOfProcessIdsInList; i++)
            {
                clientId.UniqueProcess = (HANDLE)processIdList->ProcessIdList[i];
                name = handleGetClientIdName(&clientId);

                if (name)
                {
                    PhAppendStringBuilder(&sb, &name->sr);
                    PhAppendStringBuilder2(&sb, L"; ");
                    PhDereferenceObject(name);
                }
            }

            PhFree(processIdList);

            if (sb.String->Length != 0)
                PhRemoveEndStringBuilder(&sb, 2);

            if (sb.String->Length == 0)
                PhAppendStringBuilder2(&sb, L"(No processes)");

            bestObjectName = PhFinalStringBuilderString(&sb);
        }

        NtClose(dupHandle);
    }
    else if (PhEqualString2(TypeName, L"Key", TRUE))
    {
        bestObjectName = PhFormatNativeKeyName(ObjectName);
    }
    else if (PhEqualString2(TypeName, L"Process", TRUE))
    {
        CLIENT_ID clientId;

        clientId.UniqueThread = NULL;

        if (KphIsConnected())
        {
            PROCESS_BASIC_INFORMATION basicInfo;

            status = KphQueryInformationObject(
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

        if (handleGetClientIdName)
            bestObjectName = handleGetClientIdName(&clientId);
    }
    else if (PhEqualString2(TypeName, L"Section", TRUE))
    {
        HANDLE dupHandle;
        PPH_STRING fileName;

        if (!PhIsNullOrEmptyString(ObjectName))
            goto CleanupExit;

        status = NtDuplicateObject(
            ProcessHandle,
            Handle,
            NtCurrentProcess(),
            &dupHandle,
            SECTION_QUERY | SECTION_MAP_READ,
            0,
            0
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhGetSectionFileName(dupHandle, &fileName);

        if (NT_SUCCESS(status))
        {
            bestObjectName = PhResolveDevicePrefix(fileName);
            PhDereferenceObject(fileName);
        }
        else
        {
            SECTION_BASIC_INFORMATION basicInfo;

            if (NT_SUCCESS(PhGetSectionBasicInformation(dupHandle, &basicInfo)))
            {
                PH_FORMAT format[4];
                PWSTR sectionType = L"Unknown";

                if (basicInfo.AllocationAttributes & SEC_COMMIT)
                    sectionType = L"Commit";
                else if (basicInfo.AllocationAttributes & SEC_FILE)
                    sectionType = L"File";
                else if (basicInfo.AllocationAttributes & SEC_IMAGE)
                    sectionType = L"Image";
                else if (basicInfo.AllocationAttributes & SEC_RESERVE)
                    sectionType = L"Reserve";

                PhInitFormatS(&format[0], sectionType);
                PhInitFormatS(&format[1], L" (");
                PhInitFormatSize(&format[2], basicInfo.MaximumSize.QuadPart);
                PhInitFormatC(&format[3], ')');
                bestObjectName = PhFormat(format, 4, 20);
            }
        }

        NtClose(dupHandle);
    }
    else if (PhEqualString2(TypeName, L"Thread", TRUE))
    {
        CLIENT_ID clientId;

        if (KphIsConnected())
        {
            THREAD_BASIC_INFORMATION basicInfo;

            status = KphQueryInformationObject(
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
                PH_FORMAT format[4];

                PhInitFormatSR(&format[0], fullName->sr);
                PhInitFormatS(&format[1], L": 0x");
                PhInitFormatX(&format[2], statistics.AuthenticationId.LowPart);
                PhInitFormatS(&format[3], statistics.TokenType == TokenPrimary ? L" (Primary)" : L" (Impersonation)");

                bestObjectName = PhFormat(format, 4, fullName->Length + 8 + 16 + 16);
                PhDereferenceObject(fullName);
            }

            PhFree(tokenUser);
        }

        NtClose(dupHandle);
    }

CleanupExit:

    if (!bestObjectName)
        PhSetReference(&bestObjectName, ObjectName);

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
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ ULONG ObjectTypeNumber,
    _Out_opt_ POBJECT_BASIC_INFORMATION BasicInformation,
    _Out_opt_ PPH_STRING *TypeName,
    _Out_opt_ PPH_STRING *ObjectName,
    _Out_opt_ PPH_STRING *BestObjectName
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
    {
        if (TypeName)
            PhClearReference(TypeName);
        if (ObjectName)
            PhClearReference(ObjectName);
        if (BestObjectName)
            PhClearReference(BestObjectName);

        return subStatus;
    }

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
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ ULONG ObjectTypeNumber,
    _Reserved_ ULONG Flags,
    _Out_opt_ PNTSTATUS SubStatus,
    _Out_opt_ POBJECT_BASIC_INFORMATION BasicInformation,
    _Out_opt_ PPH_STRING *TypeName,
    _Out_opt_ PPH_STRING *ObjectName,
    _Out_opt_ PPH_STRING *BestObjectName,
    _Reserved_ PVOID *ExtraInformation
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
    if (!KphIsConnected())
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
            KphIsConnected() ? Handle : dupHandle,
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
        KphIsConnected() ? Handle : dupHandle,
        ObjectTypeNumber,
        &typeName
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Exit early if we don't need to get the object name.
    if (!ObjectName && !BestObjectName)
        goto CleanupExit;

    // Get the object name.
    // If we're dealing with a file handle we must take special precautions so we don't hang.
    if (PhEqualString2(typeName, L"File", TRUE) && !KphIsConnected())
    {
#define QUERY_NORMALLY 0
#define QUERY_WITH_TIMEOUT 1
#define QUERY_FAIL 2

        ULONG hackLevel = QUERY_WITH_TIMEOUT;

        // We can't use the timeout method on XP because hanging threads can't even be terminated!
        if (WindowsVersion <= WINDOWS_XP)
            hackLevel = QUERY_FAIL;

        if (hackLevel == QUERY_NORMALLY || hackLevel == QUERY_WITH_TIMEOUT)
        {
            status = PhpGetObjectName(
                ProcessHandle,
                KphIsConnected() ? Handle : dupHandle,
                hackLevel == QUERY_WITH_TIMEOUT,
                &objectName
                );
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
            KphIsConnected() ? Handle : dupHandle,
            FALSE,
            &objectName
            );
    }

    if (!NT_SUCCESS(status))
    {
        if (PhEqualString2(typeName, L"File", TRUE) && KphIsConnected())
        {
            // PhpGetBestObjectName can provide us with a name.
            objectName = PhReferenceEmptyString();
            status = STATUS_SUCCESS;
        }
        else
        {
            subStatus = status;
            status = STATUS_SUCCESS;
            goto CleanupExit;
        }
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
            *SubStatus = subStatus;
        if (TypeName)
            PhSetReference(TypeName, typeName);
        if (ObjectName)
            PhSetReference(ObjectName, objectName);
        if (BestObjectName)
            PhSetReference(BestObjectName, bestObjectName);
    }

    if (dupHandle && ProcessHandle != NtCurrentProcess())
        NtClose(dupHandle);

    PhClearReference(&typeName);
    PhClearReference(&objectName);
    PhClearReference(&bestObjectName);

    return status;
}

NTSTATUS PhEnumObjectTypes(
    _Out_ POBJECT_TYPES_INFORMATION *ObjectTypes
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;

    bufferSize = 0x1000;
    buffer = PhAllocate(bufferSize);

    while ((status = NtQueryObject(
        NULL,
        ObjectTypesInformation,
        buffer,
        bufferSize,
        NULL
        )) == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        bufferSize *= 2;

        // Fail if we're resizing the buffer to something very large.
        if (bufferSize > PH_LARGE_BUFFER_SIZE)
            return STATUS_INSUFFICIENT_RESOURCES;

        buffer = PhAllocate(bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *ObjectTypes = (POBJECT_TYPES_INFORMATION)buffer;

    return status;
}

ULONG PhGetObjectTypeNumber(
    _In_ PUNICODE_STRING TypeName
    )
{
    POBJECT_TYPES_INFORMATION objectTypes;
    POBJECT_TYPE_INFORMATION objectType;
    ULONG i;

    if (NT_SUCCESS(PhEnumObjectTypes(&objectTypes)))
    {
        objectType = PH_FIRST_OBJECT_TYPE(objectTypes);

        for (i = 0; i < objectTypes->NumberOfTypes; i++)
        {
            if (RtlEqualUnicodeString(&objectType->TypeName, TypeName, TRUE))
            {
                if (WindowsVersion >= WINDOWS_8_1)
                    return objectType->TypeIndex;
                else if (WindowsVersion >= WINDOWS_7)
                    return i + 2;
                else
                    return i + 1;
            }

            objectType = PH_NEXT_OBJECT_TYPE(objectType);
        }

        PhFree(objectTypes);
    }

    return -1;
}

PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT PhpAcquireCallWithTimeoutThread(
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT threadContext;
    PSLIST_ENTRY listEntry;
    PH_QUEUED_WAIT_BLOCK waitBlock;

    if (PhBeginInitOnce(&initOnce))
    {
        ULONG i;

        for (i = 0; i < PH_QUERY_HACK_MAX_THREADS; i++)
        {
            threadContext = PhAllocate(sizeof(PHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT));
            memset(threadContext, 0, sizeof(PHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT));
            RtlInterlockedPushEntrySList(&PhpCallWithTimeoutThreadListHead, &threadContext->ListEntry);
        }

        PhEndInitOnce(&initOnce);
    }

    while (TRUE)
    {
        if (listEntry = RtlInterlockedPopEntrySList(&PhpCallWithTimeoutThreadListHead))
            break;

        if (!Timeout || Timeout->QuadPart != 0)
        {
            PhQueueWakeEvent(&PhpCallWithTimeoutThreadReleaseEvent, &waitBlock);

            if (listEntry = RtlInterlockedPopEntrySList(&PhpCallWithTimeoutThreadListHead))
            {
                // A new entry has just become available; cancel the wait.
                PhSetWakeEvent(&PhpCallWithTimeoutThreadReleaseEvent, &waitBlock);
                break;
            }
            else
            {
                PhWaitForWakeEvent(&PhpCallWithTimeoutThreadReleaseEvent, &waitBlock, FALSE, Timeout);
            }
        }
        else
        {
            return NULL;
        }
    }

    return CONTAINING_RECORD(listEntry, PHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT, ListEntry);
}

VOID PhpReleaseCallWithTimeoutThread(
    _Inout_ PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT ThreadContext
    )
{
    RtlInterlockedPushEntrySList(&PhpCallWithTimeoutThreadListHead, &ThreadContext->ListEntry);
    PhSetWakeEvent(&PhpCallWithTimeoutThreadReleaseEvent, NULL);
}

NTSTATUS PhpCallWithTimeout(
    _Inout_ PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT ThreadContext,
    _In_ PUSER_THREAD_START_ROUTINE Routine,
    _In_opt_ PVOID Context,
    _In_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;

    // Create objects if necessary.

    if (!ThreadContext->StartEventHandle)
    {
        if (!NT_SUCCESS(status = NtCreateEvent(&ThreadContext->StartEventHandle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE)))
            return status;
    }

    if (!ThreadContext->CompletedEventHandle)
    {
        if (!NT_SUCCESS(status = NtCreateEvent(&ThreadContext->CompletedEventHandle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE)))
            return status;
    }

    // Create a query thread if we don't have one.
    if (!ThreadContext->ThreadHandle)
    {
        CLIENT_ID clientId;

        NtClearEvent(ThreadContext->StartEventHandle);
        NtClearEvent(ThreadContext->CompletedEventHandle);

        if (!NT_SUCCESS(status = RtlCreateUserThread(
            NtCurrentProcess(),
            NULL,
            FALSE,
            0,
            0,
            32 * 1024,
            PhpCallWithTimeoutThreadStart,
            ThreadContext,
            &ThreadContext->ThreadHandle,
            &clientId)))
        {
            return status;
        }

        // Wait for the thread to initialize.
        NtWaitForSingleObject(ThreadContext->CompletedEventHandle, FALSE, NULL);
    }

    ThreadContext->Routine = Routine;
    ThreadContext->Context = Context;

    NtSetEvent(ThreadContext->StartEventHandle, NULL);
    status = NtWaitForSingleObject(ThreadContext->CompletedEventHandle, FALSE, Timeout);

    ThreadContext->Routine = NULL;
    MemoryBarrier();
    ThreadContext->Context = NULL;

    if (status != STATUS_WAIT_0)
    {
        // The operation timed out, or there was an error. Kill the thread.
        // On Vista and above, the thread stack is freed automatically.
        NtTerminateThread(ThreadContext->ThreadHandle, STATUS_UNSUCCESSFUL);
        status = NtWaitForSingleObject(ThreadContext->ThreadHandle, FALSE, NULL);
        NtClose(ThreadContext->ThreadHandle);
        ThreadContext->ThreadHandle = NULL;

        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

NTSTATUS PhpCallWithTimeoutThreadStart(
    _In_ PVOID Parameter
    )
{
    PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT threadContext = Parameter;

    NtSetEvent(threadContext->CompletedEventHandle, NULL);

    while (TRUE)
    {
        if (NtWaitForSingleObject(threadContext->StartEventHandle, FALSE, NULL) != STATUS_WAIT_0)
            continue;

        if (threadContext->Routine)
            threadContext->Routine(threadContext->Context);

        NtSetEvent(threadContext->CompletedEventHandle, NULL);
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhCallWithTimeout(
    _In_ PUSER_THREAD_START_ROUTINE Routine,
    _In_opt_ PVOID Context,
    _In_opt_ PLARGE_INTEGER AcquireTimeout,
    _In_ PLARGE_INTEGER CallTimeout
    )
{
    NTSTATUS status;
    PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT threadContext;

    if (threadContext = PhpAcquireCallWithTimeoutThread(AcquireTimeout))
    {
        status = PhpCallWithTimeout(threadContext, Routine, Context, CallTimeout);
        PhpReleaseCallWithTimeoutThread(threadContext);
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

NTSTATUS PhpCommonQueryObjectRoutine(
    _In_ PVOID Parameter
    )
{
    PPHP_QUERY_OBJECT_COMMON_CONTEXT context = Parameter;

    switch (context->Work)
    {
    case NtQueryObjectWork:
        context->Status = NtQueryObject(
            context->u.NtQueryObject.Handle,
            context->u.NtQueryObject.ObjectInformationClass,
            context->u.NtQueryObject.ObjectInformation,
            context->u.NtQueryObject.ObjectInformationLength,
            context->u.NtQueryObject.ReturnLength
            );
        break;
    case NtQuerySecurityObjectWork:
        context->Status = NtQuerySecurityObject(
            context->u.NtQuerySecurityObject.Handle,
            context->u.NtQuerySecurityObject.SecurityInformation,
            context->u.NtQuerySecurityObject.SecurityDescriptor,
            context->u.NtQuerySecurityObject.Length,
            context->u.NtQuerySecurityObject.LengthNeeded
            );
        break;
    case NtSetSecurityObjectWork:
        context->Status = NtSetSecurityObject(
            context->u.NtSetSecurityObject.Handle,
            context->u.NtSetSecurityObject.SecurityInformation,
            context->u.NtSetSecurityObject.SecurityDescriptor
            );
        break;
    case KphDuplicateObjectWork:
        context->Status = KphDuplicateObject(
            context->u.KphDuplicateObject.SourceProcessHandle,
            context->u.KphDuplicateObject.SourceHandle,
            context->u.KphDuplicateObject.TargetProcessHandle,
            context->u.KphDuplicateObject.TargetHandle,
            context->u.KphDuplicateObject.DesiredAccess,
            context->u.KphDuplicateObject.HandleAttributes,
            context->u.KphDuplicateObject.Options
            );
        break;
    default:
        context->Status = STATUS_INVALID_PARAMETER;
        break;
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhpCommonQueryObjectWithTimeout(
    _In_ PPHP_QUERY_OBJECT_COMMON_CONTEXT Context
    )
{
    NTSTATUS status;
    LARGE_INTEGER timeout;

    timeout.QuadPart = -1 * PH_TIMEOUT_SEC;
    status = PhCallWithTimeout(PhpCommonQueryObjectRoutine, Context, NULL, &timeout);

    if (NT_SUCCESS(status))
        status = Context->Status;

    PhFree(Context);

    return status;
}

NTSTATUS PhCallNtQueryObjectWithTimeout(
    _In_ HANDLE Handle,
    _In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    PPHP_QUERY_OBJECT_COMMON_CONTEXT context;

    context = PhAllocate(sizeof(PHP_QUERY_OBJECT_COMMON_CONTEXT));
    context->Work = NtQueryObjectWork;
    context->Status = STATUS_UNSUCCESSFUL;
    context->u.NtQueryObject.Handle = Handle;
    context->u.NtQueryObject.ObjectInformationClass = ObjectInformationClass;
    context->u.NtQueryObject.ObjectInformation = ObjectInformation;
    context->u.NtQueryObject.ObjectInformationLength = ObjectInformationLength;
    context->u.NtQueryObject.ReturnLength = ReturnLength;

    return PhpCommonQueryObjectWithTimeout(context);
}

NTSTATUS PhCallNtQuerySecurityObjectWithTimeout(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_writes_bytes_opt_(Length) PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ULONG Length,
    _Out_ PULONG LengthNeeded
    )
{
    PPHP_QUERY_OBJECT_COMMON_CONTEXT context;

    context = PhAllocate(sizeof(PHP_QUERY_OBJECT_COMMON_CONTEXT));
    context->Work = NtQuerySecurityObjectWork;
    context->Status = STATUS_UNSUCCESSFUL;
    context->u.NtQuerySecurityObject.Handle = Handle;
    context->u.NtQuerySecurityObject.SecurityInformation = SecurityInformation;
    context->u.NtQuerySecurityObject.SecurityDescriptor = SecurityDescriptor;
    context->u.NtQuerySecurityObject.Length = Length;
    context->u.NtQuerySecurityObject.LengthNeeded = LengthNeeded;

    return PhpCommonQueryObjectWithTimeout(context);
}

NTSTATUS PhCallNtSetSecurityObjectWithTimeout(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    PPHP_QUERY_OBJECT_COMMON_CONTEXT context;

    context = PhAllocate(sizeof(PHP_QUERY_OBJECT_COMMON_CONTEXT));
    context->Work = NtSetSecurityObjectWork;
    context->Status = STATUS_UNSUCCESSFUL;
    context->u.NtSetSecurityObject.Handle = Handle;
    context->u.NtSetSecurityObject.SecurityInformation = SecurityInformation;
    context->u.NtSetSecurityObject.SecurityDescriptor = SecurityDescriptor;

    return PhpCommonQueryObjectWithTimeout(context);
}

NTSTATUS PhCallKphDuplicateObjectWithTimeout(
    _In_ HANDLE SourceProcessHandle,
    _In_ HANDLE SourceHandle,
    _In_opt_ HANDLE TargetProcessHandle,
    _Out_opt_ PHANDLE TargetHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG HandleAttributes,
    _In_ ULONG Options
    )
{
    PPHP_QUERY_OBJECT_COMMON_CONTEXT context;

    context = PhAllocate(sizeof(PHP_QUERY_OBJECT_COMMON_CONTEXT));
    context->Work = KphDuplicateObjectWork;
    context->Status = STATUS_UNSUCCESSFUL;
    context->u.KphDuplicateObject.SourceProcessHandle = SourceProcessHandle;
    context->u.KphDuplicateObject.SourceHandle = SourceHandle;
    context->u.KphDuplicateObject.TargetProcessHandle = TargetProcessHandle;
    context->u.KphDuplicateObject.TargetHandle = TargetHandle;
    context->u.KphDuplicateObject.DesiredAccess = DesiredAccess;
    context->u.KphDuplicateObject.HandleAttributes = HandleAttributes;
    context->u.KphDuplicateObject.Options = Options;

    return PhpCommonQueryObjectWithTimeout(context);
}