/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 */

#include <phapp.h>
#include <commdlg.h>
#include <phsettings.h>
#include <settings.h>
#include <graph.h>
#include <sysinfo.h>
#include <graphstyles.h>
#include <math.h>

#define PH_GS_GROUP_BUILTIN 0
#define PH_GS_GROUP_CUSTOM 1
#define PH_GS_PREVIEW_COUNT 3
#define PH_GS_SAMPLE_COUNT 256
#define PH_GS_PREVIEW_TIMER_ID 1
#define PH_GS_PREVIEW_INTERVAL 1000 // ms, one sample per tick like the providers
#define WM_PH_GS_REFRESH (WM_APP + 1)
#define WM_PH_GS_RENAME (WM_APP + 2)

typedef struct _PH_GS_PAGE_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HWND PreviewHandles[PH_GS_PREVIEW_COUNT];
    PH_LAYOUT_MANAGER LayoutManager;
    PPH_LIST Styles; // working copy of the custom styles
    PPH_STRING LightId;
    PPH_STRING DarkId;
    PPH_GRAPH_STYLE Selected;
    ULONG SampleTime;
    FLOAT Data1[PH_GS_PREVIEW_COUNT][PH_GS_SAMPLE_COUNT];
    FLOAT Data2[PH_GS_PREVIEW_COUNT][PH_GS_SAMPLE_COUNT];
} PH_GS_PAGE_CONTEXT, *PPH_GS_PAGE_CONTEXT;

EXTERN_C BOOLEAN PhNfTransparencyEnabled;

static VOID PhpGsApplyPreviewOptions(
    _In_ PPH_GRAPH_STYLE Style,
    _In_reads_(Count) HWND* GraphHandles,
    _In_ ULONG Count
    );

static PPH_GS_PAGE_CONTEXT PhpGsCopyStore(
    VOID
    )
{
    PPH_GS_PAGE_CONTEXT context;
    PPH_LIST styles = PhGsGetCustomStyles();
    PPH_STRING id;

    context = PhAllocateZero(sizeof(PH_GS_PAGE_CONTEXT));
    context->Styles = PhCreateList(styles ? max(styles->Count, 4) : 4);

    if (styles)
    {
        for (ULONG i = 0; i < styles->Count; i++)
        {
            PPH_GRAPH_STYLE copy = PhAllocate(sizeof(PH_GRAPH_STYLE));

            PhGsCopyStyle(copy, styles->Items[i]);
            PhAddItemList(context->Styles, copy);
        }
    }

    id = PhGsGetAssignedStyleId(FALSE);
    context->LightId = id ? PhReferenceObject(id) : PhCreateString(L"default-light");
    id = PhGsGetAssignedStyleId(TRUE);
    context->DarkId = id ? PhReferenceObject(id) : PhCreateString(L"default-dark");

    return context;
}

static PPH_GRAPH_STYLE PhpGsAssignedStyle(
    _In_ PPH_GS_PAGE_CONTEXT Context,
    _In_ BOOLEAN DarkMode
    )
{
    PPH_STRING id = DarkMode ? Context->DarkId : Context->LightId;

    return PhGsFindStyle(Context->Styles, &id->sr, DarkMode);
}

static VOID PhpGsUpdateStatus(
    _In_ PPH_GS_PAGE_CONTEXT Context
    )
{
    BOOLEAN darkMode = PhGsIsDarkMode();
    PPH_STRING text;

    text = PhConcatStrings(
        5,
        L"Now in ",
        darkMode ? L"dark" : L"light",
        L" mode, so graphs use ",
        PhpGsAssignedStyle(Context, darkMode)->Name->Buffer,
        L"."
        );
    PhSetDialogItemText(Context->WindowHandle, IDC_GRAPHSTYLESTATUS, text->Buffer);
    PhDereferenceObject(text);
}

static VOID PhpGsAddListItem(
    _In_ PPH_GS_PAGE_CONTEXT Context,
    _In_ PPH_GRAPH_STYLE Style
    )
{
    BOOLEAN light = Style == PhpGsAssignedStyle(Context, FALSE);
    BOOLEAN dark = Style == PhpGsAssignedStyle(Context, TRUE);
    LONG index;

    index = PhAddListViewGroupItem(
        Context->ListViewHandle,
        Style->BuiltIn ? PH_GS_GROUP_BUILTIN : PH_GS_GROUP_CUSTOM,
        MAXINT,
        Style->Name->Buffer,
        Style
        );

    if (light || dark)
        PhSetListViewSubItem(Context->ListViewHandle, index, 1, light && dark ? L"Light, Dark" : light ? L"Light" : L"Dark");
}

static VOID PhpGsRefreshList(
    _In_ PPH_GS_PAGE_CONTEXT Context
    )
{
    PPH_GRAPH_STYLE styles;
    ULONG count;
    LONG index;

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);

    count = PhGsGetBuiltInStyles(&styles);

    for (ULONG i = 0; i < count; i++)
        PhpGsAddListItem(Context, &styles[i]);
    for (ULONG i = 0; i < Context->Styles->Count; i++)
        PhpGsAddListItem(Context, Context->Styles->Items[i]);

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);

    if ((index = PhFindListViewItemByParam(Context->ListViewHandle, INT_ERROR, Context->Selected)) != INT_ERROR)
    {
        ListView_SetItemState(Context->ListViewHandle, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(Context->ListViewHandle, index, FALSE);
    }

    PhpGsUpdateStatus(Context);
}

static VOID PhpGsUpdateSelection(
    _In_ PPH_GS_PAGE_CONTEXT Context
    )
{
    PPH_GRAPH_STYLE style = Context->Selected;

    if (!style)
        return;

    PhSetDialogItemText(Context->WindowHandle, IDC_GRAPHSTYLEEDIT, style->BuiltIn ? L"Duplicate to customize" : L"Edit...");
    EnableWindow(GetDlgItem(Context->WindowHandle, IDC_GRAPHSTYLEUSELIGHT), style != PhpGsAssignedStyle(Context, FALSE));
    EnableWindow(GetDlgItem(Context->WindowHandle, IDC_GRAPHSTYLEUSEDARK), style != PhpGsAssignedStyle(Context, TRUE));
    ShowWindow(GetDlgItem(Context->WindowHandle, IDC_GRAPHSTYLEDUPLICATE), style->BuiltIn ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(Context->WindowHandle, IDC_GRAPHSTYLERENAME), style->BuiltIn ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(Context->WindowHandle, IDC_GRAPHSTYLEDELETE), style->BuiltIn ? SW_HIDE : SW_SHOW);

    PhpGsApplyPreviewOptions(style, Context->PreviewHandles, PH_GS_PREVIEW_COUNT);
}

