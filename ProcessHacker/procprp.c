/*
 * Process Hacker - 
 *   process properties
 * 
 * Copyright (C) 2009-2010 wj32
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

#include <phgui.h>
#include <procprpp.h>

PPH_OBJECT_TYPE PhpProcessPropContextType;
PPH_OBJECT_TYPE PhpProcessPropPageContextType;

BOOLEAN PhProcessPropInitialization()
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhpProcessPropContextType,
        0,
        PhpProcessPropContextDeleteProcedure
        )))
        return FALSE;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhpProcessPropPageContextType,
        0,
        PhpProcessPropPageContextDeleteProcedure
        )))
        return FALSE;

    return TRUE;
}

PPH_PROCESS_PROPCONTEXT PhCreateProcessPropContext(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_PROCESS_PROPCONTEXT propContext;
    PROPSHEETHEADER propSheetHeader;

    if (!NT_SUCCESS(PhCreateObject(
        &propContext,
        sizeof(PH_PROCESS_PROPCONTEXT),
        0,
        PhpProcessPropContextType,
        0
        )))
        return NULL;

    memset(propContext, 0, sizeof(PH_PROCESS_PROPCONTEXT));

    propContext->PropSheetPages =
        PhAllocate(sizeof(HPROPSHEETPAGE) * PH_PROCESS_PROPCONTEXT_MAXPAGES);

    memset(&propSheetHeader, 0, sizeof(PROPSHEETHEADER));
    propSheetHeader.dwSize = sizeof(PROPSHEETHEADER);
    propSheetHeader.dwFlags =
        PSH_MODELESS |
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE |
        PSH_USECALLBACK |
        PSH_USEHICON;
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.hIcon = ProcessItem->SmallIcon;
    propSheetHeader.pszCaption = ProcessItem->ProcessName->Buffer;
    propSheetHeader.pfnCallback = PhpPropSheetProc;

    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = propContext->PropSheetPages;

    memcpy(&propContext->PropSheetHeader, &propSheetHeader, sizeof(PROPSHEETHEADER));

    propContext->ProcessItem = ProcessItem;
    PhReferenceObject(ProcessItem);
    PhInitializeEvent(&propContext->CreatedEvent);

    return propContext;
}

VOID NTAPI PhpProcessPropContextDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_PROCESS_PROPCONTEXT propContext = (PPH_PROCESS_PROPCONTEXT)Object;

    PhFree(propContext->PropSheetPages);
    PhDereferenceObject(propContext->ProcessItem);
}

INT CALLBACK PhpPropSheetProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in LPARAM lParam
    )
{
    const LONG addStyle = WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;

    switch (uMsg)
    {
    case PSCB_PRECREATE:
        {
            if (lParam)
            {
                if (((DLGTEMPLATEEX *)lParam)->signature == 0xffff)
                {
                    ((DLGTEMPLATEEX *)lParam)->style |= addStyle;
                }
                else
                {
                    ((DLGTEMPLATE *)lParam)->style |= addStyle;
                }
            }
        }
        break;
    case PSCB_BUTTONPRESSED:
        {

        }
        break;
    case PSCB_INITIALIZED:
        {

        }
        break;
    }

    return 0;
}

LRESULT CALLBACK PhpPropSheetWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    WNDPROC oldWndProc = (WNDPROC)GetProp(hwnd, L"OldWndProc");

    switch (uMsg)
    {
    case WM_SIZE:
        {
            if (wParam != SIZE_MINIMIZED)
            {
                RECT rect;

                GetClientRect(hwnd, &rect);

                if (rect.right - rect.left != 0)
                {
                    
                }
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

BOOLEAN PhAddProcessPropPage(
    __inout PPH_PROCESS_PROPCONTEXT PropContext,
    __in PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    )
{
    HPROPSHEETPAGE propSheetPageHandle;

    if (PropContext->PropSheetHeader.nPages == PH_PROCESS_PROPCONTEXT_MAXPAGES)
        return FALSE;

    propSheetPageHandle = CreatePropertySheetPage(
        &PropPageContext->PropSheetPage
        );
    PropPageContext->PropContext = PropContext;
    PhReferenceObject(PropContext);

    PropContext->PropSheetPages[PropContext->PropSheetHeader.nPages] =
        propSheetPageHandle;
    PropContext->PropSheetHeader.nPages++;

    return TRUE;
}

PPH_PROCESS_PROPPAGECONTEXT PhCreateProcessPropPageContext(
    __in LPCWSTR Template,
    __in DLGPROC DlgProc,
    __in PVOID Context
    )
{
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;

    if (!NT_SUCCESS(PhCreateObject(
        &propPageContext,
        sizeof(PH_PROCESS_PROPPAGECONTEXT),
        0,
        PhpProcessPropPageContextType,
        0
        )))
        return NULL;

    memset(&propPageContext->PropSheetPage, 0, sizeof(PROPSHEETPAGE));
    propPageContext->PropSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propPageContext->PropSheetPage.dwFlags =
        PSP_USECALLBACK;
    propPageContext->PropSheetPage.pszTemplate = Template;
    propPageContext->PropSheetPage.pfnDlgProc = DlgProc;
    propPageContext->PropSheetPage.lParam = (LPARAM)propPageContext;
    propPageContext->PropSheetPage.pfnCallback = PhpStandardPropPageProc;

    propPageContext->Context = Context;

    return propPageContext;
}

VOID NTAPI PhpProcessPropPageContextDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_PROCESS_PROPPAGECONTEXT propPageContext = (PPH_PROCESS_PROPPAGECONTEXT)Object;

    if (propPageContext->PropContext)
        PhDereferenceObject(propPageContext->PropContext);
}

INT CALLBACK PhpStandardPropPageProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in LPPROPSHEETPAGE ppsp
    )
{
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;

    propPageContext = (PPH_PROCESS_PROPPAGECONTEXT)ppsp->lParam;

    if (uMsg == PSPCB_ADDREF)
        PhReferenceObject(propPageContext);
    else if (uMsg == PSPCB_RELEASE)
        PhDereferenceObject(propPageContext);

    return 1;
}

BOOLEAN PhpPropPageDlgProcHeader(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in LPARAM lParam,
    __out LPPROPSHEETPAGE *PropSheetPage,
    __out PPH_PROCESS_PROPPAGECONTEXT *PropPageContext,
    __out PPH_PROCESS_ITEM *ProcessItem
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;

    if (uMsg == WM_INITDIALOG)
    {
        // Save the context.
        SetProp(hwndDlg, L"PropPageContext", (HANDLE)lParam);
    }

    propSheetPage = (LPPROPSHEETPAGE)GetProp(hwndDlg, L"PropPageContext");

    if (!propSheetPage)
        return FALSE;

    *PropSheetPage = propSheetPage;
    *PropPageContext = propPageContext = (PPH_PROCESS_PROPPAGECONTEXT)propSheetPage->lParam;
    *ProcessItem = propPageContext->PropContext->ProcessItem;

    return TRUE;
}

VOID PhpPropPageDlgProcDestroy(
    __in HWND hwndDlg
    )
{
    RemoveProp(hwndDlg, L"PropPageContext");
}

INT_PTR CALLBACK PhpProcessGeneralDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;

    if (!PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            SendMessage(GetDlgItem(hwndDlg, IDC_PROCGENERAL_ICON), STM_SETICON,
                (WPARAM)processItem->LargeIcon, 0);
            SetDlgItemText(hwndDlg, IDC_PROCGENERAL_NAME,
                PhGetString(processItem->VersionInfo.FileDescription));
            SetDlgItemText(hwndDlg, IDC_PROCGENERAL_COMPANYNAME,
                PhGetString(processItem->VersionInfo.CompanyName)); 
        }
        break;
    case WM_DESTROY:
        {
            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case IDC_PROCGENERAL_TERMINATE:
                {
                    if (PhShowMessage(hwndDlg, MB_ICONWARNING | MB_YESNO,
                        L"Are you sure you want to terminate %s?",
                        processItem->ProcessName->Buffer
                        ) == IDYES)
                    {
                        NTSTATUS status;
                        HANDLE processHandle;

                        if (NT_SUCCESS(status = PhOpenProcess(
                            &processHandle,
                            PROCESS_TERMINATE,
                            processItem->ProcessId
                            )))
                        {
                            status = PhTerminateProcess(
                                processHandle,
                                STATUS_SUCCESS
                                );
                            CloseHandle(processHandle);
                        }

                        if (!NT_SUCCESS(status))
                        {
                            PhShowStatus(
                                hwndDlg,
                                L"Unable to terminate the process",
                                status,
                                0
                                );
                        }
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

static VOID NTAPI ThreadAddedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    // Parameter contains a pointer to the added thread item.
    PhReferenceObject(Parameter);
    PostMessage(threadsContext->WindowHandle, WM_PH_THREAD_ADDED, 0, (LPARAM)Parameter);
}

static VOID NTAPI ThreadModifiedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PostMessage(threadsContext->WindowHandle, WM_PH_THREAD_MODIFIED, 0, (LPARAM)Parameter);
}

static VOID NTAPI ThreadRemovedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PostMessage(threadsContext->WindowHandle, WM_PH_THREAD_REMOVED, 0, (LPARAM)Parameter);
}

INT_PTR CALLBACK PhpProcessThreadsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_THREADS_CONTEXT threadsContext;
    HWND lvHandle;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        threadsContext = (PPH_THREADS_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    lvHandle = GetDlgItem(hwndDlg, IDC_PROCTHREADS_LIST);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            threadsContext = propPageContext->Context =
                PhAllocate(sizeof(PH_THREADS_CONTEXT));

            threadsContext->Provider = PhCreateThreadProvider(
                processItem->ProcessId
                );
            PhRegisterProvider(
                &PhSecondaryProviderThread,
                PhThreadProviderUpdate,
                threadsContext->Provider,
                &threadsContext->ProviderRegistration
                );
            PhRegisterCallback(
                &threadsContext->Provider->ThreadAddedEvent,
                ThreadAddedHandler,
                threadsContext,
                &threadsContext->AddedEventRegistration
                );
            PhRegisterCallback(
                &threadsContext->Provider->ThreadModifiedEvent,
                ThreadModifiedHandler,
                threadsContext,
                &threadsContext->ModifiedEventRegistration
                );
            PhRegisterCallback(
                &threadsContext->Provider->ThreadRemovedEvent,
                ThreadRemovedHandler,
                threadsContext,
                &threadsContext->RemovedEventRegistration
                );
            PhSetProviderEnabled(
                &threadsContext->ProviderRegistration,
                TRUE
                );
            PhBoostProvider(
                &PhSecondaryProviderThread,
                &threadsContext->ProviderRegistration
                );
            threadsContext->WindowHandle = hwndDlg;

            // Initialize the list.
            ListView_SetExtendedListViewStyleEx(lvHandle,
                LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, -1);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 50, L"TID");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"Cycles Delta"); 
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 200, L"Start Address"); 
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 120, L"Priority"); 
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(
                &threadsContext->Provider->ThreadAddedEvent,
                &threadsContext->AddedEventRegistration
                );
            PhUnregisterCallback(
                &threadsContext->Provider->ThreadModifiedEvent,
                &threadsContext->ModifiedEventRegistration
                );
            PhUnregisterCallback(
                &threadsContext->Provider->ThreadRemovedEvent,
                &threadsContext->RemovedEventRegistration
                );
            PhUnregisterProvider(
                &PhSecondaryProviderThread,
                &threadsContext->ProviderRegistration
                );

            PhDereferenceAllThreadItems(threadsContext->Provider);

            PhDereferenceObject(threadsContext->Provider);
            PhFree(threadsContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_SETACTIVE:
                PhSetProviderEnabled(&threadsContext->ProviderRegistration, TRUE);
                break;
            case PSN_KILLACTIVE:
                PhSetProviderEnabled(&threadsContext->ProviderRegistration, FALSE);
                break;
            }
        }
        break;
    case WM_PH_THREAD_ADDED:
        {
            INT lvItemIndex;
            PPH_THREAD_ITEM threadItem = (PPH_THREAD_ITEM)lParam;

            lvItemIndex = PhAddListViewItem(
                lvHandle,
                MAXINT,
                threadItem->ThreadIdString,
                threadItem
                );
            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, PhGetString(threadItem->StartAddressString));
            PhSetListViewSubItem(lvHandle, lvItemIndex, 3, L"Priority Here");
        }
        break;
    case WM_PH_THREAD_MODIFIED:
        {
            INT lvItemIndex;
            PPH_THREAD_ITEM threadItem = (PPH_THREAD_ITEM)lParam;

            lvItemIndex = PhFindListViewItemByParam(lvHandle, -1, threadItem);

            if (lvItemIndex != -1)
            {
                PhSetListViewSubItem(lvHandle, lvItemIndex, 1, PhGetString(threadItem->CyclesDeltaString));
                PhSetListViewSubItem(lvHandle, lvItemIndex, 2, PhGetString(threadItem->StartAddressString));
            }
        }
        break;
    case WM_PH_THREAD_REMOVED:
        {
            PPH_THREAD_ITEM threadItem = (PPH_THREAD_ITEM)lParam;

            PhRemoveListViewItem(
                lvHandle,
                PhFindListViewItemByParam(lvHandle, -1, threadItem)
                );
            PhDereferenceObject(threadItem);
        }
        break;
    }

    return FALSE;
}

static VOID NTAPI ModuleAddedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_MODULES_CONTEXT modulesContext = (PPH_MODULES_CONTEXT)Context;

    // Parameter contains a pointer to the added module item.
    PhReferenceObject(Parameter);
    PostMessage(modulesContext->WindowHandle, WM_PH_MODULE_ADDED, 0, (LPARAM)Parameter);
}

static VOID NTAPI ModuleRemovedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_MODULES_CONTEXT modulesContext = (PPH_MODULES_CONTEXT)Context;

    PostMessage(modulesContext->WindowHandle, WM_PH_MODULE_REMOVED, 0, (LPARAM)Parameter);
}

INT_PTR CALLBACK PhpProcessModulesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_MODULES_CONTEXT modulesContext;
    HWND lvHandle;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        modulesContext = (PPH_MODULES_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    lvHandle = GetDlgItem(hwndDlg, IDC_PROCMODULES_LIST);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            // Lots of boilerplate code...

            modulesContext = propPageContext->Context =
                PhAllocate(sizeof(PH_MODULES_CONTEXT));

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
                &modulesContext->Provider->ModuleRemovedEvent,
                ModuleRemovedHandler,
                modulesContext,
                &modulesContext->RemovedEventRegistration
                );
            PhSetProviderEnabled(
                &modulesContext->ProviderRegistration,
                TRUE
                );
            PhBoostProvider(
                &PhSecondaryProviderThread,
                &modulesContext->ProviderRegistration
                );
            modulesContext->WindowHandle = hwndDlg;

            // Initialize the list.
            ListView_SetExtendedListViewStyleEx(lvHandle,
                LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, -1);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 120, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 120, L"Base Address"); 
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 200, L"File Name"); 
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(
                &modulesContext->Provider->ModuleAddedEvent,
                &modulesContext->AddedEventRegistration
                );
            PhUnregisterCallback(
                &modulesContext->Provider->ModuleRemovedEvent,
                &modulesContext->RemovedEventRegistration
                );
            PhUnregisterProvider(
                &PhSecondaryProviderThread,
                &modulesContext->ProviderRegistration
                );

            PhDereferenceAllModuleItems(modulesContext->Provider);

            PhDereferenceObject(modulesContext->Provider);
            PhFree(modulesContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_SETACTIVE:
                PhSetProviderEnabled(&modulesContext->ProviderRegistration, TRUE);
                break;
            case PSN_KILLACTIVE:
                PhSetProviderEnabled(&modulesContext->ProviderRegistration, FALSE);
                break;
            }
        }
        break;
    case WM_PH_MODULE_ADDED:
        {
            INT lvItemIndex;
            PPH_MODULE_ITEM moduleItem = (PPH_MODULE_ITEM)lParam;

            lvItemIndex = PhAddListViewItem(
                lvHandle,
                MAXINT,
                moduleItem->Name->Buffer,
                moduleItem
                );
            PhSetListViewSubItem(lvHandle, lvItemIndex, 1, moduleItem->BaseAddressString);
            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, PhGetString(moduleItem->FileName));
        }
        break;
    case WM_PH_MODULE_REMOVED:
        {
            PPH_MODULE_ITEM moduleItem = (PPH_MODULE_ITEM)lParam;

            PhRemoveListViewItem(
                lvHandle,
                PhFindListViewItemByParam(lvHandle, -1, moduleItem)
                );
            PhDereferenceObject(moduleItem);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PhpProcessEnvironmentDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;

    if (!PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE processHandle;
            PPH_ENVIRONMENT_VARIABLE variables;
            ULONG numberOfVariables;
            ULONG i;
            HWND lvHandle = GetDlgItem(hwndDlg, IDC_PROCENVIRONMENT_LIST);

            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 140, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Value");
            ListView_SetExtendedListViewStyleEx(lvHandle,
                LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, -1);
            PhSetControlTheme(lvHandle, L"explorer");

            if (NT_SUCCESS(PhOpenProcess(
                &processHandle,
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                processItem->ProcessId
                )))
            {
                if (NT_SUCCESS(PhGetProcessEnvironmentVariables(
                    processHandle,
                    &variables,
                    &numberOfVariables
                    )))
                {
                    for (i = 0; i < numberOfVariables; i++)
                    {
                        INT lvItemIndex;

                        // Don't display pairs with no name.
                        if (variables[i].Name->Length == 0)
                            continue;

                        lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, PhGetString(variables[i].Name), NULL);
                        PhSetListViewSubItem(lvHandle, lvItemIndex, 1, PhGetString(variables[i].Value));
                    }

                    PhFreeProcessEnvironmentVariables(variables, numberOfVariables);
                }

                CloseHandle(processHandle);
            }
        }
        break;
    case WM_DESTROY:
        {
            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    }

    return FALSE;
}

static VOID NTAPI HandleAddedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    // Parameter contains a pointer to the added handle item.
    PhReferenceObject(Parameter);
    PostMessage(handlesContext->WindowHandle, WM_PH_HANDLE_ADDED, 0, (LPARAM)Parameter);
}

static VOID NTAPI HandleModifiedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    PostMessage(handlesContext->WindowHandle, WM_PH_HANDLE_MODIFIED, 0, (LPARAM)Parameter);
}

static VOID NTAPI HandleRemovedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    PostMessage(handlesContext->WindowHandle, WM_PH_HANDLE_REMOVED, 0, (LPARAM)Parameter);
}

static VOID NTAPI HandlesUpdatedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_HANDLES_CONTEXT handlesContext = (PPH_HANDLES_CONTEXT)Context;

    PostMessage(handlesContext->WindowHandle, WM_PH_HANDLES_UPDATED, 0, 0);
}

INT_PTR CALLBACK PhpProcessHandlesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_HANDLES_CONTEXT handlesContext;
    HWND lvHandle;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        handlesContext = (PPH_HANDLES_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    lvHandle = GetDlgItem(hwndDlg, IDC_PROCHANDLES_LIST);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            handlesContext = propPageContext->Context =
                PhAllocate(sizeof(PH_HANDLES_CONTEXT));

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
            PhSetProviderEnabled(
                &handlesContext->ProviderRegistration,
                TRUE
                );
            PhBoostProvider(
                &PhSecondaryProviderThread,
                &handlesContext->ProviderRegistration
                );
            handlesContext->WindowHandle = hwndDlg;

            // Initialize the list.
            ListView_SetExtendedListViewStyleEx(lvHandle,
                LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, -1);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Type");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Name"); 
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"Handle"); 
        }
        break;
    case WM_DESTROY:
        {
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
            PhUnregisterProvider(
                &PhSecondaryProviderThread,
                &handlesContext->ProviderRegistration
                );

            PhDereferenceAllHandleItems(handlesContext->Provider);

            PhDereferenceObject(handlesContext->Provider);
            PhFree(handlesContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_SETACTIVE:
                PhSetProviderEnabled(&handlesContext->ProviderRegistration, TRUE);
                break;
            case PSN_KILLACTIVE:
                PhSetProviderEnabled(&handlesContext->ProviderRegistration, FALSE);
                break;
            }
        }
        break;
    case WM_PH_HANDLE_ADDED:
        {
            INT lvItemIndex;
            PPH_HANDLE_ITEM handleItem = (PPH_HANDLE_ITEM)lParam;

            // Disable redraw. It will be re-enabled later.
            SendMessage(lvHandle, WM_SETREDRAW, FALSE, 0);

            lvItemIndex = PhAddListViewItem(
                lvHandle,
                MAXINT,
                PhGetString(handleItem->TypeName),
                handleItem
                );
            PhSetListViewSubItem(lvHandle, lvItemIndex, 1, PhGetString(handleItem->BestObjectName));
            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, handleItem->HandleString);
        }
        break;
    case WM_PH_HANDLE_MODIFIED:
        {
            INT lvItemIndex;
            PPH_HANDLE_ITEM handleItem = (PPH_HANDLE_ITEM)lParam;

            lvItemIndex = PhFindListViewItemByParam(lvHandle, -1, handleItem);

            if (lvItemIndex != -1)
            {
                // TODO when highlighting is implemented
            }
        }
        break;
    case WM_PH_HANDLE_REMOVED:
        {
            PPH_HANDLE_ITEM handleItem = (PPH_HANDLE_ITEM)lParam;

            SendMessage(lvHandle, WM_SETREDRAW, FALSE, 0);

            PhRemoveListViewItem(
                lvHandle,
                PhFindListViewItemByParam(lvHandle, -1, handleItem)
                );
            PhDereferenceObject(handleItem);
        }
        break;
    case WM_PH_HANDLES_UPDATED:
        {
            // Enable redraw.
            SendMessage(lvHandle, WM_SETREDRAW, TRUE, 0);
            InvalidateRect(lvHandle, NULL, FALSE);
        }
        break;
    }

    return FALSE;
}

NTSTATUS PhpProcessPropertiesThreadStart(
    __in PVOID Parameter
    )
{
    PPH_PROCESS_PROPCONTEXT PropContext = (PPH_PROCESS_PROPCONTEXT)Parameter;
    PPH_PROCESS_PROPPAGECONTEXT newPage;
    HWND hwnd;
    BOOL result;
    MSG msg;

    // Wait for stage 1 to be processed.
    PhWaitForEvent(&PropContext->ProcessItem->Stage1Event, INFINITE);

    // Add the pages...

    // General
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCGENERAL),
        PhpProcessGeneralDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);
    PhDereferenceObject(newPage);

    // Threads
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCTHREADS),
        PhpProcessThreadsDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);
    PhDereferenceObject(newPage);

    // Modules
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCMODULES),
        PhpProcessModulesDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);
    PhDereferenceObject(newPage);

    // Environment
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCENVIRONMENT),
        PhpProcessEnvironmentDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);
    PhDereferenceObject(newPage);

    // Handles
    newPage = PhCreateProcessPropPageContext(
        MAKEINTRESOURCE(IDD_PROCHANDLES),
        PhpProcessHandlesDlgProc,
        NULL
        );
    PhAddProcessPropPage(PropContext, newPage);
    PhDereferenceObject(newPage);

    // Create the property sheet

    hwnd = (HWND)PropertySheet(&PropContext->PropSheetHeader);

    PropContext->WindowHandle = hwnd;
    PhSetEvent(&PropContext->CreatedEvent);

    // Main event loop

    while (result = GetMessage(&msg, NULL, 0, 0))
    {
        if (result == -1)
            return STATUS_UNSUCCESSFUL;

        if (!PropSheet_IsDialogMessage(hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Destroy the window when necessary.
        if (!PropSheet_GetCurrentPageHwnd(hwnd))
        {
            DestroyWindow(hwnd);
            break;
        }
    }

    PhDereferenceObject(PropContext);

    return STATUS_SUCCESS;
}

BOOLEAN PhShowProcessProperties(
    __in PPH_PROCESS_PROPCONTEXT Context
    )
{
    HANDLE threadHandle;

    PhReferenceObject(Context);
    threadHandle = CreateThread(NULL, 0, PhpProcessPropertiesThreadStart, Context, 0, NULL);

    if (threadHandle)
    {
        CloseHandle(threadHandle);
        return TRUE;
    }
    else
    {
        PhDereferenceObject(Context);
        return FALSE;
    }
}
