/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020-2026
 *
 */

#include <peview.h>

typedef struct _PVP_PE_PRODUCTION_ID_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HWND HashListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_PRODUCTION_ID_CONTEXT, *PPVP_PE_PRODUCTION_ID_CONTEXT;

typedef enum _PVP_PRODID_HASH_INDEX
{
    PVP_PRODID_HASH_INDEX_XORKEY,
    PVP_PRODID_HASH_INDEX_CHECKSUMVALID,
    PVP_PRODID_HASH_INDEX_DANSSIGNATURE,
    PVP_PRODID_HASH_INDEX_RICHMD5,
    PVP_PRODID_HASH_INDEX_RICHRAWMD5,
    PVP_PRODID_HASH_INDEX_RICHSHA1,
    PVP_PRODID_HASH_INDEX_RICHRAWSHA1,
    PVP_PRODID_HASH_INDEX_PRODID,
    PVP_PRODID_HASH_INDEX_PRODIDCOUNT,
    PVP_PRODID_HASH_INDEX_PRODIDVERSION,
    PVP_PRODID_HASH_INDEX_PRODIDVERSIONCOUNT,
    PVP_PRODID_HASH_INDEX_SORTEDPRODID,
    PVP_PRODID_HASH_INDEX_SORTEDPRODIDCOUNT,
    PVP_PRODID_HASH_INDEX_SORTEDPRODIDVERSION,
    PVP_PRODID_HASH_INDEX_SORTEDPRODIDVERSIONCOUNT,
    PVP_PRODID_HASH_INDEX_MAXIMUM
} PVP_PRODID_HASH_INDEX;

typedef enum _PVP_PRODID_HASH_FLAGS
{
    PVP_PRODID_HASH_PRODIDONLY = 0x0,
    PVP_PRODID_HASH_VERSION = 0x1,  // include ProductBuild
    PVP_PRODID_HASH_COUNT = 0x2,    // include ProductCount
    PVP_PRODID_HASH_SORTED = 0x4,   // sort entries ascending by ProductId
} PVP_PRODID_HASH_FLAGS;

PWSTR PvpGetProductIdName(
    _In_ ULONG ProductId
    )
{
    if (ProductId >= prodidAliasObj1400)
        return L"Visual Studio 2015 (14.0)";
    else if (ProductId >= prodidAliasObj1210)
        return L"Visual Studio 2013 (12.1)";
    else if (ProductId >= prodidAliasObj1200)
        return L"Visual Studio 2013 (12.0)";
    else if (ProductId >= prodidAliasObj1100)
        return L"Visual Studio 2012 (11.0)";
    else if (ProductId >= prodidAliasObj1010)
        return L"Visual Studio 2010 (10.1)";
    else if (ProductId >= prodidAliasObj1000)
        return L"Visual Studio 2010 (10.0)";
    else if (ProductId >= prodidLinker900) // prodidAliasObj900
        return L"Visual Studio 2008 (9.0)";
    else if (ProductId >= prodidLinker800) // prodidAliasObj710
        return L"Visual Studio 2003 (7.1)";
    else if (ProductId >= prodidLinker800) // prodidAliasObj710
        return L"Visual Studio 2003 (7.1)";
    else if (ProductId >= prodidAliasObj70)
        return L"Visual Studio 2002 (7.0)";
    else if (ProductId >= prodidAliasObj60)
        return L"Visual Studio 1998 (6.0)";
    else if (ProductId == 1)
        return L"Linker (Import Table)";

    return PhaFormatString(L"Report error (%lu)", ProductId)->Buffer;
}

