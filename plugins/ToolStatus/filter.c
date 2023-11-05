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

typedef enum _FILTER_RESULT_TYPE
{
    FILTER_RESULT_NOT_FOUND,
    FILTER_RESULT_FOUND,
    FILTER_RESULT_FOUND_NAME
} FILTER_RESULT_TYPE;

static PPH_STRING SearchboxTextCache = NULL;
static PPH_STRING SearchboxFullTextCache = NULL;
static PPH_STRING SearchboxParentTextCache = NULL;

BOOLEAN WordMatchStringRef(
    _In_ PPH_STRINGREF Text
    )
{
    return PhWordMatchStringRef(&SearchboxText->sr, Text);
}

FILTER_RESULT_TYPE ProcessTreeFilterMatchTypeCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context,
    _In_opt_ PPH_STRING SearchText
    )
{
    PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;

    if (PhIsNullOrEmptyString(SearchText))
        return FILTER_RESULT_FOUND;

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->ProcessName))
    {
        if (PhWordMatchStringRef(&SearchText->sr, &processNode->ProcessItem->ProcessName->sr))
            return FILTER_RESULT_FOUND_NAME;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->FileNameWin32))
    {
        if (PhWordMatchStringRef(&SearchText->sr, &processNode->ProcessItem->FileNameWin32->sr))
            return FILTER_RESULT_FOUND;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->FileName))
    {
        if (PhWordMatchStringRef(&SearchText->sr, &processNode->ProcessItem->FileName->sr))
            return FILTER_RESULT_FOUND;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->CommandLine))
    {
        if (PhWordMatchStringRef(&SearchText->sr, &processNode->ProcessItem->CommandLine->sr))
            return FILTER_RESULT_FOUND;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.CompanyName))
    {
        if (PhWordMatchStringRef(&SearchText->sr, &processNode->ProcessItem->VersionInfo.CompanyName->sr))
            return FILTER_RESULT_FOUND;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.FileDescription))
    {
        if (PhWordMatchStringRef(&SearchText->sr, &processNode->ProcessItem->VersionInfo.FileDescription->sr))
            return FILTER_RESULT_FOUND;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.FileVersion))
    {
        if (PhWordMatchStringRef(&SearchText->sr, &processNode->ProcessItem->VersionInfo.FileVersion->sr))
            return FILTER_RESULT_FOUND;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VersionInfo.ProductName))
    {
        if (PhWordMatchStringRef(&SearchText->sr, &processNode->ProcessItem->VersionInfo.ProductName->sr))
            return FILTER_RESULT_FOUND;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->UserName))
    {
        if (PhWordMatchStringRef(&SearchText->sr, &processNode->ProcessItem->UserName->sr))
            return FILTER_RESULT_FOUND;
    }

    if (processNode->ProcessItem->IntegrityString)
    {
        if (PhWordMatchStringLongHintZ(SearchText, processNode->ProcessItem->IntegrityString))
            return FILTER_RESULT_FOUND;
    }

    //if (!PhIsNullOrEmptyString(processNode->ProcessItem->JobName))
    //{
    //    if (PhWordMatchStringZ(SearchText, &processNode->ProcessItem->JobName->sr))
    //        return Matched;
    //}

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->VerifySignerName))
    {
        if (PhWordMatchStringRef(&SearchText->sr, &processNode->ProcessItem->VerifySignerName->sr))
            return FILTER_RESULT_FOUND;
    }

    if (PH_IS_REAL_PROCESS_ID(processNode->ProcessItem->ProcessId))
    {
        if (processNode->ProcessItem->ProcessIdString[0])
        {
            if (PhWordMatchStringLongHintZ(SearchText, processNode->ProcessItem->ProcessIdString))
                return FILTER_RESULT_FOUND_NAME;
        }

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

                if (PhWordMatchStringRef(&SearchText->sr, &processIdHex))
                    return FILTER_RESULT_FOUND_NAME;
            }
        }
    }

    //if (processNode->ProcessItem->ParentProcessIdString[0])
    //{
    //    if (PhWordMatchStringLongHintZ(SearchText, processNode->ProcessItem->ParentProcessIdString))
    //        return Matched;
    //}

    //if (processNode->ProcessItem->SessionIdString[0])
    //{
    //    if (PhWordMatchStringLongHintZ(SearchText, processNode->ProcessItem->SessionIdString))
    //        return Matched;
    //}

    if (processNode->ProcessItem->LxssProcessIdString[0])
    {
        if (PhWordMatchStringLongHintZ(SearchText, processNode->ProcessItem->LxssProcessIdString))
            return FILTER_RESULT_FOUND_NAME;
    }

    if (!PhIsNullOrEmptyString(processNode->ProcessItem->PackageFullName))
    {
        if (PhWordMatchStringRef(&SearchText->sr, &processNode->ProcessItem->PackageFullName->sr))
            return FILTER_RESULT_FOUND;
    }

    {
        PPH_STRINGREF value;

        if (value = PhGetProcessPriorityClassString(processNode->ProcessItem->PriorityClass))
        {
            if (PhWordMatchStringRef(&SearchText->sr, value))
            {
                return FILTER_RESULT_FOUND;
            }
        }
    }

    if (processNode->ProcessItem->VerifyResult != VrUnknown)
    {
        switch (processNode->ProcessItem->VerifyResult)
        {
        case VrNoSignature:
            if (PhWordMatchStringZ(SearchText, L"NoSignature"))
                return FILTER_RESULT_FOUND;
            break;
        case VrTrusted:
            if (PhWordMatchStringZ(SearchText, L"Trusted"))
                return FILTER_RESULT_FOUND;
            break;
        case VrExpired:
            if (PhWordMatchStringZ(SearchText, L"Expired"))
                return FILTER_RESULT_FOUND;
            break;
        case VrRevoked:
            if (PhWordMatchStringZ(SearchText, L"Revoked"))
                return FILTER_RESULT_FOUND;
            break;
        case VrDistrust:
            if (PhWordMatchStringZ(SearchText, L"Distrust"))
                return FILTER_RESULT_FOUND;
            break;
        case VrSecuritySettings:
            if (PhWordMatchStringZ(SearchText, L"SecuritySettings"))
                return FILTER_RESULT_FOUND;
            break;
        case VrBadSignature:
            if (PhWordMatchStringZ(SearchText, L"BadSignature"))
                return FILTER_RESULT_FOUND;
            break;
        default:
            if (PhWordMatchStringZ(SearchText, L"Unknown"))
                return FILTER_RESULT_FOUND;
            break;
        }
    }

    if (processNode->ProcessItem->ElevationType != TokenElevationTypeDefault)
    {
        switch (processNode->ProcessItem->ElevationType)
        {
        case TokenElevationTypeLimited:
            if (PhWordMatchStringZ(SearchText, L"Limited"))
                return FILTER_RESULT_FOUND;
            break;
        case TokenElevationTypeFull:
            if (PhWordMatchStringZ(SearchText, L"Full"))
                return FILTER_RESULT_FOUND;
            break;
        default:
            if (PhWordMatchStringZ(SearchText, L"Unknown"))
                return FILTER_RESULT_FOUND;
            break;
        }
    }

    if (processNode->ProcessItem->IsBeingDebugged && PhWordMatchStringZ(SearchText, L"IsBeingDebugged"))
    {
        return FILTER_RESULT_FOUND;
    }

    if (processNode->ProcessItem->IsDotNet && PhWordMatchStringZ(SearchText, L"IsDotNet"))
    {
        return FILTER_RESULT_FOUND;
    }

    if (processNode->ProcessItem->IsElevated && PhWordMatchStringZ(SearchText, L"IsElevated"))
    {
        return FILTER_RESULT_FOUND;
    }

    if (processNode->ProcessItem->IsInJob && PhWordMatchStringZ(SearchText, L"IsInJob"))
    {
        return FILTER_RESULT_FOUND;
    }

    if (processNode->ProcessItem->IsInSignificantJob && PhWordMatchStringZ(SearchText, L"IsInSignificantJob"))
    {
        return FILTER_RESULT_FOUND;
    }

    if (processNode->ProcessItem->IsPacked && PhWordMatchStringZ(SearchText, L"IsPacked"))
    {
        return FILTER_RESULT_FOUND;
    }

    if (processNode->ProcessItem->IsSuspended && PhWordMatchStringZ(SearchText, L"IsSuspended"))
    {
        return FILTER_RESULT_FOUND;
    }

    if (processNode->ProcessItem->IsWow64 && PhWordMatchStringZ(SearchText, L"IsWow64"))
    {
        return FILTER_RESULT_FOUND;
    }

    if (processNode->ProcessItem->IsImmersive && PhWordMatchStringZ(SearchText, L"IsImmersive"))
    {
        return FILTER_RESULT_FOUND;
    }

    if (processNode->ProcessItem->IsPackagedProcess && PhWordMatchStringZ(SearchText, L"IsPackagedProcess"))
    {
        return FILTER_RESULT_FOUND;
    }

    if (processNode->ProcessItem->IsProtectedProcess && PhWordMatchStringZ(SearchText, L"IsProtectedProcess"))
    {
        return FILTER_RESULT_FOUND;
    }

    if (processNode->ProcessItem->IsSecureProcess && PhWordMatchStringZ(SearchText, L"IsSecureProcess"))
    {
        return FILTER_RESULT_FOUND;
    }

    if (processNode->ProcessItem->IsSubsystemProcess && PhWordMatchStringZ(SearchText, L"IsPicoProcess"))
    {
        return FILTER_RESULT_FOUND;
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
                if (PhWordMatchStringRef(&SearchText->sr, &serviceItem->Name->sr))
                {
                    matched = TRUE;
                    break;
                }
            }

            if (!PhIsNullOrEmptyString(serviceItem->DisplayName))
            {
                if (PhWordMatchStringRef(&SearchText->sr, &serviceItem->DisplayName->sr))
                {
                    matched = TRUE;
                    break;
                }
            }

            if (serviceItem->ProcessId)
            {
                if (PhWordMatchStringLongHintZ(SearchText, serviceItem->ProcessIdString))
                {
                    matched = TRUE;
                    break;
                }
            }

            if (!PhIsNullOrEmptyString(serviceItem->FileName))
            {
                if (PhWordMatchStringRef(&SearchText->sr, &serviceItem->FileName->sr))
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
            return FILTER_RESULT_FOUND;
    }

    return FILTER_RESULT_NOT_FOUND;
}

