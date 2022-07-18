#ifndef PH_PROCPRP_H
#define PH_PROCPRP_H

typedef struct _PH_PROCESS_WAITPROPCONTEXT
{
    SLIST_ENTRY ListEntry;
    HWND PropSheetWindowHandle;
    HANDLE ProcessWaitHandle;
    HANDLE ProcessHandle;
    PPH_PROCESS_ITEM ProcessItem;
} PH_PROCESS_WAITPROPCONTEXT, *PPH_PROCESS_WAITPROPCONTEXT;

#define PH_PROCESS_PROPCONTEXT_MAXPAGES 20

typedef struct _PH_PROCESS_PROPCONTEXT
{
    PPH_PROCESS_ITEM ProcessItem;
    PPH_STRING Title;
    PROPSHEETHEADER PropSheetHeader;
    HPROPSHEETPAGE *PropSheetPages;

    HANDLE SelectThreadId;

    PPH_PROCESS_WAITPROPCONTEXT ProcessWaitContext;
    BOOLEAN WaitInitialized;
} PH_PROCESS_PROPCONTEXT, *PPH_PROCESS_PROPCONTEXT;

// begin_phapppub
typedef struct _PH_PROCESS_PROPPAGECONTEXT
{
    PPH_PROCESS_PROPCONTEXT PropContext;
    PVOID Context;
    PROPSHEETPAGE PropSheetPage;

    BOOLEAN LayoutInitialized;
} PH_PROCESS_PROPPAGECONTEXT, *PPH_PROCESS_PROPPAGECONTEXT;
// end_phapppub

// begin_phapppub
PHAPPAPI
PPH_PROCESS_PROPCONTEXT
NTAPI
PhCreateProcessPropContext(
    _In_opt_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    );
// end_phapppub

VOID PhRefreshProcessPropContext(
    _Inout_ PPH_PROCESS_PROPCONTEXT PropContext
    );

// begin_phapppub
PHAPPAPI
VOID
NTAPI
PhSetSelectThreadIdProcessPropContext(
    _Inout_ PPH_PROCESS_PROPCONTEXT PropContext,
    _In_ HANDLE ThreadId
    );

PHAPPAPI
BOOLEAN
NTAPI
PhAddProcessPropPage(
    _Inout_ PPH_PROCESS_PROPCONTEXT PropContext,
    _In_ _Assume_refs_(1) PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    );

PHAPPAPI
BOOLEAN
NTAPI
PhAddProcessPropPage2(
    _Inout_ PPH_PROCESS_PROPCONTEXT PropContext,
    _In_ HPROPSHEETPAGE PropSheetPageHandle
    );

PHAPPAPI
PPH_PROCESS_PROPPAGECONTEXT
NTAPI
PhCreateProcessPropPageContext(
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    );

PHAPPAPI
PPH_PROCESS_PROPPAGECONTEXT
NTAPI
PhCreateProcessPropPageContextEx(
    _In_opt_ PVOID InstanceHandle,
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    );

_Success_(return)
PHAPPAPI
BOOLEAN
NTAPI
PhPropPageDlgProcHeader(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam,
    _Out_opt_ LPPROPSHEETPAGE *PropSheetPage,
    _Out_opt_ PPH_PROCESS_PROPPAGECONTEXT *PropPageContext,
    _Out_opt_ PPH_PROCESS_ITEM *ProcessItem
    );

#define PH_PROP_PAGE_TAB_CONTROL_PARENT ((PPH_LAYOUT_ITEM)0x1)

PHAPPAPI
PPH_LAYOUT_ITEM
NTAPI
PhAddPropPageLayoutItem(
    _In_ HWND hwnd,
    _In_ HWND Handle,
    _In_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor
    );

PHAPPAPI
VOID
NTAPI
PhDoPropPageLayout(
    _In_ HWND hwnd
    );

FORCEINLINE
PPH_LAYOUT_ITEM
PhBeginPropPageLayout(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    )
{
    if (!PropPageContext->LayoutInitialized)
    {
        return PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
            PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
    }
    else
    {
        return NULL;
    }
}

FORCEINLINE
VOID
PhEndPropPageLayout(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    )
{
    PhDoPropPageLayout(hwndDlg);
    PropPageContext->LayoutInitialized = TRUE;
}

PHAPPAPI
VOID
NTAPI
PhShowProcessProperties(
    _In_ PPH_PROCESS_PROPCONTEXT Context
    );
// end_phapppub

#endif
