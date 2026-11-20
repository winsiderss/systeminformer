/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 */

#include <phapp.h>
#include <phsettings.h>
#include <settings.h>
#include <guisup.h>
#include <graph.h>
#include <graphstyles.h>

EXTERN_C BOOLEAN PhNfTransparencyEnabled;

// Guards the store; tray icons resolve colors on their own thread.
static PH_QUEUED_LOCK PhpGsLock = PH_QUEUED_LOCK_INIT;
static PPH_LIST PhpGsCustomStyles = NULL;
static PPH_STRING PhpGsLightId = NULL;
static PPH_STRING PhpGsDarkId = NULL;

static PPH_GRAPH_STYLE PhpGsActiveStyle = NULL; // NULL until applied
static BOOLEAN PhpGsActiveDarkMode = FALSE;
static BOOLEAN PhpGsActiveIconsTransparent = FALSE;
static COLORREF PhpGsActiveBackColor = 0;
static COLORREF PhpGsActiveTextColor = 0;
static PH_GRAPH_STYLE_COLORS PhpGsActiveColors[3]; // per PH_GRAPH_SURFACE
static BOOLEAN PhpGsActiveShowGrid = TRUE;
static BOOLEAN PhpGsSnapshotValid = FALSE; // PhpGsActiveColors has been filled once
static ULONG PhpGsUiThreadId = 0;

static CONST PCWSTR PhpGsSeriesSettings[PhGraphSeriesMaximum] =
{
    SETTING_COLOR_CPU_KERNEL,
    SETTING_COLOR_CPU_USER,
    SETTING_COLOR_IO_READ_OTHER,
    SETTING_COLOR_IO_WRITE,
    SETTING_COLOR_PRIVATE,
    SETTING_COLOR_PHYSICAL,
    SETTING_COLOR_POWER_USAGE,
    SETTING_COLOR_TEMPERATURE,
    SETTING_COLOR_FAN_RPM,
};

static ULONG* CONST PhpGsSeriesCache[PhGraphSeriesMaximum] =
{
    &PhCsColorCpuKernel,
    &PhCsColorCpuUser,
    &PhCsColorIoReadOther,
    &PhCsColorIoWrite,
    &PhCsColorPrivate,
    &PhCsColorPhysical,
    &PhCsColorPowerUsage,
    &PhCsColorTemperature,
    &PhCsColorFanRpm,
};

static PPH_GRAPH_STYLE PhpGsFindCustomStyle(
    _In_ PPH_LIST Styles,
    _In_ PPH_STRING Id
    )
{
    for (ULONG i = 0; i < Styles->Count; i++)
    {
        PPH_GRAPH_STYLE style = Styles->Items[i];

        if (PhEqualString(style->Id, Id, TRUE))
            return style;
    }

    return NULL;
}

static ULONG64 PhpGsNextCustomNumber(
    _In_ PPH_LIST Styles
    )
{
    static CONST PH_STRINGREF customPrefix = PH_STRINGREF_INIT(L"custom-");
    ULONG64 highest = 0;

    for (ULONG i = 0; i < Styles->Count; i++)
    {
        PPH_GRAPH_STYLE style = Styles->Items[i];
        PH_STRINGREF number;
        ULONG64 value;

        if (!PhStartsWithStringRef(&style->Id->sr, &customPrefix, TRUE))
            continue;

        number = style->Id->sr;
        PhSkipStringRef(&number, customPrefix.Length);

        if (PhStringToUInt64(&number, 10, &value) && value > highest)
            highest = value;
    }

    return highest + 1;
}

// Makes the name unique among Styles by appending " (2)", " (3)", ...
static VOID PhpGsMakeNameUnique(
    _In_ PPH_LIST Styles,
    _Inout_ PPH_GRAPH_STYLE Style
    )
{
    PPH_STRING baseName = PhReferenceObject(Style->Name);

    for (ULONG n = 2; !PhGsValidateName(Styles, &Style->Name->sr, NULL); n++)
        PhMoveReference(&Style->Name, PhFormatString(L"%s (%lu)", baseName->Buffer, n));

    PhDereferenceObject(baseName);
}

static VOID PhpGsRemapId(
    _Inout_ PPH_STRING* Id,
    _In_ PPH_STRING OldId,
    _In_ PPH_STRING NewId
    )
{
    if (PhEqualString(*Id, OldId, TRUE))
        PhSwapReference(Id, NewId);
}

