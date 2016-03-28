/*
 * Process Hacker -
 *   Process properties: Handles page
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
#include <cpysave.h>
#include <emenu.h>
#include <kphuser.h>
#include <settings.h>
#include <procprv.h>
#include <hndlprv.h>
#include <hndllist.h>
#include <actions.h>
#include <phplug.h>
#include <extmgri.h>
#include <hndlmenu.h>
#include <windowsx.h>
#include <procprpp.h>

static PH_STRINGREF EmptyHandlesText = PH_STRINGREF_INIT(L"There are no handles to display.");

static VOID NTAPI HandleAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    // Parameter contains a pointer to the added handle item.
    PhReferenceObject(Parameter);
    PhPushProviderEventQueue(&handlesContext->EventQueue, ProviderAddedEvent, Parameter, PhGetRunIdProvider(&handlesContext->ProviderRegistration));
}

static VOID NTAPI HandleModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    PhPushProviderEventQueue(&handlesContext->EventQueue, ProviderModifiedEvent, Parameter, PhGetRunIdProvider(&handlesContext->ProviderRegistration));
}

static VOID NTAPI HandleRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    PhPushProviderEventQueue(&handlesContext->EventQueue, ProviderRemovedEvent, Parameter, PhGetRunIdProvider(&handlesContext->ProviderRegistration));
}

static VOID NTAPI HandlesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    PostMessage(handlesContext->WindowHandle, WM_PH_HANDLES_UPDATED, PhGetRunIdProvider(&handlesContext->ProviderRegistration), 0);
}

VOID PhpInitializeHandleMenu(
    _In_ PPH_EMENU Menu,
    _In_ HANDLE ProcessId,
    _In_ PPH_HANDLE_ITEM *Handles,
    _In_ ULONG NumberOfHandles,
    _Inout_ PPH_HANDLES_CONTEXT HandlesContext
    )
{
    PPH_EMENU_ITEM item;

    if (NumberOfHandles == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }
    else if (NumberOfHandles == 1)
    {
        PH_HANDLE_ITEM_INFO info;

        info.ProcessId = ProcessId;
        info.Handle = Handles[0]->Handle;
        info.TypeName = Handles[0]->TypeName;
        info.BestObjectName = Handles[0]->BestObjectName;
        PhInsertHandleObjectPropertiesEMenuItems(Menu, ID_HANDLE_COPY, TRUE, &info);
    }
    else
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);

        PhEnableEMenuItem(Menu, ID_HANDLE_CLOSE, TRUE);
        PhEnableEMenuItem(Menu, ID_HANDLE_COPY, TRUE);
    }

    // Remove irrelevant menu items.

    if (!KphIsConnected())
    {
        if (item = PhFindEMenuItem(Menu, 0, NULL, ID_HANDLE_PROTECTED))
            PhDestroyEMenuItem(item);
        if (item = PhFindEMenuItem(Menu, 0, NULL, ID_HANDLE_INHERIT))
            PhDestroyEMenuItem(item);
    }

    // Protected, Inherit
    if (NumberOfHandles == 1 && KphIsConnected())
    {
        HandlesContext->SelectedHandleProtected = FALSE;
        HandlesContext->SelectedHandleInherit = FALSE;

        if (Handles[0]->Attributes & OBJ_PROTECT_CLOSE)
        {
            HandlesContext->SelectedHandleProtected = TRUE;
            PhSetFlagsEMenuItem(Menu, ID_HANDLE_PROTECTED, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
        }

        if (Handles[0]->Attributes & OBJ_INHERIT)
        {
            HandlesContext->SelectedHandleInherit = TRUE;
            PhSetFlagsEMenuItem(Menu, ID_HANDLE_INHERIT, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
        }
    }
}

VOID PhShowHandleContextMenu(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_HANDLES_CONTEXT Context,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PPH_HANDLE_ITEM *handles;
    ULONG numberOfHandles;

    PhGetSelectedHandleItems(&Context->ListContext, &handles, &numberOfHandles);

    if (numberOfHandles != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;
        PH_PLUGIN_MENU_INFORMATION menuInfo;

        menu = PhCreateEMenu();
        PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_HANDLE), 0);
        PhSetFlagsEMenuItem(menu, ID_HANDLE_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        PhpInitializeHandleMenu(menu, ProcessItem->ProcessId, handles, numberOfHandles, Context);
        PhInsertCopyCellEMenuItem(menu, ID_HANDLE_COPY, Context->ListContext.TreeNewHandle, ContextMenu->Column);

        if (PhPluginsEnabled)
        {
            PhPluginInitializeMenuInfo(&menuInfo, menu, hwndDlg, 0);
            menuInfo.u.Handle.ProcessId = ProcessItem->ProcessId;
            menuInfo.u.Handle.Handles = handles;
            menuInfo.u.Handle.NumberOfHandles = numberOfHandles;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackHandleMenuInitializing), &menuInfo);
        }

        item = PhShowEMenu(
            menu,
            hwndDlg,
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
                SendMessage(hwndDlg, WM_COMMAND, item->Id, 0);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(handles);
}

INT_PTR CALLBACK PhpProcessHandlesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_HANDLES_CONTEXT handlesContext;
    HWND tnHandle;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        handlesContext = (PPH_HANDLES_CONTEXT)propPageContext->Context;

        if (handlesContext)
            tnHandle = handlesContext->ListContext.TreeNewHandle;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            handlesContext = propPageContext->Context =
                PhAllocate(PhEmGetObjectSize(EmHandlesContextType, sizeof(PH_HANDLES_CONTEXT)));

            handlesContext->Provider = PhCreateHandleProvider(
                processItem->ProcessId
                );
            PhRegisterProvider(
                &PhSecondaryProviderThread,
                PhHandleProviderUpdate,
                handlesContext->Provider,
                &handlesContext->ProviderRegistration
                );
            PhRegisterCallback(
                &handlesContext->Provider->HandleAddedEvent,
                HandleAddedHandler,
                handlesContext,
                &handlesContext->AddedEventRegistration
                );
            PhRegisterCallback(
                &handlesContext->Provider->HandleModifiedEvent,
                HandleModifiedHandler,
                handlesContext,
                &handlesContext->ModifiedEventRegistration
                );
            PhRegisterCallback(
                &handlesContext->Provider->HandleRemovedEvent,
                HandleRemovedHandler,
                handlesContext,
                &handlesContext->RemovedEventRegistration
                );
            PhRegisterCallback(
                &handlesContext->Provider->UpdatedEvent,
                HandlesUpdatedHandler,
                handlesContext,
                &handlesContext->UpdatedEventRegistration
                );
            handlesContext->WindowHandle = hwndDlg;

            // Initialize the list.
            tnHandle = GetDlgItem(hwndDlg, IDC_LIST);
            BringWindowToTop(tnHandle);
            PhInitializeHandleList(hwndDlg, tnHandle, &handlesContext->ListContext);
            TreeNew_SetEmptyText(tnHandle, &PhpLoadingText, 0);
            PhInitializeProviderEventQueue(&handlesContext->EventQueue, 100);
            handlesContext->LastRunStatus = -1;
            handlesContext->ErrorMessage = NULL;

            PhEmCallObjectOperation(EmHandlesContextType, handlesContext, EmObjectCreate);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = tnHandle;
                treeNewInfo.CmData = &handlesContext->ListContext.Cm;
                treeNewInfo.SystemContext = handlesContext;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackHandleTreeNewInitializing), &treeNewInfo);
            }

            PhLoadSettingsHandleList(&handlesContext->ListContext);

            PhSetOptionsHandleList(&handlesContext->ListContext, !!PhGetIntegerSetting(L"HideUnnamedHandles"));
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_HIDEUNNAMEDHANDLES),
                handlesContext->ListContext.HideUnnamedHandles ? BST_CHECKED : BST_UNCHECKED);

            PhSetEnabledProvider(&handlesContext->ProviderRegistration, TRUE);
            PhBoostProvider(&handlesContext->ProviderRegistration, NULL);
        }
        break;
    case WM_DESTROY:
        {
            PhEmCallObjectOperation(EmHandlesContextType, handlesContext, EmObjectDelete);

            PhUnregisterCallback(
                &handlesContext->Provider->HandleAddedEvent,
                &handlesContext->AddedEventRegistration
                );
            PhUnregisterCallback(
                &handlesContext->Provider->HandleModifiedEvent,
                &handlesContext->ModifiedEventRegistration
                );
            PhUnregisterCallback(
                &handlesContext->Provider->HandleRemovedEvent,
                &handlesContext->RemovedEventRegistration
                );
            PhUnregisterCallback(
                &handlesContext->Provider->UpdatedEvent,
                &handlesContext->UpdatedEventRegistration
                );
            PhUnregisterProvider(&handlesContext->ProviderRegistration);
            PhDereferenceObject(handlesContext->Provider);
            PhDeleteProviderEventQueue(&handlesContext->EventQueue);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = tnHandle;
                treeNewInfo.CmData = &handlesContext->ListContext.Cm;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackHandleTreeNewUninitializing), &treeNewInfo);
            }

            PhSaveSettingsHandleList(&handlesContext->ListContext);
            PhDeleteHandleList(&handlesContext->ListContext);

            PhFree(handlesContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST),
                    dialogItem, PH_ANCHOR_ALL);

                PhDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case ID_SHOWCONTEXTMENU:
                {
                    PhShowHandleContextMenu(hwndDlg, processItem, handlesContext, (PPH_TREENEW_CONTEXT_MENU)lParam);
                }
                break;
            case ID_HANDLE_CLOSE:
                {
                    PPH_HANDLE_ITEM *handles;
                    ULONG numberOfHandles;

                    PhGetSelectedHandleItems(&handlesContext->ListContext, &handles, &numberOfHandles);
                    PhReferenceObjects(handles, numberOfHandles);

                    if (PhUiCloseHandles(hwndDlg, processItem->ProcessId, handles, numberOfHandles, !!lParam))
                        PhDeselectAllHandleNodes(&handlesContext->ListContext);

                    PhDereferenceObjects(handles, numberOfHandles);
                    PhFree(handles);
                }
                break;
            case ID_HANDLE_PROTECTED:
            case ID_HANDLE_INHERIT:
                {
                    PPH_HANDLE_ITEM handleItem = PhGetSelectedHandleItem(&handlesContext->ListContext);

                    if (handleItem)
                    {
                        ULONG attributes = 0;

                        // Re-create the attributes.

                        if (handlesContext->SelectedHandleProtected)
                            attributes |= OBJ_PROTECT_CLOSE;
                        if (handlesContext->SelectedHandleInherit)
                            attributes |= OBJ_INHERIT;

                        // Toggle the appropriate bit.

                        if (id == ID_HANDLE_PROTECTED)
                            attributes ^= OBJ_PROTECT_CLOSE;
                        else if (id == ID_HANDLE_INHERIT)
                            attributes ^= OBJ_INHERIT;

                        PhReferenceObject(handleItem);
                        PhUiSetAttributesHandle(hwndDlg, processItem->ProcessId, handleItem, attributes);
                        PhDereferenceObject(handleItem);
                    }
                }
                break;
            case ID_HANDLE_OBJECTPROPERTIES1:
            case ID_HANDLE_OBJECTPROPERTIES2:
                {
                    PPH_HANDLE_ITEM handleItem = PhGetSelectedHandleItem(&handlesContext->ListContext);

                    if (handleItem)
                    {
                        PH_HANDLE_ITEM_INFO info;

                        info.ProcessId = processItem->ProcessId;
                        info.Handle = handleItem->Handle;
                        info.TypeName = handleItem->TypeName;
                        info.BestObjectName = handleItem->BestObjectName;

                        if (id == ID_HANDLE_OBJECTPROPERTIES1)
                            PhShowHandleObjectProperties1(hwndDlg, &info);
                        else
                            PhShowHandleObjectProperties2(hwndDlg, &info);
                    }
                }
                break;
            case ID_HANDLE_PROPERTIES:
                {
                    PPH_HANDLE_ITEM handleItem = PhGetSelectedHandleItem(&handlesContext->ListContext);

                    if (handleItem)
                    {
                        PhReferenceObject(handleItem);
                        PhShowHandleProperties(hwndDlg, processItem->ProcessId, handleItem);
                        PhDereferenceObject(handleItem);
                    }
                }
                break;
            case ID_HANDLE_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(tnHandle, 0);
                    PhSetClipboardString(tnHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            case IDC_HIDEUNNAMEDHANDLES:
                {
                    BOOLEAN hide;

                    hide = Button_GetCheck(GetDlgItem(hwndDlg, IDC_HIDEUNNAMEDHANDLES)) == BST_CHECKED;
                    PhSetOptionsHandleList(&handlesContext->ListContext, hide);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_SETACTIVE:
                PhSetEnabledProvider(&handlesContext->ProviderRegistration, TRUE);
                break;
            case PSN_KILLACTIVE:
                PhSetEnabledProvider(&handlesContext->ProviderRegistration, FALSE);
                break;
            }
        }
        break;
    case WM_PH_HANDLES_UPDATED:
        {
            ULONG upToRunId = (ULONG)wParam;
            PPH_PROVIDER_EVENT events;
            ULONG count;
            ULONG i;

            events = PhFlushProviderEventQueue(&handlesContext->EventQueue, upToRunId, &count);

            if (events)
            {
                TreeNew_SetRedraw(tnHandle, FALSE);

                for (i = 0; i < count; i++)
                {
                    PH_PROVIDER_EVENT_TYPE type = PH_PROVIDER_EVENT_TYPE(events[i]);
                    PPH_HANDLE_ITEM handleItem = PH_PROVIDER_EVENT_OBJECT(events[i]);

                    switch (type)
                    {
                    case ProviderAddedEvent:
                        PhAddHandleNode(&handlesContext->ListContext, handleItem, events[i].RunId);
                        PhDereferenceObject(handleItem);
                        break;
                    case ProviderModifiedEvent:
                        PhUpdateHandleNode(&handlesContext->ListContext, PhFindHandleNode(&handlesContext->ListContext, handleItem->Handle));
                        break;
                    case ProviderRemovedEvent:
                        PhRemoveHandleNode(&handlesContext->ListContext, PhFindHandleNode(&handlesContext->ListContext, handleItem->Handle));
                        break;
                    }
                }

                PhFree(events);
            }

            PhTickHandleNodes(&handlesContext->ListContext);

            if (handlesContext->LastRunStatus != handlesContext->Provider->RunStatus)
            {
                NTSTATUS status;
                PPH_STRING message;

                status = handlesContext->Provider->RunStatus;
                handlesContext->LastRunStatus = status;

                if (!PH_IS_REAL_PROCESS_ID(processItem->ProcessId))
                    status = STATUS_SUCCESS;

                if (NT_SUCCESS(status))
                {
                    TreeNew_SetEmptyText(tnHandle, &EmptyHandlesText, 0);
                }
                else
                {
                    message = PhGetStatusMessage(status, 0);
                    PhMoveReference(&handlesContext->ErrorMessage, PhFormatString(L"Unable to query handle information:\n%s", PhGetStringOrDefault(message, L"Unknown error.")));
                    PhClearReference(&message);
                    TreeNew_SetEmptyText(tnHandle, &handlesContext->ErrorMessage->sr, 0);
                }

                InvalidateRect(tnHandle, NULL, FALSE);
            }

            if (count != 0)
                TreeNew_SetRedraw(tnHandle, TRUE);
        }
        break;
    }

    return FALSE;
}
