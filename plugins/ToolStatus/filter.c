/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2011-2022
 *
 */

#include "toolstatus.h"
#include <verify.h>

BOOLEAN WordMatchStringRef(
    _In_ PPH_STRINGREF Text
    )
{
    return PhWordMatchStringRef(&SearchboxText->sr, Text);
}

typedef enum
{
    NotMatched = 0,
    Matched = 1,
    MatchedName = 2
} MatchType;

static MatchType ProcessTreeFilterCallback2(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context,
    _In_ PPH_STRING SearchboxText
    )
{
    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;

    if (PhIsNullOrEmptyString(SearchboxText))
        return Matched;

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->ProcessName))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &processNode->ProcessItem->ProcessName->sr))
            return MatchedName;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->FileNameWin32))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &processNode->ProcessItem->FileNameWin32->sr))
            return Matched;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->FileName))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &processNode->ProcessItem->FileName->sr))
            return Matched;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->CommandLine))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &processNode->ProcessItem->CommandLine->sr))
            return Matched;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.CompanyName))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &processNode->ProcessItem->VersionInfo.CompanyName->sr))
            return Matched;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.FileDescription))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &processNode->ProcessItem->VersionInfo.FileDescription->sr))
            return Matched;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.FileVersion))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &processNode->ProcessItem->VersionInfo.FileVersion->sr))
            return Matched;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.ProductName))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &processNode->ProcessItem->VersionInfo.ProductName->sr))
            return Matched;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->UserName))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &processNode->ProcessItem->UserName->sr))
            return Matched;
    }

    if (processNode->ProcessItem->IntegrityString)
    {
        if (PhWordMatchStringZ(SearchboxText, processNode->ProcessItem->IntegrityString))
            return Matched;
    }

    //if (!PhIsNullOrEmptyString(processNode->ProcessItem->JobName))
    //{
    //    if (PhWordMatchStringZ(SearchboxText, &processNode->ProcessItem->JobName->sr))
    //        return Matched;
    //}

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VerifySignerName))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &processNode->ProcessItem->VerifySignerName->sr))
            return Matched;
    }

    if (PH_IS_REAL_PROCESS_ID(processNode->ProcessItem->ProcessId) && processNode->ProcessItem->ProcessIdString[0])
    {
        if (PhWordMatchStringZ(SearchboxText, processNode->ProcessItem->ProcessIdString))
            return MatchedName;

         // HACK PidHexText from PH_PROCESS_NODE is not exported (dmex)
        {
            PH_FORMAT format;
            SIZE_T returnLength;
            PH_STRINGREF processIdHex;
            WCHAR pidHexText[PH_PTR_STR_LEN_1];

            PhInitFormatIX(&format, HandleToUlong(processNode->ProcessItem->ProcessId));

            if (PhFormatToBuffer(&format, 1, pidHexText, sizeof(pidHexText), &returnLength))
            {
                processIdHex.Buffer = pidHexText;
                processIdHex.Length = returnLength - sizeof(UNICODE_NULL);

                if (PhWordMatchStringRef(&SearchboxText->sr, &processIdHex))
                    return MatchedName;
            }
        }
    }

    //if (processNode->ProcessItem->ParentProcessIdString[0])
    //{
    //    if (PhWordMatchStringZ(SearchboxText, processNode->ProcessItem->ParentProcessIdString))
    //        return Matched;
    //}

    //if (processNode->ProcessItem->SessionIdString[0])
    //{
    //    if (PhWordMatchStringZ(SearchboxText, processNode->ProcessItem->SessionIdString))
    //        return Matched;
    //}

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->PackageFullName))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &processNode->ProcessItem->PackageFullName->sr))
            return Matched;
    }

    if (PhWordMatchStringZ(SearchboxText, PhGetProcessPriorityClassString(processNode->ProcessItem->PriorityClass)))
    {
        return Matched;
    }

    if (processNode->ProcessItem->VerifyResult != VrUnknown)
    {
        switch (processNode->ProcessItem->VerifyResult)
        {
        case VrNoSignature:
            if (PhWordMatchStringZ(SearchboxText, L"NoSignature"))
                return Matched;
            break;
        case VrTrusted:
            if (PhWordMatchStringZ(SearchboxText, L"Trusted"))
                return Matched;
            break;
        case VrExpired:
            if (PhWordMatchStringZ(SearchboxText, L"Expired"))
                return Matched;
            break;
        case VrRevoked:
            if (PhWordMatchStringZ(SearchboxText, L"Revoked"))
                return Matched;
            break;
        case VrDistrust:
            if (PhWordMatchStringZ(SearchboxText, L"Distrust"))
                return Matched;
            break;
        case VrSecuritySettings:
            if (PhWordMatchStringZ(SearchboxText, L"SecuritySettings"))
                return Matched;
            break;
        case VrBadSignature:
            if (PhWordMatchStringZ(SearchboxText, L"BadSignature"))
                return Matched;
            break;
        default:
            if (PhWordMatchStringZ(SearchboxText, L"Unknown"))
                return Matched;
            break;
        }
    }

    if (processNode->ProcessItem->ElevationType != TokenElevationTypeDefault)
    {
        switch (processNode->ProcessItem->ElevationType)
        {
        case TokenElevationTypeLimited:
            if (PhWordMatchStringZ(SearchboxText, L"Limited"))
                return Matched;
            break;
        case TokenElevationTypeFull:
            if (PhWordMatchStringZ(SearchboxText, L"Full"))
                return Matched;
            break;
        default:
            if (PhWordMatchStringZ(SearchboxText, L"Unknown"))
                return Matched;
            break;
        }
    }

    if (PhWordMatchStringZ(SearchboxText, L"IsBeingDebugged") && processNode->ProcessItem->IsBeingDebugged)
    {
        return Matched;
    }

    if (PhWordMatchStringZ(SearchboxText, L"IsDotNet") && processNode->ProcessItem->IsDotNet)
    {
        return Matched;
    }

    if (PhWordMatchStringZ(SearchboxText, L"IsElevated") && processNode->ProcessItem->IsElevated)
    {
        return Matched;
    }

    if (PhWordMatchStringZ(SearchboxText, L"IsInJob") && processNode->ProcessItem->IsInJob)
    {
        return Matched;
    }

    if (PhWordMatchStringZ(SearchboxText, L"IsInSignificantJob") && processNode->ProcessItem->IsInSignificantJob)
    {
        return Matched;
    }

    if (PhWordMatchStringZ(SearchboxText, L"IsPacked") && processNode->ProcessItem->IsPacked)
    {
        return Matched;
    }

    if (PhWordMatchStringZ(SearchboxText, L"IsSuspended") && processNode->ProcessItem->IsSuspended)
    {
        return Matched;
    }

    if (PhWordMatchStringZ(SearchboxText, L"IsWow64") && processNode->ProcessItem->IsWow64)
    {
        return Matched;
    }

    if (PhWordMatchStringZ(SearchboxText, L"IsImmersive") && processNode->ProcessItem->IsImmersive)
    {
        return Matched;
    }

    if (PhWordMatchStringZ(SearchboxText, L"IsProtectedProcess") && processNode->ProcessItem->IsProtectedProcess)
    {
        return Matched;
    }

    if (PhWordMatchStringZ(SearchboxText, L"IsSecureProcess") && processNode->ProcessItem->IsSecureProcess)
    {
        return Matched;
    }

    if (PhWordMatchStringZ(SearchboxText, L"IsPicoProcess") && processNode->ProcessItem->IsSubsystemProcess)
    {
        return Matched;
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
                if (PhWordMatchStringRef(&SearchboxText->sr, &serviceItem->Name->sr))
                {
                    matched = TRUE;
                    break;
                }
            }

            if (!PhIsNullOrEmptyString(serviceItem->DisplayName))
            {
                if (PhWordMatchStringRef(&SearchboxText->sr, &serviceItem->DisplayName->sr))
                {
                    matched = TRUE;
                    break;
                }
            }

            if (serviceItem->ProcessId)
            {
                if (PhWordMatchStringZ(SearchboxText, serviceItem->ProcessIdString))
                {
                    matched = TRUE;
                    break;
                }
            }

            if (!PhIsNullOrEmptyString(serviceItem->FileName))
            {
                if (PhWordMatchStringRef(&SearchboxText->sr, &serviceItem->FileName->sr))
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
            return Matched;
    }

    return NotMatched;
}

