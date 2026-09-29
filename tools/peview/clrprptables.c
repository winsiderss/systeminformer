/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2023-2026
 *
 */

#include <peview.h>
#include <mapclr.h>

#ifdef __has_include
#if __has_include (<corhdr.h>)
#include <corhdr.h>
#else
#include "metahost/corhdr.h"
#endif
#else
#include "metahost/corhdr.h"
#endif

typedef struct _PVP_PE_CLR_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
    PVOID PdbMetadataAddress;
    BOOLEAN ClrMetadataInitialized;
    PH_MAPPED_CLR_METADATA ClrMetadata;
} PVP_PE_CLR_CONTEXT, *PPVP_PE_CLR_CONTEXT;

typedef struct _PVP_PE_CLR_TABLE_PREVIEW_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    RECT MinimumSize;
    PPH_MAPPED_CLR_METADATA ClrMetadata;
    ULONG TableIndex;
    ULONG RowCount;
    ULONG ColumnCount;
    PCSTR TableName;
} PVP_PE_CLR_TABLE_PREVIEW_CONTEXT, *PPVP_PE_CLR_TABLE_PREVIEW_CONTEXT;

PCWSTR PvClrColumnTypeToString(
    _In_ ULONG Type
    )
{
    switch (Type)
    {
    case PH_CLR_COLUMN_FIXED:
        return L"Fixed";
    case PH_CLR_COLUMN_STRING:
        return L"String";
    case PH_CLR_COLUMN_GUID:
        return L"GUID";
    case PH_CLR_COLUMN_BLOB:
        return L"Blob";
    case PH_CLR_COLUMN_TABLE:
        return L"Table";
    case PH_CLR_COLUMN_CODED:
        return L"Coded";
    default:
        return L"Unknown";
    }
}

typedef struct _PVP_CLR_FLAG_ENTRY
{
    ULONG Mask;
    ULONG Value;
    PCWSTR Name;
} PVP_CLR_FLAG_ENTRY, *PPVP_CLR_FLAG_ENTRY;

#define PVP_CLR_BIT(Flag) { Flag, Flag, TEXT(#Flag) + 2 }
#define PVP_CLR_ENUM(Mask, Flag) { Mask, Flag, TEXT(#Flag) + 2 }
#define PVP_CLR_ENUM_EX(Mask, Flag, PrefixLength) { Mask, Flag, TEXT(#Flag) + (PrefixLength) }

// TypeDef.Flags (CorTypeAttr)
static const PVP_CLR_FLAG_ENTRY PvClrTypeAttributes[] =
{
    PVP_CLR_ENUM(tdVisibilityMask, tdNotPublic),
    PVP_CLR_ENUM(tdVisibilityMask, tdPublic),
    PVP_CLR_ENUM(tdVisibilityMask, tdNestedPublic),
    PVP_CLR_ENUM(tdVisibilityMask, tdNestedPrivate),
    PVP_CLR_ENUM(tdVisibilityMask, tdNestedFamily),
    PVP_CLR_ENUM(tdVisibilityMask, tdNestedAssembly),
    PVP_CLR_ENUM(tdVisibilityMask, tdNestedFamANDAssem),
    PVP_CLR_ENUM(tdVisibilityMask, tdNestedFamORAssem),
    PVP_CLR_ENUM(tdLayoutMask, tdAutoLayout),
    PVP_CLR_ENUM(tdLayoutMask, tdSequentialLayout),
    PVP_CLR_ENUM(tdLayoutMask, tdExplicitLayout),
    PVP_CLR_ENUM(tdClassSemanticsMask, tdClass),
    PVP_CLR_ENUM(tdClassSemanticsMask, tdInterface),
    PVP_CLR_BIT(tdAbstract),
    PVP_CLR_BIT(tdSealed),
    PVP_CLR_BIT(tdSpecialName),
    PVP_CLR_BIT(tdImport),
    PVP_CLR_BIT(tdSerializable),
    PVP_CLR_BIT(tdWindowsRuntime),
    PVP_CLR_ENUM(tdStringFormatMask, tdAnsiClass),
    PVP_CLR_ENUM(tdStringFormatMask, tdUnicodeClass),
    PVP_CLR_ENUM(tdStringFormatMask, tdAutoClass),
    PVP_CLR_ENUM(tdStringFormatMask, tdCustomFormatClass),
    PVP_CLR_BIT(tdBeforeFieldInit),
    PVP_CLR_BIT(tdForwarder),
    PVP_CLR_BIT(tdRTSpecialName),
    PVP_CLR_BIT(tdHasSecurity),
};

// MethodDef.Flags (CorMethodAttr)
static const PVP_CLR_FLAG_ENTRY PvClrMethodAttributes[] =
{
    PVP_CLR_ENUM(mdMemberAccessMask, mdPrivateScope),
    PVP_CLR_ENUM(mdMemberAccessMask, mdPrivate),
    PVP_CLR_ENUM(mdMemberAccessMask, mdFamANDAssem),
    PVP_CLR_ENUM(mdMemberAccessMask, mdAssem),
    PVP_CLR_ENUM(mdMemberAccessMask, mdFamily),
    PVP_CLR_ENUM(mdMemberAccessMask, mdFamORAssem),
    PVP_CLR_ENUM(mdMemberAccessMask, mdPublic),
    PVP_CLR_BIT(mdStatic),
    PVP_CLR_BIT(mdFinal),
    PVP_CLR_BIT(mdVirtual),
    PVP_CLR_BIT(mdHideBySig),
    PVP_CLR_ENUM(mdVtableLayoutMask, mdNewSlot),
    PVP_CLR_BIT(mdCheckAccessOnOverride),
    PVP_CLR_BIT(mdAbstract),
    PVP_CLR_BIT(mdSpecialName),
    PVP_CLR_BIT(mdPinvokeImpl),
    PVP_CLR_BIT(mdUnmanagedExport),
    PVP_CLR_BIT(mdRTSpecialName),
    PVP_CLR_BIT(mdHasSecurity),
    PVP_CLR_BIT(mdRequireSecObject),
};

