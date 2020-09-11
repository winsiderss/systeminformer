/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2010-2011 wj32
 * Copyright (C) 2017-2019 dmex
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

#define ADD_VALUE(Name, Value) \
{ \
    INT lvItemIndex; \
    lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, Name, NULL); \
    PhSetListViewSubItem(lvHandle, lvItemIndex, 1, Value); \
}

PPH_STRING PvpGetPeGuardFlagsText(
    _In_ ULONG GuardFlags
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    if (GuardFlags == 0)
        return PhCreateString(L"0x0");

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (GuardFlags & IMAGE_GUARD_CF_INSTRUMENTED)
        PhAppendStringBuilder2(&stringBuilder, L"Instrumented, ");
    if (GuardFlags & IMAGE_GUARD_CFW_INSTRUMENTED)
        PhAppendStringBuilder2(&stringBuilder, L"Instrumented (Write), ");
    if (GuardFlags & IMAGE_GUARD_CF_FUNCTION_TABLE_PRESENT)
        PhAppendStringBuilder2(&stringBuilder, L"Function table, ");
    if (GuardFlags & IMAGE_GUARD_SECURITY_COOKIE_UNUSED)
        PhAppendStringBuilder2(&stringBuilder, L"Unused security cookie, ");
    if (GuardFlags & IMAGE_GUARD_PROTECT_DELAYLOAD_IAT)
        PhAppendStringBuilder2(&stringBuilder, L"Delay-load IAT protected, ");
    if (GuardFlags & IMAGE_GUARD_DELAYLOAD_IAT_IN_ITS_OWN_SECTION)
        PhAppendStringBuilder2(&stringBuilder, L"Delay-load private section, ");
    if (GuardFlags & IMAGE_GUARD_CF_ENABLE_EXPORT_SUPPRESSION)
        PhAppendStringBuilder2(&stringBuilder, L"Export supression, ");
    if (GuardFlags & IMAGE_GUARD_CF_EXPORT_SUPPRESSION_INFO_PRESENT)
        PhAppendStringBuilder2(&stringBuilder, L"Export information supression, ");
    if (GuardFlags & IMAGE_GUARD_CF_LONGJUMP_TABLE_PRESENT)
        PhAppendStringBuilder2(&stringBuilder, L"Longjump table, ");
    if (GuardFlags & IMAGE_GUARD_RETPOLINE_PRESENT)
        PhAppendStringBuilder2(&stringBuilder, L"Retpoline present, ");
    if (GuardFlags & IMAGE_GUARD_EH_CONTINUATION_TABLE_PRESENT_V1 || GuardFlags & IMAGE_GUARD_EH_CONTINUATION_TABLE_PRESENT_V2)
        PhAppendStringBuilder2(&stringBuilder, L"EH continuation table, ");
    if (GuardFlags & IMAGE_GUARD_XFG_ENABLED)
        PhAppendStringBuilder2(&stringBuilder, L"XFG, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(GuardFlags));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_STRING PvpGetPeDependentLoadFlagsText(
    _In_ ULONG DependentLoadFlags
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    if (DependentLoadFlags == 0)
        return PhCreateString(L"0x0");

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (DependentLoadFlags & DONT_RESOLVE_DLL_REFERENCES)
        PhAppendStringBuilder2(&stringBuilder, L"Ignore dll references, ");
    if (DependentLoadFlags & LOAD_LIBRARY_AS_DATAFILE)
        PhAppendStringBuilder2(&stringBuilder, L"Datafile, ");
    if (DependentLoadFlags & 0x00000004) // LOAD_PACKAGED_LIBRARY
        PhAppendStringBuilder2(&stringBuilder, L"Packaged_library, ");
    if (DependentLoadFlags & LOAD_WITH_ALTERED_SEARCH_PATH)
        PhAppendStringBuilder2(&stringBuilder, L"Altered search path, ");
    if (DependentLoadFlags & LOAD_IGNORE_CODE_AUTHZ_LEVEL)
        PhAppendStringBuilder2(&stringBuilder, L"Ignore authz level, ");
    if (DependentLoadFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE)
        PhAppendStringBuilder2(&stringBuilder, L"Image resource, ");
    if (DependentLoadFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE)
        PhAppendStringBuilder2(&stringBuilder, L"Datafile (Exclusive), ");
    if (DependentLoadFlags & LOAD_LIBRARY_REQUIRE_SIGNED_TARGET)
        PhAppendStringBuilder2(&stringBuilder, L"Signed target required, ");
    if (DependentLoadFlags & LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR)
        PhAppendStringBuilder2(&stringBuilder, L"Search DLL load directory, ");
    if (DependentLoadFlags & LOAD_LIBRARY_SEARCH_APPLICATION_DIR)
        PhAppendStringBuilder2(&stringBuilder, L"Search application directory, ");
    if (DependentLoadFlags & LOAD_LIBRARY_SEARCH_USER_DIRS)
        PhAppendStringBuilder2(&stringBuilder, L"Search user directory, ");
    if (DependentLoadFlags & LOAD_LIBRARY_SEARCH_SYSTEM32)
        PhAppendStringBuilder2(&stringBuilder, L"Search system32, ");
    if (DependentLoadFlags & LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)
        PhAppendStringBuilder2(&stringBuilder, L"Search default directory, ");
    if (DependentLoadFlags & LOAD_LIBRARY_SAFE_CURRENT_DIRS)
        PhAppendStringBuilder2(&stringBuilder, L"Search safe current directory, ");
    if (DependentLoadFlags & LOAD_LIBRARY_SEARCH_SYSTEM32_NO_FORWARDER)
        PhAppendStringBuilder2(&stringBuilder, L"Search system32 (No forwarders), ");
    if (DependentLoadFlags & LOAD_LIBRARY_OS_INTEGRITY_CONTINUITY)
        PhAppendStringBuilder2(&stringBuilder, L"OS integrity continuity, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(DependentLoadFlags));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_STRING PvpGetPeEnclaveImportsText(
    _In_ PVOID EnclaveConfig
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG i;

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        PIMAGE_ENCLAVE_CONFIG32 enclaveConfig32 = EnclaveConfig;
        PIMAGE_ENCLAVE_IMPORT enclaveImports;

        enclaveImports = PhMappedImageRvaToVa(
            &PvMappedImage,
            enclaveConfig32->ImportList,
            NULL
            );

        for (i = 0; i < enclaveConfig32->NumberOfImports; i++)
        {
            PSTR importName;

            if (!enclaveImports || enclaveImports->ImportName == USHRT_MAX)
                break;

            if (importName = PhMappedImageRvaToVa(
                &PvMappedImage,
                enclaveImports->ImportName,
                NULL
                ))
            {
                PhAppendFormatStringBuilder(&stringBuilder, L"%hs, ", importName);
            }

            enclaveImports++;
        }
    }
    else
    {
        PIMAGE_ENCLAVE_CONFIG64 enclaveConfig64 = EnclaveConfig;
        PIMAGE_ENCLAVE_IMPORT enclaveImports;

        enclaveImports = PhMappedImageRvaToVa(
            &PvMappedImage,
            enclaveConfig64->ImportList,
            NULL
            );

        for (i = 0; i < enclaveConfig64->NumberOfImports; i++)
        {
            PSTR importName;

            if (!enclaveImports || enclaveImports->ImportName == USHRT_MAX)
                break;

            if (importName = PhMappedImageRvaToVa(
                &PvMappedImage,
                enclaveImports->ImportName,
                NULL
                ))
            {
                PhAppendFormatStringBuilder(&stringBuilder, L"%hs, ", importName);
            }

            enclaveImports++;
        }
    }

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    return PhFinalStringBuilderString(&stringBuilder);
}

VOID PvpAddPeEnclaveConfig(
    _In_ PVOID ImageConfig,
    _In_ HWND lvHandle
    )
{
    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        PIMAGE_LOAD_CONFIG_DIRECTORY32 imageConfig32 = ImageConfig;
        PIMAGE_ENCLAVE_CONFIG32 enclaveConfig;

        if (!RTL_CONTAINS_FIELD(imageConfig32, imageConfig32->Size, EnclaveConfigurationPointer))
            return;

        enclaveConfig = PhMappedImageVaToVa(
            &PvMappedImage,
            (ULONG)imageConfig32->EnclaveConfigurationPointer,
            NULL
            );

        if (enclaveConfig)
        {
            ADD_VALUE(L"Enclave PolicyFlags", PhaFormatUInt64(enclaveConfig->PolicyFlags, TRUE)->Buffer);
            ADD_VALUE(L"Enclave FamilyID", PH_AUTO_T(PH_STRING, PhFormatGuid((PGUID)enclaveConfig->FamilyID))->Buffer);
            ADD_VALUE(L"Enclave ImageID", PH_AUTO_T(PH_STRING, PhFormatGuid((PGUID)enclaveConfig->ImageID))->Buffer);
            ADD_VALUE(L"Enclave ImageVersion", PhaFormatUInt64(enclaveConfig->ImageVersion, TRUE)->Buffer);
            ADD_VALUE(L"Enclave SecurityVersion", PhaFormatUInt64(enclaveConfig->SecurityVersion, TRUE)->Buffer);
            ADD_VALUE(L"Enclave EnclaveSize", PhaFormatUInt64(enclaveConfig->EnclaveSize, TRUE)->Buffer);
            ADD_VALUE(L"Enclave NumberOfThreads", PhaFormatUInt64(enclaveConfig->NumberOfThreads, TRUE)->Buffer);
            ADD_VALUE(L"Enclave EnclaveFlags", PhaFormatUInt64(enclaveConfig->EnclaveFlags, TRUE)->Buffer);
            ADD_VALUE(L"Enclave NumberOfImports", PhaFormatUInt64(enclaveConfig->NumberOfImports, TRUE)->Buffer);
            ADD_VALUE(L"Enclave Imports", PH_AUTO_T(PH_STRING, PvpGetPeEnclaveImportsText(enclaveConfig))->Buffer);
        }
    }
    else
    {
        PIMAGE_LOAD_CONFIG_DIRECTORY64 imageConfig64 = ImageConfig;
        PIMAGE_ENCLAVE_CONFIG64 enclaveConfig;

        if (!RTL_CONTAINS_FIELD(imageConfig64, imageConfig64->Size, EnclaveConfigurationPointer))
            return;

        enclaveConfig = PhMappedImageVaToVa(
            &PvMappedImage,
            (ULONG)imageConfig64->EnclaveConfigurationPointer,
            NULL
            );

        if (enclaveConfig)
        {
            ADD_VALUE(L"Enclave PolicyFlags", PhaFormatUInt64(enclaveConfig->PolicyFlags, TRUE)->Buffer);
            ADD_VALUE(L"Enclave FamilyID", PH_AUTO_T(PH_STRING, PhFormatGuid((PGUID)enclaveConfig->FamilyID))->Buffer);
            ADD_VALUE(L"Enclave ImageID", PH_AUTO_T(PH_STRING, PhFormatGuid((PGUID)enclaveConfig->ImageID))->Buffer);
            ADD_VALUE(L"Enclave ImageVersion", PhaFormatUInt64(enclaveConfig->ImageVersion, TRUE)->Buffer);
            ADD_VALUE(L"Enclave SecurityVersion", PhaFormatUInt64(enclaveConfig->SecurityVersion, TRUE)->Buffer);
            ADD_VALUE(L"Enclave EnclaveSize", PhaFormatUInt64(enclaveConfig->EnclaveSize, TRUE)->Buffer);
            ADD_VALUE(L"Enclave NumberOfThreads", PhaFormatUInt64(enclaveConfig->NumberOfThreads, TRUE)->Buffer);
            ADD_VALUE(L"Enclave EnclaveFlags", PhaFormatUInt64(enclaveConfig->EnclaveFlags, TRUE)->Buffer);
            ADD_VALUE(L"Enclave NumberOfImports", PhaFormatUInt64(enclaveConfig->NumberOfImports, TRUE)->Buffer);
            ADD_VALUE(L"Enclave Imports", PH_AUTO_T(PH_STRING, PvpGetPeEnclaveImportsText(enclaveConfig))->Buffer);
        }
    }
}

