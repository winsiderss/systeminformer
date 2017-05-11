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

typedef BOOL(WINAPI *_SymGetModuleInfoW64)(
    _In_ HANDLE hProcess,
    _In_ ULONG64 qwAddr,
    _Out_ PIMAGEHLP_MODULEW64 ModuleInfo
    );

_SymInitialize SymInitialize_I = NULL;
_SymCleanup SymCleanup_I = NULL;
_SymEnumSymbolsW SymEnumSymbolsW_I = NULL;
_SymSetSearchPathW SymSetSearchPathW_I = NULL;
_SymGetOptions SymGetOptions_I = NULL;
_SymSetOptions SymSetOptions_I = NULL;
_SymLoadModuleExW SymLoadModuleExW_I = NULL;
_SymGetModuleInfoW64 SymGetModuleInfoW64_I = NULL;
_SymGetTypeInfo SymGetTypeInfo_I = NULL;

BOOLEAN SymInfoDump_DumpBasicType(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ BaseTypeInfo *Info
    )
{
    ULONG baseType = btNoType;
    ULONG64 length = 0;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagBaseType))
        return FALSE;

    // Basic type ("basicType" in DIA)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_BASETYPE, &baseType))
        return FALSE;

    // Length ("length" in DIA)     
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_LENGTH, &length))
        return FALSE;

    Info->BaseType = (BasicType)baseType;
    Info->Length = length;
    return TRUE;
}

BOOLEAN SymInfoDump_DumpPointerType(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ PointerTypeInfo* Info
    )
{
    ULONG TypeIndex = 0;
    ULONG64 Length = 0;
 
    if (!SymInfoDump_CheckTag(Context, Index, SymTagPointerType))
        return FALSE;

    // Type index ("typeId" in DIA)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_TYPEID, &TypeIndex))
        return FALSE;

    // Length ("length" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_LENGTH, &Length))
        return FALSE;

    // Reference ("reference" in DIA) 
    // This property is not available via SymGetTypeInfo_I??
    // TODO: Figure out how DIA determines if the pointer is a pointer or a reference.

    Info->TypeIndex = TypeIndex;
    Info->Length = Length;

    return TRUE;
}

BOOLEAN SymInfoDump_DumpTypedef(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ TypedefInfo* Info
    )
{
    ULONG typeIndex = 0;
    PWSTR symbolName = NULL;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagTypedef))
        return FALSE;
       
    // Type index ("typeId" in DIA)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_TYPEID, &typeIndex))
        return FALSE;

    // Name ("name" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_SYMNAME, &symbolName) || !symbolName)
        return FALSE;

    Info->TypeIndex = typeIndex;

    wcsncpy(Info->Name, symbolName, ARRAYSIZE(Info->Name));
    Info->Name[ARRAYSIZE(Info->Name) - 1] = 0;
    LocalFree(symbolName);

    return TRUE;
}

BOOLEAN SymInfoDump_DumpEnum(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ EnumInfo* Info
    )
{
    ULONG TypeIndex = 0;
    ULONG Nested = 0;
    PWSTR symbolName = NULL;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagEnum))
        return FALSE;

    // Name ("name" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_SYMNAME, &symbolName) || !symbolName)
        return FALSE;

    wcsncpy(Info->Name, symbolName, ARRAYSIZE(Info->Name));
    Info->Name[ARRAYSIZE(Info->Name) - 1] = 0;
    LocalFree(symbolName);

    // Type index ("typeId" in DIA)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_TYPEID, &TypeIndex))
        return FALSE;

    // Nested ("nested" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_NESTED, &Nested))
        return FALSE;

    // Enumerators 
    if (!SymInfoDump_Enumerators(Context, Index, Info->Enums, &Info->NumEnums, ARRAYSIZE(Info->Enums)))
        return FALSE;

    Info->TypeIndex = TypeIndex;
    Info->Nested = (Nested != 0);

    return TRUE;
}

BOOLEAN SymInfoDump_DumpArrayType(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ ArrayTypeInfo* Info
    )
{
    ULONG elementTypeIndex = 0;
    ULONG64 length = 0;
    ULONG indexTypeIndex = 0;

    // Check if it is really SymTagArrayType 
    if (!SymInfoDump_CheckTag(Context, Index, SymTagArrayType))
        return FALSE;

    // Element type index 
    if (!SymInfoDump_ArrayElementTypeIndex(Context, Index, &elementTypeIndex))
        return FALSE;

    // Length ("length" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_LENGTH, &length))
        return FALSE;

    // Type index of the array index element 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_ARRAYINDEXTYPEID, &indexTypeIndex))
        return FALSE;

    Info->ElementTypeIndex = elementTypeIndex;
    Info->Length = length;
    Info->IndexTypeIndex = indexTypeIndex;

    if (length > 0)
    {
        // Dimensions 
        if (!SymInfoDump_ArrayDims(
            Context,
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

BOOLEAN SymInfoDump_DumpUDT(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ TypeInfo* Info
    )
{
    ULONG UDTKind = 0;
    BOOLEAN result = FALSE;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagUDT))
        return FALSE;

    // Determine UDT kind (class/structure or union?)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_UDTKIND, &UDTKind))
        return FALSE;

    switch (UDTKind)
    {
    case UdtStruct:
        Info->UdtKind = TRUE;
        result = SymInfoDump_DumpUDTClass(Context, Index, &Info->sUdtClassInfo);
        break;
    case UdtClass:
        Info->UdtKind = TRUE;
        result = SymInfoDump_DumpUDTClass(Context, Index, &Info->sUdtClassInfo);
        break;
    case UdtUnion:
        Info->UdtKind = FALSE;
        result = SymInfoDump_DumpUDTUnion(Context, Index, &Info->sUdtUnionInfo);
        break;
    }

    return result;
}

BOOLEAN SymInfoDump_DumpUDTClass(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ UdtClassInfo* Info
    )
{
    ULONG UDTKind = 0;
    PWSTR symbolName = NULL;
    ULONG64 Length = 0;
    ULONG Nested = 0;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagUDT))
        return FALSE;

    // Check if it is really a class or structure UDT ? 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_UDTKIND, &UDTKind))
        return FALSE;

    if ((UDTKind != UdtStruct) && (UDTKind != UdtClass))
        return FALSE;

    // Name ("name" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_SYMNAME, &symbolName) || !symbolName)
        return FALSE;

    wcsncpy(Info->Name, symbolName, ARRAYSIZE(Info->Name));
    Info->Name[ARRAYSIZE(Info->Name) - 1] = 0;
    LocalFree(symbolName);

    // Length ("length" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_LENGTH, &Length))
        return FALSE;

    // Nested ("nested" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_NESTED, &Nested))
        return FALSE;

    // Member variables 
    if (!SymInfoDump_UdtVariables(Context, Index, Info->Variables, &Info->NumVariables, ARRAYSIZE(Info->Variables)))
        return FALSE;

    // Member functions 
    if (!SymInfoDump_UdtFunctions(Context, Index, Info->Functions, &Info->NumFunctions, ARRAYSIZE(Info->Functions)))
        return FALSE;

    // Base classes 
    if (!SymInfoDump_UdtBaseClasses(Context, Index, Info->BaseClasses, &Info->NumBaseClasses, ARRAYSIZE(Info->BaseClasses)))
        return FALSE;

    Info->UDTKind = (UdtKind)UDTKind;
    Info->Length = Length;
    Info->Nested = (Nested != 0);

    return TRUE;
}

