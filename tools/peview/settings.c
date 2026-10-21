/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2026
 *
 */

#include <peview.h>

static PPH_STRING PvSettingsFileName = NULL;

VOID PvAddDefaultSettings(
    VOID
    )
{
    PhpAddIntegerSetting(L"FirstRun", L"1");
    PhpAddStringSetting(L"Font", L""); // null
    PhpAddStringSetting(L"DbgHelpSearchPath", L"SRV*C:\\Symbols*https://msdl.microsoft.com/download/symbols");
    PhpAddIntegerSetting(L"DbgHelpUndecorate", L"1");
    PhpAddIntegerSetting(L"EnableLegacyPropertiesDialog", L"0");
    PhpAddIntegerSetting(L"EnableSecurityAdvancedDialog", L"1");
    PhpAddIntegerSetting(L"EnableStreamerMode", L"0");
    PhpAddIntegerSetting(L"EnableThemeSupport", L"1");
    PhpAddIntegerSetting(L"FontQuality", L"6");
    PhpAddIntegerSetting(L"PvThemeMode", L"0"); // PvThemeModeAutomatic
    PhpAddIntegerSetting(L"EnableThemeAcrylicSupport", L"1");
    PhpAddIntegerSetting(L"EnableThemeAcrylicWindowSupport", L"0");
    PhpAddIntegerSetting(L"EnableThemeAnimation", L"1");
    PhpAddIntegerSetting(L"EnableThemeNativeButtons", L"0");
    PhpAddIntegerSetting(L"EnableWindowBorderColor", L"1");
    PhpAddIntegerSetting(L"ThemeWindowForegroundColor", L"1c1c1c"); // RGB(28, 28, 28)
    PhpAddIntegerSetting(L"ThemeWindowBackgroundColor", L"2b2b2b"); // RGB(43, 43, 43)
    PhpAddIntegerSetting(L"ThemeWindowBackground2Color", L"414141"); // RGB(65, 65, 65)
    PhpAddIntegerSetting(L"ThemeWindowHighlightColor", L"808080"); // RGB(128, 128, 128)
    PhpAddIntegerSetting(L"ThemeWindowHighlight2Color", L"8f8f8f"); // RGB(143, 143, 143)
    PhpAddIntegerSetting(L"ThemeWindowTextColor", L"ffffff"); // RGB(255, 255, 255)
    PhpAddIntegerSetting(L"EnableTreeListBorder", L"1");
    PhpAddIntegerSetting(L"EnableVersionSupport", L"0");
    PhpAddIntegerSetting(L"SearchControlRegex", L"0");
    PhpAddIntegerSetting(L"SearchControlCaseSensitive", L"0");
    PhpAddIntegerSetting(L"SearchControlFuzzy", L"0");
    PhpAddIntegerSetting(L"GraphColorMode", L"1");
    PhpAddIntegerSetting(L"HashAlgorithm", L"0");
    PhpAddIntegerSetting(L"MaxSizeUnit", L"6");
    PhpAddIntegerSetting(L"MainWindowPageRestoreEnabled", L"1");
    PhpAddIntegerSetting(L"HideInvalidExports", L"0");
    PhpAddStringSetting(L"MainWindowPage", L"General");
    PhpAddIntegerPairSetting(L"MainWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"MainWindowSize", L"@96|550,580");
    PhpAddIntegerSetting(L"MainWindowState", L"1");
    PhpAddIntegerPairSetting(L"StartWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"StartWindowSize", L"@96|560,320");
    PhpAddStringSetting(L"StartWindowListViewColumns", L"");
    PhpAddStringSetting(L"RecentFiles", L"");
    PhpAddIntegerSetting(L"PeViewSidebarWidth", L"145");
    PhpAddStringSetting(L"ImageGeneralPropertiesListViewColumns", L"");
    PhpAddStringSetting(L"ImageGeneralPropertiesListViewSort", L"");
    PhpAddStringSetting(L"ImageGeneralPropertiesListViewGroupStates", L"");
    PhpAddStringSetting(L"ImageDirectoryTreeListColumns", L"");
    PhpAddStringSetting(L"ImageDirectoryTreeListSort", L"0,1"); // 0, AscendingSortOrder
    PhpAddStringSetting(L"ImageExportTreeListColumns", L"");
    PhpAddStringSetting(L"ImageExportTreeListSort", L"0,1"); // 0, AscendingSortOrder
    PhpAddStringSetting(L"ImageImportTreeListColumns", L"");
    PhpAddStringSetting(L"ImageImportTreeListSort", L"0,1"); // 0, AscendingSortOrder
    PhpAddStringSetting(L"ImageSectionsTreeListColumns", L"");
    PhpAddStringSetting(L"ImageSectionsTreeListSort", L"0,1"); // 0, AscendingSortOrder
    PhpAddIntegerSetting(L"ImageSectionsTreeListFlags", L"0");
    PhpAddStringSetting(L"ImageResourcesTreeListColumns", L"");
    PhpAddStringSetting(L"ImageResourcesTreeListSort", L"0,1"); // 0, AscendingSortOrder
    PhpAddStringSetting(L"ImageLoadCfgListViewColumns", L"");
    PhpAddStringSetting(L"ImageLoadCfgListViewGroupStates", L"");
    PhpAddStringSetting(L"ImageExceptionsIa32ListViewColumns", L"");
    PhpAddStringSetting(L"ImageExceptionsAmd64ListViewColumns", L"");
    PhpAddStringSetting(L"ImageExceptionsArm64ListViewColumns", L"");
    PhpAddStringSetting(L"ImageHeadersListViewColumns", L"");
    PhpAddStringSetting(L"ImageHeadersListViewGroupStates", L"");
    PhpAddStringSetting(L"ImageLayoutTreeColumns", L"");
    PhpAddStringSetting(L"ImageCfgListViewColumns", L"");
    PhpAddStringSetting(L"ImageClrListViewColumns", L"");
    PhpAddStringSetting(L"ImageClrImportsListViewColumns", L"");
    PhpAddStringSetting(L"ImageClrTablesListViewColumns", L"");
    PhpAddStringSetting(L"ImageClrTablePreviewListViewColumns", L"");
    PhpAddStringSetting(L"ImageAttributesListViewColumns", L"");
    PhpAddStringSetting(L"ImagePropertiesListViewColumns", L"");
    PhpAddStringSetting(L"ImageRelocationsListViewColumns", L"");
    PhpAddStringSetting(L"ImageDynamicRelocationsListViewColumns", L"");
    PhpAddStringSetting(L"ImageDynamicRelocationsTreeColumns", L"");
    PhpAddStringSetting(L"ImageMuiListViewColumns", L"");
    PhpAddStringSetting(L"ImageSecurityListViewColumns", L"");
    PhpAddStringSetting(L"ImageSecurityListViewSort", L"");
    PhpAddStringSetting(L"ImageSecurityTreeColumns", L"");
    PhpAddStringSetting(L"ImageSecurityCertColumns", L"");
    PhpAddIntegerPairSetting(L"ImageSecurityCertWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"ImageSecurityCertWindowSize", L"@96|0,0");
    PhpAddStringSetting(L"ImageMappingsListViewColumns", L"");
    PhpAddStringSetting(L"ImageStreamsListViewColumns", L"");
    PhpAddStringSetting(L"ImageHardLinksListViewColumns", L"");
    PhpAddStringSetting(L"ImageHashesListViewColumns", L"");
    PhpAddStringSetting(L"ImagePidsListViewColumns", L"");
    PhpAddStringSetting(L"ImageTlsListViewColumns", L"");
    PhpAddStringSetting(L"ImageProdIdListViewColumns", L"");
    PhpAddStringSetting(L"ImageDebugListViewColumns", L"");
    PhpAddStringSetting(L"ImageProdIdHashListViewColumns", L"");
    PhpAddStringSetting(L"ImageDebugListViewGroupColumns", L"");
    PhpAddStringSetting(L"ImageDebugCrtListViewColumns", L"");
    PhpAddStringSetting(L"ImageDebugPogoListViewColumns", L"");
    PhpAddStringSetting(L"ImageDisasmTreeColumns", L"");
    PhpAddIntegerPairSetting(L"ImageDisasmWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"ImageDisasmWindowSize", L"@96|0,0");
    PhpAddStringSetting(L"ImageEhContListViewColumns", L"");
    PhpAddStringSetting(L"ImageVolatileListViewColumns", L"");
    PhpAddStringSetting(L"ImageVersionInfoListViewColumns", L"");
    PhpAddStringSetting(L"LibListViewColumns", L"");
    PhpAddStringSetting(L"SymbolsTreeListColumns", L"");
    PhpAddStringSetting(L"SymbolsTreeListSort", L"0,1"); // 0, AscendingSortOrder
    PhpAddIntegerSetting(L"SymbolsTreeListFlags", L"0");
    PhpAddStringSetting(L"StringsTreeListColumns", L"");
    PhpAddStringSetting(L"StringsTreeListSort", L"0,1"); // 0, AscendingSortOrder
    PhpAddIntegerSetting(L"StringsTreeListFlags", L"1b");
    PhpAddIntegerSetting(L"StringsMinimumLength", L"4");
    PhpAddIntegerSetting(L"TreeListBorderEnable", L"0");
    PhpAddIntegerSetting(L"TreeListCustomRowSize", L"0");
    PhpAddStringSetting(L"CHPEListViewColumns", L"");
    // Wsl properties
    PhpAddStringSetting(L"GeneralWslTreeListColumns", L"");
    PhpAddStringSetting(L"DynamicWslListViewColumns", L"");
    PhpAddStringSetting(L"ImportsWslListViewColumns", L"");
    PhpAddStringSetting(L"ExportsWslListViewColumns", L"");
}

VOID PvUpdateCachedSettings(
    VOID
    )
{
    PhMaxSizeUnit = PhGetIntegerSetting(L"MaxSizeUnit");
    PhEnableSecurityAdvancedDialog = !!PhGetIntegerSetting(L"EnableSecurityAdvancedDialog");
    PhEnableThemeSupport = !!PhGetIntegerSetting(L"EnableThemeSupport");
    PhFontQuality = PhGetFontQualitySetting(PhGetIntegerSetting(L"FontQuality"));
    PhEnableWindowBorderColor = !!PhGetIntegerSetting(L"EnableWindowBorderColor");
    PhThemeWindowForegroundColor = PhGetIntegerSetting(L"ThemeWindowForegroundColor");
    PhThemeWindowBackgroundColor = PhGetIntegerSetting(L"ThemeWindowBackgroundColor");
    PhThemeWindowBackground2Color = PhGetIntegerSetting(L"ThemeWindowBackground2Color");
    PhThemeWindowHighlightColor = PhGetIntegerSetting(L"ThemeWindowHighlightColor");
    PhThemeWindowHighlight2Color = PhGetIntegerSetting(L"ThemeWindowHighlight2Color");
    PhThemeWindowTextColor = PhGetIntegerSetting(L"ThemeWindowTextColor");
    // Always off: phlib's listview branch applies WS_BORDER *and* WS_EX_CLIENTEDGE
    // together, which renders as two nested frames. peview owns listview border
    // policy through PvConfigTreeBorders and the dialog templates, so the "Enable
    // view borders" option drives those instead. (dmex)
    PhEnableThemeListviewBorder = FALSE;
}

VOID PvInitializeSettings(
    VOID
    )
{
    NTSTATUS status = STATUS_OBJECT_NAME_NOT_FOUND;
    PPH_STRING settingsPath = NULL;

    PvAddDefaultSettings();

    // 1. Default locations (Portable, AppData or Registry)
    status = PhLoadSettingsAutoDetect(NULL, L"peview", &settingsPath, NULL, NULL);

    if (NT_SUCCESS(status) || status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        PhMoveReference(&PvSettingsFileName, settingsPath);
    }

    if (PvSettingsFileName)
    {
        // If we didn't find the file, it will be created. Otherwise,
        // there was probably a parsing error and we don't want to
        // change anything.
        if (status == STATUS_FILE_CORRUPT_ERROR)
        {
            if (PhShowMessage2(
                NULL,
                TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
                TD_WARNING_ICON,
                L"PE View's settings file is corrupt. Do you want to reset it?",
                L"If you select No, the settings system will not function properly."
                ) == IDYES)
            {
                PhResetSettingsFile(&PvSettingsFileName->sr);
            }
            else
            {
                // Pretend we don't have a settings store so bad things don't happen.
                PhDereferenceObject(PvSettingsFileName);
                PvSettingsFileName = NULL;
            }
        }
    }

    PvUpdateCachedSettings();
}

VOID PvSaveSettings(
    VOID
    )
{
    if (!PhIsNullOrEmptyString(PvSettingsFileName))
        PhSaveSettings(&PvSettingsFileName->sr);
}
