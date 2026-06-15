/**
 * \file envdlg.c
 * \brief Environment variables dialog for the ExtendedTools plugin.
 */

/*
 * Copyright (c) 2024 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2024
 *
 */

#include "exttools.h"
#include <phsettings.h>
#include <emenu.h>
#include <cpysave.h>

#define IDD_EDITENV PHAPP_IDD_EDITENV
#define IDC_NAME PHAPP_IDC_NAME
#define IDC_VALUE PHAPP_IDC_VALUE

#define ENV_GROUP_USER 0
#define ENV_GROUP_SYSTEM 1

CONST PH_STRINGREF EtUserEnvironmentKeyName = PH_STRINGREF_INIT(L"Environment");
CONST PH_STRINGREF EtSystemEnvironmentKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Control\\Session Manager\\Environment");
HWND EtEnvironmentVariablesWindowHandle = NULL;

/**
 * Represents an environment variable entry.
 */
typedef struct _ENV_VARIABLE_ENTRY
{
    PPH_STRING Name;   /**< The name of the environment variable. */
    PPH_STRING Value;  /**< The value of the environment variable. */
    BOOLEAN System;    /**< TRUE if the variable is a system-wide variable. */
    BOOLEAN Expand;    /**< TRUE if the variable contains expandable strings (REG_EXPAND_SZ). */
} ENV_VARIABLE_ENTRY, *PENV_VARIABLE_ENTRY;

/**
 * Context for the environment variables dialog.
 */
typedef struct _ENV_VARIABLES_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPH_LIST Entries;
    BOOLEAN Elevated;
} ENV_VARIABLES_CONTEXT, *PENV_VARIABLES_CONTEXT;

/**
 * Context for the environment variable edit dialog.
 */
typedef struct _ENV_EDIT_CONTEXT
{
    PCWSTR Name;
    PCWSTR Value;
    BOOLEAN NameReadOnly;
    BOOLEAN Committed;
    PPH_STRING OutName;
    PPH_STRING OutValue;
    PH_LAYOUT_MANAGER LayoutManager;
    RECT MinimumSize;
} ENV_EDIT_CONTEXT, *PENV_EDIT_CONTEXT;

/**
 * Context for the environment variable split (path) dialog.
 */
typedef struct _ENV_SPLIT_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PPH_STRING Name;
    PPH_STRING Value;
    BOOLEAN Committed;
    PH_LAYOUT_MANAGER LayoutManager;
    RECT MinimumSize;
} ENV_SPLIT_CONTEXT, *PENV_SPLIT_CONTEXT;

INT_PTR CALLBACK EtEnvironmentVariablesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

/**
 * Shows the environment variables dialog.
 *
 * \param ParentWindowHandle The handle to the parent window.
 */
VOID EtShowEnvironmentVariablesDialog(
    _In_ HWND ParentWindowHandle
    )
{
    if (!EtEnvironmentVariablesWindowHandle)
    {
        EtEnvironmentVariablesWindowHandle = PhCreateDialog(
            NtCurrentImageBase(),
            MAKEINTRESOURCE(IDD_ENVIRONMENTVARIABLES),
            PhCsForceNoParent ? NULL : ParentWindowHandle,
            EtEnvironmentVariablesDlgProc,
            NULL
            );
        PhRegisterDialog(EtEnvironmentVariablesWindowHandle);
        ShowWindow(EtEnvironmentVariablesWindowHandle, SW_SHOW);
    }

    if (IsMinimized(EtEnvironmentVariablesWindowHandle))
        ShowWindow(EtEnvironmentVariablesWindowHandle, SW_RESTORE);
    else
        SetForegroundWindow(EtEnvironmentVariablesWindowHandle);
}

// Registry helpers

/**
 * Context for environment variable enumeration.
 */
typedef struct _ENV_ENUM_CONTEXT
{
    PPH_LIST Entries;
    BOOLEAN System;
} ENV_ENUM_CONTEXT, *PENV_ENUM_CONTEXT;

/**
 * Callback function for enumerating environment variables in the registry.
 *
 * \param RootDirectory The handle to the registry key being enumerated.
 * \param Information The full information about the key value.
 * \param Context The context for enumeration.
 * \return TRUE to continue enumeration, FALSE to stop.
 */