static VOID PhpGsSelect(
    _In_ PPH_GS_PAGE_CONTEXT Context,
    _In_ PPH_GRAPH_STYLE Style
    )
{
    Context->Selected = Style;
    PhpGsRefreshList(Context);
    PhpGsUpdateSelection(Context);
}

static FLOAT PhpGsClampSample(
    _In_ FLOAT Value
    )
{
    return Value < 0.0f ? 0.0f : Value > 1.0f ? 1.0f : Value;
}

// A repeatable pseudo-random value in [0, 1) for sample T.
static FLOAT PhpGsNoise(
    _In_ LONG T
    )
{
    ULONG x = (ULONG)T * 0x9e3779b1;

    x ^= x >> 15;
    x *= 0x85ebca6b;
    x ^= x >> 13;

    return (FLOAT)(x & 0xffff) / 65536.0f;
}

// Fixed sample data for preview Index at sample T (newest sample first).
static VOID PhpGsGetSamples(
    _In_ ULONG Index,
    _In_ LONG T,
    _Out_ PFLOAT Value1,
    _Out_ PFLOAT Value2
    )
{
    FLOAT t = (FLOAT)T;
    FLOAT n;

    switch (Index)
    {
    case 0: // CPU kernel and user
        *Value1 = PhpGsClampSample(0.10f + 0.06f * sinf(t * 0.31f) + 0.04f * sinf(t * 0.11f));
        *Value2 = PhpGsClampSample(0.22f + 0.16f * sinf(t * 0.17f + 1.0f) + 0.08f * sinf(t * 0.53f));
        break;
    case 1: // I/O read+other and write: sharp, irregular bursts
        n = PhpGsNoise(T);
        *Value1 = PhpGsClampSample(n > 0.7f ? 0.15f + 2.4f * (n - 0.7f) : 0.02f + 0.05f * n);
        n = PhpGsNoise(T * 7 + 3);
        *Value2 = PhpGsClampSample(n > 0.8f ? 0.10f + 1.6f * (n - 0.8f) : 0.01f + 0.03f * n);
        break;
    default: // physical memory
        *Value1 = PhpGsClampSample(0.52f + 0.06f * sinf(t * 0.04f) + 0.02f * sinf(t * 0.37f));
        *Value2 = 0.0f;
        break;
    }
}

_Function_class_(PH_GRAPH_LABEL_Y_FUNCTION)
static PPH_STRING NTAPI PhpGsLabelYFunction(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataIndex,
    _In_ FLOAT Value,
    _In_ FLOAT Parameter
    )
{
    return PhFormatString(L"%.0f%%", min(Value, 1.0f) * 100.0f);
}