// MethodDef.ImplFlags (CorMethodImpl)
static const PVP_CLR_FLAG_ENTRY PvClrMethodImplAttributes[] =
{
    PVP_CLR_ENUM(miCodeTypeMask, miIL),
    PVP_CLR_ENUM(miCodeTypeMask, miNative),
    PVP_CLR_ENUM(miCodeTypeMask, miOPTIL),
    PVP_CLR_ENUM(miCodeTypeMask, miRuntime),
    PVP_CLR_ENUM(miManagedMask, miUnmanaged),
    PVP_CLR_BIT(miForwardRef),
    PVP_CLR_BIT(miPreserveSig),
    PVP_CLR_BIT(miInternalCall),
    PVP_CLR_BIT(miSynchronized),
    PVP_CLR_BIT(miNoInlining),
    PVP_CLR_BIT(miAggressiveInlining),
    PVP_CLR_BIT(miNoOptimization),
};

// Field.Flags (CorFieldAttr)
static const PVP_CLR_FLAG_ENTRY PvClrFieldAttributes[] =
{
    PVP_CLR_ENUM(fdFieldAccessMask, fdPrivateScope),
    PVP_CLR_ENUM(fdFieldAccessMask, fdPrivate),
    PVP_CLR_ENUM(fdFieldAccessMask, fdFamANDAssem),
    PVP_CLR_ENUM(fdFieldAccessMask, fdAssembly),
    PVP_CLR_ENUM(fdFieldAccessMask, fdFamily),
    PVP_CLR_ENUM(fdFieldAccessMask, fdFamORAssem),
    PVP_CLR_ENUM(fdFieldAccessMask, fdPublic),
    PVP_CLR_BIT(fdStatic),
    PVP_CLR_BIT(fdInitOnly),
    PVP_CLR_BIT(fdLiteral),
    PVP_CLR_BIT(fdNotSerialized),
    PVP_CLR_BIT(fdSpecialName),
    PVP_CLR_BIT(fdPinvokeImpl),
    PVP_CLR_BIT(fdRTSpecialName),
    PVP_CLR_BIT(fdHasFieldMarshal),
    PVP_CLR_BIT(fdHasDefault),
    PVP_CLR_BIT(fdHasFieldRVA),
};

// Param.Flags (CorParamAttr)
static const PVP_CLR_FLAG_ENTRY PvClrParamAttributes[] =
{
    PVP_CLR_BIT(pdIn),
    PVP_CLR_BIT(pdOut),
    PVP_CLR_BIT(pdOptional),
    PVP_CLR_BIT(pdHasDefault),
    PVP_CLR_BIT(pdHasFieldMarshal),
};

// Property.Flags (CorPropertyAttr)
static const PVP_CLR_FLAG_ENTRY PvClrPropertyAttributes[] =
{
    PVP_CLR_BIT(prSpecialName),
    PVP_CLR_BIT(prRTSpecialName),
    PVP_CLR_BIT(prHasDefault),
};

// Event.Flags (CorEventAttr)
static const PVP_CLR_FLAG_ENTRY PvClrEventAttributes[] =
{
    PVP_CLR_BIT(evSpecialName),
    PVP_CLR_BIT(evRTSpecialName),
};

// MethodSemantics.Semantics (CorMethodSemanticsAttr)
static const PVP_CLR_FLAG_ENTRY PvClrMethodSemantics[] =
{
    PVP_CLR_BIT(msSetter),
    PVP_CLR_BIT(msGetter),
    PVP_CLR_BIT(msOther),
    PVP_CLR_BIT(msAddOn),
    PVP_CLR_BIT(msRemoveOn),
    PVP_CLR_BIT(msFire),
};

// ImplMap.MappingFlags (CorPinvokeMap)
static const PVP_CLR_FLAG_ENTRY PvClrPinvokeAttributes[] =
{
    PVP_CLR_BIT(pmNoMangle),
    PVP_CLR_ENUM(pmCharSetMask, pmCharSetNotSpec),
    PVP_CLR_ENUM(pmCharSetMask, pmCharSetAnsi),
    PVP_CLR_ENUM(pmCharSetMask, pmCharSetUnicode),
    PVP_CLR_ENUM(pmCharSetMask, pmCharSetAuto),
    PVP_CLR_ENUM(pmBestFitMask, pmBestFitEnabled),
    PVP_CLR_ENUM(pmBestFitMask, pmBestFitDisabled),
    PVP_CLR_BIT(pmSupportsLastError),
    PVP_CLR_ENUM(pmCallConvMask, pmCallConvWinapi),
    PVP_CLR_ENUM(pmCallConvMask, pmCallConvCdecl),
    PVP_CLR_ENUM(pmCallConvMask, pmCallConvStdcall),
    PVP_CLR_ENUM(pmCallConvMask, pmCallConvThiscall),
    PVP_CLR_ENUM(pmCallConvMask, pmCallConvFastcall),
};

