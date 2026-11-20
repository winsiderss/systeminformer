/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 */

#ifndef _PH_GRAPHSTYLE_H
#define _PH_GRAPHSTYLE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _PH_GRAPH_SERIES
{
    PhGraphSeriesCpuKernel,
    PhGraphSeriesCpuUser,
    PhGraphSeriesIoReadOther,
    PhGraphSeriesIoWrite,
    PhGraphSeriesPrivate,
    PhGraphSeriesPhysical,
    PhGraphSeriesPower,
    PhGraphSeriesTemperature,
    PhGraphSeriesFan,
    PhGraphSeriesMaximum
} PH_GRAPH_SERIES;

typedef enum _PH_GRAPH_FILL
{
    PhGraphFillGradient,
    PhGraphFillNone,
    PhGraphFillTranslucent,
    PhGraphFillSolid,
    PhGraphFillMaximum
} PH_GRAPH_FILL;

typedef enum _PH_GRAPH_SURFACE
{
    PhGraphSurfacePanel,
    PhGraphSurfaceIcon,
    PhGraphSurfaceCell
} PH_GRAPH_SURFACE;

#define PH_GRAPH_COLOR_AUTOMATIC ((COLORREF)0xFF000000)
#define PH_GRAPH_COLOR_TRANSPARENT ((COLORREF)0xFE000000) // IconBackColor and CellBackColor only

typedef struct _PH_GRAPH_STYLE
{
    PPH_STRING Id;
    PPH_STRING Name;
    PPH_STRING BaseId; // built-in this was copied from; NULL for built-ins
    BOOLEAN BuiltIn;
    BOOLEAN Dark; // tone that Automatic colors resolve for; copies keep their base's
    BOOLEAN Smooth;
    BOOLEAN Motion;
    BOOLEAN ShowText;
    BOOLEAN ShowGrid;
    UCHAR Fill; // PH_GRAPH_FILL
    UCHAR TextBoxOpacity; // 0-100
    COLORREF PanelBackColor; // or PH_GRAPH_COLOR_AUTOMATIC
    COLORREF IconBackColor; // or PH_GRAPH_COLOR_AUTOMATIC, PH_GRAPH_COLOR_TRANSPARENT
    COLORREF CellBackColor; // or PH_GRAPH_COLOR_AUTOMATIC, PH_GRAPH_COLOR_TRANSPARENT
    COLORREF GridColor; // or PH_GRAPH_COLOR_AUTOMATIC
    COLORREF TextColor; // or PH_GRAPH_COLOR_AUTOMATIC
    COLORREF TextBoxColor; // or PH_GRAPH_COLOR_AUTOMATIC
    COLORREF LabelYColor; // or PH_GRAPH_COLOR_AUTOMATIC
    COLORREF SeriesColor[PhGraphSeriesMaximum]; // drawn line colors
} PH_GRAPH_STYLE, *PPH_GRAPH_STYLE;

// Returns the built-in styles (Default, Classic and Microsoft in Light and Dark, then Hacker).
PHLIBAPI
ULONG
NTAPI
PhGsGetBuiltInStyles(
    _Outptr_result_buffer_(return) PPH_GRAPH_STYLE *Styles
    );

PHLIBAPI
VOID
NTAPI
PhGsCopyStyle(
    _Out_ PPH_GRAPH_STYLE Destination,
    _In_ PPH_GRAPH_STYLE Source
    );

PHLIBAPI
VOID
NTAPI
PhGsDeleteStyle(
    _Inout_ PPH_GRAPH_STYLE Style
    );

// Returns a PhAllocate'd custom copy with a unique id and the unique name "<Source> (custom)".
PHLIBAPI
PPH_GRAPH_STYLE
NTAPI
PhGsDuplicateStyle(
    _In_ PPH_GRAPH_STYLE Source,
    _In_opt_ PPH_LIST CustomStyles
    );

