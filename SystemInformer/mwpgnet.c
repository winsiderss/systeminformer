/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <mainwnd.h>

#include <emenu.h>

#include <netlist.h>
#include <netprv.h>
#include <settings.h>

#include <mainwndp.h>

PPH_MAIN_TAB_PAGE PhMwpNetworkPage;
HWND PhMwpNetworkTreeNewHandle;
PH_PROVIDER_EVENT_QUEUE PhMwpNetworkEventQueue;

static PH_CALLBACK_REGISTRATION NetworkItemAddedRegistration;
static PH_CALLBACK_REGISTRATION NetworkItemModifiedRegistration;
static PH_CALLBACK_REGISTRATION NetworkItemRemovedRegistration;
static PH_CALLBACK_REGISTRATION NetworkItemsUpdatedRegistration;

static BOOLEAN NetworkFirstTime = TRUE;
static BOOLEAN NetworkTreeListLoaded = FALSE;
static PPH_TN_FILTER_ENTRY NetworkFilterEntry = NULL;

BOOLEAN PhMwpNetworkPageCallback(
    _In_ struct _PH_MAIN_TAB_PAGE *Page,
    _In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case MainTabPageCreate:
        {
            PhMwpNetworkPage = Page;

            PhInitializeProviderEventQueue(&PhMwpNetworkEventQueue, 100);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackNetworkProviderAddedEvent),
                PhMwpNetworkItemAddedHandler,
                NULL,
                &NetworkItemAddedRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackNetworkProviderModifiedEvent),
                PhMwpNetworkItemModifiedHandler,
                NULL,
                &NetworkItemModifiedRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackNetworkProviderRemovedEvent),
                PhMwpNetworkItemRemovedHandler,
                NULL,
                &NetworkItemRemovedRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackNetworkProviderUpdatedEvent),
                PhMwpNetworkItemsUpdatedHandler,
                NULL,
                &NetworkItemsUpdatedRegistration
                );
        }
        break;
    case MainTabPageCreateWindow:
        {
            if (Parameter1)
            {
                *(HWND*)Parameter1 = PhMwpNetworkTreeNewHandle;
            }
        }
        return TRUE;
    case MainTabPageSelected:
        {
            BOOLEAN selected = (BOOLEAN)Parameter1;

            if (selected)
            {
                PhMwpNeedNetworkTreeList();

                PhSetEnabledProvider(&PhMwpNetworkProviderRegistration, PhMwpUpdateAutomatically);

                if (PhMwpUpdateAutomatically || NetworkFirstTime)
                {
                    PhBoostProvider(&PhMwpNetworkProviderRegistration, NULL);
                    NetworkFirstTime = FALSE;
                }
            }
            else
            {
                PhSetEnabledProvider(&PhMwpNetworkProviderRegistration, FALSE);
            }
        }
        break;
    case MainTabPageInitializeSectionMenuItems:
        {
            PPH_MAIN_TAB_PAGE_MENU_INFORMATION menuInfo = Parameter1;
            PPH_EMENU menu;
            ULONG startIndex;
            PPH_EMENU_ITEM menuItem;

            if (!menuInfo)
                break;

            menu = menuInfo->Menu;
            startIndex = menuInfo->StartIndex;

            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_VIEW_HIDEWAITINGCONNECTIONS, L"&Hide waiting connections", NULL, NULL), startIndex);

            if (NetworkFilterEntry && (menuItem = PhFindEMenuItem(menu, 0, NULL, ID_VIEW_HIDEWAITINGCONNECTIONS)))
                menuItem->Flags |= PH_EMENU_CHECKED;
        }
        return TRUE;
    case MainTabPageLoadSettings:
        {
            if (PhGetIntegerSetting(L"HideWaitingConnections"))
                NetworkFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportNetworkTreeList(), PhMwpNetworkTreeFilter, NULL);
        }
        return TRUE;
    case MainTabPageSaveSettings:
        {
            if (NetworkTreeListLoaded)
                PhSaveSettingsNetworkTreeList();
        }
        return TRUE;
    case MainTabPageExportContent:
        {
            PPH_MAIN_TAB_PAGE_EXPORT_CONTENT exportContent = Parameter1;

            if (!exportContent)
                break;

            PhWriteNetworkList(exportContent->FileStream, exportContent->Mode);
        }
        return TRUE;
    case MainTabPageFontChanged:
        {
            HFONT font = (HFONT)Parameter1;

            SetWindowFont(PhMwpNetworkTreeNewHandle, font, TRUE);
        }
        break;
    case MainTabPageUpdateAutomaticallyChanged:
        {
            BOOLEAN updateAutomatically = (BOOLEAN)Parameter1;

            if (PhMwpNetworkPage->Selected)
                PhSetEnabledProvider(&PhMwpNetworkProviderRegistration, updateAutomatically);
        }
        break;
    }

    return FALSE;
}

