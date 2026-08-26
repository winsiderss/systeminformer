/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s    2023
 *     dmex     2026
 *
 */

#include <peview.h>
#include "colmgr.h"

#include <overridecapabilities.h>

#define WM_PV_DYNRELOC_CONTEXTMENU (WM_APP + 801)

typedef enum _PV_DYNRELOC_TREE_COLUMN
{
    PV_DYNRELOC_TREE_COLUMN_RVA,
    PV_DYNRELOC_TREE_COLUMN_TYPE,
    PV_DYNRELOC_TREE_COLUMN_INFO,
    PV_DYNRELOC_TREE_COLUMN_SECTION,
    PV_DYNRELOC_TREE_COLUMN_SYMBOL,
    PV_DYNRELOC_TREE_COLUMN_MAXIMUM
} PV_DYNRELOC_TREE_COLUMN;

typedef struct _PV_DYNRELOC_NODE
{
    PH_TREENEW_NODE Node;

    ULONG64 UniqueId;
    PPH_STRING Rva;
    PPH_STRING Type;
    PPH_STRING Info;
    PPH_STRING Section;
    PPH_STRING Symbol;

    ULONG64 RvaValue;   // numeric key for sorting the RVA column
    BOOLEAN HasRvaValue; // FALSE for structural rows (Patch sites, Outcomes)

    struct _PV_DYNRELOC_NODE* Parent;
    PPH_LIST Children;

    PH_STRINGREF TextCache[PV_DYNRELOC_TREE_COLUMN_MAXIMUM];
} PV_DYNRELOC_NODE, *PPV_DYNRELOC_NODE;

typedef struct _PV_PE_DYNRELOC_CONTEXT
{
    HWND WindowHandle;
    HWND DialogHandle;
    HWND SearchHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_TN_FILTER_SUPPORT FilterSupport;
    PPH_LIST NodeList;
    PPH_LIST NodeRootList;
    ULONG_PTR SearchMatchHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
    HANDLE ThreadHandle;
    BOOLEAN Loading;
    BOOLEAN Cancel;
} PV_PE_DYNRELOC_CONTEXT, *PPV_PE_DYNRELOC_CONTEXT;

static PH_STRINGREF LoadingDynRelocText = PH_STRINGREF_INIT(L"Loading dynamic relocations from image...");
static PH_STRINGREF EmptyDynRelocText = PH_STRINGREF_INIT(L"There are no dynamic relocations to display.");

typedef struct _PV_DYNRELOC_OVERRIDE_GROUP
{
    ULONG OriginalRva;
    ULONG BDDOffset;
    PPH_IMAGE_DYNAMIC_RELOC_ENTRY Representative;
    PPH_LIST Sites;
    PPV_DYNRELOC_NODE RootNode;
} PV_DYNRELOC_OVERRIDE_GROUP, *PPV_DYNRELOC_OVERRIDE_GROUP;

BOOLEAN NTAPI PvDynRelocTreeNewCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    );

PPV_DYNRELOC_NODE PvAddDynRelocNode(
    _Inout_ PPV_PE_DYNRELOC_CONTEXT Context,
    _In_opt_ PPV_DYNRELOC_NODE ParentNode,
    _In_opt_ PWSTR Rva,
    _In_opt_ PWSTR Type,
    _In_opt_ PPH_STRING Info
    )
{
    static ULONG64 index = 0;
    PPV_DYNRELOC_NODE node;

    node = PhAllocateZero(sizeof(PV_DYNRELOC_NODE));
    node->UniqueId = ++index;
    PhInitializeTreeNewNode(&node->Node);

    // PhInitializeTreeNewNode defaults Expanded = TRUE; start collapsed so the tree opens compact.
    node->Node.Expanded = FALSE;

    node->Node.TextCache = node->TextCache;
    node->Node.TextCacheSize = PV_DYNRELOC_TREE_COLUMN_MAXIMUM;

    if (Rva) node->Rva = PhCreateString(Rva);
    if (Type) node->Type = PhCreateString(Type);
    node->Info = Info;
    node->Children = PhCreateList(1);

    // Derive a numeric key for RVA-column sorting only when the cell actually holds an address;
    // structural rows (Patch sites, Outcomes) sort as 0.
    if (Rva && Rva[0] == L'0' && (Rva[1] == L'x' || Rva[1] == L'X'))
    {
        ULONG64 value;
        PH_STRINGREF rvaRef;

        PhInitializeStringRefLongHint(&rvaRef, Rva);

        if (PhStringToUInt64(&rvaRef, 0, &value))
        {
            node->RvaValue = value;
            node->HasRvaValue = TRUE;
        }
    }

    PhAddItemList(Context->NodeList, node);

    if (ParentNode)
    {
        node->Parent = ParentNode;
        PhAddItemList(ParentNode->Children, node);
    }
    else
    {
        PhAddItemList(Context->NodeRootList, node);
    }

    return node;
}

VOID PvDestroyDynRelocNode(
    _In_ PPV_DYNRELOC_NODE Node
    )
{
    PhDereferenceObject(Node->Children);

    if (Node->Rva) PhDereferenceObject(Node->Rva);
    if (Node->Type) PhDereferenceObject(Node->Type);
    if (Node->Info) PhDereferenceObject(Node->Info);
    if (Node->Section) PhDereferenceObject(Node->Section);
    if (Node->Symbol) PhDereferenceObject(Node->Symbol);

    PhFree(Node);
}

// Fills the Section and Symbol columns for a node from a resolved image RVA.
VOID PvDynRelocSetRvaColumns(
    _In_ PPV_DYNRELOC_NODE Node,
    _In_ ULONG Rva
    )
{
    PVOID mappedVa;
    PIMAGE_SECTION_HEADER section;
    PPH_STRING symbol;

    if (NT_SUCCESS(PhMappedImageRvaToVa(&PvMappedImage, Rva, &mappedVa)))
    {
        if (NT_SUCCESS(PhMappedImageRvaToSection(&PvMappedImage, Rva, &section)))
        {
            WCHAR sectionName[IMAGE_SIZEOF_SHORT_NAME + 1];

            if (NT_SUCCESS(PhGetMappedImageSectionName(section, sectionName, RTL_NUMBER_OF(sectionName), NULL)))
                Node->Section = PhCreateString(sectionName);
        }
    }

    symbol = PhGetSymbolFromAddress(
        PvSymbolProvider,
        PTR_ADD_OFFSET(PvMappedImage.NtHeaders->OptionalHeader.ImageBase, Rva),
        NULL,
        NULL,
        NULL,
        NULL
        );

    if (symbol)
        Node->Symbol = symbol;
}

// Fills Section/Symbol for a node from an entry's captured patch location.
VOID PvDynRelocSetEntryColumns(
    _In_ PPV_DYNRELOC_NODE Node,
    _In_ PPH_IMAGE_DYNAMIC_RELOC_ENTRY Entry
    )
{
    PIMAGE_SECTION_HEADER section;
    PPH_STRING symbol;

    if (!Entry->MappedImageVa)
        return;

    if (NT_SUCCESS(PhMappedImageRvaToSection(
        &PvMappedImage,
        PtrToUlong(PTR_SUB_OFFSET(Entry->MappedImageVa, PvMappedImage.ViewBase)),
        &section
        )))
    {
        WCHAR sectionName[IMAGE_SIZEOF_SHORT_NAME + 1];

        if (NT_SUCCESS(PhGetMappedImageSectionName(section, sectionName, RTL_NUMBER_OF(sectionName), NULL)))
            Node->Section = PhCreateString(sectionName);
    }

    symbol = PhGetSymbolFromAddress(PvSymbolProvider, Entry->ImageBaseVa, NULL, NULL, NULL, NULL);

    if (symbol)
        Node->Symbol = symbol;
}