// Fills a preview graph's draw info: Kind 0 = CPU, 1 = I/O, 2 = memory; icons and cells draw CPU without text.
static VOID PhpGsFillPreviewDrawInfo(
    _In_ PPH_GRAPH_STYLE Style,
    _In_ PH_GRAPH_SURFACE Surface,
    _In_ ULONG Kind,
    _In_ ULONG SampleTime,
    _In_ HWND GraphHandle,
    _In_ LONG WindowDpi,
    _Out_writes_to_(PH_GS_SAMPLE_COUNT, DrawInfo->LineDataCount) PFLOAT Data1,
    _Out_writes_to_(PH_GS_SAMPLE_COUNT, DrawInfo->LineDataCount) PFLOAT Data2,
    _Inout_ PPH_GRAPH_DRAW_INFO DrawInfo
    )
{
    static CONST PH_GRAPH_SERIES series1[PH_GS_PREVIEW_COUNT] = { PhGraphSeriesCpuKernel, PhGraphSeriesIoReadOther, PhGraphSeriesPhysical };
    static CONST PH_GRAPH_SERIES series2[PH_GS_PREVIEW_COUNT] = { PhGraphSeriesCpuUser, PhGraphSeriesIoWrite, PhGraphSeriesPhysical };
    static CONST PCWSTR texts[PH_GS_PREVIEW_COUNT] = { L"CPU 37.20%", L"R+O 12 MB/s, W 3 MB/s", L"Physical memory 52%" };
    PH_GRAPH_STYLE_COLORS colors;
    COLORREF themeBackColor;
    COLORREF themeTextColor;
    BOOLEAN panel = Surface == PhGraphSurfacePanel;
    BOOLEAN line2 = Kind < 2;
    COLORREF color1;
    COLORREF color2;
    ULONG count;

    color1 = Style->SeriesColor[series1[Kind]];
    color2 = line2 ? Style->SeriesColor[series2[Kind]] : 0;

    // Fonts (text and scale label) as every panel graph; then the previewed style's colors.
    if (panel)
    {
        DrawInfo->Flags = PH_GRAPH_LABEL_MAX_Y;
        PhSiSetColorsGraphDrawInfo(DrawInfo, color1, color2, WindowDpi);
    }

    PhGsGetToneThemeColors(Style->Dark, &themeBackColor, &themeTextColor);
    PhGsResolveColors(Style, Surface, Style->Dark, themeBackColor, themeTextColor, PhNfTransparencyEnabled, &colors);

    // A transparent background shows what is behind it: here, the window.
    if (colors.TransparentBack)
    {
        colors.BackColor = PhEnableThemeSupport ? PhThemeWindowBackgroundColor : GetSysColor(COLOR_WINDOW);
        colors.TransparentBack = FALSE;
    }

    DrawInfo->Flags = (line2 ? PH_GRAPH_USE_LINE_2 : 0);

    if (panel)
    {
        SetFlag(DrawInfo->Flags, PH_GRAPH_LABEL_MAX_Y);

        if (Style->ShowGrid)
            SetFlag(DrawInfo->Flags, PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y);
    }

    DrawInfo->LabelYFunction = PhpGsLabelYFunction;
    DrawInfo->LabelYFunctionParameter = 1.0f;
    DrawInfo->LabelMaxYIndexLimit = ULONG_MAX;
    DrawInfo->BackColor = colors.BackColor;
    DrawInfo->GridColor = colors.GridColor;
    DrawInfo->TextColor = colors.TextColor;
    DrawInfo->TextBoxColor = colors.TextBoxColor;
    DrawInfo->LabelYColor = colors.LabelYColor;
    DrawInfo->LineColor1 = color1;
    DrawInfo->LineBackColor1 = PhGsDeriveFillColor(color1, colors.BackColor, colors.TransparentBack);
    DrawInfo->LineColor2 = color2;
    DrawInfo->LineBackColor2 = PhGsDeriveFillColor(color2, colors.BackColor, colors.TransparentBack);

    count = min((ULONG)DrawInfo->Width / 2 + 2, PH_GS_SAMPLE_COUNT);

    for (ULONG i = 0; i < count; i++)
        PhpGsGetSamples(Kind, (LONG)SampleTime - (LONG)i, &Data1[i], &Data2[i]);

    DrawInfo->LineDataCount = count;
    DrawInfo->LineData1 = Data1;
    DrawInfo->LineData2 = Data2;

    if (panel && Style->ShowText)
    {
        PH_STRINGREF text;

        PhInitializeStringRefLongHint(&text, (PWSTR)texts[Kind]);
        PhSetGraphText(Graph_GetBufferedContext(GraphHandle), DrawInfo, &text, &PhNormalGraphTextMargin, &PhNormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
    }
    else
    {
        DrawInfo->Text.Buffer = NULL;
    }
}

static VOID PhpGsApplyPreviewOptions(
    _In_ PPH_GRAPH_STYLE Style,
    _In_reads_(Count) HWND* GraphHandles,
    _In_ ULONG Count
    )
{
    PH_GRAPH_RENDER_OPTIONS options;

    options.Smooth = Style->Smooth;
    options.Motion = Style->Motion;
    options.Fill = Style->Fill;
    options.TextBoxAlpha = (UCHAR)(Style->TextBoxOpacity * 255 / 100);

    for (ULONG i = 0; i < Count; i++)
    {
        Graph_SetRenderOptions(GraphHandles[i], &options);
        Graph_Draw(GraphHandles[i]);
        InvalidateRect(GraphHandles[i], NULL, FALSE);
    }
}

static VOID PhpGsAssign(
    _In_ PPH_GS_PAGE_CONTEXT Context,
    _In_ BOOLEAN DarkMode
    )
{
    PhSwapReference(DarkMode ? &Context->DarkId : &Context->LightId, Context->Selected->Id);
    PhpGsSelect(Context, Context->Selected);
}

// Copies the selected style into the list and opens its name for editing.
static VOID PhpGsDuplicate(
    _In_ PPH_GS_PAGE_CONTEXT Context
    )
{
    PPH_GRAPH_STYLE style;

    style = PhGsDuplicateStyle(Context->Selected, Context->Styles); // unique id and suggested name
    PhAddItemList(Context->Styles, style);
    PhpGsSelect(Context, style);
    PostMessage(Context->WindowHandle, WM_PH_GS_RENAME, 0, 0);
}

static VOID PhpGsEdit(
    _In_ PPH_GS_PAGE_CONTEXT Context
    )
{
    PPH_GRAPH_STYLE style = Context->Selected;

    // A built-in can't be changed; copy it instead.
    if (style->BuiltIn)
    {
        PhpGsDuplicate(Context);
        return;
    }

    PhGsShowStyleEditor(Context->WindowHandle, style, Context->Styles);
    PhpGsSelect(Context, style);
}

static VOID PhpGsBeginRename(
    _In_ PPH_GS_PAGE_CONTEXT Context
    )
{
    LONG index;

    if (!Context->Selected || Context->Selected->BuiltIn)
        return;

    if ((index = PhFindListViewItemByParam(Context->ListViewHandle, INT_ERROR, Context->Selected)) != INT_ERROR)
    {
        SetFocus(Context->ListViewHandle);
        ListView_EditLabel(Context->ListViewHandle, index);
    }
}

static VOID PhpGsDelete(
    _In_ PPH_GS_PAGE_CONTEXT Context
    )
{
    PPH_GRAPH_STYLE style = Context->Selected;
    ULONG index;

    if (style->BuiltIn || (index = PhFindItemList(Context->Styles, style)) == ULONG_MAX)
        return;

    // A deleted assignment falls back to that mode's default built-in.
    if (PhEqualString(Context->LightId, style->Id, TRUE))
        PhMoveReference(&Context->LightId, PhCreateString(L"default-light"));
    if (PhEqualString(Context->DarkId, style->Id, TRUE))
        PhMoveReference(&Context->DarkId, PhCreateString(L"default-dark"));

    PhRemoveItemList(Context->Styles, index);
    PhGsDeleteStyle(style);
    PhFree(style);

    PhpGsSelect(Context, PhpGsAssignedStyle(Context, PhGsIsDarkMode()));
}

static BOOLEAN PhpGsEndRename(
    _In_ PPH_GS_PAGE_CONTEXT Context,
    _In_ NMLVDISPINFO* DispInfo
    )
{
    static CONST PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" \t");
    PPH_GRAPH_STYLE style = (PPH_GRAPH_STYLE)DispInfo->item.lParam;
    PH_STRINGREF name;

    if (!DispInfo->item.pszText || !style || style->BuiltIn)
        return FALSE;

    PhInitializeStringRefLongHint(&name, DispInfo->item.pszText);

    if (!PhGsValidateName(Context->Styles, &name, &style->Id->sr))
    {
        PhShowError2(Context->WindowHandle, L"Unable to rename the style.", L"%s", L"The name must not be empty or the same as another style's name.");
        return FALSE;
    }

    PhTrimStringRef(&name, &whitespace, 0);
    PhMoveReference(&style->Name, PhCreateString2(&name));
    PostMessage(Context->WindowHandle, WM_PH_GS_REFRESH, 0, 0); // shows the trimmed name

    return TRUE;
}

// Style editor

#define PH_GSE_PREVIEW_COUNT 5 // CPU, I/O, memory, icon, cell

UINT_PTR CALLBACK PhpColorDlgHookProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

typedef struct _PH_GSE_COLOR_ROW
{
    ULONG ButtonId;
    ULONG AutoId;
    ULONG Offset; // COLORREF field in PH_GRAPH_STYLE
    PH_GRAPH_SURFACE Surface; // surface the Automatic value is resolved for
} PH_GSE_COLOR_ROW, *PPH_GSE_COLOR_ROW;

