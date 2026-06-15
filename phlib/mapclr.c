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
#include <mapclr.h>

#define PH_CLR_STORAGE_MAGIC 0x424A5342 // "BSJB"

// CorHdr.h import flag helpers, kept local to avoid a peview dependency.
#define PM_NO_MANGLE                    0x0001
#define PM_CHARSET_ANSI                 0x0002
#define PM_CHARSET_UNICODE              0x0004
#define PM_CHARSET_AUTO                 0x0006
#define PM_SUPPRESS_GC_TRANSITION       0x0008
#define PM_SUPPORTS_LAST_ERROR          0x0040
#define PM_CALLCONV_WINAPI              0x0100
#define PM_CALLCONV_CDECL               0x0200
#define PM_CALLCONV_STDCALL             0x0300
#define PM_CALLCONV_THISCALL            0x0400
#define PM_CALLCONV_FASTCALL            0x0500
#define PM_BEST_FIT_ENABLED              0x0010
#define PM_BEST_FIT_DISABLED             0x0020
#define PM_BEST_FIT_MASK                 0x0030
#define PM_THROW_ON_UNMAPPABLE_CHAR_ENABLED  0x1000
#define PM_THROW_ON_UNMAPPABLE_CHAR_DISABLED 0x2000
#define PM_THROW_ON_UNMAPPABLE_CHAR_MASK     0x3000

#define PM_CHARSET_MASK                 0x0006
#define PM_CALLCONV_MASK                0x0700

static BOOLEAN PhPmNoMangle(_In_ ULONG Flags) { return (Flags & PM_NO_MANGLE) != 0; }
static BOOLEAN PhPmCharSetAnsi(_In_ ULONG Flags) { return (Flags & PM_CHARSET_MASK) == PM_CHARSET_ANSI; }
static BOOLEAN PhPmCharSetUnicode(_In_ ULONG Flags) { return (Flags & PM_CHARSET_MASK) == PM_CHARSET_UNICODE; }
static BOOLEAN PhPmCharSetAuto(_In_ ULONG Flags) { return (Flags & PM_CHARSET_MASK) == PM_CHARSET_AUTO; }
static BOOLEAN PhPmSupportsLastError(_In_ ULONG Flags) { return (Flags & PM_SUPPORTS_LAST_ERROR) != 0; }
static BOOLEAN PhPmCallConvWinapi(_In_ ULONG Flags) { return (Flags & PM_CALLCONV_MASK) == PM_CALLCONV_WINAPI; }
static BOOLEAN PhPmCallConvCdecl(_In_ ULONG Flags) { return (Flags & PM_CALLCONV_MASK) == PM_CALLCONV_CDECL; }
static BOOLEAN PhPmCallConvStdcall(_In_ ULONG Flags) { return (Flags & PM_CALLCONV_MASK) == PM_CALLCONV_STDCALL; }
static BOOLEAN PhPmCallConvThiscall(_In_ ULONG Flags) { return (Flags & PM_CALLCONV_MASK) == PM_CALLCONV_THISCALL; }
static BOOLEAN PhPmCallConvFastcall(_In_ ULONG Flags) { return (Flags & PM_CALLCONV_MASK) == PM_CALLCONV_FASTCALL; }
static BOOLEAN PhPmBestFitEnabled(_In_ ULONG Flags) { return (Flags & PM_BEST_FIT_MASK) == PM_BEST_FIT_ENABLED; }
static BOOLEAN PhPmBestFitDisabled(_In_ ULONG Flags) { return (Flags & PM_BEST_FIT_MASK) == PM_BEST_FIT_DISABLED; }
static BOOLEAN PhPmThrowOnUnmappableCharEnabled(_In_ ULONG Flags) { return (Flags & PM_THROW_ON_UNMAPPABLE_CHAR_MASK) == PM_THROW_ON_UNMAPPABLE_CHAR_ENABLED; }
static BOOLEAN PhPmThrowOnUnmappableCharDisabled(_In_ ULONG Flags) { return (Flags & PM_THROW_ON_UNMAPPABLE_CHAR_MASK) == PM_THROW_ON_UNMAPPABLE_CHAR_DISABLED; }

