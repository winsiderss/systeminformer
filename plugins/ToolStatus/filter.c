/*
 * Process Hacker ToolStatus -
 *   search filter callbacks
 *
 * Copyright (C) 2011-2013 dmex
 * Copyright (C) 2010-2012 wj32
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

#include "toolstatus.h"

static BOOLEAN WordMatch(
    __in PPH_STRINGREF Text,
    __in PPH_STRINGREF Search
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    remainingPart = *Search;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, ' ', &part, &remainingPart);

        if (part.Length != 0)
        {
            if (PhFindStringInStringRef(Text, &part, TRUE) != -1)
                return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN ProcessTreeFilterCallback(
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    )
{
    HWND textboxHandle = (HWND)Context;
    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;
    PPH_STRING textboxText = PhGetWindowText(textboxHandle);

    if (textboxText)
    {
        if (textboxText->Length > 0)
        {
            BOOLEAN showItem = FALSE;
            
            // TODO: Unimplemented fields
            // PH_HASH_ENTRY HashEntry;
            // ULONG State;
            // PPH_PROCESS_RECORD Record;      
            // LARGE_INTEGER CreateTime;
            // HANDLE QueryHandle;
            // PH_IMAGE_VERSION_INFO VersionInfo;
            // HANDLE ConsoleHostProcessId;      
            // ULONG ImportFunctions;
            // ULONG ImportModules; 
            // KPRIORITY BasePriority; 
            // LARGE_INTEGER KernelTime;
            // LARGE_INTEGER UserTime;
            // ULONG NumberOfHandles;
            // ULONG NumberOfThreads;
            // FLOAT CpuUsage; // Below Windows 7, sum of kernel and user CPU usage; above Windows 7, cycle-based CPU usage.
            // FLOAT CpuKernelUsage;
            // FLOAT CpuUserUsage;
            // PH_UINT64_DELTA CpuKernelDelta;
            // PH_UINT64_DELTA CpuUserDelta;
            // PH_UINT64_DELTA IoReadDelta;
            // PH_UINT64_DELTA IoWriteDelta;
            // PH_UINT64_DELTA IoOtherDelta;
            // PH_UINT64_DELTA IoReadCountDelta;
            // PH_UINT64_DELTA IoWriteCountDelta;
            // PH_UINT64_DELTA IoOtherCountDelta;
            // PH_UINT32_DELTA ContextSwitchesDelta;
            // PH_UINT32_DELTA PageFaultsDelta;
            // PH_UINT64_DELTA CycleTimeDelta; // since WIN7
            // VM_COUNTERS_EX VmCounters;
            // IO_COUNTERS IoCounters;
            // SIZE_T WorkingSetPrivateSize; // since VISTA
            // ULONG PeakNumberOfThreads; // since WIN7
            // ULONG HardFaultCount; // since WIN7
            // ULONG SequenceNumber;
            // PH_CIRCULAR_BUFFER_FLOAT CpuKernelHistory;
            // PH_CIRCULAR_BUFFER_FLOAT CpuUserHistory;
            // PH_CIRCULAR_BUFFER_ULONG64 IoReadHistory;
            // PH_CIRCULAR_BUFFER_ULONG64 IoWriteHistory;
            // PH_CIRCULAR_BUFFER_ULONG64 IoOtherHistory;
            // PH_CIRCULAR_BUFFER_SIZE_T PrivateBytesHistory;
            // PH_CIRCULAR_BUFFER_SIZE_T WorkingSetHistory;
            // PH_UINTPTR_DELTA PrivateBytesDelta;

            if (WINDOWS_HAS_UAC)
            {
                PH_STRINGREF elevationTypeStringRef;

                switch (processNode->ProcessItem->ElevationType)
                {
                case TokenElevationTypeLimited:
                    PhInitializeStringRef(&elevationTypeStringRef, L"Limited");
                    break;
                case TokenElevationTypeFull:
                    PhInitializeStringRef(&elevationTypeStringRef, L"Full");
                    break;           
                default:
                case TokenElevationTypeDefault:
                    PhInitializeStringRef(&elevationTypeStringRef, L"N/A");
                    break;
                }

                if (WordMatch(&elevationTypeStringRef, &textboxText->sr))
                    showItem = TRUE;
            }

            {  
                PH_STRINGREF verifyResultStringRef;

                switch (processNode->ProcessItem->VerifyResult)
                {
                case VrNoSignature:
                    PhInitializeStringRef(&verifyResultStringRef, L"NoSignature");
                    break;
                case VrTrusted:
                    PhInitializeStringRef(&verifyResultStringRef, L"Trusted");
                    break;
                case VrExpired:
                    PhInitializeStringRef(&verifyResultStringRef, L"Expired");
                    break;
                case VrRevoked:
                    PhInitializeStringRef(&verifyResultStringRef, L"Revoked");
                    break;
                case VrDistrust:
                    PhInitializeStringRef(&verifyResultStringRef, L"Distrust");
                    break;
                case VrSecuritySettings:
                    PhInitializeStringRef(&verifyResultStringRef, L"SecuritySettings");
                    break;
                case VrBadSignature:
                    PhInitializeStringRef(&verifyResultStringRef, L"BadSignature");
                    break;  
                default:
                case VrUnknown:
                    PhInitializeStringRef(&verifyResultStringRef, L"Unknown");
                    break;
                }

                if (WordMatch(&verifyResultStringRef, &textboxText->sr))
                    showItem = TRUE;
            }

            {
                PH_STRINGREF stringRef;

                PhInitializeStringRef(&stringRef, PhGetProcessPriorityClassString(processNode->ProcessItem->PriorityClass));

                if (WordMatch(&stringRef, &textboxText->sr))
                    showItem = TRUE;
            }

            if (processNode->ProcessItem->ProcessName)
            {
                if (WordMatch(&processNode->ProcessItem->ProcessName->sr, &textboxText->sr))
                    showItem = TRUE;
            }

            if (processNode->ProcessItem->FileName)
            {
                if (WordMatch(&processNode->ProcessItem->FileName->sr, &textboxText->sr))
                    showItem = TRUE;
            }

            if (processNode->ProcessItem->CommandLine)
            {
                if (WordMatch(&processNode->ProcessItem->CommandLine->sr, &textboxText->sr))
                    showItem = TRUE;
            }

            if (processNode->ProcessItem->VersionInfo.CompanyName)
            {
                if (WordMatch(&processNode->ProcessItem->VersionInfo.CompanyName->sr, &textboxText->sr))
                    showItem = TRUE;
            }

            if (processNode->ProcessItem->VersionInfo.FileDescription)
            {
                if (WordMatch(&processNode->ProcessItem->VersionInfo.FileDescription->sr, &textboxText->sr))
                    showItem = TRUE;
            }

            if (processNode->ProcessItem->VersionInfo.FileVersion)
            {
                if (WordMatch(&processNode->ProcessItem->VersionInfo.FileVersion->sr, &textboxText->sr))
                    showItem = TRUE;
            }

            if (processNode->ProcessItem->VersionInfo.ProductName)
            {
                if (WordMatch(&processNode->ProcessItem->VersionInfo.ProductName->sr, &textboxText->sr))
                    showItem = TRUE;
            }

            if (processNode->ProcessItem->UserName)
            {
                if (WordMatch(&processNode->ProcessItem->UserName->sr, &textboxText->sr))
                    showItem = TRUE;
            }

            if (processNode->ProcessItem->IntegrityString)
            {
                PH_STRINGREF stringRef;

                PhInitializeStringRef(&stringRef, processNode->ProcessItem->IntegrityString);    

                if (WordMatch(&stringRef, &textboxText->sr))
                    showItem = TRUE;
            }
     
            if (processNode->ProcessItem->JobName)
            {
                if (WordMatch(&processNode->ProcessItem->JobName->sr, &textboxText->sr))
                    showItem = TRUE;
            }

            if (processNode->ProcessItem->VerifySignerName)
            {
                if (WordMatch(&processNode->ProcessItem->VerifySignerName->sr, &textboxText->sr))
                    showItem = TRUE;
            }

            if (processNode->ProcessItem->ProcessIdString)
            {
                PH_STRINGREF pidStringRef;

                PhInitializeStringRef(&pidStringRef, processNode->ProcessItem->ProcessIdString);

                if (WordMatch(&pidStringRef, &textboxText->sr))
                    showItem = TRUE;
            }
  
            if (processNode->ProcessItem->ParentProcessIdString)
            {
                PH_STRINGREF stringRef;

                PhInitializeStringRef(&stringRef, processNode->ProcessItem->ParentProcessIdString);

                if (WordMatch(&stringRef, &textboxText->sr))
                    showItem = TRUE;
            }
         
            if (processNode->ProcessItem->SessionIdString)
            {
                PH_STRINGREF stringRef;

                PhInitializeStringRef(&stringRef, processNode->ProcessItem->SessionIdString);    

                if (WordMatch(&stringRef, &textboxText->sr))
                    showItem = TRUE;
            }

            if (processNode->ProcessItem->PackageFullName)
            {
                if (WordMatch(&processNode->ProcessItem->PackageFullName->sr, &textboxText->sr))
                    showItem = TRUE;
            }
       
            if (WSTR_IEQUAL(textboxText->Buffer, L"UpdateIsDotNet"))
            {
                showItem = processNode->ProcessItem->UpdateIsDotNet == TRUE;
            }

            if (WSTR_IEQUAL(textboxText->Buffer, L"IsBeingDebugged"))
            {
                showItem = processNode->ProcessItem->IsBeingDebugged == TRUE;
            }
 
            if (WSTR_IEQUAL(textboxText->Buffer, L"IsDotNet"))
            {
                showItem = processNode->ProcessItem->IsDotNet == TRUE;
            }
       
            if (WSTR_IEQUAL(textboxText->Buffer, L"IsElevated"))
            {
                showItem = processNode->ProcessItem->IsElevated == TRUE;
            }
          
            if (WSTR_IEQUAL(textboxText->Buffer, L"IsInJob"))
            {
                showItem = processNode->ProcessItem->IsInJob == TRUE;
            }
 
            if (WSTR_IEQUAL(textboxText->Buffer, L"IsInSignificantJob"))
            {
                showItem = processNode->ProcessItem->IsInSignificantJob == TRUE;
            }

            if (WSTR_IEQUAL(textboxText->Buffer, L"IsPacked"))
            {
                showItem = processNode->ProcessItem->IsPacked == TRUE;
            }

            if (WSTR_IEQUAL(textboxText->Buffer, L"IsPosix"))
            {
                showItem = processNode->ProcessItem->IsPosix == TRUE;
            }
        
            if (WSTR_IEQUAL(textboxText->Buffer, L"IsSuspended"))
            {
                showItem = processNode->ProcessItem->IsSuspended == TRUE;
            }

            if (WSTR_IEQUAL(textboxText->Buffer, L"IsWow64"))
            {
                showItem = processNode->ProcessItem->IsWow64 == TRUE;
            }
    
            if (WSTR_IEQUAL(textboxText->Buffer, L"IsImmersive"))
            {
                showItem = processNode->ProcessItem->IsImmersive == TRUE;
            }

            if (WSTR_IEQUAL(textboxText->Buffer, L"IsWow64Valid"))
            {
                showItem = processNode->ProcessItem->IsWow64Valid == TRUE;
            }

            PhDereferenceObject(textboxText);
            return showItem;
        }

        PhDereferenceObject(textboxText);
    }

    return TRUE;
}

BOOLEAN ServiceTreeFilterCallback(
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    )
{
    HWND textboxHandle = (HWND)Context;
    PPH_SERVICE_NODE serviceNode = (PPH_SERVICE_NODE)Node; 
    PPH_STRING textboxText = PhGetWindowText(textboxHandle);

    if (textboxText)
    {
        if (textboxText->Length > 0)
        {
            BOOLEAN showItem = FALSE;
            PH_STRINGREF typeStringRef;
            PH_STRINGREF stateStringRef;
            PH_STRINGREF startTypeStringRef;
            PH_STRINGREF errorControlStringRef;

            // TODO: Unimplemented fields
            // ULONG ControlsAccepted;
            // ULONG Flags; // e.g. SERVICE_RUNS_IN_SYSTEM_PROCESS
           
            PhInitializeStringRef(&typeStringRef, PhGetServiceTypeString(serviceNode->ServiceItem->Type));
            PhInitializeStringRef(&stateStringRef, PhGetServiceStateString(serviceNode->ServiceItem->State));
            PhInitializeStringRef(&startTypeStringRef, PhGetServiceStartTypeString(serviceNode->ServiceItem->StartType));
            PhInitializeStringRef(&errorControlStringRef, PhGetServiceErrorControlString(serviceNode->ServiceItem->ErrorControl));

            if (WordMatch(&typeStringRef, &textboxText->sr))
                showItem = TRUE;
            if (WordMatch(&stateStringRef, &textboxText->sr))
                showItem = TRUE;
            if (WordMatch(&startTypeStringRef, &textboxText->sr))
                showItem = TRUE;
            if (WordMatch(&errorControlStringRef, &textboxText->sr))
                showItem = TRUE;

            if (serviceNode->ServiceItem->Name)
            {
                if (WordMatch(&serviceNode->ServiceItem->Name->sr, &textboxText->sr))
                {
                    showItem = TRUE;
                }
            }

            if (serviceNode->ServiceItem->DisplayName)
            {
                if (WordMatch(&serviceNode->ServiceItem->DisplayName->sr, &textboxText->sr))
                {
                    showItem = TRUE;
                }
            }

            if (serviceNode->ServiceItem->ProcessIdString)
            {
                PH_STRINGREF pidStringRef;

                PhInitializeStringRef(&pidStringRef, serviceNode->ServiceItem->ProcessIdString);

                // Search process PIDs
                if (WordMatch(&pidStringRef, &textboxText->sr))
                    showItem = TRUE;
            }

            PhDereferenceObject(textboxText);
            return showItem;
        }

        PhDereferenceObject(textboxText);
    }

    return TRUE;
}

BOOLEAN NetworkTreeFilterCallback(
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    )
{
    HWND textboxHandle = (HWND)Context;
    PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)Node;
    PPH_STRING textboxText = PhGetWindowText(textboxHandle);

    if (textboxText)
    {
        if (textboxText->Length > 0)
        {
            BOOLEAN showItem = FALSE;
            PH_STRINGREF pidStringRef;
            WCHAR pidString[32];

            //ULONG ProtocolType;
            //PH_IP_ENDPOINT LocalEndpoint;
            //PH_IP_ENDPOINT RemoteEndpoint;
            //ULONG State;
            //WCHAR LocalPortString[PH_INT32_STR_LEN_1];
            //WCHAR RemoteAddressString[65];
            //WCHAR RemotePortString[PH_INT32_STR_LEN_1];
            //PPH_STRING LocalHostString;
            //PPH_STRING RemoteHostString;
            //LARGE_INTEGER CreateTime;
            //ULONGLONG OwnerInfo[PH_NETWORK_OWNER_INFO_SIZE];

            PhPrintUInt32(pidString, (ULONG)networkNode->NetworkItem->ProcessId);
            PhInitializeStringRef(&pidStringRef, pidString);

            if (WordMatch(&pidStringRef, &textboxText->sr))
            {
                showItem = TRUE;
            }
      
            if (networkNode->NetworkItem->ProcessName)
            {
                if (WordMatch(&networkNode->NetworkItem->ProcessName->sr, &textboxText->sr))
                {
                    showItem = TRUE;
                }
            }
                        
            if (networkNode->NetworkItem->OwnerName)
            {
                if (WordMatch(&networkNode->NetworkItem->OwnerName->sr, &textboxText->sr))
                {
                    showItem = TRUE;
                }
            }

            if (networkNode->NetworkItem->LocalAddressString)
            {
                PH_STRINGREF localAddressRef;

                PhInitializeStringRef(&localAddressRef, networkNode->NetworkItem->LocalAddressString);

                if (WordMatch(&localAddressRef, &textboxText->sr))
                {
                    showItem = TRUE;
                }
            }

            if (networkNode->NetworkItem->LocalPortString)
            {
                PH_STRINGREF localPortRef;

                PhInitializeStringRef(&localPortRef, networkNode->NetworkItem->LocalPortString);

                if (WordMatch(&localPortRef, &textboxText->sr))
                    showItem = TRUE;
            }

            if (networkNode->NetworkItem->RemoteAddressString)
            {
                PH_STRINGREF remoteAddressRef;

                PhInitializeStringRef(&remoteAddressRef, networkNode->NetworkItem->RemoteAddressString);

                if (WordMatch(&remoteAddressRef, &textboxText->sr))
                    showItem = TRUE;
            }
                        
            if (networkNode->NetworkItem->RemotePortString)
            {
                PH_STRINGREF remotePortRef;

                PhInitializeStringRef(&remotePortRef, networkNode->NetworkItem->RemotePortString);

                if (WordMatch(&remotePortRef, &textboxText->sr))
                    showItem = TRUE;
            }

            if (networkNode->NetworkItem->RemoteHostString)
            {
                if (WordMatch(&networkNode->NetworkItem->RemoteHostString->sr, &textboxText->sr))
                    showItem = TRUE;
            }

            PhDereferenceObject(textboxText);
            return showItem;
        }

        PhDereferenceObject(textboxText);
    }

    return TRUE;
}