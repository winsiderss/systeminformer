/**
 * \file mapclr.h
 * \brief Self-contained parser for ECMA-335 CLI metadata tables and Portable PDB extensions.
 */

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

#ifndef _PH_MAPCLR_H
#define _PH_MAPCLR_H

EXTERN_C_START

/**
 * ECMA-335 metadata table indices (II.22). Portable PDB tables (0x30-0x37) are
 * defined by the Portable PDB specification.
 */
typedef enum _PH_CLR_TABLE
{
    PH_CLR_TABLE_MODULE = 0x00,
    PH_CLR_TABLE_TYPEREF = 0x01,
    PH_CLR_TABLE_TYPEDEF = 0x02,
    PH_CLR_TABLE_FIELDPTR = 0x03,
    PH_CLR_TABLE_FIELD = 0x04,
    PH_CLR_TABLE_METHODPTR = 0x05,
    PH_CLR_TABLE_METHODDEF = 0x06,
    PH_CLR_TABLE_PARAMPTR = 0x07,
    PH_CLR_TABLE_PARAM = 0x08,
    PH_CLR_TABLE_INTERFACEIMPL = 0x09,
    PH_CLR_TABLE_MEMBERREF = 0x0A,
    PH_CLR_TABLE_CONSTANT = 0x0B,
    PH_CLR_TABLE_CUSTOMATTRIBUTE = 0x0C,
    PH_CLR_TABLE_FIELDMARSHAL = 0x0D,
    PH_CLR_TABLE_DECLSECURITY = 0x0E,
    PH_CLR_TABLE_CLASSLAYOUT = 0x0F,
    PH_CLR_TABLE_FIELDLAYOUT = 0x10,
    PH_CLR_TABLE_STANDALONESIG = 0x11,
    PH_CLR_TABLE_EVENTMAP = 0x12,
    PH_CLR_TABLE_EVENTPTR = 0x13,
    PH_CLR_TABLE_EVENT = 0x14,
    PH_CLR_TABLE_PROPERTYMAP = 0x15,
    PH_CLR_TABLE_PROPERTYPTR = 0x16,
    PH_CLR_TABLE_PROPERTY = 0x17,
    PH_CLR_TABLE_METHODSEMANTICS = 0x18,
    PH_CLR_TABLE_METHODIMPL = 0x19,
    PH_CLR_TABLE_MODULEREF = 0x1A,
    PH_CLR_TABLE_TYPESPEC = 0x1B,
    PH_CLR_TABLE_IMPLMAP = 0x1C,
    PH_CLR_TABLE_FIELDRVA = 0x1D,
    PH_CLR_TABLE_ENCLOG = 0x1E,
    PH_CLR_TABLE_ENCMAP = 0x1F,
    PH_CLR_TABLE_ASSEMBLY = 0x20,
    PH_CLR_TABLE_ASSEMBLYPROCESSOR = 0x21,
    PH_CLR_TABLE_ASSEMBLYOS = 0x22,
    PH_CLR_TABLE_ASSEMBLYREF = 0x23,
    PH_CLR_TABLE_ASSEMBLYREFPROCESSOR = 0x24,
    PH_CLR_TABLE_ASSEMBLYREFOS = 0x25,
    PH_CLR_TABLE_FILE = 0x26,
    PH_CLR_TABLE_EXPORTEDTYPE = 0x27,
    PH_CLR_TABLE_MANIFESTRESOURCE = 0x28,
    PH_CLR_TABLE_NESTEDCLASS = 0x29,
    PH_CLR_TABLE_GENERICPARAM = 0x2A,
    PH_CLR_TABLE_METHODSPEC = 0x2B,
    PH_CLR_TABLE_GENERICPARAMCONSTRAINT = 0x2C,
    // Portable PDB
    PH_CLR_TABLE_DOCUMENT = 0x30,
    PH_CLR_TABLE_METHODDEBUGINFORMATION = 0x31,
    PH_CLR_TABLE_LOCALSCOPE = 0x32,
    PH_CLR_TABLE_LOCALVARIABLE = 0x33,
    PH_CLR_TABLE_LOCALCONSTANT = 0x34,
    PH_CLR_TABLE_IMPORTSCOPE = 0x35,
    PH_CLR_TABLE_STATEMACHINEMETHOD = 0x36,
    PH_CLR_TABLE_CUSTOMDEBUGINFORMATION = 0x37,

    PH_CLR_TABLE_MAXIMUM = 0x40
} PH_CLR_TABLE;

typedef enum _PH_CLR_MODULE_REC
{
    PH_CLR_MODULE_REC_COL_GENERATION = 0,
    PH_CLR_MODULE_REC_COL_NAME,
    PH_CLR_MODULE_REC_COL_MVID,
    PH_CLR_MODULE_REC_COL_ENCID,
    PH_CLR_MODULE_REC_COL_ENCBASEID
} PH_CLR_MODULE_REC;

typedef enum _PH_CLR_TYPEREF_REC
{
    PH_CLR_TYPEREF_REC_COL_RESOLUTIONSCOPE = 0,
    PH_CLR_TYPEREF_REC_COL_TYPENAME,
    PH_CLR_TYPEREF_REC_COL_TYPENAMESPACE
} PH_CLR_TYPEREF_REC;

typedef enum _PH_CLR_TYPEDEF_REC
{
    PH_CLR_TYPEDEF_REC_COL_FLAGS = 0,
    PH_CLR_TYPEDEF_REC_COL_TYPENAME,
    PH_CLR_TYPEDEF_REC_COL_TYPENAMESPACE,
    PH_CLR_TYPEDEF_REC_COL_EXTENDS,
    PH_CLR_TYPEDEF_REC_COL_FIELDLIST,
    PH_CLR_TYPEDEF_REC_COL_METHODLIST
} PH_CLR_TYPEDEF_REC;

typedef enum _PH_CLR_FIELDPTR_REC
{
    PH_CLR_FIELDPTR_REC_COL_FIELD = 0
} PH_CLR_FIELDPTR_REC;

