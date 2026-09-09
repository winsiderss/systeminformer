/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "agenttools.h"

#define AT_OPTIONS_REFRESH_TIMER 1
#define AT_OPTIONS_REFRESH_INTERVAL_MS 1000

#define AT_COLUMN_TOOL 0
#define AT_COLUMN_ACCESS 1
#define AT_COLUMN_AUTHORIZATION 2

#define AT_MENU_ACCESS_ALLOWED 1
#define AT_MENU_ACCESS_DENIED 2
#define AT_MENU_CONFIRM_ALWAYS 3
#define AT_MENU_CONFIRM_DELEGATE 4
#define AT_MENU_CONFIRM_NONE 5
#define AT_MENU_RESET 6

#define AT_AGENT_COLUMN_ID 0
#define AT_AGENT_COLUMN_USER 1
#define AT_AGENT_COLUMN_CLIENT 2
#define AT_AGENT_COLUMN_LAUNCHER 3
#define AT_AGENT_COLUMN_CONNECTED 4
#define AT_AGENT_COLUMN_CALLS 5
#define AT_AGENT_COLUMN_GRANTS 6

typedef struct _AT_TOOL_NODE
{
    PH_TREENEW_NODE Node;
    PCAT_TOOL Tool;
} AT_TOOL_NODE, *PAT_TOOL_NODE;

typedef struct _AT_TOOLS_CONTEXT
{
    HWND WindowHandle;
    HWND SearchHandle;
    HWND TreeNewHandle;
    PPH_LIST NodeList;
    PH_TN_FILTER_SUPPORT FilterSupport;
    ULONG_PTR SearchMatchHandle;
    ULONG SortColumn;
    PH_SORT_ORDER SortOrder;
    PH_LAYOUT_MANAGER LayoutManager;
} AT_TOOLS_CONTEXT, *PAT_TOOLS_CONTEXT;

typedef struct _AT_AGENT_NODE
{
    PH_TREENEW_NODE Node;
    ULONG ConnectionId;
    ULONG CallCount;
    PPH_STRING IdText;
    PPH_STRING UserText;
    PPH_STRING ClientText;
    PPH_STRING LauncherText;
    PPH_STRING ConnectedText;
    PPH_STRING CallsText;
    PPH_STRING GrantsText;
} AT_AGENT_NODE, *PAT_AGENT_NODE;

typedef struct _AT_AGENTS_CONTEXT
{
    HWND WindowHandle;
    HWND TreeNewHandle;
    PPH_LIST NodeList;
    ULONG SelectedConnectionId;
    ULONG SortColumn;
    PH_SORT_ORDER SortOrder;
    PH_LAYOUT_MANAGER LayoutManager;
} AT_AGENTS_CONTEXT, *PAT_AGENTS_CONTEXT;

PCWSTR AtpAccessText(
    _In_ PCAT_TOOL Tool
    )
{
    return PhGetIntegerSetting(Tool->AccessSetting) == AT_ACCESS_ALLOWED ? L"Allowed" : L"Denied";
}

PCWSTR AtpAuthorizationText(
    _In_ PCAT_TOOL Tool
    )
{
    switch (PhGetIntegerSetting(Tool->ConfirmSetting))
    {
    case AT_CONFIRM_ALWAYS:
        return L"Always ask";
    case AT_CONFIRM_DELEGATE:
        return L"Delegate to client";
    default:
        return L"Not required";
    }
}

VOID AtpUpdateStatus(
    _In_ HWND WindowHandle
    )
{
    NTSTATUS status;
    BOOLEAN elevated;
    PPH_STRING text;

    switch (AtServerGetState(&status, &elevated))
    {
    case AtServerRunning:
        text = PhFormatString(
            L"Server running. Pipe: %s%s%u",
            elevated ? SIMCP_PIPE_PROTECTED_PREFIX : L"",
            SIMCP_PIPE_NAME_PREFIX,
            NtCurrentPeb()->SessionId
            );
        break;
    case AtServerFailedPipeExists:
        text = PhCreateString(L"Not started: the pipe already exists (another instance in this session).");
        break;
    case AtServerFailed:
        {
            PPH_STRING message = PhGetStatusMessage(status, 0);

            text = PhFormatString(L"Not started: %s", PhGetStringOrDefault(message, L"unknown error"));
            PhClearReference(&message);
        }
        break;
    default:
        text = PhCreateString(L"Server stopped.");
        break;
    }

    PhSetDialogItemText(WindowHandle, IDC_STATUS, PhGetString(text));
    PhDereferenceObject(text);
}

VOID AtpApplyServerEnabled(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN Enabled
    )
{
    if (Enabled)
        AtServerStart();
    else
        AtServerStop(SimcpCloseServerDisabled);

    AtpUpdateStatus(WindowHandle);
}

