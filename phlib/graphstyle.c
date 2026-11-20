/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 */

#include <ph.h>
#include <guisup.h>
#include <graphstyle.h>
#include <json.h>

#define PH_GRAPH_STYLE_BUILTIN_COUNT 7

static PH_GRAPH_STYLE PhpBuiltInStyles[PH_GRAPH_STYLE_BUILTIN_COUNT];

static CONST COLORREF PhpLegacySeriesColors[PhGraphSeriesMaximum] =
{
    0x00ff00, // CPU kernel
    0x0000ff, // CPU user
    0x00ffff, // I/O read+other
    0xff0077, // I/O write
    0x0077ff, // private bytes
    0xff8000, // physical memory
    0x00ff00, // power
    0x0000ff, // temperature
    0xff0077, // fan
};

static VOID PhpInitializeBuiltInStyle(
    _Out_ PPH_GRAPH_STYLE Style,
    _In_ PCWSTR Id,
    _In_ PCWSTR Name,
    _In_ BOOLEAN Dark,
    _In_ BOOLEAN Smooth,
    _In_ BOOLEAN Motion,
    _In_ PH_GRAPH_FILL Fill,
    _In_ UCHAR TextBoxOpacity
    )
{
    memset(Style, 0, sizeof(PH_GRAPH_STYLE));
    Style->Id = PhCreateString(Id);
    Style->Name = PhCreateString(Name);
    Style->BuiltIn = TRUE;
    Style->Dark = Dark;
    Style->Smooth = Smooth;
    Style->Motion = Motion;
    Style->ShowText = TRUE;
    Style->ShowGrid = TRUE;
    Style->Fill = (UCHAR)Fill;
    Style->TextBoxOpacity = TextBoxOpacity;
    Style->PanelBackColor = PH_GRAPH_COLOR_AUTOMATIC;
    Style->IconBackColor = PH_GRAPH_COLOR_AUTOMATIC;
    Style->CellBackColor = PH_GRAPH_COLOR_AUTOMATIC;
    Style->GridColor = PH_GRAPH_COLOR_AUTOMATIC;
    Style->TextColor = PH_GRAPH_COLOR_AUTOMATIC;
    Style->TextBoxColor = PH_GRAPH_COLOR_AUTOMATIC;
    Style->LabelYColor = PH_GRAPH_COLOR_AUTOMATIC;
    memcpy(Style->SeriesColor, PhpLegacySeriesColors, sizeof(PhpLegacySeriesColors));
}