typedef enum _PH_CLR_FIELD_REC
{
    PH_CLR_FIELD_REC_COL_FLAGS = 0,
    PH_CLR_FIELD_REC_COL_NAME,
    PH_CLR_FIELD_REC_COL_SIGNATURE
} PH_CLR_FIELD_REC;

typedef enum _PH_CLR_METHODPTR_REC
{
    PH_CLR_METHODPTR_REC_COL_METHOD = 0
} PH_CLR_METHODPTR_REC;

typedef enum _PH_CLR_METHODDEF_REC
{
    PH_CLR_METHODDEF_REC_COL_RVA = 0,
    PH_CLR_METHODDEF_REC_COL_IMPLFLAGS,
    PH_CLR_METHODDEF_REC_COL_FLAGS,
    PH_CLR_METHODDEF_REC_COL_NAME,
    PH_CLR_METHODDEF_REC_COL_SIGNATURE,
    PH_CLR_METHODDEF_REC_COL_PARAMLIST
} PH_CLR_METHODDEF_REC;

typedef enum _PH_CLR_PARAMPTR_REC
{
    PH_CLR_PARAMPTR_REC_COL_PARAM = 0
} PH_CLR_PARAMPTR_REC;

typedef enum _PH_CLR_PARAM_REC
{
    PH_CLR_PARAM_REC_COL_FLAGS = 0,
    PH_CLR_PARAM_REC_COL_SEQUENCE,
    PH_CLR_PARAM_REC_COL_NAME
} PH_CLR_PARAM_REC;

typedef enum _PH_CLR_INTERFACEIMPL_REC
{
    PH_CLR_INTERFACEIMPL_REC_COL_CLASS = 0,
    PH_CLR_INTERFACEIMPL_REC_COL_INTERFACE
} PH_CLR_INTERFACEIMPL_REC;

typedef enum _PH_CLR_MEMBERREF_REC
{
    PH_CLR_MEMBERREF_REC_COL_CLASS = 0,
    PH_CLR_MEMBERREF_REC_COL_NAME,
    PH_CLR_MEMBERREF_REC_COL_SIGNATURE
} PH_CLR_MEMBERREF_REC;

typedef enum _PH_CLR_CONSTANT_REC
{
    PH_CLR_CONSTANT_REC_COL_TYPE = 0,
    PH_CLR_CONSTANT_REC_COL_PARENT,
    PH_CLR_CONSTANT_REC_COL_VALUE
} PH_CLR_CONSTANT_REC;

typedef enum _PH_CLR_CUSTOMATTRIBUTE_REC
{
    PH_CLR_CUSTOMATTRIBUTE_REC_COL_PARENT = 0,
    PH_CLR_CUSTOMATTRIBUTE_REC_COL_TYPE,
    PH_CLR_CUSTOMATTRIBUTE_REC_COL_VALUE
} PH_CLR_CUSTOMATTRIBUTE_REC;

typedef enum _PH_CLR_FIELDMARSHAL_REC
{
    PH_CLR_FIELDMARSHAL_REC_COL_PARENT = 0,
    PH_CLR_FIELDMARSHAL_REC_COL_NATIVETYPE
} PH_CLR_FIELDMARSHAL_REC;

typedef enum _PH_CLR_DECLSECURITY_REC
{
    PH_CLR_DECLSECURITY_REC_COL_ACTION = 0,
    PH_CLR_DECLSECURITY_REC_COL_PARENT,
    PH_CLR_DECLSECURITY_REC_COL_PERMISSIONSET
} PH_CLR_DECLSECURITY_REC;

typedef enum _PH_CLR_CLASSLAYOUT_REC
{
    PH_CLR_CLASSLAYOUT_REC_COL_PACKINGSIZE = 0,
    PH_CLR_CLASSLAYOUT_REC_COL_CLASSSIZE,
    PH_CLR_CLASSLAYOUT_REC_COL_PARENT
} PH_CLR_CLASSLAYOUT_REC;

typedef enum _PH_CLR_FIELDLAYOUT_REC
{
    PH_CLR_FIELDLAYOUT_REC_COL_OFFSET = 0,
    PH_CLR_FIELDLAYOUT_REC_COL_FIELD
} PH_CLR_FIELDLAYOUT_REC;

typedef enum _PH_CLR_STANDALONESIG_REC
{
    PH_CLR_STANDALONESIG_REC_COL_SIGNATURE = 0
} PH_CLR_STANDALONESIG_REC;

typedef enum _PH_CLR_EVENTMAP_REC
{
    PH_CLR_EVENTMAP_REC_COL_PARENT = 0,
    PH_CLR_EVENTMAP_REC_COL_EVENTLIST
} PH_CLR_EVENTMAP_REC;

typedef enum _PH_CLR_EVENTPTR_REC
{
    PH_CLR_EVENTPTR_REC_COL_EVENT = 0
} PH_CLR_EVENTPTR_REC;

typedef enum _PH_CLR_EVENT_REC
{
    PH_CLR_EVENT_REC_COL_FLAGS = 0,
    PH_CLR_EVENT_REC_COL_NAME,
    PH_CLR_EVENT_REC_COL_EVENTTYPE
} PH_CLR_EVENT_REC;

typedef enum _PH_CLR_PROPERTYMAP_REC
{
    PH_CLR_PROPERTYMAP_REC_COL_PARENT = 0,
    PH_CLR_PROPERTYMAP_REC_COL_PROPERTYLIST
} PH_CLR_PROPERTYMAP_REC;

typedef enum _PH_CLR_PROPERTYPTR_REC
{
    PH_CLR_PROPERTYPTR_REC_COL_PROPERTY = 0
} PH_CLR_PROPERTYPTR_REC;

typedef enum _PH_CLR_PROPERTY_REC
{
    PH_CLR_PROPERTY_REC_COL_FLAGS = 0,
    PH_CLR_PROPERTY_REC_COL_NAME,
    PH_CLR_PROPERTY_REC_COL_TYPE
} PH_CLR_PROPERTY_REC;

