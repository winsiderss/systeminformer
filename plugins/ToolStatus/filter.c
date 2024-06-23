/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2011-2023
 *
 */

#include "toolstatus.h"
#include <verify.h>

BOOLEAN WordMatchStringRef(
    _In_ PPH_STRINGREF Text
    )
{
    return PhSearchControlMatch(SearchMatchHandle, Text);
}

BOOLEAN ProcessTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;

    if (!SearchMatchHandle)
        return TRUE;

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->ProcessName))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &processNode->ProcessItem->ProcessName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->FileNameWin32))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &processNode->ProcessItem->FileNameWin32->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->FileName))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &processNode->ProcessItem->FileName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->CommandLine))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &processNode->ProcessItem->CommandLine->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.CompanyName))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &processNode->ProcessItem->VersionInfo.CompanyName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.FileDescription))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &processNode->ProcessItem->VersionInfo.FileDescription->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.FileVersion))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &processNode->ProcessItem->VersionInfo.FileVersion->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.ProductName))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &processNode->ProcessItem->VersionInfo.ProductName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->UserName))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &processNode->ProcessItem->UserName->sr))
            return TRUE;
    }

    if (processNode->ProcessItem->IntegrityString)
    {
        if (PhSearchControlMatchLongHintZ(SearchMatchHandle, processNode->ProcessItem->IntegrityString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VerifySignerName))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &processNode->ProcessItem->VerifySignerName->sr))
            return TRUE;
    }

    if (PH_IS_REAL_PROCESS_ID(processNode->ProcessItem->ProcessId))
    {
        if (processNode->ProcessItem->ProcessIdString[0])
        {
            if (PhSearchControlMatchLongHintZ(SearchMatchHandle, processNode->ProcessItem->ProcessIdString))
                return TRUE;
        }

        if (processNode->ProcessItem->ProcessIdHexString[0])
        {
            if (PhSearchControlMatchLongHintZ(SearchMatchHandle, processNode->ProcessItem->ProcessIdHexString))
                return TRUE;
        }
    }

    if (processNode->ProcessItem->LxssProcessIdString[0])
    {
        if (PhSearchControlMatchLongHintZ(SearchMatchHandle, processNode->ProcessItem->LxssProcessIdString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->PackageFullName))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &processNode->ProcessItem->PackageFullName->sr))
            return TRUE;
    }

    {
        PPH_STRINGREF value;

        if (value = PhGetProcessPriorityClassString(processNode->ProcessItem->PriorityClass))
        {
            if (PhSearchControlMatch(SearchMatchHandle, value))
            {
                return TRUE;
            }
        }
    }

    if (processNode->ProcessItem->VerifyResult != VrUnknown)
    {
        switch (processNode->ProcessItem->VerifyResult)
        {
        case VrNoSignature:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"NoSignature"))
                return TRUE;
            break;
        case VrTrusted:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"Trusted"))
                return TRUE;
            break;
        case VrExpired:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"Expired"))
                return TRUE;
            break;
        case VrRevoked:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"Revoked"))
                return TRUE;
            break;
        case VrDistrust:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"Distrust"))
                return TRUE;
            break;
        case VrSecuritySettings:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"SecuritySettings"))
                return TRUE;
            break;
        case VrBadSignature:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"BadSignature"))
                return TRUE;
            break;
        default:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"Unknown"))
                return TRUE;
            break;
        }
    }

    if (processNode->ProcessItem->ElevationType != TokenElevationTypeDefault)
    {
        switch (processNode->ProcessItem->ElevationType)
        {
        case TokenElevationTypeLimited:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"Limited"))
                return TRUE;
            break;
        case TokenElevationTypeFull:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"Full"))
                return TRUE;
            break;
        default:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"Unknown"))
                return TRUE;
            break;
        }
    }

    if (processNode->ProcessItem->IsBeingDebugged && PhSearchControlMatchZ(SearchMatchHandle, L"IsBeingDebugged"))
    {
        return TRUE;
    }

    if (processNode->ProcessItem->IsDotNet && PhSearchControlMatchZ(SearchMatchHandle, L"IsDotNet"))
    {
        return TRUE;
    }

    if (processNode->ProcessItem->IsElevated && PhSearchControlMatchZ(SearchMatchHandle, L"IsElevated"))
    {
        return TRUE;
    }

    if (processNode->ProcessItem->IsInJob && PhSearchControlMatchZ(SearchMatchHandle, L"IsInJob"))
    {
        return TRUE;
    }

    if (processNode->ProcessItem->IsInSignificantJob && PhSearchControlMatchZ(SearchMatchHandle, L"IsInSignificantJob"))
    {
        return TRUE;
    }

    if (processNode->ProcessItem->IsPacked && PhSearchControlMatchZ(SearchMatchHandle, L"IsPacked"))
    {
        return TRUE;
    }

    if (processNode->ProcessItem->IsSuspended && PhSearchControlMatchZ(SearchMatchHandle, L"IsSuspended"))
    {
        return TRUE;
    }

    if (processNode->ProcessItem->IsWow64 && PhSearchControlMatchZ(SearchMatchHandle, L"IsWow64"))
    {
        return TRUE;
    }

    if (processNode->ProcessItem->IsImmersive && PhSearchControlMatchZ(SearchMatchHandle, L"IsImmersive"))
    {
        return TRUE;
    }

    if (processNode->ProcessItem->IsPackagedProcess && PhSearchControlMatchZ(SearchMatchHandle, L"IsPackagedProcess"))
    {
        return TRUE;
    }

    if (processNode->ProcessItem->IsProtectedProcess && PhSearchControlMatchZ(SearchMatchHandle, L"IsProtectedProcess"))
    {
        return TRUE;
    }

    if (processNode->ProcessItem->IsSecureProcess && PhSearchControlMatchZ(SearchMatchHandle, L"IsSecureProcess"))
    {
        return TRUE;
    }

    if (processNode->ProcessItem->IsSubsystemProcess && PhSearchControlMatchZ(SearchMatchHandle, L"IsPicoProcess"))
    {
        return TRUE;
    }

    if (processNode->ProcessItem->ServiceList && processNode->ProcessItem->ServiceList->Count)
    {
        ULONG enumerationKey = 0;
        PPH_SERVICE_ITEM serviceItem;
        PPH_LIST serviceList;
        ULONG i;
        BOOLEAN matched = FALSE;

        PhAcquireQueuedLockShared(&processNode->ProcessItem->ServiceListLock);

        // Copy the service list so we can search it.
        serviceList = PhCreateList(processNode->ProcessItem->ServiceList->Count);

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
                if (PhSearchControlMatch(SearchMatchHandle, &serviceItem->Name->sr))
                {
                    matched = TRUE;
                    break;
                }
            }

            if (!PhIsNullOrEmptyString(serviceItem->DisplayName))
            {
                if (PhSearchControlMatch(SearchMatchHandle, &serviceItem->DisplayName->sr))
                {
                    matched = TRUE;
                    break;
                }
            }

            if (serviceItem->ProcessId)
            {
                if (PhSearchControlMatchLongHintZ(SearchMatchHandle, serviceItem->ProcessIdString))
                {
                    matched = TRUE;
                    break;
                }
            }

            if (!PhIsNullOrEmptyString(serviceItem->FileName))
            {
                if (PhSearchControlMatch(SearchMatchHandle, &serviceItem->FileName->sr))
                {
                    matched = TRUE;
                    break;
                }
            }

            {
                PPH_SERVICE_NODE serviceNode;

                // Search the service node
                if (!PtrToUlong(Context) && (serviceNode = PhFindServiceNode(serviceItem)))
                {
                    if (ServiceTreeFilterCallback(&serviceNode->Node, UlongToPtr(TRUE)))
                        return TRUE;
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

    if (!SearchMatchHandle)
        return TRUE;

    if (PhSearchControlMatch(SearchMatchHandle, PhGetServiceTypeString(serviceNode->ServiceItem->Type)))
        return TRUE;

    if (PhSearchControlMatch(SearchMatchHandle, PhGetServiceTypeString(serviceNode->ServiceItem->State)))
        return TRUE;

    if (PhSearchControlMatch(SearchMatchHandle, PhGetServiceTypeString(serviceNode->ServiceItem->StartType)))
        return TRUE;

    if (PhSearchControlMatch(SearchMatchHandle, PhGetServiceTypeString(serviceNode->ServiceItem->ErrorControl)))
        return TRUE;

    if (!PhIsNullOrEmptyString(serviceNode->ServiceItem->Name))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &serviceNode->ServiceItem->Name->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(serviceNode->ServiceItem->DisplayName))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &serviceNode->ServiceItem->DisplayName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(serviceNode->ServiceItem->VerifySignerName))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &serviceNode->ServiceItem->VerifySignerName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(serviceNode->ServiceItem->FileName))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &serviceNode->ServiceItem->FileName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(serviceNode->BinaryPath))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &serviceNode->BinaryPath->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(serviceNode->LoadOrderGroup))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &serviceNode->LoadOrderGroup->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(serviceNode->Description))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &serviceNode->Description->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(serviceNode->KeyModifiedTimeText))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &serviceNode->KeyModifiedTimeText->sr))
            return TRUE;
    }

    if (serviceNode->ServiceItem->ProcessId)
    {
        PPH_PROCESS_NODE processNode;

        if (PhSearchControlMatchLongHintZ(SearchMatchHandle, serviceNode->ServiceItem->ProcessIdString))
            return TRUE;

        // Search the process node
        if (!PtrToUlong(Context) && (processNode = PhFindProcessNode(serviceNode->ServiceItem->ProcessId)))
        {
            if (ProcessTreeFilterCallback(&processNode->Node, UlongToPtr(TRUE)))
                return TRUE;
        }
    }

    if (serviceNode->ServiceItem->VerifyResult != VrUnknown)
    {
        switch (serviceNode->ServiceItem->VerifyResult)
        {
        case VrNoSignature:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"NoSignature"))
                return TRUE;
            break;
        case VrTrusted:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"Trusted"))
                return TRUE;
            break;
        case VrExpired:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"Expired"))
                return TRUE;
            break;
        case VrRevoked:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"Revoked"))
                return TRUE;
            break;
        case VrDistrust:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"Distrust"))
                return TRUE;
            break;
        case VrSecuritySettings:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"SecuritySettings"))
                return TRUE;
            break;
        case VrBadSignature:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"BadSignature"))
                return TRUE;
            break;
        default:
            if (PhSearchControlMatchZ(SearchMatchHandle, L"Unknown"))
                return TRUE;
            break;
        }
    }

    return FALSE;
}

