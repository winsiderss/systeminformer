#define PROCPRV_PRIVATE
#include <ph.h>
#include <wchar.h>

VOID NTAPI PhpProcessItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

BOOLEAN NTAPI PhpProcessHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG NTAPI PhpProcessHashtableHashFunction(
    __in PVOID Entry
    );

PPH_OBJECT_TYPE PhProcessItemType;

PPH_HASHTABLE PhProcessHashtable;
PH_FAST_LOCK PhProcessHashtableLock;

PH_CALLBACK PhProcessAddedEvent;
PH_CALLBACK PhProcessModifiedEvent;
PH_CALLBACK PhProcessRemovedEvent;

BOOLEAN PhInitializeProcessProvider()
{
    if (!PhInitializeProcessItem())
        return FALSE;

    PhProcessHashtable = PhCreateHashtable(
        sizeof(PPH_PROCESS_ITEM),
        PhpProcessHashtableCompareFunction,
        PhpProcessHashtableHashFunction,
        NULL,
        40
        );
    PhInitializeFastLock(&PhProcessHashtableLock);

    PhInitializeCallback(&PhProcessAddedEvent);
    PhInitializeCallback(&PhProcessModifiedEvent);
    PhInitializeCallback(&PhProcessRemovedEvent);

    return TRUE;
}

BOOLEAN PhInitializeProcessItem()
{
    return NT_SUCCESS(PhCreateObjectType(
        &PhProcessItemType,
        0,
        PhpProcessItemDeleteProcedure
        ));
}

PPH_PROCESS_ITEM PhCreateProcessItem(
    __in HANDLE ProcessId
    )
{
    PPH_PROCESS_ITEM processItem;

    if (!NT_SUCCESS(PhCreateObject(
        &processItem,
        sizeof(PH_PROCESS_ITEM),
        0,
        PhProcessItemType,
        0
        )))
        return NULL;

    memset(processItem, 0, sizeof(PH_PROCESS_ITEM));
    processItem->ProcessId = ProcessId;

    return processItem;
}

VOID PhpProcessItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Object;

    if (processItem->ProcessName) PhDereferenceObject(processItem->ProcessName);
    if (processItem->FileName) PhDereferenceObject(processItem->FileName);
    if (processItem->CommandLine) PhDereferenceObject(processItem->CommandLine);
    if (processItem->UserName) PhDereferenceObject(processItem->UserName);
}

BOOLEAN PhpProcessHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    return
        (*(PPH_PROCESS_ITEM *)Entry1)->ProcessId ==
        (*(PPH_PROCESS_ITEM *)Entry2)->ProcessId;
}

ULONG PhpProcessHashtableHashFunction(
    __in PVOID Entry
    )
{
    return (ULONG)(*(PPH_PROCESS_ITEM *)Entry)->ProcessId;
}

PPH_PROCESS_ITEM PhReferenceProcessItem(
    __in HANDLE ProcessId
    )
{
    PH_PROCESS_ITEM lookupProcessItem;
    PPH_PROCESS_ITEM lookupProcessItemPtr = &lookupProcessItem;
    PPH_PROCESS_ITEM *processItemPtr;
    PPH_PROCESS_ITEM processItem;

    lookupProcessItem.ProcessId = ProcessId;

    PhAcquireFastLockShared(&PhProcessHashtableLock);

    processItemPtr = (PPH_PROCESS_ITEM *)PhGetHashtableEntry(
        PhProcessHashtable,
        &lookupProcessItemPtr
        );

    if (processItemPtr)
    {
        processItem = *processItemPtr;
        PhReferenceObject(processItem);
    }
    else
    {
        processItem = NULL;
    }

    PhReleaseFastLockShared(&PhProcessHashtableLock);

    return processItem;
}

VOID PhpRemoveProcessItem(
    __in HANDLE ProcessId
    )
{
    PH_PROCESS_ITEM lookupProcessItem;
    PPH_PROCESS_ITEM lookupProcessItemPtr = &lookupProcessItem;
    PPH_PROCESS_ITEM *processItemPtr;

    lookupProcessItem.ProcessId = ProcessId;

    processItemPtr = PhGetHashtableEntry(PhProcessHashtable, &lookupProcessItemPtr);

    if (processItemPtr)
    {
        PhRemoveHashtableEntry(PhProcessHashtable, &lookupProcessItemPtr);
        PhDereferenceObject(*processItemPtr);
    }
}