typedef enum _PH_CLR_METHODSEMANTICS_REC
{
    PH_CLR_METHODSEMANTICS_REC_COL_SEMANTICS = 0,
    PH_CLR_METHODSEMANTICS_REC_COL_METHOD,
    PH_CLR_METHODSEMANTICS_REC_COL_ASSOCIATION
} PH_CLR_METHODSEMANTICS_REC;

typedef enum _PH_CLR_METHODIMPL_REC
{
    PH_CLR_METHODIMPL_REC_COL_CLASS = 0,
    PH_CLR_METHODIMPL_REC_COL_METHODBODY,
    PH_CLR_METHODIMPL_REC_COL_METHODDECLARATION
} PH_CLR_METHODIMPL_REC;

typedef enum _PH_CLR_MODULEREF_REC
{
    PH_CLR_MODULEREF_REC_COL_NAME = 0
} PH_CLR_MODULEREF_REC;

typedef enum _PH_CLR_TYPESPEC_REC
{
    PH_CLR_TYPESPEC_REC_COL_SIGNATURE = 0
} PH_CLR_TYPESPEC_REC;

typedef enum _PH_CLR_IMPLMAP_REC
{
    PH_CLR_IMPLMAP_REC_COL_MAPPINGFLAGS = 0,
    PH_CLR_IMPLMAP_REC_COL_MEMBERFORWARDED,
    PH_CLR_IMPLMAP_REC_COL_IMPORTNAME,
    PH_CLR_IMPLMAP_REC_COL_IMPORTSCOPE
} PH_CLR_IMPLMAP_REC;

typedef enum _PH_CLR_FIELDRVA_REC
{
    PH_CLR_FIELDRVA_REC_COL_RVA = 0,
    PH_CLR_FIELDRVA_REC_COL_FIELD
} PH_CLR_FIELDRVA_REC;

typedef enum _PH_CLR_ENCLOG_REC
{
    PH_CLR_ENCLOG_REC_COL_ADVANCEMETHOD = 0,
    PH_CLR_ENCLOG_REC_COL_OPERATION
} PH_CLR_ENCLOG_REC;

typedef enum _PH_CLR_ENCMAP_REC
{
    PH_CLR_ENCMAP_REC_COL_PTOKEN = 0
} PH_CLR_ENCMAP_REC;

typedef enum _PH_CLR_ASSEMBLY_REC
{
    PH_CLR_ASSEMBLY_REC_COL_HASHALGID = 0,
    PH_CLR_ASSEMBLY_REC_COL_MAJORVERSION,
    PH_CLR_ASSEMBLY_REC_COL_MINORVERSION,
    PH_CLR_ASSEMBLY_REC_COL_BUILDNUMBER,
    PH_CLR_ASSEMBLY_REC_COL_REVISIONNUMBER,
    PH_CLR_ASSEMBLY_REC_COL_FLAGS,
    PH_CLR_ASSEMBLY_REC_COL_PUBLICKEY,
    PH_CLR_ASSEMBLY_REC_COL_NAME,
    PH_CLR_ASSEMBLY_REC_COL_CULTURE
} PH_CLR_ASSEMBLY_REC;

typedef enum _PH_CLR_ASSEMBLYPROCESSOR_REC
{
    PH_CLR_ASSEMBLYPROCESSOR_REC_COL_PROCESSOR = 0
} PH_CLR_ASSEMBLYPROCESSOR_REC;

typedef enum _PH_CLR_ASSEMBLYOS_REC
{
    PH_CLR_ASSEMBLYOS_REC_COL_OSPLATFORMID = 0,
    PH_CLR_ASSEMBLYOS_REC_COL_OSMAJORVERSION,
    PH_CLR_ASSEMBLYOS_REC_COL_OSMINORVERSION
} PH_CLR_ASSEMBLYOS_REC;

typedef enum _PH_CLR_ASSEMBLYREF_REC
{
    PH_CLR_ASSEMBLYREF_REC_COL_MAJORVERSION = 0,
    PH_CLR_ASSEMBLYREF_REC_COL_MINORVERSION,
    PH_CLR_ASSEMBLYREF_REC_COL_BUILDNUMBER,
    PH_CLR_ASSEMBLYREF_REC_COL_REVISIONNUMBER,
    PH_CLR_ASSEMBLYREF_REC_COL_FLAGS,
    PH_CLR_ASSEMBLYREF_REC_COL_PUBLICKEYORTOKEN,
    PH_CLR_ASSEMBLYREF_REC_COL_NAME,
    PH_CLR_ASSEMBLYREF_REC_COL_CULTURE,
    PH_CLR_ASSEMBLYREF_REC_COL_HASHVALUE
} PH_CLR_ASSEMBLYREF_REC;

typedef enum _PH_CLR_ASSEMBLYREFPROCESSOR_REC
{
    PH_CLR_ASSEMBLYREFPROCESSOR_REC_COL_PROCESSOR = 0,
    PH_CLR_ASSEMBLYREFPROCESSOR_REC_COL_ASSEMBLYREF
} PH_CLR_ASSEMBLYREFPROCESSOR_REC;

typedef enum _PH_CLR_ASSEMBLYREFOS_REC
{
    PH_CLR_ASSEMBLYREFOS_REC_COL_OSPLATFORMID = 0,
    PH_CLR_ASSEMBLYREFOS_REC_COL_OSMAJORVERSION,
    PH_CLR_ASSEMBLYREFOS_REC_COL_OSMINORVERSION,
    PH_CLR_ASSEMBLYREFOS_REC_COL_ASSEMBLYREF
} PH_CLR_ASSEMBLYREFOS_REC;

typedef enum _PH_CLR_FILE_REC
{
    PH_CLR_FILE_REC_COL_FLAGS = 0,
    PH_CLR_FILE_REC_COL_NAME,
    PH_CLR_FILE_REC_COL_HASHVALUE
} PH_CLR_FILE_REC;

typedef enum _PH_CLR_EXPORTEDTYPE_REC
{
    PH_CLR_EXPORTEDTYPE_REC_COL_FLAGS = 0,
    PH_CLR_EXPORTEDTYPE_REC_COL_TYPEDEFID,
    PH_CLR_EXPORTEDTYPE_REC_COL_TYPENAME,
    PH_CLR_EXPORTEDTYPE_REC_COL_TYPENAMESPACE,
    PH_CLR_EXPORTEDTYPE_REC_COL_IMPLEMENTATION
} PH_CLR_EXPORTEDTYPE_REC;

