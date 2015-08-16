/*
 * Process Hacker -
 *   item tooltips
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

#include <phapp.h>
#include <phplug.h>
#include <verify.h>
#define CINTERFACE
#define COBJMACROS
#include <taskschd.h>

VOID PhpFillUmdfDrivers(
    _In_ PPH_PROCESS_ITEM Process,
    _Inout_ PPH_STRING_BUILDER Drivers
    );

VOID PhpFillRunningTasks(
    _In_ PPH_PROCESS_ITEM Process,
    _Inout_ PPH_STRING_BUILDER Tasks
    );

static PH_STRINGREF StandardIndent = PH_STRINGREF_INIT(L"    ");

VOID PhpAppendStringWithLineBreaks(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ PPH_STRINGREF String,
    _In_ ULONG CharactersPerLine,
    _In_opt_ PPH_STRINGREF IndentAfterFirstLine
    )
{
    PH_STRINGREF line;
    SIZE_T bytesPerLine;
    BOOLEAN afterFirstLine;
    SIZE_T bytesToAppend;

    line = *String;
    bytesPerLine = CharactersPerLine * sizeof(WCHAR);
    afterFirstLine = FALSE;

    while (line.Length != 0)
    {
        bytesToAppend = line.Length;

        if (bytesToAppend > bytesPerLine)
            bytesToAppend = bytesPerLine;

        if (afterFirstLine)
        {
            PhAppendCharStringBuilder(StringBuilder, '\n');

            if (IndentAfterFirstLine)
                PhAppendStringBuilder(StringBuilder, IndentAfterFirstLine);
        }

        PhAppendStringBuilderEx(StringBuilder, line.Buffer, bytesToAppend);
        afterFirstLine = TRUE;
        PhSkipStringRef(&line, bytesToAppend);
    }
}

static int __cdecl ServiceForTooltipCompare(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PPH_SERVICE_ITEM serviceItem1 = *(PPH_SERVICE_ITEM *)elem1;
    PPH_SERVICE_ITEM serviceItem2 = *(PPH_SERVICE_ITEM *)elem2;

    return PhCompareString(serviceItem1->Name, serviceItem2->Name, TRUE);
}

PPH_STRING PhGetProcessTooltipText(
    _In_ PPH_PROCESS_ITEM Process,
    _Out_opt_ PULONG ValidToTickCount
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG validForMs = 60 * 60 * 1000; // 1 hour
    PPH_STRING tempString;
    PH_KNOWN_PROCESS_TYPE knownProcessType = UnknownProcessType;

    PhInitializeStringBuilder(&stringBuilder, 200);

    // Command line

    if (Process->CommandLine)
    {
        tempString = PhEllipsisString(Process->CommandLine, 100 * 10);

        // This is necessary because the tooltip control seems to use some kind of O(n^9999) word-wrapping
        // algorithm.
        PhpAppendStringWithLineBreaks(&stringBuilder, &tempString->sr, 100, NULL);
        PhAppendCharStringBuilder(&stringBuilder, '\n');

        PhDereferenceObject(tempString);
    }

    // File information

    tempString = PhFormatImageVersionInfo(
        Process->FileName,
        &Process->VersionInfo,
        &StandardIndent,
        0
        );

    if (!PhIsNullOrEmptyString(tempString))
    {
        PhAppendStringBuilder2(&stringBuilder, L"File:\n");
        PhAppendStringBuilder(&stringBuilder, &tempString->sr);
        PhAppendCharStringBuilder(&stringBuilder, '\n');
    }

    if (tempString)
        PhDereferenceObject(tempString);

    // Known command line information

    if (Process->QueryHandle)
        PhGetProcessKnownType(Process->QueryHandle, &knownProcessType);

    if (Process->CommandLine && Process->QueryHandle)
    {
        PH_KNOWN_PROCESS_COMMAND_LINE knownCommandLine;

        if (knownProcessType != UnknownProcessType && PhaGetProcessKnownCommandLine(
            Process->CommandLine,
            knownProcessType,
            &knownCommandLine
            ))
        {
            switch (knownProcessType & KnownProcessTypeMask)
            {
            case ServiceHostProcessType:
                PhAppendStringBuilder2(&stringBuilder, L"Service group name:\n    ");
                PhAppendStringBuilder(&stringBuilder, &knownCommandLine.ServiceHost.GroupName->sr);
                PhAppendCharStringBuilder(&stringBuilder, '\n');
                break;
            case RunDllAsAppProcessType:
                {
                    PH_IMAGE_VERSION_INFO versionInfo;

                    if (PhInitializeImageVersionInfo(
                        &versionInfo,
                        knownCommandLine.RunDllAsApp.FileName->Buffer
                        ))
                    {
                        tempString = PhFormatImageVersionInfo(
                            knownCommandLine.RunDllAsApp.FileName,
                            &versionInfo,
                            &StandardIndent,
                            0
                            );

                        if (!PhIsNullOrEmptyString(tempString))
                        {
                            PhAppendStringBuilder2(&stringBuilder, L"Run DLL target file:\n");
                            PhAppendStringBuilder(&stringBuilder, &tempString->sr);
                            PhAppendCharStringBuilder(&stringBuilder, '\n');
                        }

                        if (tempString)
                            PhDereferenceObject(tempString);

                        PhDeleteImageVersionInfo(&versionInfo);
                    }
                }
                break;
            case ComSurrogateProcessType:
                {
                    PH_IMAGE_VERSION_INFO versionInfo;
                    PPH_STRING guidString;

                    PhAppendStringBuilder2(&stringBuilder, L"COM target:\n");

                    if (knownCommandLine.ComSurrogate.Name)
                    {
                        PhAppendStringBuilder(&stringBuilder, &StandardIndent);
                        PhAppendStringBuilder(&stringBuilder, &knownCommandLine.ComSurrogate.Name->sr);
                        PhAppendCharStringBuilder(&stringBuilder, '\n');
                    }

                    if (guidString = PhFormatGuid(&knownCommandLine.ComSurrogate.Guid))
                    {
                        PhAppendStringBuilder(&stringBuilder, &StandardIndent);
                        PhAppendStringBuilder(&stringBuilder, &guidString->sr);
                        PhDereferenceObject(guidString);
                        PhAppendCharStringBuilder(&stringBuilder, '\n');
                    }

                    if (knownCommandLine.ComSurrogate.FileName && PhInitializeImageVersionInfo(
                        &versionInfo,
                        knownCommandLine.ComSurrogate.FileName->Buffer
                        ))
                    {
                        tempString = PhFormatImageVersionInfo(
                            knownCommandLine.ComSurrogate.FileName,
                            &versionInfo,
                            &StandardIndent,
                            0
                            );

                        if (!PhIsNullOrEmptyString(tempString))
                        {
                            PhAppendStringBuilder2(&stringBuilder, L"COM target file:\n");
                            PhAppendStringBuilder(&stringBuilder, &tempString->sr);
                            PhAppendCharStringBuilder(&stringBuilder, '\n');
                        }

                        if (tempString)
                            PhDereferenceObject(tempString);

                        PhDeleteImageVersionInfo(&versionInfo);
                    }
                }
                break;
            }
        }
    }

    // Services

    if (Process->ServiceList && Process->ServiceList->Count != 0)
    {
        ULONG enumerationKey = 0;
        PPH_SERVICE_ITEM serviceItem;
        PPH_LIST serviceList;
        ULONG i;

        // Copy the service list into our own list so we can sort it.

        serviceList = PhCreateList(Process->ServiceList->Count);

        PhAcquireQueuedLockShared(&Process->ServiceListLock);

        while (PhEnumPointerList(
            Process->ServiceList,
            &enumerationKey,
            &serviceItem
            ))
        {
            PhReferenceObject(serviceItem);
            PhAddItemList(serviceList, serviceItem);
        }

        PhReleaseQueuedLockShared(&Process->ServiceListLock);

        qsort(serviceList->Items, serviceList->Count, sizeof(PPH_SERVICE_ITEM), ServiceForTooltipCompare);

        PhAppendStringBuilder2(&stringBuilder, L"Services:\n");

        // Add the services.
        for (i = 0; i < serviceList->Count; i++)
        {
            serviceItem = serviceList->Items[i];

            PhAppendStringBuilder(&stringBuilder, &StandardIndent);
            PhAppendStringBuilder(&stringBuilder, &serviceItem->Name->sr);
            PhAppendStringBuilder2(&stringBuilder, L" (");
            PhAppendStringBuilder(&stringBuilder, &serviceItem->DisplayName->sr);
            PhAppendStringBuilder2(&stringBuilder, L")\n");
        }

        PhDereferenceObjects(serviceList->Items, serviceList->Count);
        PhDereferenceObject(serviceList);
    }

    // Tasks, Drivers
    switch (knownProcessType & KnownProcessTypeMask)
    {
    case TaskHostProcessType:
        {
            PH_STRING_BUILDER tasks;

            PhInitializeStringBuilder(&tasks, 40);

            PhpFillRunningTasks(Process, &tasks);

            if (tasks.String->Length != 0)
            {
                PhAppendStringBuilder2(&stringBuilder, L"Tasks:\n");
                PhAppendStringBuilder(&stringBuilder, &tasks.String->sr);
            }

            PhDeleteStringBuilder(&tasks);
        }
        break;
    case UmdfHostProcessType:
        {
            PH_STRING_BUILDER drivers;

            PhInitializeStringBuilder(&drivers, 40);

            PhpFillUmdfDrivers(Process, &drivers);

            if (drivers.String->Length != 0)
            {
                PhAppendStringBuilder2(&stringBuilder, L"Drivers:\n");
                PhAppendStringBuilder(&stringBuilder, &drivers.String->sr);
            }

            PhDeleteStringBuilder(&drivers);

            validForMs = 10 * 1000; // 10 seconds
        }
        break;
    }

    // Plugin
    if (PhPluginsEnabled)
    {
        PH_PLUGIN_GET_TOOLTIP_TEXT getTooltipText;

        getTooltipText.Parameter = Process;
        getTooltipText.StringBuilder = &stringBuilder;
        getTooltipText.ValidForMs = validForMs;

        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackGetProcessTooltipText), &getTooltipText);
        validForMs = getTooltipText.ValidForMs;
    }

    // Notes

    {
        PH_STRING_BUILDER notes;

        PhInitializeStringBuilder(&notes, 40);

        if (Process->FileName)
        {
            if (Process->VerifyResult == VrTrusted)
            {
                if (!PhIsNullOrEmptyString(Process->VerifySignerName))
                    PhAppendFormatStringBuilder(&notes, L"    Signer: %s\n", Process->VerifySignerName->Buffer);
                else
                    PhAppendStringBuilder2(&notes, L"    Signed.\n");
            }
            else if (Process->VerifyResult == VrUnknown)
            {
                // Nothing
            }
            else if (Process->VerifyResult != VrNoSignature)
            {
                PhAppendStringBuilder2(&notes, L"    Signature invalid.\n");
            }
        }

        if (Process->IsPacked)
        {
            PhAppendFormatStringBuilder(
                &notes,
                L"    Image is probably packed (%u imports over %u modules).\n",
                Process->ImportFunctions,
                Process->ImportModules
                );
        }

        if ((ULONG_PTR)Process->ConsoleHostProcessId & ~3)
        {
            CLIENT_ID clientId;
            PWSTR description = L"Console host";
            PPH_STRING clientIdString;

            clientId.UniqueProcess = (HANDLE)((ULONG_PTR)Process->ConsoleHostProcessId & ~3);
            clientId.UniqueThread = NULL;

            if ((ULONG_PTR)Process->ConsoleHostProcessId & 2)
                description = L"Console application";

            clientIdString = PhGetClientIdName(&clientId);
            PhAppendFormatStringBuilder(&notes, L"    %s: %s\n", description, clientIdString->Buffer);
            PhDereferenceObject(clientIdString);
        }

        if (Process->PackageFullName)
        {
            PhAppendFormatStringBuilder(&notes, L"    Package name: %s\n", Process->PackageFullName->Buffer);
        }

        if (Process->IsDotNet)
            PhAppendStringBuilder2(&notes, L"    Process is managed (.NET).\n");
        if (Process->IsElevated)
            PhAppendStringBuilder2(&notes, L"    Process is elevated.\n");
        if (Process->IsImmersive)
            PhAppendStringBuilder2(&notes, L"    Process is a Modern UI app.\n");
        if (Process->IsInJob)
            PhAppendStringBuilder2(&notes, L"    Process is in a job.\n");
        if (Process->IsPosix)
            PhAppendStringBuilder2(&notes, L"    Process is POSIX.\n");
        if (Process->IsWow64)
            PhAppendStringBuilder2(&notes, L"    Process is 32-bit (WOW64).\n");

        if (notes.String->Length != 0)
        {
            PhAppendStringBuilder2(&stringBuilder, L"Notes:\n");
            PhAppendStringBuilder(&stringBuilder, &notes.String->sr);
        }

        PhDeleteStringBuilder(&notes);
    }

    if (ValidToTickCount)
        *ValidToTickCount = GetTickCount() + validForMs;

    // Remove the trailing newline.
    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}

VOID PhpFillUmdfDrivers(
    _In_ PPH_PROCESS_ITEM Process,
    _Inout_ PPH_STRING_BUILDER Drivers
    )
{
    static PH_STRINGREF activeDevices = PH_STRINGREF_INIT(L"ACTIVE_DEVICES");
    static PH_STRINGREF currentControlSetEnum = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Enum\\");

    HANDLE processHandle;
    ULONG flags = 0;
    PVOID environment;
    ULONG environmentLength;
    ULONG enumerationKey;
    PH_ENVIRONMENT_VARIABLE variable;

    if (!NT_SUCCESS(PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        Process->ProcessId
        )))
        return;

#ifdef _WIN64
    // Just in case.
    if (Process->IsWow64)
        flags |= PH_GET_PROCESS_ENVIRONMENT_WOW64;
#endif

    if (NT_SUCCESS(PhGetProcessEnvironment(
        processHandle,
        flags,
        &environment,
        &environmentLength
        )))
    {
        enumerationKey = 0;

        while (PhEnumProcessEnvironmentVariables(environment, environmentLength, &enumerationKey, &variable))
        {
            PH_STRINGREF part;
            PH_STRINGREF remainingPart;

            if (!PhEqualStringRef(&variable.Name, &activeDevices, TRUE))
                continue;

            remainingPart = variable.Value;

            while (remainingPart.Length != 0)
            {
                PhSplitStringRefAtChar(&remainingPart, ';', &part, &remainingPart);

                if (part.Length != 0)
                {
                    HANDLE driverKeyHandle;
                    PPH_STRING driverKeyPath;

                    driverKeyPath = PhConcatStringRef2(&currentControlSetEnum, &part);

                    if (NT_SUCCESS(PhOpenKey(
                        &driverKeyHandle,
                        KEY_READ,
                        PH_KEY_LOCAL_MACHINE,
                        &driverKeyPath->sr,
                        0
                        )))
                    {
                        PPH_STRING deviceDesc;
                        PH_STRINGREF deviceName;
                        PPH_STRING hardwareId;

                        if (deviceDesc = PhQueryRegistryString(driverKeyHandle, L"DeviceDesc"))
                        {
                            PH_STRINGREF firstPart;
                            PH_STRINGREF secondPart;

                            if (PhSplitStringRefAtLastChar(&deviceDesc->sr, ';', &firstPart, &secondPart))
                                deviceName = secondPart;
                            else
                                deviceName = deviceDesc->sr;
                        }
                        else
                        {
                            PhInitializeStringRef(&deviceName, L"Unknown Device");
                        }

                        hardwareId = PhQueryRegistryString(driverKeyHandle, L"HardwareID");

                        PhAppendStringBuilder(Drivers, &StandardIndent);
                        PhAppendStringBuilder(Drivers, &deviceName);

                        if (hardwareId)
                        {
                            PhTrimToNullTerminatorString(hardwareId);

                            if (hardwareId->Length != 0)
                            {
                                PhAppendStringBuilder2(Drivers, L" (");
                                PhAppendStringBuilder(Drivers, &hardwareId->sr);
                                PhAppendCharStringBuilder(Drivers, ')');
                            }
                        }

                        PhAppendCharStringBuilder(Drivers, '\n');

                        PhClearReference(&hardwareId);
                        PhClearReference(&deviceDesc);
                        NtClose(driverKeyHandle);
                    }

                    PhDereferenceObject(driverKeyPath);
                }
            }
        }

        PhFreePage(environment);
    }

    NtClose(processHandle);
}

VOID PhpFillRunningTasks(
    _In_ PPH_PROCESS_ITEM Process,
    _Inout_ PPH_STRING_BUILDER Tasks
    )
{
    static CLSID CLSID_TaskScheduler_I = { 0x0f87369f, 0xa4e5, 0x4cfc, { 0xbd, 0x3e, 0x73, 0xe6, 0x15, 0x45, 0x72, 0xdd } };
    static IID IID_ITaskService_I = { 0x2faba4c7, 0x4da9, 0x4013, { 0x96, 0x97, 0x20, 0xcc, 0x3f, 0xd4, 0x0f, 0x85 } };

    ITaskService *taskService;

    if (SUCCEEDED(CoCreateInstance(
        &CLSID_TaskScheduler_I,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_ITaskService_I,
        &taskService
        )))
    {
        VARIANT empty = { 0 };

        if (SUCCEEDED(ITaskService_Connect(taskService, empty, empty, empty, empty)))
        {
            IRunningTaskCollection *runningTasks;

            if (SUCCEEDED(ITaskService_GetRunningTasks(
                taskService,
                TASK_ENUM_HIDDEN,
                &runningTasks
                )))
            {
                LONG count;
                LONG i;
                VARIANT index;

                index.vt = VT_INT;

                if (SUCCEEDED(IRunningTaskCollection_get_Count(runningTasks, &count)))
                {
                    for (i = 1; i <= count; i++) // collections are 1-based
                    {
                        IRunningTask *runningTask;

                        index.lVal = i;

                        if (SUCCEEDED(IRunningTaskCollection_get_Item(runningTasks, index, &runningTask)))
                        {
                            ULONG pid;
                            BSTR action = NULL;
                            BSTR path = NULL;

                            if (
                                SUCCEEDED(IRunningTask_get_EnginePID(runningTask, &pid)) &&
                                pid == HandleToUlong(Process->ProcessId)
                                )
                            {
                                IRunningTask_get_CurrentAction(runningTask, &action);
                                IRunningTask_get_Path(runningTask, &path);

                                PhAppendStringBuilder(Tasks, &StandardIndent);
                                PhAppendStringBuilder2(Tasks, action ? action : L"Unknown Action");
                                PhAppendStringBuilder2(Tasks, L" (");
                                PhAppendStringBuilder2(Tasks, path ? path : L"Unknown Path");
                                PhAppendStringBuilder2(Tasks, L")\n");

                                if (action)
                                    SysFreeString(action);
                                if (path)
                                    SysFreeString(path);
                            }

                            IRunningTask_Release(runningTask);
                        }
                    }
                }

                IRunningTaskCollection_Release(runningTasks);
            }
        }

        ITaskService_Release(taskService);
    }
}

PPH_STRING PhGetServiceTooltipText(
    _In_ PPH_SERVICE_ITEM Service
    )
{
    PH_STRING_BUILDER stringBuilder;
    SC_HANDLE serviceHandle;

    PhInitializeStringBuilder(&stringBuilder, 200);

    if (serviceHandle = PhOpenService(Service->Name->Buffer, SERVICE_QUERY_CONFIG))
    {
        PPH_STRING fileName;
        PPH_STRING description;

        // File information

        if (fileName = PhGetServiceRelevantFileName(&Service->Name->sr, serviceHandle))
        {
            PH_IMAGE_VERSION_INFO versionInfo;
            PPH_STRING versionInfoText;

            if (PhInitializeImageVersionInfo(
                &versionInfo,
                fileName->Buffer
                ))
            {
                versionInfoText = PhFormatImageVersionInfo(
                    fileName,
                    &versionInfo,
                    &StandardIndent,
                    0
                    );

                if (!PhIsNullOrEmptyString(versionInfoText))
                {
                    PhAppendStringBuilder2(&stringBuilder, L"File:\n");
                    PhAppendStringBuilder(&stringBuilder, &versionInfoText->sr);
                    PhAppendCharStringBuilder(&stringBuilder, '\n');
                }

                PhClearReference(&versionInfoText);
                PhDeleteImageVersionInfo(&versionInfo);
            }

            PhDereferenceObject(fileName);
        }

        // Description

        if (description = PhGetServiceDescription(serviceHandle))
        {
            PhAppendStringBuilder2(&stringBuilder, L"Description:\n    ");
            PhAppendStringBuilder(&stringBuilder, &description->sr);
            PhAppendCharStringBuilder(&stringBuilder, '\n');
            PhDereferenceObject(description);
        }

        CloseServiceHandle(serviceHandle);
    }

    // Remove the trailing newline.
    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}