static VOID PhpInitializeBuiltInStyles(
    VOID
    )
{
    PPH_GRAPH_STYLE style;

    style = &PhpBuiltInStyles[0];
    PhpInitializeBuiltInStyle(style, L"default-light", L"Default Light", FALSE, TRUE, TRUE, PhGraphFillGradient, 88);
    for (ULONG i = 0; i < PhGraphSeriesMaximum; i++)
        style->SeriesColor[i] = PhHalveColorBrightness(PhpLegacySeriesColors[i]);

    style = &PhpBuiltInStyles[1];
    PhpInitializeBuiltInStyle(style, L"default-dark", L"Default Dark", TRUE, TRUE, TRUE, PhGraphFillGradient, 88);

    style = &PhpBuiltInStyles[2];
    PhpInitializeBuiltInStyle(style, L"classic-light", L"Classic Light", FALSE, FALSE, FALSE, PhGraphFillSolid, 100);
    memcpy(style->SeriesColor, PhpBuiltInStyles[0].SeriesColor, sizeof(style->SeriesColor));

    style = &PhpBuiltInStyles[3];
    PhpInitializeBuiltInStyle(style, L"classic-dark", L"Classic Dark", TRUE, FALSE, FALSE, PhGraphFillSolid, 100);

    // Palette from PR #3108.
    style = &PhpBuiltInStyles[4];
    PhpInitializeBuiltInStyle(style, L"microsoft-light", L"Microsoft Light", FALSE, TRUE, FALSE, PhGraphFillTranslucent, 80);
    style->PanelBackColor = RGB(0xff, 0xff, 0xff);
    style->GridColor = RGB(0xd9, 0xeb, 0xf5);
    style->TextColor = RGB(0x00, 0x00, 0x00);
    style->TextBoxColor = RGB(0xff, 0xff, 0xff);
    style->SeriesColor[PhGraphSeriesCpuKernel] = RGB(0x11, 0x7d, 0xbb);
    style->SeriesColor[PhGraphSeriesCpuUser] = RGB(0x11, 0x7d, 0xbb);
    style->SeriesColor[PhGraphSeriesIoReadOther] = RGB(0x4d, 0xa6, 0x0c);
    style->SeriesColor[PhGraphSeriesIoWrite] = RGB(0x4d, 0xa6, 0x0c);
    style->SeriesColor[PhGraphSeriesPrivate] = RGB(0x8b, 0x12, 0xae);
    style->SeriesColor[PhGraphSeriesPhysical] = RGB(0x8b, 0x12, 0xae);

    style = &PhpBuiltInStyles[5];
    PhpInitializeBuiltInStyle(style, L"microsoft-dark", L"Microsoft Dark", TRUE, TRUE, FALSE, PhGraphFillTranslucent, 80);
    style->PanelBackColor = RGB(0x19, 0x19, 0x19);
    style->GridColor = RGB(0x2b, 0x3a, 0x44);
    style->TextColor = RGB(0xff, 0xff, 0xff);
    style->TextBoxColor = RGB(0x20, 0x20, 0x20);
    style->SeriesColor[PhGraphSeriesCpuKernel] = RGB(0x4c, 0xc2, 0xff);
    style->SeriesColor[PhGraphSeriesCpuUser] = RGB(0x4c, 0xc2, 0xff);
    style->SeriesColor[PhGraphSeriesIoReadOther] = RGB(0x6c, 0xcb, 0x5f);
    style->SeriesColor[PhGraphSeriesIoWrite] = RGB(0x6c, 0xcb, 0x5f);
    style->SeriesColor[PhGraphSeriesPrivate] = RGB(0xd6, 0x7a, 0xff);
    style->SeriesColor[PhGraphSeriesPhysical] = RGB(0xd6, 0x7a, 0xff);

    // The Process Hacker look: black background, green grid and text.
    style = &PhpBuiltInStyles[6];
    PhpInitializeBuiltInStyle(style, L"hacker", L"Hacker", TRUE, FALSE, FALSE, PhGraphFillSolid, 100);
    style->PanelBackColor = RGB(0x00, 0x00, 0x00);
    style->GridColor = RGB(0x00, 0x57, 0x00);
    style->TextColor = RGB(0x00, 0xff, 0x00);
    style->TextBoxColor = RGB(0x00, 0x22, 0x00);
    style->LabelYColor = RGB(0xd0, 0xa0, 0x70);
}

ULONG NTAPI PhGsGetBuiltInStyles(
    _Outptr_result_buffer_(return) PPH_GRAPH_STYLE *Styles
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PhpInitializeBuiltInStyles();
        PhEndInitOnce(&initOnce);
    }

    *Styles = PhpBuiltInStyles;
    return PH_GRAPH_STYLE_BUILTIN_COUNT;
}

VOID NTAPI PhGsCopyStyle(
    _Out_ PPH_GRAPH_STYLE Destination,
    _In_ PPH_GRAPH_STYLE Source
    )
{
    *Destination = *Source;

    if (Destination->Id)
        PhReferenceObject(Destination->Id);
    if (Destination->Name)
        PhReferenceObject(Destination->Name);
    if (Destination->BaseId)
        PhReferenceObject(Destination->BaseId);
}

VOID NTAPI PhGsDeleteStyle(
    _Inout_ PPH_GRAPH_STYLE Style
    )
{
    PhClearReference(&Style->Id);
    PhClearReference(&Style->Name);
    PhClearReference(&Style->BaseId);
}

static PPH_GRAPH_STYLE PhpFindStyleById(
    _In_opt_ PPH_LIST CustomStyles,
    _In_ PCPH_STRINGREF Id
    )
{
    PPH_GRAPH_STYLE styles;
    ULONG count;

    count = PhGsGetBuiltInStyles(&styles);

    for (ULONG i = 0; i < count; i++)
    {
        if (PhEqualStringRef(&styles[i].Id->sr, Id, TRUE))
            return &styles[i];
    }

    if (CustomStyles)
    {
        for (ULONG i = 0; i < CustomStyles->Count; i++)
        {
            PPH_GRAPH_STYLE style = CustomStyles->Items[i];

            if (style->Id && PhEqualStringRef(&style->Id->sr, Id, TRUE))
                return style;
        }
    }

    return NULL;
}

PPH_GRAPH_STYLE NTAPI PhGsFindStyle(
    _In_opt_ PPH_LIST CustomStyles,
    _In_opt_ PCPH_STRINGREF Id,
    _In_ BOOLEAN DarkMode
    )
{
    PPH_GRAPH_STYLE styles;
    PPH_GRAPH_STYLE style;

    if (Id && (style = PhpFindStyleById(CustomStyles, Id)))
        return style;

    PhGsGetBuiltInStyles(&styles);

    return DarkMode ? &styles[1] : &styles[0];
}