typedef enum _PH_CLR_MANIFESTRESOURCE_REC
{
    PH_CLR_MANIFESTRESOURCE_REC_COL_OFFSET = 0,
    PH_CLR_MANIFESTRESOURCE_REC_COL_FLAGS,
    PH_CLR_MANIFESTRESOURCE_REC_COL_NAME,
    PH_CLR_MANIFESTRESOURCE_REC_COL_IMPLEMENTATION
} PH_CLR_MANIFESTRESOURCE_REC;

typedef enum _PH_CLR_NESTEDCLASS_REC
{
    PH_CLR_NESTEDCLASS_REC_COL_NESTEDCLASS = 0,
    PH_CLR_NESTEDCLASS_REC_COL_ENCLOSINGCLASS
} PH_CLR_NESTEDCLASS_REC;

typedef enum _PH_CLR_GENERICPARAM_REC
{
    PH_CLR_GENERICPARAM_REC_COL_NUMBER = 0,
    PH_CLR_GENERICPARAM_REC_COL_FLAGS,
    PH_CLR_GENERICPARAM_REC_COL_OWNER,
    PH_CLR_GENERICPARAM_REC_COL_NAME
} PH_CLR_GENERICPARAM_REC;

typedef enum _PH_CLR_METHODSPEC_REC
{
    PH_CLR_METHODSPEC_REC_COL_METHOD = 0,
    PH_CLR_METHODSPEC_REC_COL_INSTANTIATION
} PH_CLR_METHODSPEC_REC;

typedef enum _PH_CLR_GENERICPARAMCONSTRAINT_REC
{
    PH_CLR_GENERICPARAMCONSTRAINT_REC_COL_OWNER = 0,
    PH_CLR_GENERICPARAMCONSTRAINT_REC_COL_CONSTRAINT
} PH_CLR_GENERICPARAMCONSTRAINT_REC;

typedef enum _PH_CLR_DOCUMENT_REC
{
    PH_CLR_DOCUMENT_REC_COL_NAME = 0,
    PH_CLR_DOCUMENT_REC_COL_HASHALGORITHM,
    PH_CLR_DOCUMENT_REC_COL_HASH,
    PH_CLR_DOCUMENT_REC_COL_LANGUAGE
} PH_CLR_DOCUMENT_REC;

typedef enum _PH_CLR_METHODDEBUGINFORMATION_REC
{
    PH_CLR_METHODDEBUGINFORMATION_REC_COL_DOCUMENT = 0,
    PH_CLR_METHODDEBUGINFORMATION_REC_COL_SEQUENCEPOINTS
} PH_CLR_METHODDEBUGINFORMATION_REC;

typedef enum _PH_CLR_LOCALSCOPE_REC
{
    PH_CLR_LOCALSCOPE_REC_COL_METHOD = 0,
    PH_CLR_LOCALSCOPE_REC_COL_IMPORTSCOPE,
    PH_CLR_LOCALSCOPE_REC_COL_VARIABLELIST,
    PH_CLR_LOCALSCOPE_REC_COL_CONSTANTLIST,
    PH_CLR_LOCALSCOPE_REC_COL_STARTOFFSET,
    PH_CLR_LOCALSCOPE_REC_COL_LENGTH
} PH_CLR_LOCALSCOPE_REC;

typedef enum _PH_CLR_LOCALVARIABLE_REC
{
    PH_CLR_LOCALVARIABLE_REC_COL_ATTRIBUTES = 0,
    PH_CLR_LOCALVARIABLE_REC_COL_INDEX,
    PH_CLR_LOCALVARIABLE_REC_COL_NAME
} PH_CLR_LOCALVARIABLE_REC;

typedef enum _PH_CLR_LOCALCONSTANT_REC
{
    PH_CLR_LOCALCONSTANT_REC_COL_NAME = 0,
    PH_CLR_LOCALCONSTANT_REC_COL_SIGNATURE
} PH_CLR_LOCALCONSTANT_REC;

typedef enum _PH_CLR_IMPORTSCOPE_REC
{
    PH_CLR_IMPORTSCOPE_REC_COL_PARENT = 0,
    PH_CLR_IMPORTSCOPE_REC_COL_IMPORTS
} PH_CLR_IMPORTSCOPE_REC;

typedef enum _PH_CLR_STATEMACHINEMETHOD_REC
{
    PH_CLR_STATEMACHINEMETHOD_REC_COL_MOVENEXTMETHOD = 0,
    PH_CLR_STATEMACHINEMETHOD_REC_COL_KICKOFFMETHOD
} PH_CLR_STATEMACHINEMETHOD_REC;

typedef enum _PH_CLR_CUSTOMDEBUGINFORMATION_REC
{
    PH_CLR_CUSTOMDEBUGINFORMATION_REC_COL_OWNER = 0,
    PH_CLR_CUSTOMDEBUGINFORMATION_REC_COL_KIND,
    PH_CLR_CUSTOMDEBUGINFORMATION_REC_COL_VALUE
} PH_CLR_CUSTOMDEBUGINFORMATION_REC;

#define PH_CLR_STORAGE_MAGIC 0x424A5342 // "BSJB"

// HeapSizes flag indicating an extra 4-byte field follows the row-count array.
#define PH_CLR_HEAPSIZE_EXTRADATA 0x40

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

C_ASSERT(sizeof(PH_CLR_STORAGESIGNATURE) == 16);
C_ASSERT(sizeof(PH_CLR_STORAGEHEADER) == 4);
C_ASSERT(UFIELD_OFFSET(PH_CLR_STORAGESTREAM, Name) == 8);
C_ASSERT(sizeof(PH_CLR_TABLESHEADER) == 24);

