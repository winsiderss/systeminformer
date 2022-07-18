/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2022
 *
 */

#include <phapp.h>
#include <procprp.h>
#include <procprpp.h>

#include <cpysave.h>
#include <emenu.h>
#include <hndlinfo.h>
#include <kphuser.h>
#include <settings.h>

#include <actions.h>
#include <extmgri.h>
#include <hndlmenu.h>
#include <hndlprv.h>
#include <phplug.h>
#include <phsettings.h>
#include <procprv.h>
#include <secedit.h>

static PH_STRINGREF EmptyHandlesText = PH_STRINGREF_INIT(L"There are no handles to display.");

static VOID NTAPI HandleAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    if (!handlesContext)
        return;
    if (!Parameter)
        return;

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

    if (!handlesContext)
        return;

    PhPushProviderEventQueue(&handlesContext->EventQueue, ProviderModifiedEvent, Parameter, PhGetRunIdProvider(&handlesContext->ProviderRegistration));
}

static VOID NTAPI HandleRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    if (!handlesContext)
        return;

    PhPushProviderEventQueue(&handlesContext->EventQueue, ProviderRemovedEvent, Parameter, PhGetRunIdProvider(&handlesContext->ProviderRegistration));
}

static VOID NTAPI HandlesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    if (!handlesContext)
        return;

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
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_HANDLE_CLOSE, L"C&lose\bDel", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_HANDLE_PROTECTED, L"&Protected", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_HANDLE_INHERIT, L"&Inherit", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_HANDLE_SECURITY, L"Secu&rity", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_HANDLE_PROPERTIES, L"Prope&rties\bEnter", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_HANDLE_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
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