BOOLEAN NTAPI PhGsValidateName(
    _In_opt_ PPH_LIST CustomStyles,
    _In_ PCPH_STRINGREF Name,
    _In_opt_ PCPH_STRINGREF ExceptId
    )
{
    static CONST PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" \t");
    PPH_GRAPH_STYLE styles;
    PH_STRINGREF name;
    ULONG count;

    name = *Name;
    PhTrimStringRef(&name, &whitespace, 0);

    if (name.Length == 0)
        return FALSE;

    count = PhGsGetBuiltInStyles(&styles);

    for (ULONG i = 0; i < count; i++)
    {
        if (PhEqualStringRef(&styles[i].Name->sr, &name, TRUE))
            return FALSE;
    }

    if (CustomStyles)
    {
        for (ULONG i = 0; i < CustomStyles->Count; i++)
        {
            PPH_GRAPH_STYLE style = CustomStyles->Items[i];
            PH_STRINGREF otherName;

            if (ExceptId && style->Id && PhEqualStringRef(&style->Id->sr, ExceptId, TRUE))
                continue;
            if (!style->Name)
                continue;

            otherName = style->Name->sr;
            PhTrimStringRef(&otherName, &whitespace, 0);

            if (PhEqualStringRef(&otherName, &name, TRUE))
                return FALSE;
        }
    }

    return TRUE;
}

PPH_GRAPH_STYLE NTAPI PhGsDuplicateStyle(
    _In_ PPH_GRAPH_STYLE Source,
    _In_opt_ PPH_LIST CustomStyles
    )
{
    static CONST PH_STRINGREF customPrefix = PH_STRINGREF_INIT(L"custom-");
    PPH_GRAPH_STYLE style;
    ULONG64 highest = 0;
    PPH_STRING name;

    if (CustomStyles)
    {
        for (ULONG i = 0; i < CustomStyles->Count; i++)
        {
            PPH_GRAPH_STYLE other = CustomStyles->Items[i];
            PH_STRINGREF number;
            ULONG64 value;

            if (!other->Id || !PhStartsWithStringRef(&other->Id->sr, &customPrefix, TRUE))
                continue;

            number = other->Id->sr;
            PhSkipStringRef(&number, customPrefix.Length);

            if (PhStringToUInt64(&number, 10, &value) && value > highest)
                highest = value;
        }
    }

    name = PhConcatStringRefZ(&Source->Name->sr, L" (custom)");

    for (ULONG n = 2; !PhGsValidateName(CustomStyles, &name->sr, NULL); n++)
        PhMoveReference(&name, PhFormatString(L"%s (custom %lu)", Source->Name->Buffer, n));

    style = PhAllocate(sizeof(PH_GRAPH_STYLE));
    *style = *Source;
    style->Id = PhFormatString(L"custom-%I64u", highest + 1);
    style->Name = name;
    style->BuiltIn = FALSE;

    if (Source->BuiltIn)
        style->BaseId = PhReferenceObject(Source->Id);
    else if (Source->BaseId)
        style->BaseId = PhReferenceObject(Source->BaseId);
    else
        style->BaseId = NULL;

    return style;
}

COLORREF NTAPI PhGsMixColor(
    _In_ COLORREF Color1,
    _In_ COLORREF Color2,
    _In_ ULONG Weight
    )
{
    return RGB(
        (GetRValue(Color1) * (255 - Weight) + GetRValue(Color2) * Weight) / 255,
        (GetGValue(Color1) * (255 - Weight) + GetGValue(Color2) * Weight) / 255,
        (GetBValue(Color1) * (255 - Weight) + GetBValue(Color2) * Weight) / 255
        );
}

static BOOLEAN PhpIsDarkColor(
    _In_ COLORREF Color
    )
{
    return GetRValue(Color) * 299 + GetGValue(Color) * 587 + GetBValue(Color) * 114 < 128 * 1000;
}

static UCHAR PhpBrightenChannel(
    _In_ UCHAR Channel
    )
{
    ULONG value = Channel * 2 + 125;

    return (UCHAR)min(value, 255);
}

