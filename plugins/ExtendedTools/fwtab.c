/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2015-2024
 *
 */

#include "exttools.h"
#include <toolstatusintf.h>
#include <fwpmu.h>
#include <fwpsu.h>

BOOLEAN FwTreeNewCreated = FALSE;
HWND FwTreeNewHandle = NULL;
ULONG FwTreeNewSortColumn = FW_COLUMN_NAME;
PH_SORT_ORDER FwTreeNewSortOrder = NoSortOrder;
PH_STRINGREF FwTreeEmptyText = PH_STRINGREF_INIT(L"Firewall monitoring requires System Informer to be restarted with administrative privileges.");
PPH_STRING FwTreeErrorText = NULL;
LONG FwTreeIconHeightPadding = 0;
LONG FwTreeLeftMarginPadding = 0;
LONG FwTreeRightMarginPadding = 0;
PPH_MAIN_TAB_PAGE EtFwAddedTabPage;
PH_PROVIDER_EVENT_QUEUE FwNetworkEventQueue;
PH_CALLBACK_REGISTRATION FwItemAddedRegistration;
PH_CALLBACK_REGISTRATION FwItemModifiedRegistration;
PH_CALLBACK_REGISTRATION FwItemRemovedRegistration;
PH_CALLBACK_REGISTRATION FwItemsUpdatedRegistration;
PH_TN_FILTER_SUPPORT EtFwFilterSupport;
PTOOLSTATUS_INTERFACE EtFwToolStatusInterface;
PH_CALLBACK_REGISTRATION EtFwSearchChangedRegistration;
BOOLEAN EtFwEnabled = FALSE;
ULONG EtFwStatus = ERROR_SUCCESS;
PPH_STRING EtFwStatusText = NULL;
PPH_LIST FwNodeList = NULL;

BOOLEAN FwTabPageCallback(
    _In_ PPH_MAIN_TAB_PAGE Page,
    _In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2)
{
    switch (Message)
    {
    case MainTabPageCreateWindow:
        {
            HWND hwnd;
            ULONG thinRows;
            ULONG treelistBorder;
            ULONG treelistCustomColors;
            PH_TREENEW_CREATEPARAMS treelistCreateParams = { 0 };

            thinRows = PhGetIntegerSetting(L"ThinRows") ? TN_STYLE_THIN_ROWS : 0;
            treelistBorder = (PhGetIntegerSetting(L"TreeListBorderEnable") && !PhGetIntegerSetting(L"EnableThemeSupport")) ? WS_BORDER : 0;
            treelistCustomColors = PhGetIntegerSetting(L"TreeListCustomColorsEnable") ? TN_STYLE_CUSTOM_COLORS : 0;

            if (treelistCustomColors)
            {
                treelistCreateParams.TextColor = PhGetIntegerSetting(L"TreeListCustomColorText");
                treelistCreateParams.FocusColor = PhGetIntegerSetting(L"TreeListCustomColorFocus");
                treelistCreateParams.SelectionColor = PhGetIntegerSetting(L"TreeListCustomColorSelection");
            }

            hwnd = CreateWindow(
                PH_TREENEW_CLASSNAME,
                NULL,
                WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED | thinRows | treelistBorder | treelistCustomColors,
                0,
                0,
                0,
                0,
                Parameter2,
                NULL,
                PluginInstance->DllBase,
                &treelistCreateParams
                );

            if (!hwnd)
                return FALSE;

            FwTreeNewCreated = TRUE;

            if (PhGetIntegerSetting(L"EnableThemeSupport"))
            {
                PhInitializeWindowTheme(hwnd, TRUE); // HACK (dmex)
                TreeNew_ThemeSupport(hwnd, TRUE);
            }

            PhInitializeProviderEventQueue(&FwNetworkEventQueue, 100);

            InitializeFwTreeList(hwnd);

            if (PhGetOwnTokenAttributes().Elevated)
            {
                EtFwStatus = EtFwMonitorInitialize();

                if (EtFwStatus == ERROR_SUCCESS)
                    EtFwEnabled = TRUE;
            }

            if (EtFwEnabled)
            {
                EtFwMonitorEnumEvents();
            }
            else
            {
                if (EtFwStatus != ERROR_SUCCESS)
                {
                    PPH_STRING statusMessage;

                    if (statusMessage = PhGetStatusMessage(0, EtFwStatus))
                    {
                        EtFwStatusText = PhFormatString(
                            L"%s %s (%lu)",
                            L"Unable to start the firewall event tracing session: ",
                            statusMessage->Buffer,
                            EtFwStatus);
                        PhDereferenceObject(statusMessage);
                    }
                    else
                    {
                        EtFwStatusText = PhFormatString(
                            L"%s (%lu)",
                            L"Unable to start the firewall event tracing session: ",
                            EtFwStatus);
                    }

                    TreeNew_SetEmptyText(hwnd, &EtFwStatusText->sr, 0);
                }
                else
                {
                    TreeNew_SetEmptyText(hwnd, &FwTreeEmptyText, 0);
                }
            }

            PhRegisterCallback(
                &FwItemAddedEvent,
                FwItemAddedHandler,
                NULL,
                &FwItemAddedRegistration
                );
            PhRegisterCallback(
                &FwItemModifiedEvent,
                FwItemModifiedHandler,
                NULL,
                &FwItemModifiedRegistration
                );
            PhRegisterCallback(
                &FwItemRemovedEvent,
                FwItemRemovedHandler,
                NULL,
                &FwItemRemovedRegistration
                );
            PhRegisterCallback(
                &FwItemsUpdatedEvent,
                FwItemsUpdatedHandler,
                NULL,
                &FwItemsUpdatedRegistration
                );

            if (Parameter1)
            {
                *(HWND*)Parameter1 = hwnd;
            }
        }
        return TRUE;
    case MainTabPageLoadSettings:
        {
            NOTHING;
        }
        return TRUE;
    case MainTabPageSaveSettings:
        {
            SaveSettingsFwTreeList();
        }
        return TRUE;
    case MainTabPageSelected:
        {
            BOOLEAN selected = (BOOLEAN)Parameter1;

            if (selected)
            {
                EtFwEnabled = TRUE;
            }
            else
            {
                EtFwEnabled = FALSE;
            }
        }
        break;
    case MainTabPageExportContent:
        {
            PPH_MAIN_TAB_PAGE_EXPORT_CONTENT exportContent = Parameter1;

            if (!(EtFwEnabled && exportContent))
                return FALSE;

            EtFwWriteFwList(exportContent->FileStream, exportContent->Mode);
        }
        return TRUE;
    case MainTabPageFontChanged:
        {
            HFONT font = (HFONT)Parameter1;

            if (FwTreeNewHandle)
                SendMessage(FwTreeNewHandle, WM_SETFONT, (WPARAM)Parameter1, TRUE);
        }
        break;
    case MainTabPageDpiChanged:
        {
            if (FwTreeNewHandle)
            {
                InitializeFwTreeListDpi(FwTreeNewHandle);
            }
        }
        break;
    }

    return FALSE;
}

VOID EtInitializeFirewallTab(
    VOID
    )
{
    PH_MAIN_TAB_PAGE page;

    memset(&page, 0, sizeof(PH_MAIN_TAB_PAGE));
    PhInitializeStringRef(&page.Name, L"Firewall");
    page.Callback = FwTabPageCallback;
    EtFwAddedTabPage = PhPluginCreateTabPage(&page);

    if (EtFwToolStatusInterface = PhGetPluginInterfaceZ(TOOLSTATUS_PLUGIN_NAME, TOOLSTATUS_INTERFACE_VERSION))
    {
        PTOOLSTATUS_TAB_INFO tabInfo;

        tabInfo = EtFwToolStatusInterface->RegisterTabInfo(EtFwAddedTabPage->Index);
        tabInfo->BannerText = L"Search Firewall";
        tabInfo->ActivateContent = FwToolStatusActivateContent;
        tabInfo->GetTreeNewHandle = FwToolStatusGetTreeNewHandle;
    }
}

