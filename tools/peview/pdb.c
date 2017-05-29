/*
 * PE viewer -
 *   pdb support
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
#include <cpysave.h>
#include <symprv.h>
#include <pdb.h>
#include <uxtheme.h>

typedef BOOL (WINAPI *_SymInitialize)(
    _In_ HANDLE hProcess,
    _In_opt_ PCSTR UserSearchPath,
    _In_ BOOL fInvadeProcess
    );

typedef BOOL (WINAPI *_SymCleanup)(
    _In_ HANDLE hProcess
    );

typedef BOOL (CALLBACK *_SymGetTypeInfo)(
    _In_ HANDLE hProcess,
    _In_ ULONG64 ModBase,
    _In_ ULONG TypeId,
    _In_ IMAGEHLP_SYMBOL_TYPE_INFO GetType,
    _Out_ PVOID pInfo
    );

typedef BOOL (WINAPI *_SymEnumSymbolsW)(
    _In_ HANDLE hProcess,
    _In_ ULONG64 BaseOfDll,
    _In_opt_ PCWSTR Mask,
    _In_ PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
    _In_opt_ const PVOID UserContext
    );

typedef BOOL (WINAPI *_SymEnumTypesW)(
    _In_ HANDLE hProcess,
    _In_ ULONG64 BaseOfDll,
    _In_ PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
    _In_opt_ PVOID UserContext
    );

typedef BOOL (WINAPI *_SymSetSearchPathW)(
    _In_ HANDLE hProcess,
    _In_opt_ PCWSTR SearchPath
    );

typedef ULONG (WINAPI *_SymGetOptions)();

typedef ULONG (WINAPI *_SymSetOptions)(
    _In_ ULONG SymOptions
    );

typedef ULONG64 (WINAPI *_SymLoadModuleExW)(
    _In_ HANDLE hProcess,
    _In_ HANDLE hFile,
    _In_ PCWSTR ImageName,
    _In_ PCWSTR ModuleName,
    _In_ ULONG64 BaseOfDll,
    _In_ ULONG DllSize,
    _In_ PMODLOAD_DATA Data,
    _In_ ULONG Flags
    );

typedef BOOL (WINAPI *_SymGetModuleInfoW64)(
    _In_ HANDLE hProcess,
    _In_ ULONG64 qwAddr,
    _Out_ PIMAGEHLP_MODULEW64 ModuleInfo
    );

typedef BOOL (WINAPI *_SymGetTypeFromNameW)(
    _In_ HANDLE hProcess,
    _In_ ULONG64 BaseOfDll,
    _In_ PCTSTR Name,
    _Inout_ PSYMBOL_INFO Symbol
    );

typedef BOOL (WINAPI *_SymSetContext)(
    _In_ HANDLE hProcess,
    _In_ PIMAGEHLP_STACK_FRAME StackFrame,
    _In_opt_ PIMAGEHLP_CONTEXT Context
    );

_SymInitialize SymInitialize_I = NULL;
_SymCleanup SymCleanup_I = NULL;
_SymEnumSymbolsW SymEnumSymbolsW_I = NULL;
_SymEnumTypesW SymEnumTypesW_I = NULL;
_SymSetSearchPathW SymSetSearchPathW_I = NULL;
_SymGetOptions SymGetOptions_I = NULL;
_SymSetOptions SymSetOptions_I = NULL;
_SymLoadModuleExW SymLoadModuleExW_I = NULL;
_SymGetModuleInfoW64 SymGetModuleInfoW64_I = NULL;
_SymGetTypeFromNameW SymGetTypeFromNameW_I = NULL;
_SymGetTypeInfo SymGetTypeInfo_I = NULL;
_SymSetContext SymSetContext_I = NULL;

ULONG SearchResultsAddIndex = 0;
PPH_LIST SearchResults = NULL;
PH_QUEUED_LOCK SearchResultsLock = PH_QUEUED_LOCK_INIT;

BOOLEAN PdbGetSymbolBasicType(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ BaseTypeInfo *Info
    )
{
    ULONG baseType = btNoType;
    ULONG64 length = 0;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagBaseType))
        return FALSE;

    // Basic type ("basicType" in DIA)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_BASETYPE, &baseType))
        return FALSE;

    // Length ("length" in DIA)     
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_LENGTH, &length))
        return FALSE;

    Info->BaseType = (BasicType)baseType;
    Info->Length = length;

    return TRUE;
}

BOOLEAN PdbGetSymbolPointerType(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ PointerTypeInfo* Info
    )
{
    ULONG typeIndex = 0;
    ULONG64 length = 0;
    BOOL reference = FALSE;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagPointerType))
        return FALSE;

    // Type index ("typeId" in DIA)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_TYPEID, &typeIndex))
        return FALSE;

    // Length ("length" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_LENGTH, &length))
        return FALSE;

    // Reference ("reference" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_IS_REFERENCE, &reference))
        return FALSE;

    Info->TypeIndex = typeIndex;
    Info->Length = length;
    Info->TypeReference = reference;

    return TRUE;
}

BOOLEAN PdbGetSymbolTypedefType(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ TypedefInfo* Info
    )
{
    ULONG typeIndex = 0;
    PWSTR symbolName = NULL;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagTypedef))
        return FALSE;
       
    // Type index ("typeId" in DIA)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_TYPEID, &typeIndex))
        return FALSE;

    // Name ("name" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_SYMNAME, &symbolName) || !symbolName)
        return FALSE;

    Info->TypeIndex = typeIndex;

    wcsncpy(Info->Name, symbolName, ARRAYSIZE(Info->Name));
    Info->Name[ARRAYSIZE(Info->Name) - 1] = 0;
    LocalFree(symbolName);

    return TRUE;
}

BOOLEAN PdbGetSymbolEnumType(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ EnumInfo* Info
    )
{
    ULONG TypeIndex = 0;
    ULONG Nested = 0;
    PWSTR symbolName = NULL;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagEnum))
        return FALSE;

    // Name ("name" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_SYMNAME, &symbolName) || !symbolName)
        return FALSE;

    wcsncpy(Info->Name, symbolName, ARRAYSIZE(Info->Name));
    Info->Name[ARRAYSIZE(Info->Name) - 1] = 0;
    LocalFree(symbolName);

    // Type index ("typeId" in DIA)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_TYPEID, &TypeIndex))
        return FALSE;

    // Nested ("nested" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_NESTED, &Nested))
        return FALSE;

    // Enumerators 
    if (!PdbGetSymbolEnumerations(BaseAddress, Index, Info->Enums, &Info->NumEnums, ARRAYSIZE(Info->Enums)))
        return FALSE;

    Info->TypeIndex = TypeIndex;
    Info->Nested = (Nested != 0);

    return TRUE;
}

BOOLEAN PdbGetSymbolArrayType(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ ArrayTypeInfo* Info
    )
{
    ULONG elementTypeIndex = 0;
    ULONG64 length = 0;
    ULONG indexTypeIndex = 0;

    // Check if it is really SymTagArrayType 
    if (!PdbCheckTagType(BaseAddress, Index, SymTagArrayType))
        return FALSE;

    // Element type index 
    if (!PdbGetSymbolArrayElementTypeIndex(BaseAddress, Index, &elementTypeIndex))
        return FALSE;

    // Length ("length" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_LENGTH, &length))
        return FALSE;

    // Type index of the array index element 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_ARRAYINDEXTYPEID, &indexTypeIndex))
        return FALSE;

    Info->ElementTypeIndex = elementTypeIndex;
    Info->Length = length;
    Info->IndexTypeIndex = indexTypeIndex;

    if (length > 0)
    {
        if (!PdbGetSymbolArrayDimensions(
            BaseAddress,
            Index, 
            Info->Dimensions,
            &Info->NumDimensions, 
            ARRAYSIZE(Info->Dimensions)
            ) || (Info->NumDimensions == 0))
        {
            return FALSE;
        }
    }
    else
    {
        Info->NumDimensions = 0; // No dimensions 
    }

    return TRUE;
}

BOOLEAN SymbolInfo_DumpUDT(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ TypeInfo* Info
    )
{
    ULONG UDTKind = 0;
    BOOLEAN result = FALSE;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagUDT))
        return FALSE;

    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_UDTKIND, &UDTKind))
        return FALSE;

    switch (UDTKind)
    {
    case UdtStruct:
        Info->UdtKind = TRUE;
        result = PdbGetSymbolUDTClass(BaseAddress, Index, &Info->UdtClassInfo);
        break;
    case UdtClass:
        Info->UdtKind = TRUE;
        result = PdbGetSymbolUDTClass(BaseAddress, Index, &Info->UdtClassInfo);
        break;
    case UdtUnion:
        Info->UdtKind = FALSE;
        result = PdbGetSymbolUDTUnion(BaseAddress, Index, &Info->UdtUnionInfo);
        break;
    }

    return result;
}

BOOLEAN PdbGetSymbolUDTClass(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ UdtClassInfo* Info
    )
{
    ULONG UDTKind = 0;
    PWSTR symbolName = NULL;
    ULONG64 Length = 0;
    ULONG Nested = 0;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagUDT))
        return FALSE;

    // Check if it is really a class or structure UDT ? 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_UDTKIND, &UDTKind))
        return FALSE;

    if ((UDTKind != UdtStruct) && (UDTKind != UdtClass))
        return FALSE;

    // Name ("name" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_SYMNAME, &symbolName) || !symbolName)
        return FALSE;

    wcsncpy(Info->Name, symbolName, ARRAYSIZE(Info->Name));
    Info->Name[ARRAYSIZE(Info->Name) - 1] = 0;
    LocalFree(symbolName);

    // Length ("length" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_LENGTH, &Length))
        return FALSE;

    // Nested ("nested" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_NESTED, &Nested))
        return FALSE;

    // Member variables 
    if (!PdbGetSymbolUdtVariables(BaseAddress, Index, Info->Variables, &Info->NumVariables, ARRAYSIZE(Info->Variables)))
        return FALSE;

    // Member functions 
    if (!PdbGetSymbolUdtFunctions(BaseAddress, Index, Info->Functions, &Info->NumFunctions, ARRAYSIZE(Info->Functions)))
        return FALSE;

    // Base classes 
    if (!PdbGetSymbolUdtBaseClasses(BaseAddress, Index, Info->BaseClasses, &Info->NumBaseClasses, ARRAYSIZE(Info->BaseClasses)))
        return FALSE;

    Info->UDTKind = (UdtKind)UDTKind;
    Info->Length = Length;
    Info->Nested = (Nested != 0);

    return TRUE;
}

BOOLEAN PdbGetSymbolUDTUnion(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ UdtUnionInfo *Info
    )
{
    ULONG UDTKind = 0;
    PWSTR symbolName = 0;
    ULONG64 Length = 0;
    ULONG Nested = 0;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagUDT))
        return FALSE;

    // Check if it is really a union UDT ? 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_UDTKIND, &UDTKind))
        return FALSE;

    if (UDTKind != UdtUnion)
        return FALSE;

    // Name ("name" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_SYMNAME, &symbolName) || !symbolName)
        return FALSE;

    wcsncpy(Info->Name, symbolName, ARRAYSIZE(Info->Name));
    Info->Name[ARRAYSIZE(Info->Name) - 1] = 0;
    LocalFree(symbolName);

    // Length ("length" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_LENGTH, &Length))
        return FALSE;

    // Nested ("nested" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_NESTED, &Nested))
        return FALSE;

    // Union members 
    if (!PdbGetSymbolUdtUnionMembers(BaseAddress, Index, Info->Members, &Info->NumMembers, ARRAYSIZE(Info->Members)))
        return FALSE;

    Info->UDTKind = (UdtKind)UDTKind;
    Info->Length = Length;
    Info->Nested = (Nested != 0);

    return TRUE;
}

BOOLEAN PdbGetSymbolFunctionType(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ FunctionTypeInfo *Info
    )
{
    ULONG RetTypeIndex = 0;
    ULONG NumArgs = 0;
    ULONG CallConv = 0;
    ULONG ClassIndex = 0;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagFunctionType))
        return FALSE;

    // Index of the return type symbol ("typeId" in DIA)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_TYPEID, &RetTypeIndex))
        return FALSE;

    // Number of arguments ("count" in DIA) 
    // Note: For non-static member functions, it includes "this" 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_COUNT, &NumArgs))
        return FALSE;

    // Do not save it in the data structure, since we obtain the number of arguments 
    // again using FunctionArguments() (see below). 
    // But later we will use this value to determine whether the function 
    // is static or not (if it is a member function) 

    // Calling convention ("callingConvention" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_CALLING_CONVENTION, &CallConv))
        return FALSE;

    // Parent class type index ("classParent" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_CLASSPARENTID, &ClassIndex))
    {
        // The function is not a member function 
        Info->ClassIndex = 0;
        Info->MemberFunction = FALSE;
    }
    else
    {
        // This is a member function of a class 
        Info->ClassIndex = ClassIndex;
        Info->MemberFunction = TRUE;
    }

    // If this is a member function, obtain additional data 
    if (Info->MemberFunction)
    {
        LONG ThisAdjust = 0;

        // "this" adjustment ("thisAdjust" in DIA) 
        if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_THISADJUST, &ThisAdjust))
            return FALSE;

        Info->ThisAdjust = ThisAdjust;
    }

    // Test for GetChildren()
    /*
        ULONG cMaxChildren = 4;
        ULONG Children[cMaxChildren];
        ULONG NumChildren = 0;

        GetChildren( m_hProcess, BaseAddress, Index, Children, NumChildren, cMaxChildren)
    */

    // Dump function arguments 
    if (!PdbGetSymbolFunctionArguments(BaseAddress, Index, Info->Args, &Info->NumArgs, ARRAYSIZE(Info->Args)))
        return FALSE;

    // Is the function static ? (If it is a member function) 
    if (Info->MemberFunction)
    {
        // The function is static if the value of Count property 
        // it the same as the number of child FunctionArgType symbols 
        Info->StaticFunction = (NumArgs == Info->NumArgs);
    }

    Info->RetTypeIndex = RetTypeIndex;
    Info->CallConv = (CV_call_e)CallConv;

    return TRUE;
}