// Assembly.Flags / AssemblyRef.Flags (CorAssemblyFlags)
static const PVP_CLR_FLAG_ENTRY PvClrAssemblyAttributes[] =
{
    PVP_CLR_BIT(afPublicKey),
    PVP_CLR_BIT(afRetargetable),
    PVP_CLR_BIT(afEnableJITcompileTracking),
    PVP_CLR_BIT(afDisableJITcompileOptimizer),
};

// ManifestResource.Flags (CorManifestResourceFlags)
static const PVP_CLR_FLAG_ENTRY PvClrManifestResourceAttributes[] =
{
    PVP_CLR_ENUM(mrVisibilityMask, mrPublic),
    PVP_CLR_ENUM(mrVisibilityMask, mrPrivate),
};

// File.Flags (CorFileFlags)
static const PVP_CLR_FLAG_ENTRY PvClrFileAttributes[] =
{
    PVP_CLR_ENUM(ffContainsNoMetaData, ffContainsMetaData),
    PVP_CLR_BIT(ffContainsNoMetaData),
};

// GenericParam.Flags (CorGenericParamAttr)
static const PVP_CLR_FLAG_ENTRY PvClrGenericParamAttributes[] =
{
    PVP_CLR_ENUM(gpVarianceMask, gpNonVariant),
    PVP_CLR_ENUM(gpVarianceMask, gpCovariant),
    PVP_CLR_ENUM(gpVarianceMask, gpContravariant),
    PVP_CLR_BIT(gpReferenceTypeConstraint),
    PVP_CLR_BIT(gpNotNullableValueTypeConstraint),
    PVP_CLR_BIT(gpDefaultConstructorConstraint),
};

// Constant.Type (CorElementType)
static const PVP_CLR_FLAG_ENTRY PvClrElementTypes[] =
{
#define PVP_CLR_ELEMENT_TYPE(Flag) PVP_CLR_ENUM_EX(0xff, Flag, RTL_NUMBER_OF(L"ELEMENT_TYPE_") - 1)
    PVP_CLR_ELEMENT_TYPE(ELEMENT_TYPE_END),
    PVP_CLR_ELEMENT_TYPE(ELEMENT_TYPE_VOID),
    PVP_CLR_ELEMENT_TYPE(ELEMENT_TYPE_BOOLEAN),
    PVP_CLR_ELEMENT_TYPE(ELEMENT_TYPE_CHAR),
    PVP_CLR_ELEMENT_TYPE(ELEMENT_TYPE_I1),
    PVP_CLR_ELEMENT_TYPE(ELEMENT_TYPE_U1),
    PVP_CLR_ELEMENT_TYPE(ELEMENT_TYPE_I2),
    PVP_CLR_ELEMENT_TYPE(ELEMENT_TYPE_U2),
    PVP_CLR_ELEMENT_TYPE(ELEMENT_TYPE_I4),
    PVP_CLR_ELEMENT_TYPE(ELEMENT_TYPE_U4),
    PVP_CLR_ELEMENT_TYPE(ELEMENT_TYPE_I8),
    PVP_CLR_ELEMENT_TYPE(ELEMENT_TYPE_U8),
    PVP_CLR_ELEMENT_TYPE(ELEMENT_TYPE_R4),
    PVP_CLR_ELEMENT_TYPE(ELEMENT_TYPE_R8),
    PVP_CLR_ELEMENT_TYPE(ELEMENT_TYPE_STRING),
    PVP_CLR_ELEMENT_TYPE(ELEMENT_TYPE_CLASS),
};

// DeclSecurity.Action (CorDeclSecurity)
static const PVP_CLR_FLAG_ENTRY PvClrDeclSecurityActions[] =
{
#define PVP_CLR_DECL_SECURITY(Flag) PVP_CLR_ENUM_EX(dclActionMask, Flag, RTL_NUMBER_OF(L"dcl") - 1)
    PVP_CLR_DECL_SECURITY(dclActionNil),
    PVP_CLR_DECL_SECURITY(dclRequest),
    PVP_CLR_DECL_SECURITY(dclDemand),
    PVP_CLR_DECL_SECURITY(dclAssert),
    PVP_CLR_DECL_SECURITY(dclDeny),
    PVP_CLR_DECL_SECURITY(dclPermitOnly),
    PVP_CLR_DECL_SECURITY(dclLinktimeCheck),
    PVP_CLR_DECL_SECURITY(dclInheritanceCheck),
    PVP_CLR_DECL_SECURITY(dclRequestMinimum),
    PVP_CLR_DECL_SECURITY(dclRequestOptional),
    PVP_CLR_DECL_SECURITY(dclRequestRefuse),
    PVP_CLR_DECL_SECURITY(dclPrejitGrant),
    PVP_CLR_DECL_SECURITY(dclPrejitDenied),
    PVP_CLR_DECL_SECURITY(dclNonCasDemand),
    PVP_CLR_DECL_SECURITY(dclNonCasLinkDemand),
    PVP_CLR_DECL_SECURITY(dclNonCasInheritance),
};

// Maps a (table, column) pair carrying a fixed-width value onto its flag names.
typedef struct _PVP_CLR_FLAG_COLUMN
{
    UCHAR TableIndex;
    UCHAR Column;
    ULONG Count;
    const PVP_CLR_FLAG_ENTRY* Entries;
} PVP_CLR_FLAG_COLUMN, *PPVP_CLR_FLAG_COLUMN;

