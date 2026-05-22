/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2022
 *
 */

#include <peview.h>
#include <symprv.h>

#include <dbghelp.h>
#ifndef COM_NO_WINDOWS_H
#define COM_NO_WINDOWS_H 1
#endif
#include <thirdparty.h>

ULONG SearchResultsAddIndex = 0;
PPH_LIST SearchResults = NULL;
PH_QUEUED_LOCK SearchResultsLock = PH_QUEUED_LOCK_INIT;

BOOLEAN PePdbShouldUndecorateSymbol(
    _In_ ULONG SymbolTag
    );

BOOLEAN PePdbShouldExpandDiaSymbol(
    _In_ ULONG SymbolTag
    );

VOID PePdbPrintDiaSymbol(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSymbol* DiaSymbol,
    _In_opt_ PPV_SYMBOL_NODE Parent
    );

PPH_STRINGREF rgBaseType[] =
{
    SREF(L"<NoType>"),                         // btNoType = 0,
    SREF(L"void"),                             // btVoid = 1,
    SREF(L"char"),                             // btChar = 2,
    SREF(L"wchar_t"),                          // btWChar = 3,
    SREF(L"signed char"),
    SREF(L"unsigned char"),
    SREF(L"int"),                              // btInt = 6,
    SREF(L"unsigned int"),                     // btUInt = 7,
    SREF(L"float"),                            // btFloat = 8,
    SREF(L"<BCD>"),                            // btBCD = 9,
    SREF(L"bool"),                             // btBool = 10,
    SREF(L"short"),
    SREF(L"unsigned short"),
    SREF(L"long"),                             // btLong = 13,
    SREF(L"unsigned long"),                    // btULong = 14,
    SREF(L"__int8"),
    SREF(L"__int16"),
    SREF(L"__int32"),
    SREF(L"__int64"),
    SREF(L"__int128"),
    SREF(L"unsigned __int8"),
    SREF(L"unsigned __int16"),
    SREF(L"unsigned __int32"),
    SREF(L"unsigned __int64"),
    SREF(L"unsigned __int128"),
    SREF(L"<currency>"),                       // btCurrency = 25,
    SREF(L"<date>"),                           // btDate = 26,
    SREF(L"VARIANT"),                          // btVariant = 27,
    SREF(L"<complex>"),                        // btComplex = 28,
    SREF(L"<bit>"),                            // btBit = 29,
    SREF(L"BSTR"),                             // btBSTR = 30,
    SREF(L"HRESULT"),                          // btHresult = 31
    SREF(L"char16_t"),                         // btChar16 = 32
    SREF(L"char32_t"),                         // btChar32 = 33
    SREF(L"char8_t")                           // btChar8  = 34
};

PPH_STRINGREF rgTags[] =
{
    SREF(L"(SymTagNull)"),                     // SymTagNull
    SREF(L"Executable (Global)"),              // SymTagExe
    SREF(L"Compiland"),                        // SymTagCompiland
    SREF(L"CompilandDetails"),                 // SymTagCompilandDetails
    SREF(L"CompilandEnv"),                     // SymTagCompilandEnv
    SREF(L"Function"),                         // SymTagFunction
    SREF(L"Block"),                            // SymTagBlock
    SREF(L"Data"),                             // SymTagData
    SREF(L"Annotation"),                       // SymTagAnnotation
    SREF(L"Label"),                            // SymTagLabel
    SREF(L"PublicSymbol"),                     // SymTagPublicSymbol
    SREF(L"UserDefinedType"),                  // SymTagUDT
    SREF(L"Enum"),                             // SymTagEnum
    SREF(L"FunctionType"),                     // SymTagFunctionType
    SREF(L"PointerType"),                      // SymTagPointerType
    SREF(L"ArrayType"),                        // SymTagArrayType
    SREF(L"BaseType"),                         // SymTagBaseType
    SREF(L"Typedef"),                          // SymTagTypedef
    SREF(L"BaseClass"),                        // SymTagBaseClass
    SREF(L"Friend"),                           // SymTagFriend
    SREF(L"FunctionArgType"),                  // SymTagFunctionArgType
    SREF(L"FuncDebugStart"),                   // SymTagFuncDebugStart
    SREF(L"FuncDebugEnd"),                     // SymTagFuncDebugEnd
    SREF(L"UsingNamespace"),                   // SymTagUsingNamespace
    SREF(L"VTableShape"),                      // SymTagVTableShape
    SREF(L"VTable"),                           // SymTagVTable
    SREF(L"Custom"),                           // SymTagCustom
    SREF(L"Thunk"),                            // SymTagThunk
    SREF(L"CustomType"),                       // SymTagCustomType
    SREF(L"ManagedType"),                      // SymTagManagedType
    SREF(L"Dimension"),                        // SymTagDimension
    SREF(L"CallSite"),                         // SymTagCallSite
    SREF(L"InlineSite"),                       // SymTagInlineSite
    SREF(L"BaseInterface"),                    // SymTagBaseInterface
    SREF(L"VectorType"),                       // SymTagVectorType
    SREF(L"MatrixType"),                       // SymTagMatrixType
    SREF(L"HLSLType"),                         // SymTagHLSLType
    SREF(L"Caller"),                           // SymTagCaller
    SREF(L"Callee"),                           // SymTagCallee
    SREF(L"Export"),                           // SymTagExport
    SREF(L"HeapAllocationSite"),               // SymTagHeapAllocationSite
    SREF(L"CoffGroup"),                        // SymTagCoffGroup
    SREF(L"Inlinee"),                          // SymTagInlinee
};

PPH_STRINGREF rgLocationTypeString[] =
{
    SREF(L"NULL"),
    SREF(L"static"),
    SREF(L"TLS"),
    SREF(L"RegRel"),
    SREF(L"ThisRel"),
    SREF(L"Enregistered"),
    SREF(L"BitField"),
    SREF(L"Slot"),
    SREF(L"IL Relative"),
    SREF(L"In MetaData"),
    SREF(L"Constant"),
    SREF(L"RegRelAliasIndir")
};

PPH_STRINGREF rgUdtKind[] =
{
    SREF(L"struct"),
    SREF(L"class"),
    SREF(L"union"),
    SREF(L"interface"),
};

PPH_STRINGREF rgDataKind[] =
{
    SREF(L"Unknown"),
    SREF(L"Local"),
    SREF(L"Static Local"),
    SREF(L"Param"),
    SREF(L"Object Ptr"),
    SREF(L"File Static"),
    SREF(L"Global"),
    SREF(L"Member"),
    SREF(L"Static Member"),
    SREF(L"Constant"),
};

PPH_STRINGREF rgLanguage[] =
{
    SREF(L"C"),
    SREF(L"C++"),
    SREF(L"Fortran"),
    SREF(L"Masm"),
    SREF(L"Pascal"),
    SREF(L"Basic"),
    SREF(L"Cobol"),
    SREF(L"Link"),
    SREF(L"Cvtres"),
    SREF(L"Cvtpgd"),
    SREF(L"C#"),
    SREF(L"Visual Basic"),
    SREF(L"ILasm"),
    SREF(L"Java"),
    SREF(L"JScript"),
    SREF(L"MSIL"),
    SREF(L"HLSL"),
    SREF(L"Objective-C"),
    SREF(L"Objective-C++"),
    SREF(L"Swift"),
    SREF(L"AliasObj"),
    SREF(L"Rust"),
};

#define PV_DIA_MAX_TREE_DEPTH 16

DEFINE_GUID(IID_IDiaSymbol, 0xcb787b2f, 0xbd6c, 0x4635, 0xba, 0x52, 0x93, 0x31, 0x26, 0xbd, 0x2d, 0xcd);
DEFINE_GUID(IID_IDiaSymbol3, 0x99b665f7, 0xc1b2, 0x49d3, 0x89, 0xb2, 0xa3, 0x84, 0x36, 0x1a, 0xca, 0xb5);
DEFINE_GUID(IID_IDiaSymbol4, 0xbf6c88a7, 0xe9d6, 0x4346, 0x99, 0xa1, 0xd0, 0x53, 0xde, 0x5a, 0x78, 0x08);
DEFINE_GUID(IID_IDiaSymbol7, 0x64ce6cd5, 0x7315, 0x4328, 0x86, 0xd6, 0x10, 0xe3, 0x03, 0xe0, 0x10, 0xb4);
DEFINE_GUID(IID_IDiaSymbol9, 0xa89e5969, 0x92a1, 0x4f8a, 0xb7, 0x04, 0x00, 0x12, 0x1c, 0x37, 0xab, 0xbb);
DEFINE_GUID(IID_IDiaSymbol10, 0x9034a70b, 0xb0b7, 0x4605, 0x8a, 0x97, 0x33, 0x77, 0x2f, 0x3a, 0x7b, 0x8c);
DEFINE_GUID(IID_IDiaSymbol11, 0xb6f54fcd, 0x05e3, 0x433d, 0xb3, 0x05, 0xb0, 0xc1, 0x43, 0x7d, 0x2d, 0x16);
DEFINE_GUID(IID_IDiaEnumSourceFiles, 0x10f3dbd9, 0x664f, 0x4469, 0xb8, 0x08, 0x94, 0x71, 0xc7, 0xa5, 0x05, 0x38);
DEFINE_GUID(IID_IDiaEnumSegments, 0xe8368ca9, 0x01d1, 0x419d, 0xac, 0x0c, 0xe3, 0x12, 0x35, 0xdb, 0xda, 0x9f);
DEFINE_GUID(IID_IDiaEnumSectionContribs, 0x1994deb2, 0x2c82, 0x4b1d, 0xa5, 0x7f, 0xaf, 0xf4, 0x24, 0xd5, 0x4a, 0x68);
DEFINE_GUID(IID_IDiaEnumFrameData, 0x9fc77a4b, 0x3c1c, 0x44ed, 0xa7, 0x98, 0x6c, 0x1d, 0xee, 0xa5, 0x3e, 0x1f);
DEFINE_GUID(IID_IDiaEnumDebugStreamData, 0x486943e8, 0xd187, 0x4a6b, 0xa3, 0xc4, 0x29, 0x12, 0x59, 0xff, 0xf6, 0x0d);

VOID PrintSymbolType(
    _In_ PPH_STRING_BUILDER StringBuilder,
    _In_ IDiaSymbol* Symbol
    );

VOID PrintSymTag(
    _In_ PPH_STRING_BUILDER StringBuilder,
    _In_ ULONG SymbolTag
    )
{
    PhAppendStringBuilder(StringBuilder, rgTags[SymbolTag]);
    PhAppendStringBuilder2(StringBuilder, L": ");
}

VOID PrintVariant(
    _In_ PPH_STRING_BUILDER StringBuilder,
    _In_ VARIANT* var)
{
    switch (V_VT(var))
    {
    case VT_UI1:
    case VT_I1:
        PhAppendFormatStringBuilder(StringBuilder, L" 0x%X", V_UI1(var));
        break;
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        PhAppendFormatStringBuilder(StringBuilder, L" 0x%X", V_I2(var));
        break;
    case VT_I4:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:
        PhAppendFormatStringBuilder(StringBuilder, L" 0x%X", V_I4(var));
        break;
    case VT_R4:
        PhAppendFormatStringBuilder(StringBuilder, L" %g", V_R4(var));
        break;
    case VT_R8:
        PhAppendFormatStringBuilder(StringBuilder, L" %g", V_R8(var));
        break;
    case VT_BSTR:
        PhAppendFormatStringBuilder(StringBuilder, L" \"%s\"", V_BSTR(var));
        break;
    default:
        PhAppendStringBuilder2(StringBuilder, L" ??");
    }
}