BOOLEAN PdbGetSymbolFunctionArgType(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ FunctionArgTypeInfo *Info
    )
{
    ULONG typeIndex = 0;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagFunctionArgType))
        return FALSE;

    // Index of the argument type ("typeId" in DIA)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_TYPEID, &typeIndex))
        return FALSE;

    Info->TypeIndex = typeIndex;

    return TRUE;
}

BOOLEAN PdbGetSymbolBaseClass(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ BaseClassInfo *Info
    )
{
    ULONG typeIndex = 0;
    ULONG virtualBase = 0;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagBaseClass))
        return FALSE;

    // Base class UDT 
    if (!SymGetTypeInfo_I(
        NtCurrentProcess(),
        BaseAddress,
        Index,
        TI_GET_TYPEID,
        &typeIndex
        ))
    {
        return FALSE;
    }

    // Is this base class virtual ? 
    if (!SymGetTypeInfo_I(
        NtCurrentProcess(),
        BaseAddress,
        Index,
        TI_GET_VIRTUALBASECLASS,
        &virtualBase
        ))
    {
        return FALSE;
    }

    Info->TypeIndex = typeIndex;
    Info->Virtual = (virtualBase != 0);

    if (virtualBase)
    {
        ULONG virtualBasePtrOffset = 0;

        // Virtual base pointer offset
        if (!SymGetTypeInfo_I(
            NtCurrentProcess(),
            BaseAddress,
            Index,
            TI_GET_VIRTUALBASEPOINTEROFFSET,
            &virtualBasePtrOffset
            ))
        {
            return FALSE;
        }

        Info->Offset = 0;
        Info->VirtualBasePointerOffset = virtualBasePtrOffset;
    }
    else
    {
        ULONG offset = 0;

        // Offset in the parent UDT 
        if (!SymGetTypeInfo_I(
            NtCurrentProcess(),
            BaseAddress,
            Index,
            TI_GET_OFFSET,
            &offset
            ))
        {
            return FALSE;
        }

        Info->Offset = offset;
        Info->VirtualBasePointerOffset = 0;
    }

    return TRUE;
}

BOOLEAN PdbGetSymbolData(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ DataInfo *Info
    )
{
    PWSTR symbolName = NULL;
    ULONG TypeIndex = 0;
    ULONG dataKind = 0;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagData))
        return FALSE;

    // Name ("name" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_SYMNAME, &symbolName) || !symbolName)
        return FALSE;

    wcsncpy(Info->Name, symbolName, ARRAYSIZE(Info->Name));
    Info->Name[ARRAYSIZE(Info->Name) - 1] = 0;
    LocalFree(symbolName);

    // Index of type symbol ("typeId" in DIA)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_TYPEID, &TypeIndex))
        return FALSE;

    // Data kind ("dataKind" in DIA)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_DATAKIND, &dataKind))
        return FALSE;

    Info->TypeIndex = TypeIndex;
    Info->dataKind = (DataKind)dataKind;

    switch (dataKind)
    {
    case DataIsGlobal:
    case DataIsStaticLocal:
    case DataIsFileStatic:
    case DataIsStaticMember:
        {
            ULONG64 address = 0;

            // Use Address; Offset is not defined
            // Note: If it is DataIsStaticMember, then this is a static member of a class defined in another module 
            // (it does not have an address in this module) 

            if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_ADDRESS, &address))
                return FALSE;

            Info->Address = address;
            Info->Offset = 0;
        }
        break;

    case DataIsLocal:
    case DataIsParam:
    case DataIsObjectPtr:
    case DataIsMember:
        {
            ULONG offset = 0;

            // Use Offset; Address is not defined
            if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_OFFSET, &offset))
                return FALSE;

            Info->Offset = offset;
            Info->Address = 0;
        }
        break;

    default:
        Info->Address = 0;
        Info->Offset = 0;
        break;
    }

    return TRUE;
}