#define PVP_CLR_FLAG_COLUMN_ENTRY(Table, Column, Entries) \
    { PH_CLR_TABLE_##Table, Column, RTL_NUMBER_OF(Entries), Entries }

static const PVP_CLR_FLAG_COLUMN PvClrFlagColumns[] =
{
    PVP_CLR_FLAG_COLUMN_ENTRY(TYPEDEF, PH_CLR_TYPEDEF_REC_COL_FLAGS, PvClrTypeAttributes),
    PVP_CLR_FLAG_COLUMN_ENTRY(METHODDEF, PH_CLR_METHODDEF_REC_COL_IMPLFLAGS, PvClrMethodImplAttributes),
    PVP_CLR_FLAG_COLUMN_ENTRY(METHODDEF, PH_CLR_METHODDEF_REC_COL_FLAGS, PvClrMethodAttributes),
    PVP_CLR_FLAG_COLUMN_ENTRY(FIELD, PH_CLR_FIELD_REC_COL_FLAGS, PvClrFieldAttributes),
    PVP_CLR_FLAG_COLUMN_ENTRY(PARAM, PH_CLR_PARAM_REC_COL_FLAGS, PvClrParamAttributes),
    PVP_CLR_FLAG_COLUMN_ENTRY(PROPERTY, PH_CLR_PROPERTY_REC_COL_FLAGS, PvClrPropertyAttributes),
    PVP_CLR_FLAG_COLUMN_ENTRY(EVENT, PH_CLR_EVENT_REC_COL_FLAGS, PvClrEventAttributes),
    PVP_CLR_FLAG_COLUMN_ENTRY(METHODSEMANTICS, PH_CLR_METHODSEMANTICS_REC_COL_SEMANTICS, PvClrMethodSemantics),
    PVP_CLR_FLAG_COLUMN_ENTRY(IMPLMAP, PH_CLR_IMPLMAP_REC_COL_MAPPINGFLAGS, PvClrPinvokeAttributes),
    PVP_CLR_FLAG_COLUMN_ENTRY(ASSEMBLY, PH_CLR_ASSEMBLY_REC_COL_FLAGS, PvClrAssemblyAttributes),
    PVP_CLR_FLAG_COLUMN_ENTRY(ASSEMBLYREF, PH_CLR_ASSEMBLYREF_REC_COL_FLAGS, PvClrAssemblyAttributes),
    PVP_CLR_FLAG_COLUMN_ENTRY(MANIFESTRESOURCE, PH_CLR_MANIFESTRESOURCE_REC_COL_FLAGS, PvClrManifestResourceAttributes),
    PVP_CLR_FLAG_COLUMN_ENTRY(FILE, PH_CLR_FILE_REC_COL_FLAGS, PvClrFileAttributes),
    PVP_CLR_FLAG_COLUMN_ENTRY(GENERICPARAM, PH_CLR_GENERICPARAM_REC_COL_FLAGS, PvClrGenericParamAttributes),
    PVP_CLR_FLAG_COLUMN_ENTRY(EXPORTEDTYPE, PH_CLR_EXPORTEDTYPE_REC_COL_FLAGS, PvClrTypeAttributes),
    PVP_CLR_FLAG_COLUMN_ENTRY(CONSTANT, PH_CLR_CONSTANT_REC_COL_TYPE, PvClrElementTypes),
    PVP_CLR_FLAG_COLUMN_ENTRY(DECLSECURITY, PH_CLR_DECLSECURITY_REC_COL_ACTION, PvClrDeclSecurityActions),
};

// Maps a table onto the columns holding its display name.
typedef struct _PVP_CLR_NAME_COLUMN
{
    UCHAR TableIndex;
    UCHAR NameColumn;
    UCHAR NamespaceColumn;    // MAXUCHAR when the table has no namespace column
} PVP_CLR_NAME_COLUMN, *PPVP_CLR_NAME_COLUMN;

static const PVP_CLR_NAME_COLUMN PvClrNameColumns[] =
{
    { PH_CLR_TABLE_MODULE, PH_CLR_MODULE_REC_COL_NAME, MAXUCHAR },
    { PH_CLR_TABLE_TYPEREF, PH_CLR_TYPEREF_REC_COL_TYPENAME, PH_CLR_TYPEREF_REC_COL_TYPENAMESPACE },
    { PH_CLR_TABLE_TYPEDEF, PH_CLR_TYPEDEF_REC_COL_TYPENAME, PH_CLR_TYPEDEF_REC_COL_TYPENAMESPACE },
    { PH_CLR_TABLE_FIELD, PH_CLR_FIELD_REC_COL_NAME, MAXUCHAR },
    { PH_CLR_TABLE_METHODDEF, PH_CLR_METHODDEF_REC_COL_NAME, MAXUCHAR },
    { PH_CLR_TABLE_PARAM, PH_CLR_PARAM_REC_COL_NAME, MAXUCHAR },
    { PH_CLR_TABLE_MEMBERREF, PH_CLR_MEMBERREF_REC_COL_NAME, MAXUCHAR },
    { PH_CLR_TABLE_EVENT, PH_CLR_EVENT_REC_COL_NAME, MAXUCHAR },
    { PH_CLR_TABLE_PROPERTY, PH_CLR_PROPERTY_REC_COL_NAME, MAXUCHAR },
    { PH_CLR_TABLE_MODULEREF, PH_CLR_MODULEREF_REC_COL_NAME, MAXUCHAR },
    { PH_CLR_TABLE_ASSEMBLY, PH_CLR_ASSEMBLY_REC_COL_NAME, MAXUCHAR },
    { PH_CLR_TABLE_ASSEMBLYREF, PH_CLR_ASSEMBLYREF_REC_COL_NAME, MAXUCHAR },
    { PH_CLR_TABLE_FILE, PH_CLR_FILE_REC_COL_NAME, MAXUCHAR },
    { PH_CLR_TABLE_EXPORTEDTYPE, PH_CLR_EXPORTEDTYPE_REC_COL_TYPENAME, PH_CLR_EXPORTEDTYPE_REC_COL_TYPENAMESPACE },
    { PH_CLR_TABLE_MANIFESTRESOURCE, PH_CLR_MANIFESTRESOURCE_REC_COL_NAME, MAXUCHAR },
    { PH_CLR_TABLE_GENERICPARAM, PH_CLR_GENERICPARAM_REC_COL_NAME, MAXUCHAR },
    { PH_CLR_TABLE_LOCALVARIABLE, PH_CLR_LOCALVARIABLE_REC_COL_NAME, MAXUCHAR },
    { PH_CLR_TABLE_LOCALCONSTANT, PH_CLR_LOCALCONSTANT_REC_COL_NAME, MAXUCHAR },
};

// Maximum number of characters of a heap string shown in a cell.
#define PVP_CLR_MAX_STRING_LENGTH 128
// Maximum number of blob bytes shown in a cell.
#define PVP_CLR_MAX_BLOB_BYTES 8

VOID PvClrAppendFlags(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ ULONG Value,
    _In_reads_(Count) const PVP_CLR_FLAG_ENTRY* Entries,
    _In_ ULONG Count
    )
{
    SIZE_T length = StringBuilder->String->Length;

    for (ULONG i = 0; i < Count; i++)
    {
        if ((Value & Entries[i].Mask) == Entries[i].Value)
        {
            PhAppendStringBuilder2(StringBuilder, Entries[i].Name);
            PhAppendStringBuilder2(StringBuilder, L", ");
        }
    }

    if (StringBuilder->String->Length != length)
        PhRemoveEndStringBuilder(StringBuilder, 2);
}

// Appends the decoded form of a fixed-width column, or nothing when the column
// has no known flag mapping.
BOOLEAN PvClrAppendFixedColumn(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ ULONG TableIndex,
    _In_ ULONG Column,
    _In_ ULONG Value
    )
{
    for (ULONG i = 0; i < RTL_NUMBER_OF(PvClrFlagColumns); i++)
    {
        const PVP_CLR_FLAG_COLUMN* entry = &PvClrFlagColumns[i];

        if (entry->TableIndex == TableIndex && entry->Column == Column)
        {
            SIZE_T length = StringBuilder->String->Length;

            PvClrAppendFlags(StringBuilder, Value, entry->Entries, entry->Count);

            return StringBuilder->String->Length != length;
        }
    }

    return FALSE;
}

// Returns the display name of a row in the referenced table, or NULL.
PPH_STRING PvClrGetRowName(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Rid
    )
{
    for (ULONG i = 0; i < RTL_NUMBER_OF(PvClrNameColumns); i++)
    {
        const PVP_CLR_NAME_COLUMN* entry = &PvClrNameColumns[i];
        PPH_STRING name;
        PPH_STRING nameSpace;

        if (entry->TableIndex != TableIndex)
            continue;

        if (!(name = PhGetMappedClrTableString(ClrMetadata, TableIndex, Rid, entry->NameColumn)))
            return NULL;

        if (entry->NamespaceColumn != MAXUCHAR)
        {
            if (nameSpace = PhGetMappedClrTableString(ClrMetadata, TableIndex, Rid, entry->NamespaceColumn))
            {
                if (nameSpace->Length)
                {
                    PPH_STRING fullName = PhConcatStrings(3, nameSpace->Buffer, L".", name->Buffer);

                    PhDereferenceObject(nameSpace);
                    PhDereferenceObject(name);
                    return fullName;
                }

                PhDereferenceObject(nameSpace);
            }
        }

        return name;
    }

    return NULL;
}

VOID PvClrAppendQuotedString(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ PPH_STRING String
    )
{
    PhAppendCharStringBuilder(StringBuilder, L'\"');

    if (String->Length / sizeof(WCHAR) > PVP_CLR_MAX_STRING_LENGTH)
    {
        PhAppendStringBuilderEx(StringBuilder, String->Buffer, PVP_CLR_MAX_STRING_LENGTH * sizeof(WCHAR));
        PhAppendStringBuilder2(StringBuilder, L"...");
    }
    else
    {
        PhAppendStringBuilder(StringBuilder, &String->sr);
    }

    PhAppendCharStringBuilder(StringBuilder, L'\"');
}

// Formats a single cell as the raw column value followed by its resolved meaning.
PPH_STRING PvClrFormatColumnValue(
    _In_ PPVP_PE_CLR_TABLE_PREVIEW_CONTEXT Context,
    _In_ ULONG Column,
    _In_ ULONG Rid
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG type = PH_CLR_COLUMN_FIXED;
    ULONG value = 0;

    if (!NT_SUCCESS(PhGetMappedClrColumnValue(
        Context->ClrMetadata,
        Context->TableIndex,
        Column,
        Rid,
        &value
        )))
    {
        return NULL;
    }

    PhGetMappedClrColumnInfo(
        Context->ClrMetadata,
        Context->TableIndex,
        Column,
        NULL,
        NULL,
        &type
        );

    PhInitializeStringBuilder(&stringBuilder, 0x40);

    switch (type)
    {
    case PH_CLR_COLUMN_TABLE:
    case PH_CLR_COLUMN_CODED:
        {
            ULONG targetTable = 0;
            ULONG targetRid = 0;
            ULONG token = 0;
            PCSTR tableName;
            PPH_STRING rowName;

            if (!NT_SUCCESS(PhGetMappedClrColumnToken(
                Context->ClrMetadata,
                Context->TableIndex,
                Column,
                Rid,
                &targetTable,
                &targetRid,
                &token
                )))
            {
                PhAppendFormatStringBuilder(&stringBuilder, L"0x%08x", value);
                break;
            }

            // Simple table indexes are plain RIDs, coded indexes are real metadata tokens.
            PhAppendFormatStringBuilder(&stringBuilder, L"0x%08x", type == PH_CLR_COLUMN_TABLE ? value : token);

            if (targetRid == 0)
                break;

            if (tableName = PhGetMappedClrTableName(targetTable))
            {
                PPH_STRING tableNameString;

                if (tableNameString = PhConvertUtf8ToUtf16(tableName))
                {
                    PhAppendFormatStringBuilder(&stringBuilder, L" (%s:%lu)", tableNameString->Buffer, targetRid);
                    PhDereferenceObject(tableNameString);
                }
            }

            if (rowName = PvClrGetRowName(Context->ClrMetadata, targetTable, targetRid))
            {
                if (rowName->Length)
                {
                    PhAppendCharStringBuilder(&stringBuilder, L' ');
                    PvClrAppendQuotedString(&stringBuilder, rowName);
                }

                PhDereferenceObject(rowName);
            }
        }
        break;
    case PH_CLR_COLUMN_STRING:
        {
            PPH_STRING stringValue;

            PhAppendFormatStringBuilder(&stringBuilder, L"0x%08x", value);

            if (value == 0)
                break;

            if (NT_SUCCESS(PhGetMappedClrStringEx(Context->ClrMetadata, value, &stringValue)))
            {
                if (stringValue->Length)
                {
                    PhAppendCharStringBuilder(&stringBuilder, L' ');
                    PvClrAppendQuotedString(&stringBuilder, stringValue);
                }

                PhDereferenceObject(stringValue);
            }
        }
        break;
    case PH_CLR_COLUMN_GUID:
        {
            GUID guid;
            PPH_STRING guidString;

            PhAppendFormatStringBuilder(&stringBuilder, L"0x%08x", value);

            if (value == 0)
                break;

            if (NT_SUCCESS(PhGetMappedClrGuid(Context->ClrMetadata, value, &guid)))
            {
                if (guidString = PhFormatGuid(&guid))
                {
                    PhAppendCharStringBuilder(&stringBuilder, L' ');
                    PhAppendStringBuilder(&stringBuilder, &guidString->sr);
                    PhDereferenceObject(guidString);
                }
            }
        }
        break;
    case PH_CLR_COLUMN_BLOB:
        {
            PVOID data;
            ULONG length;

            PhAppendFormatStringBuilder(&stringBuilder, L"0x%08x", value);

            if (NT_SUCCESS(PhGetMappedClrBlob(Context->ClrMetadata, value, &data, &length)))
            {
                PPH_STRING hexString;

                PhAppendFormatStringBuilder(&stringBuilder, L" (%lu bytes)", length);

                if (length && (hexString = PhBufferToHexString(data, min(length, PVP_CLR_MAX_BLOB_BYTES))))
                {
                    PhAppendCharStringBuilder(&stringBuilder, L' ');
                    PhAppendStringBuilder(&stringBuilder, &hexString->sr);

                    if (length > PVP_CLR_MAX_BLOB_BYTES)
                        PhAppendStringBuilder2(&stringBuilder, L"...");

                    PhDereferenceObject(hexString);
                }
            }
        }
        break;
    default:
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"0x%08x (", value);

            if (!PvClrAppendFixedColumn(&stringBuilder, Context->TableIndex, Column, value))
            {
                PhAppendFormatStringBuilder(&stringBuilder, L"%lu", value);
            }

            PhAppendCharStringBuilder(&stringBuilder, L')');
        }
        break;
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

VOID PvPeClrTablePreviewAddColumns(
    _In_ PPVP_PE_CLR_TABLE_PREVIEW_CONTEXT Context
    )
{
    PhAddListViewColumn(Context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 60, L"RID");

    for (ULONG i = 0; i < Context->ColumnCount; i++)
    {
        ULONG type = 0;
        ULONG width;
        PCWSTR typeName;
        PCWSTR columnName = NULL;

        if (!NT_SUCCESS(PhGetMappedClrColumnInfo(
            Context->ClrMetadata,
            Context->TableIndex,
            i,
            NULL,
            NULL,
            &type
            )))
        {
            typeName = L"Unknown";
        }
        else
        {
            typeName = PvClrColumnTypeToString(type);
        }

        // Get the actual column name from the mapping table
        if (Context->TableIndex < PH_CLR_TABLE_MAXIMUM && 
            i < PH_CLR_MAX_COLUMNS && 
            PhClrTableColumnNames[Context->TableIndex][i])
        {
            columnName = PhClrTableColumnNames[Context->TableIndex][i];
        }

        // Columns carrying resolved names, strings or blobs need the extra room.
        switch (type)
        {
        case PH_CLR_COLUMN_STRING:
        case PH_CLR_COLUMN_BLOB:
        case PH_CLR_COLUMN_TABLE:
        case PH_CLR_COLUMN_CODED:
            width = 220;
            break;
        default:
            width = 110;
            break;
        }

        if (columnName)
        {
            PhAddListViewColumn(
                Context->ListViewHandle,
                i + 1,
                i + 1,
                i + 1,
                LVCFMT_LEFT,
                width,
                columnName
                );
        }
        else
        {
            PhAddListViewColumn(
                Context->ListViewHandle,
                i + 1,
                i + 1,
                i + 1,
                LVCFMT_LEFT,
                width,
                PhaFormatString(L"Column %lu (%s)", i, typeName)->Buffer
                );
        }
    }
}

VOID PvPeClrTablePreviewAddRows(
    _In_ PPVP_PE_CLR_TABLE_PREVIEW_CONTEXT Context
    )
{
    WCHAR value[PH_INT64_STR_LEN_1];

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);

    for (ULONG row = 0; row < Context->RowCount; row++)
    {
        INT lvItemIndex;
        ULONG rid = row + 1;

        PhPrintUInt32(value, rid);
        lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, value, NULL);

        for (ULONG column = 0; column < Context->ColumnCount; column++)
        {
            PPH_STRING columnValue;

            if (columnValue = PvClrFormatColumnValue(Context, column, rid))
            {
                PhSetListViewSubItem(
                    Context->ListViewHandle,
                    lvItemIndex,
                    column + 1,
                    columnValue->Buffer
                    );

                PhDereferenceObject(columnValue);
            }
        }
    }

    for (ULONG column = 0; column <= Context->ColumnCount; column++)
        ListView_SetColumnWidth(Context->ListViewHandle, column, LVSCW_AUTOSIZE_USEHEADER);

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvPeClrTablePreviewDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_CLR_TABLE_PREVIEW_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPVP_PE_CLR_TABLE_PREVIEW_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HICON smallIcon;
            HICON largeIcon;
            PPH_STRING tableName;

            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhGetStockApplicationIcon(&smallIcon, &largeIcon, PhGetWindowDpi(hwndDlg));
            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)largeIcon);

            if (tableName = PhConvertUtf8ToUtf16(context->TableName))
            {
                PhSetWindowText(hwndDlg, PhaFormatString(L"CLR Table: %s", tableName->Buffer)->Buffer);
                PhDereferenceObject(tableName);
            }

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PvConfigListViewFont(hwndDlg, context->ListViewHandle);
            PhSetExtendedListView(context->ListViewHandle);
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhLayoutManagerLayout(&context->LayoutManager);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 280;
            context->MinimumSize.bottom = 180;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            PvPeClrTablePreviewAddColumns(context);
            PvPeClrTablePreviewAddRows(context);
            PhLoadListViewColumnsFromSetting(L"ImageClrTablePreviewListViewColumns", context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageClrTablePreviewListViewColumns", context->ListViewHandle);

            PhDeleteLayoutManager(&context->LayoutManager);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, context->MinimumSize.right, context->MinimumSize.bottom);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                return TRUE;
            }
        }
        break;
    case WM_CLOSE:
        {
            DestroyWindow(hwndDlg);
        }
        return TRUE;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)PhGetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}

