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

#ifndef PH_SYSINFO_H
#define PH_SYSINFO_H

// begin_phapppub
typedef enum _PH_SYSINFO_VIEW_TYPE
{
    SysInfoSummaryView,
    SysInfoSectionView
} PH_SYSINFO_VIEW_TYPE;

typedef _Function_class_(PH_SYSINFO_COLOR_SETUP_FUNCTION)
VOID NTAPI PH_SYSINFO_COLOR_SETUP_FUNCTION(
    _Out_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ COLORREF Color1,
    _In_ COLORREF Color2,
    _In_ LONG WindowDpi
    );
typedef PH_SYSINFO_COLOR_SETUP_FUNCTION* PPH_SYSINFO_COLOR_SETUP_FUNCTION;

typedef struct _PH_SYSINFO_PARAMETERS
{
    HWND SysInfoWindowHandle;
    HWND ContainerWindowHandle;

    PPH_SYSINFO_COLOR_SETUP_FUNCTION ColorSetupFunction;

    HFONT Font;
    HFONT MediumFont;
    HFONT LargeFont;
    ULONG FontHeight;
    ULONG FontAverageWidth;
    ULONG MediumFontHeight;
    ULONG MediumFontAverageWidth;
    COLORREF GraphBackColor;
    COLORREF PanelForeColor;

    ULONG MinimumGraphHeight;
    ULONG SectionViewGraphHeight;
    LONG PanelWidth;
    LONG WindowDpi;
// end_phapppub

    ULONG PanelPadding;
    ULONG WindowPadding;
    ULONG GraphPadding;
    ULONG SmallGraphWidth;
    ULONG SmallGraphPadding;
    ULONG SeparatorWidth;
    ULONG CpuPadding;
    ULONG MemoryPadding;
// begin_phapppub
} PH_SYSINFO_PARAMETERS, *PPH_SYSINFO_PARAMETERS;

typedef enum _PH_SYSINFO_SECTION_MESSAGE
{
    SysInfoCreate,
    SysInfoDestroy,
    SysInfoTick,
    SysInfoViewChanging, // PH_SYSINFO_VIEW_TYPE Parameter1, PPH_SYSINFO_SECTION Parameter2
    SysInfoCreateDialog, // PPH_SYSINFO_CREATE_DIALOG Parameter1
    SysInfoGraphGetDrawInfo, // PPH_GRAPH_DRAW_INFO Parameter1
    SysInfoGraphGetTooltipText, // PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT Parameter1
    SysInfoGraphDrawPanel, // PPH_SYSINFO_DRAW_PANEL Parameter1
    SysInfoDpiChanged, // ULONG Parameter1
    MaxSysInfoMessage
} PH_SYSINFO_SECTION_MESSAGE;

typedef struct _PH_SYSINFO_SECTION *PPH_SYSINFO_SECTION;

typedef _Function_class_(PH_SYSINFO_SECTION_CALLBACK)
BOOLEAN NTAPI PH_SYSINFO_SECTION_CALLBACK(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );
typedef PH_SYSINFO_SECTION_CALLBACK* PPH_SYSINFO_SECTION_CALLBACK;

typedef struct _PH_SYSINFO_CREATE_DIALOG
{
    BOOLEAN CustomCreate;

    // Parameters for default create
    PVOID Instance;
    PWSTR Template;
    DLGPROC DialogProc;
    PVOID Parameter;
} PH_SYSINFO_CREATE_DIALOG, *PPH_SYSINFO_CREATE_DIALOG;

typedef struct _PH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT
{
    ULONG Index;
    PH_STRINGREF Text;
} PH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT, *PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT;

typedef struct _PH_SYSINFO_DRAW_PANEL
{
    HDC hdc;
    RECT Rect;
    BOOLEAN CustomDraw;

    // Parameters for default draw
    PPH_STRING Title;
    PPH_STRING SubTitle;
    PPH_STRING SubTitleOverflow;
} PH_SYSINFO_DRAW_PANEL, *PPH_SYSINFO_DRAW_PANEL;
// end_phapppub

// begin_phapppub
typedef struct _PH_SYSINFO_SECTION
{
    // Public

    // Initialization
    PH_STRINGREF Name;
    ULONG Flags;
    PPH_SYSINFO_SECTION_CALLBACK Callback;
    PVOID Context;

    // State
    HWND GraphHandle;
    PH_GRAPH_STATE GraphState;
    PPH_SYSINFO_PARAMETERS Parameters;

// end_phapppub

    // Private

    struct
    {
        ULONG GraphHot : 1;
        ULONG PanelHot : 1;
        ULONG HasFocus : 1;
        ULONG HideFocus : 1;
        ULONG SpareFlags : 28;
    };

    HWND DialogHandle;
    HWND PanelHandle;
    ULONG PanelId;

    WNDPROC GraphWindowProc;
    WNDPROC PanelWindowProc;
// begin_phapppub
} PH_SYSINFO_SECTION, *PPH_SYSINFO_SECTION;
// end_phapppub

VOID PhSiNotifyChangeSettings(
    VOID
    );

// begin_phapppub
PHAPPAPI
VOID
NTAPI
PhSiSetColorsGraphDrawInfo(
    _Out_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ COLORREF Color1,
    _In_ COLORREF Color2,
    _In_ LONG WindowDpi
    );

PHAPPAPI
PPH_STRING
NTAPI
PhSiSizeLabelYFunction(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataIndex,
    _In_ FLOAT Value,
    _In_ FLOAT Parameter
    );

PHAPPAPI
PPH_STRING
NTAPI
PhSiDoubleLabelYFunction(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataIndex,
    _In_ FLOAT Value,
    _In_ FLOAT Parameter
    );

PHAPPAPI
PPH_STRING
NTAPI
PhSiUInt64LabelYFunction(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataIndex,
    _In_ FLOAT Value,
    _In_ FLOAT Parameter
    );

PHAPPAPI
VOID
NTAPI
PhShowSystemInformationDialog(
    _In_opt_ PCWSTR SectionName
    );
// end_phapppub

#endif
