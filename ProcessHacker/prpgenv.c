/*
 * Process Hacker -
 *   Process properties: Environment page
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
#include <procprv.h>
#include <uxtheme.h>
#include <procprpp.h>

INT_PTR CALLBACK PhpProcessEnvironmentDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_ENVIRONMENT_CONTEXT environmentContext;
    HWND lvHandle;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        environmentContext = propPageContext->Context;

        if (environmentContext)
            lvHandle = environmentContext->ListViewHandle;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE processHandle;
            PVOID environment;
            ULONG environmentLength;
            ULONG enumerationKey;
            PH_ENVIRONMENT_VARIABLE variable;

            environmentContext = propPageContext->Context = PhAllocate(sizeof(PH_ENVIRONMENT_CONTEXT));
            memset(environmentContext, 0, sizeof(PH_ENVIRONMENT_CONTEXT));
            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            environmentContext->ListViewHandle = lvHandle;
            environmentContext->Values = PhCreateList(100);

            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 140, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Value");

            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"EnvironmentListViewColumns", lvHandle);

            EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), FALSE);

            if (NT_SUCCESS(PhOpenProcess(
                &processHandle,
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                processItem->ProcessId
                )))
            {
                ULONG flags;

                flags = 0;

#ifdef _WIN64
                if (processItem->IsWow64)
                    flags |= PH_GET_PROCESS_ENVIRONMENT_WOW64;
#endif

                if (NT_SUCCESS(PhGetProcessEnvironment(
                    processHandle,
                    flags,
                    &environment,
                    &environmentLength
                    )))
                {
                    enumerationKey = 0;

                    while (PhEnumProcessEnvironmentVariables(environment, environmentLength, &enumerationKey, &variable))
                    {
                        INT lvItemIndex;
                        PPH_STRING nameString;
                        PPH_STRING valueString;

                        // Don't display pairs with no name.
                        if (variable.Name.Length == 0)
                            continue;

                        // The strings are not guaranteed to be null-terminated, so we need to create some temporary strings.
                        nameString = PhCreateString2(&variable.Name);
                        valueString = PhCreateString2(&variable.Value);

                        lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, nameString->Buffer, valueString);
                        PhSetListViewSubItem(lvHandle, lvItemIndex, 1, valueString->Buffer);

                        PhDereferenceObject(nameString);
                        PhAddItemList(environmentContext->Values, valueString);
                    }

                    PhFreePage(environment);
                }

                NtClose(processHandle);
            }

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"EnvironmentListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));

            PhDereferenceObjects(environmentContext->Values->Items, environmentContext->Values->Count);
            PhDereferenceObject(environmentContext->Values);
            PhFree(environmentContext);

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
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_NEW),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_EDIT),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_DELETE),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

                PhDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case ID_ENVIRONMENT_VIEW:
                {
                    PPH_STRING *items;
                    ULONG numberOfItems;

                    PhGetSelectedListViewItemParams(lvHandle, &items, &numberOfItems);

                    if (numberOfItems == 1)
                    {
                        PhShowInformationDialog(hwndDlg, items[0]->Buffer, 0);
                    }
                    else if (numberOfItems > 1)
                    {
                        PhShowInformationDialog(hwndDlg, PH_AUTO_T(PH_STRING, PhGetListViewText(lvHandle))->Buffer, 0);
                    }

                    PhFree(items);
                }
                break;
            case ID_ENVIRONMENT_COPY:
                {
                    PhCopyListView(lvHandle);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            PhHandleListViewNotifyBehaviors(lParam, lvHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);

            switch (header->code)
            {
            case LVN_ITEMCHANGED:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        ULONG selectedCount;

                        selectedCount = ListView_GetSelectedCount(lvHandle);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), selectedCount == 1);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), selectedCount == 1);
                    }
                }
                break;
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        SendMessage(hwndDlg, WM_COMMAND, ID_ENVIRONMENT_VIEW, 0);
                    }
                }
                break;
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == lvHandle)
            {
                POINT point;
                PPH_STRING *items;
                ULONG numberOfItems;

                point.x = (SHORT)LOWORD(lParam);
                point.y = (SHORT)HIWORD(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint((HWND)wParam, &point);

                PhGetSelectedListViewItemParams(lvHandle, &items, &numberOfItems);

                if (numberOfItems != 0)
                {
                    PPH_EMENU menu;

                    menu = PhCreateEMenu();
                    PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_ENVIRONMENT), 0);
                    PhSetFlagsEMenuItem(menu, ID_ENVIRONMENT_VIEW, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

                    PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );
                    PhDestroyEMenu(menu);
                }

                PhFree(items);
            }
        }
        break;
    }

    return FALSE;
}
