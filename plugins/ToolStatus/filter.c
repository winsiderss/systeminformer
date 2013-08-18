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

static BOOLEAN WordMatch(
    __in PPH_STRINGREF Text
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

static BOOLEAN WordMatch2(
    __in PWSTR Text
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
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    )
{
    BOOLEAN showItem = FALSE;
    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;

    if (PhIsNullOrEmptyString(SearchboxText))
        return TRUE;

    if (WordMatch2(PhGetProcessPriorityClassString(processNode->ProcessItem->PriorityClass)))
        showItem = TRUE;

    if (processNode->ProcessItem->ProcessName)
    {
        if (WordMatch(&processNode->ProcessItem->ProcessName->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->FileName)
    {
        if (WordMatch(&processNode->ProcessItem->FileName->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->CommandLine)
    {
        if (WordMatch(&processNode->ProcessItem->CommandLine->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->VersionInfo.CompanyName)
    {
        if (WordMatch(&processNode->ProcessItem->VersionInfo.CompanyName->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->VersionInfo.FileDescription)
    {
        if (WordMatch(&processNode->ProcessItem->VersionInfo.FileDescription->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->VersionInfo.FileVersion)
    {
        if (WordMatch(&processNode->ProcessItem->VersionInfo.FileVersion->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->VersionInfo.ProductName)
    {
        if (WordMatch(&processNode->ProcessItem->VersionInfo.ProductName->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->UserName)
    {
        if (WordMatch(&processNode->ProcessItem->UserName->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->IntegrityString)
    {
        if (WordMatch2(processNode->ProcessItem->IntegrityString))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->JobName)
    {
        if (WordMatch(&processNode->ProcessItem->JobName->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->VerifySignerName)
    {
        if (WordMatch(&processNode->ProcessItem->VerifySignerName->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->ProcessIdString)
    {
        if (WordMatch2(processNode->ProcessItem->ProcessIdString))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->ParentProcessIdString)
    {
        if (WordMatch2(processNode->ProcessItem->ParentProcessIdString))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->SessionIdString)
    {
        if (WordMatch2(processNode->ProcessItem->SessionIdString))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->PackageFullName)
    {
        if (WordMatch(&processNode->ProcessItem->PackageFullName->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->VerifyResult != VrUnknown)
    {
        switch (processNode->ProcessItem->VerifyResult)
        {
        case VrNoSignature:
            if (WordMatch2(L"NoSignature"))
                showItem = TRUE;
            break;
        case VrTrusted:
            if (WordMatch2(L"Trusted"))
                showItem = TRUE;
            break;
        case VrExpired:
            if (WordMatch2(L"Expired"))
                showItem = TRUE;
            break;
        case VrRevoked:
            if (WordMatch2(L"Revoked"))
                showItem = TRUE;
            break;
        case VrDistrust:
            if (WordMatch2(L"Distrust"))
                showItem = TRUE;
            break;
        case VrSecuritySettings:
            if (WordMatch2(L"SecuritySettings"))
                showItem = TRUE;
            break;
        case VrBadSignature:
            if (WordMatch2(L"BadSignature"))
                showItem = TRUE;
            break;
        default:
            if (WordMatch2(L"Unknown"))
                showItem = TRUE;
            break;
        }
    }

    if (WINDOWS_HAS_UAC && processNode->ProcessItem->ElevationType != TokenElevationTypeDefault)
    {
        switch (processNode->ProcessItem->ElevationType)
        {
        case TokenElevationTypeLimited:
            if (WordMatch2(L"Limited"))
                showItem = TRUE;
            break;
        case TokenElevationTypeFull:
            if (WordMatch2(L"Full"))
                showItem = TRUE;
            break;
        default:
            if (WordMatch2(L"Unknown"))
                showItem = TRUE;
            break;
        }
    }

    if (WordMatch2(L"UpdateIsDotNet") && processNode->ProcessItem->UpdateIsDotNet)
    {
        showItem = TRUE;
    }

    if (WordMatch2(L"IsBeingDebugged") && processNode->ProcessItem->IsBeingDebugged)
    {
        showItem = TRUE;
    }

    if (WordMatch2(L"IsDotNet") && processNode->ProcessItem->IsDotNet)
    {
        showItem = TRUE;
    }

    if (WordMatch2(L"IsElevated") && processNode->ProcessItem->IsElevated)
    {
        showItem = TRUE;
    }

    if (WordMatch2(L"IsInJob") && processNode->ProcessItem->IsInJob)
    {
        showItem = TRUE;
    }

    if (WordMatch2(L"IsInSignificantJob") && processNode->ProcessItem->IsInSignificantJob)
    {
        showItem = TRUE;
    }

    if (WordMatch2(L"IsPacked") && processNode->ProcessItem->IsPacked)
    {
        showItem = TRUE;
    }

    if (WordMatch2(L"IsPosix") && processNode->ProcessItem->IsPosix)
    {
        showItem = TRUE;
    }

    if (WordMatch2(L"IsSuspended") && processNode->ProcessItem->IsSuspended)
    {
        showItem = TRUE;
    }

    if (WordMatch2(L"IsWow64") && processNode->ProcessItem->IsWow64)
    {
        showItem = TRUE;
    }

    if (WordMatch2(L"IsImmersive") && processNode->ProcessItem->IsImmersive)
    {
        showItem = TRUE;
    }

    return showItem;
}

BOOLEAN ServiceTreeFilterCallback(
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    )
{
    BOOLEAN showItem = FALSE;
    PPH_SERVICE_NODE serviceNode = (PPH_SERVICE_NODE)Node;

    if (PhIsNullOrEmptyString(SearchboxText))
        return TRUE;

    if (WordMatch2(PhGetServiceTypeString(serviceNode->ServiceItem->Type)))
        showItem = TRUE;

    if (WordMatch2(PhGetServiceStateString(serviceNode->ServiceItem->State)))
        showItem = TRUE;

    if (WordMatch2(PhGetServiceStartTypeString(serviceNode->ServiceItem->StartType)))
        showItem = TRUE;

    if (WordMatch2(PhGetServiceErrorControlString(serviceNode->ServiceItem->ErrorControl)))
        showItem = TRUE;

    if (serviceNode->ServiceItem->Name)
    {
        if (WordMatch(&serviceNode->ServiceItem->Name->sr))
            showItem = TRUE;
    }

    if (serviceNode->ServiceItem->DisplayName)
    {
        if (WordMatch(&serviceNode->ServiceItem->DisplayName->sr))
            showItem = TRUE;
    }

    if (serviceNode->ServiceItem->ProcessIdString)
    {
        if (WordMatch2(serviceNode->ServiceItem->ProcessIdString))
            showItem = TRUE;
    }

    return showItem;
}

BOOLEAN NetworkTreeFilterCallback(
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    )
{
    BOOLEAN showItem = FALSE;
    PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)Node;

    if (PhIsNullOrEmptyString(SearchboxText))
        return TRUE;

    if (networkNode->NetworkItem->ProcessName)
    {
        if (WordMatch(&networkNode->NetworkItem->ProcessName->sr))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->OwnerName)
    {
        if (WordMatch(&networkNode->NetworkItem->OwnerName->sr))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->LocalAddressString)
    {
        if (WordMatch2(networkNode->NetworkItem->LocalAddressString))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->LocalPortString)
    {
        if (WordMatch2(networkNode->NetworkItem->LocalPortString))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->LocalHostString)
    {
        if (WordMatch(&networkNode->NetworkItem->LocalHostString->sr))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->RemoteAddressString)
    {
        if (WordMatch2(networkNode->NetworkItem->RemoteAddressString))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->RemotePortString)
    {
        if (WordMatch2(networkNode->NetworkItem->RemotePortString))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->RemoteHostString)
    {
        if (WordMatch(&networkNode->NetworkItem->RemoteHostString->sr))
            showItem = TRUE;
    }

    {
        WCHAR pidString[32];

        PhPrintUInt32(pidString, HandleToUlong(networkNode->NetworkItem->ProcessId));

        if (WordMatch2(pidString))
            showItem = TRUE;
    }

    return showItem;
}