static NTSTATUS PhClrUncompressData(
    _In_ PVOID Pointer,
    _In_ PVOID End,
    _Out_ PULONG Value,
    _Out_ PULONG BytesRead
    );

// HeapSizes flag indicating an extra 4-byte field follows the row-count array.
#define PH_CLR_HEAPSIZE_EXTRADATA 0x40

#include <pshpack1.h>
typedef struct _PH_CLR_STORAGESIGNATURE
{
    ULONG Signature;
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG ExtraData;
    ULONG VersionLength;
    // CHAR VersionString[VersionLength];
} PH_CLR_STORAGESIGNATURE, *PPH_CLR_STORAGESIGNATURE;

typedef struct _PH_CLR_STORAGEHEADER
{
    UCHAR Flags;
    UCHAR Reserved;
    USHORT Streams;
} PH_CLR_STORAGEHEADER, *PPH_CLR_STORAGEHEADER;

typedef struct _PH_CLR_STORAGESTREAM
{
    ULONG Offset;
    ULONG Size;
    CHAR Name[1]; // null-terminated, padded to a 4-byte boundary
} PH_CLR_STORAGESTREAM, *PPH_CLR_STORAGESTREAM;

typedef struct _PH_CLR_TABLESHEADER
{
    ULONG Reserved;
    UCHAR Major;
    UCHAR Minor;
    UCHAR HeapSizes;
    UCHAR Rid;
    ULONG64 ValidMask;
    ULONG64 SortedMask;
    // ULONG RowCounts[popcount(ValidMask)];
} PH_CLR_TABLESHEADER, *PPH_CLR_TABLESHEADER;
#include <poppack.h>

// Schema authoring column codes (see PhClrTableSchema).
#define CC_I1   0x00
#define CC_I2   0x01
#define CC_I4   0x02
#define CC_STR  0x03 // #Strings index
#define CC_GUID 0x04 // #GUID index
#define CC_BLOB 0x05 // #Blob index
#define CC_TBL  0x40 // | table index (simple index into one table)
#define CC_COD  0x80 // | coded-index set id
#define CC_END  0xFF

// Coded-index sets (ECMA-335 II.24.2.6 + Portable PDB).
typedef enum _PH_CLR_CODED_INDEX
{
    CIX_TypeDefOrRef,
    CIX_HasConstant,
    CIX_HasCustomAttribute,
    CIX_HasFieldMarshall,
    CIX_HasDeclSecurity,
    CIX_MemberRefParent,
    CIX_HasSemantics,
    CIX_MethodDefOrRef,
    CIX_MemberForwarded,
    CIX_Implementation,
    CIX_CustomAttributeType,
    CIX_ResolutionScope,
    CIX_TypeOrMethodDef,
    CIX_HasCustomDebugInformation,
    CIX_MAXIMUM
} PH_CLR_CODED_INDEX;

#define CIX_RESERVED 0xFF
#define CIX_MAX_TABLES 28

typedef struct _PH_CLR_CODED_INDEX_INFO
{
    UCHAR Bits;
    UCHAR Count;
    UCHAR Tables[CIX_MAX_TABLES];
} PH_CLR_CODED_INDEX_INFO;

typedef struct _PH_CLR_TABLE_SCHEMA
{
    PCSTR Name;
    UCHAR Columns[PH_CLR_MAX_COLUMNS];
} PH_CLR_TABLE_SCHEMA;

#define T(x) (CC_TBL | PH_CLR_TABLE_##x)
#define C(x) (CC_COD | CIX_##x)

