/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 */

/*
 * Self-contained parser for ECMA-335 CLI metadata tables (and the Portable PDB
 * table extensions), modeled on PH_MAPPED_IMAGE. Unlike the runtime-backed
 * IMetaDataTables approach, this reads the #~/#- table stream and the #Strings,
 * #US, #Blob and #GUID heaps directly from the mapped bytes, with every access
 * bounds-checked against the mapped view. References:
 *   ECMA-335 II.22 (table schemas) and II.24.2 (physical layout)
 *   https://github.com/dotnet/coreclr/blob/master/src/md/inc/metamodel.h
 */

#include <ph.h>
#include <mapimg.h>
#include <mapclr.h>

/**
 * Determines if the PM_NO_MANGLE flag is set.
 *
 * \param Flags The PInvoke import flags.
 * \return TRUE if the flag is set, FALSE otherwise.
 */
BOOLEAN PhPmNoMangle(
    _In_ ULONG Flags
    )
{
    return (Flags & PM_NO_MANGLE) != 0;
}

/**
 * Determines if the ANSI charset flag is set.
 *
 * \param Flags The PInvoke import flags.
 * \return TRUE if the flag is set, FALSE otherwise.
 */
BOOLEAN PhPmCharSetAnsi(
    _In_ ULONG Flags
    )
{
    return (Flags & PM_CHARSET_MASK) == PM_CHARSET_ANSI;
}

/**
 * Determines if the Unicode charset flag is set.
 *
 * \param Flags The PInvoke import flags.
 * \return TRUE if the flag is set, FALSE otherwise.
 */
BOOLEAN PhPmCharSetUnicode(
    _In_ ULONG Flags
    )
{
    return (Flags & PM_CHARSET_MASK) == PM_CHARSET_UNICODE;
}

/**
 * Determines if the Auto charset flag is set.
 *
 * \param Flags The PInvoke import flags.
 * \return TRUE if the flag is set, FALSE otherwise.
 */
BOOLEAN PhPmCharSetAuto(
    _In_ ULONG Flags
    )
{
    return (Flags & PM_CHARSET_MASK) == PM_CHARSET_AUTO;
}

/**
 * Determines if the PM_SUPPORTS_LAST_ERROR flag is set.
 *
 * \param Flags The PInvoke import flags.
 * \return TRUE if the flag is set, FALSE otherwise.
 */
BOOLEAN PhPmSupportsLastError(
    _In_ ULONG Flags
    )
{
    return (Flags & PM_SUPPORTS_LAST_ERROR) != 0;
}

/**
 * Determines if the calling convention is Winapi.
 *
 * \param Flags The PInvoke import flags.
 * \return TRUE if the calling convention is Winapi, FALSE otherwise.
 */
BOOLEAN PhPmCallConvWinapi(
    _In_ ULONG Flags
    )
{
    return (Flags & PM_CALLCONV_MASK) == PM_CALLCONV_WINAPI;
}

/**
 * Determines if the calling convention is Cdecl.
 *
 * \param Flags The PInvoke import flags.
 * \return TRUE if the calling convention is Cdecl, FALSE otherwise.
 */
BOOLEAN PhPmCallConvCdecl(
    _In_ ULONG Flags
    )
{
    return (Flags & PM_CALLCONV_MASK) == PM_CALLCONV_CDECL;
}

/**
 * Determines if the calling convention is Stdcall.
 *
 * \param Flags The PInvoke import flags.
 * \return TRUE if the calling convention is Stdcall, FALSE otherwise.
 */
BOOLEAN PhPmCallConvStdcall(
    _In_ ULONG Flags
    )
{
    return (Flags & PM_CALLCONV_MASK) == PM_CALLCONV_STDCALL;
}

/**
 * Determines if the calling convention is Thiscall.
 *
 * \param Flags The PInvoke import flags.
 * \return TRUE if the calling convention is Thiscall, FALSE otherwise.
 */
BOOLEAN PhPmCallConvThiscall(
    _In_ ULONG Flags
    )
{
    return (Flags & PM_CALLCONV_MASK) == PM_CALLCONV_THISCALL;
}

/**
 * Determines if the calling convention is Fastcall.
 *
 * \param Flags The PInvoke import flags.
 * \return TRUE if the calling convention is Fastcall, FALSE otherwise.
 */
BOOLEAN PhPmCallConvFastcall(
    _In_ ULONG Flags
    )
{
    return (Flags & PM_CALLCONV_MASK) == PM_CALLCONV_FASTCALL;
}

/**
 * Determines if the BestFitEnabled flag is set.
 *
 * \param Flags The PInvoke import flags.
 * \return TRUE if the flag is set, FALSE otherwise.
 */
BOOLEAN PhPmBestFitEnabled(
    _In_ ULONG Flags
    )
{
    return (Flags & PM_BEST_FIT_MASK) == PM_BEST_FIT_ENABLED;
}

/**
 * Determines if the BestFitDisabled flag is set.
 *
 * \param Flags The PInvoke import flags.
 * \return TRUE if the flag is set, FALSE otherwise.
 */
BOOLEAN PhPmBestFitDisabled(
    _In_ ULONG Flags
    )
{
    return (Flags & PM_BEST_FIT_MASK) == PM_BEST_FIT_DISABLED;
}

/**
 * Determines if the ThrowOnUnmappableCharEnabled flag is set.
 *
 * \param Flags The PInvoke import flags.
 * \return TRUE if the flag is set, FALSE otherwise.
 */
BOOLEAN PhPmThrowOnUnmappableCharEnabled(
    _In_ ULONG Flags
    )
{
    return (Flags & PM_THROW_ON_UNMAPPABLE_CHAR_MASK) == PM_THROW_ON_UNMAPPABLE_CHAR_ENABLED;
}

/**
 * Determines if the ThrowOnUnmappableCharDisabled flag is set.
 *
 * \param Flags The PInvoke import flags.
 * \return TRUE if the flag is set, FALSE otherwise.
 */
BOOLEAN PhPmThrowOnUnmappableCharDisabled(
    _In_ ULONG Flags
    )
{
    return (Flags & PM_THROW_ON_UNMAPPABLE_CHAR_MASK) == PM_THROW_ON_UNMAPPABLE_CHAR_DISABLED;
}

static const PH_CLR_CODED_INDEX_INFO PhClrCodedIndex[CIX_MAXIMUM] =
{
    [CIX_TYPEDEFORREF] = { 2, 3, { PH_CLR_TABLE_TYPEDEF, PH_CLR_TABLE_TYPEREF, PH_CLR_TABLE_TYPESPEC } },
    [CIX_HASCONSTANT] = { 2, 3, { PH_CLR_TABLE_FIELD, PH_CLR_TABLE_PARAM, PH_CLR_TABLE_PROPERTY } },
    [CIX_HASCUSTOMATTRIBUTE] =
    { 5, 22, {
        PH_CLR_TABLE_METHODDEF, PH_CLR_TABLE_FIELD, PH_CLR_TABLE_TYPEREF, PH_CLR_TABLE_TYPEDEF,
        PH_CLR_TABLE_PARAM, PH_CLR_TABLE_INTERFACEIMPL, PH_CLR_TABLE_MEMBERREF, PH_CLR_TABLE_MODULE,
        PH_CLR_TABLE_DECLSECURITY, PH_CLR_TABLE_PROPERTY, PH_CLR_TABLE_EVENT, PH_CLR_TABLE_STANDALONESIG,
        PH_CLR_TABLE_MODULEREF, PH_CLR_TABLE_TYPESPEC, PH_CLR_TABLE_ASSEMBLY, PH_CLR_TABLE_ASSEMBLYREF,
        PH_CLR_TABLE_FILE, PH_CLR_TABLE_EXPORTEDTYPE, PH_CLR_TABLE_MANIFESTRESOURCE, PH_CLR_TABLE_GENERICPARAM,
        PH_CLR_TABLE_GENERICPARAMCONSTRAINT, PH_CLR_TABLE_METHODSPEC
    } },
    [CIX_HASFIELDMARSHALL] = { 1, 2, { PH_CLR_TABLE_FIELD, PH_CLR_TABLE_PARAM } },
    [CIX_HASDECLSECURITY] = { 2, 3, { PH_CLR_TABLE_TYPEDEF, PH_CLR_TABLE_METHODDEF, PH_CLR_TABLE_ASSEMBLY } },
    [CIX_MEMBERREFPARENT] = { 3, 5, { PH_CLR_TABLE_TYPEDEF, PH_CLR_TABLE_TYPEREF, PH_CLR_TABLE_MODULEREF, PH_CLR_TABLE_METHODDEF,  PH_CLR_TABLE_TYPESPEC } },
    [CIX_HASSEMANTICS] = { 1, 2, { PH_CLR_TABLE_EVENT, PH_CLR_TABLE_PROPERTY } },
    [CIX_METHODDEFORREF] = { 1, 2, { PH_CLR_TABLE_METHODDEF, PH_CLR_TABLE_MEMBERREF } },
    [CIX_MEMBERFORWARDED] = { 1, 2, { PH_CLR_TABLE_FIELD, PH_CLR_TABLE_METHODDEF } },
    [CIX_IMPLEMENTATION] = { 2, 3, { PH_CLR_TABLE_FILE, PH_CLR_TABLE_ASSEMBLYREF, PH_CLR_TABLE_EXPORTEDTYPE } },
    [CIX_CUSTOMATTRIBUTETYPE] = { 3, 5, { CIX_RESERVED, CIX_RESERVED, PH_CLR_TABLE_METHODDEF, PH_CLR_TABLE_MEMBERREF, CIX_RESERVED } },
    [CIX_RESOLUTIONSCOPE] = { 2, 4, { PH_CLR_TABLE_MODULE, PH_CLR_TABLE_MODULEREF, PH_CLR_TABLE_ASSEMBLYREF, PH_CLR_TABLE_TYPEREF } },
    [CIX_TYPEORMETHODDEF] = { 1, 2, { PH_CLR_TABLE_TYPEDEF, PH_CLR_TABLE_METHODDEF } },
    [CIX_HASCUSTOMDEBUGINFORMATION] =
    { 5, 27, {
        PH_CLR_TABLE_METHODDEF, PH_CLR_TABLE_FIELD, PH_CLR_TABLE_TYPEREF, PH_CLR_TABLE_TYPEDEF,
        PH_CLR_TABLE_PARAM, PH_CLR_TABLE_INTERFACEIMPL, PH_CLR_TABLE_MEMBERREF, PH_CLR_TABLE_MODULE,
        PH_CLR_TABLE_DECLSECURITY, PH_CLR_TABLE_PROPERTY, PH_CLR_TABLE_EVENT, PH_CLR_TABLE_STANDALONESIG,
        PH_CLR_TABLE_MODULEREF, PH_CLR_TABLE_TYPESPEC, PH_CLR_TABLE_ASSEMBLY, PH_CLR_TABLE_ASSEMBLYREF,
        PH_CLR_TABLE_FILE, PH_CLR_TABLE_EXPORTEDTYPE, PH_CLR_TABLE_MANIFESTRESOURCE, PH_CLR_TABLE_GENERICPARAM,
        PH_CLR_TABLE_GENERICPARAMCONSTRAINT, PH_CLR_TABLE_METHODSPEC, PH_CLR_TABLE_DOCUMENT,
        PH_CLR_TABLE_LOCALSCOPE, PH_CLR_TABLE_LOCALVARIABLE, PH_CLR_TABLE_LOCALCONSTANT,
        PH_CLR_TABLE_IMPORTSCOPE
    } },
};

