/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <srvlist.h>

#include <cpysave.h>
#include <emenu.h>
#include <svcsup.h>
#include <settings.h>
#include <verify.h>
#include <mapldr.h>

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

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
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
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
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

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
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

    TreeNew_SetRedraw(hwnd, FALSE);
    PhSetControlTheme(ServiceTreeListHandle, L"explorer");
    TreeNew_SetCallback(hwnd, PhpServiceTreeNewCallback, NULL);
    TreeNew_SetImageList(hwnd, PhProcessSmallImageList);

    // Default columns
    PhAddTreeNewColumn(hwnd, PHSVTLC_NAME, TRUE, L"Name", 140, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_PID, TRUE, L"PID", 50, PH_ALIGN_RIGHT, 1, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHSVTLC_DISPLAYNAME, TRUE, L"Display name", 220, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_TYPE, TRUE, L"Type", 100, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_STATUS, TRUE, L"Status", 70, PH_ALIGN_LEFT, 4, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_STARTTYPE, TRUE, L"Start type", 130, PH_ALIGN_LEFT, 5, 0);
    // Customizable columns
    PhAddTreeNewColumn(hwnd, PHSVTLC_BINARYPATH, FALSE, L"Binary path", 180, PH_ALIGN_LEFT, ULONG_MAX, DT_PATH_ELLIPSIS);
    PhAddTreeNewColumn(hwnd, PHSVTLC_ERRORCONTROL, FALSE, L"Error control", 70, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_GROUP, FALSE, L"Group", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_DESCRIPTION, FALSE, L"Description", 200, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumnEx(hwnd, PHSVTLC_KEYMODIFIEDTIME, FALSE, L"Key modified time", 140, PH_ALIGN_LEFT, ULONG_MAX, 0, TRUE);
    PhAddTreeNewColumn(hwnd, PHSVTLC_VERIFICATIONSTATUS, FALSE, L"Verification status", 70, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_VERIFIEDSIGNER, FALSE, L"Verified signer", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_FILENAME, FALSE, L"File name", 100, PH_ALIGN_LEFT, ULONG_MAX, DT_PATH_ELLIPSIS);
    PhAddTreeNewColumnEx2(hwnd, PHSVTLC_TIMELINE, FALSE, L"Timeline", 100, PH_ALIGN_LEFT, ULONG_MAX, 0, TN_COLUMN_FLAG_CUSTOMDRAW | TN_COLUMN_FLAG_SORTDESCENDING);
    PhAddTreeNewColumn(hwnd, PHSVTLC_EXITCODE, FALSE, L"Exit code", 100, PH_ALIGN_LEFT, ULONG_MAX, 0);

    PhCmInitializeManager(&ServiceTreeListCm, hwnd, PHSVTLC_MAXIMUM, PhpServiceTreeNewPostSortFunction);
    PhInitializeTreeNewFilterSupport(&FilterSupport, hwnd, ServiceNodeList);

    TreeNew_SetTriState(hwnd, TRUE);
    TreeNew_SetRedraw(hwnd, TRUE);

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

        treeNewInfo.TreeNewHandle = hwnd;
        treeNewInfo.CmData = &ServiceTreeListCm;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackServiceTreeNewInitializing), &treeNewInfo);
    }
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

    if (PhGetIntegerSetting(L"EnableInstantTooltips"))
        SendMessage(TreeNew_GetTooltips(ServiceTreeListHandle), TTM_SETDELAYTIME, TTDT_INITIAL, 0);
    else
        SendMessage(TreeNew_GetTooltips(ServiceTreeListHandle), TTM_SETDELAYTIME, TTDT_AUTOPOP, MAXSHORT);
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
    PhClearReference(&ServiceNode->ExitCodeText);

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
    TreeNew_InvalidateNode(ServiceTreeListHandle, &ServiceNode->Node);
}