VOID PrintLocation(
    _In_ PPH_STRING_BUILDER StringBuilder,
    _In_ IDiaSymbol* IDiaSymbol)
{
    ULONG dwLocType;
    ULONG dwRVA;
    ULONG dwSect;
    ULONG dwOff;
    ULONG dwReg;
    ULONG dwBitPos;
    ULONG dwSlot;
    LONG lOffset;
    ULONGLONG ulLen;
    VARIANT vt = { VT_EMPTY };

    if (IDiaSymbol_get_locationType(IDiaSymbol, &dwLocType) != S_OK)
    {
        PhAppendFormatStringBuilder(StringBuilder, L"symbol in optimized code");
        return;
    }

    switch (dwLocType)
    {
    case LocIsStatic:
        if ((IDiaSymbol_get_relativeVirtualAddress(IDiaSymbol, &dwRVA) == S_OK) &&
            (IDiaSymbol_get_addressSection(IDiaSymbol, &dwSect) == S_OK) &&
            (IDiaSymbol_get_addressOffset(IDiaSymbol, &dwOff) == S_OK))
        {
            PhAppendStringBuilder(StringBuilder,rgLocationTypeString[dwLocType]);
            PhAppendFormatStringBuilder(StringBuilder, L", [%08X][%04X:%08X]", dwRVA, dwSect, dwOff);
        }
        break;

    case LocIsTLS:
    case LocInMetaData:
    case LocIsIlRel:
        if ((IDiaSymbol_get_relativeVirtualAddress(IDiaSymbol, &dwRVA) == S_OK) &&
            (IDiaSymbol_get_addressSection(IDiaSymbol, &dwSect) == S_OK) &&
            (IDiaSymbol_get_addressOffset(IDiaSymbol, &dwOff) == S_OK))
        {
            PhAppendStringBuilder(StringBuilder, rgLocationTypeString[dwLocType]);
            PhAppendFormatStringBuilder(StringBuilder, L", [%08X][%04X:%08X]", dwRVA, dwSect, dwOff);
        }
        break;

    case LocIsRegRel:
        if ((IDiaSymbol_get_registerId(IDiaSymbol, &dwReg) == S_OK) &&
            (IDiaSymbol_get_offset(IDiaSymbol, &lOffset) == S_OK))
        {
            //PhAppendFormatStringBuilder(StringBuilder, L"%s Relative, [%08X]", SzNameC7Reg((USHORT)dwReg), lOffset);
        }
        break;

    case LocIsThisRel:
        if (IDiaSymbol_get_offset(IDiaSymbol, &lOffset) == S_OK) {
            PhAppendFormatStringBuilder(StringBuilder, L"this+0x%X", lOffset);
        }
        break;

    case LocIsBitField:
        if ((IDiaSymbol_get_offset(IDiaSymbol, &lOffset) == S_OK) &&
            (IDiaSymbol_get_bitPosition(IDiaSymbol, &dwBitPos) == S_OK) &&
            (IDiaSymbol_get_length(IDiaSymbol, &ulLen) == S_OK)) {
            PhAppendFormatStringBuilder(StringBuilder, L"this(bf)+0x%X:0x%X len(0x%X)", lOffset, dwBitPos, (ULONG)ulLen);
        }
        break;

    case LocIsEnregistered:
        {
            if (IDiaSymbol_get_registerId(IDiaSymbol, &dwReg) == S_OK)
            {
                //PhAppendFormatStringBuilder(StringBuilder, L"enregistered %s", SzNameC7Reg((USHORT)dwReg));
            }
        }
        break;
    case LocIsSlot:
        {
            if (IDiaSymbol_get_slot(IDiaSymbol, &dwSlot) == S_OK)
            {
                PhAppendStringBuilder(StringBuilder, rgLocationTypeString[dwLocType]);
                PhAppendFormatStringBuilder(StringBuilder, L", [%08X]", dwSlot);
            }
        }
        break;
    case LocIsConstant:
        {
            //PhAppendStringBuilder2(StringBuilder, L"constant");

            VariantInit(&vt);

            if (IDiaSymbol_get_value(IDiaSymbol, &vt) == S_OK)
            {
                PrintVariant(StringBuilder, &vt);

                if (vt.vt != VT_EMPTY)
                    VariantClear((VARIANTARG*)&vt);
            }
        }
        break;
    case LocIsNull:
        break;
    default:
        PhAppendFormatStringBuilder(StringBuilder, L"Error - invalid location type: 0x%X", dwLocType);
        break;
    }
}

VOID PrintBool(
    _In_ PPH_STRING_BUILDER StringBuilder,
    _In_ PWSTR Name,
    _In_ BOOL Value
    )
{
    if (Value)
    {
        PhAppendStringBuilder2(StringBuilder, Name);
        PhAppendStringBuilder2(StringBuilder, L": Yes\r\n");
    }
}

VOID PrintHex(
    _In_ PPH_STRING_BUILDER StringBuilder,
    _In_ PWSTR Name,
    _In_ ULONG64 Value
    )
{
    if (Value != 0)
    {
        PhAppendStringBuilder2(StringBuilder, Name);
        PhAppendFormatStringBuilder(StringBuilder, L": 0x%llx\r\n", Value);
    }
}

VOID PrintBSTR(
    _In_ PPH_STRING_BUILDER StringBuilder,
    _In_ PWSTR Name,
    _In_ BSTR Value
    )
{
    if (Value && *Value)
    {
        PhAppendStringBuilder2(StringBuilder, Name);
        PhAppendStringBuilder2(StringBuilder, L": ");
        PhAppendStringBuilder2(StringBuilder, Value);
        PhAppendStringBuilder2(StringBuilder, L"\r\n");
    }
}

VOID PrintName(
    _In_ PPH_STRING_BUILDER StringBuilder,
    _In_ IDiaSymbol* pSymbol
    )
{
    BSTR bstrName;
    BSTR bstrUndName;

    if (IDiaSymbol_get_name(pSymbol, &bstrName) != S_OK)
    {
        PhAppendFormatStringBuilder(StringBuilder, L"(none)");
        return;
    }

    if (IDiaSymbol_get_undecoratedName(pSymbol, &bstrUndName) == S_OK)
    {
        if (wcscmp(bstrName, bstrUndName) == 0)
        {
            PhAppendStringBuilder2(StringBuilder, bstrName);
        }
        else
        {
            PhAppendStringBuilder2(StringBuilder, bstrUndName);
            PhAppendFormatStringBuilder(StringBuilder, L"(%s)", bstrName);
        }

        PhSymbolProviderFreeDiaString(bstrUndName);
    }
    else
    {
        PhAppendStringBuilder2(StringBuilder, bstrName);
    }

    PhSymbolProviderFreeDiaString(bstrName);
}

VOID PrintData(
    _In_ PPH_STRING_BUILDER StringBuilder,
    _In_ IDiaSymbol* IDiaSymbol
    )
{
    ULONG dwDataKind;

    //PrintLocation(StringBuilder, IDiaSymbol);

    if (IDiaSymbol_get_dataKind(IDiaSymbol, &dwDataKind) != S_OK)
    {
        PhAppendFormatStringBuilder(StringBuilder, L"ERROR - PrintData() get_dataKind");
        return;
    }

    PhAppendStringBuilder(StringBuilder, rgDataKind[dwDataKind]);
    PrintSymbolType(StringBuilder, IDiaSymbol);

    PhAppendStringBuilder2(StringBuilder, L", ");
    PrintName(StringBuilder, IDiaSymbol);

    PhAppendStringBuilder2(StringBuilder, L" = ");
    PrintLocation(StringBuilder, IDiaSymbol);
}

VOID PrintUdtKind(
    _In_ PPH_STRING_BUILDER StringBuilder,
    _In_ IDiaSymbol* pSymbol
    )
{
    ULONG dwKind = 0;

    if (IDiaSymbol_get_udtKind(pSymbol, &dwKind) == S_OK)
    {
        PhAppendStringBuilder(StringBuilder, rgUdtKind[dwKind]);
        PhAppendStringBuilder2(StringBuilder, L" ");
    }
}

