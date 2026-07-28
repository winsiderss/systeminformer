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
    BOOLEAN HasRvaValue; // FALSE for structural rows (Patch sites, Decision tree, TRUE/FALSE edges)

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
    // structural rows (Patch sites, Decision tree) and edge labels (TRUE/FALSE) sort as 0.
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
        *TypeName = L"";
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
    case IMAGE_FUNCTION_OVERRIDE_X64_REL32: return L"X64 REL32";
    case IMAGE_FUNCTION_OVERRIDE_ARM64_BRANCH26: return L"ARM64 BRANCH26";
    case IMAGE_FUNCTION_OVERRIDE_ARM64_THUNK: return L"ARM64 THUNK";
    default: return L"UNKNOWN";
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

PPH_STRING PvDynRelocOutcomeString(
    _In_ PPH_FUNCTION_OVERRIDE_OUTCOME Outcome
    )
{
    if (Outcome->Type == PhFunctionOverrideKeepOriginal)
        return PhCreateString(L"keep original");

    return PhFormatString(L"override[%lu]", Outcome->RvaIndex);
}

// Emits a single leaf (terminal) row under ParentNode: the resolved target RVA in the RVA column
// (so it sorts and resolves Section/Symbol) and the outcome kind in Info.
VOID PvDynRelocAddBddLeaf(
    _Inout_ PPV_PE_DYNRELOC_CONTEXT Context,
    _In_ PPV_DYNRELOC_NODE ParentNode,
    _In_ PPH_FUNCTION_OVERRIDE_BDD_NODE Node
    )
{
    PPV_DYNRELOC_NODE treeNode;

    treeNode = PvAddDynRelocNode(
        Context,
        ParentNode,
        PH_AUTO_T(PH_STRING, PhFormatString(L"0x%lx", Node->Terminal.Rva))->Buffer,
        NULL,
        PvDynRelocOutcomeString(&Node->Terminal)
        );

    PvDynRelocSetRvaColumns(treeNode, Node->Terminal.Rva);
}

// Lays out the BDD node at Index as content under ParentNode, reading like C.
//
// Two shapes are flattened so the tree stays shallow and legible:
//
//   * else-spine: internal nodes chained by their FALSE (feature-absent) edge become sibling
//     "if" / "else if" / ... / "else" rows rather than ever-deeper nesting.
//   * AND-chain: a run of internal nodes chained by their TRUE (feature-present) edge that all
//     share one common FALSE target is a short-circuit AND, collapsed into a single
//     "if (feature A && feature B && ...)" row. Only a genuine sub-decision inside a then-branch
//     (a TRUE target that is itself a fresh decision) increases depth.
//
// A BDD is a DAG: each internal node is expanded at most once (tracked in Expanded); a node
// reached again elsewhere emits a compact "(shared node N)" reference instead of being re-walked
// (which would be exponential and, for malformed back-edges, unbounded).
//
// Root call passes ParentNode = the "Decision tree" node and Index = 0. A terminal root (an
// unconditional override) renders as a single outcome row with no if/else.
VOID PvDynRelocAddBddNode(
    _Inout_ PPV_PE_DYNRELOC_CONTEXT Context,
    _In_ PPV_DYNRELOC_NODE ParentNode,
    _In_ PPH_LIST Nodes,
    _In_ ULONG Index,
    _In_ PBOOLEAN Expanded
    )
{
    BOOLEAN first = TRUE; // first link in the else-spine -> "if"; subsequent -> "else if"

    for (;;)
    {
        PPH_FUNCTION_OVERRIDE_BDD_NODE node;
        PPV_DYNRELOC_NODE condNode;
        PH_STRING_BUILDER conditionBuilder;
        ULONG commonFalse;
        ULONG thenIndex;
        BOOLEAN alwaysAbsent = FALSE;
        PPH_STRING info;

        if (Index >= Nodes->Count)
            return;

        node = Nodes->Items[Index];

        // Tail of the else-spine: a terminal outcome, or a shared node we cannot re-expand. When it
        // is the very first link there was no preceding condition, so emit it directly (an
        // unconditional outcome); otherwise wrap it in a trailing "else".
        if (node->IsTerminal)
        {
            if (first)
            {
                PvDynRelocAddBddLeaf(Context, ParentNode, node);
            }
            else
            {
                PPV_DYNRELOC_NODE elseNode = PvAddDynRelocNode(Context, ParentNode, L"else", NULL, NULL);
                PvDynRelocAddBddLeaf(Context, elseNode, node);
            }
            return;
        }

        if (Expanded[Index])
        {
            info = PhFormatString(L"(shared node %lu)", Index);

            if (first)
            {
                PvAddDynRelocNode(Context, ParentNode, NULL, NULL, info);
            }
            else
            {
                PPV_DYNRELOC_NODE elseNode = PvAddDynRelocNode(Context, ParentNode, L"else", NULL, NULL);
                PvAddDynRelocNode(Context, elseNode, NULL, NULL, info);
            }
            return;
        }

        // Collapse a short-circuit AND: follow TRUE edges while each node is a fresh internal node
        // sharing the head's FALSE target, accumulating "feature A && feature B && ...". thenIndex
        // is the body taken when every feature is present; commonFalse continues the else-spine.
        commonFalse = node->Internal.FalseEdge;
        thenIndex = node->Internal.TrueEdge;

        PhInitializeStringBuilder(&conditionBuilder, 40);
        PhAppendFormatStringBuilder(&conditionBuilder, L"%ls (feature %lu", first ? L"if" : L"else if", node->Internal.FeatureNumber);

        if (node->Internal.FeatureNumber >= PH_FUNCTION_OVERRIDE_FEATURE_ALWAYS)
            alwaysAbsent = TRUE;

        Expanded[Index] = TRUE;

        for (;;)
        {
            PPH_FUNCTION_OVERRIDE_BDD_NODE nextNode;

            if (thenIndex >= Nodes->Count)
                break;

            nextNode = Nodes->Items[thenIndex];

            // Extend the AND only through fresh internal nodes that share the common FALSE target;
            // anything else is the then-body (or a distinct sub-decision) and ends the chain.
            if (nextNode->IsTerminal || Expanded[thenIndex] || nextNode->Internal.FalseEdge != commonFalse)
                break;

            PhAppendFormatStringBuilder(&conditionBuilder, L" && feature %lu", nextNode->Internal.FeatureNumber);

            if (nextNode->Internal.FeatureNumber >= PH_FUNCTION_OVERRIDE_FEATURE_ALWAYS)
                alwaysAbsent = TRUE;

            Expanded[thenIndex] = TRUE;
            thenIndex = nextNode->Internal.TrueEdge;
        }

        PhAppendCharStringBuilder(&conditionBuilder, L')');

        info = alwaysAbsent ? PhCreateString(L"always absent") : NULL;

        condNode = PvAddDynRelocNode(
            Context,
            ParentNode,
            PhFinalStringBuilderString(&conditionBuilder)->Buffer,
            NULL,
            info
            );
        PhDeleteStringBuilder(&conditionBuilder);

        // then-branch: every feature present. Never reached if any term is an always-absent feature
        // (at or above the loader ceiling), in which case the AND can never be satisfied.
        if (!alwaysAbsent)
            PvDynRelocAddBddNode(Context, condNode, Nodes, thenIndex, Expanded);

        // Continue the else-spine (all-features-absent path) as siblings.
        Index = commonFalse;
        first = FALSE;
    }
}