BOOLEAN NetworkTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)Node;

    if (!SearchMatchHandle)
        return TRUE;

    if (!PhIsNullOrEmptyString(networkNode->ProcessNameText))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &networkNode->ProcessNameText->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->TimeStampText))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &networkNode->TimeStampText->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->ProcessName))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &networkNode->NetworkItem->ProcessName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->OwnerName))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &networkNode->NetworkItem->OwnerName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->LocalAddressString))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &networkNode->NetworkItem->LocalAddressString->sr))
            return TRUE;
    }

    if (networkNode->NetworkItem->LocalPortString[0])
    {
        if (PhSearchControlMatchLongHintZ(SearchMatchHandle, networkNode->NetworkItem->LocalPortString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->LocalHostString))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &networkNode->NetworkItem->LocalHostString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->RemoteAddressString))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &networkNode->NetworkItem->RemoteAddressString->sr))
            return TRUE;
    }

    if (networkNode->NetworkItem->RemotePortString[0])
    {
        if (PhSearchControlMatchLongHintZ(SearchMatchHandle, networkNode->NetworkItem->RemotePortString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->RemoteHostString))
    {
        if (PhSearchControlMatch(SearchMatchHandle, &networkNode->NetworkItem->RemoteHostString->sr))
            return TRUE;
    }

    {
        PPH_STRINGREF protocolType;

        if (protocolType = PhGetProtocolTypeName(networkNode->NetworkItem->ProtocolType))
        {
            if (PhSearchControlMatch(SearchMatchHandle, protocolType))
                return TRUE;
        }
    }

    {
        if (FlagOn(networkNode->NetworkItem->ProtocolType, PH_TCP_PROTOCOL_TYPE))
        {
            PPH_STRINGREF stateName;

            if (stateName = PhGetTcpStateName(networkNode->NetworkItem->State))
            {
                if (PhSearchControlMatch(SearchMatchHandle, stateName))
                    return TRUE;
            }
        }
    }

    {
        PPH_PROCESS_NODE processNode;

        // Search the process node
        if (processNode = PhFindProcessNode(networkNode->NetworkItem->ProcessId))
        {
            if (ProcessTreeFilterCallback(&processNode->Node, NULL))
                return TRUE;
        }
    }

    return FALSE;
}
