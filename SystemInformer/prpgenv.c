/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2018-2023
 *
 */

#include <phapp.h>
#include <cpysave.h>
#include <emenu.h>
#include <settings.h>

#include <apiimport.h>
#include <phsettings.h>
#include <procprp.h>
#include <procprpp.h>
#include <procprv.h>

typedef enum _ENVIRONMENT_TREE_MENU_ITEM
{
    ENVIRONMENT_TREE_MENU_ITEM_HIDE_PROCESS_TYPE = 1,
    ENVIRONMENT_TREE_MENU_ITEM_HIDE_USER_TYPE,
    ENVIRONMENT_TREE_MENU_ITEM_HIDE_SYSTEM_TYPE,
    ENVIRONMENT_TREE_MENU_ITEM_HIDE_CMD_TYPE,
    ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_PROCESS_TYPE,
    ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_USER_TYPE,
    ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_SYSTEM_TYPE,
    ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_CMD_TYPE,
    ENVIRONMENT_TREE_MENU_ITEM_NEW_ENVIRONMENT_VARIABLE,
    ENVIRONMENT_TREE_MENU_ITEM_MAXIMUM
} ENVIRONMENT_TREE_MENU_ITEM;

typedef enum _ENVIRONMENT_TREE_COLUMN_ITEM
{
    ENVIRONMENT_COLUMN_ITEM_NAME,
    ENVIRONMENT_COLUMN_ITEM_VALUE,
    ENVIRONMENT_COLUMN_ITEM_MAXIMUM
} ENVIRONMENT_TREE_COLUMN_ITEM;

typedef enum _PROCESS_ENVIRONMENT_TREENODE_TYPE
{
    PROCESS_ENVIRONMENT_TREENODE_TYPE_GROUP = 0x1,
    PROCESS_ENVIRONMENT_TREENODE_TYPE_PROCESS = 0x2,
    PROCESS_ENVIRONMENT_TREENODE_TYPE_USER = 0x4,
    PROCESS_ENVIRONMENT_TREENODE_TYPE_SYSTEM = 0x8
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
    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN IsCmdVariable : 1;
            BOOLEAN Spare : 7;
        };
    };
} PHP_PROCESS_ENVIRONMENT_TREENODE, *PPHP_PROCESS_ENVIRONMENT_TREENODE;

PPHP_PROCESS_ENVIRONMENT_TREENODE PhpAddEnvironmentNode(
    _In_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_opt_ PPHP_PROCESS_ENVIRONMENT_TREENODE ParentNode,
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

_Success_(return)
BOOLEAN PhpGetSelectedEnvironmentNodes(
    _In_ PPH_ENVIRONMENT_CONTEXT Context,
    _Out_ PPHP_PROCESS_ENVIRONMENT_TREENODE * *Nodes,
    _Out_ PULONG NumberOfNodes
    );

BOOLEAN PhpHasSelectedEnvironmentGroupNode(
    _In_ PPHP_PROCESS_ENVIRONMENT_TREENODE* Nodes,
    _In_ ULONG NumberOfNodes
    );

#define ID_ENV_EDIT 50001
#define ID_ENV_DELETE 50002
#define ID_ENV_COPY 50003

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
    SIZE_T i;
    PPH_ENVIRONMENT_ITEM item;

    for (i = 0; i < Context->Items.Count; i++)
    {
        item = PhItemArray(&Context->Items, i);

        if (item->Name)
            PhDereferenceObject(item->Name);
        if (item->Value)
            PhDereferenceObject(item->Value);
    }

    PhClearArray(&Context->Items);
}

VOID PhpSetEnvironmentListStatusMessage(
    _Inout_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_ ULONG Status
    )
{
    if (Context->ProcessItem->State & PH_PROCESS_ITEM_REMOVED || Status == STATUS_PARTIAL_COPY)
    {
        PhMoveReference(&Context->StatusMessage, PhCreateString(L"There are no environment variables to display."));
        TreeNew_SetEmptyText(Context->TreeNewHandle, &Context->StatusMessage->sr, 0);
    }
    else
    {
        PPH_STRING statusMessage;

        statusMessage = PhGetStatusMessage(Status, 0);
        PhMoveReference(&Context->StatusMessage, PhConcatStrings2(
            L"Unable to query environment information:\n",
            PhGetStringOrDefault(statusMessage, L"Unknown error.")
            ));
        TreeNew_SetEmptyText(Context->TreeNewHandle, &Context->StatusMessage->sr, 0);
        //TreeNew_NodesStructured(Context->TreeNewHandle);
        PhClearReference(&statusMessage);
    }
}