VOID InitializeFwTreeList(
    _In_ HWND TreeNewHandle
    )
{
    FwNodeList = PhCreateList(100);
    FwTreeNewHandle = TreeNewHandle;

    InitializeFwTreeListDpi(FwTreeNewHandle);

    PhSetControlTheme(FwTreeNewHandle, !PhGetIntegerSetting(L"EnableThemeSupport") ? L"explorer" : L"DarkMode_Explorer");
    TreeNew_SetRedraw(FwTreeNewHandle, FALSE);
    TreeNew_SetCallback(FwTreeNewHandle, FwTreeNewCallback, NULL);

    PhAddTreeNewColumnEx2(FwTreeNewHandle, FW_COLUMN_NAME, TRUE, L"Name", 140, PH_ALIGN_LEFT, FW_COLUMN_NAME, 0, TN_COLUMN_FLAG_CUSTOMDRAW);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_ACTION, TRUE, L"Action", 70, PH_ALIGN_LEFT, FW_COLUMN_ACTION, 0);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_DIRECTION, TRUE, L"Direction", 40, PH_ALIGN_LEFT, FW_COLUMN_DIRECTION, 0);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_RULENAME, TRUE, L"Rule", 240, PH_ALIGN_LEFT, FW_COLUMN_RULENAME, 0);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_RULEDESCRIPTION, TRUE, L"Description", 180, PH_ALIGN_LEFT, FW_COLUMN_RULEDESCRIPTION, 0);
    PhAddTreeNewColumnEx(FwTreeNewHandle, FW_COLUMN_LOCALADDRESS, TRUE, L"Local address", 220, PH_ALIGN_RIGHT, FW_COLUMN_LOCALADDRESS, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(FwTreeNewHandle, FW_COLUMN_LOCALPORT, TRUE, L"Local port", 50, PH_ALIGN_LEFT, FW_COLUMN_LOCALPORT, DT_LEFT, TRUE);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_LOCALHOSTNAME, TRUE, L"Local hostname", 70, PH_ALIGN_LEFT, FW_COLUMN_LOCALHOSTNAME, 0);
    PhAddTreeNewColumnEx(FwTreeNewHandle, FW_COLUMN_REMOTEADDRESS, TRUE, L"Remote address", 220, PH_ALIGN_RIGHT, FW_COLUMN_REMOTEADDRESS, DT_RIGHT, TRUE);
    PhAddTreeNewColumnEx(FwTreeNewHandle, FW_COLUMN_REMOTEPORT, TRUE, L"Remote port", 50, PH_ALIGN_LEFT, FW_COLUMN_REMOTEPORT, DT_LEFT, TRUE);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_REMOTEHOSTNAME, TRUE, L"Remote hostname", 70, PH_ALIGN_LEFT, FW_COLUMN_REMOTEHOSTNAME, 0);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_PROTOCOL, TRUE, L"Protocol", 60, PH_ALIGN_LEFT, FW_COLUMN_PROTOCOL, 0);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_TIMESTAMP, TRUE, L"Timestamp", 60, PH_ALIGN_LEFT, FW_COLUMN_TIMESTAMP, 0);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_PROCESSFILENAME, FALSE, L"File path", 100, PH_ALIGN_LEFT, FW_COLUMN_PROCESSFILENAME, DT_PATH_ELLIPSIS);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_USER, FALSE, L"Username", 100, PH_ALIGN_LEFT, FW_COLUMN_USER, 0);
    //PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_PACKAGE, FALSE, L"Package", 100, PH_ALIGN_LEFT, FW_COLUMN_PACKAGE, 0);
    PhAddTreeNewColumnEx2(FwTreeNewHandle, FW_COLUMN_COUNTRY, FALSE, L"Country", 80, PH_ALIGN_LEFT, FW_COLUMN_COUNTRY, 0, TN_COLUMN_FLAG_CUSTOMDRAW);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_LOCALADDRESSCLASS, FALSE, L"Local address class", 80, PH_ALIGN_LEFT, FW_COLUMN_LOCALADDRESSCLASS, 0);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_REMOTEADDRESSCLASS, FALSE, L"Remote address class", 80, PH_ALIGN_LEFT, FW_COLUMN_REMOTEADDRESSCLASS, 0);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_LOCALADDRESSSSCOPE, FALSE, L"Local address scope", 80, PH_ALIGN_LEFT, FW_COLUMN_LOCALADDRESSSSCOPE, 0);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_REMOTEADDRESSSCOPE, FALSE, L"Remote address scope", 80, PH_ALIGN_LEFT, FW_COLUMN_REMOTEADDRESSSCOPE, 0);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_ORIGINALNAME, FALSE, L"Original name", 100, PH_ALIGN_LEFT, FW_COLUMN_ORIGINALNAME, DT_PATH_ELLIPSIS);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_LOCALSERVICENAME, FALSE, L"Local port service", 80, PH_ALIGN_LEFT, FW_COLUMN_LOCALSERVICENAME, 0);
    PhAddTreeNewColumn(FwTreeNewHandle, FW_COLUMN_REMOTESERVICENAME, FALSE, L"Remote port service", 80, PH_ALIGN_LEFT, FW_COLUMN_REMOTESERVICENAME, 0);

    PhInitializeTreeNewFilterSupport(&EtFwFilterSupport, TreeNewHandle, FwNodeList);

    TreeNew_SetSort(FwTreeNewHandle, FW_COLUMN_TIMESTAMP, NoSortOrder);
    TreeNew_SetTriState(FwTreeNewHandle, TRUE);
    TreeNew_SetRedraw(FwTreeNewHandle, TRUE);

    LoadSettingsFwTreeList(TreeNewHandle);

    if (EtFwToolStatusInterface)
    {
        PhRegisterCallback(EtFwToolStatusInterface->SearchChangedEvent, FwSearchChangedHandler, NULL, &EtFwSearchChangedRegistration);
        PhAddTreeNewFilter(&EtFwFilterSupport, FwSearchFilterCallback, NULL);
    }
}

VOID InitializeFwTreeListDpi(
    _In_ HWND TreeNewHandle
    )
{
#define TNP_CELL_LEFT_MARGIN 6
#define TNP_ICON_RIGHT_PADDING 4
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(TreeNewHandle);
    FwTreeIconHeightPadding = PhGetSystemMetrics(SM_CYSMICON, dpiValue);
    FwTreeLeftMarginPadding = PhGetDpi(TNP_CELL_LEFT_MARGIN, dpiValue);
    FwTreeRightMarginPadding = PhGetSystemMetrics(SM_CXSMICON, dpiValue) + PhGetDpi(TNP_ICON_RIGHT_PADDING, dpiValue);
}

VOID LoadSettingsFwTreeUpdateMask(
    VOID
    )
{
    PH_TREENEW_COLUMN column;
    BOOLEAN current;

    current = BooleanFlagOn(EtFwFlagsMask, FW_PROVIDER_FLAG_HOSTNAME);

    if (
        (TreeNew_GetColumn(FwTreeNewHandle, FW_COLUMN_LOCALHOSTNAME, &column) && column.Visible) ||
        (TreeNew_GetColumn(FwTreeNewHandle, FW_COLUMN_REMOTEHOSTNAME, &column) && column.Visible)
        )
    {
        SetFlag(EtFwFlagsMask, FW_PROVIDER_FLAG_HOSTNAME);
    }
    else
    {
        ClearFlag(EtFwFlagsMask, FW_PROVIDER_FLAG_HOSTNAME);
    }

    if (ProcessesUpdatedCount != 0 && current != BooleanFlagOn(EtFwFlagsMask, FW_PROVIDER_FLAG_HOSTNAME))
    {
        EtFwInvalidateAllFwNodesHostnames();
    }
}

VOID LoadSettingsFwTreeList(
    _In_ HWND TreeNewHandle
    )
{
    PPH_STRING settings;
    PH_INTEGER_PAIR sortSettings;

    settings = PhGetStringSetting(SETTING_NAME_FW_TREE_LIST_COLUMNS);
    PhCmLoadSettings(TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);

    sortSettings = PhGetIntegerPairSetting(SETTING_NAME_FW_TREE_LIST_SORT);
    TreeNew_SetSort(TreeNewHandle, (ULONG)sortSettings.X, (PH_SORT_ORDER)sortSettings.Y);

    if (PhGetIntegerSetting(L"EnableInstantTooltips"))
    {
        SendMessage(TreeNew_GetTooltips(TreeNewHandle), TTM_SETDELAYTIME, TTDT_INITIAL, 0);
    }
    else
    {
        SendMessage(TreeNew_GetTooltips(TreeNewHandle), TTM_SETDELAYTIME, TTDT_AUTOPOP, MAXSHORT);
    }

    LoadSettingsFwTreeUpdateMask();
}

VOID SaveSettingsFwTreeList(
    VOID
    )
{
    PPH_STRING settings;
    PH_INTEGER_PAIR sortSettings;
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;

    if (!FwTreeNewCreated)
        return;

    settings = PhCmSaveSettings(FwTreeNewHandle);
    PhSetStringSetting2(SETTING_NAME_FW_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);

    TreeNew_GetSort(FwTreeNewHandle, &sortColumn, &sortOrder);
    sortSettings.X = sortColumn;
    sortSettings.Y = sortOrder;
    PhSetIntegerPairSetting(SETTING_NAME_FW_TREE_LIST_SORT, sortSettings);
}

PFW_EVENT_ITEM AddFwNode(
    _In_ PFW_EVENT_ITEM FwItem
    )
{
    PhInitializeTreeNewNode(&FwItem->Node);

    memset(FwItem->TextCache, 0, sizeof(PH_STRINGREF) * FW_COLUMN_MAXIMUM);
    FwItem->Node.TextCache = FwItem->TextCache;
    FwItem->Node.TextCacheSize = FW_COLUMN_MAXIMUM;

    PhInsertItemList(FwNodeList, 0, FwItem);

    if (EtFwFilterSupport.NodeList)
        FwItem->Node.Visible = PhApplyTreeNewFiltersToNode(&EtFwFilterSupport, &FwItem->Node);

    //TreeNew_NodesStructured(FwTreeNewHandle);

    return FwItem;
}

VOID RemoveFwNode(
    _In_ PFW_EVENT_ITEM FwNode
    )
{
    ULONG index;

    if ((index = PhFindItemList(FwNodeList, FwNode)) != ULONG_MAX)
        PhRemoveItemList(FwNodeList, index);

    PhDereferenceObject(FwNode);

    //TreeNew_NodesStructured(FwTreeNewHandle);
}

