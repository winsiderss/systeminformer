/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2022-2023
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
enum
{
    TBL_Module = 0,
    TBL_TypeRef = 1,
    TBL_TypeDef = 2,
    TBL_FieldPtr = 3,
    TBL_Field = 4,
    TBL_MethodPtr = 5,
    TBL_Method = 6,
    TBL_ParamPtr = 7,
    TBL_Param = 8,
    TBL_InterfaceImpl = 9,
    TBL_MemberRef = 10,
    TBL_Constant = 11,
    TBL_CustomAttribute = 12,
    TBL_FieldMarshal = 13,
    TBL_DeclSecurity = 14,
    TBL_ClassLayout = 15,
    TBL_FieldLayout = 16,
    TBL_StandAloneSig = 17,
    TBL_EventMap = 18,
    TBL_EventPtr = 19,
    TBL_Event = 20,
    TBL_PropertyMap = 21,
    TBL_PropertyPtr = 22,
    TBL_Property = 23,
    TBL_MethodSemantics = 24,
    TBL_MethodImpl = 25,
    TBL_ModuleRef = 26,
    TBL_TypeSpec = 27,
    TBL_ImplMap = 28,
    TBL_FieldRVA = 29,
    TBL_ENCLog = 30,
    TBL_ENCMap = 31,
    TBL_Assembly = 32,
    TBL_AssemblyProcessor = 33,
    TBL_AssemblyOS = 34,
    TBL_AssemblyRef = 35,
    TBL_AssemblyRefProcessor = 36,
    TBL_AssemblyRefOS = 37,
    TBL_File = 38,
    TBL_ExportedType = 39,
    TBL_ManifestResource = 40,
    TBL_NestedClass = 41,
    TBL_GenericParam = 42,
    TBL_MethodSpec = 43,
    TBL_GenericParamConstraint = 44,
    // Portable PDB
};

// ModuleRefRec
#define ModuleRefRec_COL_Name 0UL
// ImplMapRec
#define ImplMapRec_COL_MappingFlags 0UL
#define ImplMapRec_COL_MemberForwarded 1UL // mdField or mdMethod
#define ImplMapRec_COL_ImportName 2UL
#define ImplMapRec_COL_ImportScope 3UL // mdModuleRef
// AssemblyRefRec
enum
{
    AssemblyRefRec_COL_MajorVersion,
    AssemblyRefRec_COL_MinorVersion,
    AssemblyRefRec_COL_BuildNumber,
    AssemblyRefRec_COL_RevisionNumber,
    AssemblyRefRec_COL_Flags,
    AssemblyRefRec_COL_PublicKeyOrToken,
    AssemblyRefRec_COL_Name,
    AssemblyRefRec_COL_Locale,
    AssemblyRefRec_COL_HashValue
};

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

static int __cdecl PvClrCoreNameCompare(
    _In_ void* Context,
    _In_ void const*elem1,
    _In_ void const*elem2
    )
{
    PPH_STRING item1 = *(PPH_STRING*)elem1;
    PPH_STRING item2 = *(PPH_STRING*)elem2;

    return PhCompareStringWithNull(item1, item2, TRUE);
}

static BOOLEAN PvClrCoreDirectoryCallback(
    _In_ PVOID Information,
    _In_ PVOID Context
    )
{
    PFILE_DIRECTORY_INFORMATION directoryInfo = reinterpret_cast<PFILE_DIRECTORY_INFORMATION>(Information);
    PH_STRINGREF baseName;

    baseName.Buffer = directoryInfo->FileName;
    baseName.Length = directoryInfo->FileNameLength;

    if (PhEqualStringRef2(&baseName, const_cast<wchar_t*>(L"."), TRUE) || PhEqualStringRef2(&baseName, const_cast<wchar_t*>(L".."), TRUE))
        return TRUE;

    if (directoryInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        PhAddItemList(reinterpret_cast<PPH_LIST>(Context), PhCreateString2(&baseName));
    }

    return TRUE;
}