VOID PhMwpNeedNetworkTreeList(
    VOID
    )
{
    if (!NetworkTreeListLoaded)
    {
        NetworkTreeListLoaded = TRUE;

        PhLoadSettingsNetworkTreeList();
    }
}

VOID PhMwpToggleNetworkWaitingConnectionTreeFilter(
    VOID
    )
{
    if (NetworkFilterEntry)
    {
        PhRemoveTreeNewFilter(PhGetFilterSupportNetworkTreeList(), NetworkFilterEntry);
        NetworkFilterEntry = NULL;
    }
    else
    {
        NetworkFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportNetworkTreeList(), PhMwpNetworkTreeFilter, NULL);
    }

    PhApplyTreeNewFilters(PhGetFilterSupportNetworkTreeList());

    PhSetIntegerSetting(L"HideWaitingConnections", !!NetworkFilterEntry);
}

BOOLEAN PhMwpNetworkTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)Node;

    if (!networkNode->NetworkItem->ProcessId)
        return FALSE;
    if (networkNode->NetworkItem->State == MIB_TCP_STATE_CLOSE_WAIT)
        return FALSE;

    return TRUE;
}

VOID PhMwpInitializeNetworkMenu(
    _In_ PPH_EMENU Menu,
    _In_ PPH_NETWORK_ITEM *NetworkItems,
    _In_ ULONG NumberOfNetworkItems
    )
{
    ULONG i;
    PPH_EMENU_ITEM item;

    if (NumberOfNetworkItems == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }
    else if (NumberOfNetworkItems == 1)
    {
        if (!NetworkItems[0]->ProcessId)
            PhEnableEMenuItem(Menu, ID_NETWORK_GOTOPROCESS, FALSE);
    }
    else
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        PhEnableEMenuItem(Menu, ID_NETWORK_CLOSE, TRUE);
        PhEnableEMenuItem(Menu, ID_NETWORK_COPY, TRUE);
    }

    // Go to Service
    if (NumberOfNetworkItems != 1 || !NetworkItems[0]->OwnerName)
    {
        if (item = PhFindEMenuItem(Menu, 0, NULL, ID_NETWORK_GOTOSERVICE))
            PhDestroyEMenuItem(item);
    }

    // Close
    if (NumberOfNetworkItems != 0)
    {
        BOOLEAN closeOk = TRUE;

        for (i = 0; i < NumberOfNetworkItems; i++)
        {
            if (
                NetworkItems[i]->ProtocolType != PH_TCP4_NETWORK_PROTOCOL ||
                NetworkItems[i]->State != MIB_TCP_STATE_ESTAB
                )
            {
                closeOk = FALSE;
                break;
            }
        }

        if (!closeOk)
            PhEnableEMenuItem(Menu, ID_NETWORK_CLOSE, FALSE);
    }
}