// Returns the per-type info label for a non-function-override DVRT entry, matching
// the labels the previous flat list used.
PPH_STRING PvDynRelocEntryInfoString(
    _In_ PPH_IMAGE_DYNAMIC_RELOC_ENTRY Entry,
    _Out_ PWSTR* TypeName
    )
{
    switch (Entry->Symbol)
    {
    case IMAGE_DYNAMIC_RELOCATION_ARM64X:
        {
            *TypeName = L"ARM64X";

            switch (Entry->ARM64X.RecordFixup.Type)
            {
            case IMAGE_DVRT_ARM64X_FIXUP_TYPE_ZEROFILL:
                switch (Entry->ARM64X.RecordFixup.Size)
                {
                case IMAGE_DVRT_ARM64X_FIXUP_SIZE_2BYTES: return PhCreateString(L"Zero 2 bytes");
                case IMAGE_DVRT_ARM64X_FIXUP_SIZE_4BYTES: return PhCreateString(L"Zero 4 bytes");
                case IMAGE_DVRT_ARM64X_FIXUP_SIZE_8BYTES: return PhCreateString(L"Zero 8 bytes");
                default: return PhCreateString(L"UNKNOWN");
                }
            case IMAGE_DVRT_ARM64X_FIXUP_TYPE_VALUE:
                switch (Entry->ARM64X.RecordFixup.Size)
                {
                case IMAGE_DVRT_ARM64X_FIXUP_SIZE_2BYTES: return PhFormatString(L"0x%04x", Entry->ARM64X.Value2);
                case IMAGE_DVRT_ARM64X_FIXUP_SIZE_4BYTES: return PhFormatString(L"0x%08lx", Entry->ARM64X.Value4);
                case IMAGE_DVRT_ARM64X_FIXUP_SIZE_8BYTES: return PhFormatString(L"0x%016llx", Entry->ARM64X.Value8);
                default: return PhCreateString(L"UNKNOWN");
                }
            case IMAGE_DVRT_ARM64X_FIXUP_TYPE_DELTA:
                if (Entry->ARM64X.RecordDelta.Sign)
                    return PhFormatString(L"-0x%llx", Entry->ARM64X.Delta);
                else
                    return PhFormatString(L"+0x%llx", Entry->ARM64X.Delta);
            default:
                return PhCreateString(L"UNKNOWN");
            }
        }
    case IMAGE_DYNAMIC_RELOCATION_GUARD_RF_PROLOGUE:
        *TypeName = L"RF_PROLOGUE";
        return PhFormatString(L"%u bytes", Entry->RFPrologue.PrologueByteCount);
    case IMAGE_DYNAMIC_RELOCATION_GUARD_RF_EPILOGUE:
        *TypeName = L"RF_EPILOGUE";
        return PhFormatString(
            L"%u epilogues, %u bytes, %u branches (%u byte elems)",
            Entry->RFEpilogue.EpilogueCount,
            Entry->RFEpilogue.EpilogueByteCount,
            Entry->RFEpilogue.BranchDescriptorCount,
            Entry->RFEpilogue.BranchDescriptorElementSize
            );
    case IMAGE_DYNAMIC_RELOCATION_GUARD_IMPORT_CONTROL_TRANSFER:
        *TypeName = L"IMPORT";
        return PhFormatString(
            L"IAT index %05x%ls",
            Entry->ImportControl.Record.IATIndex,
            Entry->ImportControl.Record.IndirectCall ? L" call" : L" branch"
            );
    case IMAGE_DYNAMIC_RELOCATION_ARM64_KERNEL_IMPORT_CALL_TRANSFER:
        *TypeName = L"ARM64_IMPORT";
        return PhFormatString(
            L"IAT %05x reg x%u%ls%ls",
            Entry->ARM64ImportControl.Record.IATIndex,
            Entry->ARM64ImportControl.Record.RegisterIndex,
            Entry->ARM64ImportControl.Record.IndirectCall ? L" BLR" : L" BR",
            Entry->ARM64ImportControl.Record.ImportType ? L" delay" : L""
            );
    case IMAGE_DYNAMIC_RELOCATION_GUARD_INDIR_CONTROL_TRANSFER:
        *TypeName = L"INDIRECT";
        return PhFormatString(
            L"%ls%ls%ls",
            Entry->IndirControl.Record.IndirectCall ? L"CALL " : L"",
            Entry->IndirControl.Record.RexWPrefix ? L"REXW " : L"",
            Entry->IndirControl.Record.CfgCheck ? L"CFG " : L""
            );
    case IMAGE_DYNAMIC_RELOCATION_GUARD_SWITCHTABLE_BRANCH:
        *TypeName = L"BRANCH";
        return PhFormatString(L"Register %u", Entry->SwitchBranch.Record.RegisterNumber);
    default:
        // Entries with no dedicated DVRT symbol carry an ordinary base-relocation record; name it
        // from the record type as the flat list did (this should only be ABS in practice, but the
        // rest are named for visibility rather than shown as a blank Type column).
        switch (Entry->Other.Record.Type)
        {
        case IMAGE_REL_BASED_ABSOLUTE:    *TypeName = L"ABS"; break;
        case IMAGE_REL_BASED_HIGH:        *TypeName = L"HIGH"; break;
        case IMAGE_REL_BASED_LOW:         *TypeName = L"LOW"; break;
        case IMAGE_REL_BASED_HIGHLOW:     *TypeName = L"HIGHLOW"; break;
        case IMAGE_REL_BASED_DIR64:       *TypeName = L"DIR64"; break;
        case IMAGE_REL_BASED_ARM_MOV32:   *TypeName = L"MOV32"; break;
        case IMAGE_REL_BASED_THUMB_MOV32: *TypeName = L"MOV32(T)"; break;
        default:                          *TypeName = L""; break;
        }
        return PhFormatString(L"0x%llx", Entry->Symbol);
    }
}

// Returns the RVA a non-function-override entry patches, matching the previous list.
ULONG PvDynRelocEntryRva(
    _In_ PPH_IMAGE_DYNAMIC_RELOC_ENTRY Entry
    )
{
    switch (Entry->Symbol)
    {
    case IMAGE_DYNAMIC_RELOCATION_ARM64X:
        return Entry->ARM64X.BlockRva + Entry->ARM64X.RecordFixup.Offset;
    case IMAGE_DYNAMIC_RELOCATION_GUARD_RF_PROLOGUE:
        return Entry->RFPrologue.BlockRva;
    case IMAGE_DYNAMIC_RELOCATION_GUARD_RF_EPILOGUE:
        return Entry->RFEpilogue.BlockRva;
    case IMAGE_DYNAMIC_RELOCATION_GUARD_IMPORT_CONTROL_TRANSFER:
        return Entry->ImportControl.BlockRva + Entry->ImportControl.Record.PageRelativeOffset;
    case IMAGE_DYNAMIC_RELOCATION_ARM64_KERNEL_IMPORT_CALL_TRANSFER:
        return Entry->ARM64ImportControl.BlockRva + (Entry->ARM64ImportControl.Record.PageRelativeOffset << 2);
    case IMAGE_DYNAMIC_RELOCATION_GUARD_INDIR_CONTROL_TRANSFER:
        return Entry->IndirControl.BlockRva + Entry->IndirControl.Record.PageRelativeOffset;
    case IMAGE_DYNAMIC_RELOCATION_GUARD_SWITCHTABLE_BRANCH:
        return Entry->SwitchBranch.BlockRva + Entry->SwitchBranch.Record.PageRelativeOffset;
    default:
        return Entry->Other.BlockRva + Entry->Other.Record.Offset;
    }
}

PWSTR PvDynRelocOverrideTypeName(
    _In_ ULONG Type
    )
{
    switch (Type)
    {
    case IMAGE_FUNCTION_OVERRIDE_INVALID: return L"INVALID";
    case IMAGE_FUNCTION_OVERRIDE_X64_REL32: return L"X64 REL32";
    case IMAGE_FUNCTION_OVERRIDE_ARM64_BRANCH26: return L"ARM64 BRANCH26";
    case IMAGE_FUNCTION_OVERRIDE_ARM64_THUNK: return L"ARM64 THUNK";
    default: return L"UNKNOWN";
    }
}

// Bytes the loader rewrites at a patch site, which is a property of the record type (taken from
// RtlpParseFunctionOverrideRelocations): a THUNK patch is twice the width of a branch/rel32 one.
// Returns 0 for types with no defined width.
ULONG PvDynRelocOverridePatchWidth(
    _In_ ULONG Type
    )
{
    switch (Type)
    {
    case IMAGE_FUNCTION_OVERRIDE_X64_REL32:
    case IMAGE_FUNCTION_OVERRIDE_ARM64_BRANCH26:
        return 4;
    case IMAGE_FUNCTION_OVERRIDE_ARM64_THUNK:
        return 8;
    default:
        return 0;
    }
}

typedef struct _PV_DYNRELOC_BDD_COLLECT
{
    PPH_LIST Nodes; // PH_FUNCTION_OVERRIDE_BDD_NODE (copied)
} PV_DYNRELOC_BDD_COLLECT, *PPV_DYNRELOC_BDD_COLLECT;

_Function_class_(PH_FUNCTION_OVERRIDE_NODE_CALLBACK)
BOOLEAN NTAPI PvDynRelocBddCollectCallback(
    _In_ PPH_IMAGE_DYNAMIC_RELOC_ENTRY Entry,
    _In_ PPH_FUNCTION_OVERRIDE_BDD_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPV_DYNRELOC_BDD_COLLECT collect = Context;

    // Nodes arrive in index order, so list position matches node Index.
    PhAddItemList(collect->Nodes, PhAllocateCopy(Node, sizeof(PH_FUNCTION_OVERRIDE_BDD_NODE)));

    return TRUE;
}