static const PH_CLR_TABLE_SCHEMA PhClrTableSchema[PH_CLR_TABLE_MAXIMUM] =
{
    [PH_CLR_TABLE_MODULE] = { "Module", { CC_I2, CC_STR, CC_GUID, CC_GUID, CC_GUID, CC_END } },
    [PH_CLR_TABLE_TYPEREF] = { "TypeRef", { PH_CLR_SCHEMA_CODED(RESOLUTIONSCOPE), CC_STR, CC_STR, CC_END } },
    [PH_CLR_TABLE_TYPEDEF] = { "TypeDef", { CC_I4, CC_STR, CC_STR, PH_CLR_SCHEMA_CODED(TYPEDEFORREF), PH_CLR_SCHEMA_TABLE(FIELD), PH_CLR_SCHEMA_TABLE(METHODDEF), CC_END } },
    [PH_CLR_TABLE_FIELDPTR] = { "FieldPtr", { PH_CLR_SCHEMA_TABLE(FIELD), CC_END } },
    [PH_CLR_TABLE_FIELD] = { "Field", { CC_I2, CC_STR, CC_BLOB, CC_END } },
    [PH_CLR_TABLE_METHODPTR] = { "MethodPtr", { PH_CLR_SCHEMA_TABLE(METHODDEF), CC_END } },
    [PH_CLR_TABLE_METHODDEF] = { "MethodDef", { CC_I4, CC_I2, CC_I2, CC_STR, CC_BLOB, PH_CLR_SCHEMA_TABLE(PARAM), CC_END } },
    [PH_CLR_TABLE_PARAMPTR] = { "ParamPtr", { PH_CLR_SCHEMA_TABLE(PARAM), CC_END } },
    [PH_CLR_TABLE_PARAM] = { "Param", { CC_I2, CC_I2, CC_STR, CC_END } },
    [PH_CLR_TABLE_INTERFACEIMPL] = { "InterfaceImpl", { PH_CLR_SCHEMA_TABLE(TYPEDEF), PH_CLR_SCHEMA_CODED(TYPEDEFORREF), CC_END } },
    [PH_CLR_TABLE_MEMBERREF] = { "MemberRef", { PH_CLR_SCHEMA_CODED(MEMBERREFPARENT), CC_STR, CC_BLOB, CC_END } },
    [PH_CLR_TABLE_CONSTANT] = { "Constant", { CC_I2, PH_CLR_SCHEMA_CODED(HASCONSTANT), CC_BLOB, CC_END } },
    [PH_CLR_TABLE_CUSTOMATTRIBUTE] = { "CustomAttribute", { PH_CLR_SCHEMA_CODED(HASCUSTOMATTRIBUTE), PH_CLR_SCHEMA_CODED(CUSTOMATTRIBUTETYPE), CC_BLOB, CC_END } },
    [PH_CLR_TABLE_FIELDMARSHAL] = { "FieldMarshal", { PH_CLR_SCHEMA_CODED(HASFIELDMARSHALL), CC_BLOB, CC_END } },
    [PH_CLR_TABLE_DECLSECURITY] = { "DeclSecurity", { CC_I2, PH_CLR_SCHEMA_CODED(HASDECLSECURITY), CC_BLOB, CC_END } },
    [PH_CLR_TABLE_CLASSLAYOUT] = { "ClassLayout", { CC_I2, CC_I4, PH_CLR_SCHEMA_TABLE(TYPEDEF), CC_END } },
    [PH_CLR_TABLE_FIELDLAYOUT] = { "FieldLayout", { CC_I4, PH_CLR_SCHEMA_TABLE(FIELD), CC_END } },
    [PH_CLR_TABLE_STANDALONESIG] = { "StandAloneSig", { CC_BLOB, CC_END } },
    [PH_CLR_TABLE_EVENTMAP] = { "EventMap", { PH_CLR_SCHEMA_TABLE(TYPEDEF), PH_CLR_SCHEMA_TABLE(EVENT), CC_END } },
    [PH_CLR_TABLE_EVENTPTR] = { "EventPtr", { PH_CLR_SCHEMA_TABLE(EVENT), CC_END } },
    [PH_CLR_TABLE_EVENT] = { "Event", { CC_I2, CC_STR, PH_CLR_SCHEMA_CODED(TYPEDEFORREF), CC_END } },
    [PH_CLR_TABLE_PROPERTYMAP] = { "PropertyMap", { PH_CLR_SCHEMA_TABLE(TYPEDEF), PH_CLR_SCHEMA_TABLE(PROPERTY), CC_END } },
    [PH_CLR_TABLE_PROPERTYPTR] = { "PropertyPtr", { PH_CLR_SCHEMA_TABLE(PROPERTY), CC_END } },
    [PH_CLR_TABLE_PROPERTY] = { "Property", { CC_I2, CC_STR, CC_BLOB, CC_END } },
    [PH_CLR_TABLE_METHODSEMANTICS] = { "MethodSemantics", { CC_I2, PH_CLR_SCHEMA_TABLE(METHODDEF), PH_CLR_SCHEMA_CODED(HASSEMANTICS), CC_END } },
    [PH_CLR_TABLE_METHODIMPL] = { "MethodImpl", { PH_CLR_SCHEMA_TABLE(TYPEDEF), PH_CLR_SCHEMA_CODED(METHODDEFORREF), PH_CLR_SCHEMA_CODED(METHODDEFORREF), CC_END } },
    [PH_CLR_TABLE_MODULEREF] = { "ModuleRef", { CC_STR, CC_END } },
    [PH_CLR_TABLE_TYPESPEC] = { "TypeSpec", { CC_BLOB, CC_END } },
    [PH_CLR_TABLE_IMPLMAP] = { "ImplMap", { CC_I2, PH_CLR_SCHEMA_CODED(MEMBERFORWARDED), CC_STR, PH_CLR_SCHEMA_TABLE(MODULEREF), CC_END } },
    [PH_CLR_TABLE_FIELDRVA] = { "FieldRVA", { CC_I4, PH_CLR_SCHEMA_TABLE(FIELD), CC_END } },
    [PH_CLR_TABLE_ENCLOG] = { "ENCLog", { CC_I4, CC_I4, CC_END } },
    [PH_CLR_TABLE_ENCMAP] = { "ENCMap", { CC_I4, CC_END } },
    [PH_CLR_TABLE_ASSEMBLY] = { "Assembly", { CC_I4, CC_I2, CC_I2, CC_I2, CC_I2, CC_I4, CC_BLOB, CC_STR, CC_STR } },
    [PH_CLR_TABLE_ASSEMBLYPROCESSOR] = { "AssemblyProcessor", { CC_I4, CC_END } },
    [PH_CLR_TABLE_ASSEMBLYOS] = { "AssemblyOS", { CC_I4, CC_I4, CC_I4, CC_END } },
    [PH_CLR_TABLE_ASSEMBLYREF] = { "AssemblyRef", { CC_I2, CC_I2, CC_I2, CC_I2, CC_I4, CC_BLOB, CC_STR, CC_STR, CC_BLOB } },
    [PH_CLR_TABLE_ASSEMBLYREFPROCESSOR] = { "AssemblyRefProcessor", { CC_I4, PH_CLR_SCHEMA_TABLE(ASSEMBLYREF), CC_END } },
    [PH_CLR_TABLE_ASSEMBLYREFOS] = { "AssemblyRefOS", { CC_I4, CC_I4, CC_I4, PH_CLR_SCHEMA_TABLE(ASSEMBLYREF), CC_END } },
    [PH_CLR_TABLE_FILE] = { "File", { CC_I4, CC_STR, CC_BLOB, CC_END } },
    [PH_CLR_TABLE_EXPORTEDTYPE] = { "ExportedType", { CC_I4, CC_I4, CC_STR, CC_STR, PH_CLR_SCHEMA_CODED(IMPLEMENTATION), CC_END } },
    [PH_CLR_TABLE_MANIFESTRESOURCE] = { "ManifestResource", { CC_I4, CC_I4, CC_STR, PH_CLR_SCHEMA_CODED(IMPLEMENTATION), CC_END } },
    [PH_CLR_TABLE_NESTEDCLASS] = { "NestedClass", { PH_CLR_SCHEMA_TABLE(TYPEDEF), PH_CLR_SCHEMA_TABLE(TYPEDEF), CC_END } },
    [PH_CLR_TABLE_GENERICPARAM] = { "GenericParam", { CC_I2, CC_I2, PH_CLR_SCHEMA_CODED(TYPEORMETHODDEF), CC_STR, CC_END } },
    [PH_CLR_TABLE_METHODSPEC] = { "MethodSpec", { PH_CLR_SCHEMA_CODED(METHODDEFORREF), CC_BLOB, CC_END } },
    [PH_CLR_TABLE_GENERICPARAMCONSTRAINT] = { "GenericParamConstraint", { PH_CLR_SCHEMA_TABLE(GENERICPARAM), PH_CLR_SCHEMA_CODED(TYPEDEFORREF), CC_END } },
    // Portable PDB
    [PH_CLR_TABLE_DOCUMENT] = { "Document", { CC_BLOB, CC_GUID, CC_BLOB, CC_GUID, CC_END } },
    [PH_CLR_TABLE_METHODDEBUGINFORMATION] = { "MethodDebugInformation", { PH_CLR_SCHEMA_TABLE(DOCUMENT), CC_BLOB, CC_END } },
    [PH_CLR_TABLE_LOCALSCOPE] = { "LocalScope", { PH_CLR_SCHEMA_TABLE(METHODDEF), PH_CLR_SCHEMA_TABLE(IMPORTSCOPE), PH_CLR_SCHEMA_TABLE(LOCALVARIABLE), PH_CLR_SCHEMA_TABLE(LOCALCONSTANT), CC_I4, CC_I4, CC_END } },
    [PH_CLR_TABLE_LOCALVARIABLE] = { "LocalVariable", { CC_I2, CC_I2, CC_STR, CC_END } },
    [PH_CLR_TABLE_LOCALCONSTANT] = { "LocalConstant", { CC_STR, CC_BLOB, CC_END } },
    [PH_CLR_TABLE_IMPORTSCOPE] = { "ImportScope", { PH_CLR_SCHEMA_TABLE(IMPORTSCOPE), CC_BLOB, CC_END } },
    [PH_CLR_TABLE_STATEMACHINEMETHOD] = { "StateMachineMethod", { PH_CLR_SCHEMA_TABLE(METHODDEF), PH_CLR_SCHEMA_TABLE(METHODDEF), CC_END } },
    [PH_CLR_TABLE_CUSTOMDEBUGINFORMATION] = { "CustomDebugInformation", { PH_CLR_SCHEMA_CODED(HASCUSTOMDEBUGINFORMATION), CC_GUID, CC_BLOB, CC_END } },
};


// Column name mappings for each table (based on ECMA-335 schema)
const PCWSTR PhClrTableColumnNames[PH_CLR_TABLE_MAXIMUM][PH_CLR_MAX_COLUMNS] =
{
    [PH_CLR_TABLE_MODULE] = { L"Generation", L"Name", L"MVID", L"EncId", L"EncBaseId" },
    [PH_CLR_TABLE_TYPEREF] = { L"ResolutionScope", L"TypeName", L"TypeNamespace" },
    [PH_CLR_TABLE_TYPEDEF] = { L"Flags", L"TypeName", L"TypeNamespace", L"Extends", L"FieldList", L"MethodList" },
    [PH_CLR_TABLE_FIELDPTR] = { L"Field" },
    [PH_CLR_TABLE_FIELD] = { L"Flags", L"Name", L"Signature" },
    [PH_CLR_TABLE_METHODPTR] = { L"Method" },
    [PH_CLR_TABLE_METHODDEF] = { L"RVA", L"ImplFlags", L"Flags", L"Name", L"Signature", L"ParamList" },
    [PH_CLR_TABLE_PARAMPTR] = { L"Param" },
    [PH_CLR_TABLE_PARAM] = { L"Flags", L"Sequence", L"Name" },
    [PH_CLR_TABLE_INTERFACEIMPL] = { L"Class", L"Interface" },
    [PH_CLR_TABLE_MEMBERREF] = { L"Class", L"Name", L"Signature" },
    [PH_CLR_TABLE_CONSTANT] = { L"Type", L"Parent", L"Value" },
    [PH_CLR_TABLE_CUSTOMATTRIBUTE] = { L"Parent", L"Type", L"Value" },
    [PH_CLR_TABLE_FIELDMARSHAL] = { L"Parent", L"NativeType" },
    [PH_CLR_TABLE_DECLSECURITY] = { L"Action", L"Parent", L"PermissionSet" },
    [PH_CLR_TABLE_CLASSLAYOUT] = { L"PackingSize", L"ClassSize", L"Parent" },
    [PH_CLR_TABLE_FIELDLAYOUT] = { L"Offset", L"Field" },
    [PH_CLR_TABLE_STANDALONESIG] = { L"Signature" },
    [PH_CLR_TABLE_EVENTMAP] = { L"Parent", L"EventList" },
    [PH_CLR_TABLE_EVENTPTR] = { L"Event" },
    [PH_CLR_TABLE_EVENT] = { L"Flags", L"Name", L"EventType" },
    [PH_CLR_TABLE_PROPERTYMAP] = { L"Parent", L"PropertyList" },
    [PH_CLR_TABLE_PROPERTYPTR] = { L"Property" },
    [PH_CLR_TABLE_PROPERTY] = { L"Flags", L"Name", L"Type" },
    [PH_CLR_TABLE_METHODSEMANTICS] = { L"Semantics", L"Method", L"Association" },
    [PH_CLR_TABLE_METHODIMPL] = { L"Class", L"MethodBody", L"MethodDeclaration" },
    [PH_CLR_TABLE_MODULEREF] = { L"Name" },
    [PH_CLR_TABLE_TYPESPEC] = { L"Signature" },
    [PH_CLR_TABLE_IMPLMAP] = { L"MappingFlags", L"MemberForwarded", L"ImportName", L"ImportScope" },
    [PH_CLR_TABLE_FIELDRVA] = { L"RVA", L"Field" },
    [PH_CLR_TABLE_ENCLOG] = { L"AdvanceMethod", L"Operation" },
    [PH_CLR_TABLE_ENCMAP] = { L"PToken" },
    [PH_CLR_TABLE_ASSEMBLY] = { L"HashAlgId", L"MajorVersion", L"MinorVersion", L"BuildNumber", L"RevisionNumber", L"Flags", L"PublicKey", L"Name", L"Culture" },
    [PH_CLR_TABLE_ASSEMBLYPROCESSOR] = { L"Processor" },
    [PH_CLR_TABLE_ASSEMBLYOS] = { L"OSPlatformId", L"OSMajorVersion", L"OSMinorVersion" },
    [PH_CLR_TABLE_ASSEMBLYREF] = { L"MajorVersion", L"MinorVersion", L"BuildNumber", L"RevisionNumber", L"Flags", L"PublicKeyOrToken", L"Name", L"Culture", L"HashValue" },
    [PH_CLR_TABLE_ASSEMBLYREFPROCESSOR] = { L"Processor", L"AssemblyRef" },
    [PH_CLR_TABLE_ASSEMBLYREFOS] = { L"OSPlatformId", L"OSMajorVersion", L"OSMinorVersion", L"AssemblyRef" },
    [PH_CLR_TABLE_FILE] = { L"Flags", L"Name", L"HashValue" },
    [PH_CLR_TABLE_EXPORTEDTYPE] = { L"Flags", L"TypeDefId", L"TypeName", L"TypeNamespace", L"Implementation" },
    [PH_CLR_TABLE_MANIFESTRESOURCE] = { L"Offset", L"Flags", L"Name", L"Implementation" },
    [PH_CLR_TABLE_NESTEDCLASS] = { L"NestedClass", L"EnclosingClass" },
    [PH_CLR_TABLE_GENERICPARAM] = { L"Number", L"Flags", L"Owner", L"Name" },
    [PH_CLR_TABLE_METHODSPEC] = { L"Method", L"Instantiation" },
    [PH_CLR_TABLE_GENERICPARAMCONSTRAINT] = { L"Owner", L"Constraint" },
    [PH_CLR_TABLE_DOCUMENT] = { L"Name", L"HashAlgorithm", L"Hash", L"Language" },
    [PH_CLR_TABLE_METHODDEBUGINFORMATION] = { L"Document", L"SequencePoints" },
    [PH_CLR_TABLE_LOCALSCOPE] = { L"Method", L"ImportScope", L"VariableList", L"ConstantList", L"StartOffset", L"Length" },
    [PH_CLR_TABLE_LOCALVARIABLE] = { L"Attributes", L"Index", L"Name" },
    [PH_CLR_TABLE_LOCALCONSTANT] = { L"Name", L"Signature" },
    [PH_CLR_TABLE_IMPORTSCOPE] = { L"Parent", L"Imports" },
    [PH_CLR_TABLE_STATEMACHINEMETHOD] = { L"MoveNextMethod", L"KickoffMethod" },
    [PH_CLR_TABLE_CUSTOMDEBUGINFORMATION] = { L"Owner", L"Kind", L"Value" },
};