VOID UpdateFwNode(
    _In_ PFW_EVENT_ITEM FwNode
    )
{
    memset(FwNode->TextCache, 0, sizeof(PH_STRINGREF) * FW_COLUMN_MAXIMUM);

    PhInvalidateTreeNewNode(&FwNode->Node, TN_CACHE_ICON);
    //TreeNew_NodesStructured(FwTreeNewHandle);
}

VOID FwTickNodes(
    VOID
    )
{
    // Reset list once in a while.
    {
        static ULONG64 lastTickCount = 0;
        ULONG64 tickCount = NtGetTickCount64();

        if (tickCount - lastTickCount >= UInt32x32To64(240, CLOCKS_PER_SEC))
        {
            PPH_LIST newList;
            PPH_LIST oldList;

            newList = PhCreateList(FwNodeList->Count);
            PhAddItemsList(newList, FwNodeList->Items, FwNodeList->Count);

            oldList = FwNodeList;
            FwNodeList = newList;
            EtFwFilterSupport.NodeList = newList; // HACK
            PhDereferenceObject(oldList);
            lastTickCount = tickCount;
        }
    }

    // Text invalidation
    for (ULONG i = 0; i < FwNodeList->Count; i++)
    {
        PFW_EVENT_ITEM node = (PFW_EVENT_ITEM)FwNodeList->Items[i];

        // The name and file name never change, so we don't invalidate that.
        memset(&node->TextCache[1], 0, sizeof(PH_STRINGREF) * (FW_COLUMN_MAXIMUM - 2));
        // Always get the newest tooltip text from the process tree.
        PhSwapReference(&node->TooltipText, NULL);
    }

    //InvalidateRect(FwTreeNewHandle, NULL, FALSE);
    TreeNew_NodesStructured(FwTreeNewHandle);
}

VOID FwUpdateNodeTimeStamp(
    _In_ PFW_EVENT_ITEM FwNode
    )
{
    SYSTEMTIME systemTime;

    PhLargeIntegerToLocalSystemTime(&systemTime, &FwNode->TimeStamp);

    PhMoveReference(&FwNode->TimeString, PhFormatDateTime(&systemTime));
}

VOID FwUpdateNodeUserSid(
    _In_ PFW_EVENT_ITEM FwNode
    )
{
    if (FwNode->UserSid)
    {
        PhMoveReference(&FwNode->UserName, EtFwGetSidFullNameCachedSlow(FwNode->UserSid));
    }
    else
    {
        PhClearReference(&FwNode->UserName);
    }
}

VOID FwUpdateNodeLocalPortServiceName(
    _In_ PFW_EVENT_ITEM FwNode
    )
{
    if (!FwNode->LocalPortServiceResolved)
    {
        PPH_STRINGREF string;

        if (EtFwLookupPortServiceName(FwNode->LocalEndpoint.Port, &string))
        {
            FwNode->LocalPortServiceName = string;
        }

        FwNode->LocalPortServiceResolved = TRUE;
    }
}

VOID FwUpdateNodeRemotePortServiceName(
    _In_ PFW_EVENT_ITEM FwNode
    )
{
    if (!FwNode->RemotePortServiceResolved)
    {
        PPH_STRINGREF string;

        if (EtFwLookupPortServiceName(FwNode->RemoteEndpoint.Port, &string))
        {
            FwNode->RemotePortServiceName = string;
        }

        FwNode->RemotePortServiceResolved = TRUE;
    }
}