VOID PvPeClrShowTablePreview(
    _In_ PPVP_PE_CLR_CONTEXT Context,
    _In_ ULONG TableIndex
    )
{
    NTSTATUS status;
    HWND dialogHandle;
    PPVP_PE_CLR_TABLE_PREVIEW_CONTEXT previewContext;

    if (!Context->ClrMetadataInitialized)
    {
        PhShowError(Context->WindowHandle, L"%s", L"Unable to preview CLR table rows because CLR metadata is unavailable.");
        return;
    }

    previewContext = PhAllocateZero(sizeof(PVP_PE_CLR_TABLE_PREVIEW_CONTEXT));
    previewContext->ClrMetadata = &Context->ClrMetadata;
    previewContext->TableIndex = TableIndex;

    status = PhGetMappedClrTableInfoEx(
        previewContext->ClrMetadata,
        previewContext->TableIndex,
        NULL,
        &previewContext->RowCount,
        &previewContext->ColumnCount,
        &previewContext->TableName,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        PhFree(previewContext);
        PhShowStatus(Context->WindowHandle, L"Unable to preview CLR table rows", status, 0);
        return;
    }

    dialogHandle = PhCreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PECLRTABLEPREVIEW),
        Context->WindowHandle,
        PvPeClrTablePreviewDlgProc,
        previewContext
        );

    if (!dialogHandle)
    {
        PhFree(previewContext);
        PhShowError(Context->WindowHandle, L"%s", L"Unable to create the CLR table preview window.");
        return;
    }

    ShowWindow(dialogHandle, SW_SHOW);
}