COLORREF NTAPI PhGsDeriveFillColor(
    _In_ COLORREF LineColor,
    _In_ COLORREF BackColor,
    _In_ BOOLEAN TransparentBack
    )
{
    if (TransparentBack || PhpIsDarkColor(BackColor))
        return PhHalveColorBrightness(LineColor);

    return RGB(
        PhpBrightenChannel(GetRValue(LineColor)),
        PhpBrightenChannel(GetGValue(LineColor)),
        PhpBrightenChannel(GetBValue(LineColor))
        );
}

static COLORREF PhpResolveColor(
    _In_ COLORREF StyleColor,
    _In_ COLORREF AutomaticColor
    )
{
    return StyleColor == PH_GRAPH_COLOR_AUTOMATIC ? AutomaticColor : StyleColor;
}

static ULONG PhpColorBrightness(
    _In_ COLORREF Color
    )
{
    return (GetRValue(Color) * 299 + GetGValue(Color) * 587 + GetBValue(Color) * 114) / 1000;
}

VOID NTAPI PhGsResolveColors(
    _In_ PPH_GRAPH_STYLE Style,
    _In_ PH_GRAPH_SURFACE Surface,
    _In_ BOOLEAN DarkMode,
    _In_ COLORREF ThemeBackColor,
    _In_ COLORREF ThemeTextColor,
    _In_ BOOLEAN IconsTransparent,
    _Out_ PPH_GRAPH_STYLE_COLORS Colors
    )
{
    COLORREF panelBackColor;
    COLORREF backColor;
    BOOLEAN smoothBox;

    Colors->TransparentBack = FALSE;
    panelBackColor = PhpResolveColor(Style->PanelBackColor, DarkMode ? ThemeBackColor : RGB(0xef, 0xef, 0xef));

    // Automatic icons and cells take the panel background.
    switch (Surface)
    {
    case PhGraphSurfaceIcon:
        backColor = Style->IconBackColor;

        if (backColor == PH_GRAPH_COLOR_AUTOMATIC)
            backColor = IconsTransparent ? PH_GRAPH_COLOR_TRANSPARENT : panelBackColor;
        break;
    case PhGraphSurfaceCell:
        backColor = PhpResolveColor(Style->CellBackColor, panelBackColor);
        break;
    default:
        backColor = panelBackColor;
        break;
    }

    if (backColor == PH_GRAPH_COLOR_TRANSPARENT)
    {
        Colors->TransparentBack = TRUE;
        Colors->BackColor = 0;
    }
    else
    {
        Colors->BackColor = backColor;
    }

    // Automatic smooth text boxes fill with the background.
    smoothBox = Style->Smooth && !Colors->TransparentBack;

    // A custom background drives the Automatic chrome by its own brightness.
    if (!Colors->TransparentBack && backColor != (DarkMode ? ThemeBackColor : RGB(0xef, 0xef, 0xef)))
    {
        BOOLEAN darkBack = PhpColorBrightness(backColor) < 128;
        COLORREF ink = darkBack ? RGB(0xff, 0xff, 0xff) : RGB(0x00, 0x00, 0x00);

        if (darkBack)
        {
            Colors->GridColor = PhpResolveColor(Style->GridColor, PhGsMixColor(backColor, ink, 28));
            Colors->TextColor = PhpResolveColor(Style->TextColor, PhGsMixColor(ink, backColor, 40));
            Colors->TextBoxColor = PhpResolveColor(Style->TextBoxColor, smoothBox ? backColor : PhGsMixColor(backColor, ink, 20));
            Colors->LabelYColor = PhpResolveColor(Style->LabelYColor, Style->Smooth ? PhGsMixColor(ink, backColor, 40) : RGB(0xd0, 0xa0, 0x70));
        }
        else
        {
            Colors->GridColor = PhpResolveColor(Style->GridColor, PhGsMixColor(backColor, ink, 42));
            Colors->TextColor = PhpResolveColor(Style->TextColor, ink);
            Colors->TextBoxColor = PhpResolveColor(Style->TextBoxColor, smoothBox ? backColor : PhGsMixColor(backColor, ink, 8));
            Colors->LabelYColor = PhpResolveColor(Style->LabelYColor, Style->Smooth ? PhGsMixColor(backColor, ink, 187) : RGB(0xa0, 0x60, 0x20));
        }

        return;
    }

    if (DarkMode)
    {
        Colors->GridColor = PhpResolveColor(Style->GridColor, PhGsMixColor(ThemeBackColor, ThemeTextColor, 28));
        Colors->TextColor = PhpResolveColor(Style->TextColor, PhGsMixColor(ThemeTextColor, ThemeBackColor, 40));
        Colors->TextBoxColor = PhpResolveColor(Style->TextBoxColor, smoothBox ? Colors->BackColor : PhGsMixColor(ThemeBackColor, ThemeTextColor, 20));
        Colors->LabelYColor = PhpResolveColor(Style->LabelYColor, Style->Smooth ? PhGsMixColor(ThemeTextColor, ThemeBackColor, 40) : RGB(0xd0, 0xa0, 0x70));
    }
    else
    {
        Colors->GridColor = PhpResolveColor(Style->GridColor, RGB(0xc7, 0xc7, 0xc7));
        Colors->TextColor = PhpResolveColor(Style->TextColor, RGB(0x00, 0x00, 0x00));
        Colors->TextBoxColor = PhpResolveColor(Style->TextBoxColor, smoothBox ? Colors->BackColor : RGB(0xe7, 0xe7, 0xe7));
        Colors->LabelYColor = PhpResolveColor(Style->LabelYColor, Style->Smooth ? RGB(0x40, 0x40, 0x40) : RGB(0xa0, 0x60, 0x20));
    }
}