BOOLEAN PhpHandleTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = Context;
    PPH_HANDLE_NODE handleNode = (PPH_HANDLE_NODE)Node;
    PPH_HANDLE_ITEM handleItem = handleNode->HandleItem;

    if (handlesContext->ListContext.HideProtectedHandles && handleItem->Attributes & OBJ_PROTECT_CLOSE)
        return FALSE;
    if (handlesContext->ListContext.HideInheritHandles && handleItem->Attributes & OBJ_INHERIT)
        return FALSE;
    if (handlesContext->ListContext.HideUnnamedHandles && PhIsNullOrEmptyString(handleItem->BestObjectName))
        return FALSE;

    if (handlesContext->ListContext.HideEtwHandles)
    {
        static PH_INITONCE initOnce = PH_INITONCE_INIT;
        static ULONG eventTraceTypeIndex = ULONG_MAX;

        if (PhBeginInitOnce(&initOnce))
        {
            static PH_STRINGREF etwTypeName = PH_STRINGREF_INIT(L"EtwRegistration");
            eventTraceTypeIndex = PhGetObjectTypeNumber(&etwTypeName);
            PhEndInitOnce(&initOnce);
        }

        if (handleItem->TypeIndex == eventTraceTypeIndex)
            return FALSE;
    }

    if (PhIsNullOrEmptyString(handlesContext->SearchboxText))
        return TRUE;

    // handle properties

    if (handlesContext->UseSearchPointer && handleItem->Handle == (PVOID)handlesContext->SearchPointer)
        return TRUE;

    if (!PhIsNullOrEmptyString(handleItem->TypeName))
    {
        if (PhWordMatchStringRef(&handlesContext->SearchboxText->sr, &handleItem->TypeName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(handleItem->ObjectName))
    {
        if (PhWordMatchStringRef(&handlesContext->SearchboxText->sr, &handleItem->ObjectName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(handleItem->BestObjectName))
    {
        if (PhWordMatchStringRef(&handlesContext->SearchboxText->sr, &handleItem->BestObjectName->sr))
            return TRUE;
    }

    if (handleItem->HandleString[0])
    {
        if (PhWordMatchStringZ(handlesContext->SearchboxText, handleItem->HandleString))
            return TRUE;
    }

    if (handleItem->GrantedAccessString[0])
    {
        if (PhWordMatchStringZ(handlesContext->SearchboxText, handleItem->GrantedAccessString))
            return TRUE;
    }

    // TODO: Add search for handleItem->Attributes

    // node properties

    if (handleNode->ObjectString[0])
    {
        if (PhWordMatchStringZ(handlesContext->SearchboxText, handleNode->ObjectString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(handleNode->GrantedAccessSymbolicText))
    {
        if (PhWordMatchStringRef(&handlesContext->SearchboxText->sr, &handleNode->GrantedAccessSymbolicText->sr))
            return TRUE;
    }

    if (handleNode->FileShareAccessText[0])
    {
        if (PhWordMatchStringZ(handlesContext->SearchboxText, handleNode->FileShareAccessText))
            return TRUE;
    }

    return FALSE;
}

typedef struct _HANDLE_OPEN_CONTEXT
{
    HANDLE ProcessId;
    PPH_HANDLE_ITEM HandleItem;
} HANDLE_OPEN_CONTEXT, *PHANDLE_OPEN_CONTEXT;

NTSTATUS PhpProcessHandleOpenCallback(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    PHANDLE_OPEN_CONTEXT context = Context;
    NTSTATUS status;
    HANDLE processHandle;

    if (!context)
        return STATUS_UNSUCCESSFUL;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_DUP_HANDLE,
        context->ProcessId
        )))
        return status;

    status = NtDuplicateObject(
        processHandle,
        context->HandleItem->Handle,
        NtCurrentProcess(),
        Handle,
        DesiredAccess,
        0,
        0
        );
    NtClose(processHandle);

    return status;
}

NTSTATUS PhpProcessHandleCloseCallback(
    _In_opt_ PVOID Context
    )
{
    PHANDLE_OPEN_CONTEXT context = Context;

    if (!context)
        return STATUS_UNSUCCESSFUL;

    PhDereferenceObject(context->HandleItem);
    PhFree(context);

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK PhpProcessHandlesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_HANDLES_CONTEXT handlesContext;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, NULL, &propPageContext, &processItem))
    {
        handlesContext = propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            handlesContext = propPageContext->Context = PhAllocate(PhEmGetObjectSize(EmHandlesContextType, sizeof(PH_HANDLES_CONTEXT)));
            memset(handlesContext, 0, sizeof(PH_HANDLES_CONTEXT));

            handlesContext->WindowHandle = hwndDlg;
            handlesContext->SearchWindowHandle = GetDlgItem(hwndDlg, IDC_HANDLESEARCH);
            handlesContext->TreeNewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            handlesContext->Provider = PhCreateHandleProvider(
                processItem->ProcessId
                );
            PhRegisterProvider(
                &PhTertiaryProviderThread,
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
                &handlesContext->Provider->HandleUpdatedEvent,
                HandlesUpdatedHandler,
                handlesContext,
                &handlesContext->UpdatedEventRegistration
                );

            // Initialize the list.
            PhInitializeHandleList(hwndDlg, handlesContext->TreeNewHandle, &handlesContext->ListContext);
            TreeNew_SetEmptyText(handlesContext->TreeNewHandle, &PhpLoadingText, 0);
            PhInitializeProviderEventQueue(&handlesContext->EventQueue, 100);
            handlesContext->LastRunStatus = -1;
            handlesContext->ErrorMessage = NULL;
            handlesContext->SearchboxText = PhReferenceEmptyString();
            handlesContext->FilterEntry = PhAddTreeNewFilter(&handlesContext->ListContext.TreeFilterSupport, PhpHandleTreeFilterCallback, handlesContext);

            PhCreateSearchControl(hwndDlg, handlesContext->SearchWindowHandle, L"Search Handles (Ctrl+K)");

            PhEmCallObjectOperation(EmHandlesContextType, handlesContext, EmObjectCreate);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = handlesContext->TreeNewHandle;
                treeNewInfo.CmData = &handlesContext->ListContext.Cm;
                treeNewInfo.SystemContext = handlesContext;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackHandleTreeNewInitializing), &treeNewInfo);
            }

            PhLoadSettingsHandleList(&handlesContext->ListContext);

            PhSetEnabledProvider(&handlesContext->ProviderRegistration, TRUE);
            PhBoostProvider(&handlesContext->ProviderRegistration, NULL);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveTreeNewFilter(&handlesContext->ListContext.TreeFilterSupport, handlesContext->FilterEntry);
            if (handlesContext->SearchboxText) PhDereferenceObject(handlesContext->SearchboxText);

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
                &handlesContext->Provider->HandleUpdatedEvent,
                &handlesContext->UpdatedEventRegistration
                );
            PhUnregisterProvider(&handlesContext->ProviderRegistration);
            PhDereferenceObject(handlesContext->Provider);
            PhDeleteProviderEventQueue(&handlesContext->EventQueue);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = handlesContext->TreeNewHandle;
                treeNewInfo.CmData = &handlesContext->ListContext.Cm;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackHandleTreeNewUninitializing), &treeNewInfo);
            }

            PhSaveSettingsHandleList(&handlesContext->ListContext);
            PhDeleteHandleList(&handlesContext->ListContext);

            PhFree(handlesContext);
        }
        break;
    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_ITEM dialogItem;

            if (dialogItem = PhBeginPropPageLayout(hwndDlg, propPageContext))
            {
                PhAddPropPageLayoutItem(hwndDlg, handlesContext->SearchWindowHandle, dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, handlesContext->TreeNewHandle, dialogItem, PH_ANCHOR_ALL);
                PhEndPropPageLayout(hwndDlg, propPageContext);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case EN_CHANGE:
                {
                    PPH_STRING newSearchboxText;

                    if (GET_WM_COMMAND_HWND(wParam, lParam) != handlesContext->SearchWindowHandle)
                        break;

                    newSearchboxText = PH_AUTO(PhGetWindowText(handlesContext->SearchWindowHandle));

                    if (!PhEqualString(handlesContext->SearchboxText, newSearchboxText, FALSE))
                    {
                        // Cache the current search text for our callback.
                        PhSwapReference(&handlesContext->SearchboxText, newSearchboxText);
                        // Try to get a search pointer from the search string.
                        handlesContext->UseSearchPointer = PhStringToInteger64(&handlesContext->SearchboxText->sr, 0, &handlesContext->SearchPointer);

                        if (!PhIsNullOrEmptyString(handlesContext->SearchboxText))
                        {
                            // Expand any hidden nodes to make search results visible.
                            PhExpandAllHandleNodes(&handlesContext->ListContext, TRUE);
                        }

                        PhApplyTreeNewFilters(&handlesContext->ListContext.TreeFilterSupport);
                    }
                }
                break;
            }

            switch (GET_WM_COMMAND_ID(wParam, lParam))
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

                        if (GET_WM_COMMAND_ID(wParam, lParam) == ID_HANDLE_PROTECTED)
                            attributes ^= OBJ_PROTECT_CLOSE;
                        else if (GET_WM_COMMAND_ID(wParam, lParam) == ID_HANDLE_INHERIT)
                            attributes ^= OBJ_INHERIT;

                        PhReferenceObject(handleItem);
                        PhUiSetAttributesHandle(hwndDlg, processItem->ProcessId, handleItem, attributes);
                        PhDereferenceObject(handleItem);
                    }
                }
                break;
            case ID_HANDLE_SECURITY:
                {
                    PPH_HANDLE_ITEM handleItem = PhGetSelectedHandleItem(&handlesContext->ListContext);

                    if (handleItem)
                    {
                        PHANDLE_OPEN_CONTEXT context;

                        context = PhAllocateZero(sizeof(HANDLE_OPEN_CONTEXT));
                        context->HandleItem = PhReferenceObject(handleItem);
                        context->ProcessId = processItem->ProcessId;

                        PhEditSecurity(
                            PhCsForceNoParent ? NULL : hwndDlg,
                            PhGetString(handleItem->ObjectName),
                            PhGetString(handleItem->TypeName),
                            PhpProcessHandleOpenCallback,
                            PhpProcessHandleCloseCallback,
                            context
                            );
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

                        if (GET_WM_COMMAND_ID(wParam, lParam) == ID_HANDLE_OBJECTPROPERTIES1)
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

                    text = PhGetTreeNewText(handlesContext->TreeNewHandle, 0);
                    PhSetClipboardString(handlesContext->TreeNewHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            case IDC_OPTIONS:
                {
                    RECT rect;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM protectedMenuItem;
                    PPH_EMENU_ITEM inheritMenuItem;
                    PPH_EMENU_ITEM unnamedMenuItem;
                    PPH_EMENU_ITEM etwMenuItem;
                    PPH_EMENU_ITEM protectedHighlightMenuItem;
                    PPH_EMENU_ITEM inheritHighlightMenuItem;
                    PPH_EMENU_ITEM selectedItem;

                    GetWindowRect(GetDlgItem(hwndDlg, IDC_OPTIONS), &rect);

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, protectedMenuItem = PhCreateEMenuItem(0, PH_HANDLE_TREE_MENUITEM_HIDE_PROTECTED_HANDLES, L"Hide protected handles", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, inheritMenuItem = PhCreateEMenuItem(0, PH_HANDLE_TREE_MENUITEM_HIDE_INHERIT_HANDLES, L"Hide inherit handles", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, unnamedMenuItem = PhCreateEMenuItem(0, PH_HANDLE_TREE_MENUITEM_HIDE_UNNAMED_HANDLES, L"Hide unnamed handles", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, etwMenuItem = PhCreateEMenuItem(0, PH_HANDLE_TREE_MENUITEM_HIDE_ETW_HANDLES, L"Hide etw handles", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, protectedHighlightMenuItem = PhCreateEMenuItem(0, PH_HANDLE_TREE_MENUITEM_HIGHLIGHT_PROTECTED_HANDLES, L"Highlight protected handles", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, inheritHighlightMenuItem = PhCreateEMenuItem(0, PH_HANDLE_TREE_MENUITEM_HIGHLIGHT_INHERIT_HANDLES, L"Highlight inherit handles", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_HANDLE_TREE_MENUITEM_HANDLESTATS, L"Statistics", NULL, NULL), ULONG_MAX);

                    if (handlesContext->ListContext.HideProtectedHandles)
                        protectedMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (handlesContext->ListContext.HideInheritHandles)
                        inheritMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (handlesContext->ListContext.HideUnnamedHandles)
                        unnamedMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (handlesContext->ListContext.HideEtwHandles)
                        etwMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (PhCsUseColorProtectedHandles)
                        protectedHighlightMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (PhCsUseColorInheritHandles)
                        inheritHighlightMenuItem->Flags |= PH_EMENU_CHECKED;

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        rect.left,
                        rect.bottom
                        );

                    if (selectedItem && selectedItem->Id)
                    {
                        if (selectedItem->Id == PH_HANDLE_TREE_MENUITEM_HIGHLIGHT_PROTECTED_HANDLES)
                        {
                            // HACK (dmex)
                            PhSetIntegerSetting(L"UseColorProtectedHandles", !PhCsUseColorProtectedHandles);
                            PhCsUseColorProtectedHandles = !PhCsUseColorProtectedHandles;
                            TreeNew_NodesStructured(handlesContext->TreeNewHandle);
                        }
                        else if (selectedItem->Id == PH_HANDLE_TREE_MENUITEM_HIGHLIGHT_INHERIT_HANDLES)
                        {
                            // HACK (dmex)
                            PhSetIntegerSetting(L"UseColorInheritHandles", !PhCsUseColorInheritHandles);
                            PhCsUseColorInheritHandles = !PhCsUseColorInheritHandles;
                            TreeNew_NodesStructured(handlesContext->TreeNewHandle);
                        }
                        else if (selectedItem->Id == PH_HANDLE_TREE_MENUITEM_HANDLESTATS)
                        {
                            PhShowHandleStatisticsDialog(hwndDlg, handlesContext->Provider->ProcessId);
                        }
                        else
                        {
                            PhSetOptionsHandleList(&handlesContext->ListContext, selectedItem->Id);
                            PhSaveSettingsHandleList(&handlesContext->ListContext);
                            PhApplyTreeNewFilters(&handlesContext->ListContext.TreeFilterSupport);
                        }
                    }

                    PhDestroyEMenu(menu);
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
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)handlesContext->TreeNewHandle);
                return TRUE;
            }
        }
        break;
    case WM_KEYDOWN:
        {
            if (LOWORD(wParam) == 'K')
            {
                if (GetKeyState(VK_CONTROL) < 0)
                {
                    SetFocus(handlesContext->SearchWindowHandle);
                    return TRUE;
                }
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
                TreeNew_SetRedraw(handlesContext->TreeNewHandle, FALSE);

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
                    TreeNew_SetEmptyText(handlesContext->TreeNewHandle, &EmptyHandlesText, 0);
                }
                else
                {
                    message = PhGetStatusMessage(status, 0);
                    PhMoveReference(&handlesContext->ErrorMessage, PhFormatString(L"Unable to query handle information:\n%s", PhGetStringOrDefault(message, L"Unknown error.")));
                    PhClearReference(&message);
                    TreeNew_SetEmptyText(handlesContext->TreeNewHandle, &handlesContext->ErrorMessage->sr, 0);
                }

                InvalidateRect(handlesContext->TreeNewHandle, NULL, FALSE);
            }

            if (count != 0)
            {
                // Refresh the visible nodes.
                PhApplyTreeNewFilters(&handlesContext->ListContext.TreeFilterSupport);

                TreeNew_SetRedraw(handlesContext->TreeNewHandle, TRUE);
            }
        }
        break;
    }

    return FALSE;
}