// Finds a built-in or custom style by id; an unknown id gives Default Dark or Default Light.
PHLIBAPI
PPH_GRAPH_STYLE
NTAPI
PhGsFindStyle(
    _In_opt_ PPH_LIST CustomStyles,
    _In_opt_ PCPH_STRINGREF Id,
    _In_ BOOLEAN DarkMode
    );

// TRUE if the trimmed name is non-empty and unique (case-insensitive) among built-in and other custom styles.
PHLIBAPI
BOOLEAN
NTAPI
PhGsValidateName(
    _In_opt_ PPH_LIST CustomStyles,
    _In_ PCPH_STRINGREF Name,
    _In_opt_ PCPH_STRINGREF ExceptId
    );

typedef struct _PH_GRAPH_STYLE_COLORS
{
    COLORREF BackColor; // 0 when TransparentBack
    BOOLEAN TransparentBack;
    COLORREF GridColor;
    COLORREF TextColor;
    COLORREF TextBoxColor;
    COLORREF LabelYColor;
} PH_GRAPH_STYLE_COLORS, *PPH_GRAPH_STYLE_COLORS;

// Resolves the style's chrome for a surface; Automatic colors derive from the theme colors given.
PHLIBAPI
VOID
NTAPI
PhGsResolveColors(
    _In_ PPH_GRAPH_STYLE Style,
    _In_ PH_GRAPH_SURFACE Surface,
    _In_ BOOLEAN DarkMode,
    _In_ COLORREF ThemeBackColor,
    _In_ COLORREF ThemeTextColor,
    _In_ BOOLEAN IconsTransparent,
    _Out_ PPH_GRAPH_STYLE_COLORS Colors
    );

// Returns the solid fill color under a line drawn on the given background.
PHLIBAPI
COLORREF
NTAPI
PhGsDeriveFillColor(
    _In_ COLORREF LineColor,
    _In_ COLORREF BackColor,
    _In_ BOOLEAN TransparentBack
    );

// Formats a list of custom styles as the GraphStyles setting value.
PHLIBAPI
PPH_STRING
NTAPI
PhGsFormatStyles(
    _In_ PPH_LIST CustomStyles
    );

// Parses the GraphStyles setting value; invalid entries are dropped and invalid fields take the base style's values.
PHLIBAPI
NTSTATUS
NTAPI
PhGsParseStyles(
    _In_ PCPH_STRINGREF Json,
    _Out_ PPH_LIST *CustomStyles
    );

// Frees a list of PhAllocate'd custom styles.
PHLIBAPI
VOID
NTAPI
PhGsFreeStyleList(
    _In_ PPH_LIST CustomStyles
    );

typedef struct _PH_GRAPH_STYLE_MIGRATION
{
    PPH_LIST NewStyles; // custom styles to add (PPH_GRAPH_STYLE)
    PPH_STRING LightId;
    PPH_STRING DarkId;
} PH_GRAPH_STYLE_MIGRATION, *PPH_GRAPH_STYLE_MIGRATION;

// Maps the legacy GraphColorMode and Color* series settings to light and dark styles.
PHLIBAPI
VOID
NTAPI
PhGsMigrateLegacy(
    _In_ ULONG GraphColorMode,
    _In_ BOOLEAN ThemeSupport,
    _In_reads_(PhGraphSeriesMaximum) CONST COLORREF* LegacySeries,
    _Out_ PPH_GRAPH_STYLE_MIGRATION Result
    );

PHLIBAPI
VOID
NTAPI
PhGsDeleteMigration(
    _Inout_ PPH_GRAPH_STYLE_MIGRATION Result
    );

// Mixes Weight/255 of Color2 into Color1.
PHLIBAPI
COLORREF
NTAPI
PhGsMixColor(
    _In_ COLORREF Color1,
    _In_ COLORREF Color2,
    _In_ ULONG Weight
    );

#ifdef __cplusplus
}
#endif

#endif