static CONST PCSTR PhpGraphSeriesKeys[PhGraphSeriesMaximum] =
{
    "cpuKernel",
    "cpuUser",
    "ioReadOther",
    "ioWrite",
    "private",
    "physical",
    "power",
    "temperature",
    "fan",
};

static CONST PCSTR PhpGraphFillNames[PhGraphFillMaximum] =
{
    "gradient",
    "none",
    "translucent",
    "solid",
};

static CONST PH_STRINGREF PhpGraphFillNamesSr[PhGraphFillMaximum] =
{
    PH_STRINGREF_INIT(L"gradient"),
    PH_STRINGREF_INIT(L"none"),
    PH_STRINGREF_INIT(L"translucent"),
    PH_STRINGREF_INIT(L"solid"),
};

static VOID PhpAddJsonString(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PPH_STRING String
    )
{
    PPH_BYTES utf8;

    utf8 = PhConvertUtf16ToUtf8Ex(String->Buffer, String->Length);
    PhAddJsonObjectUtf8(Object, Key, utf8);
    PhDereferenceObject(utf8);
}

static VOID PhpAddJsonColor(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ COLORREF Color
    )
{
    static CONST CHAR hex[] = "0123456789abcdef";
    CHAR buffer[8];

    if (Color == PH_GRAPH_COLOR_AUTOMATIC)
    {
        PhAddJsonObject(Object, Key, "auto");
        return;
    }

    if (Color == PH_GRAPH_COLOR_TRANSPARENT)
    {
        PhAddJsonObject(Object, Key, "transparent");
        return;
    }

    buffer[0] = '#';
    buffer[1] = hex[GetRValue(Color) >> 4];
    buffer[2] = hex[GetRValue(Color) & 0xf];
    buffer[3] = hex[GetGValue(Color) >> 4];
    buffer[4] = hex[GetGValue(Color) & 0xf];
    buffer[5] = hex[GetBValue(Color) >> 4];
    buffer[6] = hex[GetBValue(Color) & 0xf];
    buffer[7] = ANSI_NULL;
    PhAddJsonObject(Object, Key, buffer);
}

PPH_STRING NTAPI PhGsFormatStyles(
    _In_ PPH_LIST CustomStyles
    )
{
    PVOID root;
    PVOID array;
    PPH_BYTES json;
    PPH_STRING string = NULL;

    root = PhCreateJsonObject();
    array = PhCreateJsonArray();
    PhAddJsonObjectInt64(root, "version", 1);

    for (ULONG i = 0; i < CustomStyles->Count; i++)
    {
        PPH_GRAPH_STYLE style = CustomStyles->Items[i];
        PVOID object;
        PVOID series;

        object = PhCreateJsonObject();
        PhpAddJsonString(object, "id", style->Id);
        PhpAddJsonString(object, "name", style->Name);
        if (style->BaseId)
            PhpAddJsonString(object, "base", style->BaseId);
        PhAddJsonObjectBoolean(object, "smooth", style->Smooth);
        PhAddJsonObjectBoolean(object, "motion", style->Motion);
        PhAddJsonObjectBoolean(object, "showText", style->ShowText);
        PhAddJsonObjectBoolean(object, "showGrid", style->ShowGrid);
        PhAddJsonObject(object, "fill", PhpGraphFillNames[style->Fill < PhGraphFillMaximum ? style->Fill : PhGraphFillGradient]);
        PhAddJsonObjectInt64(object, "textBoxOpacity", style->TextBoxOpacity);
        PhpAddJsonColor(object, "panelBack", style->PanelBackColor);
        PhpAddJsonColor(object, "iconBack", style->IconBackColor);
        PhpAddJsonColor(object, "cellBack", style->CellBackColor);
        PhpAddJsonColor(object, "gridColor", style->GridColor);
        PhpAddJsonColor(object, "textColor", style->TextColor);
        PhpAddJsonColor(object, "textBoxColor", style->TextBoxColor);
        PhpAddJsonColor(object, "labelColor", style->LabelYColor);

        series = PhCreateJsonObject();
        for (ULONG j = 0; j < PhGraphSeriesMaximum; j++)
            PhpAddJsonColor(series, PhpGraphSeriesKeys[j], style->SeriesColor[j]);
        PhAddJsonObjectValue(object, "series", series);

        PhAddJsonArrayObject(array, object);
    }

    PhAddJsonObjectValue(root, "styles", array);

    if (json = PhJsonObjectToJsonString(root, 0))
    {
        string = PhConvertUtf8ToUtf16Ex(json->Buffer, json->Length);
        PhDereferenceObject(json);
    }

    PhFreeJsonObject(root);

    return string;
}