HRESULT PvGetClrMetaDataInterface(
    _In_ PWSTR FileName,
    _Out_ PVOID* ClrCoreBaseAddress,
    _Out_ IMetaDataDispenser** ClrMetaDataInterface
    )
{
    HRESULT (WINAPI* MetaDataGetDispenser_I)(_In_ REFCLSID rclsid, _In_ REFIID riid, _COM_Outptr_ PVOID* ppv) = nullptr;
    HRESULT status = E_FAIL;
    PVOID clrCoreBaseAddress = nullptr;
    IMetaDataDispenser* clrMetadataInterface = nullptr;
    PH_STRINGREF dotNetCorePath;
    PPH_STRING directoryPath;
    HANDLE directoryHandle;

    PhInitializeStringRef(&dotNetCorePath, const_cast<PWSTR>(L"%ProgramFiles%\\dotnet\\shared\\Microsoft.NETCore.App\\"));

    if (directoryPath = PhExpandEnvironmentStrings(&dotNetCorePath))
    {
        if (PhDoesDirectoryExistWin32(PhGetString(directoryPath)))
        {
            PPH_LIST directoryList = PhCreateList(2);
            PH_STRINGREF dotNetCoreName;

            PhInitializeStringRef(&dotNetCoreName, const_cast<PWSTR>(L"\\coreclr.dll"));

            if (NT_SUCCESS(PhOpenFileWin32(
                &directoryHandle,
                PhGetString(directoryPath),
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                )))
            {
                PhEnumDirectoryFile(directoryHandle, nullptr, PvClrCoreDirectoryCallback, directoryList);
                NtClose(directoryHandle);
            }

            // Sort by version?
            qsort_s(directoryList->Items, directoryList->Count, sizeof(PVOID), PvClrCoreNameCompare, nullptr);

            {
                PPH_STRING directoryName = reinterpret_cast<PPH_STRING>(directoryList->Items[directoryList->Count - 1]);
                PPH_STRING fileName = PhConcatStringRef3(
                    &directoryPath->sr,
                    &directoryName->sr,
                    &dotNetCoreName
                    );

                if (PhDoesFileExistWin32(PhGetString(fileName)))
                {
                    clrCoreBaseAddress = PhLoadLibrary(PhGetString(fileName));
                }

                PhDereferenceObject(fileName);
            }

            if (!clrCoreBaseAddress)
            {
                for (ULONG i = 0; i < directoryList->Count; i++)
                {
                    PPH_STRING directoryName = reinterpret_cast<PPH_STRING>(directoryList->Items[i]);
                    PPH_STRING fileName = PhConcatStringRef3(
                        &directoryPath->sr,
                        &directoryName->sr,
                        &dotNetCoreName
                        );

                    if (PhDoesFileExistWin32(PhGetString(fileName)))
                    {
                        clrCoreBaseAddress = PhLoadLibrary(PhGetString(fileName));
                    }

                    PhDereferenceObject(fileName);

                    if (clrCoreBaseAddress)
                        break;
                }
            }

            PhDereferenceObjects(directoryList->Items, directoryList->Count);
            PhDereferenceObject(directoryList);
        }

        PhDereferenceObject(directoryPath);
    }

    if (!clrCoreBaseAddress)
        clrCoreBaseAddress = PhLoadLibrary(L"mscoree.dll");
    if (!clrCoreBaseAddress)
        return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);

    if (MetaDataGetDispenser_I = reinterpret_cast<decltype(MetaDataGetDispenser_I)>(PhGetDllBaseProcedureAddress(clrCoreBaseAddress, const_cast<PSTR>("MetaDataGetDispenser"), 0)))
    {
        MetaDataGetDispenser_I(CLSID_CorMetaDataDispenser, IID_IMetaDataDispenser, reinterpret_cast<PVOID*>(&clrMetadataInterface));
    }

    if (!clrMetadataInterface)
    {
        PhGetClassObjectDllBase(clrCoreBaseAddress, CLSID_CorMetaDataDispenser, IID_IMetaDataDispenser, reinterpret_cast<PVOID*>(&clrMetadataInterface));
    }

    if (!clrMetadataInterface)
    {
        CLRCreateInstanceFnPtr CLRCreateInstance_I = nullptr;
        ICLRMetaHost* clrMetaHost = nullptr;
        ICLRRuntimeInfo* clrRuntimeInfo = nullptr;

        if (CLRCreateInstance_I = reinterpret_cast<decltype(CLRCreateInstance_I)>(PhGetDllBaseProcedureAddress(clrCoreBaseAddress, const_cast<PSTR>("CLRCreateInstance"), 0)))
        {
            status = CLRCreateInstance_I(
                CLSID_CLRMetaHost, 
                IID_ICLRMetaHost, 
                reinterpret_cast<PVOID*>(&clrMetaHost)
                );

            if (SUCCEEDED(status))
            {
                ULONG size = MAX_PATH;
                WCHAR version[MAX_PATH] = L"";

                status = clrMetaHost->GetVersionFromFile(FileName, version, &size);

                if (SUCCEEDED(status))
                {
                    status = clrMetaHost->GetRuntime(
                        version, 
                        IID_ICLRRuntimeInfo, 
                        reinterpret_cast<PVOID*>(&clrRuntimeInfo)
                        );

                    if (SUCCEEDED(status))
                    {
                        status = clrRuntimeInfo->GetInterface(
                            CLSID_CorMetaDataDispenser, 
                            IID_IMetaDataDispenser, 
                            reinterpret_cast<PVOID*>(&clrMetadataInterface)
                            );
                        clrRuntimeInfo->Release();
                    }
                }

                clrMetaHost->Release();
            }
        }
    }

    if (clrMetadataInterface)
    {
        *ClrCoreBaseAddress = clrCoreBaseAddress;
        *ClrMetaDataInterface = clrMetadataInterface;
        return S_OK;
    }

    if (clrCoreBaseAddress)
    {
        FreeLibrary(reinterpret_cast<HMODULE>(clrCoreBaseAddress));
    }

    return status;
}