PPH_LIST AtpGetSelectedTools(
    _In_ PAT_TOOLS_CONTEXT Context
    )
{
    PPH_LIST list;
    ULONG i;

    list = PhCreateList(AtToolCount);

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PAT_TOOL_NODE node = Context->NodeList->Items[i];

        if (node->Node.Visible && node->Node.Selected)
            PhAddItemList(list, (PVOID)node->Tool);
    }

    return list;
}

PPH_LIST AtpGetAllTools(
    VOID
    )
{
    PPH_LIST list;
    ULONG i;

    list = PhCreateList(AtToolCount);

    for (i = 0; i < AtToolCount; i++)
        PhAddItemList(list, (PVOID)&AtTools[i]);

    return list;
}

ULONG AtpGetCommonValue(
    _In_ PPH_LIST Tools,
    _In_ BOOLEAN Access
    )
{
    ULONG common = ULONG_MAX;
    ULONG i;

    for (i = 0; i < Tools->Count; i++)
    {
        PCAT_TOOL tool = Tools->Items[i];
        ULONG value = PhGetIntegerSetting(Access ? tool->AccessSetting : tool->ConfirmSetting);

        if (i == 0)
            common = value;
        else if (value != common)
            return ULONG_MAX;
    }

    return common;
}

PPH_EMENU_ITEM AtpCreateRadioItem(
    _In_ ULONG Id,
    _In_ PWSTR Text,
    _In_ BOOLEAN Checked
    )
{
    return PhCreateEMenuItem(Checked ? PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK : 0, Id, Text, NULL, NULL);
}

VOID AtpAppendAccessItems(
    _In_ PPH_EMENU_ITEM Menu,
    _In_ PPH_LIST Tools
    )
{
    ULONG access = AtpGetCommonValue(Tools, TRUE);

    PhInsertEMenuItem(Menu, AtpCreateRadioItem(AT_MENU_ACCESS_ALLOWED, L"Allowed", access == AT_ACCESS_ALLOWED), ULONG_MAX);
    PhInsertEMenuItem(Menu, AtpCreateRadioItem(AT_MENU_ACCESS_DENIED, L"Denied", access == AT_ACCESS_DENIED), ULONG_MAX);
}

VOID AtpAppendAuthorizationItems(
    _In_ PPH_EMENU_ITEM Menu,
    _In_ PPH_LIST Tools
    )
{
    ULONG confirm = AtpGetCommonValue(Tools, FALSE);

    PhInsertEMenuItem(Menu, AtpCreateRadioItem(AT_MENU_CONFIRM_ALWAYS, L"Always ask", confirm == AT_CONFIRM_ALWAYS), ULONG_MAX);
    PhInsertEMenuItem(Menu, AtpCreateRadioItem(AT_MENU_CONFIRM_DELEGATE, L"Delegate to client", confirm == AT_CONFIRM_DELEGATE), ULONG_MAX);
    PhInsertEMenuItem(Menu, AtpCreateRadioItem(AT_MENU_CONFIRM_NONE, L"Not required", confirm == AT_CONFIRM_NONE), ULONG_MAX);
}

VOID AtpApplyMenuChoice(
    _In_ PAT_TOOLS_CONTEXT Context,
    _In_ PPH_LIST Tools,
    _In_ ULONG Id
    )
{
    ULONG i;

    if (Id == AT_MENU_CONFIRM_NONE)
    {
        ULONG gated = 0;

        for (i = 0; i < Tools->Count; i++)
        {
            if (((PCAT_TOOL)Tools->Items[i])->Tier != AtTierRead)
                gated++;
        }

        if (gated)
        {
            PPH_STRING object;
            BOOLEAN proceed;

            if (gated == 1)
                object = PhCreateString(L"the authorization for this tool");
            else
                object = PhFormatString(L"the authorization for %u tools", gated);

            proceed = PhShowConfirmMessage(
                Context->WindowHandle,
                L"remove",
                PhGetString(object),
                L"Connected agents will be able to use it without anyone being asked.",
                TRUE
                );

            PhDereferenceObject(object);

            if (!proceed)
                return;
        }
    }

    for (i = 0; i < Tools->Count; i++)
    {
        PCAT_TOOL tool = Tools->Items[i];

        switch (Id)
        {
        case AT_MENU_ACCESS_ALLOWED:
            PhSetIntegerSetting(tool->AccessSetting, AT_ACCESS_ALLOWED);
            break;
        case AT_MENU_ACCESS_DENIED:
            PhSetIntegerSetting(tool->AccessSetting, AT_ACCESS_DENIED);
            break;
        case AT_MENU_CONFIRM_ALWAYS:
            PhSetIntegerSetting(tool->ConfirmSetting, AT_CONFIRM_ALWAYS);
            break;
        case AT_MENU_CONFIRM_DELEGATE:
            PhSetIntegerSetting(tool->ConfirmSetting, AT_CONFIRM_DELEGATE);
            break;
        case AT_MENU_CONFIRM_NONE:
            PhSetIntegerSetting(tool->ConfirmSetting, AT_CONFIRM_NONE);
            break;
        case AT_MENU_RESET:
            PhSetIntegerSetting(tool->AccessSetting, AtToolDefaultAccess(tool));
            PhSetIntegerSetting(tool->ConfirmSetting, AtToolDefaultConfirm(tool));
            break;
        }
    }

    InvalidateRect(Context->TreeNewHandle, NULL, FALSE);
}