BOOLEAN SymbolInfo_DumpType(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ TypeInfo *Info
    )
{
    ULONG tag = SymTagNull;

    if (!SymGetTypeInfo_I(
        NtCurrentProcess(),
        BaseAddress,
        Index,
        TI_GET_SYMTAG,
        &tag
        ))
    {
        return FALSE;
    }

    Info->Tag = (enum SymTagEnum)tag;

    switch (tag)
    {
    case SymTagBaseType:
        return PdbGetSymbolBasicType(BaseAddress, Index, &Info->BaseTypeInfo);
    case SymTagPointerType:
        return PdbGetSymbolPointerType(BaseAddress, Index, &Info->PointerTypeInfo);
    case SymTagTypedef:
        return PdbGetSymbolTypedefType(BaseAddress, Index, &Info->TypedefInfo);
    case SymTagEnum:
        return PdbGetSymbolEnumType(BaseAddress, Index, &Info->EnumInfo);
    case SymTagArrayType:
        return PdbGetSymbolArrayType(BaseAddress, Index, &Info->ArrayTypeInfo);
    case SymTagUDT:
        return SymbolInfo_DumpUDT(BaseAddress, Index, Info);
    case SymTagFunctionType:
        return PdbGetSymbolFunctionType(BaseAddress, Index, &Info->FunctionTypeInfo);
    case SymTagFunctionArgType:
        return PdbGetSymbolFunctionArgType(BaseAddress, Index, &Info->FunctionArgTypeInfo);
    case SymTagBaseClass:
        return PdbGetSymbolBaseClass(BaseAddress, Index, &Info->BaseClassInfo);
    case SymTagData:
        return PdbGetSymbolData(BaseAddress, Index, &Info->DataInfo);
    }

    return FALSE;
}

BOOLEAN SymbolInfo_DumpSymbolType(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ TypeInfo *Info,
    _Inout_ ULONG *TypeIndex
    )
{
    // Obtain the index of the symbol's type symbol
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_TYPEID, &TypeIndex))
        return FALSE;

    // Dump the type symbol 
    return SymbolInfo_DumpType(Context->BaseAddress, *TypeIndex, Info);
}

BOOLEAN PdbCheckTagType(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _In_ ULONG Tag
    )
{
    ULONG symTag = SymTagNull;

    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_SYMTAG, &symTag))
        return FALSE;

    return symTag == Tag;
}

BOOLEAN PdbGetSymbolSize(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ ULONG64* Size
    )
{
    ULONG index;
    ULONG64 length = 0;

    // Does the symbol support "length" property ? 
    if (SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_GET_LENGTH, &length))
    {
        *Size = length;
        return TRUE;
    }
    else
    {
        // No, it does not - it can be SymTagTypedef 
        if (!PdbCheckTagType(BaseAddress, Index, SymTagTypedef))
        {
            // No, this symbol does not have length 
            return FALSE;
        }
        else
        {
            // Yes, it is a SymTagTypedef - skip to its underlying type 
            index = Index;

            do
            {
                ULONG tempIndex = 0;

                if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, index, TI_GET_TYPEID, &tempIndex))
                    return FALSE;

                index = tempIndex;

            } while (PdbCheckTagType(BaseAddress, index, SymTagTypedef));

            // And get the length 
            if (SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, index, TI_GET_LENGTH, &length))
            {
                *Size = length;
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOLEAN PdbGetSymbolArrayElementTypeIndex(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG ArrayIndex,
    _Inout_ ULONG* ElementTypeIndex
    )
{
    ULONG index;
    ULONG elementIndex = 0;

    if (!PdbCheckTagType(BaseAddress, ArrayIndex, SymTagArrayType))
        return FALSE;

    // Get the array element type 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, ArrayIndex, TI_GET_TYPEID, &elementIndex))
        return FALSE;

    // If the array element type is SymTagArrayType, skip to its type 
    index = elementIndex;

    while (PdbCheckTagType(BaseAddress, elementIndex, SymTagArrayType))
    {
        if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, index, TI_GET_TYPEID, &elementIndex))
            return FALSE;

        index = elementIndex;
    }

    // We have found the ultimate type of the array element 
    *ElementTypeIndex = elementIndex;

    return TRUE;
}

BOOLEAN PdbGetSymbolArrayDimensions(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ ULONG64* pDims,
    _Inout_ ULONG* Dims,
    _In_ ULONG MaxDims
    )
{
    ULONG index;
    ULONG dimCount;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagArrayType))
        return FALSE;

    if (MaxDims <= 0)
        return FALSE;

    index = Index;
    dimCount = 0;

    for (ULONG i = 0; i < MaxDims; i++)
    {
        ULONG typeIndex = 0;
        ULONG64 length = 0;
        ULONG64 typeSize = 0;

        // Length ("length" in DIA) 
        if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, index, TI_GET_LENGTH, &length) || (length == 0))
            return FALSE;

        // Size of the current dimension 
        // (it is the size of its SymTagArrayType symbol divided by the size of its 
        // type symbol, which can be another SymTagArrayType symbol or the array element symbol) 

        // Its type 
        if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, index, TI_GET_TYPEID, &typeIndex))
        {
            // No type - we are done 
            break;
        }

        // Size of its type 
        if (!PdbGetSymbolSize(BaseAddress, typeIndex, &typeSize) || (typeSize == 0))
            return FALSE;

        // Size of the dimension 
        pDims[i] = length / typeSize;
        dimCount++;

        /*
        ULONG ElemCount = 0;
        if( !SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, CurIndex, TI_GET_COUNT, &ElemCount))
        {
            // and continue ...
        }
        else if( ElemCount != pDims[i] )
        {
            _ASSERTE( !_T("TI_GET_COUNT does not match.") );
        }*/

        // If the type symbol is not SymTagArrayType, we are done 
        if (!PdbCheckTagType(BaseAddress, typeIndex, SymTagArrayType))
            break;

        index = typeIndex;
    }

    if (dimCount == 0)
        return FALSE;

    *Dims = dimCount;

    return TRUE;
}

BOOLEAN PdbGetSymbolChildren(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ ULONG* Count,
    _Inout_ TI_FINDCHILDREN_PARAMS **Params
    )
{
    ULONG length;
    ULONG symbolCount = 0;
    TI_FINDCHILDREN_PARAMS* symbols;
    
    if (!SymGetTypeInfo_I(
        NtCurrentProcess(),
        BaseAddress,
        Index,
        TI_GET_CHILDRENCOUNT,
        &symbolCount
        ))
    {
        return FALSE;
    }

    if (symbolCount == 0)
        return TRUE;

    length = sizeof(TI_FINDCHILDREN_PARAMS) + symbolCount * sizeof(ULONG);
    symbols = _alloca(length);
    memset(symbols, 0, length);

    symbols->Count = symbolCount;

    if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, Index, TI_FINDCHILDREN, symbols))
        return FALSE;

    *Count = symbolCount;
    *Params = symbols;

    return TRUE;
}

BOOLEAN PdbGetSymbolUdtVariables(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ ULONG* pVars,
    _Inout_ ULONG* Vars,
    _In_ ULONG MaxVars
    )
{
    ULONG childrenLength = 0;
    TI_FINDCHILDREN_PARAMS* symbolParams;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagUDT))
        return FALSE;

    if (MaxVars <= 0)
        return FALSE;

    if (!PdbGetSymbolChildren(BaseAddress, Index, &childrenLength, &symbolParams))
        return FALSE;

    *Vars = 0;

    for (ULONG i = 0; i < childrenLength; i++)
    {
        if (PdbCheckTagType(BaseAddress, symbolParams->ChildId[i], SymTagData))
        {
            pVars[*Vars] = symbolParams->ChildId[i];

            (*Vars)++;

            if (*Vars == MaxVars)
                break;
        }
    }

    return TRUE;
}