#define SORT_FUNCTION(Column) FwTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl FwTreeNewCompare##Column( \
    _In_ void const* _elem1, \
    _In_ void const* _elem2 \
    ) \
{ \
    PFW_EVENT_ITEM node1 = *(PFW_EVENT_ITEM *)_elem1; \
    PFW_EVENT_ITEM node2 = *(PFW_EVENT_ITEM *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uint64cmp(node1->Index, node2->Index); \
    \
    return PhModifySort(sortResult, FwTreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->ProcessBaseString, node2->ProcessBaseString, FwTreeNewSortOrder, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Action)
{
    sortResult = uint64cmp(node1->Type, node2->Type);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Direction)
{
    sortResult = uint64cmp(node1->Direction, node2->Direction);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RuleName)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->RuleName, node2->RuleName, FwTreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RuleDescription)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->RuleDescription, node2->RuleDescription, FwTreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LocalAddress)
{
    SOCKADDR_IN6 localAddress1;
    SOCKADDR_IN6 localAddress2;
    SCOPE_ID scopeId1;
    SCOPE_ID scopeId2;

    memset(&localAddress1, 0, sizeof(SOCKADDR_IN6)); // memset for zero padding (dmex)
    memset(&localAddress2, 0, sizeof(SOCKADDR_IN6));

    if (node1->LocalEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        localAddress1.sin6_family = AF_INET6;
        IN6_SET_ADDR_V4COMPAT(&localAddress1.sin6_addr, &node1->LocalEndpoint.Address.InAddr);
        IN4_UNCANONICALIZE_SCOPE_ID(&node1->LocalEndpoint.Address.InAddr, &localAddress1.sin6_scope_struct);
        //IN6ADDR_SETV4MAPPED(&localAddress1, &node1->LocalEndpoint.Address.InAddr, (SCOPE_ID)SCOPEID_UNSPECIFIED_INIT, 0);
    }
    else if (node1->LocalEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
    {
        scopeId1.Value = node1->ScopeId;
        IN6ADDR_SETSOCKADDR(&localAddress1, &node1->LocalEndpoint.Address.In6Addr, scopeId1, 0);
    }

    if (node2->LocalEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        localAddress2.sin6_family = AF_INET6;
        IN6_SET_ADDR_V4COMPAT(&localAddress2.sin6_addr, &node2->LocalEndpoint.Address.InAddr);
        IN4_UNCANONICALIZE_SCOPE_ID(&node2->LocalEndpoint.Address.InAddr, &localAddress2.sin6_scope_struct);
        //IN6ADDR_SETV4MAPPED(&localAddress2, &node2->LocalEndpoint.Address.InAddr, (SCOPE_ID)SCOPEID_UNSPECIFIED_INIT, 0);
    }
    else if (node2->LocalEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
    {
        scopeId2.Value = node2->ScopeId;
        IN6ADDR_SETSOCKADDR(&localAddress2, &node2->LocalEndpoint.Address.In6Addr, scopeId2, 0);
    }

    sortResult = memcmp(&localAddress1, &localAddress2, sizeof(SOCKADDR_IN6));
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LocalPort)
{
    sortResult = uintcmp(node1->LocalEndpoint.Port, node2->LocalEndpoint.Port);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LocalHostname)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->LocalHostnameString, node2->LocalHostnameString, FwTreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RemoteAddress)
{
    SOCKADDR_IN6 remoteAddress1;
    SOCKADDR_IN6 remoteAddress2;
    SCOPE_ID scopeId1;
    SCOPE_ID scopeId2;

    memset(&remoteAddress1, 0, sizeof(SOCKADDR_IN6)); // memset for zero padding (dmex)
    memset(&remoteAddress2, 0, sizeof(SOCKADDR_IN6));

    if (node1->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        remoteAddress1.sin6_family = AF_INET6;
        IN6_SET_ADDR_V4COMPAT(&remoteAddress1.sin6_addr, &node1->RemoteEndpoint.Address.InAddr);
        IN4_UNCANONICALIZE_SCOPE_ID(&node1->RemoteEndpoint.Address.InAddr, &remoteAddress1.sin6_scope_struct);
        //IN6ADDR_SETV4MAPPED(&remoteAddress1, &node1->RemoteEndpoint.Address.InAddr, (SCOPE_ID)SCOPEID_UNSPECIFIED_INIT, 0);
    }
    else if (node1->RemoteEndpoint.Address.Type & PH_IPV6_NETWORK_TYPE)
    {
        scopeId1.Value = node1->ScopeId;
        IN6ADDR_SETSOCKADDR(&remoteAddress1, &node1->RemoteEndpoint.Address.In6Addr, scopeId1, 0);
    }

    if (node2->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        remoteAddress2.sin6_family = AF_INET6;
        IN6_SET_ADDR_V4COMPAT(&remoteAddress2.sin6_addr, &node2->RemoteEndpoint.Address.InAddr);
        IN4_UNCANONICALIZE_SCOPE_ID(&node2->RemoteEndpoint.Address.InAddr, &remoteAddress2.sin6_scope_struct);
        //IN6ADDR_SETV4MAPPED(&remoteAddress2, &node2->RemoteEndpoint.Address.InAddr, (SCOPE_ID)SCOPEID_UNSPECIFIED_INIT, 0);
    }
    else if (node2->RemoteEndpoint.Address.Type & PH_IPV6_NETWORK_TYPE)
    {
        scopeId2.Value = node2->ScopeId;
        IN6ADDR_SETSOCKADDR(&remoteAddress2, &node2->RemoteEndpoint.Address.In6Addr, scopeId2, 0);
    }

    sortResult = memcmp(&remoteAddress1, &remoteAddress2, sizeof(SOCKADDR_IN6));
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RemotePort)
{
    sortResult = uintcmp(node1->RemoteEndpoint.Port, node2->RemoteEndpoint.Port);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RemoteHostname)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->RemoteHostnameString, node2->RemoteHostnameString, FwTreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Protocal)
{
    sortResult = uint64cmp(node1->IpProtocol, node2->IpProtocol);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Timestamp)
{
    FwUpdateNodeTimeStamp(node1);
    FwUpdateNodeTimeStamp(node2);

    sortResult = uint64cmp(node1->Index, node2->Index);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Filename)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->ProcessFileName, node2->ProcessFileName, FwTreeNewSortOrder, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(User)
{
    FwUpdateNodeUserSid(node1);
    FwUpdateNodeUserSid(node2);

    sortResult = PhCompareStringWithNullSortOrder(node1->UserName, node2->UserName, FwTreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Package)
{
    NOTHING;
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Country)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->RemoteCountryName, node2->RemoteCountryName, FwTreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LocalAddressClass)
{
    NOTHING;
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RemoteAddressClass)
{
    NOTHING;
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LocalAddressScope)
{
    NOTHING;
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RemoteAddressScope)
{
    NOTHING;
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LocalPortServiceName)
{
    FwUpdateNodeLocalPortServiceName(node1);
    FwUpdateNodeLocalPortServiceName(node2);

    sortResult = PhCompareStringRefWithNullSortOrder(node1->LocalPortServiceName, node2->LocalPortServiceName, FwTreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RemotePortServiceName)
{
    FwUpdateNodeRemotePortServiceName(node1);
    FwUpdateNodeRemotePortServiceName(node2);

    sortResult = PhCompareStringRefWithNullSortOrder(node1->RemotePortServiceName, node2->RemotePortServiceName, FwTreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

int __cdecl EtFwNodeNoOrderSortFunction(
    _In_ void const* _elem1,
    _In_ void const* _elem2
    )
{
    PFW_EVENT_ITEM node1 = *(PFW_EVENT_ITEM*)_elem1;
    PFW_EVENT_ITEM node2 = *(PFW_EVENT_ITEM*)_elem2;
    int sortResult;

    sortResult = uint64cmp(node1->Index, node2->Index);

    return PhModifySort(sortResult, DescendingSortOrder);
}

BOOLEAN NTAPI FwTreeNewCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PFW_EVENT_ITEM node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PFW_EVENT_ITEM)getChildren->Node;

            if (FwTreeNewSortOrder == NoSortOrder)
            {
                if (!node)
                {
                    qsort(FwNodeList->Items, FwNodeList->Count, sizeof(PVOID), EtFwNodeNoOrderSortFunction);

                    getChildren->Children = (PPH_TREENEW_NODE*)FwNodeList->Items;
                    getChildren->NumberOfChildren = FwNodeList->Count;
                }
            }
            else
            {
                if (!getChildren->Node)
                {
                    static PVOID sortFunctions[] =
                    {
                        SORT_FUNCTION(Name),
                        SORT_FUNCTION(Action),
                        SORT_FUNCTION(Direction),
                        SORT_FUNCTION(RuleName),
                        SORT_FUNCTION(RuleDescription),
                        SORT_FUNCTION(LocalAddress),
                        SORT_FUNCTION(LocalPort),
                        SORT_FUNCTION(LocalHostname),
                        SORT_FUNCTION(RemoteAddress),
                        SORT_FUNCTION(RemotePort),
                        SORT_FUNCTION(RemoteHostname),
                        SORT_FUNCTION(Protocal),
                        SORT_FUNCTION(Timestamp),
                        SORT_FUNCTION(Filename),
                        SORT_FUNCTION(User),
                        SORT_FUNCTION(Package),
                        SORT_FUNCTION(Country),
                        SORT_FUNCTION(LocalAddressClass),
                        SORT_FUNCTION(RemoteAddressClass),
                        SORT_FUNCTION(LocalAddressScope),
                        SORT_FUNCTION(RemoteAddressScope),
                        SORT_FUNCTION(Filename),
                        SORT_FUNCTION(LocalPortServiceName),
                        SORT_FUNCTION(RemotePortServiceName),
                    };
                    int (__cdecl* sortFunction)(void const*, void const*);

                    static_assert(RTL_NUMBER_OF(sortFunctions) == FW_COLUMN_MAXIMUM, "SortFunctions must equal maximum.");

                    if (FwTreeNewSortColumn < FW_COLUMN_MAXIMUM)
                        sortFunction = sortFunctions[FwTreeNewSortColumn];
                    else
                        sortFunction = NULL;

                    if (sortFunction)
                    {
                        qsort(FwNodeList->Items, FwNodeList->Count, sizeof(PVOID), sortFunction);
                    }

                    getChildren->Children = (PPH_TREENEW_NODE*)FwNodeList->Items;
                    getChildren->NumberOfChildren = FwNodeList->Count;
                }
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = (PPH_TREENEW_IS_LEAF)Parameter1;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PFW_EVENT_ITEM)getCellText->Node;

            switch (getCellText->Id)
            {
            case FW_COLUMN_NAME:
                {
                    getCellText->Text = PhGetStringRef(node->ProcessBaseString);
                }
                break;
            case FW_COLUMN_ACTION:
                {
                    switch (node->Type)
                    {
                    case FWPM_NET_EVENT_TYPE_CLASSIFY_DROP:
                    case FWPM_NET_EVENT_TYPE_CAPABILITY_DROP:
                        PhInitializeStringRef(&getCellText->Text, L"DROP");
                        break;
                    case FWPM_NET_EVENT_TYPE_CLASSIFY_ALLOW:
                    case FWPM_NET_EVENT_TYPE_CAPABILITY_ALLOW:
                        PhInitializeStringRef(&getCellText->Text, L"ALLOW");
                        break;
                    case FWPM_NET_EVENT_TYPE_CLASSIFY_DROP_MAC:
                        PhInitializeStringRef(&getCellText->Text, L"DROP_MAC");
                        break;
                    case FWPM_NET_EVENT_TYPE_IPSEC_KERNEL_DROP:
                        PhInitializeStringRef(&getCellText->Text, L"IPSEC_KERNEL_DROP");
                        break;
                    case FWPM_NET_EVENT_TYPE_IPSEC_DOSP_DROP:
                        PhInitializeStringRef(&getCellText->Text, L"IPSEC_DOSP_DROP");
                        break;
                    case FWPM_NET_EVENT_TYPE_IKEEXT_MM_FAILURE:
                        PhInitializeStringRef(&getCellText->Text, L"IKEEXT_MM_FAILURE");
                        break;
                    case FWPM_NET_EVENT_TYPE_IKEEXT_QM_FAILURE:
                        PhInitializeStringRef(&getCellText->Text, L"QM_FAILURE");
                        break;
                    case FWPM_NET_EVENT_TYPE_IKEEXT_EM_FAILURE:
                        PhInitializeStringRef(&getCellText->Text, L"EM_FAILURE");
                        break;
                    }
                }
                break;
            case FW_COLUMN_DIRECTION:
                {
                    if (node->Loopback)
                    {
                        switch (node->Direction)
                        {
                        case FW_EVENT_DIRECTION_INBOUND:
                            PhInitializeStringRef(&getCellText->Text, L"Loopback [In]");
                            break;
                        case FW_EVENT_DIRECTION_OUTBOUND:
                            PhInitializeStringRef(&getCellText->Text, L"Loopback [Out]");
                            break;
                        case FW_EVENT_DIRECTION_FORWARD:
                            PhInitializeStringRef(&getCellText->Text, L"Loopback [Fwd]");
                            break;
                        case FW_EVENT_DIRECTION_BIDIRECTIONAL:
                            PhInitializeStringRef(&getCellText->Text, L"Loopback [Bi]");
                            break;
                        }
                    }
                    else
                    {
                        switch (node->Direction)
                        {
                        case FW_EVENT_DIRECTION_INBOUND:
                            PhInitializeStringRef(&getCellText->Text, L"In");
                            break;
                        case FW_EVENT_DIRECTION_OUTBOUND:
                            PhInitializeStringRef(&getCellText->Text, L"Out");
                            break;
                        case FW_EVENT_DIRECTION_FORWARD:
                            PhInitializeStringRef(&getCellText->Text, L"Fwd");
                            break;
                        case FW_EVENT_DIRECTION_BIDIRECTIONAL:
                            PhInitializeStringRef(&getCellText->Text, L"Bi");
                            break;
                        }
                    }
                }
                break;
            case FW_COLUMN_RULENAME:
                {
                    getCellText->Text = PhGetStringRef(node->RuleName);
                }
                break;
            case FW_COLUMN_RULEDESCRIPTION:
                {
                    getCellText->Text = PhGetStringRef(node->RuleDescription);
                }
                break;
            case FW_COLUMN_LOCALADDRESS:
                {
                    switch (node->LocalEndpoint.Address.Type)
                    {
                    case PH_IPV4_NETWORK_TYPE:
                        {
                            ULONG ipv4AddressStringLength = RTL_NUMBER_OF(node->LocalAddressString);

                            if (NT_SUCCESS(RtlIpv4AddressToStringEx((PIN_ADDR)&node->LocalEndpoint.Address.Ipv4, 0, node->LocalAddressString, &ipv4AddressStringLength)))
                            {
                                getCellText->Text.Buffer = node->LocalAddressString;
                                getCellText->Text.Length = (node->LocalAddressStringLength = (ipv4AddressStringLength - 1)) * sizeof(WCHAR);
                            }
                            else
                            {
                                PhInitializeEmptyStringRef(&getCellText->Text);
                                node->LocalAddressStringLength = 0;
                            }
                        }
                        break;
                    case PH_IPV6_NETWORK_TYPE:
                        {
                            ULONG ipv6AddressStringLength = RTL_NUMBER_OF(node->LocalAddressString);

                            if (NT_SUCCESS(RtlIpv6AddressToStringEx((PIN6_ADDR)&node->LocalEndpoint.Address.Ipv6, node->ScopeId, 0, node->LocalAddressString, &ipv6AddressStringLength)))
                            {
                                getCellText->Text.Buffer = node->LocalAddressString;
                                getCellText->Text.Length = (node->LocalAddressStringLength = (ipv6AddressStringLength - 1)) * sizeof(WCHAR);
                            }
                            else
                            {
                                PhInitializeEmptyStringRef(&getCellText->Text);
                                node->LocalAddressStringLength = 0;
                            }
                        }
                        break;
                    }
                }
                break;
            case FW_COLUMN_LOCALPORT:
                {
                    if (node->LocalEndpoint.Port)
                    {
                        SIZE_T returnLength;
                        PH_FORMAT format[1];

                        PhInitFormatU(&format[0], node->LocalEndpoint.Port);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), node->LocalPortString, sizeof(node->LocalPortString), &returnLength))
                        {
                            getCellText->Text.Buffer = node->LocalPortString;
                            getCellText->Text.Length = (node->LocalPortStringLength = (ULONG)(returnLength - sizeof(UNICODE_NULL)));
                        }
                        else
                        {
                            PhInitializeEmptyStringRef(&getCellText->Text);
                            node->LocalPortStringLength = 0;
                        }
                    }
                }
                break;
            case FW_COLUMN_LOCALHOSTNAME:
                {
                    if (node->LocalHostnameResolved)
                    {
                        // Exclude empty hostnames from drawing (dmex)
                        if (!PhIsNullOrEmptyString(node->LocalHostnameString))
                            getCellText->Text = node->LocalHostnameString->sr;
                        else
                            PhInitializeEmptyStringRef(&getCellText->Text);
                    }
                    else
                    {
                        PhInitializeStringRef(&getCellText->Text, L"Resolving....");
                    }
                }
                break;
            case FW_COLUMN_REMOTEADDRESS:
                {
                    switch (node->RemoteEndpoint.Address.Type)
                    {
                    case PH_IPV4_NETWORK_TYPE:
                        {
                            ULONG ipv4AddressStringLength = RTL_NUMBER_OF(node->RemoteAddressString);

                            if (NT_SUCCESS(RtlIpv4AddressToStringEx((PIN_ADDR)&node->RemoteEndpoint.Address.Ipv4, 0, node->RemoteAddressString, &ipv4AddressStringLength)))
                            {
                                getCellText->Text.Buffer = node->RemoteAddressString;
                                getCellText->Text.Length = (node->RemoteAddressStringLength = (ipv4AddressStringLength - 1)) * sizeof(WCHAR);
                            }
                            else
                            {
                                PhInitializeEmptyStringRef(&getCellText->Text);
                                node->RemoteAddressStringLength = 0;
                            }
                        }
                        break;
                    case PH_IPV6_NETWORK_TYPE:
                        {
                            ULONG ipv6AddressStringLength = RTL_NUMBER_OF(node->RemoteAddressString);

                            if (NT_SUCCESS(RtlIpv6AddressToStringEx((PIN6_ADDR)&node->RemoteEndpoint.Address.Ipv6, node->ScopeId, 0, node->RemoteAddressString, &ipv6AddressStringLength)))
                            {
                                getCellText->Text.Buffer = node->RemoteAddressString;
                                getCellText->Text.Length = (node->RemoteAddressStringLength = (ipv6AddressStringLength - 1)) * sizeof(WCHAR);
                            }
                            else
                            {
                                PhInitializeEmptyStringRef(&getCellText->Text);
                                node->RemoteAddressStringLength = 0;
                            }
                        }
                        break;
                    }
                }
                break;
            case FW_COLUMN_REMOTEPORT:
                {
                    if (node->RemoteEndpoint.Port)
                    {
                        SIZE_T returnLength;
                        PH_FORMAT format[1];

                        PhInitFormatU(&format[0], node->RemoteEndpoint.Port);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), node->RemotePortString, sizeof(node->RemotePortString), &returnLength))
                        {
                            getCellText->Text.Buffer = node->RemotePortString;
                            getCellText->Text.Length = (node->RemotePortStringLength = (ULONG)(returnLength - sizeof(UNICODE_NULL)));
                        }
                        else
                        {
                            PhInitializeEmptyStringRef(&getCellText->Text);
                            node->RemotePortStringLength = 0;
                        }
                    }
                }
                break;
            case FW_COLUMN_REMOTEHOSTNAME:
                {
                    if (node->RemoteHostnameResolved)
                    {
                        // Exclude empty hostnames from drawing (dmex)
                        if (!PhIsNullOrEmptyString(node->RemoteHostnameString))
                            getCellText->Text = PhGetStringRef(node->RemoteHostnameString);
                        else
                            PhInitializeEmptyStringRef(&getCellText->Text);
                    }
                    else
                    {
                        PhInitializeStringRef(&getCellText->Text, L"Resolving....");
                    }
                }
                break;
            case FW_COLUMN_PROTOCOL:
                {
                    switch (node->IpProtocol)
                    {
                    case IPPROTO_HOPOPTS:
                        PhInitializeStringRef(&getCellText->Text, L"HOPOPTS");
                        break;
                    case IPPROTO_ICMP:
                        PhInitializeStringRef(&getCellText->Text, L"ICMP");
                        break;
                    case IPPROTO_IGMP:
                        PhInitializeStringRef(&getCellText->Text, L"IGMP");
                        break;
                    case IPPROTO_GGP:
                        PhInitializeStringRef(&getCellText->Text, L"GGP");
                        break;
                    case IPPROTO_IPV4:
                        PhInitializeStringRef(&getCellText->Text, L"IPv4");
                        break;
                    case IPPROTO_ST:
                        PhInitializeStringRef(&getCellText->Text, L"ST");
                        break;
                    case IPPROTO_TCP:
                        PhInitializeStringRef(&getCellText->Text, L"TCP");
                        break;
                    case IPPROTO_CBT:
                        PhInitializeStringRef(&getCellText->Text, L"CBT");
                        break;
                    case IPPROTO_EGP:
                        PhInitializeStringRef(&getCellText->Text, L"EGP");
                        break;
                    case IPPROTO_IGP:
                        PhInitializeStringRef(&getCellText->Text, L"IGP");
                        break;
                    case IPPROTO_PUP:
                        PhInitializeStringRef(&getCellText->Text, L"PUP");
                        break;
                    case IPPROTO_UDP:
                        PhInitializeStringRef(&getCellText->Text, L"UDP");
                        break;
                    case IPPROTO_IDP:
                        PhInitializeStringRef(&getCellText->Text, L"IDP");
                        break;
                    case IPPROTO_RDP:
                        PhInitializeStringRef(&getCellText->Text, L"RDP");
                        break;
                    case IPPROTO_IPV6:
                        PhInitializeStringRef(&getCellText->Text, L"IPv6");
                        break;
                    case IPPROTO_ROUTING:
                        PhInitializeStringRef(&getCellText->Text, L"ROUTING");
                        break;
                    case IPPROTO_FRAGMENT:
                        PhInitializeStringRef(&getCellText->Text, L"FRAGMENT");
                        break;
                    case IPPROTO_ESP:
                        PhInitializeStringRef(&getCellText->Text, L"ESP");
                        break;
                    case IPPROTO_AH:
                        PhInitializeStringRef(&getCellText->Text, L"AH");
                        break;
                    case IPPROTO_ICMPV6:
                        PhInitializeStringRef(&getCellText->Text, L"ICMPv6");
                        break;
                    case IPPROTO_DSTOPTS:
                        PhInitializeStringRef(&getCellText->Text, L"DSTOPTS");
                        break;
                    case IPPROTO_ND:
                        PhInitializeStringRef(&getCellText->Text, L"ND");
                        break;
                    case IPPROTO_ICLFXBM:
                        PhInitializeStringRef(&getCellText->Text, L"ICLFXBM");
                        break;
                    case IPPROTO_PIM:
                        PhInitializeStringRef(&getCellText->Text, L"PIM");
                        break;
                    case IPPROTO_PGM:
                        PhInitializeStringRef(&getCellText->Text, L"PGM");
                        break;
                    case IPPROTO_L2TP:
                        PhInitializeStringRef(&getCellText->Text, L"L2TP");
                        break;
                    case IPPROTO_SCTP:
                        PhInitializeStringRef(&getCellText->Text, L"SCTP");
                        break;
                    case IPPROTO_RESERVED_IPSEC:
                        PhInitializeStringRef(&getCellText->Text, L"IPSEC");
                        break;
                    case IPPROTO_RESERVED_IPSECOFFLOAD:
                        PhInitializeStringRef(&getCellText->Text, L"IPSECOFFLOAD");
                        break;
                    case IPPROTO_RESERVED_WNV:
                        PhInitializeStringRef(&getCellText->Text, L"WNV");
                        break;
                    case IPPROTO_RAW:
                    case IPPROTO_RESERVED_RAW:
                        PhInitializeStringRef(&getCellText->Text, L"RAW");
                        break;
                    }
                }
                break;
           case FW_COLUMN_TIMESTAMP:
               {
                   FwUpdateNodeTimeStamp(node);
                   getCellText->Text = PhGetStringRef(node->TimeString);
               }
               break;
           case FW_COLUMN_PROCESSFILENAME:
               {
                   getCellText->Text = PhGetStringRef(node->ProcessFileNameWin32);
               }
               break;
           case FW_COLUMN_USER:
               {
                   FwUpdateNodeUserSid(node);
                   getCellText->Text = PhGetStringRef(node->UserName);
               }
               break;
           //case FW_COLUMN_PACKAGE:
           //    {
           //        if (node->PackageSid)
           //        {
           //            PhMoveReference(&node->PackageName, EtFwGetSidFullNameCachedSlow(node->PackageSid));
           //            getCellText->Text = PhGetStringRef(node->PackageName);
           //        }
           //    }
           //    break;
           case FW_COLUMN_COUNTRY:
               {
                   getCellText->Text = PhGetStringRef(node->RemoteCountryName);
               }
               break;
           case FW_COLUMN_LOCALADDRESSCLASS:
               {
                   PH_STRINGREF string;

                   if (EtFwLookupAddressClass(&node->LocalEndpoint.Address, &string))
                   {
                       getCellText->Text.Buffer = string.Buffer;
                       getCellText->Text.Length = string.Length;
                   }
                   else
                   {
                       PhInitializeEmptyStringRef(&getCellText->Text);
                   }
               }
               break;
           case FW_COLUMN_REMOTEADDRESSCLASS:
               {
                   PH_STRINGREF string;

                   if (EtFwLookupAddressClass(&node->RemoteEndpoint.Address, &string))
                   {
                       getCellText->Text.Buffer = string.Buffer;
                       getCellText->Text.Length = string.Length;
                   }
                   else
                   {
                       PhInitializeEmptyStringRef(&getCellText->Text);
                   }
               }
               break;
           case FW_COLUMN_LOCALADDRESSSSCOPE:
               {
                   PH_STRINGREF string;

                   if (EtFwLookupAddressScope(&node->LocalEndpoint.Address, &string))
                   {
                       getCellText->Text.Buffer = string.Buffer;
                       getCellText->Text.Length = string.Length;
                   }
                   else
                   {
                       PhInitializeEmptyStringRef(&getCellText->Text);
                   }
               }
               break;
           case FW_COLUMN_REMOTEADDRESSSCOPE:
               {
                   PH_STRINGREF string;

                   if (EtFwLookupAddressScope(&node->RemoteEndpoint.Address, &string))
                   {
                       getCellText->Text.Buffer = string.Buffer;
                       getCellText->Text.Length = string.Length;
                   }
                   else
                   {
                       PhInitializeEmptyStringRef(&getCellText->Text);
                   }
               }
               break;
           case FW_COLUMN_ORIGINALNAME:
               {
                   getCellText->Text = PhGetStringRef(node->ProcessFileName);
               }
               break;
           case FW_COLUMN_LOCALSERVICENAME:
               {
                   FwUpdateNodeLocalPortServiceName(node);

                   if (node->LocalPortServiceName && node->LocalPortServiceName->Length)
                   {
                       getCellText->Text.Buffer = node->LocalPortServiceName->Buffer;
                       getCellText->Text.Length = node->LocalPortServiceName->Length;
                   }
                   else
                   {
                       PhInitializeEmptyStringRef(&getCellText->Text);
                   }
               }
               break;
           case FW_COLUMN_REMOTESERVICENAME:
               {
                   FwUpdateNodeRemotePortServiceName(node);

                   if (node->RemotePortServiceName && node->RemotePortServiceName->Length)
                   {
                       getCellText->Text.Buffer = node->RemotePortServiceName->Buffer;
                       getCellText->Text.Length = node->RemotePortServiceName->Length;
                   }
                   else
                   {
                       PhInitializeEmptyStringRef(&getCellText->Text);
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
            PPH_TREENEW_GET_NODE_ICON getNodeIcon = (PPH_TREENEW_GET_NODE_ICON)Parameter1;
            node = (PFW_EVENT_ITEM)getNodeIcon->Node;

            getNodeIcon->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = (PPH_TREENEW_GET_NODE_COLOR)Parameter1;
            node = (PFW_EVENT_ITEM)getNodeColor->Node;

            switch (node->Type)
            {
            case FWPM_NET_EVENT_TYPE_CLASSIFY_DROP:
            case FWPM_NET_EVENT_TYPE_CAPABILITY_DROP:
                getNodeColor->ForeColor = RGB(0xff, 0x0, 0x0);
                break;
            case FWPM_NET_EVENT_TYPE_CLASSIFY_ALLOW:
            case FWPM_NET_EVENT_TYPE_CAPABILITY_ALLOW:
                getNodeColor->ForeColor = RGB(0x0, 0x0, 0x0);
                break;
            }

            getNodeColor->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            PPH_TREENEW_SORT_CHANGED_EVENT sorting = Parameter1;

            FwTreeNewSortColumn = sorting->SortColumn;
            FwTreeNewSortOrder = sorting->SortOrder;

            TreeNew_NodesStructured(WindowHandle);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    EtFwHandleFwCommand(WindowHandle, ID_DISK_COPY);
                break;
            }
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = WindowHandle;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = FW_COLUMN_TIMESTAMP;
            data.DefaultSortOrder = NoSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            data.Selection = PhShowEMenu(
                data.Menu,
                WindowHandle,
                PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP,
                data.MouseEvent->ScreenLocation.x,
                data.MouseEvent->ScreenLocation.y
                );

            if (data.Selection)
            {
                if (data.Selection->Id == PH_TN_COLUMN_MENU_HIDE_COLUMN_ID ||
                    data.Selection->Id == PH_TN_COLUMN_MENU_CHOOSE_COLUMNS_ID)
                {
                    LoadSettingsFwTreeUpdateMask();
                }
            }

            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            EtFwHandleFwCommand(WindowHandle, ID_DISK_INSPECT);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            ShowFwContextMenu(WindowHandle, contextMenuEvent);
        }
        return TRUE;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = Parameter1;
            HDC hdc;
            RECT rect;

            hdc = customDraw->Dc;
            rect = customDraw->CellRect;
            node = (PFW_EVENT_ITEM)customDraw->Node;

            if (customDraw->Column->Id == FW_COLUMN_COUNTRY)
            {
                if (node->RemoteCountryName)
                {
                    if (node->CountryIconIndex != INT_ERROR)
                    {
                        rect.left += FwTreeLeftMarginPadding;
                        EtFwDrawCountryIcon(hdc, rect, node->CountryIconIndex);
                        rect.left += FwTreeRightMarginPadding;
                    }

                    DrawText(
                        hdc,
                        node->RemoteCountryName->Buffer,
                        (INT)node->RemoteCountryName->Length / sizeof(WCHAR),
                        &rect,
                        DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE
                        );
                }

                return TRUE;
            }

            if (customDraw->Column->Id != FW_COLUMN_NAME)
                break;

            if (rect.right - rect.left <= 1)
            {
                // nothing to draw
                break;
            }

            // Padding
            rect.left += FwTreeLeftMarginPadding;

            PhImageListDrawIcon(
                PhGetProcessSmallImageList(),
                (ULONG)(ULONG_PTR)node->ProcessIconIndex, // HACK (dmex)
                hdc,
                rect.left,
                rect.top + ((rect.bottom - rect.top) - FwTreeIconHeightPadding) / 2,
                ILD_NORMAL | ILD_TRANSPARENT,
                FALSE
                );

            // Padding
            rect.left += FwTreeRightMarginPadding;

            if (!PhIsNullOrEmptyString(node->ProcessBaseString))
            {
                DrawText(
                    hdc,
                    node->ProcessBaseString->Buffer,
                    (UINT)node->ProcessBaseString->Length / sizeof(WCHAR),
                    &rect,
                    DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE
                    );
            }
        }
        return TRUE;
    }

    return FALSE;
}