_Function_class_(PH_CLR_ENUM_TABLES_CALLBACK)
BOOLEAN NTAPI PvClrEnumTableCallback(
    _In_opt_ ULONG TableIndex,
    _In_ ULONG RowSize,
    _In_ ULONG RowCount,
    _In_ PCSTR Name,
    _In_opt_ PVOID Rows,
    _In_opt_ PVOID Context
    )
{
    PPVP_PE_CLR_CONTEXT context = Context;
    WCHAR value[PH_INT64_STR_LEN_1];
    WCHAR countValue[PH_INT64_STR_LEN_1];
    WCHAR rvaStartValue[PH_INT64_STR_LEN_1];
    WCHAR rvaEndValue[PH_INT64_STR_LEN_1];
    WCHAR sizeValue[PH_INT64_STR_LEN_1];
    INT lvItemIndex;
    PPH_STRING nameSr;
    ULONG rvaStart = 0;
    ULONG rvaEnd = 0;
    ULONG64 tableSize = 0;
    ULONG hashAlgorithm;
    PH_HASH_CONTEXT hashContext;
    UCHAR hashResult[32];
    ULONG hashResultSize = 0;
    PPH_STRING hashString;

    PhPrintUInt32(value, TableIndex);

    // If this is the initial pass (Name is NULL), add a blank row with TableIndex as lParam
    if (!Name)
    {
        lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, value, NULL);
        PhSetListViewItemParam(context->ListViewHandle, lvItemIndex, UlongToPtr(TableIndex));
        return TRUE;
    }

    // This is the data-population pass; find the existing row by TableIndex
    lvItemIndex = PhFindListViewItemByParam(context->ListViewHandle, -1, UlongToPtr(TableIndex));

    if (lvItemIndex == -1)
    {
        lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, value, NULL);
        PhSetListViewItemParam(context->ListViewHandle, lvItemIndex, UlongToPtr(TableIndex));
    }

    PhPrintUInt32(countValue, RowCount);

    // Column 1: Name
    if (nameSr = PhConvertUtf8ToUtf16(Name))
    {
        PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 1, nameSr->Buffer);
        PhDereferenceObject(nameSr);
    }
    
    // Column 2: Count
    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 2, countValue);

    // Column 3: Size
    PhPrintUInt32(sizeValue, RowSize);
    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 3, sizeValue);

    // Column 4-5: RVA start and end
    if (Rows && PvImageCor20Header)
    {
        PVOID metadataVa;
        
        // Get the VA of the metadata section
        if (NT_SUCCESS(PhMappedImageRvaToVa(&PvMappedImage, PvImageCor20Header->MetaData.VirtualAddress, &metadataVa)))
        {
            // Calculate offset within metadata and compute RVA
            ULONG64 rowsVa = (ULONG64)Rows;
            ULONG64 metadataVa64 = (ULONG64)metadataVa;
            
            if (rowsVa >= metadataVa64)
            {
                rvaStart = PvImageCor20Header->MetaData.VirtualAddress + (ULONG)(rowsVa - metadataVa64);
                PhPrintUInt32(rvaStartValue, rvaStart);
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 4, rvaStartValue);

                tableSize = (ULONG64)RowCount * RowSize;
                rvaEnd = rvaStart + (ULONG)tableSize;
                PhPrintUInt32(rvaEndValue, rvaEnd);
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 5, rvaEndValue);
            }
        }
    }

    // Column 6: Hash
    hashAlgorithm = PhGetIntegerSetting(L"HashAlgorithm");

    if (Rows && RowCount > 0 && NT_SUCCESS(PhInitializeHash(&hashContext, hashAlgorithm)))
    {
        PhUpdateHash(&hashContext, Rows, (ULONG64)RowCount * RowSize);

        hashResultSize = sizeof(hashResult);

        if (NT_SUCCESS(PhFinalHash(&hashContext, hashResult, sizeof(hashResult), &hashResultSize)))
        {
            // Format hash as hex string
            if (hashResultSize > 0)
            {
                hashString = PhBufferToHexString(hashResult, hashResultSize);
                if (hashString)
                {
                    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 6, hashString->Buffer);
                    PhDereferenceObject(hashString);
                }
            }
        }
    }

    return TRUE;
}