// Translates a single function-override BDD feature number into a human-readable term.
//
// Names come from the SDK overridecapabilities.h (OVRDCAP_*), which defines the capability numbering
// the linker emits into the BDD. Two families are deliberately not spelled out here:
//
//  - CPU-signature selectors (vendor / family / model and their extended forms) are fused by the
//    condition builder into "Intel family 6 model 0x8F", which reads far better than the 304
//    separate OVRDCAP_AMD64_CPU_* terms those four ranges span (16 family, 256 extended family,
//    16 model, 16 extended model). Only the vendor names are handled below.
//  - Capability-set markers (V1/V2/V3/V4_CAPSET) gate on the OS knowing a capability set at all
//    rather than on hardware, so they are named as capability sets to read differently from a
//    feature test.
//
// Feature numbers are per-architecture and never overlap (the header guarantees this, because AMD64
// code runs on ARM64 under emulation), so one switch covers both namespaces.
PPH_STRING PvDynRelocFeatureString(
    _In_ ULONG Feature
    )
{
    PWSTR name = NULL;

    // OVRDCAP_ALWAYS_OFF is 0x7FFFFFFF -- the top bit is CLEAR, so this must be tested before (not
    // inside) the top-bit branch below.
    if (Feature == OVRDCAP_ALWAYS_OFF)
        return PhCreateString(L"never");

    // A set top bit marks an "OS Special Function": the OS supplies the implementation (e.g. from
    // ntdll) rather than the image selecting one of its own bodies. The low 31 bits are still a
    // capability number, so name it if we can.
    if (Feature & 0x80000000)
    {
        ULONG capability = Feature & ~0x80000000ul;
        PPH_STRING inner = PvDynRelocFeatureString(capability);
        PPH_STRING result = PhFormatString(L"OS special function: %ls", inner->Buffer);

        PhDereferenceObject(inner);
        return result;
    }

    switch (Feature)
    {
    // ---- AMD64 -----------------------------------------------------------------------------

    // Vendor selectors. The remaining CPU-signature ranges are fused by the caller.
    case OVRDCAP_AMD64_CPU_MANUFACTURER_RECOGNIZED: name = L"any known CPU vendor"; break;
    case OVRDCAP_AMD64_CPU_MANUFACTURER_INTEL:      name = L"Intel"; break;
    case OVRDCAP_AMD64_CPU_MANUFACTURER_AMD:        name = L"AMD"; break;
    case OVRDCAP_AMD64_CPU_MANUFACTURER_VIA:        name = L"VIA"; break;

    // String/memory-copy instruction capabilities.
    case OVRDCAP_AMD64_ERMSB:                   name = L"ERMSB"; break;
    case OVRDCAP_AMD64_FAST_SHORT_REPMOV:       name = L"fast short REP MOV"; break;
    case OVRDCAP_AMD64_FAST_ZERO_LEN_REPMOV:    name = L"fast zero-length REP MOV"; break;
    case OVRDCAP_AMD64_FAST_SHORT_REPSTOSB:     name = L"fast short REP STOSB"; break;
    case OVRDCAP_AMD64_FAST_SHORT_REPCMPSB:     name = L"fast short REP CMPSB"; break;

    // Execution mode.
    case OVRDCAP_AMD64_USERMODE:                name = L"user mode"; break;
    case OVRDCAP_AMD64_KERNELMODE:              name = L"kernel mode"; break;

    // Instruction-set extensions.
    case OVRDCAP_AMD64_AVX:                     name = L"AVX"; break;
    case OVRDCAP_AMD64_AVX2:                    name = L"AVX2"; break;
    case OVRDCAP_AMD64_AVX512F:                 name = L"AVX-512F"; break;
    case OVRDCAP_AMD64_SSE41:                   name = L"SSE4.1"; break;

    // Control-flow guard dispatch variants (values fixed; hard coded in the compiler).
    case OVRDCAP_AMD64_CFG_CHECK_OPT:           name = L"CFG check optimization"; break;
    case OVRDCAP_AMD64_CFG_DISPATCH_OPT:        name = L"CFG dispatch optimization"; break;
    case OVRDCAP_AMD64_XFG_DISPATCH_OPT:        name = L"XFG dispatch optimization"; break;
    case OVRDCAP_AMD64_KCFG_DISPATCH_KSCP:      name = L"kCFG dispatch (KSCP)"; break;

    // Supervisor-mode protections.
    case OVRDCAP_AMD64_SMEP:                    name = L"SMEP"; break;
    case OVRDCAP_AMD64_SMAP:                    name = L"SMAP"; break;

    case OVRDCAP_AMD64_NOT_LIVE_MIGRATEABLE:    name = L"not live-migrateable"; break;

    // User-mode access (UMA) special-function overrides.
    case OVRDCAP_AMD64_UMA_DISPATCH_KSCP:                   name = L"UMA dispatch (KSCP)"; break;
    case OVRDCAP_AMD64_UMA_COPY_FROM_USER_SO:               name = L"UMA copy-from-user"; break;
    case OVRDCAP_AMD64_UMA_COPY_TO_USER_SO:                 name = L"UMA copy-to-user"; break;
    case OVRDCAP_AMD64_UMA_COPY_TO_USER_FROM_USER_SO:       name = L"UMA copy-to-user-from-user"; break;
    case OVRDCAP_AMD64_UMA_MOVE_TO_USER_FROM_USER_SO:       name = L"UMA move-to-user-from-user"; break;
    case OVRDCAP_AMD64_UMA_SET_USER_MEMORY_SO:              name = L"UMA set-user-memory"; break;
    case OVRDCAP_AMD64_UMA_READ_UCHAR_FROM_USER_SO:         name = L"UMA read UCHAR"; break;
    case OVRDCAP_AMD64_UMA_WRITE_UCHAR_TO_USER_SO:          name = L"UMA write UCHAR"; break;
    case OVRDCAP_AMD64_UMA_READ_USHORT_FROM_USER_SO:        name = L"UMA read USHORT"; break;
    case OVRDCAP_AMD64_UMA_WRITE_USHORT_TO_USER_SO:         name = L"UMA write USHORT"; break;
    case OVRDCAP_AMD64_UMA_READ_ULONG_FROM_USER_SO:         name = L"UMA read ULONG"; break;
    case OVRDCAP_AMD64_UMA_WRITE_ULONG_TO_USER_SO:          name = L"UMA write ULONG"; break;
    case OVRDCAP_AMD64_UMA_READ_ULONG64_FROM_USER_SO:       name = L"UMA read ULONG64"; break;
    case OVRDCAP_AMD64_UMA_WRITE_ULONG64_TO_USER_SO:        name = L"UMA write ULONG64"; break;
    case OVRDCAP_AMD64_UMA_STRING_LENGTH_FROM_USER_SO:      name = L"UMA string length"; break;
    case OVRDCAP_AMD64_UMA_WSTRING_LENGTH_FROM_USER_SO:     name = L"UMA wide string length"; break;
    case OVRDCAP_AMD64_UMA_COPY_FROM_USER_NON_TEMPORAL_SO:  name = L"UMA copy-from-user (non-temporal)"; break;
    case OVRDCAP_AMD64_UMA_COPY_TO_USER_NON_TEMPORAL_SO:    name = L"UMA copy-to-user (non-temporal)"; break;
    case OVRDCAP_AMD64_UMA_CAS_64_TO_USER_SO:               name = L"UMA compare-exchange 64"; break;
    case OVRDCAP_AMD64_UMA_IOR_32_TO_USER_SO:               name = L"UMA interlocked-or 32"; break;
    case OVRDCAP_AMD64_UMA_IOR_64_TO_USER_SO:               name = L"UMA interlocked-or 64"; break;
    case OVRDCAP_AMD64_UMA_IAND_32_TO_USER_SO:              name = L"UMA interlocked-and 32"; break;
    case OVRDCAP_AMD64_UMA_IAND_64_TO_USER_SO:              name = L"UMA interlocked-and 64"; break;

    // Capability-set version markers: the OS is aware of this capability set.
    case OVRDCAP_AMD64_V1_CAPSET:               name = L"AMD64 capability set v1"; break;
    case OVRDCAP_AMD64_V2_CAPSET:               name = L"AMD64 capability set v2"; break;
    case OVRDCAP_AMD64_V3_CAPSET:               name = L"AMD64 capability set v3"; break;
    case OVRDCAP_AMD64_V4_CAPSET:               name = L"AMD64 capability set v4"; break;

    // ---- ARM64 -----------------------------------------------------------------------------

    case OVRDCAP_ARM64_USERMODE:                name = L"user mode"; break;
    case OVRDCAP_ARM64_KERNELMODE:              name = L"kernel mode"; break;

    // Cryptographic and arithmetic extensions.
    case OVRDCAP_ARM64_SHA256:                  name = L"SHA-256"; break;
    case OVRDCAP_ARM64_SHA512:                  name = L"SHA-512"; break;
    case OVRDCAP_ARM64_SHA3:                    name = L"SHA-3"; break;
    case OVRDCAP_ARM64_SM3:                     name = L"SM3"; break;
    case OVRDCAP_ARM64_SM4:                     name = L"SM4"; break;
    case OVRDCAP_ARM64_LSE:                     name = L"LSE"; break;
    case OVRDCAP_ARM64_LSE2:                    name = L"LSE2"; break;
    case OVRDCAP_ARM64_RDM:                     name = L"RDM"; break;
    case OVRDCAP_ARM64_DP:                      name = L"dot product"; break;
    case OVRDCAP_ARM64_FHM:                     name = L"FHM"; break;
    case OVRDCAP_ARM64_FLAGM:                   name = L"FlagM"; break;
    case OVRDCAP_ARM64_FLAGM2:                  name = L"FlagM2"; break;
    case OVRDCAP_ARM64_FCMA:                    name = L"FCMA"; break;
    case OVRDCAP_ARM64_LRCPC:                   name = L"LRCPC"; break;
    case OVRDCAP_ARM64_LRCPC2:                  name = L"LRCPC2"; break;
    case OVRDCAP_ARM64_BF16:                    name = L"BF16"; break;
    case OVRDCAP_ARM64_I8MM:                    name = L"I8MM"; break;
    case OVRDCAP_ARM64_FP16:                    name = L"FP16"; break;
    case OVRDCAP_ARM64_SVE:                     name = L"SVE"; break;
    case OVRDCAP_ARM64_SVE2:                    name = L"SVE2"; break;
    case OVRDCAP_ARM64_F32MM:                   name = L"F32MM"; break;
    case OVRDCAP_ARM64_F64MM:                   name = L"F64MM"; break;

    // Control-flow guard dispatch variants (values fixed; hard coded in the compiler).
    case OVRDCAP_ARM64_CFG_CHECK_OPT:           name = L"CFG check optimization"; break;
    case OVRDCAP_ARM64_CFG_DISPATCH_OPT:        name = L"CFG dispatch optimization"; break;
    case OVRDCAP_ARM64_EC_CFG_CHECK_OPT:        name = L"EC CFG check optimization"; break;
    case OVRDCAP_ARM64_EC_ICALL_CHECK_OPT:      name = L"EC indirect-call check optimization"; break;
    case OVRDCAP_ARM64_EC_CALL_CHECK_OPT:       name = L"EC call check optimization"; break;
    case OVRDCAP_ARM64_KCFG_CHECK_KSCP:         name = L"kCFG check (KSCP)"; break;

    // Alignment and cache behaviour.
    case OVRDCAP_ARM64_UNALIGNED_CRT_STRESS_TEST: name = L"unaligned CRT stress test"; break;
    case OVRDCAP_ARM64_UNALIGNED_CRT:           name = L"unaligned CRT"; break;
    case OVRDCAP_ARM64_DCZVA:                   name = L"DC ZVA"; break;
    case OVRDCAP_ARM64_DCZVA_STRIDE_64BYTES:    name = L"DC ZVA 64-byte stride"; break;

    case OVRDCAP_ARM64_PAN:                     name = L"PAN"; break;
    case OVRDCAP_ARM64_NO_DEVICE_MEMORY_ALLOCATION: name = L"no device-memory allocation"; break;
    case OVRDCAP_ARM64_NOT_LIVE_MIGRATEABLE:    name = L"not live-migrateable"; break;
    case OVRDCAP_ARM64_HYPERVISOR_VENDOR_MICROSOFT: name = L"Microsoft hypervisor"; break;

    // CPU implementers.
    case OVRDCAP_ARM64_CPU_IMPLEMENTER_ARM:         name = L"ARM"; break;
    case OVRDCAP_ARM64_CPU_IMPLEMENTER_BROADCOM:    name = L"Broadcom"; break;
    case OVRDCAP_ARM64_CPU_IMPLEMENTER_CAVIUM:      name = L"Cavium"; break;
    case OVRDCAP_ARM64_CPU_IMPLEMENTER_DEC:         name = L"DEC"; break;
    case OVRDCAP_ARM64_CPU_IMPLEMENTER_FUJITSU:     name = L"Fujitsu"; break;
    case OVRDCAP_ARM64_CPU_IMPLEMENTER_INFINEON:    name = L"Infineon"; break;
    case OVRDCAP_ARM64_CPU_IMPLEMENTER_MOTOROLA_OR_FREESCALE: name = L"Motorola/Freescale"; break;
    case OVRDCAP_ARM64_CPU_IMPLEMENTER_NVIDIA:      name = L"NVIDIA"; break;
    case OVRDCAP_ARM64_CPU_IMPLEMENTER_APPLIED_MICRO_CIRCUITS: name = L"Applied Micro Circuits"; break;
    case OVRDCAP_ARM64_CPU_IMPLEMENTER_QUALCOMM:    name = L"Qualcomm"; break;
    case OVRDCAP_ARM64_CPU_IMPLEMENTER_MARVELL:     name = L"Marvell"; break;
    case OVRDCAP_ARM64_CPU_IMPLEMENTER_INTEL:       name = L"Intel"; break;
    case OVRDCAP_ARM64_CPU_IMPLEMENTER_AMPERE:      name = L"Ampere"; break;
    case OVRDCAP_ARM64_CPU_IMPLEMENTER_MICROSOFT:   name = L"Microsoft"; break;
    case OVRDCAP_ARM64_CPU_IMPLEMENTER_APPLE:       name = L"Apple"; break;

    // Qualcomm chipsets.
    case OVRDCAP_ARM64_QC_CHIPSET_850:          name = L"Qualcomm 850"; break;
    case OVRDCAP_ARM64_QC_CHIPSET_8180:         name = L"Qualcomm 8180"; break;
    case OVRDCAP_ARM64_QC_CHIPSET_8280:         name = L"Qualcomm 8280"; break;
    case OVRDCAP_ARM64_QC_CHIPSET_8380:         name = L"Qualcomm 8380"; break;
#ifdef OVRDCAP_ARM64_QC_CHIPSET_8480
    case OVRDCAP_ARM64_QC_CHIPSET_8480:         name = L"Qualcomm 8480"; break;
#endif

    // User-mode access (UMA) special-function overrides.
    case OVRDCAP_ARM64_UMA_DISPATCH_KSCP:                   name = L"UMA dispatch (KSCP)"; break;
    case OVRDCAP_ARM64_UMA_COPY_FROM_USER_SO:               name = L"UMA copy-from-user"; break;
    case OVRDCAP_ARM64_UMA_COPY_TO_USER_SO:                 name = L"UMA copy-to-user"; break;
    case OVRDCAP_ARM64_UMA_COPY_TO_USER_FROM_USER_SO:       name = L"UMA copy-to-user-from-user"; break;
    case OVRDCAP_ARM64_UMA_MOVE_TO_USER_FROM_USER_SO:       name = L"UMA move-to-user-from-user"; break;
    case OVRDCAP_ARM64_UMA_SET_USER_MEMORY_SO:              name = L"UMA set-user-memory"; break;
    case OVRDCAP_ARM64_UMA_READ_UCHAR_FROM_USER_SO:         name = L"UMA read UCHAR"; break;
    case OVRDCAP_ARM64_UMA_WRITE_UCHAR_TO_USER_SO:          name = L"UMA write UCHAR"; break;
    case OVRDCAP_ARM64_UMA_READ_USHORT_FROM_USER_SO:        name = L"UMA read USHORT"; break;
    case OVRDCAP_ARM64_UMA_WRITE_USHORT_TO_USER_SO:         name = L"UMA write USHORT"; break;
    case OVRDCAP_ARM64_UMA_READ_ULONG_FROM_USER_SO:         name = L"UMA read ULONG"; break;
    case OVRDCAP_ARM64_UMA_WRITE_ULONG_TO_USER_SO:          name = L"UMA write ULONG"; break;
    case OVRDCAP_ARM64_UMA_READ_ULONG64_FROM_USER_SO:       name = L"UMA read ULONG64"; break;
    case OVRDCAP_ARM64_UMA_WRITE_ULONG64_TO_USER_SO:        name = L"UMA write ULONG64"; break;
    case OVRDCAP_ARM64_UMA_STRING_LENGTH_FROM_USER_SO:      name = L"UMA string length"; break;
    case OVRDCAP_ARM64_UMA_WSTRING_LENGTH_FROM_USER_SO:     name = L"UMA wide string length"; break;
    case OVRDCAP_ARM64_UMA_READ_UCHAR_FROM_USER_ACQ_SO:     name = L"UMA read UCHAR (acquire)"; break;
    case OVRDCAP_ARM64_UMA_WRITE_UCHAR_TO_USER_REL_SO:      name = L"UMA write UCHAR (release)"; break;
    case OVRDCAP_ARM64_UMA_READ_USHORT_FROM_USER_ACQ_SO:    name = L"UMA read USHORT (acquire)"; break;
    case OVRDCAP_ARM64_UMA_WRITE_USHORT_TO_USER_REL_SO:     name = L"UMA write USHORT (release)"; break;
    case OVRDCAP_ARM64_UMA_READ_ULONG_FROM_USER_ACQ_SO:     name = L"UMA read ULONG (acquire)"; break;
    case OVRDCAP_ARM64_UMA_WRITE_ULONG_TO_USER_REL_SO:      name = L"UMA write ULONG (release)"; break;
    case OVRDCAP_ARM64_UMA_READ_ULONG64_FROM_USER_ACQ_SO:   name = L"UMA read ULONG64 (acquire)"; break;
    case OVRDCAP_ARM64_UMA_WRITE_ULONG64_TO_USER_REL_SO:    name = L"UMA write ULONG64 (release)"; break;
    case OVRDCAP_ARM64_UMA_COPY_FROM_USER_NON_TEMPORAL_SO:  name = L"UMA copy-from-user (non-temporal)"; break;
    case OVRDCAP_ARM64_UMA_COPY_TO_USER_NON_TEMPORAL_SO:    name = L"UMA copy-to-user (non-temporal)"; break;
    case OVRDCAP_ARM64_UMA_CAS_64_TO_USER_SO:               name = L"UMA compare-exchange 64"; break;
    case OVRDCAP_ARM64_UMA_IOR_32_TO_USER_SO:               name = L"UMA interlocked-or 32"; break;
    case OVRDCAP_ARM64_UMA_IOR_64_TO_USER_SO:               name = L"UMA interlocked-or 64"; break;
    case OVRDCAP_ARM64_UMA_IAND_32_TO_USER_SO:              name = L"UMA interlocked-and 32"; break;
    case OVRDCAP_ARM64_UMA_IAND_64_TO_USER_SO:              name = L"UMA interlocked-and 64"; break;

    // Capability-set version markers: the OS is aware of this capability set.
    case OVRDCAP_ARM64_V1_CAPSET:               name = L"ARM64 capability set v1"; break;
    case OVRDCAP_ARM64_V2_CAPSET:               name = L"ARM64 capability set v2"; break;
    case OVRDCAP_ARM64_V3_CAPSET:               name = L"ARM64 capability set v3"; break;
    case OVRDCAP_ARM64_V4_CAPSET:               name = L"ARM64 capability set v4"; break;
    }

    if (name)
        return PhCreateString(name);

    // A number inside a namespace that this switch does not name: either a capability added after the
    // SDK header this build was compiled against, or one deliberately left to the caller's signature
    // fusion. Show the raw value in hex so it can be matched against overridecapabilities.h.
    return PhFormatString(L"feature 0x%lx", Feature);
}

