/*
 * Process Hacker - 
 *   item tooltips
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

#include <phapp.h>
#define CINTERFACE
#define COBJMACROS
#include <taskschd.h>

VOID PhpFillRunningTasks(
    __in PPH_PROCESS_ITEM Process,
    __inout PPH_STRING_BUILDER Tasks
    );

static int __cdecl ServiceForTooltipCompare(
    __in const void *elem1,
    __in const void *elem2
    )
{
    PPH_SERVICE_ITEM serviceItem1 = *(PPH_SERVICE_ITEM *)elem1;
    PPH_SERVICE_ITEM serviceItem2 = *(PPH_SERVICE_ITEM *)elem2;

    return PhStringCompare(serviceItem1->Name, serviceItem2->Name, TRUE);
}

PPH_STRING PhGetProcessTooltipText(
    __in PPH_PROCESS_ITEM Process
    )
{
    PPH_STRING tooltipText;
    PH_STRING_BUILDER stringBuilder;
    PPH_STRING tempString;

    PhInitializeStringBuilder(&stringBuilder, 200);

    // Command line

    if (Process->CommandLine)
    {
        tempString = PhEllipsisString(Process->CommandLine, 80);
        PhStringBuilderAppend(&stringBuilder, tempString);
        PhDereferenceObject(tempString);
        PhStringBuilderAppendChar(&stringBuilder, '\n');
    }

    // File information

    tempString = PhFormatImageVersionInfo(
        Process->FileName,
        &Process->VersionInfo,
        L"    ",
        76
        );

    if (!PhIsStringNullOrEmpty(tempString))
    {
        PhStringBuilderAppend2(&stringBuilder, L"File:\n");
        PhStringBuilderAppend(&stringBuilder, tempString);
        PhStringBuilderAppendChar(&stringBuilder, '\n');
    }

    if (tempString)
        PhDereferenceObject(tempString);

    // Known command line information

    if (Process->CommandLine && Process->QueryHandle)
    {
        PH_KNOWN_PROCESS_TYPE knownProcessType;
        PH_KNOWN_PROCESS_COMMAND_LINE knownCommandLine;

        if (NT_SUCCESS(PhGetProcessKnownType(
            Process->QueryHandle,
            &knownProcessType
            )) && PhaGetProcessKnownCommandLine(
            Process->CommandLine,
            knownProcessType,
            &knownCommandLine
            ))
        {
            switch (knownProcessType)
            {
            case ServiceHostProcessType:
                PhStringBuilderAppend2(&stringBuilder, L"Service group name:\n    ");
                PhStringBuilderAppend(&stringBuilder, knownCommandLine.ServiceHost.GroupName);
                PhStringBuilderAppendChar(&stringBuilder, '\n');
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
                            L"    ",
                            76
                            );

                        if (!PhIsStringNullOrEmpty(tempString))
                        {
                            PhStringBuilderAppend2(&stringBuilder, L"Run DLL target file:\n");
                            PhStringBuilderAppend(&stringBuilder, tempString);
                            PhStringBuilderAppendChar(&stringBuilder, '\n');
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

                    PhStringBuilderAppend2(&stringBuilder, L"COM target:\n");

                    if (knownCommandLine.ComSurrogate.Name)
                    {
                        PhStringBuilderAppend2(&stringBuilder, L"    ");
                        PhStringBuilderAppend(&stringBuilder, knownCommandLine.ComSurrogate.Name);
                        PhStringBuilderAppendChar(&stringBuilder, '\n');
                    }

                    if (guidString = PhFormatGuid(&knownCommandLine.ComSurrogate.Guid))
                    {
                        PhStringBuilderAppend2(&stringBuilder, L"    ");
                        PhStringBuilderAppend(&stringBuilder, guidString);
                        PhDereferenceObject(guidString);
                        PhStringBuilderAppendChar(&stringBuilder, '\n');
                    }

                    if (knownCommandLine.ComSurrogate.FileName && PhInitializeImageVersionInfo(
                        &versionInfo,
                        knownCommandLine.ComSurrogate.FileName->Buffer
                        ))
                    {
                        tempString = PhFormatImageVersionInfo(
                            knownCommandLine.ComSurrogate.FileName,
                            &versionInfo,
                            L"    ",
                            76
                            );

                        if (!PhIsStringNullOrEmpty(tempString))
                        {
                            PhStringBuilderAppend2(&stringBuilder, L"COM target file:\n");
                            PhStringBuilderAppend(&stringBuilder, tempString);
                            PhStringBuilderAppendChar(&stringBuilder, '\n');
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

    if (Process->ServiceList->Count != 0)
    {
        ULONG enumerationKey = 0;
        PPH_SERVICE_ITEM serviceItem;
        PPH_LIST serviceList;
        ULONG i;

        // Copy the service list into our own list so we can sort it.

        serviceList = PhCreateList(Process->ServiceList->Count);

        PhAcquireQueuedLockExclusive(&Process->ServiceListLock);

        while (PhEnumPointerList(
            Process->ServiceList,
            &enumerationKey,
            &serviceItem
            ))
        {
            PhReferenceObject(serviceItem);
            PhAddListItem(serviceList, serviceItem);
        }

        PhReleaseQueuedLockExclusive(&Process->ServiceListLock);

        qsort(serviceList->Items, serviceList->Count, sizeof(PPH_SERVICE_ITEM), ServiceForTooltipCompare);

        PhStringBuilderAppend2(&stringBuilder, L"Services:\n");

        // Add the services.
        for (i = 0; i < serviceList->Count; i++)
        {
            PPH_SERVICE_ITEM serviceItem = serviceList->Items[i];

            PhStringBuilderAppend2(&stringBuilder, L"    ");
            PhStringBuilderAppend(&stringBuilder, serviceItem->Name);
            PhStringBuilderAppend2(&stringBuilder, L" (");
            PhStringBuilderAppend(&stringBuilder, serviceItem->DisplayName);
            PhStringBuilderAppend2(&stringBuilder, L")\n");
        }

        PhDereferenceObjects(serviceList->Items, serviceList->Count);
        PhDereferenceObject(serviceList);
    }

    // Tasks
    if (WindowsVersion >= WINDOWS_VISTA && (
        PhStringEquals2(Process->ProcessName, L"taskeng.exe", TRUE) ||
        PhStringEquals2(Process->ProcessName, L"taskhost.exe", TRUE)
        ))
    {
        PH_STRING_BUILDER tasks;

        PhInitializeStringBuilder(&tasks, 40);

        PhpFillRunningTasks(Process, &tasks);

        if (tasks.String->Length != 0)
        {
            PhStringBuilderAppend2(&stringBuilder, L"Tasks:\n");
            PhStringBuilderAppend(&stringBuilder, tasks.String);
        }

        PhDeleteStringBuilder(&tasks);
    }

    // Notes

    {
        PH_STRING_BUILDER notes;

        PhInitializeStringBuilder(&notes, 40);

        if (Process->FileName)
        {
            if (Process->VerifyResult == VrTrusted)
            {
                if (!PhIsStringNullOrEmpty(Process->VerifySignerName))
                    PhStringBuilderAppendFormat(&notes, L"    Signer: %s\n", Process->VerifySignerName->Buffer);
                else
                    PhStringBuilderAppend2(&notes, L"    Signed.\n");
            }
            else if (Process->VerifyResult == VrUnknown)
            {
                // Nothing
            }
            else if (Process->VerifyResult != VrNoSignature)
            {
                PhStringBuilderAppend2(&notes, L"    Signature invalid.\n");
            }
        }

        if (Process->IsPacked)
        {
            PhStringBuilderAppendFormat(
                &notes,
                L"    Image is probably packed (%u imports over %u modules).\n",
                Process->ImportFunctions,
                Process->ImportModules
                );
        }

        if (Process->ConsoleHostProcessId)
        {
            CLIENT_ID clientId;
            PPH_STRING clientIdString;

            clientId.UniqueProcess = Process->ConsoleHostProcessId;
            clientId.UniqueThread = NULL;

            clientIdString = PhGetClientIdName(&clientId);
            PhStringBuilderAppendFormat(&notes, L"    Console host: %s\n", clientIdString->Buffer);
            PhDereferenceObject(clientIdString);
        }

        if (Process->IsDotNet)
            PhStringBuilderAppend2(&notes, L"    Process is managed (.NET).\n");
        if (Process->IsElevated)
            PhStringBuilderAppend2(&notes, L"    Process is elevated.\n");
        if (Process->IsInJob)
            PhStringBuilderAppend2(&notes, L"    Process is in a job.\n");
        if (Process->IsPosix)
            PhStringBuilderAppend2(&notes, L"    Process is POSIX.\n");
        if (Process->IsWow64)
            PhStringBuilderAppend2(&notes, L"    Process is 32-bit (WOW64).\n");

        if (notes.String->Length != 0)
        {
            PhStringBuilderAppend2(&stringBuilder, L"Notes:\n");
            PhStringBuilderAppend(&stringBuilder, notes.String);
        }

        PhDeleteStringBuilder(&notes);
    }

    // Remove the trailing newline.
    if (stringBuilder.String->Length != 0)
        PhStringBuilderRemove(&stringBuilder, stringBuilder.String->Length / 2 - 1, 1);

    tooltipText = PhReferenceStringBuilderString(&stringBuilder);
    PhDeleteStringBuilder(&stringBuilder);

    return tooltipText;
}

VOID PhpFillRunningTasks(
    __in PPH_PROCESS_ITEM Process,
    __inout PPH_STRING_BUILDER Tasks
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
                                pid == (ULONG)Process->ProcessId
                                )
                            {
                                IRunningTask_get_CurrentAction(runningTask, &action);
                                IRunningTask_get_Path(runningTask, &path);

                                PhStringBuilderAppend2(Tasks, L"    ");
                                PhStringBuilderAppend2(Tasks, action ? action : L"Unknown Action");
                                PhStringBuilderAppend2(Tasks, L" (");
                                PhStringBuilderAppend2(Tasks, path ? path : L"Unknown Path");
                                PhStringBuilderAppend2(Tasks, L")\n");

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
    __in PPH_SERVICE_ITEM Service
    )
{
    PPH_STRING tooltipText;
    PH_STRING_BUILDER stringBuilder;
    PPH_STRING tempString;
    SC_HANDLE serviceHandle;

    PhInitializeStringBuilder(&stringBuilder, 200);

    if (serviceHandle = PhOpenService(Service->Name->Buffer, SERVICE_QUERY_CONFIG))
    {
        //LPQUERY_SERVICE_CONFIG config;

        // File information
        // (Disabled for now because of file name resolution issues)

        /*if (config = PhGetServiceConfig(serviceHandle))
        {
            PPH_STRING fileName;
            PPH_STRING newFileName;
            PH_IMAGE_VERSION_INFO versionInfo;

            fileName = PhCreateString(config->lpBinaryPathName);
            newFileName = PhGetFileName(fileName);
            PhDereferenceObject(fileName);
            fileName = newFileName;

            if (PhInitializeImageVersionInfo(
                &versionInfo,
                fileName->Buffer
                ))
            {
                tempString = PhFormatImageVersionInfo(
                    fileName,
                    &versionInfo,
                    L"    ",
                    76
                    );

                if (!PhIsStringNullOrEmpty(tempString))
                {
                    PhStringBuilderAppend2(&stringBuilder, L"File:\n");
                    PhStringBuilderAppend(&stringBuilder, tempString);
                    PhStringBuilderAppendChar(&stringBuilder, '\n');
                }

                if (tempString)
                    PhDereferenceObject(tempString);

                PhDeleteImageVersionInfo(&versionInfo);
            }

            PhDereferenceObject(fileName);
            PhFree(config);
        }*/

        // Description

        if (tempString = PhGetServiceDescription(serviceHandle))
        {
            PhStringBuilderAppend(&stringBuilder, tempString);
            PhStringBuilderAppendChar(&stringBuilder, '\n');
            PhDereferenceObject(tempString);
        }

        CloseServiceHandle(serviceHandle);
    }

    // Remove the trailing newline.
    if (stringBuilder.String->Length != 0)
        PhStringBuilderRemove(&stringBuilder, stringBuilder.String->Length / 2 - 1, 1);

    tooltipText = PhReferenceStringBuilderString(&stringBuilder);
    PhDeleteStringBuilder(&stringBuilder);

    return tooltipText;
}
