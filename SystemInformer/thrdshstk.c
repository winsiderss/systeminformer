/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc. All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 */

#include <phapp.h>
#include <phsettings.h>
#include <symprv.h>

typedef struct _PH_THREAD_SHADOW_STACK_DIALOG_CONTEXT
{
    HANDLE ProcessId;
    HANDLE ThreadId;
    PPH_SYMBOL_PROVIDER SymbolProvider;
    ULONG64 ShadowStackPointer;
    ULONG64 CetUMsr;
    PULONG64 Entries;
    ULONG Count;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
} PH_THREAD_SHADOW_STACK_DIALOG_CONTEXT, *PPH_THREAD_SHADOW_STACK_DIALOG_CONTEXT;

INT_PTR CALLBACK PhpThreadShadowStackDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_THREAD_SHADOW_STACK_DIALOG_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_THREAD_SHADOW_STACK_DIALOG_CONTEXT)lParam;
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
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetWindowText(hwndDlg, PhaFormatString(
                L"Shadow stack - thread %lu - SSP 0x%016I64x (%s%s)",
                HandleToUlong(context->ThreadId),
                context->ShadowStackPointer,
                FlagOn(context->CetUMsr, PH_CET_U_SH_STK_EN) ? L"enabled" : L"disabled",
                FlagOn(context->CetUMsr, PH_CET_U_WR_SHSTK_EN) ? L", write-enabled" : L""
                )->Buffer);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_RIGHT, 45, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 135, L"Address");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 135, L"Value");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 280, L"Symbol");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Type");
            PhSetExtendedListView(context->ListViewHandle);

            PhLoadSymbolProviderModules(context->SymbolProvider, context->ProcessId);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            for (ULONG i = 0; i < context->Count; i++)
            {
                ULONG64 value = context->Entries[i];
                ULONG64 rangeEnd = context->ShadowStackPointer + (ULONG64)context->Count * sizeof(ULONG64);
                BOOLEAN restoreToken = (value & 1) &&
                    (value & ~1ULL) >= context->ShadowStackPointer &&
                    (value & ~1ULL) < rangeEnd;
                PPH_STRING symbol = NULL;
                LONG index;

                index = PhAddListViewItem(context->ListViewHandle, MAXINT, PhaFormatString(L"%lu", i)->Buffer, NULL);
                PhSetListViewSubItem(context->ListViewHandle, index, 1, PhaFormatString(
                    L"0x%016I64x", context->ShadowStackPointer + (ULONG64)i * sizeof(ULONG64))->Buffer);
                PhSetListViewSubItem(context->ListViewHandle, index, 2, PhaFormatString(L"0x%016I64x", value)->Buffer);

                if (!restoreToken && value)
                    symbol = PhGetSymbolFromAddress(context->SymbolProvider, (PVOID)(ULONG_PTR)value, NULL, NULL, NULL, NULL);

                if (symbol)
                {
                    PhSetListViewSubItem(context->ListViewHandle, index, 3, symbol->Buffer);
                    PhDereferenceObject(symbol);
                }

                PhSetListViewSubItem(context->ListViewHandle, index, 4,
                    restoreToken ? L"Restore token" : (value ? L"Return address" : L"Unknown"));
            }

            PhSetDialogFocus(hwndDlg, context->ListViewHandle);
            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        PhDeleteLayoutManager(&context->LayoutManager);
        PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDCANCEL:
        case IDOK:
            EndDialog(hwndDlg, IDOK);
            break;
        }
        break;
    case WM_NOTIFY:
        PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
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

VOID PhShowThreadShadowStackDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG64 ShadowStackPointer,
    _In_ ULONG64 CetUMsr,
    _In_reads_opt_(Count) PULONG64 Entries,
    _In_ ULONG Count
    )
{
    PH_THREAD_SHADOW_STACK_DIALOG_CONTEXT context;

    memset(&context, 0, sizeof(context));
    context.ProcessId = ProcessId;
    context.ThreadId = ThreadId;
    context.SymbolProvider = SymbolProvider;
    context.ShadowStackPointer = ShadowStackPointer;
    context.CetUMsr = CetUMsr;
    context.Entries = Entries;
    context.Count = Count;

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_THRDSHADOWSTACK),
        ParentWindowHandle,
        PhpThreadShadowStackDlgProc,
        &context
        );
}
