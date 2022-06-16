/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2017-2021
 *
 */

#include <phapp.h>
#include <srvlist.h>

#include <cpysave.h>
#include <emenu.h>
#include <svcsup.h>
#include <settings.h>
#include <verify.h>

#include <colmgr.h>
#include <extmgri.h>
#include <mainwnd.h>
#include <phplug.h>
#include <phsettings.h>
#include <procprv.h>
#include <srvprv.h>

BOOLEAN PhpServiceNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG PhpServiceNodeHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID PhpRemoveServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode,
    _In_opt_ PVOID Context
    );

LONG PhpServiceTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    );

BOOLEAN NTAPI PhpServiceTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

static HWND ServiceTreeListHandle;
static ULONG ServiceTreeListSortColumn;
static PH_SORT_ORDER ServiceTreeListSortOrder;
static PH_CM_MANAGER ServiceTreeListCm;

static PPH_HASHTABLE ServiceNodeHashtable; // hashtable of all nodes
static PPH_LIST ServiceNodeList; // list of all nodes

static PH_TN_FILTER_SUPPORT FilterSupport;

BOOLEAN PhServiceTreeListStateHighlighting = TRUE;
static PPH_POINTER_LIST ServiceNodeStateList = NULL; // list of nodes which need to be processed

VOID PhServiceTreeListInitialization(
    VOID
    )
{
    ServiceNodeHashtable = PhCreateHashtable(
        sizeof(PPH_SERVICE_NODE),
        PhpServiceNodeHashtableEqualFunction,
        PhpServiceNodeHashtableHashFunction,
        100
        );
    ServiceNodeList = PhCreateList(100);
}

BOOLEAN PhpServiceNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_SERVICE_NODE serviceNode1 = *(PPH_SERVICE_NODE *)Entry1;
    PPH_SERVICE_NODE serviceNode2 = *(PPH_SERVICE_NODE *)Entry2;

    return serviceNode1->ServiceItem == serviceNode2->ServiceItem;
}

ULONG PhpServiceNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashIntPtr((ULONG_PTR)(*(PPH_SERVICE_NODE *)Entry)->ServiceItem);
}

VOID PhInitializeServiceTreeList(
    _In_ HWND hwnd
    )
{
    ServiceTreeListHandle = hwnd;
    PhSetControlTheme(ServiceTreeListHandle, L"explorer");
    SendMessage(TreeNew_GetTooltips(ServiceTreeListHandle), TTM_SETDELAYTIME, TTDT_AUTOPOP, MAXSHORT);

    TreeNew_SetCallback(hwnd, PhpServiceTreeNewCallback, NULL);
    TreeNew_SetImageList(hwnd, PhProcessSmallImageList);

    TreeNew_SetRedraw(hwnd, FALSE);

    // Default columns
    PhAddTreeNewColumn(hwnd, PHSVTLC_NAME, TRUE, L"Name", 140, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_DISPLAYNAME, TRUE, L"Display name", 220, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_TYPE, TRUE, L"Type", 100, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_STATUS, TRUE, L"Status", 70, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_STARTTYPE, TRUE, L"Start type", 130, PH_ALIGN_LEFT, 4, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_PID, TRUE, L"PID", 50, PH_ALIGN_RIGHT, 5, DT_RIGHT);

    PhAddTreeNewColumn(hwnd, PHSVTLC_BINARYPATH, FALSE, L"Binary path", 180, PH_ALIGN_LEFT, ULONG_MAX, DT_PATH_ELLIPSIS);
    PhAddTreeNewColumn(hwnd, PHSVTLC_ERRORCONTROL, FALSE, L"Error control", 70, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_GROUP, FALSE, L"Group", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_DESCRIPTION, FALSE, L"Description", 200, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumnEx(hwnd, PHSVTLC_KEYMODIFIEDTIME, FALSE, L"Key modified time", 140, PH_ALIGN_LEFT, ULONG_MAX, 0, TRUE);
    PhAddTreeNewColumn(hwnd, PHSVTLC_VERIFICATIONSTATUS, FALSE, L"Verification status", 70, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_VERIFIEDSIGNER, FALSE, L"Verified signer", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_FILENAME, FALSE, L"File name", 100, PH_ALIGN_LEFT, ULONG_MAX, DT_PATH_ELLIPSIS);
    PhAddTreeNewColumnEx2(hwnd, PHSVTLC_TIMELINE, FALSE, L"Timeline", 100, PH_ALIGN_LEFT, ULONG_MAX, 0, TN_COLUMN_FLAG_CUSTOMDRAW | TN_COLUMN_FLAG_SORTDESCENDING);

    TreeNew_SetRedraw(hwnd, TRUE);

    TreeNew_SetTriState(hwnd, TRUE);
    TreeNew_SetSort(hwnd, 0, AscendingSortOrder);

    PhCmInitializeManager(&ServiceTreeListCm, hwnd, PHSVTLC_MAXIMUM, PhpServiceTreeNewPostSortFunction);

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

        treeNewInfo.TreeNewHandle = hwnd;
        treeNewInfo.CmData = &ServiceTreeListCm;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackServiceTreeNewInitializing), &treeNewInfo);
    }

    PhInitializeTreeNewFilterSupport(&FilterSupport, hwnd, ServiceNodeList);
}