HRESULT PvGetClrImageMetaDataTables(
    _Out_ IMetaDataImport** ClrMetaDataImport,
    _Out_ IMetaDataTables** ClrMetaDataTables
    )
{
    HRESULT status;
    PVOID clrCoreBaseAddress = nullptr;
    IMetaDataDispenser* metaDataDispenser;
    IMetaDataImport* metaDataImport = nullptr;
    IMetaDataTables* metaDataTables = nullptr;

    status = PvGetClrMetaDataInterface(
        PvFileName->Buffer, 
        &clrCoreBaseAddress, 
        &metaDataDispenser
        );

    if (!SUCCEEDED(status))
        return status;

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
            PvFileName->Buffer,
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

    metaDataDispenser->Release();

    *ClrMetaDataImport = metaDataImport;
    *ClrMetaDataTables = metaDataTables;
    return S_OK;
}

//HRESULT PvParseSerializedFrameworkDisplayName(
//    _In_ const void* Buffer,
//    _In_ ULONG BufferLength)
//{
//    ULONG offset = 0;
//    PCCOR_SIGNATURE data = (PCCOR_SIGNATURE)Buffer;
//    ULONG maybeCountOfArguments;
//    ULONG unknown;
//    ULONG serializedType;
//    ULONG valueType;
//    ULONG stringLengthDisplayName;
//    ULONG stringLengthDisplayNameValue;
//
//    offset += CorSigUncompressData(data, &maybeCountOfArguments);
//    data = (PCCOR_SIGNATURE)PTR_ADD_OFFSET(Buffer, offset);
//    if (offset > BufferLength)
//        return COR_E_OVERFLOW;
//
//    offset += CorSigUncompressData(data, &unknown);
//    data = (PCCOR_SIGNATURE)PTR_ADD_OFFSET(Buffer, offset);
//    if (offset > BufferLength)
//        return COR_E_OVERFLOW;
//
//    offset += CorSigUncompressData(data, &serializedType);
//    data = (PCCOR_SIGNATURE)PTR_ADD_OFFSET(Buffer, offset);
//    if (offset > BufferLength)
//        return COR_E_OVERFLOW;
//
//    if (serializedType != SERIALIZATION_TYPE_PROPERTY)
//        return META_E_CA_INVALID_BLOB;
//
//    offset += CorSigUncompressData(data, &valueType);
//    data = (PCCOR_SIGNATURE)PTR_ADD_OFFSET(Buffer, offset);
//    if (offset > BufferLength)
//        return COR_E_OVERFLOW;
//
//    if (valueType != ELEMENT_TYPE_STRING)
//        return META_E_CA_INVALID_BLOB;
//
//    offset += CorSigUncompressData(data, &stringLengthDisplayName);
//    data = (PCCOR_SIGNATURE)PTR_ADD_OFFSET(Buffer, offset);
//    if (offset > BufferLength)
//        return COR_E_OVERFLOW;
//
//    PPH_STRING fieldName = PhConvertUtf8ToUtf16Ex((PCHAR)data, stringLengthDisplayName);
//
//    if (PhEqualString2(fieldName, const_cast<PWSTR>(L"FrameworkDisplayName"), TRUE))
//    {
//        offset += stringLengthDisplayName;
//        data = (PCCOR_SIGNATURE)PTR_ADD_OFFSET(Buffer, offset);
//        if (offset > BufferLength)
//            return COR_E_OVERFLOW;
//
//        offset += CorSigUncompressData(data, &stringLengthDisplayNameValue);
//        data = (PCCOR_SIGNATURE)PTR_ADD_OFFSET(Buffer, offset);
//        if (offset > BufferLength)
//            return COR_E_OVERFLOW;
//
//        PPH_STRING displayname = PhConvertUtf8ToUtf16Ex((PCHAR)data, stringLengthDisplayNameValue);
//
//        offset += stringLengthDisplayNameValue;
//        data = (PCCOR_SIGNATURE)PTR_ADD_OFFSET(Buffer, offset);
//        if (offset > BufferLength)
//            return COR_E_OVERFLOW;
//
//        assert(offset == BufferLength);
//    }
//
//    return E_UNEXPECTED;
//}

