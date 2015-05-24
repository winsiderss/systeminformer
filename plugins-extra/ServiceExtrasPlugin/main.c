/*
 * Process Hacker Extra Plugins -
 *   Service Extras Plugin
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

HWND ServiceTreeNewHandle = NULL;
PPH_PLUGIN PluginInstance = NULL;
_RtlCreateServiceSid RtlCreateServiceSid_I = NULL;
static PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
static PH_CALLBACK_REGISTRATION ServicePropertiesInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION ServiceTreeNewInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION TreeNewMessageCallbackRegistration;

static LONG NTAPI ServiceSidSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PVOID Context
    )
{
    PPH_SERVICE_NODE node1 = Node1;
    PPH_SERVICE_NODE node2 = Node2;
    PSERVICE_EXTENSION extension1 = PhPluginGetObjectExtension(PluginInstance, node1->ServiceItem, EmServiceItemType);
    PSERVICE_EXTENSION extension2 = PhPluginGetObjectExtension(PluginInstance, node2->ServiceItem, EmServiceItemType);

    UpdateServiceSid(node1, extension1);
    UpdateServiceSid(node2, extension2);

    return PhCompareStringWithNull(extension1->ServiceSid, extension2->ServiceSid, TRUE);
}

static VOID LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    RtlCreateServiceSid_I = PhGetProcAddress(L"ntdll.dll", "RtlCreateServiceSid");
}

static VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

static VOID NTAPI ServicePropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_OBJECT_PROPERTIES objectProperties = Parameter;
    PROPSHEETPAGE propSheetPage;

    if (objectProperties->NumberOfPages < objectProperties->MaximumNumberOfPages)
    {
        memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
        propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
        propSheetPage.dwFlags = PSP_USETITLE;
        propSheetPage.hInstance = PluginInstance->DllBase;
        propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_SRVEXTRA);
        propSheetPage.pszTitle = L"Extra";
        propSheetPage.pfnDlgProc = ServiceExtraDlgProc;
        propSheetPage.lParam = (LPARAM)objectProperties->Parameter;
        objectProperties->Pages[objectProperties->NumberOfPages++] = CreatePropertySheetPage(&propSheetPage);
    }
}

static VOID ServiceTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PH_TREENEW_COLUMN column;

    *(HWND*)Context = info->TreeNewHandle;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"SID";
    column.Width = 140;
    column.Alignment = PH_ALIGN_LEFT;

    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, SERVICE_SID_COLUMN_ID, NULL, ServiceSidSortFunction);
}

static VOID ServiceItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    //PPH_SERVICE_ITEM processItem = Object;
    PSERVICE_EXTENSION extension = Extension;

    memset(extension, 0, sizeof(SERVICE_EXTENSION));
}

static VOID ServiceItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    //PPH_SERVICE_ITEM processItem = Object;
    PSERVICE_EXTENSION extension = Extension;

    PhSwapReference(&extension->ServiceSid, NULL);
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
            PPH_TREENEW_GET_CELL_TEXT getCellText = message->Parameter1;

            if (message->TreeNewHandle == ServiceTreeNewHandle)
            {
                PPH_SERVICE_NODE node;

                node = (PPH_SERVICE_NODE)getCellText->Node;

                switch (message->SubId)
                {
                case SERVICE_SID_COLUMN_ID:
                    {
                        PSERVICE_EXTENSION extension;

                        extension = PhPluginGetObjectExtension(PluginInstance, node->ServiceItem, EmServiceItemType);

                        UpdateServiceSid(node, extension);

                        getCellText->Text = PhGetStringRef(extension->ServiceSid);
                    }
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

            PluginInstance = PhRegisterPlugin(SETTING_PREFIX, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Service Extras Plugin";
            info->Author = L"dmex";
            info->Description = L"Plugin for extra services configuration properties.";
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
                PhGetGeneralCallback(GeneralCallbackServicePropertiesInitializing),
                ServicePropertiesInitializingCallback,
                NULL,
                &ServicePropertiesInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackServiceTreeNewInitializing),
                ServiceTreeNewInitializingCallback,
                &ServiceTreeNewHandle,
                &ServiceTreeNewInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackTreeNewMessage),
                TreeNewMessageCallback,
                NULL,
                &TreeNewMessageCallbackRegistration
                );     

            PhPluginSetObjectExtension(
                PluginInstance, 
                EmServiceItemType, 
                sizeof(SERVICE_EXTENSION),
                ServiceItemCreateCallback,
                ServiceItemDeleteCallback
                );
        }
        break;
    }

    return TRUE;
}