_Function_class_(PH_ENUM_KEY_CALLBACK)
BOOLEAN NTAPI EtEnumEnvironmentKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_VALUE_FULL_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PENV_ENUM_CONTEXT context = Context;

    if (Information->Type == REG_SZ || Information->Type == REG_EXPAND_SZ)
    {
        PENV_VARIABLE_ENTRY entry;
        SIZE_T dataLength;

        entry = PhAllocateZero(sizeof(ENV_VARIABLE_ENTRY));
        entry->Name = PhCreateStringEx(Information->Name, Information->NameLength);

        // Trim a trailing null terminator from the value if present.
        dataLength = Information->DataLength;
        if (dataLength >= sizeof(WCHAR) &&
            *(PWCHAR)PTR_ADD_OFFSET(Information, Information->DataOffset + dataLength - sizeof(WCHAR)) == UNICODE_NULL)
        {
            dataLength -= sizeof(WCHAR);
        }

        entry->Value = PhCreateStringEx(PTR_ADD_OFFSET(Information, Information->DataOffset), dataLength);
        entry->System = context->System;
        entry->Expand = Information->Type == REG_EXPAND_SZ;

        PhAddItemList(context->Entries, entry);
    }

    return TRUE;
}

/**
 * Reads environment variables from a registry key.
 *
 * \param RootDirectory The handle to the root registry key.
 * \param KeyName The name of the environment key.
 * \param System TRUE if the variables are system-wide.
 * \param Entries The list to receive the environment variable entries.
 */
VOID EtReadEnvironmentKey(
    _In_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF KeyName,
    _In_ BOOLEAN System,
    _In_ PPH_LIST Entries
    )
{
    HANDLE keyHandle;
    ENV_ENUM_CONTEXT enumContext;

    enumContext.Entries = Entries;
    enumContext.System = System;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        RootDirectory,
        KeyName,
        0
        )))
    {
        PhEnumerateValueKey(
            keyHandle,
            KeyValueFullInformation,
            EtEnumEnvironmentKeyCallback,
            &enumContext
            );

        NtClose(keyHandle);
    }
}

/**
 * Writes an environment variable to the registry.
 *
 * \param System TRUE if the variable is system-wide.
 * \param Name The name of the variable.
 * \param Value The value of the variable.
 * \param Expand TRUE if the variable should be written as REG_EXPAND_SZ.
 * \return STATUS_SUCCESS on success, or an appropriate NTSTATUS error code.
 */
NTSTATUS EtWriteEnvironmentVariable(
    _In_ BOOLEAN System,
    _In_ PPH_STRING Name,
    _In_ PPH_STRING Value,
    _In_ BOOLEAN Expand
    )
{
    NTSTATUS status;
    HANDLE keyHandle;

    status = PhCreateKey(
        &keyHandle,
        KEY_WRITE,
        System ? PH_KEY_LOCAL_MACHINE : PH_KEY_CURRENT_USER,
        System ? &EtSystemEnvironmentKeyName : &EtUserEnvironmentKeyName,
        0,
        0,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        status = PhSetValueKey(
            keyHandle,
            &Name->sr,
            Expand ? REG_EXPAND_SZ : REG_SZ,
            Value->Buffer,
            (ULONG)Value->Length + sizeof(UNICODE_NULL)
            );

        NtClose(keyHandle);
    }

    return status;
}

/**
 * Deletes an environment variable from the registry.
 *
 * \param System TRUE if the variable is system-wide.
 * \param Name The name of the variable.
 * \return STATUS_SUCCESS on success, or an appropriate NTSTATUS error code.
 */
NTSTATUS EtDeleteEnvironmentVariable(
    _In_ BOOLEAN System,
    _In_ PPH_STRING Name
    )
{
    NTSTATUS status;
    HANDLE keyHandle;

    status = PhOpenKey(
        &keyHandle,
        KEY_WRITE,
        System ? PH_KEY_LOCAL_MACHINE : PH_KEY_CURRENT_USER,
        System ? &EtSystemEnvironmentKeyName : &EtUserEnvironmentKeyName,
        0
        );

    if (NT_SUCCESS(status))
    {
        status = PhDeleteValueKey(keyHandle, &Name->sr);
        NtClose(keyHandle);
    }

    return status;
}

