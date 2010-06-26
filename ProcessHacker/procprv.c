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
#include <phapp.h>
#include <kph.h>
#include <wtsapi32.h>

VOID PhpQueueProcessQueryStage1(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhpQueueProcessQueryStage2(
    __in PPH_PROCESS_ITEM ProcessItem
    );

typedef struct _PH_PROCESS_QUERY_DATA
{
    SLIST_ENTRY ListEntry;
    ULONG Stage;
    PPH_PROCESS_ITEM ProcessItem;
} PH_PROCESS_QUERY_DATA, *PPH_PROCESS_QUERY_DATA;

typedef struct _PH_PROCESS_QUERY_S1_DATA
{
    PH_PROCESS_QUERY_DATA Header;

    PPH_STRING CommandLine;

    HICON SmallIcon;
    HICON LargeIcon;
    PH_IMAGE_VERSION_INFO VersionInfo;

    TOKEN_ELEVATION_TYPE ElevationType;
    BOOLEAN IsElevated;
    PH_INTEGRITY IntegrityLevel;
    PPH_STRING IntegrityString;

    PPH_STRING JobName;
    BOOLEAN IsInJob;
    BOOLEAN IsInSignificantJob;

    HANDLE ConsoleHostProcessId;

    BOOLEAN IsPosix;
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
PH_QUEUED_LOCK PhProcessHashtableLock = PH_QUEUED_LOCK_INIT;

SLIST_HEADER PhProcessQueryDataListHead;

PH_CALLBACK PhProcessAddedEvent;
PH_CALLBACK PhProcessModifiedEvent;
PH_CALLBACK PhProcessRemovedEvent;
PH_CALLBACK PhProcessesUpdatedEvent;

ULONG PhStatisticsSampleCount = 500;

SYSTEM_PERFORMANCE_INFORMATION PhPerfInformation;
PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuInformation;
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuTotals;

SYSTEM_PROCESS_INFORMATION PhDpcsProcessInformation;
SYSTEM_PROCESS_INFORMATION PhInterruptsProcessInformation;

static PH_UINT64_DELTA PhCpuKernelDelta;
static PH_UINT64_DELTA PhCpuUserDelta;
static PH_UINT64_DELTA PhCpuOtherDelta;

static PWTS_PROCESS_INFO PhpWtsProcesses = NULL;
static ULONG PhpWtsNumberOfProcesses;

BOOLEAN PhProcessProviderInitialization()
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhProcessItemType,
        L"ProcessItem",
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

    RtlInitializeSListHead(&PhProcessQueryDataListHead);

    PhInitializeCallback(&PhProcessAddedEvent);
    PhInitializeCallback(&PhProcessModifiedEvent);
    PhInitializeCallback(&PhProcessRemovedEvent);
    PhInitializeCallback(&PhProcessesUpdatedEvent);

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
        (ULONG)PhSystemBasicInformation.NumberOfProcessors
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

PPH_STRING PhFormatImageVersionInfo(
    __in_opt PPH_STRING FileName,
    __in PPH_IMAGE_VERSION_INFO ImageVersionInfo,
    __in_opt PWSTR Indent,
    __in_opt ULONG LineLimit
    )
{
    PPH_STRING string;
    PH_STRING_BUILDER stringBuilder;
    ULONG indentLength;

    if (Indent)
        indentLength = (ULONG)wcslen(Indent) * sizeof(WCHAR);
    if (LineLimit == 0)
        LineLimit = MAXULONG32;

    PhInitializeStringBuilder(&stringBuilder, 40);

    // File name

    if (!PhIsStringNullOrEmpty(FileName))
    {
        PPH_STRING temp;

        if (Indent) PhStringBuilderAppendEx(&stringBuilder, Indent, indentLength);

        temp = PhEllipsisStringPath(FileName, LineLimit);
        PhStringBuilderAppendEx(&stringBuilder, temp->Buffer, temp->Length);
        PhDereferenceObject(temp);
        PhStringBuilderAppendChar(&stringBuilder, '\n');
    }

    // File description & version

    if (!(
        PhIsStringNullOrEmpty(ImageVersionInfo->FileDescription) &&
        PhIsStringNullOrEmpty(ImageVersionInfo->FileVersion)
        ))
    {
        PPH_STRING tempDescription = NULL;
        PPH_STRING tempVersion = NULL;
        ULONG limitForDescription;
        ULONG limitForVersion;

        if (LineLimit != MAXULONG32)
        {
            limitForVersion = (LineLimit - 1) / 5; // 1/5 space for version (and space character)
            limitForDescription = LineLimit - limitForVersion;
        }
        else
        {
            limitForDescription = MAXULONG32;
            limitForVersion = MAXULONG32;
        }

        if (!PhIsStringNullOrEmpty(ImageVersionInfo->FileDescription))
        {
            tempDescription = PhEllipsisString(
                ImageVersionInfo->FileDescription,
                limitForDescription
                );
        }

        if (!PhIsStringNullOrEmpty(ImageVersionInfo->FileVersion))
        {
            tempVersion = PhEllipsisString(
                ImageVersionInfo->FileVersion,
                limitForVersion
                );
        }

        if (Indent) PhStringBuilderAppendEx(&stringBuilder, Indent, indentLength);

        if (tempDescription)
        {
            PhStringBuilderAppendEx(
                &stringBuilder,
                tempDescription->Buffer,
                tempDescription->Length
                );

            if (tempVersion)
                PhStringBuilderAppendChar(&stringBuilder, ' '); 
        }

        if (tempVersion)
        {
            PhStringBuilderAppendEx(
                &stringBuilder,
                tempVersion->Buffer,
                tempVersion->Length
                );
        }

        if (tempDescription)
            PhDereferenceObject(tempDescription);
        if (tempVersion)
            PhDereferenceObject(tempVersion);

        PhStringBuilderAppendChar(&stringBuilder, '\n');
    }

    // File company

    if (!PhIsStringNullOrEmpty(ImageVersionInfo->CompanyName))
    {
        PPH_STRING temp;

        if (Indent) PhStringBuilderAppendEx(&stringBuilder, Indent, indentLength);

        temp = PhEllipsisString(ImageVersionInfo->CompanyName, LineLimit);
        PhStringBuilderAppendEx(&stringBuilder, temp->Buffer, temp->Length);
        PhDereferenceObject(temp);
        PhStringBuilderAppendChar(&stringBuilder, '\n');
    }

    // Remove the extra newline.
    if (stringBuilder.String->Length != 0)
        PhStringBuilderRemove(&stringBuilder, stringBuilder.String->Length / 2 - 1, 1);

    string = PhReferenceStringBuilderString(&stringBuilder);
    PhDeleteStringBuilder(&stringBuilder);

    return string;
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
    PhInitializeQueuedLock(&processItem->ServiceListLock);

    processItem->ProcessId = ProcessId;

    if (!PH_IS_FAKE_PROCESS_ID(ProcessId))
        PhPrintUInt32(processItem->ProcessIdString, (ULONG)ProcessId);

    // Create the statistics buffers.
    PhInitializeCircularBuffer_FLOAT(&processItem->CpuKernelHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_FLOAT(&processItem->CpuUserHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG64(&processItem->IoReadHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG64(&processItem->IoWriteHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG64(&processItem->IoOtherHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory, PhStatisticsSampleCount);
    //PhInitializeCircularBuffer_SIZE_T(&processItem->WorkingSetHistory, PhStatisticsSampleCount);

    return processItem;
}

VOID PhpProcessItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Object;
    ULONG i;

    PhDeleteCircularBuffer_FLOAT(&processItem->CpuKernelHistory);
    PhDeleteCircularBuffer_FLOAT(&processItem->CpuUserHistory);
    PhDeleteCircularBuffer_ULONG64(&processItem->IoReadHistory);
    PhDeleteCircularBuffer_ULONG64(&processItem->IoWriteHistory);
    PhDeleteCircularBuffer_ULONG64(&processItem->IoOtherHistory);
    PhDeleteCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory);
    //PhDeleteCircularBuffer_SIZE_T(&processItem->WorkingSetHistory);

    for (i = 0; i < processItem->ServiceList->Count; i++)
        PhDereferenceObject(processItem->ServiceList->Items[i]);

    PhDereferenceObject(processItem->ServiceList);

    if (processItem->ProcessName) PhDereferenceObject(processItem->ProcessName);
    if (processItem->FileName) PhDereferenceObject(processItem->FileName);
    if (processItem->CommandLine) PhDereferenceObject(processItem->CommandLine);
    if (processItem->SmallIcon) DestroyIcon(processItem->SmallIcon);
    if (processItem->LargeIcon) DestroyIcon(processItem->LargeIcon);
    PhDeleteImageVersionInfo(&processItem->VersionInfo);
    if (processItem->UserName) PhDereferenceObject(processItem->UserName);
    if (processItem->VerifySignerName) PhDereferenceObject(processItem->VerifySignerName);

    if (processItem->QueryHandle) NtClose(processItem->QueryHandle);
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

__assumeLocked PPH_PROCESS_ITEM PhpLookupProcessItem(
    __in HANDLE ProcessId
    )
{
    PH_PROCESS_ITEM lookupProcessItem;
    PPH_PROCESS_ITEM lookupProcessItemPtr = &lookupProcessItem;
    PPH_PROCESS_ITEM *processItemPtr;

    // Construct a temporary process item for the lookup.
    lookupProcessItem.ProcessId = ProcessId;

    processItemPtr = (PPH_PROCESS_ITEM *)PhGetHashtableEntry(
        PhProcessHashtable,
        &lookupProcessItemPtr
        );

    if (processItemPtr)
        return *processItemPtr;
    else
        return NULL;
}

PPH_PROCESS_ITEM PhReferenceProcessItem(
    __in HANDLE ProcessId
    )
{
    PPH_PROCESS_ITEM processItem;

    PhAcquireQueuedLockShared(&PhProcessHashtableLock);

    processItem = PhpLookupProcessItem(ProcessId);

    if (processItem)
        PhReferenceObject(processItem);

    PhReleaseQueuedLockShared(&PhProcessHashtableLock);

    return processItem;
}

__assumeLocked VOID PhpRemoveProcessItem(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PhRemoveHashtableEntry(PhProcessHashtable, &ProcessItem);
    PhDereferenceObject(ProcessItem);
}

PPH_STRING PhGetClientIdName(
    __in PCLIENT_ID ClientId
    )
{
    PPH_STRING name;
    PPH_STRING processName = NULL;
    PPH_PROCESS_ITEM processItem;

    processItem = PhReferenceProcessItem(ClientId->UniqueProcess);

    if (processItem)
    {
        processName = processItem->ProcessName;
        PhReferenceObject(processName);
        PhDereferenceObject(processItem);
    }

    if (ClientId->UniqueThread)
    {
        if (processName)
        {
            name = PhFormatString(L"%s (%u): %u", processName->Buffer,
                (ULONG)ClientId->UniqueProcess, (ULONG)ClientId->UniqueThread);
        }
        else
        {
            name = PhFormatString(L"Non-existent process (%u): %u",
                (ULONG)ClientId->UniqueProcess, (ULONG)ClientId->UniqueThread);
        }
    }
    else
    {
        if (processName)
            name = PhFormatString(L"%s (%u)", processName->Buffer, (ULONG)ClientId->UniqueProcess);
        else
            name = PhFormatString(L"Non-existent process (%u)", (ULONG)ClientId->UniqueProcess);
    }

    if (processName)
        PhDereferenceObject(processName);

    return name;
}

VOID PhpProcessQueryStage1(
    __inout PPH_PROCESS_QUERY_S1_DATA Data
    )
{
    NTSTATUS status;
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;
    HANDLE processId = processItem->ProcessId;
    HANDLE processHandleLimited = NULL;

    PhOpenProcess(&processHandleLimited, ProcessQueryAccess, processId);

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

    // POSIX, command line
    {
        HANDLE processHandle;

        status = PhOpenProcess(
            &processHandle,
            ProcessQueryAccess | PROCESS_VM_READ,
            processId
            );

        if (NT_SUCCESS(status))
        {
            BOOLEAN isPosix = FALSE;
            PPH_STRING commandLine;
            ULONG i;

            status = PhGetProcessIsPosix(processHandle, &isPosix);
            Data->IsPosix = isPosix;

            if (!NT_SUCCESS(status) || !isPosix)
            {
                status = PhGetProcessCommandLine(processHandle, &commandLine);

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
                status = PhGetProcessPosixCommandLine(processHandle, &commandLine);
            }

            if (NT_SUCCESS(status))
            {
                Data->CommandLine = commandLine;
            }

            NtClose(processHandle);
        }
    }

    // Token information
    if (processHandleLimited)
    {
        if (WINDOWS_HAS_UAC)
        {
            HANDLE tokenHandle;

            status = PhOpenProcessToken(&tokenHandle, TOKEN_QUERY, processHandleLimited);

            if (NT_SUCCESS(status))
            {
                // Elevation
                if (NT_SUCCESS(PhGetTokenElevationType(
                    tokenHandle,
                    &Data->ElevationType
                    )))
                {
                    Data->IsElevated = Data->ElevationType == TokenElevationTypeFull;
                }

                // Integrity
                PhGetTokenIntegrityLevel(
                    tokenHandle,
                    &Data->IntegrityLevel,
                    &Data->IntegrityString
                    );

                NtClose(tokenHandle);
            }
        }

#ifdef _M_X64
        // WOW64
        PhGetProcessIsWow64(processHandleLimited, &Data->IsWow64);
#endif
    }

    // Job
    if (processHandleLimited)
    {
        if (PhKphHandle)
        {
            HANDLE jobHandle = NULL;

            status = KphOpenProcessJob(
                PhKphHandle,
                &jobHandle,
                processHandleLimited,
                JOB_OBJECT_QUERY
                );

            if (NT_SUCCESS(status) && status != STATUS_PROCESS_NOT_IN_JOB && jobHandle)
            {
                JOBOBJECT_BASIC_LIMIT_INFORMATION basicLimits;

                Data->IsInJob = TRUE;

                PhGetHandleInformation(
                    NtCurrentProcess(),
                    jobHandle,
                    -1,
                    NULL,
                    NULL,
                    NULL,
                    &Data->JobName
                    );

                // Process Explorer only recognizes processes as being in jobs if they 
                // don't have the silent-breakaway-OK limit as their only limit.
                // Emulate this behaviour.
                if (NT_SUCCESS(PhGetJobBasicLimits(jobHandle, &basicLimits)))
                {
                    Data->IsInSignificantJob =
                        basicLimits.LimitFlags != JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
                }

                NtClose(jobHandle);
            }
        }
        else
        {
            // KProcessHacker not available. We can determine if the process is 
            // in a job, but we can't get a handle to the job.

            status = NtIsProcessInJob(processHandleLimited, NULL);

            if (NT_SUCCESS(status))
                Data->IsInJob = status == STATUS_PROCESS_IN_JOB;
        }
    }

    // Console host process
    if (WINDOWS_HAS_CONSOLE_HOST)
    {
        PhGetProcessConsoleHostProcessId(processHandleLimited, &Data->ConsoleHostProcessId);
    }

    if (processHandleLimited)
        NtClose(processHandleLimited);

    PhpQueueProcessQueryStage2(processItem);
}

VOID PhpProcessQueryStage2(
    __inout PPH_PROCESS_QUERY_S2_DATA Data
    )
{
    NTSTATUS status;
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;
    HANDLE processId = processItem->ProcessId;

    PhGetProcessIsDotNet(processId, &Data->IsDotNet);

    if (processItem->FileName)
    {
        Data->VerifyResult = PhVerifyFile(
            processItem->FileName->Buffer,
            &Data->VerifySignerName
            );

        status = PhIsExecutablePacked(
            processItem->FileName->Buffer,
            &Data->IsPacked,
            &Data->ImportModules,
            &Data->ImportFunctions
            );

        // If we got an image-related error, the image is packed.
        if (
            status == STATUS_INVALID_IMAGE_NOT_MZ ||
            status == STATUS_INVALID_IMAGE_FORMAT ||
            status == STATUS_ACCESS_VIOLATION
            )
        {
            Data->IsPacked = TRUE;
            Data->ImportModules = -1;
            Data->ImportFunctions = -1;
        }
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

    RtlInterlockedPushEntrySList(&PhProcessQueryDataListHead, &data->Header.ListEntry);

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

    RtlInterlockedPushEntrySList(&PhProcessQueryDataListHead, &data->Header.ListEntry);

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

    processItem->CommandLine = Data->CommandLine;
    processItem->SmallIcon = Data->SmallIcon;
    processItem->LargeIcon = Data->LargeIcon;
    memcpy(&processItem->VersionInfo, &Data->VersionInfo, sizeof(PH_IMAGE_VERSION_INFO));
    processItem->ElevationType = Data->ElevationType;
    processItem->IntegrityLevel = Data->IntegrityLevel;
    processItem->JobName = Data->JobName;
    processItem->ConsoleHostProcessId = Data->ConsoleHostProcessId;
    processItem->IsElevated = Data->IsElevated;
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
    HANDLE processHandle = NULL;

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

    PhOpenProcess(&processHandle, ProcessQueryAccess, ProcessItem->ProcessId);

    // Process information
    {
        // If we're dealing with System (PID 4), we need to get the 
        // kernel file name. Otherwise, get the image file name.

        if (ProcessItem->ProcessId != SYSTEM_PROCESS_ID)
        {
            PPH_STRING fileName;

            if (WINDOWS_HAS_IMAGE_FILE_NAME_BY_PROCESS_ID)
                status = PhGetProcessImageFileNameByProcessId(ProcessItem->ProcessId, &fileName);
            else
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

    // Token-related information
    if (
        processHandle &&
        !(WindowsVersion <= WINDOWS_XP && ProcessItem->ProcessId == SYSTEM_PROCESS_ID)
        )
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
                    ProcessItem->UserName = PhGetSidFullName(user->User.Sid, TRUE, NULL);
                    PhFree(user);
                }
            }

            NtClose(tokenHandle);
        }
    }
    else
    {
        if (ProcessItem->ProcessId == SYSTEM_IDLE_PROCESS_ID)
            ProcessItem->UserName = PhDuplicateString(PhLocalSystemName);
        else if (ProcessItem->ProcessId == SYSTEM_PROCESS_ID) // System token can't be opened on XP
            ProcessItem->UserName = PhDuplicateString(PhLocalSystemName);
    }

    if (!ProcessItem->UserName && WindowsVersion <= WINDOWS_XP)
    {
        // In some cases we can get the user SID using WTS (only works on XP and below).

        if (!PhpWtsProcesses)
        {
            WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &PhpWtsProcesses, &PhpWtsNumberOfProcesses);
        }

        if (PhpWtsProcesses)
        {
            ULONG i;

            for (i = 0; i < PhpWtsNumberOfProcesses; i++)
            {
                if (PhpWtsProcesses[i].ProcessId == (ULONG)ProcessItem->ProcessId)
                    ProcessItem->UserName = PhGetSidFullName(PhpWtsProcesses[i].pUserSid, TRUE, NULL);
            }
        }
    }

    NtClose(processHandle);
}

FORCEINLINE VOID PhpUpdateDynamicInfoProcessItem(
    __inout PPH_PROCESS_ITEM ProcessItem,
    __in PSYSTEM_PROCESS_INFORMATION Process
    )
{
    ProcessItem->BasePriority = Process->BasePriority;
    ProcessItem->KernelTime = Process->KernelTime;
    ProcessItem->UserTime = Process->UserTime;
    ProcessItem->NumberOfHandles = Process->HandleCount;
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
        (ULONG)PhSystemBasicInformation.NumberOfProcessors,
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

PWSTR PhGetProcessPriorityClassWin32String(
    __in ULONG PriorityClassWin32
    )
{
    switch (PriorityClassWin32)
    {
    case REALTIME_PRIORITY_CLASS:
        return L"Real Time";
    case HIGH_PRIORITY_CLASS:
        return L"High";
    case ABOVE_NORMAL_PRIORITY_CLASS:
        return L"Above Normal";
    case NORMAL_PRIORITY_CLASS:
        return L"Normal";
    case BELOW_NORMAL_PRIORITY_CLASS:
        return L"Below Normal";
    case IDLE_PRIORITY_CLASS:
        return L"Idle";
    default:
        return L"Unknown";
    }
}

VOID PhProcessProviderUpdate(
    __in PVOID Object
    )
{
    static ULONG runCount = 0;
    static PPH_LIST pids = NULL;

    // Note about locking:
    // Since this is the only function that is allowed to 
    // modify the process hashtable, locking is not needed 
    // for shared accesses. However, exclusive accesses 
    // need locking.

    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;

    ULONG64 sysTotalTime;

    // Pre-update tasks

    if (runCount % 2 == 0)
        PhRefreshDosDevicePrefixes();

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

    if (!pids)
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
            PhAcquireQueuedLockExclusive(&PhProcessHashtableLock);

            for (i = 0; i < processesToRemove->Count; i++)
            {
                PhpRemoveProcessItem((PPH_PROCESS_ITEM)processesToRemove->Items[i]);
            }

            PhReleaseQueuedLockExclusive(&PhProcessHashtableLock);
            PhDereferenceObject(processesToRemove);
        }
    }

    // Go through the queued process query data.
    if (RtlQueryDepthSList(&PhProcessQueryDataListHead) != 0)
    {
        PSLIST_ENTRY entry;
        PPH_PROCESS_QUERY_DATA data;

        while (entry = RtlInterlockedPopEntrySList(&PhProcessQueryDataListHead))
        {
            data = CONTAINING_RECORD(entry, PH_PROCESS_QUERY_DATA, ListEntry);

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
        }
    }

    // Look for new processes and update existing ones.
    process = PH_FIRST_PROCESS(processes);

    while (process)
    {
        PPH_PROCESS_ITEM processItem;

        processItem = PhpLookupProcessItem(process->UniqueProcessId);

        if (!processItem)
        {
            // Create the process item and fill in basic information.
            processItem = PhCreateProcessItem(process->UniqueProcessId);
            PhpFillProcessItem(processItem, process);

            // Open a handle to the process for later usage.
            PhOpenProcess(&processItem->QueryHandle, PROCESS_QUERY_INFORMATION, processItem->ProcessId);

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

            PhpUpdateDynamicInfoProcessItem(processItem, process);

            // Initialize the deltas.
            PhUpdateDelta(&processItem->CpuKernelDelta, process->KernelTime.QuadPart);
            PhUpdateDelta(&processItem->CpuUserDelta, process->UserTime.QuadPart);
            PhUpdateDelta(&processItem->IoReadDelta, process->ReadTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoWriteDelta, process->WriteTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoOtherDelta, process->OtherTransferCount.QuadPart);

            // Update VM and I/O statistics.
            processItem->VmCounters = *(PVM_COUNTERS_EX)&process->PeakVirtualSize;
            processItem->IoCounters = *(PIO_COUNTERS)&process->ReadOperationCount;

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
            PhAcquireQueuedLockExclusive(&PhProcessHashtableLock);
            PhAddHashtableEntry(PhProcessHashtable, &processItem);
            PhReleaseQueuedLockExclusive(&PhProcessHashtableLock);

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
            FLOAT kernelCpuUsage;
            FLOAT userCpuUsage;
            FLOAT newCpuUsage;

            // Update the deltas.
            PhUpdateDelta(&processItem->CpuKernelDelta, process->KernelTime.QuadPart);
            PhUpdateDelta(&processItem->CpuUserDelta, process->UserTime.QuadPart);
            PhUpdateDelta(&processItem->IoReadDelta, process->ReadTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoWriteDelta, process->WriteTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoOtherDelta, process->OtherTransferCount.QuadPart);

            PhCircularBufferAdd_ULONG64(&processItem->IoReadHistory, processItem->IoReadDelta.Delta);
            PhCircularBufferAdd_ULONG64(&processItem->IoWriteHistory, processItem->IoWriteDelta.Delta);
            PhCircularBufferAdd_ULONG64(&processItem->IoOtherHistory, processItem->IoOtherDelta.Delta);

            PhCircularBufferAdd_SIZE_T(&processItem->PrivateBytesHistory, processItem->VmCounters.PagefileUsage);
            //PhCircularBufferAdd_SIZE_T(&processItem->WorkingSetHistory, processItem->VmCounters.WorkingSetSize);

            PhpUpdateDynamicInfoProcessItem(processItem, process);

            // Update VM and I/O statistics.
            processItem->VmCounters = *(PVM_COUNTERS_EX)&process->PeakVirtualSize;
            processItem->IoCounters = *(PIO_COUNTERS)&process->ReadOperationCount;

            if (processItem->JustProcessed)
            {
                processItem->JustProcessed = FALSE;
                modified = TRUE;
            }

            kernelCpuUsage = (FLOAT)processItem->CpuKernelDelta.Delta / sysTotalTime;
            userCpuUsage = (FLOAT)processItem->CpuUserDelta.Delta / sysTotalTime;

            processItem->CpuKernelUsage = kernelCpuUsage;
            processItem->CpuUserUsage = userCpuUsage;
            PhCircularBufferAdd_FLOAT(&processItem->CpuKernelHistory, kernelCpuUsage);
            PhCircularBufferAdd_FLOAT(&processItem->CpuUserHistory, userCpuUsage);

            newCpuUsage = kernelCpuUsage + userCpuUsage;

            if (processItem->CpuUsage != newCpuUsage)
            {
                processItem->CpuUsage = newCpuUsage;

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

            // Debugged
            {
                BOOLEAN isBeingDebugged;

                if (NT_SUCCESS(PhGetProcessIsBeingDebugged(
                    processItem->QueryHandle,
                    &isBeingDebugged
                    )) && processItem->IsBeingDebugged != isBeingDebugged)
                {
                    processItem->IsBeingDebugged = isBeingDebugged;
                    modified = TRUE;
                }
            }

            // Suspended
            {
                BOOLEAN isSuspended;

                isSuspended = PhGetProcessIsSuspended(process);

                if (processItem->IsSuspended != isSuspended)
                {
                    processItem->IsSuspended = isSuspended;
                    modified = TRUE;
                }
            }

            if (modified)
            {
                PhInvokeCallback(&PhProcessModifiedEvent, processItem);
            }

            // No reference added by PhpLookupProcessItem.
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

    PhClearList(pids);
    PhFree(processes);

    if (PhpWtsProcesses)
    {
        WTSFreeMemory(PhpWtsProcesses);
        PhpWtsProcesses = NULL;
    }

    PhInvokeCallback(&PhProcessesUpdatedEvent, NULL);
    runCount++;
}
