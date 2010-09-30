/*
 * Process Hacker - 
 *   network stack viewer
 * 
 * Copyright (C) 2010 wj32
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

typedef struct NETWORK_STACK_CONTEXT
{
    HWND ListViewHandle;
    PPH_NETWORK_ITEM NetworkItem;
    PPH_SYMBOL_PROVIDER SymbolProvider;
    HANDLE LoadingProcessId;
} NETWORK_STACK_CONTEXT, *PNETWORK_STACK_CONTEXT;

INT_PTR CALLBACK PhpNetworkStackDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

static BOOLEAN LoadSymbolsEnumGenericModulesCallback(
    __in PPH_MODULE_INFO Module,
    __in_opt PVOID Context
    )
{
    PNETWORK_STACK_CONTEXT context = Context;
    PPH_SYMBOL_PROVIDER symbolProvider = context->SymbolProvider;

    // If we're loading kernel module symbols for a process other than 
    // System, ignore modules which are in user space. This may happen 
    // in Windows 7.
    if (
        context->LoadingProcessId == SYSTEM_PROCESS_ID &&
        context->NetworkItem->ProcessId != SYSTEM_PROCESS_ID &&
        (ULONG_PTR)Module->BaseAddress <= PhSystemBasicInformation.MaximumUserModeAddress
        )
        return TRUE;

    PhLoadModuleSymbolProvider(
        symbolProvider,
        Module->FileName->Buffer,
        (ULONG64)Module->BaseAddress,
        Module->Size
        );

    return TRUE;
}

VOID PhShowNetworkStackDialog(
    __in HWND ParentWindowHandle,
    __in PPH_NETWORK_ITEM NetworkItem
    )
{
    NETWORK_STACK_CONTEXT networkStackContext;

    networkStackContext.NetworkItem = NetworkItem;
    networkStackContext.SymbolProvider = PhCreateSymbolProvider(NetworkItem->ProcessId);

    if (networkStackContext.SymbolProvider->IsRealHandle)
    {
        // Load symbols for the process.
        networkStackContext.LoadingProcessId = NetworkItem->ProcessId;
        PhEnumGenericModules(
            NetworkItem->ProcessId,
            networkStackContext.SymbolProvider->ProcessHandle,
            0,
            LoadSymbolsEnumGenericModulesCallback,
            &networkStackContext
            );
        // Load symbols for kernel-mode.
        networkStackContext.LoadingProcessId = SYSTEM_PROCESS_ID;
        PhEnumGenericModules(
            SYSTEM_PROCESS_ID,
            NULL,
            0,
            LoadSymbolsEnumGenericModulesCallback,
            &networkStackContext
            );
    }
    else
    {
        PhDereferenceObject(networkStackContext.SymbolProvider);
        PhShowError(ParentWindowHandle, L"Unable to open the process.");
        return;
    }

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_NETSTACK),
        ParentWindowHandle,
        PhpNetworkStackDlgProc,
        (LPARAM)&networkStackContext
        );

    PhDereferenceObject(networkStackContext.SymbolProvider);
} 

static INT_PTR CALLBACK PhpNetworkStackDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PNETWORK_STACK_CONTEXT networkStackContext;
            HWND lvHandle;
            PPH_LAYOUT_MANAGER layoutManager;
            PVOID address;
            ULONG i;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            networkStackContext = (PNETWORK_STACK_CONTEXT)lParam;
            SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)networkStackContext);

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 350, L"Name");
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");

            networkStackContext->ListViewHandle = lvHandle;

            layoutManager = PhAllocate(sizeof(PH_LAYOUT_MANAGER));
            PhInitializeLayoutManager(layoutManager, hwndDlg);
            SetProp(hwndDlg, L"LayoutManager", (HANDLE)layoutManager);

            PhAddLayoutItem(layoutManager, lvHandle, NULL,
                PH_ANCHOR_ALL);
            PhAddLayoutItem(layoutManager, GetDlgItem(hwndDlg, IDOK),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            for (i = 0; i < PH_NETWORK_OWNER_INFO_SIZE; i++)
            {
                PPH_STRING name;

                address = *(PPVOID)&networkStackContext->NetworkItem->OwnerInfo[i];

                if ((ULONG_PTR)address < PAGE_SIZE) // stop at an invalid address
                    break;

                name = PhGetSymbolFromAddress(
                    networkStackContext->SymbolProvider,
                    (ULONG64)address,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );
                PhAddListViewItem(lvHandle, MAXINT, name->Buffer, NULL);
                PhDereferenceObject(name);
            }
        }
        break;
    case WM_DESTROY:
        {
            PPH_LAYOUT_MANAGER layoutManager;
            PNETWORK_STACK_CONTEXT networkStackContext;

            layoutManager = (PPH_LAYOUT_MANAGER)GetProp(hwndDlg, L"LayoutManager");
            PhDeleteLayoutManager(layoutManager);
            PhFree(layoutManager);

            networkStackContext = (PNETWORK_STACK_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());

            RemoveProp(hwndDlg, PhMakeContextAtom());
            RemoveProp(hwndDlg, L"LayoutManager");
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case IDCANCEL: // Esc and X button to close
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            PPH_LAYOUT_MANAGER layoutManager;

            layoutManager = (PPH_LAYOUT_MANAGER)GetProp(hwndDlg, L"LayoutManager");
            PhLayoutManagerLayout(layoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, 250, 350);
        }
        break;
    }

    return FALSE;
}