// #~ stream header HeapSizes bitfield (ECMA-335 II.24.2.6)
#define PH_CLR_HEAPSIZE_STRING  0x01 // #Strings indexes are 4 bytes
#define PH_CLR_HEAPSIZE_GUID    0x02 // #GUID indexes are 4 bytes
#define PH_CLR_HEAPSIZE_BLOB    0x04 // #Blob indexes are 4 bytes

// Column value type returned by PhGetMappedClrColumnInfo
typedef enum _PH_CLR_COLUMN_TYPE
{
    PH_CLR_COLUMN_FIXED,        // fixed-width scalar (1/2/4/8 bytes)
    PH_CLR_COLUMN_STRING,       // index into #Strings
    PH_CLR_COLUMN_GUID,         // index into #GUID
    PH_CLR_COLUMN_BLOB,         // index into #Blob
    PH_CLR_COLUMN_TABLE,        // RID into a single table
    PH_CLR_COLUMN_CODED         // coded index across a set of tables
} PH_CLR_COLUMN_TYPE;

typedef struct _PH_MAPPED_CLR_COLUMN
{
    UCHAR Type;         // PH_CLR_COLUMN_TYPE
    UCHAR Size;         // width in bytes (1/2/4/8)
    UCHAR Offset;       // offset within the row
    UCHAR Codec;        // PH_CLR_COLUMN_TABLE -> table index; PH_CLR_COLUMN_CODED -> coded-index set
} PH_MAPPED_CLR_COLUMN, *PPH_MAPPED_CLR_COLUMN;

#define PH_CLR_MAX_COLUMNS 9

// Coded-index sets (ECMA-335 II.24.2.6 + Portable PDB).
typedef enum _PH_CLR_CODED_INDEX
{
    CIX_TYPEDEFORREF,
    CIX_HASCONSTANT,
    CIX_HASCUSTOMATTRIBUTE,
    CIX_HASFIELDMARSHALL,
    CIX_HASDECLSECURITY,
    CIX_MEMBERREFPARENT,
    CIX_HASSEMANTICS,
    CIX_METHODDEFORREF,
    CIX_MEMBERFORWARDED,
    CIX_IMPLEMENTATION,
    CIX_CUSTOMATTRIBUTETYPE,
    CIX_RESOLUTIONSCOPE,
    CIX_TYPEORMETHODDEF,
    CIX_HASCUSTOMDEBUGINFORMATION,
    CIX_MAXIMUM
} PH_CLR_CODED_INDEX;

#define CIX_RESERVED 0xFF
#define CIX_MAX_TABLES 28

typedef struct _PH_CLR_CODED_INDEX_INFO
{
    UCHAR Bits;
    UCHAR Count;
    UCHAR Tables[CIX_MAX_TABLES];
} PH_CLR_CODED_INDEX_INFO, *PPH_CLR_CODED_INDEX_INFO;

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

#define PH_CLR_SCHEMA_TABLE(x) (CC_TBL | PH_CLR_TABLE_##x)
#define PH_CLR_SCHEMA_CODED(x) (CC_COD | CIX_##x)

// P/Invoke mapping flags (CorHdr.h).
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

typedef struct _PH_CLR_TABLE_SCHEMA
{
    PCSTR Name;
    UCHAR Columns[PH_CLR_MAX_COLUMNS];
} PH_CLR_TABLE_SCHEMA, *PPH_CLR_TABLE_SCHEMA;

typedef struct _PH_MAPPED_CLR_TABLE
{
    ULONG RowCount;
    ULONG RowSize;
    ULONG ColumnCount;
    PVOID Rows;         // VA of the first row (NULL when RowCount == 0)
    PCSTR Name;
    PH_MAPPED_CLR_COLUMN Columns[PH_CLR_MAX_COLUMNS];
} PH_MAPPED_CLR_TABLE, *PPH_MAPPED_CLR_TABLE;

typedef struct _PH_MAPPED_CLR_METADATA
{
    PPH_MAPPED_IMAGE MappedImage;
    PIMAGE_COR20_HEADER Cor20Header;
    PVOID MetadataAddress;          // VA of the BSJB storage signature
    ULONG MetadataSize;

    PVOID TablesStream;             // "#~" or "#-"
    ULONG TablesStreamSize;
    PSTR StringHeap;                // "#Strings"
    ULONG StringHeapSize;
    PVOID BlobHeap;                 // "#Blob"
    ULONG BlobHeapSize;
    PVOID GuidHeap;                 // "#GUID"
    ULONG GuidHeapSize;
    PVOID UserStringHeap;           // "#US"
    ULONG UserStringHeapSize;
    PVOID PdbStream;                // "#Pdb"
    ULONG PdbStreamSize;

    UCHAR MajorVersion;
    UCHAR MinorVersion;
    UCHAR HeapSizes;
    ULONG64 ValidMask;
    ULONG64 SortedMask;
    BOOLEAN Uncompressed;           // "#-" stream

    // Standalone Portable PDB backing view and external type-system row counts.
    // A successfully initialized context must not be copied or moved.
    PH_MAPPED_IMAGE PortablePdbView;
    ULONG ExternalRowCounts[PH_CLR_TABLE_MAXIMUM];
    PH_MAPPED_CLR_TABLE Tables[PH_CLR_TABLE_MAXIMUM];
} PH_MAPPED_CLR_METADATA, *PPH_MAPPED_CLR_METADATA;

static FORCEINLINE ULONG64 PhClrReadInteger(
    _In_ PVOID Pointer,
    _In_ ULONG Size
    )
{
    switch (Size)
    {
    case 1:
        return *(PUCHAR)Pointer;
    case 2:
        return *(PUSHORT)Pointer;
    case 4:
        return *(PULONG)Pointer;
    case 8:
        return *(PULONG64)Pointer;
    }

    return 0;
}

// Computes whether a table RID index needs 2 or 4 bytes based on row count.
static FORCEINLINE ULONG PhClrSimpleIndexSize(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex
    )
{
    if (TableIndex < PH_CLR_TABLE_MAXIMUM && (ClrMetadata->Tables[TableIndex].RowCount > 0xFFFF ||
        ClrMetadata->ExternalRowCounts[TableIndex] > 0xFFFF))
        return 4;

    return 2;
}