PFW_EVENT_ITEM EtFwGetSelectedFwItem(
    VOID
    )
{
    PFW_EVENT_ITEM FwItem = NULL;

    for (ULONG i = 0; i < FwNodeList->Count; i++)
    {
        PFW_EVENT_ITEM node = FwNodeList->Items[i];

        if (node->Node.Selected)
        {
            FwItem = node;
            break;
        }
    }

    return FwItem;
}

_Success_(return)
BOOLEAN EtFwGetSelectedFwItems(
    _Out_ PFW_EVENT_ITEM **FwItems,
    _Out_ PULONG NumberOfFwItems
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < FwNodeList->Count; i++)
    {
        PFW_EVENT_ITEM node = FwNodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node);
        }
    }

    if (list->Count)
    {
        *FwItems = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
        *NumberOfFwItems = list->Count;

        PhDereferenceObject(list);
        return TRUE;
    }

    return FALSE;
}

VOID EtFwDeselectAllFwNodes(
    VOID
    )
{
    TreeNew_DeselectRange(FwTreeNewHandle, 0, -1);
}

VOID EtFwSelectAndEnsureVisibleFwNode(
    _In_ PFW_EVENT_ITEM FwNode
    )
{
    EtFwDeselectAllFwNodes();

    if (!FwNode->Node.Visible)
        return;

    TreeNew_FocusMarkSelectNode(FwTreeNewHandle, &FwNode->Node);
}