static PVOID PhpGetJsonTyped(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PH_JSON_OBJECT_TYPE Type
    )
{
    PVOID value;

    if ((value = PhGetJsonObject(Object, Key)) && PhGetJsonObjectType(value) == Type)
        return value;

    return NULL;
}

static PPH_STRING PhpGetJsonString(
    _In_ PVOID Object,
    _In_ PCSTR Key
    )
{
    PVOID value;

    if (value = PhpGetJsonTyped(Object, Key, PH_JSON_OBJECT_TYPE_STRING))
        return PhGetJsonObjectString(value);

    return NULL;
}

static VOID PhpGetJsonBoolean(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _Inout_ PBOOLEAN Value
    )
{
    if (PhpGetJsonTyped(Object, Key, PH_JSON_OBJECT_TYPE_BOOLEAN))
        *Value = PhGetJsonObjectBool(Object, Key);
}

static LONG PhpHexDigit(
    _In_ WCHAR Character
    )
{
    if (Character >= L'0' && Character <= L'9')
        return Character - L'0';
    if (Character >= L'a' && Character <= L'f')
        return Character - L'a' + 10;
    if (Character >= L'A' && Character <= L'F')
        return Character - L'A' + 10;

    return -1;
}

static VOID PhpGetJsonColor(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ BOOLEAN AllowAutomatic,
    _In_ BOOLEAN AllowTransparent,
    _Inout_ COLORREF* Color
    )
{
    PPH_STRING string;
    UCHAR channels[3];

    if (!(string = PhpGetJsonString(Object, Key)))
        return;

    if (AllowAutomatic && PhEqualString2(string, L"auto", TRUE))
    {
        *Color = PH_GRAPH_COLOR_AUTOMATIC;
    }
    else if (AllowTransparent && PhEqualString2(string, L"transparent", TRUE))
    {
        *Color = PH_GRAPH_COLOR_TRANSPARENT;
    }
    else if (string->Length == 7 * sizeof(WCHAR) && string->Buffer[0] == L'#')
    {
        for (ULONG i = 0; i < 3; i++)
        {
            LONG high = PhpHexDigit(string->Buffer[1 + i * 2]);
            LONG low = PhpHexDigit(string->Buffer[2 + i * 2]);

            if (high < 0 || low < 0)
                goto CleanupExit;

            channels[i] = (UCHAR)(high * 16 + low);
        }

        *Color = RGB(channels[0], channels[1], channels[2]);
    }

CleanupExit:
    PhDereferenceObject(string);
}

