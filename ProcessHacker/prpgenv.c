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

typedef struct _EDIT_ENV_DIALOG_CONTEXT
{
    PPH_PROCESS_ITEM ProcessItem;
    PWSTR Name;
    PWSTR Value;
    BOOLEAN Refresh;

    PH_LAYOUT_MANAGER LayoutManager;
    RECT MinimumSize;
} EDIT_ENV_DIALOG_CONTEXT, *PEDIT_ENV_DIALOG_CONTEXT;

INT_PTR PhpShowEditEnvDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PWSTR Name,
    _In_opt_ PWSTR Value,
    _Out_opt_ PBOOLEAN Refresh
    );

INT_PTR CALLBACK PhpEditEnvDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

static VOID PhpClearEnvironmentItems(
    _Inout_ PPH_ENVIRONMENT_CONTEXT Context
    )
{
    ULONG i;
    PPH_ENVIRONMENT_ITEM item;

    for (i = 0; i < Context->Items.Count; i++)
    {
        item = PhItemArray(&Context->Items, i);
        PhDereferenceObject(item->Name);
        PhDereferenceObject(item->Value);
    }

    PhClearArray(&Context->Items);
}

static VOID PhpRefreshEnvironment(
    _In_ HWND hwndDlg,
    _Inout_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PVOID selectedIndex;
    HANDLE processHandle;
    PVOID environment;
    ULONG environmentLength;
    ULONG enumerationKey;
    PH_ENVIRONMENT_VARIABLE variable;

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);

    selectedIndex = PhGetSelectedListViewItemParam(Context->ListViewHandle);
    ListView_DeleteAllItems(Context->ListViewHandle);
    PhpClearEnvironmentItems(Context);

    if (NT_SUCCESS(PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        ProcessItem->ProcessId
        )))
    {
        ULONG flags;

        flags = 0;

#ifdef _WIN64
        if (ProcessItem->IsWow64)
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
                PH_ENVIRONMENT_ITEM item;
                INT lvItemIndex;

                // Don't display pairs with no name.
                if (variable.Name.Length == 0)
                    continue;

                // The strings are not guaranteed to be null-terminated, so we need to create some temporary strings.
                item.Name = PhCreateString2(&variable.Name);
                item.Value = PhCreateString2(&variable.Value);

                lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, item.Name->Buffer,
                    UlongToPtr((ULONG)Context->Items.Count + 1));
                PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, item.Value->Buffer);

                PhAddItemArray(&Context->Items, &item);
            }

            PhFreePage(environment);
        }

        NtClose(processHandle);
    }

    if (selectedIndex)
    {
        ListView_SetItemState(Context->ListViewHandle, PtrToUlong(selectedIndex) - 1,
            LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        ListView_EnsureVisible(Context->ListViewHandle, PtrToUlong(selectedIndex) - 1, FALSE);
    }

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

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
            environmentContext = propPageContext->Context = PhAllocate(sizeof(PH_ENVIRONMENT_CONTEXT));
            memset(environmentContext, 0, sizeof(PH_ENVIRONMENT_CONTEXT));
            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            environmentContext->ListViewHandle = lvHandle;
            PhInitializeArray(&environmentContext->Items, sizeof(PH_ENVIRONMENT_ITEM), 100);

            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 140, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Value");

            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"EnvironmentListViewColumns", lvHandle);

            EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), FALSE);

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);

            PhpRefreshEnvironment(hwndDlg, environmentContext, processItem);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"EnvironmentListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));

            PhpClearEnvironmentItems(environmentContext);
            PhDeleteArray(&environmentContext->Items);

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
            case IDC_NEW:
                {
                    BOOLEAN refresh;

                    if (PhpShowEditEnvDialog(hwndDlg, processItem, L"", NULL, &refresh) == IDOK &&
                        refresh)
                    {
                        PhpRefreshEnvironment(hwndDlg, environmentContext, processItem);
                    }
                }
                break;
            case IDC_EDIT:
            case ID_ENVIRONMENT_EDIT:
                {
                    PVOID index = PhGetSelectedListViewItemParam(lvHandle);

                    if (index)
                    {
                        PPH_ENVIRONMENT_ITEM item = PhItemArray(&environmentContext->Items, PtrToUlong(index) - 1);
                        BOOLEAN refresh;

                        if (PhpShowEditEnvDialog(hwndDlg, processItem, item->Name->Buffer,
                            item->Value->Buffer, &refresh) == IDOK && refresh)
                        {
                            PhpRefreshEnvironment(hwndDlg, environmentContext, processItem);
                        }
                    }
                }
                break;
            case IDC_DELETE:
            case ID_ENVIRONMENT_DELETE:
                {
                    NTSTATUS status;
                    PVOID *indices;
                    ULONG numberOfIndices;
                    HANDLE processHandle;
                    LARGE_INTEGER timeout;
                    ULONG i;
                    PPH_ENVIRONMENT_ITEM item;

                    PhGetSelectedListViewItemParams(lvHandle, &indices, &numberOfIndices);

                    if (numberOfIndices != 0 && PhShowConfirmMessage(
                        hwndDlg,
                        L"delete",
                        numberOfIndices != 1 ? L"the selected environment variables" : L"the selected environment variable",
                        NULL,
                        FALSE))
                    {
                        if (NT_SUCCESS(status = PhOpenProcess(
                            &processHandle,
                            ProcessQueryAccess | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
                            PROCESS_VM_READ | PROCESS_VM_WRITE,
                            processItem->ProcessId
                            )))
                        {
                            timeout.QuadPart = -10 * PH_TIMEOUT_SEC;

                            for (i = 0; i < numberOfIndices; i++)
                            {
                                item = PhItemArray(&environmentContext->Items, PtrToUlong(indices[i]) - 1);
                                PhSetEnvironmentVariableRemote(processHandle, &item->Name->sr, NULL, &timeout);
                            }

                            NtClose(processHandle);

                            PhpRefreshEnvironment(hwndDlg, environmentContext, processItem);
                        }
                        else
                        {
                            PhShowStatus(hwndDlg, L"Unable to open the process", status, 0);
                        }
                    }

                    PhFree(indices);
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
                        EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), selectedCount >= 1);
                    }
                }
                break;
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        SendMessage(hwndDlg, WM_COMMAND, ID_ENVIRONMENT_EDIT, 0);
                    }
                }
                break;
            case LVN_KEYDOWN:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        LPNMLVKEYDOWN keyDown = (LPNMLVKEYDOWN)header;

                        switch (keyDown->wVKey)
                        {
                        case VK_DELETE:
                            SendMessage(hwndDlg, WM_COMMAND, ID_ENVIRONMENT_DELETE, 0);
                            break;
                        }
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
                PVOID *indices;
                ULONG numberOfIndices;

                point.x = (SHORT)LOWORD(lParam);
                point.y = (SHORT)HIWORD(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint((HWND)wParam, &point);

                PhGetSelectedListViewItemParams(lvHandle, &indices, &numberOfIndices);

                if (numberOfIndices != 0)
                {
                    PPH_EMENU menu;

                    menu = PhCreateEMenu();
                    PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_ENVIRONMENT), 0);
                    PhSetFlagsEMenuItem(menu, ID_ENVIRONMENT_EDIT, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

                    if (numberOfIndices > 1)
                        PhEnableEMenuItem(menu, ID_ENVIRONMENT_EDIT, FALSE);

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

                PhFree(indices);
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PhpEditEnvDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PEDIT_ENV_DIALOG_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PEDIT_ENV_DIALOG_CONTEXT)lParam;
        SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)context);
    }
    else
    {
        context = (PEDIT_ENV_DIALOG_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());

        if (uMsg == WM_DESTROY)
        {
            RemoveProp(hwndDlg, PhMakeContextAtom());
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_NAME), NULL,
                PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_VALUE), NULL,
                PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL,
                PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL,
                PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhLayoutManagerLayout(&context->LayoutManager);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 200;
            context->MinimumSize.bottom = 140;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            SetDlgItemText(hwndDlg, IDC_NAME, context->Name);
            SetDlgItemText(hwndDlg, IDC_VALUE, context->Value ? context->Value : L"");

            if (context->Value)
            {
                SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwndDlg, IDC_VALUE), TRUE);
            }
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                {
                    EndDialog(hwndDlg, IDCANCEL);
                }
                break;
            case IDOK:
                {
                    NTSTATUS status;
                    PPH_STRING name = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_NAME)));
                    PPH_STRING value = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_VALUE)));
                    HANDLE processHandle;
                    LARGE_INTEGER timeout;

                    if (!PhIsNullOrEmptyString(name))
                    {
                        if (!PhEqualString2(name, context->Name, FALSE) ||
                            !context->Value || !PhEqualString2(value, context->Value, FALSE))
                        {
                            if (NT_SUCCESS(status = PhOpenProcess(
                                &processHandle,
                                ProcessQueryAccess | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
                                PROCESS_VM_READ | PROCESS_VM_WRITE,
                                context->ProcessItem->ProcessId
                                )))
                            {
                                timeout.QuadPart = -10 * PH_TIMEOUT_SEC;

                                // Delete the old environment variable if necessary.
                                if (!PhEqualStringZ(context->Name, L"", FALSE) &&
                                    !PhEqualString2(name, context->Name, FALSE))
                                {
                                    PH_STRINGREF nameSr;

                                    PhInitializeStringRefLongHint(&nameSr, context->Name);
                                    PhSetEnvironmentVariableRemote(
                                        processHandle,
                                        &nameSr,
                                        NULL,
                                        &timeout
                                        );
                                }

                                status = PhSetEnvironmentVariableRemote(
                                    processHandle,
                                    &name->sr,
                                    &value->sr,
                                    &timeout
                                    );

                                NtClose(processHandle);
                            }

                            if (!NT_SUCCESS(status))
                            {
                                PhShowStatus(hwndDlg, L"Unable to set the environment variable", status, 0);
                                break;
                            }

                            context->Refresh = TRUE;
                        }

                        EndDialog(hwndDlg, IDOK);
                    }
                }
                break;
            case IDC_NAME:
                {
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        EnableWindow(GetDlgItem(hwndDlg, IDOK), GetWindowTextLength(GetDlgItem(hwndDlg, IDC_NAME)) > 0);
                    }
                }
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
            PhResizingMinimumSize((PRECT)lParam, wParam, context->MinimumSize.right, context->MinimumSize.bottom);
        }
        break;
    }

    return FALSE;
}

INT_PTR PhpShowEditEnvDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PWSTR Name,
    _In_opt_ PWSTR Value,
    _Out_opt_ PBOOLEAN Refresh
    )
{
    INT_PTR result;
    EDIT_ENV_DIALOG_CONTEXT context;

    memset(&context, 0, sizeof(EDIT_ENV_DIALOG_CONTEXT));
    context.ProcessItem = ProcessItem;
    context.Name = Name;
    context.Value = Value;

    result = DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_EDITENV),
        ParentWindowHandle,
        PhpEditEnvDlgProc,
        (LPARAM)&context
        );

    if (Refresh)
        *Refresh = context.Refresh;

    return result;
}
