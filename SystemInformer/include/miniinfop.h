/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015-2016
 *     dmex    2017-2023
 *
 */

#ifndef PH_MINIINFOP_H
#define PH_MINIINFOP_H

// Constants

#define MIP_TIMER_PIN_FIRST 1
#define MIP_TIMER_PIN_LAST (MIP_TIMER_PIN_FIRST + MaxMiniInfoPinType - 1)

#define MIP_MSG_FIRST (WM_APP + 150)
#define MIP_MSG_UPDATE (WM_APP + 150)
#define MIP_MSG_LAST (WM_APP + 151)

#define MIP_UNPIN_CHILD_CONTROL_DELAY 1000
#define MIP_UNPIN_HOVER_DELAY 250

#define MIP_REFRESH_AUTOMATICALLY_PINNED 0x1
#define MIP_REFRESH_AUTOMATICALLY_UNPINNED 0x2
#define MIP_REFRESH_AUTOMATICALLY_FLAG(Pinned) \
    ((Pinned) ? MIP_REFRESH_AUTOMATICALLY_PINNED : MIP_REFRESH_AUTOMATICALLY_UNPINNED)

// Misc.

#define SET_BUTTON_ICON(hwndDlg, Id, Icon) \
    SendMessage(GetDlgItem(hwndDlg, (Id)), BM_SETIMAGE, IMAGE_ICON, (LPARAM)(Icon))

// Dialog procedure

RTL_ATOM PhMipContainerInitializeWindowClass(
    VOID
    );

LRESULT CALLBACK PhMipContainerWndProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhMipMiniInfoDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// Container event handlers

VOID PhMipContainerOnShowWindow(
    _In_ BOOLEAN Showing,
    _In_ ULONG State
    );

VOID PhMipContainerOnActivate(
    _In_ ULONG Type,
    _In_ BOOLEAN Minimized
    );

VOID PhMipContainerOnSize(
    VOID
    );

VOID PhMipContainerOnSizing(
    _In_ ULONG Edge,
    _In_ PRECT DragRectangle
    );

VOID PhMipContainerOnExitSizeMove(
    VOID
    );

BOOLEAN PhMipContainerOnEraseBkgnd(
    _In_ HDC hdc
    );

VOID PhMipContainerOnTimer(
    _In_ ULONG Id
    );

// Child dialog event handlers

VOID PhMipOnInitDialog(
    VOID
    );

VOID PhMipOnShowWindow(
    _In_ BOOLEAN Showing,
    _In_ ULONG State
    );

VOID PhMipOnCommand(
    _In_ ULONG Id,
    _In_ ULONG Code
    );

_Success_(return)
BOOLEAN
PhMipOnNotify(
    _In_ NMHDR *Header,
    _Out_ LRESULT *Result
    );

_Success_(return)
BOOLEAN
PhMipOnCtlColorXxx(
    _In_ ULONG Message,
    _In_ HWND hwnd,
    _In_ HDC hdc,
    _Out_ HBRUSH *Brush
    );

BOOLEAN PhMipOnDrawItem(
    _In_ ULONG_PTR Id,
    _In_ DRAWITEMSTRUCT *DrawItemStruct
    );

VOID PhMipOnUserMessage(
    _In_ ULONG Message,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam
    );

// Framework

typedef enum _PH_MIP_ADJUST_PIN_RESULT
{
    NoAdjustPinResult,
    ShowAdjustPinResult,
    HideAdjustPinResult
} PH_MIP_ADJUST_PIN_RESULT;

BOOLEAN NTAPI PhMipMessageLoopFilter(
    _In_ PMSG Message,
    _In_ PVOID Context
    );

VOID NTAPI PhMipUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

PH_MIP_ADJUST_PIN_RESULT PhMipAdjustPin(
    _In_ PH_MINIINFO_PIN_TYPE PinType,
    _In_ LONG PinCount
    );

VOID PhMipCalculateWindowRectangle(
    _In_ PPOINT SourcePoint,
    _Out_ PPH_RECTANGLE WindowRectangle
    );

VOID PhMipInitializeParameters(
    VOID
    );

PPH_MINIINFO_SECTION PhMipCreateSection(
    _In_ PPH_MINIINFO_SECTION Template
    );

VOID PhMipDestroySection(
    _In_ PPH_MINIINFO_SECTION Section
    );

PPH_MINIINFO_SECTION PhMipFindSection(
    _In_ PPH_STRINGREF Name
    );

PPH_MINIINFO_SECTION PhMipCreateInternalSection(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_ PPH_MINIINFO_SECTION_CALLBACK Callback
    );

VOID PhMipCreateSectionDialog(
    _In_ PPH_MINIINFO_SECTION Section
    );

VOID PhMipChangeSection(
    _In_ PPH_MINIINFO_SECTION NewSection
    );

VOID PhMipSetSectionText(
    _In_ struct _PH_MINIINFO_SECTION *Section,
    _In_opt_ PPH_STRING Text
    );