VOID PhpRefreshEnvironmentList(
    _Inout_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    PVOID environment;
    ULONG environmentLength;
    ULONG enumerationKey;
    PH_ENVIRONMENT_VARIABLE variable;
    PPH_ENVIRONMENT_ITEM item;
    PPHP_PROCESS_ENVIRONMENT_TREENODE processRootNode;
    PPHP_PROCESS_ENVIRONMENT_TREENODE userRootNode;
    PPHP_PROCESS_ENVIRONMENT_TREENODE systemRootNode;
    PVOID systemDefaultEnvironment = NULL;
    PVOID userDefaultEnvironment = NULL;
    SIZE_T i;

    PhpClearEnvironmentTree(Context);
    processRootNode = PhpAddEnvironmentNode(Context, NULL, PROCESS_ENVIRONMENT_TREENODE_TYPE_GROUP | PROCESS_ENVIRONMENT_TREENODE_TYPE_PROCESS, PhaCreateString(L"Process"), NULL);
    userRootNode = PhpAddEnvironmentNode(Context, NULL, PROCESS_ENVIRONMENT_TREENODE_TYPE_GROUP | PROCESS_ENVIRONMENT_TREENODE_TYPE_USER, PhaCreateString(L"User"), NULL);
    systemRootNode = PhpAddEnvironmentNode(Context, NULL, PROCESS_ENVIRONMENT_TREENODE_TYPE_GROUP | PROCESS_ENVIRONMENT_TREENODE_TYPE_SYSTEM, PhaCreateString(L"System"), NULL);

    status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        ProcessItem->ProcessId
        );

    if (!NT_SUCCESS(status))
    {
        PhpSetEnvironmentListStatusMessage(Context, status);
        TreeNew_NodesStructured(Context->TreeNewHandle);
        return;
    }

    if (NT_SUCCESS(status))
    {
        HANDLE tokenHandle;
        ULONG flags = 0;

        if (CreateEnvironmentBlock_Import())
        {
            CreateEnvironmentBlock_Import()(&systemDefaultEnvironment, NULL, FALSE);

            if (NT_SUCCESS(PhOpenProcessToken(
                processHandle,
                TOKEN_QUERY | TOKEN_DUPLICATE,
                &tokenHandle
                )))
            {
                CreateEnvironmentBlock_Import()(&userDefaultEnvironment, tokenHandle, FALSE);
                NtClose(tokenHandle);
            }
        }

#ifdef _WIN64
        if (ProcessItem->IsWow64)
            flags |= PH_GET_PROCESS_ENVIRONMENT_WOW64;
#endif

        status = PhGetProcessEnvironment(
            processHandle,
            flags,
            &environment,
            &environmentLength
            );

        if (NT_SUCCESS(status))
        {
            enumerationKey = 0;

            while (PhEnumProcessEnvironmentVariables(environment, environmentLength, &enumerationKey, &variable))
            {
                PH_ENVIRONMENT_ITEM item;

                item.Name = PhCreateString2(&variable.Name);
                item.Value = PhCreateString2(&variable.Value);

                PhAddItemArray(&Context->Items, &item);
            }

            PhFreePage(environment);
        }
        else
        {
            PhpSetEnvironmentListStatusMessage(Context, status);
        }

        NtClose(processHandle);
    }

    for (i = 0; i < Context->Items.Count; i++)
    {
        PPHP_PROCESS_ENVIRONMENT_TREENODE node;
        PPHP_PROCESS_ENVIRONMENT_TREENODE parentNode;
        PROCESS_ENVIRONMENT_TREENODE_TYPE nodeType;
        PPH_STRING variableValue;

        item = PhItemArray(&Context->Items, i);

        if (!item->Name)
            continue;

        if (systemDefaultEnvironment && PhQueryEnvironmentVariable(
            systemDefaultEnvironment,
            &item->Name->sr,
            NULL
            ) == STATUS_BUFFER_TOO_SMALL)
        {
            nodeType = PROCESS_ENVIRONMENT_TREENODE_TYPE_SYSTEM;
            parentNode = systemRootNode;

            if (NT_SUCCESS(PhQueryEnvironmentVariable(
                systemDefaultEnvironment,
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
        else if (userDefaultEnvironment && PhQueryEnvironmentVariable(
            userDefaultEnvironment,
            &item->Name->sr,
            NULL
            ) == STATUS_BUFFER_TOO_SMALL)
        {
            nodeType = PROCESS_ENVIRONMENT_TREENODE_TYPE_USER;
            parentNode = userRootNode;

            if (NT_SUCCESS(PhQueryEnvironmentVariable(
                userDefaultEnvironment,
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

        node = PhpAddEnvironmentNode(
            Context,
            parentNode,
            nodeType,
            item->Name,
            item->Value
            );

        if (item->Name && item->Name->Buffer[0] == L'=') // HACK (dmex)
        {
            node->IsCmdVariable = TRUE;
        }
    }

    PhApplyTreeNewFilters(&Context->TreeFilterSupport);
    TreeNew_NodesStructured(Context->TreeNewHandle);

    if (DestroyEnvironmentBlock_Import())
    {
        if (systemDefaultEnvironment)
            DestroyEnvironmentBlock_Import()(systemDefaultEnvironment);
        if (userDefaultEnvironment)
            DestroyEnvironmentBlock_Import()(userDefaultEnvironment);
    }
}

NTSTATUS PhpEditDlgSetEnvironment(
    _Inout_ PEDIT_ENV_DIALOG_CONTEXT Context,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_STRING Name,
    _In_ PPH_STRING Value
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE processHandle = NULL;
    LARGE_INTEGER timeout;

    // Windows 8 requires ALL_ACCESS for PLM execution requests. (dmex)
    if (WindowsVersion >= WINDOWS_8 && WindowsVersion <= WINDOWS_8_1)
    {
        status = PhOpenProcess(
            &processHandle,
            PROCESS_ALL_ACCESS,
            ProcessItem->ProcessId
            );
    }

    // Windows 10 and above require SET_LIMITED for PLM execution requests. (dmex)
    if (!NT_SUCCESS(status))
    {
        status = PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_LIMITED_INFORMATION |
            PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
            PROCESS_VM_READ | PROCESS_VM_WRITE,
            ProcessItem->ProcessId
            );
    }

    if (NT_SUCCESS(status))
    {
        timeout.QuadPart = -(LONGLONG)UInt32x32To64(10, PH_TIMEOUT_SEC);

        // Delete the old environment variable if necessary.
        if (!PhEqualStringZ(Context->Name, L"", FALSE) &&
            !PhEqualString2(Name, Context->Name, FALSE))
        {
            PH_STRINGREF nameSr;

            PhInitializeStringRefLongHint(&nameSr, Context->Name);
            PhSetEnvironmentVariableRemote(
                processHandle,
                &nameSr,
                NULL,
                &timeout
                );
        }

        status = PhSetEnvironmentVariableRemote(
            processHandle,
            &Name->sr,
            &Value->sr,
            &timeout
            );

        NtClose(processHandle);
    }

    return status;
}

NTSTATUS PhpEditDeleteEnvironment(
    _Inout_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_STRING Name
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE processHandle = NULL;
    LARGE_INTEGER timeout;

    // Windows 8 requires ALL_ACCESS for PLM execution requests. (dmex)
    if (WindowsVersion >= WINDOWS_8 && WindowsVersion <= WINDOWS_8_1)
    {
        status = PhOpenProcess(
            &processHandle,
            PROCESS_ALL_ACCESS,
            ProcessItem->ProcessId
            );
    }

    // Windows 10 and above require SET_LIMITED for PLM execution requests. (dmex)
    if (!NT_SUCCESS(status))
    {
        status = PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_LIMITED_INFORMATION |
            PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
            PROCESS_VM_READ | PROCESS_VM_WRITE,
            ProcessItem->ProcessId
            );
    }

    if (NT_SUCCESS(status))
    {
        timeout.QuadPart = -(LONGLONG)UInt32x32To64(10, PH_TIMEOUT_SEC);
        status = PhSetEnvironmentVariableRemote(
            processHandle,
            &Name->sr,
            NULL,
            &timeout
            );

        NtClose(processHandle);
    }

    return status;
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
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhSetApplicationWindowIcon(hwndDlg);

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

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
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
                    NTSTATUS status = STATUS_UNSUCCESSFUL;
                    PPH_STRING name;
                    PPH_STRING value;

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

                    name = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_NAME)));
                    value = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_VALUE)));

                    if (!PhIsNullOrEmptyString(name))
                    {
                        if (!PhEqualString2(name, context->Name, FALSE) ||
                            !context->Value || !PhEqualString2(value, context->Value, FALSE))
                        {
                            status = PhpEditDlgSetEnvironment(
                                context,
                                context->ProcessItem,
                                name,
                                value
                                );

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
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
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

    result = PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_EDITENV),
        ParentWindowHandle,
        PhpEditEnvDlgProc,
        &context
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
    PPHP_PROCESS_ENVIRONMENT_TREENODE* nodes;
    ULONG numberOfNodes;
    PPH_EMENU menu;
    PPH_EMENU_ITEM selectedItem;

    if (!PhpGetSelectedEnvironmentNodes(Context, &nodes, &numberOfNodes))
        return;
    if (numberOfNodes == 0)
        return;

    menu = PhCreateEMenu();
    if (!PhpHasSelectedEnvironmentGroupNode(nodes, numberOfNodes))
    {
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_ENV_EDIT, L"Edit", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_ENV_DELETE, L"Delete", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    }
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_ENV_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
    PhInsertCopyCellEMenuItem(menu, ID_ENV_COPY, Context->TreeNewHandle, ContextMenuEvent->Column);

    selectedItem = PhShowEMenu(
        menu,
        Context->WindowHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        ContextMenuEvent->Location.x,
        ContextMenuEvent->Location.y
        );

    if (selectedItem && selectedItem->Id != ULONG_MAX)
    {
        if (!PhHandleCopyCellEMenuItem(selectedItem))
        {
            SendMessage(Context->WindowHandle, WM_COMMAND, selectedItem->Id, 0);
        }
    }

    PhDestroyEMenu(menu);
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
    case ENVIRONMENT_TREE_MENU_ITEM_HIDE_CMD_TYPE:
        Context->HideCmdTypeEnvironment = !Context->HideCmdTypeEnvironment;
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
    case ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_CMD_TYPE:
        Context->HighlightCmdEnvironment = !Context->HighlightCmdEnvironment;
        break;
    }
}