BOOLEAN PdbGetSymbolUdtFunctions(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ ULONG* pFuncs,
    _Inout_ ULONG* Funcs,
    _In_ ULONG MaxFuncs
    )
{
    ULONG childrenLength = 0;
    TI_FINDCHILDREN_PARAMS* symbolParams;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagUDT))
        return FALSE;

    if (MaxFuncs <= 0)
        return FALSE;

    if (!PdbGetSymbolChildren(BaseAddress, Index, &childrenLength, &symbolParams))
        return FALSE;

    *Funcs = 0;

    for (ULONG i = 0; i < childrenLength; i++)
    {
        if (PdbCheckTagType(BaseAddress, symbolParams->ChildId[i], SymTagFunction))
        {
            pFuncs[*Funcs] = symbolParams->ChildId[i];

            (*Funcs)++;

            if (*Funcs == MaxFuncs)
                break;
        }
    }

    return TRUE;
}

BOOLEAN PdbGetSymbolUdtBaseClasses(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ ULONG* pBases,
    _Inout_ ULONG* Bases,
    _In_ ULONG MaxBases
    )
{
    ULONG childrenLength = 0;
    TI_FINDCHILDREN_PARAMS* symbolParams;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagUDT))
        return FALSE;

    if (MaxBases <= 0)
        return FALSE;

    if (!PdbGetSymbolChildren(BaseAddress, Index, &childrenLength, &symbolParams))
        return FALSE;

    *Bases = 0;

    for (ULONG i = 0; i < childrenLength; i++)
    {
        if (PdbCheckTagType(BaseAddress, symbolParams->ChildId[i], SymTagBaseClass))
        {
            pBases[*Bases] = symbolParams->ChildId[i];

            (*Bases)++;

            if (*Bases == MaxBases)
                break;
        }
    }

    return TRUE;
}

BOOLEAN PdbGetSymbolUdtUnionMembers(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ ULONG* pMembers,
    _Inout_ ULONG* Members,
    _In_ ULONG MaxMembers
    )
{
    ULONG childrenLength = 0;
    TI_FINDCHILDREN_PARAMS* symbolParams;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagUDT))
        return FALSE;

    if (MaxMembers <= 0)
        return FALSE;

    if (!PdbGetSymbolChildren(BaseAddress, Index, &childrenLength, &symbolParams))
        return FALSE;

    *Members = 0;

    for (ULONG i = 0; i < childrenLength; i++)
    {
        if (PdbCheckTagType(BaseAddress, symbolParams->ChildId[i], SymTagData))
        {
            pMembers[*Members] = symbolParams->ChildId[i];

            (*Members)++;

            if (*Members == MaxMembers)
                break;
        }
    }

    return TRUE;
}

BOOLEAN PdbGetSymbolFunctionArguments(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ ULONG* pArgs,
    _Inout_ ULONG* Args,
    _In_ ULONG MaxArgs
    )
{
    ULONG childrenLength = 0;
    TI_FINDCHILDREN_PARAMS* symbolParams;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagFunctionType))
        return FALSE;

    if (MaxArgs <= 0)
        return FALSE;

    if (!PdbGetSymbolChildren(BaseAddress, Index, &childrenLength, &symbolParams))
        return FALSE;

    *Args = 0;

    for (ULONG i = 0; i < childrenLength; i++)
    {
        if (PdbCheckTagType(BaseAddress, symbolParams->ChildId[i], SymTagFunctionArgType))
        {
            pArgs[*Args] = symbolParams->ChildId[i];

            (*Args)++;

            if (*Args == MaxArgs)
                break;
        }
    }

    return TRUE;
}

BOOLEAN PdbGetSymbolEnumerations(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ ULONG *pEnums,
    _Inout_ ULONG *Enums,
    _In_ ULONG MaxEnums
    )
{
    ULONG childrenLength = 0;
    TI_FINDCHILDREN_PARAMS* symbolParams;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagEnum))
        return FALSE;

    if (MaxEnums <= 0)
        return FALSE;

    if (!PdbGetSymbolChildren(BaseAddress, Index, &childrenLength, &symbolParams))
        return FALSE;

    *Enums = 0;

    for (ULONG i = 0; i < childrenLength; i++)
    {
        if (PdbCheckTagType(BaseAddress, symbolParams->ChildId[i], SymTagData))
        {
            pEnums[*Enums] = symbolParams->ChildId[i];

            (*Enums)++;

            if (*Enums == MaxEnums)
                break;
        }
    }

    return TRUE;
}

BOOLEAN PdbGetSymbolTypeDefTypeIndex(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ ULONG *UndTypeIndex
    )
{
    ULONG index;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagTypedef))
        return FALSE;

    // Skip to the type behind the type definition 
    index = Index;

    while (PdbCheckTagType(BaseAddress, index, SymTagTypedef))
    {
        if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, index, TI_GET_TYPEID, UndTypeIndex))
            return FALSE;

        index = *UndTypeIndex;
    }

    return TRUE;
}

BOOLEAN PdbGetSymbolPointers(
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Index,
    _Inout_ ULONG *UndTypeIndex,
    _Inout_ ULONG *NumPointers
    )
{
    ULONG index;

    if (!PdbCheckTagType(BaseAddress, Index, SymTagPointerType))
        return FALSE;

    // Skip to the type pointer points to 
    *NumPointers = 0;
    index = Index;

    while (PdbCheckTagType(BaseAddress, index, SymTagPointerType))
    {
        if (!SymGetTypeInfo_I(NtCurrentProcess(), BaseAddress, index, TI_GET_TYPEID, UndTypeIndex))
            return FALSE;

        (*NumPointers)++;

        index = *UndTypeIndex;
    }

    return TRUE;
}

BOOLEAN SymbolInfo_GetTypeNameHelper(
    _In_ ULONG Index,
    _Inout_ PPDB_SYMBOL_CONTEXT Obj,
    _Out_ PWSTR *VarName,
    _Inout_ PPH_STRING_BUILDER TypeName
    )
{
    TypeInfo Info;

    if (!SymbolInfo_DumpType(Obj->BaseAddress, Index, &Info))
    {
        return FALSE;
    }
    else
    {
        ULONG numPointers = 0;

        if (Info.Tag == SymTagPointerType)
        {
            ULONG typeIndex = 0;

            // Yes, get the number of * to show 
            if (!PdbGetSymbolPointers(Obj->BaseAddress, Index, &typeIndex, &numPointers))
                return FALSE;

            // Get more information about the type the pointer points to                
            if (!SymbolInfo_DumpType(Obj->BaseAddress, typeIndex, &Info))
                return FALSE;

            // Save the index of the type the pointer points to 
            Index = typeIndex;

            // ... and proceed with that type 
        }

        switch (Info.Tag)
        {
        case SymTagBaseType:
            PhAppendStringBuilder2(TypeName, SymbolInfo_BaseTypeStr(Info.BaseTypeInfo.BaseType, Info.BaseTypeInfo.Length));
            PhAppendStringBuilder2(TypeName, L" ");
            break;
        case SymTagTypedef:
            PhAppendStringBuilder2(TypeName, Info.TypedefInfo.Name);
            PhAppendStringBuilder2(TypeName, L" ");
            break;
        case SymTagUDT:
            {
                if (Info.UdtKind)
                {
                    // A class/structure 
                    PhAppendStringBuilder2(TypeName, Info.UdtClassInfo.Name);
                    PhAppendStringBuilder2(TypeName, L" ");

                    // Add the UDT and its base classes to the collection of UDT indexes 
                    PhAddItemList(Obj->UdtList, UlongToPtr(Index));
                }
                else
                {
                    // A union 
                    PhAppendStringBuilder2(TypeName, Info.UdtUnionInfo.Name);
                    PhAppendStringBuilder2(TypeName, L" ");
                }
            }
            break;
        case SymTagEnum:
            PhAppendStringBuilder2(TypeName, Info.EnumInfo.Name);
            break;
        case SymTagFunctionType:
            {
                if (Info.FunctionTypeInfo.MemberFunction && Info.FunctionTypeInfo.StaticFunction)
                    PhAppendStringBuilder2(TypeName, L"static ");

                // return value 
                if (!SymbolInfo_GetTypeNameHelper(Info.FunctionTypeInfo.RetTypeIndex, Obj, VarName, TypeName))
                    return FALSE;

                // calling convention
                PhAppendStringBuilder2(TypeName, SymbolInfo_CallConvStr(Info.FunctionTypeInfo.CallConv));
                PhAppendStringBuilder2(TypeName, L" ");

                // If member function, save the class type index 
                if (Info.FunctionTypeInfo.MemberFunction)
                {
                    PhAddItemList(Obj->UdtList, UlongToPtr(Info.FunctionTypeInfo.ClassIndex));

                    /* 
                      // It is not needed to print the class name here, because it is contained in the function name
                      if (!SymbolInfo_GetTypeNameHelper(Info.sFunctionTypeInfo.ClassIndex, Obj, VarName, TypeName))
                          return false;
                      TypeName += "::");
                    */
                }

                // Print name
                PhAppendStringBuilder2(TypeName, *VarName);

                // Print parameters 
                PhAppendStringBuilder2(TypeName, L"(");

                for (ULONG i = 0; i < Info.FunctionTypeInfo.NumArgs; i++)
                {
                    if (!SymbolInfo_GetTypeNameHelper(Info.FunctionTypeInfo.Args[i], Obj, VarName, TypeName))
                        return FALSE;

                    if (i < (Info.FunctionTypeInfo.NumArgs - 1))
                        PhAppendStringBuilder2(TypeName, L", ");
                }

                PhAppendStringBuilder2(TypeName, L") ");

                // Print "this" adjustment value 
                if (Info.FunctionTypeInfo.MemberFunction && Info.FunctionTypeInfo.ThisAdjust != 0)
                    PhAppendFormatStringBuilder(TypeName, L": this+%u ", Info.FunctionTypeInfo.ThisAdjust);
            }
            break;

        case SymTagFunctionArgType:
            {   
                if (!SymbolInfo_GetTypeNameHelper(Info.FunctionArgTypeInfo.TypeIndex, Obj, VarName, TypeName))
                    return FALSE;
            }
            break;
        case SymTagArrayType:
            {
                // Print element type name 
                if (!SymbolInfo_GetTypeNameHelper(Info.ArrayTypeInfo.ElementTypeIndex, Obj, VarName, TypeName))
                    return FALSE;

                PhAppendStringBuilder2(TypeName, L" ");

                // Print dimensions 
                for (ULONG i = 0; i < Info.ArrayTypeInfo.NumDimensions; i++)
                    PhAppendFormatStringBuilder(TypeName, L"[%I64u]", Info.ArrayTypeInfo.Dimensions[i]);
            }
            break;
        default:
            {
                PhAppendFormatStringBuilder(TypeName, L"Unknown+%lu", Info.Tag);
            }
            break;
        }

        // If it is a pointer, display * characters 
        if (numPointers != 0)
        {
            for (ULONG i = 0; i < numPointers; i++)
                PhAppendStringBuilder2(TypeName, L"*");
        }
    }

    return TRUE;
}