// CPU-signature field ranges. Each is a base capability plus the field value, so a run of
// consecutive numbers encodes one field; the caller fuses them into a single readable term.
#define PV_DYNRELOC_IS_MODEL(f)       ((f) >= OVRDCAP_AMD64_CPU_MODEL_0 && \
                                       (f) <= OVRDCAP_AMD64_CPU_MODEL_15)
#define PV_DYNRELOC_IS_EXTMODEL(f)    ((f) >= OVRDCAP_AMD64_CPU_EXTENDED_MODEL_0 && \
                                       (f) <= OVRDCAP_AMD64_CPU_EXTENDED_MODEL_15)
#define PV_DYNRELOC_IS_FAMILY(f)      ((f) >= OVRDCAP_AMD64_CPU_FAMILY_0 && \
                                       (f) <= OVRDCAP_AMD64_CPU_FAMILY_15)
#define PV_DYNRELOC_IS_EXTFAMILY(f)   ((f) >= OVRDCAP_AMD64_CPU_EXTENDED_FAMILY_0 && \
                                       (f) <= OVRDCAP_AMD64_CPU_EXTENDED_FAMILY_255)

#define PV_DYNRELOC_MAX_BDD_DEPTH 128

PPH_STRING PvDynRelocOutcomeString(
    _In_ PPH_FUNCTION_OVERRIDE_OUTCOME Outcome
    )
{
    switch (Outcome->Type)
    {
    case PhFunctionOverrideKeepOriginal:
        return PhCreateString(L"keep original");
    case PhFunctionOverrideInvalid:
        return PhCreateString(L"(malformed terminal)");
    default:
        return PhFormatString(L"override[%lu]", Outcome->RvaIndex);
    }
}