BOOLEAN SymInfoDump_DumpUDTUnion(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ UdtUnionInfo *Info
    )
{
    ULONG UDTKind = 0;
    PWSTR symbolName = 0;
    ULONG64 Length = 0;
    ULONG Nested = 0;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagUDT))
        return FALSE;

    // Check if it is really a union UDT ? 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_UDTKIND, &UDTKind))
        return FALSE;

    if (UDTKind != UdtUnion)
        return FALSE;

    // Name ("name" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_SYMNAME, &symbolName) || !symbolName)
        return FALSE;

    wcsncpy(Info->Name, symbolName, ARRAYSIZE(Info->Name));
    Info->Name[ARRAYSIZE(Info->Name) - 1] = 0;
    LocalFree(symbolName);

    // Length ("length" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_LENGTH, &Length))
        return FALSE;

    // Nested ("nested" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_NESTED, &Nested))
        return FALSE;

    // Union members 
    if (!SymInfoDump_UdtUnionMembers(Context, Index, Info->Members, &Info->NumMembers, ARRAYSIZE(Info->Members)))
        return FALSE;

    Info->UDTKind = (UdtKind)UDTKind;
    Info->Length = Length;
    Info->Nested = (Nested != 0);

    return TRUE;
}

BOOLEAN SymInfoDump_DumpFunctionType(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ FunctionTypeInfo *Info
    )
{
    ULONG RetTypeIndex = 0;
    ULONG NumArgs = 0;
    ULONG CallConv = 0;
    ULONG ClassIndex = 0;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagFunctionType))
        return FALSE;

    // Index of the return type symbol ("typeId" in DIA)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_TYPEID, &RetTypeIndex))
        return FALSE;

    // Number of arguments ("count" in DIA) 
    // Note: For non-static member functions, it includes "this" 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_COUNT, &NumArgs))
        return FALSE;

    // Do not save it in the data structure, since we obtain the number of arguments 
    // again using FunctionArguments() (see below). 
    // But later we will use this value to determine whether the function 
    // is static or not (if it is a member function) 

    // Calling convention ("callingConvention" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_CALLING_CONVENTION, &CallConv))
        return FALSE;

    // Parent class type index ("classParent" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_CLASSPARENTID, &ClassIndex))
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
        if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_THISADJUST, &ThisAdjust))
            return FALSE;

        Info->ThisAdjust = ThisAdjust;
    }

    // Test for GetChildren()
    /*
        ULONG cMaxChildren = 4;
        ULONG Children[cMaxChildren];
        ULONG NumChildren = 0;

        if( !GetChildren( m_hProcess, BaseAddress, Index, Children, NumChildren, cMaxChildren ) )
        {
            ULONG ErrCode = 0;
            _ASSERTE( !_T("GetChildren() failed.") );
        }
        else
        {
            ULONG ErrCode = 0;
        }
    */

    // Dump function arguments 
    if (!SymInfoDump_FunctionArguments(Context, Index, Info->Args, &Info->NumArgs, ARRAYSIZE(Info->Args)))
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

BOOLEAN SymInfoDump_DumpFunctionArgType(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ FunctionArgTypeInfo *Info
    )
{
    ULONG typeIndex = 0;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagFunctionArgType))
        return FALSE;

    // Index of the argument type ("typeId" in DIA)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_TYPEID, &typeIndex))
        return FALSE;

    Info->TypeIndex = typeIndex;

    return TRUE;
}

BOOLEAN SymInfoDump_DumpBaseClass(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ BaseClassInfo *Info
    )
{
    ULONG typeIndex = 0;
    ULONG virtualBase = 0;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagBaseClass))
        return FALSE;

    // Base class UDT 
    if (!SymGetTypeInfo_I(
        NtCurrentProcess(),
        Context->BaseAddress,
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
        Context->BaseAddress,
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
            Context->BaseAddress,
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
            Context->BaseAddress,
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

BOOLEAN SymInfoDump_DumpData(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ DataInfo *Info
    )
{
    PWSTR symbolName = NULL;
    ULONG TypeIndex = 0;
    ULONG dataKind = 0;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagData))
        return FALSE;

    // Name ("name" in DIA) 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_SYMNAME, &symbolName) || !symbolName)
        return FALSE;

    wcsncpy(Info->Name, symbolName, ARRAYSIZE(Info->Name));
    Info->Name[ARRAYSIZE(Info->Name) - 1] = 0;
    LocalFree(symbolName);

    // Index of type symbol ("typeId" in DIA)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_TYPEID, &TypeIndex))
        return FALSE;

    // Data kind ("dataKind" in DIA)
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_DATAKIND, &dataKind))
        return FALSE;

    Info->TypeIndex = TypeIndex;
    Info->dataKind = (DataKind)dataKind;

    // Location, depending on the data kind 
    switch (dataKind)
    {
    case DataIsGlobal:
    case DataIsStaticLocal:
    case DataIsFileStatic:
    case DataIsStaticMember:
        {
            // Use Address; Offset is not defined
            // Note: If it is DataIsStaticMember, then this is a static member of a class defined in another module 
            // (it does not have an address in this module) 

            ULONG64 address = 0;

            if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_ADDRESS, &address))
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
            // Use Offset; Address is not defined
            ULONG offset = 0;

            if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_OFFSET, &offset))
                return FALSE;

            Info->Offset = offset;
            Info->Address = 0;
        }
        break;

    default:
        // Unknown location 
        Info->Address = 0;
        Info->Offset = 0;
        break;
    }

    return TRUE;
}

BOOLEAN SymInfoDump_DumpType(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ TypeInfo *Info
    )
{
    ULONG tag = SymTagNull;
    BOOLEAN result = FALSE;

    // Get the symbol's tag 
    if (!SymGetTypeInfo_I(
        NtCurrentProcess(),
        Context->BaseAddress,
        Index,
        TI_GET_SYMTAG,
        &tag
        ))
    {
        return FALSE;
    }

    Info->Tag = (enum SymTagEnum)tag;

    // Dump information about the symbol (depending on the tag).
    switch (tag)
    {
    case SymTagBaseType:
        result = SymInfoDump_DumpBasicType(Context, Index, &Info->sBaseTypeInfo);
        break;
    case SymTagPointerType:
        result = SymInfoDump_DumpPointerType(Context, Index, &Info->sPointerTypeInfo);
        break;
    case SymTagTypedef:
        result = SymInfoDump_DumpTypedef(Context, Index, &Info->sTypedefInfo);
        break;
    case SymTagEnum:
        result = SymInfoDump_DumpEnum(Context, Index, &Info->sEnumInfo);
        break;
    case SymTagArrayType:
        result = SymInfoDump_DumpArrayType(Context, Index, &Info->sArrayTypeInfo);
        break;
    case SymTagUDT:
        result = SymInfoDump_DumpUDT(Context, Index, Info);
        break;
    case SymTagFunctionType:
        result = SymInfoDump_DumpFunctionType(Context, Index, &Info->sFunctionTypeInfo);
        break;
    case SymTagFunctionArgType:
        result = SymInfoDump_DumpFunctionArgType(Context, Index, &Info->sFunctionArgTypeInfo);
        break;
    case SymTagBaseClass:
        result = SymInfoDump_DumpBaseClass(Context, Index, &Info->sBaseClassInfo);
        break;
    case SymTagData:
        result = SymInfoDump_DumpData(Context, Index, &Info->sDataInfo);
        break;
    }

    return result;
}

BOOLEAN SymInfoDump_DumpSymbolType(
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
    return SymInfoDump_DumpType(Context, *TypeIndex, Info);
}