PPH_STRING SymbolInfo_GetTypeName(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _In_ PWSTR VarName
    )
{
    PWSTR typeVarName = NULL;
    PH_STRING_BUILDER typeNamesb;

    PhInitializeStringBuilder(&typeNamesb, 0x100);

    if (VarName)
        typeVarName = VarName;

    if (!SymbolInfo_GetTypeNameHelper(Index, Context, &typeVarName, &typeNamesb))
        return NULL;

    return PhFinalStringBuilderString(&typeNamesb);
}

PWSTR SymbolInfo_TagStr(
    _In_ enum SymTagEnum Tag
    )
{
    switch (Tag)
    {
    case SymTagNull:
        return L"Null";
    case SymTagExe:
        return L"Exe";
    case SymTagCompiland:
        return L"Compiland";
    case SymTagCompilandDetails:
        return L"CompilandDetails";
    case SymTagCompilandEnv:
        return L"CompilandEnv";
    case SymTagFunction:
        return L"Function";
    case SymTagBlock:
        return L"Block";
    case SymTagData:
        return L"Data";
    case SymTagAnnotation:
        return L"Annotation";
    case SymTagLabel:
        return L"Label";
    case SymTagPublicSymbol:
        return L"PublicSymbol";
    case SymTagUDT:
        return L"UDT";
    case SymTagEnum:
        return L"Enum";
    case SymTagFunctionType:
        return L"FunctionType";
    case SymTagPointerType:
        return L"PointerType";
    case SymTagArrayType:
        return L"ArrayType";
    case SymTagBaseType:
        return L"BaseType";
    case SymTagTypedef:
        return L"Typedef";
    case SymTagBaseClass:
        return L"BaseClass";
    case SymTagFriend:
        return L"Friend";
    case SymTagFunctionArgType:
        return L"FunctionArgType";
    case SymTagFuncDebugStart:
        return L"FuncDebugStart";
    case SymTagFuncDebugEnd:
        return L"FuncDebugEnd";
    case SymTagUsingNamespace:
        return L"UsingNamespace";
    case SymTagVTableShape:
        return L"VTableShape";
    case SymTagVTable:
        return L"VTable";
    case SymTagCustom:
        return L"Custom";
    case SymTagThunk:
        return L"Thunk";
    case SymTagCustomType:
        return L"CustomType";
    case SymTagManagedType:
        return L"ManagedType";
    case SymTagDimension:
        return L"Dimension";
    }

    return L"UNKNOWN";

}

PWSTR SymbolInfo_BaseTypeStr(
    _In_ BasicType Type, 
    _In_ ULONG64 Length
    )
{
    switch (Type)
    {
    case btNoType:
        return L"";
    case btVoid:
        return L"void";
    case btChar:
        return L"char";
    case btWChar:
        return L"wchar_t";
    case btInt:
        {
            if (Length == 0)
            {
                return L"int";
            }
            else
            {
                if (Length == 1)
                    return L"char";
                else if (Length == 2)
                    return L"short";
                else
                    return L"int";
            }
        }
    case btUInt:
        {
            if (Length == 0)
            {
                return L"unsigned int";
            }
            else
            {
                if (Length == 1)
                    return L"unsigned char";
                else if (Length == 2)
                    return L"unsigned short";
                else
                    return L"unsigned int";
            }
        }
        break;
    case btFloat:
        {
            if (Length == 0)
            {
                return L"float";
            }
            else
            {
                if (Length == 4)
                    return L"float";
                else
                    return L"double";
            }
        }
        break;
    case btBCD:
        return L"BCD";
    case btBool:
        return L"bool";
    case btLong:
        return L"long";
    case btULong:
        return L"unsigned long";
    case btCurrency:
        return L"Currency";
    case btDate:
        return L"Date";
    case btVariant:
        return L"Variant";
    case btComplex:
        return L"Complex";
    case btBit:
        return L"Bit";
    case btBSTR:
        return L"BSTR";
    case btHresult:
        return L"HRESULT";
    }

    return L"UNKNOWN";

}

PWSTR SymbolInfo_CallConvStr(
    _In_ CV_call_e CallConv
    )
{
    switch (CallConv)
    {
    case CV_CALL_NEAR_C:
        return L"__cdecl";
    case CV_CALL_FAR_C:
        return L"FAR_C";
    case CV_CALL_NEAR_PASCAL:
        return L"NEAR_PASCAL";
    case CV_CALL_FAR_PASCAL:
        return L"FAR_PASCAL";
    case CV_CALL_NEAR_FAST:
        return L"__fastcall";
    case CV_CALL_FAR_FAST:
        return L"FAR_FAST";
    case CV_CALL_SKIPPED:
        return L"SKIPPED";
    case CV_CALL_NEAR_STD:
        return L"__stdcall";
    case CV_CALL_FAR_STD:
        return L"FAR_STD";
    case CV_CALL_NEAR_SYS:
        return L"__syscall";
    case CV_CALL_FAR_SYS:
        return L"FAR_SYS";
    case CV_CALL_THISCALL:
        return L"__thiscall";
    case CV_CALL_MIPSCALL:
        return L"MIPSCALL";
    case CV_CALL_GENERIC:
        return L"GENERIC";
    case CV_CALL_ALPHACALL:
        return L"ALPHACALL";
    case CV_CALL_PPCCALL:
        return L"PPCCALL";
    case CV_CALL_SHCALL:
        return L"SHCALL";
    case CV_CALL_ARMCALL:
        return L"ARMCALL";
    case CV_CALL_AM33CALL:
        return L"AM33CALL";
    case CV_CALL_TRICALL:
        return L"TRICALL";
    case CV_CALL_SH5CALL:
        return L"SH5CALL";
    case CV_CALL_M32RCALL:
        return L"M32RCALL";
    case CV_CALL_RESERVED:
        return L"RESERVED";
    }

    return L"UNKNOWN";

}

PWSTR SymbolInfo_DataKindFromSymbolInfo(
    _In_ PSYMBOL_INFOW SymbolInfo
    )
{
    ULONG dataKindType = 0;

    if (!SymGetTypeInfo_I(NtCurrentProcess(), SymbolInfo->ModBase, SymbolInfo->Index, TI_GET_DATAKIND, &dataKindType))
        return L"UNKNOWN";

    return SymbolInfo_DataKindStr(dataKindType);
}