static const PH_CLR_CODED_INDEX_INFO PhClrCodedIndex[CIX_MAXIMUM] =
{
    [CIX_TypeDefOrRef] = { 2, 3, { PH_CLR_TABLE_TYPEDEF, PH_CLR_TABLE_TYPEREF, PH_CLR_TABLE_TYPESPEC } },
    [CIX_HasConstant] = { 2, 3, { PH_CLR_TABLE_FIELD, PH_CLR_TABLE_PARAM, PH_CLR_TABLE_PROPERTY } },
    [CIX_HasCustomAttribute] =
    { 5, 22, {
        PH_CLR_TABLE_METHODDEF, PH_CLR_TABLE_FIELD, PH_CLR_TABLE_TYPEREF, PH_CLR_TABLE_TYPEDEF,
        PH_CLR_TABLE_PARAM, PH_CLR_TABLE_INTERFACEIMPL, PH_CLR_TABLE_MEMBERREF, PH_CLR_TABLE_MODULE,
        PH_CLR_TABLE_DECLSECURITY, PH_CLR_TABLE_PROPERTY, PH_CLR_TABLE_EVENT, PH_CLR_TABLE_STANDALONESIG,
        PH_CLR_TABLE_MODULEREF, PH_CLR_TABLE_TYPESPEC, PH_CLR_TABLE_ASSEMBLY, PH_CLR_TABLE_ASSEMBLYREF,
        PH_CLR_TABLE_FILE, PH_CLR_TABLE_EXPORTEDTYPE, PH_CLR_TABLE_MANIFESTRESOURCE, PH_CLR_TABLE_GENERICPARAM,
        PH_CLR_TABLE_GENERICPARAMCONSTRAINT, PH_CLR_TABLE_METHODSPEC
    } },
    [CIX_HasFieldMarshall] = { 1, 2, { PH_CLR_TABLE_FIELD, PH_CLR_TABLE_PARAM } },
    [CIX_HasDeclSecurity] = { 2, 3, { PH_CLR_TABLE_TYPEDEF, PH_CLR_TABLE_METHODDEF, PH_CLR_TABLE_ASSEMBLY } },
    [CIX_MemberRefParent] = { 3, 5, { PH_CLR_TABLE_TYPEDEF, PH_CLR_TABLE_TYPEREF, PH_CLR_TABLE_MODULEREF, PH_CLR_TABLE_METHODDEF,  PH_CLR_TABLE_TYPESPEC } },
    [CIX_HasSemantics] = { 1, 2, { PH_CLR_TABLE_EVENT, PH_CLR_TABLE_PROPERTY } },
    [CIX_MethodDefOrRef] = { 1, 2, { PH_CLR_TABLE_METHODDEF, PH_CLR_TABLE_MEMBERREF } },
    [CIX_MemberForwarded] = { 1, 2, { PH_CLR_TABLE_FIELD, PH_CLR_TABLE_METHODDEF } },
    [CIX_Implementation] = { 2, 3, { PH_CLR_TABLE_FILE, PH_CLR_TABLE_ASSEMBLYREF, PH_CLR_TABLE_EXPORTEDTYPE } },
    [CIX_CustomAttributeType] = { 3, 5, { CIX_RESERVED, CIX_RESERVED, PH_CLR_TABLE_METHODDEF, PH_CLR_TABLE_MEMBERREF, CIX_RESERVED } },
    [CIX_ResolutionScope] = { 2, 4, { PH_CLR_TABLE_MODULE, PH_CLR_TABLE_MODULEREF, PH_CLR_TABLE_ASSEMBLYREF, PH_CLR_TABLE_TYPEREF } },
    [CIX_TypeOrMethodDef] = { 1, 2, { PH_CLR_TABLE_TYPEDEF, PH_CLR_TABLE_METHODDEF } },
    [CIX_HasCustomDebugInformation] =
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
    [PH_CLR_TABLE_TYPEREF] = { "TypeRef", { C(ResolutionScope), CC_STR, CC_STR, CC_END } },
    [PH_CLR_TABLE_TYPEDEF] = { "TypeDef", { CC_I4, CC_STR, CC_STR, C(TypeDefOrRef), T(FIELD), T(METHODDEF), CC_END } },
    [PH_CLR_TABLE_FIELDPTR] = { "FieldPtr", { T(FIELD), CC_END } },
    [PH_CLR_TABLE_FIELD] = { "Field", { CC_I2, CC_STR, CC_BLOB, CC_END } },
    [PH_CLR_TABLE_METHODPTR] = { "MethodPtr", { T(METHODDEF), CC_END } },
    [PH_CLR_TABLE_METHODDEF] = { "MethodDef", { CC_I4, CC_I2, CC_I2, CC_STR, CC_BLOB, T(PARAM), CC_END } },
    [PH_CLR_TABLE_PARAMPTR] = { "ParamPtr", { T(PARAM), CC_END } },
    [PH_CLR_TABLE_PARAM] = { "Param", { CC_I2, CC_I2, CC_STR, CC_END } },
    [PH_CLR_TABLE_INTERFACEIMPL] = { "InterfaceImpl", { T(TYPEDEF), C(TypeDefOrRef), CC_END } },
    [PH_CLR_TABLE_MEMBERREF] = { "MemberRef", { C(MemberRefParent), CC_STR, CC_BLOB, CC_END } },
    [PH_CLR_TABLE_CONSTANT] = { "Constant", { CC_I2, C(HasConstant), CC_BLOB, CC_END } },
    [PH_CLR_TABLE_CUSTOMATTRIBUTE] = { "CustomAttribute", { C(HasCustomAttribute), C(CustomAttributeType), CC_BLOB, CC_END } },
    [PH_CLR_TABLE_FIELDMARSHAL] = { "FieldMarshal", { C(HasFieldMarshall), CC_BLOB, CC_END } },
    [PH_CLR_TABLE_DECLSECURITY] = { "DeclSecurity", { CC_I2, C(HasDeclSecurity), CC_BLOB, CC_END } },
    [PH_CLR_TABLE_CLASSLAYOUT] = { "ClassLayout", { CC_I2, CC_I4, T(TYPEDEF), CC_END } },
    [PH_CLR_TABLE_FIELDLAYOUT] = { "FieldLayout", { CC_I4, T(FIELD), CC_END } },
    [PH_CLR_TABLE_STANDALONESIG] = { "StandAloneSig", { CC_BLOB, CC_END } },
    [PH_CLR_TABLE_EVENTMAP] = { "EventMap", { T(TYPEDEF), T(EVENT), CC_END } },
    [PH_CLR_TABLE_EVENTPTR] = { "EventPtr", { T(EVENT), CC_END } },
    [PH_CLR_TABLE_EVENT] = { "Event", { CC_I2, CC_STR, C(TypeDefOrRef), CC_END } },
    [PH_CLR_TABLE_PROPERTYMAP] = { "PropertyMap", { T(TYPEDEF), T(PROPERTY), CC_END } },
    [PH_CLR_TABLE_PROPERTYPTR] = { "PropertyPtr", { T(PROPERTY), CC_END } },
    [PH_CLR_TABLE_PROPERTY] = { "Property", { CC_I2, CC_STR, CC_BLOB, CC_END } },
    [PH_CLR_TABLE_METHODSEMANTICS] = { "MethodSemantics", { CC_I2, T(METHODDEF), C(HasSemantics), CC_END } },
    [PH_CLR_TABLE_METHODIMPL] = { "MethodImpl", { T(TYPEDEF), C(MethodDefOrRef), C(MethodDefOrRef), CC_END } },
    [PH_CLR_TABLE_MODULEREF] = { "ModuleRef", { CC_STR, CC_END } },
    [PH_CLR_TABLE_TYPESPEC] = { "TypeSpec", { CC_BLOB, CC_END } },
    [PH_CLR_TABLE_IMPLMAP] = { "ImplMap", { CC_I2, C(MemberForwarded), CC_STR, T(MODULEREF), CC_END } },
    [PH_CLR_TABLE_FIELDRVA] = { "FieldRVA", { CC_I4, T(FIELD), CC_END } },
    [PH_CLR_TABLE_ENCLOG] = { "ENCLog", { CC_I4, CC_I4, CC_END } },
    [PH_CLR_TABLE_ENCMAP] = { "ENCMap", { CC_I4, CC_END } },
    [PH_CLR_TABLE_ASSEMBLY] = { "Assembly", { CC_I4, CC_I2, CC_I2, CC_I2, CC_I2, CC_I4, CC_BLOB, CC_STR, CC_STR } },
    [PH_CLR_TABLE_ASSEMBLYPROCESSOR] = { "AssemblyProcessor", { CC_I4, CC_END } },
    [PH_CLR_TABLE_ASSEMBLYOS] = { "AssemblyOS", { CC_I4, CC_I4, CC_I4, CC_END } },
    [PH_CLR_TABLE_ASSEMBLYREF] = { "AssemblyRef", { CC_I2, CC_I2, CC_I2, CC_I2, CC_I4, CC_BLOB, CC_STR, CC_STR, CC_BLOB } },
    [PH_CLR_TABLE_ASSEMBLYREFPROCESSOR] = { "AssemblyRefProcessor", { CC_I4, T(ASSEMBLYREF), CC_END } },
    [PH_CLR_TABLE_ASSEMBLYREFOS] = { "AssemblyRefOS", { CC_I4, CC_I4, CC_I4, T(ASSEMBLYREF), CC_END } },
    [PH_CLR_TABLE_FILE] = { "File", { CC_I4, CC_STR, CC_BLOB, CC_END } },
    [PH_CLR_TABLE_EXPORTEDTYPE] = { "ExportedType", { CC_I4, CC_I4, CC_STR, CC_STR, C(Implementation), CC_END } },
    [PH_CLR_TABLE_MANIFESTRESOURCE] = { "ManifestResource", { CC_I4, CC_I4, CC_STR, C(Implementation), CC_END } },
    [PH_CLR_TABLE_NESTEDCLASS] = { "NestedClass", { T(TYPEDEF), T(TYPEDEF), CC_END } },
    [PH_CLR_TABLE_GENERICPARAM] = { "GenericParam", { CC_I2, CC_I2, C(TypeOrMethodDef), CC_STR, CC_END } },
    [PH_CLR_TABLE_METHODSPEC] = { "MethodSpec", { C(MethodDefOrRef), CC_BLOB, CC_END } },
    [PH_CLR_TABLE_GENERICPARAMCONSTRAINT] = { "GenericParamConstraint", { T(GENERICPARAM), C(TypeDefOrRef), CC_END } },
    // Portable PDB
    [PH_CLR_TABLE_DOCUMENT] = { "Document", { CC_BLOB, CC_GUID, CC_BLOB, CC_GUID, CC_END } },
    [PH_CLR_TABLE_METHODDEBUGINFORMATION] = { "MethodDebugInformation", { T(DOCUMENT), CC_BLOB, CC_END } },
    [PH_CLR_TABLE_LOCALSCOPE] = { "LocalScope", { T(METHODDEF), T(IMPORTSCOPE), T(LOCALVARIABLE), T(LOCALCONSTANT), CC_I4, CC_I4, CC_END } },
    [PH_CLR_TABLE_LOCALVARIABLE] = { "LocalVariable", { CC_I2, CC_I2, CC_STR, CC_END } },
    [PH_CLR_TABLE_LOCALCONSTANT] = { "LocalConstant", { CC_STR, CC_BLOB, CC_END } },
    [PH_CLR_TABLE_IMPORTSCOPE] = { "ImportScope", { T(IMPORTSCOPE), CC_BLOB, CC_END } },
    [PH_CLR_TABLE_STATEMACHINEMETHOD] = { "StateMachineMethod", { T(METHODDEF), T(METHODDEF), CC_END } },
    [PH_CLR_TABLE_CUSTOMDEBUGINFORMATION] = { "CustomDebugInformation", { C(HasCustomDebugInformation), CC_GUID, CC_BLOB, CC_END } },
};

