/*
 * Process Hacker - 
 *   process provider
 * 
 * Copyright (C) 2009-2010 wj32
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

#define PROCPRV_PRIVATE
#include <ph.h>

VOID PhpQueueProcessQueryStage1(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhpQueueProcessQueryStage2(
    __in PPH_PROCESS_ITEM ProcessItem
    );

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

    BOOLEAN IsWow64;
} PH_PROCESS_QUERY_S1_DATA, *PPH_PROCESS_QUERY_S1_DATA;

typedef struct _PH_PROCESS_QUERY_S2_DATA
{
    PH_PROCESS_QUERY_DATA Header;

    BOOLEAN IsDotNet;

    VERIFY_RESULT VerifyResult;
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

VOID PhpUpdatePerfInformation();

VOID PhpUpdateCpuInformation();

PPH_OBJECT_TYPE PhProcessItemType;

PPH_HASHTABLE PhProcessHashtable;
PH_FAST_LOCK PhProcessHashtableLock;

PPH_QUEUE PhProcessQueryDataQueue;
PH_MUTEX PhProcessQueryDataQueueLock;

PH_CALLBACK PhProcessAddedEvent;
PH_CALLBACK PhProcessModifiedEvent;
PH_CALLBACK PhProcessRemovedEvent;

SYSTEM_PERFORMANCE_INFORMATION PhPerfInformation;
PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuInformation;
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuTotals;

SYSTEM_PROCESS_INFORMATION PhDpcsProcessInformation;
SYSTEM_PROCESS_INFORMATION PhInterruptsProcessInformation;

static PH_UINT64_DELTA PhCpuKernelDelta;
static PH_UINT64_DELTA PhCpuUserDelta;
static PH_UINT64_DELTA PhCpuOtherDelta;

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

    RtlInitUnicodeString(
        &PhDpcsProcessInformation.ImageName,
        L"DPCs"
        );
    PhDpcsProcessInformation.UniqueProcessId = DPCS_PROCESS_ID;
    PhDpcsProcessInformation.InheritedFromUniqueProcessId = SYSTEM_IDLE_PROCESS_ID;

    RtlInitUnicodeString(
        &PhInterruptsProcessInformation.ImageName,
        L"Interrupts"
        );
    PhInterruptsProcessInformation.UniqueProcessId = INTERRUPTS_PROCESS_ID;
    PhInterruptsProcessInformation.InheritedFromUniqueProcessId = SYSTEM_IDLE_PROCESS_ID;

    PhCpuInformation = PhAllocate(
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) *
        PhSystemBasicInformation.NumberOfProcessors
        );

    PhInitializeDelta(&PhCpuKernelDelta);
    PhInitializeDelta(&PhCpuUserDelta);
    PhInitializeDelta(&PhCpuOtherDelta);

    PhpUpdatePerfInformation();
    PhpUpdateCpuInformation();

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
    PhInitializeEvent(&processItem->Stage1Event);
    processItem->ServiceList = PhCreatePointerList(1);
    PhInitializeFastLock(&processItem->ServiceListLock);

    processItem->ProcessId = ProcessId;

    if (ProcessId != DPCS_PROCESS_ID && ProcessId != INTERRUPTS_PROCESS_ID)
        PhPrintUInt32(processItem->ProcessIdString, (ULONG)ProcessId);

    return processItem;
}

VOID PhpProcessItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Object;
    ULONG i;

    for (i = 0; i < processItem->ServiceList->Count; i++)
        PhDereferenceObject(processItem->ServiceList->Items[i]);

    PhDereferenceObject(processItem->ServiceList);
    PhDeleteFastLock(&processItem->ServiceListLock);

    if (processItem->ProcessName) PhDereferenceObject(processItem->ProcessName);
    if (processItem->FileName) PhDereferenceObject(processItem->FileName);
    if (processItem->CommandLine) PhDereferenceObject(processItem->CommandLine);
    if (processItem->SmallIcon) DestroyIcon(processItem->SmallIcon);
    if (processItem->LargeIcon) DestroyIcon(processItem->LargeIcon);
    PhDeleteImageVersionInfo(&processItem->VersionInfo);
    if (processItem->UserName) PhDereferenceObject(processItem->UserName);
    if (processItem->VerifySignerName) PhDereferenceObject(processItem->VerifySignerName);
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
    return (ULONG)(*(PPH_PROCESS_ITEM *)Entry)->ProcessId / 4;
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

    // Use the default EXE icon if we didn't get the file's icon.
    {
        SHFILEINFO fileInfo;

        if (!Data->SmallIcon)
        {
            if (SHGetFileInfo(
                L".exe",
                FILE_ATTRIBUTE_NORMAL,
                &fileInfo,
                sizeof(SHFILEINFO),
                SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES
                ))
                Data->SmallIcon = fileInfo.hIcon;
        }

        if (!Data->LargeIcon)
        {
            if (SHGetFileInfo(
                L".exe",
                FILE_ATTRIBUTE_NORMAL,
                &fileInfo,
                sizeof(SHFILEINFO),
                SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES
                ))
                Data->LargeIcon = fileInfo.hIcon;
        }
    }

    {
        HANDLE processHandle;

        status = PhOpenProcess(&processHandle, ProcessQueryAccess, processId);

        if (NT_SUCCESS(status))
        {
            // Integrity
            if (WINDOWS_HAS_UAC)
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

    PhpQueueProcessQueryStage2(processItem);
}

VOID PhpProcessQueryStage2(
    __inout PPH_PROCESS_QUERY_S2_DATA Data
    )
{
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;
    HANDLE processId = processItem->ProcessId;

    if (processItem->FileName)
    {
        Data->VerifyResult = PhVerifyFile(
            processItem->FileName->Buffer,
            &Data->VerifySignerName
            );
    }
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
    // Ref: dereferenced when the provider update function removes the item from 
    // the queue.
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
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;

    processItem->IsDotNet = Data->IsDotNet;
    processItem->VerifyResult = Data->VerifyResult;
    processItem->VerifySignerName = Data->VerifySignerName;
    processItem->IsPacked = Data->IsPacked;
    processItem->ImportFunctions = Data->ImportFunctions;
    processItem->ImportModules = Data->ImportModules;
}

VOID PhpFillProcessItem(
    __inout PPH_PROCESS_ITEM ProcessItem,
    __in PSYSTEM_PROCESS_INFORMATION Process
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    ProcessItem->ParentProcessId = Process->InheritedFromUniqueProcessId;
    ProcessItem->SessionId = Process->SessionId;
    ProcessItem->CreateTime = Process->CreateTime;

    if (ProcessItem->ProcessId != SYSTEM_IDLE_PROCESS_ID)
    {
        ProcessItem->ProcessName = PhCreateStringEx(Process->ImageName.Buffer, Process->ImageName.Length);
    }
    else
    {
        ProcessItem->ProcessName = PhCreateString(L"System Idle Process");
    }

    PhPrintUInt32(ProcessItem->ParentProcessIdString, (ULONG)ProcessItem->ParentProcessId);
    PhPrintUInt32(ProcessItem->SessionIdString, ProcessItem->SessionId);

    status = PhOpenProcess(&processHandle, ProcessQueryAccess, ProcessItem->ProcessId);

    if (!NT_SUCCESS(status))
        return;

    // Process information
    {
        // If we're dealing with System (PID 4), we need to get the 
        // kernel file name. Otherwise, get the image file name.

        if (ProcessItem->ProcessId != SYSTEM_PROCESS_ID)
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
        else
        {
            PPH_STRING fileName;

            fileName = PhGetKernelFileName();

            if (fileName)
            {
                PPH_STRING newFileName;

                newFileName = PhGetFileName(fileName);
                ProcessItem->FileName = newFileName;

                PhDereferenceObject(fileName);
            }
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
            BOOLEAN isPosix;
            PPH_STRING commandLine;
            ULONG i;

            status = PhGetProcessIsPosix(processHandle2, &isPosix);
            ProcessItem->IsPosix = isPosix;

            if (!NT_SUCCESS(status) || !isPosix)
            {
                status = PhGetProcessCommandLine(processHandle2, &commandLine);

                if (NT_SUCCESS(status))
                {
                    // Some command lines (e.g. from taskeng.exe) have nulls in them. 
                    // Since Windows can't display them, we'll replace them with 
                    // spaces.
                    for (i = 0; i < (ULONG)commandLine->Length / 2; i++)
                    {
                        if (commandLine->Buffer[i] == 0)
                            commandLine->Buffer[i] = ' ';
                    }
                }
            }
            else
            {
                // Get the POSIX command line.
                status = PhGetProcessPosixCommandLine(processHandle2, &commandLine);
            }

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

VOID PhpUpdatePerfInformation()
{
    ULONG returnLength;

    NtQuerySystemInformation(
        SystemPerformanceInformation,
        &PhPerfInformation,
        sizeof(SYSTEM_PERFORMANCE_INFORMATION),
        &returnLength
        );
}

VOID PhpUpdateCpuInformation()
{
    ULONG returnLength;
    ULONG i;

    NtQuerySystemInformation(
        SystemProcessorPerformanceInformation,
        PhCpuInformation,
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) *
        PhSystemBasicInformation.NumberOfProcessors,
        &returnLength
        );

    // Zero the CPU totals.
    memset(&PhCpuTotals, 0, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));

    for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
    {
        PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION cpuInfo =
            &PhCpuInformation[i];

        // KernelTime includes kernel + idle + DPC + interrupt time.
        cpuInfo->KernelTime.QuadPart -=
            cpuInfo->IdleTime.QuadPart +
            cpuInfo->DpcTime.QuadPart +
            cpuInfo->InterruptTime.QuadPart;

        PhCpuTotals.DpcTime.QuadPart += cpuInfo->DpcTime.QuadPart;
        PhCpuTotals.IdleTime.QuadPart += cpuInfo->IdleTime.QuadPart;
        PhCpuTotals.InterruptCount += cpuInfo->InterruptCount;
        PhCpuTotals.InterruptTime.QuadPart += cpuInfo->InterruptTime.QuadPart;
        PhCpuTotals.KernelTime.QuadPart += cpuInfo->KernelTime.QuadPart;
        PhCpuTotals.UserTime.QuadPart += cpuInfo->UserTime.QuadPart;
    }

    PhUpdateDelta(&PhCpuKernelDelta, PhCpuTotals.KernelTime.QuadPart);
    PhUpdateDelta(&PhCpuUserDelta, PhCpuTotals.UserTime.QuadPart);
    PhUpdateDelta(&PhCpuOtherDelta,
        PhCpuTotals.IdleTime.QuadPart +
        PhCpuTotals.DpcTime.QuadPart +
        PhCpuTotals.InterruptTime.QuadPart
        );
}

VOID PhProcessProviderUpdate(
    __in PVOID Object
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

    ULONG64 sysTotalTime;

    // Pre-update tasks

    if (runCount % 2 == 0)
        PhRefreshDosDeviceNames();

    PhpUpdatePerfInformation();
    PhpUpdateCpuInformation();

    sysTotalTime =
        PhCpuKernelDelta.Delta +
        PhCpuUserDelta.Delta +
        PhCpuOtherDelta.Delta;

    // Get the process list.

    if (!NT_SUCCESS(PhEnumProcesses(&processes)))
        return;

    // Create a PID list.

    pids = PhCreateList(40);

    process = PH_FIRST_PROCESS(processes);

    do
    {
        if (process->UniqueProcessId == SYSTEM_IDLE_PROCESS_ID)
        {
            process->KernelTime = PhCpuTotals.IdleTime;
        }

        PhAddListItem(pids, process->UniqueProcessId);
    } while (process = PH_NEXT_PROCESS(process));

    // Add the fake processes to the PID list.
    PhAddListItem(pids, DPCS_PROCESS_ID);
    PhAddListItem(pids, INTERRUPTS_PROCESS_ID);

    PhDpcsProcessInformation.KernelTime = PhCpuTotals.DpcTime;
    PhInterruptsProcessInformation.KernelTime = PhCpuTotals.InterruptTime;

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
                PhSetEvent(&data->ProcessItem->Stage1Event);
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

    while (process)
    {
        PPH_PROCESS_ITEM processItem;

        processItem = PhReferenceProcessItem(process->UniqueProcessId);

        if (!processItem)
        {
            // Create the process item and fill in basic information.
            processItem = PhCreateProcessItem(process->UniqueProcessId);
            processItem->RunId = runCount;
            PhpFillProcessItem(processItem, process);

            // Check if process actually has a parent.
            {
                PSYSTEM_PROCESS_INFORMATION parentProcess;

                processItem->HasParent = TRUE;
                parentProcess = PhFindProcessInformation(processes, processItem->ParentProcessId);

                if (!parentProcess || processItem->ParentProcessId == processItem->ProcessId)
                {
                    processItem->HasParent = FALSE;
                }
                else if (parentProcess)
                {
                    // Check the parent's creation time to see if it is 
                    // actually the parent.
                    if (parentProcess->CreateTime.QuadPart > processItem->CreateTime.QuadPart)
                        processItem->HasParent = FALSE;
                }
            }

            // Initialize the deltas.
            PhUpdateDelta(&processItem->CpuKernelDelta, process->KernelTime.QuadPart);
            PhUpdateDelta(&processItem->CpuUserDelta, process->UserTime.QuadPart);
            PhUpdateDelta(&processItem->IoReadDelta, process->ReadTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoWriteDelta, process->WriteTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoOtherDelta, process->OtherTransferCount.QuadPart);

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
                PhSetEvent(&processItem->Stage1Event);
            }
            else
            {
                PhpQueueProcessQueryStage1(processItem);
            }

            // Add pending service items to the process item.
            PhUpdateProcessItemServices(processItem);

            // Add the process item to the hashtable.
            PhAcquireFastLockExclusive(&PhProcessHashtableLock);
            PhAddHashtableEntry(PhProcessHashtable, &processItem);
            PhReleaseFastLockExclusive(&PhProcessHashtableLock);

            // Raise the process added event.
            PhInvokeCallback(&PhProcessAddedEvent, processItem);

            // (Ref: for the process item being in the hashtable.)
            // Instead of referencing then dereferencing we simply don't 
            // do anything.
            // Dereferenced in PhpRemoveProcessItem.
        }
        else
        {
            BOOLEAN modified = FALSE;
            FLOAT newCpuUsage;

            // Update the deltas.
            PhUpdateDelta(&processItem->CpuKernelDelta, process->KernelTime.QuadPart);
            PhUpdateDelta(&processItem->CpuUserDelta, process->UserTime.QuadPart);
            PhUpdateDelta(&processItem->IoReadDelta, process->ReadTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoWriteDelta, process->WriteTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoOtherDelta, process->OtherTransferCount.QuadPart);

            newCpuUsage = (FLOAT)(
                processItem->CpuKernelDelta.Delta +
                processItem->CpuUserDelta.Delta
                ) / sysTotalTime;

            if (processItem->JustProcessed)
            {
                processItem->JustProcessed = FALSE;
                modified = TRUE;
            }

            if (processItem->CpuUsage != newCpuUsage)
            {
                processItem->CpuUsage = newCpuUsage;
                modified = TRUE;

                if ((newCpuUsage * 100) >= 0.01)
                {
                    _snwprintf(processItem->CpuUsageString, PH_INT32_STR_LEN,
                        L"%.2f", (DOUBLE)newCpuUsage * 100);
                }
                else
                {
                    processItem->CpuUsageString[0] = 0;
                }
            }

            if (modified)
            {
                PhInvokeCallback(&PhProcessModifiedEvent, processItem);
            }

            PhDereferenceObject(processItem);
        }

        // Trick ourselves into thinking that DPCs and Interrupts 
        // are on the list.
        if (process == &PhInterruptsProcessInformation)
        {
            process = NULL;
        }
        else if (process == &PhDpcsProcessInformation)
        {
            process = &PhInterruptsProcessInformation;
        }
        else
        {
            process = PH_NEXT_PROCESS(process);

            if (process == NULL)
                process = &PhDpcsProcessInformation;
        }
    }

    PhDereferenceObject(pids);
    PhFree(processes);

    runCount++;
}