VOID PrintType(
    _In_ PPH_STRING_BUILDER StringBuilder,
    _In_ IDiaSymbol* pSymbol)
{
    //IDiaSymbol* pBaseType;
    //IDiaEnumSymbols* pEnumSym;
    //IDiaSymbol* pSym;
    ULONG dwTag;
    BSTR bstrName;
    ULONG dwInfo;
    BOOL bSet;
    //ULONG dwRank;
    //LONG lCount = 0;
    //ULONG celt = 1;
    ULONGLONG ulLen;

    if (IDiaSymbol_get_symTag(pSymbol, &dwTag) != S_OK)
    {
        PhAppendStringBuilder2(StringBuilder, L"ERROR - can't retrieve the symbol's SymTag\n");
        return;
    }
    IDiaSymbol_get_length(pSymbol, &ulLen);

    if (IDiaSymbol_get_name(pSymbol, &bstrName) != S_OK)
    {
        bstrName = NULL;
    }

    if (dwTag != SymTagPointerType)
    {
        if ((IDiaSymbol_get_constType(pSymbol, &bSet) == S_OK) && bSet)
        {
            PhAppendStringBuilder2(StringBuilder, L"const ");
        }

        if ((IDiaSymbol_get_volatileType(pSymbol, &bSet) == S_OK) && bSet)
        {
            PhAppendStringBuilder2(StringBuilder, L"volatile ");
        }

        if ((IDiaSymbol_get_unalignedType(pSymbol, &bSet) == S_OK) && bSet)
        {
            PhAppendStringBuilder2(StringBuilder, L"__unaligned ");
        }
    }

    switch (dwTag)
    {
    case SymTagUDT:
        //PrintUdtKind(pSymbol);
        PrintName(StringBuilder, pSymbol);
        break;

    case SymTagEnum:
        PhAppendStringBuilder2(StringBuilder, L"enum ");
        PrintName(StringBuilder, pSymbol);
        break;

    case SymTagFunctionType:
        PhAppendStringBuilder2(StringBuilder, L"function ");
        break;

    case SymTagPointerType:
        //if (pSymbol->get_type(&pBaseType) != S_OK) {
        //    wprintf(L"ERROR - SymTagPointerType get_type");
        //    if (bstrName != NULL) {
        //        SysFreeString(bstrName);
        //    }
        //    return;
        //}
        //PrintType(StringBuilder, pBaseType);
        ////pBaseType->Release();
        //if ((pSymbol->get_reference(&bSet) == S_OK) && bSet) {
        //    PhAppendStringBuilder2(StringBuilder, L" &");
        //}
        //else {
        //    PhAppendStringBuilder2(StringBuilder, L" *");
        //}
        //if ((pSymbol->get_constType(&bSet) == S_OK) && bSet) {
        //    wprintf(L" const");
        //}
        //if ((pSymbol->get_volatileType(&bSet) == S_OK) && bSet) {
        //    wprintf(L" volatile");
        //}
        //if ((pSymbol->get_unalignedType(&bSet) == S_OK) && bSet) {
        //    wprintf(L" __unaligned");
        //}
        break;

    case SymTagArrayType:
   /*     if (pSymbol->get_type(&pBaseType) == S_OK) {
            PrintType(pBaseType);

            if (pSymbol->get_rank(&dwRank) == S_OK) {
                if (SUCCEEDED(pSymbol->findChildren(SymTagDimension, NULL, nsNone, &pEnumSym)) && (pEnumSym != NULL)) {
                    while (SUCCEEDED(pEnumSym->Next(1, &pSym, &celt)) && (celt == 1)) {
                        IDiaSymbol* pBound;

                        wprintf(L"[");

                        if (pSym->get_lowerBound(&pBound) == S_OK) {
                            PrintBound(pBound);

                            wprintf(L"..");

                            pBound->Release();
                        }

                        pBound = NULL;

                        if (pSym->get_upperBound(&pBound) == S_OK) {
                            PrintBound(pBound);

                            pBound->Release();
                        }

                        pSym->Release();
                        pSym = NULL;

                        wprintf(L"]");
                    }

                    pEnumSym->Release();
                }
            }

            else if (SUCCEEDED(pSymbol->findChildren(SymTagCustomType, NULL, nsNone, &pEnumSym)) &&
                     (pEnumSym != NULL) &&
                     (pEnumSym->get_Count(&lCount) == S_OK) &&
                     (lCount > 0)) {
                while (SUCCEEDED(pEnumSym->Next(1, &pSym, &celt)) && (celt == 1)) {
                    wprintf(L"[");
                    PrintType(pSym);
                    wprintf(L"]");

                    pSym->Release();
                }

                pEnumSym->Release();
            }

            else {
                ULONG dwCountElems;
                ULONGLONG ulLenArray;
                ULONGLONG ulLenElem;

                if (pSymbol->get_count(&dwCountElems) == S_OK) {
                    wprintf(L"[0x%X]", dwCountElems);
                }

                else if ((pSymbol->get_length(&ulLenArray) == S_OK) &&
                         (pBaseType->get_length(&ulLenElem) == S_OK)) {
                    if (ulLenElem == 0) {
                        wprintf(L"[0x%lX]", (ULONG)ulLenArray);
                    }

                    else {
                        wprintf(L"[0x%lX]", (ULONG)ulLenArray / (ULONG)ulLenElem);
                    }
                }
            }

            pBaseType->Release();
        }

        else {
            wprintf(L"ERROR - SymTagArrayType get_type\n");
            if (bstrName != NULL) {
                SysFreeString(bstrName);
            }
            return;
        }*/
        break;

    case SymTagBaseType:
        {
            if (IDiaSymbol_get_baseType(pSymbol, &dwInfo) != S_OK)
            {
                PhAppendStringBuilder2(StringBuilder, L"SymTagBaseType get_baseType\n");

                if (bstrName)
                {
                    PhSymbolProviderFreeDiaString(bstrName);
                }
                return;
            }

            switch (dwInfo)
            {
            case btUInt:
                PhAppendStringBuilder2(StringBuilder, L"unsigned ");
                __fallthrough;
            case btInt:
                {
                    switch (ulLen)
                    {
                    case 1:
                        {
                            if (dwInfo == btInt)
                            {
                                PhAppendStringBuilder2(StringBuilder, L"signed ");
                            }

                            PhAppendStringBuilder2(StringBuilder, L"char");
                        }
                        break;
                    case 2:
                        PhAppendStringBuilder2(StringBuilder, L"short");
                        break;
                    case 4:
                        PhAppendStringBuilder2(StringBuilder, L"int");
                        break;
                    case 8:
                        PhAppendStringBuilder2(StringBuilder, L"__int64");
                        break;
                    }

                    dwInfo = ULONG_MAX;
                }
                break;

            case btFloat:
                {
                    switch (ulLen)
                    {
                    case 4:
                        PhAppendStringBuilder2(StringBuilder, L"float");
                        break;

                    case 8:
                        PhAppendStringBuilder2(StringBuilder, L"double");
                        break;
                    }

                    dwInfo = ULONG_MAX;
                }
                break;
            }

            if (dwInfo == ULONG_MAX)
                break;

            PhAppendFormatStringBuilder(StringBuilder, L"%s", rgBaseType[dwInfo]->Buffer);
        }
        break;
    case SymTagTypedef:
        {
            PrintName(StringBuilder, pSymbol);
        }
        break;
    case SymTagCustomType:
        {
            ULONG idOEM, idOEMSym;
            ULONG cbData = 0;
            ULONG count;

            if (IDiaSymbol_get_oemId(pSymbol, &idOEM) == S_OK)
            {
                PhAppendFormatStringBuilder(StringBuilder, L"OEMId = %X, ", idOEM);
            }

            if (IDiaSymbol_get_oemSymbolId(pSymbol, &idOEMSym) == S_OK)
            {
                PhAppendFormatStringBuilder(StringBuilder, L"SymbolId = %X, ", idOEMSym);
            }

            if (IDiaSymbol_get_types(pSymbol, 0, &count, NULL) == S_OK)
            {
                IDiaSymbol** rgpDiaSymbols = PhAllocate(sizeof(IDiaSymbol*) * count);

                if (IDiaSymbol_get_types(pSymbol, count, &count, rgpDiaSymbols) == S_OK)
                {
                    for (ULONG i = 0; i < count; i++)
                    {
                        PrintType(StringBuilder, rgpDiaSymbols[i]);

                        IDiaSymbol_Release(rgpDiaSymbols[i]);
                    }
                }

                PhFree(rgpDiaSymbols);
            }

            // print custom data

            if ((IDiaSymbol_get_dataBytes(pSymbol, cbData, &cbData, NULL) == S_OK) && (cbData != 0))
            {
                PhAppendFormatStringBuilder(StringBuilder, L", Data: ");

                BYTE* pbData = PhAllocate(cbData);

                IDiaSymbol_get_dataBytes(pSymbol, cbData, &cbData, pbData);

                for (ULONG i = 0; i < cbData; i++)
                {
                    PhAppendFormatStringBuilder(StringBuilder, L"0x%02X ", pbData[i]);
                }

                PhFree(pbData);
            }
        }
        break;

    case SymTagData: // This really is member data, just print its location
        PrintLocation(StringBuilder, pSymbol);
        break;
    }

    if (bstrName)
    {
        PhSymbolProviderFreeDiaString(bstrName);
    }
}

VOID PrintSymbolType(
    _In_ PPH_STRING_BUILDER StringBuilder,
    _In_ IDiaSymbol* Symbol
    )
{
    IDiaSymbol* idiaSymbolType;

    if (IDiaSymbol_get_type(Symbol, &idiaSymbolType) == S_OK)
    {
        PhAppendFormatStringBuilder(StringBuilder, L", Type: ");

        PrintType(StringBuilder, idiaSymbolType);

        IDiaSymbol_Release(idiaSymbolType);
    }
}

VOID PrintUDT(
    _In_ PPH_STRING_BUILDER StringBuilder,
    _In_ IDiaSymbol* Symbol)
{
    PrintName(StringBuilder, Symbol);
    PrintSymbolType(StringBuilder, Symbol);
}