static PPH_GRAPH_STYLE PhpParseStyle(
    _In_ PVOID Object,
    _In_ PPH_LIST Accepted
    )
{
    static CONST PH_STRINGREF customPrefix = PH_STRINGREF_INIT(L"custom-");
    static CONST PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" \t");
    PPH_GRAPH_STYLE style;
    PPH_GRAPH_STYLE base;
    PPH_STRING id;
    PPH_STRING string;
    PH_STRINGREF nameSr;
    PVOID value;

    if (PhGetJsonObjectType(Object) != PH_JSON_OBJECT_TYPE_OBJECT)
        return NULL;
    if (!(id = PhpGetJsonString(Object, "id")))
        return NULL;

    if (id->Length <= customPrefix.Length || !PhStartsWithStringRef(&id->sr, &customPrefix, TRUE))
    {
        PhDereferenceObject(id);
        return NULL;
    }

    for (ULONG i = 0; i < Accepted->Count; i++)
    {
        if (PhEqualString(((PPH_GRAPH_STYLE)Accepted->Items[i])->Id, id, TRUE))
        {
            PhDereferenceObject(id);
            return NULL;
        }
    }

    // Missing and invalid fields come from the base built-in.
    string = PhpGetJsonString(Object, "base");
    base = PhGsFindStyle(NULL, string ? &string->sr : NULL, TRUE);
    PhClearReference(&string);

    style = PhAllocate(sizeof(PH_GRAPH_STYLE));
    *style = *base;
    style->Id = id;
    style->Name = NULL;
    style->BaseId = PhReferenceObject(base->Id);
    style->BuiltIn = FALSE;

    PhpGetJsonBoolean(Object, "smooth", &style->Smooth);
    PhpGetJsonBoolean(Object, "motion", &style->Motion);
    PhpGetJsonBoolean(Object, "showText", &style->ShowText);
    PhpGetJsonBoolean(Object, "showGrid", &style->ShowGrid);

    if (string = PhpGetJsonString(Object, "fill"))
    {
        for (UCHAR i = 0; i < PhGraphFillMaximum; i++)
        {
            if (PhEqualStringRef(&string->sr, &PhpGraphFillNamesSr[i], TRUE))
            {
                style->Fill = i;
                break;
            }
        }

        PhDereferenceObject(string);
    }

    if (value = PhpGetJsonTyped(Object, "textBoxOpacity", PH_JSON_OBJECT_TYPE_INT))
    {
        LONGLONG opacity = PhGetJsonInt64Object(value);

        style->TextBoxOpacity = (UCHAR)(opacity < 0 ? 0 : opacity > 100 ? 100 : opacity);
    }

    PhpGetJsonColor(Object, "panelBack", TRUE, FALSE, &style->PanelBackColor);
    PhpGetJsonColor(Object, "iconBack", TRUE, TRUE, &style->IconBackColor);
    PhpGetJsonColor(Object, "cellBack", TRUE, TRUE, &style->CellBackColor);
    PhpGetJsonColor(Object, "gridColor", TRUE, FALSE, &style->GridColor);
    PhpGetJsonColor(Object, "textColor", TRUE, FALSE, &style->TextColor);
    PhpGetJsonColor(Object, "textBoxColor", TRUE, FALSE, &style->TextBoxColor);
    PhpGetJsonColor(Object, "labelColor", TRUE, FALSE, &style->LabelYColor);

    if (value = PhpGetJsonTyped(Object, "series", PH_JSON_OBJECT_TYPE_OBJECT))
    {
        for (ULONG i = 0; i < PhGraphSeriesMaximum; i++)
            PhpGetJsonColor(value, PhpGraphSeriesKeys[i], FALSE, FALSE, &style->SeriesColor[i]);
    }

    // An empty name takes the base name; a taken one gets " (2)", " (3)", ...
    string = PhpGetJsonString(Object, "name");
    nameSr = string ? string->sr : base->Name->sr;
    PhTrimStringRef(&nameSr, &whitespace, 0);

    if (nameSr.Length == 0)
        nameSr = base->Name->sr;

    style->Name = PhCreateString2(&nameSr);

    for (ULONG n = 2; !PhGsValidateName(Accepted, &style->Name->sr, NULL); n++)
        PhMoveReference(&style->Name, PhFormatString(L"%.*s (%lu)", (ULONG)(nameSr.Length / sizeof(WCHAR)), nameSr.Buffer, n));

    PhClearReference(&string);

    return style;
}

NTSTATUS NTAPI PhGsParseStyles(
    _In_ PCPH_STRINGREF Json,
    _Out_ PPH_LIST *CustomStyles
    )
{
    PPH_STRING json;
    PVOID root;
    PVOID array;
    PPH_LIST list;
    NTSTATUS status;

    json = PhCreateString2(Json);
    status = PhCreateJsonParserEx(&root, json, TRUE);
    PhDereferenceObject(json);

    if (!NT_SUCCESS(status))
        return STATUS_INVALID_PARAMETER;

    if (PhGetJsonObjectType(root) != PH_JSON_OBJECT_TYPE_OBJECT)
    {
        PhFreeJsonObject(root);
        return STATUS_INVALID_PARAMETER;
    }

    list = PhCreateList(4);

    if (array = PhpGetJsonTyped(root, "styles", PH_JSON_OBJECT_TYPE_ARRAY))
    {
        ULONG count = PhGetJsonArrayLength(array);

        for (ULONG i = 0; i < count; i++)
        {
            PPH_GRAPH_STYLE style;

            if (style = PhpParseStyle(PhGetJsonArrayIndexObject(array, i), list))
                PhAddItemList(list, style);
        }
    }

    PhFreeJsonObject(root);
    *CustomStyles = list;

    return STATUS_SUCCESS;
}