INT_PTR CALLBACK PvPeClrTablesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_CLR_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_CLR_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;

            if (context->PropSheetContext->Context)
            {
                PPV_CLR_PAGECONTEXT pageContext = context->PropSheetContext->Context;
                context->PdbMetadataAddress = pageContext->PdbMetadataAddress;
            }
        }
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PvConfigListViewFont(hwndDlg, context->ListViewHandle);
            PhSetExtendedListView(context->ListViewHandle);
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);

            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 50, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"Count");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Size");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 120, L"RVA (start)");
            PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 120, L"RVA (end)");
            PhAddListViewColumn(context->ListViewHandle, 6, 6, 6, LVCFMT_LEFT, 200, L"Hash");
            PhLoadListViewColumnsFromSetting(L"ImageClrTablesListViewColumns", context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            context->ClrMetadataInitialized = FALSE;

            {
                NTSTATUS status;

                status = PhInitializeMappedClrMetadata(
                    &context->ClrMetadata,
                    &PvMappedImage
                    );

                if (NT_SUCCESS(status))
                {
                    context->ClrMetadataInitialized = TRUE;

                    // Iterate through all possible table indices
                    for (ULONG tableIndex = 0; tableIndex < PH_CLR_TABLE_MAXIMUM; tableIndex++)
                    {
                        PvClrEnumTableCallback(
                            tableIndex,
                            0,
                            0,
                            NULL,
                            NULL,
                            context
                            );
                    }

                    // Now populate data for existing tables
                    PhEnumMappedClrTables(
                        &context->ClrMetadata,
                        PvClrEnumTableCallback,
                        context
                        );
                }
            }

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageClrTablesListViewColumns", context->ListViewHandle);

            if (context->ClrMetadataInitialized)
            {
                PhDeleteMappedClrMetadata(&context->ClrMetadata);
            }

            PhDeleteLayoutManager(&context->LayoutManager);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case NM_DBLCLK:
                {
                    LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)lParam;
                    PVOID tableIndex;

                    if (itemActivate->iItem != -1 && PhGetListViewItemParam(
                        context->ListViewHandle,
                        itemActivate->iItem,
                        &tableIndex
                        ))
                    {
                        PvPeClrShowTablePreview(context, PtrToUlong(tableIndex));
                    }
                }
                break;
            case PSN_QUERYINITIALFOCUS:
                return (INT_PTR)context->ListViewHandle;
            }

            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)PhGetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}