VOID PhLoadSettingsServiceTreeList(
    VOID
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhGetStringSetting(L"ServiceTreeListColumns");
    sortSettings = PhGetStringSetting(L"ServiceTreeListSort");
    PhCmLoadSettingsEx(ServiceTreeListHandle, &ServiceTreeListCm, 0, &settings->sr, &sortSettings->sr);
    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSaveSettingsServiceTreeList(
    VOID
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(ServiceTreeListHandle, &ServiceTreeListCm, 0, &sortSettings);
    PhSetStringSetting2(L"ServiceTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"ServiceTreeListSort", &sortSettings->sr);
    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

struct _PH_TN_FILTER_SUPPORT *PhGetFilterSupportServiceTreeList(
    VOID
    )
{
    return &FilterSupport;
}

PPH_SERVICE_NODE PhAddServiceNode(
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ ULONG RunId
    )
{
    PPH_SERVICE_NODE serviceNode;

    serviceNode = PhAllocate(PhEmGetObjectSize(EmServiceNodeType, sizeof(PH_SERVICE_NODE)));
    memset(serviceNode, 0, sizeof(PH_SERVICE_NODE));
    PhInitializeTreeNewNode(&serviceNode->Node);

    if (PhServiceTreeListStateHighlighting && RunId != 1)
    {
        PhChangeShStateTn(
            &serviceNode->Node,
            &serviceNode->ShState,
            &ServiceNodeStateList,
            NewItemState,
            PhCsColorNew,
            NULL
            );
    }

    serviceNode->ServiceItem = ServiceItem;
    PhReferenceObject(ServiceItem);

    memset(serviceNode->TextCache, 0, sizeof(PH_STRINGREF) * PHSVTLC_MAXIMUM);
    serviceNode->Node.TextCache = serviceNode->TextCache;
    serviceNode->Node.TextCacheSize = PHSVTLC_MAXIMUM;

    PhAddEntryHashtable(ServiceNodeHashtable, &serviceNode);
    PhAddItemList(ServiceNodeList, serviceNode);

    if (FilterSupport.FilterList)
        serviceNode->Node.Visible = PhApplyTreeNewFiltersToNode(&FilterSupport, &serviceNode->Node);

    PhEmCallObjectOperation(EmServiceNodeType, serviceNode, EmObjectCreate);

    TreeNew_NodesStructured(ServiceTreeListHandle);

    return serviceNode;
}

PPH_SERVICE_NODE PhFindServiceNode(
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    PH_SERVICE_NODE lookupServiceNode;
    PPH_SERVICE_NODE lookupServiceNodePtr = &lookupServiceNode;
    PPH_SERVICE_NODE *serviceNode;

    lookupServiceNode.ServiceItem = ServiceItem;

    serviceNode = (PPH_SERVICE_NODE *)PhFindEntryHashtable(
        ServiceNodeHashtable,
        &lookupServiceNodePtr
        );

    if (serviceNode)
        return *serviceNode;
    else
        return NULL;
}

VOID PhRemoveServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    )
{
    // Remove from the hashtable here to avoid problems in case the key is re-used.
    PhRemoveEntryHashtable(ServiceNodeHashtable, &ServiceNode);

    if (PhServiceTreeListStateHighlighting)
    {
        PhChangeShStateTn(
            &ServiceNode->Node,
            &ServiceNode->ShState,
            &ServiceNodeStateList,
            RemovingItemState,
            PhCsColorRemoved,
            ServiceTreeListHandle
            );
    }
    else
    {
        PhpRemoveServiceNode(ServiceNode, NULL);
    }
}

VOID PhpRemoveServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode,
    _In_opt_ PVOID Context
    )
{
    ULONG index;

    PhEmCallObjectOperation(EmServiceNodeType, ServiceNode, EmObjectDelete);

    // Remove from list and cleanup.

    if ((index = PhFindItemList(ServiceNodeList, ServiceNode)) != ULONG_MAX)
        PhRemoveItemList(ServiceNodeList, index);

    PhClearReference(&ServiceNode->BinaryPath);
    PhClearReference(&ServiceNode->LoadOrderGroup);
    PhClearReference(&ServiceNode->Description);
    PhClearReference(&ServiceNode->TooltipText);
    PhClearReference(&ServiceNode->KeyModifiedTimeText);

    PhDereferenceObject(ServiceNode->ServiceItem);

    PhFree(ServiceNode);

    TreeNew_NodesStructured(ServiceTreeListHandle);
}

VOID PhUpdateServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    )
{
    memset(ServiceNode->TextCache, 0, sizeof(PH_STRINGREF) * PHSVTLC_MAXIMUM);
    PhClearReference(&ServiceNode->TooltipText);

    ServiceNode->ValidMask = 0;
    PhInvalidateTreeNewNode(&ServiceNode->Node, TN_CACHE_ICON);
    TreeNew_NodesStructured(ServiceTreeListHandle);
}