/**
 * Initializes a CLR metadata structure from a mapped image.
 *
 * \param ClrMetadata The CLR metadata structure to initialize.
 * \param MappedImage The mapped image.
 * \return STATUS_SUCCESS on success, or an appropriate NTSTATUS error code.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhInitializeMappedClrMetadata(
    _Out_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

/**
 * Deletes a CLR metadata structure.
 *
 * \param ClrMetadata The CLR metadata structure to delete.
 */
PHLIBAPI
VOID
NTAPI
PhDeleteMappedClrMetadata(
    _Inout_ PPH_MAPPED_CLR_METADATA ClrMetadata
    );

/**
 * Array of CLR table column names (schema field names).
 * Indexed by PH_CLR_TABLE enum, with up to PH_CLR_MAX_COLUMNS entries per table.
 * Based on ECMA-335 metadata specification.
 */
extern const PCWSTR PhClrTableColumnNames[PH_CLR_TABLE_MAXIMUM][PH_CLR_MAX_COLUMNS];

/**
 * Retrieves the number of tables in the CLR metadata.
 *
 * \param ClrMetadata The CLR metadata structure.
 * \return The number of tables.
 */
PHLIBAPI
ULONG
NTAPI
PhGetMappedClrNumberOfTables(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata
    );

/**
 * Retrieves information about a CLR metadata table.
 *
 * \param ClrMetadata The CLR metadata structure.
 * \param TableIndex The index of the table.
 * \param RowSize Receives the size of a row in bytes.
 * \param RowCount Receives the number of rows in the table.
 * \param ColumnCount Receives the number of columns in the table.
 * \param Name Receives the name of the table.
 * \return STATUS_SUCCESS on success, or an appropriate NTSTATUS error code.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrTableInfo(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _Out_opt_ PULONG RowSize,
    _Out_opt_ PULONG RowCount,
    _Out_opt_ PULONG ColumnCount,
    _Out_opt_ PCSTR *Name
    );

/**
 * Retrieves extended information about a CLR metadata table.
 *
 * \param ClrMetadata The CLR metadata structure.
 * \param TableIndex The index of the table.
 * \param RowSize Receives the size of a row in bytes.
 * \param RowCount Receives the number of rows in the table.
 * \param ColumnCount Receives the number of columns in the table.
 * \param Name Receives the name of the table.
 * \param Table Receives a pointer to the internal table structure.
 * \return STATUS_SUCCESS on success, or an appropriate NTSTATUS error code.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrTableInfoEx(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _Out_opt_ PULONG RowSize,
    _Out_opt_ PULONG RowCount,
    _Out_opt_ PULONG ColumnCount,
    _Out_opt_ PCSTR *Name,
    _Out_opt_ PPH_MAPPED_CLR_TABLE *Table
    );

/**
 * Retrieves information about a column in a CLR metadata table.
 *
 * \param ClrMetadata The CLR metadata structure.
 * \param TableIndex The index of the table.
 * \param Column The index of the column.
 * \param Offset Receives the offset of the column within a row.
 * \param Size Receives the size of the column in bytes.
 * \param Type Receives the type of the column.
 * \return STATUS_SUCCESS on success, or an appropriate NTSTATUS error code.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrColumnInfo(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Column,
    _Out_opt_ PULONG Offset,
    _Out_opt_ PULONG Size,
    _Out_opt_ PULONG Type
    );

/**
 * Retrieves a pointer to a row in a CLR metadata table.
 *
 * \param ClrMetadata The CLR metadata structure.
 * \param TableIndex The index of the table.
 * \param Rid The 1-based RID of the row.
 * \param Row Receives a pointer to the row data.
 * \return STATUS_SUCCESS on success, or an appropriate NTSTATUS error code.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrTableRow(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Rid,
    _Out_ PVOID *Row
    );

/**
 * Retrieves the value of a column in a CLR metadata table.
 *
 * \param ClrMetadata The CLR metadata structure.
 * \param TableIndex The index of the table.
 * \param Column The index of the column.
 * \param Rid The 1-based RID of the row.
 * \param Value Receives the column value.
 * \return STATUS_SUCCESS on success, or an appropriate NTSTATUS error code.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrColumnValue(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Column,
    _In_ ULONG Rid,
    _Out_ PULONG Value
    );

/**
 * Retrieves the size of the CLR metadata string heap.
 *
 * \param ClrMetadata The CLR metadata structure.
 * \return The size of the string heap in bytes.
 */
PHLIBAPI
ULONG
NTAPI
PhGetMappedClrStringHeapSize(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata
    );

/**
 * Retrieves a string from the CLR metadata string heap.
 *
 * \param ClrMetadata The CLR metadata structure.
 * \param Index The offset of the string in the heap.
 * \param String Receives a pointer to the null-terminated UTF-8 string.
 * \param Length Optionally receives the UTF-8 string length in bytes, excluding the null terminator.
 * \return STATUS_SUCCESS on success, or an appropriate NTSTATUS error code.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrString(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Index,
    _Out_ PCSTR *String,
    _Out_opt_ PULONG Length
    );

/**
 * Retrieves a blob from the CLR metadata blob heap.
 *
 * \param ClrMetadata The CLR metadata structure.
 * \param Index The offset of the blob in the heap.
 * \param Data Receives a pointer to the blob data.
 * \param Length Receives the size of the blob in bytes.
 * \return STATUS_SUCCESS on success, or an appropriate NTSTATUS error code.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrBlob(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Index,
    _Out_ PVOID *Data,
    _Out_ PULONG Length
    );

/**
 * Retrieves a GUID from the CLR metadata GUID heap.
 *
 * \param ClrMetadata The CLR metadata structure.
 * \param Index The 1-based index of the GUID in the heap.
 * \param Guid Receives the GUID.
 * \return STATUS_SUCCESS on success, or an appropriate NTSTATUS error code.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrGuid(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Index,
    _Out_ PGUID Guid
    );

/**
 * Retrieves a string from a CLR metadata table column as a PH_STRING.
 *
 * \param ClrMetadata The CLR metadata structure.
 * \param TableIndex The index of the table.
 * \param Row The 1-based row index.
 * \param Column The 0-based column index.
 * \return A string representing the column value, or NULL if not found.
 */