BOOLEAN PhpEnvironmentNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPHP_PROCESS_ENVIRONMENT_TREENODE node1 = *(PPHP_PROCESS_ENVIRONMENT_TREENODE *)Entry1;
    PPHP_PROCESS_ENVIRONMENT_TREENODE node2 = *(PPHP_PROCESS_ENVIRONMENT_TREENODE *)Entry2;

    return PhEqualStringRef(&node1->NameText->sr, &node2->NameText->sr, TRUE);
}

ULONG PhpEnvironmentNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashStringRefEx(&(*(PPHP_PROCESS_ENVIRONMENT_TREENODE*)Entry)->NameText->sr, TRUE, PH_STRING_HASH_X65599);
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

PPHP_PROCESS_ENVIRONMENT_TREENODE PhpAddEnvironmentNode(
    _In_ PPH_ENVIRONMENT_CONTEXT Context,
    _In_opt_ PPHP_PROCESS_ENVIRONMENT_TREENODE ParentNode,
    _In_ PROCESS_ENVIRONMENT_TREENODE_TYPE Type,
    _In_ PPH_STRING Name,
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

    node->Type = Type;
    PhSetReference(&node->NameText, Name);
    if (Value) PhSetReference(&node->ValueText, Value);

    PhAddEntryHashtable(Context->NodeHashtable, &node);
    PhAddItemList(Context->NodeList, node);

    if (Context->TreeFilterSupport.FilterList)
        node->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->TreeFilterSupport, &node->Node);

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
    PHP_PROCESS_ENVIRONMENT_TREENODE lookupEnvironmentNode;
    PPHP_PROCESS_ENVIRONMENT_TREENODE lookupEnvironmentNodePtr = &lookupEnvironmentNode;
    PPHP_PROCESS_ENVIRONMENT_TREENODE *environmentNode;

    PhInitializeStringRefLongHint(&lookupEnvironmentNode.NameText->sr, KeyPath);

    environmentNode = (PPHP_PROCESS_ENVIRONMENT_TREENODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupEnvironmentNodePtr
        );

    if (environmentNode)
        return *environmentNode;
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

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
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
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPH_ENVIRONMENT_CONTEXT context = Context;
    PPHP_PROCESS_ENVIRONMENT_TREENODE node;

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

            //if (node->HasChildren)
            //{
            //    NOTHING;
            //}
            //else
            {
                if (context->HighlightCmdEnvironment && node->IsCmdVariable)
                    getNodeColor->BackColor = PhCsColorDebuggedProcesses;
                else if (context->HighlightProcessEnvironment && node->Type & PROCESS_ENVIRONMENT_TREENODE_TYPE_PROCESS)
                    getNodeColor->BackColor = PhCsColorServiceProcesses;
                else if (context->HighlightUserEnvironment && node->Type & PROCESS_ENVIRONMENT_TREENODE_TYPE_USER)
                    getNodeColor->BackColor = PhCsColorOwnProcesses;
                else if (context->HighlightSystemEnvironment && node->Type & PROCESS_ENVIRONMENT_TREENODE_TYPE_SYSTEM)
                    getNodeColor->BackColor = PhCsColorSystemProcesses;
            }

            getNodeColor->Flags = TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &context->TreeNewSortColumn, &context->TreeNewSortOrder);

            // HACK
            if (context->TreeFilterSupport.FilterList)
                PhApplyTreeNewFilters(&context->TreeFilterSupport);

            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            if (!contextMenuEvent)
                break;

            PhpShowEnvironmentNodeContextMenu(context, contextMenuEvent);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(context->WindowHandle, WM_COMMAND, ID_ENV_COPY, 0);
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(context->TreeNewHandle, 0, -1);
                break;
            case VK_DELETE:
                SendMessage(context->WindowHandle, WM_COMMAND, ID_ENV_DELETE, 0);
                break;
            case VK_RETURN:
                SendMessage(context->WindowHandle, WM_COMMAND, ID_ENV_EDIT, 0);
                break;
            }
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = NoSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

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
            SendMessage(context->WindowHandle, WM_COMMAND, ID_SHOWCONTEXTMENU, 0); // HACK
        }
        return TRUE;
    case TreeNewGetDialogCode:
        {
            PULONG code = Parameter2;

            if (PtrToUlong(Parameter1) == VK_RETURN)
            {
                *code = DLGC_WANTMESSAGE;
                return TRUE;
            }
        }
        return FALSE;
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
    //TreeNew_NodesStructured(Context->TreeNewHandle);
}