VOID PhTickServiceNodes(
    VOID
    )
{
    if (ServiceTreeListSortOrder != NoSortOrder && ServiceTreeListSortColumn >= PHSVTLC_MAXIMUM)
    {
        // Sorting is on, but it's not one of our columns. Force a rebuild. (If it was one of our
        // columns, the restructure would have been handled in PhUpdateServiceNode.)
        TreeNew_NodesStructured(ServiceTreeListHandle);
    }

    PH_TICK_SH_STATE_TN(PH_SERVICE_NODE, ShState, ServiceNodeStateList, PhpRemoveServiceNode, PhCsHighlightingDuration, ServiceTreeListHandle, TRUE, NULL, NULL);
}

static VOID PhpUpdateServiceNodeConfig(
    _Inout_ PPH_SERVICE_NODE ServiceNode
    )
{
    if (!(ServiceNode->ValidMask & PHSN_CONFIG))
    {
        SC_HANDLE serviceHandle;
        LPQUERY_SERVICE_CONFIG serviceConfig;

        if (serviceHandle = PhOpenService(ServiceNode->ServiceItem->Name->Buffer, SERVICE_QUERY_CONFIG))
        {
            if (serviceConfig = PhGetServiceConfig(serviceHandle))
            {
                if (serviceConfig->lpBinaryPathName)
                    PhMoveReference(&ServiceNode->BinaryPath, PhCreateString(serviceConfig->lpBinaryPathName));
                if (serviceConfig->lpLoadOrderGroup)
                    PhMoveReference(&ServiceNode->LoadOrderGroup, PhCreateString(serviceConfig->lpLoadOrderGroup));

                PhFree(serviceConfig);
            }

            CloseServiceHandle(serviceHandle);
        }

        ServiceNode->ValidMask |= PHSN_CONFIG;
    }
}