VOID AtpShowCellPicker(
    _In_ PAT_TOOLS_CONTEXT Context,
    _In_ PAT_TOOL_NODE Node,
    _In_ ULONG ColumnId,
    _In_ POINT ScreenPoint
    )
{
    PPH_LIST tools;
    PPH_EMENU menu;
    PPH_EMENU_ITEM item;

    if (ColumnId != AT_COLUMN_ACCESS && ColumnId != AT_COLUMN_AUTHORIZATION)
        return;

    tools = AtpGetSelectedTools(Context);

    if (tools->Count == 0)
        PhAddItemList(tools, (PVOID)Node->Tool);

    menu = PhCreateEMenu();

    if (ColumnId == AT_COLUMN_ACCESS)
        AtpAppendAccessItems(menu, tools);
    else
        AtpAppendAuthorizationItems(menu, tools);

    item = PhShowEMenu(
        menu,
        Context->WindowHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        ScreenPoint.x,
        ScreenPoint.y
        );

    if (item)
        AtpApplyMenuChoice(Context, tools, item->Id);

    PhDestroyEMenu(menu);
    PhDereferenceObject(tools);
}

VOID AtpShowContextMenu(
    _In_ PAT_TOOLS_CONTEXT Context,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PPH_LIST tools;
    BOOLEAN selection;
    PPH_EMENU menu;
    PPH_EMENU_ITEM access;
    PPH_EMENU_ITEM authorization;
    PPH_EMENU_ITEM item;

    tools = AtpGetSelectedTools(Context);
    selection = tools->Count != 0;

    if (!selection)
    {
        PhDereferenceObject(tools);
        tools = AtpGetAllTools();
    }

    menu = PhCreateEMenu();
    access = PhCreateEMenuItem(selection ? 0 : PH_EMENU_DISABLED, 0, L"Access", NULL, NULL);
    authorization = PhCreateEMenuItem(selection ? 0 : PH_EMENU_DISABLED, 0, L"Authorization", NULL, NULL);
    AtpAppendAccessItems(access, tools);
    AtpAppendAuthorizationItems(authorization, tools);
    PhInsertEMenuItem(menu, access, ULONG_MAX);
    PhInsertEMenuItem(menu, authorization, ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, AT_MENU_RESET, selection ? L"Reset to defaults" : L"Reset all to defaults", NULL, NULL), ULONG_MAX);

    item = PhShowEMenu(
        menu,
        Context->WindowHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        ContextMenu->Location.x,
        ContextMenu->Location.y
        );

    if (item && item->Id)
        AtpApplyMenuChoice(Context, tools, item->Id);

    PhDestroyEMenu(menu);
    PhDereferenceObject(tools);
}

int __cdecl AtpToolsSortFunction(
    _In_ void* Context,
    _In_ const void* Elem1,
    _In_ const void* Elem2
    )
{
    PAT_TOOLS_CONTEXT context = Context;
    PAT_TOOL_NODE node1 = *(PAT_TOOL_NODE*)Elem1;
    PAT_TOOL_NODE node2 = *(PAT_TOOL_NODE*)Elem2;
    int result = 0;

    if (context->SortOrder != NoSortOrder)
    {
        switch (context->SortColumn)
        {
        case AT_COLUMN_TOOL:
            result = PhCompareStringZ(node1->Tool->DisplayName, node2->Tool->DisplayName, TRUE);
            break;
        case AT_COLUMN_ACCESS:
            result = uintcmp(PhGetIntegerSetting(node1->Tool->AccessSetting), PhGetIntegerSetting(node2->Tool->AccessSetting));
            break;
        case AT_COLUMN_AUTHORIZATION:
            result = uintcmp(PhGetIntegerSetting(node1->Tool->ConfirmSetting), PhGetIntegerSetting(node2->Tool->ConfirmSetting));
            break;
        }
    }

    if (result == 0)
        result = uintptrcmp((ULONG_PTR)node1->Tool, (ULONG_PTR)node2->Tool);

    return PhModifySort(result, context->SortOrder);
}

_Function_class_(PH_TN_FILTER_FUNCTION)
BOOLEAN NTAPI AtpToolsFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PAT_TOOLS_CONTEXT context = Context;
    PAT_TOOL_NODE node = (PAT_TOOL_NODE)Node;

    if (!context || !context->SearchMatchHandle)
        return TRUE;

    if (PhSearchControlMatchZ(context->SearchMatchHandle, node->Tool->DisplayName))
        return TRUE;

    return PhSearchControlMatchZ(context->SearchMatchHandle, PH_AUTO_T(PH_STRING, PhZeroExtendToUtf16(node->Tool->Name))->Buffer);
}

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI AtpToolsSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PAT_TOOLS_CONTEXT context = Context;

    context->SearchMatchHandle = MatchHandle;
    PhApplyTreeNewFilters(&context->FilterSupport);
}

