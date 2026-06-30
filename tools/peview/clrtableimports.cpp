/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2022-2026
 *
 */

#include <peview.h>
#include <mapclr.h>

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

#include <mapclr.h>
#else
#define _WINDOWS_UPDATES_
#include "metahost/corhdr.h"
#include "metahost/cor.h"
#endif

static int __cdecl PvClrCoreNameCompare(
    _In_ void* Context,
    _In_ void const* elem1,
    _In_ void const* elem2
    )
{
    PPH_STRING item1 = *static_cast<PPH_STRING const*>(elem1);
    PPH_STRING item2 = *static_cast<PPH_STRING const*>(elem2);

    return PhCompareStringWithNull(item1, item2, TRUE);
}

_Function_class_(PH_ENUM_DIRECTORY_FILE)
static BOOLEAN PvClrCoreDirectoryCallback(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_ PVOID Context
    )
{
    PFILE_DIRECTORY_INFORMATION directoryInfo = static_cast<PFILE_DIRECTORY_INFORMATION>(Information);
    PH_STRINGREF baseName = { directoryInfo->FileNameLength, directoryInfo->FileName };

    if (FlagOn(directoryInfo->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
    {
        if (PATH_IS_WIN32_RELATIVE_PREFIX(&baseName))
            return TRUE;

        PhAddItemList(static_cast<PPH_LIST>(Context), PhCreateString2(&baseName));
    }

    return TRUE;
}

HRESULT PvGetClrMetaDataInterface(
    _In_ PWSTR FileName,
    _Out_ PVOID* ClrCoreBaseAddress,
    _Out_ IMetaDataDispenser** ClrMetaDataInterface
    )
{
    static CONST PH_STRINGREF dotNetCorePath = PH_STRINGREF_INIT(L"%ProgramFiles%\\dotnet\\shared\\Microsoft.NETCore.App\\");
    static CONST PH_STRINGREF dotNetCoreName = PH_STRINGREF_INIT(L"\\coreclr.dll");
    HRESULT (WINAPI* MetaDataGetDispenser_I)(_In_ REFCLSID rclsid, _In_ REFIID riid, _COM_Outptr_ PVOID* ppv) = nullptr;
    HRESULT status = E_FAIL;
    PVOID clrCoreBaseAddress = nullptr;
    IMetaDataDispenser* clrMetadataInterface = nullptr;
    PPH_STRING directoryPath;
    HANDLE directoryHandle;

    if (directoryPath = PhExpandEnvironmentStrings(&dotNetCorePath))
    {
        if (PhDoesDirectoryExistWin32(PhGetString(directoryPath)))
        {
            PPH_LIST directoryList = PhCreateList(2);

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

            if (directoryList->Count > 0)
            {
                PPH_STRING directoryName = static_cast<PPH_STRING>(directoryList->Items[directoryList->Count - 1]);
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
                    PPH_STRING directoryName = static_cast<PPH_STRING>(directoryList->Items[i]);
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

    if (MetaDataGetDispenser_I = reinterpret_cast<decltype(MetaDataGetDispenser_I)>(PhGetDllBaseProcedureAddress(clrCoreBaseAddress, "MetaDataGetDispenser", 0)))
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

        if (CLRCreateInstance_I = reinterpret_cast<decltype(CLRCreateInstance_I)>(PhGetDllBaseProcedureAddress(clrCoreBaseAddress, "CLRCreateInstance", 0)))
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
        PhFreeLibrary(clrCoreBaseAddress);
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
        static_cast<ULONG>(PvMappedImage.ViewSize),
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

    USHORT prolog = *static_cast<USHORT const*>(Buffer);
    if (prolog != 0x0001)
        return META_E_CA_INVALID_BLOB;

    offset += sizeof(USHORT);
    data = static_cast<PCCOR_SIGNATURE>(PTR_ADD_OFFSET(const_cast<PVOID>(Buffer), offset));
    if (offset > BufferLength)
        return COR_E_OVERFLOW;

    BYTE null = *const_cast<PBYTE>(data);
    if (null == 0xFF) // 0xFF indicates NULL string
        return COR_E_OVERFLOW;

    if (FAILED(CorSigUncompressData(data, sizeof(ULONG), &stringLength, &skipLength)))
        return COR_E_OVERFLOW;
    if (stringLength >= BufferLength)
        return COR_E_OVERFLOW;

    offset += skipLength;
    data = static_cast<PCCOR_SIGNATURE>(PTR_ADD_OFFSET(const_cast<PVOID>(Buffer), offset));
    if (offset > BufferLength)
        return COR_E_OVERFLOW;

    *VersionString = PhConvertUtf8ToUtf16Ex(reinterpret_cast<char*>(const_cast<unsigned char*>(data)), stringLength);

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
    PH_MAPPED_CLR_METADATA clrMetadata;
    PPH_STRING version = nullptr;
    NTSTATUS status;

    status = PhInitializeMappedClrMetadata(&clrMetadata, &PvMappedImage);

    if (NT_SUCCESS(status))
    {
        if (PhTryGetMappedClrTargetFramework(&clrMetadata, &version))
        {
            PhDeleteMappedClrMetadata(&clrMetadata);
            return version;
        }

        ULONG assemblyRefCount = 0;

        if (NT_SUCCESS(PhGetMappedClrTableInfoEx(&clrMetadata, PH_CLR_TABLE_ASSEMBLYREF, nullptr, &assemblyRefCount, nullptr, nullptr, nullptr)))
        {
            for (ULONG i = 1; i <= assemblyRefCount; i++)
            {
                PPH_STRING name = nullptr;
                BOOLEAN runtime = FALSE;
                BOOLEAN framework = FALSE;
                ULONG majorVersion = 0;
                ULONG minorVersion = 0;
                ULONG buildVersion = 0;
                ULONG revisionVersion = 0;

                if (name = PhGetMappedClrTableString(&clrMetadata, PH_CLR_TABLE_ASSEMBLYREF, i, PH_CLR_ASSEMBLYREF_REC_COL_NAME))
                {
                    if (PhEqualString2(name, L"System.Runtime", TRUE))
                    {
                        runtime = TRUE;
                    }
                    else if (PhEqualString2(name, L"mscorlib", TRUE))
                    {
                        framework = TRUE;
                    }

                    PhDereferenceObject(name);
                }

                if (runtime || framework)
                {
                    if (!NT_SUCCESS(PhGetMappedClrColumnValue(&clrMetadata, PH_CLR_TABLE_ASSEMBLYREF, PH_CLR_ASSEMBLYREF_REC_COL_MAJORVERSION, i, &majorVersion)))
                        break;
                    if (!NT_SUCCESS(PhGetMappedClrColumnValue(&clrMetadata, PH_CLR_TABLE_ASSEMBLYREF, PH_CLR_ASSEMBLYREF_REC_COL_MINORVERSION, i, &minorVersion)))
                        break;
                    if (!NT_SUCCESS(PhGetMappedClrColumnValue(&clrMetadata, PH_CLR_TABLE_ASSEMBLYREF, PH_CLR_ASSEMBLYREF_REC_COL_BUILDNUMBER, i, &buildVersion)))
                        break;
                    if (!NT_SUCCESS(PhGetMappedClrColumnValue(&clrMetadata, PH_CLR_TABLE_ASSEMBLYREF, PH_CLR_ASSEMBLYREF_REC_COL_REVISIONNUMBER, i, &revisionVersion)))
                        break;

                    if (runtime)
                    {
                        version = PhFormatString(L".NET Core %lu.%lu.%lu.%lu", majorVersion, minorVersion, buildVersion, revisionVersion);
                        break;
                    }
                    else if (framework)
                    {
                        version = PhFormatString(L".NET Framework %lu.%lu.%lu.%lu", majorVersion, minorVersion, buildVersion, revisionVersion);
                        break;
                    }
                }
            }
        }
        PhDeleteMappedClrMetadata(&clrMetadata);
    }

    if (version)
        return version;

    IMetaDataImport* metaDataImport = nullptr;
    IMetaDataTables* metaDataTables = nullptr;
    HRESULT comStatus;
    const void* attributeBuffer;
    ULONG attributeLength;
    ULONG count;
    ULONG columns;

    comStatus = PvGetClrImageMetaDataTables(&metaDataImport, &metaDataTables);

    if (comStatus != S_OK)
        return nullptr;

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

    if (metaDataTables->GetTableInfo(PH_CLR_TABLE_ASSEMBLYREF, nullptr, &count, &columns, nullptr, nullptr) == S_OK)
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

            if (metaDataTables->GetColumn(PH_CLR_TABLE_ASSEMBLYREF, PH_CLR_ASSEMBLYREF_REC_COL_NAME, i, &index) == S_OK)
            {
                if (metaDataTables->GetString(index, &name) == S_OK)
                {
                    if (PhEqualBytesZ(name, "System.Runtime", TRUE))
                        runtime = TRUE;
                    else if (PhEqualBytesZ(name, "mscorlib", TRUE))
                        framework = TRUE;
                }
            }

            if (runtime || framework)
            {
                if (metaDataTables->GetColumn(PH_CLR_TABLE_ASSEMBLYREF, PH_CLR_ASSEMBLYREF_REC_COL_MAJORVERSION, i, &majorVersion) != S_OK)
                    break;
                if (metaDataTables->GetColumn(PH_CLR_TABLE_ASSEMBLYREF, PH_CLR_ASSEMBLYREF_REC_COL_MINORVERSION, i, &minorVersion) != S_OK)
                    break;
                if (metaDataTables->GetColumn(PH_CLR_TABLE_ASSEMBLYREF, PH_CLR_ASSEMBLYREF_REC_COL_BUILDNUMBER, i, &buildVersion) != S_OK)
                    break;
                if (metaDataTables->GetColumn(PH_CLR_TABLE_ASSEMBLYREF, PH_CLR_ASSEMBLYREF_REC_COL_REVISIONNUMBER, i, &revisionVersion) != S_OK)
                    break;

                if (runtime)
                {
                    version = PhFormatString(L".NET Core %lu.%lu.%lu.%lu", majorVersion, minorVersion, buildVersion, revisionVersion);
                    break;
                }
                else if (framework)
                {
                    version = PhFormatString(L".NET Framework %lu.%lu.%lu.%lu", majorVersion, minorVersion, buildVersion, revisionVersion);
                    break;
                }
            }
        }
    }

    metaDataTables->Release();
    metaDataImport->Release();

    return version;
}

EXTERN_C HRESULT PvGetClrImageImports(
    _Out_ PPH_LIST* ClrImportsList
    )
{
    PPH_LIST clrImportsList;
    PH_MAPPED_CLR_METADATA clrMetadata;
    NTSTATUS status;

    clrImportsList = PhCreateList(64);

    // dummy unknown entry at index 0
    {
        PPV_CLR_IMAGE_IMPORT_DLL importDll;

        importDll = static_cast<PPV_CLR_IMAGE_IMPORT_DLL>(PhAllocateZero(sizeof(PV_CLR_IMAGE_IMPORT_DLL)));
        importDll->ImportName = PhCreateString(L"Unknown");
        importDll->ImportToken = ULONG_MAX;

        PhAddItemList(clrImportsList, importDll);
    }

    status = PhInitializeMappedClrMetadata(&clrMetadata, &PvMappedImage);

    if (!NT_SUCCESS(status))
    {
        *ClrImportsList = clrImportsList;
        return S_OK;
    }

    {
        ULONG moduleRefCount = 0;

        if (NT_SUCCESS(PhGetMappedClrTableInfoEx(&clrMetadata, PH_CLR_TABLE_MODULEREF, nullptr, &moduleRefCount, nullptr, nullptr, nullptr)))
        {
            for (ULONG i = 1; i <= moduleRefCount; i++)
            {
                PPH_STRING moduleName = nullptr;

                if (moduleName = PhGetMappedClrTableString(&clrMetadata, PH_CLR_TABLE_MODULEREF, i, PH_CLR_MODULEREF_REC_COL_NAME))
                {
                    PPV_CLR_IMAGE_IMPORT_DLL importDll;

                    importDll = static_cast<PPV_CLR_IMAGE_IMPORT_DLL>(PhAllocateZero(sizeof(PV_CLR_IMAGE_IMPORT_DLL)));
                    importDll->ImportName = moduleName;
                    importDll->ImportToken = TokenFromRid(i, mdtModuleRef);

                    PhAddItemList(clrImportsList, importDll);
                }
            }
        }
    }

    {
        ULONG implMapCount = 0;

        if (NT_SUCCESS(PhGetMappedClrTableInfoEx(&clrMetadata, PH_CLR_TABLE_IMPLMAP, nullptr, &implMapCount, nullptr, nullptr, nullptr)))
        {
            for (ULONG i = 1; i <= implMapCount; i++)
            {
                BOOLEAN found = false;
                ULONG importFlagsValue = 0;
                ULONG importNameValue = 0;
                ULONG moduleTokenValue = 0;
                PVOID importRowValue = nullptr;
                PVOID importOffsetValue = nullptr;
                PPH_STRING importName = nullptr;

                if (NT_SUCCESS(PhGetMappedClrColumnValue(&clrMetadata, PH_CLR_TABLE_IMPLMAP, PH_CLR_IMPLMAP_REC_COL_MAPPINGFLAGS, i, &importFlagsValue)))
                {
                    // continue
                }

                if (NT_SUCCESS(PhGetMappedClrColumnValue(&clrMetadata, PH_CLR_TABLE_IMPLMAP, PH_CLR_IMPLMAP_REC_COL_IMPORTNAME, i, &importNameValue)))
                {
                    const char* importNameA = nullptr;

                    if (NT_SUCCESS(PhGetMappedClrString(&clrMetadata, importNameValue, &importNameA)))
                        importName = PhConvertUtf8ToUtf16(importNameA);
                }

                if (NT_SUCCESS(PhGetMappedClrColumnValue(&clrMetadata, PH_CLR_TABLE_IMPLMAP, PH_CLR_IMPLMAP_REC_COL_IMPORTSCOPE, i, &moduleTokenValue)))
                {
                    moduleTokenValue = TokenFromRid(moduleTokenValue, mdtModuleRef);
                }
                else
                    moduleTokenValue = ULONG_MAX;

                if (NT_SUCCESS(PhGetMappedClrTableRow(&clrMetadata, PH_CLR_TABLE_IMPLMAP, i, &importRowValue)))
                    importOffsetValue = PTR_SUB_OFFSET(importRowValue, reinterpret_cast<ULONG_PTR>(PvMappedImage.ViewBase));

                for (ULONG j = 0; j < clrImportsList->Count; j++)
                {
                    PPV_CLR_IMAGE_IMPORT_DLL importDll = static_cast<PPV_CLR_IMAGE_IMPORT_DLL>(clrImportsList->Items[j]);

                    if (importDll->ImportToken == moduleTokenValue)
                    {
                        if (!importDll->Functions)
                            importDll->Functions = PhCreateList(1);
                        if (!importName)
                            importName = PhCreateString(L"Unknown");

                        if (importDll->Functions)
                        {
                            PPV_CLR_IMAGE_IMPORT_FUNCTION importFunction;

                            importFunction = static_cast<PPV_CLR_IMAGE_IMPORT_FUNCTION>(PhAllocateZero(sizeof(PV_CLR_IMAGE_IMPORT_FUNCTION)));
                            importFunction->FunctionName = importName;
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
                        importName = PhCreateString(L"Unknown");

                    if (unknownImportDll->Functions)
                    {
                        PPV_CLR_IMAGE_IMPORT_FUNCTION importFunction;

                        importFunction = static_cast<PPV_CLR_IMAGE_IMPORT_FUNCTION>(PhAllocateZero(sizeof(PV_CLR_IMAGE_IMPORT_FUNCTION)));
                        importFunction->FunctionName = importName;
                        importFunction->Flags = importFlagsValue;
                        importFunction->Offset = importOffsetValue;

                        PhAddItemList(unknownImportDll->Functions, importFunction);
                    }
                }
            }
        }
    }

    PhDeleteMappedClrMetadata(&clrMetadata);

    *ClrImportsList = clrImportsList;
    return S_OK;
}

EXTERN_C HRESULT PvClrImageEnumTables(
    _In_ PPV_CLRTABLE_FUNCTION Callback,
    _In_ PVOID Context
    )
{
    PH_MAPPED_CLR_METADATA clrMetadata;
    NTSTATUS status = PhInitializeMappedClrMetadata(&clrMetadata, &PvMappedImage);
    PPH_MAPPED_CLR_TABLE table = nullptr;

    if (!NT_SUCCESS(status))
        return HRESULT_FROM_NT(status);

    for (ULONG i = 0; i < PH_CLR_TABLE_MAXIMUM; i++)
    {
        PPH_STRING tableName = nullptr;
        ULONG size = 0;
        ULONG tablecount = 0;

        if (!NT_SUCCESS(PhGetMappedClrTableInfoEx(&clrMetadata, i, &size, &tablecount, nullptr, nullptr, &table)))
            continue;

        if (!table->Name)
            continue;

        tableName = PhConvertUtf8ToUtf16(table->Name);

        if (!Callback(i, size, tablecount, tableName, table->Rows, Context))
        {
            PhDereferenceObject(tableName);
            break;
        }

        PhDereferenceObject(tableName);
    }

    PhDeleteMappedClrMetadata(&clrMetadata);

    return S_OK;
}