PWSTR PvpGetProductIdComponent(
    _In_ ULONG ProductId
    )
{
    switch (ProductId)
    {
    case prodidUnknown: // linker generated unnamed ordinal export stubs with RVAs of zero?
        return L"Linker generated export object";
    case prodidImport0:
        return L"Linker generated import object";
    case prodidResource:
        return L"Resource compiler object";

    case prodidAliasObj60:
        return L"ALIASOBJ (6.0)";
    case prodidAliasObj70:
        return L"ALIASOBJ (7.0)";
    case prodidAliasObj710:
    case prodidAliasObj710p:
        return L"ALIASOBJ (7.1)";
    case prodidAliasObj800:
        return L"ALIASOBJ (8.0)";
    case prodidAliasObj900:
        return L"ALIASOBJ (9.0)";
    case prodidAliasObj1000:
        return L"ALIASOBJ (10.0)";
    case prodidAliasObj1010:
        return L"ALIASOBJ (10.1)";
    case prodidAliasObj1100:
        return L"ALIASOBJ (11.0)";
    case prodidAliasObj1200:
        return L"ALIASOBJ (12.0)";
    case prodidAliasObj1210:
        return L"ALIASOBJ (12.1)";
    case prodidAliasObj1400:
        return L"ALIASOBJ (14.0)";

    case prodidCvtpgd1400:
        return L"Profile Guided Optimization (PGO) (14.0)";
    case prodidCvtpgd1500:
        return L"Profile Guided Optimization (PGO) (15.0)";
    case prodidCvtpgd1600:
        return L"Profile Guided Optimization (PGO) (16.0)";
    case prodidCvtpgd1610:
        return L"Profile Guided Optimization (PGO) (16.1)";
    case prodidCvtpgd1700:
        return L"Profile Guided Optimization (PGO) (17.0)";
    case prodidCvtpgd1800:
        return L"Profile Guided Optimization (PGO) (18.0)";
    case prodidCvtpgd1810:
        return L"Profile Guided Optimization (PGO) (18.1)";
    case prodidCvtpgd1900:
        return L"Profile Guided Optimization (PGO) (19.0)";

    case prodidCvtres800:
        return L"Resource File To COFF Object (8.0)";
    case prodidCvtres900:
        return L"Resource File To COFF Object (9.0)";
    case prodidCvtres1000:
        return L"Resource File To COFF Object (10.0)";
    case prodidCvtres1010:
        return L"Resource File To COFF Object (10.1)";
    case prodidCvtres1100:
        return L"Resource File To COFF Object (11.0)";
    case prodidCvtres1200:
        return L"Resource File To COFF Object (12.0)";
    case prodidCvtres1210:
        return L"Resource File To COFF Object (12.1)";
    case prodidCvtres1400:
        return L"Resource File To COFF Object (14.0)";

    case prodidExport800:
        return L"Export (8.0)";
    case prodidExport900:
        return L"Export (9.0)";
    case prodidExport1000:
        return L"Export (10.0)";
    case prodidExport1010:
        return L"Export (10.1)";
    case prodidExport1100:
        return L"Export (11.0)";
    case prodidExport1200:
        return L"Export (12.0)";
    case prodidExport1210:
        return L"Export (12.1)";
    case prodidExport1400:
        return L"Export (14.0)";

    case prodidImplib800:
        return L"Import library tool (LIB) (8.0)";
    case prodidImplib900:
        return L"Import library tool (LIB) (9.0)";
    case prodidImplib1000:
        return L"Import library tool (LIB) (10.0)";
    case prodidImplib1010:
        return L"Import library tool (LIB) (10.1)";
    case prodidImplib1100:
        return L"Import library tool (LIB) (11.0)";
    case prodidImplib1200:
        return L"Import library tool (LIB) (12.0)";
    case prodidImplib1210:
        return L"Import library tool (LIB) (12.1)";
    case prodidImplib1400:
        return L"Import library tool (LIB) (14.0)";

    case prodidLinker800:
        return L"Linker (8.0)";
    case prodidLinker900:
        return L"Linker (9.0)";
    case prodidLinker1000:
        return L"Linker (10.0)";
    case prodidLinker1010:
        return L"Linker (10.1)";
    case prodidLinker1100:
        return L"Linker (11.0)";
    case prodidLinker1200:
        return L"Linker (12.0)";
    case prodidLinker1210:
        return L"Linker (12.1)";
    case prodidLinker1400:
        return L"Linker (14.0)";

    case prodidMasm800:
        return L"MASM (8.0)";
    case prodidMasm900:
        return L"MASM (9.0)";
    case prodidMasm1000:
        return L"MASM (10.0)";
    case prodidMasm1010:
        return L"MASM (10.1)";
    case prodidMasm1100:
        return L"MASM (11.0)";
    case prodidMasm1200:
        return L"MASM (12.0)";
    case prodidMasm1210:
        return L"MASM (12.1)";
    case prodidMasm1400:
        return L"MASM (14.0)";

    case prodidUtc1400_C:
        return L"C files (14.0)";
    case prodidUtc1500_C:
        return L"C files (15.0)";
    case prodidUtc1610_C:
        return L"C files (16.1)";
    case prodidUtc1700_C:
        return L"C files (17.0)";
    case prodidUtc1800_C:
        return L"C files (18.0)";
    case prodidUtc1810_C:
        return L"C files (18.1)";
    case prodidUtc1900_C:
        return L"C files (19.0)";

    case prodidUtc1400_CPP:
        return L"CPP files (14.0)";
    case prodidUtc1500_CPP:
        return L"CPP files (15.0)";
    case prodidUtc1610_CPP:
        return L"CPP files (16.1)";
    case prodidUtc1700_CPP:
        return L"CPP files (17.0)";
    case prodidUtc1800_CPP:
        return L"CPP files (18.0)";
    case prodidUtc1810_CPP:
        return L"CPP files (18.1)";
    case prodidUtc1900_CPP:
        return L"CPP files (19.0)";

    case prodidUtc1500_CVTCIL_C:
        return L"CIL to Native Converter (C99) (15.0)";
    case prodidUtc1610_CVTCIL_C:
        return L"CIL to Native Converter (C99) (16.1)";
    case prodidUtc1700_CVTCIL_C:
        return L"CIL to Native Converter (C99) (17.0)";
    case prodidUtc1800_CVTCIL_C:
        return L"CIL to Native Converter (C11) (18.0)";
    case prodidUtc1810_CVTCIL_C:
        return L"CIL to Native Converter (C11) (18.1)";
    case prodidUtc1900_CVTCIL_C:
        return L"CIL to Native Converter (C11) (19.0)";

    case prodidUtc1500_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (15.0)";
    case prodidUtc1610_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (16.1)";
    case prodidUtc1700_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (17.0)";
    case prodidUtc1800_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (18.0)";
    case prodidUtc1810_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (18.1)";
    case prodidUtc1900_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (19.0)";

    case prodidUtc1500_LTCG_C:
        return L"Link-time Code Generation (C99) (15.0)";
    case prodidUtc1610_LTCG_C:
        return L"Link-time Code Generation (C99) (16.1)";
    case prodidUtc1700_LTCG_C:
        return L"Link-time Code Generation (C99) (17.0)";
    case prodidUtc1800_LTCG_C:
        return L"Link-time Code Generation (C11) (18.0)";
    case prodidUtc1810_LTCG_C:
        return L"Link-time Code Generation (C11) (18.1)";
    case prodidUtc1900_LTCG_C:
        return L"Link-time Code Generation (C11) (19.0)";

    case prodidUtc1500_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (15.0)";
    case prodidUtc1610_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (16.1)";
    case prodidUtc1700_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (17.0)";
    case prodidUtc1800_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (18.0)";
    case prodidUtc1810_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (18.1)";
    case prodidUtc1900_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (19.0)";

    case prodidUtc1500_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (15.0)";
    case prodidUtc1610_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (16.1)";
    case prodidUtc1700_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (17.0)";
    case prodidUtc1800_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (18.0)";
    case prodidUtc1810_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (18.1)";
    case prodidUtc1900_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (19.0)";

    case prodidUtc1500_POGO_I_C:
        return L"Profile Guided Optimization (Input) (C11) (15.0)";
    case prodidUtc1610_POGO_I_C:
        return L"Profile Guided Optimization (Input) (C11) (16.1)";
    case prodidUtc1700_POGO_I_C:
        return L"Profile Guided Optimization (Input) (C11) (17.0)";
    case prodidUtc1800_POGO_I_C:
        return L"Profile Guided Optimization (Input) (C11) (18.0)";
    case prodidUtc1810_POGO_I_C:
        return L"Profile Guided Optimization (Input) (C11) (18.1)";
    case prodidUtc1900_POGO_I_C:
        return L"Profile Guided Optimization (Input) (C11) (19.0)";

    case prodidUtc1400_POGO_O_C:
    case prodidUtc1500_POGO_O_C:
    case prodidUtc1610_POGO_O_C:
    case prodidUtc1700_POGO_O_C:
    case prodidUtc1800_POGO_O_C:
    case prodidUtc1810_POGO_O_C:
    case prodidUtc1900_POGO_O_C:
        return L"Profile Guided Optimization (Output) (C11)";

    case prodidUtc1400_POGO_I_CPP:
    case prodidUtc1500_POGO_I_CPP:
    case prodidUtc1610_POGO_I_CPP:
    case prodidUtc1700_POGO_I_CPP:
    case prodidUtc1800_POGO_I_CPP:
    case prodidUtc1810_POGO_I_CPP:
    case prodidUtc1900_POGO_I_CPP:
        return L"Profile Guided Optimization (Input) (CPP)";

    case prodidUtc1400_POGO_O_CPP:
    case prodidUtc1500_POGO_O_CPP:
    case prodidUtc1610_POGO_O_CPP:
    case prodidUtc1700_POGO_O_CPP:
    case prodidUtc1800_POGO_O_CPP:
    case prodidUtc1810_POGO_O_CPP:
    case prodidUtc1900_POGO_O_CPP:
        return L"Profile Guided Optimization (Output) (C11)";
    }

    return PhaFormatString(L"Report error (%lu)", ProductId)->Buffer;
}