// Builds the AND-chain condition string starting at *Index, following TRUE edges through fresh
// internal nodes that share the head's FALSE target. On return *Index is the "then" target taken
// when every feature is present, CommonFalse is the shared else target, and AlwaysAbsent is set if
// any term is an always-absent feature (so the whole AND can never hold). Each consumed node is
// marked Expanded.
PPH_STRING PvDynRelocBuildCondition(
    _In_ PPH_LIST Nodes,
    _Inout_ PULONG Index,
    _Out_ PULONG CommonFalse,
    _Out_ PBOOLEAN AlwaysAbsent,
    _Inout_ PBOOLEAN Expanded
    )
{
    PPH_FUNCTION_OVERRIDE_BDD_NODE node = Nodes->Items[*Index];
    PH_STRING_BUILDER sb;
    ULONG commonFalse = node->Internal.FalseEdge;
    ULONG thenIndex = *Index;
    BOOLEAN alwaysAbsent = FALSE;
    BOOLEAN needAnd = FALSE;

    // CPU-signature pieces are accumulated and fused into "family F, model 0xMM" (matching the
    // DisplayFamily/DisplayModel formula in RtlGetProcessorSignature) rather than shown as three
    // separate raw fields. Presence flags track which pieces the AND-chain actually constrained.
    BOOLEAN haveFamily = FALSE, haveExtFamily = FALSE, haveModel = FALSE, haveExtModel = FALSE;
    ULONG family = 0, extFamily = 0, model = 0, extModel = 0;

    PhInitializeStringBuilder(&sb, 64);

    // Walk the AND-chain: nodes linked by TRUE edges that share the common FALSE target.
    for (;;)
    {
        PPH_FUNCTION_OVERRIDE_BDD_NODE link;
        ULONG feature;

        if (thenIndex >= Nodes->Count)
            break;

        link = Nodes->Items[thenIndex];

        // Stop at a terminal, an already-expanded node, or a node with a different else target.
        if (thenIndex != *Index &&
            (link->IsTerminal || Expanded[thenIndex] || link->Internal.FalseEdge != commonFalse))
            break;

        feature = link->Internal.FeatureNumber;

        if (PhFunctionOverrideIsFeatureAlwaysAbsent(feature))
            alwaysAbsent = TRUE;

        if (PV_DYNRELOC_IS_FAMILY(feature))      { haveFamily = TRUE; family = feature - OVRDCAP_AMD64_CPU_FAMILY_0; }
        else if (PV_DYNRELOC_IS_EXTFAMILY(feature)) { haveExtFamily = TRUE; extFamily = feature - OVRDCAP_AMD64_CPU_EXTENDED_FAMILY_0; }
        else if (PV_DYNRELOC_IS_MODEL(feature))     { haveModel = TRUE; model = feature - OVRDCAP_AMD64_CPU_MODEL_0; }
        else if (PV_DYNRELOC_IS_EXTMODEL(feature))  { haveExtModel = TRUE; extModel = feature - OVRDCAP_AMD64_CPU_EXTENDED_MODEL_0; }
        else
        {
            // A non-signature feature: render inline in encounter order (space-separated tags).
            if (needAnd) PhAppendStringBuilder2(&sb, L" ");
            PhAppendStringBuilder2(&sb, PH_AUTO_T(PH_STRING, PvDynRelocFeatureString(feature))->Buffer);
            needAnd = TRUE;
        }

        Expanded[thenIndex] = TRUE;
        thenIndex = link->Internal.TrueEdge;
    }

    // Emit the fused CPU-signature term, if any signature field was constrained.
    //
    // Fusion must be injective: the guards form a first-match-wins ladder, so two rungs that render
    // identically read as a contradiction. Rungs can differ purely by which fields they constrain --
    // "base family F AND extended family E" versus "base family F" alone (observed as consecutive
    // rungs in ntoskrnl 26100's KeCopyPage at F=15) -- so a field that was constrained is always
    // named, even when DisplayFamily/DisplayModel folding would otherwise absorb it into a single
    // number. Presenting the fold is useful; presenting only the fold loses the rung's identity.
    if (haveFamily || haveExtFamily || haveModel || haveExtModel)
    {
        if (needAnd) PhAppendStringBuilder2(&sb, L" ");

        if (haveFamily)
        {
            if (haveExtFamily)
            {
                // A constrained extended family must always be visible. DisplayFamily folds it only
                // for base family 15, so show the fused value there and the raw pair otherwise --
                // either way the rendering names both fields, so no two rungs that differ in
                // extended family can collapse to the same string.
                if (family == 15)
                    PhAppendFormatStringBuilder(&sb, L"family %lu (15+%lu)", family + extFamily, extFamily);
                else
                    PhAppendFormatStringBuilder(&sb, L"family %lu ext family %lu", family, extFamily);
            }
            else
            {
                PhAppendFormatStringBuilder(&sb, L"family %lu", family);
            }
        }
        else if (haveExtFamily)
        {
            PhAppendFormatStringBuilder(&sb, L"ext family %lu", extFamily);
        }

        if (haveModel || haveExtModel)
        {
            if (haveFamily || haveExtFamily)
                PhAppendStringBuilder2(&sb, L" ");

            if (haveModel && haveExtModel && (family == 6 || family == 15))
            {
                // DisplayModel folds extended model for base family 6 and 15
                // (RtlGetProcessorSignature). As with family above, name the extended field too so
                // this rung cannot collapse onto one that constrained only the base model.
                PhAppendFormatStringBuilder(
                    &sb,
                    L"model 0x%02lX (%lu:%lu)",
                    (extModel << 4) | model,
                    extModel,
                    model
                    );
            }
            else if (haveModel && haveExtModel)
            {
                // Base family does not fold the extended model: both fields stand alone.
                PhAppendFormatStringBuilder(&sb, L"model %lu ext model %lu", model, extModel);
            }
            else if (haveModel)
            {
                PhAppendFormatStringBuilder(&sb, L"model %lu", model);
            }
            else
            {
                PhAppendFormatStringBuilder(&sb, L"ext model %lu", extModel);
            }
        }

        needAnd = TRUE;
    }

    if (!needAnd)
        PhAppendStringBuilder2(&sb, L"(unconstrained)");

    *Index = thenIndex;
    *CommonFalse = commonFalse;
    *AlwaysAbsent = alwaysAbsent;
    return PhFinalStringBuilderString(&sb);
}