/**
 * Broadcasts a message to all windows that environment variables have changed.
 */
VOID EtBroadcastEnvironmentChange(
    VOID
    )
{
    SendMessageTimeout(
        HWND_BROADCAST,
        WM_SETTINGCHANGE,
        0,
        (LPARAM)L"Environment",
        SMTO_ABORTIFHUNG,
        5000,
        NULL
        );
}


/**
 * Determines if an environment variable entry represents a path-like variable.
 *
 * \param Entry The environment variable entry.
 * \return TRUE if the variable is path-like, FALSE otherwise.
 */
BOOLEAN EtIsPathEnvironmentVariable(
    _In_ PENV_VARIABLE_ENTRY Entry
    )
{
    if (
        PhEqualString2(Entry->Name, L"Path", FALSE) ||
        PhEqualString2(Entry->Name, L"PATHEXT", FALSE) ||
        PhEqualString2(Entry->Name, L"LIBPATH", FALSE)
        )
    {
        return TRUE;
    }

    if (Entry->Value && PhFindCharInString(Entry->Value, 0, L';') != SIZE_MAX)
        return TRUE;

    return FALSE;
}

/**
 * Clears all environment variable entries from the context.
 *
 * \param Context The environment variables context.
 */
VOID EtClearEnvironmentEntries(
    _In_ PENV_VARIABLES_CONTEXT Context
    )
{
    ULONG i;

    for (i = 0; i < Context->Entries->Count; i++)
    {
        PENV_VARIABLE_ENTRY entry = Context->Entries->Items[i];

        if (entry->Name)
            PhDereferenceObject(entry->Name);
        if (entry->Value)
            PhDereferenceObject(entry->Value);

        PhFree(entry);
    }

    PhClearList(Context->Entries);
}

/**
 * Refreshes the list of environment variables in the dialog.
 *
 * \param Context The environment variables context.
 */
VOID EtRefreshEnvironmentVariables(
    _In_ PENV_VARIABLES_CONTEXT Context
    )
{
    ULONG i;

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);
    EtClearEnvironmentEntries(Context);

    EtReadEnvironmentKey(PH_KEY_CURRENT_USER, &EtUserEnvironmentKeyName, FALSE, Context->Entries);
    EtReadEnvironmentKey(PH_KEY_LOCAL_MACHINE, &EtSystemEnvironmentKeyName, TRUE, Context->Entries);

    for (i = 0; i < Context->Entries->Count; i++)
    {
        PENV_VARIABLE_ENTRY entry = Context->Entries->Items[i];
        INT index;

        index = PhAddListViewGroupItem(
            Context->ListViewHandle,
            entry->System ? ENV_GROUP_SYSTEM : ENV_GROUP_USER,
            MAXINT,
            PhGetString(entry->Name),
            entry
            );
        PhSetListViewSubItem(Context->ListViewHandle, index, 1, PhGetString(entry->Value));
    }

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

/**
 * Retrieves the currently selected environment variable entry in the list view.
 *
 * \param Context The environment variables context.
 * \return The selected environment variable entry, or NULL if no entry is selected.
 */
PENV_VARIABLE_ENTRY EtGetSelectedEnvironmentEntry(
    _In_ PENV_VARIABLES_CONTEXT Context
    )
{
    INT index;
    PVOID param;

    index = PhFindListViewItemByFlags(Context->ListViewHandle, INT_ERROR, LVNI_SELECTED);

    if (index == INT_ERROR)
        return NULL;

    if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
        return param;

    return NULL;
}

// Edit dialog (reuses IDD_EDITENV)

/**
 * Subclass procedure for the environment variable value edit control.
 *
 * \param WindowHandle The handle to the edit control.
 * \param WindowMessage The window message.
 * \param wParam The message-specific parameter.
 * \param lParam The message-specific parameter.
 * \return The message result.
 */