static PPH_STRING searchboxTextCache = NULL;
static PPH_STRING fullSearchboxText = NULL;
static PPH_STRING parentSearchboxText = NULL;

BOOLEAN ProcessTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
)
{
    if (searchboxTextCache && (PhIsNullOrEmptyString(SearchboxText) || PhCompareString(SearchboxText, searchboxTextCache, TRUE)))
    {
        // clear cache
        if (searchboxTextCache)
        {
            PhDereferenceObject(searchboxTextCache);
            searchboxTextCache = NULL;
        }
        if (fullSearchboxText)
        {
            PhDereferenceObject(fullSearchboxText);
            fullSearchboxText = NULL;
        }
        if (parentSearchboxText)
        {
            PhDereferenceObject(parentSearchboxText);
            parentSearchboxText = NULL;
        }
    }

    if (!searchboxTextCache && !PhIsNullOrEmptyString(SearchboxText))
    {
        // process SearchboxText:
        //    fullSearchboxText will contain full search string stripped of '*' markers to perform as before
        //    parentSearchBoxText will contain only search string parts marked with '*' to search through parents

        searchboxTextCache = PhDuplicateString(SearchboxText);

        if (PhFindCharInString(SearchboxText, L'*', FALSE) != -1)
        {
            PH_STRING_BUILDER fullSearchboxBuilder;
            PhInitializeStringBuilder(&fullSearchboxBuilder, 100);
            PH_STRING_BUILDER parentSearchboxBuilder;
            PhInitializeStringBuilder(&parentSearchboxBuilder, 100);

            PH_STRINGREF part;
            PH_STRINGREF remainingPart;

            PhInitializeStringRef(&remainingPart, PhGetString(SearchboxText));

            while (remainingPart.Length)
            {
                PhSplitStringRefAtChar(&remainingPart, L'|', &part, &remainingPart);

                if (part.Length)
                {
                    static PH_STRINGREF withChildrenMarker = PH_STRINGREF_INIT(L"*");
                    BOOLEAN withChildren = PhEndsWithStringRef(&part, &withChildrenMarker, FALSE);
                    if (withChildren)
                    {
                        PhTrimStringRef(&part, &withChildrenMarker, PH_TRIM_END_ONLY);

                        if (parentSearchboxBuilder.String->Length)
                            PhAppendCharStringBuilder(&parentSearchboxBuilder, L'|');
                        PhAppendStringBuilder(&parentSearchboxBuilder, &part);
                    }

                    if (fullSearchboxBuilder.String->Length)
                        PhAppendCharStringBuilder(&fullSearchboxBuilder, L'|');
                    PhAppendStringBuilder(&fullSearchboxBuilder, &part);
                }
            }

            fullSearchboxText = PhFinalStringBuilderString(&fullSearchboxBuilder);
            parentSearchboxText = PhFinalStringBuilderString(&parentSearchboxBuilder);
        }
    }

    BOOLEAN ret = !!ProcessTreeFilterCallback2(Node, Context, fullSearchboxText ? fullSearchboxText : SearchboxText);

    if (!ret && !PhIsNullOrEmptyString(parentSearchboxText))
    {
        PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;

        for (PPH_PROCESS_NODE parentNode = processNode->Parent; !ret && parentNode; parentNode = parentNode->Parent)
            ret = MatchedName == ProcessTreeFilterCallback2(&parentNode->Node, Context, parentSearchboxText);
    }

    return ret;
}