VOID PhMipUpdateSectionText(
    _In_ PPH_MINIINFO_SECTION Section
    );

VOID PhMipLayout(
    VOID
    );

VOID PhMipBeginChildControlPin(
    VOID
    );

VOID PhMipEndChildControlPin(
    VOID
    );

VOID PhMipRefresh(
    VOID
    );

VOID PhMipToggleRefreshAutomatically(
    VOID
    );

VOID PhMipSetPinned(
    _In_ BOOLEAN Pinned
    );

VOID PhMipShowSectionMenu(
    VOID
    );

VOID PhMipShowOptionsMenu(
    VOID
    );

LRESULT CALLBACK PhMipSectionControlHookWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// List-based section

#define MIP_MAX_PROCESS_GROUPS 15
#define MIP_SINGLE_COLUMN_ID 0

#define MIP_CELL_PADDING 5
#define MIP_ICON_PADDING 3
#define MIP_INNER_PADDING 3

typedef struct _PH_MIP_GROUP_NODE
{
    union
    {
        PH_TREENEW_NODE Node;
        PH_MINIINFO_LIST_SECTION_SORT_DATA SortData;
    };
    PPH_PROCESS_GROUP ProcessGroup;
    HANDLE RepresentativeProcessId;
    LARGE_INTEGER RepresentativeCreateTime;
    BOOLEAN RepresentativeIsHung;

    PPH_STRING TooltipText;
} PH_MIP_GROUP_NODE, *PPH_MIP_GROUP_NODE;

PPH_MINIINFO_LIST_SECTION PhMipCreateListSection(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_ PPH_MINIINFO_LIST_SECTION Template
    );

PPH_MINIINFO_LIST_SECTION PhMipCreateInternalListSection(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_ PPH_MINIINFO_LIST_SECTION_CALLBACK Callback
    );

BOOLEAN PhMipListSectionCallback(
    _In_ PPH_MINIINFO_SECTION Section,
    _In_ PH_MINIINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

INT_PTR CALLBACK PhMipListSectionDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhMipListSectionSortFunction(
    _In_ PPH_LIST List,
    _In_opt_ PVOID Context
    );

VOID PhMipTickListSection(
    _In_ PPH_MINIINFO_LIST_SECTION ListSection
    );

VOID PhMipClearListSection(
    _In_ PPH_MINIINFO_LIST_SECTION ListSection
    );

LONG PhMipCalculateRowHeight(
    _In_ HWND hwnd
    );

PPH_MIP_GROUP_NODE PhMipAddGroupNode(
    _In_ PPH_MINIINFO_LIST_SECTION ListSection,
    _In_ PPH_PROCESS_GROUP ProcessGroup
    );

VOID PhMipDestroyGroupNode(
    _In_ PPH_MIP_GROUP_NODE Node
    );

BOOLEAN PhMipListSectionTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

PPH_STRING PhMipGetGroupNodeTooltip(
    _In_ PPH_MINIINFO_LIST_SECTION ListSection,
    _In_ PPH_MIP_GROUP_NODE Node
    );

PPH_MIP_GROUP_NODE PhMipGetSelectedGroupNode(
    _In_ PPH_MINIINFO_LIST_SECTION ListSection
    );

VOID PhMipShowListSectionContextMenu(
    _In_ PPH_MINIINFO_LIST_SECTION ListSection,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    );

VOID PhMipHandleListSectionCommand(
    _In_ PPH_MINIINFO_LIST_SECTION ListSection,
    _In_ PPH_PROCESS_GROUP ProcessGroup,
    _In_ ULONG Id
    );

// CPU section

BOOLEAN PhMipCpuListSectionCallback(
    _In_ struct _PH_MINIINFO_LIST_SECTION *ListSection,
    _In_ PH_MINIINFO_LIST_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

int __cdecl PhMipCpuListSectionProcessCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    );

int __cdecl PhMipCpuListSectionNodeCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    );

// Commit charge section

BOOLEAN PhMipCommitListSectionCallback(
    _In_ struct _PH_MINIINFO_LIST_SECTION *ListSection,
    _In_ PH_MINIINFO_LIST_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

int __cdecl PhMipCommitListSectionProcessCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    );

int __cdecl PhMipCommitListSectionNodeCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    );

// Physical memory section

BOOLEAN PhMipPhysicalListSectionCallback(
    _In_ struct _PH_MINIINFO_LIST_SECTION *ListSection,
    _In_ PH_MINIINFO_LIST_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

int __cdecl PhMipPhysicalListSectionProcessCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    );

int __cdecl PhMipPhysicalListSectionNodeCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    );

// I/O section

BOOLEAN PhMipIoListSectionCallback(
    _In_ struct _PH_MINIINFO_LIST_SECTION *ListSection,
    _In_ PH_MINIINFO_LIST_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

int __cdecl PhMipIoListSectionProcessCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    );

int __cdecl PhMipIoListSectionNodeCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    );

#endif
