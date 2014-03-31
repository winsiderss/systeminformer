/*
 * Process Hacker ToolStatus -
 *   search filter callbacks
 *
 * Copyright (C) 2011-2013 dmex
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

static BOOLEAN WordMatchStringRef(
    _In_ PPH_STRINGREF Text
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;
 
    remainingPart = SearchboxText->sr;

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

static BOOLEAN WordMatchString(
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    PhInitializeStringRef(&text, Text);

    remainingPart = SearchboxText->sr;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, ' ', &part, &remainingPart);

        if (part.Length != 0)
        {
            if (PhFindStringInStringRef(&text, &part, TRUE) != -1)
                return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN ProcessTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    BOOLEAN showItem = FALSE;
    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;

    if (PhIsNullOrEmptyString(SearchboxText))
        return TRUE;

    if (processNode->ProcessItem->ProcessName)
    {
        if (WordMatchStringRef(&processNode->ProcessItem->ProcessName->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->FileName)
    {
        if (WordMatchStringRef(&processNode->ProcessItem->FileName->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->CommandLine)
    {
        if (WordMatchStringRef(&processNode->ProcessItem->CommandLine->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->VersionInfo.CompanyName)
    {
        if (WordMatchStringRef(&processNode->ProcessItem->VersionInfo.CompanyName->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->VersionInfo.FileDescription)
    {
        if (WordMatchStringRef(&processNode->ProcessItem->VersionInfo.FileDescription->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->VersionInfo.FileVersion)
    {
        if (WordMatchStringRef(&processNode->ProcessItem->VersionInfo.FileVersion->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->VersionInfo.ProductName)
    {
        if (WordMatchStringRef(&processNode->ProcessItem->VersionInfo.ProductName->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->UserName)
    {
        if (WordMatchStringRef(&processNode->ProcessItem->UserName->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->IntegrityString)
    {
        if (WordMatchString(processNode->ProcessItem->IntegrityString))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->JobName)
    {
        if (WordMatchStringRef(&processNode->ProcessItem->JobName->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->VerifySignerName)
    {
        if (WordMatchStringRef(&processNode->ProcessItem->VerifySignerName->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->ProcessIdString)
    {
        if (WordMatchString(processNode->ProcessItem->ProcessIdString))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->ParentProcessIdString)
    {
        if (WordMatchString(processNode->ProcessItem->ParentProcessIdString))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->SessionIdString)
    {
        if (WordMatchString(processNode->ProcessItem->SessionIdString))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->PackageFullName)
    {
        if (WordMatchStringRef(&processNode->ProcessItem->PackageFullName->sr))
            showItem = TRUE;
    }

    if (WordMatchString(PhGetProcessPriorityClassString(processNode->ProcessItem->PriorityClass)))
        showItem = TRUE;

    if (processNode->ProcessItem->VerifyResult != VrUnknown)
    {
        switch (processNode->ProcessItem->VerifyResult)
        {
        case VrNoSignature:
            if (WordMatchString(L"NoSignature"))
                showItem = TRUE;
            break;
        case VrTrusted:
            if (WordMatchString(L"Trusted"))
                showItem = TRUE;
            break;
        case VrExpired:
            if (WordMatchString(L"Expired"))
                showItem = TRUE;
            break;
        case VrRevoked:
            if (WordMatchString(L"Revoked"))
                showItem = TRUE;
            break;
        case VrDistrust:
            if (WordMatchString(L"Distrust"))
                showItem = TRUE;
            break;
        case VrSecuritySettings:
            if (WordMatchString(L"SecuritySettings"))
                showItem = TRUE;
            break;
        case VrBadSignature:
            if (WordMatchString(L"BadSignature"))
                showItem = TRUE;
            break;
        default:
            if (WordMatchString(L"Unknown"))
                showItem = TRUE;
            break;
        }
    }

    if (WINDOWS_HAS_UAC && processNode->ProcessItem->ElevationType != TokenElevationTypeDefault)
    {
        switch (processNode->ProcessItem->ElevationType)
        {
        case TokenElevationTypeLimited:
            if (WordMatchString(L"Limited"))
                showItem = TRUE;
            break;
        case TokenElevationTypeFull:
            if (WordMatchString(L"Full"))
                showItem = TRUE;
            break;
        default:
            if (WordMatchString(L"Unknown"))
                showItem = TRUE;
            break;
        }
    }

    if (WordMatchString(L"UpdateIsDotNet") && processNode->ProcessItem->UpdateIsDotNet)
    {
        showItem = TRUE;
    }

    if (WordMatchString(L"IsBeingDebugged") && processNode->ProcessItem->IsBeingDebugged)
    {
        showItem = TRUE;
    }

    if (WordMatchString(L"IsDotNet") && processNode->ProcessItem->IsDotNet)
    {
        showItem = TRUE;
    }

    if (WordMatchString(L"IsElevated") && processNode->ProcessItem->IsElevated)
    {
        showItem = TRUE;
    }

    if (WordMatchString(L"IsInJob") && processNode->ProcessItem->IsInJob)
    {
        showItem = TRUE;
    }

    if (WordMatchString(L"IsInSignificantJob") && processNode->ProcessItem->IsInSignificantJob)
    {
        showItem = TRUE;
    }

    if (WordMatchString(L"IsPacked") && processNode->ProcessItem->IsPacked)
    {
        showItem = TRUE;
    }

    if (WordMatchString(L"IsPosix") && processNode->ProcessItem->IsPosix)
    {
        showItem = TRUE;
    }

    if (WordMatchString(L"IsSuspended") && processNode->ProcessItem->IsSuspended)
    {
        showItem = TRUE;
    }

    if (WordMatchString(L"IsWow64") && processNode->ProcessItem->IsWow64)
    {
        showItem = TRUE;
    }

    if (WordMatchString(L"IsImmersive") && processNode->ProcessItem->IsImmersive)
    {
        showItem = TRUE;
    }

    if (processNode->ProcessItem->ServiceList && processNode->ProcessItem->ServiceList->Count != 0)
    {
        ULONG enumerationKey = 0;
        PPH_SERVICE_ITEM serviceItem;
        PPH_LIST serviceList;
        ULONG i;

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

        // Add the services.
        for (i = 0; i < serviceList->Count; i++)
        {
            serviceItem = serviceList->Items[i];

            if (WordMatchString(PhGetServiceTypeString(serviceItem->Type)))
                showItem = TRUE;

            if (WordMatchString(PhGetServiceStateString(serviceItem->State)))
                showItem = TRUE;

            if (WordMatchString(PhGetServiceStartTypeString(serviceItem->StartType)))
                showItem = TRUE;

            if (WordMatchString(PhGetServiceErrorControlString(serviceItem->ErrorControl)))
                showItem = TRUE;

            if (serviceItem->Name)
            {
                if (WordMatchStringRef(&serviceItem->Name->sr))
                    showItem = TRUE;
            }

            if (serviceItem->DisplayName)
            {
                if (WordMatchStringRef(&serviceItem->DisplayName->sr))
                    showItem = TRUE;
            }

            if (serviceItem->ProcessIdString)
            {
                if (WordMatchString(serviceItem->ProcessIdString))
                    showItem = TRUE;
            }
        }

        PhDereferenceObjects(serviceList->Items, serviceList->Count);
        PhDereferenceObject(serviceList);
    }

    return showItem;
}

BOOLEAN ServiceTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    BOOLEAN showItem = FALSE;
    PPH_SERVICE_NODE serviceNode = (PPH_SERVICE_NODE)Node;

    if (PhIsNullOrEmptyString(SearchboxText))
        return TRUE;

    if (WordMatchString(PhGetServiceTypeString(serviceNode->ServiceItem->Type)))
        showItem = TRUE;

    if (WordMatchString(PhGetServiceStateString(serviceNode->ServiceItem->State)))
        showItem = TRUE;

    if (WordMatchString(PhGetServiceStartTypeString(serviceNode->ServiceItem->StartType)))
        showItem = TRUE;

    if (WordMatchString(PhGetServiceErrorControlString(serviceNode->ServiceItem->ErrorControl)))
        showItem = TRUE;

    if (serviceNode->ServiceItem->Name)
    {
        if (WordMatchStringRef(&serviceNode->ServiceItem->Name->sr))
            showItem = TRUE;
    }

    if (serviceNode->ServiceItem->DisplayName)
    {
        if (WordMatchStringRef(&serviceNode->ServiceItem->DisplayName->sr))
            showItem = TRUE;
    }

    if (serviceNode->ServiceItem->ProcessIdString)
    {
        if (WordMatchString(serviceNode->ServiceItem->ProcessIdString))
            showItem = TRUE;
    }

    return showItem;
}

BOOLEAN NetworkTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    BOOLEAN showItem = FALSE;
    PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)Node;

    if (PhIsNullOrEmptyString(SearchboxText))
        return TRUE;

    if (networkNode->NetworkItem->ProcessName)
    {
        if (WordMatchStringRef(&networkNode->NetworkItem->ProcessName->sr))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->OwnerName)
    {
        if (WordMatchStringRef(&networkNode->NetworkItem->OwnerName->sr))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->LocalAddressString)
    {
        if (WordMatchString(networkNode->NetworkItem->LocalAddressString))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->LocalPortString)
    {
        if (WordMatchString(networkNode->NetworkItem->LocalPortString))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->LocalHostString)
    {
        if (WordMatchStringRef(&networkNode->NetworkItem->LocalHostString->sr))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->RemoteAddressString)
    {
        if (WordMatchString(networkNode->NetworkItem->RemoteAddressString))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->RemotePortString)
    {
        if (WordMatchString(networkNode->NetworkItem->RemotePortString))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->RemoteHostString)
    {
        if (WordMatchStringRef(&networkNode->NetworkItem->RemoteHostString->sr))
            showItem = TRUE;
    }

    {
        WCHAR pidString[32];

        PhPrintUInt32(pidString, HandleToUlong(networkNode->NetworkItem->ProcessId));

        if (WordMatchString(pidString))
            showItem = TRUE;
    }

    return showItem;
}