VOID EtFwInvalidateAllFwNodes(
    VOID
    )
{
    for (ULONG i = 0; i < FwNodeList->Count; i++)
    {
        PFW_EVENT_ITEM node = FwNodeList->Items[i];

        memset(node->TextCache, 0, sizeof(PH_STRINGREF) * (FW_COLUMN_MAXIMUM - 2));
        PhInvalidateTreeNewNode(&node->Node, TN_CACHE_COLOR);
    }

    InvalidateRect(FwTreeNewHandle, NULL, FALSE);
}

VOID EtFwInvalidateAllFwNodesHostnames(
    VOID
    )
{
    EtFwFlushResolveCache();

    for (ULONG i = 0; i < FwNodeList->Count; i++)
    {
        PFW_EVENT_ITEM node = FwNodeList->Items[i];

        memset(node->TextCache, 0, sizeof(PH_STRINGREF) * (FW_COLUMN_MAXIMUM - 2));
        PhInvalidateTreeNewNode(&node->Node, TN_CACHE_COLOR);

        EtFwQueryHostnameForEntry(node);
    }

    //PPH_NETWORK_ITEM* networkItems;
    //ULONG numberOfNetworkItems;
    //
    //PhEnumNetworkItems(&networkItems, &numberOfNetworkItems);
    //
    //for (ULONG j = 0; j < numberOfNetworkItems; j++)
    //{
    //    PPH_NETWORK_ITEM networkItem = networkItems[j];
    //
    //    networkItem->InvalidateHostname = TRUE;
    //}
    //
    //PhDereferenceObjects(networkItems, numberOfNetworkItems);
    //PhFree(networkItems);
}