/**
 * Compares two ProdID entries by their product identifier.
 */
static int __cdecl PvpPeProdIdEntryCompare(
    _In_ const void* Left,
    _In_ const void* Right
    )
{
    CONST PH_MAPPED_IMAGE_PRODID_ENTRY* left = Left;
    CONST PH_MAPPED_IMAGE_PRODID_ENTRY* right = Right;

    if (left->ProductId != right->ProductId)
        return left->ProductId < right->ProductId ? -1 : 1;
    if (left->ProductBuild != right->ProductBuild)
        return left->ProductBuild < right->ProductBuild ? -1 : 1;
    if (left->ProductCount != right->ProductCount)
        return left->ProductCount < right->ProductCount ? -1 : 1;

    return 0;
}

/**
 * Computes the SHA-256 hash of a canonical rendering of the ProdID entry list.
 *
 * The canonical form renders each entry as its decimal fields joined by '.' in the
 * order id[.version][.count], with entries joined by ',' and no trailing separator.
 * The resulting ASCII text is hashed. Entries with a zero count are skipped, matching
 * the entries shown in the listview.
 *
 * \param ProdIdHeader The parsed ProdID header.
 * \param Flags Controls which fields are included and whether entries are sorted.
 * \param HashString A variable which receives the hexadecimal hash string.
 * \return An NTSTATUS value indicating success or failure.
 */