BOOLEAN SymInfoDump_CheckTag(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _In_ ULONG Tag
    )
{
    ULONG symTag = SymTagNull;

    if (!SymGetTypeInfo_I(
        NtCurrentProcess(),
        Context->BaseAddress,
        Index,
        TI_GET_SYMTAG,
        &symTag
        ))
    {
        return FALSE;
    }

    return symTag == Tag;
}

BOOLEAN SymInfoDump_SymbolSize(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ ULONG64* Size
    )
{
    ULONG index;
    ULONG64 length = 0;

    // Does the symbol support "length" property ? 
    if (SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_LENGTH, &length))
    {
        *Size = length;
        return TRUE;
    }
    else
    {
        // No, it does not - it can be SymTagTypedef 
        if (!SymInfoDump_CheckTag(Context, Index, SymTagTypedef))
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

                if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, index, TI_GET_TYPEID, &tempIndex))
                    return FALSE;

                index = tempIndex;

            } while (SymInfoDump_CheckTag(Context, index, SymTagTypedef));

            // And get the length 
            if (SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, index, TI_GET_LENGTH, &length))
            {
                *Size = length;
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOLEAN SymInfoDump_ArrayElementTypeIndex(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG ArrayIndex,
    _Inout_ ULONG* ElementTypeIndex
    )
{
    ULONG index;
    ULONG elementIndex = 0;

    if (!SymInfoDump_CheckTag(Context, ArrayIndex, SymTagArrayType))
        return FALSE;

    // Get the array element type 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, ArrayIndex, TI_GET_TYPEID, &elementIndex))
        return FALSE;

    // If the array element type is SymTagArrayType, skip to its type 
    index = elementIndex;

    while (SymInfoDump_CheckTag(Context, elementIndex, SymTagArrayType))
    {
        if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, index, TI_GET_TYPEID, &elementIndex))
            return FALSE;

        index = elementIndex;
    }

    // We have found the ultimate type of the array element 
    *ElementTypeIndex = elementIndex;

    return TRUE;
}

BOOLEAN SymInfoDump_ArrayDims(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ ULONG64* pDims,
    _Inout_ ULONG* Dims,
    _In_ ULONG MaxDims
    )
{
    ULONG index;
    ULONG dimCount;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagArrayType))
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
        if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, index, TI_GET_LENGTH, &length) || (length == 0))
            return FALSE;

        // Size of the current dimension 
        // (it is the size of its SymTagArrayType symbol divided by the size of its 
        // type symbol, which can be another SymTagArrayType symbol or the array element symbol) 

        // Its type 
        if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, index, TI_GET_TYPEID, &typeIndex))
        {
            // No type - we are done 
            break;
        }

        // Size of its type 
        if (!SymInfoDump_SymbolSize(Context, typeIndex, &typeSize) || (typeSize == 0))
            return FALSE;

        // Size of the dimension 
        pDims[i] = length / typeSize;
        dimCount++;

        // Test only 
/*
        ULONG ElemCount = 0;
        if( !SymGetTypeInfo_I( m_hProcess, BaseAddress, CurIndex, TI_GET_COUNT, &ElemCount ) )
        {
            // and continue ...
        }
        else if( ElemCount != pDims[i] )
        {
            _ASSERTE( !_T("TI_GET_COUNT does not match.") );
        }
        else
        {
            //_ASSERTE( !_T("TI_GET_COUNT works!") );
        }
*/
        // If the type symbol is not SymTagArrayType, we are done 
        if (!SymInfoDump_CheckTag(Context, typeIndex, SymTagArrayType))
            break;

        index = typeIndex;
    }

    if (dimCount == 0)
        return FALSE;

    *Dims = dimCount;

    return TRUE;
}

BOOLEAN SymInfoDump_UdtVariables(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ ULONG* pVars,
    _Inout_ ULONG* Vars,
    _In_ ULONG MaxVars
    )
{
    ULONG childrenLength = 0;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagUDT))
        return FALSE;

    if (MaxVars <= 0)
        return FALSE;

    // Get the number of children 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_CHILDRENCOUNT, &childrenLength))
        return FALSE;

    if (childrenLength == 0)
    {
        *Vars = 0;
        return TRUE; // No children -> no member variables 
    }

    // Get the children 
    ULONG FindChildrenSize = sizeof(TI_FINDCHILDREN_PARAMS) + childrenLength * sizeof(ULONG);
    TI_FINDCHILDREN_PARAMS* params = (TI_FINDCHILDREN_PARAMS*)_alloca(FindChildrenSize);
    memset(params, 0, FindChildrenSize);
    params->Count = childrenLength;

    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_FINDCHILDREN, params))
        return FALSE;

    *Vars = 0;

    // Enumerate children, looking for base classes, and copy their indexes.
    for (ULONG i = 0; i < childrenLength; i++)
    {
        if (SymInfoDump_CheckTag(Context, params->ChildId[i], SymTagData))
        {
            pVars[*Vars] = params->ChildId[i];

            (*Vars)++;

            if (*Vars == MaxVars)
                break;
        }
    }

    return TRUE;
}

BOOLEAN SymInfoDump_UdtFunctions(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ ULONG* pFuncs,
    _Inout_ ULONG* Funcs,
    _In_ ULONG MaxFuncs
    )
{
    ULONG childrenLength = 0;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagUDT))
        return FALSE;

    if (MaxFuncs <= 0)
        return FALSE;

    // Get the number of children 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_CHILDRENCOUNT, &childrenLength))
        return FALSE;

    if (childrenLength == 0)
    {
        *Funcs = 0; // No children -> no member variables 
        return TRUE;
    }

    // Get the children 
    ULONG FindChildrenSize = sizeof(TI_FINDCHILDREN_PARAMS) + childrenLength * sizeof(ULONG);
    TI_FINDCHILDREN_PARAMS* params = (TI_FINDCHILDREN_PARAMS*)_alloca(FindChildrenSize);
    memset(params, 0, FindChildrenSize);

    params->Count = childrenLength;

    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_FINDCHILDREN, params))
        return FALSE;

    *Funcs = 0;

    // Enumerate children, looking for base classes, and copy their indexes.
    for (ULONG i = 0; i < childrenLength; i++)
    {
        if (SymInfoDump_CheckTag(Context, params->ChildId[i], SymTagFunction))
        {
            pFuncs[*Funcs] = params->ChildId[i];

            (*Funcs)++;

            if (*Funcs == MaxFuncs)
                break;
        }
    }

    return TRUE;
}

BOOLEAN SymInfoDump_UdtBaseClasses(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ ULONG* pBases,
    _Inout_ ULONG* Bases,
    _In_ ULONG MaxBases
    )
{
    ULONG childrenLength = 0;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagUDT))
        return FALSE;

    if (MaxBases <= 0)
        return FALSE;

    // Get the number of children 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_CHILDRENCOUNT, &childrenLength))
        return FALSE;

    if (childrenLength == 0)
    {
        *Bases = 0; // No children -> no member variables 
        return TRUE;
    }

    // Get the children 
    ULONG FindChildrenSize = sizeof(TI_FINDCHILDREN_PARAMS) + childrenLength * sizeof(ULONG);
    TI_FINDCHILDREN_PARAMS* params = (TI_FINDCHILDREN_PARAMS*)_alloca(FindChildrenSize);
    memset(params, 0, FindChildrenSize);

    params->Count = childrenLength;

    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_FINDCHILDREN, params))
        return FALSE;

    *Bases = 0;

    // Enumerate children, looking for base classes, and copy their indexes.
    for (ULONG i = 0; i < childrenLength; i++)
    {
        if (SymInfoDump_CheckTag(Context, params->ChildId[i], SymTagBaseClass))
        {
            pBases[*Bases] = params->ChildId[i];

            (*Bases)++;

            if (*Bases == MaxBases)
                break;
        }
    }

    return TRUE;
}

