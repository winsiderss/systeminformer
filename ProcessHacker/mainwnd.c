/*
 * Process Hacker - 
 *   main window
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
#include <wchar.h>

typedef BOOL (WINAPI *_FileIconInit)(
    BOOL RestoreCache
    );

VOID PhMainWndOnCreate();
VOID PhMainWndOnLayout();
VOID PhMainWndTabControlOnLayout();
VOID PhMainWndTabControlOnNotify(
    __in LPNMHDR Header
    );
VOID PhMainWndTabControlOnSelectionChanged();
VOID PhMainWndProcessListViewOnNotify(
    __in LPNMHDR Header
    );

VOID PhpShowProcessProperties(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhMainWndOnProcessAdded(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhMainWndOnProcessModified(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhMainWndOnProcessRemoved(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhMainWndOnServiceAdded(
    __in PPH_SERVICE_ITEM ServiceItem
    );

VOID PhMainWndOnServiceModified(
    __in PPH_SERVICE_MODIFIED_DATA ServiceModifiedData
    );

VOID PhMainWndOnServiceRemoved(
    __in PPH_SERVICE_ITEM ServiceItem
    );

HWND PhMainWndHandle;
static HWND TabControlHandle;
static INT ProcessesTabIndex;
static INT ServicesTabIndex;
static INT NetworkTabIndex;
static HWND ProcessListViewHandle;
static HWND ServiceListViewHandle;
static HWND NetworkListViewHandle;

static PH_PROVIDER_REGISTRATION ProcessProviderRegistration;
static PH_CALLBACK_REGISTRATION ProcessAddedRegistration;
static PH_CALLBACK_REGISTRATION ProcessModifiedRegistration;
static PH_CALLBACK_REGISTRATION ProcessRemovedRegistration;

static PH_PROVIDER_REGISTRATION ServiceProviderRegistration;
static PH_CALLBACK_REGISTRATION ServiceAddedRegistration;
static PH_CALLBACK_REGISTRATION ServiceModifiedRegistration;
static PH_CALLBACK_REGISTRATION ServiceRemovedRegistration;

BOOLEAN PhMainWndInitialization(
    __in INT ShowCommand
    )
{
    // Enable debug privilege if possible.
    {
        HANDLE tokenHandle;

        if (NT_SUCCESS(PhOpenProcessToken(
            &tokenHandle,
            TOKEN_ADJUST_PRIVILEGES,
            NtCurrentProcess()
            )))
        {
            PhSetTokenPrivilege(tokenHandle, L"SeDebugPrivilege", NULL, SE_PRIVILEGE_ENABLED);
            CloseHandle(tokenHandle);
        }
    }

    // Initialize the system image lists.
    {
        HMODULE shell32;
        _FileIconInit fileIconInit;

        shell32 = LoadLibrary(L"shell32.dll");

        if (shell32)
        {
            fileIconInit = (_FileIconInit)GetProcAddress(shell32, (PSTR)660);

            if (fileIconInit)
                fileIconInit(TRUE);

            FreeLibrary(shell32);
        }
    }

    // Initialize the providers.
    PhInitializeProviderThread(&PhPrimaryProviderThread, 1000);
    PhInitializeProviderThread(&PhSecondaryProviderThread, 1000);

    PhRegisterProvider(&PhPrimaryProviderThread, PhProcessProviderUpdate, NULL, &ProcessProviderRegistration);
    PhSetProviderEnabled(&ProcessProviderRegistration, TRUE);
    PhRegisterProvider(&PhPrimaryProviderThread, PhServiceProviderUpdate, NULL, &ServiceProviderRegistration);
    PhSetProviderEnabled(&ServiceProviderRegistration, TRUE);

    PhMainWndHandle = CreateWindow(
        PhWindowClassName,
        PH_APP_NAME,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        0,
        CW_USEDEFAULT,
        0,
        NULL,
        NULL,
        PhInstanceHandle,
        NULL
        );

    if (!PhMainWndHandle)
        return FALSE;

    PhInitializeFont(PhMainWndHandle);

    // Initialize child controls.
    PhMainWndOnCreate();

    PhMainWndTabControlOnSelectionChanged();

    // Perform a layout.
    PhMainWndOnLayout();

    PhStartProviderThread(&PhPrimaryProviderThread);
    PhStartProviderThread(&PhSecondaryProviderThread);

    ShowWindow(PhMainWndHandle, ShowCommand);

    return TRUE;
}

LRESULT CALLBACK PhMainWndProc(      
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    { 
    case WM_DESTROY:
        {
            PostQuitMessage(0);
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case ID_HACKER_EXIT:
                DestroyWindow(hWnd);
                break;
            case ID_PROCESS_PROPERTIES:
                {
                    INT index;
                    PPH_PROCESS_ITEM processItem;

                    index = PhFindListViewItemByFlags(
                        ProcessListViewHandle,
                        -1,
                        LVNI_SELECTED
                        );

                    if (index != -1)
                    {
                        if (PhGetListViewItemParam(
                            ProcessListViewHandle,
                            index,
                            &processItem
                            ))
                        {
                            PhpShowProcessProperties(processItem);
                        }
                    }
                }
                break;
            default:
                return DefWindowProc(hWnd, uMsg, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT paintStruct;

            hdc = BeginPaint(hWnd, &paintStruct);
            EndPaint(hWnd, &paintStruct);
        }
        break;
    case WM_SIZE:
        {
            if (wParam != SIZE_MINIMIZED)
            {
                PhMainWndOnLayout();
                InvalidateRect(hWnd, NULL, TRUE);
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->hwndFrom == TabControlHandle)
            {
                PhMainWndTabControlOnNotify(header);
            }
            else if (header->hwndFrom == ProcessListViewHandle)
            {
                PhMainWndProcessListViewOnNotify(header);
            }
        }
        break;
    case WM_PH_PROCESS_ADDED:
        {
            PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)lParam;

            PhMainWndOnProcessAdded(processItem);
            PhDereferenceObject(processItem);
        }
        break;
    case WM_PH_PROCESS_MODIFIED:
        {
            PhMainWndOnProcessModified((PPH_PROCESS_ITEM)lParam);
        }
        break;
    case WM_PH_PROCESS_REMOVED:
        {
            PhMainWndOnProcessRemoved((PPH_PROCESS_ITEM)lParam);
        }
        break;
    case WM_PH_SERVICE_ADDED:
        {
            PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)lParam;

            PhMainWndOnServiceAdded(serviceItem);
            PhDereferenceObject(serviceItem);
        }
        break;
    case WM_PH_SERVICE_MODIFIED:
        {
            PPH_SERVICE_MODIFIED_DATA serviceModifiedData = (PPH_SERVICE_MODIFIED_DATA)lParam;

            PhMainWndOnServiceModified(serviceModifiedData);
            PhFree(serviceModifiedData);
        }
        break;
    case WM_PH_SERVICE_REMOVED:
        {
            PhMainWndOnServiceRemoved((PPH_SERVICE_ITEM)lParam);
        }
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

VOID PhpShowProcessProperties(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_PROCESS_PROPCONTEXT propContext;

    propContext = PhCreateProcessPropContext(
        PhMainWndHandle,
        ProcessItem
        );

    if (propContext)
    {
        PhShowProcessProperties(propContext);
        PhDereferenceObject(propContext);
    }
}

static VOID NTAPI ProcessAddedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    // Reference the process item so it doesn't get deleted before 
    // we handle the event in the main thread.
    PhReferenceObject(processItem);
    PostMessage(PhMainWndHandle, WM_PH_PROCESS_ADDED, 0, (LPARAM)processItem);
}

static VOID NTAPI ProcessModifiedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    PostMessage(PhMainWndHandle, WM_PH_PROCESS_MODIFIED, 0, (LPARAM)processItem);
}

static VOID NTAPI ProcessRemovedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    // We already have a reference to the process item, so we don't need to 
    // reference it here.
    PostMessage(PhMainWndHandle, WM_PH_PROCESS_REMOVED, 0, (LPARAM)processItem);
}

static VOID NTAPI ServiceAddedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)Parameter;

    PhReferenceObject(serviceItem);
    PostMessage(PhMainWndHandle, WM_PH_SERVICE_ADDED, 0, (LPARAM)serviceItem);
}

static VOID NTAPI ServiceModifiedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_SERVICE_MODIFIED_DATA serviceModifiedData = (PPH_SERVICE_MODIFIED_DATA)Parameter;
    PPH_SERVICE_MODIFIED_DATA copy;

    copy = PhAllocateCopy(serviceModifiedData, sizeof(PH_SERVICE_MODIFIED_DATA));

    PostMessage(PhMainWndHandle, WM_PH_SERVICE_MODIFIED, 0, (LPARAM)copy);
}

static VOID NTAPI ServiceRemovedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)Parameter;

    PostMessage(PhMainWndHandle, WM_PH_SERVICE_REMOVED, 0, (LPARAM)serviceItem);
}

VOID PhMainWndOnCreate()
{
    TabControlHandle = PhCreateTabControl(PhMainWndHandle);
    ProcessesTabIndex = PhAddTabControlTab(TabControlHandle, 0, L"Processes");
    ServicesTabIndex = PhAddTabControlTab(TabControlHandle, 1, L"Services");
    NetworkTabIndex = PhAddTabControlTab(TabControlHandle, 2, L"Network");

    ProcessListViewHandle = PhCreateListViewControl(PhMainWndHandle, ID_MAINWND_PROCESSLV);
    ListView_SetExtendedListViewStyleEx(ProcessListViewHandle, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, -1);
    ServiceListViewHandle = PhCreateListViewControl(PhMainWndHandle, ID_MAINWND_SERVICELV);
    ListView_SetExtendedListViewStyleEx(ServiceListViewHandle, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, -1);
    NetworkListViewHandle = PhCreateListViewControl(PhMainWndHandle, ID_MAINWND_NETWORKLV);
    ListView_SetExtendedListViewStyleEx(NetworkListViewHandle, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, -1);

    PhSetControlTheme(ProcessListViewHandle, L"explorer");
    PhSetControlTheme(ServiceListViewHandle, L"explorer");
    PhSetControlTheme(NetworkListViewHandle, L"explorer");

    PhAddListViewColumn(ProcessListViewHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Name");
    PhAddListViewColumn(ProcessListViewHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"PID");
    PhAddListViewColumn(ProcessListViewHandle, 2, 2, 2, LVCFMT_LEFT, 140, L"User Name");
    PhAddListViewColumn(ProcessListViewHandle, 3, 3, 3, LVCFMT_LEFT, 300, L"File Name");
    PhAddListViewColumn(ProcessListViewHandle, 4, 4, 4, LVCFMT_LEFT, 300, L"Command Line");
    PhAddListViewColumn(ProcessListViewHandle, 5, 5, 5, LVCFMT_LEFT, 140, L"Verified Signer");
    PhAddListViewColumn(ServiceListViewHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Name");
    PhAddListViewColumn(ServiceListViewHandle, 1, 1, 1, LVCFMT_LEFT, 140, L"Display Name");
    PhAddListViewColumn(ServiceListViewHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"PID");
    PhAddListViewColumn(NetworkListViewHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Process Name");

    PhRegisterCallback(
        &PhProcessAddedEvent,
        ProcessAddedHandler,
        NULL,
        &ProcessAddedRegistration
        );
    PhRegisterCallback(
        &PhProcessModifiedEvent,
        ProcessModifiedHandler,
        NULL,
        &ProcessModifiedRegistration
        );
    PhRegisterCallback(
        &PhProcessRemovedEvent,
        ProcessRemovedHandler,
        NULL,
        &ProcessRemovedRegistration
        );

    PhRegisterCallback(
        &PhServiceAddedEvent,
        ServiceAddedHandler,
        NULL,
        &ServiceAddedRegistration
        );
    PhRegisterCallback(
        &PhServiceModifiedEvent,
        ServiceModifiedHandler,
        NULL,
        &ServiceModifiedRegistration
        );
    PhRegisterCallback(
        &PhServiceRemovedEvent,
        ServiceRemovedHandler,
        NULL,
        &ServiceRemovedRegistration
        );
}

VOID PhMainWndOnLayout()
{
    RECT rect;

    // Resize the tab control.
    GetClientRect(PhMainWndHandle, &rect);
    PhSetControlPosition(TabControlHandle, rect.left, rect.top, rect.right, rect.bottom);

    PhMainWndTabControlOnLayout();
}

VOID PhMainWndTabControlOnLayout()
{
    RECT rect;
    INT selectedIndex;

    GetClientRect(PhMainWndHandle, &rect);
    TabCtrl_AdjustRect(TabControlHandle, FALSE, &rect);

    selectedIndex = TabCtrl_GetCurSel(TabControlHandle);

    if (selectedIndex == ProcessesTabIndex)
    {
        PhSetControlPosition(ProcessListViewHandle, rect.left, rect.top, rect.right, rect.bottom);
    }
    else if (selectedIndex == ServicesTabIndex)
    {
        PhSetControlPosition(ServiceListViewHandle, rect.left, rect.top, rect.right, rect.bottom);
    }
    else if (selectedIndex == NetworkTabIndex)
    {
        PhSetControlPosition(NetworkListViewHandle, rect.left, rect.top, rect.right, rect.bottom);
    }
}

VOID PhMainWndTabControlOnNotify(
    __in LPNMHDR Header
    )
{
    if (Header->code == TCN_SELCHANGE)
    {
        PhMainWndTabControlOnSelectionChanged();
    }
}

VOID PhMainWndTabControlOnSelectionChanged()
{
    INT selectedIndex;

    selectedIndex = TabCtrl_GetCurSel(TabControlHandle);
    ShowWindow(ProcessListViewHandle, selectedIndex == ProcessesTabIndex ? SW_SHOW : SW_HIDE);
    ShowWindow(ServiceListViewHandle, selectedIndex == ServicesTabIndex ? SW_SHOW : SW_HIDE);
    ShowWindow(NetworkListViewHandle, selectedIndex == NetworkTabIndex ? SW_SHOW : SW_HIDE);

    PhMainWndTabControlOnLayout();
}

VOID PhMainWndProcessListViewOnNotify(
    __in LPNMHDR Header
    )
{
    if (Header->code == NM_DBLCLK)
    {
        LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)Header;

        if (itemActivate->iItem != -1)
        {
            PPH_PROCESS_ITEM processItem;

            if (PhGetListViewItemParam(
                ProcessListViewHandle,
                itemActivate->iItem,
                &processItem
                ))
            {
                PhpShowProcessProperties(processItem);
            }
        }
    }
    else if (Header->code == NM_RCLICK)
    {
        LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)Header;

        if (itemActivate->iItem != -1)
        {
            HMENU menu;
            HMENU subMenu;

            menu = LoadMenu(PhInstanceHandle, MAKEINTRESOURCE(IDR_MAINWND));
            subMenu = GetSubMenu(menu, 1);

            PhShowContextMenu(
                PhMainWndHandle,
                ProcessListViewHandle,
                subMenu,
                itemActivate->ptAction
                );
            DestroyMenu(menu);
        }
    }
}

VOID PhMainWndOnProcessAdded(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    INT lvItemIndex;

    // Add a reference for the pointer being stored in the list view item.
    PhReferenceObject(ProcessItem);

    lvItemIndex = PhAddListViewItem(
        ProcessListViewHandle,
        MAXINT,
        ProcessItem->ProcessName->Buffer,
        ProcessItem
        );
    PhSetListViewSubItem(ProcessListViewHandle, lvItemIndex, 1, ProcessItem->ProcessIdString);
    PhSetListViewSubItem(ProcessListViewHandle, lvItemIndex, 2, PhGetString(ProcessItem->UserName));
    PhSetListViewSubItem(ProcessListViewHandle, lvItemIndex, 3, PhGetString(ProcessItem->FileName));
    PhSetListViewSubItem(ProcessListViewHandle, lvItemIndex, 4, PhGetString(ProcessItem->CommandLine));
}

VOID PhMainWndOnProcessModified(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    INT lvItemIndex;

    lvItemIndex = PhFindListViewItemByParam(ProcessListViewHandle, -1, ProcessItem);

    if (lvItemIndex != -1)
    {
        PhSetListViewSubItem(ProcessListViewHandle, lvItemIndex, 5, PhGetString(ProcessItem->VerifySignerName));
    }
}

VOID PhMainWndOnProcessRemoved(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PhRemoveListViewItem(
        ProcessListViewHandle,
        PhFindListViewItemByParam(ProcessListViewHandle, -1, ProcessItem)
        );
    // Remove the reference we added in PhMainWndOnProcessAdded.
    PhDereferenceObject(ProcessItem);
}

VOID PhMainWndOnServiceAdded(
    __in PPH_SERVICE_ITEM ServiceItem
    )
{
    INT lvItemIndex;

    // Add a reference for the pointer being stored in the list view item.
    PhReferenceObject(ServiceItem);

    lvItemIndex = PhAddListViewItem(
        ServiceListViewHandle,
        MAXINT,
        ServiceItem->Name->Buffer,
        ServiceItem
        );
    PhSetListViewSubItem(ServiceListViewHandle, lvItemIndex, 1, PhGetString(ServiceItem->DisplayName));
    PhSetListViewSubItem(ServiceListViewHandle, lvItemIndex, 2, ServiceItem->ProcessIdString);
}

VOID PhMainWndOnServiceModified(
    __in PPH_SERVICE_MODIFIED_DATA ServiceModifiedData
    )
{
    INT lvItemIndex;

    lvItemIndex = PhFindListViewItemByParam(ServiceListViewHandle, -1, ServiceModifiedData->Service);
    PhSetListViewSubItem(ServiceListViewHandle, lvItemIndex, 2, ServiceModifiedData->Service->ProcessIdString);
}

VOID PhMainWndOnServiceRemoved(
    __in PPH_SERVICE_ITEM ServiceItem
    )
{
    PhRemoveListViewItem(
        ServiceListViewHandle,
        PhFindListViewItemByParam(ServiceListViewHandle, -1, ServiceItem)
        );
    // Remove the reference we added in PhMainWndOnServiceAdded.
    PhDereferenceObject(ServiceItem);
}