PHLIBAPI
PPH_STRING
NTAPI
PhGetMappedClrTableString(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Row,
    _In_ ULONG Column
    );

/**
 * Retrieves a RID (Relative Index) from a CLR metadata table column.
 *
 * \param ClrMetadata The CLR metadata structure.
 * \param TableIndex The index of the table.
 * \param Row The 1-based row index.
 * \param Column The 0-based column index.
 * \param Rid Receives the RID.
 * \return STATUS_SUCCESS on success, or an appropriate NTSTATUS error code.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrTableRowRid(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Row,
    _In_ ULONG Column,
    _Out_ PULONG Rid
    );

/**
 * Retrieves the schema name of a CLR metadata table.
 *
 * \param TableIndex The index of the table.
 * \return The ECMA-335 table name, or NULL when the index is reserved or unknown.
 */
PHLIBAPI
PCSTR
NTAPI
PhGetMappedClrTableName(
    _In_ ULONG TableIndex
    );

/**
 * Decodes a table index or coded index column into the table and row it references.
 *
 * \param ClrMetadata The CLR metadata structure.
 * \param TableIndex The index of the table.
 * \param Column The 0-based column index.
 * \param Rid The 1-based RID of the row.
 * \param TargetTable Receives the referenced table index.
 * \param TargetRid Receives the referenced 1-based RID (zero when the reference is nil).
 * \param Token Receives the metadata token of the reference.
 * \return STATUS_SUCCESS on success, or an appropriate NTSTATUS error code. Columns which are
 * not table or coded indexes return STATUS_INVALID_PARAMETER.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrColumnToken(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Column,
    _In_ ULONG Rid,
    _Out_opt_ PULONG TargetTable,
    _Out_opt_ PULONG TargetRid,
    _Out_opt_ PULONG Token
    );

/**
 * Retrieves the name of a custom attribute target and checks if it's the TargetFrameworkAttribute.
 *
 * \param ClrMetadata The CLR metadata structure.
 * \param Row The 1-based row index in the CustomAttribute table.
 * \param IsTargetFrameworkAttribute Receives TRUE if the attribute is TargetFrameworkAttribute.
 * \return STATUS_SUCCESS on success, or an appropriate NTSTATUS error code.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrCustomAttributeTargetName(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Row,
    _Out_ PBOOLEAN IsTargetFrameworkAttribute
    );

/**
 * Attempts to retrieve the target framework version from the CLR metadata.
 *
 * \param ClrMetadata The CLR metadata structure.
 * \param Version Receives the framework version string.
 * \return TRUE if the version was successfully retrieved.
 */
PHLIBAPI
BOOLEAN
NTAPI
PhTryGetMappedClrTargetFramework(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _Out_ PPH_STRING* Version
    );

/**
 * Converts CLR import flags to a human-readable string.
 *
 * \param Flags The import flags.
 * \return A string representing the flags. The caller is responsible for dereferencing the string.
 */
PHLIBAPI
PPH_STRING
NTAPI
PhClrImportFlagsToString(
    _In_ ULONG Flags
    );

/**
 * Enumeration callback for CLR metadata tables.
 *
 * \param TableIndex The index of the table.
 * \param RowSize The size of a row in bytes.
 * \param RowCount The number of rows in the table.
 * \param Name The name of the table.
 * \param Rows A pointer to the table rows.
 * \param Context A user-defined context.
 * \return TRUE to continue enumeration, FALSE to stop.
 */
typedef _Function_class_(PH_CLR_ENUM_TABLES_CALLBACK)
BOOLEAN NTAPI PH_CLR_ENUM_TABLES_CALLBACK(
    _In_ ULONG TableIndex,
    _In_ ULONG RowSize,
    _In_ ULONG RowCount,
    _In_ PCSTR Name,
    _In_opt_ PVOID Rows,
    _In_opt_ PVOID Context
    );
typedef PH_CLR_ENUM_TABLES_CALLBACK* PPH_CLR_ENUM_TABLES_CALLBACK;

/**
 * Enumerates all present tables in the CLR metadata.
 *
 * \param ClrMetadata The CLR metadata structure.
 * \param Callback A callback function to be called for each table.
 * \param Context A user-defined context to be passed to the callback.
 * \return STATUS_SUCCESS on success, or an appropriate NTSTATUS error code.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhEnumMappedClrTables(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ PPH_CLR_ENUM_TABLES_CALLBACK Callback,
    _In_opt_ PVOID Context
    );


/**
 * Returns an owned UTF-16 string from a #Strings byte offset.
 * Outputs are unchanged on failure. Dereference owned strings after use.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrStringEx(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Index,
    _Out_ PPH_STRING *String
    );

/**
 * Returns an owned string from a string column, with failure status.
 * Outputs are unchanged on failure. Dereference owned strings after use.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrTableStringEx(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Row,
    _In_ ULONG Column,
    _Out_ PPH_STRING* String
    );

/**
 * Returns borrowed blob data valid only while the image remains mapped.
 * Outputs are unchanged on failure. Length is in bytes.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrTableBlob(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Row,
    _In_ ULONG Column,
    _Out_ PVOID *Data,
    _Out_ PULONG Length
    );

/**
 * Copies the GUID referenced by a GUID column.
 * Outputs are unchanged on failure.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrTableGuid(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG TableIndex,
    _In_ ULONG Row,
    _In_ ULONG Column,
    _Out_ PGUID Guid
    );

/**
 * Returns an owned #US string from a byte offset, not a token; preserves embedded nulls and excludes the trailing flag.
 * Outputs are unchanged on failure. Dereference owned strings after use.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrUserString(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Index,
    _Out_ PPH_STRING *String
    );


/** Names are owned references; blobs are borrowed for the mapped-image lifetime.
 * Release a successful result with PhDeleteMappedClrAssemblyProps.
 * Assembly results have no HashValue; AssemblyRef results have no HashAlgorithm.
 */