BOOLEAN SymInfoDump_UdtUnionMembers(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ ULONG* pMembers,
    _Inout_ ULONG* Members,
    _In_ ULONG MaxMembers
    )
{
    ULONG childrenLength = 0;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagUDT))
        return FALSE;

    if (MaxMembers <= 0)
        return FALSE;

    // Get the number of children 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_CHILDRENCOUNT, &childrenLength))
        return FALSE;

    if (childrenLength == 0)
    {
        *Members = 0;
        return TRUE; // No children -> no members 
    }

    // Get the children 
    ULONG FindChildrenSize = sizeof(TI_FINDCHILDREN_PARAMS) + childrenLength * sizeof(ULONG);
    TI_FINDCHILDREN_PARAMS* params = (TI_FINDCHILDREN_PARAMS*)_alloca(FindChildrenSize);
    memset(params, 0, FindChildrenSize);

    params->Count = childrenLength;

    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_FINDCHILDREN, params))
        return FALSE;

    *Members = 0;

    // Enumerate children, looking for enumerators, and copy their indexes.
    for (ULONG i = 0; i < childrenLength; i++)
    {
        if (SymInfoDump_CheckTag(Context, params->ChildId[i], SymTagData))
        {
            pMembers[*Members] = params->ChildId[i];

            (*Members)++;

            if (*Members == MaxMembers)
                break;
        }
    }

    return TRUE;
}

BOOLEAN SymInfoDump_FunctionArguments(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ ULONG* pArgs,
    _Inout_ ULONG* Args,
    _In_ ULONG MaxArgs
    )
{
    ULONG childrenLength = 0;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagFunctionType))
        return FALSE;

    if (MaxArgs <= 0)
        return FALSE;

    // Get the number of children 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_CHILDRENCOUNT, &childrenLength))
        return FALSE;

    if (childrenLength == 0)
    {
        *Args = 0;
        return TRUE; // No children -> no member variables 
    }

    // Get the children 
    ULONG FindChildrenSize = sizeof(TI_FINDCHILDREN_PARAMS) + childrenLength * sizeof(ULONG);
    TI_FINDCHILDREN_PARAMS* params = (TI_FINDCHILDREN_PARAMS*)_alloca(FindChildrenSize);
    memset(params, 0, FindChildrenSize);

    params->Count = childrenLength;

    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_FINDCHILDREN, params))
        return FALSE;

    *Args = 0;

    // Enumerate children, looking for enumerators, and copy their indexes.
    for (ULONG i = 0; i < childrenLength; i++)
    {
        if (SymInfoDump_CheckTag(Context, params->ChildId[i], SymTagFunctionArgType))
        {
            pArgs[*Args] = params->ChildId[i];

            (*Args)++;

            if (*Args == MaxArgs)
                break;
        }
    }

    return TRUE;
}

BOOLEAN SymInfoDump_Enumerators(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ ULONG *pEnums,
    _Inout_ ULONG *Enums,
    _In_ ULONG MaxEnums
    )
{
    ULONG childrenLength = 0;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagEnum))
        return FALSE;

    if (MaxEnums <= 0)
        return FALSE;

    // Get the number of children 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_GET_CHILDRENCOUNT, &childrenLength))
        return FALSE;

    if (childrenLength == 0)
    {
        // No children -> no enumerators 
        *Enums = 0;
        return TRUE;
    }
    
    ULONG FindChildrenSize = sizeof(TI_FINDCHILDREN_PARAMS) + childrenLength * sizeof(ULONG);
    TI_FINDCHILDREN_PARAMS* params = (TI_FINDCHILDREN_PARAMS*)_alloca(FindChildrenSize);
    memset(params, 0, FindChildrenSize);
    params->Count = childrenLength;

    // Get the children 
    if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, Index, TI_FINDCHILDREN, params))
        return FALSE;

    *Enums = 0;

    // Enumerate children, looking for enumerators, and copy their indexes.
    for (ULONG i = 0; i < childrenLength; i++)
    {
        if (SymInfoDump_CheckTag(Context, params->ChildId[i], SymTagData))
        {
            pEnums[*Enums] = params->ChildId[i];

            (*Enums)++;

            if (*Enums == MaxEnums)
                break;
        }
    }

    return TRUE;
}

BOOLEAN SymInfoDump_TypeDefType(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ ULONG *UndTypeIndex
    )
{
    ULONG index;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagTypedef))
        return FALSE;

    // Skip to the type behind the type definition 
    index = Index;

    while (SymInfoDump_CheckTag(Context, index, SymTagTypedef))
    {
        if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, index, TI_GET_TYPEID, UndTypeIndex))
            return FALSE;

        index = *UndTypeIndex;
    }

    return TRUE;
}

BOOLEAN SymInfoDump_PointerType(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _Inout_ ULONG *UndTypeIndex,
    _Inout_ ULONG *NumPointers
    )
{
    ULONG index;

    if (!SymInfoDump_CheckTag(Context, Index, SymTagPointerType))
        return FALSE;

    // Skip to the type pointer points to 
    *NumPointers = 0;
    index = Index;

    while (SymInfoDump_CheckTag(Context, index, SymTagPointerType))
    {
        if (!SymGetTypeInfo_I(NtCurrentProcess(), Context->BaseAddress, index, TI_GET_TYPEID, UndTypeIndex))
            return FALSE;

        (*NumPointers)++;

        index = *UndTypeIndex;
    }

    return TRUE;
}