VOID PhTickServiceNodes(
    VOID
    )
{
    if (ServiceTreeListSortOrder != NoSortOrder)
    {
        // Force a rebuild to sort the items.
        TreeNew_NodesStructured(ServiceTreeListHandle);
    }

    PH_TICK_SH_STATE_TN(PH_SERVICE_NODE, ShState, ServiceNodeStateList, PhpRemoveServiceNode, PhCsHighlightingDuration, ServiceTreeListHandle, TRUE, NULL, NULL);
}

static VOID PhpUpdateServiceNodeConfig(
    _Inout_ PPH_SERVICE_NODE ServiceNode
    )
{
    if (!FlagOn(ServiceNode->ValidMask, PHSN_CONFIG))
    {
        SC_HANDLE serviceHandle;
        LPQUERY_SERVICE_CONFIG serviceConfig;

        if (NT_SUCCESS(PhOpenService(&serviceHandle, SERVICE_QUERY_CONFIG, PhGetString(ServiceNode->ServiceItem->Name))))
        {
            if (NT_SUCCESS(PhGetServiceConfig(serviceHandle, &serviceConfig)))
            {
                if (serviceConfig->lpBinaryPathName)
                    PhMoveReference(&ServiceNode->BinaryPath, PhCreateString(serviceConfig->lpBinaryPathName));
                if (serviceConfig->lpLoadOrderGroup)
                    PhMoveReference(&ServiceNode->LoadOrderGroup, PhCreateString(serviceConfig->lpLoadOrderGroup));

                PhFree(serviceConfig);
            }

            PhCloseServiceHandle(serviceHandle);
        }

        SetFlag(ServiceNode->ValidMask, PHSN_CONFIG);
    }
}