PWSTR SymbolInfo_DataKindStr(
    _In_ DataKind SymDataKind
    )
{
    switch (SymDataKind)
    {
    case DataIsLocal:
        return L"LOCAL_VAR";
    case DataIsStaticLocal:
        return L"STATIC_LOCAL_VAR";
    case DataIsParam:
        return L"PARAMETER";
    case DataIsObjectPtr:
        return L"OBJECT_PTR";
    case DataIsFileStatic:
        return L"STATIC_VAR";
    case DataIsGlobal:
        return L"GLOBAL_VAR";
    case DataIsMember:
        return L"MEMBER";
    case DataIsStaticMember:
        return L"STATIC_MEMBER";
    case DataIsConstant:
        return L"CONSTANT";
    }

    return L"UNKNOWN";
}

VOID SymbolInfo_SymbolLocationStr(
    _In_ PSYMBOL_INFOW SymbolInfo, 
    _In_ PWSTR Buffer
    )
{
    if (SymbolInfo->Flags & SYMFLAG_REGISTER)
    {
        PWSTR regString = SymbolInfo_RegisterStr(SymbolInfo->Register);

        if (regString)
            wcscpy(Buffer, regString);
        else
            _swprintf(Buffer, L"Reg+%u", SymbolInfo->Register);
    }
    else if (SymbolInfo->Flags & SYMFLAG_REGREL)
    {
        WCHAR szReg[32];
        PWSTR regString = SymbolInfo_RegisterStr(SymbolInfo->Register);

        if (regString)
            wcscpy(szReg, regString);
        else
            _swprintf(szReg, L"Reg+%u", SymbolInfo->Register);

        _swprintf(Buffer, L"%s+%I64u", szReg, SymbolInfo->Address);
        //PhPrintPointer(Buffer, PTR_ADD_OFFSET(SymbolInfo->ModBase, SymbolInfo->Address));
    }
    else if (SymbolInfo->Flags & SYMFLAG_FRAMEREL)
    {
        wcscpy(Buffer, L"N/A");
    }
    else
    {
        if (SymbolInfo->Address)
            PhPrintPointer(Buffer, PTR_SUB_OFFSET(SymbolInfo->Address, SymbolInfo->ModBase));
    }
}

PWSTR SymbolInfo_RegisterStr(
    _In_ CV_HREG_e RegCode
    )
{
    switch (RegCode)
    {
    case CV_REG_EAX:
        return L"EAX";
    case CV_REG_ECX:
        return L"ECX";
    case CV_REG_EDX:
        return L"EDX";
    case CV_REG_EBX:
        return L"EBX";
    case CV_REG_ESP:
        return L"ESP";
    case CV_REG_EBP:
        return L"EBP";
    case CV_REG_ESI:
        return L"ESI";
    case CV_REG_EDI:
        return L"EDI";
    }

    return L"Unknown";
}

PWSTR SymbolInfo_UdtKindStr(
    _In_ UdtKind KindType
    )
{
    switch (KindType)
    {
    case UdtStruct:
        return L"STRUCT";
    case UdtClass:
        return L"CLASS";
    case UdtUnion:
        return L"UNION";
    }

    return L"UNKNOWN";
}

PWSTR SymbolInfo_LocationTypeStr(
    _In_ LocationType LocType
    )
{
    switch (LocType)
    {
    case LocIsNull:
        return L"Null";
    case LocIsStatic:
        return L"Static";
    case LocIsTLS:
        return L"TLS";
    case LocIsRegRel:
        return L"RegRel";
    case LocIsThisRel:
        return L"ThisRel";
    case LocIsEnregistered:
        return L"Enregistered";
    case LocIsBitField:
        return L"BitField";
    case LocIsSlot:
        return L"Slot";
    case LocIsIlRel:
        return L"IlRel";
    case LocInMetaData:
        return L"MetaData";
    case LocIsConstant:
        return L"Constant";
    }

    return L"Unknown";
}

///////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK EnumCallbackProc(
    _In_ PSYMBOL_INFOW SymbolInfo,
    _In_ ULONG SymbolSize, 
    _In_ PVOID Context
    )
{
    if (SymbolInfo)
    {
        switch (SymbolInfo->Tag)
        {
        case SymTagFunction:
            {
                PPDB_SYMBOL_CONTEXT context = Context;
                PPV_SYMBOL_NODE symbol;

                symbol = PhAllocate(sizeof(PV_SYMBOL_NODE));
                memset(symbol, 0, sizeof(PV_SYMBOL_NODE));

                symbol->Type = PV_SYMBOL_TYPE_FUNCTION;
                symbol->Address = SymbolInfo->Address;
                symbol->Size = SymbolInfo->Size;
                symbol->Name = PhCreateStringEx(SymbolInfo->Name, SymbolInfo->NameLen * sizeof(WCHAR));
                symbol->Data = SymbolInfo_GetTypeName(
                    context,
                    SymbolInfo->TypeIndex, 
                    SymbolInfo->Name
                    );
                SymbolInfo_SymbolLocationStr(SymbolInfo, symbol->Pointer);

                PhAcquireQueuedLockExclusive(&SearchResultsLock);
                PhAddItemList(SearchResults, symbol);
                PhReleaseQueuedLockExclusive(&SearchResultsLock);

                // Enumerate parameters and variables...
                PdbDumpAddress(context, SymbolInfo->Address);
            }
            break;
        case SymTagData:
            {
                PPDB_SYMBOL_CONTEXT context = Context;
                PPV_SYMBOL_NODE symbol;
                PWSTR symDataKind;
                ULONG dataKindType = 0;

                if (SymbolInfo->Address == 0)
                    break;

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

                //if (dataKindType == DataIsLocal ||
                //    dataKindType == DataIsParam ||
                //    dataKindType == DataIsObjectPtr)
                //{
                //    break;
                //}

                symbol = PhAllocate(sizeof(PV_SYMBOL_NODE));
                memset(symbol, 0, sizeof(PV_SYMBOL_NODE));

                switch (dataKindType)
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

                symbol->Address = SymbolInfo->Address;
                symbol->Size = SymbolInfo->Size;
                symbol->Name = PhCreateStringEx(SymbolInfo->Name, SymbolInfo->NameLen * sizeof(WCHAR));
                symbol->Data = SymbolInfo_GetTypeName(context, SymbolInfo->TypeIndex, SymbolInfo->Name);
                SymbolInfo_SymbolLocationStr(SymbolInfo, symbol->Pointer);

                PhAcquireQueuedLockExclusive(&SearchResultsLock);
                PhAddItemList(SearchResults, symbol);
                PhReleaseQueuedLockExclusive(&SearchResultsLock);
            }
            break;
        default:
            {
                PPDB_SYMBOL_CONTEXT context = Context;
                PPV_SYMBOL_NODE symbol;

                // TODO: Remove filter 
                //if (SymbolInfo->Tag != SymTagPublicSymbol)
                //    break;

                symbol = PhAllocate(sizeof(PV_SYMBOL_NODE));
                memset(symbol, 0, sizeof(PV_SYMBOL_NODE));

                symbol->Type = PV_SYMBOL_TYPE_SYMBOL;
                symbol->Address = SymbolInfo->Address;
                symbol->Size = SymbolInfo->Size;
                symbol->Name = PhCreateStringEx(SymbolInfo->Name, SymbolInfo->NameLen * sizeof(WCHAR));
                symbol->Data = SymbolInfo_GetTypeName(context, SymbolInfo->TypeIndex, SymbolInfo->Name);
                SymbolInfo_SymbolLocationStr(SymbolInfo, symbol->Pointer);

                PhAcquireQueuedLockExclusive(&SearchResultsLock);
                PhAddItemList(SearchResults, symbol);
                PhReleaseQueuedLockExclusive(&SearchResultsLock);
            }
            break;
        }
    }

    return TRUE;
}