static CONST PH_GSE_COLOR_ROW PhpGseColorRows[] =
{
    { IDC_GSE_PANELCOLOR, IDC_GSE_PANELAUTO, FIELD_OFFSET(PH_GRAPH_STYLE, PanelBackColor), PhGraphSurfacePanel },
    { IDC_GSE_ICONCOLOR, IDC_GSE_ICONAUTO, FIELD_OFFSET(PH_GRAPH_STYLE, IconBackColor), PhGraphSurfaceIcon },
    { IDC_GSE_CELLCOLOR, IDC_GSE_CELLAUTO, FIELD_OFFSET(PH_GRAPH_STYLE, CellBackColor), PhGraphSurfaceCell },
    { IDC_GSE_GRIDCOLOR, IDC_GSE_GRIDAUTO, FIELD_OFFSET(PH_GRAPH_STYLE, GridColor), PhGraphSurfacePanel },
    { IDC_GSE_TEXTCOLOR, IDC_GSE_TEXTAUTO, FIELD_OFFSET(PH_GRAPH_STYLE, TextColor), PhGraphSurfacePanel },
    { IDC_GSE_TEXTBOXCOLOR, IDC_GSE_TEXTBOXAUTO, FIELD_OFFSET(PH_GRAPH_STYLE, TextBoxColor), PhGraphSurfacePanel },
    { IDC_GSE_LABELCOLOR, IDC_GSE_LABELAUTO, FIELD_OFFSET(PH_GRAPH_STYLE, LabelYColor), PhGraphSurfacePanel },
};

static CONST PCWSTR PhpGseSeriesNames[PhGraphSeriesMaximum] =
{
    L"CPU kernel",
    L"CPU user",
    L"I/O read and other",
    L"I/O write",
    L"Private bytes",
    L"Physical memory",
    L"Power",
    L"Temperature",
    L"Fan",
};

// Fill choices in the order shown.
static CONST UCHAR PhpGseFillOrder[] = { PhGraphFillNone, PhGraphFillSolid, PhGraphFillTranslucent, PhGraphFillGradient };
static CONST PCWSTR PhpGseFillNames[] = { L"None", L"Solid", L"Translucent", L"Gradient" };

typedef struct _PH_GSE_CONTEXT
{
    HWND WindowHandle;
    HWND SeriesListHandle;
    PH_GRAPH_STYLE Style; // the copy being edited
    PPH_LIST CustomStyles;
    HWND PreviewHandles[PH_GSE_PREVIEW_COUNT];
    ULONG SampleTime;
    FLOAT Data1[PH_GSE_PREVIEW_COUNT][PH_GS_SAMPLE_COUNT];
    FLOAT Data2[PH_GSE_PREVIEW_COUNT][PH_GS_SAMPLE_COUNT];
} PH_GSE_CONTEXT, *PPH_GSE_CONTEXT;

FORCEINLINE COLORREF* PhpGseField(
    _In_ PPH_GSE_CONTEXT Context,
    _In_ CONST PH_GSE_COLOR_ROW* Row
    )
{
    return (COLORREF*)PTR_ADD_OFFSET(&Context->Style, Row->Offset);
}

// The color a row shows: its own, or what Automatic resolves to for the style's tone.
static COLORREF PhpGseShownColor(
    _In_ PPH_GSE_CONTEXT Context,
    _In_ CONST PH_GSE_COLOR_ROW* Row
    )
{
    COLORREF color = *PhpGseField(Context, Row);
    PH_GRAPH_STYLE_COLORS colors;
    COLORREF themeBackColor;
    COLORREF themeTextColor;

    if (color != PH_GRAPH_COLOR_AUTOMATIC && color != PH_GRAPH_COLOR_TRANSPARENT)
        return color;

    PhGsGetToneThemeColors(Context->Style.Dark, &themeBackColor, &themeTextColor);
    PhGsResolveColors(&Context->Style, Row->Surface, Context->Style.Dark, themeBackColor, themeTextColor, FALSE, &colors);

    switch (Row->ButtonId)
    {
    case IDC_GSE_GRIDCOLOR:
        return colors.GridColor;
    case IDC_GSE_TEXTCOLOR:
        return colors.TextColor;
    case IDC_GSE_TEXTBOXCOLOR:
        return colors.TextBoxColor;
    case IDC_GSE_LABELCOLOR:
        return colors.LabelYColor;
    default:
        return colors.BackColor;
    }
}

static VOID PhpGseLoad(
    _In_ PPH_GSE_CONTEXT Context
    )
{
    HWND hwnd = Context->WindowHandle;
    PPH_GRAPH_STYLE style = &Context->Style;
    WCHAR opacity[16];

    Button_SetCheck(GetDlgItem(hwnd, IDC_GSE_SMOOTH), style->Smooth ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(hwnd, IDC_GSE_CLASSIC), style->Smooth ? BST_UNCHECKED : BST_CHECKED);
    Button_SetCheck(GetDlgItem(hwnd, IDC_GSE_MOTION), style->Motion ? BST_CHECKED : BST_UNCHECKED);
    EnableWindow(GetDlgItem(hwnd, IDC_GSE_MOTION), style->Smooth);

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpGseFillOrder); i++)
    {
        if (PhpGseFillOrder[i] == style->Fill)
            ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_GSE_FILL), i);
    }

    Button_SetCheck(GetDlgItem(hwnd, IDC_GSE_SHOWGRID), style->ShowGrid ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(hwnd, IDC_GSE_SHOWTEXT), style->ShowText ? BST_CHECKED : BST_UNCHECKED);
    SendDlgItemMessage(hwnd, IDC_GSE_OPACITY, TBM_SETPOS, TRUE, style->TextBoxOpacity);
    _snwprintf_s(opacity, RTL_NUMBER_OF(opacity), _TRUNCATE, L"%u%%", style->TextBoxOpacity);
    PhSetDialogItemText(hwnd, IDC_GSE_OPACITYTEXT, opacity);

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpGseColorRows); i++)
    {
        CONST PH_GSE_COLOR_ROW* row = &PhpGseColorRows[i];
        BOOLEAN enabled = row->ButtonId != IDC_GSE_GRIDCOLOR || style->ShowGrid;

        Button_SetCheck(GetDlgItem(hwnd, row->AutoId), *PhpGseField(Context, row) == PH_GRAPH_COLOR_AUTOMATIC ? BST_CHECKED : BST_UNCHECKED);
        EnableWindow(GetDlgItem(hwnd, row->AutoId), enabled);
        EnableWindow(GetDlgItem(hwnd, row->ButtonId), enabled);
        InvalidateRect(GetDlgItem(hwnd, row->ButtonId), NULL, TRUE);
    }

    Button_SetCheck(GetDlgItem(hwnd, IDC_GSE_ICONTRANSPARENT), style->IconBackColor == PH_GRAPH_COLOR_TRANSPARENT ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(hwnd, IDC_GSE_CELLTRANSPARENT), style->CellBackColor == PH_GRAPH_COLOR_TRANSPARENT ? BST_CHECKED : BST_UNCHECKED);
    InvalidateRect(Context->SeriesListHandle, NULL, TRUE);

    PhpGsApplyPreviewOptions(style, Context->PreviewHandles, PH_GSE_PREVIEW_COUNT);
}

