/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2023
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
    PhpAddIntegerSetting(L"EnableThemeSupport", L"0");
    PhpAddIntegerSetting(L"EnableThemeAcrylicSupport", L"1");
    PhpAddIntegerSetting(L"EnableThemeAcrylicWindowSupport", L"0");
    PhpAddIntegerSetting(L"EnableTreeListBorder", L"1");
    PhpAddIntegerSetting(L"EnableVersionSupport", L"0");
    PhpAddIntegerSetting(L"GraphColorMode", L"1");
    PhpAddIntegerSetting(L"HashAlgorithm", L"0");
    PhpAddIntegerSetting(L"MaxSizeUnit", L"6");
    PhpAddIntegerSetting(L"MainWindowPageRestoreEnabled", L"1");
    PhpAddStringSetting(L"MainWindowPage", L"General");
    PhpAddIntegerPairSetting(L"MainWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"MainWindowSize", L"@96|550,580");
    PhpAddIntegerSetting(L"MainWindowState", L"1");
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
    PhpAddStringSetting(L"ImageExceptionsIa32ListViewColumns", L"");
    PhpAddStringSetting(L"ImageExceptionsAmd64ListViewColumns", L"");
    PhpAddStringSetting(L"ImageHeadersListViewColumns", L"");
    PhpAddStringSetting(L"ImageHeadersListViewGroupStates", L"");
    PhpAddStringSetting(L"ImageLayoutTreeColumns", L"");
    PhpAddStringSetting(L"ImageCfgListViewColumns", L"");
    PhpAddStringSetting(L"ImageClrListViewColumns", L"");
    PhpAddStringSetting(L"ImageClrImportsListViewColumns", L"");
    PhpAddStringSetting(L"ImageClrTablesListViewColumns", L"");
    PhpAddStringSetting(L"ImageAttributesListViewColumns", L"");
    PhpAddStringSetting(L"ImagePropertiesListViewColumns", L"");
    PhpAddStringSetting(L"ImageRelocationsListViewColumns", L"");
    PhpAddStringSetting(L"ImageDynamicRelocationsListViewColumns", L"");
    PhpAddStringSetting(L"ImageSecurityListViewColumns", L"");
    PhpAddStringSetting(L"ImageSecurityListViewSort", L"");
    PhpAddStringSetting(L"ImageSecurityTreeColumns", L"");
    PhpAddStringSetting(L"ImageSecurityCertColumns", L"");
    PhpAddIntegerPairSetting(L"ImageSecurityCertWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"ImageSecurityCertWindowSize", L"@96|0,0");
    PhpAddStringSetting(L"ImageStreamsListViewColumns", L"");
    PhpAddStringSetting(L"ImageHardLinksListViewColumns", L"");
    PhpAddStringSetting(L"ImageHashesListViewColumns", L"");
    PhpAddStringSetting(L"ImagePidsListViewColumns", L"");
    PhpAddStringSetting(L"ImageTlsListViewColumns", L"");
    PhpAddStringSetting(L"ImageProdIdListViewColumns", L"");
    PhpAddStringSetting(L"ImageDebugListViewColumns", L"");
    PhpAddStringSetting(L"ImageDebugCrtListViewColumns", L"");
    PhpAddStringSetting(L"ImageDebugPogoListViewColumns", L"");
    PhpAddStringSetting(L"ImageDisasmTreeColumns", L"");
    PhpAddIntegerPairSetting(L"ImageDisasmWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"ImageDisasmWindowSize", L"@96|0,0");
    PhpAddStringSetting(L"ImageEhContListViewColumns", L"");
    PhpAddStringSetting(L"ImageVolatileListViewColumns", L"");
    PhpAddStringSetting(L"ImageVersionInfoListViewColumns", L"");
    PhpAddStringSetting(L"LibListViewColumns", L"");
    PhpAddStringSetting(L"PdbTreeListColumns", L"");
    PhpAddIntegerSetting(L"TreeListBorderEnable", L"0");
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
    PhEnableThemeListviewBorder = !!PhGetIntegerSetting(L"TreeListBorderEnable");
}

VOID PvInitializeSettings(
    VOID
    )
{
    NTSTATUS status;
    PPH_STRING appFileName;
    PPH_STRING tempFileName;

    PvAddDefaultSettings();

    // There are three possible locations for the settings file:
    // 1. A file named peview.exe.settings.xml in the program directory. (This changes
    //    based on the executable file name.)
    // 2. The default location.

    // 1. File in program directory

    if (appFileName = PhGetApplicationFileName())
    {
        tempFileName = PhConcatStringRefZ(&appFileName->sr, L".settings.xml");

        if (PhDoesFileExist(&tempFileName->sr))
        {
            PvSettingsFileName = tempFileName;
        }
        else
        {
            PhDereferenceObject(tempFileName);
        }

        PhDereferenceObject(appFileName);
    }

    // 2. Default location
    if (PhIsNullOrEmptyString(PvSettingsFileName))
    {
        PvSettingsFileName = PhGetRoamingAppDataDirectoryZ(L"peview.xml", TRUE);
    }

    if (!PhIsNullOrEmptyString(PvSettingsFileName))
    {
        status = PhLoadSettings(&PvSettingsFileName->sr);

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
                HANDLE fileHandle;
                IO_STATUS_BLOCK isb;
                CHAR data[] = "<settings></settings>";

                // This used to delete the file. But it's better to keep the file there
                // and overwrite it with some valid XML, especially with case (2) above.
                if (NT_SUCCESS(PhCreateFile(
                    &fileHandle,
                    &PvSettingsFileName->sr,
                    FILE_GENERIC_WRITE,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_DELETE,
                    FILE_OVERWRITE,
                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                    )))
                {
                    NtWriteFile(fileHandle, NULL, NULL, NULL, &isb, data, sizeof(data) - 1, NULL, NULL);
                    NtClose(fileHandle);
                }
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