VOID PrintTypeInDetail(
    _In_ PPH_STRING_BUILDER StringBuilder,
    _In_ IDiaSymbol* IDiaSymbolDetail,
    _In_ ULONG dwIndent)
{
    IDiaSymbol* pType;
    //IDiaSymbol* idiaSymbol;
    ULONG dwSymTag;
    ULONG dwSymTagType;
    ULONG celt = 0;
    //BOOL bFlag;

    //if (dwIndent > MAX_TYPE_IN_DETAIL) {
    //    return;
    //}

    if (IDiaSymbol_get_symTag(IDiaSymbolDetail, &dwSymTag) != S_OK)
        return;

    PrintSymTag(StringBuilder, dwSymTag);

    for (ULONG i = 0; i < dwIndent; i++)
        PhAppendCharStringBuilder(StringBuilder, L' ');

    switch (dwSymTag)
    {
    case SymTagData:
        {
            PrintData(StringBuilder, IDiaSymbolDetail);

            if (IDiaSymbol_get_type(IDiaSymbolDetail, &pType) == S_OK)
            {
                if (IDiaSymbol_get_symTag(pType, &dwSymTagType) == S_OK)
                {
                    if (dwSymTagType == SymTagUDT)
                    {
                        PhAppendCharStringBuilder(StringBuilder, L'\n');

                        PrintTypeInDetail(StringBuilder, pType, dwIndent + 2);
                    }
                }

                IDiaSymbol_Release(pType);
            }
        }
        break;

    case SymTagTypedef:
    case SymTagVTable:
        PrintSymbolType(StringBuilder, IDiaSymbolDetail);
        break;
    case SymTagEnum:
    case SymTagUDT:
        {
            IDiaEnumSymbols* idiaEnumSymbols;
            IDiaSymbol* idiaSymbol;

            PrintUDT(StringBuilder, IDiaSymbolDetail);

            PhAppendCharStringBuilder(StringBuilder, L'\n');

            if (IDiaSymbol_findChildren(IDiaSymbolDetail, SymTagNull, NULL, nsNone, &idiaEnumSymbols) == S_OK)
            {
                while (IDiaEnumSymbols_Next(idiaEnumSymbols, 1, &idiaSymbol, &celt) == S_OK && celt == 1)
                {
                    PrintTypeInDetail(StringBuilder, idiaSymbol, dwIndent + 2);

                    IDiaSymbol_Release(idiaSymbol);
                }

                IDiaEnumSymbols_Release(idiaEnumSymbols);
            }
        }
        break;
    case SymTagFunction:
        {
            PrintName(StringBuilder, IDiaSymbolDetail);
            //PrintFunctionType(pSymbol);
        }
        break;
    case SymTagPointerType:
        {
            PrintName(StringBuilder, IDiaSymbolDetail);
            //wprintf(L" has type ");
            //PrintType(pSymbol);
        }
        break;
    case SymTagArrayType:
    case SymTagBaseType:
    case SymTagFunctionArgType:
    case SymTagUsingNamespace:
    case SymTagCustom:
    case SymTagFriend:
        {
            PrintName(StringBuilder, IDiaSymbolDetail);
            //PrintSymbolType(pSymbol);
        }
        break;

    case SymTagVTableShape:
    case SymTagBaseClass:
        {
            PrintName(StringBuilder, IDiaSymbolDetail);

            //if ((pSymbol->get_virtualBaseClass(&bFlag) == S_OK) && bFlag) {
            //    IDiaSymbol* pVBTableType;
            //    LONG ptrOffset;
            //    DWORD dispIndex;

            //    if ((pSymbol->get_virtualBaseDispIndex(&dispIndex) == S_OK) &&
            //        (pSymbol->get_virtualBasePointerOffset(&ptrOffset) == S_OK)) {
            //        wprintf(L" virtual, offset = 0x%X, pointer offset = %ld, virtual base pointer type = ", dispIndex, ptrOffset);

            //        if (pSymbol->get_virtualBaseTableType(&pVBTableType) == S_OK) {
            //            PrintType(pVBTableType);
            //            pVBTableType->Release();
            //        }

            //        else {
            //            wprintf(L"(unknown)");
            //        }
            //    }
            //}

            //else {
            //    LONG offset;

            //    if (pSymbol->get_offset(&offset) == S_OK) {
            //        wprintf(L", offset = 0x%X", offset);
            //    }
            //}

            //putwchar(L'\n');

            //if (SUCCEEDED(pSymbol->findChildren(SymTagNull, NULL, nsNone, &idiaEnumSymbols))) {
            //    while (SUCCEEDED(idiaEnumSymbols->Next(1, &idiaSymbol, &celt)) && (celt == 1)) {
            //        PrintTypeInDetail(idiaSymbol, dwIndent + 2);
            //        idiaSymbol->Release();
            //    }

            //    idiaEnumSymbols->Release();
            //}
        }
        break;

    case SymTagFunctionType:
        {
            if (IDiaSymbol_get_type(IDiaSymbolDetail , &pType) == S_OK)
            {
                PrintType(StringBuilder, pType);

                IDiaSymbol_Release(pType);
            }
        }
        break;

    case SymTagThunk:
      // Happens for functions which only have S_PROCREF
        //PrintThunk(pSymbol);
        break;

    default:
        PhAppendFormatStringBuilder(StringBuilder, L"ERROR - PrintTypeInDetail() invalid SymTag\n");
    }

    PhAppendCharStringBuilder(StringBuilder, L'\n');

    {
        BOOL bValue;
        ULONG dwValue;
        ULONGLONG ullValue;
        BSTR bstrValue;
        ULONG cbData;
        PVOID pbData;

        if (IDiaSymbol_get_language(IDiaSymbolDetail, &dwValue) == S_OK)
        {
            if (dwValue < RTL_NUMBER_OF(rgLanguage))
            {
                PhAppendStringBuilder2(StringBuilder, L"Language: ");
                PhAppendStringBuilder(StringBuilder, rgLanguage[dwValue]);
                PhAppendStringBuilder2(StringBuilder, L"\r\n");
            }
            else
            {
                PrintHex(StringBuilder, L"Language", (ULONG64)dwValue);
            }
        }

        if (IDiaSymbol_get_wasInlined(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"Was Inlined", bValue);
        if (IDiaSymbol_get_sourceFileName(IDiaSymbolDetail, &bstrValue) == S_OK)
        {
            PrintBSTR(StringBuilder, L"Source File", bstrValue);
            PhSymbolProviderFreeDiaString(bstrValue);
        }
        if (IDiaSymbol_get_volatileType(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"Volatile", bValue);
        if (IDiaSymbol_get_compilerGenerated(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"Compiler Generated", bValue);
        if (IDiaSymbol_get_exceptionHandlerVirtualAddress(IDiaSymbolDetail, &ullValue) == S_OK)
            PrintHex(StringBuilder, L"Exception Handler VA", ullValue);
        if (IDiaSymbol_get_framePointerPresent(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"Frame Pointer Present", bValue);
        if (IDiaSymbol_get_hasAlloca(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"Has Alloca", bValue);
        if (IDiaSymbol_get_hasInlAsm(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"Has Inline Asm", bValue);
        if (IDiaSymbol_get_isLocationControlFlowDependent(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"Location Control Flow Dependent", bValue);
        if (IDiaSymbol_get_isLTCG(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"LTCG", bValue);
        if (IDiaSymbol_get_isPGO(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"PGO", bValue);
        if (IDiaSymbol_get_frameSize(IDiaSymbolDetail, &dwValue) == S_OK)
            PrintHex(StringBuilder, L"Frame Size", (ULONG64)dwValue);

        {
            IDiaSymbol9* pSymbol9;
            if (IUnknown_QueryInterface(IDiaSymbolDetail, &IID_IDiaSymbol9, (void**)&pSymbol9) == S_OK)
            {
                if (pSymbol9->lpVtbl->get_framePadSize(pSymbol9, &dwValue) == S_OK)
                    PrintHex(StringBuilder, L"Frame Pad Size", (ULONG64)dwValue);
                IDiaSymbol9_Release(pSymbol9);
            }
        }

        if (IDiaSymbol_get_hasControlFlowCheck(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"Has Control Flow Check", bValue);
        if (IDiaSymbol_get_hasLongJump(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"Has Long Jump", bValue);
        if (IDiaSymbol_get_hasSetJump(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"Has Set Jump", bValue);
        if (IDiaSymbol_get_hasEH(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"Has EH", bValue);
        if (IDiaSymbol_get_hasSecurityChecks(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"Has Security Checks", bValue);
        if (IDiaSymbol_get_libraryName(IDiaSymbolDetail, &bstrValue) == S_OK)
        {
            PrintBSTR(StringBuilder, L"Library Name", bstrValue);
            PhSymbolProviderFreeDiaString(bstrValue);
        }
        if (IDiaSymbol_get_managed(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"Managed", bValue);
        if (IDiaSymbol_get_packed(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"Packed", bValue);
        if (IDiaSymbol_get_platform(IDiaSymbolDetail, &dwValue) == S_OK)
            PrintHex(StringBuilder, L"Platform", (ULONG64)dwValue);
        if (IDiaSymbol_get_staticSize(IDiaSymbolDetail, &dwValue) == S_OK)
            PrintHex(StringBuilder, L"Static Size", (ULONG64)dwValue);
        if (IDiaSymbol_get_finalLiveStaticSize(IDiaSymbolDetail, &dwValue) == S_OK)
            PrintHex(StringBuilder, L"Final Live Static Size", (ULONG64)dwValue);
        if (IDiaSymbol_get_PGODynamicInstructionCount(IDiaSymbolDetail, &ullValue) == S_OK)
            PrintHex(StringBuilder, L"PGO Dynamic Instruction Count", ullValue);
        if (IDiaSymbol_get_strictGSCheck(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"Strict GS Check", bValue);
        if (IDiaSymbol_get_targetSection(IDiaSymbolDetail, &dwValue) == S_OK)
            PrintHex(StringBuilder, L"Target Section", (ULONG64)dwValue);
        if (IDiaSymbol_get_pure(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"Pure", bValue);
        if (IDiaSymbol_get_rank(IDiaSymbolDetail, &dwValue) == S_OK)
            PrintHex(StringBuilder, L"Rank", (ULONG64)dwValue);
        if (IDiaSymbol_get_RValueReference(IDiaSymbolDetail, &bValue) == S_OK)
            PrintBool(StringBuilder, L"RValue Reference", bValue);

        {
            IDiaSymbol3* pSymbol3;
            if (IUnknown_QueryInterface(IDiaSymbolDetail, &IID_IDiaSymbol3, (void**)&pSymbol3) == S_OK)
            {
                IDiaSymbol* pInlinee;
                if (pSymbol3->lpVtbl->get_inlinee(pSymbol3, &pInlinee) == S_OK)
                {
                    PhAppendStringBuilder2(StringBuilder, L"Inlinee: ");
                    PrintName(StringBuilder, pInlinee);
                    PhAppendStringBuilder2(StringBuilder, L"\r\n");
                    IDiaSymbol_Release(pInlinee);
                }
                if (pSymbol3->lpVtbl->get_inlineeId(pSymbol3, &dwValue) == S_OK)
                    PrintHex(StringBuilder, L"Inlinee ID", (ULONG64)dwValue);
                IDiaSymbol3_Release(pSymbol3);
            }
        }

        {
            IDiaSymbol4* pSymbol4;
            if (IUnknown_QueryInterface(IDiaSymbolDetail, &IID_IDiaSymbol4, (void**)&pSymbol4) == S_OK)
            {
                if (pSymbol4->lpVtbl->get_noexcept(pSymbol4, &bValue) == S_OK)
                    PrintBool(StringBuilder, L"Noexcept", bValue);
                IDiaSymbol4_Release(pSymbol4);
            }
        }

        {
            IDiaSymbol7* pSymbol7;
            if (IUnknown_QueryInterface(IDiaSymbolDetail, &IID_IDiaSymbol7, (void**)&pSymbol7) == S_OK)
            {
                if (pSymbol7->lpVtbl->get_isSignRet(pSymbol7, &bValue) == S_OK)
                    PrintBool(StringBuilder, L"Is Sign Ret", bValue);
                IDiaSymbol7_Release(pSymbol7);
            }
        }

        {
            IDiaSymbol10* pSymbol10;
            if (IUnknown_QueryInterface(IDiaSymbolDetail, &IID_IDiaSymbol10, (void**)&pSymbol10) == S_OK)
            {
                if (pSymbol10->lpVtbl->get_sourceLink(pSymbol10, 0, &cbData, NULL) == S_OK && cbData != 0)
                {
                    pbData = PhAllocate(cbData);
                    if (pSymbol10->lpVtbl->get_sourceLink(pSymbol10, cbData, &cbData, (BYTE*)pbData) == S_OK)
                    {
                        PhAppendStringBuilder2(StringBuilder, L"Source Link: Found\r\n");
                    }
                    PhFree(pbData);
                }
                IDiaSymbol10_Release(pSymbol10);
            }
        }

        {
            IDiaSymbol11* pSymbol11;
            if (IUnknown_QueryInterface(IDiaSymbolDetail, &IID_IDiaSymbol11, (void**)&pSymbol11) == S_OK)
            {
                IDiaSymbol11_Release(pSymbol11);
            }
        }
    }
}

BOOLEAN PePdbShouldUndecorateSymbol(
    _In_ ULONG SymbolTag
    )
{
    switch (SymbolTag)
    {
    case SymTagFunction:
    case SymTagThunk:
    case SymTagPublicSymbol:
    case SymTagData:
        return TRUE;
    default:
        return FALSE;
    }
}

BOOLEAN PePdbShouldExpandDiaSymbol(
    _In_ ULONG SymbolTag
    )
{
    switch (SymbolTag)
    {
    case SymTagFunction:
    case SymTagBlock:
    case SymTagUDT:
    case SymTagEnum:
    case SymTagData:
        return TRUE;
    default:
        return FALSE;
    }
}

ULONG PePdbGetSymbolDepth(
    _In_opt_ PPV_SYMBOL_NODE Node
    )
{
    ULONG depth = 0;

    while (Node)
    {
        depth++;
        Node = Node->Parent;
    }

    return depth;
}

PPV_SYMBOL_NODE PePdbCreateSyntheticSymbolNode(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_opt_ PPV_SYMBOL_NODE Parent,
    _In_ PWSTR Name,
    _In_ PPH_STRINGREF Data
    )
{
    PPV_SYMBOL_NODE symbol;

    symbol = PhAllocateZero(sizeof(PV_SYMBOL_NODE));
    PhInitializeTreeNewNode(&symbol->Node);
    symbol->Parent = Parent;
    symbol->UniqueId = ++Context->Count;
    symbol->Type = PV_SYMBOL_TYPE_SYMBOL;
    symbol->Name = PhCreateString(Name);
    symbol->Data = Data;
    PhPrintInt64(symbol->Index, symbol->UniqueId);

    PhAcquireQueuedLockExclusive(&SearchResultsLock);

    if (Parent)
    {
        if (!Parent->Children)
            Parent->Children = PhCreateList(8);

        PhAddItemList(Parent->Children, symbol);
    }
    else
    {
        PhAddItemList(SearchResults, symbol);
    }

    PhReleaseQueuedLockExclusive(&SearchResultsLock);

    return symbol;
}

PPV_SYMBOL_NODE PePdbCreateDiaSymbolNode(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSymbol* DiaSymbol,
    _In_opt_ PPV_SYMBOL_NODE Parent
    )
{
    ULONG symbolTag = SymTagNull;
    ULONG dataKind = DataIsUnknown;
    ULONG symbolId = 0;
    ULONG symbolRva = 0;
    ULONGLONG symbolLength = 0;
    BSTR bstrName = NULL;
    BSTR bstrUndname = NULL;
    PPH_STRINGREF symbolTagText;
    PPV_SYMBOL_NODE symbol;
    LONG lOffset = 0;

    if (IDiaSymbol_get_symTag(DiaSymbol, &symbolTag) != S_OK)
        return NULL;

    if (symbolTag == SymTagFuncDebugStart || symbolTag == SymTagFuncDebugEnd)
        return NULL;

    IDiaSymbol_get_dataKind(DiaSymbol, &dataKind);
    IDiaSymbol_get_typeId(DiaSymbol, &symbolId);
    IDiaSymbol_get_relativeVirtualAddress(DiaSymbol, &symbolRva);
    IDiaSymbol_get_length(DiaSymbol, &symbolLength);
    IDiaSymbol_get_name(DiaSymbol, &bstrName);
    IDiaSymbol_get_offset(DiaSymbol, &lOffset);

    if (PePdbShouldUndecorateSymbol(symbolTag) &&
        IDiaSymbol_get_undecoratedNameEx(DiaSymbol, UNDNAME_NAME_ONLY, &bstrUndname) != S_OK)
    {
        IDiaSymbol_get_undecoratedName(DiaSymbol, &bstrUndname);
    }

    if (symbolTag < RTL_NUMBER_OF(rgTags))
        symbolTagText = rgTags[symbolTag];
    else
        symbolTagText = rgTags[SymTagNull];

    symbol = PhAllocateZero(sizeof(PV_SYMBOL_NODE));
    symbol->Parent = Parent;
    symbol->UniqueId = ++Context->Count;
    symbol->TypeId = symbolId;
    symbol->Address = symbolRva;
    symbol->Size = symbolLength;
    symbol->Data = symbolTagText;
    symbol->Offset = (ULONG64)lOffset;
    PhPrintPointer(symbol->Pointer, UlongToPtr(symbolRva));
    PhPrintInt64(symbol->Index, symbol->UniqueId);

    if (IDiaSymbol_get_offset(DiaSymbol, &lOffset) == S_OK)
    {
        symbol->Offset = (ULONG64)lOffset;
        PhPrintPointer(symbol->OffsetText, UlongToPtr(lOffset));
    }

    {
        PH_STRING_BUILDER locBuilder;

        PhInitializeStringBuilder(&locBuilder, 32);
        PrintLocation(&locBuilder, DiaSymbol);

        if (locBuilder.String->Length != 0)
            symbol->LocationString = PhFinalStringBuilderString(&locBuilder);
        else
            PhDeleteStringBuilder(&locBuilder);
    }

    if (bstrUndname)
        symbol->Name = PhCreateString(bstrUndname);
    else if (bstrName)
        symbol->Name = PhCreateString(bstrName);
    else if (symbolTag == SymTagExe)
        symbol->Name = PhCreateString(L"Global Scope");
    else
        symbol->Name = PhFormatString(L"%s <%lu>", symbolTagText->Buffer, symbolId);

    switch (symbolTag)
    {
    case SymTagFunction:
        symbol->Type = PV_SYMBOL_TYPE_FUNCTION;
        break;
    case SymTagData:
        {
            switch (dataKind)
            {
            case DataIsLocal:
                symbol->Type = PV_SYMBOL_TYPE_LOCAL_VAR;
                break;
            case DataIsStaticLocal:
                symbol->Type = PV_SYMBOL_TYPE_STATIC_LOCAL_VAR;
                break;
            case DataIsParam:
                symbol->Type = PV_SYMBOL_TYPE_PARAMETER;
                break;
            case DataIsObjectPtr:
                symbol->Type = PV_SYMBOL_TYPE_OBJECT_PTR;
                break;
            case DataIsFileStatic:
                symbol->Type = PV_SYMBOL_TYPE_STATIC_VAR;
                break;
            case DataIsGlobal:
                symbol->Type = PV_SYMBOL_TYPE_GLOBAL_VAR;
                break;
            case DataIsMember:
                symbol->Type = PV_SYMBOL_TYPE_MEMBER;
                break;
            case DataIsStaticMember:
                symbol->Type = PV_SYMBOL_TYPE_STATIC_MEMBER;
                break;
            case DataIsConstant:
                {
                    VARIANT vt = { VT_EMPTY };

                    VariantInit(&vt);

                    symbol->Type = PV_SYMBOL_TYPE_CONSTANT;

                    if (IDiaSymbol_get_value(DiaSymbol, &vt) == S_OK)
                    {
                        PH_STRING_BUILDER sb;

                        PhInitializeStringBuilder(&sb, 16);
                        PrintVariant(&sb, &vt);
                        symbol->Value = PhFinalStringBuilderString(&sb);

                        VariantClear((VARIANTARG*)&vt);
                    }
                }
                break;
            default:
                symbol->Type = PV_SYMBOL_TYPE_UNKNOWN;
                break;
            }
        }
        break;
    case SymTagUDT:
        symbol->Type = PV_SYMBOL_TYPE_STRUCT;
        break;
    case SymTagEnum:
        symbol->Type = PV_SYMBOL_TYPE_CLASS;
        break;
    case SymTagUsingNamespace:
    case SymTagCompilandEnv:
        {
            VARIANT value = { VT_EMPTY };

            VariantInit(&value);

            symbol->Type = PV_SYMBOL_TYPE_SYMBOL;

            if (IDiaSymbol_get_value(DiaSymbol, &value) == S_OK)
            {
                PH_STRING_BUILDER sb;

                PhInitializeStringBuilder(&sb, 16);
                PrintVariant(&sb, &value);
                PhMoveReference(&symbol->Name, PhConcatStringRef2(&symbol->Name->sr, &sb.String->sr));
                PhDeleteStringBuilder(&sb);
            }
        }
        break;
    default:
        symbol->Type = PV_SYMBOL_TYPE_SYMBOL;
        break;
    }

    if (symbolRva)
    {
        PIMAGE_SECTION_HEADER directorySection = NULL;

        PhMappedImageRvaToVa(&PvMappedImage, symbolRva, &directorySection);

        if (directorySection)
        {
            symbol->Characteristics = directorySection->Characteristics;
            PhGetMappedImageSectionName(
                directorySection,
                symbol->SectionName,
                RTL_NUMBER_OF(symbol->SectionName),
                &symbol->SectionNameLength
                );
        }
    }

    PhAcquireQueuedLockExclusive(&SearchResultsLock);

    if (Parent)
    {
        if (!Parent->Children)
            Parent->Children = PhCreateList(8);

        PhAddItemList(Parent->Children, symbol);
    }
    else
    {
        PhAddItemList(SearchResults, symbol);
    }

    PhReleaseQueuedLockExclusive(&SearchResultsLock);

    if (bstrUndname)
        PhSymbolProviderFreeDiaString(bstrUndname);
    if (bstrName)
        PhSymbolProviderFreeDiaString(bstrName);

    return symbol;
}

VOID PePdbEnumLocalVariables(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSymbol* DiaSymbol,
    _In_ PPV_SYMBOL_NODE Parent
    )
{
    IDiaSymbol* pBlock = NULL;
    ULONG rva;
    ULONG tag;

    if (IDiaSymbol_get_relativeVirtualAddress(DiaSymbol, &rva) != S_OK)
        return;

    if (IDiaSession_findSymbolByRVA((IDiaSession*)Context->IDiaSession, rva, SymTagBlock, &pBlock) != S_OK)
    {
        // Try finding a function if block fails
        if (IDiaSession_findSymbolByRVA((IDiaSession*)Context->IDiaSession, rva, SymTagFunction, &pBlock) != S_OK)
            return;
    }

    while (pBlock)
    {
        IDiaEnumSymbols* pEnum;

        if (IDiaSymbol_findChildren(pBlock, SymTagNull, NULL, nsNone, &pEnum) == S_OK)
        {
            IDiaSymbol* pSymbol;
            ULONG celt;

            while (IDiaEnumSymbols_Next(pEnum, 1, &pSymbol, &celt) == S_OK && celt == 1)
            {
                ULONG symbolTag;

                if (IDiaSymbol_get_symTag(pSymbol, &symbolTag) == S_OK)
                {
                    if (symbolTag == SymTagData || symbolTag == SymTagAnnotation)
                    {
                        PePdbPrintDiaSymbol(Context, pSymbol, Parent);
                    }
                }

                IDiaSymbol_Release(pSymbol);
            }

            IDiaEnumSymbols_Release(pEnum);
        }

        if (IDiaSymbol_get_symTag(pBlock, &tag) == S_OK && tag == SymTagFunction)
            break;

        {
            IDiaSymbol* pParent = NULL;
            if (IDiaSymbol_get_lexicalParent(pBlock, &pParent) == S_OK && pParent)
            {
                IDiaSymbol_Release(pBlock);
                pBlock = pParent;
            }
            else
            {
                break;
            }
        }
    }

    if (pBlock)
        IDiaSymbol_Release(pBlock);
}

VOID PePdbEnumDiaSymbolChildren(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSymbol* DiaSymbol,
    _In_ PPV_SYMBOL_NODE Parent
    );

VOID PePdbPrintDiaSymbol(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSymbol* DiaSymbol,
    _In_opt_ PPV_SYMBOL_NODE Parent
    )
{
    ULONG symbolTag = SymTagNull;
    PPV_SYMBOL_NODE symbol;

    if (IDiaSymbol_get_symTag(DiaSymbol, &symbolTag) != S_OK)
        return;

    symbol = PePdbCreateDiaSymbolNode(Context, DiaSymbol, Parent);

    if (!symbol)
        return;

    if (symbolTag == SymTagFunction || symbolTag == SymTagBlock)
    {
        PePdbEnumLocalVariables(Context, DiaSymbol, symbol);
    }

    if (PePdbShouldExpandDiaSymbol(symbolTag) &&
        PePdbGetSymbolDepth(symbol) < PV_DIA_MAX_TREE_DEPTH)
    {
        PePdbEnumDiaSymbolChildren(Context, DiaSymbol, symbol);
    }
}

VOID PePdbEnumDiaSymbolChildren(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSymbol* DiaSymbol,
    _In_ PPV_SYMBOL_NODE Parent
    )
{
    IDiaEnumSymbols* idiaEnumSymbols;
    IDiaSymbol* childSymbol;
    ULONG count = 0;

    if (IDiaSymbol_findChildren(DiaSymbol, SymTagNull, NULL, nsNone, &idiaEnumSymbols) != S_OK)
        return;

    while (IDiaEnumSymbols_Next(idiaEnumSymbols, 1, &childSymbol, &count) == S_OK && count == 1)
    {
        PePdbPrintDiaSymbol(Context, childSymbol, Parent);
        IDiaSymbol_Release(childSymbol);
    }

    IDiaEnumSymbols_Release(idiaEnumSymbols);
}

VOID PePdbEnumDiaSymbol2(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSymbol* DiaSymbol,
    _In_ PPV_SYMBOL_NODE Parent
    )
{
    IDiaEnumSymbols* idiaEnumSymbols;
    IDiaSymbol* childSymbol;
    ULONG count = 0;

    if (IDiaSymbol_findChildren(DiaSymbol, SymTagNull, NULL, nsNone, &idiaEnumSymbols) != S_OK)
        return;

    while (IDiaEnumSymbols_Next(idiaEnumSymbols, 1, &childSymbol, &count) == S_OK && count == 1)
    {
        PePdbPrintDiaSymbol(Context, childSymbol, Parent);

        IDiaSymbol_Release(childSymbol);
    }

    IDiaEnumSymbols_Release(idiaEnumSymbols);
}

VOID PePdbSetSyntheticSymbolData(
    _In_ PPV_SYMBOL_NODE Symbol,
    _In_ PPH_STRING Data
    )
{
    Symbol->DataString = Data;
    Symbol->Data = &Symbol->DataString->sr;
}

PPV_SYMBOL_NODE PePdbCreateSyntheticSymbolNode2(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_opt_ PPV_SYMBOL_NODE Parent,
    _In_ PWSTR Name,
    _In_opt_ PPH_STRING Data
    )
{
    PPV_SYMBOL_NODE symbol;

    symbol = PePdbCreateSyntheticSymbolNode(Context, Parent, Name, NULL);

    if (symbol && Data)
        PePdbSetSyntheticSymbolData(symbol, Data);

    return symbol;
}

VOID PePdbSetSyntheticSymbolRva(
    _In_ PPV_SYMBOL_NODE Symbol,
    _In_ ULONG Rva
    )
{
    Symbol->Address = Rva;
    PhPrintPointer(Symbol->Pointer, UlongToPtr(Rva));
}

VOID PePdbSetSyntheticSymbolOffset(
    _In_ PPV_SYMBOL_NODE Symbol,
    _In_ ULONG Offset
    )
{
    Symbol->Offset = Offset;
    PhPrintPointer(Symbol->OffsetText, UlongToPtr(Offset));
}

VOID PePdbDumpDiaSourceFiles(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSession* DiaSession,
    _In_ PPV_SYMBOL_NODE Root
    )
{
    IDiaEnumSourceFiles* enumSourceFiles;
    IDiaSourceFile* sourceFile;
    ULONG count = 0;
    PPV_SYMBOL_NODE categoryNode;

    if (IDiaSession_findFile(DiaSession, NULL, NULL, nsNone, &enumSourceFiles) != S_OK)
        return;

    categoryNode = PePdbCreateSyntheticSymbolNode2(Context, Root, L"SourceFiles", NULL);

    while (IDiaEnumSourceFiles_Next(enumSourceFiles, 1, &sourceFile, &count) == S_OK && count == 1)
    {
        BSTR fileName = NULL;
        ULONG uniqueId = 0;
        ULONG checksumType = 0;

        IDiaSourceFile_get_fileName(sourceFile, &fileName);
        IDiaSourceFile_get_uniqueId(sourceFile, &uniqueId);
        IDiaSourceFile_get_checksumType(sourceFile, &checksumType);

        PePdbCreateSyntheticSymbolNode2(
            Context,
            categoryNode,
            fileName ? fileName : L"SourceFile",
            PhFormatString(L"Id: %lu, ChecksumType: %lu", uniqueId, checksumType)
            );

        if (fileName)
            PhSymbolProviderFreeDiaString(fileName);

        IDiaSourceFile_Release(sourceFile);
    }

    IDiaEnumSourceFiles_Release(enumSourceFiles);
}

VOID PePdbDumpDiaLineNumbers(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSession* DiaSession,
    _In_ PPV_SYMBOL_NODE Root
    )
{
    IDiaEnumLineNumbers* enumLineNumbers;
    IDiaLineNumber* lineNumber;
    ULONG count = 0;
    PPV_SYMBOL_NODE categoryNode;

    if (IDiaSession_findLinesByRVA(DiaSession, 0, ULONG_MAX, &enumLineNumbers) != S_OK)
        return;

    categoryNode = PePdbCreateSyntheticSymbolNode2(Context, Root, L"LineNumbers", NULL);

    while (IDiaEnumLineNumbers_Next(enumLineNumbers, 1, &lineNumber, &count) == S_OK && count == 1)
    {
        IDiaSourceFile* sourceFile = NULL;
        BSTR fileName = NULL;
        ULONG line = 0;
        ULONG lineEnd = 0;
        ULONG column = 0;
        ULONG columnEnd = 0;
        ULONG rva = 0;
        ULONG length = 0;
        ULONG sourceFileId = 0;
        ULONG compilandId = 0;
        PPH_STRING name;
        PPV_SYMBOL_NODE node;

        IDiaLineNumber_get_lineNumber(lineNumber, &line);
        IDiaLineNumber_get_lineNumberEnd(lineNumber, &lineEnd);
        IDiaLineNumber_get_columnNumber(lineNumber, &column);
        IDiaLineNumber_get_columnNumberEnd(lineNumber, &columnEnd);
        IDiaLineNumber_get_relativeVirtualAddress(lineNumber, &rva);
        IDiaLineNumber_get_length(lineNumber, &length);
        IDiaLineNumber_get_sourceFileId(lineNumber, &sourceFileId);
        IDiaLineNumber_get_compilandId(lineNumber, &compilandId);

        if (IDiaLineNumber_get_sourceFile(lineNumber, &sourceFile) == S_OK)
        {
            IDiaSourceFile_get_fileName(sourceFile, &fileName);
            IDiaSourceFile_Release(sourceFile);
        }

        if (fileName)
            name = PhFormatString(L"%s:%lu", fileName, line);
        else
            name = PhFormatString(L"Line %lu", line);

        node = PePdbCreateSyntheticSymbolNode2(
            Context,
            categoryNode,
            name->Buffer,
            PhFormatString(
                L"LineEnd: %lu, Column: %lu-%lu, Length: %lu, SourceFileId: %lu, CompilandId: %lu",
                lineEnd,
                column,
                columnEnd,
                length,
                sourceFileId,
                compilandId
                )
            );

        if (node)
        {
            PePdbSetSyntheticSymbolRva(node, rva);
            node->Size = length;
        }

        PhDereferenceObject(name);

        if (fileName)
            PhSymbolProviderFreeDiaString(fileName);

        IDiaLineNumber_Release(lineNumber);
    }

    IDiaEnumLineNumbers_Release(enumLineNumbers);
}

VOID PePdbDumpDiaInputAssemblyFiles(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSession* DiaSession,
    _In_ PPV_SYMBOL_NODE Root
    )
{
    IDiaEnumInputAssemblyFiles* enumInputAssemblyFiles;
    IDiaInputAssemblyFile* inputAssemblyFile;
    ULONG count = 0;
    PPV_SYMBOL_NODE categoryNode;

    if (IDiaSession_findInputAssemblyFiles(DiaSession, &enumInputAssemblyFiles) != S_OK)
        return;

    categoryNode = PePdbCreateSyntheticSymbolNode2(Context, Root, L"InputAssemblyFiles", NULL);

    while (IDiaEnumInputAssemblyFiles_Next(enumInputAssemblyFiles, 1, &inputAssemblyFile, &count) == S_OK && count == 1)
    {
        BSTR fileName = NULL;
        ULONG uniqueId = 0;
        ULONG index = 0;
        ULONG timestamp = 0;
        BOOL pdbAvailable = FALSE;

        IDiaInputAssemblyFile_get_fileName(inputAssemblyFile, &fileName);
        IDiaInputAssemblyFile_get_uniqueId(inputAssemblyFile, &uniqueId);
        IDiaInputAssemblyFile_get_index(inputAssemblyFile, &index);
        IDiaInputAssemblyFile_get_timestamp(inputAssemblyFile, &timestamp);
        IDiaInputAssemblyFile_get_pdbAvailableAtILMerge(inputAssemblyFile, &pdbAvailable);

        PePdbCreateSyntheticSymbolNode2(
            Context,
            categoryNode,
            fileName ? fileName : L"InputAssemblyFile",
            PhFormatString(
                L"Id: %lu, Index: %lu, TimeStamp: 0x%08lx, PdbAvailableAtILMerge: %s",
                uniqueId,
                index,
                timestamp,
                pdbAvailable ? L"true" : L"false"
                )
            );

        if (fileName)
            PhSymbolProviderFreeDiaString(fileName);

        IDiaInputAssemblyFile_Release(inputAssemblyFile);
    }

    IDiaEnumInputAssemblyFiles_Release(enumInputAssemblyFiles);
}

VOID PePdbDumpDiaInjectedSource(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSession* DiaSession,
    _In_ PPV_SYMBOL_NODE Root
    )
{
    IDiaEnumInjectedSources* enumInjectedSources;
    IDiaInjectedSource* injectedSource;
    ULONG count = 0;
    PPV_SYMBOL_NODE categoryNode;

    if (IDiaSession_findInjectedSource(DiaSession, NULL, &enumInjectedSources) != S_OK)
        return;

    categoryNode = PePdbCreateSyntheticSymbolNode2(Context, Root, L"InjectedSource", NULL);

    while (IDiaEnumInjectedSources_Next(enumInjectedSources, 1, &injectedSource, &count) == S_OK && count == 1)
    {
        BSTR fileName = NULL;
        BSTR objectFileName = NULL;
        BSTR virtualFileName = NULL;
        ULONG crc = 0;
        ULONG compression = 0;
        ULONGLONG length = 0;
        PPV_SYMBOL_NODE node;

        IDiaInjectedSource_get_filename(injectedSource, &fileName);
        IDiaInjectedSource_get_objectFilename(injectedSource, &objectFileName);
        IDiaInjectedSource_get_virtualFilename(injectedSource, &virtualFileName);
        IDiaInjectedSource_get_crc(injectedSource, &crc);
        IDiaInjectedSource_get_sourceCompression(injectedSource, &compression);
        IDiaInjectedSource_get_length(injectedSource, &length);

        node = PePdbCreateSyntheticSymbolNode2(
            Context,
            categoryNode,
            fileName ? fileName : L"InjectedSource",
            PhFormatString(
                L"VirtualFile: %s, ObjectFile: %s, Crc: 0x%08lx, Compression: %lu",
                virtualFileName ? virtualFileName : L"",
                objectFileName ? objectFileName : L"",
                crc,
                compression
                )
            );

        if (node)
            node->Size = length;

        if (virtualFileName)
            PhSymbolProviderFreeDiaString(virtualFileName);
        if (objectFileName)
            PhSymbolProviderFreeDiaString(objectFileName);
        if (fileName)
            PhSymbolProviderFreeDiaString(fileName);

        IDiaInjectedSource_Release(injectedSource);
    }

    IDiaEnumInjectedSources_Release(enumInjectedSources);
}

VOID PePdbDumpDiaSectionContribs(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaEnumSectionContribs* EnumSectionContribs,
    _In_ PPV_SYMBOL_NODE Root
    )
{
    IDiaSectionContrib* sectionContrib;
    ULONG count = 0;
    PPV_SYMBOL_NODE categoryNode;

    categoryNode = PePdbCreateSyntheticSymbolNode2(Context, Root, L"Sections", NULL);

    while (IDiaEnumSectionContribs_Next(EnumSectionContribs, 1, &sectionContrib, &count) == S_OK && count == 1)
    {
        ULONG addressSection = 0;
        ULONG addressOffset = 0;
        ULONG rva = 0;
        ULONG length = 0;
        ULONG dataCrc = 0;
        ULONG relocationsCrc = 0;
        ULONG compilandId = 0;
        BOOL code = FALSE;
        BOOL initializedData = FALSE;
        BOOL uninitializedData = FALSE;
        BOOL execute = FALSE;
        BOOL read = FALSE;
        BOOL write = FALSE;
        PPH_STRING name;
        PPV_SYMBOL_NODE node;

        IDiaSectionContrib_get_addressSection(sectionContrib, &addressSection);
        IDiaSectionContrib_get_addressOffset(sectionContrib, &addressOffset);
        IDiaSectionContrib_get_relativeVirtualAddress(sectionContrib, &rva);
        IDiaSectionContrib_get_length(sectionContrib, &length);
        IDiaSectionContrib_get_dataCrc(sectionContrib, &dataCrc);
        IDiaSectionContrib_get_relocationsCrc(sectionContrib, &relocationsCrc);
        IDiaSectionContrib_get_compilandId(sectionContrib, &compilandId);
        IDiaSectionContrib_get_code(sectionContrib, &code);
        IDiaSectionContrib_get_initializedData(sectionContrib, &initializedData);
        IDiaSectionContrib_get_uninitializedData(sectionContrib, &uninitializedData);
        IDiaSectionContrib_get_execute(sectionContrib, &execute);
        IDiaSectionContrib_get_read(sectionContrib, &read);
        IDiaSectionContrib_get_write(sectionContrib, &write);

        name = PhFormatString(L"%lu:%08lx", addressSection, addressOffset);
        node = PePdbCreateSyntheticSymbolNode2(
            Context,
            categoryNode,
            name->Buffer,
            PhFormatString(
                L"CompilandId: %lu, Flags: %s%s%s%s%s%s, DataCrc: 0x%08lx, RelocationsCrc: 0x%08lx",
                compilandId,
                code ? L"Code " : L"",
                initializedData ? L"InitializedData " : L"",
                uninitializedData ? L"UninitializedData " : L"",
                execute ? L"Execute " : L"",
                read ? L"Read " : L"",
                write ? L"Write" : L"",
                dataCrc,
                relocationsCrc
                )
            );

        if (node)
        {
            PePdbSetSyntheticSymbolRva(node, rva);
            PePdbSetSyntheticSymbolOffset(node, addressOffset);
            node->Size = length;
        }

        PhDereferenceObject(name);
        IDiaSectionContrib_Release(sectionContrib);
    }
}

VOID PePdbDumpDiaSegments(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaEnumSegments* EnumSegments,
    _In_ PPV_SYMBOL_NODE Root
    )
{
    IDiaSegment* segment;
    ULONG count = 0;
    PPV_SYMBOL_NODE categoryNode;

    categoryNode = PePdbCreateSyntheticSymbolNode2(Context, Root, L"SegmentMap", NULL);

    while (IDiaEnumSegments_Next(EnumSegments, 1, &segment, &count) == S_OK && count == 1)
    {
        ULONG frame = 0;
        ULONG offset = 0;
        ULONG length = 0;
        ULONG addressSection = 0;
        ULONG rva = 0;
        BOOL read = FALSE;
        BOOL write = FALSE;
        BOOL execute = FALSE;
        PPH_STRING name;
        PPV_SYMBOL_NODE node;

        IDiaSegment_get_frame(segment, &frame);
        IDiaSegment_get_offset(segment, &offset);
        IDiaSegment_get_length(segment, &length);
        IDiaSegment_get_addressSection(segment, &addressSection);
        IDiaSegment_get_relativeVirtualAddress(segment, &rva);
        IDiaSegment_get_read(segment, &read);
        IDiaSegment_get_write(segment, &write);
        IDiaSegment_get_execute(segment, &execute);

        name = PhFormatString(L"Frame %lu", frame);
        node = PePdbCreateSyntheticSymbolNode2(
            Context,
            categoryNode,
            name->Buffer,
            PhFormatString(
                L"AddressSection: %lu, Flags: %s%s%s",
                addressSection,
                read ? L"Read " : L"",
                write ? L"Write " : L"",
                execute ? L"Execute" : L""
                )
            );

        if (node)
        {
            PePdbSetSyntheticSymbolRva(node, rva);
            PePdbSetSyntheticSymbolOffset(node, offset);
            node->Size = length;
        }

        PhDereferenceObject(name);
        IDiaSegment_Release(segment);
    }
}

VOID PePdbDumpDiaFrameData(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaEnumFrameData* EnumFrameData,
    _In_ PPV_SYMBOL_NODE Root
    )
{
    IDiaFrameData* frameData;
    ULONG count = 0;
    PPV_SYMBOL_NODE categoryNode;

    categoryNode = PePdbCreateSyntheticSymbolNode2(Context, Root, L"FrameData", NULL);

    while (IDiaEnumFrameData_Next(EnumFrameData, 1, &frameData, &count) == S_OK && count == 1)
    {
        BSTR program = NULL;
        ULONG addressSection = 0;
        ULONG addressOffset = 0;
        ULONG rva = 0;
        ULONG lengthBlock = 0;
        ULONG lengthLocals = 0;
        ULONG lengthParams = 0;
        ULONG maxStack = 0;
        ULONG lengthProlog = 0;
        ULONG type = 0;
        BOOL functionStart = FALSE;
        BOOL allocatesBasePointer = FALSE;
        PPH_STRING name;
        PPV_SYMBOL_NODE node;

        IDiaFrameData_get_addressSection(frameData, &addressSection);
        IDiaFrameData_get_addressOffset(frameData, &addressOffset);
        IDiaFrameData_get_relativeVirtualAddress(frameData, &rva);
        IDiaFrameData_get_lengthBlock(frameData, &lengthBlock);
        IDiaFrameData_get_lengthLocals(frameData, &lengthLocals);
        IDiaFrameData_get_lengthParams(frameData, &lengthParams);
        IDiaFrameData_get_maxStack(frameData, &maxStack);
        IDiaFrameData_get_lengthProlog(frameData, &lengthProlog);
        IDiaFrameData_get_type(frameData, &type);
        IDiaFrameData_get_functionStart(frameData, &functionStart);
        IDiaFrameData_get_allocatesBasePointer(frameData, &allocatesBasePointer);
        IDiaFrameData_get_program(frameData, &program);

        name = PhFormatString(L"%lu:%08lx", addressSection, addressOffset);
        node = PePdbCreateSyntheticSymbolNode2(
            Context,
            categoryNode,
            name->Buffer,
            PhFormatString(
                L"Locals: %lu, Params: %lu, MaxStack: %lu, Prolog: %lu, Type: %lu, FunctionStart: %s, AllocatesBasePointer: %s, Program: %s",
                lengthLocals,
                lengthParams,
                maxStack,
                lengthProlog,
                type,
                functionStart ? L"true" : L"false",
                allocatesBasePointer ? L"true" : L"false",
                program ? program : L""
                )
            );

        if (node)
        {
            PePdbSetSyntheticSymbolRva(node, rva);
            PePdbSetSyntheticSymbolOffset(node, addressOffset);
            node->Size = lengthBlock;
        }

        PhDereferenceObject(name);

        if (program)
            PhSymbolProviderFreeDiaString(program);

        IDiaFrameData_Release(frameData);
    }
}

VOID PePdbDumpDiaTables(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSession* DiaSession,
    _In_ PPV_SYMBOL_NODE Root
    )
{
    IDiaEnumTables* enumTables;
    IDiaTable* diaTable;
    ULONG count = 0;
    BOOLEAN dumpedSectionContribs = FALSE;
    BOOLEAN dumpedSegments = FALSE;
    BOOLEAN dumpedFrameData = FALSE;

    if (IDiaSession_getEnumTables(DiaSession, &enumTables) != S_OK)
        return;

    while (IDiaEnumTables_Next(enumTables, 1, &diaTable, &count) == S_OK && count == 1)
    {
        IDiaEnumSectionContribs* enumSectionContribs = NULL;
        IDiaEnumSegments* enumSegments = NULL;
        IDiaEnumFrameData* enumFrameData = NULL;

        if (!dumpedSectionContribs && IUnknown_QueryInterface((IUnknown*)diaTable, &IID_IDiaEnumSectionContribs, (void**)&enumSectionContribs) == S_OK)
        {
            PePdbDumpDiaSectionContribs(Context, enumSectionContribs, Root);
            IDiaEnumSectionContribs_Release(enumSectionContribs);
            dumpedSectionContribs = TRUE;
        }

        if (!dumpedSegments && IUnknown_QueryInterface((IUnknown*)diaTable, &IID_IDiaEnumSegments, (void**)&enumSegments) == S_OK)
        {
            PePdbDumpDiaSegments(Context, enumSegments, Root);
            IDiaEnumSegments_Release(enumSegments);
            dumpedSegments = TRUE;
        }

        if (!dumpedFrameData && IUnknown_QueryInterface((IUnknown*)diaTable, &IID_IDiaEnumFrameData, (void**)&enumFrameData) == S_OK)
        {
            PePdbDumpDiaFrameData(Context, enumFrameData, Root);
            IDiaEnumFrameData_Release(enumFrameData);
            dumpedFrameData = TRUE;
        }

        IDiaTable_Release(diaTable);
    }

    IDiaEnumTables_Release(enumTables);
}

VOID PePdbDumpGlobalScopeChildren(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSession* DiaSession,
    _In_ PPV_SYMBOL_NODE Root
    )
{
    IDiaEnumSymbols* rootEnumSymbols;
    IDiaSymbol* compilandSymbol;
    ULONG celt = 0;
    ULONG iMod = 1;

    for (ULONG i = 0; i < SymTagMax; i++)
    {
        if (SUCCEEDED(IDiaSession_findChildren(DiaSession, NULL, i, NULL, nsNone, &rootEnumSymbols)))
        {
            while (SUCCEEDED(IDiaEnumSymbols_Next(rootEnumSymbols, 1, &compilandSymbol, &celt)) && (celt == 1))
            {
                switch (i)
                {
                case SymTagExe:
                case SymTagCompiland:
                case SymTagCompilandDetails:
                case SymTagCompilandEnv:
                    PePdbEnumDiaSymbol2(Context, compilandSymbol, Root);
                    break;
                default:
                    PePdbPrintDiaSymbol(Context, compilandSymbol, Root);
                    break;
                }

                IDiaSymbol_Release(compilandSymbol);
            }

            IDiaEnumSymbols_Release(rootEnumSymbols);
        }
    }

    PePdbDumpDiaSourceFiles(Context, DiaSession, Root);
    PePdbDumpDiaLineNumbers(Context, DiaSession, Root);
    PePdbDumpDiaTables(Context, DiaSession, Root);
    PePdbDumpDiaInjectedSource(Context, DiaSession, Root);
    PePdbDumpDiaInputAssemblyFiles(Context, DiaSession, Root);

    //if (IDiaSession_getEnumTables(DiaSession, &enumTables) != S_OK)
    //    return;
    //
    //while (IDiaEnumTables_Next(enumTables, 1, &diaTable, &count) == S_OK && count == 1)
    //{
    //    BSTR bstrName = NULL;
    //    IUnknown* unknown;
    //    ULONG fetched = 0;
    //    PPV_SYMBOL_NODE tableNode;
    //
    //    IDiaTable_get_name(diaTable, &bstrName);
    //    tableNode = PePdbCreateSyntheticSymbolNode(
    //        Context,
    //        Root,
    //        bstrName ? bstrName : L"Unnamed Table",
    //        &tableText
    //        );
    //
    //    while (IDiaTable_Next(diaTable, 1, &unknown, &fetched) == S_OK && fetched == 1)
    //    {
    //        IDiaSymbol* tableSymbol = NULL;
    //
    //        if (IUnknown_QueryInterface(unknown, &IID_IDiaSymbol, (void**)&tableSymbol) == S_OK)
    //        {
    //            PePdbPrintDiaSymbol(Context, tableSymbol, tableNode);
    //            IDiaSymbol_Release(tableSymbol);
    //        }
    //
    //        IUnknown_Release(unknown);
    //    }
    //
    //    if (bstrName)
    //        PhSymbolProviderFreeDiaString(bstrName);
    //
    //    IDiaTable_Release(diaTable);
    //}
    //
    //IDiaEnumTables_Release(enumTables);
    //
    // static PH_STRINGREF tableText = PH_STRINGREF_INIT(L"Table");
    // IDiaEnumTables* enumTables;
    // IDiaTable* diaTable;
    // ULONG count = 0;
    //
    // if (IDiaSession_getEnumTables(DiaSession, &enumTables) != S_OK)
    //     return;
    //
    // while (IDiaEnumTables_Next(enumTables, 1, &diaTable, &count) == S_OK && count == 1)
    // {
    //     BSTR bstrName = NULL;
    //     IUnknown* unknown;
    //     ULONG fetched = 0;
    //     PPV_SYMBOL_NODE tableNode;
    //
    //     IDiaTable_get_name(diaTable, &bstrName);
    //     tableNode = PePdbCreateSyntheticSymbolNode(
    //         Context,
    //         Root,
    //         bstrName ? bstrName : L"Unnamed Table",
    //         &tableText
    //         );
    //
    //     while (IDiaTable_Next(diaTable, 1, &unknown, &fetched) == S_OK && fetched == 1)
    //     {
    //         IDiaSymbol* tableSymbol = NULL;
    //         IDiaEnumFrameData* tableFrames = NULL;
    //         IDiaEnumSegments* segmentData = NULL;
    //         IDiaEnumSourceFiles* sourceFiles = NULL;
    //         IDiaEnumSectionContribs* sectionContribs = NULL;
    //         IDiaEnumDebugStreamData* debugStreamData = NULL;
    //
    //         if (IUnknown_QueryInterface(unknown, &IID_IDiaSymbol, (void**)&tableSymbol) == S_OK)
    //         {
    //             PePdbPrintDiaSymbol(Context, tableSymbol, tableNode);
    //             IUnknown_Release(tableSymbol);
    //         }
    //
    //         if (IUnknown_QueryInterface(unknown, &IID_IDiaEnumFrameData, (void**)&tableFrames) == S_OK)
    //         {
    //             IUnknown_Release(tableFrames);
    //         }
    //
    //         if (IUnknown_QueryInterface(unknown, &IID_IDiaEnumSegments, (void**)&segmentData) == S_OK)
    //         {
    //             IUnknown_Release(sourceFiles);
    //         }
    //
    //         if (IUnknown_QueryInterface(unknown, &IID_IDiaEnumSourceFiles, (void**)&sourceFiles) == S_OK)
    //         {
    //             IUnknown_Release(sourceFiles);
    //         }
    //
    //         if (IUnknown_QueryInterface(unknown, &IID_IDiaEnumSectionContribs, (void**)&sectionContribs) == S_OK)
    //         {
    //             IUnknown_Release(sectionContribs);
    //         }
    //
    //         if (IUnknown_QueryInterface(unknown, &IID_IDiaEnumDebugStreamData, (void**)&debugStreamData) == S_OK)
    //         {
    //             IUnknown_Release(debugStreamData);
    //         }
    //         IUnknown_Release(unknown);
    //     }
    //
    //     if (bstrName)
    //         PhSymbolProviderFreeDiaString(bstrName);
    //
    //     IDiaTable_Release(diaTable);
    // }
    //
    //IDiaEnumTables_Release(enumTables);
}

BOOLEAN DumpAllGlobals(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSymbol* IDiaGlobalSymbol
    )
{
    IDiaEnumSymbols* idiaEnumSymbols;
    IDiaSymbol* idiaSymbol;
    enum SymTagEnum diaSymTags[] = { SymTagFunction, SymTagThunk, SymTagData };
    ULONG count = 0;

    for (ULONG i = 0; i < RTL_NUMBER_OF(diaSymTags); i++)
    {
        if (IDiaSymbol_findChildrenEx(IDiaGlobalSymbol, diaSymTags[i], NULL, nsNone, &idiaEnumSymbols) != S_OK)
            continue;

        while (IDiaEnumSymbols_Next(idiaEnumSymbols, 1, &idiaSymbol, &count) == S_OK && count == 1)
        {
            PePdbPrintDiaSymbol(Context, idiaSymbol, NULL);

            IDiaSymbol_Release(idiaSymbol);
        }

        IDiaEnumSymbols_Release(idiaEnumSymbols);
    }

    return TRUE;
}

BOOLEAN DumpAllPublics(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSymbol* IDiaGlobalSymbol
    )
{
    IDiaEnumSymbols* idiaEnumSymbols;
    IDiaSymbol* idiaSymbol;
    ULONG count = 0;

    if (IDiaSymbol_findChildrenEx(IDiaGlobalSymbol, SymTagPublicSymbol, NULL, nsNone, &idiaEnumSymbols) != S_OK)
        return FALSE;

    while (IDiaEnumSymbols_Next(idiaEnumSymbols, 1, &idiaSymbol, &count) == S_OK && count == 1)
    {
        PePdbPrintDiaSymbol(Context, idiaSymbol, NULL);

        IDiaSymbol_Release(idiaSymbol);
    }

    IDiaEnumSymbols_Release(idiaEnumSymbols);

    return TRUE;
}

BOOLEAN DumpAllUDTs(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSymbol* IDiaGlobalSymbol
    )
{
    IDiaEnumSymbols* idiaEnumSymbols;
    IDiaSymbol* idiaSymbol;
    ULONG count = 0;

    if (IDiaSymbol_findChildrenEx(IDiaGlobalSymbol, SymTagUDT, NULL, nsNone, &idiaEnumSymbols) != S_OK)
        return FALSE;

    while (IDiaEnumSymbols_Next(idiaEnumSymbols, 1, &idiaSymbol, &count) == S_OK && count == 1)
    {
        PePdbPrintDiaSymbol(Context, idiaSymbol, NULL);
        //PrintTypeInDetail(Context, idiaSymbol);

        IDiaSymbol_Release(idiaSymbol);
    }

    IDiaEnumSymbols_Release(idiaEnumSymbols);

    return TRUE;
}

BOOLEAN DumpAllEnums(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSymbol* IDiaGlobalSymbol
    )
{
    IDiaEnumSymbols* idiaEnumSymbols;
    IDiaSymbol* idiaSymbol;
    ULONG count = 0;

    if (IDiaSymbol_findChildrenEx(IDiaGlobalSymbol, SymTagEnum, NULL, nsNone, &idiaEnumSymbols) != S_OK)
        return FALSE;

    while (IDiaEnumSymbols_Next(idiaEnumSymbols, 1, &idiaSymbol, &count) == S_OK && count == 1)
    {
        PePdbPrintDiaSymbol(Context, idiaSymbol, NULL);
        //PrintTypeInDetail(Context, idiaSymbol);

        IDiaSymbol_Release(idiaSymbol);
    }

    IDiaEnumSymbols_Release(idiaEnumSymbols);

    return TRUE;
}

BOOLEAN DumpAllTypedefs(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSymbol* IDiaGlobalSymbol
    )
{
    IDiaEnumSymbols* idiaEnumSymbols;
    IDiaSymbol* idiaSymbol;
    ULONG count = 0;

    if (IDiaSymbol_findChildrenEx(IDiaGlobalSymbol, SymTagTypedef, NULL, nsNone, &idiaEnumSymbols) != S_OK)
        return FALSE;

    while (IDiaEnumSymbols_Next(idiaEnumSymbols, 1, &idiaSymbol, &count) == S_OK && count == 1)
    {
        PePdbPrintDiaSymbol(Context, idiaSymbol, NULL);
        //PrintTypeInDetail(Context, idiaSymbol);

        IDiaSymbol_Release(idiaSymbol);
    }

    IDiaEnumSymbols_Release(idiaEnumSymbols);

    return TRUE;
}

NTSTATUS PeDumpFileSymbols(
    _In_ PPDB_SYMBOL_CONTEXT Context
    )
{
    PVOID baseOfDll = NULL;
    IDiaSession* idiaSession;
    IDiaSymbol* idiaSymbol;

    if (PvMappedImage.Signature) // HACK: Null when opening a pdb file.
    {
        if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            baseOfDll = PTR_ADD_OFFSET(UlongToPtr(PvMappedImage.NtHeaders32->OptionalHeader.ImageBase), 0);
        else
            baseOfDll = PTR_ADD_OFFSET(PvMappedImage.NtHeaders->OptionalHeader.ImageBase, 0);
    }
    else
    {
        NTSTATUS status;
        HANDLE fileHandle;

        status = PhCreateFileWin32(
            &fileHandle,
            PhGetString(PvFileName),
            FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );

        if (NT_SUCCESS(status))
        {
            PVOID viewBase;
            SIZE_T size;

            status = PhMapViewOfEntireFile(
                PhGetString(PvFileName),
                fileHandle,
                &viewBase,
                &size
                );

            if (!NT_SUCCESS(status))
            {
                PhShowStatus(NULL, L"Unable to load the file.", status, 0);
                return status;
            }

            if (PvpLoadDbgHelp(&PvSymbolProvider))
            {
                if (PhLoadFileNameSymbolProvider(
                    PvSymbolProvider,
                    PvFileName,
                    viewBase,
                    (ULONG)size
                    ))
                {
                    baseOfDll = viewBase;
                }
            }

            NtClose(fileHandle);
        }

        if (!NT_SUCCESS(status))
        {
            PhShowStatus(NULL, L"Unable to load the file.", status, 0);
            return status;
        }
    }

    if (!baseOfDll)
    {
        PostMessage(Context->WindowHandle, WM_PV_SEARCH_FINISHED, 0, 0);
        PhShowStatus(NULL, L"Unable to load the file.", STATUS_UNSUCCESSFUL, 0);
        return STATUS_UNSUCCESSFUL;
    }

    if (!PhGetSymbolProviderDiaSession(
        PvSymbolProvider,
        baseOfDll,
        &idiaSession
        ))
    {
        PostMessage(Context->WindowHandle, WM_PV_SEARCH_FINISHED, 0, 0);
        return STATUS_UNSUCCESSFUL;
    }

    if (IDiaSession_get_globalScope(idiaSession, &idiaSymbol) == S_OK)
    {
        PPV_SYMBOL_NODE rootNode;

        Context->IDiaSession = idiaSession; // HACK

        rootNode = PePdbCreateDiaSymbolNode(Context, idiaSymbol, NULL);

        if (rootNode)
        {
            PePdbDumpGlobalScopeChildren(Context, idiaSession, rootNode);
        }

        IDiaSymbol_Release(idiaSymbol);
    }

    //IDiaSession_Release(idiaSession);

    PostMessage(Context->WindowHandle, WM_PV_SEARCH_FINISHED, 0, 0);
    return STATUS_SUCCESS;
}

VOID PdbDumpAddress(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG_PTR Rva,
    _In_opt_ PPV_SYMBOL_NODE Parent
    )
{
    IDiaSymbol* idiaSymbol;
    LONG displacement;

    if (IDiaSession_findSymbolByRVAEx(
        (IDiaSession*)Context->IDiaSession,
        (ULONG)Rva,
        SymTagNull,
        &idiaSymbol,
        &displacement
        ) == S_OK)
    {
        DumpAllPublics(Context, idiaSymbol);
        DumpAllGlobals(Context, idiaSymbol);

        DumpAllUDTs(Context, idiaSymbol);
        DumpAllEnums(Context, idiaSymbol);
        DumpAllTypedefs(Context, idiaSymbol);

        IDiaSymbol_Release(idiaSymbol);
    }
}

//VOID EnumPrintTags(IDiaSession* IDiaSession, IDiaSymbol* Symbol, PWSTR Name)
//{
//    IDiaEnumSymbols* idiaEnumSymbols;
//    IDiaSymbol* idiaSymbol;
//    ULONG count = 0;
//
//    if (IDiaSession_findChildrenEx(
//        IDiaSession,
//        Symbol,
//        SymTagNull,
//        Name,
//        nsNone,
//        &idiaEnumSymbols
//        ) == S_OK)
//    {
//        while (IDiaEnumSymbols_Next(idiaEnumSymbols, 1, &idiaSymbol, &count) == S_OK && count == 1)
//        {
//            PrintTypeInDetail(&sb, idiaSymbol, 0);
//
//            EnumPrintTags(
//            IDiaSymbol_Release(idiaSymbol);
//        }
//    }
//
//    IDiaEnumSymbols_Release(idiaEnumSymbols);
//}

PPH_STRING PdbGetSymbolDetails(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_opt_ PPH_STRING Name,
    _In_opt_ ULONG Rva
    )
{
    IDiaSymbol* idiaSymbol;
    LONG displacement;
    PH_STRING_BUILDER sb;

    PhInitializeStringBuilder(&sb, 0x100);

    if (Name)
    {
        IDiaEnumSymbols* idiaEnumSymbols;
        IDiaSymbol* idiaGlobalSymbol;
        ULONG count;

        if (IDiaSession_get_globalScope(
            (IDiaSession*)Context->IDiaSession,
            &idiaGlobalSymbol
            ) == S_OK)
        {
            //IDiaSession_symbolById(
            //    (IDiaSession*)Context->IDiaSession,
            //    0,
            //    &idiaSymbol
            //    );

            if (IDiaSession_findChildrenEx(
                (IDiaSession*)Context->IDiaSession,
                idiaGlobalSymbol, // needs parent
                SymTagNull,
                Name->Buffer,
                nsNone,
                &idiaEnumSymbols
                ) == S_OK)
            {
                while (IDiaEnumSymbols_Next(idiaEnumSymbols, 1, &idiaSymbol, &count) == S_OK && count == 1)
                {
                    PrintTypeInDetail(&sb, idiaSymbol, 0);

                    IDiaSymbol_Release(idiaSymbol);
                }
            }

            IDiaEnumSymbols_Release(idiaEnumSymbols);
        }
    }
    else
    {
        if (IDiaSession_findSymbolByRVAEx(
            (IDiaSession*)Context->IDiaSession,
            Rva,
            SymTagNull,
            &idiaSymbol,
            &displacement
            ) == S_OK)
        {
            PrintTypeInDetail(&sb, idiaSymbol, 0);

            IDiaSymbol_Release(idiaSymbol);
        }
    }

    return PhFinalStringBuilderString(&sb);
}
