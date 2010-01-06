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

typedef struct _PH_QUERY_OBJECT_CONTEXT
{
    LOGICAL Initialized;
    HANDLE Handle;
    POBJECT_NAME_INFORMATION Buffer;
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
                stringSid = PhConvertSidToStringSid(tokenUser->User.Sid);

                PhFree(tokenUser);
            }

            CloseHandle(tokenHandle);
        }

        HkcuPrefix = PhConcatStrings2(L"\\REGISTRY\\USER\\", stringSid->Buffer);
        HkcucrPrefix = PhConcatStrings2(HkcuPrefix->Buffer, L"_Classes");
    }
}

NTSTATUS PhpGetObjectTypeName(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in ULONG ObjectTypeNumber,
    __out PPH_STRING *TypeName
    )
{
    NTSTATUS status;
    PPH_STRING typeName = NULL;

    // If the cache contains the object type name, use it. Otherwise, 
    // query the type name.

    typeName = PhObjectTypeNames[ObjectTypeNumber];

    if (typeName)
    {
        PhReferenceObject(typeName);
    }
    else
    {
        POBJECT_TYPE_INFORMATION buffer;
        ULONG returnLength;
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

        if (!NT_SUCCESS(status))
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
    ULONG returnLength;

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

    if (!NT_SUCCESS(status))
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

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *ObjectName = PhCreateStringEx(buffer->Name.Buffer, buffer->Name.Length);

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

PPH_STRING PhGetClientIdName(
    __in PCLIENT_ID ClientId
    )
{
    PPH_STRING name;
    PPH_STRING processName = NULL;
    PPH_PROCESS_ITEM processItem;
    WCHAR processIdString[PH_INT_STR_LEN_1];
    WCHAR threadIdString[PH_INT_STR_LEN_1];

    processItem = PhReferenceProcessItem(ClientId->UniqueProcess);

    if (processItem)
    {
        processName = processItem->ProcessName;
        PhReferenceObject(processName);
        PhDereferenceObject(processItem);
    }

    PhPrintInteger(processIdString, (ULONG)ClientId->UniqueProcess);

    if (ClientId->UniqueThread)
    {
        PhPrintInteger(threadIdString, (ULONG)ClientId->UniqueThread);

        if (processName)
        {
            name = PhConcatStrings(5, processName->Buffer, L" (", processIdString,
                L"): ", threadIdString);
        }
        else
        {
            name = PhConcatStrings(4, L"Non-existent process (", processIdString,
                L"): ", threadIdString);
        }
    }
    else
    {
        if (processName)
            name = PhConcatStrings(4, processName->Buffer, L" (", processIdString, L")");
        else
            name = PhConcatStrings(3, L"Non-existent process (", processIdString, L")");
    }

    PhDereferenceObject(processName);

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
        bestObjectName = PhGetFileName(ObjectName);
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
            CloseHandle(dupHandle);

            clientId.UniqueProcess = basicInfo.UniqueProcessId;
        }

        bestObjectName = PhGetClientIdName(&clientId);
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

            if (!NT_SUCCESS(status) || !clientId.UniqueProcess || !clientId.UniqueThread)
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
            CloseHandle(dupHandle);

            clientId = basicInfo.ClientId;
        }

        bestObjectName = PhGetClientIdName(&clientId);
    }
    else if (PhStringEquals2(TypeName, L"Token", TRUE))
    {
        HANDLE dupHandle;
        PTOKEN_USER tokenUser = NULL;
        TOKEN_STATISTICS statistics;

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

            fullName = PhGetSidFullName(tokenUser->User.Sid);

            if (fullName)
            {
                WCHAR authIdString[PH_INT_STR_LEN_1];

                _snwprintf(authIdString, PH_INT_STR_LEN, L"%x", statistics.AuthenticationId.LowPart);
                bestObjectName = PhConcatStrings(3, fullName, L": 0x", authIdString);

                PhDereferenceObject(fullName);
            }

            PhFree(tokenUser);
        }

        CloseHandle(dupHandle);
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

