/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2021 dmex
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

#include <peview.h>
#include "metahost.h"

typedef struct _PVP_PE_CLR_IMPORTS_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_CLR_IMPORTS_CONTEXT, *PPVP_PE_CLR_IMPORTS_CONTEXT;

VOID PvpEnumerateClrImports(
    _In_ HWND ListViewHandle
    )
{
    PPH_LIST clrImportsList = NULL;
    ICLRMetaHost* clrMetaHost = NULL;
    ICLRRuntimeInfo* clrRuntimInfo = NULL;
    ICLRStrongName* clrStrongName = NULL;
    CLRCreateInstanceFnPtr CLRCreateInstance_I = NULL;
    PVOID mscoreeHandle = NULL;
    ULONG clrTokenLength = 0;
    ULONG clrKeyLength = 0;
    PBYTE clrToken = NULL;
    PBYTE clrKey = NULL;
    ULONG size = MAX_PATH;
    WCHAR version[MAX_PATH] = L"";

    if (mscoreeHandle = PhLoadLibrarySafe(L"mscoree.dll"))
    {
        if (CLRCreateInstance_I = PhGetDllBaseProcedureAddress(mscoreeHandle, "CLRCreateInstance", 0))
        {
            if (!SUCCEEDED(CLRCreateInstance_I(&CLSID_CLRMetaHost, &IID_ICLRMetaHost, &clrMetaHost)))
                goto CleanupExit;
        }
        else
        {
            goto CleanupExit;
        }
    }
    else
    {
        goto CleanupExit;
    }

    if (!SUCCEEDED(ICLRMetaHost_GetVersionFromFile(
        clrMetaHost,
        PvFileName->Buffer,
        version,
        &size
        )))
    {
        goto CleanupExit;
    }

    if (!SUCCEEDED(ICLRMetaHost_GetRuntime(
        clrMetaHost,
        version,
        &IID_ICLRRuntimeInfo,
        &clrRuntimInfo
        )))
    {
        goto CleanupExit;
    }

    if (SUCCEEDED(PvGetClrImageImports(clrRuntimInfo, PvFileName->Buffer, &clrImportsList)))
    {
        ULONG count = 0;
        ULONG i;
        ULONG j;

        for (i = 0; i < clrImportsList->Count; i++)
        {
            PPV_CLR_IMAGE_IMPORT_DLL importDll = clrImportsList->Items[i];

            if (importDll->Functions)
            {
                for (j = 0; j < importDll->Functions->Count; j++)
                {
                    PPV_CLR_IMAGE_IMPORT_FUNCTION importFunction = importDll->Functions->Items[j];
                    INT lvItemIndex;
                    WCHAR value[PH_INT64_STR_LEN_1];

                    PhPrintUInt32(value, ++count);
                    lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, value, NULL);

                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, PhGetString(importDll->ImportName));
                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(importFunction->FunctionName));
                    if (importFunction->Flags)
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, PH_AUTO_T(PH_STRING, PvClrImportFlagsToString(importFunction->Flags))->Buffer);

                    PhClearReference(&importFunction->FunctionName);
                    PhFree(importFunction);
                }

                PhDereferenceObject(importDll->Functions);
            }

            PhClearReference(&importDll->ImportName);
            PhFree(importDll);
        }

        PhDereferenceObject(clrImportsList);
    }

CleanupExit:

    if (clrRuntimInfo)
        ICLRRuntimeInfo_Release(clrRuntimInfo);
    if (clrMetaHost)
        ICLRMetaHost_Release(clrMetaHost);
    if (mscoreeHandle)
        FreeLibrary(mscoreeHandle);
}

INT_PTR CALLBACK PvpPeClrImportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_CLR_IMPORTS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_CLR_IMPORTS_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;
        }
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"DLL");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 250, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Flags");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageClrImportsListViewColumns", context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_CLRGROUP), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_FLAGS), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_MVIDSTRING), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_TOKENSTRING), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);         
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvpEnumerateClrImports(context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageClrImportsListViewColumns", context->ListViewHandle);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (context->PropSheetContext && !context->PropSheetContext->LayoutInitialized)
            {
                PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                context->PropSheetContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)GetDlgItem(hwndDlg, IDC_RUNTIMEVERSION));
                return TRUE;
            }

            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
        }
        break;
    }

    return FALSE;
}