// Renders the BDD rooted at Index as a flat, priority-ordered outcome list under ParentNode.
//
// Real function-override BDDs are feature-priority ladders: each else-spine rung is an AND of
// features guarding a single outcome, with a final unconditional default. Each rung is emitted as
// ONE row -- RVA + resolved symbol for the target, the override index / "keep original" tag in the
// Type column, and the guard condition ("if ...", "else if ...", "else") in Info -- so the whole
// decision reads as a flat list rather than a nested tree.
//
// The BDD format does permit a rung whose then-branch is itself a further decision; that case
// cannot be one flat row, so it falls back to a nested condition row with the sub-decision beneath
// it. A BDD is a DAG, so each internal node is expanded at most once (Expanded); a node reached
// again emits a compact "(shared node N)" reference.
//
// Root call passes Index = 0 and Depth = 0. A terminal root (unconditional override) is a single
// outcome row. Count is incremented once per emitted outcome row (leaf), not for condition or
// shared-ref rows.
VOID PvDynRelocAddBddOutcomes(
    _Inout_ PPV_PE_DYNRELOC_CONTEXT Context,
    _In_ PPV_DYNRELOC_NODE ParentNode,
    _In_ PPH_LIST Nodes,
    _In_ ULONG Index,
    _Inout_ PBOOLEAN Expanded,
    _Inout_ PULONG Count,
    _In_ ULONG Depth
    )
{
    BOOLEAN first = TRUE; // first rung is the primary; the trailing terminal is the (default)

    if (Depth >= PV_DYNRELOC_MAX_BDD_DEPTH)
    {
        PvAddDynRelocNode(Context, ParentNode, NULL, NULL, PhCreateString(L"(nesting too deep)"));
        return;
    }

    for (;;)
    {
        PPH_FUNCTION_OVERRIDE_BDD_NODE node;
        PPV_DYNRELOC_NODE row;
        PPH_STRING condition;
        PPH_STRING guard;
        ULONG commonFalse;
        ULONG thenIndex;
        BOOLEAN alwaysAbsent;

        if (Index >= Nodes->Count)
            return;

        node = Nodes->Items[Index];

        // Else-spine tail: the fall-through outcome taken when no condition above matched.
        if (node->IsTerminal)
        {
            row = PvAddDynRelocNode(
                Context,
                ParentNode,
                PH_AUTO_T(PH_STRING, PhFormatString(L"0x%lx", node->Terminal.Rva))->Buffer,
                PH_AUTO_T(PH_STRING, PvDynRelocOutcomeString(&node->Terminal))->Buffer,
                first ? NULL : PhCreateString(L"(default)")
                );
            PvDynRelocSetRvaColumns(row, node->Terminal.Rva);
            (*Count)++;
            return;
        }

        if (Expanded[Index])
        {
            PvAddDynRelocNode(Context, ParentNode, NULL, NULL,
                PhFormatString(L"(shared node %lu)", Index));
            return;
        }

        // Collapse the AND-chain for this rung; thenIndex becomes the then-target. The list is a
        // priority ladder (first match wins), so conditions stand alone without if/else-if noise.
        thenIndex = Index;
        condition = PvDynRelocBuildCondition(Nodes, &thenIndex, &commonFalse, &alwaysAbsent, Expanded);

        if (alwaysAbsent)
        {
            guard = PhFormatString(L"%ls (always absent)", condition->Buffer);
            PhDereferenceObject(condition);
        }
        else
        {
            guard = condition; // transfer ownership to the row
        }

        if (thenIndex < Nodes->Count && !alwaysAbsent && ((PPH_FUNCTION_OVERRIDE_BDD_NODE)Nodes->Items[thenIndex])->IsTerminal)
        {
            // Flat case: the guarded body is a single outcome -> one combined row.
            PPH_FUNCTION_OVERRIDE_BDD_NODE thenNode = Nodes->Items[thenIndex];

            row = PvAddDynRelocNode(
                Context,
                ParentNode,
                PH_AUTO_T(PH_STRING, PhFormatString(L"0x%lx", thenNode->Terminal.Rva))->Buffer,
                PH_AUTO_T(PH_STRING, PvDynRelocOutcomeString(&thenNode->Terminal))->Buffer,
                guard
                );
            PvDynRelocSetRvaColumns(row, thenNode->Terminal.Rva);
            (*Count)++;
        }
        else
        {
            // Fallback: an always-absent guard (then-branch unreachable) or a genuine sub-decision.
            // Emit the condition row; nest the sub-decision beneath it when one is reachable.
            row = PvAddDynRelocNode(
                Context,
                ParentNode,
                NULL,
                NULL,
                guard
                );

            if (!alwaysAbsent)
                PvDynRelocAddBddOutcomes(Context, row, Nodes, thenIndex, Expanded, Count, Depth + 1);
        }

        // Continue down the else-spine as sibling rungs.
        Index = commonFalse;
        first = FALSE;
    }
}

// Populates ParentNode with the flattened outcome list and returns the number of outcome rows.
ULONG PvDynRelocAddOverrideDecisionTree(
    _Inout_ PPV_PE_DYNRELOC_CONTEXT Context,
    _In_ PPV_DYNRELOC_NODE ParentNode,
    _In_ PPH_IMAGE_DYNAMIC_RELOC_ENTRY Representative
    )
{
    PV_DYNRELOC_BDD_COLLECT collect;
    PBOOLEAN expanded;
    ULONG count = 0;

    collect.Nodes = PhCreateList(8);

    if (!NT_SUCCESS(PhFunctionOverrideEnumerateBddNodes(Representative, PvDynRelocBddCollectCallback, &collect)) ||
        collect.Nodes->Count == 0)
    {
        PvAddDynRelocNode(Context, ParentNode, L"(no decision diagram)", NULL, NULL);

        for (ULONG i = 0; i < collect.Nodes->Count; i++)
            PhFree(collect.Nodes->Items[i]);
        PhDereferenceObject(collect.Nodes);
        return 0;
    }

    expanded = PhAllocateZero(collect.Nodes->Count * sizeof(BOOLEAN));

    PvDynRelocAddBddOutcomes(Context, ParentNode, collect.Nodes, 0, expanded, &count, 0);

    PhFree(expanded);

    for (ULONG i = 0; i < collect.Nodes->Count; i++)
        PhFree(collect.Nodes->Items[i]);
    PhDereferenceObject(collect.Nodes);

    return count;
}