VOID PhpFillProcessItem(
    __inout PPH_PROCESS_ITEM ProcessItem,
    __in PSYSTEM_PROCESS_INFORMATION Process
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    ProcessItem->ParentProcessId = Process->InheritedFromUniqueProcessId;
    ProcessItem->ProcessName = PhCreateStringEx(Process->ImageName.Buffer, Process->ImageName.Length);
    ProcessItem->SessionId = Process->SessionId;
    ProcessItem->CreateTime = Process->CreateTime;

    _snwprintf(ProcessItem->ProcessIdString, PH_INT_STR_LEN, L"%d", ProcessItem->ProcessId);
    _snwprintf(ProcessItem->ParentProcessIdString, PH_INT_STR_LEN, L"%d", ProcessItem->ParentProcessId);
    _snwprintf(ProcessItem->SessionIdString, PH_INT_STR_LEN, L"%d", ProcessItem->SessionId);

    status = PhOpenProcess(&processHandle, ProcessQueryAccess, ProcessItem->ProcessId);

    if (!NT_SUCCESS(status))
        return;

    // Process information
    {
        PPH_STRING fileName;

        status = PhGetProcessImageFileName(processHandle, &fileName);

        if (NT_SUCCESS(status))
        {
            PPH_STRING newFileName;
            
            newFileName = PhGetFileName(fileName);
            PhSwapReference(&ProcessItem->FileName, newFileName);

            PhDereferenceObject(fileName);
            PhDereferenceObject(newFileName);
        }
    }

    {
        HANDLE processHandle2;

        status = PhOpenProcess(
            &processHandle2,
            ProcessQueryAccess | PROCESS_VM_READ,
            ProcessItem->ProcessId
            );

        if (NT_SUCCESS(status))
        {
            PPH_STRING commandLine;

            status = PhGetProcessCommandLine(processHandle2, &commandLine);

            if (NT_SUCCESS(status))
            {
                PhSwapReference(&ProcessItem->CommandLine, commandLine);
                PhDereferenceObject(commandLine);
            }

            CloseHandle(processHandle2);
        }
    }

    // Token-related information
    {
        HANDLE tokenHandle;

        status = PhOpenProcessToken(&tokenHandle, TOKEN_QUERY, processHandle);

        if (NT_SUCCESS(status))
        {
            // User name
            {
                PTOKEN_USER user;

                status = PhGetTokenUser(tokenHandle, &user);

                if (NT_SUCCESS(status))
                {
                    PPH_STRING userName;
                    PPH_STRING domainName;

                    if (PhLookupSid(user->User.Sid, &userName, &domainName, NULL))
                    {
                        PPH_STRING fullName;

                        fullName = PhCreateStringEx(NULL, domainName->Length + 2 + userName->Length);
                        memcpy(fullName->Buffer, domainName->Buffer, domainName->Length);
                        fullName->Buffer[domainName->Length / 2] = '\\';
                        memcpy(&fullName->Buffer[domainName->Length / 2 + 1], userName->Buffer, userName->Length);

                        PhSwapReference(&ProcessItem->UserName, fullName);

                        PhDereferenceObject(userName);
                        PhDereferenceObject(domainName);
                        PhDereferenceObject(fullName);
                    }

                    PhFree(user);
                }
            }

            CloseHandle(tokenHandle);
        }
    }

    CloseHandle(processHandle);
}

VOID PhUpdateProcesses()
{
    // Note about locking:
    // Since this is the only function that is allowed to 
    // modify the process hashtable, locking is not needed 
    // for shared accesses. However, exclusive accesses 
    // need locking.

    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;
    PPH_LIST pids;

    if (!NT_SUCCESS(PhEnumProcesses(&processes)))
        return;

    // Create a PID list.

    pids = PhCreateList(40);

    process = PH_FIRST_PROCESS(processes);

    do
    {
        PhAddListItem(pids, process->UniqueProcessId);
    } while (process = PH_NEXT_PROCESS(process));

    // Look for dead processes.
    {
        PPH_LIST pidsToRemove;
        ULONG enumerationKey = 0;
        PPH_PROCESS_ITEM *processItem;
        ULONG i;

        pidsToRemove = PhCreateList(1);

        while (PhEnumHashtable(PhProcessHashtable, (PPVOID)&processItem, &enumerationKey))
        {
            // Check if the process still exists.
            if (PhIndexOfListItem(pids, (*processItem)->ProcessId) == -1)
            {
                // Raise the process removed event.
                PhInvokeCallback(&PhProcessRemovedEvent, *processItem);
                PhAddListItem(pidsToRemove, (*processItem)->ProcessId);
            }
        }

        PhAcquireFastLockExclusive(&PhProcessHashtableLock);

        for (i = 0; i < pidsToRemove->Count; i++)
        {
            PhpRemoveProcessItem(pidsToRemove->Items[i]);
        }

        PhReleaseFastLockExclusive(&PhProcessHashtableLock);

        PhDereferenceObject(pidsToRemove);
    }

    // Look for new processes and update existing ones.
    {
        process = PH_FIRST_PROCESS(processes);

        do
        {
            PPH_PROCESS_ITEM processItem;

            processItem = PhReferenceProcessItem(process->UniqueProcessId);

            if (!processItem)
            {
                // Create the process item and fill in basic information.
                processItem = PhCreateProcessItem(process->UniqueProcessId);
                PhpFillProcessItem(processItem, process);

                // Add the process item to the hashtable.
                PhAcquireFastLockExclusive(&PhProcessHashtableLock);
                PhAddHashtableEntry(PhProcessHashtable, &processItem);
                PhReleaseFastLockExclusive(&PhProcessHashtableLock);

                // Raise the process added event.
                PhInvokeCallback(&PhProcessAddedEvent, processItem);

                // (Add a reference for the process item being in the hashtable.)
                // Instead of referencing then dereferencing we simply don't 
                // do anything.
            }
            else
            {
                // Nothing for now.
                PhDereferenceObject(processItem);
            }
        } while (process = PH_NEXT_PROCESS(process));
    }

    PhDereferenceObject(pids);
    PhFree(processes);
}

VOID NTAPI PhProcessProviderUpdate(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PhUpdateProcesses();
}