NTSTATUS PvpPeGetProdIdHash(
    _In_ PPH_MAPPED_IMAGE_PRODID ProdIdHeader,
    _In_ ULONG Flags,
    _Out_ PPH_STRING* HashString
    )
{
    NTSTATUS status;
    PH_HASH_CONTEXT hashContext;
    PPH_MAPPED_IMAGE_PRODID_ENTRY entries;
    PH_STRING_BUILDER stringBuilder;
    PPH_BYTES bytes;
    SIZE_T count;

    if (ProdIdHeader->NumberOfEntries == 0 || !ProdIdHeader->ProdIdEntries)
        return STATUS_NOT_FOUND;

    count = ProdIdHeader->NumberOfEntries;
    entries = PhAllocateCopy(ProdIdHeader->ProdIdEntries, count * sizeof(PH_MAPPED_IMAGE_PRODID_ENTRY));

    if (FlagOn(Flags, PVP_PRODID_HASH_SORTED))
    {
        qsort(entries, count, sizeof(PH_MAPPED_IMAGE_PRODID_ENTRY), PvpPeProdIdEntryCompare);
    }

    PhInitializeStringBuilder(&stringBuilder, 100);

    for (SIZE_T i = 0; i < count; i++)
    {
        PH_MAPPED_IMAGE_PRODID_ENTRY entry = entries[i];

        if (!entry.ProductCount)
            continue;

        if (stringBuilder.String->Length)
            PhAppendCharStringBuilder(&stringBuilder, L',');

        PhAppendFormatStringBuilder(&stringBuilder, L"%hu", entry.ProductId);

        if (FlagOn(Flags, PVP_PRODID_HASH_VERSION))
            PhAppendFormatStringBuilder(&stringBuilder, L".%hu", entry.ProductBuild);
        if (FlagOn(Flags, PVP_PRODID_HASH_COUNT))
            PhAppendFormatStringBuilder(&stringBuilder, L".%lu", entry.ProductCount);
    }

    PhFree(entries);

    bytes = PhConvertUtf16ToUtf8Ex(stringBuilder.String->Buffer, stringBuilder.String->Length);
    PhDeleteStringBuilder(&stringBuilder);

    if (!bytes)
        return STATUS_UNSUCCESSFUL;

    if (NT_SUCCESS(status = PhInitializeHash(&hashContext, Sha256HashAlgorithm)))
    {
        if (NT_SUCCESS(status = PhUpdateHash(&hashContext, bytes->Buffer, (ULONG)bytes->Length)))
        {
            status = PhFinalHashString(&hashContext, HashString);
        }
    }

    PhDereferenceObject(bytes);

    return status;
}

