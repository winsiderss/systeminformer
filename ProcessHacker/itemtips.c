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

        PhAcquireQueuedLockExclusiveFast(&Process->ServiceListLock);

        while (PhEnumPointerList(
            Process->ServiceList,
            &enumerationKey,
            &serviceItem
            ))
        {
            PhReferenceObject(serviceItem);
            PhAddListItem(serviceList, serviceItem);
        }

        PhReleaseQueuedLockExclusiveFast(&Process->ServiceListLock);

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