BOOLEAN ServiceTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_SERVICE_NODE serviceNode = (PPH_SERVICE_NODE)Node;

    if (PhIsNullOrEmptyString(SearchboxText))
        return TRUE;

    if (PhWordMatchStringRef(&SearchboxText->sr, PhGetServiceTypeString(serviceNode->ServiceItem->Type)))
        return TRUE;

    if (PhWordMatchStringRef(&SearchboxText->sr, PhGetServiceStateString(serviceNode->ServiceItem->State)))
        return TRUE;

    if (PhWordMatchStringRef(&SearchboxText->sr, PhGetServiceStartTypeString(serviceNode->ServiceItem->StartType)))
        return TRUE;

    if (PhWordMatchStringRef(&SearchboxText->sr, PhGetServiceErrorControlString(serviceNode->ServiceItem->ErrorControl)))
        return TRUE;

    if (!PhIsNullOrEmptyString(serviceNode->ServiceItem->Name))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &serviceNode->ServiceItem->Name->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(serviceNode->ServiceItem->DisplayName))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &serviceNode->ServiceItem->DisplayName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(serviceNode->ServiceItem->VerifySignerName))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &serviceNode->ServiceItem->VerifySignerName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(serviceNode->ServiceItem->FileName))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &serviceNode->ServiceItem->FileName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(serviceNode->BinaryPath))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &serviceNode->BinaryPath->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(serviceNode->LoadOrderGroup))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &serviceNode->LoadOrderGroup->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(serviceNode->Description))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &serviceNode->Description->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(serviceNode->KeyModifiedTimeText))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &serviceNode->KeyModifiedTimeText->sr))
            return TRUE;
    }

    if (serviceNode->ServiceItem->ProcessId)
    {
        PPH_PROCESS_NODE processNode;

        if (PhWordMatchStringZ(SearchboxText, serviceNode->ServiceItem->ProcessIdString))
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
            if (PhWordMatchStringZ(SearchboxText, L"NoSignature"))
                return TRUE;
            break;
        case VrTrusted:
            if (PhWordMatchStringZ(SearchboxText, L"Trusted"))
                return TRUE;
            break;
        case VrExpired:
            if (PhWordMatchStringZ(SearchboxText, L"Expired"))
                return TRUE;
            break;
        case VrRevoked:
            if (PhWordMatchStringZ(SearchboxText, L"Revoked"))
                return TRUE;
            break;
        case VrDistrust:
            if (PhWordMatchStringZ(SearchboxText, L"Distrust"))
                return TRUE;
            break;
        case VrSecuritySettings:
            if (PhWordMatchStringZ(SearchboxText, L"SecuritySettings"))
                return TRUE;
            break;
        case VrBadSignature:
            if (PhWordMatchStringZ(SearchboxText, L"BadSignature"))
                return TRUE;
            break;
        default:
            if (PhWordMatchStringZ(SearchboxText, L"Unknown"))
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

    if (PhIsNullOrEmptyString(SearchboxText))
        return TRUE;

    if (!PhIsNullOrEmptyString(networkNode->ProcessNameText))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &networkNode->ProcessNameText->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->TimeStampText))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &networkNode->TimeStampText->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->ProcessName))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &networkNode->NetworkItem->ProcessName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->OwnerName))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &networkNode->NetworkItem->OwnerName->sr))
            return TRUE;
    }

    if (networkNode->NetworkItem->LocalAddressString[0])
    {
        if (PhWordMatchStringZ(SearchboxText, networkNode->NetworkItem->LocalAddressString))
            return TRUE;
    }

    if (networkNode->NetworkItem->LocalPortString[0])
    {
        if (PhWordMatchStringZ(SearchboxText, networkNode->NetworkItem->LocalPortString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->LocalHostString))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &networkNode->NetworkItem->LocalHostString->sr))
            return TRUE;
    }

    if (networkNode->NetworkItem->RemoteAddressString[0])
    {
        if (PhWordMatchStringZ(SearchboxText, networkNode->NetworkItem->RemoteAddressString))
            return TRUE;
    }

    if (networkNode->NetworkItem->RemotePortString[0])
    {
        if (PhWordMatchStringZ(SearchboxText, networkNode->NetworkItem->RemotePortString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->RemoteHostString))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &networkNode->NetworkItem->RemoteHostString->sr))
            return TRUE;
    }

    {
        PH_STRINGREF protocolType = PhGetProtocolTypeName(networkNode->NetworkItem->ProtocolType);

        if (PhWordMatchStringRef(&SearchboxText->sr, &protocolType))
            return TRUE;
    }

    {
        if (networkNode->NetworkItem->ProtocolType & PH_TCP_PROTOCOL_TYPE)
        {
            PH_STRINGREF stateName = PhGetTcpStateName(networkNode->NetworkItem->State);

            if (PhWordMatchStringRef(&SearchboxText->sr, &stateName))
                return TRUE;
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