VOID EtFwCopyFwList(
    VOID
    )
{
    PPH_STRING text;

    text = PhGetTreeNewText(FwTreeNewHandle, 0);
    PhSetClipboardString(FwTreeNewHandle, &text->sr);
    PhDereferenceObject(text);
}

VOID EtFwWriteFwList(
    _In_ PPH_FILE_STREAM FileStream,
    _In_ ULONG Mode
    )
{
    PPH_LIST lines = PhGetGenericTreeNewLines(FwTreeNewHandle, Mode);

    for (ULONG i = 0; i < lines->Count; i++)
    {
        PPH_STRING line;

        line = lines->Items[i];

        PhWriteStringAsUtf8FileStream(FileStream, &line->sr);
        PhDereferenceObject(line);
        PhWriteStringAsUtf8FileStream2(FileStream, L"\r\n");
    }

    PhDereferenceObject(lines);
}

typedef enum _FW_ITEM_COMMAND_ID
{
    FW_ITEM_COMMAND_ID_PING = 1,
    FW_ITEM_COMMAND_ID_TRACERT,
    FW_ITEM_COMMAND_ID_WHOIS,
    FW_ITEM_COMMAND_ID_OPENFILELOCATION,
    FW_ITEM_COMMAND_ID_INSPECT,
    FW_ITEM_COMMAND_ID_COPY,
} FW_ITEM_COMMAND_ID;

VOID EtFwHandleFwCommand(
    _In_ HWND TreeWindowHandle,
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case FW_ITEM_COMMAND_ID_PING:
        {
            PFW_EVENT_ITEM entry;

            if (entry = EtFwGetSelectedFwItem())
            {
                EtFwShowPingWindow(GetParent(TreeWindowHandle), entry->RemoteEndpoint);
            }
        }
        break;
    case FW_ITEM_COMMAND_ID_TRACERT:
        {
            PFW_EVENT_ITEM entry;

            if (entry = EtFwGetSelectedFwItem())
            {
                EtFwShowTracerWindow(GetParent(TreeWindowHandle), entry->RemoteEndpoint);
            }
        }
        break;
    case FW_ITEM_COMMAND_ID_WHOIS:
        {
            PFW_EVENT_ITEM entry;

            if (entry = EtFwGetSelectedFwItem())
            {
                EtFwShowWhoisWindow(GetParent(TreeWindowHandle), entry->RemoteEndpoint);
            }
        }
        break;
    case FW_ITEM_COMMAND_ID_OPENFILELOCATION:
        {
            PFW_EVENT_ITEM entry;

            if (entry = EtFwGetSelectedFwItem())
            {
                if (!PhIsNullOrEmptyString(entry->ProcessFileName))
                {
                    PPH_STRING filenameWin32 = PhGetFileName(entry->ProcessFileName);
                    PhShellExploreFile(TreeWindowHandle, PhGetStringOrEmpty(filenameWin32));
                    PhClearReference(&filenameWin32);
                }
            }
        }
        break;
    case FW_ITEM_COMMAND_ID_INSPECT:
        {
            PFW_EVENT_ITEM entry;

            if (entry = EtFwGetSelectedFwItem())
            {
                if (
                    !PhIsNullOrEmptyString(entry->ProcessFileName) &&
                    PhDoesFileExist(&entry->ProcessFileName->sr)
                    )
                {
                    PhShellExecuteUserString(
                        TreeWindowHandle,
                        L"ProgramInspectExecutables",
                        PhGetString(entry->ProcessFileName),
                        FALSE,
                        L"Make sure the PE Viewer executable file is present."
                        );
                }
            }
        }
        break;
    case FW_ITEM_COMMAND_ID_COPY:
        {
            EtFwCopyFwList();
        }
        break;
    }
}

VOID InitializeFwMenu(
    _In_ PPH_EMENU Menu,
    _In_ PFW_EVENT_ITEM *FwItems,
    _In_ ULONG NumberOfFwItems
    )
{
    if (NumberOfFwItems == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }
    else if (NumberOfFwItems == 1)
    {
        if (PhIsNullOrEmptyString(FwItems[0]->ProcessFileName))
        {
            PhEnableEMenuItem(Menu, ID_DISK_OPENFILELOCATION, FALSE);
            PhEnableEMenuItem(Menu, ID_DISK_INSPECT, FALSE);
        }
        else
        {
            if (!PhDoesFileExist(&FwItems[0]->ProcessFileName->sr))
            {
                PhEnableEMenuItem(Menu, ID_DISK_OPENFILELOCATION, FALSE);
                PhEnableEMenuItem(Menu, ID_DISK_INSPECT, FALSE);
            }
        }
    }
    else
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        PhEnableEMenuItem(Menu, ID_DISK_COPY, TRUE);
    }
}