HRESULT PvSafeParseAttributeString(
    _In_ const void* Buffer, 
    _In_ ULONG BufferLength, 
    _Out_ PPH_STRING* VersionString
    )
{
    ULONG stringLength = 0;
    ULONG skipLength = 0;
    ULONG offset = 0;
    PCCOR_SIGNATURE data;

    if (BufferLength < sizeof(USHORT) + sizeof(ULONG)) // prolog + length (1-4 compressed)
        return META_E_CA_INVALID_BLOB;

    USHORT prolog = *(PUSHORT)Buffer;
    if (prolog != 0x0001)
        return META_E_CA_INVALID_BLOB;

    offset += sizeof(USHORT);
    data = (PCCOR_SIGNATURE)PTR_ADD_OFFSET(Buffer, offset);
    if (offset > BufferLength)
        return COR_E_OVERFLOW;

    BYTE null = *(PBYTE)data;
    if (null == 0xFF) // 0xFF indicates NULL string
        return COR_E_OVERFLOW;

    if (FAILED(CorSigUncompressData(data, sizeof(ULONG), &stringLength, &skipLength)))
        return COR_E_OVERFLOW;
    if (stringLength >= BufferLength)
        return COR_E_OVERFLOW;

    offset += skipLength;
    data = (PCCOR_SIGNATURE)PTR_ADD_OFFSET(Buffer, offset);
    if (offset > BufferLength)
        return COR_E_OVERFLOW;

    *VersionString = PhConvertUtf8ToUtf16Ex((PCHAR)data, stringLength);

    //offset += stringLength;
    //data = (PCCOR_SIGNATURE)PTR_ADD_OFFSET(Buffer, offset);
    //if (offset > BufferLength)
    //    return COR_E_OVERFLOW;
    //
    //PvParseSerializedFrameworkDisplayName(data, BufferLength - offset);

    return S_OK;
}

EXTERN_C PPH_STRING PvGetClrImageTargetFramework(
    VOID
    )
{
    IMetaDataImport* metaDataImport = nullptr;
    IMetaDataTables* metaDataTables = nullptr;
    HRESULT status;
    PPH_STRING version = nullptr;
    const void* attributeBuffer;
    ULONG attributeLength;
    ULONG count;
    ULONG columns;

    status = PvGetClrImageMetaDataTables(&metaDataImport, &metaDataTables);

    if (status != S_OK)
        return nullptr;

    // 1) Query the version using the TargetFrameworkAttribute (dmex)

    if (metaDataImport->GetCustomAttributeByName(
        TokenFromRid(1, mdtAssembly),
        TARGET_FRAMEWORK_TYPE_W,
        &attributeBuffer,
        &attributeLength
        ) == S_OK)
    {
        if (PvSafeParseAttributeString(attributeBuffer, attributeLength, &version) == S_OK)
        {
            metaDataTables->Release();
            metaDataImport->Release();

            return version;
        }
    }

    // 2) Query the version using the assembly reference for System.Runtime or mscorlib (dmex)

    if (metaDataTables->GetTableInfo(TBL_AssemblyRef, nullptr, &count, &columns, nullptr, nullptr) == S_OK)
    {
        for (ULONG i = 1; i <= count; i++)
        {
            BOOLEAN runtime = FALSE;
            BOOLEAN framework = FALSE;
            ULONG index = 0;
            const char* name = nullptr;
            ULONG majorVersion = 0;
            ULONG minorVersion = 0;
            ULONG buildVersion = 0;
            ULONG revisionVersion = 0;

            if (metaDataTables->GetColumn(TBL_AssemblyRef, AssemblyRefRec_COL_Name, i, &index) == S_OK)
            {
                if (metaDataTables->GetString(index, &name) == S_OK)
                {
                    if (PhEqualBytesZ(const_cast<PSTR>(name), const_cast<PSTR>("System.Runtime"), TRUE))
                    {
                        // .NET Core
                        runtime = TRUE;
                    }
                    else if (PhEqualBytesZ(const_cast<PSTR>(name), const_cast<PSTR>("mscorlib"), TRUE))
                    {
                        // .NET Framework
                        framework = TRUE;
                    }
                }
            }

            if (runtime || framework)
            {
                if (metaDataTables->GetColumn(TBL_AssemblyRef, AssemblyRefRec_COL_MajorVersion, i, &majorVersion) != S_OK)
                    break;
                if (metaDataTables->GetColumn(TBL_AssemblyRef, AssemblyRefRec_COL_MinorVersion, i, &minorVersion) != S_OK)
                    break;
                if (metaDataTables->GetColumn(TBL_AssemblyRef, AssemblyRefRec_COL_BuildNumber, i, &buildVersion) != S_OK)
                    break;
                if (metaDataTables->GetColumn(TBL_AssemblyRef, AssemblyRefRec_COL_RevisionNumber, i, &revisionVersion) != S_OK)
                    break;

                if (runtime)
                {
                    version = PhFormatString(const_cast<PWSTR>(L".NET Core %hu.%hu.%hu.%hu"),
                        majorVersion, minorVersion, buildVersion, revisionVersion);
                    break;
                }
                else if (framework)
                {
                    version = PhFormatString(const_cast<PWSTR>(L".NET Framework %hu.%hu.%hu.%hu"),
                        majorVersion, minorVersion, buildVersion, revisionVersion);
                    break;
                }
            }
        }
    }

    metaDataTables->Release();
    metaDataImport->Release();

    return version;
}

