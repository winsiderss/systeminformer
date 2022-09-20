/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2022
 *
 */

#include <peview.h>

#ifdef __has_include
#if __has_include (<metahost.h>)
#include <metahost.h>
#else
#include "metahost/metahost.h"
#endif
#else
#include "metahost/metahost.h"
#endif

#ifdef __has_include
#if __has_include (<cor.h>)
#include <cor.h>
#else
#define _WINDOWS_UPDATES_
#include "metahost/corhdr.h"
#include "metahost/cor.h"
#endif
#else
#define _WINDOWS_UPDATES_
#include "metahost/corhdr.h"
#include "metahost/cor.h"
#endif

// metamodelpub.h
#define TBL_Method 6UL
#define TBL_ModuleRef 26UL
#define TBL_ImplMap 28UL
// ModuleRefRec
#define ModuleRefRec_COL_Name 0UL
// ImplMapRec
#define ImplMapRec_COL_MappingFlags 0UL
#define ImplMapRec_COL_MemberForwarded 1UL // mdField or mdMethod
#define ImplMapRec_COL_ImportName 2UL
#define ImplMapRec_COL_ImportScope 3UL // mdModuleRef

EXTERN_C
PPH_STRING PvClrImportFlagsToString(
    _In_ ULONG Flags
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (IsPmNoMangle(Flags))
        PhAppendStringBuilder2(&stringBuilder, const_cast<wchar_t*>(L"No mangle, "));
    if (IsPmCharSetAnsi(Flags))
        PhAppendStringBuilder2(&stringBuilder, const_cast<wchar_t*>(L"Ansi charset, "));
    if (IsPmCharSetUnicode(Flags))
        PhAppendStringBuilder2(&stringBuilder, const_cast<wchar_t*>(L"Unicode charset, "));
    if (IsPmCharSetAuto(Flags))
        PhAppendStringBuilder2(&stringBuilder, const_cast<wchar_t*>(L"Auto charset, "));
    if (IsPmSupportsLastError(Flags))
        PhAppendStringBuilder2(&stringBuilder, const_cast<wchar_t*>(L"Supports last error, "));
    if (IsPmCallConvWinapi(Flags))
        PhAppendStringBuilder2(&stringBuilder, const_cast<wchar_t*>(L"Winapi, "));
    if (IsPmCallConvCdecl(Flags))
        PhAppendStringBuilder2(&stringBuilder, const_cast<wchar_t*>(L"Cdecl, "));
    if (IsPmCallConvStdcall(Flags))
        PhAppendStringBuilder2(&stringBuilder, const_cast<wchar_t*>(L"Stdcall, "));
    if (IsPmCallConvThiscall(Flags))
        PhAppendStringBuilder2(&stringBuilder, const_cast<wchar_t*>(L"Thiscall, "));
    if (IsPmCallConvFastcall(Flags))
        PhAppendStringBuilder2(&stringBuilder, const_cast<wchar_t*>(L"Fastcall, "));
    if (IsPmBestFitEnabled(Flags))
        PhAppendStringBuilder2(&stringBuilder, const_cast<wchar_t*>(L"Bestfit enabled, "));
    if (IsPmBestFitDisabled(Flags))
        PhAppendStringBuilder2(&stringBuilder, const_cast<wchar_t*>(L"Bestfit disabled, "));
    if (IsPmBestFitUseAssem(Flags))
        PhAppendStringBuilder2(&stringBuilder, const_cast<wchar_t*>(L"Bestfit assembly, "));
    if (IsPmThrowOnUnmappableCharEnabled(Flags))
        PhAppendStringBuilder2(&stringBuilder, const_cast<wchar_t*>(L"ThrowOnUnmappableChar enabled, "));
    if (IsPmThrowOnUnmappableCharDisabled(Flags))
        PhAppendStringBuilder2(&stringBuilder, const_cast<wchar_t*>(L"ThrowOnUnmappableChar disabled, "));
    if (IsPmThrowOnUnmappableCharUseAssem(Flags))
        PhAppendStringBuilder2(&stringBuilder, const_cast<wchar_t*>(L"ThrowOnUnmappableChar assembly, "));

    if (PhEndsWithString2(stringBuilder.String, const_cast<wchar_t*>(L", "), FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(Flags));
    PhAppendFormatStringBuilder(&stringBuilder, const_cast<wchar_t*>(L" (%s)"), pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

// TODO: Add support for dynamic imports by enumerating the types. (dmex)
EXTERN_C HRESULT PvGetClrImageImports(
    _In_ PVOID ClrMetaDataDispenser,
    _In_ PWSTR FileName,
    _Out_ PPH_LIST* ClrImportsList
    )
{
    IMetaDataDispenser* metaDataDispenser = reinterpret_cast<IMetaDataDispenser*>(ClrMetaDataDispenser);
    IMetaDataImport* metaDataImport = nullptr;
    IMetaDataTables* metaDataTables = nullptr;
    PPH_LIST clrImportsList = nullptr;
    HRESULT status = E_FAIL;
    ULONG rowModuleCount = 0;
    ULONG rowModuleColumns = 0;
    ULONG rowImportCount = 0;
    ULONG rowImportColumns = 0;

    status = metaDataDispenser->OpenScopeOnMemory(
        PvMappedImage.ViewBase,
        static_cast<ULONG>(PvMappedImage.Size),
        ofReadOnly,
        IID_IMetaDataImport,
        reinterpret_cast<IUnknown**>(&metaDataImport)
        );

    if (!SUCCEEDED(status))
    {
        status = metaDataDispenser->OpenScope(
            FileName,
            ofReadOnly,
            IID_IMetaDataImport,
            reinterpret_cast<IUnknown**>(&metaDataImport)
            );
    }

    if (!SUCCEEDED(status))
    {
        metaDataDispenser->Release();
        return status;
    }

    status = metaDataImport->QueryInterface(
        IID_IMetaDataTables,
        reinterpret_cast<void**>(&metaDataTables)
        );

    if (!SUCCEEDED(status))
    {
        metaDataImport->Release();
        metaDataDispenser->Release();
        return status;
    }

    clrImportsList = PhCreateList(64);

    // dummy unknown entry at index 0
    {
        PPV_CLR_IMAGE_IMPORT_DLL importDll;

        importDll = static_cast<PPV_CLR_IMAGE_IMPORT_DLL>(PhAllocateZero(sizeof(PV_CLR_IMAGE_IMPORT_DLL)));
        importDll->ImportName = PhCreateString(const_cast<wchar_t*>(L"Unknown"));
        importDll->ImportToken = ULONG_MAX;

        PhAddItemList(clrImportsList, importDll);
    }

    if (SUCCEEDED(metaDataTables->GetTableInfo(TBL_ModuleRef, nullptr, &rowModuleCount, &rowModuleColumns, nullptr, nullptr)))
    {
        for (ULONG i = 1; i <= rowModuleCount; i++)
        {
            ULONG moduleNameValue = 0;
            const char* moduleName = nullptr;

            if (SUCCEEDED(metaDataTables->GetColumn(TBL_ModuleRef, ModuleRefRec_COL_Name, i, &moduleNameValue)))
            {
                if (SUCCEEDED(metaDataTables->GetString(moduleNameValue, &moduleName)))
                {
                    PPV_CLR_IMAGE_IMPORT_DLL importDll;

                    importDll = static_cast<PPV_CLR_IMAGE_IMPORT_DLL>(PhAllocateZero(sizeof(PV_CLR_IMAGE_IMPORT_DLL)));
                    importDll->ImportName = PhConvertUtf8ToUtf16(const_cast<char*>(moduleName));
                    importDll->ImportToken = TokenFromRid(i, mdtModuleRef);

                    PhAddItemList(clrImportsList, importDll);
                }
            }
        }
    }

    if (SUCCEEDED(metaDataTables->GetTableInfo(TBL_ImplMap, nullptr, &rowImportCount, &rowImportColumns, nullptr, nullptr)))
    {
        for (ULONG i = 1; i <= rowImportCount; i++)
        {
            bool found = false;
            ULONG importFlagsValue = 0;
            ULONG importNameValue = 0;
            ULONG moduleTokenValue = 0;
            PVOID importRowValue = 0;
            PVOID importOffsetValue = 0;
            const char* importName = nullptr;

            metaDataTables->GetColumn(TBL_ImplMap, ImplMapRec_COL_MappingFlags, i, &importFlagsValue);

            if (SUCCEEDED(metaDataTables->GetColumn(TBL_ImplMap, ImplMapRec_COL_ImportName, i, &importNameValue)))
            {
                metaDataTables->GetString(importNameValue, &importName);
            }

            if (!SUCCEEDED(metaDataTables->GetColumn(TBL_ImplMap, ImplMapRec_COL_ImportScope, i, &moduleTokenValue)))
            {
                moduleTokenValue = ULONG_MAX;
            }

            if (SUCCEEDED(metaDataTables->GetRow(TBL_ImplMap, i, &importRowValue)))
            {
                importOffsetValue = PTR_SUB_OFFSET(importRowValue, PvMappedImage.ViewBase);
            }

            for (ULONG i = 0; i < clrImportsList->Count; i++)
            {
                PPV_CLR_IMAGE_IMPORT_DLL importDll = static_cast<PPV_CLR_IMAGE_IMPORT_DLL>(clrImportsList->Items[i]);

                if (importDll->ImportToken == moduleTokenValue)
                {
                    if (!importDll->Functions)
                        importDll->Functions = PhCreateList(1);
                    if (!importName)
                        importName = "Unknown";

                    if (importDll->Functions)
                    {
                        PPV_CLR_IMAGE_IMPORT_FUNCTION importFunction;

                        importFunction = static_cast<PPV_CLR_IMAGE_IMPORT_FUNCTION>(PhAllocateZero(sizeof(PV_CLR_IMAGE_IMPORT_FUNCTION)));
                        importFunction->FunctionName = PhConvertUtf8ToUtf16(const_cast<char*>(importName));
                        importFunction->Flags = importFlagsValue;
                        importFunction->Offset = importOffsetValue;

                        PhAddItemList(importDll->Functions, importFunction);
                    }

                    found = true;
                    break;
                }
            }

            if (!found)
            {
                PPV_CLR_IMAGE_IMPORT_DLL unknownImportDll = static_cast<PPV_CLR_IMAGE_IMPORT_DLL>(clrImportsList->Items[0]);

                if (!unknownImportDll->Functions)
                    unknownImportDll->Functions = PhCreateList(1);
                if (!importName)
                    importName = "Unknown";

                if (unknownImportDll->Functions)
                {
                    PPV_CLR_IMAGE_IMPORT_FUNCTION importFunction;

                    importFunction = static_cast<PPV_CLR_IMAGE_IMPORT_FUNCTION>(PhAllocateZero(sizeof(PV_CLR_IMAGE_IMPORT_FUNCTION)));
                    importFunction->FunctionName = PhConvertUtf8ToUtf16(const_cast<char*>(importName));
                    importFunction->Flags = importFlagsValue;
                    importFunction->Offset = importOffsetValue;

                    PhAddItemList(unknownImportDll->Functions, importFunction);
                }
            }
        }
    }

    metaDataTables->Release();
    metaDataImport->Release();
    metaDataDispenser->Release();

    *ClrImportsList = clrImportsList;
    return S_OK;
}