BOOLEAN SymInfoDump_GetTypeNameHelper(
    _In_ ULONG Index,
    _Inout_ PPDB_SYMBOL_CONTEXT Obj,
    _Out_ PWSTR *VarName,
    _Out_ PWSTR *TypeName
    )
{
    TypeInfo Info;

    // Get the type information 
    if (!SymInfoDump_DumpType(Obj, Index, &Info))
    {
        return FALSE;
    }
    else
    {
        // Is it a pointer ?  
        ULONG numPointers = 0;

        if (Info.Tag == SymTagPointerType)
        {
            // Yes, get the number of * to show 
            ULONG typeIndex = 0;

            if (!SymInfoDump_PointerType(Obj, Index, &typeIndex, &numPointers))
                return FALSE;

            // Get more information about the type the pointer points to                
            if (!SymInfoDump_DumpType(Obj, typeIndex, &Info))
                return FALSE;

            // Save the index of the type the pointer points to 
            Index = typeIndex;

            // ... and proceed with that type 
        }

        switch (Info.Tag)
        {
        case SymTagBaseType:
            *TypeName = SymInfoDump_BaseTypeStr(Info.sBaseTypeInfo.BaseType, Info.sBaseTypeInfo.Length);
            break;

        case SymTagTypedef:
            *TypeName = _wcsdup(Info.sTypedefInfo.Name);
            break;

        case SymTagUDT:
            {
                if (Info.UdtKind)
                {
                    // A class/structure 
                    *TypeName = _wcsdup(Info.sUdtClassInfo.Name);

                    // Add the UDT and its base classes to the collection of UDT indexes 
                    PhAddItemList(Obj->UdtList, UlongToPtr(Index));
                }
                else
                {
                    // A union 
                    *TypeName = _wcsdup(Info.sUdtUnionInfo.Name);
                }
            }
            break;

        case SymTagEnum:
            *TypeName = _wcsdup(Info.sEnumInfo.Name);
            break;

        case SymTagFunctionType:
            {
                if (Info.sFunctionTypeInfo.MemberFunction && Info.sFunctionTypeInfo.StaticFunction)
                {
                    //*TypeName = L"static ";
                }

                // Print return value 
                if (!SymInfoDump_GetTypeNameHelper(Info.sFunctionTypeInfo.RetTypeIndex, Obj, VarName, TypeName))
                    return FALSE;

                // Print calling convention 
                //TypeName += " ";
                //TypeName += SymInfoDump_CallConvStr(Info.Info.sFunctionTypeInfo.CallConv);
                //TypeName += " ";

                // If member function, save the class type index 
                if (Info.sFunctionTypeInfo.MemberFunction)
                {
                    PhAddItemList(Obj->UdtList, UlongToPtr(Info.sFunctionTypeInfo.ClassIndex));

                    /*
                      // It is not needed to print the class name here, because
                      // it is contained in the function name
                      if( !SymInfoDump_GetTypeNameHelper( Info.Info.sFunctionTypeInfo.ClassIndex, Obj, VarName, TypeName ) )
                          return false;

                      TypeName += "::");
                    */
                }

                // Print that it is a function 
                //TypeName += VarName;
                // Print parameters 
                //TypeName += " (";
                for (ULONG i = 0; i < Info.sFunctionTypeInfo.NumArgs; i++)
                {
                    if (!SymInfoDump_GetTypeNameHelper(Info.sFunctionTypeInfo.Args[i], Obj, VarName, TypeName))
                        return FALSE;

                    //if (i < (Info.sFunctionTypeInfo.NumArgs - 1))
                    //TypeName += ", ";
                }
                //TypeName += ")";

                // Print "this" adjustment value 
                if (Info.sFunctionTypeInfo.MemberFunction && Info.sFunctionTypeInfo.ThisAdjust != 0)
                {
                    WCHAR buffer[MAX_PATH + 1] = L"";

                    //TypeName += "ThisAdjust: ";
                    _snwprintf(buffer, ARRAYSIZE(buffer), L"this+%u", Info.sFunctionTypeInfo.ThisAdjust);
                    //TypeName += buffer;

                    *TypeName = _wcsdup(buffer);
                }
            }
            break;

        case SymTagFunctionArgType:
            {   
                if (!SymInfoDump_GetTypeNameHelper(Info.sFunctionArgTypeInfo.TypeIndex, Obj, VarName, TypeName))
                    return FALSE;
            }
            break;

        case SymTagArrayType:
            {
                // Print element type name 
                if (!SymInfoDump_GetTypeNameHelper(Info.sArrayTypeInfo.ElementTypeIndex, Obj, VarName, TypeName))
                    return FALSE;

                //TypeName += " ";
                //TypeName += VarName;

                // Print dimensions 
                for (ULONG i = 0; i < Info.sArrayTypeInfo.NumDimensions; i++)
                {
                    //WCHAR buffer[MAX_PATH + 1] = L"";
                    //_snwprintf(buffer, cTempBufSize, "[%u]"), Info.Info.sArrayTypeInfo.Dimensions[i]);
                    //TypeName += buffer;
                }
            }
            break;
        default:
            {
                WCHAR buffer[MAX_PATH + 1] = L"";

                _snwprintf(buffer, ARRAYSIZE(buffer), L"Unknown(%lu)", Info.Tag);

                *TypeName = _wcsdup(buffer);
            }
            break;
        }

        // If it is a pointer, display * characters 
        if (numPointers != 0)
        {
            //for (ULONG i = 0; i < NumPointers; i++)
            //    TypeName += "*";
        }
    }

    return TRUE;
}

