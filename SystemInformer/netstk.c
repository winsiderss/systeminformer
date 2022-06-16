/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *
 */

#include <phapp.h>
#include <symprv.h>
#include <netprv.h>

typedef struct NETWORK_STACK_CONTEXT
{
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPH_NETWORK_ITEM NetworkItem;
    PPH_SYMBOL_PROVIDER SymbolProvider;
    HANDLE LoadingProcessId;
} NETWORK_STACK_CONTEXT, *PNETWORK_STACK_CONTEXT;

INT_PTR CALLBACK PhpNetworkStackDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

static RECT MinimumSize = { -1, -1, -1, -1 };

static BOOLEAN LoadSymbolsEnumGenericModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
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
        Module->FileName,
        (ULONG64)Module->BaseAddress,
        Module->Size
        );

    return TRUE;
}

VOID PhShowNetworkStackDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_NETWORK_ITEM NetworkItem
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
        PhShowError(ParentWindowHandle, L"%s", L"Unable to open the process.");
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PNETWORK_STACK_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PNETWORK_STACK_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;
            PVOID address;
            ULONG i;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            context->ListViewHandle = lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 350, L"Name");
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, lvHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (MinimumSize.left == -1)
            {
                RECT rect;

                rect.left = 0;
                rect.top = 0;
                rect.right = 190;
                rect.bottom = 120;
                MapDialogRect(hwndDlg, &rect);
                MinimumSize = rect;
                MinimumSize.left = 0;
            }

            for (i = 0; i < PH_NETWORK_OWNER_INFO_SIZE; i++)
            {
                PPH_STRING name;

                address = *(PVOID *)&context->NetworkItem->OwnerInfo[i];

                if ((ULONG_PTR)address < PAGE_SIZE) // stop at an invalid address
                    break;

                name = PhGetSymbolFromAddress(
                    context->SymbolProvider,
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
            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
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
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, MinimumSize.right, MinimumSize.bottom);
        }
        break;
    }

    return FALSE;
}
