/*
 * Process Hacker Extra Plugins -
 *   Plugin Manager
 *
 * Copyright (C) 2016 dmex
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

#include "main.h"

VOID PhAddDisabledPlugins(
    _In_ PPLUGIN_DISABLED_CONTEXT Context
    )
{
    PPH_STRING disabled;
    PH_STRINGREF remainingPart;
    PH_STRINGREF part;
    PPH_STRING displayText;
    INT lvItemIndex;

    disabled = PhGetStringSetting(L"DisabledPlugins");
    remainingPart = disabled->sr;

    while (remainingPart.Length)
    {
        PhSplitStringRefAtChar(&remainingPart, '|', &part, &remainingPart);

        if (part.Length)
        {
            displayText = PhCreateString2(&part);

            PhAcquireQueuedLockExclusive(&Context->ListLock);
            lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, PhGetString(displayText), displayText);
            PhReleaseQueuedLockExclusive(&Context->ListLock);

            ListView_SetCheckState(Context->ListViewHandle, lvItemIndex, TRUE);
        }
    }

    PhDereferenceObject(disabled);
}

INT_PTR CALLBACK DisabledPluginsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPLUGIN_DISABLED_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPLUGIN_DISABLED_CONTEXT)PhAllocate(sizeof(PLUGIN_DISABLED_CONTEXT));
        memset(context, 0, sizeof(PLUGIN_DISABLED_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PPLUGIN_DISABLED_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            PhDeleteLayoutManager(&context->LayoutManager);
            PhUnregisterDialog(hwndDlg);
            RemoveProp(hwndDlg, L"Context");
            PhFree(context);
        }
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->DialogHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST_DISABLED);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            ListView_SetExtendedListViewStyleEx(context->ListViewHandle, 
                LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, 
                LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 400, L"Property");
            PhSetExtendedListView(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);       
            
            PhAddDisabledPlugins(context);

            SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwndDlg, IDOK), TRUE);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                {
                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->code == LVN_ITEMCHANGED)
            {
                LPNM_LISTVIEW listView = (LPNM_LISTVIEW)lParam;

                if (!PhTryAcquireReleaseQueuedLockExclusive(&context->ListLock))
                    break;

                if (listView->uChanged & LVIF_STATE)
                {
                    switch (listView->uNewState & LVIS_STATEIMAGEMASK)
                    {
                    case INDEXTOSTATEIMAGEMASK(2): // checked
                        {
                            PPH_STRING param = (PPH_STRING)listView->lParam;

                            PhSetPluginDisabled(&param->sr, TRUE);

                            /*if (existingNode = FindTreeNode(
                                &context->TreeContext,
                                PLUGIN_STATE_LOCAL,
                                selectedNode->InternalName
                                ))
                            {
                                existingNode->State = PLUGIN_STATE_RESTART;
                            }*/

                            //entry->State = PLUGIN_STATE_LOCAL;

                            //context->OptionsChanged = TRUE;
                        }
                        break;
                    case INDEXTOSTATEIMAGEMASK(1): // unchecked
                        {
                            PPH_STRING param = (PPH_STRING)listView->lParam;

                            PhSetPluginDisabled(&param->sr, FALSE);

                            //context->OptionsChanged = TRUE;
                        }
                        break;
                    }
                }
            }
        }
        break;
    }

    return FALSE;
}

VOID ShowDisabledPluginsDialog(
    _In_ HWND Parent
    )
{
    DialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_DIALOG1),
        Parent,
        DisabledPluginsDlgProc
        );
}