/*
 * Process Hacker -
 *   item tooltips
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

#include <phapp.h>
#include <phplug.h>
#define CINTERFACE
#define COBJMACROS
#include <taskschd.h>

VOID PhpFillRunningTasks(
    __in PPH_PROCESS_ITEM Process,
    __inout PPH_STRING_BUILDER Tasks
    );

VOID PhpAppendStringWithLineBreaks(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in PPH_STRINGREF String,
    __in ULONG CharactersPerLine,
    __in_opt PWSTR IndentAfterFirstLine
    )
{
    PH_STRINGREF line;
    SIZE_T bytesPerLine;
    BOOLEAN afterFirstLine;
    SIZE_T bytesToAppend;
    SIZE_T indentAfterFirstLineLength;

    line = *String;
    bytesPerLine = CharactersPerLine * sizeof(WCHAR);
    afterFirstLine = FALSE;

    if (IndentAfterFirstLine)
        indentAfterFirstLineLength = wcslen(IndentAfterFirstLine) * sizeof(WCHAR);

    while (line.Length != 0)
    {
        bytesToAppend = line.Length;

        if (bytesToAppend > bytesPerLine)
            bytesToAppend = bytesPerLine;

        if (afterFirstLine)
        {
            PhAppendCharStringBuilder(StringBuilder, '\n');

            if (IndentAfterFirstLine)
                PhAppendStringBuilderEx(StringBuilder, IndentAfterFirstLine, indentAfterFirstLineLength);
        }

        PhAppendStringBuilderEx(StringBuilder, line.Buffer, bytesToAppend);
        afterFirstLine = TRUE;

        line.Buffer = (PWSTR)((PCHAR)line.Buffer + bytesToAppend);
        line.Length -= bytesToAppend;
    }
}

static int __cdecl ServiceForTooltipCompare(
    __in const void *elem1,
    __in const void *elem2
    )
{
    PPH_SERVICE_ITEM serviceItem1 = *(PPH_SERVICE_ITEM *)elem1;
    PPH_SERVICE_ITEM serviceItem2 = *(PPH_SERVICE_ITEM *)elem2;

    return PhCompareString(serviceItem1->Name, serviceItem2->Name, TRUE);
}

PPH_STRING PhGetProcessTooltipText(
    __in PPH_PROCESS_ITEM Process
    )
{
    PH_STRING_BUILDER stringBuilder;
    PPH_STRING tempString;

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
        L"    ",
        0
        );

    if (!PhIsNullOrEmptyString(tempString))
    {
        PhAppendStringBuilder2(&stringBuilder, L"File:\n");
        PhAppendStringBuilder(&stringBuilder, tempString);
        PhAppendCharStringBuilder(&stringBuilder, '\n');
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
            switch (knownProcessType & KnownProcessTypeMask)
            {
            case ServiceHostProcessType:
                PhAppendStringBuilder2(&stringBuilder, L"Service group name:\n    ");
                PhAppendStringBuilder(&stringBuilder, knownCommandLine.ServiceHost.GroupName);
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
                            L"    ",
                            0
                            );

                        if (!PhIsNullOrEmptyString(tempString))
                        {
                            PhAppendStringBuilder2(&stringBuilder, L"Run DLL target file:\n");
                            PhAppendStringBuilder(&stringBuilder, tempString);
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
                        PhAppendStringBuilder2(&stringBuilder, L"    ");
                        PhAppendStringBuilder(&stringBuilder, knownCommandLine.ComSurrogate.Name);
                        PhAppendCharStringBuilder(&stringBuilder, '\n');
                    }

                    if (guidString = PhFormatGuid(&knownCommandLine.ComSurrogate.Guid))
                    {
                        PhAppendStringBuilder2(&stringBuilder, L"    ");
                        PhAppendStringBuilder(&stringBuilder, guidString);
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
                            L"    ",
                            0
                            );

                        if (!PhIsNullOrEmptyString(tempString))
                        {
                            PhAppendStringBuilder2(&stringBuilder, L"COM target file:\n");
                            PhAppendStringBuilder(&stringBuilder, tempString);
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

            PhAppendStringBuilder2(&stringBuilder, L"    ");
            PhAppendStringBuilder(&stringBuilder, serviceItem->Name);
            PhAppendStringBuilder2(&stringBuilder, L" (");
            PhAppendStringBuilder(&stringBuilder, serviceItem->DisplayName);
            PhAppendStringBuilder2(&stringBuilder, L")\n");
        }

        PhDereferenceObjects(serviceList->Items, serviceList->Count);
        PhDereferenceObject(serviceList);
    }

    // Tasks
    if (WindowsVersion >= WINDOWS_VISTA && (
        PhEqualString2(Process->ProcessName, L"taskeng.exe", TRUE) ||
        PhEqualString2(Process->ProcessName, L"taskhost.exe", TRUE)
        ))
    {
        PH_STRING_BUILDER tasks;

        PhInitializeStringBuilder(&tasks, 40);

        PhpFillRunningTasks(Process, &tasks);

        if (tasks.String->Length != 0)
        {
            PhAppendStringBuilder2(&stringBuilder, L"Tasks:\n");
            PhAppendStringBuilder(&stringBuilder, tasks.String);
        }

        PhDeleteStringBuilder(&tasks);
    }

    // Plugin
    if (PhPluginsEnabled)
    {
        PH_PLUGIN_GET_TOOLTIP_TEXT getTooltipText;

        getTooltipText.Parameter = Process;
        getTooltipText.StringBuilder = &stringBuilder;

        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackGetProcessTooltipText), &getTooltipText);
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

        if (Process->ConsoleHostProcessId)
        {
            CLIENT_ID clientId;
            PPH_STRING clientIdString;

            clientId.UniqueProcess = Process->ConsoleHostProcessId;
            clientId.UniqueThread = NULL;

            clientIdString = PhGetClientIdName(&clientId);
            PhAppendFormatStringBuilder(&notes, L"    Console host: %s\n", clientIdString->Buffer);
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
            PhAppendStringBuilder2(&notes, L"    Process is a Metro style app.\n");
        if (Process->IsInJob)
            PhAppendStringBuilder2(&notes, L"    Process is in a job.\n");
        if (Process->IsPosix)
            PhAppendStringBuilder2(&notes, L"    Process is POSIX.\n");
        if (Process->IsWow64)
            PhAppendStringBuilder2(&notes, L"    Process is 32-bit (WOW64).\n");

        if (notes.String->Length != 0)
        {
            PhAppendStringBuilder2(&stringBuilder, L"Notes:\n");
            PhAppendStringBuilder(&stringBuilder, notes.String);
        }

        PhDeleteStringBuilder(&notes);
    }

    // Remove the trailing newline.
    if (stringBuilder.String->Length != 0)
        PhRemoveStringBuilder(&stringBuilder, stringBuilder.String->Length / 2 - 1, 1);

    return PhFinalStringBuilderString(&stringBuilder);
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

                                PhAppendStringBuilder2(Tasks, L"    ");
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
    __in PPH_SERVICE_ITEM Service
    )
{
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
                    0
                    );

                if (!PhIsNullOrEmptyString(tempString))
                {
                    PhAppendStringBuilder2(&stringBuilder, L"File:\n");
                    PhAppendStringBuilder(&stringBuilder, tempString);
                    PhAppendCharStringBuilder(&stringBuilder, '\n');
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
            PhAppendStringBuilder(&stringBuilder, tempString);
            PhAppendCharStringBuilder(&stringBuilder, '\n');
            PhDereferenceObject(tempString);
        }

        CloseServiceHandle(serviceHandle);
    }

    // Remove the trailing newline.
    if (stringBuilder.String->Length != 0)
        PhRemoveStringBuilder(&stringBuilder, stringBuilder.String->Length / 2 - 1, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}