NTSTATUS PhGetHandleInformation(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in ULONG ObjectTypeNumber,
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
    if (ObjectTypeNumber > MAX_OBJECT_TYPE_NUMBER)
        return STATUS_INVALID_PARAMETER_3;

    // Duplicate the handle if we're not using KPH.
    if (!PhKphHandle)
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
        if (TypeName)
        {
            *TypeName = typeName;
            PhReferenceObject(typeName);
        }

        if (ObjectName)
        {
            *ObjectName = objectName;
            PhReferenceObject(objectName);
        }

        if (BestObjectName)
        {
            *BestObjectName = bestObjectName;
            PhReferenceObject(bestObjectName);
        }
    }

    if (dupHandle)
        CloseHandle(dupHandle);
    if (typeName)
        PhDereferenceObject(typeName);
    if (objectName)
        PhDereferenceObject(objectName);
    if (bestObjectName)
        PhDereferenceObject(bestObjectName);

    return status;
}

NTSTATUS PhQueryObjectNameHack(
    __in HANDLE Handle,
    __out_bcount(ObjectNameInformationLength) POBJECT_NAME_INFORMATION ObjectNameInformation,
    __in ULONG ObjectNameInformationLength,
    __out_opt PULONG ReturnLength
    )
{
    ULONG waitResult;

    PhAcquireMutex(&PhQueryObjectMutex);

    // Create a query thread if we don't have one.
    if (!PhQueryObjectThreadHandle)
    {
        PhQueryObjectThreadHandle = CreateThread(
            NULL, 0, (LPTHREAD_START_ROUTINE)PhpQueryObjectThreadStart, NULL, 0, NULL);

        if (!PhQueryObjectThreadHandle)
        {
            PhReleaseMutex(&PhQueryObjectMutex);
            return STATUS_UNSUCCESSFUL;
        }
    }

    // Create the events if they don't exist.
    if (!PhQueryObjectStartEvent)
    {
        if (!(PhQueryObjectStartEvent = CreateEvent(NULL, FALSE, FALSE, NULL)))
        {
            PhReleaseMutex(&PhQueryObjectMutex);
            return STATUS_UNSUCCESSFUL;
        }
    }

    if (!PhQueryObjectCompletedEvent)
    {
        if (!(PhQueryObjectCompletedEvent = CreateEvent(NULL, FALSE, FALSE, NULL)))
        {
            PhReleaseMutex(&PhQueryObjectMutex);
            return STATUS_UNSUCCESSFUL;
        }
    }

    // Initialize the work context.
    PhQueryObjectContext.Handle = Handle;
    PhQueryObjectContext.Buffer = ObjectNameInformation;
    PhQueryObjectContext.Length = ObjectNameInformationLength;
    PhQueryObjectContext.Initialized = TRUE;
    // Allow the worker thread to start.
    SetEvent(PhQueryObjectStartEvent);
    // Wait for the work to complete, with a timeout of 1 second.
    waitResult = WaitForSingleObject(PhQueryObjectCompletedEvent, 1000);
    // Set the context as uninitialized.
    PhQueryObjectContext.Initialized = FALSE;

    // Return normally if the work was completed.
    if (waitResult == WAIT_OBJECT_0)
    {
        NTSTATUS status;
        ULONG returnLength;

        // Copy the status information before we release the mutex.
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
            PhQueryObjectContext.Status = NtQueryObject(
                PhQueryObjectContext.Handle,
                ObjectNameInformation,
                PhQueryObjectContext.Buffer,
                PhQueryObjectContext.Length,
                &PhQueryObjectContext.ReturnLength
                );

            // Work done.
            SetEvent(PhQueryObjectCompletedEvent);
        }
    }

    return STATUS_SUCCESS;
}