VOID PvEnumerateDynamicRelocationEntries(
    _In_ PPV_PE_DYNRELOC_CONTEXT Context
    )
{
    PH_MAPPED_IMAGE_DYNAMIC_RELOC relocations;
    PPH_LIST groups;

    groups = PhCreateList(4);

    if (NT_SUCCESS(PhGetMappedImageDynamicRelocations(&PvMappedImage, &relocations)))
    {
        // Pass 1: create leaf roots for ordinary DVRT entries in order, and bucket
        // function-override patch sites by (OriginalRva, BDDOffset) so their shared decision
        // tree is rendered once.
        for (ULONG i = 0; i < relocations.NumberOfEntries; i++)
        {
            PPH_IMAGE_DYNAMIC_RELOC_ENTRY entry = &relocations.RelocationEntries[i];

            if (Context->Cancel)
                break;

            if (entry->Symbol == IMAGE_DYNAMIC_RELOCATION_FUNCTION_OVERRIDE)
            {
                PPV_DYNRELOC_OVERRIDE_GROUP group = NULL;

                for (ULONG j = 0; j < groups->Count; j++)
                {
                    PPV_DYNRELOC_OVERRIDE_GROUP existing = groups->Items[j];

                    if (existing->OriginalRva == entry->FuncOverride.OriginalRva &&
                        existing->BDDOffset == entry->FuncOverride.BDDOffset)
                    {
                        group = existing;
                        break;
                    }
                }

                if (!group)
                {
                    group = PhAllocateZero(sizeof(PV_DYNRELOC_OVERRIDE_GROUP));
                    group->OriginalRva = entry->FuncOverride.OriginalRva;
                    group->BDDOffset = entry->FuncOverride.BDDOffset;
                    group->Representative = entry;
                    group->Sites = PhCreateList(4);
                    PhAddItemList(groups, group);

                    group->RootNode = PvAddDynRelocNode(
                        Context,
                        NULL,
                        PH_AUTO_T(PH_STRING, PhFormatString(L"0x%lx", entry->FuncOverride.OriginalRva))->Buffer,
                        L"FUNCTION",
                        PhFormatString(L"original 0x%lx", entry->FuncOverride.OriginalRva)
                        );

                    PvDynRelocSetRvaColumns(group->RootNode, entry->FuncOverride.OriginalRva);
                }

                PhAddItemList(group->Sites, entry);
            }
            else
            {
                PPV_DYNRELOC_NODE node;
                PWSTR typeName;
                PPH_STRING info;
                ULONG rva;

                rva = PvDynRelocEntryRva(entry);
                info = PvDynRelocEntryInfoString(entry, &typeName);

                node = PvAddDynRelocNode(
                    Context,
                    NULL,
                    PH_AUTO_T(PH_STRING, PhFormatString(L"0x%lx", rva))->Buffer,
                    typeName,
                    info
                    );

                PvDynRelocSetEntryColumns(node, entry);
            }
        }

        // Pass 2: populate each override group with its patch sites and single decision tree.
        for (ULONG j = 0; j < groups->Count; j++)
        {
            PPV_DYNRELOC_OVERRIDE_GROUP group = groups->Items[j];
            PPV_DYNRELOC_NODE sitesNode;
            PPV_DYNRELOC_NODE treeNode;

            if (Context->Cancel)
                break;

            sitesNode = PvAddDynRelocNode(
                Context,
                group->RootNode,
                PH_AUTO_T(PH_STRING, PhFormatString(L"Patch sites (%lu)", group->Sites->Count))->Buffer,
                NULL,
                NULL
                );

            // Which outcome a site ends up with is a property of the shared decision diagram, not
            // of the site, and it cannot be determined from the file: the deciding capabilities are
            // set at boot from the CPU. The Outcomes list below enumerates every possibility with
            // its guard, so no per-site resolution is claimed here -- Info carries the patch width,
            // which is file-derived and not otherwise visible.
            for (ULONG k = 0; k < group->Sites->Count; k++)
            {
                PPH_IMAGE_DYNAMIC_RELOC_ENTRY site = group->Sites->Items[k];
                PPV_DYNRELOC_NODE siteNode;
                ULONG patchWidth;

                // Each site resolves a symbol under the global symbol lock, so a large override
                // (ntoskrnl has one with 4707 sites) is where a close-during-load lands.
                if (Context->Cancel)
                    break;

                patchWidth = PvDynRelocOverridePatchWidth(site->FuncOverride.Record.Type);

                siteNode = PvAddDynRelocNode(
                    Context,
                    sitesNode,
                    PH_AUTO_T(PH_STRING, PhFormatString(L"0x%lx", site->FuncOverride.BlockRva + site->FuncOverride.Record.Offset))->Buffer,
                    PvDynRelocOverrideTypeName(site->FuncOverride.Record.Type),
                    patchWidth ? PhFormatString(L"%lu bytes", patchWidth) : NULL
                    );

                PvDynRelocSetEntryColumns(siteNode, site);
            }

            treeNode = PvAddDynRelocNode(
                Context,
                group->RootNode,
                NULL,
                NULL,
                NULL
                );

            {
                ULONG outcomeCount = PvDynRelocAddOverrideDecisionTree(Context, treeNode, group->Representative);

                // Label the group node with the outcome count, mirroring "Patch sites (N)".
                treeNode->Rva = PhFormatString(L"Outcomes (%lu)", outcomeCount);
            }
        }

        PhFreeMappedImageDynamicRelocations(&relocations);
    }

    for (ULONG j = 0; j < groups->Count; j++)
    {
        PPV_DYNRELOC_OVERRIDE_GROUP group = groups->Items[j];
        PhDereferenceObject(group->Sites);
        PhFree(group);
    }
    PhDereferenceObject(groups);
}

// Builds the tree off the UI thread (symbol resolution can be slow) and notifies the dialog to
// structure the tree once the nodes are ready.
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PvpPeDynRelocEnumerateThread(
    _In_ PPV_PE_DYNRELOC_CONTEXT Context
    )
{
    PH_AUTO_POOL autoPool;

    // The enumeration uses PH_AUTO_T temporaries; auto-dereference pools are per-thread, so this
    // worker needs its own (the UI thread's is set up by the message loop).
    PhInitializeAutoPool(&autoPool);

    PvEnumerateDynamicRelocationEntries(Context);

    PostMessage(Context->DialogHandle, WM_PV_SEARCH_FINISHED, 0, 0);

    PhDeleteAutoPool(&autoPool);
    return STATUS_SUCCESS;
}

VOID PvInitializeDynRelocTree(
    _In_ PPV_PE_DYNRELOC_CONTEXT Context
    )
{
    PPH_STRING settings;

    Context->NodeList = PhCreateList(64);
    Context->NodeRootList = PhCreateList(16);

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");
    TreeNew_SetCallback(Context->TreeNewHandle, PvDynRelocTreeNewCallback, Context);

    PhAddTreeNewColumn(Context->TreeNewHandle, PV_DYNRELOC_TREE_COLUMN_RVA, TRUE, L"RVA", 140, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PV_DYNRELOC_TREE_COLUMN_TYPE, TRUE, L"Type", 110, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PV_DYNRELOC_TREE_COLUMN_INFO, TRUE, L"Info", 260, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PV_DYNRELOC_TREE_COLUMN_SECTION, TRUE, L"Section", 80, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PV_DYNRELOC_TREE_COLUMN_SYMBOL, TRUE, L"Symbol", 260, PH_ALIGN_LEFT, 4, 0);

    settings = PhGetStringSetting(L"ImageDynamicRelocationsTreeColumns");
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, Context->TreeNewHandle, Context->NodeList);
}

VOID PvDeleteDynRelocTree(
    _In_ PPV_PE_DYNRELOC_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(L"ImageDynamicRelocationsTreeColumns", &settings->sr);
    PhDereferenceObject(settings);

    PhDeleteTreeNewFilterSupport(&Context->FilterSupport);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PvDestroyDynRelocNode(Context->NodeList->Items[i]);

    PhDereferenceObject(Context->NodeList);
    PhDereferenceObject(Context->NodeRootList);
}