BOOLEAN ProcessTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    if (EnableChildWildcardSearch)
    {
        if (SearchboxTextCache && (PhIsNullOrEmptyString(SearchboxText) || PhCompareString(SearchboxText, SearchboxTextCache, TRUE)))
        {
            // clear cache (gmit3)
            if (SearchboxTextCache)
            {
                PhDereferenceObject(SearchboxTextCache);
                SearchboxTextCache = NULL;
            }
            if (SearchboxFullTextCache)
            {
                PhDereferenceObject(SearchboxFullTextCache);
                SearchboxFullTextCache = NULL;
            }
            if (SearchboxParentTextCache)
            {
                PhDereferenceObject(SearchboxParentTextCache);
                SearchboxParentTextCache = NULL;
            }
        }

        if (!SearchboxTextCache && !PhIsNullOrEmptyString(SearchboxText))
        {
            static PH_STRINGREF includeChildrenWildcard = PH_STRINGREF_INIT(L"*");

            // process SearchboxText:
            //    fullSearchboxText will contain full search string stripped of '*' markers to perform as before (gmit3)
            //    parentSearchBoxText will contain only search string parts marked with '*' to search through parents (gmit3)

            SearchboxTextCache = PhReferenceObject(SearchboxText);

            if (PhFindStringInStringRef(&SearchboxText->sr, &includeChildrenWildcard, FALSE) != SIZE_MAX)
            {
                PH_STRING_BUILDER fullSearchboxBuilder;
                PH_STRING_BUILDER parentSearchboxBuilder;
                PH_STRINGREF part;
                PH_STRINGREF remainingPart;

                remainingPart = SearchboxText->sr;
                PhInitializeStringBuilder(&fullSearchboxBuilder, 100);
                PhInitializeStringBuilder(&parentSearchboxBuilder, 100);

                while (remainingPart.Length)
                {
                    PhSplitStringRefAtChar(&remainingPart, L'|', &part, &remainingPart);

                    if (part.Length)
                    {
                        BOOLEAN withChildren = PhEndsWithStringRef(&part, &includeChildrenWildcard, FALSE);

                        if (withChildren)
                        {
                            PhTrimStringRef(&part, &includeChildrenWildcard, PH_TRIM_END_ONLY);

                            if (parentSearchboxBuilder.String->Length)
                                PhAppendCharStringBuilder(&parentSearchboxBuilder, L'|');
                            PhAppendStringBuilder(&parentSearchboxBuilder, &part);
                        }

                        if (fullSearchboxBuilder.String->Length)
                            PhAppendCharStringBuilder(&fullSearchboxBuilder, L'|');
                        PhAppendStringBuilder(&fullSearchboxBuilder, &part);
                    }
                }

                PhMoveReference(&SearchboxFullTextCache, PhFinalStringBuilderString(&fullSearchboxBuilder));
                PhMoveReference(&SearchboxParentTextCache, PhFinalStringBuilderString(&parentSearchboxBuilder));
            }
        }

        {
            FILTER_RESULT_TYPE result;

            result = ProcessTreeFilterMatchTypeCallback(
                Node,
                Context,
                !PhIsNullOrEmptyString(SearchboxFullTextCache) ? SearchboxFullTextCache : SearchboxText
                );

            if (result == FILTER_RESULT_NOT_FOUND && !PhIsNullOrEmptyString(SearchboxParentTextCache))
            {
                PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)Node;

                for (PPH_PROCESS_NODE parentNode = processNode->Parent; result == FILTER_RESULT_NOT_FOUND && parentNode; parentNode = parentNode->Parent)
                {
                    if (ProcessTreeFilterMatchTypeCallback(&parentNode->Node, Context, SearchboxParentTextCache) == FILTER_RESULT_FOUND_NAME)
                    {
                        result = FILTER_RESULT_FOUND_NAME;
                        break;
                    }
                }
            }

            return result != FILTER_RESULT_NOT_FOUND;
        }
    }
    else
    {
        FILTER_RESULT_TYPE result;

        result = ProcessTreeFilterMatchTypeCallback(
            Node,
            Context,
            SearchboxText
            );

        return result != FILTER_RESULT_NOT_FOUND;
    }
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

        if (PhWordMatchStringLongHintZ(SearchboxText, serviceNode->ServiceItem->ProcessIdString))
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

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->LocalAddressString))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &networkNode->NetworkItem->LocalAddressString->sr))
            return TRUE;
    }

    if (networkNode->NetworkItem->LocalPortString[0])
    {
        if (PhWordMatchStringLongHintZ(SearchboxText, networkNode->NetworkItem->LocalPortString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->LocalHostString))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &networkNode->NetworkItem->LocalHostString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->RemoteAddressString))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &networkNode->NetworkItem->RemoteAddressString->sr))
            return TRUE;
    }

    if (networkNode->NetworkItem->RemotePortString[0])
    {
        if (PhWordMatchStringLongHintZ(SearchboxText, networkNode->NetworkItem->RemotePortString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(networkNode->NetworkItem->RemoteHostString))
    {
        if (PhWordMatchStringRef(&SearchboxText->sr, &networkNode->NetworkItem->RemoteHostString->sr))
            return TRUE;
    }

    {
        PPH_STRINGREF protocolType;

        if (protocolType = PhGetProtocolTypeName(networkNode->NetworkItem->ProtocolType))
        {
            if (PhWordMatchStringRef(&SearchboxText->sr, protocolType))
                return TRUE;
        }
    }

    {
        if (FlagOn(networkNode->NetworkItem->ProtocolType, PH_TCP_PROTOCOL_TYPE))
        {
            PPH_STRINGREF stateName;

            if (stateName = PhGetTcpStateName(networkNode->NetworkItem->State))
            {
                if (PhWordMatchStringRef(&SearchboxText->sr, stateName))
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
