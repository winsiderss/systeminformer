#ifndef PH_PROCPRP_H
#define PH_PROCPRP_H

#define PH_PROCESS_PROPCONTEXT_MAXPAGES 20

typedef struct _PV_PROPSHEETCONTEXT
{
    WNDPROC OldWndProc;
    PH_LAYOUT_MANAGER LayoutManager;
    PPH_LAYOUT_ITEM TabPageItem;
    BOOLEAN LayoutInitialized;
} PV_PROPSHEETCONTEXT, *PPV_PROPSHEETCONTEXT;

typedef struct _PPV_PROPCONTEXT
{
    HWND WindowHandle;
    PH_EVENT CreatedEvent;
    PPH_STRING Title;
    PROPSHEETHEADER PropSheetHeader;
    HPROPSHEETPAGE *PropSheetPages;
} PV_PROPCONTEXT, *PPV_PROPCONTEXT;

typedef struct _PH_PROCESS_PROPPAGECONTEXT
{
    PPV_PROPCONTEXT PropContext;
    PVOID Context;
    PROPSHEETPAGE PropSheetPage;

    BOOLEAN LayoutInitialized;
} PH_PROCESS_PROPPAGECONTEXT, *PPH_PROCESS_PROPPAGECONTEXT;

BOOLEAN PvPropInitialization(
    VOID
    );

PPV_PROPCONTEXT
NTAPI
PvCreateProcessPropContext(
    _In_ PPH_STRING Caption
    );

VOID 
PvRefreshProcessPropContext(
    _Inout_ PPV_PROPCONTEXT PropContext
    );

VOID
NTAPI
PvSetSelectThreadIdProcessPropContext(
    _Inout_ PPV_PROPCONTEXT PropContext,
    _In_ HANDLE ThreadId
    );


BOOLEAN
NTAPI
PvAddProcessPropPage(
    _Inout_ PPV_PROPCONTEXT PropContext,
    _In_ _Assume_refs_(1) PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    );

BOOLEAN
NTAPI
PvAddProcessPropPage2(
    _Inout_ PPV_PROPCONTEXT PropContext,
    _In_ HPROPSHEETPAGE PropSheetPageHandle
    );

PPH_PROCESS_PROPPAGECONTEXT
NTAPI
PvCreateProcessPropPageContext(
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    );

PPH_PROCESS_PROPPAGECONTEXT
NTAPI
PvCreateProcessPropPageContextEx(
    _In_opt_ PVOID InstanceHandle,
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    );

BOOLEAN
NTAPI
PvPropPageDlgProcHeader(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam,
    _Out_ LPPROPSHEETPAGE *PropSheetPage,
    _Out_ PPH_PROCESS_PROPPAGECONTEXT *PropPageContext
    );

VOID
NTAPI
PhPropPageDlgProcDestroy(
    _In_ HWND hwndDlg
    );

#define PH_PROP_PAGE_TAB_CONTROL_PARENT ((PPH_LAYOUT_ITEM)0x1)

PPH_LAYOUT_ITEM
NTAPI
PvAddPropPageLayoutItem(
    _In_ HWND hwnd,
    _In_ HWND Handle,
    _In_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor
    );

VOID
NTAPI
PvDoPropPageLayout(
    _In_ HWND hwnd
    );

FORCEINLINE
PPH_LAYOUT_ITEM
PvBeginPropPageLayout(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    )
{
    if (!PropPageContext->LayoutInitialized)
    {
        return PvAddPropPageLayoutItem(hwndDlg, hwndDlg,
            PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
    }
    else
    {
        return NULL;
    }
}

FORCEINLINE
VOID
PvEndPropPageLayout(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    )
{
    PvDoPropPageLayout(hwndDlg);
    PropPageContext->LayoutInitialized = TRUE;
}

#endif