VOID NTAPI PhGsFreeStyleList(
    _In_ PPH_LIST CustomStyles
    )
{
    for (ULONG i = 0; i < CustomStyles->Count; i++)
    {
        PhGsDeleteStyle(CustomStyles->Items[i]);
        PhFree(CustomStyles->Items[i]);
    }

    PhDereferenceObject(CustomStyles);
}

static PPH_STRING PhpAddMigratedStyle(
    _In_ PPH_LIST NewStyles,
    _In_ PPH_GRAPH_STYLE Template,
    _In_ PCWSTR Name
    )
{
    PPH_GRAPH_STYLE style;

    style = PhGsDuplicateStyle(Template, NewStyles);
    PhMoveReference(&style->Name, PhCreateString(Name));
    PhAddItemList(NewStyles, style);

    return PhReferenceObject(style->Id);
}

VOID NTAPI PhGsMigrateLegacy(
    _In_ ULONG GraphColorMode,
    _In_ BOOLEAN ThemeSupport,
    _In_reads_(PhGraphSeriesMaximum) CONST COLORREF* LegacySeries,
    _Out_ PPH_GRAPH_STYLE_MIGRATION Result
    )
{
    PPH_GRAPH_STYLE styles;
    PH_GRAPH_STYLE light;
    PH_GRAPH_STYLE dark;
    PCWSTR lightName = NULL; // non-NULL when the light style must be a custom copy
    PCWSTR darkName = NULL;
    BOOLEAN shared = FALSE; // one style serves both modes

    NT_VERIFY(PhGsGetBuiltInStyles(&styles) == PH_GRAPH_STYLE_BUILTIN_COUNT);

    if (GraphColorMode == 1)
        GraphColorMode = ThemeSupport ? 0 : 3;
    else if (GraphColorMode > 4)
        GraphColorMode = 0;

    // Templates are shallow copies of built-ins; PhGsDuplicateStyle takes the built-in id as BaseId.
    switch (GraphColorMode)
    {
    case 2:
        light = styles[2];
        dark = styles[3];
        break;
    case 3:
        light = dark = styles[6];
        shared = TRUE;
        break;
    case 4:
        light = dark = styles[6];
        light.Smooth = TRUE;
        light.Motion = TRUE;
        light.Fill = PhGraphFillGradient;
        light.TextBoxOpacity = 88;
        light.LabelYColor = RGB(0xc8, 0xc8, 0xc8);
        lightName = L"Hacker (smooth)";
        shared = TRUE;
        break;
    default:
        light = styles[0];
        dark = styles[1];
        break;
    }

    if (memcmp(LegacySeries, PhpLegacySeriesColors, sizeof(PhpLegacySeriesColors)) != 0)
    {
        if (shared)
        {
            memcpy(light.SeriesColor, LegacySeries, sizeof(light.SeriesColor));
            lightName = L"My colors";
        }
        else
        {
            for (ULONG i = 0; i < PhGraphSeriesMaximum; i++)
                light.SeriesColor[i] = PhHalveColorBrightness(LegacySeries[i]);

            memcpy(dark.SeriesColor, LegacySeries, sizeof(dark.SeriesColor));
            lightName = L"My colors (light)";
            darkName = L"My colors (dark)";
        }
    }

    Result->NewStyles = PhCreateList(2);

    if (lightName)
        Result->LightId = PhpAddMigratedStyle(Result->NewStyles, &light, lightName);
    else
        Result->LightId = PhReferenceObject(light.Id);

    if (shared)
        Result->DarkId = PhReferenceObject(Result->LightId);
    else if (darkName)
        Result->DarkId = PhpAddMigratedStyle(Result->NewStyles, &dark, darkName);
    else
        Result->DarkId = PhReferenceObject(dark.Id);
}

VOID NTAPI PhGsDeleteMigration(
    _Inout_ PPH_GRAPH_STYLE_MIGRATION Result
    )
{
    if (Result->NewStyles)
    {
        PhGsFreeStyleList(Result->NewStyles);
        Result->NewStyles = NULL;
    }

    PhClearReference(&Result->LightId);
    PhClearReference(&Result->DarkId);
}