/**
 * Converts PInvoke import flags into a readable string representation.
 *
 * \param Flags The PInvoke import flags.
 * \return A string representation of the import flags.
 */
PPH_STRING NTAPI PhClrImportFlagsToString(
    _In_ ULONG Flags
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (PhPmNoMangle(Flags))
        PhAppendStringBuilder2(&stringBuilder, L"No mangle, ");
    if (PhPmCharSetAnsi(Flags))
        PhAppendStringBuilder2(&stringBuilder, L"Ansi charset, ");
    if (PhPmCharSetUnicode(Flags))
        PhAppendStringBuilder2(&stringBuilder, L"Unicode charset, ");
    if (PhPmCharSetAuto(Flags))
        PhAppendStringBuilder2(&stringBuilder, L"Auto charset, ");
    if (PhPmSupportsLastError(Flags))
        PhAppendStringBuilder2(&stringBuilder, L"Supports last error, ");
    if (PhPmCallConvWinapi(Flags))
        PhAppendStringBuilder2(&stringBuilder, L"Winapi, ");
    if (PhPmCallConvCdecl(Flags))
        PhAppendStringBuilder2(&stringBuilder, L"Cdecl, ");
    if (PhPmCallConvStdcall(Flags))
        PhAppendStringBuilder2(&stringBuilder, L"Stdcall, ");
    if (PhPmCallConvThiscall(Flags))
        PhAppendStringBuilder2(&stringBuilder, L"Thiscall, ");
    if (PhPmCallConvFastcall(Flags))
        PhAppendStringBuilder2(&stringBuilder, L"Fastcall, ");
    if (PhPmBestFitEnabled(Flags))
        PhAppendStringBuilder2(&stringBuilder, L"Bestfit enabled, ");
    if (PhPmBestFitDisabled(Flags))
        PhAppendStringBuilder2(&stringBuilder, L"Bestfit disabled, ");
    if (Flags & PM_BEST_FIT_MASK)
        PhAppendStringBuilder2(&stringBuilder, L"Bestfit assembly, ");
    if (PhPmThrowOnUnmappableCharEnabled(Flags))
        PhAppendStringBuilder2(&stringBuilder, L"ThrowOnUnmappableChar enabled, ");
    if (PhPmThrowOnUnmappableCharDisabled(Flags))
        PhAppendStringBuilder2(&stringBuilder, L"ThrowOnUnmappableChar disabled, ");
    if (Flags & PM_THROW_ON_UNMAPPABLE_CHAR_MASK)
        PhAppendStringBuilder2(&stringBuilder, L"ThrowOnUnmappableChar assembly, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(Flags));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

/**
 * Uncompresses an unsigned integer from the ECMA-335 metadata stream with bounds checking.
 *
 * \param Pointer A pointer to the compressed data.
 * \param End A pointer to the end of the metadata stream.
 * \param Value A variable that receives the uncompressed value.
 * \param BytesRead A variable that receives the number of bytes read.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhClrUncompressData(
    _In_ PVOID Pointer,
    _In_ PVOID End,
    _Out_ PULONG Value,
    _Out_ PULONG BytesRead
    )
{
    PUCHAR data = Pointer;

    if (data >= (PUCHAR)End)
        return STATUS_BUFFER_OVERFLOW;

    if ((data[0] & 0x80) == 0)
    {
        *Value = data[0];
        *BytesRead = 1;
        return STATUS_SUCCESS;
    }

    if ((data[0] & 0xC0) == 0x80)
    {
        if (data + 2 > (PUCHAR)End)
            return STATUS_BUFFER_OVERFLOW;

        *Value = ((ULONG)(data[0] & 0x3F) << 8) | data[1];
        *BytesRead = 2;
        return STATUS_SUCCESS;
    }

    if ((data[0] & 0xE0) == 0xC0)
    {
        if (data + 4 > (PUCHAR)End)
            return STATUS_BUFFER_OVERFLOW;

        *Value = ((ULONG)(data[0] & 0x1F) << 24) | ((ULONG)data[1] << 16) | ((ULONG)data[2] << 8) | data[3];
        *BytesRead = 4;
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_IMAGE_FORMAT;
}

/**
 * Retrieves a string from the #Strings heap for a given table row and column.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param TableIndex The index of the table.
 * \param Row The row index (1-based).
 * \param Column The column index (0-based).
 * \param String A variable that receives the retrieved string.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhGetMappedClrTableStringEx(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Row,
    _In_ ULONG Column,
    _Out_ PPH_STRING* String
    )
{
    NTSTATUS status;
    ULONG index;
    ULONG type;

    status = PhGetMappedClrColumnInfo(ClrMetadata, TableIndex, Column, NULL, NULL, &type);
    if (!NT_SUCCESS(status))
        return status;
    if (type != PH_CLR_COLUMN_STRING)
        return STATUS_INVALID_PARAMETER;

    status = PhGetMappedClrColumnValue(ClrMetadata, TableIndex, Column, Row, &index);
    if (!NT_SUCCESS(status))
        return status;

    return PhGetMappedClrStringEx(ClrMetadata, index, String);
}

/**
 * Retrieves the raw row ID (RID) from a specified table row and column.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param TableIndex The index of the table.
 * \param Row The row index (1-based).
 * \param Column The column index (0-based).
 * \param Rid A variable that receives the retrieved RID.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhGetMappedClrTableRowRid(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Row,
    _In_ ULONG Column,
    _Out_ PULONG Rid
    )
{
    return PhGetMappedClrColumnValue(ClrMetadata, TableIndex, Column, Row, Rid);
}

/**
 * Retrieves the schema name for a given table index.
 *
 * \param TableIndex The index of the table.
 * \return The schema name of the table.
 */
PCSTR NTAPI PhGetMappedClrTableName(
    _In_ ULONG TableIndex
    )
{
    if (TableIndex >= PH_CLR_TABLE_MAXIMUM)
        return NULL;

    return PhClrTableSchema[TableIndex].Name;
}

/**
 * Retrieves the target table and RID token from a coded or simple index column.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param TableIndex The index of the table.
 * \param Column The column index (0-based).
 * \param Rid The row index (1-based).
 * \param TargetTable A variable that receives the target table index.
 * \param TargetRid A variable that receives the target RID.
 * \param Token A variable that receives the metadata token.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhGetMappedClrColumnToken(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Column,
    _In_ ULONG Rid,
    _Out_opt_ PULONG TargetTable,
    _Out_opt_ PULONG TargetRid,
    _Out_opt_ PULONG Token
    )
{
    NTSTATUS status;
    PPH_MAPPED_CLR_COLUMN column;
    ULONG value = 0;
    ULONG targetTable;
    ULONG targetRid;

    if (TableIndex >= PH_CLR_TABLE_MAXIMUM)
        return STATUS_INVALID_PARAMETER;
    if (Column >= ClrMetadata->Tables[TableIndex].ColumnCount)
        return STATUS_INVALID_PARAMETER;

    status = PhGetMappedClrColumnValue(ClrMetadata, TableIndex, Column, Rid, &value);

    if (!NT_SUCCESS(status))
        return status;

    column = &ClrMetadata->Tables[TableIndex].Columns[Column];

    if (column->Type == PH_CLR_COLUMN_TABLE)
    {
        targetTable = column->Codec;
        targetRid = value;
    }
    else if (column->Type == PH_CLR_COLUMN_CODED)
    {
        const PH_CLR_CODED_INDEX_INFO* info;
        ULONG tag;

        if (column->Codec >= CIX_MAXIMUM)
            return STATUS_INVALID_PARAMETER;

        info = &PhClrCodedIndex[column->Codec];
        tag = value & (((ULONG)1 << info->Bits) - 1);

        if (tag >= info->Count)
            return STATUS_INVALID_PARAMETER;
        if (info->Tables[tag] == CIX_RESERVED)
            return STATUS_INVALID_PARAMETER;

        targetTable = info->Tables[tag];
        targetRid = value >> info->Bits;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (targetTable >= PH_CLR_TABLE_MAXIMUM)
        return STATUS_INVALID_PARAMETER;

    if (TargetTable)
        *TargetTable = targetTable;
    if (TargetRid)
        *TargetRid = targetRid;
    if (Token)
        *Token = (targetTable << 24) | (targetRid & 0x00ffffff);

    return STATUS_SUCCESS;
}

/**
 * Retrieves the target name of a custom attribute row and checks if it is a TargetFrameworkAttribute.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param Row The row index (1-based) in the CustomAttribute table.
 * \param IsTargetFrameworkAttribute A variable that receives TRUE if the attribute is a TargetFrameworkAttribute.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhGetMappedClrCustomAttributeTargetName(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Row,
    _Out_ PBOOLEAN IsTargetFrameworkAttribute
    )
{
    NTSTATUS status;
    ULONG typeValue = 0;
    ULONG memberRid = 0;
    ULONG parentRid = 0;
    PPH_STRING memberName = NULL;
    PPH_STRING typeName = NULL;

    *IsTargetFrameworkAttribute = FALSE;

    status = PhGetMappedClrColumnValue(
        ClrMetadata, 
        PH_CLR_TABLE_CUSTOMATTRIBUTE, 
        1, 
        Row, 
        &typeValue
        );

    if (!NT_SUCCESS(status))
        return status;

    if ((typeValue & 3) != 1)
        return STATUS_NOT_FOUND;

    memberRid = typeValue >> 2;
    if (!memberRid)
        return STATUS_NOT_FOUND;

    status = PhGetMappedClrTableStringEx(
        ClrMetadata, 
        PH_CLR_TABLE_MEMBERREF, 
        memberRid, 
        1, 
        &memberName
        );

    if (!NT_SUCCESS(status))
        return status;

    if (!PhEqualString2(memberName, L".ctor", FALSE))
    {
        PhDereferenceObject(memberName);
        return STATUS_NOT_FOUND;
    }

    status = PhGetMappedClrTableRowRid(
        ClrMetadata, 
        PH_CLR_TABLE_MEMBERREF, 
        memberRid, 
        0,
        &parentRid
        );

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(memberName);
        return status;
    }

    switch (parentRid & 3)
    {
    case 0:
        status = PhGetMappedClrTableStringEx(ClrMetadata, PH_CLR_TABLE_TYPEDEF, parentRid >> 2, 1, &typeName);
        break;
    case 1:
        status = PhGetMappedClrTableStringEx(ClrMetadata, PH_CLR_TABLE_TYPEREF, parentRid >> 2, 1, &typeName);
        break;
    default:
        status = STATUS_NOT_FOUND;
        break;
    }

    PhDereferenceObject(memberName);

    if (!NT_SUCCESS(status))
        return status;

    *IsTargetFrameworkAttribute = PhEqualString2(typeName, L"TargetFrameworkAttribute", FALSE);
    PhDereferenceObject(typeName);
    return *IsTargetFrameworkAttribute ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}

/**
 * Retrieves a string from the #Strings heap for a given table row and column.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param TableIndex The index of the table.
 * \param Row The row index (1-based).
 * \param Column The column index (0-based).
 * \return The retrieved string or NULL on failure.
 */
PPH_STRING NTAPI PhGetMappedClrTableString(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Row,
    _In_ ULONG Column
    )
{
    PPH_STRING string = NULL;

    if (NT_SUCCESS(PhGetMappedClrTableStringEx(ClrMetadata, TableIndex, Row, Column, &string)))
        return string;

    return NULL;
}

/**
 * Attempts to read the TargetFramework attribute from the mapped CLR metadata.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param Version A variable that receives the target framework version string.
 * \return TRUE if the TargetFramework attribute was found and parsed, FALSE otherwise.
 */
BOOLEAN NTAPI PhTryGetMappedClrTargetFramework(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _Out_ PPH_STRING* Version
    )
{
    NTSTATUS status;
    PVOID data;
    ULONG length;
    ULONG stringLength;
    ULONG bytesRead;
    SIZE_T utf16Bytes;
    PCCH buffer;
    PPH_STRING version = NULL;

    status = PhGetMappedClrCustomAttributeByName(
        ClrMetadata,
        (PH_CLR_TABLE_ASSEMBLY << 24) | 1,
        L"System.Runtime.Versioning.TargetFrameworkAttribute",
        &data,
        &length
        );

    if (!NT_SUCCESS(status))
        return FALSE;

    __try
    {
        if (length < sizeof(USHORT) + 1 || *(PUSHORT)data != 1)
            return FALSE;

        status = PhClrUncompressData(
            PTR_ADD_OFFSET(data, sizeof(USHORT)),
            PTR_ADD_OFFSET(data, length),
            &stringLength,
            &bytesRead
            );

        if (!NT_SUCCESS(status) || !stringLength || stringLength > length - sizeof(USHORT) - bytesRead)
            return FALSE;

        buffer = PTR_ADD_OFFSET(data, sizeof(USHORT) + bytesRead);
        status = PhConvertUtf8ToUtf16Size(&utf16Bytes, buffer, stringLength);

        if (NT_SUCCESS(status))
        {
            version = PhCreateStringEx(NULL, utf16Bytes);
            status = PhConvertUtf8ToUtf16Buffer(version->Buffer, utf16Bytes, &utf16Bytes, buffer, stringLength);

            if (NT_SUCCESS(status))
            {
                version->Length = utf16Bytes;
                version->Buffer[utf16Bytes / sizeof(WCHAR)] = UNICODE_NULL;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    if (!NT_SUCCESS(status))
    {
        if (version) PhDereferenceObject(version);
        return FALSE;
    }
    *Version = version;
    return TRUE;
}

/**
 * Calculates the required index size (2 or 4 bytes) for a coded index set based on table row counts.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param CodedSet The coded index set ID.
 * \return The required index size in bytes (2 or 4).
 */
ULONG PhClrCodedIndexSize(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG CodedSet
    )
{
    const PH_CLR_CODED_INDEX_INFO *info = &PhClrCodedIndex[CodedSet];
    ULONG maxRows = 0;

    for (ULONG i = 0; i < info->Count; i++)
    {
        UCHAR table = info->Tables[i];

        if (table != CIX_RESERVED && table < PH_CLR_TABLE_MAXIMUM)
        {
            if (ClrMetadata->ExternalRowCounts[table] > maxRows)
                maxRows = ClrMetadata->ExternalRowCounts[table];
            if (ClrMetadata->Tables[table].RowCount > maxRows)
                maxRows = ClrMetadata->Tables[table].RowCount;
        }
    }

    if (maxRows < ((ULONG)1 << (16 - info->Bits)))
        return 2;

    return 4;
}

/**
 * Computes the per-column offsets and total row size for a given table based on the global heap and index widths.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param TableIndex The index of the table to compute the layout for.
 * \remarks Requires every table's RowCount to already be populated.
 */
VOID PhClrComputeTableLayout(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex
    )
{
    PPH_MAPPED_CLR_TABLE table = &ClrMetadata->Tables[TableIndex];
    const PH_CLR_TABLE_SCHEMA *schema = &PhClrTableSchema[TableIndex];
    ULONG offset = 0;
    ULONG columnCount = 0;

    table->Name = schema->Name;

    for (ULONG i = 0; i < PH_CLR_MAX_COLUMNS; i++)
    {
        UCHAR code = schema->Columns[i];
        PPH_MAPPED_CLR_COLUMN column;
        ULONG size;
        UCHAR type;
        UCHAR codec = 0;

        if (code == CC_END)
            break;

        if (FlagOn(code, CC_COD))
        {
            type = PH_CLR_COLUMN_CODED;
            codec = ClearFlag(code, CC_COD);
            size = PhClrCodedIndexSize(ClrMetadata, codec);
        }
        else if (FlagOn(code, CC_TBL))
        {
            type = PH_CLR_COLUMN_TABLE;
            codec = ClearFlag(code, CC_TBL);
            size = PhClrSimpleIndexSize(ClrMetadata, codec);
        }
        else
        {
            switch (code)
            {
            case CC_I1:
                type = PH_CLR_COLUMN_FIXED;
                size = 1;
                break;
            case CC_I2:
                type = PH_CLR_COLUMN_FIXED;
                size = 2;
                break;
            case CC_I4:
                type = PH_CLR_COLUMN_FIXED;
                size = 4;
                break;
            case CC_STR:
                type = PH_CLR_COLUMN_STRING;
                size = (ClrMetadata->HeapSizes & PH_CLR_HEAPSIZE_STRING) ? 4 : 2;
                break;
            case CC_GUID:
                type = PH_CLR_COLUMN_GUID;
                size = (ClrMetadata->HeapSizes & PH_CLR_HEAPSIZE_GUID) ? 4 : 2;
                break;
            case CC_BLOB:
                type = PH_CLR_COLUMN_BLOB;
                size = (ClrMetadata->HeapSizes & PH_CLR_HEAPSIZE_BLOB) ? 4 : 2;
                break;
            default:
                type = PH_CLR_COLUMN_FIXED;
                size = 2;
                break;
            }
        }

        column = &table->Columns[columnCount];
        column->Type = type;
        column->Size = (UCHAR)size;
        column->Offset = (UCHAR)offset;
        column->Codec = codec;

        offset += size;
        columnCount++;
    }

    table->ColumnCount = columnCount;
    table->RowSize = offset;
}

/**
 * Parses the CLR metadata stream headers and locates the various heaps and tables streams.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhClrParseStreams(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata
    )
{
    PH_MAPPED_IMAGE bounds = { 0 };
    PPH_MAPPED_IMAGE mappedImage = &bounds;
    PPH_CLR_STORAGESIGNATURE signature = ClrMetadata->MetadataAddress;
    PPH_CLR_STORAGEHEADER header;
    PPH_CLR_STORAGESTREAM stream;
    ULONG streamCount;

    bounds.ViewBase = ClrMetadata->MetadataAddress;
    bounds.ViewSize = ClrMetadata->MetadataSize;

    __try
    {
        PhMappedImageProbe(mappedImage, signature, sizeof(PH_CLR_STORAGESIGNATURE));

        if (signature->Signature != PH_CLR_STORAGE_MAGIC)
            return STATUS_INVALID_IMAGE_FORMAT;

        header = PTR_ADD_OFFSET(signature, RTL_SIZEOF_THROUGH_FIELD(PH_CLR_STORAGESIGNATURE, VersionLength) + ALIGN_UP((SIZE_T)signature->VersionLength, ULONG));
        PhMappedImageProbe(mappedImage, header, sizeof(PH_CLR_STORAGEHEADER));

        streamCount = header->Streams;
        stream = PTR_ADD_OFFSET(header, sizeof(PH_CLR_STORAGEHEADER));

        for (ULONG i = 0; i < streamCount; i++)
        {
            SIZE_T nameLength;
            PVOID streamData;

            PhMappedImageProbe(mappedImage, stream, UFIELD_OFFSET(PH_CLR_STORAGESTREAM, Name));

            // Bounded name scan (max 32 chars per the metadata format).
            nameLength = 0;

            while (nameLength < 32)
            {
                PhMappedImageProbe(mappedImage, &stream->Name[nameLength], sizeof(CHAR));

                if (stream->Name[nameLength] == ANSI_NULL)
                    break;

                nameLength++;
            }

            if (nameLength >= 32)
                return STATUS_INVALID_IMAGE_FORMAT;

            streamData = PTR_ADD_OFFSET(ClrMetadata->MetadataAddress, stream->Offset);
            PhMappedImageProbe(mappedImage, streamData, stream->Size);

            if (PhEqualBytesZ(stream->Name, "#~", FALSE) || PhEqualBytesZ(stream->Name, "#-", FALSE))
            {
                ClrMetadata->TablesStream = streamData;
                ClrMetadata->TablesStreamSize = stream->Size;
                ClrMetadata->Uncompressed = stream->Name[1] == '-';
            }
            else if (PhEqualBytesZ(stream->Name, "#Strings", FALSE))
            {
                ClrMetadata->StringHeap = streamData;
                ClrMetadata->StringHeapSize = stream->Size;
            }
            else if (PhEqualBytesZ(stream->Name, "#US", FALSE))
            {
                ClrMetadata->UserStringHeap = streamData;
                ClrMetadata->UserStringHeapSize = stream->Size;
            }
            else if (PhEqualBytesZ(stream->Name, "#Blob", FALSE))
            {
                ClrMetadata->BlobHeap = streamData;
                ClrMetadata->BlobHeapSize = stream->Size;
            }
            else if (PhEqualBytesZ(stream->Name, "#GUID", FALSE))
            {
                ClrMetadata->GuidHeap = streamData;
                ClrMetadata->GuidHeapSize = stream->Size;
            }
            else if (PhEqualBytesZ(stream->Name, "#Pdb", FALSE))
            {
                ClrMetadata->PdbStream = streamData;
                ClrMetadata->PdbStreamSize = stream->Size;
            }

            // Advance to the next stream (name is null-terminated and padded to 4 bytes).
            stream = PTR_ADD_OFFSET(stream, UFIELD_OFFSET(PH_CLR_STORAGESTREAM, Name) + ALIGN_UP(nameLength + 1, ULONG));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    if (!ClrMetadata->TablesStream)
        return STATUS_INVALID_IMAGE_FORMAT;

    return STATUS_SUCCESS;
}

/**
 * Parses the tables stream (#~ or #-) to determine valid tables, row counts, and memory layout.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhClrParseTables(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata
    )
{
    PH_MAPPED_IMAGE bounds = { 0 };
    PPH_MAPPED_IMAGE mappedImage = &bounds;
    PPH_CLR_TABLESHEADER header = ClrMetadata->TablesStream;
    PULONG rowCounts;
    PVOID rows;
    ULONG validCount;

    bounds.ViewBase = ClrMetadata->TablesStream;
    bounds.ViewSize = ClrMetadata->TablesStreamSize;

    __try
    {
        PhMappedImageProbe(mappedImage, header, sizeof(PH_CLR_TABLESHEADER));

        ClrMetadata->MajorVersion = header->Major;
        ClrMetadata->MinorVersion = header->Minor;
        ClrMetadata->HeapSizes = header->HeapSizes;
        ClrMetadata->ValidMask = header->ValidMask;
        ClrMetadata->SortedMask = header->SortedMask;

        if (ClrMetadata->PdbStream && (header->ValidMask & ~((ULONG64)0xff << PH_CLR_TABLE_DOCUMENT)))
            return STATUS_INVALID_IMAGE_FORMAT;
        validCount = PhCountBitsUlong64(header->ValidMask);
        rowCounts = PTR_ADD_OFFSET(header, sizeof(PH_CLR_TABLESHEADER));
        PhMappedImageProbe(mappedImage, rowCounts, validCount * sizeof(ULONG));

        // Assign row counts to the tables flagged in ValidMask, in ascending order.
        {
            ULONG next = 0;

            for (ULONG i = 0; i < PH_CLR_TABLE_MAXIMUM; i++)
            {
                if (header->ValidMask & ((ULONG64)1 << i))
                {
                    if (rowCounts[next] > 0x00ffffff)
                        return STATUS_INVALID_IMAGE_FORMAT;
                    ClrMetadata->Tables[i].RowCount = rowCounts[next];
                    next++;
                }
            }
        }

        rows = PTR_ADD_OFFSET(rowCounts, validCount * sizeof(ULONG));

        if (FlagOn(header->HeapSizes, PH_CLR_HEAPSIZE_EXTRADATA))
        {
            rows = PTR_ADD_OFFSET(rows, sizeof(ULONG)); // edit-and-continue extra data
        }

        // Compute the layout for every schema-defined table (row counts are known now).

        for (ULONG i = 0; i < PH_CLR_TABLE_MAXIMUM; i++)
        {
            if (PhClrTableSchema[i].Name)
                PhClrComputeTableLayout(ClrMetadata, i);
        }

        // Walk the table data assigning each present table its row pointer.

        for (ULONG i = 0; i < PH_CLR_TABLE_MAXIMUM; i++)
        {
            PPH_MAPPED_CLR_TABLE table = &ClrMetadata->Tables[i];

            if (!(header->ValidMask & ((ULONG64)1 << i)))
                continue;

            if (!PhClrTableSchema[i].Name)
                return STATUS_INVALID_IMAGE_FORMAT; // present but unknown table

            if (table->RowCount)
            {
                SIZE_T length = UInt32x32To64(table->RowCount, table->RowSize);

                PhMappedImageProbe(mappedImage, rows, length);
                table->Rows = rows;
                rows = PTR_ADD_OFFSET(rows, length);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

/**
 * Initializes a CLR metadata structure from a mapped PE image by locating the COM descriptor and parsing metadata headers.
 *
 * \param ClrMetadata A pointer to a structure that receives the mapped CLR metadata information.
 * \param MappedImage A pointer to the mapped PE image.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhInitializeMappedClrMetadata(
    _Out_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ PPH_MAPPED_IMAGE MappedImage
    )
{
    NTSTATUS status;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_COR20_HEADER cor20Header;
    PVOID metadata;

    memset(ClrMetadata, 0, sizeof(PH_MAPPED_CLR_METADATA));
    ClrMetadata->MappedImage = MappedImage;

    status = PhGetMappedImageDataDirectory(
        MappedImage,
        IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
        &dataDirectory
        );

    if (!NT_SUCCESS(status))
        return status;

    if (dataDirectory->VirtualAddress == 0 || dataDirectory->Size == 0)
        return STATUS_INVALID_IMAGE_FORMAT;

    status = PhMappedImageRvaToVa(
        MappedImage,
        dataDirectory->VirtualAddress,
        &cor20Header
        );

    if (!NT_SUCCESS(status))
        return status;

    __try
    {
        PhMappedImageProbe(MappedImage, cor20Header, sizeof(IMAGE_COR20_HEADER));

        if (cor20Header->MetaData.VirtualAddress == 0 || cor20Header->MetaData.Size == 0)
            return STATUS_INVALID_IMAGE_FORMAT;

        status = PhMappedImageRvaToVa(
            MappedImage,
            cor20Header->MetaData.VirtualAddress,
            &metadata);

        if (!NT_SUCCESS(status))
            return status;

        PhMappedImageProbe(MappedImage, metadata, cor20Header->MetaData.Size);

        ClrMetadata->Cor20Header = cor20Header;
        ClrMetadata->MetadataAddress = metadata;
        ClrMetadata->MetadataSize = cor20Header->MetaData.Size;

        status = PhClrParseStreams(ClrMetadata);

        if (!NT_SUCCESS(status))
            return status;

        status = PhClrParseTables(ClrMetadata);

        if (!NT_SUCCESS(status))
            return status;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

/**
 * Cleans up resources associated with mapped CLR metadata.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata structure to clean up.
 */
VOID NTAPI PhDeleteMappedClrMetadata(
    _Inout_ PPH_MAPPED_CLR_METADATA ClrMetadata
    )
{
    // The structure does not own any heap allocations; everything points into
    // the mapped image. Provided for API symmetry with Initialize.
    NOTHING;
}

/**
 * Gets the total number of valid tables present in the metadata.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \return The number of valid tables.
 */
ULONG NTAPI PhGetMappedClrNumberOfTables(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata
    )
{
    return PhCountBitsUlong64(ClrMetadata->ValidMask);
}

/**
 * Retrieves layout and dimension information for a specified table.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param TableIndex The index of the table.
 * \param RowSize A variable that receives the size of a single row in bytes.
 * \param RowCount A variable that receives the total number of rows.
 * \param ColumnCount A variable that receives the total number of columns.
 * \param Name A variable that receives the schema name of the table.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhGetMappedClrTableInfo(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _Out_opt_ PULONG RowSize,
    _Out_opt_ PULONG RowCount,
    _Out_opt_ PULONG ColumnCount,
    _Out_opt_ PCSTR *Name
    )
{
    return PhGetMappedClrTableInfoEx(ClrMetadata, TableIndex, RowSize, RowCount, ColumnCount, Name, NULL);
}

/**
 * Retrieves extended layout and dimension information for a specified table.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param TableIndex The index of the table.
 * \param RowSize A variable that receives the size of a single row in bytes.
 * \param RowCount A variable that receives the total number of rows.
 * \param ColumnCount A variable that receives the total number of columns.
 * \param Name A variable that receives the schema name of the table.
 * \param Table A variable that receives a pointer to the internal table structure.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhGetMappedClrTableInfoEx(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _Out_opt_ PULONG RowSize,
    _Out_opt_ PULONG RowCount,
    _Out_opt_ PULONG ColumnCount,
    _Out_opt_ PCSTR *Name,
    _Out_opt_ PPH_MAPPED_CLR_TABLE *Table
    )
{
    PPH_MAPPED_CLR_TABLE table;

    if (TableIndex >= PH_CLR_TABLE_MAXIMUM || !PhClrTableSchema[TableIndex].Name)
        return STATUS_INVALID_PARAMETER;

    table = &ClrMetadata->Tables[TableIndex];

    if (RowSize)
        *RowSize = table->RowSize;
    if (RowCount)
        *RowCount = table->RowCount;
    if (ColumnCount)
        *ColumnCount = table->ColumnCount;
    if (Name)
        *Name = table->Name;
    if (Table)
        *Table = table;

    return STATUS_SUCCESS;
}

/**
 * Retrieves offset and sizing information for a specific column in a table.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param TableIndex The index of the table.
 * \param Column The column index (0-based).
 * \param Offset A variable that receives the byte offset of the column within a row.
 * \param Size A variable that receives the size of the column in bytes.
 * \param Type A variable that receives the column type.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhGetMappedClrColumnInfo(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Column,
    _Out_opt_ PULONG Offset,
    _Out_opt_ PULONG Size,
    _Out_opt_ PULONG Type
    )
{
    PPH_MAPPED_CLR_TABLE table;
    PPH_MAPPED_CLR_COLUMN column;

    if (TableIndex >= PH_CLR_TABLE_MAXIMUM || !PhClrTableSchema[TableIndex].Name)
        return STATUS_INVALID_PARAMETER;

    table = &ClrMetadata->Tables[TableIndex];

    if (Column >= table->ColumnCount)
        return STATUS_INVALID_PARAMETER;

    column = &table->Columns[Column];

    if (Offset)
        *Offset = column->Offset;
    if (Size)
        *Size = column->Size;
    if (Type)
        *Type = column->Type;

    return STATUS_SUCCESS;
}

/**
 * Retrieves a pointer to the raw row data for a given table and RID.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param TableIndex The index of the table.
 * \param Rid The row index (1-based).
 * \param Row A variable that receives a pointer to the row data.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhGetMappedClrTableRow(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Rid,
    _Out_ PVOID *Row
    )
{
    PPH_MAPPED_CLR_TABLE table;
    PVOID row;

    if (TableIndex >= PH_CLR_TABLE_MAXIMUM || !PhClrTableSchema[TableIndex].Name)
        return STATUS_INVALID_PARAMETER;

    __try
    {
        table = &ClrMetadata->Tables[TableIndex];

        if (Rid < 1 || Rid > table->RowCount || !table->Rows)
            return STATUS_INVALID_PARAMETER;

        row = PTR_ADD_OFFSET(table->Rows, UInt32x32To64(Rid - 1, table->RowSize));

        PhMappedImageProbe(ClrMetadata->MappedImage, row, table->RowSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    *Row = row;
    return STATUS_SUCCESS;
}

/**
 * Retrieves the raw integer value of a specific column in a given row.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param TableIndex The index of the table.
 * \param Column The column index (0-based).
 * \param Rid The row index (1-based).
 * \param Value A variable that receives the raw column value.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhGetMappedClrColumnValue(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Column,
    _In_ ULONG Rid,
    _Out_ PULONG Value
    )
{
    NTSTATUS status;
    PPH_MAPPED_CLR_COLUMN column;
    PVOID row;

    if (TableIndex >= PH_CLR_TABLE_MAXIMUM || !PhClrTableSchema[TableIndex].Name)
        return STATUS_INVALID_PARAMETER;

    if (Column >= ClrMetadata->Tables[TableIndex].ColumnCount)
        return STATUS_INVALID_PARAMETER;

    column = &ClrMetadata->Tables[TableIndex].Columns[Column];

    status = PhGetMappedClrTableRow(ClrMetadata, TableIndex, Rid, &row);

    if (!NT_SUCCESS(status))
        return status;

    __try
    {
        *Value = (ULONG)PhClrReadInteger(PTR_ADD_OFFSET(row, column->Offset), column->Size);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

/**
 * Gets the total size of the #Strings heap.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \return The size of the #Strings heap in bytes.
 */
ULONG NTAPI PhGetMappedClrStringHeapSize(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata
    )
{
    return ClrMetadata->StringHeapSize;
}

/**
 * Retrieves a string from the #Strings heap by index.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param Index The byte offset index into the #Strings heap.
 * \param String A variable that receives a pointer to the null-terminated UTF-8 string.
 * \param Length Optionally receives the UTF-8 string length in bytes, excluding the null terminator.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhGetMappedClrString(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Index,
    _Out_ PCSTR *String,
    _Out_opt_ PULONG Length
    )
{
    PSTR string;
    ULONG length = 0;

    if (!ClrMetadata->StringHeap || Index >= ClrMetadata->StringHeapSize)
        return STATUS_INVALID_PARAMETER;

    string = PTR_ADD_OFFSET(ClrMetadata->StringHeap, Index);

    __try
    {
        // Ensure the string is null-terminated within the heap bounds.
        while (Index + length < ClrMetadata->StringHeapSize)
        {
            PhMappedImageProbe(ClrMetadata->MappedImage, &string[length], sizeof(CHAR));

            if (string[length] == ANSI_NULL)
                break;

            length++;
        }

        if (Index + length >= ClrMetadata->StringHeapSize)
            return STATUS_INVALID_IMAGE_FORMAT;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    *String = string;

    if (Length)
        *Length = length;

    return STATUS_SUCCESS;
}

/**
 * Retrieves a blob of data from the #Blob heap by index.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param Index The byte offset index into the #Blob heap.
 * \param Data A variable that receives a pointer to the blob data.
 * \param Length A variable that receives the length of the blob data.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhGetMappedClrBlob(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Index,
    _Out_ PVOID *Data,
    _Out_ PULONG Length
    )
{
    NTSTATUS status;
    PVOID blobEnd;
    PVOID entry;
    ULONG length;
    ULONG bytesRead;

    if (!ClrMetadata->BlobHeap || Index >= ClrMetadata->BlobHeapSize)
        return STATUS_INVALID_PARAMETER;

    entry = PTR_ADD_OFFSET(ClrMetadata->BlobHeap, Index);
    blobEnd = PTR_ADD_OFFSET(ClrMetadata->BlobHeap, ClrMetadata->BlobHeapSize);

    __try
    {
        PhMappedImageProbe(ClrMetadata->MappedImage, entry, sizeof(UCHAR));

        status = PhClrUncompressData(entry, blobEnd, &length, &bytesRead);

        if (!NT_SUCCESS(status))
            return status;

        entry = PTR_ADD_OFFSET(entry, bytesRead);

        if (PTR_ADD_OFFSET(entry, length) > blobEnd)
            return STATUS_INVALID_IMAGE_FORMAT;

        PhMappedImageProbe(ClrMetadata->MappedImage, entry, length);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    *Data = entry;
    *Length = length;
    return STATUS_SUCCESS;
}

/**
 * Retrieves a GUID from the #GUID heap by index.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param Index The 1-based index into the #GUID heap.
 * \param Guid A variable that receives the retrieved GUID.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhGetMappedClrGuid(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Index,
    _Out_ PGUID Guid
    )
{
    PVOID entry;

    // The #GUID heap is a 1-based array of 16-byte GUIDs (index 0 means none).
    if (!ClrMetadata->GuidHeap || Index == 0)
        return STATUS_INVALID_PARAMETER;

    if (UInt32x32To64(Index, sizeof(GUID)) > ClrMetadata->GuidHeapSize)
        return STATUS_INVALID_PARAMETER;

    entry = PTR_ADD_OFFSET(ClrMetadata->GuidHeap, UInt32x32To64(Index - 1, sizeof(GUID)));

    __try
    {
        PhMappedImageProbe(ClrMetadata->MappedImage, entry, sizeof(GUID));
        memcpy(Guid, entry, sizeof(GUID));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

/**
 * Enumerates all valid tables in the mapped CLR metadata.
 *
 * \param ClrMetadata A pointer to the mapped CLR metadata.
 * \param Callback A user-defined callback function invoked for each valid table.
 * \param Context An optional user-defined context passed to the callback function.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhEnumMappedClrTables(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ PPH_CLR_ENUM_TABLES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    for (ULONG i = 0; i < PH_CLR_TABLE_MAXIMUM; i++)
    {
        PPH_MAPPED_CLR_TABLE table = &ClrMetadata->Tables[i];

        if (!(ClrMetadata->ValidMask & ((ULONG64)1 << i)))
            continue;

        if (!Callback(i, table->RowSize, table->RowCount, table->Name, table->Rows, Context))
            break;
    }

    return STATUS_SUCCESS;
}


/**
 * Retrieves an owned UTF-16 string from a #Strings byte offset.
 * The caller dereferences String on success. Outputs are unchanged on failure.
 */
NTSTATUS NTAPI PhGetMappedClrStringEx(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Index,
    _Out_ PPH_STRING *String
    )
{
    NTSTATUS status;
    PCSTR buffer;
    ULONG length;
    SIZE_T utf16Bytes;
    PPH_STRING string = NULL;

    status = PhGetMappedClrString(ClrMetadata, Index, &buffer, &length);
    if (!NT_SUCCESS(status))
        return status;

    __try
    {
        status = PhConvertUtf8ToUtf16Size(&utf16Bytes, buffer, length);
        if (NT_SUCCESS(status))
        {
            string = PhCreateStringEx(NULL, utf16Bytes);
            status = PhConvertUtf8ToUtf16Buffer(string->Buffer, utf16Bytes, &utf16Bytes, buffer, length);
            if (NT_SUCCESS(status))
            {
                string->Length = utf16Bytes;
                string->Buffer[utf16Bytes / sizeof(WCHAR)] = UNICODE_NULL;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    if (!NT_SUCCESS(status))
    {
        if (string)
            PhDereferenceObject(string);
        return status;
    }

    *String = string;
    return STATUS_SUCCESS;
}

/**
 * Resolves a blob table column. Outputs are unchanged on failure.
 */
NTSTATUS NTAPI PhGetMappedClrTableBlob(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Row,
    _In_ ULONG Column,
    _Out_ PVOID *Data,
    _Out_ PULONG Length
    )
{
    NTSTATUS status;
    ULONG index;
    ULONG type;

    status = PhGetMappedClrColumnInfo(ClrMetadata, TableIndex, Column, NULL, NULL, &type);
    if (!NT_SUCCESS(status))
        return status;
    if (type != PH_CLR_COLUMN_BLOB)
        return STATUS_INVALID_PARAMETER;

    status = PhGetMappedClrColumnValue(ClrMetadata, TableIndex, Column, Row, &index);
    if (!NT_SUCCESS(status))
        return status;

    return PhGetMappedClrBlob(ClrMetadata, index, Data, Length);
}

/**
 * Resolves a guid table column. Outputs are unchanged on failure.
 */
NTSTATUS NTAPI PhGetMappedClrTableGuid(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Row,
    _In_ ULONG Column,
    _Out_ PGUID Guid
    )
{
    NTSTATUS status;
    ULONG index;
    ULONG type;
    GUID guid;

    status = PhGetMappedClrColumnInfo(ClrMetadata, TableIndex, Column, NULL, NULL, &type);
    if (!NT_SUCCESS(status))
        return status;
    if (type != PH_CLR_COLUMN_GUID)
        return STATUS_INVALID_PARAMETER;

    status = PhGetMappedClrColumnValue(ClrMetadata, TableIndex, Column, Row, &index);
    if (!NT_SUCCESS(status))
        return status;

    status = PhGetMappedClrGuid(ClrMetadata, index, &guid);
    if (!NT_SUCCESS(status))
        return status;

    *Guid = guid;
    return STATUS_SUCCESS;
}

/**
 * Copies a #US entry at a byte offset (not a metadata token) to an owned string.
 * Preserves embedded nulls and excludes the trailing special-character flag.
 * The caller dereferences String on success. Outputs are unchanged on failure.
 */
NTSTATUS NTAPI PhGetMappedClrUserString(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Index,
    _Out_ PPH_STRING *String
    )
{
    NTSTATUS status;
    PUCHAR entry;
    ULONG remaining;
    ULONG length;
    ULONG bytesRead;
    PPH_STRING string = NULL;

    if (!ClrMetadata->UserStringHeap || Index >= ClrMetadata->UserStringHeapSize)
        return STATUS_INVALID_PARAMETER;

    entry = PTR_ADD_OFFSET(ClrMetadata->UserStringHeap, Index);
    remaining = ClrMetadata->UserStringHeapSize - Index;

    __try
    {
        PhMappedImageProbe(ClrMetadata->MappedImage, entry, remaining);
        status = PhClrUncompressData(entry, entry + remaining, &length, &bytesRead);
        if (!NT_SUCCESS(status))
            return status;
        if (length > remaining - bytesRead)
            return STATUS_INVALID_IMAGE_FORMAT;

        entry += bytesRead;
        if (length)
        {
            if (!(length & 1) || entry[length - 1] > 1)
                return STATUS_INVALID_IMAGE_FORMAT;
            length--;
        }

        string = PhCreateStringEx(NULL, length);
        if (length)
            memcpy(string->Buffer, entry, length);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        if (string)
            PhDereferenceObject(string);
        return GetExceptionCode();
    }

    *String = string;
    return STATUS_SUCCESS;
}

// Validates a non-nil table token without touching mapped row data.
static NTSTATUS PhpValidateClrToken(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Token
    )
{
    ULONG table = Token >> 24;
    ULONG rid = Token & 0x00ffffff;

    if (table >= PH_CLR_TABLE_MAXIMUM || !PhClrTableSchema[table].Name ||
        !rid || rid > ClrMetadata->Tables[table].RowCount)
        return STATUS_INVALID_PARAMETER;

    return STATUS_SUCCESS;
}

/** Retrieves a custom attribute by token. Blob data is borrowed from the mapped image. */
NTSTATUS NTAPI PhGetMappedClrCustomAttributeProps(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Token,
    _Out_opt_ PULONG ParentToken,
    _Out_opt_ PULONG ConstructorToken,
    _Out_opt_ PVOID *Data,
    _Out_opt_ PULONG Length
    )
{
    NTSTATUS status;
    ULONG row = Token & 0x00ffffff;
    ULONG parent;
    ULONG constructor;
    PVOID data;
    ULONG length;

    if ((Token >> 24) != PH_CLR_TABLE_CUSTOMATTRIBUTE)
        return STATUS_INVALID_PARAMETER;

    status = PhpValidateClrToken(ClrMetadata, Token);
    if (!NT_SUCCESS(status))
        return status;
    status = PhGetMappedClrColumnToken(ClrMetadata, PH_CLR_TABLE_CUSTOMATTRIBUTE,
        PH_CLR_CUSTOMATTRIBUTE_REC_COL_PARENT, row, NULL, NULL, &parent);
    if (!NT_SUCCESS(status))
        return status;
    status = PhpValidateClrToken(ClrMetadata, parent);
    if (!NT_SUCCESS(status))
        return status;
    status = PhGetMappedClrColumnToken(ClrMetadata, PH_CLR_TABLE_CUSTOMATTRIBUTE,
        PH_CLR_CUSTOMATTRIBUTE_REC_COL_TYPE, row, NULL, NULL, &constructor);
    if (!NT_SUCCESS(status))
        return status;
    status = PhpValidateClrToken(ClrMetadata, constructor);
    if (!NT_SUCCESS(status))
        return status;
    status = PhGetMappedClrTableBlob(ClrMetadata, PH_CLR_TABLE_CUSTOMATTRIBUTE,
        row, PH_CLR_CUSTOMATTRIBUTE_REC_COL_VALUE, &data, &length);
    if (!NT_SUCCESS(status))
        return status;

    if (ParentToken) *ParentToken = parent;
    if (ConstructorToken) *ConstructorToken = constructor;
    if (Data) *Data = data;
    if (Length) *Length = length;
    return STATUS_SUCCESS;
}

// Finds the declaring TypeDef, including MethodPtr indirection in unoptimized metadata.
static NTSTATUS PhpGetClrMethodOwner(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG MethodToken,
    _Out_ PULONG TypeToken
    )
{
    NTSTATUS status;
    ULONG method = MethodToken & 0x00ffffff;
    ULONG listTable = (ClrMetadata->ValidMask & ((ULONG64)1 << PH_CLR_TABLE_METHODPTR)) ? PH_CLR_TABLE_METHODPTR : PH_CLR_TABLE_METHODDEF;
    ULONG count = ClrMetadata->Tables[listTable].RowCount;

    if (count > 0x00ffffff)
        return STATUS_INVALID_IMAGE_FORMAT;

    status = PhpValidateClrToken(ClrMetadata, MethodToken);
    if (!NT_SUCCESS(status) || (MethodToken >> 24) != PH_CLR_TABLE_METHODDEF)
        return STATUS_INVALID_PARAMETER;

    for (ULONG i = 1; i <= ClrMetadata->Tables[PH_CLR_TABLE_TYPEDEF].RowCount; i++)
    {
        ULONG start;
        ULONG end = count + 1;

        status = PhGetMappedClrColumnValue(
            ClrMetadata,
            PH_CLR_TABLE_TYPEDEF,
            PH_CLR_TYPEDEF_REC_COL_METHODLIST,
            i,
            &start
            );

        if (!NT_SUCCESS(status))
            return status;

        if (i < ClrMetadata->Tables[PH_CLR_TABLE_TYPEDEF].RowCount)
        {
            status = PhGetMappedClrColumnValue(
                ClrMetadata,
                PH_CLR_TABLE_TYPEDEF,
                PH_CLR_TYPEDEF_REC_COL_METHODLIST,
                i + 1,
                &end
                );

            if (!NT_SUCCESS(status))
                return status;
        }

        if (!start || start > end || end > (ULONG64)count + 1)
            return STATUS_INVALID_IMAGE_FORMAT;

        for (ULONG j = start; j < end; j++)
        {
            ULONG candidate = j;

            if (listTable == PH_CLR_TABLE_METHODPTR)
            {
                status = PhGetMappedClrColumnValue(ClrMetadata, listTable, 0, j, &candidate);
                if (!NT_SUCCESS(status)) return status;
            }

            if (candidate == method)
            {
                *TypeToken = (PH_CLR_TABLE_TYPEDEF << 24) | i;
                return STATUS_SUCCESS;
            }
        }
    }
    return STATUS_NOT_FOUND;
}

// Resolves a namespace-qualified type name; nested names use '+'.
static NTSTATUS PhpGetClrTypeName(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Token,
    _In_ ULONG Depth,
    _Out_ PPH_STRING *Name
    )
{
    NTSTATUS status;
    ULONG table = Token >> 24;
    ULONG row = Token & 0x00ffffff;
    ULONG enclosing = 0;
    PPH_STRING name = NULL;
    PPH_STRING prefix = NULL;
    PPH_STRING result = NULL;

    if (Depth >= 64 || (table != PH_CLR_TABLE_TYPEDEF && table != PH_CLR_TABLE_TYPEREF))
        return STATUS_INVALID_IMAGE_FORMAT;

    status = PhpValidateClrToken(ClrMetadata, Token);
    if (!NT_SUCCESS(status))
        return status;

    if (table == PH_CLR_TABLE_TYPEREF)
    {
        status = PhGetMappedClrColumnToken(
            ClrMetadata,
            table,
            0,
            row,
            NULL,
            NULL,
            &enclosing
            );

        if (!NT_SUCCESS(status))
            return status;

        if ((enclosing >> 24) != PH_CLR_TABLE_TYPEREF) enclosing = 0;
    }
    else
    {
        for (ULONG i = 1; i <= ClrMetadata->Tables[PH_CLR_TABLE_NESTEDCLASS].RowCount; i++)
        {
            ULONG nested;

            status = PhGetMappedClrColumnValue(
                ClrMetadata,
                PH_CLR_TABLE_NESTEDCLASS,
                0,
                i,
                &nested
                );

            if (!NT_SUCCESS(status))
                return status;

            if (nested == row)
            {
                status = PhGetMappedClrColumnToken(
                    ClrMetadata,
                    PH_CLR_TABLE_NESTEDCLASS,
                    1,
                    i,
                    NULL,
                    NULL,
                    &enclosing
                    );

                if (!NT_SUCCESS(status))
                    return status;

                break;
            }
        }
    }

    status = PhGetMappedClrTableStringEx(ClrMetadata, table, row, 1, &name);
    if (!NT_SUCCESS(status)) return status;

    if (enclosing)
        status = PhpGetClrTypeName(ClrMetadata, enclosing, Depth + 1, &prefix);
    else
        status = PhGetMappedClrTableStringEx(ClrMetadata, table, row, 2, &prefix);

    if (NT_SUCCESS(status))
    {
        result = PhConcatStrings(3, prefix->Buffer, prefix->Length ? (enclosing ? L"+" : L".") : L"", name->Buffer);
    }

    PhDereferenceObject(name);
    if (prefix) PhDereferenceObject(prefix);

    if (!NT_SUCCESS(status))
        return status;

    *Name = result;
    return STATUS_SUCCESS;
}

/** Finds the first matching attribute by parent token and case-sensitive qualified type name. */
NTSTATUS NTAPI PhGetMappedClrCustomAttributeByName(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG ParentToken,
    _In_ PCWSTR Name,
    _Out_ PVOID *Data,
    _Out_ PULONG Length
    )
{
    NTSTATUS status;

    status = PhpValidateClrToken(ClrMetadata, ParentToken);
    if (!NT_SUCCESS(status)) return status;
    for (ULONG i = 1; i <= ClrMetadata->Tables[PH_CLR_TABLE_CUSTOMATTRIBUTE].RowCount; i++)
    {
        ULONG parent;
        ULONG constructor;
        ULONG type;
        PVOID data;
        ULONG length;
        PPH_STRING name;
        BOOLEAN match;

        status = PhGetMappedClrColumnToken(ClrMetadata, PH_CLR_TABLE_CUSTOMATTRIBUTE,
            PH_CLR_CUSTOMATTRIBUTE_REC_COL_PARENT, i, NULL, NULL, &parent);
        if (!NT_SUCCESS(status)) return status;
        if (parent != ParentToken) continue;
        status = PhGetMappedClrCustomAttributeProps(ClrMetadata,
            (PH_CLR_TABLE_CUSTOMATTRIBUTE << 24) | i, NULL, &constructor, &data, &length);
        if (!NT_SUCCESS(status)) return status;
        if ((constructor >> 24) == PH_CLR_TABLE_MEMBERREF)
        {
            status = PhGetMappedClrColumnToken(ClrMetadata, PH_CLR_TABLE_MEMBERREF, 0,
                constructor & 0x00ffffff, NULL, NULL, &type);
            if (!NT_SUCCESS(status)) return status;
            if ((type >> 24) == PH_CLR_TABLE_METHODDEF)
                status = PhpGetClrMethodOwner(ClrMetadata, type, &type);
        }
        else
            status = PhpGetClrMethodOwner(ClrMetadata, constructor, &type);
        if (!NT_SUCCESS(status)) return status;
        if ((type >> 24) != PH_CLR_TABLE_TYPEDEF && (type >> 24) != PH_CLR_TABLE_TYPEREF)
            return STATUS_NOT_SUPPORTED;
        status = PhpGetClrTypeName(ClrMetadata, type, 0, &name);
        if (!NT_SUCCESS(status)) return status;
        match = PhEqualString2(name, Name, FALSE);
        PhDereferenceObject(name);
        if (match)
        {
            *Data = data;
            *Length = length;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_NOT_FOUND;
}

/** Resolves a FieldDef or MethodDef token to its P/Invoke mapping. */
NTSTATUS NTAPI PhGetMappedClrPinvokeMap(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Token,
    _Out_ PULONG MappingFlags,
    _Out_ PPH_STRING *ImportName,
    _Out_ PULONG ModuleToken
    )
{
    NTSTATUS status;

    if ((Token >> 24) != PH_CLR_TABLE_METHODDEF && (Token >> 24) != PH_CLR_TABLE_FIELD)
        return STATUS_INVALID_PARAMETER;
    status = PhpValidateClrToken(ClrMetadata, Token);
    if (!NT_SUCCESS(status)) return status;
    for (ULONG i = 1; i <= ClrMetadata->Tables[PH_CLR_TABLE_IMPLMAP].RowCount; i++)
    {
        ULONG member;
        ULONG flags;
        ULONG module;
        PPH_STRING name;

        status = PhGetMappedClrColumnToken(ClrMetadata, PH_CLR_TABLE_IMPLMAP,
            PH_CLR_IMPLMAP_REC_COL_MEMBERFORWARDED, i, NULL, NULL, &member);
        if (!NT_SUCCESS(status)) return status;
        if (member != Token) continue;
        status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_IMPLMAP,
            PH_CLR_IMPLMAP_REC_COL_MAPPINGFLAGS, i, &flags);
        if (!NT_SUCCESS(status)) return status;
        status = PhGetMappedClrColumnToken(ClrMetadata, PH_CLR_TABLE_IMPLMAP,
            PH_CLR_IMPLMAP_REC_COL_IMPORTSCOPE, i, NULL, NULL, &module);
        if (!NT_SUCCESS(status)) return status;
        status = PhpValidateClrToken(ClrMetadata, module);
        if (!NT_SUCCESS(status)) return status;
        status = PhGetMappedClrTableStringEx(ClrMetadata, PH_CLR_TABLE_IMPLMAP,
            i, PH_CLR_IMPLMAP_REC_COL_IMPORTNAME, &name);
        if (!NT_SUCCESS(status)) return status;
        *MappingFlags = flags;
        *ImportName = name;
        *ModuleToken = module;
        return STATUS_SUCCESS;
    }
    return STATUS_NOT_FOUND;
}

// An absent optional blob is represented by index zero, even with no #Blob stream.
static NTSTATUS PhpGetClrAssemblyBlob(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Table,
    _In_ ULONG Row,
    _In_ ULONG Column,
    _Out_ PVOID *Data,
    _Out_ PULONG Length
    )
{
    NTSTATUS status;
    ULONG index;

    status = PhGetMappedClrColumnValue(ClrMetadata, Table, Column, Row, &index);
    if (!NT_SUCCESS(status)) return status;
    if (index)
        return PhGetMappedClrBlob(ClrMetadata, index, Data, Length);
    *Data = NULL;
    *Length = 0;
    return STATUS_SUCCESS;
}

/** Releases owned assembly strings; borrowed blobs are never freed. */
VOID NTAPI PhDeleteMappedClrAssemblyProps(
    _Inout_ PPH_MAPPED_CLR_ASSEMBLY_PROPS Properties
    )
{
    if (Properties->Name) PhDereferenceObject(Properties->Name);
    if (Properties->Culture) PhDereferenceObject(Properties->Culture);
    memset(Properties, 0, sizeof(*Properties));
}

/** Retrieves assembly properties by token. Outputs are unchanged on failure. */
NTSTATUS NTAPI PhGetMappedClrAssemblyProps(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Token,
    _Out_ PPH_MAPPED_CLR_ASSEMBLY_PROPS Properties
    )
{
    NTSTATUS status;
    ULONG row = Token & 0x00ffffff;
    PH_MAPPED_CLR_ASSEMBLY_PROPS properties = { 0 };

    if ((Token >> 24) != PH_CLR_TABLE_ASSEMBLY)
        return STATUS_INVALID_PARAMETER;
    status = PhpValidateClrToken(ClrMetadata, Token);
    if (!NT_SUCCESS(status)) return status;
    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_ASSEMBLY,
        PH_CLR_ASSEMBLY_REC_COL_MAJORVERSION, row, &properties.MajorVersion);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_ASSEMBLY,
        PH_CLR_ASSEMBLY_REC_COL_MINORVERSION, row, &properties.MinorVersion);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_ASSEMBLY,
        PH_CLR_ASSEMBLY_REC_COL_BUILDNUMBER, row, &properties.BuildNumber);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_ASSEMBLY,
        PH_CLR_ASSEMBLY_REC_COL_REVISIONNUMBER, row, &properties.RevisionNumber);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_ASSEMBLY,
        PH_CLR_ASSEMBLY_REC_COL_FLAGS, row, &properties.Flags);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_ASSEMBLY,
        PH_CLR_ASSEMBLY_REC_COL_HASHALGID, row, &properties.HashAlgorithm);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = PhpGetClrAssemblyBlob(ClrMetadata, PH_CLR_TABLE_ASSEMBLY, row,
        PH_CLR_ASSEMBLY_REC_COL_PUBLICKEY, &properties.PublicKeyOrToken, &properties.PublicKeyOrTokenLength);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = PhGetMappedClrTableStringEx(ClrMetadata, PH_CLR_TABLE_ASSEMBLY, row,
        PH_CLR_ASSEMBLY_REC_COL_NAME, &properties.Name);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = PhGetMappedClrTableStringEx(ClrMetadata, PH_CLR_TABLE_ASSEMBLY, row,
        PH_CLR_ASSEMBLY_REC_COL_CULTURE, &properties.Culture);
    if (!NT_SUCCESS(status)) goto Cleanup;
    *Properties = properties;
    return STATUS_SUCCESS;

Cleanup:
    PhDeleteMappedClrAssemblyProps(&properties);
    return status;
}

/** Retrieves assembly properties by token. Outputs are unchanged on failure. */
NTSTATUS NTAPI PhGetMappedClrAssemblyRefProps(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Token,
    _Out_ PPH_MAPPED_CLR_ASSEMBLY_PROPS Properties
    )
{
    NTSTATUS status;
    ULONG row = Token & 0x00ffffff;
    PH_MAPPED_CLR_ASSEMBLY_PROPS properties = { 0 };

    if ((Token >> 24) != PH_CLR_TABLE_ASSEMBLYREF)
        return STATUS_INVALID_PARAMETER;
    status = PhpValidateClrToken(ClrMetadata, Token);
    if (!NT_SUCCESS(status)) return status;
    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_ASSEMBLYREF,
        PH_CLR_ASSEMBLYREF_REC_COL_MAJORVERSION, row, &properties.MajorVersion);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_ASSEMBLYREF,
        PH_CLR_ASSEMBLYREF_REC_COL_MINORVERSION, row, &properties.MinorVersion);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_ASSEMBLYREF,
        PH_CLR_ASSEMBLYREF_REC_COL_BUILDNUMBER, row, &properties.BuildNumber);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_ASSEMBLYREF,
        PH_CLR_ASSEMBLYREF_REC_COL_REVISIONNUMBER, row, &properties.RevisionNumber);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_ASSEMBLYREF,
        PH_CLR_ASSEMBLYREF_REC_COL_FLAGS, row, &properties.Flags);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = PhpGetClrAssemblyBlob(ClrMetadata, PH_CLR_TABLE_ASSEMBLYREF, row,
        PH_CLR_ASSEMBLYREF_REC_COL_PUBLICKEYORTOKEN, &properties.PublicKeyOrToken, &properties.PublicKeyOrTokenLength);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = PhpGetClrAssemblyBlob(ClrMetadata, PH_CLR_TABLE_ASSEMBLYREF, row,
        PH_CLR_ASSEMBLYREF_REC_COL_HASHVALUE, &properties.HashValue, &properties.HashValueLength);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = PhGetMappedClrTableStringEx(ClrMetadata, PH_CLR_TABLE_ASSEMBLYREF, row,
        PH_CLR_ASSEMBLYREF_REC_COL_NAME, &properties.Name);
    if (!NT_SUCCESS(status)) goto Cleanup;
    status = PhGetMappedClrTableStringEx(ClrMetadata, PH_CLR_TABLE_ASSEMBLYREF, row,
        PH_CLR_ASSEMBLYREF_REC_COL_CULTURE, &properties.Culture);
    if (!NT_SUCCESS(status)) goto Cleanup;
    *Properties = properties;
    return STATUS_SUCCESS;