#define SORT_FUNCTION(Column) PvDynRelocTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PvDynRelocTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPV_DYNRELOC_NODE node1 = *(PPV_DYNRELOC_NODE *)_elem1; \
    PPV_DYNRELOC_NODE node2 = *(PPV_DYNRELOC_NODE *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uint64cmp(node1->UniqueId, node2->UniqueId); \
    return PhModifySort(sortResult, ((PPV_PE_DYNRELOC_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Rva)
{
    // Rows carrying an actual address sort numerically; structural rows fall back to insertion
    // order (via the UniqueId tiebreak) so the tree layout stays coherent.
    if (node1->HasRvaValue && node2->HasRvaValue)
        sortResult = uint64cmp(node1->RvaValue, node2->RvaValue);
    else if (node1->HasRvaValue != node2->HasRvaValue)
        sortResult = node1->HasRvaValue ? 1 : -1;
    else
        sortResult = PhCompareStringWithNull(node1->Rva, node2->Rva, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Type)
{
    sortResult = PhCompareStringWithNull(node1->Type, node2->Type, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Info)
{
    sortResult = PhCompareStringWithNull(node1->Info, node2->Info, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Section)
{
    sortResult = PhCompareStringWithNull(node1->Section, node2->Section, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Symbol)
{
    sortResult = PhCompareStringWithNull(node1->Symbol, node2->Symbol, TRUE);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PvDynRelocTreeNewCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPV_PE_DYNRELOC_CONTEXT context = Context;
    PPV_DYNRELOC_NODE node;

    if (!context)
        return FALSE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPV_DYNRELOC_NODE)getChildren->Node;

            if (!node)
            {
                if (context->Loading)
                {
                    getChildren->Children = NULL;
                    getChildren->NumberOfChildren = 0;
                    return TRUE;
                }

                // Sort only the root list; child rows (patch sites, BDD edges) keep their
                // structural insertion order regardless of the active column sort.
                static CONST _CoreCrtSecureSearchSortCompareFunction sortFunctions[] =
                {
                    SORT_FUNCTION(Rva),
                    SORT_FUNCTION(Type),
                    SORT_FUNCTION(Info),
                    SORT_FUNCTION(Section),
                    SORT_FUNCTION(Symbol),
                };
                _CoreCrtSecureSearchSortCompareFunction sortFunction;

                static_assert(RTL_NUMBER_OF(sortFunctions) == PV_DYNRELOC_TREE_COLUMN_MAXIMUM, "SortFunctions must equal maximum.");

                if (context->TreeNewSortColumn < PV_DYNRELOC_TREE_COLUMN_MAXIMUM)
                    sortFunction = sortFunctions[context->TreeNewSortColumn];
                else
                    sortFunction = NULL;

                if (sortFunction && context->TreeNewSortOrder != NoSortOrder)
                {
                    qsort_s(context->NodeRootList->Items, context->NodeRootList->Count, sizeof(PVOID), sortFunction, context);
                }

                getChildren->Children = (PPH_TREENEW_NODE*)context->NodeRootList->Items;
                getChildren->NumberOfChildren = context->NodeRootList->Count;
            }
            else
            {
                getChildren->Children = (PPH_TREENEW_NODE*)node->Children->Items;
                getChildren->NumberOfChildren = node->Children->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;
            node = (PPV_DYNRELOC_NODE)isLeaf->Node;

            isLeaf->IsLeaf = !(node->Children && node->Children->Count);
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            node = (PPV_DYNRELOC_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PV_DYNRELOC_TREE_COLUMN_RVA:
                getCellText->Text = PhGetStringRef(node->Rva);
                break;
            case PV_DYNRELOC_TREE_COLUMN_TYPE:
                getCellText->Text = PhGetStringRef(node->Type);
                break;
            case PV_DYNRELOC_TREE_COLUMN_INFO:
                getCellText->Text = PhGetStringRef(node->Info);
                break;
            case PV_DYNRELOC_TREE_COLUMN_SECTION:
                getCellText->Text = PhGetStringRef(node->Section);
                break;
            case PV_DYNRELOC_TREE_COLUMN_SYMBOL:
                getCellText->Text = PhGetStringRef(node->Symbol);
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;
            getNodeColor->Flags = TN_AUTO_FORECOLOR | TN_CACHE;
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            SendMessage(context->WindowHandle, WM_COMMAND, WM_PV_DYNRELOC_CONTEXTMENU, (LPARAM)contextMenuEvent);
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(WindowHandle, &context->TreeNewSortColumn, &context->TreeNewSortOrder);
            TreeNew_NodesStructured(WindowHandle);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                {
                    // Ctrl+C copies every selected row (multi-select via click-drag / ctrl / shift).
                    if (GetKeyState(VK_CONTROL) < 0)
                    {
                        PPH_STRING text;

                        text = PhGetTreeNewText(WindowHandle, 0);
                        PhSetClipboardString(WindowHandle, &text->sr);
                        PhDereferenceObject(text);
                    }
                }
                break;
            case 'A':
                {
                    if (GetKeyState(VK_CONTROL) < 0)
                        TreeNew_SelectRange(WindowHandle, 0, -1);
                }
                break;
            }
        }
        return TRUE;
    }

    return FALSE;
}

BOOLEAN PvDynRelocTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_ PVOID Context
    )
{
    PPV_PE_DYNRELOC_CONTEXT context = Context;
    PPV_DYNRELOC_NODE node = (PPV_DYNRELOC_NODE)Node;

    if (!context->SearchMatchHandle)
        return TRUE;

    if (!PhIsNullOrEmptyString(node->Rva) && PvSearchControlMatch(context->SearchMatchHandle, &node->Rva->sr))
        return TRUE;
    if (!PhIsNullOrEmptyString(node->Type) && PvSearchControlMatch(context->SearchMatchHandle, &node->Type->sr))
        return TRUE;
    if (!PhIsNullOrEmptyString(node->Info) && PvSearchControlMatch(context->SearchMatchHandle, &node->Info->sr))
        return TRUE;
    if (!PhIsNullOrEmptyString(node->Section) && PvSearchControlMatch(context->SearchMatchHandle, &node->Section->sr))
        return TRUE;
    if (!PhIsNullOrEmptyString(node->Symbol) && PvSearchControlMatch(context->SearchMatchHandle, &node->Symbol->sr))
        return TRUE;

    return FALSE;
}

// Makes every filter-matched row reachable: a match can sit several levels down (override -> Patch
// sites -> site, or override -> Outcomes -> outcome), and PhApplyTreeNewFilters marks Visible per node
// without consulting ancestors. Force each match's ancestor chain visible and expanded so the match is
// actually on screen, mirroring the reveal-upwards behaviour of the certificates tree.
VOID PvRevealDynRelocMatches(
    _In_ PPV_PE_DYNRELOC_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_DYNRELOC_NODE node = Context->NodeList->Items[i];
        PPV_DYNRELOC_NODE parent;

        if (!node->Node.Visible)
            continue;

        for (parent = node->Parent; parent; parent = parent->Parent)
        {
            parent->Node.Visible = TRUE;
            parent->Node.Expanded = TRUE;
        }
    }
}

// Restores the compact default view once the search box is cleared.
VOID PvCollapseDynRelocNodes(
    _In_ PPV_PE_DYNRELOC_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_DYNRELOC_NODE node = Context->NodeList->Items[i];

        if (node->Children->Count != 0)
            node->Node.Expanded = FALSE;
    }
}

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI PvpPeDynRelocSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPV_PE_DYNRELOC_CONTEXT context = Context;

    assert(context);

    context->SearchMatchHandle = MatchHandle;

    // PhApplyTreeNewFilters walks the whole NodeList; skip it while the worker is still appending.
    // WM_PV_SEARCH_FINISHED re-applies the filter once the list is stable.
    if (context->Loading)
        return;

    PhApplyTreeNewFilters(&context->FilterSupport);

    // PhApplyTreeNewFilters sets Visible per node with no regard for ancestors, and this tree opens
    // collapsed -- so a match on a nested row (an override's patch site or outcome) would be filtered
    // in yet stay off-screen behind a collapsed or hidden parent. Reveal matches by expanding and
    // showing their ancestor chain; on an empty query, restore the compact collapsed view.
    if (context->SearchMatchHandle)
        PvRevealDynRelocMatches(context);
    else
        PvCollapseDynRelocNodes(context);

    TreeNew_NodesStructured(context->TreeNewHandle);
}

INT_PTR CALLBACK PvpPeDynamicRelocationDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_PE_DYNRELOC_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_PE_DYNRELOC_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;
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
            context->DialogHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_TREELIST);
            context->SearchHandle = GetDlgItem(hwndDlg, IDC_TREESEARCH);

            PvCreateSearchControl(
                hwndDlg,
                context->SearchHandle,
                L"Search Relocations (Ctrl+K)",
                PvpPeDynRelocSearchControlCallback,
                context
                );

            PvConfigTreeBorders(context->TreeNewHandle);

            PvInitializeDynRelocTree(context);
            PhAddTreeNewFilter(&context->FilterSupport, PvDynRelocTreeFilterCallback, context);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);

            TreeNew_SetEmptyText(context->TreeNewHandle, &LoadingDynRelocText, 0);

            // Loading must be set before the worker starts: it is what keeps UI-thread readers off
            // NodeList/NodeRootList while the worker appends to them.
            context->Loading = TRUE;

            if (!NT_SUCCESS(PhCreateThreadEx(&context->ThreadHandle, PvpPeDynRelocEnumerateThread, context)))
            {
                context->ThreadHandle = NULL;
                context->Loading = FALSE;
                TreeNew_SetEmptyText(context->TreeNewHandle, &EmptyDynRelocText, 0);
            }

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            // The worker writes into NodeList/NodeRootList and into the nodes themselves, so it must
            // be finished before PvDeleteDynRelocTree frees any of that. Ask it to stop, then join.
            if (context->ThreadHandle)
            {
                context->Cancel = TRUE;
                PhWaitForSingleObject(context->ThreadHandle, INFINITE);
                NtClose(context->ThreadHandle);
                context->ThreadHandle = NULL;
            }

            PvDeleteDynRelocTree(context);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (context->PropSheetContext && !context->PropSheetContext->LayoutInitialized)
            {
                PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                context->PropSheetContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_PV_SEARCH_FINISHED:
        {
            // Publish the worker's results: after this point the lists are stable and UI-thread
            // readers (TreeNewGetChildren, the filter callback) may touch them.
            context->Loading = FALSE;

            TreeNew_SetRedraw(context->TreeNewHandle, FALSE);
            TreeNew_NodesStructured(context->TreeNewHandle);
            PhApplyTreeNewFilters(&context->FilterSupport);

            // A query typed while loading was deferred; honour it now that the list is stable.
            if (context->SearchMatchHandle)
                PvRevealDynRelocMatches(context);

            TreeNew_SetRedraw(context->TreeNewHandle, TRUE);

            TreeNew_SetEmptyText(context->TreeNewHandle, &EmptyDynRelocText, 0);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case WM_PV_DYNRELOC_CONTEXTMENU:
                {
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM selectedItem;

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, USHRT_MAX, L"Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyCellEMenuItem(menu, USHRT_MAX, context->TreeNewHandle, contextMenuEvent->Column);

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        contextMenuEvent->Location.x,
                        contextMenuEvent->Location.y
                        );

                    if (selectedItem && selectedItem->Id != ULONG_MAX)
                    {
                        if (!PhHandleCopyCellEMenuItem(selectedItem))
                        {
                            switch (selectedItem->Id)
                            {
                            case USHRT_MAX:
                                {
                                    PPH_STRING text;

                                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                                    PhSetClipboardString(context->TreeNewHandle, &text->sr);
                                    PhDereferenceObject(text);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)context->TreeNewHandle);
                return TRUE;
            }
        }
        break;
    case WM_KEYDOWN:
        {
            if (LOWORD(wParam) == 'K' && GetKeyState(VK_CONTROL) < 0)
            {
                SetFocus(context->SearchHandle);
                return TRUE;
            }
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