static VOID PhpGseLoadBase(
    _In_ PPH_GSE_CONTEXT Context
    )
{
    PPH_GRAPH_STYLE base = PhGsFindStyle(NULL, Context->Style.BaseId ? &Context->Style.BaseId->sr : NULL, TRUE);
    PPH_STRING text;

    text = PhConcatStrings2(L"Copied from ", base->Name->Buffer);
    PhSetDialogItemText(Context->WindowHandle, IDC_GSE_BASE, text->Buffer);
    PhDereferenceObject(text);
    text = PhConcatStrings2(L"Reset to ", base->Name->Buffer);
    PhSetDialogItemText(Context->WindowHandle, IDC_GSE_RESET, text->Buffer);
    PhDereferenceObject(text);
}

// Resets every property to the base built-in's; the id, name and base stay.
static VOID PhpGseReset(
    _In_ PPH_GSE_CONTEXT Context
    )
{
    PPH_GRAPH_STYLE base = PhGsFindStyle(NULL, Context->Style.BaseId ? &Context->Style.BaseId->sr : NULL, TRUE);
    PPH_STRING id = Context->Style.Id;
    PPH_STRING name = Context->Style.Name;
    PPH_STRING baseId = Context->Style.BaseId;

    Context->Style = *base;
    Context->Style.Id = id;
    Context->Style.Name = name;
    Context->Style.BaseId = baseId;
    Context->Style.BuiltIn = FALSE;
}

// Shows the color picker; TRUE with the chosen color.
_Success_(return)
static BOOLEAN PhpGseChooseColor(
    _In_ PPH_GSE_CONTEXT Context,
    _In_ COLORREF InitialColor,
    _Out_ COLORREF* Color
    )
{
    CHOOSECOLOR chooseColor;
    COLORREF customColors[16] = { 0 };
    BOOLEAN chosen;

    PhLoadCustomColorList(SETTING_OPTIONS_CUSTOM_COLOR_LIST, customColors, RTL_NUMBER_OF(customColors));

    memset(&chooseColor, 0, sizeof(CHOOSECOLOR));
    chooseColor.lStructSize = sizeof(CHOOSECOLOR);
    chooseColor.hwndOwner = Context->WindowHandle;
    chooseColor.rgbResult = InitialColor;
    chooseColor.lpCustColors = customColors;
    chooseColor.lpfnHook = PhpColorDlgHookProc;
    chooseColor.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_SOLIDCOLOR | CC_ENABLEHOOK | CC_RGBINIT;

    chosen = !!ChooseColor(&chooseColor);
    PhSaveCustomColorList(SETTING_OPTIONS_CUSTOM_COLOR_LIST, customColors, RTL_NUMBER_OF(customColors));

    if (!chosen)
        return FALSE;

    *Color = chooseColor.rgbResult & 0xffffff;
    return TRUE;
}

static BOOLEAN PhpGseDrawColorButton(
    _In_ PPH_GSE_CONTEXT Context,
    _In_ LPDRAWITEMSTRUCT DrawItem
    )
{
    CONST PH_GSE_COLOR_ROW* row = NULL;
    COLORREF backColor = PhEnableThemeSupport ? PhThemeWindowBackgroundColor : GetSysColor(COLOR_BTNFACE);
    COLORREF textColor = PhEnableThemeSupport ? PhThemeWindowTextColor : GetSysColor(COLOR_BTNTEXT);
    BOOLEAN disabled = !!(DrawItem->itemState & ODS_DISABLED);
    HDC hdc = DrawItem->hDC;
    RECT rect = DrawItem->rcItem;
    RECT swatch;
    COLORREF field;
    COLORREF color;

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpGseColorRows); i++)
    {
        if (PhpGseColorRows[i].ButtonId == DrawItem->CtlID)
            row = &PhpGseColorRows[i];
    }

    if (!row)
        return FALSE;

    SetDCBrushColor(hdc, backColor);
    FillRect(hdc, &rect, PhGetStockBrush(DC_BRUSH));

    field = *PhpGseField(Context, row);
    color = PhpGseShownColor(Context, row);

    if (disabled)
        color = PhGsMixColor(color, backColor, 160);

    swatch = rect;
    InflateRect(&swatch, -2, -2);
    SetBkMode(hdc, TRANSPARENT);

    if (field == PH_GRAPH_COLOR_TRANSPARENT)
    {
        SetTextColor(hdc, PhGsMixColor(textColor, backColor, 100));
        DrawText(hdc, L"None", -1, &swatch, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);
    }
    else
    {
        SetDCBrushColor(hdc, color);
        FillRect(hdc, &swatch, PhGetStockBrush(DC_BRUSH));

        // Automatic shows the color it resolves to, marked as such.
        if (field == PH_GRAPH_COLOR_AUTOMATIC)
        {
            SetTextColor(hdc, GetRValue(color) * 299 + GetGValue(color) * 587 + GetBValue(color) * 114 < 128 * 1000 ? RGB(255, 255, 255) : RGB(0, 0, 0));
            DrawText(hdc, L"Auto", -1, &swatch, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);
        }
    }

    SetDCBrushColor(hdc, PhGsMixColor(backColor, textColor, disabled ? 60 : 140));
    FrameRect(hdc, &swatch, PhGetStockBrush(DC_BRUSH));

    if (DrawItem->itemState & ODS_FOCUS)
        DrawFocusRect(hdc, &rect);

    return TRUE;
}