VOID PhShowNetworkContextMenu(
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PH_PLUGIN_MENU_INFORMATION menuInfo;
    PPH_NETWORK_ITEM *networkItems;
    ULONG numberOfNetworkItems;

    PhGetSelectedNetworkItems(&networkItems, &numberOfNetworkItems);

    if (numberOfNetworkItems != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;

        menu = PhCreateEMenu();
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_NETWORK_GOTOPROCESS, L"&Go to process\bEnter", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_NETWORK_GOTOSERVICE, L"Go to service", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_NETWORK_CLOSE, L"C&lose", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_NETWORK_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
        PhSetFlagsEMenuItem(menu, ID_NETWORK_GOTOPROCESS, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);
        PhMwpInitializeNetworkMenu(menu, networkItems, numberOfNetworkItems);
        PhInsertCopyCellEMenuItem(menu, ID_NETWORK_COPY, PhMwpNetworkTreeNewHandle, ContextMenu->Column);

        if (PhPluginsEnabled)
        {
            PhPluginInitializeMenuInfo(&menuInfo, menu, PhMainWndHandle, 0);
            menuInfo.u.Network.NetworkItems = networkItems;
            menuInfo.u.Network.NumberOfNetworkItems = numberOfNetworkItems;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackNetworkMenuInitializing), &menuInfo);
        }

        item = PhShowEMenu(
            menu,
            PhMainWndHandle,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            ContextMenu->Location.x,
            ContextMenu->Location.y
            );

        if (item)
        {
            BOOLEAN handled = FALSE;

            handled = PhHandleCopyCellEMenuItem(item);

            if (!handled && PhPluginsEnabled)
                handled = PhPluginTriggerEMenuItem(&menuInfo, item);

            if (!handled)
                SendMessage(PhMainWndHandle, WM_COMMAND, item->Id, 0);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(networkItems);
}

VOID NTAPI PhMwpNetworkItemAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_NETWORK_ITEM networkItem = (PPH_NETWORK_ITEM)Parameter;

    if (!networkItem)
        return;

    PhReferenceObject(networkItem);
    PhPushProviderEventQueue(&PhMwpNetworkEventQueue, ProviderAddedEvent, Parameter, PhGetRunIdProvider(&PhMwpNetworkProviderRegistration));
}

VOID NTAPI PhMwpNetworkItemModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_NETWORK_ITEM networkItem = (PPH_NETWORK_ITEM)Parameter;

    PhPushProviderEventQueue(&PhMwpNetworkEventQueue, ProviderModifiedEvent, Parameter, PhGetRunIdProvider(&PhMwpNetworkProviderRegistration));
}

VOID NTAPI PhMwpNetworkItemRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_NETWORK_ITEM networkItem = (PPH_NETWORK_ITEM)Parameter;

    PhPushProviderEventQueue(&PhMwpNetworkEventQueue, ProviderRemovedEvent, Parameter, PhGetRunIdProvider(&PhMwpNetworkProviderRegistration));
}

VOID NTAPI PhMwpNetworkItemsUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ProcessHacker_Invoke(PhMwpOnNetworkItemsUpdated, PhGetRunIdProvider(&PhMwpNetworkProviderRegistration));
}

VOID PhMwpOnNetworkItemsUpdated(
    _In_ ULONG RunId
    )
{
    PPH_PROVIDER_EVENT events;
    ULONG count;
    ULONG i;

    events = PhFlushProviderEventQueue(&PhMwpNetworkEventQueue, RunId, &count);

    if (events)
    {
        TreeNew_SetRedraw(PhMwpNetworkTreeNewHandle, FALSE);

        for (i = 0; i < count; i++)
        {
            PH_PROVIDER_EVENT_TYPE type = PH_PROVIDER_EVENT_TYPE(events[i]);
            PPH_NETWORK_ITEM networkItem = PH_PROVIDER_EVENT_OBJECT(events[i]);

            switch (type)
            {
            case ProviderAddedEvent:
                PhAddNetworkNode(networkItem, events[i].RunId);
                PhDereferenceObject(networkItem);
                break;
            case ProviderModifiedEvent:
                PhUpdateNetworkNode(PhFindNetworkNode(networkItem));
                break;
            case ProviderRemovedEvent:
                PhRemoveNetworkNode(PhFindNetworkNode(networkItem));
                break;
            }
        }

        PhFree(events);
    }

    PhTickNetworkNodes();

    if (count != 0)
        TreeNew_SetRedraw(PhMwpNetworkTreeNewHandle, TRUE);
}
