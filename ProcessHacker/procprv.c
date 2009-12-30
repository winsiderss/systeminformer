#define PROCPRV_PRIVATE
#include <ph.h>
#include <wchar.h>

typedef struct _PH_PROCESS_QUERY_DATA
{
    ULONG Stage;
    PPH_PROCESS_ITEM ProcessItem;
} PH_PROCESS_QUERY_DATA, *PPH_PROCESS_QUERY_DATA;

typedef struct _PH_PROCESS_QUERY_S1_DATA
{
    PH_PROCESS_QUERY_DATA Header;

    HICON SmallIcon;
    HICON LargeIcon;
    PH_IMAGE_VERSION_INFO VersionInfo;

    PH_INTEGRITY IntegrityLevel;
    PPH_STRING IntegrityString;

    PPH_STRING JobName;
    BOOLEAN IsInJob;
    BOOLEAN IsInSignificantJob;

    BOOLEAN IsPosix;
    BOOLEAN IsWow64;
} PH_PROCESS_QUERY_S1_DATA, *PPH_PROCESS_QUERY_S1_DATA;

typedef struct _PH_PROCESS_QUERY_S2_DATA
{
    PH_PROCESS_QUERY_DATA Header;

    BOOLEAN IsDotNet;

    ULONG VerifyResult;
    PPH_STRING VerifySignerName;

    BOOLEAN IsPacked;
    ULONG ImportFunctions;
    ULONG ImportModules;
} PH_PROCESS_QUERY_S2_DATA, *PPH_PROCESS_QUERY_S2_DATA;

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

PPH_QUEUE PhProcessQueryDataQueue;
PH_MUTEX PhProcessQueryDataQueueLock;

PH_CALLBACK PhProcessAddedEvent;
PH_CALLBACK PhProcessModifiedEvent;
PH_CALLBACK PhProcessRemovedEvent;

BOOLEAN PhInitializeProcessProvider()
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhProcessItemType,
        0,
        PhpProcessItemDeleteProcedure
        )))
        return FALSE;

    PhProcessHashtable = PhCreateHashtable(
        sizeof(PPH_PROCESS_ITEM),
        PhpProcessHashtableCompareFunction,
        PhpProcessHashtableHashFunction,
        40
        );
    PhInitializeFastLock(&PhProcessHashtableLock);

    PhProcessQueryDataQueue = PhCreateQueue(40);
    PhInitializeMutex(&PhProcessQueryDataQueueLock);

    PhInitializeCallback(&PhProcessAddedEvent);
    PhInitializeCallback(&PhProcessModifiedEvent);
    PhInitializeCallback(&PhProcessRemovedEvent);

    return TRUE;
}

BOOLEAN PhInitializeImageVersionInfo(
    __out PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    __in PWSTR FileName
    )
{
    PVOID versionInfo;
    ULONG langCodePage;

    versionInfo = PhGetFileVersionInfo(FileName);

    if (!versionInfo)
        return FALSE;

    langCodePage = PhGetFileVersionInfoLangCodePage(versionInfo);

    ImageVersionInfo->CompanyName = PhGetFileVersionInfoString2(versionInfo, langCodePage, L"CompanyName");
    ImageVersionInfo->FileDescription = PhGetFileVersionInfoString2(versionInfo, langCodePage, L"FileDescription");
    ImageVersionInfo->FileVersion = PhGetFileVersionInfoString2(versionInfo, langCodePage, L"FileVersion");
    ImageVersionInfo->ProductName = PhGetFileVersionInfoString2(versionInfo, langCodePage, L"ProductName");

    PhFree(versionInfo);

    return TRUE;
}

VOID PhDeleteImageVersionInfo(
    __inout PPH_IMAGE_VERSION_INFO ImageVersionInfo
    )
{
    if (ImageVersionInfo->CompanyName) PhDereferenceObject(ImageVersionInfo->CompanyName);
    if (ImageVersionInfo->FileDescription) PhDereferenceObject(ImageVersionInfo->FileDescription);
    if (ImageVersionInfo->FileVersion) PhDereferenceObject(ImageVersionInfo->FileVersion);
    if (ImageVersionInfo->ProductName) PhDereferenceObject(ImageVersionInfo->ProductName);
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
    if (processItem->SmallIcon) DestroyIcon(processItem->SmallIcon);
    if (processItem->LargeIcon) DestroyIcon(processItem->LargeIcon);
    PhDeleteImageVersionInfo(&processItem->VersionInfo);
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

    // Construct a temporary process item for the lookup.
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
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PhRemoveHashtableEntry(PhProcessHashtable, &ProcessItem);
    PhDereferenceObject(ProcessItem);
}