BOOLEAN NTAPI AtpToolsTreeNewCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PAT_TOOLS_CONTEXT context = Context;

    if (!context)
        return FALSE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren->Node)
            {
                qsort_s(context->NodeList->Items, context->NodeList->Count, sizeof(PVOID), AtpToolsSortFunction, context);

                getChildren->Children = (PPH_TREENEW_NODE*)context->NodeList->Items;
                getChildren->NumberOfChildren = context->NodeList->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            PPH_TREENEW_SORT_CHANGED_EVENT sorting = Parameter1;

            context->SortColumn = sorting->SortColumn;
            context->SortOrder = sorting->SortOrder;
            TreeNew_NodesStructured(WindowHandle);
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            PAT_TOOL_NODE node = (PAT_TOOL_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case AT_COLUMN_TOOL:
                PhInitializeStringRefLongHint(&getCellText->Text, node->Tool->DisplayName);
                break;
            case AT_COLUMN_ACCESS:
                PhInitializeStringRefLongHint(&getCellText->Text, (PWSTR)AtpAccessText(node->Tool));
                break;
            case AT_COLUMN_AUTHORIZATION:
                PhInitializeStringRefLongHint(&getCellText->Text, (PWSTR)AtpAuthorizationText(node->Tool));
                break;
            default:
                return FALSE;
            }
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            if (keyEvent->VirtualKey == 'A' && GetKeyState(VK_CONTROL) < 0)
            {
                TreeNew_SelectRange(WindowHandle, 0, -1);
                keyEvent->Handled = TRUE;
            }
        }
        return TRUE;
    case TreeNewLeftClick:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;

            if (mouseEvent->Node && mouseEvent->Column)
            {
                POINT point = mouseEvent->Location;

                ClientToScreen(WindowHandle, &point);
                AtpShowCellPicker(context, (PAT_TOOL_NODE)mouseEvent->Node, mouseEvent->Column->Id, point);
            }
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            AtpShowContextMenu(context, Parameter1);
        }
        return TRUE;
    }

    return FALSE;
}

