/*
 * Process Hacker -
 *   Process properties: Modules page
 *
 * Copyright (C) 2009-2016 wj32
 * Copyright (C) 2017-2019 dmex
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
#include <procprp.h>
#include <procprpp.h>

#include <cpysave.h>
#include <emenu.h>

#include <actions.h>
#include <extmgri.h>
#include <modprv.h>
#include <phplug.h>
#include <phsettings.h>
#include <procprv.h>
#include <settings.h>
#include <verify.h>

static PH_STRINGREF EmptyModulesText = PH_STRINGREF_INIT(L"There are no modules to display.");

static VOID NTAPI ModuleAddedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_MODULES_CONTEXT modulesContext = (PPH_MODULES_CONTEXT)Context;

    if (!modulesContext)
        return;
    if (!Parameter)
        return;

    // Parameter contains a pointer to the added module item.
    PhReferenceObject(Parameter);
    PhPushProviderEventQueue(&modulesContext->EventQueue, ProviderAddedEvent, Parameter, PhGetRunIdProvider(&modulesContext->ProviderRegistration));
}

static VOID NTAPI ModuleModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_MODULES_CONTEXT modulesContext = (PPH_MODULES_CONTEXT)Context;

    if (!modulesContext)
        return;

    PhPushProviderEventQueue(&modulesContext->EventQueue, ProviderModifiedEvent, Parameter, PhGetRunIdProvider(&modulesContext->ProviderRegistration));
}

static VOID NTAPI ModuleRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_MODULES_CONTEXT modulesContext = (PPH_MODULES_CONTEXT)Context;

    if (!modulesContext)
        return;

    PhPushProviderEventQueue(&modulesContext->EventQueue, ProviderRemovedEvent, Parameter, PhGetRunIdProvider(&modulesContext->ProviderRegistration));
}

static VOID NTAPI ModulesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_MODULES_CONTEXT modulesContext = (PPH_MODULES_CONTEXT)Context;

    if (!modulesContext)
        return;

    PostMessage(modulesContext->WindowHandle, WM_PH_MODULES_UPDATED, PhGetRunIdProvider(&modulesContext->ProviderRegistration), 0);
}

VOID PhpInitializeModuleMenu(
    _In_ PPH_EMENU Menu,
    _In_ HANDLE ProcessId,
    _In_ PPH_MODULE_ITEM *Modules,
    _In_ ULONG NumberOfModules
    )
{
    if (NumberOfModules == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }
    else if (NumberOfModules == 1)
    {
        // Nothing
    }
    else
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        PhEnableEMenuItem(Menu, ID_MODULE_COPY, TRUE);
    }
}

VOID PhShowModuleContextMenu(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_MODULES_CONTEXT Context,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PPH_MODULE_ITEM *modules;
    ULONG numberOfModules;

    PhGetSelectedModuleItems(&Context->ListContext, &modules, &numberOfModules);

    if (numberOfModules != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;
        PH_PLUGIN_MENU_INFORMATION menuInfo;

        menu = PhCreateEMenu();
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MODULE_UNLOAD, L"&Unload\bDel", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MODULE_OPENFILELOCATION, L"Open &file location\bCtrl+Enter", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MODULE_PROPERTIES, L"P&roperties", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MODULE_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
        PhSetFlagsEMenuItem(menu, ID_MODULE_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);
        PhpInitializeModuleMenu(menu, ProcessItem->ProcessId, modules, numberOfModules);
        PhInsertCopyCellEMenuItem(menu, ID_MODULE_COPY, Context->ListContext.TreeNewHandle, ContextMenu->Column);

        if (PhPluginsEnabled)
        {
            PhPluginInitializeMenuInfo(&menuInfo, menu, hwndDlg, 0);
            menuInfo.u.Module.ProcessId = ProcessItem->ProcessId;
            menuInfo.u.Module.Modules = modules;
            menuInfo.u.Module.NumberOfModules = numberOfModules;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackModuleMenuInitializing), &menuInfo);
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

    PhFree(modules);
}

static BOOLEAN PhpWordMatchHandleStringRef(
    _In_ PPH_STRING SearchText,
    _In_ PPH_STRINGREF Text
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    remainingPart = SearchText->sr;

    while (remainingPart.Length)
    {
        PhSplitStringRefAtChar(&remainingPart, L'|', &part, &remainingPart);

        if (part.Length)
        {
            if (PhFindStringInStringRef(Text, &part, TRUE) != -1)
                return TRUE;
        }
    }

    return FALSE;
}

static BOOLEAN PhpWordMatchHandleStringZ(
    _In_ PPH_STRING SearchText,
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRef(&text, Text);

    return PhpWordMatchHandleStringRef(SearchText, &text);
}

BOOLEAN PhpModulesTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PPH_MODULES_CONTEXT Context
    )
{
    PPH_MODULE_NODE moduleNode = (PPH_MODULE_NODE)Node;
    PPH_MODULE_ITEM moduleItem = moduleNode->ModuleItem;

    switch (moduleItem->LoadReason)
    {
    case LoadReasonStaticDependency:
    case LoadReasonStaticForwarderDependency:
        {
            if (Context->ListContext.HideStaticModules)
                return FALSE;
        }
        break;
    case LoadReasonDynamicForwarderDependency:
    case LoadReasonDynamicLoad:
        {
            if (Context->ListContext.HideDynamicModules)
                return FALSE;
        }
        break;
    }

    switch (moduleItem->Type)
    {
    case PH_MODULE_TYPE_MAPPED_FILE:
    case PH_MODULE_TYPE_MAPPED_IMAGE:
        {
            if (Context->ListContext.HideMappedModules)
                return FALSE;
        }
        break;
    }

    if (Context->ListContext.HideSignedModules && moduleItem->VerifyResult == VrTrusted)
        return FALSE;

    if (
        PhEnableProcessQueryStage2 &&
        Context->ListContext.HideSystemModules &&
        moduleItem->VerifyResult == VrTrusted &&
        PhEqualStringRef2(&moduleItem->VerifySignerName->sr, L"Microsoft Windows", TRUE)
        )
    {
        return FALSE;
    }

    if (PhIsNullOrEmptyString(Context->SearchboxText))
        return TRUE;

    // (dmex) TODO: Add search support for the following fields:
    //ULONG Flags;
    //ULONG Type;
    //USHORT ImageCharacteristics;
    //USHORT ImageDllCharacteristics;

    // module properties

    if (!PhIsNullOrEmptyString(moduleItem->Name))
    {
        if (PhpWordMatchHandleStringRef(Context->SearchboxText, &moduleItem->Name->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(moduleItem->FileName))
    {
        if (PhpWordMatchHandleStringRef(Context->SearchboxText, &moduleItem->FileName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(moduleItem->VerifySignerName))
    {
        if (PhpWordMatchHandleStringRef(Context->SearchboxText, &moduleItem->VerifySignerName->sr))
            return TRUE;
    }

    if (moduleItem->BaseAddressString[0])
    {
        if (PhpWordMatchHandleStringZ(Context->SearchboxText, moduleItem->BaseAddressString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(moduleItem->VersionInfo.CompanyName))
    {
        if (PhpWordMatchHandleStringRef(Context->SearchboxText, &moduleItem->VersionInfo.CompanyName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(moduleItem->VersionInfo.FileDescription))
    {
        if (PhpWordMatchHandleStringRef(Context->SearchboxText, &moduleItem->VersionInfo.FileDescription->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(moduleItem->VersionInfo.FileVersion))
    {
        if (PhpWordMatchHandleStringRef(Context->SearchboxText, &moduleItem->VersionInfo.FileVersion->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(moduleItem->VersionInfo.ProductName))
    {
        if (PhpWordMatchHandleStringRef(Context->SearchboxText, &moduleItem->VersionInfo.ProductName->sr))
            return TRUE;
    }

    if (moduleItem->EntryPointAddressString[0])
    {
        if (PhpWordMatchHandleStringZ(Context->SearchboxText, moduleItem->EntryPointAddressString))
            return TRUE;
    }

    if (moduleItem->ParentBaseAddressString[0])
    {
        if (PhpWordMatchHandleStringZ(Context->SearchboxText, moduleItem->ParentBaseAddressString))
            return TRUE;
    }

    switch (moduleItem->LoadReason)
    {
    case LoadReasonStaticDependency:
        if (PhpWordMatchHandleStringZ(Context->SearchboxText, L"Static dependency"))
            return TRUE;
        break;
    case LoadReasonStaticForwarderDependency:
        if (PhpWordMatchHandleStringZ(Context->SearchboxText, L"Static forwarder dependency"))
            return TRUE;
        break;
    case LoadReasonDynamicForwarderDependency:
        if (PhpWordMatchHandleStringZ(Context->SearchboxText, L"Dynamic forwarder dependency"))
            return TRUE;
        break;
    case LoadReasonDelayloadDependency:
        if (PhpWordMatchHandleStringZ(Context->SearchboxText, L"Delay load dependency"))
            return TRUE;
        break;
    case LoadReasonDynamicLoad:
        if (PhpWordMatchHandleStringZ(Context->SearchboxText, L"Dynamic"))
            return TRUE;
        break;
    case LoadReasonAsImageLoad:
        if (PhpWordMatchHandleStringZ(Context->SearchboxText, L"Image"))
            return TRUE;
        break;
    case LoadReasonAsDataLoad:
        if (PhpWordMatchHandleStringZ(Context->SearchboxText, L"Data"))
            return TRUE;
        break;
    }

    switch (moduleItem->VerifyResult)
    {
    case VrNoSignature:
        if (PhpWordMatchHandleStringZ(Context->SearchboxText, L"No Signature"))
            return TRUE;
        break;
    case VrExpired:
        if (PhpWordMatchHandleStringZ(Context->SearchboxText, L"Expired"))
            return TRUE;
        break;
    case VrRevoked:
    case VrDistrust:
        if (PhpWordMatchHandleStringZ(Context->SearchboxText, L"Revoked"))
            return TRUE;
        break;
    }

    switch (moduleItem->VerifyResult)
    {
    case VrTrusted:
        if (PhpWordMatchHandleStringZ(Context->SearchboxText, L"Trusted"))
            return TRUE;
        break;
    case VrNoSignature:
    case VrExpired:
    case VrRevoked:
    case VrDistrust:
    case VrUnknown:
    case VrBadSignature:
        if (PhpWordMatchHandleStringZ(Context->SearchboxText, L"Bad"))
            return TRUE;
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PhpProcessModulesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_MODULES_CONTEXT modulesContext;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext, &processItem))
    {
        modulesContext = (PPH_MODULES_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            modulesContext = propPageContext->Context = PhAllocate(PhEmGetObjectSize(EmModulesContextType, sizeof(PH_MODULES_CONTEXT)));
            memset(modulesContext, 0, sizeof(PH_MODULES_CONTEXT));

            modulesContext->WindowHandle = hwndDlg;
            modulesContext->TreeNewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            modulesContext->SearchboxHandle = GetDlgItem(hwndDlg, IDC_SEARCH);

            modulesContext->Provider = PhCreateModuleProvider(
                processItem->ProcessId
                );
            PhRegisterProvider(
                &PhSecondaryProviderThread,
                PhModuleProviderUpdate,
                modulesContext->Provider,
                &modulesContext->ProviderRegistration
                );
            PhRegisterCallback(
                &modulesContext->Provider->ModuleAddedEvent,
                ModuleAddedHandler,
                modulesContext,
                &modulesContext->AddedEventRegistration
                );
            PhRegisterCallback(
                &modulesContext->Provider->ModuleModifiedEvent,
                ModuleModifiedHandler,
                modulesContext,
                &modulesContext->ModifiedEventRegistration
                );
            PhRegisterCallback(
                &modulesContext->Provider->ModuleRemovedEvent,
                ModuleRemovedHandler,
                modulesContext,
                &modulesContext->RemovedEventRegistration
                );
            PhRegisterCallback(
                &modulesContext->Provider->UpdatedEvent,
                ModulesUpdatedHandler,
                modulesContext,
                &modulesContext->UpdatedEventRegistration
                );

            // Initialize the list.
            PhInitializeModuleList(hwndDlg, modulesContext->TreeNewHandle, &modulesContext->ListContext);
            TreeNew_SetEmptyText(modulesContext->TreeNewHandle, &PhpLoadingText, 0);
            PhInitializeProviderEventQueue(&modulesContext->EventQueue, 100);
            modulesContext->LastRunStatus = -1;
            modulesContext->ErrorMessage = NULL;
            modulesContext->SearchboxText = PhReferenceEmptyString();
            modulesContext->FilterEntry = PhAddTreeNewFilter(&modulesContext->ListContext.TreeFilterSupport, PhpModulesTreeFilterCallback, modulesContext);

            // Initialize the search box. (dmex)
            PhCreateSearchControl(hwndDlg, modulesContext->SearchboxHandle, L"Search Modules (Ctrl+K)");

            PhEmCallObjectOperation(EmModulesContextType, modulesContext, EmObjectCreate);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = modulesContext->TreeNewHandle;
                treeNewInfo.CmData = &modulesContext->ListContext.Cm;
                treeNewInfo.SystemContext = modulesContext;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackModuleTreeNewInitializing), &treeNewInfo);
            }

            PhLoadSettingsModuleList(&modulesContext->ListContext);

            PhSetEnabledProvider(&modulesContext->ProviderRegistration, TRUE);
            PhBoostProvider(&modulesContext->ProviderRegistration, NULL);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveTreeNewFilter(&modulesContext->ListContext.TreeFilterSupport, modulesContext->FilterEntry);
            if (modulesContext->SearchboxText) PhDereferenceObject(modulesContext->SearchboxText);

            PhEmCallObjectOperation(EmModulesContextType, modulesContext, EmObjectDelete);

            PhUnregisterCallback(
                &modulesContext->Provider->ModuleAddedEvent,
                &modulesContext->AddedEventRegistration
                );
            PhUnregisterCallback(
                &modulesContext->Provider->ModuleModifiedEvent,
                &modulesContext->ModifiedEventRegistration
                );
            PhUnregisterCallback(
                &modulesContext->Provider->ModuleRemovedEvent,
                &modulesContext->RemovedEventRegistration
                );
            PhUnregisterCallback(
                &modulesContext->Provider->UpdatedEvent,
                &modulesContext->UpdatedEventRegistration
                );
            PhUnregisterProvider(&modulesContext->ProviderRegistration);
            PhDereferenceObject(modulesContext->Provider);
            PhDeleteProviderEventQueue(&modulesContext->EventQueue);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = modulesContext->TreeNewHandle;
                treeNewInfo.CmData = &modulesContext->ListContext.Cm;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackModuleTreeNewUninitializing), &treeNewInfo);
            }

            PhSaveSettingsModuleList(&modulesContext->ListContext);
            PhDeleteModuleList(&modulesContext->ListContext);

            PhClearReference(&modulesContext->ErrorMessage);
            PhFree(modulesContext);
        }
        break;
    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_ITEM dialogItem;

            if (dialogItem = PhBeginPropPageLayout(hwndDlg, propPageContext))
            {
                PhAddPropPageLayoutItem(hwndDlg, modulesContext->SearchboxHandle, dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, modulesContext->TreeNewHandle, dialogItem, PH_ANCHOR_ALL);
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

                    if (GET_WM_COMMAND_HWND(wParam, lParam) != modulesContext->SearchboxHandle)
                        break;

                    newSearchboxText = PH_AUTO(PhGetWindowText(modulesContext->SearchboxHandle));

                    if (!PhEqualString(modulesContext->SearchboxText, newSearchboxText, FALSE))
                    {
                        // Cache the current search text for our callback.
                        PhSwapReference(&modulesContext->SearchboxText, newSearchboxText);

                        // Expand any hidden nodes to make search results visible.
                        PhExpandAllModuleNodes(&modulesContext->ListContext, TRUE);

                        PhApplyTreeNewFilters(&modulesContext->ListContext.TreeFilterSupport);
                    }
                }
                break;
            }

            switch (LOWORD(wParam))
            {
            case ID_SHOWCONTEXTMENU:
                {
                    PhShowModuleContextMenu(hwndDlg, processItem, modulesContext, (PPH_TREENEW_CONTEXT_MENU)lParam);
                }
                break;
            case ID_MODULE_UNLOAD:
                {
                    PPH_MODULE_ITEM moduleItem = PhGetSelectedModuleItem(&modulesContext->ListContext);

                    if (moduleItem)
                    {
                        PhReferenceObject(moduleItem);

                        if (PhUiUnloadModule(hwndDlg, processItem->ProcessId, moduleItem))
                            PhDeselectAllModuleNodes(&modulesContext->ListContext);

                        PhDereferenceObject(moduleItem);
                    }
                }
                break;
            case ID_MODULE_OPENFILELOCATION:
                {
                    PPH_MODULE_ITEM moduleItem = PhGetSelectedModuleItem(&modulesContext->ListContext);

                    if (moduleItem)
                    {
                        PhShellExecuteUserString(
                            hwndDlg,
                            L"FileBrowseExecutable",
                            moduleItem->FileName->Buffer,
                            FALSE,
                            L"Make sure the Explorer executable file is present."
                            );
                    }
                }
                break;
            case ID_MODULE_PROPERTIES:
                {
                    PPH_MODULE_ITEM moduleItem = PhGetSelectedModuleItem(&modulesContext->ListContext);

                    if (moduleItem)
                    {
                        PhShellExecuteUserString(
                            hwndDlg,
                            L"ProgramInspectExecutables",
                            moduleItem->FileName->Buffer,
                            FALSE,
                            L"Make sure the PE Viewer executable file is present."
                            );
                    }
                }
                break;
            case ID_MODULE_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(modulesContext->TreeNewHandle, 0);
                    PhSetClipboardString(modulesContext->TreeNewHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            case IDC_FILTEROPTIONS:
                {
                    RECT rect;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM dynamicItem;
                    PPH_EMENU_ITEM mappedItem;
                    PPH_EMENU_ITEM staticItem;
                    PPH_EMENU_ITEM verifiedItem;
                    PPH_EMENU_ITEM systemItem;
                    PPH_EMENU_ITEM untrustedItem;
                    PPH_EMENU_ITEM systemHighlightItem;
                    PPH_EMENU_ITEM dotnetItem;
                    PPH_EMENU_ITEM immersiveItem;
                    PPH_EMENU_ITEM relocatedItem;
                    PPH_EMENU_ITEM selectedItem;

                    GetWindowRect(GetDlgItem(hwndDlg, IDC_FILTEROPTIONS), &rect);

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, dynamicItem = PhCreateEMenuItem(0, PH_MODULE_FLAGS_DYNAMIC_OPTION, L"Hide dynamic", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, mappedItem = PhCreateEMenuItem(0, PH_MODULE_FLAGS_MAPPED_OPTION, L"Hide mapped", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, staticItem = PhCreateEMenuItem(0, PH_MODULE_FLAGS_STATIC_OPTION, L"Hide static", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, verifiedItem = PhCreateEMenuItem(0, PH_MODULE_FLAGS_SIGNED_OPTION, L"Hide verified", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, systemItem = PhCreateEMenuItem(0, PH_MODULE_FLAGS_SYSTEM_OPTION, L"Hide system", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, dotnetItem = PhCreateEMenuItem(0, PH_MODULE_FLAGS_HIGHLIGHT_DOTNET_OPTION, L"Highlight .NET modules", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, immersiveItem = PhCreateEMenuItem(0, PH_MODULE_FLAGS_HIGHLIGHT_IMMERSIVE_OPTION, L"Highlight immersive modules", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, relocatedItem = PhCreateEMenuItem(0, PH_MODULE_FLAGS_HIGHLIGHT_RELOCATED_OPTION, L"Highlight relocated modules", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, untrustedItem = PhCreateEMenuItem(0, PH_MODULE_FLAGS_HIGHLIGHT_UNSIGNED_OPTION, L"Highlight untrusted modules", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, systemHighlightItem = PhCreateEMenuItem(0, PH_MODULE_FLAGS_HIGHLIGHT_SYSTEM_OPTION, L"Highlight system modules", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_MODULE_FLAGS_LOAD_MODULE_OPTION, L"Load module...", NULL, NULL), ULONG_MAX);
                    //PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    //PhInsertEMenuItem(menu, stringsItem = PhCreateEMenuItem(0, PH_MODULE_FLAGS_MODULE_STRINGS_OPTION, L"Strings...", NULL, NULL), ULONG_MAX);

                    if (modulesContext->ListContext.HideDynamicModules)
                        dynamicItem->Flags |= PH_EMENU_CHECKED;
                    if (modulesContext->ListContext.HideMappedModules)
                        mappedItem->Flags |= PH_EMENU_CHECKED;
                    if (modulesContext->ListContext.HideStaticModules)
                        staticItem->Flags |= PH_EMENU_CHECKED;
                    if (modulesContext->ListContext.HideSignedModules)
                        verifiedItem->Flags |= PH_EMENU_CHECKED;
                    if (modulesContext->ListContext.HideSystemModules)
                        systemItem->Flags |= PH_EMENU_CHECKED;
                    if (modulesContext->ListContext.HighlightDotNetModules)
                        dotnetItem->Flags |= PH_EMENU_CHECKED;
                    if (modulesContext->ListContext.HighlightImmersiveModules)
                        immersiveItem->Flags |= PH_EMENU_CHECKED;
                    if (modulesContext->ListContext.HighlightRelocatedModules)
                        relocatedItem->Flags |= PH_EMENU_CHECKED;
                    if (modulesContext->ListContext.HighlightUntrustedModules)
                        untrustedItem->Flags |= PH_EMENU_CHECKED;
                    if (modulesContext->ListContext.HighlightSystemModules)
                        systemHighlightItem->Flags |= PH_EMENU_CHECKED;

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
                        if (selectedItem->Id == PH_MODULE_FLAGS_LOAD_MODULE_OPTION)
                        {
                            if (PhGetIntegerSetting(L"EnableWarnings") && !PhShowConfirmMessage(
                                hwndDlg,
                                L"load",
                                L"a module",
                                L"Some programs may restrict access or ban your account when loading modules into the process.",
                                FALSE
                                ))
                            {
                                break;
                            }

                            PhReferenceObject(processItem);
                            PhUiLoadDllProcess(hwndDlg, processItem);
                            PhDereferenceObject(processItem);
                        }
                        else if (selectedItem->Id == PH_MODULE_FLAGS_MODULE_STRINGS_OPTION)
                        {
                            // TODO
                        }
                        else
                        {
                            PhSetOptionsModuleList(&modulesContext->ListContext, selectedItem->Id);
                            PhSaveSettingsModuleList(&modulesContext->ListContext);
                            PhApplyTreeNewFilters(&modulesContext->ListContext.TreeFilterSupport);
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
                PhSetEnabledProvider(&modulesContext->ProviderRegistration, TRUE);
                break;
            case PSN_KILLACTIVE:
                PhSetEnabledProvider(&modulesContext->ProviderRegistration, FALSE);
                break;
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)GetDlgItem(hwndDlg, IDC_LIST));
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
                    SetFocus(modulesContext->SearchboxHandle);
                    return TRUE;
                }
            }
        }
        break;
    case WM_PH_MODULES_UPDATED:
        {
            ULONG upToRunId = (ULONG)wParam;
            PPH_PROVIDER_EVENT events;
            ULONG count;
            ULONG i;

            events = PhFlushProviderEventQueue(&modulesContext->EventQueue, upToRunId, &count);

            if (events)
            {
                TreeNew_SetRedraw(modulesContext->TreeNewHandle, FALSE);

                for (i = 0; i < count; i++)
                {
                    PH_PROVIDER_EVENT_TYPE type = PH_PROVIDER_EVENT_TYPE(events[i]);
                    PPH_MODULE_ITEM moduleItem = PH_PROVIDER_EVENT_OBJECT(events[i]);

                    switch (type)
                    {
                    case ProviderAddedEvent:
                        PhAddModuleNode(&modulesContext->ListContext, moduleItem, events[i].RunId);
                        PhDereferenceObject(moduleItem);
                        break;
                    case ProviderModifiedEvent:
                        PhUpdateModuleNode(&modulesContext->ListContext, PhFindModuleNode(&modulesContext->ListContext, moduleItem));
                        break;
                    case ProviderRemovedEvent:
                        PhRemoveModuleNode(&modulesContext->ListContext, PhFindModuleNode(&modulesContext->ListContext, moduleItem));
                        break;
                    }
                }

                PhFree(events);
            }

            PhTickModuleNodes(&modulesContext->ListContext);

            if (modulesContext->LastRunStatus != modulesContext->Provider->RunStatus)
            {
                NTSTATUS status;
                PPH_STRING message;

                status = modulesContext->Provider->RunStatus;
                modulesContext->LastRunStatus = status;

                if (!PH_IS_REAL_PROCESS_ID(processItem->ProcessId))
                    status = STATUS_SUCCESS;

                if (NT_SUCCESS(status) || status == STATUS_PARTIAL_COPY) // partial when process exits (dmex)
                {
                    TreeNew_SetEmptyText(modulesContext->TreeNewHandle, &EmptyModulesText, 0);
                }
                else
                {
                    message = PhGetStatusMessage(status, 0);
                    PhMoveReference(&modulesContext->ErrorMessage, PhFormatString(L"Unable to query module information:\n%s", PhGetStringOrDefault(message, L"Unknown error.")));
                    PhClearReference(&message);
                    TreeNew_SetEmptyText(modulesContext->TreeNewHandle, &modulesContext->ErrorMessage->sr, 0);
                }

                InvalidateRect(modulesContext->TreeNewHandle, NULL, FALSE);
            }

            // Refresh the visible nodes.
            PhApplyTreeNewFilters(&modulesContext->ListContext.TreeFilterSupport);

            if (count != 0)
                TreeNew_SetRedraw(modulesContext->TreeNewHandle, TRUE);
        }
        break;
    }

    return FALSE;
}
