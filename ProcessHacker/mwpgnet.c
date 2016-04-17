/*
 * Process Hacker -
 *   Main window: Network tab
 *
 * Copyright (C) 2009-2016 wj32
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

#include <phapp.h>
#include <mainwnd.h>

#include <iphlpapi.h>

#include <cpysave.h>
#include <emenu.h>

#include <netlist.h>
#include <netprv.h>
#include <phplug.h>
#include <proctree.h>
#include <settings.h>

#include <mainwndp.h>

PPH_MAIN_TAB_PAGE PhMwpNetworkPage;
HWND PhMwpNetworkTreeNewHandle;

static PH_CALLBACK_REGISTRATION NetworkItemAddedRegistration;
static PH_CALLBACK_REGISTRATION NetworkItemModifiedRegistration;
static PH_CALLBACK_REGISTRATION NetworkItemRemovedRegistration;
static PH_CALLBACK_REGISTRATION NetworkItemsUpdatedRegistration;

static BOOLEAN NetworkFirstTime = TRUE;
static BOOLEAN NetworkTreeListLoaded = FALSE;
static BOOLEAN NetworkNeedsRedraw = FALSE;

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

            PhRegisterCallback(
                &PhNetworkItemAddedEvent,
                PhMwpNetworkItemAddedHandler,
                NULL,
                &NetworkItemAddedRegistration
                );
            PhRegisterCallback(
                &PhNetworkItemModifiedEvent,
                PhMwpNetworkItemModifiedHandler,
                NULL,
                &NetworkItemModifiedRegistration
                );
            PhRegisterCallback(
                &PhNetworkItemRemovedEvent,
                PhMwpNetworkItemRemovedHandler,
                NULL,
                &NetworkItemRemovedRegistration
                );
            PhRegisterCallback(
                &PhNetworkItemsUpdatedEvent,
                PhMwpNetworkItemsUpdatedHandler,
                NULL,
                &NetworkItemsUpdatedRegistration
                );
        }
        break;
    case MainTabPageCreateWindow:
        {
            *(HWND *)Parameter1 = PhMwpNetworkTreeNewHandle;
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
    case MainTabPageLoadSettings:
        {
            // Nothing
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

            PhWriteNetworkList(exportContent->FileStream, exportContent->Mode);
        }
        return TRUE;
    case MainTabPageFontChanged:
        {
            HFONT font = (HFONT)Parameter1;

            SendMessage(PhMwpNetworkTreeNewHandle, WM_SETFONT, (WPARAM)font, TRUE);
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

BOOLEAN PhMwpCurrentUserNetworkTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)Node;
    PPH_PROCESS_NODE processNode;

    processNode = PhFindProcessNode(networkNode->NetworkItem->ProcessId);

    if (processNode)
        return PhMwpCurrentUserProcessTreeFilter(&processNode->Node, NULL);

    return TRUE;
}

BOOLEAN PhMwpSignedNetworkTreeFilter(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)Node;
    PPH_PROCESS_NODE processNode;

    processNode = PhFindProcessNode(networkNode->NetworkItem->ProcessId);

    if (processNode)
        return PhMwpSignedProcessTreeFilter(&processNode->Node, NULL);

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

    if (WindowsVersion >= WINDOWS_VISTA)
    {
        if (item = PhFindEMenuItem(Menu, 0, NULL, ID_NETWORK_VIEWSTACK))
            PhDestroyEMenuItem(item);
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
        PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_NETWORK), 0);
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

VOID PhMwpOnNetworkItemAdded(
    _In_ ULONG RunId,
    _In_ _Assume_refs_(1) PPH_NETWORK_ITEM NetworkItem
    )
{
    PPH_NETWORK_NODE networkNode;

    if (!NetworkNeedsRedraw)
    {
        TreeNew_SetRedraw(PhMwpNetworkTreeNewHandle, FALSE);
        NetworkNeedsRedraw = TRUE;
    }

    networkNode = PhAddNetworkNode(NetworkItem, RunId);
    PhDereferenceObject(NetworkItem);
}

VOID PhMwpOnNetworkItemModified(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    PhUpdateNetworkNode(PhFindNetworkNode(NetworkItem));
}

VOID PhMwpOnNetworkItemRemoved(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    if (!NetworkNeedsRedraw)
    {
        TreeNew_SetRedraw(PhMwpNetworkTreeNewHandle, FALSE);
        NetworkNeedsRedraw = TRUE;
    }

    PhRemoveNetworkNode(PhFindNetworkNode(NetworkItem));
}

VOID PhMwpOnNetworkItemsUpdated(
    VOID
    )
{
    PhTickNetworkNodes();

    if (NetworkNeedsRedraw)
    {
        TreeNew_SetRedraw(PhMwpNetworkTreeNewHandle, TRUE);
        NetworkNeedsRedraw = FALSE;
    }
}