VOID PrintClassInfo(
    _In_ PPDB_SYMBOL_CONTEXT Context, 
    _In_ ULONG Index, 
    _In_ UdtClassInfo* Info
    )
{
    //PPV_SYMBOL_NODE symbol;
    //symbol = PhAllocate(sizeof(PV_SYMBOL_NODE));
    //memset(symbol, 0, sizeof(PV_SYMBOL_NODE));
    //symbol->Type = PV_SYMBOL_TYPE_CLASS;
    //symbol->Address = VarInfo.sDataInfo.Address;
    //symbol->Size = (ULONG)Info->Offset;
    //symbol->Name = SymbolInfo_GetTypeName(Context, Index, Info->Name);// PhCreateString(SymbolInfo_UdtKindStr(Info->UDTKind));
    //symbol->Data = SymbolInfo_GetTypeName(Context, Index, Info->Name);
    //SymbolInfo_SymbolLocationStr(SymbolInfo, symbol->Pointer);
    //Info.Nested
    //if (PhEqualString2(symbol->Name, L"STRUCT", TRUE))
    //    symbol->Type = PV_SYMBOL_TYPE_STRUCT;
    //PhAcquireQueuedLockExclusive(&SearchResultsLock);
    //PhAddItemList(SearchResults, symbol);
    //PhReleaseQueuedLockExclusive(&SearchResultsLock);

    for (ULONG i = 0; i < Info->NumVariables; i++)
    {
        TypeInfo VarInfo;

        if (!SymbolInfo_DumpType(Context->BaseAddress, Info->Variables[i], &VarInfo))
        {
            // Continue
        }
        else if (VarInfo.Tag != SymTagData)
        {
            // Unexpected symbol tag.
        }
        else
        {
            //PPV_SYMBOL_NODE symbol;
            //
            //symbol = PhAllocate(sizeof(PV_SYMBOL_NODE));
            //memset(symbol, 0, sizeof(PV_SYMBOL_NODE));
            //
            //symbol->Type = PV_SYMBOL_TYPE_MEMBER;
            //
            // Location 
            switch (VarInfo.DataInfo.dataKind)
            {
            case DataIsGlobal:
            case DataIsStaticLocal:
            case DataIsFileStatic:
            case DataIsStaticMember:
                {
                    //symbol->Address = VarInfo.sDataInfo.Address;
                }
                break;
            case DataIsLocal:
            case DataIsParam:
            case DataIsObjectPtr:
            case DataIsMember:
                {
                    //symbol->Address = VarInfo.sDataInfo.Offset;
                }
                break;
            default:
                break; // TODO Add support for constants
            }
          
            //symbol->Size = (ULONG)VarInfo.sDataInfo.Offset;
            //symbol->Name = PhCreateString(VarInfo.sDataInfo.Name);
            //symbol->Data = SymbolInfo_GetTypeName(Context, VarInfo.sDataInfo.TypeIndex, VarInfo.sDataInfo.Name);
            //SymbolInfo_SymbolLocationStr(SymbolInfo, symbol->Pointer);
            //Info.Info.sUdtClassInfo.Nested  
            //Info.Info.sUdtClassInfo.NumVariables
            //PhAcquireQueuedLockExclusive(&SearchResultsLock);
            //PhAddItemList(SearchResults, symbol);
            //PhReleaseQueuedLockExclusive(&SearchResultsLock);
        }
    }

    for (ULONG i = 0; i < Info->NumFunctions; i++)
    {
        TypeInfo VarInfo;

        if (!SymbolInfo_DumpType(Context->BaseAddress, Info->Variables[i], &VarInfo))
        {
            // Continue
        }
        else if (VarInfo.Tag != SymTagFunction)
        {
            // Unexpected symbol tag.
        }
        else
        {
            PPV_SYMBOL_NODE symbol;

            symbol = PhAllocate(sizeof(PV_SYMBOL_NODE));
            memset(symbol, 0, sizeof(PV_SYMBOL_NODE));

            symbol->Type = PV_SYMBOL_TYPE_FUNCTION;
            symbol->Address = VarInfo.DataInfo.Address;
            symbol->Size = (ULONG)VarInfo.DataInfo.Offset;
            symbol->Name = PhCreateString(VarInfo.DataInfo.Name);
            symbol->Data = SymbolInfo_GetTypeName(Context, VarInfo.DataInfo.TypeIndex, VarInfo.DataInfo.Name);
            //SymbolInfo_SymbolLocationStr(SymbolInfo, symbol->Pointer); 
            // Info.Info.sUdtClassInfo.Nested
            // Info.Info.sUdtClassInfo.NumVariables

            PhAcquireQueuedLockExclusive(&SearchResultsLock);
            PhAddItemList(SearchResults, symbol);
            PhReleaseQueuedLockExclusive(&SearchResultsLock);

            // Enumerate function parameters and local variables...
            PdbDumpAddress(Context, VarInfo.DataInfo.Address);
        }
    }

    for (ULONG i = 0; i < Info->NumBaseClasses; i++)
    {
        TypeInfo baseInfo;

        if (!SymbolInfo_DumpType(Context->BaseAddress, Info->BaseClasses[i], &baseInfo))
        {
            // Continue
        }
        else if (baseInfo.Tag != SymTagBaseClass)
        {
            // Unexpected symbol tag
        }
        else
        {
            TypeInfo BaseUdtInfo;

            // Obtain the next base class 
            if (!SymbolInfo_DumpType(Context->BaseAddress, baseInfo.BaseClassInfo.TypeIndex, &BaseUdtInfo))
            {
                // Continue
            }
            else if (BaseUdtInfo.Tag != SymTagUDT)
            {
                // Unexpected symbol tag
            }
            else
            {
                if (baseInfo.BaseClassInfo.Virtual)
                {
                    //PhAddItemList(L"VIRTUAL_BASE_CLASS", BaseUdtInfo.sUdtClassInfo.Name)
                }
                else
                {
                    //PhAddItemList(L"BASE_CLASS", BaseUdtInfo.sUdtClassInfo.Name)
                }
            }
        }
    }
}

VOID PrintUserDefinedTypes(
    _In_ PPDB_SYMBOL_CONTEXT Context
    )
{
    ULONG i;
    ULONG index;
    TypeInfo info;

    for (i = 0; i < Context->UdtList->Count; i++)
    {
        index = PtrToUlong(Context->UdtList->Items[i]);

        if (SymbolInfo_DumpType(Context->BaseAddress, index, &info))
        {
            if (info.Tag == SymTagUDT)
            {
                if (info.UdtKind)
                {
                    // Print information about the class 
                    PrintClassInfo(Context, index, &info.UdtClassInfo);

                    // Print information about its base classes 
                    for (ULONG i = 0; i < info.UdtClassInfo.NumBaseClasses; i++)
                    {
                        TypeInfo baseInfo;

                        if (!SymbolInfo_DumpType(Context->BaseAddress, info.UdtClassInfo.BaseClasses[i], &baseInfo))
                        {
                            // Continue
                        }
                        else if (baseInfo.Tag != SymTagBaseClass)
                        {
                            // Continue
                        }
                        else
                        {
                            // Obtain information about the base class 
                            TypeInfo baseUdtInfo;

                            if (!SymbolInfo_DumpType(Context->BaseAddress, baseInfo.BaseClassInfo.TypeIndex, &baseUdtInfo))
                            {
                                // Continue
                            }
                            else if (baseUdtInfo.Tag != SymTagUDT)
                            {
                                // Continue
                            }
                            else
                            {
                                // Print information about the base class 
                                PrintClassInfo(Context, baseInfo.BaseClassInfo.TypeIndex, &baseUdtInfo.UdtClassInfo);
                            }
                        }
                    }
                }
                else
                {
                    //info.sUdtUnionInfo.Name
                    //PhCreateString(SymbolInfo_UdtKindStr(Info->UDTKind));                                                               
                    //SymbolInfo_GetTypeName(Context, index, info.sUdtUnionInfo.Name);
                }
            }
        }
    }
}

VOID ShowModuleSymbolInfo(
    _In_ ULONG64 BaseAddress
    )
{
    IMAGEHLP_MODULE64 info = { sizeof(IMAGEHLP_MODULE64) };

    if (!SymGetModuleInfoW64_I(NtCurrentProcess(), BaseAddress, &info))
        return;

    switch (info.SymType)
    {
    case SymNone:
        //PhCreateString(L"No symbols available for the module.");
        break;
    case SymExport:
        //PhFormatString(L"Loaded Exports symbols, Type information: %s", info.TypeInfo ? L"Available" : L"Not available");
        break;
    case SymCoff:
        //PhFormatString(L"Loaded COFF symbols, Type information: %s", info.TypeInfo ? L"Available" : L"Not available");
        break;
    case SymCv:
        //PhFormatString(L"Loaded CodeView symbols, Type information: %s", info.TypeInfo ? L"Available" : L"Not available");
        break;
    case SymSym:
        //PhFormatString(L"Loaded SYM symbols, Type information: %s", info.TypeInfo ? L"Available" : L"Not available");
        break;
    case SymVirtual:
        //PhFormatString(L"Loaded Virtual symbols, Type information: %s", info.TypeInfo ? L"Available" : L"Not available");
        break;
    case SymPdb:
        //PhFormatString(L"Loaded PDB symbols, Type information: %s", info.TypeInfo ? L"Available" : L"Not available");
        break;
    case SymDia:
        //PhFormatString(L"Loaded DIA symbols, Type information: %s", info.TypeInfo ? L"Available" : L"Not available");
        break;
    case SymDeferred:
        //PhCreateString(L"Loaded Deferred symbols"); // not actually loaded 
        break;
    default:
        //PhCreateString(L"Error: Unknown symbol format.");
        break;
    }

    //if (wcslen(info.ImageName) > 0)
    //L"Image name: %s\n" info.ImageName
    //if (wcslen(info.LoadedImageName) > 0)
    //L"Loaded image name: %s\n" info.LoadedImageName
    //if (wcslen(info.LoadedPdbName) > 0)
    //L"PDB file name: %s\n" info.LoadedPdbName
    // (It can only happen if the debug information is contained in a separate file (.DBG or .PDB) 
    //if (info.PdbUnmatched || info.DbgUnmatched)
    //    L"Warning: Unmatched symbols.

    //PhaFormatString(L"Line numbers: %s\n", info.LineNumbers ? L"Available" : L"Not available")->Buffer
    //PhaFormatString(L"Global symbols: %s\n", info.GlobalSymbols ? L"Available" : L"Not available")->Buffer
    //PhaFormatString(L"Type information: %s\n", info.TypeInfo ? L"Available" : L"Not available")->Buffer
    //PhaFormatString(L"Source indexing: %s\n", info.SourceIndexed ? L"Yes" : L"No")->Buffer
    //PhaFormatString(L"Public symbols: %s\n", info.Publics ? L"Available" : L"Not available")->Buffer
}

