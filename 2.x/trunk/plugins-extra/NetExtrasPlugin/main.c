/*
 * Process Hacker Extra Plugins -
 *   Network Extras Plugin
 *
 * Copyright (C) 2015 dmex
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

#include "main.h"

static HWND NetworkTreeNewHandle = NULL;
static PPH_PLUGIN PluginInstance = NULL;
static PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
static PH_CALLBACK_REGISTRATION NetworkTreeNewInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION TreeNewMessageCallbackRegistration;

static VOID LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

static VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

static LONG NTAPI NetworkServiceSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PVOID Context
    )
{
    PPH_NETWORK_NODE node1 = Node1;
    PPH_NETWORK_NODE node2 = Node2;
    PNETWORK_EXTENSION extension1 = PhPluginGetObjectExtension(PluginInstance, node1->NetworkItem, EmNetworkItemType);
    PNETWORK_EXTENSION extension2 = PhPluginGetObjectExtension(PluginInstance, node2->NetworkItem, EmNetworkItemType);

    UpdateNetworkNode(SubId, node1, extension1);
    UpdateNetworkNode(SubId, node2, extension2);

    switch (SubId)
    {
    case NETWORK_COLUMN_ID_LOCAL_SERVICE:
         return PhCompareStringWithNull(extension1->LocalServiceName, extension2->LocalServiceName, TRUE);
    case NETWORK_COLUMN_ID_REMOTE_SERVICE:
         return PhCompareStringWithNull(extension1->RemoteServiceName, extension2->RemoteServiceName, TRUE);
    }

    return 0;
}

static VOID NetworkTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PH_TREENEW_COLUMN column;

    *(HWND*)Context = info->TreeNewHandle;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Local Service";
    column.Width = 140;
    column.Alignment = PH_ALIGN_LEFT;
    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, NETWORK_COLUMN_ID_LOCAL_SERVICE, NULL, NetworkServiceSortFunction);
 
    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Remote Service";
    column.Width = 140;
    column.Alignment = PH_ALIGN_LEFT;
    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, NETWORK_COLUMN_ID_REMOTE_SERVICE, NULL, NetworkServiceSortFunction);
}

static VOID NetworkItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    //PPH_NETWORK_ITEM networkItem = Object;
    PNETWORK_EXTENSION extension = Extension;

    memset(extension, 0, sizeof(NETWORK_EXTENSION));
}

static VOID NetworkItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    //PPH_NETWORK_ITEM networkItem = Object;
    PNETWORK_EXTENSION extension = Extension;

    PhSwapReference(&extension->LocalServiceName, NULL);
    PhSwapReference(&extension->RemoteServiceName, NULL);
}

static VOID TreeNewMessageCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_MESSAGE message = Parameter;

    switch (message->Message)
    {
    case TreeNewGetCellText:
        {
            if (message->TreeNewHandle == NetworkTreeNewHandle)
            {
                PPH_TREENEW_GET_CELL_TEXT getCellText = message->Parameter1;
                PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)getCellText->Node;
                PNETWORK_EXTENSION extension = PhPluginGetObjectExtension(PluginInstance, networkNode->NetworkItem, EmNetworkItemType);
                
                UpdateNetworkNode(message->SubId, networkNode, extension);

                switch (message->SubId)
                {
                case NETWORK_COLUMN_ID_LOCAL_SERVICE:
                    getCellText->Text = PhGetStringRef(extension->LocalServiceName);
                    break;
                case NETWORK_COLUMN_ID_REMOTE_SERVICE: 
                    getCellText->Text = PhGetStringRef(extension->RemoteServiceName);
                    break;
                }
            }
        }
        break;
    }
}

LOGICAL DllMain(
    _In_ HINSTANCE Instance,
    _In_ ULONG Reason,
    _Reserved_ PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Network Extras Plugin";
            info->Author = L"dmex";
            info->Description = L"Plugin for extra network information.";
            info->HasOptions = FALSE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackUnload),
                UnloadCallback,
                NULL,
                &PluginUnloadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackNetworkTreeNewInitializing),
                NetworkTreeNewInitializingCallback,
                &NetworkTreeNewHandle,
                &NetworkTreeNewInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackTreeNewMessage),
                TreeNewMessageCallback,
                NULL,
                &TreeNewMessageCallbackRegistration
                );     

            PhPluginSetObjectExtension(
                PluginInstance, 
                EmNetworkItemType, 
                sizeof(NETWORK_EXTENSION),
                NetworkItemCreateCallback,
                NetworkItemDeleteCallback
                );
        }
        break;
    }

    return TRUE;
}