#undef T
#undef C

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

// Reads a compressed unsigned integer (ECMA-335 II.23.2) with bounds checking.
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

NTSTATUS PhClrGetTableString(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Row,
    _In_ ULONG Column,
    _Out_ PPH_STRING* String
    )
{
    NTSTATUS status;
    ULONG index = 0;
    const char* string = NULL;

    status = PhGetMappedClrColumnValue(ClrMetadata, TableIndex, Column, Row, &index);

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetMappedClrString(ClrMetadata, index, &string);

    if (!NT_SUCCESS(status))
        return status;

    *String = PhConvertUtf8ToUtf16(string);
    return STATUS_SUCCESS;
}

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

    status = PhClrGetTableString(
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
        status = PhClrGetTableString(ClrMetadata, PH_CLR_TABLE_TYPEDEF, parentRid >> 2, 1, &typeName);
        break;
    case 1:
        status = PhClrGetTableString(ClrMetadata, PH_CLR_TABLE_TYPEREF, parentRid >> 2, 1, &typeName);
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

PPH_STRING NTAPI PhGetMappedClrTableString(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Row,
    _In_ ULONG Column
    )
{
    PPH_STRING string = NULL;

    if (NT_SUCCESS(PhClrGetTableString(ClrMetadata, TableIndex, Row, Column, &string)))
        return string;

    return NULL;
}

BOOLEAN NTAPI PhTryGetMappedClrTargetFramework(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _Out_ PPH_STRING* Version
    )
{
    NTSTATUS status;
    ULONG tableCount = 0;

    *Version = NULL;

    if (NT_SUCCESS(PhGetMappedClrTableInfo(ClrMetadata, PH_CLR_TABLE_CUSTOMATTRIBUTE, NULL, &tableCount, NULL, NULL)))
    {
        for (ULONG i = 1; i <= tableCount; i++)
        {
            PVOID attributeData = NULL;
            ULONG attributeIndex = 0;
            ULONG valueLength = 0;
            USHORT prolog;
            ULONG skipLength = 0;
            ULONG stringLength = 0;
            PPH_STRING version = NULL;
            BOOLEAN isTargetFramework = FALSE;

            status = PhGetMappedClrCustomAttributeTargetName(ClrMetadata, i, &isTargetFramework);

            if (!NT_SUCCESS(status) || !isTargetFramework)
                continue;

            status = PhGetMappedClrColumnValue(
                ClrMetadata,
                PH_CLR_TABLE_CUSTOMATTRIBUTE,
                2,
                i,
                &attributeIndex
                );

            if (!NT_SUCCESS(status))
                continue;

            status = PhGetMappedClrBlob(
                ClrMetadata,
                attributeIndex,
                &attributeData,
                &valueLength
                );

            if (!NT_SUCCESS(status) || valueLength < sizeof(USHORT) + 1)
                continue;

            __try
            {
                prolog = *(PUSHORT)attributeData;

                if (prolog != 0x0001)
                    continue;

                if (!NT_SUCCESS(PhClrUncompressData(
                    PTR_ADD_OFFSET(attributeData, sizeof(USHORT)),
                    PTR_ADD_OFFSET(attributeData, valueLength),
                    &stringLength,
                    &skipLength
                    )))
                {
                    continue;
                }

                if (stringLength == 0 || stringLength >= valueLength)
                    continue;

                version = PhConvertUtf8ToUtf16Ex((PCHAR)PTR_ADD_OFFSET(attributeData, sizeof(USHORT) + skipLength), stringLength);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                version = NULL;
            }

            if (version)
            {
                *Version = version;
                return TRUE;
            }
        }
    }

    return FALSE;
}

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
            if (ClrMetadata->Tables[table].RowCount > maxRows)
                maxRows = ClrMetadata->Tables[table].RowCount;
        }
    }

    if (maxRows < ((ULONG)1 << (16 - info->Bits)))
        return 2;

    return 4;
}