Cleanup:
    PhDeleteMappedClrAssemblyProps(&properties);
    return status;
}

NTSTATUS NTAPI PhGetMappedPortablePdbInfo(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _Out_ PPH_PORTABLE_PDB_INFO Info
    )
{
    PH_PORTABLE_PDB_INFO info = { 0 };
    PUCHAR data = ClrMetadata->PdbStream;
    ULONG count;
    ULONG next = 0;

    if (!data || ClrMetadata->PdbStreamSize < 32)
        return STATUS_INVALID_IMAGE_FORMAT;
    __try
    {
        PhMappedImageProbe(ClrMetadata->MappedImage, data, ClrMetadata->PdbStreamSize);
        memcpy(info.Id, data, sizeof(info.Id));
        memcpy(&info.EntryPoint, data + 20, sizeof(ULONG));
        memcpy(&info.ReferencedTables, data + 24, sizeof(ULONG64));
        // Only schema-defined type-system tables may be referenced by a standalone PDB.
        for (ULONG i = 0; i < PH_CLR_TABLE_MAXIMUM; i++)
        {
            if ((info.ReferencedTables & ((ULONG64)1 << i)) &&
                (i >= PH_CLR_TABLE_DOCUMENT || !PhClrTableSchema[i].Name))
                return STATUS_INVALID_IMAGE_FORMAT;
        }
        count = PhCountBitsUlong64(info.ReferencedTables);
        if (count > (ClrMetadata->PdbStreamSize - 32) / sizeof(ULONG))
            return STATUS_INVALID_IMAGE_FORMAT;
        for (ULONG i = 0; i < PH_CLR_TABLE_MAXIMUM; i++)
        {
            if (info.ReferencedTables & ((ULONG64)1 << i))
            {
                memcpy(&info.RowCounts[i], data + 32 + next++ * sizeof(ULONG), sizeof(ULONG));
                if (info.RowCounts[i] > 0x00ffffff)
                    return STATUS_INVALID_IMAGE_FORMAT;
            }
        }
        if (info.EntryPoint && ((info.EntryPoint >> 24) != PH_CLR_TABLE_METHODDEF ||
            !(info.EntryPoint & 0x00ffffff) ||
            (info.EntryPoint & 0x00ffffff) > info.RowCounts[PH_CLR_TABLE_METHODDEF]))
            return STATUS_INVALID_IMAGE_FORMAT;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }
    *Info = info;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PhInitializeMappedPortablePdb(
    _Out_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ SIZE_T Length
    )
{
    NTSTATUS status;
    PH_MAPPED_CLR_METADATA metadata = { 0 };
    PH_PORTABLE_PDB_INFO info;

    if (!Buffer || Length < sizeof(PH_CLR_STORAGESIGNATURE) || Length > MAXULONG)
        return STATUS_INVALID_PARAMETER;
    metadata.PortablePdbView.ViewBase = Buffer;
    metadata.PortablePdbView.ViewSize = Length;
    metadata.MappedImage = &metadata.PortablePdbView;
    metadata.MetadataAddress = Buffer;
    metadata.MetadataSize = (ULONG)Length;
    status = PhClrParseStreams(&metadata);
    if (!NT_SUCCESS(status)) return status;
    if (metadata.Uncompressed) return STATUS_INVALID_IMAGE_FORMAT;
    status = PhGetMappedPortablePdbInfo(&metadata, &info);
    if (!NT_SUCCESS(status)) return status;
    memcpy(metadata.ExternalRowCounts, info.RowCounts, sizeof(info.RowCounts));
    status = PhClrParseTables(&metadata);
    if (!NT_SUCCESS(status)) return status;
    if (metadata.Tables[PH_CLR_TABLE_METHODDEBUGINFORMATION].RowCount &&
        metadata.Tables[PH_CLR_TABLE_METHODDEBUGINFORMATION].RowCount != info.RowCounts[PH_CLR_TABLE_METHODDEF])
        return STATUS_INVALID_IMAGE_FORMAT;
    *ClrMetadata = metadata;
    ClrMetadata->MappedImage = &ClrMetadata->PortablePdbView;
    return STATUS_SUCCESS;
}

// All reads are bounded and made under the caller's SEH handler.
static NTSTATUS PhpReadPdbInteger(
    _Inout_ PUCHAR *Cursor,
    _In_ PUCHAR End,
    _In_ BOOLEAN Signed,
    _Out_ PLONG64 Value
    )
{
    NTSTATUS status;
    ULONG value;
    ULONG bytes;
    LONG decoded;

    status = PhClrUncompressData(*Cursor, End, &value, &bytes);
    if (!NT_SUCCESS(status)) return status;
    *Cursor += bytes;
    if (Signed)
    {
        decoded = (LONG)(value >> 1);
        if (value & 1)
            decoded |= bytes == 1 ? ~0x3f : bytes == 2 ? ~0x1fff : ~0x0fffffff;
        *Value = decoded;
    }
    else
        *Value = value;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PhGetMappedClrDocument(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG DocumentRid,
    _Out_ PPH_CLR_DOCUMENT Document
    )
{
    NTSTATUS status;
    PH_CLR_DOCUMENT document = { 0 };
    ULONG nameIndex;
    ULONG index;
    ULONG length;
    PVOID data;
    PPH_STRING name = NULL;
    PPH_STRING part = NULL;
    PH_STRING_BUILDER builder;
    PUCHAR cursor;
    PUCHAR end;
    CHAR separator[4];
    ULONG separatorLength;
    PPH_STRING separatorString = NULL;
    BOOLEAN first = TRUE;

    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_DOCUMENT,
        PH_CLR_DOCUMENT_REC_COL_NAME, DocumentRid, &nameIndex);
    if (!NT_SUCCESS(status)) return status;
    if (!nameIndex) return STATUS_INVALID_IMAGE_FORMAT;
    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_DOCUMENT,
        PH_CLR_DOCUMENT_REC_COL_HASHALGORITHM, DocumentRid, &index);
    if (!NT_SUCCESS(status)) return status;
    if (index)
    {
        status = PhGetMappedClrGuid(ClrMetadata, index, &document.HashAlgorithm);
        if (!NT_SUCCESS(status)) return status;
    }
    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_DOCUMENT,
        PH_CLR_DOCUMENT_REC_COL_LANGUAGE, DocumentRid, &index);
    if (!NT_SUCCESS(status)) return status;
    if (index)
    {
        status = PhGetMappedClrGuid(ClrMetadata, index, &document.Language);
        if (!NT_SUCCESS(status)) return status;
    }
    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_DOCUMENT,
        PH_CLR_DOCUMENT_REC_COL_HASH, DocumentRid, &index);
    if (!NT_SUCCESS(status)) return status;
    if (index)
    {
        status = PhGetMappedClrBlob(ClrMetadata, index, &document.Hash, &document.HashLength);
        if (!NT_SUCCESS(status)) return status;
    }
    status = PhGetMappedClrBlob(ClrMetadata, nameIndex, &data, &length);
    if (!NT_SUCCESS(status)) return status;
    if (length < 2) return STATUS_INVALID_IMAGE_FORMAT;
    PhInitializeStringBuilder(&builder, 128);
    __try
    {
        cursor = data;
        end = cursor + length;
        separatorLength = *cursor < 0x80 ? 1 : (*cursor & 0xe0) == 0xc0 ? 2 :
            (*cursor & 0xf0) == 0xe0 ? 3 : (*cursor & 0xf8) == 0xf0 ? 4 : 0;
        if (!separatorLength || separatorLength >= length)
        {
            status = STATUS_INVALID_IMAGE_FORMAT;
            __leave;
        }
        memcpy(separator, cursor, separatorLength);
        cursor += separatorLength;
        separatorString = PhConvertUtf8ToUtf16Ex(separator, separator[0] ? separatorLength : 0);
        if (!separatorString)
        {
            status = STATUS_INVALID_IMAGE_FORMAT;
            __leave;
        }
        while (cursor < end)
        {
            LONG64 value;
            SIZE_T utf16Bytes;

            status = PhpReadPdbInteger(&cursor, end, FALSE, &value);
            if (!NT_SUCCESS(status)) break;
            if (!first) PhAppendStringBuilder(&builder, &separatorString->sr);
            first = FALSE;
            if (!value) continue;
            status = PhGetMappedClrBlob(ClrMetadata, (ULONG)value, &data, &length);
            if (!NT_SUCCESS(status)) break;
            status = PhConvertUtf8ToUtf16Size(&utf16Bytes, data, length);
            if (!NT_SUCCESS(status)) break;
            part = PhCreateStringEx(NULL, utf16Bytes);
            status = PhConvertUtf8ToUtf16Buffer(part->Buffer, utf16Bytes, &utf16Bytes, data, length);
            if (!NT_SUCCESS(status)) break;
            part->Length = utf16Bytes;
            part->Buffer[utf16Bytes / sizeof(WCHAR)] = UNICODE_NULL;
            PhAppendStringBuilder(&builder, &part->sr);
            PhClearReference(&part);
        }
        if (NT_SUCCESS(status)) name = PhReferenceObject(builder.String);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }
    if (part) PhDereferenceObject(part);
    if (separatorString) PhDereferenceObject(separatorString);
    PhDeleteStringBuilder(&builder);
    if (!NT_SUCCESS(status)) return status;
    document.Name = name;
    *Document = document;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PhEnumMappedClrSequencePoints(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG MethodToken,
    _In_ PPH_CLR_SEQUENCE_POINT_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    ULONG rid = MethodToken & 0x00ffffff;
    ULONG document;
    ULONG index;
    ULONG length;
    PVOID data;
    PUCHAR cursor;
    PUCHAR end;
    LONG64 value;
    LONG64 offset = 0;
    LONG64 line = 0;
    LONG64 column = 0;
    BOOLEAN first = TRUE;
    BOOLEAN firstVisible = TRUE;
    BOOLEAN pendingDocument = FALSE;

    if ((MethodToken >> 24) != PH_CLR_TABLE_METHODDEF || !rid || !Callback)
        return STATUS_INVALID_PARAMETER;
    if (rid > ClrMetadata->Tables[PH_CLR_TABLE_METHODDEBUGINFORMATION].RowCount)
        return STATUS_NOT_FOUND;
    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_METHODDEBUGINFORMATION,
        PH_CLR_METHODDEBUGINFORMATION_REC_COL_DOCUMENT, rid, &document);
    if (!NT_SUCCESS(status)) return status;
    status = PhGetMappedClrColumnValue(ClrMetadata, PH_CLR_TABLE_METHODDEBUGINFORMATION,
        PH_CLR_METHODDEBUGINFORMATION_REC_COL_SEQUENCEPOINTS, rid, &index);
    if (!NT_SUCCESS(status)) return status;
    if (!index) return STATUS_NOT_FOUND;
    status = PhGetMappedClrBlob(ClrMetadata, index, &data, &length);
    if (!NT_SUCCESS(status)) return status;
    cursor = data;
    end = cursor + length;
    __try
    {
        status = PhpReadPdbInteger(&cursor, end, FALSE, &value);
        if (!NT_SUCCESS(status)) return status;
        if (value > max(ClrMetadata->ExternalRowCounts[PH_CLR_TABLE_STANDALONESIG],
            ClrMetadata->Tables[PH_CLR_TABLE_STANDALONESIG].RowCount))
            return STATUS_INVALID_IMAGE_FORMAT;
        if (!document)
        {
            status = PhpReadPdbInteger(&cursor, end, FALSE, &value);
            if (!NT_SUCCESS(status)) return status;
            document = (ULONG)value;
        }
        if (!document || document > ClrMetadata->Tables[PH_CLR_TABLE_DOCUMENT].RowCount)
            return STATUS_INVALID_IMAGE_FORMAT;
        while (cursor < end)
        {
            PH_CLR_SEQUENCE_POINT point = { 0 };
            LONG64 deltaLines;
            LONG64 deltaColumns;
            LONG64 delta;

            status = PhpReadPdbInteger(&cursor, end, FALSE, &delta);
            if (!NT_SUCCESS(status)) return status;
            if (!first && !delta)
            {
                if (pendingDocument) return STATUS_INVALID_IMAGE_FORMAT;
                status = PhpReadPdbInteger(&cursor, end, FALSE, &value);
                if (!NT_SUCCESS(status)) return status;
                if (!value || value > ClrMetadata->Tables[PH_CLR_TABLE_DOCUMENT].RowCount)
                    return STATUS_INVALID_IMAGE_FORMAT;
                document = (ULONG)value;
                pendingDocument = TRUE;
                continue;
            }
            offset += delta;
            if (offset >= 0x20000000) return STATUS_INVALID_IMAGE_FORMAT;
            status = PhpReadPdbInteger(&cursor, end, FALSE, &deltaLines);
            if (!NT_SUCCESS(status)) return status;
            status = PhpReadPdbInteger(&cursor, end, deltaLines != 0, &deltaColumns);
            if (!NT_SUCCESS(status)) return status;
            point.IlOffset = (ULONG)offset;
            point.Document = document;
            point.Hidden = !deltaLines && !deltaColumns;
            if (point.Hidden)
                point.StartLine = point.EndLine = PH_CLR_HIDDEN_LINE;
            else
            {
                status = PhpReadPdbInteger(&cursor, end, !firstVisible, &value);
                if (!NT_SUCCESS(status)) return status;
                line = firstVisible ? value : line + value;
                status = PhpReadPdbInteger(&cursor, end, !firstVisible, &value);
                if (!NT_SUCCESS(status)) return status;
                column = firstVisible ? value : column + value;
                if (line < 0 || line >= 0x20000000 || line == PH_CLR_HIDDEN_LINE ||
                    line + deltaLines >= 0x20000000 || line + deltaLines == PH_CLR_HIDDEN_LINE ||
                    column < 0 || column >= 0x10000 || column + deltaColumns < 0 ||
                    column + deltaColumns >= 0x10000 || (!deltaLines && deltaColumns <= 0))
                    return STATUS_INVALID_IMAGE_FORMAT;
                point.StartLine = (ULONG)line;
                point.EndLine = (ULONG)(line + deltaLines);
                point.StartColumn = (ULONG)column;
                point.EndColumn = (ULONG)(column + deltaColumns);
                firstVisible = FALSE;
            }
            first = FALSE;
            pendingDocument = FALSE;
            if (!Callback(&point, Context)) return STATUS_SUCCESS;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }
    return first || pendingDocument ? STATUS_INVALID_IMAGE_FORMAT : STATUS_SUCCESS;
}