// Draws a series row: a swatch of its color and its name in the normal text color.
static VOID PhpGseDrawSeriesRow(
    _In_ PPH_GSE_CONTEXT Context,
    _In_ LPDRAWITEMSTRUCT DrawItem
    )
{
    COLORREF backColor = PhEnableThemeSupport ? PhThemeWindowBackgroundColor : GetSysColor(COLOR_WINDOW);
    COLORREF textColor = PhEnableThemeSupport ? PhThemeWindowTextColor : GetSysColor(COLOR_WINDOWTEXT);
    ULONG series = DrawItem->itemID;
    HDC hdc = DrawItem->hDC;
    RECT rect = DrawItem->rcItem;
    RECT swatch;

    if (series >= PhGraphSeriesMaximum)
        return;

    if (DrawItem->itemState & ODS_SELECTED)
        backColor = PhEnableThemeSupport ? PhGsMixColor(backColor, textColor, 40) : GetSysColor(COLOR_HIGHLIGHT);
    if ((DrawItem->itemState & ODS_SELECTED) && !PhEnableThemeSupport)
        textColor = GetSysColor(COLOR_HIGHLIGHTTEXT);

    SetDCBrushColor(hdc, backColor);
    FillRect(hdc, &rect, PhGetStockBrush(DC_BRUSH));

    swatch = rect;
    InflateRect(&swatch, 0, -2);
    swatch.left += 4;
    swatch.right = swatch.left + (swatch.bottom - swatch.top) * 2;
    SetDCBrushColor(hdc, Context->Style.SeriesColor[series]);
    FillRect(hdc, &swatch, PhGetStockBrush(DC_BRUSH));
    SetDCBrushColor(hdc, PhGsMixColor(backColor, textColor, 140));
    FrameRect(hdc, &swatch, PhGetStockBrush(DC_BRUSH));

    rect.left = swatch.right + 8;
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);
    DrawText(hdc, PhpGseSeriesNames[series], -1, &rect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);

    if (DrawItem->itemState & ODS_FOCUS)
        DrawFocusRect(hdc, &DrawItem->rcItem);
}

static VOID PhpGseChooseSeriesColor(
    _In_ PPH_GSE_CONTEXT Context,
    _In_ LONG Index
    )
{
    COLORREF color;

    if (Index < 0 || (ULONG)Index >= PhGraphSeriesMaximum)
        return;

    if (PhpGseChooseColor(Context, Context->Style.SeriesColor[Index], &color))
    {
        Context->Style.SeriesColor[Index] = color;
        PhpGseLoad(Context);
    }
}

static BOOLEAN PhpGseSave(
    _In_ PPH_GSE_CONTEXT Context
    )
{
    static CONST PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" \t");
    PPH_STRING name;
    PH_STRINGREF nameSr;

    name = PhGetWindowText(GetDlgItem(Context->WindowHandle, IDC_GSE_NAME));
    nameSr = name->sr;

    if (!PhGsValidateName(Context->CustomStyles, &nameSr, &Context->Style.Id->sr))
    {
        PhShowError2(Context->WindowHandle, L"Unable to save the style.", L"%s", L"The name must not be empty or the same as another style's name.");
        PhDereferenceObject(name);
        SetFocus(GetDlgItem(Context->WindowHandle, IDC_GSE_NAME));
        return FALSE;
    }

    PhTrimStringRef(&nameSr, &whitespace, 0);
    PhMoveReference(&Context->Style.Name, PhCreateString2(&nameSr));
    PhDereferenceObject(name);

    return TRUE;
}