// Adds a migrated style, renumbering its id when it clashes with an existing custom style.
static VOID PhpGsAddMigratedStyle(
    _In_ PPH_LIST Styles,
    _In_ PPH_GRAPH_STYLE Style
    )
{
    if (PhpGsFindCustomStyle(Styles, Style->Id))
        PhMoveReference(&Style->Id, PhFormatString(L"custom-%I64u", PhpGsNextCustomNumber(Styles)));

    PhpGsMakeNameUnique(Styles, Style);
    PhAddItemList(Styles, Style);
}

// Turns off text in the assigned styles, copying a built-in into a custom style first.
static VOID PhpGsMigrateShowText(
    _In_ PPH_LIST Styles,
    _Inout_ PPH_STRING* LightId,
    _Inout_ PPH_STRING* DarkId
    )
{
    for (ULONG i = 0; i < 2; i++)
    {
        PPH_STRING* id = i ? DarkId : LightId;
        PPH_GRAPH_STYLE style = PhGsFindStyle(Styles, &(*id)->sr, !!i);

        if (!style->ShowText)
            continue;

        if (style->BuiltIn)
        {
            PPH_GRAPH_STYLE builtIn = style;

            style = PhGsDuplicateStyle(builtIn, Styles);
            PhMoveReference(&style->Name, PhConcatStringRefZ(&builtIn->Name->sr, L" (no text)"));
            PhpGsMakeNameUnique(Styles, style);
            PhAddItemList(Styles, style);

            // A built-in serving both modes becomes one custom style serving both.
            PhpGsRemapId(LightId, builtIn->Id, style->Id);
            PhpGsRemapId(DarkId, builtIn->Id, style->Id);
        }

        style->ShowText = FALSE;
    }
}

static VOID PhpGsSaveSettings(
    VOID
    )
{
    PPH_STRING json;

    if (json = PhGsFormatStyles(PhpGsCustomStyles))
    {
        PhSetStringSetting2(SETTING_GRAPH_STYLES, &json->sr);
        PhDereferenceObject(json);
    }

    PhSetStringSetting2(SETTING_GRAPH_STYLE_LIGHT, &PhpGsLightId->sr);
    PhSetStringSetting2(SETTING_GRAPH_STYLE_DARK, &PhpGsDarkId->sr);
}

VOID PhGsInitialize(
    VOID
    )
{
    PPH_STRING json;
    PPH_LIST styles;

    PhpGsUiThreadId = HandleToUlong(NtCurrentThreadId());
    json = PhGetStringSetting(SETTING_GRAPH_STYLES);

    if (PhIsNullOrEmptyString(json) || !NT_SUCCESS(PhGsParseStyles(&json->sr, &styles)))
        styles = PhCreateList(4);

    PhClearReference(&json);

    PhpGsCustomStyles = styles;
    PhpGsLightId = PhGetStringSetting(SETTING_GRAPH_STYLE_LIGHT);
    PhpGsDarkId = PhGetStringSetting(SETTING_GRAPH_STYLE_DARK);

    // Migration runs once: only while neither mode has an assignment.
    if (PhIsNullOrEmptyString(PhpGsLightId) && PhIsNullOrEmptyString(PhpGsDarkId))
    {
        PH_GRAPH_STYLE_MIGRATION migration;
        COLORREF series[PhGraphSeriesMaximum];

        for (ULONG i = 0; i < PhGraphSeriesMaximum; i++)
            series[i] = PhGetIntegerSetting(PhpGsSeriesSettings[i]);

        PhGsMigrateLegacy(
            PhGetIntegerSetting(SETTING_GRAPH_COLOR_MODE),
            !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT),
            series,
            &migration
            );

        PhSwapReference(&PhpGsLightId, migration.LightId);
        PhSwapReference(&PhpGsDarkId, migration.DarkId);

        {
            PPH_GRAPH_STYLE lightStyle = NULL;
            PPH_GRAPH_STYLE darkStyle = NULL;

            // Assignments follow the style, not its id, which may be renumbered.
            for (ULONG i = 0; i < migration.NewStyles->Count; i++)
            {
                PPH_GRAPH_STYLE style = migration.NewStyles->Items[i];

                if (PhEqualString(style->Id, PhpGsLightId, TRUE))
                    lightStyle = style;
                if (PhEqualString(style->Id, PhpGsDarkId, TRUE))
                    darkStyle = style;
            }

            for (ULONG i = 0; i < migration.NewStyles->Count; i++)
                PhpGsAddMigratedStyle(styles, migration.NewStyles->Items[i]);

            if (lightStyle)
                PhSwapReference(&PhpGsLightId, lightStyle->Id);
            if (darkStyle)
                PhSwapReference(&PhpGsDarkId, darkStyle->Id);
        }

        PhClearList(migration.NewStyles); // the styles now belong to the custom list
        PhGsDeleteMigration(&migration);

        if (!PhGetIntegerSetting(SETTING_GRAPH_SHOW_TEXT))
            PhpGsMigrateShowText(styles, &PhpGsLightId, &PhpGsDarkId);

        PhpGsSaveSettings();
    }

    PhpGsActiveStyle = NULL;
}