VOID ShowFwContextMenu(
    _In_ HWND TreeWindowHandle,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenuEvent
    )
{
    PFW_EVENT_ITEM *fwItems;
    ULONG numberOfFwItems;

    if (!EtFwGetSelectedFwItems(&fwItems, &numberOfFwItems))
        return;

    if (numberOfFwItems != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;
        PPH_EMENU_ITEM pingMenu;
        PPH_EMENU_ITEM traceMenu;
        PPH_EMENU_ITEM whoisMenu;

        menu = PhCreateEMenu();
        PhInsertEMenuItem(menu, pingMenu = PhCreateEMenuItem(0, FW_ITEM_COMMAND_ID_PING, L"&Ping", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, traceMenu = PhCreateEMenuItem(0, FW_ITEM_COMMAND_ID_TRACERT, L"&Traceroute", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, whoisMenu = PhCreateEMenuItem(0, FW_ITEM_COMMAND_ID_WHOIS, L"&Whois", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, FW_ITEM_COMMAND_ID_OPENFILELOCATION, L"Open &file location\bEnter", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, FW_ITEM_COMMAND_ID_INSPECT, L"&Inspect", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, FW_ITEM_COMMAND_ID_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
        InitializeFwMenu(menu, fwItems, numberOfFwItems);
        PhInsertCopyCellEMenuItem(menu, FW_ITEM_COMMAND_ID_COPY, TreeWindowHandle, ContextMenuEvent->Column);
        PhSetFlagsEMenuItem(menu, FW_ITEM_COMMAND_ID_OPENFILELOCATION, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        if (PhIsNullIpAddress(&fwItems[0]->RemoteEndpoint.Address))
        {
            PhSetDisabledEMenuItem(pingMenu);
            PhSetDisabledEMenuItem(traceMenu);
            PhSetDisabledEMenuItem(whoisMenu);
        }
        else if (fwItems[0]->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
        {
            if (
                IN4_IS_ADDR_UNSPECIFIED(&fwItems[0]->RemoteEndpoint.Address.InAddr) ||
                IN4_IS_ADDR_LOOPBACK(&fwItems[0]->RemoteEndpoint.Address.InAddr)
                )
            {
                PhSetDisabledEMenuItem(pingMenu);
                PhSetDisabledEMenuItem(traceMenu);
                PhSetDisabledEMenuItem(whoisMenu);
            }

            if (IN4_IS_ADDR_RFC1918(&fwItems[0]->RemoteEndpoint.Address.InAddr))
            {
                PhSetDisabledEMenuItem(whoisMenu);
            }
        }
        else if (fwItems[0]->RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
        {
            if (
                IN6_IS_ADDR_UNSPECIFIED(&fwItems[0]->RemoteEndpoint.Address.In6Addr) ||
                IN6_IS_ADDR_LOOPBACK(&fwItems[0]->RemoteEndpoint.Address.In6Addr)
                )
            {
                PhSetDisabledEMenuItem(pingMenu);
                PhSetDisabledEMenuItem(traceMenu);
                PhSetDisabledEMenuItem(whoisMenu);
            }
        }

        if (item = PhShowEMenu(
            menu,
            TreeWindowHandle,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            ContextMenuEvent->Location.x,
            ContextMenuEvent->Location.y
            ))
        {
            if (!PhHandleCopyCellEMenuItem(item))
                EtFwHandleFwCommand(TreeWindowHandle, item->Id);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(fwItems);
}

VOID NTAPI FwItemAddedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PFW_EVENT_ITEM fwItem = (PFW_EVENT_ITEM)Parameter;

    PhReferenceObject(fwItem);
    PhPushProviderEventQueue(&FwNetworkEventQueue, ProviderAddedEvent, Parameter, FwRunCount);
}

VOID NTAPI FwItemModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PhPushProviderEventQueue(&FwNetworkEventQueue, ProviderModifiedEvent, Parameter, FwRunCount);
}

VOID NTAPI FwItemRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PhPushProviderEventQueue(&FwNetworkEventQueue, ProviderRemovedEvent, Parameter, FwRunCount);
}

VOID NTAPI FwItemsUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    SystemInformer_Invoke(OnFwItemsUpdated, UlongToPtr(FwRunCount));
}

VOID NTAPI OnFwItemsUpdated(
    _In_ ULONG RunId
    )
{
    PPH_PROVIDER_EVENT events;
    ULONG count;
    ULONG i;

    events = PhFlushProviderEventQueue(&FwNetworkEventQueue, RunId, &count);

    if (events)
    {
        TreeNew_SetRedraw(FwTreeNewHandle, FALSE);

        for (i = 0; i < count; i++)
        {
            PH_PROVIDER_EVENT_TYPE type = PH_PROVIDER_EVENT_TYPE(events[i]);
            PFW_EVENT_ITEM fwEventItem = PH_PROVIDER_EVENT_OBJECT(events[i]);

            switch (type)
            {
            case ProviderAddedEvent:
                AddFwNode(fwEventItem);
                PhDereferenceObject(fwEventItem);
                break;
            case ProviderModifiedEvent:
                UpdateFwNode(fwEventItem);
                break;
            case ProviderRemovedEvent:
                RemoveFwNode(fwEventItem);
                break;
            }
        }

        PhFree(events);
    }

    FwTickNodes();

    if (count != 0)
        TreeNew_SetRedraw(FwTreeNewHandle, TRUE);
}

VOID NTAPI FwSearchChangedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (!EtFwEnabled)
        return;

    PhApplyTreeNewFilters(&EtFwFilterSupport);
}

BOOLEAN NTAPI FwSearchFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PFW_EVENT_ITEM fwNode = (PFW_EVENT_ITEM)Node;
    PTOOLSTATUS_WORD_MATCH wordMatch = EtFwToolStatusInterface->WordMatch;

    if (!EtFwToolStatusInterface->GetSearchMatchHandle())
        return TRUE;

    if (fwNode->ProcessBaseString)
    {
        if (wordMatch(&fwNode->ProcessBaseString->sr))
            return TRUE;
    }
    else
    {
        static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"System");

        if (wordMatch(&stringSr))
            return TRUE;
    }

    if (fwNode->ProcessFileName)
    {
        if (wordMatch(&fwNode->ProcessFileName->sr))
            return TRUE;
    }

    if (fwNode->ProcessFileName)
    {
        if (wordMatch(&fwNode->ProcessFileName->sr))
            return TRUE;
    }

    if (fwNode->LocalAddressStringLength)
    {
        PH_STRINGREF localAddressSr;

        localAddressSr.Buffer = fwNode->LocalAddressString;
        localAddressSr.Length = fwNode->LocalAddressStringLength * sizeof(WCHAR);

        if (wordMatch(&localAddressSr))
            return TRUE;
    }

    if (fwNode->LocalHostnameString)
    {
        if (wordMatch(&fwNode->LocalHostnameString->sr))
            return TRUE;
    }

    if (fwNode->LocalPortStringLength)
    {
        PH_STRINGREF localPortSr;

        localPortSr.Buffer = fwNode->LocalPortString;
        localPortSr.Length = fwNode->LocalPortStringLength * sizeof(WCHAR);

        if (wordMatch(&localPortSr))
            return TRUE;
    }

    if (fwNode->RemoteAddressStringLength)
    {
        PH_STRINGREF remoteAddressSr;

        remoteAddressSr.Buffer = fwNode->RemoteAddressString;
        remoteAddressSr.Length = fwNode->RemoteAddressStringLength * sizeof(WCHAR);

        if (wordMatch(&remoteAddressSr))
            return TRUE;
    }

    if (fwNode->RemoteHostnameString)
    {
        if (wordMatch(&fwNode->RemoteHostnameString->sr))
            return TRUE;
    }

    if (fwNode->RemotePortStringLength)
    {
        PH_STRINGREF remotePortSr;

        remotePortSr.Buffer = fwNode->RemotePortString;
        remotePortSr.Length = fwNode->RemotePortStringLength * sizeof(WCHAR);

        if (wordMatch(&remotePortSr))
            return TRUE;
    }

    if (fwNode->RuleName)
    {
        if (wordMatch(&fwNode->RuleName->sr))
            return TRUE;
    }

    if (fwNode->RuleDescription)
    {
        if (wordMatch(&fwNode->RuleDescription->sr))
            return TRUE;
    }

    if (fwNode->ProcessFileNameWin32)
    {
        if (wordMatch(&fwNode->ProcessFileNameWin32->sr))
            return TRUE;
    }

    if (fwNode->UserName)
    {
        if (wordMatch(&fwNode->UserName->sr))
            return TRUE;
    }

    if (fwNode->RemoteCountryName)
    {
        if (wordMatch(&fwNode->RemoteCountryName->sr))
            return TRUE;
    }

    switch (fwNode->Type)
    {
    case FWPM_NET_EVENT_TYPE_CLASSIFY_DROP:
    case FWPM_NET_EVENT_TYPE_CAPABILITY_DROP:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"DROP");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case FWPM_NET_EVENT_TYPE_CLASSIFY_ALLOW:
    case FWPM_NET_EVENT_TYPE_CAPABILITY_ALLOW:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"ALLOW");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case FWPM_NET_EVENT_TYPE_CLASSIFY_DROP_MAC:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"DROP_MAC");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case FWPM_NET_EVENT_TYPE_IPSEC_KERNEL_DROP:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"IPSEC_KERNEL_DROP");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case FWPM_NET_EVENT_TYPE_IPSEC_DOSP_DROP:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"IPSEC_DOSP_DROP");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case FWPM_NET_EVENT_TYPE_IKEEXT_MM_FAILURE:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"IKEEXT_MM_FAILURE");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case FWPM_NET_EVENT_TYPE_IKEEXT_QM_FAILURE:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"QM_FAILURE");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case FWPM_NET_EVENT_TYPE_IKEEXT_EM_FAILURE:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"EM_FAILURE");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    }

    switch (fwNode->IpProtocol)
    {
    case IPPROTO_HOPOPTS:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"HOPOPTS");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_ICMP:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"ICMP");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_IGMP:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"IGMP");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_GGP:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"GGP");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_IPV4:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"IPv4");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_ST:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"ST");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_TCP:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"TCP");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_CBT:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"CBT");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_EGP:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"EGP");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_IGP:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"IGP");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_PUP:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"PUP");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_UDP:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"UDP");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_IDP:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"IDP");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_RDP:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"RDP");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_IPV6:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"IPv6");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_ROUTING:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"ROUTING");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_FRAGMENT:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"FRAGMENT");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_ESP:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"ESP");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_AH:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"AH");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_ICMPV6:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"ICMPv6");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_DSTOPTS:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"DSTOPTS");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_ND:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"ND");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_ICLFXBM:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"ICLFXBM");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_PIM:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"PIM");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_PGM:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"PGM");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_L2TP:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"L2TP");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_SCTP:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"SCTP");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_RESERVED_IPSEC:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"IPSEC");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_RESERVED_IPSECOFFLOAD:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"IPSECOFFLOAD");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_RESERVED_WNV:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"WNV");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    case IPPROTO_RAW:
    case IPPROTO_RESERVED_RAW:
        {
            static PH_STRINGREF stringSr = PH_STRINGREF_INIT(L"RAW");

            if (wordMatch(&stringSr))
                return TRUE;
        }
        break;
    }

    return FALSE;
}

VOID NTAPI FwToolStatusActivateContent(
    _In_ BOOLEAN Select
    )
{
    SetFocus(FwTreeNewHandle);

    if (Select)
    {
        if (TreeNew_GetFlatNodeCount(FwTreeNewHandle) > 0)
            EtFwSelectAndEnsureVisibleFwNode((PFW_EVENT_ITEM)TreeNew_GetFlatNode(FwTreeNewHandle, 0));
    }
}

HWND NTAPI FwToolStatusGetTreeNewHandle(
    VOID
    )
{
    return FwTreeNewHandle;
}