// Computes the per-column layout (offset, size, type) and total row size for a
// table from the global heap/index widths. Requires every table's RowCount to
// already be populated.
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

        if (code & CC_COD)
        {
            type = PH_CLR_COLUMN_CODED;
            codec = code & ~CC_COD;
            size = PhClrCodedIndexSize(ClrMetadata, codec);
        }
        else if (code & CC_TBL)
        {
            type = PH_CLR_COLUMN_TABLE;
            codec = code & ~CC_TBL;
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

NTSTATUS PhClrParseStreams(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata
    )
{
    PPH_MAPPED_IMAGE mappedImage = ClrMetadata->MappedImage;
    PPH_CLR_STORAGESIGNATURE signature = ClrMetadata->MetadataAddress;
    PPH_CLR_STORAGEHEADER header;
    PPH_CLR_STORAGESTREAM stream;
    ULONG streamCount;

    __try
    {
        PhMappedImageProbe(mappedImage, signature, sizeof(PH_CLR_STORAGESIGNATURE));

        if (signature->Signature != PH_CLR_STORAGE_MAGIC)
            return STATUS_INVALID_IMAGE_FORMAT;

        header = PTR_ADD_OFFSET(signature, RTL_SIZEOF_THROUGH_FIELD(PH_CLR_STORAGESIGNATURE, VersionLength) + ALIGN_UP(signature->VersionLength, ULONG));
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

NTSTATUS PhClrParseTables(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata
    )
{
    PPH_MAPPED_IMAGE mappedImage = ClrMetadata->MappedImage;
    PPH_CLR_TABLESHEADER header = ClrMetadata->TablesStream;
    PULONG rowCounts;
    PVOID rows;
    ULONG validCount;

    __try
    {
        PhMappedImageProbe(mappedImage, header, sizeof(PH_CLR_TABLESHEADER));

        ClrMetadata->MajorVersion = header->Major;
        ClrMetadata->MinorVersion = header->Minor;
        ClrMetadata->HeapSizes = header->HeapSizes;
        ClrMetadata->ValidMask = header->ValidMask;
        ClrMetadata->SortedMask = header->SortedMask;

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

VOID NTAPI PhDeleteMappedClrMetadata(
    _Inout_ PPH_MAPPED_CLR_METADATA ClrMetadata
    )
{
    // The structure does not own any heap allocations; everything points into
    // the mapped image. Provided for API symmetry with Initialize.
    NOTHING;
}

ULONG NTAPI PhGetMappedClrNumberOfTables(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata
    )
{
    return PhCountBitsUlong64(ClrMetadata->ValidMask);
}

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

    table = &ClrMetadata->Tables[TableIndex];

    if (Rid < 1 || Rid > table->RowCount || !table->Rows)
        return STATUS_INVALID_PARAMETER;

    row = PTR_ADD_OFFSET(table->Rows, UInt32x32To64(Rid - 1, table->RowSize));

    __try
    {
        PhMappedImageProbe(ClrMetadata->MappedImage, row, table->RowSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    *Row = row;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PhGetMappedClrColumnValue(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Column,
    _In_ ULONG Rid,
    _Out_ PULONG Value
    )
{
    NTSTATUS status;
    PPH_MAPPED_CLR_TABLE table;
    PPH_MAPPED_CLR_COLUMN column;
    PVOID row;

    status = PhGetMappedClrTableRow(ClrMetadata, TableIndex, Rid, &row);

    if (!NT_SUCCESS(status))
        return status;

    table = &ClrMetadata->Tables[TableIndex];

    if (Column >= table->ColumnCount)
        return STATUS_INVALID_PARAMETER;

    column = &table->Columns[Column];

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

ULONG NTAPI PhGetMappedClrStringHeapSize(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata
    )
{
    return ClrMetadata->StringHeapSize;
}

NTSTATUS NTAPI PhGetMappedClrString(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Index,
    _Out_ PCSTR *String
    )
{
    PSTR string;

    if (!ClrMetadata->StringHeap || Index >= ClrMetadata->StringHeapSize)
        return STATUS_INVALID_PARAMETER;

    string = PTR_ADD_OFFSET(ClrMetadata->StringHeap, Index);

    __try
    {
        ULONG length = 0;

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
    return STATUS_SUCCESS;
}

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
