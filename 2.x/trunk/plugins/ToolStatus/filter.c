/*
 * Process Hacker ToolStatus -
 *   search filter callbacks
 *
 * Copyright (C) 2011-2015 dmex
 * Copyright (C) 2010-2013 wj32
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
#include <verify.h>

BOOLEAN WordMatchStringRef(
    _In_ PPH_STRINGREF Text
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    remainingPart = SearchboxText->sr;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, '|', &part, &remainingPart);

        if (part.Length != 0)
        {
            if (PhFindStringInStringRef(Text, &part, TRUE) != -1)
                return TRUE;
        }
    }

    return FALSE;
}

static BOOLEAN WordMatchStringZ(
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRef(&text, Text);
    return WordMatchStringRef(&text);
}

BOOLEAN ProcessTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;

    if (PhIsNullOrEmptyString(SearchboxText))
        return TRUE;

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->ProcessName))
    {
        if (WordMatchStringRef(&processNode->ProcessItem->ProcessName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->FileName))
    {
        if (WordMatchStringRef(&processNode->ProcessItem->FileName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->CommandLine))
    {
        if (WordMatchStringRef(&processNode->ProcessItem->CommandLine->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.CompanyName))
    {
        if (WordMatchStringRef(&processNode->ProcessItem->VersionInfo.CompanyName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.FileDescription))
    {
        if (WordMatchStringRef(&processNode->ProcessItem->VersionInfo.FileDescription->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.FileVersion))
    {
        if (WordMatchStringRef(&processNode->ProcessItem->VersionInfo.FileVersion->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.ProductName))
    {
        if (WordMatchStringRef(&processNode->ProcessItem->VersionInfo.ProductName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->UserName))
    {
        if (WordMatchStringRef(&processNode->ProcessItem->UserName->sr))
            return TRUE;
    }

    if (processNode->ProcessItem->IntegrityString)
    {
        if (WordMatchStringZ(processNode->ProcessItem->IntegrityString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->JobName))
    {
        if (WordMatchStringRef(&processNode->ProcessItem->JobName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VerifySignerName))
    {
        if (WordMatchStringRef(&processNode->ProcessItem->VerifySignerName->sr))
            return TRUE;
    }

    if (processNode->ProcessItem->ProcessIdString[0] != 0)
    {
        if (WordMatchStringZ(processNode->ProcessItem->ProcessIdString))
            return TRUE;
    }

    if (processNode->ProcessItem->ParentProcessIdString[0] != 0)
    {
        if (WordMatchStringZ(processNode->ProcessItem->ParentProcessIdString))
            return TRUE;
    }

    if (processNode->ProcessItem->SessionIdString[0] != 0)
    {
        if (WordMatchStringZ(processNode->ProcessItem->SessionIdString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->PackageFullName))
    {
        if (WordMatchStringRef(&processNode->ProcessItem->PackageFullName->sr))
            return TRUE;
    }

    if (WordMatchStringZ(PhGetProcessPriorityClassString(processNode->ProcessItem->PriorityClass)))
    {
        return TRUE;
    }

    if (processNode->ProcessItem->VerifyResult != VrUnknown)
    {
        switch (processNode->ProcessItem->VerifyResult)
        {
        case VrNoSignature:
            if (WordMatchStringZ(L"NoSignature"))
                return TRUE;
            break;
        case VrTrusted:
            if (WordMatchStringZ(L"Trusted"))
                return TRUE;
            break;
        case VrExpired:
            if (WordMatchStringZ(L"Expired"))
                return TRUE;
            break;
        case VrRevoked:
            if (WordMatchStringZ(L"Revoked"))
                return TRUE;
            break;
        case VrDistrust:
            if (WordMatchStringZ(L"Distrust"))
                return TRUE;
            break;
        case VrSecuritySettings:
            if (WordMatchStringZ(L"SecuritySettings"))
                return TRUE;
            break;
        case VrBadSignature:
            if (WordMatchStringZ(L"BadSignature"))
                return TRUE;
            break;
        default:
            if (WordMatchStringZ(L"Unknown"))
                return TRUE;
            break;
        }
    }

    if (WINDOWS_HAS_UAC && processNode->ProcessItem->ElevationType != TokenElevationTypeDefault)
    {
        switch (processNode->ProcessItem->ElevationType)
        {
        case TokenElevationTypeLimited:
            if (WordMatchStringZ(L"Limited"))
                return TRUE;
            break;
        case TokenElevationTypeFull:
            if (WordMatchStringZ(L"Full"))
                return TRUE;
            break;
        default:
            if (WordMatchStringZ(L"Unknown"))
                return TRUE;
            break;
        }
    }

    if (WordMatchStringZ(L"UpdateIsDotNet") && processNode->ProcessItem->UpdateIsDotNet)
    {
        return TRUE;
    }

    if (WordMatchStringZ(L"IsBeingDebugged") && processNode->ProcessItem->IsBeingDebugged)
    {
        return TRUE;
    }

    if (WordMatchStringZ(L"IsDotNet") && processNode->ProcessItem->IsDotNet)
    {
        return TRUE;
    }

    if (WordMatchStringZ(L"IsElevated") && processNode->ProcessItem->IsElevated)
    {
        return TRUE;
    }

    if (WordMatchStringZ(L"IsInJob") && processNode->ProcessItem->IsInJob)
    {
        return TRUE;
    }

    if (WordMatchStringZ(L"IsInSignificantJob") && processNode->ProcessItem->IsInSignificantJob)
    {
        return TRUE;
    }

    if (WordMatchStringZ(L"IsPacked") && processNode->ProcessItem->IsPacked)
    {
        return TRUE;
    }

    if (WordMatchStringZ(L"IsPosix") && processNode->ProcessItem->IsPosix)
    {
        return TRUE;
    }

    if (WordMatchStringZ(L"IsSuspended") && processNode->ProcessItem->IsSuspended)
    {
        return TRUE;
    }

    if (WordMatchStringZ(L"IsWow64") && processNode->ProcessItem->IsWow64)
    {
        return TRUE;
    }

    if (WordMatchStringZ(L"IsImmersive") && processNode->ProcessItem->IsImmersive)
    {
        return TRUE;
    }

    if (processNode->ProcessItem->ServiceList && processNode->ProcessItem->ServiceList->Count != 0)
    {
        ULONG enumerationKey = 0;
        PPH_SERVICE_ITEM serviceItem;
        PPH_LIST serviceList;
        ULONG i;
        BOOLEAN matched = FALSE;

        // Copy the service list so we can search it.
        serviceList = PhCreateList(processNode->ProcessItem->ServiceList->Count);

        PhAcquireQueuedLockShared(&processNode->ProcessItem->ServiceListLock);

        while (PhEnumPointerList(
            processNode->ProcessItem->ServiceList,
            &enumerationKey,
            &serviceItem
            ))
        {
            PhReferenceObject(serviceItem);
            PhAddItemList(serviceList, serviceItem);
        }

        PhReleaseQueuedLockShared(&processNode->ProcessItem->ServiceListLock);

        for (i = 0; i < serviceList->Count; i++)
        {
            serviceItem = serviceList->Items[i];

            if (!PhIsNullOrEmptyString(serviceItem->Name))
            {
                if (WordMatchStringRef(&serviceItem->Name->sr))
                {
                    matched = TRUE;
                    break;
                }
            }

            if (!PhIsNullOrEmptyString(serviceItem->DisplayName))
            {
                if (WordMatchStringRef(&serviceItem->DisplayName->sr))
                {
                    matched = TRUE;
                    break;
                }
            }

            if (serviceItem->ProcessId)
            {
                WCHAR processIdString[PH_INT32_STR_LEN_1];

                PhPrintUInt32(processIdString, HandleToUlong(serviceItem->ProcessId));

                if (WordMatchStringZ(processIdString))
                {
                    matched = TRUE;
                    break;
                }
            }
        }

        PhDereferenceObjects(serviceList->Items, serviceList->Count);
        PhDereferenceObject(serviceList);

        if (matched)
            return TRUE;
    }

    return FALSE;
}

BOOLEAN ServiceTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_SERVICE_NODE serviceNode = (PPH_SERVICE_NODE)Node;

    if (PhIsNullOrEmptyString(SearchboxText))
        return TRUE;

    if (WordMatchStringZ(PhGetServiceTypeString(serviceNode->ServiceItem->Type)))
        return TRUE;

    if (WordMatchStringZ(PhGetServiceStateString(serviceNode->ServiceItem->State)))
        return TRUE;

    if (WordMatchStringZ(PhGetServiceStartTypeString(serviceNode->ServiceItem->StartType)))
        return TRUE;

    if (WordMatchStringZ(PhGetServiceErrorControlString(serviceNode->ServiceItem->ErrorControl)))
        return TRUE;

    if (!PhIsNullOrEmptyString(serviceNode->ServiceItem->Name))
    {
        if (WordMatchStringRef(&serviceNode->ServiceItem->Name->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(serviceNode->ServiceItem->DisplayName))
    {
        if (WordMatchStringRef(&serviceNode->ServiceItem->DisplayName->sr))
            return TRUE;
    }

    if (serviceNode->ServiceItem->ProcessId)
    {
        WCHAR processIdString[PH_INT32_STR_LEN_1];

        PhPrintUInt32(processIdString, HandleToUlong(serviceNode->ServiceItem->ProcessId));

        if (WordMatchStringZ(processIdString))
            return TRUE;
    }

    return FALSE;
}

BOOLEAN NetworkTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)Node;

    if (PhIsNullOrEmptyString(SearchboxText))
        return TRUE;

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->ProcessName))
    {
        if (WordMatchStringRef(&networkNode->NetworkItem->ProcessName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->OwnerName))
    {
        if (WordMatchStringRef(&networkNode->NetworkItem->OwnerName->sr))
            return TRUE;
    }

    if (networkNode->NetworkItem->LocalAddressString[0] != 0)
    {
        if (WordMatchStringZ(networkNode->NetworkItem->LocalAddressString))
            return TRUE;
    }

    if (networkNode->NetworkItem->LocalPortString[0] != 0)
    {
        if (WordMatchStringZ(networkNode->NetworkItem->LocalPortString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->LocalHostString))
    {
        if (WordMatchStringRef(&networkNode->NetworkItem->LocalHostString->sr))
            return TRUE;
    }

    if (networkNode->NetworkItem->RemoteAddressString[0] != 0)
    {
        if (WordMatchStringZ(networkNode->NetworkItem->RemoteAddressString))
            return TRUE;
    }

    if (networkNode->NetworkItem->RemotePortString[0] != 0)
    {
        if (WordMatchStringZ(networkNode->NetworkItem->RemotePortString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->RemoteHostString))
    {
        if (WordMatchStringRef(&networkNode->NetworkItem->RemoteHostString->sr))
            return TRUE;
    }

    if (WordMatchStringZ(PhGetProtocolTypeName(networkNode->NetworkItem->ProtocolType)))
        return TRUE;

    if ((networkNode->NetworkItem->ProtocolType & PH_TCP_PROTOCOL_TYPE) &&
        WordMatchStringZ(PhGetTcpStateName(networkNode->NetworkItem->State)))
        return TRUE;

    if (networkNode->NetworkItem->ProcessId)
    {
        WCHAR processIdString[PH_INT32_STR_LEN_1];

        PhPrintUInt32(processIdString, HandleToUlong(networkNode->NetworkItem->ProcessId));

        if (WordMatchStringZ(processIdString))
            return TRUE;
    }

    return FALSE;
}