VOID PhpProcessQueryStage1(
    __inout PPH_PROCESS_QUERY_S1_DATA Data
    )
{
    NTSTATUS status;
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;
    HANDLE processId = processItem->ProcessId;

    if (processItem->FileName)
    {
        // Small icon, large icon.
        {
            SHFILEINFO fileInfo;

            if (SHGetFileInfo(
                processItem->FileName->Buffer,
                0,
                &fileInfo,
                sizeof(SHFILEINFO),
                SHGFI_ICON | SHGFI_SMALLICON
                ))
                Data->SmallIcon = fileInfo.hIcon;

            if (SHGetFileInfo(
                processItem->FileName->Buffer,
                0,
                &fileInfo,
                sizeof(SHFILEINFO),
                SHGFI_ICON | SHGFI_LARGEICON
                ))
                Data->LargeIcon = fileInfo.hIcon;
        }

        // Version info.
        PhInitializeImageVersionInfo(&Data->VersionInfo, processItem->FileName->Buffer);
    }

    {
        HANDLE processHandle;

        status = PhOpenProcess(&processHandle, ProcessQueryAccess, processId);

        if (NT_SUCCESS(status))
        {
            // Integrity
            if (WindowsVersion >= WINDOWS_VISTA)
            {
                HANDLE tokenHandle;

                status = PhOpenProcessToken(&tokenHandle, TOKEN_QUERY, processHandle);

                if (NT_SUCCESS(status))
                {
                    PhGetTokenIntegrityLevel(
                        tokenHandle,
                        &Data->IntegrityLevel,
                        &Data->IntegrityString
                        );

                    CloseHandle(tokenHandle);
                }
            }

#ifdef _M_X64
            // WOW64
            PhGetProcessIsWow64(processHandle, &Data->IsWow64);
#endif

            CloseHandle(processHandle);
        }
    }

    // Posix
    {
        HANDLE processHandle;

        status = PhOpenProcess(
            &processHandle,
            ProcessQueryAccess | PROCESS_VM_READ,
            processId
            );

        if (NT_SUCCESS(status))
        {
            PhGetProcessIsPosix(processHandle, &Data->IsPosix);
            CloseHandle(processHandle);
        }
    }
}

VOID PhpProcessQueryStage2(
    __inout PPH_PROCESS_QUERY_S2_DATA Data
    )
{
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;
    HANDLE processId = processItem->ProcessId;

}

NTSTATUS PhpProcessQueryStage1Worker(
    __in PVOID Parameter
    )
{
    PPH_PROCESS_QUERY_S1_DATA data;
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    data = PhAllocate(sizeof(PH_PROCESS_QUERY_S1_DATA));
    memset(data, 0, sizeof(PH_PROCESS_QUERY_S1_DATA));
    data->Header.Stage = 1;
    data->Header.ProcessItem = processItem;

    PhpProcessQueryStage1(data);

    PhAcquireMutex(&PhProcessQueryDataQueueLock);
    PhEnqueueQueueItem(PhProcessQueryDataQueue, data);
    PhReleaseMutex(&PhProcessQueryDataQueueLock);

    return STATUS_SUCCESS;
}

NTSTATUS PhpProcessQueryStage2Worker(
    __in PVOID Parameter
    )
{
    PPH_PROCESS_QUERY_S2_DATA data;
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    data = PhAllocate(sizeof(PH_PROCESS_QUERY_S2_DATA));
    memset(data, 0, sizeof(PH_PROCESS_QUERY_S2_DATA));
    data->Header.Stage = 2;
    data->Header.ProcessItem = processItem;

    PhpProcessQueryStage2(data);

    PhAcquireMutex(&PhProcessQueryDataQueueLock);
    PhEnqueueQueueItem(PhProcessQueryDataQueue, data);
    PhReleaseMutex(&PhProcessQueryDataQueueLock);

    return STATUS_SUCCESS;
}

VOID PhpQueueProcessQueryStage1(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PhReferenceObject(ProcessItem);
    PhQueueGlobalWorkQueueItem(PhpProcessQueryStage1Worker, ProcessItem);
}

VOID PhpQueueProcessQueryStage2(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PhReferenceObject(ProcessItem);
    PhQueueGlobalWorkQueueItem(PhpProcessQueryStage2Worker, ProcessItem);
}