typedef struct _PH_MAPPED_CLR_ASSEMBLY_PROPS
{
    PPH_STRING Name;
    PPH_STRING Culture;
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG BuildNumber;
    ULONG RevisionNumber;
    ULONG Flags;
    ULONG HashAlgorithm;
    PVOID PublicKeyOrToken;
    ULONG PublicKeyOrTokenLength;
    PVOID HashValue;
    ULONG HashValueLength;
} PH_MAPPED_CLR_ASSEMBLY_PROPS, *PPH_MAPPED_CLR_ASSEMBLY_PROPS;

/** Takes a CustomAttribute token; returned blob is borrowed, lengths are bytes.
 * Outputs are unchanged on failure.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrCustomAttributeProps(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Token,
    _Out_opt_ PULONG ParentToken,
    _Out_opt_ PULONG ConstructorToken,
    _Out_opt_ PVOID *Data,
    _Out_opt_ PULONG Length
    );

/** Takes a non-nil parent token and case-sensitive Namespace.Type name (nested types use +).
 * Returns the first matching borrowed blob; TypeSpec constructors return STATUS_NOT_SUPPORTED.
 * Outputs are unchanged on failure.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrCustomAttributeByName(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG ParentToken,
    _In_ PCWSTR Name,
    _Out_ PVOID *Data,
    _Out_ PULONG Length
    );

/** Takes a FieldDef or MethodDef token. ImportName is owned; caller must dereference it.
 * Outputs are unchanged on failure.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrPinvokeMap(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Token,
    _Out_ PULONG MappingFlags,
    _Out_ PPH_STRING *ImportName,
    _Out_ PULONG ModuleToken
    );

/** Releases owned names and zeroes a successful assembly result. */
PHLIBAPI
VOID
NTAPI
PhDeleteMappedClrAssemblyProps(
    _Inout_ PPH_MAPPED_CLR_ASSEMBLY_PROPS Properties
    );

/** Takes an Assembly token and returns properties; release with PhDeleteMappedClrAssemblyProps.
 * Outputs are unchanged on failure.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrAssemblyProps(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Token,
    _Out_ PPH_MAPPED_CLR_ASSEMBLY_PROPS Properties
    );

/** Takes an AssemblyRef token and returns properties; release with PhDeleteMappedClrAssemblyProps.
 * Outputs are unchanged on failure.
 */
PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedClrAssemblyRefProps(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG Token,
    _Out_ PPH_MAPPED_CLR_ASSEMBLY_PROPS Properties
    );


#define PH_CLR_HIDDEN_LINE 0x00feefee
#define PH_PORTABLE_PDB_ID_SIZE 20

typedef struct _PH_PORTABLE_PDB_INFO
{
    UCHAR Id[PH_PORTABLE_PDB_ID_SIZE];
    ULONG EntryPoint;
    ULONG64 ReferencedTables;
    ULONG RowCounts[PH_CLR_TABLE_MAXIMUM];
} PH_PORTABLE_PDB_INFO, *PPH_PORTABLE_PDB_INFO;

typedef struct _PH_CLR_DOCUMENT
{
    PPH_STRING Name; // owned reference
    GUID Language;
    GUID HashAlgorithm;
    PVOID Hash; // borrowed until the PDB buffer is released
    ULONG HashLength;
} PH_CLR_DOCUMENT, *PPH_CLR_DOCUMENT;

typedef struct _PH_CLR_SEQUENCE_POINT
{
    ULONG IlOffset;
    ULONG Document; // Document RID
    ULONG StartLine;
    ULONG StartColumn;
    ULONG EndLine;
    ULONG EndColumn;
    BOOLEAN Hidden;
} PH_CLR_SEQUENCE_POINT, *PPH_CLR_SEQUENCE_POINT;

typedef struct _PH_CLR_SOURCE_LOCATION
{
    PH_CLR_SEQUENCE_POINT Point;
    PPH_STRING FileName; // owned; NULL for a hidden point
} PH_CLR_SOURCE_LOCATION, *PPH_CLR_SOURCE_LOCATION;

typedef BOOLEAN (NTAPI *PPH_CLR_SEQUENCE_POINT_CALLBACK)(
    _In_ PPH_CLR_SEQUENCE_POINT Point,
    _In_opt_ PVOID Context
    );

/** Borrows a standalone Portable PDB buffer. Context and buffer must remain at stable addresses.
 * Failure leaves ClrMetadata unchanged. No mapping is created or owned by this API.
 */
PHLIBAPI NTSTATUS NTAPI PhInitializeMappedPortablePdb(
    _Out_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ SIZE_T Length
    );
PHLIBAPI NTSTATUS NTAPI PhGetMappedPortablePdbInfo(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _Out_ PPH_PORTABLE_PDB_INFO Info
    );
/** Outputs commit on success. Dereference Document->Name; Hash remains borrowed. */
PHLIBAPI NTSTATUS NTAPI PhGetMappedClrDocument(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG DocumentRid,
    _Out_ PPH_CLR_DOCUMENT Document
    );
/** MethodToken is a MethodDef token. Callback FALSE stops successfully; points are temporary.
 * No sequence points returns STATUS_NOT_FOUND. Later malformed records may fail after callbacks.
 */
PHLIBAPI NTSTATUS NTAPI PhEnumMappedClrSequencePoints(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG MethodToken,
    _In_ PPH_CLR_SEQUENCE_POINT_CALLBACK Callback,
    _In_opt_ PVOID Context
    );
/** Resolves the nearest sequence point at or before IlOffset, including hidden points.
 * No preceding point returns STATUS_NOT_FOUND. Hidden success has FileName == NULL.
 * Dereference FileName after use. Outputs are unchanged on failure.
 */
PHLIBAPI NTSTATUS NTAPI PhGetMappedClrSourceLocation(
    _In_ PPH_MAPPED_CLR_METADATA ClrMetadata,
    _In_ ULONG MethodToken,
    _In_ ULONG IlOffset,
    _Out_ PPH_CLR_SOURCE_LOCATION Location
    );

EXTERN_C_END

#endif