// TODO: Add support for dynamic imports by enumerating the types. (dmex)
EXTERN_C HRESULT PvGetClrImageImports(
    _Out_ PPH_LIST* ClrImportsList
    )
{
    IMetaDataImport* metaDataImport = nullptr;
    IMetaDataTables* metaDataTables = nullptr;
    PPH_LIST clrImportsList;
    HRESULT status;
    ULONG rowModuleCount;
    ULONG rowImportCount;
    ULONG rowImportColumns;

    status = PvGetClrImageMetaDataTables(&metaDataImport, &metaDataTables);

    if (!SUCCEEDED(status))
        return status;

    clrImportsList = PhCreateList(64);

    // dummy unknown entry at index 0
    {
        PPV_CLR_IMAGE_IMPORT_DLL importDll;

        importDll = static_cast<PPV_CLR_IMAGE_IMPORT_DLL>(PhAllocateZero(sizeof(PV_CLR_IMAGE_IMPORT_DLL)));
        importDll->ImportName = PhCreateString(const_cast<wchar_t*>(L"Unknown"));
        importDll->ImportToken = ULONG_MAX;

        PhAddItemList(clrImportsList, importDll);
    }

    if (SUCCEEDED(metaDataTables->GetTableInfo(TBL_ModuleRef, nullptr, &rowModuleCount, nullptr, nullptr, nullptr)))
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

    *ClrImportsList = clrImportsList;
    return S_OK;
}

EXTERN_C HRESULT PvClrImageEnumTables(
    _In_ PPV_CLRTABLE_FUNCTION Callback,
    _In_ PVOID Context
    )
{
    IMetaDataImport* metaDataImport = nullptr;
    IMetaDataTables* metaDataTables = nullptr;
    HRESULT status;
    ULONG count;

    status = PvGetClrImageMetaDataTables(&metaDataImport, &metaDataTables);

    if (!SUCCEEDED(status))
        return status;

    status = metaDataTables->GetNumTables(&count);

    if (!SUCCEEDED(status))
    {
        metaDataTables->Release();
        return status;
    }

    for (ULONG i = 0; i < count; i++)
    {
        ULONG size;
        ULONG count;
        ULONG columns;
        ULONG key;
        const char* name;

        if (SUCCEEDED(metaDataTables->GetTableInfo(i, &size, &count, &columns, &key, &name)))
        {
            PPH_STRING tableName;
            PVOID offset = nullptr;

            tableName = PhConvertUtf8ToUtf16((PSTR)name);
            metaDataTables->GetRow(i, 1, &offset);

            if (!Callback(i, size, count, tableName, offset, Context))
            {
                PhDereferenceObject(tableName);
                break;
            }

            PhDereferenceObject(tableName);
        }
    }

    metaDataTables->Release();
    metaDataImport->Release();

    return S_OK;
}
