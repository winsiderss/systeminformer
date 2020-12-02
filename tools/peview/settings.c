/*
 * PE viewer -
 *   program settings
 *
 * Copyright (C) 2017 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <peview.h>

static PPH_STRING PeSettingsFileName = NULL;
BOOLEAN PeEnableThemeSupport = FALSE;

VOID PhAddDefaultSettings(
    VOID
    )
{
    PhpAddIntegerSetting(L"FirstRun", L"1");
    PhpAddStringSetting(L"Font", L""); // null
    PhpAddStringSetting(L"DbgHelpSearchPath", L"SRV*C:\\Symbols*https://msdl.microsoft.com/download/symbols");
    PhpAddIntegerSetting(L"EnableSecurityAdvancedDialog", L"1");
    PhpAddIntegerSetting(L"EnableThemeSupport", L"0");
    PhpAddIntegerSetting(L"EnableVersionSupport", L"0");
    PhpAddIntegerSetting(L"GraphColorMode", L"1");
    PhpAddIntegerSetting(L"HashAlgorithm", L"0");
    PhpAddIntegerSetting(L"MaxSizeUnit", L"6");
    PhpAddStringSetting(L"MainWindowPage", L"General");
    PhpAddIntegerPairSetting(L"MainWindowPosition", L"150,150");
    PhpAddScalableIntegerPairSetting(L"MainWindowSize", L"@96|550,580");
    PhpAddStringSetting(L"ImageGeneralPropertiesListViewColumns", L"");
    PhpAddStringSetting(L"ImageGeneralPropertiesListViewSort", L"");
    PhpAddStringSetting(L"ImageDirectoryListViewColumns", L"");
    PhpAddStringSetting(L"ImageLoadCfgListViewColumns", L"");
    PhpAddStringSetting(L"ImageExportsListViewColumns", L"");
    PhpAddStringSetting(L"ImageImportsListViewColumns", L"");
    PhpAddStringSetting(L"ImageCfgListViewColumns", L"");
    PhpAddStringSetting(L"ImageResourcesListViewColumns", L"");
    PhpAddStringSetting(L"ImageAttributesListViewColumns", L"");
    PhpAddStringSetting(L"ImagePropertiesListViewColumns", L"");
    PhpAddStringSetting(L"ImageSecurityListViewColumns", L"");
    PhpAddStringSetting(L"ImageSecurityListViewSort", L"");
    PhpAddStringSetting(L"ImageSecurityTreeColumns", L"");
    PhpAddStringSetting(L"ImageSecurityCertColumns", L"");
    PhpAddIntegerPairSetting(L"ImageSecurityCertWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"ImageSecurityCertWindowSize", L"@96|0,0");
    PhpAddStringSetting(L"ImageStreamsListViewColumns", L"");
    PhpAddStringSetting(L"ImageHardLinksListViewColumns", L"");
    PhpAddStringSetting(L"ImagePidsListViewColumns", L"");
    PhpAddStringSetting(L"ImageSectionsListViewColumns", L"");
    PhpAddStringSetting(L"ImageSectionsListViewSort", L"");
    PhpAddStringSetting(L"ImageTlsListViewColumns", L"");
    PhpAddStringSetting(L"ImageProdIdListViewColumns", L"");
    PhpAddStringSetting(L"ImageDebugListViewColumns", L"");
    PhpAddStringSetting(L"ImageDebugCrtListViewColumns", L"");
    PhpAddStringSetting(L"ImageDebugPogoListViewColumns", L"");
    PhpAddStringSetting(L"ImageEhContListViewColumns", L"");
    PhpAddStringSetting(L"LibListViewColumns", L"");
    PhpAddStringSetting(L"PdbTreeListColumns", L"");
    PhpAddIntegerSetting(L"TreeListBorderEnable", L"0");
    // Wsl properties
    PhpAddStringSetting(L"GeneralWslTreeListColumns", L"");
    PhpAddStringSetting(L"DynamicWslListViewColumns", L"");
    PhpAddStringSetting(L"ImportsWslListViewColumns", L"");
    PhpAddStringSetting(L"ExportsWslListViewColumns", L"");
}

VOID PhUpdateCachedSettings(
    VOID
    )
{
    PeEnableThemeSupport = !!PhGetIntegerSetting(L"EnableThemeSupport");
}

VOID PeInitializeSettings(
    VOID
    )
{
    static PH_STRINGREF settingsPath = PH_STRINGREF_INIT(L"%APPDATA%\\Process Hacker\\peview.xml");
    static PH_STRINGREF settingsSuffix = PH_STRINGREF_INIT(L".settings.xml");
    NTSTATUS status;
    PPH_STRING appFileName;
    PPH_STRING tempFileName;  

    // There are three possible locations for the settings file:
    // 1. A file named peview.exe.settings.xml in the program directory. (This changes
    //    based on the executable file name.)
    // 2. The default location.

    // 1. File in program directory

    appFileName = PhGetApplicationFileName();
    tempFileName = PhConcatStringRef2(&appFileName->sr, &settingsSuffix);
    PhDereferenceObject(appFileName);

    if (PhDoesFileExistsWin32(tempFileName->Buffer))
    {
        PeSettingsFileName = tempFileName;
    }
    else
    {
        PhDereferenceObject(tempFileName);
    }

    // 2. Default location
    if (!PeSettingsFileName)
    {
        PeSettingsFileName = PhExpandEnvironmentStrings(&settingsPath);
    }

    if (PeSettingsFileName)
    {
        status = PhLoadSettings(PeSettingsFileName->Buffer);

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
                if (NT_SUCCESS(PhCreateFileWin32(
                    &fileHandle,
                    PeSettingsFileName->Buffer,
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
                PhDereferenceObject(PeSettingsFileName);
                PeSettingsFileName = NULL;
            }
        }
    }

    // Apply basic global settings.
    PhMaxSizeUnit = PhGetIntegerSetting(L"MaxSizeUnit");
}

VOID PeSaveSettings(
    VOID
    )
{
    if (PeSettingsFileName)
        PhSaveSettings(PeSettingsFileName->Buffer);
}