static VOID PhpUpdateServiceNodeDescription(
    _Inout_ PPH_SERVICE_NODE ServiceNode
    )
{
    if (!FlagOn(ServiceNode->ValidMask, PHSN_DESCRIPTION))
    {
        NTSTATUS status;
        HANDLE keyHandle;

        status = PhOpenServiceKey(
            &keyHandle,
            KEY_QUERY_VALUE,
            &ServiceNode->ServiceItem->Name->sr
            );

        if (NT_SUCCESS(status))
        {
            PPH_STRING descriptionString;
            PPH_STRING serviceDescriptionString;

            if (descriptionString = PhQueryRegistryStringZ(keyHandle, L"Description"))
            {
                if (serviceDescriptionString = PhLoadIndirectString(&descriptionString->sr))
                    PhMoveReference(&ServiceNode->Description, serviceDescriptionString);
                else
                    PhSwapReference(&ServiceNode->Description, descriptionString);

                PhDereferenceObject(descriptionString);
            }

            NtClose(keyHandle);
        }
        else
        {
            PhMoveReference(&ServiceNode->Description, PhGetStatusMessage(status, 0));
        }

        SetFlag(ServiceNode->ValidMask, PHSN_DESCRIPTION);
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
    if (!FlagOn(ServiceNode->ValidMask, PHSN_KEY))
    {
        HANDLE keyHandle;

        if (NT_SUCCESS(PhOpenServiceKey(
            &keyHandle,
            KEY_QUERY_VALUE,
            &ServiceNode->ServiceItem->Name->sr
            )))
        {
            LARGE_INTEGER lastWriteTime;

            if (NT_SUCCESS(PhQueryKeyLastWriteTime(keyHandle, &lastWriteTime)))
            {
                ServiceNode->KeyLastWriteTime = lastWriteTime;
            }

            NtClose(keyHandle);
        }
        else
        {
            RtlZeroMemory(&ServiceNode->KeyLastWriteTime, sizeof(ServiceNode->KeyLastWriteTime));
        }

        SetFlag(ServiceNode->ValidMask, PHSN_KEY);
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

BEGIN_SORT_FUNCTION(Pid)
{
    sortResult = uintptrcmp((ULONG_PTR)serviceItem1->ProcessId, (ULONG_PTR)serviceItem2->ProcessId);
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

BEGIN_SORT_FUNCTION(ExitCode)
{
    sortResult = uintcmp(serviceItem1->Win32ExitCode, serviceItem2->Win32ExitCode);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpServiceTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
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

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(Pid),
                    SORT_FUNCTION(DisplayName),
                    SORT_FUNCTION(Type),
                    SORT_FUNCTION(Status),
                    SORT_FUNCTION(StartType),
                    SORT_FUNCTION(BinaryPath),
                    SORT_FUNCTION(ErrorControl),
                    SORT_FUNCTION(Group),
                    SORT_FUNCTION(Description),
                    SORT_FUNCTION(KeyModifiedTime),
                    SORT_FUNCTION(VerificationStatus),
                    SORT_FUNCTION(VerifiedSigner),
                    SORT_FUNCTION(FileName),
                    SORT_FUNCTION(KeyModifiedTime), // Timeline
                    SORT_FUNCTION(ExitCode),
                };
                int (__cdecl *sortFunction)(const void *, const void *);

                static_assert(RTL_NUMBER_OF(sortFunctions) == PHSVTLC_MAXIMUM, "SortFunctions must equal maximum.");

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

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            PPH_SERVICE_ITEM serviceItem;

            node = (PPH_SERVICE_NODE)getCellText->Node;
            serviceItem = node->ServiceItem;

            switch (getCellText->Id)
            {
            case PHSVTLC_NAME:
                getCellText->Text = PhGetStringRef(serviceItem->Name);
                break;
            case PHSVTLC_PID:
                {
                    if (PH_IS_REAL_PROCESS_ID(serviceItem->ProcessId))
                    {
                        PhInitializeStringRefLongHint(&getCellText->Text, serviceItem->ProcessIdString);
                    }
                }
                break;
            case PHSVTLC_DISPLAYNAME:
                getCellText->Text = PhGetStringRef(serviceItem->DisplayName);
                break;
            case PHSVTLC_TYPE:
                {
                    PCPH_STRINGREF string;

                    string = PhGetServiceTypeString(serviceItem->Type);
                    getCellText->Text.Buffer = string->Buffer;
                    getCellText->Text.Length = string->Length;
                }
                break;
            case PHSVTLC_STATUS:
                {
                    PCPH_STRINGREF string;

                    string = PhGetServiceStateString(serviceItem->State);
                    getCellText->Text.Buffer = string->Buffer;
                    getCellText->Text.Length = string->Length;
                }
                break;
            case PHSVTLC_STARTTYPE:
                {
                    PH_FORMAT format[2];
                    PCPH_STRINGREF string;
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
            case PHSVTLC_BINARYPATH:
                {
                    PhpUpdateServiceNodeConfig(node);
                    getCellText->Text = PhGetStringRef(node->BinaryPath);
                }
                break;
            case PHSVTLC_ERRORCONTROL:
                {
                    PCPH_STRINGREF string;

                    string = PhGetServiceErrorControlString(serviceItem->ErrorControl);
                    getCellText->Text.Buffer = string->Buffer;
                    getCellText->Text.Length = string->Length;
                }
                break;
            case PHSVTLC_GROUP:
                {
                    PhpUpdateServiceNodeConfig(node);
                    getCellText->Text = PhGetStringRef(node->LoadOrderGroup);
                }
                break;
            case PHSVTLC_DESCRIPTION:
                {
                    PhpUpdateServiceNodeDescription(node);
                    getCellText->Text = PhGetStringRef(node->Description);
                }
                break;
            case PHSVTLC_KEYMODIFIEDTIME:
                {
                    PhpUpdateServiceNodeKey(node);

                    if (node->KeyLastWriteTime.QuadPart != 0)
                    {
                        SYSTEMTIME systemTime;

                        PhLargeIntegerToLocalSystemTime(&systemTime, &node->KeyLastWriteTime);
                        PhMoveReference(&node->KeyModifiedTimeText, PhFormatDateTime(&systemTime));
                        getCellText->Text = node->KeyModifiedTimeText->sr;
                    }
                }
                break;
            case PHSVTLC_VERIFICATIONSTATUS:
                {
                    if (PhEnableServiceQueryStage2)
                        getCellText->Text = PhVerifyResultToStringRef(serviceItem->VerifyResult);
                    else
                        PhInitializeStringRef(&getCellText->Text, L"Service digital signature support disabled.");
                }
                break;
            case PHSVTLC_VERIFIEDSIGNER:
                {
                    if (PhEnableServiceQueryStage2)
                        getCellText->Text = PhGetStringRef(serviceItem->VerifySignerName);
                    else
                        PhInitializeStringRef(&getCellText->Text, L"Service digital signature support disabled.");
                }
                break;
            case PHSVTLC_FILENAME:
                {
                    getCellText->Text = PhGetStringRef(serviceItem->FileName);
                }
                break;
            case PHSVTLC_EXITCODE:
                {
                    if (serviceItem->Win32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR)
                    {
                        PhMoveReference(&node->ExitCodeText, PhFormatUInt64(serviceItem->ServiceSpecificExitCode, FALSE));
                        getCellText->Text = node->ExitCodeText->sr;
                    }
                    else
                    {
                        PH_STRING_BUILDER stringBuilder;
                        PPH_STRING statusMessage;

                        PhInitializeStringBuilder(&stringBuilder, 0x50);
                        statusMessage = PhGetStatusMessage(0, serviceItem->Win32ExitCode);
                        PhAppendFormatStringBuilder(&stringBuilder, L"(0x%lx) ", serviceItem->Win32ExitCode);

                        if (!PhIsNullOrEmptyString(statusMessage))
                        {
                            PhAppendStringBuilder(&stringBuilder, &statusMessage->sr);
                            PhClearReference(&statusMessage);
                        }

                        PhMoveReference(&node->ExitCodeText, PhFinalStringBuilderString(&stringBuilder));
                        getCellText->Text = node->ExitCodeText->sr;
                    }
                }
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
            node = (PPH_SERVICE_NODE)getNodeIcon->Node;

            if (node->ServiceItem->IconEntry)
            {
                getNodeIcon->Icon = (HICON)(ULONG_PTR)node->ServiceItem->IconEntry->SmallIconIndex;
            }
            else
            {
                if (FlagOn(node->ServiceItem->Type, SERVICE_DRIVER))
                    getNodeIcon->Icon = (HICON)(ULONG_PTR)1; // ServiceCogIcon
                else
                    getNodeIcon->Icon = (HICON)(ULONG_PTR)0; // ServiceApplicationIcon
            }

            //getNodeIcon->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;
            PPH_SERVICE_ITEM serviceItem;

            node = (PPH_SERVICE_NODE)getNodeColor->Node;
            serviceItem = node->ServiceItem;

            if (!serviceItem)
                ; // Dummy
            else if (PhEnableServiceQueryStage2 && PhCsUseColorUnknown && PH_VERIFY_UNTRUSTED(serviceItem->VerifyResult))
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
                        &customDraw->CellRect,
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
            PPH_TREENEW_SORT_CHANGED_EVENT sorting = Parameter1;

            ServiceTreeListSortColumn = sorting->SortColumn;
            ServiceTreeListSortOrder = sorting->SortOrder;

            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(PhMainWndHandle, WM_COMMAND, ID_SERVICE_COPY, 0);
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
            data.DefaultSortColumn = PHSVTLC_NAME;
            data.DefaultSortOrder = NoSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

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

    *NumberOfServices = (ULONG)PhFinalArrayCount(&array);
    *Services = PhFinalArrayItems(&array);
}

VOID PhDeselectAllServiceNodes(
    VOID
    )
{
    TreeNew_DeselectRange(ServiceTreeListHandle, 0, -1);
}

BOOLEAN PhSelectAndEnsureVisibleServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    )
{
    PhDeselectAllServiceNodes();

    if (ServiceNode->Node.Visible)
    {
        TreeNew_FocusMarkSelectNode(ServiceTreeListHandle, &ServiceNode->Node);
        return TRUE;
    }
    else
    {
        PhShowInformation2(
            PhMainWndHandle,
            L"Unable to perform the operation.",
            L"%s",
            L"This node cannot be displayed because it is currently hidden by your active filter settings or preferences."
            );
        return FALSE;
    }
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