PPH_STRING PvFindDbghelpPath(
    _In_ ULONG Type
    )
{
    static struct
    {
        BOOLEAN Type;
        ULONG Folder;
        PWSTR AppendPath;
    } locations[] =
    {
#ifdef _WIN64
        { FALSE, CSIDL_PROGRAM_FILESX86, L"\\Windows Kits\\10\\Debuggers\\x64\\dbghelp.dll" },
        { FALSE, CSIDL_PROGRAM_FILESX86, L"\\Windows Kits\\8.1\\Debuggers\\x64\\dbghelp.dll" },
        { FALSE, CSIDL_PROGRAM_FILESX86, L"\\Windows Kits\\8.0\\Debuggers\\x64\\dbghelp.dll" },
        { FALSE, CSIDL_PROGRAM_FILES, L"\\Debugging Tools for Windows (x64)\\dbghelp.dll" },
        { TRUE, CSIDL_PROGRAM_FILESX86, L"\\Windows Kits\\10\\Debuggers\\x64\\symsrv.dll" },
        { TRUE, CSIDL_PROGRAM_FILESX86, L"\\Windows Kits\\8.1\\Debuggers\\x64\\symsrv.dll" },
        { TRUE, CSIDL_PROGRAM_FILESX86, L"\\Windows Kits\\8.0\\Debuggers\\x64\\symsrv.dll" },
        { TRUE, CSIDL_PROGRAM_FILES, L"\\Debugging Tools for Windows (x64)\\symsrv.dll" }
#else
        { FALSE, CSIDL_PROGRAM_FILES, L"\\Windows Kits\\10\\Debuggers\\x86\\dbghelp.dll" },
        { FALSE, CSIDL_PROGRAM_FILES, L"\\Windows Kits\\8.1\\Debuggers\\x86\\dbghelp.dll" },
        { FALSE, CSIDL_PROGRAM_FILES, L"\\Windows Kits\\8.0\\Debuggers\\x86\\dbghelp.dll" },
        { FALSE, CSIDL_PROGRAM_FILES, L"\\Debugging Tools for Windows (x86)\\dbghelp.dll" },
        { TRUE, CSIDL_PROGRAM_FILES, L"\\Windows Kits\\10\\Debuggers\\x86\\symsrv.dll" },
        { TRUE, CSIDL_PROGRAM_FILES, L"\\Windows Kits\\8.1\\Debuggers\\x86\\symsrv.dll" },
        { TRUE, CSIDL_PROGRAM_FILES, L"\\Windows Kits\\8.0\\Debuggers\\x86\\symsrv.dll" },
        { TRUE, CSIDL_PROGRAM_FILES, L"\\Debugging Tools for Windows (x86)\\symsrv.dll" }
#endif
    };

    PPH_STRING path;
    ULONG i;

    for (i = 0; i < sizeof(locations) / sizeof(locations[0]); i++)
    {
        if (locations[i].Type != Type)
            continue;

        path = PhGetKnownLocation(locations[i].Folder, locations[i].AppendPath);

        if (path)
        {
            if (RtlDoesFileExists_U(path->Buffer))
                return path;

            PhDereferenceObject(path);
        }
    }

    return NULL;
}

NTSTATUS PeDumpFileSymbols(
    _In_ PPDB_SYMBOL_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE fileHandle = NULL;
    HMODULE dbghelpHandle;
    HMODULE symsrvHandle;
    ULONG64 baseAddress = 0;
    LARGE_INTEGER fileSize;
    PPH_STRING dbghelpPath;
    PPH_STRING symsrvPath;

    if (!(dbghelpPath = PvFindDbghelpPath(FALSE)))
        return 1;
    if (!(symsrvPath = PvFindDbghelpPath(TRUE)))
        return 1;

    if (!(dbghelpHandle = LoadLibrary(dbghelpPath->Buffer)))
        return 1;
    if (!(symsrvHandle = LoadLibrary(symsrvPath->Buffer)))
        return 1;

    SymInitialize_I = PhGetProcedureAddress(dbghelpHandle, "SymInitialize", 0);
    SymCleanup_I = PhGetProcedureAddress(dbghelpHandle, "SymCleanup", 0);
    SymEnumSymbolsW_I = PhGetProcedureAddress(dbghelpHandle, "SymEnumSymbolsW", 0);
    SymEnumTypesW_I = PhGetProcedureAddress(dbghelpHandle, "SymEnumTypesW", 0);
    SymSetSearchPathW_I = PhGetProcedureAddress(dbghelpHandle, "SymSetSearchPathW", 0);
    SymGetOptions_I = PhGetProcedureAddress(dbghelpHandle, "SymGetOptions", 0);
    SymSetOptions_I = PhGetProcedureAddress(dbghelpHandle, "SymSetOptions", 0);
    SymLoadModuleExW_I = PhGetProcedureAddress(dbghelpHandle, "SymLoadModuleExW", 0);
    SymGetModuleInfoW64_I = PhGetProcedureAddress(dbghelpHandle, "SymGetModuleInfoW64", 0);
    SymGetTypeFromNameW_I = PhGetProcedureAddress(dbghelpHandle, "SymGetTypeFromNameW", 0);
    SymGetTypeInfo_I = PhGetProcedureAddress(dbghelpHandle, "SymGetTypeInfo", 0);
    SymSetContext_I = PhGetProcedureAddress(dbghelpHandle, "SymSetContext", 0);

    SymSetOptions_I(SymGetOptions_I() | SYMOPT_DEBUG | SYMOPT_UNDNAME);

    if (!SymInitialize_I(NtCurrentProcess(), NULL, FALSE))
        return 1;

    if (!SymSetSearchPathW_I(NtCurrentProcess(), L"SRV*C:\\symbols*http://msdl.microsoft.com/download/symbols"))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(PvFileName),
        FILE_GENERIC_READ,
        0,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
        goto CleanupExit;

    if (NT_SUCCESS(PhGetFileSize(fileHandle, &fileSize)) && fileSize.QuadPart == 0)
        goto CleanupExit;

    if (PhEndsWithString2(PvFileName, L".pdb", TRUE))
        baseAddress = 0x10000000;

    if (Context->BaseAddress = SymLoadModuleExW_I(
        NtCurrentProcess(),
        fileHandle,
        PhGetString(PvFileName),
        NULL,
        baseAddress,
        (ULONG)fileSize.QuadPart,
        NULL,
        0
        ))
    {
        //ShowModuleSymbolInfo(symbolBaseAddress);
        SymEnumSymbolsW_I(NtCurrentProcess(), Context->BaseAddress, NULL, EnumCallbackProc, Context);

        // Enumerate user defined types 
        //PrintUserDefinedTypes(Context);
        //SymEnumTypesW_I(NtCurrentProcess(), Context->BaseAddress, EnumCallbackProc, Context);
    }

CleanupExit:

    SymCleanup_I(NtCurrentProcess());

    if (fileHandle)
        NtClose(fileHandle);

    PostMessage(Context->DialogHandle, WM_PV_SEARCH_FINISHED, 0, 0);

    return 0;
}

VOID PdbDumpAddress(
    _In_ PPDB_SYMBOL_CONTEXT Context, 
    _In_ ULONG64 Address
    )
{
    IMAGEHLP_STACK_FRAME sf;

    sf.InstructionOffset = Address;

    SymSetContext_I(NtCurrentProcess(), &sf, 0);
    SymEnumSymbolsW_I(NtCurrentProcess(), 0, NULL, EnumCallbackProc, Context);
}