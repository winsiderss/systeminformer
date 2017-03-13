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

BOOLEAN WordMatchStringRef(
    _In_ PWCT_CONTEXT Context,
    _In_ PPH_STRINGREF Text
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    remainingPart = Context->SearchboxText->sr;

    while (remainingPart.Length)
    {
        PhSplitStringRefAtChar(&remainingPart, '|', &part, &remainingPart);

        if (part.Length)
        {
            if (PhFindStringInStringRef(Text, &part, TRUE) != -1)
                return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN WordMatchStringZ(
    _In_ PWCT_CONTEXT Context,
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRef(&text, Text);
    return WordMatchStringRef(Context, &text);
}

VOID UpdateTreeView(
    _In_ PWCT_CONTEXT Context
    )
{
    //PhMoveReference(&Context->TreeText, PhCreateString(L"Loading Plugins..."));
    //TreeNew_SetEmptyText(Context->TreeNewHandle, &Context->TreeText->sr, 0);

    PhApplyTreeNewFilters(GetPluginListFilterSupport(Context));
    TreeNew_AutoSizeColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_NAME, TN_AUTOSIZE_REMAINING_SPACE);
}

VOID UpdateMenuView(_In_ PWCT_CONTEXT Context, UINT Id)
{
    HWND active = Context->PluginMenuActive;
    Context->PluginMenuActiveId = Id;
    Context->PluginMenuActive = GetDlgItem(Context->DialogHandle, Id);
    RedrawWindow(active, NULL, NULL, RDW_ERASENOW | RDW_INVALIDATE | RDW_FRAME);
}

LRESULT DrawButton(
    _In_ PWCT_CONTEXT Context, 
    _In_ LPARAM lParam
    )
{
    LPNMTVCUSTOMDRAW drawInfo = (LPNMTVCUSTOMDRAW)lParam;
    BOOLEAN isGrayed = (drawInfo->nmcd.uItemState & CDIS_GRAYED) == CDIS_GRAYED;
    BOOLEAN isChecked = (drawInfo->nmcd.uItemState & CDIS_CHECKED) == CDIS_CHECKED;
    BOOLEAN isDisabled = (drawInfo->nmcd.uItemState & CDIS_DISABLED) == CDIS_DISABLED;
    BOOLEAN isSelected = (drawInfo->nmcd.uItemState & CDIS_SELECTED) == CDIS_SELECTED;
    BOOLEAN isHighlighted = (drawInfo->nmcd.uItemState & CDIS_HOT) == CDIS_HOT;
    BOOLEAN isFocused = (drawInfo->nmcd.uItemState & CDIS_FOCUS) == CDIS_FOCUS;

    if (drawInfo->nmcd.dwDrawStage == CDDS_PREPAINT)
    {
        HPEN PenActive = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
        //HPEN PenNormal = CreatePen(PS_SOLID, 1, RGB(0, 0xff, 0));
        HBRUSH BrushNormal = GetSysColorBrush(COLOR_3DFACE);
        //HBRUSH BrushSelected = CreateSolidBrush(RGB(0xff, 0xff, 0xff));
        HBRUSH BrushPushed = CreateSolidBrush(RGB(153, 209, 255));
        PPH_STRING windowText = PH_AUTO(PhGetWindowText(drawInfo->nmcd.hdr.hwndFrom));

        SetBkMode(drawInfo->nmcd.hdc, TRANSPARENT);

        if (isHighlighted || Context->PluginMenuActiveId == drawInfo->nmcd.hdr.idFrom)
        {
            FillRect(drawInfo->nmcd.hdc, &drawInfo->nmcd.rc, BrushNormal);

            SelectObject(drawInfo->nmcd.hdc, PenActive);
            SelectObject(drawInfo->nmcd.hdc, Context->BoldFontHandle);
        }
        else if (isSelected)
        {
            FillRect(drawInfo->nmcd.hdc, &drawInfo->nmcd.rc, BrushPushed);
           
            SelectObject(drawInfo->nmcd.hdc, PenActive);
            SelectObject(drawInfo->nmcd.hdc, Context->BoldFontHandle);
        }
        else
        {
            FillRect(drawInfo->nmcd.hdc, &drawInfo->nmcd.rc, BrushNormal);
        }

        DrawText(
            drawInfo->nmcd.hdc,
            windowText->Buffer,
            (ULONG)windowText->Length / 2,
            &drawInfo->nmcd.rc,
            DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE
            );

        MoveToEx(drawInfo->nmcd.hdc, drawInfo->nmcd.rc.left, drawInfo->nmcd.rc.bottom, 0);
        LineTo(drawInfo->nmcd.hdc, drawInfo->nmcd.rc.right, drawInfo->nmcd.rc.bottom);

        DeletePen(PenActive);
        DeleteBrush(BrushNormal);
        DeleteBrush(BrushPushed);
        return CDRF_SKIPDEFAULT;
    }

    return CDRF_DODEFAULT;
}

BOOLEAN ProcessTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PWCT_CONTEXT context = Context;
    PPLUGIN_NODE node = (PPLUGIN_NODE)Node;

    // Hide plugins from different tabs
    if (node->State != context->Type)
        return FALSE;

    // Hide uninstalled plugins from the local tab
    //if (node->Type == PLUGIN_VIEW_LOCAL && node->State == PLUGIN_STATE_UNINSTALLED)
    //    return FALSE;

    //// Hide installed plugins from the remote tab
    //if (node->Type == PLUGIN_VIEW_REMOTE && node->State == PLUGIN_STATE_INSTALLED)
    //    return FALSE;

    if (PhIsNullOrEmptyString(context->SearchboxText))
        return TRUE;

    if (!PhIsNullOrEmptyString(node->InternalName))
    {
        if (WordMatchStringRef(context, &node->InternalName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->Id))
    {
        if (WordMatchStringRef(context, &node->Id->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->Name))
    {
        if (WordMatchStringRef(context, &node->Name->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->Version))
    {
        if (WordMatchStringRef(context, &node->Version->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->Author))
    {
        if (WordMatchStringRef(context, &node->Author->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->Description))
    {
        if (WordMatchStringRef(context, &node->Description->sr))
            return TRUE;
    }

    return FALSE;
}

INT_PTR CALLBACK CloudPluginsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWCT_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PWCT_CONTEXT)PhAllocate(sizeof(WCT_CONTEXT));
        memset(context, 0, sizeof(WCT_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PWCT_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            PhSaveWindowPlacementToSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);
            PhDeleteLayoutManager(&context->LayoutManager);
            PhUnregisterDialog(hwndDlg);
            RemoveProp(hwndDlg, L"Context");
            PhFree(context);

            PostQuitMessage(0);
        }
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->DialogHandle = hwndDlg;
            context->PluginMenuActiveId = IDC_INSTALLED;
            context->PluginMenuActive = GetDlgItem(hwndDlg, IDC_INSTALLED);
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_PLUGINTREE);
            context->SearchHandle = GetDlgItem(hwndDlg, IDC_SEARCHBOX);
            context->SearchboxText = PhReferenceEmptyString();

            CreateSearchControl(hwndDlg, context->SearchHandle, L"Search Plugins (Ctrl+K)");
            context->NormalFontHandle = CommonCreateFont(-14, FW_NORMAL, NULL);
            context->BoldFontHandle = CommonCreateFont(-16, FW_BOLD, NULL);
      
            PhCenterWindow(hwndDlg, PhMainWndHandle);
            InitializePluginsTree(context, hwndDlg, context->TreeNewHandle);
            PhAddTreeNewFilter(GetPluginListFilterSupport(context), ProcessTreeFilterCallback, context);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->SearchHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);           
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_CLEANUP), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_DISABLED), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);       
            PhLoadWindowPlacementFromSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);

            EnumerateLoadedPlugins(context);
            SetWindowText(GetDlgItem(hwndDlg, IDC_DISABLED), PhaFormatString(L"Disabled Plugins (%lu)", PhDisabledPluginsCount())->Buffer);
            PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), QueryPluginsCallbackThread, context);
            UpdateTreeView(context);

            SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwndDlg, IDC_INSTALLED), TRUE);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        TreeNew_AutoSizeColumn(context->TreeNewHandle, TREE_COLUMN_ITEM_NAME, TN_AUTOSIZE_REMAINING_SPACE);
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case EN_CHANGE:
                {
                    PPH_STRING newSearchboxText;

                    newSearchboxText = PH_AUTO(PhGetWindowText(context->SearchHandle));

                    if (!PhEqualString(context->SearchboxText, newSearchboxText, FALSE))
                    {
                        // Cache the current search text for our callback.
                        PhSwapReference(&context->SearchboxText, newSearchboxText);

                        if (!PhIsNullOrEmptyString(context->SearchboxText))
                        {
                            // Expand the nodes to ensure that they will be visible to the user.
                        }

                        PhApplyTreeNewFilters(GetPluginListFilterSupport(context));
                    }
                }
                break;
            }

            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            //case ID_SEARCH_CLEAR:
            //    {
            //        if (context->SearchHandle)
            //        {
            //            SetFocus(context->SearchHandle);
            //            Static_SetText(context->SearchHandle, L"");

            //            UpdateTreeView(context);
            //        }
            //    }
            //    break;
            case IDC_INSTALLED:
                {
                    context->Type = PLUGIN_STATE_LOCAL;

                    UpdateMenuView(context, IDC_INSTALLED);
                    UpdateTreeView(context);
                }
                break;
            case IDC_BROWSE:
                {
                    context->Type = PLUGIN_STATE_REMOTE;

                    UpdateMenuView(context, IDC_BROWSE);
                    UpdateTreeView(context);
                }
                break;
            case IDC_UPDATES:
                {
                    context->Type = PLUGIN_STATE_UPDATE;

                    UpdateMenuView(context, IDC_UPDATES);
                    UpdateTreeView(context);
                }
                break;
            case WM_ACTION:
                {
                    PPLUGIN_NODE selectedNode;

                    if (selectedNode = WeGetSelectedWindowNode(context))
                    {
                        if (selectedNode->State == PLUGIN_STATE_LOCAL)
                        {
                            if (selectedNode->PluginInstance && selectedNode->PluginInstance->Information.HasOptions)
                            {
                                PhInvokeCallback(PhGetPluginCallback((PPH_PLUGIN)selectedNode->PluginInstance, PluginCallbackShowOptions), hwndDlg);
                            }
                        }
                        else if (selectedNode->State == PLUGIN_STATE_REMOTE)
                        {
                            if (StartInitialCheck(hwndDlg, selectedNode, PLUGIN_ACTION_INSTALL))
                            {

                            }
                        }
                        else if (selectedNode->State == PLUGIN_STATE_UPDATE)
                        {
                            if (StartInitialCheck(hwndDlg, selectedNode, PLUGIN_ACTION_INSTALL))
                            {

                            }
                        }
                    }
                }
                break;
            case ID_WCTSHOWCONTEXTMENU:
                {
                    POINT cursorPos;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM selectedItem;
                    PPLUGIN_NODE selectedNode;

                    GetCursorPos(&cursorPos);

                    if (!(selectedNode = WeGetSelectedWindowNode(context)))
                        break;

                    menu = PhCreateEMenu();

                    if (selectedNode->State == PLUGIN_STATE_LOCAL)
                    {
                        HICON shieldIcon;

                        shieldIcon = PhLoadIcon(NULL, IDI_SHIELD, PH_LOAD_ICON_SIZE_SMALL | PH_LOAD_ICON_STRICT, 0, 0);
                        selectedItem = PhCreateEMenuItem(0, ID_MENU_UNINSTALL, L"Uninstall", NULL, NULL);

                        if (shieldIcon)
                        {
                            selectedItem->Bitmap = PhIconToBitmap(
                                shieldIcon, 
                                GetSystemMetrics(SM_CXSMICON), 
                                GetSystemMetrics(SM_CYSMICON)
                                );
                            DestroyIcon(shieldIcon);
                        }
       
                        PhInsertEMenuItem(menu, selectedItem, -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, NULL, NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MENU_DISABLE, L"Disable", NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, NULL, NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MENU_PROPERTIES, L"Options", NULL, NULL), -1);

                        if (!selectedNode->PluginInstance || !selectedNode->PluginInstance->Information.HasOptions)
                        {
                            PhSetFlagsEMenuItem(menu, ID_MENU_PROPERTIES, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                        }

                        if (!PhGetOwnTokenAttributes().Elevated)
                        {
                            PhSetFlagsEMenuItem(menu, ID_MENU_UNINSTALL, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                        }
                    }
                    else
                    {
                        HICON shieldIcon;

                        shieldIcon = PhLoadIcon(NULL, IDI_SHIELD, PH_LOAD_ICON_SIZE_SMALL | PH_LOAD_ICON_STRICT, 0, 0);
                        selectedItem = PhCreateEMenuItem(0, ID_MENU_INSTALL, PhaFormatString(L"Install %s plugin", PhGetStringOrEmpty(selectedNode->Name))->Buffer, NULL, NULL);

                        if (shieldIcon)
                        {
                            selectedItem->Bitmap = PhIconToBitmap(
                                shieldIcon, 
                                GetSystemMetrics(SM_CXSMICON), 
                                GetSystemMetrics(SM_CYSMICON)
                                );
                            DestroyIcon(shieldIcon);
                        }

                        PhInsertEMenuItem(menu, selectedItem, -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, NULL, NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MENU_DISABLE, L"Disable", NULL, NULL), -1);

                        if (!PhGetOwnTokenAttributes().Elevated)
                        {
                            PhSetFlagsEMenuItem(menu, ID_MENU_INSTALL, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                        }
                    }

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        cursorPos.x,
                        cursorPos.y
                        );

                    if (selectedItem && selectedItem->Id != -1)
                    {
                        switch (selectedItem->Id)
                        {
                        case ID_MENU_INSTALL:
                            {
                                if (!PhGetOwnTokenAttributes().Elevated)
                                {
                                    //SHELLEXECUTEINFO info = { sizeof(SHELLEXECUTEINFO) };
                                    //info.lpFile = L"ProcessHacker.exe";
                                    //info.lpParameters = L"-plugin dmex.ExtraPlugins:INSTALL -plugin dmex.ExtraPlugins:hex64value";
                                    //info.lpVerb = L"runas";
                                    //info.nShow = SW_SHOW;
                                    //info.hwnd = hwndDlg;

                                    //if (!ShellExecuteEx(&info))
                                    //{

                                    //}
                                }
                                else
                                {
                                    PPLUGIN_NODE existingNode;

                                    if (selectedNode->State == PLUGIN_STATE_LOCAL)
                                    {
                                        if (existingNode = FindTreeNode(
                                            context,
                                            PLUGIN_STATE_REMOTE,
                                            selectedNode->InternalName
                                            ))
                                        {
                                            if (StartInitialCheck(hwndDlg, existingNode, PLUGIN_ACTION_INSTALL))
                                            {

                                            }
                                        }

                                        UpdateTreeView(context);
                                    }
                                    else
                                    {
                                        if (StartInitialCheck(hwndDlg, selectedNode, PLUGIN_ACTION_INSTALL))
                                        {

                                        }

                                        if (existingNode = FindTreeNode(
                                            context,
                                            PLUGIN_STATE_REMOTE,
                                            selectedNode->InternalName
                                            ))
                                        {
                                            selectedNode->State = PLUGIN_STATE_RESTART;
                                        }

                                        UpdateTreeView(context);
                                    }
                                }
                            }
                            break;
                        case ID_MENU_UNINSTALL:
                            {
                                if (!PhGetOwnTokenAttributes().Elevated)
                                {
                                    //SHELLEXECUTEINFO info = { sizeof(SHELLEXECUTEINFO) };
                                    //info.lpFile = L"ProcessHacker.exe";
                                    //info.lpParameters = L"-plugin dmex.ExtraPlugins:UNINSTALL -plugin dmex.ExtraPlugins:hex64value";
                                    //info.lpVerb = L"runas";
                                    //info.nShow = SW_SHOW;
                                    //info.hwnd = hwndDlg;
                                    //
                                    //if (!ShellExecuteEx(&info))
                                    //{
                                    //
                                    //}
                                }
                                else
                                {
                                    PPLUGIN_NODE selectedNode;
                                    PPLUGIN_NODE existingNode;

                                    if (selectedNode = WeGetSelectedWindowNode(context))
                                    {
                                        if (StartInitialCheck(hwndDlg, selectedNode, PLUGIN_ACTION_UNINSTALL))
                                        {
                                            if (existingNode = FindTreeNode(
                                                context,
                                                PLUGIN_STATE_LOCAL, 
                                                selectedNode->InternalName
                                                ))
                                            {
                                                existingNode->State = PLUGIN_STATE_RESTART;
                                            }
                                        }

                                        UpdateTreeView(context);
                                    }
                                }
                            }
                            break;
                        case ID_MENU_DISABLE:
                            {
                                PPH_STRING baseName;

                                baseName = PhGetBaseName(selectedNode->FileName);
             
                                PhSetPluginDisabled(&baseName->sr, TRUE);

                                selectedNode->State = PLUGIN_STATE_RESTART;
                                UpdateTreeView(context);

                                SetWindowText(GetDlgItem(hwndDlg, IDC_DISABLED),
                                    PhGetString(PhaFormatString(L"Disabled Plugins (%lu)", PhDisabledPluginsCount()))
                                    );
                            }
                            break;
                        case ID_MENU_PROPERTIES:
                            {
                                if (selectedNode->PluginInstance)
                                {
                                    PhInvokeCallback(PhGetPluginCallback((PPH_PLUGIN)selectedNode->PluginInstance, PluginCallbackShowOptions), hwndDlg);
                                }
                            }
                            break;
                        }
                    }
                }
                break;
            case IDCANCEL:
            case IDOK:
                {
                    //ShowUpdateDialog(hwndDlg, PLUGIN_ACTION_RESTART);
                    DestroyWindow(hwndDlg);
                }
                break;
            case IDC_DISABLED:
                {
                    ShowDisabledPluginsDialog(hwndDlg);

                    PluginsClearTree(context);
                    EnumerateLoadedPlugins(context);
                    SetWindowText(GetDlgItem(hwndDlg, IDC_DISABLED), PhGetString(PhaFormatString(L"Disabled Plugins (%lu)", PhDisabledPluginsCount())));
                    PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), QueryPluginsCallbackThread, context);
                    UpdateTreeView(context);

                    SetWindowText(GetDlgItem(hwndDlg, IDC_DISABLED), 
                        PhGetString(PhaFormatString(L"Disabled Plugins (%lu)", PhDisabledPluginsCount()))
                        );
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
            case NM_CUSTOMDRAW:
                {
                    if (header->idFrom == IDC_INSTALLED || header->idFrom == IDC_BROWSE || header->idFrom == IDC_UPDATES)
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, DrawButton(context, lParam));
                        return TRUE;
                    }
                }
                break;
            }
        }
        break;
    case ID_UPDATE_ADD:
            {
                PPLUGIN_NODE entry = (PPLUGIN_NODE)lParam;

                TreeNew_SetRedraw(context->TreeNewHandle, FALSE);

                PluginsAddTreeNode(context, entry);
                UpdateTreeView(context);

                TreeNew_SetRedraw(context->TreeNewHandle, TRUE);
            }
            break;
    case ID_UPDATE_COUNT:
        {
            //ULONG count = 0;
            //
            //for (ULONG i = 0; i < context->TreeContext.NodeList->Count; i++)
            //{
            //    PPLUGIN_NODE windowNode = context->TreeContext.NodeList->Items[i];
            //
            //    if (windowNode->State == PLUGIN_STATE_UPDATE)
            //    {
            //        count++;
            //    }
            //}
            //
            //SetWindowText(GetDlgItem(hwndDlg, IDC_UPDATES), PhGetString(PhaFormatString(L"Updates (%lu)", count)));
        }
        break;
    }

    return FALSE;
}

NTSTATUS CloudDialogThread(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    HWND cloudDialogHandle;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    cloudDialogHandle = CreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_PLUGINS_DIALOG),
        NULL,
        CloudPluginsDlgProc
        );

    if (IsIconic(cloudDialogHandle))
        ShowWindow(cloudDialogHandle, SW_RESTORE);
    else
        ShowWindow(cloudDialogHandle, SW_SHOW);

    SetForegroundWindow(cloudDialogHandle);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(cloudDialogHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}


VOID ShowPluginManagerDialog(
    VOID
    )
{
    HANDLE threadHandle;

    if (threadHandle = PhCreateThread(0, CloudDialogThread, NULL))
    {
        NtClose(threadHandle);
    }
}