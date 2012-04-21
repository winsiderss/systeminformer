#ifndef FWTABP_H
#define FWTABP_H

HWND NTAPI FwTabCreateFunction(
    __in PVOID Context
    );

VOID NTAPI FwTabSelectionChangedCallback(
    __in PVOID Parameter1,
    __in PVOID Parameter2,
    __in PVOID Parameter3,
    __in PVOID Context
    );

VOID NTAPI FwTabSaveContentCallback(
    __in PVOID Parameter1,
    __in PVOID Parameter2,
    __in PVOID Parameter3,
    __in PVOID Context
    );

VOID NTAPI FwTabFontChangedCallback(
    __in PVOID Parameter1,
    __in PVOID Parameter2,
    __in PVOID Parameter3,
    __in PVOID Context
    );

BOOLEAN FwNodeHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG FwNodeHashtableHashFunction(
    __in PVOID Entry
    );

VOID InitializeFwTreeList(
    __in HWND hwnd
    );

PFW_EVENT_NODE AddFwNode(
    __in PFW_EVENT_ITEM FwItem
    );

PFW_EVENT_NODE FindFwNode(
    __in PFW_EVENT_ITEM FwItem
    );

VOID RemoveFwNode(
    __in PFW_EVENT_NODE FwNode
    );

VOID UpdateFwNode(
    __in PFW_EVENT_NODE FwNode
    );

BOOLEAN NTAPI FwTreeNewCallback(
    __in HWND hwnd,
    __in PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    );

PFW_EVENT_ITEM GetSelectedFwItem(
    VOID
    );

VOID GetSelectedFwItems(
    __out PFW_EVENT_ITEM **FwItems,
    __out PULONG NumberOfFwItems
    );

VOID DeselectAllFwNodes(
    VOID
    );

VOID SelectAndEnsureVisibleFwNode(
    __in PFW_EVENT_NODE FwNode
    );

VOID CopyFwList(
    VOID
    );

VOID WriteFwList(
    __inout PPH_FILE_STREAM FileStream,
    __in ULONG Mode
    );

VOID HandleFwCommand(
    __in ULONG Id
    );

VOID InitializeFwMenu(
    __in PPH_EMENU Menu,
    __in PFW_EVENT_ITEM *FwItems,
    __in ULONG NumberOfFwItems
    );

VOID ShowFwContextMenu(
    __in POINT Location
    );

VOID NTAPI FwItemAddedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI FwItemModifiedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI FwItemRemovedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI FwItemsUpdatedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI OnFwItemAdded(
    __in PVOID Parameter
    );

VOID NTAPI OnFwItemModified(
    __in PVOID Parameter
    );

VOID NTAPI OnFwItemRemoved(
    __in PVOID Parameter
    );

VOID NTAPI OnFwItemsUpdated(
    __in PVOID Parameter
    );

#endif