VOID PhpFillProcessItemStage1(
    __in PPH_PROCESS_QUERY_S1_DATA Data
    )
{
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;

    processItem->SmallIcon = Data->SmallIcon;
    processItem->LargeIcon = Data->LargeIcon;
    memcpy(&processItem->VersionInfo, &Data->VersionInfo, sizeof(PH_IMAGE_VERSION_INFO));
    processItem->IntegrityLevel = Data->IntegrityLevel;
    processItem->JobName = Data->JobName;
    processItem->IsInJob = Data->IsInJob;
    processItem->IsInSignificantJob = Data->IsInSignificantJob;
    processItem->IsPosix = Data->IsPosix;
    processItem->IsWow64 = Data->IsWow64;

    if (Data->IntegrityString)
    {
        wcsncpy(processItem->IntegrityString, Data->IntegrityString->Buffer, PH_INTEGRITY_STR_LEN);
        PhDereferenceObject(Data->IntegrityString);
    }
}

VOID PhpFillProcessItemStage2(
    __in PPH_PROCESS_QUERY_S2_DATA Data
    )
{
    
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
            ProcessItem->FileName = newFileName;

            PhDereferenceObject(fileName);
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
                ProcessItem->CommandLine = commandLine;
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

                        ProcessItem->UserName = fullName;

                        PhDereferenceObject(userName);
                        PhDereferenceObject(domainName);
                    }

                    PhFree(user);
                }
            }

            CloseHandle(tokenHandle);
        }
    }

    CloseHandle(processHandle);
}

VOID PhProcessProviderUpdate(
    __in PVOID Context
    )
{
    static ULONG runCount = 0;

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
        PPH_LIST processesToRemove = NULL;
        ULONG enumerationKey = 0;
        PPH_PROCESS_ITEM *processItem;
        ULONG i;

        while (PhEnumHashtable(PhProcessHashtable, (PPVOID)&processItem, &enumerationKey))
        {
            // Check if the process still exists.
            if (PhIndexOfListItem(pids, (*processItem)->ProcessId) == -1)
            {
                // Raise the process removed event.
                PhInvokeCallback(&PhProcessRemovedEvent, *processItem);

                if (!processesToRemove)
                    processesToRemove = PhCreateList(2);

                PhAddListItem(processesToRemove, *processItem);
            }
        }

        // Lock only if we have something to do.
        if (processesToRemove)
        {
            PhAcquireFastLockExclusive(&PhProcessHashtableLock);

            for (i = 0; i < processesToRemove->Count; i++)
            {
                PhpRemoveProcessItem((PPH_PROCESS_ITEM)processesToRemove->Items[i]);
            }

            PhReleaseFastLockExclusive(&PhProcessHashtableLock);
            PhDereferenceObject(processesToRemove);
        }
    }

    // Go through the queued process query data.
    {
        PPH_PROCESS_QUERY_DATA data;

        PhAcquireMutex(&PhProcessQueryDataQueueLock);

        while (PhDequeueQueueItem(PhProcessQueryDataQueue, &data))
        {
            PhReleaseMutex(&PhProcessQueryDataQueueLock);

            if (data->Stage == 1)
            {
                PhpFillProcessItemStage1((PPH_PROCESS_QUERY_S1_DATA)data);
                data->ProcessItem->JustProcessed = TRUE;
            }
            else if (data->Stage == 2)
            {
                PhpFillProcessItemStage2((PPH_PROCESS_QUERY_S2_DATA)data);
                data->ProcessItem->JustProcessed = TRUE;
            }

            PhDereferenceObject(data->ProcessItem);
            PhFree(data);
            PhAcquireMutex(&PhProcessQueryDataQueueLock);
        }

        PhReleaseMutex(&PhProcessQueryDataQueueLock);
    }

    // Look for new processes and update existing ones.
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

            // If this is the first run of the provider, queue the 
            // process query tasks. Otherwise, perform stage 1 
            // processing now and queue stage 2 processing.
            if (runCount > 0)
            {
                PH_PROCESS_QUERY_S1_DATA data;

                memset(&data, 0, sizeof(PH_PROCESS_QUERY_S1_DATA));
                data.Header.Stage = 1;
                data.Header.ProcessItem = processItem;
                PhpProcessQueryStage1(&data);
                PhpFillProcessItemStage1(&data);
            }
            else
            {
                PhpQueueProcessQueryStage1(processItem);
            }

            PhpQueueProcessQueryStage2(processItem);

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
            if (processItem->JustProcessed)
            {
                PhInvokeCallback(&PhProcessModifiedEvent, processItem);
                processItem->JustProcessed = FALSE;
            }

            PhDereferenceObject(processItem);
        }
    } while (process = PH_NEXT_PROCESS(process));

    PhDereferenceObject(pids);
    PhFree(processes);

    runCount++;
}
