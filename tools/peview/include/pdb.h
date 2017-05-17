/*
 * Process Hacker -
 *   property sheet 
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

#ifndef PV_PDB_H
#define PV_PDB_H

// Originally based on dbghelptypeinfo by Oleg Starodumov:
// http://www.debuginfo.com/articles/dbghelptypeinfo.html

#define _NO_CVCONST_H
#define DBGHELP_TRANSLATE_TCHAR
#include <dbghelp.h>

// Types from Cvconst.h (DIA SDK) 
#ifdef _NO_CVCONST_H

// BasicType, originally from CVCONST.H in DIA SDK 
typedef enum _BasicType
{
    btNoType = 0,
    btVoid = 1,
    btChar = 2,
    btWChar = 3,
    btInt = 6,
    btUInt = 7,
    btFloat = 8,
    btBCD = 9,
    btBool = 10,
    btLong = 13,
    btULong = 14,
    btCurrency = 25,
    btDate = 26,
    btVariant = 27,
    btComplex = 28,
    btBit = 29,
    btBSTR = 30,
    btHresult = 31
} BasicType;

// UDtKind, originally from CVCONST.H in DIA SDK 
typedef enum _UdtKind
{
    UdtStruct,
    UdtClass,
    UdtUnion
} UdtKind;

// CV_call_e, originally from CVCONST.H in DIA SDK 
typedef enum _CV_call_e 
{
    CV_CALL_NEAR_C      = 0x00, // near right to left push, caller pops stack
    CV_CALL_FAR_C       = 0x01, // far right to left push, caller pops stack
    CV_CALL_NEAR_PASCAL = 0x02, // near left to right push, callee pops stack
    CV_CALL_FAR_PASCAL  = 0x03, // far left to right push, callee pops stack
    CV_CALL_NEAR_FAST   = 0x04, // near left to right push with regs, callee pops stack
    CV_CALL_FAR_FAST    = 0x05, // far left to right push with regs, callee pops stack
    CV_CALL_SKIPPED     = 0x06, // skipped (unused) call index
    CV_CALL_NEAR_STD    = 0x07, // near standard call
    CV_CALL_FAR_STD     = 0x08, // far standard call
    CV_CALL_NEAR_SYS    = 0x09, // near sys call
    CV_CALL_FAR_SYS     = 0x0a, // far sys call
    CV_CALL_THISCALL    = 0x0b, // this call (this passed in register)
    CV_CALL_MIPSCALL    = 0x0c, // Mips call
    CV_CALL_GENERIC     = 0x0d, // Generic call sequence
    CV_CALL_ALPHACALL   = 0x0e, // Alpha call
    CV_CALL_PPCCALL     = 0x0f, // PPC call
    CV_CALL_SHCALL      = 0x10, // Hitachi SuperH call
    CV_CALL_ARMCALL     = 0x11, // ARM call
    CV_CALL_AM33CALL    = 0x12, // AM33 call
    CV_CALL_TRICALL     = 0x13, // TriCore Call
    CV_CALL_SH5CALL     = 0x14, // Hitachi SuperH-5 call
    CV_CALL_M32RCALL    = 0x15, // M32R Call
    CV_CALL_RESERVED    = 0x16  // first unused call enumeration
} CV_call_e;

// DataKind, originally from CVCONST.H in DIA SDK 
typedef enum _DataKind
{
    DataIsUnknown,
    DataIsLocal,
    DataIsStaticLocal,
    DataIsParam,
    DataIsObjectPtr,
    DataIsFileStatic,
    DataIsGlobal,
    DataIsMember,
    DataIsStaticMember,
    DataIsConstant
} DataKind;

// CV_HREG_e, originally from CVCONST.H in DIA SDK 
typedef enum _CV_HREG_e 
{
    // Only a limited number of registers included here 

    CV_REG_EAX =  17,
    CV_REG_ECX =  18,
    CV_REG_EDX =  19,
    CV_REG_EBX =  20,
    CV_REG_ESP =  21,
    CV_REG_EBP =  22,
    CV_REG_ESI =  23,
    CV_REG_EDI =  24,
} CV_HREG_e;

// LocatonType, originally from CVCONST.H in DIA SDK 
typedef enum _LocationType
{
    LocIsNull,
    LocIsStatic,
    LocIsTLS,
    LocIsRegRel,
    LocIsThisRel,
    LocIsEnregistered,
    LocIsBitField,
    LocIsSlot,
    LocIsIlRel,
    LocInMetaData,
    LocIsConstant,
    LocTypeMax
} LocationType;

#endif // _NO_CVCONST_H

// Maximal length of name buffers (in characters) 
#define TIS_MAXNAMELEN  256
// Maximal number of children (member variables, member functions, base classes, etc.) 
#define TIS_MAXNUMCHILDREN  64
// Maximal number of dimensions of an array 
#define TIS_MAXARRAYDIMS    64

// SymTagBaseType symbol 
typedef struct _BaseTypeInfo 
{
    // Basic type (DIA: baseType) 
    BasicType BaseType; 

    // Length (in bytes) (DIA: length)
    ULONG64 Length; 
} BaseTypeInfo;

// SymTagTypedef symbol 
typedef struct _TypedefInfo
{
    // Name (DIA: name) 
    WCHAR Name[TIS_MAXNAMELEN]; 

    // Index of the underlying type (DIA: typeId) 
    ULONG TypeIndex; 
} TypedefInfo;

// SymTagPointerType symbol 
typedef struct _PointerTypeInfo
{
    // Length (in bytes) (DIA: length) 
    ULONG64 Length; 

    // Index of the type the pointer points to (DIA: typeId) 
    ULONG TypeIndex; 
} PointerTypeInfo;

// SymTagUDT symbol (Class or structure) 
typedef struct _UdtClassInfo
{
    // Name (DIA: name) 
    WCHAR Name[TIS_MAXNAMELEN]; 

    // Length (DIA: length) 
    ULONG64 Length; 

    // UDT kind (class, structure or union) (DIA: udtKind) 
    UdtKind UDTKind; 

    // Nested ("true" if the declaration is nested in another UDT) (DIA: nested) 
    BOOL Nested;

    // Number of member variables 
    ULONG NumVariables;

    // Member variables 
    ULONG Variables[TIS_MAXNUMCHILDREN]; 

    // Number of member functions 
    ULONG NumFunctions;

    // Member functions 
    ULONG Functions[TIS_MAXNUMCHILDREN]; 

    // Number of base classes 
    ULONG NumBaseClasses;

    // Base classes 
    ULONG BaseClasses[TIS_MAXNUMCHILDREN]; 
} UdtClassInfo;

// SymTagUDT symbol (Union) 
typedef struct _UdtUnionInfo
{
    // Name (DIA: name) 
    WCHAR Name[TIS_MAXNAMELEN]; 

    // Length (in bytes) (DIA: length) 
    ULONG64 Length; 

    // UDT kind (class, structure or union) (DIA: udtKind) 
    UdtKind UDTKind; 

    // Nested ("true" if the declaration is nested in another UDT) (DIA: nested) 
    BOOL Nested; 

    // Number of members 
    ULONG NumMembers;

    // Members 
    ULONG Members[TIS_MAXNUMCHILDREN]; 
} UdtUnionInfo;

// SymTagBaseClass symbol 
typedef struct _BaseClassInfo
{
    // Index of the UDT symbol that represents the base class (DIA: type) 
    ULONG TypeIndex; 

    // Virtual ("true" if the base class is a virtual base class) (DIA: virtualBaseClass) 
    BOOL Virtual;

    // Offset of the base class within the class/structure (DIA: offset) 
    // (defined only if Virtual is "false")
    LONG Offset; 

    // Virtual base pointer offset (DIA: virtualBasePointerOffset) 
    // (defined only if Virtual is "true")
    LONG VirtualBasePointerOffset; 
} BaseClassInfo;

// SymTagEnum symbol 
typedef struct _EnumInfo
{
    // Name (DIA: name) 
    WCHAR Name[TIS_MAXNAMELEN]; 

    // Index of the symbol that represent the type of the enumerators (DIA: typeId) 
    ULONG TypeIndex; 

    // Nested ("true" if the declaration is nested in a UDT) (DIA: nested) 
    BOOL Nested;

    // Number of enumerators 
    ULONG NumEnums;

    // Enumerators (their type indices) 
    ULONG Enums[TIS_MAXNUMCHILDREN]; 
} EnumInfo;

// SymTagArrayType symbol 
typedef struct _ArrayTypeInfo
{
    // Index of the symbol that represents the type of the array element 
    ULONG ElementTypeIndex; 

    // Index of the symbol that represents the type of the array index (DIA: arrayIndexTypeId) 
    ULONG IndexTypeIndex; 

    // Size of the array (in bytes) (DIA: length) 
    ULONG64 Length; 

    // Number of dimensions 
    ULONG NumDimensions;

    // Dimensions 
    ULONG64 Dimensions[TIS_MAXARRAYDIMS]; 
} ArrayTypeInfo;

// SymTagFunctionType 
typedef struct FunctionTypeInfo
{
    // Index of the return value type symbol (DIA: objectPointerType)
    ULONG RetTypeIndex; 

    // Number of arguments (DIA: count) 
    ULONG NumArgs;

    // Function arguments 
    ULONG Args[TIS_MAXNUMCHILDREN]; 

    // Calling convention (DIA: callingConvention)
    CV_call_e CallConv; 

    // "Is member function" flag (member function, if "true") 
    BOOL MemberFunction; 

    // Class symbol index (DIA: classParent) 
    // (defined only if MemberFunction is "true") 
    ULONG ClassIndex; 

    // "this" adjustment (DIA: thisAdjust) 
    // (defined only if MemberFunction is "true") 
    LONG ThisAdjust; 

    // "Is static function" flag (static, if "true") 
    // (defined only if MemberFunction is "true") 
    BOOL StaticFunction;
} FunctionTypeInfo;

// SymTagFunctionArgType 
typedef struct FunctionArgTypeInfo
{
    // Index of the symbol that represents the type of the argument (DIA: typeId)
    ULONG TypeIndex; 
} FunctionArgTypeInfo;

// SymTagData 
typedef struct DataInfo
{
    // Name (DIA: name) 
    WCHAR Name[TIS_MAXNAMELEN]; 

    // Index of the symbol that represents the type of the variable (DIA: type) 
    ULONG TypeIndex; 

    // Data kind (local, global, member, etc.) (DIA: dataKind) 
    DataKind dataKind; 

    // Address (defined if dataKind is: DataIsGlobal, DataIsStaticLocal, 
    // DataIsFileStatic, DataIsStaticMember) (DIA: address) 
    ULONG64 Address; 

    // Offset (defined if dataKind is: DataIsLocal, DataIsParam, 
    // DataIsObjectPtr, DataIsMember) (DIA: offset) 
    ULONG Offset; // <OS-TODO> Verify it for all listed data kinds 

    // Note: Length is not available - use the type symbol to obtain it 
} DataInfo;

typedef struct _TypeInfo
{
    // Symbol tag 
    enum SymTagEnum Tag; 

    // UDT kind (defined only if "Tag" is SymTagUDT: "true" if the symbol is 
    // a class or a structure, "false" if the symbol is a union) 
    BOOL UdtKind; 

    union
    {
        BaseTypeInfo         sBaseTypeInfo;     // If Tag == SymTagBaseType 
        TypedefInfo          sTypedefInfo;      // If Tag == SymTagTypedef 
        PointerTypeInfo      sPointerTypeInfo;  // If Tag == SymTagPointerType 
        UdtClassInfo         sUdtClassInfo;     // If Tag == SymTagUDT and UdtKind is "true" 
        UdtUnionInfo         sUdtUnionInfo;     // If Tag == SymTagUDT and UdtKind is "false" 
        BaseClassInfo        sBaseClassInfo;    // If Tag == SymTagBaseClass 
        EnumInfo             sEnumInfo;         // If Tag == SymTagEnum 
        ArrayTypeInfo        sArrayTypeInfo;    // If Tag == SymTagArrayType 
        FunctionTypeInfo     sFunctionTypeInfo; // If Tag == SymTagFunctionType 
        FunctionArgTypeInfo  sFunctionArgTypeInfo; // If Tag == SymTagFunctionArgType 
        DataInfo             sDataInfo;         // If Tag == SymTagData 
    };
} TypeInfo;

BOOLEAN SymbolInfo_DumpBasicType(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, BaseTypeInfo* Info);
BOOLEAN SymbolInfo_DumpPointerType(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, PointerTypeInfo* Info);
BOOLEAN SymbolInfo_DumpTypedef(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, TypedefInfo* Info);
BOOLEAN SymbolInfo_DumpEnum(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, EnumInfo* Info);
BOOLEAN SymbolInfo_DumpArrayType(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, ArrayTypeInfo* Info);
BOOLEAN SymbolInfo_DumpUDT(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, TypeInfo* Info);
BOOLEAN SymbolInfo_DumpUDTClass(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, UdtClassInfo* Info);
BOOLEAN SymbolInfo_DumpUDTUnion(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, UdtUnionInfo* Info);
BOOLEAN SymbolInfo_DumpFunctionType(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, FunctionTypeInfo* Info);
BOOLEAN SymbolInfo_DumpFunctionArgType(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, FunctionArgTypeInfo* Info);
BOOLEAN SymbolInfo_DumpBaseClass(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, BaseClassInfo* Info);
BOOLEAN SymbolInfo_DumpData(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, DataInfo* Info);
BOOLEAN SymbolInfo_DumpType(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, TypeInfo* Info);
BOOLEAN SymbolInfo_DumpSymbolType(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, TypeInfo* Info, ULONG* TypeIndex);

BOOLEAN SymbolInfo_CheckTag(
    _Inout_ PPDB_SYMBOL_CONTEXT Context, 
    _In_ ULONG Index, 
    _In_ ULONG Tag
    );

BOOLEAN SymbolInfo_SymbolSize(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, ULONG64* Size);
BOOLEAN SymbolInfo_ArrayElementTypeIndex(_Inout_ PPDB_SYMBOL_CONTEXT Context, ULONG ArrayIndex, ULONG* ElementTypeIndex);
BOOLEAN SymbolInfo_ArrayDims(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, ULONG64* pDims, ULONG* Dims, _In_ ULONG MaxDims);
BOOLEAN SymbolInfo_UdtVariables(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, ULONG* pVars, ULONG* Vars, _In_ ULONG MaxVars);
BOOLEAN SymbolInfo_UdtFunctions(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, ULONG* pFuncs, ULONG* Funcs, _In_ ULONG MaxFuncs);
BOOLEAN SymbolInfo_UdtBaseClasses(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, ULONG* pBases, ULONG* Bases, _In_ ULONG MaxBases);
BOOLEAN SymbolInfo_UdtUnionMembers(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, ULONG* pMembers, ULONG* Members, _In_ ULONG MaxMembers);
BOOLEAN SymbolInfo_FunctionArguments(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, ULONG* pArgs, ULONG* Args, _In_ ULONG MaxArgs);
BOOLEAN SymbolInfo_Enumerators(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, ULONG* pEnums, ULONG* Enums, _In_ ULONG MaxEnums);
BOOLEAN SymbolInfo_TypeDefType(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, ULONG* UndTypeIndex);
BOOLEAN SymbolInfo_PointerType(_Inout_ PPDB_SYMBOL_CONTEXT Context, _In_ ULONG Index, ULONG* UndTypeIndex, ULONG* NumPointers);

BOOLEAN SymbolInfo_GetTypeNameHelper(
    _In_ ULONG Index,
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _Out_ PWSTR *VarName,
    _Inout_ PPH_STRING_BUILDER TypeName
    );

PPH_STRING SymbolInfo_GetTypeName(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _In_ PWSTR VarName
    );

PWSTR SymbolInfo_TagStr(enum SymTagEnum Tag);
PWSTR SymbolInfo_BaseTypeStr(BasicType Type, ULONG64 Length);
PWSTR SymbolInfo_CallConvStr(CV_call_e CallConv);
PWSTR SymbolInfo_DataKindFromSymbolInfo(_In_ PSYMBOL_INFOW rSymbol);
PWSTR SymbolInfo_DataKindStr(DataKind dataKind);
VOID SymbolInfo_SymbolLocationStr(PSYMBOL_INFOW rSymbol, PWSTR pBuffer);
PWSTR SymbolInfo_RegisterStr(CV_HREG_e RegCode);
PWSTR SymbolInfo_UdtKindStr(UdtKind KindType);
PWSTR SymbolInfo_LocationTypeStr(LocationType LocType);

VOID PrintClassInfo(
    _In_ PPDB_SYMBOL_CONTEXT Context, 
    _In_ ULONG Index, 
    _In_ UdtClassInfo* Info
    );

VOID PrintUserDefinedTypes(PPDB_SYMBOL_CONTEXT Context);

#endif