typedef struct _PH_CLR_SOURCE_LOOKUP
{
    ULONG IlOffset;
    BOOLEAN Found;
    PH_CLR_SEQUENCE_POINT Point;
} PH_CLR_SOURCE_LOOKUP, *PPH_CLR_SOURCE_LOOKUP;

static BOOLEAN NTAPI PhpFindClrSequencePoint(
    _In_ PPH_CLR_SEQUENCE_POINT Point,
    _In_opt_ PVOID Context
    )
{
    PPH_CLR_SOURCE_LOOKUP lookup = Context;

    if (Point->IlOffset <= lookup->IlOffset)
    {
        lookup->Point = *Point;
        lookup->Found = TRUE;
    }
    return TRUE;
}

NTSTATUS NTAPI PhGetMappedClrSourceLocation(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG MethodToken,
    _In_ ULONG IlOffset,
    _Out_ PPH_CLR_SOURCE_LOCATION Location
    )
{
    NTSTATUS status;
    PH_CLR_SOURCE_LOOKUP lookup = { 0 };
    PH_CLR_SOURCE_LOCATION location = { 0 };
    PH_CLR_DOCUMENT document;

    if (IlOffset >= 0x20000000) return STATUS_INVALID_PARAMETER;
    lookup.IlOffset = IlOffset;
    status = PhEnumMappedClrSequencePoints(ClrMetadata, MethodToken, PhpFindClrSequencePoint, &lookup);
    if (!NT_SUCCESS(status)) return status;
    if (!lookup.Found) return STATUS_NOT_FOUND;
    location.Point = lookup.Point;
    if (!lookup.Point.Hidden)
    {
        status = PhGetMappedClrDocument(ClrMetadata, lookup.Point.Document, &document);
        if (!NT_SUCCESS(status)) return status;
        location.FileName = document.Name;
    }
    *Location = location;
    return STATUS_SUCCESS;
}
