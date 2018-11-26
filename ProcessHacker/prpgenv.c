/*
 * Process Hacker -
 *   Process properties: Environment page
 *
 * Copyright (C) 2009-2016 wj32
 * Copyright (C) 2018 dmex
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
#include <settings.h>

#include <phsettings.h>
#include <procprp.h>
#include <procprpp.h>
#include <procprv.h>
#include <lsasup.h>
#include <userenv.h>

typedef enum _ENVIRONMENT_TREE_MENU_ITEM
{
    ENVIRONMENT_TREE_MENU_ITEM_HIDE_PROCESS_TYPE = 1,
    ENVIRONMENT_TREE_MENU_ITEM_HIDE_USER_TYPE,
    ENVIRONMENT_TREE_MENU_ITEM_HIDE_SYSTEM_TYPE,
    ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_PROCESS_TYPE,
    ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_USER_TYPE,
    ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_SYSTEM_TYPE,
    ENVIRONMENT_TREE_MENU_ITEM_NEW_ENVIRONMENT_VARIABLE,
    ENVIRONMENT_TREE_MENU_ITEM_MAXIMUM
} ENVIRONMENT_TREE_MENU_ITEM;

typedef enum _ENVIRONMENT_TREE_COLUMN_MENU_ITEM
{
    ENVIRONMENT_TREE_COLUMN_MENU_ITEM_EDIT = 1,
    ENVIRONMENT_TREE_COLUMN_MENU_ITEM_DELETE,
    ENVIRONMENT_TREE_COLUMN_MENU_ITEM_COPY,
    ENVIRONMENT_TREE_COLUMN_MENU_ITEM_MAXIMUM
} ENVIRONMENT_TREE_COLUMN_MENU_ITEM;

typedef enum _ENVIRONMENT_TREE_COLUMN_ITEM_NAME
{
    ENVIRONMENT_COLUMN_ITEM_NAME,
    ENVIRONMENT_COLUMN_ITEM_VALUE,
    ENVIRONMENT_COLUMN_ITEM_MAXIMUM
} ENVIRONMENT_TREE_COLUMN_ITEM_NAME;

typedef enum _PROCESS_ENVIRONMENT_TREENODE_TYPE
{
    PROCESS_ENVIRONMENT_TREENODE_TYPE_GROUP,
    PROCESS_ENVIRONMENT_TREENODE_TYPE_PROCESS,
    PROCESS_ENVIRONMENT_TREENODE_TYPE_USER,
    PROCESS_ENVIRONMENT_TREENODE_TYPE_SYSTEM
} PROCESS_ENVIRONMENT_TREENODE_TYPE;

typedef struct _PHP_PROCESS_ENVIRONMENT_TREENODE
{
    PH_TREENEW_NODE Node;
    PH_STRINGREF TextCache[ENVIRONMENT_COLUMN_ITEM_MAXIMUM];

    struct _PHP_PROCESS_ENVIRONMENT_TREENODE* Parent;
    PPH_LIST Children;
    BOOLEAN HasChildren;

    ULONG Id;
    PROCESS_ENVIRONMENT_TREENODE_TYPE Type;
    PPH_STRING NameText;
    PPH_STRING ValueText;
} PHP_PROCESS_ENVIRONMENT_TREENODE, *PPHP_PROCESS_ENVIRONMENT_TREENODE;

PPHP_PROCESS_ENVIRONMENT_TREENODE PhpAddEnvironmentChildNode(
    _In_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_opt_ PPHP_PROCESS_ENVIRONMENT_TREENODE ParentNode,
    _In_ ULONG Id,
    _In_ PROCESS_ENVIRONMENT_TREENODE_TYPE Type,
    _In_ PPH_STRING Name,
    _In_opt_ PPH_STRING Value
    );

PPHP_PROCESS_ENVIRONMENT_TREENODE PhpFindEnvironmentNode(
    _In_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_ PWSTR KeyPath
    );

VOID PhpClearEnvironmentTree(
    _In_ PPH_ENVIRONMENT_CONTEXT Context
    );

PPHP_PROCESS_ENVIRONMENT_TREENODE PhpGetSelectedEnvironmentNode(
    _In_ PPH_ENVIRONMENT_CONTEXT Context
    );

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

VOID PhpClearEnvironmentItems(
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

VOID PhpRefreshEnvironmentList(
    _In_ HWND hwndDlg,
    _Inout_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    HANDLE processHandle;
    PVOID environment;
    ULONG environmentLength;
    ULONG enumerationKey;
    PH_ENVIRONMENT_VARIABLE variable;
    PPH_ENVIRONMENT_ITEM item;
    PPHP_PROCESS_ENVIRONMENT_TREENODE processRootNode;
    PPHP_PROCESS_ENVIRONMENT_TREENODE userRootNode;
    PPHP_PROCESS_ENVIRONMENT_TREENODE systemRootNode;
    ULONG i;

    PhpClearEnvironmentTree(Context);
    processRootNode = PhpAddEnvironmentChildNode(Context, NULL, 0, 0, PhaCreateString(L"Process"), NULL);
    userRootNode = PhpAddEnvironmentChildNode(Context, NULL, 0, 0, PhaCreateString(L"User"), NULL);
    systemRootNode = PhpAddEnvironmentChildNode(Context, NULL, 0, 0, PhaCreateString(L"System"), NULL);

    if (DestroyEnvironmentBlock)
    {
        if (Context->SystemDefaultEnvironment)
        {
            DestroyEnvironmentBlock(Context->SystemDefaultEnvironment);
            Context->SystemDefaultEnvironment = NULL;
        }

        if (Context->UserDefaultEnvironment)
        {
            DestroyEnvironmentBlock(Context->UserDefaultEnvironment);
            Context->UserDefaultEnvironment = NULL;
        }
    }

    if (CreateEnvironmentBlock)
    {
        CreateEnvironmentBlock(&Context->SystemDefaultEnvironment, NULL, FALSE);
        CreateEnvironmentBlock(&Context->UserDefaultEnvironment, PhGetOwnTokenAttributes().TokenHandle, FALSE);
    }

    if (NT_SUCCESS(PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        ProcessItem->ProcessId
        )))
    {
        ULONG flags = 0;

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

                // Don't display pairs with no name.
                if (variable.Name.Length == 0)
                    continue;

                item.Name = PhCreateString2(&variable.Name);
                item.Value = PhCreateString2(&variable.Value);

                PhAddItemArray(&Context->Items, &item);
            }

            PhFreePage(environment);
        }

        NtClose(processHandle);
    }

    for (i = 0; i < Context->Items.Count; i++)
    {
        PPHP_PROCESS_ENVIRONMENT_TREENODE parentNode;
        PROCESS_ENVIRONMENT_TREENODE_TYPE nodeType;
        PPH_STRING variableValue;

        item = PhItemArray(&Context->Items, i);

        if (PhQueryEnvironmentVariable(
            Context->SystemDefaultEnvironment,
            &item->Name->sr,
            NULL
            ) == STATUS_BUFFER_TOO_SMALL)
        {
            nodeType = PROCESS_ENVIRONMENT_TREENODE_TYPE_SYSTEM;
            parentNode = systemRootNode;

            if (NT_SUCCESS(PhQueryEnvironmentVariable(
                Context->SystemDefaultEnvironment,
                &item->Name->sr,
                &variableValue
                )))
            {
                if (!PhEqualString(variableValue, item->Value, FALSE))
                {
                    nodeType = PROCESS_ENVIRONMENT_TREENODE_TYPE_PROCESS;
                    parentNode = processRootNode;
                }

                PhDereferenceObject(variableValue);
            }
        }
        else if (PhQueryEnvironmentVariable(
            Context->UserDefaultEnvironment,
            &item->Name->sr,
            NULL
            ) == STATUS_BUFFER_TOO_SMALL)
        {
            nodeType = PROCESS_ENVIRONMENT_TREENODE_TYPE_USER;
            parentNode = userRootNode;

            if (NT_SUCCESS(PhQueryEnvironmentVariable(
                Context->UserDefaultEnvironment,
                &item->Name->sr,
                &variableValue
                )))
            {
                if (!PhEqualString(variableValue, item->Value, FALSE))
                {
                    nodeType = PROCESS_ENVIRONMENT_TREENODE_TYPE_PROCESS;
                    parentNode = processRootNode;
                }

                PhDereferenceObject(variableValue);
            }
        }
        else
        {
            nodeType = PROCESS_ENVIRONMENT_TREENODE_TYPE_PROCESS;
            parentNode = processRootNode;
        }

        PhpAddEnvironmentChildNode(Context, parentNode, i, nodeType, item->Name, item->Value);
    }

    PhApplyTreeNewFilters(&Context->TreeFilterSupport);
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
            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER)));
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)PH_LOAD_SHARED_ICON_LARGE(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER)));

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_NAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_VALUE), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhLayoutManagerLayout(&context->LayoutManager);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 200;
            context->MinimumSize.bottom = 140;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            PhSetDialogItemText(hwndDlg, IDC_NAME, context->Name);
            PhSetDialogItemText(hwndDlg, IDC_VALUE, context->Value ? context->Value : L"");

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDCANCEL));

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
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

                    if (PhIsProcessSuspended(context->ProcessItem->ProcessId))
                    {
                        if (PhGetIntegerSetting(L"EnableWarnings") && PhShowMessage2(
                            hwndDlg,
                            TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
                            TD_WARNING_ICON,
                            L"The process is suspended.",
                            L"Editing environment variable(s) of suspended processes is not supported.\n\nAre you sure you want to continue?"
                            ) == IDNO)
                        {
                            break;
                        }
                    }

                    if (!PhIsNullOrEmptyString(name))
                    {
                        if (!PhEqualString2(name, context->Name, FALSE) ||
                            !context->Value || !PhEqualString2(value, context->Value, FALSE))
                        {
                            if (NT_SUCCESS(status = PhOpenProcess(
                                &processHandle,
                                PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
                                PROCESS_VM_READ | PROCESS_VM_WRITE,
                                context->ProcessItem->ProcessId
                                )))
                            {
                                timeout.QuadPart = -(LONGLONG)UInt32x32To64(10, PH_TIMEOUT_SEC);

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
                                PhShowStatus(hwndDlg, L"Unable to set the environment variable.", status, 0);
                                break;
                            }
                            else if (status == STATUS_TIMEOUT)
                            {
                                PhShowStatus(hwndDlg, L"Unable to delete the environment variable.", 0, WAIT_TIMEOUT);
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
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
                    {
                        EnableWindow(GetDlgItem(hwndDlg, IDOK), PhGetWindowTextLength(GetDlgItem(hwndDlg, IDC_NAME)) > 0);
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

VOID PhpShowEnvironmentNodeContextMenu(
    _In_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenuEvent
    )
{
    PPHP_PROCESS_ENVIRONMENT_TREENODE node;
    PPH_EMENU menu;
    PPH_EMENU_ITEM selectedItem;

    if (!(node = PhpGetSelectedEnvironmentNode(Context)))
        return;

    if (node->Type == PROCESS_ENVIRONMENT_TREENODE_TYPE_GROUP)
        return;

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ENVIRONMENT_TREE_COLUMN_MENU_ITEM_EDIT, L"Edit", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ENVIRONMENT_TREE_COLUMN_MENU_ITEM_DELETE, L"Delete", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ENVIRONMENT_TREE_COLUMN_MENU_ITEM_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
    PhInsertCopyCellEMenuItem(menu, 3, Context->TreeNewHandle, ContextMenuEvent->Column);

    selectedItem = PhShowEMenu(
        menu,
        Context->WindowHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        ContextMenuEvent->Location.x,
        ContextMenuEvent->Location.y
        );

    if (selectedItem && selectedItem->Id != -1)
    {
        if (!PhHandleCopyCellEMenuItem(selectedItem))
        {
            switch (selectedItem->Id)
            {
            case ENVIRONMENT_TREE_COLUMN_MENU_ITEM_EDIT:
                {
                    //PPH_ENVIRONMENT_ITEM item = PhItemArray(&Context->Items, node->Id);
                    BOOLEAN refresh;

                    if (PhGetIntegerSetting(L"EnableWarnings") && !PhShowConfirmMessage(
                        Context->WindowHandle,
                        L"edit",
                        L"the selected environment variable",
                        L"Some programs may restrict access or ban your account when editing the environment variable(s) of the process.",
                        FALSE
                        ))
                    {
                        break;
                    }

                    if (PhpShowEditEnvDialog(
                        Context->WindowHandle,
                        Context->ProcessItem,
                        node->NameText->Buffer,
                        node->ValueText->Buffer,
                        &refresh
                        ) == IDOK && refresh)
                    {
                        PhpRefreshEnvironmentList(Context->WindowHandle, Context, Context->ProcessItem);
                    }
                }
                break;
            case ENVIRONMENT_TREE_COLUMN_MENU_ITEM_DELETE:
                {
                    //PPH_ENVIRONMENT_ITEM item = PhItemArray(&Context->Items, node->Id);
                    NTSTATUS status;
                    HANDLE processHandle;

                    if (PhGetIntegerSetting(L"EnableWarnings") && !PhShowConfirmMessage(
                        Context->WindowHandle,
                        L"delete",
                        L"the selected environment variable",
                        L"Some programs may restrict access or ban your account when editing the environment variable(s) of the process.",
                        FALSE
                        ))
                    {
                        break;
                    }

                    if (NT_SUCCESS(status = PhOpenProcess(
                        &processHandle,
                        PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
                        PROCESS_VM_READ | PROCESS_VM_WRITE,
                        Context->ProcessItem->ProcessId
                        )))
                    {
                        LARGE_INTEGER timeout;

                        timeout.QuadPart = -(LONGLONG)UInt32x32To64(10, PH_TIMEOUT_SEC);
                        status = PhSetEnvironmentVariableRemote(
                            processHandle, 
                            &node->NameText->sr,
                            NULL, 
                            &timeout
                            );
                        NtClose(processHandle);

                        PhpRefreshEnvironmentList(Context->WindowHandle, Context, Context->ProcessItem);

                        if (status == STATUS_TIMEOUT)
                        {
                            PhShowStatus(Context->WindowHandle, L"Unable to delete the environment variable.", 0, WAIT_TIMEOUT);
                        }
                        else if (!NT_SUCCESS(status))
                        {
                            PhShowStatus(Context->WindowHandle, L"Unable to delete the environment variable.", status, 0);
                        }
                    }
                    else
                    {
                        PhShowStatus(Context->WindowHandle, L"Unable to open the process.", status, 0);
                    }
                }
                break;
            case ENVIRONMENT_TREE_COLUMN_MENU_ITEM_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(Context->TreeNewHandle, 0);
                    PhSetClipboardString(Context->TreeNewHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            }
        }
    }

    PhDestroyEMenu(menu);
}

static BOOLEAN PhpWordMatchEnvironmentStringRef(
    _In_ PPH_STRINGREF SearchText,
    _In_ PPH_STRINGREF Text
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    remainingPart = *SearchText;

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

static BOOLEAN PhpWordMatchEnvironmentStringZ(
    _In_ PPH_STRING SearchText,
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRef(&text, Text);

    return PhpWordMatchEnvironmentStringRef(&SearchText->sr, &text);
}

VOID PhLoadSettingsEnvironmentList(
    _Inout_ PPH_ENVIRONMENT_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhGetStringSetting(L"EnvironmentTreeListColumns");
    sortSettings = PhGetStringSetting(L"EnvironmentTreeListSort");
    Context->Flags = PhGetIntegerSetting(L"EnvironmentTreeListFlags");

    PhCmLoadSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &settings->sr, &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSaveSettingsEnvironmentList(
    _Inout_ PPH_ENVIRONMENT_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &sortSettings);

    PhSetIntegerSetting(L"EnvironmentTreeListFlags", Context->Flags);
    PhSetStringSetting2(L"EnvironmentTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"EnvironmentTreeListSort", &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSetOptionsEnvironmentList(
    _Inout_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_ ULONG Options
    )
{
    switch (Options)
    {
    case ENVIRONMENT_TREE_MENU_ITEM_HIDE_PROCESS_TYPE:
        Context->HideProcessEnvironment = !Context->HideProcessEnvironment;
        break;
    case ENVIRONMENT_TREE_MENU_ITEM_HIDE_USER_TYPE:
        Context->HideUserEnvironment = !Context->HideUserEnvironment;
        break;
    case ENVIRONMENT_TREE_MENU_ITEM_HIDE_SYSTEM_TYPE:
        Context->HideSystemEnvironment = !Context->HideSystemEnvironment;
        break;
    case ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_PROCESS_TYPE:
        Context->HighlightProcessEnvironment = !Context->HighlightProcessEnvironment;
        break;
    case ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_USER_TYPE:
        Context->HighlightUserEnvironment = !Context->HighlightUserEnvironment;
        break;
    case ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_SYSTEM_TYPE:
        Context->HighlightSystemEnvironment = !Context->HighlightSystemEnvironment;
        break;
    }
}

BOOLEAN PhpEnvironmentNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
)
{
    PPHP_PROCESS_ENVIRONMENT_TREENODE poolTagNode1 = *(PPHP_PROCESS_ENVIRONMENT_TREENODE *)Entry1;
    PPHP_PROCESS_ENVIRONMENT_TREENODE poolTagNode2 = *(PPHP_PROCESS_ENVIRONMENT_TREENODE *)Entry2;

    return PhEqualStringRef(&poolTagNode1->NameText->sr, &poolTagNode2->NameText->sr, TRUE);
}

ULONG PhpEnvironmentNodeHashtableHashFunction(
    _In_ PVOID Entry
)
{
    return PhHashStringRef(&(*(PPHP_PROCESS_ENVIRONMENT_TREENODE*)Entry)->NameText->sr, TRUE);
}

VOID PhpDestroyEnvironmentNode(
    _In_ PPHP_PROCESS_ENVIRONMENT_TREENODE Node
)
{
    if (Node->NameText)
        PhDereferenceObject(Node->NameText);
    if (Node->ValueText)
        PhDereferenceObject(Node->ValueText);

    PhFree(Node);
}

PPHP_PROCESS_ENVIRONMENT_TREENODE PhpAddEnvironmentRootNode(
    _In_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_ PROCESS_ENVIRONMENT_TREENODE_TYPE Type,
    _In_ PPH_STRING KeyPath,
    _In_opt_ PPH_STRING Value
    )
{
    PPHP_PROCESS_ENVIRONMENT_TREENODE node;

    node = PhAllocateZero(sizeof(PHP_PROCESS_ENVIRONMENT_TREENODE));
    PhInitializeTreeNewNode(&node->Node);

    memset(node->TextCache, 0, sizeof(PH_STRINGREF) * ENVIRONMENT_COLUMN_ITEM_MAXIMUM);
    node->Node.TextCache = node->TextCache;
    node->Node.TextCacheSize = ENVIRONMENT_COLUMN_ITEM_MAXIMUM;
    node->Children = PhCreateList(1);

    PhReferenceObject(KeyPath);
    node->Type = Type;
    node->NameText = KeyPath;

    if (Value)
    {
        PhReferenceObject(Value);
        node->ValueText = Value;
    }

    PhAddEntryHashtable(Context->NodeHashtable, &node);
    PhAddItemList(Context->NodeList, node);

    if (Context->TreeFilterSupport.FilterList)
        node->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->TreeFilterSupport, &node->Node);

    return node;
}

PPHP_PROCESS_ENVIRONMENT_TREENODE PhpAddEnvironmentChildNode(
    _In_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_opt_ PPHP_PROCESS_ENVIRONMENT_TREENODE ParentNode,
    _In_ ULONG Id,
    _In_ PROCESS_ENVIRONMENT_TREENODE_TYPE Type,
    _In_ PPH_STRING Name,
    _In_opt_ PPH_STRING Value
    )
{
    PPHP_PROCESS_ENVIRONMENT_TREENODE node;

    node = PhpAddEnvironmentRootNode(Context, Type, Name, Value);

    if (ParentNode)
    {
        node->HasChildren = FALSE;

        // This is a child node.
        node->Parent = ParentNode;
        PhAddItemList(ParentNode->Children, node);
    }
    else
    {
        node->HasChildren = TRUE;
        node->Node.Expanded = TRUE;

        // This is a root node.
        PhAddItemList(Context->NodeRootList, node);
    }

    return node;
}

PPHP_PROCESS_ENVIRONMENT_TREENODE PhpFindEnvironmentNode(
    _In_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_ PWSTR KeyPath
)
{
    PHP_PROCESS_ENVIRONMENT_TREENODE lookupWindowNode;
    PPHP_PROCESS_ENVIRONMENT_TREENODE lookupWindowNodePtr = &lookupWindowNode;
    PPHP_PROCESS_ENVIRONMENT_TREENODE *windowNode;

    PhInitializeStringRefLongHint(&lookupWindowNode.NameText->sr, KeyPath);

    windowNode = (PPHP_PROCESS_ENVIRONMENT_TREENODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupWindowNodePtr
        );

    if (windowNode)
        return *windowNode;
    else
        return NULL;
}

VOID PhpRemoveEnvironmentNode(
    _In_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_ PPHP_PROCESS_ENVIRONMENT_TREENODE Node
)
{
    ULONG index = 0;

    // Remove from hashtable/list and cleanup.
    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != -1)
    {
        PhRemoveItemList(Context->NodeList, index);
    }

    PhpDestroyEnvironmentNode(Node);
    //TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhpUpdateEnvironmentNode(
    _In_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_ PPHP_PROCESS_ENVIRONMENT_TREENODE Node
    )
{
    memset(Node->TextCache, 0, sizeof(PH_STRINGREF) * ENVIRONMENT_COLUMN_ITEM_MAXIMUM);

    PhInvalidateTreeNewNode(&Node->Node, TN_CACHE_COLOR);
    //TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhpExpandAllEnvironmentNodes(
    _In_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_ BOOLEAN Expand
    )
{
    ULONG i;
    BOOLEAN needsRestructure = FALSE;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_MODULE_NODE node = Context->NodeList->Items[i];

        if (node->Node.Expanded != Expand)
        {
            node->Node.Expanded = Expand;
            needsRestructure = TRUE;
        }
    }

    if (needsRestructure)
        TreeNew_NodesStructured(Context->TreeNewHandle);
}

#define SORT_FUNCTION(Column) PhpEnvironmentTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpEnvironmentTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPHP_PROCESS_ENVIRONMENT_TREENODE node1 = *(PPHP_PROCESS_ENVIRONMENT_TREENODE*)_elem1; \
    PPHP_PROCESS_ENVIRONMENT_TREENODE node2 = *(PPHP_PROCESS_ENVIRONMENT_TREENODE*)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
         sortResult = uintptrcmp((ULONG_PTR)node1->Node.Index, (ULONG_PTR)node2->Node.Index); \
    \
    return PhModifySort(sortResult, ((PPH_ENVIRONMENT_CONTEXT)_context)->TreeNewSortOrder); \
}

LONG PhpEnvironmentTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
        Result = uintptrcmp((ULONG_PTR)((PPHP_PROCESS_ENVIRONMENT_TREENODE)Node1)->Node.Index, (ULONG_PTR)((PPHP_PROCESS_ENVIRONMENT_TREENODE)Node2)->Node.Index);

    return PhModifySort(Result, SortOrder);
}

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringWithNull(node1->NameText, node2->NameText, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Value)
{
    sortResult = PhCompareStringWithNull(node1->ValueText, node2->ValueText, FALSE);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpEnvironmentTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
)
{
    PPH_ENVIRONMENT_CONTEXT context;
    PPHP_PROCESS_ENVIRONMENT_TREENODE node;

    context = Context;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPHP_PROCESS_ENVIRONMENT_TREENODE)getChildren->Node;

            if (context->TreeNewSortOrder == NoSortOrder)
            {
                if (!node)
                {
                    getChildren->Children = (PPH_TREENEW_NODE *)context->NodeRootList->Items;
                    getChildren->NumberOfChildren = context->NodeRootList->Count;
                }
                else
                {
                    getChildren->Children = (PPH_TREENEW_NODE *)node->Children->Items;
                    getChildren->NumberOfChildren = node->Children->Count;
                }
            }
            else
            {
                if (!node)
                {
                    static PVOID sortFunctions[] =
                    {
                        SORT_FUNCTION(Name),
                        SORT_FUNCTION(Value)
                    };
                    int (__cdecl *sortFunction)(void *, const void *, const void *);

                    if (context->TreeNewSortColumn < ENVIRONMENT_COLUMN_ITEM_MAXIMUM)
                        sortFunction = sortFunctions[context->TreeNewSortColumn];
                    else
                        sortFunction = NULL;

                    if (sortFunction)
                    {
                        qsort_s(context->NodeList->Items, context->NodeList->Count, sizeof(PVOID), sortFunction, context);
                    }

                    getChildren->Children = (PPH_TREENEW_NODE *)context->NodeList->Items;
                    getChildren->NumberOfChildren = context->NodeList->Count;
                }
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;
            node = (PPHP_PROCESS_ENVIRONMENT_TREENODE)isLeaf->Node;

            if (context->TreeNewSortOrder == NoSortOrder)
                isLeaf->IsLeaf = !node->HasChildren;
            else
                isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PPHP_PROCESS_ENVIRONMENT_TREENODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case ENVIRONMENT_COLUMN_ITEM_NAME:
                getCellText->Text = PhGetStringRef(node->NameText);
                break;
            case ENVIRONMENT_COLUMN_ITEM_VALUE:
                getCellText->Text = PhGetStringRef(node->ValueText);
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = (PPH_TREENEW_GET_NODE_COLOR)Parameter1;
            node = (PPHP_PROCESS_ENVIRONMENT_TREENODE)getNodeColor->Node;

            if (node->HasChildren)
            {
                NOTHING;
            }
            else
            {
                if (context->HighlightProcessEnvironment && node->Type == PROCESS_ENVIRONMENT_TREENODE_TYPE_PROCESS)
                    getNodeColor->BackColor = PhCsColorServiceProcesses;
                else if (context->HighlightUserEnvironment && node->Type == PROCESS_ENVIRONMENT_TREENODE_TYPE_USER)
                    getNodeColor->BackColor = PhCsColorOwnProcesses;
                else if (context->HighlightSystemEnvironment && node->Type == PROCESS_ENVIRONMENT_TREENODE_TYPE_SYSTEM)
                    getNodeColor->BackColor = PhCsColorSystemProcesses;
            }

            getNodeColor->Flags = TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &context->TreeNewSortColumn, &context->TreeNewSortOrder);
            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(hwnd);

            // HACK
            if (context->TreeFilterSupport.FilterList)
                PhApplyTreeNewFilters(&context->TreeFilterSupport);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            PhpShowEnvironmentNodeContextMenu(context, contextMenuEvent);
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = AscendingSortOrder;
            PhInitializeTreeNewColumnMenu(&data);

            data.Selection = PhShowEMenu(
                data.Menu, 
                hwnd, 
                PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, 
                data.MouseEvent->ScreenLocation.x, 
                data.MouseEvent->ScreenLocation.y
                );

            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            SendMessage(context->WindowHandle, WM_COMMAND, WM_PH_SET_LIST_VIEW_SETTINGS, 0); // HACK
        }
        return TRUE;
    }

    return FALSE;
}

VOID PhpClearEnvironmentTree(
    _In_ PPH_ENVIRONMENT_CONTEXT Context
)
{
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
        PhpDestroyEnvironmentNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
    PhClearList(Context->NodeRootList);

    PhpClearEnvironmentItems(Context);
}

PPHP_PROCESS_ENVIRONMENT_TREENODE PhpGetSelectedEnvironmentNode(
    _In_ PPH_ENVIRONMENT_CONTEXT Context
    )
{
    PPHP_PROCESS_ENVIRONMENT_TREENODE windowNode = NULL;
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        windowNode = Context->NodeList->Items[i];

        if (windowNode->Node.Selected)
            return windowNode;
    }

    return NULL;
}

VOID PhpGetSelectedEnvironmentNodes(
    _In_ PPH_ENVIRONMENT_CONTEXT Context,
    _Out_ PPHP_PROCESS_ENVIRONMENT_TREENODE **PoolTags,
    _Out_ PULONG NumberOfPoolTags
    )
{
    PPH_LIST list;
    ULONG i;

    list = PhCreateList(2);

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PPHP_PROCESS_ENVIRONMENT_TREENODE node = (PPHP_PROCESS_ENVIRONMENT_TREENODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node);
        }
    }

    *PoolTags = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfPoolTags = list->Count;

    PhDereferenceObject(list);
}

VOID PhpInitializeEnvironmentTree(
    _Inout_ PPH_ENVIRONMENT_CONTEXT Context
    )
{
    Context->NodeList = PhCreateList(100);
    Context->NodeRootList = PhCreateList(30);
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PHP_PROCESS_ENVIRONMENT_TREENODE),
        PhpEnvironmentNodeHashtableEqualFunction,
        PhpEnvironmentNodeHashtableHashFunction,
        100
        );

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");
    TreeNew_SetCallback(Context->TreeNewHandle, PhpEnvironmentTreeNewCallback, Context);

    PhAddTreeNewColumn(Context->TreeNewHandle, ENVIRONMENT_COLUMN_ITEM_NAME, TRUE, L"Name", 250, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, ENVIRONMENT_COLUMN_ITEM_VALUE, TRUE, L"Value", 250, PH_ALIGN_LEFT, 1, 0);

    TreeNew_SetTriState(Context->TreeNewHandle, TRUE);
    TreeNew_SetSort(Context->TreeNewHandle, ENVIRONMENT_COLUMN_ITEM_NAME, NoSortOrder);

    PhCmInitializeManager(&Context->Cm, Context->TreeNewHandle, PHMOTLC_MAXIMUM, PhpEnvironmentTreeNewPostSortFunction);
    PhInitializeTreeNewFilterSupport(&Context->TreeFilterSupport, Context->TreeNewHandle, Context->NodeList);
}

VOID PhpDeleteEnvironmentTree(
    _In_ PPH_ENVIRONMENT_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PhpDestroyEnvironmentNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

BOOLEAN PhpProcessEnvironmentTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_ENVIRONMENT_CONTEXT context = Context;
    PPHP_PROCESS_ENVIRONMENT_TREENODE environmentNode = (PPHP_PROCESS_ENVIRONMENT_TREENODE)Node;

    if (!environmentNode->Parent && environmentNode->Children && environmentNode->Children->Count == 0)
        return FALSE;
    if (context->TreeNewSortOrder != NoSortOrder && environmentNode->HasChildren)
        return FALSE;

    if (context->HideProcessEnvironment && environmentNode->Type == PROCESS_ENVIRONMENT_TREENODE_TYPE_PROCESS)
        return FALSE;
    if (context->HideUserEnvironment && environmentNode->Type == PROCESS_ENVIRONMENT_TREENODE_TYPE_USER)
        return FALSE;
    if (context->HideSystemEnvironment && environmentNode->Type == PROCESS_ENVIRONMENT_TREENODE_TYPE_SYSTEM)
        return FALSE;

    if (PhIsNullOrEmptyString(context->SearchboxText))
        return TRUE;

    if (!PhIsNullOrEmptyString(environmentNode->NameText))
    {
        if (PhpWordMatchEnvironmentStringRef(&context->SearchboxText->sr, &environmentNode->NameText->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(environmentNode->ValueText))
    {
        if (PhpWordMatchEnvironmentStringRef(&context->SearchboxText->sr, &environmentNode->ValueText->sr))
            return TRUE;
    }

    return FALSE;
}

INT_PTR CALLBACK PhpProcessEnvironmentDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_ENVIRONMENT_CONTEXT context;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, NULL, &propPageContext, &processItem))
    {
        context = propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context = propPageContext->Context = PhAllocateZero(sizeof(PH_ENVIRONMENT_CONTEXT));
            context->WindowHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            context->SearchWindowHandle = GetDlgItem(hwndDlg, IDC_SEARCH);
            context->SearchboxText = PhReferenceEmptyString();
            context->ProcessItem = processItem;

            PhCreateSearchControl(hwndDlg, context->SearchWindowHandle, L"Search Environment (Ctrl+K)");
            Edit_SetSel(context->SearchWindowHandle, 0, -1);
            PhpInitializeEnvironmentTree(context);

            PhInitializeArray(&context->Items, sizeof(PH_ENVIRONMENT_ITEM), 100);
            context->TreeFilterEntry = PhAddTreeNewFilter(&context->TreeFilterSupport, PhpProcessEnvironmentTreeFilterCallback, context);

            PhMoveReference(&context->StatusMessage, PhCreateString(L"There are no environment variables to display."));
            TreeNew_SetEmptyText(context->TreeNewHandle, &context->StatusMessage->sr, 0);
            PhLoadSettingsEnvironmentList(context);

            PhpRefreshEnvironmentList(hwndDlg, context, processItem);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveSettingsEnvironmentList(context);
            PhpDeleteEnvironmentTree(context);

            PhpClearEnvironmentItems(context);
            PhDeleteArray(&context->Items);
            PhClearReference(&context->StatusMessage);

            if (DestroyEnvironmentBlock)
            {
                if (context->SystemDefaultEnvironment)
                    DestroyEnvironmentBlock(context->SystemDefaultEnvironment);
                if (context->UserDefaultEnvironment)
                    DestroyEnvironmentBlock(context->UserDefaultEnvironment);
            }

            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_ITEM dialogItem;

            if (dialogItem = PhBeginPropPageLayout(hwndDlg, propPageContext))
            {
                PhAddPropPageLayoutItem(hwndDlg, context->SearchWindowHandle, dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_TOP);
                PhAddPropPageLayoutItem(hwndDlg, context->TreeNewHandle, dialogItem, PH_ANCHOR_ALL);
                PhEndPropPageLayout(hwndDlg, propPageContext);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case EN_CHANGE:
                {
                    PPH_STRING newSearchboxText;

                    if (GET_WM_COMMAND_HWND(wParam, lParam) != context->SearchWindowHandle)
                        break;

                    newSearchboxText = PH_AUTO(PhGetWindowText(context->SearchWindowHandle));

                    if (!PhEqualString(context->SearchboxText, newSearchboxText, FALSE))
                    {
                        // Cache the current search text for our callback.
                        PhSwapReference(&context->SearchboxText, newSearchboxText);

                        // Expand any hidden nodes to make search results visible.
                        PhpExpandAllEnvironmentNodes(context, TRUE);

                        PhApplyTreeNewFilters(&context->TreeFilterSupport);
                    }
                }
                break;
            }

            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_OPTIONS:
                {
                    RECT rect;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM processMenuItem;
                    PPH_EMENU_ITEM userMenuItem;
                    PPH_EMENU_ITEM systemMenuItem;
                    PPH_EMENU_ITEM highlightProcessMenuItem;
                    PPH_EMENU_ITEM highlightUserMenuItem;
                    PPH_EMENU_ITEM highlightSystemMenuItem;
                    PPH_EMENU_ITEM newProcessMenuItem;
                    PPH_EMENU_ITEM selectedItem;

                    GetWindowRect(GetDlgItem(hwndDlg, IDC_OPTIONS), &rect);

                    processMenuItem = PhCreateEMenuItem(0, ENVIRONMENT_TREE_MENU_ITEM_HIDE_PROCESS_TYPE, L"Hide process", NULL, NULL);
                    userMenuItem = PhCreateEMenuItem(0, ENVIRONMENT_TREE_MENU_ITEM_HIDE_USER_TYPE, L"Hide user", NULL, NULL);
                    systemMenuItem = PhCreateEMenuItem(0, ENVIRONMENT_TREE_MENU_ITEM_HIDE_SYSTEM_TYPE, L"Hide system", NULL, NULL);
                    highlightProcessMenuItem = PhCreateEMenuItem(0, ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_PROCESS_TYPE, L"Highlight process", NULL, NULL);
                    highlightUserMenuItem = PhCreateEMenuItem(0, ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_USER_TYPE, L"Highlight user", NULL, NULL);
                    highlightSystemMenuItem = PhCreateEMenuItem(0, ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_SYSTEM_TYPE, L"Highlight system", NULL, NULL);
                    newProcessMenuItem = PhCreateEMenuItem(0, ENVIRONMENT_TREE_MENU_ITEM_NEW_ENVIRONMENT_VARIABLE, L"New variable...", NULL, NULL);

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, processMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, userMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, systemMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightProcessMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightUserMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightSystemMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, newProcessMenuItem, ULONG_MAX);

                    if (context->HideProcessEnvironment)
                        processMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HideUserEnvironment)
                        userMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HideSystemEnvironment)
                        systemMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightProcessEnvironment)
                        highlightProcessMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightUserEnvironment)
                        highlightUserMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightSystemEnvironment)
                        highlightSystemMenuItem->Flags |= PH_EMENU_CHECKED;

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        rect.left,
                        rect.bottom
                        );

                    if (selectedItem && selectedItem->Id)
                    {
                        if (selectedItem->Id == ENVIRONMENT_TREE_MENU_ITEM_NEW_ENVIRONMENT_VARIABLE)
                        {
                            BOOLEAN refresh;

                            if (PhGetIntegerSetting(L"EnableWarnings") && !PhShowConfirmMessage(
                                hwndDlg,
                                L"create",
                                L"new environment variable(s)",
                                L"Some programs may restrict access or ban your account when creating new environment variable(s).",
                                FALSE
                                ))
                            {
                                break;
                            }

                            if (PhpShowEditEnvDialog(hwndDlg, processItem, L"", NULL, &refresh) == IDOK && refresh)
                            {
                                PhpRefreshEnvironmentList(hwndDlg, context, processItem);
                            }
                        }
                        else
                        {
                            PhSetOptionsEnvironmentList(context, selectedItem->Id);
                            PhSaveSettingsEnvironmentList(context);
                            PhApplyTreeNewFilters(&context->TreeFilterSupport);
                        }
                    }

                    PhDestroyEMenu(menu);
                }
                break;
            case IDC_REFRESH:
                {
                    PhpRefreshEnvironmentList(hwndDlg, context, processItem);
                }
                break;
            case WM_PH_SET_LIST_VIEW_SETTINGS: // HACK
                {
                    PPHP_PROCESS_ENVIRONMENT_TREENODE item = PhpGetSelectedEnvironmentNode(context);
                    BOOLEAN refresh;

                    if (!item || item->Type == PROCESS_ENVIRONMENT_TREENODE_TYPE_GROUP)
                        break;

                    if (PhGetIntegerSetting(L"EnableWarnings") && !PhShowConfirmMessage(
                        hwndDlg,
                        L"edit",
                        L"the selected environment variable",
                        L"Some programs may restrict access or ban your account when editing the environment variable(s) of the process.",
                        FALSE
                        ))
                    {
                        break;
                    }

                    if (PhpShowEditEnvDialog(
                        hwndDlg,
                        context->ProcessItem,
                        item->NameText->Buffer,
                        item->ValueText->Buffer,
                        &refresh
                        ) == IDOK && refresh)
                    {
                        PhpRefreshEnvironmentList(hwndDlg, context, context->ProcessItem);
                    }
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
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)GetDlgItem(hwndDlg, IDC_REFRESH));
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}