PPHP_PROCESS_ENVIRONMENT_TREENODE PhpGetSelectedEnvironmentNode(
    _In_ PPH_ENVIRONMENT_CONTEXT Context
    )
{
    PPHP_PROCESS_ENVIRONMENT_TREENODE environmentNode = NULL;
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        environmentNode = Context->NodeList->Items[i];

        if (environmentNode->Node.Selected)
            return environmentNode;
    }

    return NULL;
}

_Success_(return)
BOOLEAN PhpGetSelectedEnvironmentNodes(
    _In_ PPH_ENVIRONMENT_CONTEXT Context,
    _Out_ PPHP_PROCESS_ENVIRONMENT_TREENODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPHP_PROCESS_ENVIRONMENT_TREENODE node = (PPHP_PROCESS_ENVIRONMENT_TREENODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node);
        }
    }

    if (list->Count)
    {
        *Nodes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
        *NumberOfNodes = list->Count;

        PhDereferenceObject(list);
        return TRUE;
    }

    PhDereferenceObject(list);
    return FALSE;
}

BOOLEAN PhpHasSelectedEnvironmentGroupNode(
    _In_ PPHP_PROCESS_ENVIRONMENT_TREENODE* Nodes,
    _In_ ULONG NumberOfNodes
    )
{
    for (ULONG i = 0; i < NumberOfNodes; i++)
    {
        if (Nodes[i]->Type & PROCESS_ENVIRONMENT_TREENODE_TYPE_GROUP)
            return TRUE;
    }

    return FALSE;
}