static VOID PhpUpdateServiceNodeDescription(
    _Inout_ PPH_SERVICE_NODE ServiceNode
    )
{
    if (!(ServiceNode->ValidMask & PHSN_DESCRIPTION))
    {
        static PH_STRINGREF servicesKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services\\");
        HANDLE keyHandle;
        PPH_STRING keyName;

        keyName = PhConcatStringRef2(&servicesKeyName, &ServiceNode->ServiceItem->Name->sr);

        if (NT_SUCCESS(PhOpenKey(
            &keyHandle,
            KEY_QUERY_VALUE,
            PH_KEY_LOCAL_MACHINE,
            &keyName->sr,
            0
            )))
        {
            PPH_STRING descriptionString;
            PPH_STRING serviceDescriptionString;

            if (descriptionString = PhQueryRegistryString(keyHandle, L"Description"))
            {
                if (serviceDescriptionString = PhLoadIndirectString(descriptionString->Buffer))
                    PhMoveReference(&ServiceNode->Description, serviceDescriptionString);
                else
                    PhSwapReference(&ServiceNode->Description, descriptionString);

                PhDereferenceObject(descriptionString);
            }

            NtClose(keyHandle);
        }

        PhDereferenceObject(keyName);

        ServiceNode->ValidMask |= PHSN_DESCRIPTION;
    }

    // NOTE: Querying the service description via RPC is extremely slow. (dmex)
    //SC_HANDLE serviceHandle;
    //
    //if (serviceHandle = PhOpenService(ServiceNode->ServiceItem->Name->Buffer, SERVICE_QUERY_CONFIG))
    //{
    //    PhMoveReference(&ServiceNode->Description, PhGetServiceDescription(serviceHandle));
    //    CloseServiceHandle(serviceHandle);
    //}
}

static VOID PhpUpdateServiceNodeKey(
    _Inout_ PPH_SERVICE_NODE ServiceNode
    )
{
    if (!(ServiceNode->ValidMask & PHSN_KEY))
    {
        static PH_STRINGREF servicesKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services\\");
        HANDLE keyHandle;
        PPH_STRING keyName;

        keyName = PhConcatStringRef2(&servicesKeyName, &ServiceNode->ServiceItem->Name->sr);

        if (NT_SUCCESS(PhOpenKey(
            &keyHandle,
            KEY_QUERY_VALUE,
            PH_KEY_LOCAL_MACHINE,
            &keyName->sr,
            0
            )))
        {
            NTSTATUS status;
            KEY_BASIC_INFORMATION basicInfo = { 0 };
            ULONG bufferLength = 0;

            status = NtQueryKey(
                keyHandle,
                KeyBasicInformation,
                &basicInfo,
                UFIELD_OFFSET(KEY_BASIC_INFORMATION, Name),
                &bufferLength
                );

            // We can query the write time without allocating the key name. (dmex)
            if (status == STATUS_SUCCESS || status == STATUS_BUFFER_OVERFLOW)
            {
                ServiceNode->KeyLastWriteTime = basicInfo.LastWriteTime;
            }
            else
            {
                PKEY_BASIC_INFORMATION buffer;

                if (NT_SUCCESS(PhQueryKey(keyHandle, KeyBasicInformation, &buffer)))
                {
                    ServiceNode->KeyLastWriteTime = buffer->LastWriteTime;
                    PhFree(buffer);
                }
            }

            NtClose(keyHandle);
        }

        PhDereferenceObject(keyName);

        ServiceNode->ValidMask |= PHSN_KEY;
    }
}

#define SORT_FUNCTION(Column) PhpServiceTreeNewCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpServiceTreeNewCompare##Column( \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_SERVICE_NODE node1 = *(PPH_SERVICE_NODE *)_elem1; \
    PPH_SERVICE_NODE node2 = *(PPH_SERVICE_NODE *)_elem2; \
    PPH_SERVICE_ITEM serviceItem1 = node1->ServiceItem; \
    PPH_SERVICE_ITEM serviceItem2 = node2->ServiceItem; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = PhCompareString(serviceItem1->Name, serviceItem2->Name, TRUE); \
    \
    return PhModifySort(sortResult, ServiceTreeListSortOrder); \
}