static INT_PTR CALLBACK PhpGseDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_GSE_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_GSE_CONTEXT)lParam;
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
            HWND fillHandle = GetDlgItem(hwndDlg, IDC_GSE_FILL);

            context->WindowHandle = hwndDlg;
            context->SeriesListHandle = GetDlgItem(hwndDlg, IDC_GSE_SERIESLIST);
            context->PreviewHandles[0] = GetDlgItem(hwndDlg, IDC_GSE_PREVIEWCPU);
            context->PreviewHandles[1] = GetDlgItem(hwndDlg, IDC_GSE_PREVIEWIO);
            context->PreviewHandles[2] = GetDlgItem(hwndDlg, IDC_GSE_PREVIEWMEMORY);
            context->PreviewHandles[3] = GetDlgItem(hwndDlg, IDC_GSE_PREVIEWICON);
            context->PreviewHandles[4] = GetDlgItem(hwndDlg, IDC_GSE_PREVIEWCELL);
            context->SampleTime = PH_GS_SAMPLE_COUNT;

            for (ULONG i = 0; i < PH_GSE_PREVIEW_COUNT; i++)
                Graph_SetTooltip(context->PreviewHandles[i], FALSE);

            for (ULONG i = 0; i < RTL_NUMBER_OF(PhpGseFillNames); i++)
                ComboBox_AddString(fillHandle, PhpGseFillNames[i]);

            SendDlgItemMessage(hwndDlg, IDC_GSE_OPACITY, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));

            PhSetListViewStyle(context->SeriesListHandle, FALSE, TRUE);
            PhAddListViewColumn(context->SeriesListHandle, 0, 0, 0, LVCFMT_LEFT, 200, L"Series");

            for (ULONG i = 0; i < PhGraphSeriesMaximum; i++)
                PhAddListViewItem(context->SeriesListHandle, MAXINT, (PWSTR)PhpGseSeriesNames[i], NULL);

            ListView_SetColumnWidth(context->SeriesListHandle, 0, LVSCW_AUTOSIZE_USEHEADER);

            PhSetDialogItemText(hwndDlg, IDC_GSE_NAME, context->Style.Name->Buffer);
            PhpGseLoadBase(context);
            PhpGseLoad(context);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
            SetTimer(hwndDlg, PH_GS_PREVIEW_TIMER_ID, PH_GS_PREVIEW_INTERVAL, NULL);
        }
        break;
    case WM_DESTROY:
        {
            KillTimer(hwndDlg, PH_GS_PREVIEW_TIMER_ID);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_TIMER:
        {
            if (wParam == PH_GS_PREVIEW_TIMER_ID)
            {
                context->SampleTime++;

                for (ULONG i = 0; i < PH_GSE_PREVIEW_COUNT; i++)
                    Graph_Update(context->PreviewHandles[i]);
            }
        }
        break;
    case WM_HSCROLL:
        {
            if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_GSE_OPACITY))
            {
                context->Style.TextBoxOpacity = (UCHAR)SendDlgItemMessage(hwndDlg, IDC_GSE_OPACITY, TBM_GETPOS, 0, 0);
                PhpGseLoad(context);
            }
        }
        break;
    case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT drawItem = (LPDRAWITEMSTRUCT)lParam;

            if (drawItem->hwndItem == context->SeriesListHandle)
            {
                PhpGseDrawSeriesRow(context, drawItem);
                return TRUE;
            }

            return PhpGseDrawColorButton(context, drawItem);
        }
    case WM_COMMAND:
        {
            ULONG id = GET_WM_COMMAND_ID(wParam, lParam);
            ULONG code = GET_WM_COMMAND_CMD(wParam, lParam);
            BOOLEAN changed = TRUE;

            switch (id)
            {
            case IDOK:
                if (PhpGseSave(context))
                    EndDialog(hwndDlg, IDOK);
                return TRUE;
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                return TRUE;
            case IDC_GSE_RESET:
                PhpGseReset(context);
                break;
            case IDC_GSE_SMOOTH:
            case IDC_GSE_CLASSIC:
                context->Style.Smooth = Button_GetCheck(GetDlgItem(hwndDlg, IDC_GSE_SMOOTH)) == BST_CHECKED;
                break;
            case IDC_GSE_MOTION:
                context->Style.Motion = Button_GetCheck(GetDlgItem(hwndDlg, IDC_GSE_MOTION)) == BST_CHECKED;
                break;
            case IDC_GSE_FILL:
                {
                    INT index;

                    if (code != CBN_SELCHANGE)
                        return FALSE;

                    index = ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_GSE_FILL));

                    if (index >= 0 && index < RTL_NUMBER_OF(PhpGseFillOrder))
                        context->Style.Fill = PhpGseFillOrder[index];
                }
                break;
            case IDC_GSE_SHOWGRID:
                context->Style.ShowGrid = Button_GetCheck(GetDlgItem(hwndDlg, IDC_GSE_SHOWGRID)) == BST_CHECKED;
                break;
            case IDC_GSE_SHOWTEXT:
                context->Style.ShowText = Button_GetCheck(GetDlgItem(hwndDlg, IDC_GSE_SHOWTEXT)) == BST_CHECKED;
                break;
            case IDC_GSE_ICONTRANSPARENT:
                context->Style.IconBackColor = Button_GetCheck(GetDlgItem(hwndDlg, IDC_GSE_ICONTRANSPARENT)) == BST_CHECKED ?
                    PH_GRAPH_COLOR_TRANSPARENT : PH_GRAPH_COLOR_AUTOMATIC;
                break;
            case IDC_GSE_CELLTRANSPARENT:
                context->Style.CellBackColor = Button_GetCheck(GetDlgItem(hwndDlg, IDC_GSE_CELLTRANSPARENT)) == BST_CHECKED ?
                    PH_GRAPH_COLOR_TRANSPARENT : PH_GRAPH_COLOR_AUTOMATIC;
                break;
            default:
                {
                    changed = FALSE;

                    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpGseColorRows); i++)
                    {
                        CONST PH_GSE_COLOR_ROW* row = &PhpGseColorRows[i];
                        COLORREF color;

                        if (id == row->AutoId)
                        {
                            // Unchecking Automatic starts from the color it resolved to.
                            if (Button_GetCheck(GetDlgItem(hwndDlg, row->AutoId)) == BST_CHECKED)
                                *PhpGseField(context, row) = PH_GRAPH_COLOR_AUTOMATIC;
                            else
                                *PhpGseField(context, row) = PhpGseShownColor(context, row);

                            changed = TRUE;
                        }
                        else if (id == row->ButtonId && code == BN_CLICKED)
                        {
                            // Picking a color turns Automatic off.
                            if (PhpGseChooseColor(context, PhpGseShownColor(context, row), &color))
                                *PhpGseField(context, row) = color;

                            changed = TRUE;
                        }
                    }
                }
                break;
            }

            if (changed)
            {
                PhpGseLoad(context);
                return TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->hwndFrom == context->SeriesListHandle)
            {
                // One click picks a color; Enter or Space picks for the selected row.
                if (header->code == NM_CLICK)
                {
                    PhpGseChooseSeriesColor(context, ((LPNMITEMACTIVATE)lParam)->iItem);
                }
                else if (header->code == LVN_KEYDOWN)
                {
                    WORD key = ((LPNMLVKEYDOWN)lParam)->wVKey;

                    if (key == VK_RETURN || key == VK_SPACE)
                        PhpGseChooseSeriesColor(context, ListView_GetNextItem(context->SeriesListHandle, INT_ERROR, LVNI_SELECTED));
                }

                break;
            }

            if (header->code == GCN_GETDRAWINFO)
            {
                PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)header;

                for (ULONG i = 0; i < PH_GSE_PREVIEW_COUNT; i++)
                {
                    if (header->hwndFrom == context->PreviewHandles[i])
                    {
                        PhpGsFillPreviewDrawInfo(
                            &context->Style,
                            i == 3 ? PhGraphSurfaceIcon : i == 4 ? PhGraphSurfaceCell : PhGraphSurfacePanel,
                            i < 3 ? i : 0,
                            context->SampleTime,
                            header->hwndFrom,
                            PhGetWindowDpi(hwndDlg),
                            context->Data1[i],
                            context->Data2[i],
                            getDrawInfo->DrawInfo
                            );
                        break;
                    }
                }
            }
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

BOOLEAN PhGsShowStyleEditor(
    _In_ HWND ParentWindow,
    _Inout_ PPH_GRAPH_STYLE Style,
    _In_opt_ PPH_LIST CustomStyles
    )
{
    PPH_GSE_CONTEXT context;
    BOOLEAN saved;

    context = PhAllocateZero(sizeof(PH_GSE_CONTEXT));
    PhGsCopyStyle(&context->Style, Style);
    context->CustomStyles = CustomStyles;

    saved = PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_GRAPHSTYLEEDIT),
        ParentWindow,
        PhpGseDlgProc,
        context
        ) == IDOK;

    if (saved)
    {
        PhGsDeleteStyle(Style);
        *Style = context->Style; // takes the copy's references
    }
    else
    {
        PhGsDeleteStyle(&context->Style);
    }

    PhFree(context);

    return saved;
}