INT_PTR CALLBACK AtOptionsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PAT_TOOLS_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(AT_TOOLS_CONTEXT));
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            HWND treeNew;
            PPH_STRING columns;
            ULONG i;

            context->WindowHandle = WindowHandle;
            context->SearchHandle = GetDlgItem(WindowHandle, IDC_SEARCH);
            context->TreeNewHandle = treeNew = GetDlgItem(WindowHandle, IDC_TOOLS);
            context->NodeList = PhCreateList(AtToolCount);

            for (i = 0; i < AtToolCount; i++)
            {
                PAT_TOOL_NODE node;

                node = PhAllocateZero(sizeof(AT_TOOL_NODE));
                PhInitializeTreeNewNode(&node->Node);
                node->Tool = &AtTools[i];
                PhAddItemList(context->NodeList, node);
            }

            PhSetControlTheme(treeNew, L"explorer");
            TreeNew_SetCallback(treeNew, AtpToolsTreeNewCallback, context);
            TreeNew_SetExtendedFlags(treeNew, TN_FLAG_ITEM_DRAG_SELECT, TN_FLAG_ITEM_DRAG_SELECT);
            TreeNew_SetTriState(treeNew, TRUE);
            TreeNew_SetSort(treeNew, AT_COLUMN_TOOL, NoSortOrder);
            PhAddTreeNewColumn(treeNew, AT_COLUMN_TOOL, TRUE, L"Tool", 240, PH_ALIGN_LEFT, 0, 0);
            PhAddTreeNewColumn(treeNew, AT_COLUMN_ACCESS, TRUE, L"Access", 70, PH_ALIGN_LEFT, 1, 0);
            PhAddTreeNewColumn(treeNew, AT_COLUMN_AUTHORIZATION, TRUE, L"Authorization", 120, PH_ALIGN_LEFT, 2, 0);

            columns = PhGetStringSetting(SETTING_NAME_TOOLS_LISTVIEW_COLUMNS);
            PhCmLoadSettings(treeNew, &columns->sr);
            PhDereferenceObject(columns);

            PhInitializeTreeNewFilterSupport(&context->FilterSupport, treeNew, context->NodeList);
            PhAddTreeNewFilter(&context->FilterSupport, AtpToolsFilterCallback, context);
            TreeNew_NodesStructured(treeNew);

            PhCreateSearchControl(WindowHandle, context->SearchHandle, L"Search tools", AtpToolsSearchControlCallback, context);

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_STATUS), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->SearchHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, treeNew, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_NOTE), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            Button_SetCheck(GetDlgItem(WindowHandle, IDC_ENABLED), PhGetIntegerSetting(SETTING_NAME_ENABLED) ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(WindowHandle, IDC_ALLOW_SANDBOXED), PhGetIntegerSetting(SETTING_NAME_ALLOW_SANDBOXED_CLIENTS) ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(WindowHandle, IDC_CONFIRM_CONNECTIONS), PhGetIntegerSetting(SETTING_NAME_CONFIRM_CONNECTIONS) ? BST_CHECKED : BST_UNCHECKED);

            AtpUpdateStatus(WindowHandle);

            SetTimer(WindowHandle, AT_OPTIONS_REFRESH_TIMER, AT_OPTIONS_REFRESH_INTERVAL_MS, NULL);
        }
        break;
    case WM_DESTROY:
        {
            PPH_STRING columns;

            KillTimer(WindowHandle, AT_OPTIONS_REFRESH_TIMER);

            columns = PhCmSaveSettings(context->TreeNewHandle);
            PhSetStringSetting2(SETTING_NAME_TOOLS_LISTVIEW_COLUMNS, &columns->sr);
            PhDereferenceObject(columns);

            PhDeleteTreeNewFilterSupport(&context->FilterSupport);
            PhDeleteLayoutManager(&context->LayoutManager);
        }
        break;
    case WM_NCDESTROY:
        {
            ULONG i;

            for (i = 0; i < context->NodeList->Count; i++)
                PhFree(context->NodeList->Items[i]);

            PhDereferenceObject(context->NodeList);
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_TIMER:
        {
            if (wParam == AT_OPTIONS_REFRESH_TIMER)
                AtpUpdateStatus(WindowHandle);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_ENABLED:
                {
                    BOOLEAN enabled = Button_GetCheck(GET_WM_COMMAND_HWND(wParam, lParam)) == BST_CHECKED;

                    PhSetIntegerSetting(SETTING_NAME_ENABLED, enabled);
                    AtpApplyServerEnabled(WindowHandle, enabled);
                }
                break;
            case IDC_ALLOW_SANDBOXED:
                {
                    PhSetIntegerSetting(SETTING_NAME_ALLOW_SANDBOXED_CLIENTS, Button_GetCheck(GET_WM_COMMAND_HWND(wParam, lParam)) == BST_CHECKED);

                    // The pipe security descriptor is built at start; restart to apply.
                    if (AtServerGetState(NULL, NULL) == AtServerRunning)
                    {
                        AtServerStop(SimcpCloseServerDisabled);
                        AtpApplyServerEnabled(WindowHandle, TRUE);
                    }
                }
                break;
            case IDC_CONFIRM_CONNECTIONS:
                {
                    // Applies to the next connection; ones already answered keep their answer.
                    PhSetIntegerSetting(SETTING_NAME_CONFIRM_CONNECTIONS, Button_GetCheck(GET_WM_COMMAND_HWND(wParam, lParam)) == BST_CHECKED);
                }
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID AtpDestroyAgentNode(
    _In_ PAT_AGENT_NODE Node
    )
{
    PhClearReference(&Node->IdText);
    PhClearReference(&Node->UserText);
    PhClearReference(&Node->ClientText);
    PhClearReference(&Node->LauncherText);
    PhClearReference(&Node->ConnectedText);
    PhClearReference(&Node->CallsText);
    PhClearReference(&Node->GrantsText);
    PhFree(Node);
}

VOID AtpClearAgentNodes(
    _In_ PAT_AGENTS_CONTEXT Context
    )
{
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
        AtpDestroyAgentNode(Context->NodeList->Items[i]);

    PhClearList(Context->NodeList);
}

PAT_AGENT_NODE AtpCreateAgentNode(
    _In_ PAT_CONNECTION Connection
    )
{
    PAT_AGENT_NODE node;
    SYSTEMTIME systemTime;
    PH_STRING_BUILDER grants;
    ULONG i;

    node = PhAllocateZero(sizeof(AT_AGENT_NODE));
    PhInitializeTreeNewNode(&node->Node);
    node->ConnectionId = Connection->ConnectionId;

    PhAcquireQueuedLockExclusive(&Connection->Lock);

    node->IdText = PhFormatUInt64(Connection->ConnectionId, FALSE);

    if (Connection->UserName)
        node->UserText = PhReferenceObject(Connection->UserName);
    else
        node->UserText = PhCreateString(L"unknown");

    if (Connection->ClientName)
        node->ClientText = PhFormatString(L"%s %s", PhGetString(Connection->ClientName), PhGetStringOrEmpty(Connection->ClientVersion));
    else
        node->ClientText = PhCreateString(L"(not identified)");

    if (Connection->LauncherImageName)
        node->LauncherText = PhReferenceObject(Connection->LauncherImageName);
    else
        node->LauncherText = PhCreateString(L"unknown");

    PhLargeIntegerToLocalSystemTime(&systemTime, &Connection->ConnectTime);
    node->ConnectedText = PhFormatDateTime(&systemTime);

    switch (ReadAcquire((PLONG)&Connection->Approval))
    {
    case AtApprovalPending:
        PhMoveReference(&node->ConnectedText, PhConcatStrings2(PhGetString(node->ConnectedText), L" (awaiting approval)"));
        break;
    case AtApprovalDenied:
        PhMoveReference(&node->ConnectedText, PhConcatStrings2(PhGetString(node->ConnectedText), L" (denied)"));
        break;
    }

    node->CallCount = Connection->CallCount;
    node->CallsText = PhFormatUInt64(Connection->CallCount, FALSE);

    PhInitializeStringBuilder(&grants, 64);

    for (i = 0; i < AtActionMaximum; i++)
    {
        // A class grant is named once, by the class, not by every action that shares it.
        if (AtActionInfo[i].Class != AtConsentClassNone)
            continue;

        if (Connection->SessionPolicy[i] != AtSessionAsk)
        {
            if (grants.String->Length)
                PhAppendStringBuilder2(&grants, L", ");

            PhAppendStringBuilder2(&grants, AtActionInfo[i].AuditName);

            if (Connection->SessionPolicy[i] == AtSessionDelegate)
                PhAppendStringBuilder2(&grants, L" (client)");
        }
    }

    for (i = AtConsentClassNone + 1; i < AtConsentClassMaximum; i++)
    {
        if (Connection->ClassPolicy[i] != AtSessionAsk)
        {
            if (grants.String->Length)
                PhAppendStringBuilder2(&grants, L", ");

            PhAppendStringBuilder2(&grants, (PWSTR)AtConsentClassDescription(i));

            if (Connection->ClassPolicy[i] == AtSessionDelegate)
                PhAppendStringBuilder2(&grants, L" (client)");
        }
    }

    if (grants.String->Length == 0)
        PhAppendStringBuilder2(&grants, L"none");

    node->GrantsText = PhFinalStringBuilderString(&grants);

    PhReleaseQueuedLockExclusive(&Connection->Lock);

    return node;
}

VOID AtpRefreshAgents(
    _In_ PAT_AGENTS_CONTEXT Context
    )
{
    HWND treeNew = Context->TreeNewHandle;
    PPH_LIST connections;
    PAT_AGENT_NODE selectedNode = NULL;
    ULONG i;

    connections = AtServerSnapshotConnections();

    TreeNew_SetRedraw(treeNew, FALSE);
    AtpClearAgentNodes(Context);

    for (i = 0; i < connections->Count; i++)
    {
        PAT_CONNECTION connection = connections->Items[i];
        PAT_AGENT_NODE node;

        if (ReadAcquire(&connection->Authenticated))
        {
            node = AtpCreateAgentNode(connection);

            if (node->ConnectionId == Context->SelectedConnectionId)
            {
                node->Node.Selected = TRUE;
                selectedNode = node;
            }

            PhAddItemList(Context->NodeList, node);
        }

        PhDereferenceObject(connection);
    }

    PhDereferenceObject(connections);

    TreeNew_NodesStructured(treeNew);

    if (selectedNode)
        TreeNew_SetFocusNode(treeNew, &selectedNode->Node);
    else
        Context->SelectedConnectionId = 0;

    TreeNew_SetRedraw(treeNew, TRUE);

    EnableWindow(GetDlgItem(Context->WindowHandle, IDC_DISCONNECT), selectedNode != NULL);
    EnableWindow(GetDlgItem(Context->WindowHandle, IDC_REVOKE_GRANTS), selectedNode != NULL);
}

int __cdecl AtpAgentsSortFunction(
    _In_ void* Context,
    _In_ const void* Elem1,
    _In_ const void* Elem2
    )
{
    PAT_AGENTS_CONTEXT context = Context;
    PAT_AGENT_NODE node1 = *(PAT_AGENT_NODE*)Elem1;
    PAT_AGENT_NODE node2 = *(PAT_AGENT_NODE*)Elem2;
    int result = 0;

    if (context->SortOrder != NoSortOrder)
    {
        switch (context->SortColumn)
        {
        case AT_AGENT_COLUMN_USER:
            result = PhCompareString(node1->UserText, node2->UserText, TRUE);
            break;
        case AT_AGENT_COLUMN_CLIENT:
            result = PhCompareString(node1->ClientText, node2->ClientText, TRUE);
            break;
        case AT_AGENT_COLUMN_LAUNCHER:
            result = PhCompareString(node1->LauncherText, node2->LauncherText, TRUE);
            break;
        case AT_AGENT_COLUMN_CALLS:
            result = uintcmp(node1->CallCount, node2->CallCount);
            break;
        case AT_AGENT_COLUMN_GRANTS:
            result = PhCompareString(node1->GrantsText, node2->GrantsText, TRUE);
            break;
        }
    }

    if (result == 0)
        result = uintcmp(node1->ConnectionId, node2->ConnectionId);

    return PhModifySort(result, context->SortOrder);
}

BOOLEAN NTAPI AtpAgentsTreeNewCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PAT_AGENTS_CONTEXT context = Context;

    if (!context)
        return FALSE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren->Node)
            {
                qsort_s(context->NodeList->Items, context->NodeList->Count, sizeof(PVOID), AtpAgentsSortFunction, context);

                getChildren->Children = (PPH_TREENEW_NODE*)context->NodeList->Items;
                getChildren->NumberOfChildren = context->NodeList->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            PPH_TREENEW_SORT_CHANGED_EVENT sorting = Parameter1;

            context->SortColumn = sorting->SortColumn;
            context->SortOrder = sorting->SortOrder;
            TreeNew_NodesStructured(WindowHandle);
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            PAT_AGENT_NODE node = (PAT_AGENT_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case AT_AGENT_COLUMN_ID:
                getCellText->Text = PhGetStringRef(node->IdText);
                break;
            case AT_AGENT_COLUMN_USER:
                getCellText->Text = PhGetStringRef(node->UserText);
                break;
            case AT_AGENT_COLUMN_CLIENT:
                getCellText->Text = PhGetStringRef(node->ClientText);
                break;
            case AT_AGENT_COLUMN_LAUNCHER:
                getCellText->Text = PhGetStringRef(node->LauncherText);
                break;
            case AT_AGENT_COLUMN_CONNECTED:
                getCellText->Text = PhGetStringRef(node->ConnectedText);
                break;
            case AT_AGENT_COLUMN_CALLS:
                getCellText->Text = PhGetStringRef(node->CallsText);
                break;
            case AT_AGENT_COLUMN_GRANTS:
                getCellText->Text = PhGetStringRef(node->GrantsText);
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewSelectionChanged:
        {
            PAT_AGENT_NODE node = (PAT_AGENT_NODE)TreeNew_GetSelectedNode(WindowHandle);

            context->SelectedConnectionId = node ? node->ConnectionId : 0;
            EnableWindow(GetDlgItem(context->WindowHandle, IDC_DISCONNECT), node != NULL);
            EnableWindow(GetDlgItem(context->WindowHandle, IDC_REVOKE_GRANTS), node != NULL);
        }
        return TRUE;
    }

    return FALSE;
}

VOID AtpUpdateSnippet(
    _In_ HWND WindowHandle
    )
{
    static CONST PH_STRINGREF brokerFileName = PH_STRINGREF_INIT(SIMCP_BROKER_FILE_NAME);
    PPH_STRING directory;
    PPH_STRING brokerPath;
    PPH_STRING escapedPath;
    PPH_STRING snippet;
    PH_STRING_BUILDER builder;
    SIZE_T i;
    LONG target;

    if (!(directory = PhGetApplicationDirectoryWin32()))
        return;

    brokerPath = PhConcatStringRef2(&directory->sr, &brokerFileName);
    PhDereferenceObject(directory);

    PhInitializeStringBuilder(&builder, brokerPath->Length + 32);

    for (i = 0; i < brokerPath->Length / sizeof(WCHAR); i++)
    {
        WCHAR c = brokerPath->Buffer[i];

        if (c == L'\\' || c == L'"')
            PhAppendCharStringBuilder(&builder, L'\\');

        PhAppendCharStringBuilder(&builder, c);
    }

    escapedPath = PhFinalStringBuilderString(&builder);

    target = ComboBox_GetCurSel(GetDlgItem(WindowHandle, IDC_CONFIG_TARGET));

    switch (target)
    {
    case 1:
        snippet = PhFormatString(
            L"// .vscode/mcp.json\r\n{\r\n  \"servers\": {\r\n    \"systeminformer\": { \"type\": \"stdio\", \"command\": \"%s\" }\r\n  }\r\n}",
            PhGetString(escapedPath)
            );
        break;
    case 2:
        snippet = PhFormatString(
            L"// claude_desktop_config.json\r\n{\r\n  \"mcpServers\": {\r\n    \"systeminformer\": { \"command\": \"%s\" }\r\n  }\r\n}",
            PhGetString(escapedPath)
            );
        break;
    default:
        snippet = PhFormatString(
            L"claude mcp add --transport stdio systeminformer -- \"%s\"",
            PhGetString(brokerPath)
            );
        break;
    }

    PhSetDialogItemText(WindowHandle, IDC_SNIPPET, PhGetString(snippet));

    PhDereferenceObject(snippet);
    PhDereferenceObject(escapedPath);
    PhDereferenceObject(brokerPath);
}

INT_PTR CALLBACK AtAgentsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PAT_AGENTS_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(AT_AGENTS_CONTEXT));
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            static CONST PH_STRINGREF emptyText = PH_STRINGREF_INIT(L"No agents connected.");
            HWND treeNew;
            HWND configTarget;
            PPH_STRING columns;

            context->WindowHandle = WindowHandle;
            context->NodeList = PhCreateList(4);
            context->TreeNewHandle = treeNew = GetDlgItem(WindowHandle, IDC_AGENTS);

            PhSetControlTheme(treeNew, L"explorer");
            TreeNew_SetCallback(treeNew, AtpAgentsTreeNewCallback, context);
            TreeNew_SetEmptyText(treeNew, &emptyText, 0);
            TreeNew_SetTriState(treeNew, TRUE);
            TreeNew_SetSort(treeNew, AT_AGENT_COLUMN_ID, NoSortOrder);
            PhAddTreeNewColumn(treeNew, AT_AGENT_COLUMN_ID, TRUE, L"Id", 30, PH_ALIGN_LEFT, 0, 0);
            PhAddTreeNewColumn(treeNew, AT_AGENT_COLUMN_USER, TRUE, L"User", 90, PH_ALIGN_LEFT, 1, DT_PATH_ELLIPSIS);
            PhAddTreeNewColumn(treeNew, AT_AGENT_COLUMN_CLIENT, TRUE, L"Client", 110, PH_ALIGN_LEFT, 2, 0);
            PhAddTreeNewColumn(treeNew, AT_AGENT_COLUMN_LAUNCHER, TRUE, L"Launcher", 140, PH_ALIGN_LEFT, 3, DT_PATH_ELLIPSIS);
            PhAddTreeNewColumn(treeNew, AT_AGENT_COLUMN_CONNECTED, TRUE, L"Connected", 100, PH_ALIGN_LEFT, 4, 0);
            PhAddTreeNewColumn(treeNew, AT_AGENT_COLUMN_CALLS, TRUE, L"Calls", 40, PH_ALIGN_LEFT, 5, 0);
            PhAddTreeNewColumn(treeNew, AT_AGENT_COLUMN_GRANTS, TRUE, L"Session grants", 140, PH_ALIGN_LEFT, 6, 0);

            columns = PhGetStringSetting(SETTING_NAME_AGENTS_LISTVIEW_COLUMNS);
            PhCmLoadSettings(treeNew, &columns->sr);
            PhDereferenceObject(columns);

            configTarget = GetDlgItem(WindowHandle, IDC_CONFIG_TARGET);
            ComboBox_AddString(configTarget, L"Claude Code");
            ComboBox_AddString(configTarget, L"VS Code");
            ComboBox_AddString(configTarget, L"Claude Desktop");
            ComboBox_SetCurSel(configTarget, 0);

            // The agents group takes the extra height; the configuration group keeps its size
            // and stays pinned to the bottom.
            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_AGENTS_GROUP), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, treeNew, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_REVOKE_GRANTS), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_DISCONNECT), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_CONFIG_GROUP), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, configTarget, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_COPY), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_SNIPPET), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            AtpRefreshAgents(context);
            AtpUpdateSnippet(WindowHandle);

            SetTimer(WindowHandle, AT_OPTIONS_REFRESH_TIMER, AT_OPTIONS_REFRESH_INTERVAL_MS, NULL);
        }
        break;
    case WM_DESTROY:
        {
            PPH_STRING columns;

            KillTimer(WindowHandle, AT_OPTIONS_REFRESH_TIMER);

            columns = PhCmSaveSettings(context->TreeNewHandle);
            PhSetStringSetting2(SETTING_NAME_AGENTS_LISTVIEW_COLUMNS, &columns->sr);
            PhDereferenceObject(columns);

            PhDeleteLayoutManager(&context->LayoutManager);
        }
        break;
    case WM_NCDESTROY:
        {
            // The tree is gone by now, so nothing can ask for the nodes anymore.
            AtpClearAgentNodes(context);
            PhDereferenceObject(context->NodeList);
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_TIMER:
        {
            if (wParam == AT_OPTIONS_REFRESH_TIMER)
                AtpRefreshAgents(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_DISCONNECT:
                {
                    if (context->SelectedConnectionId)
                    {
                        AtServerDisconnect(context->SelectedConnectionId);
                        AtpRefreshAgents(context);
                    }
                }
                break;
            case IDC_REVOKE_GRANTS:
                {
                    // Takes back every "allow for this session" without dropping the connection.
                    if (context->SelectedConnectionId)
                    {
                        AtConsentRevokeGrants(context->SelectedConnectionId);
                        AtpRefreshAgents(context);
                    }
                }
                break;
            case IDC_CONFIG_TARGET:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                        AtpUpdateSnippet(WindowHandle);
                }
                break;
            case IDC_COPY:
                {
                    PPH_STRING text = PhGetWindowText(GetDlgItem(WindowHandle, IDC_SNIPPET));

                    if (text)
                    {
                        PhSetClipboardString(WindowHandle, &text->sr);
                        PhDereferenceObject(text);
                    }
                }
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}