LONG PhpServiceTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    PPH_SERVICE_NODE node1 = (PPH_SERVICE_NODE)Node1;
    PPH_SERVICE_NODE node2 = (PPH_SERVICE_NODE)Node2;
    PPH_SERVICE_ITEM serviceItem1 = node1->ServiceItem;
    PPH_SERVICE_ITEM serviceItem2 = node2->ServiceItem;

    if (Result == 0)
        Result = PhCompareString(serviceItem1->Name, serviceItem2->Name, TRUE);

    return PhModifySort(Result, SortOrder);
}

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareString(serviceItem1->Name, serviceItem2->Name, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(DisplayName)
{
    sortResult = PhCompareStringWithNull(serviceItem1->DisplayName, serviceItem2->DisplayName, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Type)
{
    sortResult = uintcmp(serviceItem1->Type, serviceItem2->Type);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Status)
{
    sortResult = uintcmp(serviceItem1->State, serviceItem2->State);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(StartType)
{
    sortResult = uintcmp(serviceItem1->StartType, serviceItem2->StartType);

    if (sortResult == 0)
        sortResult = ucharcmp(serviceItem1->DelayedStart, serviceItem2->DelayedStart);
    if (sortResult == 0)
        sortResult = ucharcmp(serviceItem1->HasTriggers, serviceItem2->HasTriggers);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Pid)
{
    sortResult = uintptrcmp((ULONG_PTR)serviceItem1->ProcessId, (ULONG_PTR)serviceItem2->ProcessId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(BinaryPath)
{
    PhpUpdateServiceNodeConfig(node1);
    PhpUpdateServiceNodeConfig(node2);
    sortResult = PhCompareStringWithNullSortOrder(node1->BinaryPath, node2->BinaryPath, ServiceTreeListSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ErrorControl)
{
    sortResult = uintcmp(serviceItem1->ErrorControl, serviceItem2->ErrorControl);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Group)
{
    PhpUpdateServiceNodeConfig(node1);
    PhpUpdateServiceNodeConfig(node2);
    sortResult = PhCompareStringWithNullSortOrder(node1->LoadOrderGroup, node2->LoadOrderGroup, ServiceTreeListSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Description)
{
    PhpUpdateServiceNodeDescription(node1);
    PhpUpdateServiceNodeDescription(node2);
    sortResult = PhCompareStringWithNullSortOrder(node1->Description, node2->Description, ServiceTreeListSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(KeyModifiedTime)
{
    PhpUpdateServiceNodeKey(node1);
    PhpUpdateServiceNodeKey(node2);
    sortResult = int64cmp(node1->KeyLastWriteTime.QuadPart, node2->KeyLastWriteTime.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(VerificationStatus)
{
    sortResult = uintcmp(serviceItem1->VerifyResult, serviceItem2->VerifyResult);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(VerifiedSigner)
{
    sortResult = PhCompareStringWithNullSortOrder(
        serviceItem1->VerifySignerName,
        serviceItem2->VerifySignerName,
        ServiceTreeListSortOrder,
        TRUE
        );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(FileName)
{
    sortResult = PhCompareStringWithNullSortOrder(
        serviceItem1->FileName,
        serviceItem2->FileName,
        ServiceTreeListSortOrder,
        TRUE
        );
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpServiceTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PPH_SERVICE_NODE node;

    if (PhCmForwardMessage(hwnd, Message, Parameter1, Parameter2, &ServiceTreeListCm))
        return TRUE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren)
                break;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(DisplayName),
                    SORT_FUNCTION(Type),
                    SORT_FUNCTION(Status),
                    SORT_FUNCTION(StartType),
                    SORT_FUNCTION(Pid),
                    SORT_FUNCTION(BinaryPath),
                    SORT_FUNCTION(ErrorControl),
                    SORT_FUNCTION(Group),
                    SORT_FUNCTION(Description),
                    SORT_FUNCTION(KeyModifiedTime),
                    SORT_FUNCTION(VerificationStatus),
                    SORT_FUNCTION(VerifiedSigner),
                    SORT_FUNCTION(FileName),
                    SORT_FUNCTION(KeyModifiedTime), // Timeline
                };
                int (__cdecl *sortFunction)(const void *, const void *);

                if (!PhCmForwardSort(
                    (PPH_TREENEW_NODE *)ServiceNodeList->Items,
                    ServiceNodeList->Count,
                    ServiceTreeListSortColumn,
                    ServiceTreeListSortOrder,
                    &ServiceTreeListCm
                    ))
                {
                    if (ServiceTreeListSortColumn < PHSVTLC_MAXIMUM)
                        sortFunction = sortFunctions[ServiceTreeListSortColumn];
                    else
                        sortFunction = NULL;

                    if (sortFunction)
                    {
                        qsort(ServiceNodeList->Items, ServiceNodeList->Count, sizeof(PVOID), sortFunction);
                    }
                }

                getChildren->Children = (PPH_TREENEW_NODE *)ServiceNodeList->Items;
                getChildren->NumberOfChildren = ServiceNodeList->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;

            if (!isLeaf)
                break;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            PPH_SERVICE_ITEM serviceItem;

            if (!getCellText)
                break;

            node = (PPH_SERVICE_NODE)getCellText->Node;
            serviceItem = node->ServiceItem;

            switch (getCellText->Id)
            {
            case PHSVTLC_NAME:
                getCellText->Text = PhGetStringRef(serviceItem->Name);
                break;
            case PHSVTLC_DISPLAYNAME:
                getCellText->Text = PhGetStringRef(serviceItem->DisplayName);
                break;
            case PHSVTLC_TYPE:
                {
                    PPH_STRINGREF string;

                    string = PhGetServiceTypeString(serviceItem->Type);
                    getCellText->Text.Buffer = string->Buffer;
                    getCellText->Text.Length = string->Length;
                }
                break;
            case PHSVTLC_STATUS:
                {
                    PPH_STRINGREF string;

                    string = PhGetServiceStateString(serviceItem->State);
                    getCellText->Text.Buffer = string->Buffer;
                    getCellText->Text.Length = string->Length;
                }
                break;
            case PHSVTLC_STARTTYPE:
                {
                    PH_FORMAT format[2];
                    PPH_STRINGREF string;
                    PWSTR additional = NULL;
                    SIZE_T returnLength;

                    string = PhGetServiceStartTypeString(serviceItem->StartType);
                    format[0].Type = StringFormatType;
                    format[0].u.String.Buffer = string->Buffer;
                    format[0].u.String.Length = string->Length;
                    //PhInitFormatSR(&format[0], PhGetServiceStartTypeString(serviceItem->StartType));

                    if (serviceItem->StartType == SERVICE_DISABLED)
                        additional = NULL;
                    else if (serviceItem->DelayedStart && serviceItem->HasTriggers)
                        additional = L" (delayed, trigger)";
                    else if (serviceItem->DelayedStart)
                        additional = L" (delayed)";
                    else if (serviceItem->HasTriggers)
                        additional = L" (trigger)";

                    if (additional)
                        PhInitFormatS(&format[1], additional);

                    if (PhFormatToBuffer(format, 1 + (additional ? 1 : 0), node->StartTypeText,
                        sizeof(node->StartTypeText), &returnLength))
                    {
                        getCellText->Text.Buffer = node->StartTypeText;
                        getCellText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                    }
                }
                break;
            case PHSVTLC_PID:
                {
                    if (serviceItem->ProcessId)
                        PhPrintUInt32(serviceItem->ProcessIdString, HandleToUlong(serviceItem->ProcessId));
                    else
                        serviceItem->ProcessIdString[0] = UNICODE_NULL;

                    PhInitializeStringRefLongHint(&getCellText->Text, serviceItem->ProcessIdString);
                }
                break;
            case PHSVTLC_BINARYPATH:
                PhpUpdateServiceNodeConfig(node);
                getCellText->Text = PhGetStringRef(node->BinaryPath);
                break;
            case PHSVTLC_ERRORCONTROL:
                {
                    PPH_STRINGREF string;

                    string = PhGetServiceErrorControlString(serviceItem->ErrorControl);
                    getCellText->Text.Buffer = string->Buffer;
                    getCellText->Text.Length = string->Length;
                }
                break;
            case PHSVTLC_GROUP:
                PhpUpdateServiceNodeConfig(node);
                getCellText->Text = PhGetStringRef(node->LoadOrderGroup);
                break;
            case PHSVTLC_DESCRIPTION:
                PhpUpdateServiceNodeDescription(node);
                getCellText->Text = PhGetStringRef(node->Description);
                break;
            case PHSVTLC_KEYMODIFIEDTIME:
                PhpUpdateServiceNodeKey(node);

                if (node->KeyLastWriteTime.QuadPart != 0)
                {
                    SYSTEMTIME systemTime;

                    PhLargeIntegerToLocalSystemTime(&systemTime, &node->KeyLastWriteTime);
                    PhMoveReference(&node->KeyModifiedTimeText, PhFormatDateTime(&systemTime));
                    getCellText->Text = node->KeyModifiedTimeText->sr;
                }
                break;
            case PHSVTLC_VERIFICATIONSTATUS:
                PhInitializeStringRef(&getCellText->Text,
                    serviceItem->VerifyResult == VrTrusted ? L"Trusted" : L"Not trusted");
                break;
            case PHSVTLC_VERIFIEDSIGNER:
                getCellText->Text = PhGetStringRef(serviceItem->VerifySignerName);
                break;
            case PHSVTLC_FILENAME:
                getCellText->Text = PhGetStringRef(serviceItem->FileName);
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeIcon:
        {
            PPH_TREENEW_GET_NODE_ICON getNodeIcon = Parameter1;

            if (!getNodeIcon)
                break;

            node = (PPH_SERVICE_NODE)getNodeIcon->Node;

            if (node->ServiceItem->IconEntry)
            {
                getNodeIcon->Icon = (HICON)(ULONG_PTR)node->ServiceItem->IconEntry->SmallIconIndex;
            }
            else
            {
                if (node->ServiceItem->Type == SERVICE_KERNEL_DRIVER || node->ServiceItem->Type == SERVICE_FILE_SYSTEM_DRIVER)
                    getNodeIcon->Icon = (HICON)(ULONG_PTR)1;// ServiceCogIcon;
                else
                    getNodeIcon->Icon = (HICON)(ULONG_PTR)0;//ServiceApplicationIcon;
            }

            getNodeIcon->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;
            PPH_SERVICE_ITEM serviceItem;

            if (!getNodeColor)
                break;

            node = (PPH_SERVICE_NODE)getNodeColor->Node;
            serviceItem = node->ServiceItem;

            if (!serviceItem)
                ; // Dummy
            else if (PhEnableServiceQueryStage2 && PhCsUseColorUnknown && serviceItem->VerifyResult != VrTrusted)
            {
                getNodeColor->BackColor = PhCsColorUnknown;
                getNodeColor->Flags |= TN_AUTO_FORECOLOR;
            }
            else if (PhCsUseColorServiceDisabled && serviceItem->State == SERVICE_STOPPED && serviceItem->StartType == SERVICE_DISABLED)
            {
                getNodeColor->BackColor = PhCsColorServiceDisabled;
                getNodeColor->Flags |= TN_AUTO_FORECOLOR;
            }
            else if (PhCsUseColorServiceStop && serviceItem->State == SERVICE_STOPPED)
            {
                getNodeColor->ForeColor = PhCsColorServiceStop;
            }
            else
            {
                getNodeColor->Flags |= TN_AUTO_FORECOLOR;
            }
        }
        return TRUE;
    case TreeNewGetCellTooltip:
        {
            PPH_TREENEW_GET_CELL_TOOLTIP getCellTooltip = Parameter1;

            if (!getCellTooltip)
                break;

            node = (PPH_SERVICE_NODE)getCellTooltip->Node;

            if (getCellTooltip->Column->Id != 0)
                return FALSE;

            if (!node->TooltipText)
                node->TooltipText = PhGetServiceTooltipText(node->ServiceItem);

            if (!PhIsNullOrEmptyString(node->TooltipText))
            {
                getCellTooltip->Text = node->TooltipText->sr;
                getCellTooltip->Unfolding = FALSE;
                getCellTooltip->MaximumWidth = 550;
            }
            else
            {
                return FALSE;
            }
        }
        return TRUE;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = Parameter1;
            PPH_SERVICE_ITEM serviceItem;
            RECT rect;

            if (!customDraw)
                break;

            node = (PPH_SERVICE_NODE)customDraw->Node;
            serviceItem = node->ServiceItem;
            rect = customDraw->CellRect;

            if (rect.right - rect.left <= 1)
                break; // nothing to draw

            switch (customDraw->Column->Id)
            {
            case PHSVTLC_TIMELINE:
                {
                    PhpUpdateServiceNodeKey(node);

                    if (node->KeyLastWriteTime.QuadPart == 0)
                        break; // nothing to draw

                    PhCustomDrawTreeTimeLine(
                        customDraw->Dc,
                        customDraw->CellRect,
                        PhEnableThemeSupport ? PH_DRAW_TIMELINE_DARKTHEME : 0,
                        NULL,
                        &node->KeyLastWriteTime
                        );
                }
                break;
            }
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &ServiceTreeListSortColumn, &ServiceTreeListSortOrder);
            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            if (!keyEvent)
                break;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(PhMainWndHandle, WM_COMMAND, ID_SERVICE_COPY, 0);
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(ServiceTreeListHandle, 0, -1);
                break;
            case VK_DELETE:
                SendMessage(PhMainWndHandle, WM_COMMAND, ID_SERVICE_DELETE, 0);
                break;
            case VK_RETURN:
                if (GetKeyState(VK_CONTROL) >= 0)
                    SendMessage(PhMainWndHandle, WM_COMMAND, ID_SERVICE_PROPERTIES, 0);
                else
                    SendMessage(PhMainWndHandle, WM_COMMAND, ID_SERVICE_OPENFILELOCATION, 0);
                break;
            }
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = AscendingSortOrder;
            PhInitializeTreeNewColumnMenu(&data);

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            SendMessage(PhMainWndHandle, WM_COMMAND, ID_SERVICE_PROPERTIES, 0);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenu = Parameter1;

            if (!contextMenu)
                break;

            PhShowServiceContextMenu(contextMenu);
        }
        return TRUE;
    }

    return FALSE;
}

PPH_SERVICE_ITEM PhGetSelectedServiceItem(
    VOID
    )
{
    PPH_SERVICE_ITEM serviceItem = NULL;
    ULONG i;

    for (i = 0; i < ServiceNodeList->Count; i++)
    {
        PPH_SERVICE_NODE node = ServiceNodeList->Items[i];

        if (node->Node.Selected)
        {
            serviceItem = node->ServiceItem;
            break;
        }
    }

    return serviceItem;
}

VOID PhGetSelectedServiceItems(
    _Out_ PPH_SERVICE_ITEM **Services,
    _Out_ PULONG NumberOfServices
    )
{
    PH_ARRAY array;
    ULONG i;

    PhInitializeArray(&array, sizeof(PVOID), 2);

    for (i = 0; i < ServiceNodeList->Count; i++)
    {
        PPH_SERVICE_NODE node = ServiceNodeList->Items[i];

        if (node->Node.Selected)
            PhAddItemArray(&array, &node->ServiceItem);
    }

    *NumberOfServices = (ULONG)array.Count;
    *Services = PhFinalArrayItems(&array);
}

VOID PhDeselectAllServiceNodes(
    VOID
    )
{
    TreeNew_DeselectRange(ServiceTreeListHandle, 0, -1);
}

VOID PhSelectAndEnsureVisibleServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    )
{
    PhDeselectAllServiceNodes();

    if (!ServiceNode->Node.Visible)
        return;

    TreeNew_SetFocusNode(ServiceTreeListHandle, &ServiceNode->Node);
    TreeNew_SetMarkNode(ServiceTreeListHandle, &ServiceNode->Node);
    TreeNew_SelectRange(ServiceTreeListHandle, ServiceNode->Node.Index, ServiceNode->Node.Index);
    TreeNew_EnsureVisible(ServiceTreeListHandle, &ServiceNode->Node);
}

VOID PhCopyServiceList(
    VOID
    )
{
    PPH_STRING text;

    text = PhGetTreeNewText(ServiceTreeListHandle, 0);
    PhSetClipboardString(ServiceTreeListHandle, &text->sr);
    PhDereferenceObject(text);
}

VOID PhWriteServiceList(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ ULONG Mode
    )
{
    PPH_LIST lines;
    ULONG i;

    lines = PhGetGenericTreeNewLines(ServiceTreeListHandle, Mode);

    for (i = 0; i < lines->Count; i++)
    {
        PPH_STRING line;

        line = lines->Items[i];
        PhWriteStringAsUtf8FileStream(FileStream, &line->sr);
        PhDereferenceObject(line);
        PhWriteStringAsUtf8FileStream2(FileStream, L"\r\n");
    }

    PhDereferenceObject(lines);
}