VOID PhpInitializeEnvironmentTree(
    _Inout_ PPH_ENVIRONMENT_CONTEXT Context
    )
{
    Context->NodeList = PhCreateList(100);
    Context->NodeRootList = PhCreateList(30);
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPHP_PROCESS_ENVIRONMENT_TREENODE),
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
    PhDeleteTreeNewFilterSupport(&Context->TreeFilterSupport);

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

    if (!context)
        return FALSE;
    if (!environmentNode->Parent && environmentNode->Children && environmentNode->Children->Count == 0)
        return FALSE;
    if (context->TreeNewSortOrder != NoSortOrder && environmentNode->HasChildren)
        return FALSE;

    if (context->HideProcessEnvironment && environmentNode->Type & PROCESS_ENVIRONMENT_TREENODE_TYPE_PROCESS)
        return FALSE;
    if (context->HideUserEnvironment && environmentNode->Type & PROCESS_ENVIRONMENT_TREENODE_TYPE_USER)
        return FALSE;
    if (context->HideSystemEnvironment && environmentNode->Type & PROCESS_ENVIRONMENT_TREENODE_TYPE_SYSTEM)
        return FALSE;
    if (context->HideCmdTypeEnvironment && environmentNode->IsCmdVariable)
        return FALSE;

    if (PhIsNullOrEmptyString(context->SearchboxText))
        return TRUE;

    if (!PhIsNullOrEmptyString(environmentNode->NameText))
    {
        if (PhWordMatchStringRef(&context->SearchboxText->sr, &environmentNode->NameText->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(environmentNode->ValueText))
    {
        if (PhWordMatchStringRef(&context->SearchboxText->sr, &environmentNode->ValueText->sr))
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

            PhpRefreshEnvironmentList(context, processItem);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveTreeNewFilter(&context->TreeFilterSupport, context->TreeFilterEntry);
            if (context->SearchboxText) PhDereferenceObject(context->SearchboxText);

            PhSaveSettingsEnvironmentList(context);
            PhpDeleteEnvironmentTree(context);

            PhpClearEnvironmentItems(context);
            PhDeleteArray(&context->Items);
            PhClearReference(&context->StatusMessage);

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
                    PPH_EMENU_ITEM cmdMenuItem;
                    PPH_EMENU_ITEM highlightProcessMenuItem;
                    PPH_EMENU_ITEM highlightUserMenuItem;
                    PPH_EMENU_ITEM highlightSystemMenuItem;
                    PPH_EMENU_ITEM highlightCmdMenuItem;
                    PPH_EMENU_ITEM newProcessMenuItem;
                    PPH_EMENU_ITEM selectedItem;

                    GetWindowRect(GetDlgItem(hwndDlg, IDC_OPTIONS), &rect);

                    processMenuItem = PhCreateEMenuItem(0, ENVIRONMENT_TREE_MENU_ITEM_HIDE_PROCESS_TYPE, L"Hide process", NULL, NULL);
                    userMenuItem = PhCreateEMenuItem(0, ENVIRONMENT_TREE_MENU_ITEM_HIDE_USER_TYPE, L"Hide user", NULL, NULL);
                    systemMenuItem = PhCreateEMenuItem(0, ENVIRONMENT_TREE_MENU_ITEM_HIDE_SYSTEM_TYPE, L"Hide system", NULL, NULL);
                    cmdMenuItem = PhCreateEMenuItem(0, ENVIRONMENT_TREE_MENU_ITEM_HIDE_CMD_TYPE, L"Hide cmd", NULL, NULL);
                    highlightProcessMenuItem = PhCreateEMenuItem(0, ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_PROCESS_TYPE, L"Highlight process", NULL, NULL);
                    highlightUserMenuItem = PhCreateEMenuItem(0, ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_USER_TYPE, L"Highlight user", NULL, NULL);
                    highlightSystemMenuItem = PhCreateEMenuItem(0, ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_SYSTEM_TYPE, L"Highlight system", NULL, NULL);
                    highlightCmdMenuItem = PhCreateEMenuItem(0, ENVIRONMENT_TREE_MENU_ITEM_HIGHLIGHT_CMD_TYPE, L"Highlight cmd", NULL, NULL);
                    newProcessMenuItem = PhCreateEMenuItem(0, ENVIRONMENT_TREE_MENU_ITEM_NEW_ENVIRONMENT_VARIABLE, L"New variable...", NULL, NULL);

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, processMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, userMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, systemMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, cmdMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightProcessMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightUserMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightSystemMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightCmdMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, newProcessMenuItem, ULONG_MAX);

                    if (context->HideProcessEnvironment)
                        processMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HideUserEnvironment)
                        userMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HideSystemEnvironment)
                        systemMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HideCmdTypeEnvironment)
                        cmdMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightProcessEnvironment)
                        highlightProcessMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightUserEnvironment)
                        highlightUserMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightSystemEnvironment)
                        highlightSystemMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightCmdEnvironment)
                        highlightCmdMenuItem->Flags |= PH_EMENU_CHECKED;

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

                            if (PhpShowEditEnvDialog(hwndDlg, processItem, L"", NULL, &refresh) == IDOK && refresh)
                            {
                                PhpRefreshEnvironmentList(context, processItem);
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
                    PhpRefreshEnvironmentList(context, processItem);
                }
                break;
            case ID_SHOWCONTEXTMENU:
                {
                    PPHP_PROCESS_ENVIRONMENT_TREENODE item = PhpGetSelectedEnvironmentNode(context);
                    BOOLEAN refresh;

                    if (!item || item->Type & PROCESS_ENVIRONMENT_TREENODE_TYPE_GROUP)
                        break;

                    if (PhpShowEditEnvDialog(
                        hwndDlg,
                        context->ProcessItem,
                        item->NameText->Buffer,
                        item->ValueText->Buffer,
                        &refresh
                        ) == IDOK && refresh)
                    {
                        PhpRefreshEnvironmentList(context, context->ProcessItem);
                    }
                }
                break;
            case ID_ENV_EDIT:
                {
                    PPHP_PROCESS_ENVIRONMENT_TREENODE item = PhpGetSelectedEnvironmentNode(context);
                    BOOLEAN refresh;

                    if (!item || item->Type & PROCESS_ENVIRONMENT_TREENODE_TYPE_GROUP)
                        break;

                    if (PhpShowEditEnvDialog(
                        context->WindowHandle,
                        context->ProcessItem,
                        item->NameText->Buffer,
                        item->ValueText->Buffer,
                        &refresh
                        ) == IDOK && refresh)
                    {
                        PhpRefreshEnvironmentList(context, context->ProcessItem);
                    }
                }
                break;
            case ID_ENV_DELETE:
                {
                    PPHP_PROCESS_ENVIRONMENT_TREENODE item = PhpGetSelectedEnvironmentNode(context);
                    NTSTATUS status = STATUS_UNSUCCESSFUL;

                    if (!item || item->Type & PROCESS_ENVIRONMENT_TREENODE_TYPE_GROUP)
                        break;

                    if (PhGetIntegerSetting(L"EnableWarnings") && !PhShowConfirmMessage(
                        context->WindowHandle,
                        L"delete",
                        L"the selected environment variable",
                        L"Some programs may restrict access or ban your account when editing the environment variable(s) of the process.",
                        FALSE
                        ))
                    {
                        break;
                    }

                    status = PhpEditDeleteEnvironment(
                        context,
                        context->ProcessItem,
                        item->NameText
                        );

                    PhpRefreshEnvironmentList(context, context->ProcessItem);

                    if (status == STATUS_TIMEOUT)
                    {
                        PhShowStatus(hwndDlg, L"Unable to delete the environment variable.", 0, WAIT_TIMEOUT);
                    }
                    else if (!NT_SUCCESS(status))
                    {
                        PhShowStatus(hwndDlg, L"Unable to delete the environment variable.", status, 0);
                    }
                }
                break;
            case ID_ENV_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                    PhSetClipboardString(context->TreeNewHandle, &text->sr);
                    PhDereferenceObject(text);
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
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)context->TreeNewHandle);
                return TRUE;
            }
        }
        break;
    case WM_KEYDOWN:
        {
            if (LOWORD(wParam) == 'K')
            {
                if (GetKeyState(VK_CONTROL) < 0)
                {
                    SetFocus(context->SearchWindowHandle);
                    return TRUE;
                }
            }
        }
        break;
    }

    return FALSE;
}
