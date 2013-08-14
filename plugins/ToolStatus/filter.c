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
    BOOLEAN showItem = FALSE;
    PPH_STRING textboxText = NULL;
    PH_STRINGREF stringRef;
    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;

    textboxText = PhGetWindowText(TextboxHandle);

    if (PhIsNullOrEmptyString(textboxText))
        return TRUE;

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
        PhInitializeStringRef(&stringRef, processNode->ProcessItem->ProcessIdString);
        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->ParentProcessIdString)
    {
        PhInitializeStringRef(&stringRef, processNode->ProcessItem->ParentProcessIdString);
        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->SessionIdString)
    {
        PhInitializeStringRef(&stringRef, processNode->ProcessItem->SessionIdString);
        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    if (processNode->ProcessItem->PackageFullName)
    {
        PhInitializeStringRef(&stringRef, processNode->ProcessItem->PackageFullName->Buffer);
        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    if (WINDOWS_HAS_UAC)
    {
        switch (processNode->ProcessItem->ElevationType)
        {
        case TokenElevationTypeLimited:
            PhInitializeStringRef(&stringRef, L"Limited");
            break;
        case TokenElevationTypeFull:
            PhInitializeStringRef(&stringRef, L"Full");
            break;
        default:
        case TokenElevationTypeDefault:
            PhInitializeStringRef(&stringRef, L"N/A");
            break;
        }

        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    switch (processNode->ProcessItem->VerifyResult)
    {
    case VrNoSignature:
        PhInitializeStringRef(&stringRef, L"NoSignature");
        break;
    case VrTrusted:
        PhInitializeStringRef(&stringRef, L"Trusted");
        break;
    case VrExpired:
        PhInitializeStringRef(&stringRef, L"Expired");
        break;
    case VrRevoked:
        PhInitializeStringRef(&stringRef, L"Revoked");
        break;
    case VrDistrust:
        PhInitializeStringRef(&stringRef, L"Distrust");
        break;
    case VrSecuritySettings:
        PhInitializeStringRef(&stringRef, L"SecuritySettings");
        break;
    case VrBadSignature:
        PhInitializeStringRef(&stringRef, L"BadSignature");
        break;
    default:
    case VrUnknown:
        PhInitializeStringRef(&stringRef, L"Unknown");
        break;
    }
    if (WordMatch(&stringRef, &textboxText->sr))
        showItem = TRUE;

    PhInitializeStringRef(&stringRef, PhGetProcessPriorityClassString(processNode->ProcessItem->PriorityClass));
    if (WordMatch(&stringRef, &textboxText->sr))
        showItem = TRUE;

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

BOOLEAN ServiceTreeFilterCallback(
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    )
{
    BOOLEAN showItem = FALSE;
    PPH_STRING textboxText = NULL;
    PH_STRINGREF stringRef;
    PPH_SERVICE_NODE serviceNode = (PPH_SERVICE_NODE)Node;

    textboxText = PhGetWindowText(TextboxHandle);

    if (PhIsNullOrEmptyString(textboxText))
        return TRUE;

    PhInitializeStringRef(&stringRef, PhGetServiceTypeString(serviceNode->ServiceItem->Type));
    if (WordMatch(&stringRef, &textboxText->sr))
        showItem = TRUE;

    PhInitializeStringRef(&stringRef, PhGetServiceStateString(serviceNode->ServiceItem->State));
    if (WordMatch(&stringRef, &textboxText->sr))
        showItem = TRUE;

    PhInitializeStringRef(&stringRef, PhGetServiceStartTypeString(serviceNode->ServiceItem->StartType));
    if (WordMatch(&stringRef, &textboxText->sr))
        showItem = TRUE;

    PhInitializeStringRef(&stringRef, PhGetServiceErrorControlString(serviceNode->ServiceItem->ErrorControl));
    if (WordMatch(&stringRef, &textboxText->sr))
        showItem = TRUE;

    if (serviceNode->ServiceItem->Name)
    {
        PhInitializeStringRef(&stringRef, serviceNode->ServiceItem->Name->Buffer);
        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    if (serviceNode->ServiceItem->DisplayName)
    {
        PhInitializeStringRef(&stringRef, serviceNode->ServiceItem->DisplayName->Buffer);
        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    if (serviceNode->ServiceItem->ProcessIdString)
    {
        PhInitializeStringRef(&stringRef, serviceNode->ServiceItem->ProcessIdString);
        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    PhDereferenceObject(textboxText);
    return showItem;
}

BOOLEAN NetworkTreeFilterCallback(
    __in PPH_TREENEW_NODE Node,
    __in_opt PVOID Context
    )
{
    BOOLEAN showItem = FALSE;
    PPH_STRING textboxText = NULL;
    PH_STRINGREF stringRef;
    PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)Node;

    textboxText = PhGetWindowText(TextboxHandle);

    if (PhIsNullOrEmptyString(textboxText))
        return TRUE;

    if (networkNode->NetworkItem->ProcessName)
    {
        PhInitializeStringRef(&stringRef, networkNode->NetworkItem->ProcessName->Buffer);
        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->OwnerName)
    {
        PhInitializeStringRef(&stringRef, networkNode->NetworkItem->OwnerName->Buffer);
        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->LocalAddressString)
    {
        PhInitializeStringRef(&stringRef, networkNode->NetworkItem->LocalAddressString);
        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->LocalPortString)
    {
        PhInitializeStringRef(&stringRef, networkNode->NetworkItem->LocalPortString);
        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->LocalHostString)
    {
        PhInitializeStringRef(&stringRef, networkNode->NetworkItem->LocalHostString->Buffer);
        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->RemoteAddressString)
    {
        PhInitializeStringRef(&stringRef, networkNode->NetworkItem->RemoteAddressString);
        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->RemotePortString)
    {
        PhInitializeStringRef(&stringRef, networkNode->NetworkItem->RemotePortString);
        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    if (networkNode->NetworkItem->RemoteHostString)
    {
        PhInitializeStringRef(&stringRef, networkNode->NetworkItem->RemoteHostString->Buffer);
        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    {
        WCHAR pidString[32];
        PhPrintUInt32(pidString, HandleToUlong(networkNode->NetworkItem->ProcessId));
        PhInitializeStringRef(&stringRef, pidString);

        if (WordMatch(&stringRef, &textboxText->sr))
            showItem = TRUE;
    }

    PhDereferenceObject(textboxText);
    return showItem;
}