BOOLEAN PhGsIsDarkMode(
    VOID
    )
{
    COLORREF backColor = PhThemeWindowBackgroundColor;

    return PhEnableThemeSupport && GetRValue(backColor) * 299 + GetGValue(backColor) * 587 + GetBValue(backColor) * 114 < 128 * 1000;
}

VOID PhGsGetToneThemeColors(
    _In_ BOOLEAN DarkTone,
    _Out_ COLORREF* BackColor,
    _Out_ COLORREF* TextColor
    )
{
    // A dark style follows a dark app theme; otherwise it uses the dark theme's own colors.
    if (DarkTone && !PhGsIsDarkMode())
    {
        *BackColor = RGB(18, 18, 18);
        *TextColor = RGB(255, 255, 255);
    }
    else
    {
        *BackColor = PhThemeWindowBackgroundColor;
        *TextColor = PhThemeWindowTextColor;
    }
}

static VOID PhpGsApplyStyle(
    _In_ PPH_GRAPH_STYLE Style
    )
{
    COLORREF themeBackColor;
    COLORREF themeTextColor;

    PhGraphRenderOptions.Smooth = Style->Smooth;
    PhGraphRenderOptions.Motion = Style->Motion;
    PhGraphRenderOptions.Fill = Style->Fill;
    PhGraphRenderOptions.TextBoxAlpha = (UCHAR)(Style->TextBoxOpacity * 255 / 100);
    PhCsGraphShowText = Style->ShowText;

    // Plugins read GraphShowText.
    if (PhGetIntegerSetting(SETTING_GRAPH_SHOW_TEXT) != Style->ShowText)
        PhSetIntegerSetting(SETTING_GRAPH_SHOW_TEXT, Style->ShowText);

    // Automatic colors follow the tone of the style, not the mode it is used in.
    PhGsGetToneThemeColors(Style->Dark, &themeBackColor, &themeTextColor);

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpGsActiveColors); i++)
    {
        PhGsResolveColors(
            Style,
            (PH_GRAPH_SURFACE)i,
            Style->Dark,
            themeBackColor,
            themeTextColor,
            PhpGsActiveIconsTransparent,
            &PhpGsActiveColors[i]
            );
    }

    // Third-party plugins read the series colors from the Color* settings.
    for (ULONG i = 0; i < PhGraphSeriesMaximum; i++)
    {
        if (*PhpGsSeriesCache[i] != Style->SeriesColor[i] || PhGetIntegerSetting(PhpGsSeriesSettings[i]) != Style->SeriesColor[i])
        {
            PhSetIntegerSetting(PhpGsSeriesSettings[i], Style->SeriesColor[i]);
            *PhpGsSeriesCache[i] = Style->SeriesColor[i];
        }
    }
}

FORCEINLINE BOOLEAN PhpGsIsActiveStyleCurrent(
    _In_ BOOLEAN DarkMode,
    _In_ BOOLEAN IconsTransparent
    )
{
    return
        PhpGsActiveStyle &&
        PhpGsActiveDarkMode == DarkMode &&
        PhpGsActiveIconsTransparent == IconsTransparent &&
        PhpGsActiveBackColor == PhThemeWindowBackgroundColor &&
        PhpGsActiveTextColor == PhThemeWindowTextColor;
}