INT_PTR CALLBACK PvpPeLoadConfigDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;
            PIMAGE_LOAD_CONFIG_DIRECTORY32 config32;
            PIMAGE_LOAD_CONFIG_DIRECTORY64 config64;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 220, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 170, L"Value");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"ImageLoadCfgListViewColumns", lvHandle);

            #define ADD_VALUES(Type, Config) \
            { \
                LARGE_INTEGER time; \
                SYSTEMTIME systemTime; \
                \
                RtlSecondsSince1970ToTime((Config)->TimeDateStamp, &time); \
                PhLargeIntegerToLocalSystemTime(&systemTime, &time); \
                \
                ADD_VALUE(L"Time stamp", (Config)->TimeDateStamp ? PhaFormatDateTime(&systemTime)->Buffer : L"0"); \
                ADD_VALUE(L"Version", PhaFormatString(L"%u.%u", (Config)->MajorVersion, (Config)->MinorVersion)->Buffer); \
                ADD_VALUE(L"Global flags to clear", PhaFormatString(L"0x%x", (Config)->GlobalFlagsClear)->Buffer); \
                ADD_VALUE(L"Global flags to set", PhaFormatString(L"0x%x", (Config)->GlobalFlagsSet)->Buffer); \
                ADD_VALUE(L"Critical section default timeout", PhaFormatUInt64((Config)->CriticalSectionDefaultTimeout, TRUE)->Buffer); \
                ADD_VALUE(L"De-commit free block threshold", PhaFormatUInt64((Config)->DeCommitFreeBlockThreshold, TRUE)->Buffer); \
                ADD_VALUE(L"De-commit total free threshold", PhaFormatUInt64((Config)->DeCommitTotalFreeThreshold, TRUE)->Buffer); \
                ADD_VALUE(L"Lock prefix table", PhaFormatString(L"0x%x", (Config)->LockPrefixTable)->Buffer); \
                ADD_VALUE(L"Maximum allocation size", PhaFormatString(L"0x%Ix", (Config)->MaximumAllocationSize)->Buffer); \
                ADD_VALUE(L"Virtual memory threshold", PhaFormatString(L"0x%Ix", (Config)->VirtualMemoryThreshold)->Buffer); \
                ADD_VALUE(L"Process heap flags", PhaFormatString(L"0x%Ix", (Config)->ProcessHeapFlags)->Buffer); \
                ADD_VALUE(L"Process affinity mask", PhaFormatString(L"0x%Ix", (Config)->ProcessAffinityMask)->Buffer); \
                ADD_VALUE(L"CSD version", PhaFormatString(L"%u", (Config)->CSDVersion)->Buffer); \
                ADD_VALUE(L"Dependent load flags", PH_AUTO_T(PH_STRING, PvpGetPeDependentLoadFlagsText((Config)->DependentLoadFlags))->Buffer); \
                ADD_VALUE(L"Edit list", PhaFormatString(L"0x%Ix", (Config)->EditList)->Buffer); \
                ADD_VALUE(L"Security cookie", PhaFormatString(L"0x%Ix", (Config)->SecurityCookie)->Buffer); \
                ADD_VALUE(L"SEH handler table", PhaFormatString(L"0x%Ix", (Config)->SEHandlerTable)->Buffer); \
                ADD_VALUE(L"SEH handler count", PhaFormatUInt64((Config)->SEHandlerCount, TRUE)->Buffer); \
                \
                if (RTL_CONTAINS_FIELD((Config), (Config)->Size, GuardCFCheckFunctionPointer)) \
                { \
                    ADD_VALUE(L"CFG guard flags", PH_AUTO_T(PH_STRING, PvpGetPeGuardFlagsText((Config)->GuardFlags))->Buffer); \
                    ADD_VALUE(L"CFG check-function pointer", PhaFormatString(L"0x%Ix", (Config)->GuardCFCheckFunctionPointer)->Buffer); \
                    ADD_VALUE(L"CFG check-function pointer", PhaFormatString(L"0x%Ix", (Config)->GuardCFDispatchFunctionPointer)->Buffer); \
                    ADD_VALUE(L"CFG function table", PhaFormatString(L"0x%Ix", (Config)->GuardCFFunctionTable)->Buffer); \
                    ADD_VALUE(L"CFG function table count", PhaFormatUInt64((Config)->GuardCFFunctionCount, TRUE)->Buffer); \
                    if (RTL_CONTAINS_FIELD((Config), (Config)->Size, GuardAddressTakenIatEntryTable)) \
                    { \
                        ADD_VALUE(L"CFG IatEntry table", PhaFormatString(L"0x%Ix", (Config)->GuardAddressTakenIatEntryTable)->Buffer); \
                        ADD_VALUE(L"CFG IatEntry table entry count", PhaFormatUInt64((Config)->GuardAddressTakenIatEntryCount, TRUE)->Buffer); \
                    } \
                    if (RTL_CONTAINS_FIELD((Config), (Config)->Size, GuardLongJumpTargetTable)) \
                    { \
                        ADD_VALUE(L"CFG LongJump table", PhaFormatString(L"0x%Ix", (Config)->GuardLongJumpTargetTable)->Buffer); \
                        ADD_VALUE(L"CFG LongJump table entry count", PhaFormatUInt64((Config)->GuardLongJumpTargetCount, TRUE)->Buffer); \
                    } \
                } \
                \
                if (RTL_CONTAINS_FIELD((Config), (Config)->Size, CodeIntegrity)) \
                { \
                    ADD_VALUE(L"CI flags", PhaFormatString(L"0x%x", (Config)->CodeIntegrity.Flags)->Buffer); \
                    ADD_VALUE(L"CI catalog", PhaFormatString(L"0x%Ix", (Config)->CodeIntegrity.Catalog)->Buffer); \
                    ADD_VALUE(L"CI catalog offset", PhaFormatString(L"0x%Ix", (Config)->CodeIntegrity.CatalogOffset)->Buffer); \
                } \
                \
                if (RTL_CONTAINS_FIELD((Config), (Config)->Size, DynamicValueRelocTable)) \
                { \
                    ADD_VALUE(L"Dynamic value relocation table", PhaFormatString(L"0x%Ix", (Config)->DynamicValueRelocTable)->Buffer); \
                    ADD_VALUE(L"Hybrid metadata pointer", PhaFormatString(L"0x%Ix", (Config)->CHPEMetadataPointer)->Buffer); \
                    ADD_VALUE(L"GuardRF failure-function routine", PhaFormatString(L"0x%Ix", (Config)->GuardRFFailureRoutine)->Buffer); \
                    ADD_VALUE(L"GuardRF failure-function pointer", PhaFormatString(L"0x%Ix", (Config)->GuardRFFailureRoutineFunctionPointer)->Buffer); \
                } \
                \
                if (RTL_CONTAINS_FIELD((Config), (Config)->Size, DynamicValueRelocTableOffset)) \
                { \
                    ADD_VALUE(L"DynamicValue relocation table offset", PhaFormatString(L"0x%Ix", (Config)->DynamicValueRelocTableOffset)->Buffer); \
                    ADD_VALUE(L"DynamicValue relocation section", PhaFormatString(L"0x%Ix", (Config)->DynamicValueRelocTableSection)->Buffer); \
                    ADD_VALUE(L"GuardRF verify-stack pointer", PhaFormatString(L"0x%Ix", (Config)->GuardRFVerifyStackPointerFunctionPointer)->Buffer); \
                    ADD_VALUE(L"Hot patching table offset", PhaFormatString(L"0x%Ix", (Config)->HotPatchTableOffset)->Buffer); \
                    ADD_VALUE(L"Enclave configuration pointer", PhaFormatString(L"0x%Ix", (Config)->EnclaveConfigurationPointer)->Buffer); \
                } \
                \
                if (RTL_CONTAINS_FIELD((Config), (Config)->Size, VolatileMetadataPointer)) \
                { \
                    ADD_VALUE(L"Volatile metadata pointer", PhaFormatString(L"0x%Ix", (Config)->VolatileMetadataPointer)->Buffer); \
                } \
                if (RTL_CONTAINS_FIELD((Config), (Config)->Size, GuardEHContinuationTable)) \
                { \
                    ADD_VALUE(L"Guard EH Continuation table", PhaFormatString(L"0x%Ix", (Config)->GuardEHContinuationTable)->Buffer); \
                    ADD_VALUE(L"Guard EH Continuation table entry count", PhaFormatUInt64((Config)->GuardEHContinuationCount, TRUE)->Buffer); \
                } \
            }

            if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            {
                if (NT_SUCCESS(PhGetMappedImageLoadConfig32(&PvMappedImage, &config32)))
                {
                    ADD_VALUES(IMAGE_LOAD_CONFIG_DIRECTORY32, config32);
                    PvpAddPeEnclaveConfig(config32, lvHandle);
                }
            }
            else
            {
                if (NT_SUCCESS(PhGetMappedImageLoadConfig64(&PvMappedImage, &config64)))
                {
                    ADD_VALUES(IMAGE_LOAD_CONFIG_DIRECTORY64, config64);
                    PvpAddPeEnclaveConfig(config64, lvHandle);
                }
            }

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageLoadCfgListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST), dialogItem, PH_ANCHOR_ALL);

                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    }

    return FALSE;
}