ULONG_PTR CALLBACK EtEnvEditSubclassProc(
    _In_ HWND WindowHandle,
    _In_ ULONG WindowMessage,
    _In_ ULONG_PTR wParam,
    _In_ ULONG_PTR lParam
    )
{
    WNDPROC oldWndProc;

    if (!(oldWndProc = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT)))
        return FALSE;

    switch (WindowMessage)
    {
    case WM_DESTROY:
        {
            PhSetWindowProcedure(WindowHandle, oldWndProc);
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_GETDLGCODE:
        {
            if (wParam != VK_ESCAPE)
            {
                if (wParam == VK_RETURN)
                    return DLGC_WANTMESSAGE;

                return DLGC_WANTALLKEYS;
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
}

/**
 * Dialog procedure for the environment variable edit dialog.
 *
 * \param hwndDlg The handle to the dialog.
 * \param uMsg The window message.
 * \param wParam The message-specific parameter.
 * \param lParam The message-specific parameter.
 * \return The message result.
 */
INT_PTR CALLBACK EtEnvEditDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PENV_EDIT_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PENV_EDIT_CONTEXT)lParam;
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
            HWND valueHandle;

            valueHandle = GetDlgItem(hwndDlg, IDC_VALUE);

            PhSetApplicationWindowIcon(hwndDlg);
            PhCenterWindow(hwndDlg, GEtarent(hwndDlg));

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_NAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, valueHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhLayoutManagerLayout(&context->LayoutManager);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 200;
            context->MinimumSize.bottom = 140;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            PhSetDialogItemText(hwndDlg, IDC_NAME, context->Name ? context->Name : L"");
            PhSetDialogItemText(hwndDlg, IDC_VALUE, context->Value ? context->Value : L"");

            if (context->NameReadOnly)
                Edit_SetReadOnly(GetDlgItem(hwndDlg, IDC_NAME), TRUE);

            PhSetWindowContext(valueHandle, PH_WINDOW_CONTEXT_DEFAULT, PhGetWindowProcedure(valueHandle));
            PhSetWindowProcedure(valueHandle, (WNDPROC)EtEnvEditSubclassProc);

            EnableWindow(GetDlgItem(hwndDlg, IDOK), PhGetWindowTextLength(GetDlgItem(hwndDlg, IDC_NAME)) > 0);

            PhSetDialogFocus(hwndDlg, valueHandle);

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
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    PPH_STRING name;

                    name = PhGetWindowText(GetDlgItem(hwndDlg, IDC_NAME));

                    if (PhIsNullOrEmptyString(name))
                    {
                        PhClearReference(&name);
                        break;
                    }

                    context->OutName = name;
                    context->OutValue = PhGetWindowText(GetDlgItem(hwndDlg, IDC_VALUE));
                    context->Committed = TRUE;
                    EndDialog(hwndDlg, IDOK);
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
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_SIZING:
        PhResizingMinimumSize((PRECT)lParam, wParam, context->MinimumSize.right, context->MinimumSize.bottom);
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

/**
 * Shows the environment variable edit dialog.
 *
 * \param ParentWindowHandle The handle to the parent window.
 * \param InitialName The initial name of the environment variable.
 * \param InitialValue The initial value of the environment variable.
 * \param NameReadOnly TRUE if the name should be read-only.
 * \param Name Receives the new name of the environment variable.
 * \param Value Receives the new value of the environment variable.
 * \return TRUE if the user committed the changes, FALSE otherwise.
 */
BOOLEAN EtShowEnvEditDialog(
    _In_ HWND ParentWindowHandle,
    _In_opt_ PCWSTR InitialName,
    _In_opt_ PCWSTR InitialValue,
    _In_ BOOLEAN NameReadOnly,
    _Out_ PPH_STRING *Name,
    _Out_ PPH_STRING *Value
    )
{
    ENV_EDIT_CONTEXT context;

    memset(&context, 0, sizeof(ENV_EDIT_CONTEXT));
    context.Name = InitialName;
    context.Value = InitialValue;
    context.NameReadOnly = NameReadOnly;

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_EDITENV),
        ParentWindowHandle,
        EtEnvEditDlgProc,
        &context
        );

    if (context.Committed)
    {
        *Name = context.OutName;
        *Value = context.OutValue;
        return TRUE;
    }

    return FALSE;
}

// Path splitter dialog (IDD_ENVEDIT_LIST)