VOID PvDynRelocAddOverrideDecisionTree(
    _Inout_ PPV_PE_DYNRELOC_CONTEXT Context,
    _In_ PPV_DYNRELOC_NODE ParentNode,
    _In_ PPH_IMAGE_DYNAMIC_RELOC_ENTRY Representative
    )
{
    PV_DYNRELOC_BDD_COLLECT collect;
    PBOOLEAN expanded;

    collect.Nodes = PhCreateList(8);

    if (!NT_SUCCESS(PhFunctionOverrideEnumerateBddNodes(Representative, PvDynRelocBddCollectCallback, &collect)) ||
        collect.Nodes->Count == 0)
    {
        PvAddDynRelocNode(Context, ParentNode, L"(no decision diagram)", NULL, NULL);

        for (ULONG i = 0; i < collect.Nodes->Count; i++)
            PhFree(collect.Nodes->Items[i]);
        PhDereferenceObject(collect.Nodes);
        return;
    }

    expanded = PhAllocateZero(collect.Nodes->Count * sizeof(BOOLEAN));

    PvDynRelocAddBddNode(Context, ParentNode, collect.Nodes, 0, expanded);

    PhFree(expanded);

    for (ULONG i = 0; i < collect.Nodes->Count; i++)
        PhFree(collect.Nodes->Items[i]);
    PhDereferenceObject(collect.Nodes);
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
            PH_FUNCTION_OVERRIDE_OUTCOME baseline;
            PPH_STRING baselineString = NULL;

            // Baseline resolution (no features present) is what the loader picks on a machine
            // lacking every optional capability; every site under this override resolves the same.
            if (NT_SUCCESS(PhFunctionOverrideResolveBdd(group->Representative, NULL, 0, &baseline)))
                baselineString = PvDynRelocOutcomeString(&baseline);

            sitesNode = PvAddDynRelocNode(
                Context,
                group->RootNode,
                PH_AUTO_T(PH_STRING, PhFormatString(L"Patch sites (%lu)", group->Sites->Count))->Buffer,
                NULL,
                NULL
                );

            for (ULONG k = 0; k < group->Sites->Count; k++)
            {
                PPH_IMAGE_DYNAMIC_RELOC_ENTRY site = group->Sites->Items[k];
                PPV_DYNRELOC_NODE siteNode;

                siteNode = PvAddDynRelocNode(
                    Context,
                    sitesNode,
                    PH_AUTO_T(PH_STRING, PhFormatString(L"0x%lx", site->FuncOverride.BlockRva + site->FuncOverride.Record.Offset))->Buffer,
                    PvDynRelocOverrideTypeName(site->FuncOverride.Record.Type),
                    baselineString ? PhFormatString(L"resolves to %ls (baseline)", baselineString->Buffer) : NULL
                    );

                PvDynRelocSetEntryColumns(siteNode, site);
            }

            treeNode = PvAddDynRelocNode(
                Context,
                group->RootNode,
                L"Decision tree",
                NULL,
                NULL
                );

            PvDynRelocAddOverrideDecisionTree(Context, treeNode, group->Representative);

            if (baselineString)
                PhDereferenceObject(baselineString);
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

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI PvpPeDynRelocSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPV_PE_DYNRELOC_CONTEXT context = Context;

    assert(context);

    context->SearchMatchHandle = MatchHandle;

    PhApplyTreeNewFilters(&context->FilterSupport);
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

            PhCreateThread2(PvpPeDynRelocEnumerateThread, context);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
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
            TreeNew_SetRedraw(context->TreeNewHandle, FALSE);
            TreeNew_NodesStructured(context->TreeNewHandle);
            PhApplyTreeNewFilters(&context->FilterSupport);
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