/**
 * Computes a hash over the Rich header, either as stored or after removing the XOR obfuscation.
 *
 * \param Algorithm The hash algorithm to use.
 * \param Deobfuscate TRUE to XOR the header with its key before hashing.
 * \param HashString A variable which receives the hexadecimal hash string.
 * \return An NTSTATUS value indicating success or failure.
 */
NTSTATUS PvpPeGetRichHeaderHash(
    _In_ PH_HASH_ALGORITHM Algorithm,
    _In_ BOOLEAN Deobfuscate,
    _Out_ PPH_STRING* HashString
    )
{
    NTSTATUS status;
    PH_HASH_CONTEXT hashContext;
    ULONG headerStart = 0;
    ULONG headerEnd = 0;
    ULONG headerLength = 0;
    ULONG key = 0;
    PVOID headerBuffer;
    PVOID headerAddress;

    status = PhGetMappedImageProdIdExtents(&PvMappedImage, &headerStart, &headerEnd);

    if (!NT_SUCCESS(status))
        return status;

    if (headerEnd <= headerStart)
        return STATUS_INVALID_IMAGE_FORMAT;

    headerAddress = PTR_ADD_OFFSET(PvMappedImage.ViewBase, headerStart);

    __try
    {
        PhMappedImageProbe(&PvMappedImage, headerAddress, headerEnd - headerStart);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    // PhGetMappedImageProdIdExtents returns the NT headers offset as the end boundary, which can
    // include padding after the header. Walk to the "Rich" tag and hash only the DanS tag through
    // the last ProdID entry, excluding the trailing "Rich" tag and XOR key. This span was verified
    // against a reference report: it reproduces both the obfuscated and deobfuscated digests. (dmex)

    __try
    {
        for (ULONG offset = 0; offset + sizeof(ULONG) * 2 <= headerEnd - headerStart; offset += sizeof(ULONG))
        {
            if (*(PULONG)PTR_ADD_OFFSET(headerAddress, offset) == ProdIdTagStart)
            {
                headerLength = offset;
                key = *(PULONG)PTR_ADD_OFFSET(headerAddress, offset + sizeof(ULONG));
                break;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    if (headerLength == 0)
        return STATUS_INVALID_IMAGE_FORMAT;

    headerBuffer = PhAllocateStack(headerLength);
    RtlCopyMemory(headerBuffer, headerAddress, headerLength);
    
    if (Deobfuscate)
    {
        PULONG bufferEnd = (PULONG)PTR_ADD_OFFSET(headerBuffer, headerLength);

        for (PULONG p = headerBuffer; p < bufferEnd; p++)
        {
            *p ^= key;
        }
    }

    if (NT_SUCCESS(status = PhInitializeHash(&hashContext, Algorithm)))
    {
        if (NT_SUCCESS(status = PhUpdateHash(&hashContext, headerBuffer, headerLength)))
        {
            status = PhFinalHashString(&hashContext, HashString);
        }
    }

    PhFreeStack(headerBuffer);

    return status;
}

VOID PvpPeSetProdIdHashItem(
    _In_ HWND ListViewHandle,
    _In_ ULONG Index,
    _In_ PPH_STRING String
    )
{
    if (String)
    {
        PhSetListViewSubItem(ListViewHandle, Index, 1, PhGetString(String));
        PhDereferenceObject(String);
    }
}

VOID PvpPeEnumProdHashes(
    _In_ HWND ListViewHandle,
    _In_ PPH_MAPPED_IMAGE_PRODID ProdIdHeader
    )
{
    static CONST struct
    {
        ULONG Index;
        ULONG Flags;
    } prodIdHashes[] =
    {
        { PVP_PRODID_HASH_INDEX_PRODID, PVP_PRODID_HASH_PRODIDONLY },
        { PVP_PRODID_HASH_INDEX_PRODIDCOUNT, PVP_PRODID_HASH_COUNT },
        { PVP_PRODID_HASH_INDEX_PRODIDVERSION, PVP_PRODID_HASH_VERSION },
        { PVP_PRODID_HASH_INDEX_PRODIDVERSIONCOUNT, PVP_PRODID_HASH_VERSION | PVP_PRODID_HASH_COUNT },
        { PVP_PRODID_HASH_INDEX_SORTEDPRODID, PVP_PRODID_HASH_SORTED },
        { PVP_PRODID_HASH_INDEX_SORTEDPRODIDCOUNT, PVP_PRODID_HASH_SORTED | PVP_PRODID_HASH_COUNT },
        { PVP_PRODID_HASH_INDEX_SORTEDPRODIDVERSION, PVP_PRODID_HASH_SORTED | PVP_PRODID_HASH_VERSION },
        { PVP_PRODID_HASH_INDEX_SORTEDPRODIDVERSIONCOUNT, PVP_PRODID_HASH_SORTED | PVP_PRODID_HASH_VERSION | PVP_PRODID_HASH_COUNT },
    };
    PPH_STRING string;

    // The XOR key, shown as stored (hex) and in decimal for comparison with other tools.
    {
        ULONG64 key = 0;

        if (ProdIdHeader->Key && PhStringToUInt64(&ProdIdHeader->Key->sr, 16, &key))
        {
            PH_FORMAT format[4];

            PhInitFormatSR(&format[0], ProdIdHeader->Key->sr);
            PhInitFormatS(&format[1], L" (");
            PhInitFormatI64U(&format[2], key);
            PhInitFormatC(&format[3], L')');

            string = PhFormat(format, RTL_NUMBER_OF(format), 30);
        }
        else
        {
            string = PhReferenceObject(ProdIdHeader->Key);
        }

        PvpPeSetProdIdHashItem(ListViewHandle, PVP_PRODID_HASH_INDEX_XORKEY, string);
    }

    PhSetListViewSubItem(ListViewHandle, PVP_PRODID_HASH_INDEX_CHECKSUMVALID, 1, ProdIdHeader->Valid ? L"True" : L"False");

    // PhGetMappedImageProdIdHeader only succeeds after matching both the DanS and Rich tags,
    // so reaching this point means the signature is present. (dmex)
    PhSetListViewSubItem(ListViewHandle, PVP_PRODID_HASH_INDEX_DANSSIGNATURE, 1, L"True");

    // Compute all four Rich header digests over the same verified span rather than reusing
    // ProdIdHeader->RawHash, which phlib scopes differently (it includes the trailing tag/key). (dmex)

    if (NT_SUCCESS(PvpPeGetRichHeaderHash(Md5HashAlgorithm, TRUE, &string)))
        PvpPeSetProdIdHashItem(ListViewHandle, PVP_PRODID_HASH_INDEX_RICHMD5, string);
    if (NT_SUCCESS(PvpPeGetRichHeaderHash(Md5HashAlgorithm, FALSE, &string)))
        PvpPeSetProdIdHashItem(ListViewHandle, PVP_PRODID_HASH_INDEX_RICHRAWMD5, string);
    if (NT_SUCCESS(PvpPeGetRichHeaderHash(Sha1HashAlgorithm, TRUE, &string)))
        PvpPeSetProdIdHashItem(ListViewHandle, PVP_PRODID_HASH_INDEX_RICHSHA1, string);
    if (NT_SUCCESS(PvpPeGetRichHeaderHash(Sha1HashAlgorithm, FALSE, &string)))
        PvpPeSetProdIdHashItem(ListViewHandle, PVP_PRODID_HASH_INDEX_RICHRAWSHA1, string);

    for (ULONG i = 0; i < RTL_NUMBER_OF(prodIdHashes); i++)
    {
        if (NT_SUCCESS(PvpPeGetProdIdHash(ProdIdHeader, prodIdHashes[i].Flags, &string)))
        {
            PvpPeSetProdIdHashItem(ListViewHandle, prodIdHashes[i].Index, string);
        }
    }
}

VOID PvpPeEnumProdEntries(
    _In_ HWND WindowHandle,
    _In_ HWND ListViewHandle,
    _In_ HWND HashListViewHandle
    )
{
    PH_MAPPED_IMAGE_PRODID prodids;
    PH_MAPPED_IMAGE_PRODID_ENTRY entry;
    ULONG count = 0;
    ULONG i;
    INT lvItemIndex;

    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);

    if (NT_SUCCESS(PhGetMappedImageProdIdHeader(&PvMappedImage, &prodids)))
    {
        PvpPeEnumProdHashes(HashListViewHandle, &prodids);

        for (i = 0; i < prodids.NumberOfEntries; i++)
        {
            WCHAR number[PH_INT32_STR_LEN_1];

            entry = prodids.ProdIdEntries[i];

            if (!entry.ProductCount)
                continue;

            PhPrintUInt32(number, ++count);
            lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, number, NULL);
            //PhSetListViewSubItem(lvHandle, lvItemIndex, 4, PvpGetProductIdName(entry.ProductId));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, PvpGetProductIdComponent(entry.ProductId));

            if (entry.ProductBuild)
            {
                PhPrintUInt32(number, entry.ProductBuild);
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, number);
            }

            PhPrintUInt32(number, entry.ProductCount);
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, number);
        }

        PhFree(prodids.ProdIdEntries);
        PhClearReference(&prodids.Hash);
        PhClearReference(&prodids.RawHash);
        PhClearReference(&prodids.Key);
    }
    else
    {
        PhSetListViewSubItem(HashListViewHandle, PVP_PRODID_HASH_INDEX_CHECKSUMVALID, 1, L"False");
        PhSetListViewSubItem(HashListViewHandle, PVP_PRODID_HASH_INDEX_DANSSIGNATURE, 1, L"False");
    }

    //ExtendedListView_SortItems(ListViewHandle);
    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvpPeProdIdDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_PRODUCTION_ID_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_PRODUCTION_ID_CONTEXT));
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
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            context->HashListViewHandle = GetDlgItem(hwndDlg, IDC_LIST2);

            PhSetListViewStyle(context->HashListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->HashListViewHandle, L"explorer");
            PhAddListViewColumn(context->HashListViewHandle, 0, 0, 0, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(context->HashListViewHandle, 1, 1, 1, LVCFMT_LEFT, 320, L"Value");
            PhSetExtendedListView(context->HashListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageProdIdHashListViewColumns", context->HashListViewHandle);
            PvConfigTreeBorders(context->HashListViewHandle);

            PhAddListViewItem(context->HashListViewHandle, PVP_PRODID_HASH_INDEX_XORKEY, L"XOR key", NULL);
            PhAddListViewItem(context->HashListViewHandle, PVP_PRODID_HASH_INDEX_CHECKSUMVALID, L"Checksum valid", NULL);
            PhAddListViewItem(context->HashListViewHandle, PVP_PRODID_HASH_INDEX_DANSSIGNATURE, L"DanS signature present", NULL);
            PhAddListViewItem(context->HashListViewHandle, PVP_PRODID_HASH_INDEX_RICHMD5, L"Rich header MD5 (deobfuscated)", NULL);
            PhAddListViewItem(context->HashListViewHandle, PVP_PRODID_HASH_INDEX_RICHRAWMD5, L"Rich header MD5 (obfuscated)", NULL);
            PhAddListViewItem(context->HashListViewHandle, PVP_PRODID_HASH_INDEX_RICHSHA1, L"Rich header SHA-1 (deobfuscated)", NULL);
            PhAddListViewItem(context->HashListViewHandle, PVP_PRODID_HASH_INDEX_RICHRAWSHA1, L"Rich header SHA-1 (obfuscated)", NULL);
            PhAddListViewItem(context->HashListViewHandle, PVP_PRODID_HASH_INDEX_PRODID, L"ProdID SHA-256 (PE Viewer)", NULL);
            PhAddListViewItem(context->HashListViewHandle, PVP_PRODID_HASH_INDEX_PRODIDCOUNT, L"ProdID count SHA-256 (PE Viewer)", NULL);
            PhAddListViewItem(context->HashListViewHandle, PVP_PRODID_HASH_INDEX_PRODIDVERSION, L"ProdID version SHA-256 (PE Viewer)", NULL);
            PhAddListViewItem(context->HashListViewHandle, PVP_PRODID_HASH_INDEX_PRODIDVERSIONCOUNT, L"ProdID version count SHA-256 (PE Viewer)", NULL);
            PhAddListViewItem(context->HashListViewHandle, PVP_PRODID_HASH_INDEX_SORTEDPRODID, L"Sorted ProdID SHA-256 (PE Viewer)", NULL);
            PhAddListViewItem(context->HashListViewHandle, PVP_PRODID_HASH_INDEX_SORTEDPRODIDCOUNT, L"Sorted ProdID count SHA-256 (PE Viewer)", NULL);
            PhAddListViewItem(context->HashListViewHandle, PVP_PRODID_HASH_INDEX_SORTEDPRODIDVERSION, L"Sorted ProdID version SHA-256 (PE Viewer)", NULL);
            PhAddListViewItem(context->HashListViewHandle, PVP_PRODID_HASH_INDEX_SORTEDPRODIDVERSIONCOUNT, L"Sorted ProdID version count SHA-256 (PE Viewer)", NULL);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Component");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"Version");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Count");
            //PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Product");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageProdIdListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->HashListViewHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvpPeEnumProdEntries(hwndDlg, context->ListViewHandle, context->HashListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageProdIdListViewColumns", context->ListViewHandle);
            PhSaveListViewColumnsToSetting(L"ImageProdIdHashListViewColumns", context->HashListViewHandle);
            PhDeleteLayoutManager(&context->LayoutManager);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);
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
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
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