/**
 * Populates the list view in the split dialog from a path-like string.
 *
 * \param Context The environment split context.
 */
VOID EtSplitPopulateList(
    _In_ PENV_SPLIT_CONTEXT Context
    )
{
    PH_STRINGREF remaining;
    PH_STRINGREF part;

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);

    remaining = Context->Value->sr;

    while (remaining.Length != 0)
    {
        if (!PhSplitStringRefAtChar(&remaining, L';', &part, &remaining))
        {
            part = remaining;
            PhInitializeEmptyStringRef(&remaining);
        }

        if (part.Length != 0)
        {
            PPH_STRING text = PhCreateString2(&part);
            PhAddListViewItem(Context->ListViewHandle, MAXINT, text->Buffer, NULL);
            PhDereferenceObject(text);
        }
    }

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

/**
 * Builds a path-like string from the items in the split dialog's list view.
 *
 * \param ListViewHandle The handle to the list view.
 * \return A string representing the combined values, separated by semicolons.
 */
PPH_STRING EtSplitBuildValue(
    _In_ HWND ListViewHandle
    )
{
    PH_STRING_BUILDER stringBuilder;
    INT count;
    INT i;

    PhInitializeStringBuilder(&stringBuilder, 0x100);
    count = ListView_GetItemCount(ListViewHandle);

    for (i = 0; i < count; i++)
    {
        PPH_STRING text;

        text = PhGetListViewItemText(ListViewHandle, i, 0);

        if (!PhIsNullOrEmptyString(text))
        {
            if (stringBuilder.String->Length != 0)
                PhAppendCharStringBuilder(&stringBuilder, L';');

            PhAppendStringBuilder(&stringBuilder, &text->sr);
        }

        PhClearReference(&text);
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

/**
 * Moves an item in the split dialog's list view up or down.
 *
 * \param ListViewHandle The handle to the list view.
 * \param Index The index of the item to move.
 * \param Delta The number of positions to move the item (negative for up, positive for down).
 */
VOID EtSplitMoveItem(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ INT Delta
    )
{
    INT count;
    INT target;
    PPH_STRING text;

    count = ListView_GetItemCount(ListViewHandle);
    target = Index + Delta;

    if (target < 0 || target >= count)
        return;

    text = PhGetListViewItemText(ListViewHandle, Index, 0);

    ListView_DeleteItem(ListViewHandle, Index);
    PhAddListViewItem(ListViewHandle, target, PhGetString(text), NULL);

    ListView_SetItemState(ListViewHandle, target, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(ListViewHandle, target, FALSE);

    PhClearReference(&text);
}

/**
 * Dialog procedure for the environment variable split (path) dialog.
 *
 * \param hwndDlg The handle to the dialog.
 * \param uMsg The window message.
 * \param wParam The message-specific parameter.
 * \param lParam The message-specific parameter.
 * \return The message result.
 */
INT_PTR CALLBACK EtEnvSplitDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PENV_SPLIT_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PENV_SPLIT_CONTEXT)lParam;
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
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetApplicationWindowIcon(hwndDlg);
            SetWindowText(hwndDlg, PhaFormatString(L"Edit %s", PhGetString(context->Name))->Buffer);

            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 300, L"Value");
            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhSetExtendedListView(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_NEW), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_EDIT), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_BROWSE), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_DELETE), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_MOVEUP), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_MOVEDOWN), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_EDITTEXT), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 300;
            context->MinimumSize.bottom = 200;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            EtSplitPopulateList(context);

            PhCenterWindow(hwndDlg, GEtarent(hwndDlg));
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
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    PhMoveReference(&context->Value, EtSplitBuildValue(context->ListViewHandle));
                    context->Committed = TRUE;
                    EndDialog(hwndDlg, IDOK);
                }
                break;
            case IDC_NEW:
                {
                    PPH_STRING text = NULL;

                    if (PhaChoiceDialog(
                        hwndDlg,
                        L"New entry",
                        L"Enter the new value:",
                        NULL,
                        0,
                        NULL,
                        PH_CHOICE_DIALOG_USER_CHOICE,
                        &text,
                        NULL,
                        NULL
                        ))
                    {
                        if (!PhIsNullOrEmptyString(text))
                        {
                            INT index = PhAddListViewItem(context->ListViewHandle, MAXINT, text->Buffer, NULL);
                            ListView_SetItemState(context->ListViewHandle, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                        }
                    }
                }
                break;
            case IDC_EDIT:
                {
                    INT index;
                    PPH_STRING text;

                    index = PhFindListViewItemByFlags(context->ListViewHandle, INT_ERROR, LVNI_SELECTED);

                    if (index == INT_ERROR)
                        break;

                    text = PhGetListViewItemText(context->ListViewHandle, index, 0);

                    if (PhaChoiceDialog(
                        hwndDlg,
                        L"Edit entry",
                        L"Edit the value:",
                        NULL,
                        0,
                        NULL,
                        PH_CHOICE_DIALOG_USER_CHOICE,
                        &text,
                        NULL,
                        NULL
                        ))
                    {
                        if (!PhIsNullOrEmptyString(text))
                            PhSetListViewSubItem(context->ListViewHandle, index, 0, text->Buffer);
                    }

                    PhClearReference(&text);
                }
                break;
            case IDC_BROWSE:
                {
                    PVOID fileDialog;

                    fileDialog = PhCreateOpenFileDialog();
                    PhSetFileDialogOptions(fileDialog, PhGetFileDialogOptions(fileDialog) | PH_FILEDIALOG_PICKFOLDERS);

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        PPH_STRING fileName = PhGetFileDialogFileName(fileDialog);

                        if (!PhIsNullOrEmptyString(fileName))
                        {
                            INT index = PhFindListViewItemByFlags(context->ListViewHandle, INT_ERROR, LVNI_SELECTED);

                            if (index != INT_ERROR)
                            {
                                PhSetListViewSubItem(context->ListViewHandle, index, 0, fileName->Buffer);
                            }
                            else
                            {
                                index = PhAddListViewItem(context->ListViewHandle, MAXINT, fileName->Buffer, NULL);
                                ListView_SetItemState(context->ListViewHandle, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                            }
                        }

                        PhClearReference(&fileName);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            case IDC_DELETE:
                {
                    INT index = PhFindListViewItemByFlags(context->ListViewHandle, INT_ERROR, LVNI_SELECTED);

                    if (index != INT_ERROR)
                        ListView_DeleteItem(context->ListViewHandle, index);
                }
                break;
            case IDC_MOVEUP:
                {
                    INT index = PhFindListViewItemByFlags(context->ListViewHandle, INT_ERROR, LVNI_SELECTED);

                    if (index != INT_ERROR)
                        EtSplitMoveItem(context->ListViewHandle, index, -1);
                }
                break;
            case IDC_MOVEDOWN:
                {
                    INT index = PhFindListViewItemByFlags(context->ListViewHandle, INT_ERROR, LVNI_SELECTED);

                    if (index != INT_ERROR)
                        EtSplitMoveItem(context->ListViewHandle, index, 1);
                }
                break;
            case IDC_EDITTEXT:
                {
                    PPH_STRING value;
                    PPH_STRING name;
                    PPH_STRING newValue;

                    value = EtSplitBuildValue(context->ListViewHandle);

                    if (EtShowEnvEditDialog(
                        hwndDlg,
                        PhGetString(context->Name),
                        PhGetString(value),
                        TRUE,
                        &name,
                        &newValue
                        ))
                    {
                        PhMoveReference(&context->Value, newValue);
                        EtSplitPopulateList(context);
                        PhClearReference(&name);
                    }

                    PhClearReference(&value);
                }
                break;
            }
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_SIZING:
        PhResizingMinimumSize((PRECT)lParam, wParam, context->MinimumSize.right, context->MinimumSize.bottom);
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

/**
 * Shows the environment variable split (path) dialog.
 *
 * \param ParentWindowHandle The handle to the parent window.
 * \param Name The name of the environment variable.
 * \param Value The initial value of the environment variable.
 * \param NewValue Receives the new value of the environment variable.
 * \return TRUE if the user committed the changes, FALSE otherwise.
 */
BOOLEAN EtShowEnvSplitDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_STRING Name,
    _In_ PPH_STRING Value,
    _Out_ PPH_STRING *NewValue
    )
{
    ENV_SPLIT_CONTEXT context;

    memset(&context, 0, sizeof(ENV_SPLIT_CONTEXT));
    context.Name = Name;
    context.Value = PhReferenceObject(Value);

    PhDialogBox(
        NtCurrentImageBase(),
        MAKEINTRESOURCE(IDD_ENVEDIT_LIST),
        ParentWindowHandle,
        EtEnvSplitDlgProc,
        &context
        );

    if (context.Committed)
    {
        *NewValue = context.Value;
        return TRUE;
    }

    PhClearReference(&context.Value);
    return FALSE;
}

// Actions

/**
 * Confirms if the environment variable can be modified (checks elevation for system variables).
 *
 * \param Context The environment variables context.
 * \param System TRUE if the variable is system-wide.
 * \return TRUE if the variable can be modified, FALSE otherwise.
 */
BOOLEAN EtConfirmReadOnly(
    _In_ PENV_VARIABLES_CONTEXT Context,
    _In_ BOOLEAN System
    )
{
    if (System && !Context->Elevated)
    {
        PhShowInformation2(
            Context->WindowHandle,
            L"Unable to modify system environment variables.",
            L"%s",
            L"System environment variables can only be modified when running with administrative privileges."
            );
        return FALSE;
    }

    return TRUE;
}

/**
 * Handles the "Add" action for environment variables.
 *
 * \param Context The environment variables context.
 */
VOID EtEnvironmentAdd(
    _In_ PENV_VARIABLES_CONTEXT Context
    )
{
    NTSTATUS status;
    PENV_VARIABLE_ENTRY selected;
    BOOLEAN system;
    PPH_STRING name;
    PPH_STRING value;

    selected = EtGetSelectedEnvironmentEntry(Context);
    system = (selected && selected->System && Context->Elevated) ? TRUE : FALSE;

    if (!EtShowEnvEditDialog(Context->WindowHandle, NULL, NULL, FALSE, &name, &value))
        return;

    status = EtWriteEnvironmentVariable(system, name, value, PhFindCharInString(value, 0, L'%') != SIZE_MAX);

    if (NT_SUCCESS(status))
    {
        EtBroadcastEnvironmentChange();
        EtRefreshEnvironmentVariables(Context);
    }
    else
    {
        PhShowStatus(Context->WindowHandle, L"Unable to set the environment variable.", status, 0);
    }

    PhClearReference(&name);
    PhClearReference(&value);
}

/**
 * Handles the "Edit" action for environment variables.
 *
 * \param Context The environment variables context.
 */
VOID EtEnvironmentEdit(
    _In_ PENV_VARIABLES_CONTEXT Context
    )
{
    PENV_VARIABLE_ENTRY entry;

    entry = EtGetSelectedEnvironmentEntry(Context);

    if (!entry)
        return;

    if (!EtConfirmReadOnly(Context, entry->System))
        return;

    if (EtIsPathEnvironmentVariable(entry))
    {
        PPH_STRING newValue;

        if (EtShowEnvSplitDialog(Context->WindowHandle, entry->Name, entry->Value, &newValue))
        {
            NTSTATUS status;

            status = EtWriteEnvironmentVariable(entry->System, entry->Name, newValue, entry->Expand ||
                PhFindCharInString(newValue, 0, L'%') != SIZE_MAX);

            if (NT_SUCCESS(status))
            {
                EtBroadcastEnvironmentChange();
                EtRefreshEnvironmentVariables(Context);
            }
            else
            {
                PhShowStatus(Context->WindowHandle, L"Unable to set the environment variable.", status, 0);
            }

            PhClearReference(&newValue);
        }
    }
    else
    {
        PPH_STRING name;
        PPH_STRING value;

        if (EtShowEnvEditDialog(
            Context->WindowHandle,
            PhGetString(entry->Name),
            PhGetString(entry->Value),
            FALSE,
            &name,
            &value
            ))
        {
            NTSTATUS status;

            // Delete the old variable if the name changed.
            if (!PhEqualString(name, entry->Name, FALSE))
                EtDeleteEnvironmentVariable(entry->System, entry->Name);

            status = EtWriteEnvironmentVariable(entry->System, name, value, entry->Expand ||
                PhFindCharInString(value, 0, L'%') != SIZE_MAX);

            if (NT_SUCCESS(status))
            {
                EtBroadcastEnvironmentChange();
                EtRefreshEnvironmentVariables(Context);
            }
            else
            {
                PhShowStatus(Context->WindowHandle, L"Unable to set the environment variable.", status, 0);
            }

            PhClearReference(&name);
            PhClearReference(&value);
        }
    }
}

/**
 * Handles the "Delete" action for environment variables.
 *
 * \param Context The environment variables context.
 */
VOID EtEnvironmentDelete(
    _In_ PENV_VARIABLES_CONTEXT Context
    )
{
    NTSTATUS status;
    PENV_VARIABLE_ENTRY entry;

    entry = EtGetSelectedEnvironmentEntry(Context);

    if (!entry)
        return;

    if (!EtConfirmReadOnly(Context, entry->System))
        return;

    if (!PhShowConfirmMessage(
        Context->WindowHandle,
        L"delete",
        PhaFormatString(L"the environment variable \"%s\"", PhGetString(entry->Name))->Buffer,
        NULL,
        FALSE
        ))
    {
        return;
    }

    status = EtDeleteEnvironmentVariable(entry->System, entry->Name);

    if (NT_SUCCESS(status))
    {
        EtBroadcastEnvironmentChange();
        EtRefreshEnvironmentVariables(Context);
    }
    else
    {
        PhShowStatus(Context->WindowHandle, L"Unable to delete the environment variable.", status, 0);
    }
}

/**
 * Dialog procedure for the main environment variables dialog.
 *
 * \param hwndDlg The handle to the dialog.
 * \param uMsg The window message.
 * \param wParam The message-specific parameter.
 * \param lParam The message-specific parameter.
 * \return The message result.
 */
INT_PTR CALLBACK EtEnvironmentVariablesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PENV_VARIABLES_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(ENV_VARIABLES_CONTEXT));
        context->WindowHandle = hwndDlg;
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
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            context->Entries = PhCreateList(64);
            context->Elevated = !!PhGetOwnTokenAttributes().Elevated;

            PhSetApplicationWindowIcon(hwndDlg);

            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 160, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 320, L"Value");
            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhSetExtendedListView(context->ListViewHandle);

            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhAddListViewGroup(context->ListViewHandle, ENV_GROUP_USER, L"User");
            PhAddListViewGroup(context->ListViewHandle, ENV_GROUP_SYSTEM, L"System");

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ADD), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_EDIT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_DELETE), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhLoadListViewColumnsFromSetting(SETTING_ENVIRONMENT_VARIABLES_LIST_VIEW_COLUMNS, context->ListViewHandle);

            EtRefreshEnvironmentVariables(context);

            if (PhValidWindowPlacementFromSetting(SETTING_ENVIRONMENT_VARIABLES_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_ENVIRONMENT_VARIABLES_WINDOW_POSITION, SETTING_ENVIRONMENT_VARIABLES_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, GEtarent(hwndDlg));

            PhSetDialogFocus(hwndDlg, context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(SETTING_ENVIRONMENT_VARIABLES_LIST_VIEW_COLUMNS, context->ListViewHandle);
            PhSaveWindowPlacementToSetting(SETTING_ENVIRONMENT_VARIABLES_WINDOW_POSITION, SETTING_ENVIRONMENT_VARIABLES_WINDOW_SIZE, hwndDlg);

            PhUnregisterDialog(EtEnvironmentVariablesWindowHandle);
            EtEnvironmentVariablesWindowHandle = NULL;

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            EtClearEnvironmentEntries(context);
            PhDereferenceObject(context->Entries);
            PhDeleteLayoutManager(&context->LayoutManager);
            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_ADD:
                EtEnvironmentAdd(context);
                break;
            case IDC_EDIT:
                EtEnvironmentEdit(context);
                break;
            case IDC_DELETE:
                EtEnvironmentDelete(context);
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->hwndFrom == context->ListViewHandle && header->code == NM_DBLCLK)
            {
                EtEnvironmentEdit(context);
            }
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
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