BOOLEAN SymInfoDump_GetTypeName(
    _Inout_ PPDB_SYMBOL_CONTEXT Context,
    _In_ ULONG Index,
    _In_ PWSTR pVarName,
    _In_ PWSTR pTypeName,
    _In_ ULONG MaxChars
    )
{
    PWSTR VarName = NULL;
    PWSTR TypeName = NULL;

    if (!pTypeName)
        return FALSE;

    if (pVarName)
        VarName = pVarName;

    // Obtain the type name 
    if (!SymInfoDump_GetTypeNameHelper(Index, Context, &VarName, &TypeName))
        return FALSE;

    if (!wcslen(TypeName))
        return FALSE;

    // Return the type name to the caller 
    _snwprintf(pTypeName, MaxChars, L"%s", TypeName);

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Data-to-string conversion functions 
PWSTR SymInfoDump_TagStr(
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

    return L"Unknown";

}

PWSTR SymInfoDump_BaseTypeStr(
    _In_ BasicType Type, 
    _In_ ULONG64 Length
    )
{
    switch (Type)
    {
    case btNoType:
        return L"NoType";
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

    return L"Unknown";

}

PWSTR SymInfoDump_CallConvStr(
    _In_ CV_call_e CallConv
    )
{
    switch (CallConv)
    {
    case CV_CALL_NEAR_C:
        return L"NEAR_C";
    case CV_CALL_FAR_C:
        return L"FAR_C";
    case CV_CALL_NEAR_PASCAL:
        return L"NEAR_PASCAL";
    case CV_CALL_FAR_PASCAL:
        return L"FAR_PASCAL";
    case CV_CALL_NEAR_FAST:
        return L"NEAR_FAST";
    case CV_CALL_FAR_FAST:
        return L"FAR_FAST";
    case CV_CALL_SKIPPED:
        return L"SKIPPED";
    case CV_CALL_NEAR_STD:
        return L"NEAR_STD";
    case CV_CALL_FAR_STD:
        return L"FAR_STD";
    case CV_CALL_NEAR_SYS:
        return L"NEAR_SYS";
    case CV_CALL_FAR_SYS:
        return L"FAR_SYS";
    case CV_CALL_THISCALL:
        return L"THISCALL";
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

PWSTR SymInfoDump_DataKindFromSymbolInfo(
    _In_ PSYMBOL_INFOW SymbolInfo
    )
{
    ULONG dataKindType = 0;

    if (!SymGetTypeInfo_I(NtCurrentProcess(), SymbolInfo->ModBase, SymbolInfo->Index, TI_GET_DATAKIND, &dataKindType))
        return L"UNKNOWN";

    return SymInfoDump_DataKindStr(dataKindType);
}

PWSTR SymInfoDump_DataKindStr(
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

VOID SymInfoDump_SymbolLocationStr(
    _In_ PSYMBOL_INFOW SymbolInfo, 
    _In_ PWSTR Buffer
    )
{
    if (SymbolInfo->Flags & SYMFLAG_REGISTER)
    {
        PWSTR regString = SymInfoDump_RegisterStr((CV_HREG_e)SymbolInfo->Register);

        if (regString)
            wcscpy(Buffer, regString);
        else
            _swprintf(Buffer, L"Reg%u", SymbolInfo->Register);

        return;
    }
    else if (SymbolInfo->Flags & SYMFLAG_REGREL)
    {
        WCHAR szReg[32];
        PWSTR regString = SymInfoDump_RegisterStr((CV_HREG_e)SymbolInfo->Register);

        if (regString)
            wcscpy(szReg, regString);
        else
            _swprintf(szReg, L"Reg%u", SymbolInfo->Register);

        _swprintf(Buffer, L"%s%+lu", szReg, (long)SymbolInfo->Address);

        return;
    }
    else if (SymbolInfo->Flags & SYMFLAG_FRAMEREL)
    {
        wcscpy(Buffer, L"N/A");
        return;
    }
    else
    {
        //_swprintf(Buffer, L"%16I64x", SymbolInfo->Address);
        PhPrintPointer(Buffer, PTR_SUB_OFFSET(SymbolInfo->Address, SymbolInfo->ModBase));
    }
}

PWSTR SymInfoDump_RegisterStr(
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

PWSTR SymInfoDump_UdtKindStr(
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

PWSTR SymInfoDump_LocationTypeStr(
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
    if (!SymbolInfo) // Try to enumerate other symbols 
        return TRUE;

    switch (SymbolInfo->Tag)
    {
    case SymTagFunction:
        PrintFunctionInfo(Context, SymbolInfo);
        break;
    case SymTagData:
        PrintDataInfo(Context, SymbolInfo);
        break;
    default:
        PrintDefaultInfo(Context, SymbolInfo);
        break;
    }

    return TRUE;
}

VOID PrintDataInfo(
    _In_ PPDB_SYMBOL_CONTEXT Context, 
    _In_ PSYMBOL_INFOW SymbolInfo
    )
{
    INT lvItemIndex;
    PWSTR symDataKind;
    WCHAR pointer[PH_PTR_STR_LEN_1] = L"";

    if (SymbolInfo->Tag != SymTagData)
        return;

    // Type
    symDataKind = SymInfoDump_DataKindFromSymbolInfo(SymbolInfo);
    lvItemIndex = PhAddListViewItem(Context->ListviewHandle, MAXINT, symDataKind, NULL);

    // Address  
    SymInfoDump_SymbolLocationStr(SymbolInfo, pointer);
    PhSetListViewSubItem(Context->ListviewHandle, lvItemIndex, 1, pointer);

    // Name
    PhSetListViewSubItem(Context->ListviewHandle, lvItemIndex, 2, SymbolInfo->Name);

    // Size
    PhSetListViewSubItem(Context->ListviewHandle, lvItemIndex, 3, PhaFormatString(L"%lu", SymbolInfo->Size)->Buffer);

    // Data 
    //if (SymInfoDump_GetTypeName(Context, SymbolInfo->TypeIndex, SymbolInfo->Name, szTypeName, cMaxTypeNameLen))
    //    PhSetListViewSubItem(Context->ListviewHandle, lvItemIndex, 4, szTypeName);

    // Flags: %x, SymbolInfo->Flags
    // Index: %8u, TypeIndex: %8u, SymbolInfo->Index, SymbolInfo->TypeIndex
}

VOID PrintFunctionInfo(
    _In_ PPDB_SYMBOL_CONTEXT Context, 
    _In_ PSYMBOL_INFOW SymbolInfo
    )
{
    INT lvItemIndex;
    WCHAR pointer[PH_PTR_STR_LEN_1] = L"";

    if (SymbolInfo->Tag != SymTagFunction)
        return;
    
    // Type
    lvItemIndex = PhAddListViewItem(Context->ListviewHandle, MAXINT, L"FUNCTION", NULL);

    // Address  
    SymInfoDump_SymbolLocationStr(SymbolInfo, pointer);
    PhSetListViewSubItem(Context->ListviewHandle, lvItemIndex, 1, pointer);

    // Name
    PhSetListViewSubItem(Context->ListviewHandle, lvItemIndex, 2, SymbolInfo->Name);

    // Size
    PhSetListViewSubItem(Context->ListviewHandle, lvItemIndex, 3, PhaFormatString(L"%lu", SymbolInfo->Size)->Buffer);

    // Data 
    //if (SymInfoDump_GetTypeName(Context, SymbolInfo->TypeIndex, SymbolInfo->Name, szTypeName, cMaxTypeNameLen))
    //    PhSetListViewSubItem(Context->ListviewHandle, lvItemIndex, 4, szTypeName);

    // Flags: %x, SymbolInfo->Flags
    // Index: %8u, TypeIndex: %8u, SymbolInfo->Index, SymbolInfo->TypeIndex

    // Enumerate function parameters and local variables...
    //IMAGEHLP_STACK_FRAME sf;
    //sf.InstructionOffset = SymbolInfo->Address;
    //SymSetContext(NtCurrentProcess(), &sf, 0);
    //SymEnumSymbolsW(NtCurrentProcess(), 0, NULL, EnumCallbackProc, pTypeInfo);
}

VOID PrintDefaultInfo(
    _In_ PPDB_SYMBOL_CONTEXT Context, 
    _In_ PSYMBOL_INFOW SymbolInfo
    )
{
    INT lvItemIndex;
    WCHAR pointer[PH_PTR_STR_LEN_1] = L"";

    if (SymbolInfo->Tag != SymTagPublicSymbol)
        return;
    
    // Type
    lvItemIndex = PhAddListViewItem(Context->ListviewHandle, MAXINT, L"SYMBOL", NULL);

    // Address  
    SymInfoDump_SymbolLocationStr(SymbolInfo, pointer);
    PhSetListViewSubItem(Context->ListviewHandle, lvItemIndex, 1, pointer);

    // Name
    PhSetListViewSubItem(Context->ListviewHandle, lvItemIndex, 2, SymbolInfo->Name);

    // Size
    PhSetListViewSubItem(Context->ListviewHandle, lvItemIndex, 3, PhaFormatString(L"%lu", SymbolInfo->Size)->Buffer);

    // Data 
    //if (SymInfoDump_GetTypeName(Context, SymbolInfo->TypeIndex, SymbolInfo->Name, szTypeName, cMaxTypeNameLen))
    //    PhSetListViewSubItem(Context->ListviewHandle, lvItemIndex, 4, szTypeName);

    // Flags: %x, SymbolInfo->Flags
    // Index: %8u, TypeIndex: %8u, SymbolInfo->Index, SymbolInfo->TypeIndex
}

VOID PrintClassInfo(
    _In_ PPDB_SYMBOL_CONTEXT Context, 
    _In_ ULONG Index, 
    _In_ UdtClassInfo* Info
    )
{
    // UDT kind 
    //OutputDebugString(PhaFormatString(L"%s", SymInfoDump_UdtKindStr(Info->UDTKind))->Buffer);
    // Name 
    //OutputDebugString(PhaFormatString(L" %s \n", Info->Name)->Buffer);
    // Size 
    //OutputDebugString(PhaFormatString(L"Size: %I64u", Info->Length)->Buffer);

    // Nested 
    //if (Info.Nested)
    //OutputDebugString(L"  Nested");
    // Number of member variables 
    //OutputDebugString(PhaFormatString(L"  Variables: %d", Info->NumVariables)->Buffer);
    // Number of member functions 
    //OutputDebugString(PhaFormatString(L"  Functions: %d", Info->NumFunctions)->Buffer);
    // Number of base classes 
    //OutputDebugString(PhaFormatString(L"  Base classes: %d", Info->NumBaseClasses)->Buffer);
    // Extended information about member variables 
    //OutputDebugString(L"\n");

    for (ULONG i = 0; i < Info->NumVariables; i++)
    {
        TypeInfo VarInfo;

        if (!SymInfoDump_DumpType(Context, Info->Variables[i], &VarInfo))
        {
            //_ASSERTE(!_T("SymInfoDump::DumpType() failed."));
            // Continue with the next variable 
        }
        else if (VarInfo.Tag != SymTagData)
        {
            //_ASSERTE(!_T("Unexpected symbol tag."));
            // Continue with the next variable 
        }
        else
        {
            // Display information about the variable 
            //_tprintf(_T("%s"), Context->DataKindStr(VarInfo.Info.sDataInfo.dataKind));

            // Name 
            //wprintf(L" %s \n", VarInfo.Info.sDataInfo.Name);
            // Type name 
            //_tprintf(_T("  Type:  "));

            //#define cMaxTypeNameLen 1024
            //TCHAR szTypeName[cMaxTypeNameLen + 1] = _T("");

            //if (SymInfoDump_GetTypeName(VarInfo.Info.sDataInfo.TypeIndex, W2T(VarInfo.Info.sDataInfo.Name), szTypeName, cMaxTypeNameLen))
            //_tprintf(szTypeName);
            //else
            //_tprintf(_T("n/a"));
            //_tprintf(_T("\n"));

            // Location 
            switch (VarInfo.sDataInfo.dataKind)
            {
            case DataIsGlobal:
            case DataIsStaticLocal:
            case DataIsFileStatic:
            case DataIsStaticMember:
                {
                    // Use Address 
                    //_tprintf(_T("  Address: %16I64x"), VarInfo.Info.sDataInfo.Address);
                }
                break;
            case DataIsLocal:
            case DataIsParam:
            case DataIsObjectPtr:
            case DataIsMember:
                {
                    // Use Offset 
                    //_tprintf(_T("  Offset: %8d"), (long)VarInfo.Info.sDataInfo.Offset);
                }
                break;
            default:
                break; // <OS-TODO> Add support for constants 
            }

            // Indices 
            //_tprintf(_T("  Index: %8u  TypeIndex: %8u"), Index, VarInfo.Info.sDataInfo.TypeIndex);
            //_tprintf(_T("\n"));
        }
    }

    // Extended information about member functions 
    // <OS-TODO> Implement 
    // Extended information about base classes 
    // <OS-TODO> Implement 

    for (ULONG i = 0; i < Info->NumBaseClasses; i++)
    {
        TypeInfo baseInfo;

        if (!SymInfoDump_DumpType(Context, Info->BaseClasses[i], &baseInfo))
        {
            //_ASSERTE(!_T("SymInfoDump::DumpType() failed."));
            // Continue with the next base class 
        }
        else if (baseInfo.Tag != SymTagBaseClass)
        {
            //_ASSERTE(!_T("Unexpected symbol tag."));
            // Continue with the next base class 
        }
        else
        {
            // Obtain the name of the base class 
            TypeInfo BaseUdtInfo;

            if (!SymInfoDump_DumpType(Context, baseInfo.sBaseClassInfo.TypeIndex, &BaseUdtInfo))
            {
                //_ASSERTE(!_T("SymInfoDump::DumpType() failed."));
                // Continue with the next base class 
            }
            else if (BaseUdtInfo.Tag != SymTagUDT)
            {
                //_ASSERTE(!_T("Unexpected symbol tag."));
                // Continue with the next base class 
            }
            else
            {
                // Print the name of the base class 
                if (baseInfo.sBaseClassInfo.Virtual)
                {
                    //wprintf(L"VIRTUAL_BASE_CLASS %s \n", BaseUdtInfo.Info.sUdtClassInfo.Name);
                }
                else
                {
                    //wprintf(L"BASE_CLASS %s \n", BaseUdtInfo.Info.sUdtClassInfo.Name);
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

        if (SymInfoDump_DumpType(Context, index, &info))
        {
            if (info.Tag == SymTagUDT)
            {
                if (info.UdtKind)
                {
                    // Print information about the class 
                    PrintClassInfo(Context, index, &info.sUdtClassInfo);

                    // Print information about its base classes 
                    for (ULONG i = 0; i < info.sUdtClassInfo.NumBaseClasses; i++)
                    {
                        TypeInfo baseInfo;

                        if (!SymInfoDump_DumpType(Context, info.sUdtClassInfo.BaseClasses[i], &baseInfo))
                        {
                            //_ASSERTE(!_T("SymInfoDump::DumpType() failed."));
                            // Continue with the next base class 
                        }
                        else if (baseInfo.Tag != SymTagBaseClass)
                        {
                            //_ASSERTE(!_T("Unexpected symbol tag."));
                            // Continue with the next base class 
                        }
                        else
                        {
                            // Obtain information about the base class 
                            TypeInfo baseUdtInfo;

                            if (!SymInfoDump_DumpType(Context, baseInfo.sBaseClassInfo.TypeIndex, &baseUdtInfo))
                            {
                                //_ASSERTE(!_T("SymInfoDump::DumpType() failed."));
                                // Continue with the next base class 
                            }
                            else if (baseUdtInfo.Tag != SymTagUDT)
                            {
                                //_ASSERTE(!_T("Unexpected symbol tag."));
                                // Continue with the next base class 
                            }
                            else
                            {
                                // Print information about the base class 
                                PrintClassInfo(Context, baseInfo.sBaseClassInfo.TypeIndex, &baseUdtInfo.sUdtClassInfo);
                            }
                        }
                    }
                }
                else
                {
                    // UDT kind 
                    //printf("%s", Context->UdtKindStr(Info.Info.sUdtClassInfo.UDTKind));
                    //
                    // Name 
                    //printf("%s", Info.Info.sUdtClassInfo.Name);
                    //
                    // Size 
                    //printf("Size: %I64u", Info.Info.sUdtClassInfo.Length);
                    //
                    // Nested 
                    //if (Info.Info.sUdtClassInfo.Nested)
                    //    printf("Nested");
                    //
                    // Number of members  
                    //printf("  Members: %d", Info.Info.sUdtClassInfo.NumVariables);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
BOOLEAN GetFileParamsSize(_In_ PWSTR pFileName, _Out_ ULONG* FileLength)
{
    HANDLE hFile = CreateFile(
        pFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    *FileLength = GetFileSize(hFile, NULL);
    NtClose(hFile);

    return (*FileLength != INVALID_FILE_SIZE);
}

BOOLEAN GetFileParams(_In_ PWSTR pFileName, _Out_ ULONG64 *BaseAddress, _Out_ ULONG* FileLength)
{
    TCHAR szFileExt[_MAX_EXT] = { 0 };

    _wsplitpath(pFileName, NULL, NULL, NULL, szFileExt);

    // Is it .PDB file ? 
    if (_wcsicmp(szFileExt, L".PDB") == 0)
    {
        // Yes, it is a .PDB file         
        *BaseAddress = 0x10000000;

        // Determine its size, and use a dummy base address.
        // it can be any non-zero value, but if we load symbols                      
        // from more than one file, memory regions specified      
        // for different files should not overlap               
        // (region is "base address + file size") 

        if (!GetFileParamsSize(pFileName, FileLength))
        {
            return FALSE;
        }
    }
    else
    {
        *BaseAddress = 0;
        *FileLength = 0;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
VOID PdbLoadDbgHelpFromPath(
    _In_ PWSTR DbgHelpPath
    )
{
    HMODULE dbghelpModule;

    if (dbghelpModule = LoadLibrary(DbgHelpPath))
    {
        PPH_STRING fullDbghelpPath;
        ULONG indexOfFileName;
        PH_STRINGREF dbghelpFolder;
        PPH_STRING symsrvPath;

        fullDbghelpPath = PhGetDllFileName(dbghelpModule, &indexOfFileName);

        if (fullDbghelpPath)
        {
            if (indexOfFileName != 0)
            {
                static PH_STRINGREF symsrvString = PH_STRINGREF_INIT(L"\\symsrv.dll");

                dbghelpFolder.Buffer = fullDbghelpPath->Buffer;
                dbghelpFolder.Length = indexOfFileName * sizeof(WCHAR);

                symsrvPath = PhConcatStringRef2(&dbghelpFolder, &symsrvString);

                LoadLibrary(symsrvPath->Buffer);

                PhDereferenceObject(symsrvPath);
            }

            PhDereferenceObject(fullDbghelpPath);
        }
    }
    else
    {
        dbghelpModule = LoadLibrary(L"dbghelp.dll");
    }

    PhSymbolProviderCompleteInitialization(dbghelpModule);
}


VOID ShowSymbolInfo(
    _In_ ULONG64 BaseAddress
    )
{
    IMAGEHLP_MODULE64 info;

    memset(&info, 0, sizeof(IMAGEHLP_MODULE64));
    info.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

    if (!SymGetModuleInfoW64_I(NtCurrentProcess(), BaseAddress, &info))
        return;

    switch (info.SymType)
    {
    case SymNone:
        //OutputDebugString(L"No symbols available for the module.\r\n");
        break;
    case SymExport:
        //OutputDebugString(L"Loaded symbols: Exports\r\n");
        break;
    case SymCoff:
        //OutputDebugString(L"Loaded symbols: COFF\r\n");
        break;
    case SymCv:
        //OutputDebugString(L"Loaded symbols: CodeView\r\n");
        break;
    case SymSym:
        //OutputDebugString(L"Loaded symbols: SYM\r\n");
        break;
    case SymVirtual:
        //OutputDebugString(L"Loaded symbols: Virtual\r\n");
        break;
    case SymPdb:
        //OutputDebugString(L"Loaded symbols: PDB\r\n");
        break;
    case SymDia:
        //OutputDebugString(L"Loaded symbols: DIA\r\n");
        break;
    case SymDeferred:
        //OutputDebugString(L"Loaded symbols: Deferred\r\n"); // not actually loaded 
        break;
    default:
        //OutputDebugString(L"Loaded symbols: Unknown format.\r\n");
        break;
    }

    // Image name 
    if (wcslen(info.ImageName) > 0)
    {
        //OutputDebugString(PhaFormatString(L"Image name: %s\n", info.ImageName)->Buffer);
    }

    // Loaded image name 
    if (wcslen(info.LoadedImageName) > 0)
    {
        //OutputDebugString(PhaFormatString(L"Loaded image name: %s\n", info.LoadedImageName)->Buffer);
    }

    // Loaded PDB name 
    if (wcslen(info.LoadedPdbName) > 0)
    {
        //OutputDebugString(PhaFormatString(L"PDB file name: %s\n", info.LoadedPdbName)->Buffer);
    }

    // Is debug information unmatched ? 
    // (It can only happen if the debug information is contained in a separate file (.DBG or .PDB) 
    if (info.PdbUnmatched || info.DbgUnmatched)
    {
        OutputDebugString(L"Warning: Unmatched symbols. \n");
    }

    // Load address: BaseAddress
    //OutputDebugString(PhaFormatString(L"Line numbers: %s\n", info.LineNumbers ? L"Available" : L"Not available")->Buffer);
    //OutputDebugString(PhaFormatString(L"Global symbols: %s\n", info.GlobalSymbols ? L"Available" : L"Not available")->Buffer);
    //OutputDebugString(PhaFormatString(L"Type information: %s\n", info.TypeInfo ? L"Available" : L"Not available")->Buffer);
    //OutputDebugString(PhaFormatString(L"Source indexing: %s\n", info.SourceIndexed ? L"Yes" : L"No")->Buffer);
    //OutputDebugString(PhaFormatString(L"Public symbols: %s\n", info.Publics ? L"Available" : L"Not available")->Buffer);
}


VOID PeDumpFileSymbols(
    _In_ HWND ListViewHandle, 
    _In_ PWSTR FileName
    )
{
    HMODULE dbghelpHandle;
    HMODULE symsrvHandle;
    ULONG64 symbolBaseAddress = 0;
    ULONG64 baseAddress = 0;
    ULONG fileLength = 0;

    PdbLoadDbgHelpFromPath(L"C:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x64\\dbghelp.dll");
      
    if (!(dbghelpHandle = GetModuleHandle(L"dbghelp.dll")))
        return;

    symsrvHandle = GetModuleHandle(L"symsrv.dll");
    SymInitialize_I = PhGetProcedureAddress(dbghelpHandle, "SymInitialize", 0);
    SymCleanup_I = PhGetProcedureAddress(dbghelpHandle, "SymCleanup", 0);
    SymEnumSymbolsW_I = PhGetProcedureAddress(dbghelpHandle, "SymEnumSymbolsW", 0);
    SymSetSearchPathW_I = PhGetProcedureAddress(dbghelpHandle, "SymSetSearchPathW", 0);
    SymGetOptions_I = PhGetProcedureAddress(dbghelpHandle, "SymGetOptions", 0);
    SymSetOptions_I = PhGetProcedureAddress(dbghelpHandle, "SymSetOptions", 0);
    SymLoadModuleExW_I = PhGetProcedureAddress(dbghelpHandle, "SymLoadModuleExW", 0);
    SymGetModuleInfoW64_I = PhGetProcedureAddress(dbghelpHandle, "SymGetModuleInfoW64", 0);
    SymGetTypeInfo_I = PhGetProcedureAddress(dbghelpHandle, "SymGetTypeInfo", 0);

    SymSetOptions_I(SymGetOptions_I() | SYMOPT_DEBUG | SYMOPT_UNDNAME); // SYMOPT_DEFERRED_LOADS | SYMOPT_FAVOR_COMPRESSED

    if (!SymInitialize_I(NtCurrentProcess(), NULL, FALSE))
        return;

    if (!SymSetSearchPathW_I(NtCurrentProcess(), L"SRV*C:\\symbols*http://msdl.microsoft.com/download/symbols"))
        return;

    if (GetFileParams(FileName, &baseAddress, &fileLength))
    {
        if ((symbolBaseAddress = SymLoadModuleExW_I(
            NtCurrentProcess(), 
            NULL, 
            FileName, 
            NULL, 
            baseAddress, 
            fileLength, 
            NULL, 
            0
            )))
        {
            PDB_SYMBOL_CONTEXT context;

            context.ListviewHandle = ListViewHandle;
            context.BaseAddress = symbolBaseAddress;
            context.UdtList = PhCreateList(0x100);

            ShowSymbolInfo(symbolBaseAddress);

            SymEnumSymbolsW_I(NtCurrentProcess(), symbolBaseAddress, NULL, EnumCallbackProc, &context);

            // Print information about used defined types 
            //PrintUserDefinedTypes(&context);
        }
    }

    SymCleanup_I(NtCurrentProcess());
}

VOID PvPdbProperties(
    VOID
    )
{
    PPV_PROPCONTEXT propContext;

    if (!RtlDoesFileExists_U(PvFileName->Buffer))
    {
        PhShowStatus(NULL, L"Unable to load the pdb file", STATUS_FILE_NOT_AVAILABLE, 0);
        return;
    }

    if (propContext = PvCreatePropContext(PvFileName))
    {
        PPV_PROPPAGECONTEXT newPage;

        // Symbols page
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_PESYMBOLS),
            PvpSymbolsDlgProc,
            NULL
            );
        PvAddPropPage(propContext, newPage);

        PhModalPropertySheet(&propContext->PropSheetHeader);

        PhDereferenceObject(propContext);
    }
}

INT_PTR CALLBACK PvpSymbolsDlgProc(
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

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"Type");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_RIGHT, 80, L"VA");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 250, L"Name");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 40, L"Size");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"PdbListViewColumns", lvHandle);

            PeDumpFileSymbols(lvHandle, PvFileName->Buffer);

            ExtendedListView_SortItems(lvHandle);

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"PdbListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST),
                    dialogItem, PH_ANCHOR_ALL);

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
    }

    return FALSE;
}
