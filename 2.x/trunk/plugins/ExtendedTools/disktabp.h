#ifndef DISKTABP_H
#define DISKTABP_H

HWND NTAPI EtpDiskTabCreateFunction(
    __in PVOID Context
    );

VOID NTAPI EtpDiskTabSaveContentCallback(
    __in PVOID Parameter1,
    __in PVOID Parameter2,
    __in PVOID Parameter3,
    __in PVOID Context
    );

BOOLEAN EtpDiskNodeHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG EtpDiskNodeHashtableHashFunction(
    __in PVOID Entry
    );

VOID EtInitializeDiskTreeList(
    __in HWND hwnd
    );

PET_DISK_NODE EtAddDiskNode(
    __in PET_DISK_ITEM DiskItem
    );

PET_DISK_NODE EtFindDiskNode(
    __in PET_DISK_ITEM DiskItem
    );

VOID EtRemoveDiskNode(
    __in PET_DISK_NODE DiskNode
    );

VOID EtUpdateDiskNode(
    __in PET_DISK_NODE DiskNode
    );

BOOLEAN NTAPI EtpDiskTreeNewCallback(
    __in HWND hwnd,
    __in PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    );

PPH_STRING EtpGetDiskItemProcessName(
    __in PET_DISK_ITEM DiskItem
    );

PET_DISK_ITEM EtGetSelectedDiskItem();

VOID EtGetSelectedDiskItems(
    __out PET_DISK_ITEM **DiskItems,
    __out PULONG NumberOfDiskItems
    );

VOID EtDeselectAllDiskNodes();

VOID EtSelectAndEnsureVisibleDiskNode(
    __in PET_DISK_NODE DiskNode
    );

VOID EtCopyDiskList();

VOID EtWriteDiskList(
    __inout PPH_FILE_STREAM FileStream,
    __in ULONG Mode
    );

VOID EtHandleDiskCommand(
    __in ULONG Id
    );

VOID EtpInitializeDiskMenu(
    __in PPH_EMENU Menu,
    __in PET_DISK_ITEM *DiskItems,
    __in ULONG NumberOfDiskItems
    );

VOID EtShowDiskContextMenu(
    __in POINT Location
    );

VOID NTAPI EtpDiskItemAddedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI EtpDiskItemModifiedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI EtpDiskItemRemovedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI EtpDiskItemsUpdatedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI EtpOnDiskItemAdded(
    __in PVOID Parameter
    );

VOID NTAPI EtpOnDiskItemModified(
    __in PVOID Parameter
    );

VOID NTAPI EtpOnDiskItemRemoved(
    __in PVOID Parameter
    );

VOID NTAPI EtpOnDiskItemsUpdated(
    __in PVOID Parameter
    );

#endif
