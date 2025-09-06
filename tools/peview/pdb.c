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
    _In_ VARIANT var)
{
    switch (V_VT(&var))
    {
    case VT_UI1:
    case VT_I1:
        PhAppendFormatStringBuilder(StringBuilder, L" 0x%X", V_UI1(&var));
        break;
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        PhAppendFormatStringBuilder(StringBuilder, L" 0x%X", V_I2(&var));
        break;
    case VT_I4:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:
        PhAppendFormatStringBuilder(StringBuilder, L" 0x%X", V_I4(&var));
        break;
    case VT_R4:
        PhAppendFormatStringBuilder(StringBuilder, L" %g", V_R4(&var));
        break;
    case VT_R8:
        PhAppendFormatStringBuilder(StringBuilder, L" %g", V_R8(&var));
        break;
    case VT_BSTR:
        PhAppendFormatStringBuilder(StringBuilder, L" \"%s\"", V_BSTR(&var));
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

            if (IDiaSymbol_get_value(IDiaSymbol, &vt) == S_OK)
            {
                PrintVariant(StringBuilder, vt);

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
                DWORD dwCountElems;
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
}

VOID PePdbPrintDiaSymbol(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ IDiaSymbol* IDiaSymbol
    )
{
    ULONG symbolTag = SymTagNull;
    ULONG dwDataKind = DataIsUnknown;
    ULONG symbolId = 0;
    ULONG symbolRva = 0;
    ULONG dwSeg = 0;
    ULONG dwOff = 0;
    ULONGLONG symbolLength = 0;
    BSTR bstrName = NULL;
    BSTR bstrUndname = NULL;

    if (IDiaSymbol_get_symTag(IDiaSymbol, &symbolTag) != S_OK)
        return;

    IDiaSymbol_get_dataKind(IDiaSymbol, &dwDataKind);
    IDiaSymbol_get_typeId(IDiaSymbol, &symbolId);
    IDiaSymbol_get_relativeVirtualAddress(IDiaSymbol, &symbolRva);
    IDiaSymbol_get_addressSection(IDiaSymbol, &dwSeg);
    IDiaSymbol_get_addressOffset(IDiaSymbol, &dwOff);
    IDiaSymbol_get_length(IDiaSymbol, &symbolLength);
    IDiaSymbol_get_name(IDiaSymbol, &bstrName);

    if (IDiaSymbol_get_undecoratedNameEx(IDiaSymbol, UNDNAME_COMPLETE, &bstrUndname) != S_OK)
    {
        IDiaSymbol_get_undecoratedName(IDiaSymbol, &bstrUndname);
    }

    switch (symbolTag)
    {
    case SymTagFunction:
        {
            PPV_SYMBOL_NODE symbol;

            symbol = PhAllocateZero(sizeof(PV_SYMBOL_NODE));
            symbol->UniqueId = ++Context->Count;
            symbol->TypeId = symbolId;
            symbol->Type = PV_SYMBOL_TYPE_FUNCTION;
            symbol->Address = symbolRva;
            symbol->Size = symbolLength;
            symbol->Name = PhCreateString(bstrUndname ? bstrUndname : bstrName);
            symbol->Data = rgTags[symbolTag];
            //symbol->Data = SymbolInfo_GetTypeName(
            //    context,
            //    SymbolInfo->TypeIndex,
            //    SymbolInfo->Name
            //);
            //SymbolInfo_SymbolLocationStr(SymbolInfo, symbol->Pointer);
            PhPrintPointer(symbol->Pointer, UlongToPtr(symbolRva));
            PhPrintInt64(symbol->Index, symbol->UniqueId);

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
            PhAddItemList(SearchResults, symbol);
            PhReleaseQueuedLockExclusive(&SearchResultsLock);

            // Enumerate parameters and variables...
            PdbDumpAddress(Context, symbolRva);
        }
        break;
    case SymTagData:
        {
            PPV_SYMBOL_NODE symbol;
            //PWSTR symDataKind;
            //ULONG dataKindType = 0;

            //if (symbolRva == 0)
            //    break;

           /*
            if (!SymGetTypeInfo_I(
                NtCurrentProcess(),
                SymbolInfo->ModBase,
                SymbolInfo->Index,
                TI_GET_DATAKIND,
                &dataKindType
                ))
            {
                break;
            }

            symDataKind = SymbolInfo_DataKindStr(dataKindType);

            if (
                dataKindType == DataIsLocal ||
                dataKindType == DataIsParam ||
                dataKindType == DataIsObjectPtr
                )
            {
                break;
            }*/

            symbol = PhAllocate(sizeof(PV_SYMBOL_NODE));
            memset(symbol, 0, sizeof(PV_SYMBOL_NODE));

            switch (dwDataKind)
            {
            case DataIsLocal:
                {
                    // TODO: The address variable is FUNCTION+OFFSET
                    //SymbolInfo->Address = SymbolInfo->Address;
                    symbol->Type = PV_SYMBOL_TYPE_LOCAL_VAR;
                }
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
                symbol->Type = PV_SYMBOL_TYPE_STRUCT;
                break;
            case DataIsStaticMember:
                symbol->Type = PV_SYMBOL_TYPE_STATIC_MEMBER;
                break;
            case DataIsConstant:
                symbol->Type = PV_SYMBOL_TYPE_CONSTANT;
                break;
            default:
                symbol->Type = PV_SYMBOL_TYPE_UNKNOWN;
                break;
            }

            symbol->UniqueId = ++Context->Count;
            symbol->TypeId = symbolId;
            symbol->Address = symbolRva;
            symbol->Size = symbolLength;
            symbol->Name = PhCreateString(bstrUndname ? bstrUndname : bstrName);
            //PhCreateStringEx(SymbolInfo->Name, SymbolInfo->NameLen * sizeof(WCHAR));
            symbol->Data = rgTags[symbolTag];
            //symbol->Data = SymbolInfo_GetTypeName(context, SymbolInfo->TypeIndex, SymbolInfo->Name);
            //SymbolInfo_SymbolLocationStr(SymbolInfo, symbol->Pointer);
            PhPrintPointer(symbol->Pointer, UlongToPtr(symbolRva));
            PhPrintInt64(symbol->Index, symbol->UniqueId);

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
            PhAddItemList(SearchResults, symbol);
            PhReleaseQueuedLockExclusive(&SearchResultsLock);
        }
        break;
    default:
        {
            PPV_SYMBOL_NODE symbol;

            //if (symbolRva == 0)
            //    break;

            symbol = PhAllocateZero(sizeof(PV_SYMBOL_NODE));
            symbol->UniqueId = ++Context->Count;
            symbol->TypeId = symbolId;
            symbol->Type = PV_SYMBOL_TYPE_SYMBOL;
            symbol->Address = symbolRva;
            symbol->Size = symbolLength;
            symbol->Name = PhCreateString(bstrUndname ? bstrUndname : bstrName);
            symbol->Data = rgTags[symbolTag];
            //symbol->Data = SymbolInfo_GetTypeName(context, SymbolInfo->TypeIndex, SymbolInfo->Name);
            //SymbolInfo_SymbolLocationStr(SymbolInfo, symbol->Pointer);
            PhPrintPointer(symbol->Pointer, UlongToPtr(symbolRva));
            PhPrintInt64(symbol->Index, symbol->UniqueId);

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

            //if (SymbolInfo->Name[0]) // HACK
            //{
            //    if (SymbolInfo->NameLen)
            //        symbol->Name = PhCreateStringEx(SymbolInfo->Name, SymbolInfo->NameLen * sizeof(WCHAR));
            //    else
            //        symbol->Name = PhCreateString(SymbolInfo->Name);
            //}

            PhAcquireQueuedLockExclusive(&SearchResultsLock);
            PhAddItemList(SearchResults, symbol);
            PhReleaseQueuedLockExclusive(&SearchResultsLock);
        }
        break;
    }

    if (bstrUndname)
        PhSymbolProviderFreeDiaString(bstrUndname);
    if (bstrName)
        PhSymbolProviderFreeDiaString(bstrName);
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
            PePdbPrintDiaSymbol(Context, idiaSymbol);

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
        PePdbPrintDiaSymbol(Context, idiaSymbol);

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
        PePdbPrintDiaSymbol(Context, idiaSymbol);
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
        PePdbPrintDiaSymbol(Context, idiaSymbol);
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
        PePdbPrintDiaSymbol(Context, idiaSymbol);
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
            baseOfDll = PTR_ADD_OFFSET(PvMappedImage.NtHeaders32->OptionalHeader.ImageBase, 0);
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
        Context->IDiaSession = idiaSession; // HACK

        DumpAllPublics(Context, idiaSymbol);
        DumpAllGlobals(Context, idiaSymbol);

        DumpAllUDTs(Context, idiaSymbol);
        DumpAllEnums(Context, idiaSymbol);
        DumpAllTypedefs(Context, idiaSymbol);

        IDiaSymbol_Release(idiaSymbol);
    }

    //IDiaSession_Release(idiaSession);

    PostMessage(Context->WindowHandle, WM_PV_SEARCH_FINISHED, 0, 0);
    return STATUS_SUCCESS;
}

VOID PdbDumpAddress(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Rva
    )
{
    IDiaSymbol* idiaSymbol;
    LONG displacement;

    if (IDiaSession_findSymbolByRVAEx(
        (IDiaSession*)Context->IDiaSession,
        Rva,
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