// Applies the current mode's style; UI thread only (it writes PhGraphRenderOptions and settings).
static VOID PhpGsEnsureActiveStyle(
    _In_ BOOLEAN DarkMode,
    _In_ BOOLEAN IconsTransparent
    )
{
    PhAcquireQueuedLockExclusive(&PhpGsLock);

    if (!PhpGsIsActiveStyleCurrent(DarkMode, IconsTransparent))
    {
        PPH_STRING id = DarkMode ? PhpGsDarkId : PhpGsLightId;

        PhpGsActiveStyle = PhGsFindStyle(PhpGsCustomStyles, id ? &id->sr : NULL, DarkMode);
        PhpGsActiveDarkMode = DarkMode;
        PhpGsActiveIconsTransparent = IconsTransparent;
        PhpGsActiveBackColor = PhThemeWindowBackgroundColor;
        PhpGsActiveTextColor = PhThemeWindowTextColor;
        PhpGsApplyStyle(PhpGsActiveStyle);
        PhpGsActiveShowGrid = PhpGsActiveStyle->ShowGrid;
        PhpGsSnapshotValid = TRUE;
    }

    PhReleaseQueuedLockExclusive(&PhpGsLock);
}

BOOLEAN PhGsGetStyleColors(
    _In_ PH_GRAPH_SURFACE Surface,
    _Out_ PPH_GRAPH_STYLE_COLORS Colors
    )
{
    BOOLEAN darkMode = PhGsIsDarkMode();
    BOOLEAN iconsTransparent = PhNfTransparencyEnabled;
    BOOLEAN current;
    BOOLEAN showGrid;

    if ((ULONG)Surface >= RTL_NUMBER_OF(PhpGsActiveColors))
        Surface = PhGraphSurfacePanel;

    PhAcquireQueuedLockShared(&PhpGsLock);
    current = PhpGsIsActiveStyleCurrent(darkMode, iconsTransparent);

    if (current || (PhpGsSnapshotValid && HandleToUlong(NtCurrentThreadId()) != PhpGsUiThreadId))
    {
        *Colors = PhpGsActiveColors[Surface];
        showGrid = PhpGsActiveShowGrid;
        PhReleaseQueuedLockShared(&PhpGsLock);
        return showGrid;
    }

    // Before the UI thread has applied a style, other threads resolve without applying.
    if (HandleToUlong(NtCurrentThreadId()) != PhpGsUiThreadId)
    {
        PPH_STRING id = darkMode ? PhpGsDarkId : PhpGsLightId;
        PPH_GRAPH_STYLE style = PhGsFindStyle(PhpGsCustomStyles, id ? &id->sr : NULL, darkMode);
        COLORREF themeBackColor;
        COLORREF themeTextColor;

        PhGsGetToneThemeColors(style->Dark, &themeBackColor, &themeTextColor);
        PhGsResolveColors(style, Surface, style->Dark, themeBackColor, themeTextColor, iconsTransparent, Colors);
        showGrid = style->ShowGrid;
        PhReleaseQueuedLockShared(&PhpGsLock);
        return showGrid;
    }

    PhReleaseQueuedLockShared(&PhpGsLock);

    PhpGsEnsureActiveStyle(darkMode, iconsTransparent);

    PhAcquireQueuedLockShared(&PhpGsLock);
    *Colors = PhpGsActiveColors[Surface];
    showGrid = PhpGsActiveShowGrid;
    PhReleaseQueuedLockShared(&PhpGsLock);

    return showGrid;
}

VOID PhGsInvalidateActiveStyle(
    VOID
    )
{
    PhAcquireQueuedLockExclusive(&PhpGsLock);
    PhpGsActiveStyle = NULL;
    PhReleaseQueuedLockExclusive(&PhpGsLock);
}

PPH_LIST PhGsGetCustomStyles(
    VOID
    )
{
    return PhpGsCustomStyles;
}

PPH_STRING PhGsGetAssignedStyleId(
    _In_ BOOLEAN DarkMode
    )
{
    return DarkMode ? PhpGsDarkId : PhpGsLightId;
}

VOID PhGsSetCustomStyles(
    _In_ PPH_LIST Styles,
    _In_ PCPH_STRINGREF LightId,
    _In_ PCPH_STRINGREF DarkId
    )
{
    PhAcquireQueuedLockExclusive(&PhpGsLock);

    PhpGsActiveStyle = NULL;

    if (PhpGsCustomStyles && PhpGsCustomStyles != Styles)
        PhGsFreeStyleList(PhpGsCustomStyles);

    PhpGsCustomStyles = Styles;
    PhMoveReference(&PhpGsLightId, PhCreateString2(LightId));
    PhMoveReference(&PhpGsDarkId, PhCreateString2(DarkId));

    PhpGsSaveSettings();

    PhReleaseQueuedLockExclusive(&PhpGsLock);
}