INT_PTR CALLBACK PhGsOptionsPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_GS_PAGE_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhpGsCopyStore();
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
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_GRAPHSTYLELIST);
            context->PreviewHandles[0] = GetDlgItem(hwndDlg, IDC_GRAPHSTYLEPREVIEWCPU);
            context->PreviewHandles[1] = GetDlgItem(hwndDlg, IDC_GRAPHSTYLEPREVIEWIO);
            context->PreviewHandles[2] = GetDlgItem(hwndDlg, IDC_GRAPHSTYLEPREVIEWMEMORY);
            context->SampleTime = PH_GS_SAMPLE_COUNT;

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_SHOWCOMMITINSUMMARY), PhGetIntegerSetting(SETTING_SHOW_COMMIT_IN_SUMMARY) ? BST_CHECKED : BST_UNCHECKED);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 120, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 64, L"Used for");
            PhSetExtendedListView(context->ListViewHandle);
            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhAddListViewGroup(context->ListViewHandle, PH_GS_GROUP_BUILTIN, L"Built-in");
            PhAddListViewGroup(context->ListViewHandle, PH_GS_GROUP_CUSTOM, L"Custom");

            for (ULONG i = 0; i < PH_GS_PREVIEW_COUNT; i++)
                Graph_SetTooltip(context->PreviewHandles[i], FALSE);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_GRAPHSTYLESTATUS), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            for (ULONG i = 0; i < PH_GS_PREVIEW_COUNT; i++)
                PhAddLayoutItem(&context->LayoutManager, context->PreviewHandles[i], NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            for (ULONG id = IDC_GRAPHSTYLEUSELIGHT; id <= IDC_GRAPHSTYLEDELETE; id++)
                PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, id), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SHOWCOMMITINSUMMARY), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);

            PhpGsSelect(context, PhpGsAssignedStyle(context, PhGsIsDarkMode()));
            SetTimer(hwndDlg, PH_GS_PREVIEW_TIMER_ID, PH_GS_PREVIEW_INTERVAL, NULL);
        }
        break;
    case WM_DESTROY:
        {
            KillTimer(hwndDlg, PH_GS_PREVIEW_TIMER_ID);
            PhSetIntegerSetting(SETTING_SHOW_COMMIT_IN_SUMMARY, Button_GetCheck(GetDlgItem(hwndDlg, IDC_SHOWCOMMITINSUMMARY)) == BST_CHECKED);

            PhGsSetCustomStyles(context->Styles, &context->LightId->sr, &context->DarkId->sr);

            PhDeleteLayoutManager(&context->LayoutManager);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhDereferenceObject(context->LightId);
            PhDereferenceObject(context->DarkId);
            PhFree(context);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, PhGetWindowDpi(hwndDlg));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
            ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_PH_GS_REFRESH:
        {
            if (context->Selected)
                PhpGsSelect(context, context->Selected);
        }
        break;
    case WM_PH_GS_RENAME:
        {
            PhpGsBeginRename(context);
        }
        break;
    case WM_TIMER:
        {
            if (wParam == PH_GS_PREVIEW_TIMER_ID && IsWindowVisible(hwndDlg))
            {
                context->SampleTime++;

                for (ULONG i = 0; i < PH_GS_PREVIEW_COUNT; i++)
                    Graph_Update(context->PreviewHandles[i]);
            }
        }
        break;
    case WM_COMMAND:
        {
            if (!context->Selected)
                break;

            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_GRAPHSTYLEUSELIGHT:
                PhpGsAssign(context, FALSE);
                break;
            case IDC_GRAPHSTYLEUSEDARK:
                PhpGsAssign(context, TRUE);
                break;
            case IDC_GRAPHSTYLEEDIT:
                PhpGsEdit(context);
                break;
            case IDC_GRAPHSTYLEDUPLICATE:
                PhpGsDuplicate(context);
                break;
            case IDC_GRAPHSTYLERENAME:
                PhpGsBeginRename(context);
                break;
            case IDC_GRAPHSTYLEDELETE:
                PhpGsDelete(context);
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case LVN_ITEMCHANGED:
                {
                    LPNMLISTVIEW listView = (LPNMLISTVIEW)lParam;

                    if (
                        header->hwndFrom == context->ListViewHandle &&
                        FlagOn(listView->uNewState, LVIS_SELECTED) &&
                        !FlagOn(listView->uOldState, LVIS_SELECTED) &&
                        (PPH_GRAPH_STYLE)listView->lParam != context->Selected
                        )
                    {
                        context->Selected = (PPH_GRAPH_STYLE)listView->lParam;
                        PhpGsUpdateSelection(context);
                    }
                }
                break;
            case LVN_BEGINLABELEDIT:
                {
                    NMLVDISPINFO* dispInfo = (NMLVDISPINFO*)lParam;
                    PPH_GRAPH_STYLE style = (PPH_GRAPH_STYLE)dispInfo->item.lParam;

                    if (header->hwndFrom == context->ListViewHandle)
                    {
                        // Built-in styles can't be renamed.
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, !style || style->BuiltIn);
                        return TRUE;
                    }
                }
                break;
            case LVN_ENDLABELEDIT:
                {
                    if (header->hwndFrom == context->ListViewHandle)
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PhpGsEndRename(context, (NMLVDISPINFO*)lParam));
                        return TRUE;
                    }
                }
                break;
            case GCN_GETDRAWINFO:
                {
                    PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)header;

                    if (!context->Selected)
                        break;

                    for (ULONG i = 0; i < PH_GS_PREVIEW_COUNT; i++)
                    {
                        if (header->hwndFrom == context->PreviewHandles[i])
                        {
                            PhpGsFillPreviewDrawInfo(
                                context->Selected,
                                PhGraphSurfacePanel,
                                i,
                                context->SampleTime,
                                header->hwndFrom,
                                PhGetWindowDpi(hwndDlg),
                                context->Data1[i],
                                context->Data2[i],
                                getDrawInfo->DrawInfo
                                );
                            break;
                        }
                    }
                }
                break;
            }
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
