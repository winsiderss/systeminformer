/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 * UEFI Secure Boot signature database viewer.
 *
 * The KEK, KEKDefault, db, dbDefault, dbx and dbxDefault firmware variables are
 * EFI_SIGNATURE_LIST blobs (UEFI specification 32.4.1). Each list carries a
 * signature type GUID and a run of equally sized EFI_SIGNATURE_DATA entries.
 * EFI_CERT_X509 entries hold a DER encoded X.509 certificate, every other type
 * holds a raw digest or public key blob.
 *
 * The certificate fields are decoded by the minimal DER walker below rather than
 * phlib's PhParseCertificate: the parser is not part of the plugin export
 * surface (SystemInformer.def), and plugins link only against the executable's
 * exports. Everything the walker reads is firmware-supplied, so every field is
 * bounds checked and the walk is capped. (dmex)
 */

#include "exttools.h"

// {8be4df61-93ca-11d2-aa0d-00e098032b8c}
#define ET_EFI_GLOBAL_VARIABLE_GUID L"{8be4df61-93ca-11d2-aa0d-00e098032b8c}"
// {d719b2cb-3d3a-4596-a3bc-dad00e67656f}
#define ET_EFI_IMAGE_SECURITY_DATABASE_GUID L"{d719b2cb-3d3a-4596-a3bc-dad00e67656f}"

#include <pshpack1.h>
typedef struct _ET_EFI_SIGNATURE_LIST
{
    GUID SignatureType;
    ULONG SignatureListSize;
    ULONG SignatureHeaderSize;
    ULONG SignatureSize;
    // BYTE SignatureHeader[SignatureHeaderSize];
    // ET_EFI_SIGNATURE_DATA Signatures[];
} ET_EFI_SIGNATURE_LIST, *PET_EFI_SIGNATURE_LIST;

typedef struct _ET_EFI_SIGNATURE_DATA
{
    GUID SignatureOwner;
    BYTE SignatureData[1];
} ET_EFI_SIGNATURE_DATA, *PET_EFI_SIGNATURE_DATA;
#include <poppack.h>

// A crafted or corrupt variable must not spin the enumerator. (dmex)
#define ET_FWCERT_MAX_ENTRIES 0x10000

static CONST GUID EtEfiCertX509Guid = { 0xa5c059a1, 0x94e4, 0x4aa7, { 0x87, 0xb5, 0xab, 0x15, 0x5c, 0x2b, 0xf0, 0x72 } };
static CONST GUID EtEfiCertSha1Guid = { 0x826ca512, 0xcf10, 0x4ac9, { 0xb1, 0x87, 0xbe, 0x01, 0x49, 0x66, 0x31, 0xbd } };
static CONST GUID EtEfiCertSha224Guid = { 0x0b6e5233, 0xa65c, 0x44c9, { 0x94, 0x07, 0xd9, 0xab, 0x83, 0xbf, 0xc8, 0xbd } };
static CONST GUID EtEfiCertSha256Guid = { 0xc1c41626, 0x504c, 0x4092, { 0xac, 0xa9, 0x41, 0xf9, 0x36, 0x93, 0x43, 0x28 } };
static CONST GUID EtEfiCertSha384Guid = { 0xff3e5307, 0x9fd0, 0x48c9, { 0x85, 0xf1, 0x8a, 0xd5, 0x6c, 0x70, 0x1e, 0x01 } };
static CONST GUID EtEfiCertSha512Guid = { 0x093e0fae, 0xa6c4, 0x4f50, { 0x9f, 0x1b, 0xd4, 0x1e, 0x2b, 0x89, 0xc1, 0x9a } };
static CONST GUID EtEfiCertRsa2048Guid = { 0x3c5766e8, 0x269c, 0x4e34, { 0xaa, 0x14, 0xed, 0x77, 0x6e, 0x85, 0xb3, 0xb6 } };
static CONST GUID EtEfiCertRsa2048Sha256Guid = { 0xe2b36190, 0x879b, 0x4a3d, { 0xad, 0x8d, 0xf2, 0xe7, 0xbb, 0xa3, 0x27, 0x84 } };
static CONST GUID EtEfiCertRsa2048Sha1Guid = { 0x67f8444f, 0x8743, 0x48f1, { 0xa3, 0x28, 0x1e, 0xaa, 0xb8, 0x73, 0x60, 0x80 } };
static CONST GUID EtEfiCertX509Sha256Guid = { 0x3bd2a492, 0x96c0, 0x4079, { 0xb4, 0x20, 0xfc, 0xf9, 0x8e, 0xf1, 0x03, 0xed } };
static CONST GUID EtEfiCertX509Sha384Guid = { 0x7076876e, 0x80c2, 0x4ee6, { 0xaa, 0xd2, 0x28, 0xb3, 0x49, 0xa6, 0x86, 0x5b } };
static CONST GUID EtEfiCertX509Sha512Guid = { 0x446dbf63, 0x2502, 0x4cda, { 0xbc, 0xfa, 0x24, 0x65, 0xd2, 0xb0, 0xfe, 0x9d } };
static CONST GUID EtEfiCertTypePkcs7Guid = { 0x4aafd29d, 0x68df, 0x49ee, { 0x8a, 0xa9, 0x34, 0x7d, 0x37, 0x56, 0x65, 0xa7 } };

typedef struct _ET_FWCERT_VARIABLE
{
    PCWSTR Name;
    PCWSTR VendorGuid;
    ULONG GroupId;
} ET_FWCERT_VARIABLE, *PET_FWCERT_VARIABLE;

static CONST ET_FWCERT_VARIABLE EtFwCertVariables[] =
{
    { L"KEK", ET_EFI_GLOBAL_VARIABLE_GUID, 0 },
    { L"KEKDefault", ET_EFI_GLOBAL_VARIABLE_GUID, 1 },
    { L"db", ET_EFI_IMAGE_SECURITY_DATABASE_GUID, 2 },
    { L"dbDefault", ET_EFI_GLOBAL_VARIABLE_GUID, 3 },
    { L"dbx", ET_EFI_IMAGE_SECURITY_DATABASE_GUID, 4 },
    { L"dbxDefault", ET_EFI_GLOBAL_VARIABLE_GUID, 5 },
};

typedef struct _ET_FWCERT_ENTRY
{
    PCWSTR VariableName;
    ULONG GroupId;
    GUID SignatureType;
    GUID SignatureOwner;
    PBYTE Data;                 // view into the owning variable buffer
    ULONG DataLength;
    BOOLEAN IsCertificate;

    PPH_STRING Subject;         // subject RDN, or the digest hex for non-X509 entries
    PPH_STRING Issuer;
    PPH_STRING NotBefore;
    PPH_STRING NotAfter;
    PPH_STRING Serial;
    PPH_STRING Thumbprint;
    PPH_STRING OwnerGuidString;
} ET_FWCERT_ENTRY, *PET_FWCERT_ENTRY;

typedef struct _ET_FWCERT_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HWND ParentWindowHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPH_LIST Entries;
    PPH_LIST Buffers;           // variable buffers owning the entry views
} ET_FWCERT_CONTEXT, *PET_FWCERT_CONTEXT;

// DER
//
// The tag/length reader. Position is advanced past the parsed element on success
// and left untouched on failure.

#define ET_DER_TAG_INTEGER          0x02
#define ET_DER_TAG_BIT_STRING       0x03
#define ET_DER_TAG_OCTET_STRING     0x04
#define ET_DER_TAG_OID              0x06
#define ET_DER_TAG_UTF8_STRING      0x0c
#define ET_DER_TAG_PRINTABLE_STRING 0x13
#define ET_DER_TAG_IA5_STRING       0x16
#define ET_DER_TAG_UTC_TIME         0x17
#define ET_DER_TAG_GENERALIZED_TIME 0x18
#define ET_DER_TAG_BMP_STRING       0x1e
#define ET_DER_TAG_SEQUENCE         0x30
#define ET_DER_TAG_SET              0x31

BOOLEAN EtDerRead(
    _Inout_ PBYTE* Position,
    _In_ PBYTE End,
    _Out_ PBYTE Tag,
    _Outptr_result_buffer_(*ContentLength) PBYTE* Content,
    _Out_ PULONG ContentLength
    )
{
    PBYTE position = *Position;
    BYTE tag;
    ULONG length;
    ULONG lengthBytes;

    if (position >= End || position + 2 > End)
        return FALSE;

    tag = *position++;

    // Multi-byte tags are not used by anything this viewer reads.
    if ((tag & 0x1f) == 0x1f)
        return FALSE;

    length = *position++;

    if (length & 0x80)
    {
        lengthBytes = length & 0x7f;

        if (lengthBytes == 0 || lengthBytes > 4)
            return FALSE;
        if (position + lengthBytes > End)
            return FALSE;

        length = 0;

        for (ULONG i = 0; i < lengthBytes; i++)
            length = (length << 8) | *position++;
    }

    if (length > (ULONG)(End - position))
        return FALSE;

    *Tag = tag;
    *Content = position;
    *ContentLength = length;
    *Position = position + length;

    return TRUE;
}

// Reads the next element and requires it to carry the expected tag.
BOOLEAN EtDerReadTag(
    _Inout_ PBYTE* Position,
    _In_ PBYTE End,
    _In_ BYTE ExpectedTag,
    _Outptr_result_buffer_(*ContentLength) PBYTE* Content,
    _Out_ PULONG ContentLength
    )
{
    PBYTE position = *Position;
    BYTE tag;

    if (!EtDerRead(&position, End, &tag, Content, ContentLength))
        return FALSE;

    if (tag != ExpectedTag)
        return FALSE;

    *Position = position;

    return TRUE;
}

PCWSTR EtDerAttributeOidToName(
    _In_reads_bytes_(Length) PBYTE Oid,
    _In_ ULONG Length
    )
{
    // The joint-iso-itu-t(2) ds(5) at(4) arc encodes as 0x55 0x04 <attribute>.
    if (Length == 3 && Oid[0] == 0x55 && Oid[1] == 0x04)
    {
        switch (Oid[2])
        {
        case 3: return L"CN";
        case 5: return L"SERIALNUMBER";
        case 6: return L"C";
        case 7: return L"L";
        case 8: return L"S";
        case 9: return L"STREET";
        case 10: return L"O";
        case 11: return L"OU";
        case 12: return L"T";
        }
    }

    // 1.2.840.113549.1.9.1 emailAddress
    if (Length == 9 && memcmp(Oid, "\x2a\x86\x48\x86\xf7\x0d\x01\x09\x01", 9) == 0)
        return L"E";

    // 0.9.2342.19200300.100.1.25 domainComponent
    if (Length == 10 && memcmp(Oid, "\x09\x92\x26\x89\x93\xf2\x2c\x64\x01\x19", 10) == 0)
        return L"DC";

    return NULL;
}

PPH_STRING EtDerDecodeString(
    _In_ BYTE Tag,
    _In_reads_bytes_(Length) PBYTE Data,
    _In_ ULONG Length
    )
{
    if (Length == 0)
        return PhReferenceEmptyString();

    if (Tag == ET_DER_TAG_BMP_STRING)
    {
        // BMPString is big-endian UTF-16.
        PPH_STRING string = PhCreateStringEx(NULL, (Length & ~(ULONG)1));

        for (ULONG i = 0; i + 1 < Length; i += 2)
            string->Buffer[i / 2] = (WCHAR)((Data[i] << 8) | Data[i + 1]);

        return string;
    }

    // UTF8String, PrintableString, IA5String and the other byte-oriented types.
    return PhConvertUtf8ToUtf16Ex((PCSTR)Data, Length);
}

/**
 * Formats an X.501 Name as "CN=..., O=..., C=..." with the most specific RDN first.
 */
PPH_STRING EtDerFormatName(
    _In_reads_bytes_(Length) PBYTE Name,
    _In_ ULONG Length
    )
{
    PPH_LIST parts;
    PH_STRING_BUILDER stringBuilder;
    PBYTE position = Name;
    PBYTE end = Name + Length;

    parts = PhCreateList(8);

    while (position < end)
    {
        PBYTE rdn;
        ULONG rdnLength;
        PBYTE rdnPosition;
        PBYTE rdnEnd;

        if (!EtDerReadTag(&position, end, ET_DER_TAG_SET, &rdn, &rdnLength))
            break;

        rdnPosition = rdn;
        rdnEnd = rdn + rdnLength;

        while (rdnPosition < rdnEnd)
        {
            PBYTE pair;
            ULONG pairLength;
            PBYTE pairPosition;
            PBYTE pairEnd;
            PBYTE oid;
            ULONG oidLength;
            PBYTE value;
            ULONG valueLength;
            BYTE valueTag;
            PCWSTR name;
            PPH_STRING text;

            if (!EtDerReadTag(&rdnPosition, rdnEnd, ET_DER_TAG_SEQUENCE, &pair, &pairLength))
                break;

            pairPosition = pair;
            pairEnd = pair + pairLength;

            if (!EtDerReadTag(&pairPosition, pairEnd, ET_DER_TAG_OID, &oid, &oidLength))
                continue;
            if (!EtDerRead(&pairPosition, pairEnd, &valueTag, &value, &valueLength))
                continue;

            text = EtDerDecodeString(valueTag, value, valueLength);
            name = EtDerAttributeOidToName(oid, oidLength);

            if (name)
                PhAddItemList(parts, PhFormatString(L"%s=%s", name, PhGetString(text)));
            else
                PhAddItemList(parts, PhReferenceObject(text));

            PhDereferenceObject(text);
        }
    }

    PhInitializeStringBuilder(&stringBuilder, 0x80);

    for (ULONG i = parts->Count; i > 0; i--)
    {
        PPH_STRING part = parts->Items[i - 1];

        if (stringBuilder.String->Length != 0)
            PhAppendStringBuilder2(&stringBuilder, L", ");

        PhAppendStringBuilder(&stringBuilder, &part->sr);
        PhDereferenceObject(part);
    }

    PhDereferenceObject(parts);

    return PhFinalStringBuilderString(&stringBuilder);
}

BOOLEAN EtDerReadDigits(
    _In_reads_bytes_(Length) PBYTE Data,
    _In_ ULONG Length,
    _In_ ULONG Offset,
    _In_ ULONG Count,
    _Out_ PCSHORT Value
    )
{
    ULONG value = 0;

    if (Offset + Count > Length)
        return FALSE;

    for (ULONG i = 0; i < Count; i++)
    {
        if (Data[Offset + i] < '0' || Data[Offset + i] > '9')
            return FALSE;

        value = value * 10 + (Data[Offset + i] - '0');
    }

    *Value = (CSHORT)value;

    return TRUE;
}

/**
 * Converts a UTCTime (YYMMDDHHMMSSZ) or GeneralizedTime (YYYYMMDDHHMMSSZ) into a
 * localized date/time string. Returns NULL when the value cannot be decoded.
 */
PPH_STRING EtDerFormatTime(
    _In_ BYTE Tag,
    _In_reads_bytes_(Length) PBYTE Data,
    _In_ ULONG Length
    )
{
    TIME_FIELDS timeFields;
    LARGE_INTEGER time;
    SYSTEMTIME systemTime;
    CSHORT value;
    ULONG offset;

    memset(&timeFields, 0, sizeof(TIME_FIELDS));

    if (Tag == ET_DER_TAG_UTC_TIME)
    {
        if (!EtDerReadDigits(Data, Length, 0, 2, &value))
            return NULL;

        // RFC 5280: 00-49 is 2000-2049, 50-99 is 1950-1999.
        timeFields.Year = value < 50 ? 2000 + value : 1900 + value;
        offset = 2;
    }
    else if (Tag == ET_DER_TAG_GENERALIZED_TIME)
    {
        if (!EtDerReadDigits(Data, Length, 0, 4, &value))
            return NULL;

        timeFields.Year = value;
        offset = 4;
    }
    else
    {
        return NULL;
    }

    if (!EtDerReadDigits(Data, Length, offset, 2, &timeFields.Month))
        return NULL;
    if (!EtDerReadDigits(Data, Length, offset + 2, 2, &timeFields.Day))
        return NULL;
    if (!EtDerReadDigits(Data, Length, offset + 4, 2, &timeFields.Hour))
        return NULL;
    if (!EtDerReadDigits(Data, Length, offset + 6, 2, &timeFields.Minute))
        return NULL;

    // Seconds are optional in UTCTime.
    EtDerReadDigits(Data, Length, offset + 8, 2, &timeFields.Second);

    if (!RtlTimeFieldsToTime(&timeFields, &time))
        return NULL;

    PhLargeIntegerToLocalSystemTime(&systemTime, &time);

    return PhFormatDateTime(&systemTime);
}

PPH_STRING EtFwCertHashThumbprint(
    _In_reads_bytes_(Length) PBYTE Data,
    _In_ ULONG Length
    )
{
    PH_HASH_CONTEXT hashContext;
    BYTE hash[PH_HASH_SHA1_LENGTH];

    if (!NT_SUCCESS(PhInitializeHash(&hashContext, Sha1HashAlgorithm)))
        return NULL;
    if (!NT_SUCCESS(PhUpdateHash(&hashContext, Data, Length)))
        return NULL;
    if (!NT_SUCCESS(PhFinalHash(&hashContext, hash, sizeof(hash), NULL)))
        return NULL;

    return PhBufferToHexString(hash, sizeof(hash));
}

/**
 * Decodes the certificate fields shown in the list view.
 *
 * Certificate ::= SEQUENCE { tbsCertificate, signatureAlgorithm, signatureValue }
 * TBSCertificate ::= SEQUENCE { [0] version, serialNumber, signature, issuer,
 *                               validity, subject, ... }
 *
 * \return TRUE when the certificate parsed far enough to show the subject.
 */
BOOLEAN EtFwCertParseCertificate(
    _In_ PET_FWCERT_ENTRY Entry
    )
{
    PBYTE position = Entry->Data;
    PBYTE end = Entry->Data + Entry->DataLength;
    PBYTE certificate;
    ULONG certificateLength;
    PBYTE tbs;
    ULONG tbsLength;
    PBYTE tbsPosition;
    PBYTE tbsEnd;
    PBYTE content;
    ULONG contentLength;
    PBYTE validity;
    ULONG validityLength;
    PBYTE validityPosition;
    PBYTE validityEnd;
    BYTE tag;

    if (!EtDerReadTag(&position, end, ET_DER_TAG_SEQUENCE, &certificate, &certificateLength))
        return FALSE;

    position = certificate;
    end = certificate + certificateLength;

    if (!EtDerReadTag(&position, end, ET_DER_TAG_SEQUENCE, &tbs, &tbsLength))
        return FALSE;

    tbsPosition = tbs;
    tbsEnd = tbs + tbsLength;

    if (!EtDerRead(&tbsPosition, tbsEnd, &tag, &content, &contentLength))
        return FALSE;

    // [0] EXPLICIT version, absent for v1 certificates.
    if (tag == 0xa0)
    {
        if (!EtDerRead(&tbsPosition, tbsEnd, &tag, &content, &contentLength))
            return FALSE;
    }

    if (tag != ET_DER_TAG_INTEGER)
        return FALSE;

    if (contentLength != 0)
        Entry->Serial = PhBufferToHexString(content, contentLength);

    // signature AlgorithmIdentifier
    if (!EtDerReadTag(&tbsPosition, tbsEnd, ET_DER_TAG_SEQUENCE, &content, &contentLength))
        return FALSE;

    // issuer Name
    if (!EtDerReadTag(&tbsPosition, tbsEnd, ET_DER_TAG_SEQUENCE, &content, &contentLength))
        return FALSE;

    Entry->Issuer = EtDerFormatName(content, contentLength);

    // validity SEQUENCE { notBefore, notAfter }
    if (!EtDerReadTag(&tbsPosition, tbsEnd, ET_DER_TAG_SEQUENCE, &validity, &validityLength))
        return FALSE;

    validityPosition = validity;
    validityEnd = validity + validityLength;

    if (EtDerRead(&validityPosition, validityEnd, &tag, &content, &contentLength))
        Entry->NotBefore = EtDerFormatTime(tag, content, contentLength);
    if (EtDerRead(&validityPosition, validityEnd, &tag, &content, &contentLength))
        Entry->NotAfter = EtDerFormatTime(tag, content, contentLength);

    // subject Name
    if (!EtDerReadTag(&tbsPosition, tbsEnd, ET_DER_TAG_SEQUENCE, &content, &contentLength))
        return FALSE;

    Entry->Subject = EtDerFormatName(content, contentLength);
    Entry->Thumbprint = EtFwCertHashThumbprint(Entry->Data, Entry->DataLength);

    return TRUE;
}

PCWSTR EtFwCertSignatureTypeToString(
    _In_ PCGUID SignatureType
    )
{
    if (IsEqualGUID(SignatureType, &EtEfiCertX509Guid)) return L"X509";
    if (IsEqualGUID(SignatureType, &EtEfiCertSha1Guid)) return L"SHA1";
    if (IsEqualGUID(SignatureType, &EtEfiCertSha224Guid)) return L"SHA224";
    if (IsEqualGUID(SignatureType, &EtEfiCertSha256Guid)) return L"SHA256";
    if (IsEqualGUID(SignatureType, &EtEfiCertSha384Guid)) return L"SHA384";
    if (IsEqualGUID(SignatureType, &EtEfiCertSha512Guid)) return L"SHA512";
    if (IsEqualGUID(SignatureType, &EtEfiCertRsa2048Guid)) return L"RSA2048";
    if (IsEqualGUID(SignatureType, &EtEfiCertRsa2048Sha1Guid)) return L"RSA2048-SHA1";
    if (IsEqualGUID(SignatureType, &EtEfiCertRsa2048Sha256Guid)) return L"RSA2048-SHA256";
    if (IsEqualGUID(SignatureType, &EtEfiCertX509Sha256Guid)) return L"X509-SHA256";
    if (IsEqualGUID(SignatureType, &EtEfiCertX509Sha384Guid)) return L"X509-SHA384";
    if (IsEqualGUID(SignatureType, &EtEfiCertX509Sha512Guid)) return L"X509-SHA512";
    if (IsEqualGUID(SignatureType, &EtEfiCertTypePkcs7Guid)) return L"PKCS7";

    return NULL;
}

VOID EtFwCertFreeEntry(
    _In_ PET_FWCERT_ENTRY Entry
    )
{
    PhClearReference(&Entry->Subject);
    PhClearReference(&Entry->Issuer);
    PhClearReference(&Entry->NotBefore);
    PhClearReference(&Entry->NotAfter);
    PhClearReference(&Entry->Serial);
    PhClearReference(&Entry->Thumbprint);
    PhClearReference(&Entry->OwnerGuidString);
    PhFree(Entry);
}

VOID EtFwCertClearEntries(
    _In_ PET_FWCERT_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->Entries->Count; i++)
        EtFwCertFreeEntry(Context->Entries->Items[i]);

    PhClearList(Context->Entries);

    for (ULONG i = 0; i < Context->Buffers->Count; i++)
        PhFree(Context->Buffers->Items[i]);

    PhClearList(Context->Buffers);
}

/**
 * Walks the EFI_SIGNATURE_LIST records of one variable and creates an entry for
 * every EFI_SIGNATURE_DATA. The entries reference \a Buffer, which the caller
 * keeps alive for the lifetime of the list.
 *
 * \return The number of entries appended.
 */
ULONG EtFwCertParseVariable(
    _In_ PET_FWCERT_CONTEXT Context,
    _In_ CONST ET_FWCERT_VARIABLE* Variable,
    _In_reads_bytes_(Length) PBYTE Buffer,
    _In_ ULONG Length
    )
{
    PBYTE position = Buffer;
    PBYTE end = Buffer + Length;
    ULONG count = 0;

    while ((ULONG_PTR)(end - position) >= sizeof(ET_EFI_SIGNATURE_LIST))
    {
        PET_EFI_SIGNATURE_LIST list = (PET_EFI_SIGNATURE_LIST)position;
        ULONG listSize = list->SignatureListSize;
        ULONG headerSize = list->SignatureHeaderSize;
        ULONG signatureSize = list->SignatureSize;
        PBYTE signature;
        ULONG remaining;

        if (listSize < sizeof(ET_EFI_SIGNATURE_LIST))
            break;
        if (listSize > (ULONG)(end - position))
            break;
        if (headerSize > listSize - sizeof(ET_EFI_SIGNATURE_LIST))
            break;

        remaining = listSize - sizeof(ET_EFI_SIGNATURE_LIST) - headerSize;
        signature = position + sizeof(ET_EFI_SIGNATURE_LIST) + headerSize;

        // Every signature must carry an owner GUID and at least one payload byte.
        if (signatureSize <= sizeof(GUID) || signatureSize > remaining)
        {
            position += listSize;
            continue;
        }

        while (remaining >= signatureSize)
        {
            PET_EFI_SIGNATURE_DATA data = (PET_EFI_SIGNATURE_DATA)signature;
            PET_FWCERT_ENTRY entry;

            if (count >= ET_FWCERT_MAX_ENTRIES)
                return count;

            entry = PhAllocateZero(sizeof(ET_FWCERT_ENTRY));
            entry->VariableName = Variable->Name;
            entry->GroupId = Variable->GroupId;
            entry->SignatureType = list->SignatureType;
            entry->SignatureOwner = data->SignatureOwner;
            entry->Data = data->SignatureData;
            entry->DataLength = signatureSize - sizeof(GUID);
            entry->OwnerGuidString = PhFormatGuid(&data->SignatureOwner);

            if (IsEqualGUID(&list->SignatureType, &EtEfiCertX509Guid))
                entry->IsCertificate = EtFwCertParseCertificate(entry);

            if (!entry->Subject)
            {
                // Hash and key blobs, and certificates that failed to decode.
                entry->Subject = PhBufferToHexString(entry->Data, entry->DataLength);
            }

            PhAddItemList(Context->Entries, entry);
            count++;

            signature += signatureSize;
            remaining -= signatureSize;
        }

        position += listSize;
    }

    return count;
}

VOID EtFwCertAddEntriesToListView(
    _In_ PET_FWCERT_CONTEXT Context,
    _In_ ULONG FirstIndex
    )
{
    for (ULONG i = FirstIndex; i < Context->Entries->Count; i++)
    {
        PET_FWCERT_ENTRY entry = Context->Entries->Items[i];
        PCWSTR typeName;
        LONG index;

        index = PhAddListViewGroupItem(
            Context->ListViewHandle,
            entry->GroupId,
            MAXINT,
            PhGetStringOrEmpty(entry->Subject),
            entry
            );

        PhSetListViewSubItem(Context->ListViewHandle, index, 1, PhGetStringOrEmpty(entry->Issuer));

        typeName = EtFwCertSignatureTypeToString(&entry->SignatureType);

        if (typeName)
            PhSetListViewSubItem(Context->ListViewHandle, index, 2, typeName);
        else
            PhSetListViewSubItem(Context->ListViewHandle, index, 2, PH_AUTO_T(PH_STRING, PhFormatGuid(&entry->SignatureType))->Buffer);

        PhSetListViewSubItem(Context->ListViewHandle, index, 3, PhGetStringOrEmpty(entry->OwnerGuidString));
        PhSetListViewSubItem(Context->ListViewHandle, index, 4, PhGetStringOrEmpty(entry->NotBefore));
        PhSetListViewSubItem(Context->ListViewHandle, index, 5, PhGetStringOrEmpty(entry->NotAfter));
        PhSetListViewSubItem(Context->ListViewHandle, index, 6, PhGetStringOrEmpty(entry->Thumbprint));
        PhSetListViewSubItem(Context->ListViewHandle, index, 7, PH_AUTO_T(PH_STRING, PhFormatSize(entry->DataLength, ULONG_MAX))->Buffer);
    }
}

VOID EtFwCertEnumerate(
    _In_ PET_FWCERT_CONTEXT Context
    )
{
    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);

    ListView_DeleteAllItems(Context->ListViewHandle);
    ListView_RemoveAllGroups(Context->ListViewHandle);
    EtFwCertClearEntries(Context);

    for (ULONG i = 0; i < RTL_NUMBER_OF(EtFwCertVariables); i++)
    {
        CONST ET_FWCERT_VARIABLE* variable = &EtFwCertVariables[i];
        PH_STRINGREF variableName;
        PH_STRINGREF vendorGuid;
        NTSTATUS status;
        PVOID buffer;
        ULONG length;
        ULONG count = 0;
        ULONG firstIndex;
        PPH_STRING groupText;

        PhInitializeStringRef(&variableName, variable->Name);
        PhInitializeStringRef(&vendorGuid, variable->VendorGuid);

        firstIndex = Context->Entries->Count;

        status = PhGetFirmwareEnvironmentVariable(
            &variableName,
            &vendorGuid,
            &buffer,
            &length,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            PhAddItemList(Context->Buffers, buffer);
            count = EtFwCertParseVariable(Context, variable, buffer, length);

            groupText = PhFormatString(L"%s (%lu entries)", variable->Name, count);
        }
        else
        {
            groupText = PhFormatString(L"%s (not present)", variable->Name);
        }

        PhAddListViewGroup(Context->ListViewHandle, variable->GroupId, PhGetString(groupText));
        PhDereferenceObject(groupText);

        if (count != 0)
            EtFwCertAddEntriesToListView(Context, firstIndex);
    }

    PhLoadListViewGroupStatesFromSetting(SETTING_NAME_FIRMWARE_CERT_LISTVIEW_GROUPS, Context->ListViewHandle);

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

PPH_STRING EtFwCertFormatDetails(
    _In_ PET_FWCERT_ENTRY Entry
    )
{
    PH_STRING_BUILDER stringBuilder;
    PCWSTR typeName;

    PhInitializeStringBuilder(&stringBuilder, 0x200);

    PhAppendFormatStringBuilder(&stringBuilder, L"Variable: %s\r\n", Entry->VariableName);

    typeName = EtFwCertSignatureTypeToString(&Entry->SignatureType);
    PhAppendFormatStringBuilder(
        &stringBuilder,
        L"Signature type: %s %s\r\n",
        typeName ? typeName : L"Unknown",
        PH_AUTO_T(PH_STRING, PhFormatGuid(&Entry->SignatureType))->Buffer
        );
    PhAppendFormatStringBuilder(&stringBuilder, L"Signature owner: %s\r\n", PhGetStringOrEmpty(Entry->OwnerGuidString));
    PhAppendFormatStringBuilder(&stringBuilder, L"Data length: %lu bytes\r\n", Entry->DataLength);

    return PhFinalStringBuilderString(&stringBuilder);
}

INT_PTR CALLBACK EtFwCertGeneralDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PET_FWCERT_ENTRY entry = (PET_FWCERT_ENTRY)GetWindowLongPtr(WindowHandle, GWLP_USERDATA);

    if (WindowMessage == WM_INITDIALOG)
    {
        HWND listViewHandle;
        PCWSTR typeName;
        LONG index = 0;

        entry = (PET_FWCERT_ENTRY)((PROPSHEETPAGE*)lParam)->lParam;
        SetWindowLongPtr(WindowHandle, GWLP_USERDATA, (LONG_PTR)entry);

        listViewHandle = GetDlgItem(WindowHandle, IDC_FWCERT_PROP_LIST);
        PhSetListViewStyle(listViewHandle, TRUE, TRUE);
        PhSetControlTheme(listViewHandle, L"explorer");
        PhAddListViewColumn(listViewHandle, 0, 0, 0, LVCFMT_LEFT, 110, L"Property");
        PhAddListViewColumn(listViewHandle, 1, 1, 1, LVCFMT_LEFT, 185, L"Value");
        PhSetExtendedListView(listViewHandle);

        typeName = EtFwCertSignatureTypeToString(&entry->SignatureType);

        PhAddListViewItem(listViewHandle, MAXINT, L"Variable", NULL);
        PhSetListViewSubItem(listViewHandle, index++, 1, entry->VariableName);
        PhAddListViewItem(listViewHandle, MAXINT, L"Signature type", NULL);
        PhSetListViewSubItem(listViewHandle, index++, 1, typeName ? typeName : PH_AUTO_T(PH_STRING, PhFormatGuid(&entry->SignatureType))->Buffer);
        PhAddListViewItem(listViewHandle, MAXINT, L"Signature owner", NULL);
        PhSetListViewSubItem(listViewHandle, index++, 1, PhGetStringOrEmpty(entry->OwnerGuidString));
        PhAddListViewItem(listViewHandle, MAXINT, entry->IsCertificate ? L"Subject" : L"Value", NULL);
        PhSetListViewSubItem(listViewHandle, index++, 1, PhGetStringOrEmpty(entry->Subject));

        if (entry->IsCertificate)
        {
            PhAddListViewItem(listViewHandle, MAXINT, L"Issuer", NULL);
            PhSetListViewSubItem(listViewHandle, index++, 1, PhGetStringOrEmpty(entry->Issuer));
            PhAddListViewItem(listViewHandle, MAXINT, L"Valid from", NULL);
            PhSetListViewSubItem(listViewHandle, index++, 1, PhGetStringOrEmpty(entry->NotBefore));
            PhAddListViewItem(listViewHandle, MAXINT, L"Valid to", NULL);
            PhSetListViewSubItem(listViewHandle, index++, 1, PhGetStringOrEmpty(entry->NotAfter));
            PhAddListViewItem(listViewHandle, MAXINT, L"Serial number", NULL);
            PhSetListViewSubItem(listViewHandle, index++, 1, PhGetStringOrEmpty(entry->Serial));
            PhAddListViewItem(listViewHandle, MAXINT, L"Thumbprint (SHA1)", NULL);
            PhSetListViewSubItem(listViewHandle, index++, 1, PhGetStringOrEmpty(entry->Thumbprint));
        }

        PhAddListViewItem(listViewHandle, MAXINT, L"Data length", NULL);
        PhSetListViewSubItem(listViewHandle, index++, 1, PH_AUTO_T(PH_STRING, PhFormatSize(entry->DataLength, ULONG_MAX))->Buffer);

        return TRUE;
    }

    return FALSE;
}

INT_PTR CALLBACK EtFwCertRawDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PET_FWCERT_ENTRY entry = (PET_FWCERT_ENTRY)GetWindowLongPtr(WindowHandle, GWLP_USERDATA);

    if (WindowMessage == WM_INITDIALOG)
    {
        PH_STRING_BUILDER stringBuilder;
        PPH_STRING details;
        PPH_STRING hexString;
        PPH_STRING text;

        entry = (PET_FWCERT_ENTRY)((PROPSHEETPAGE*)lParam)->lParam;
        SetWindowLongPtr(WindowHandle, GWLP_USERDATA, (LONG_PTR)entry);

        PhInitializeStringBuilder(&stringBuilder, 0x400);

        details = EtFwCertFormatDetails(entry);
        PhAppendStringBuilder(&stringBuilder, &details->sr);
        PhDereferenceObject(details);

        PhAppendStringBuilder2(&stringBuilder, L"\r\nRaw signature data:\r\n");

        if (entry->DataLength != 0)
        {
            hexString = PhBufferToHexString(entry->Data, entry->DataLength);
            PhAppendStringBuilder(&stringBuilder, &hexString->sr);
            PhDereferenceObject(hexString);
        }

        text = PhFinalStringBuilderString(&stringBuilder);
        SetDlgItemText(WindowHandle, IDC_FWCERT_PROP_TEXT, text->Buffer);
        PhDereferenceObject(text);

        return TRUE;
    }

    return FALSE;
}

VOID EtFwCertShowEntryProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PET_FWCERT_ENTRY Entry
    )
{
    PROPSHEETPAGE pages[2] = { 0 };
    HPROPSHEETPAGE handles[2];
    PROPSHEETHEADER header = { sizeof(header) };

    for (ULONG i = 0; i < RTL_NUMBER_OF(pages); i++)
    {
        pages[i].dwSize = sizeof(PROPSHEETPAGE);
        pages[i].hInstance = PluginInstance->DllBase;
        pages[i].lParam = (LPARAM)Entry;
    }

    pages[0].pszTemplate = MAKEINTRESOURCE(IDD_FWCERT_GENERAL);
    pages[0].pfnDlgProc = EtFwCertGeneralDlgProc;
    pages[1].pszTemplate = MAKEINTRESOURCE(IDD_FWCERT_RAW);
    pages[1].pfnDlgProc = EtFwCertRawDlgProc;

    handles[0] = CreatePropertySheetPage(&pages[0]);
    handles[1] = CreatePropertySheetPage(&pages[1]);

    header.dwFlags = PSH_PROPTITLE | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
    header.hwndParent = ParentWindowHandle;
    header.hInstance = PluginInstance->DllBase;
    header.pszCaption = L"Signature entry";
    header.nPages = RTL_NUMBER_OF(handles);
    header.phpage = handles;

    PhModalPropertySheet(&header);
}

INT_PTR CALLBACK EtFirmwareCertDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PET_FWCERT_CONTEXT context = NULL;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(ET_FWCERT_CONTEXT));
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = WindowHandle;
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_FWCERT_LIST);
            context->ParentWindowHandle = (HWND)lParam;
            context->Entries = PhCreateList(64);
            context->Buffers = PhCreateList(RTL_NUMBER_OF(EtFwCertVariables));

            PhSetApplicationWindowIcon(WindowHandle);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 280, L"Subject");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 240, L"Issuer");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 90, L"Type");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 240, L"Owner");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 120, L"Valid from");
            PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 120, L"Valid to");
            PhAddListViewColumn(context->ListViewHandle, 6, 6, 6, LVCFMT_LEFT, 240, L"Thumbprint");
            PhAddListViewColumn(context->ListViewHandle, 7, 7, 7, LVCFMT_LEFT, 60, L"Size");
            PhSetExtendedListView(context->ListViewHandle);
            ListView_EnableGroupView(context->ListViewHandle, TRUE);

            PhLoadListViewColumnsFromSetting(SETTING_NAME_FIRMWARE_CERT_LISTVIEW_COLUMNS, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_FWCERT_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhValidWindowPlacementFromSetting(SETTING_NAME_FIRMWARE_CERT_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_FIRMWARE_CERT_WINDOW_POSITION, SETTING_NAME_FIRMWARE_CERT_WINDOW_SIZE, WindowHandle);
            else
                PhCenterWindow(WindowHandle, context->ParentWindowHandle);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));

            EtFwCertEnumerate(context);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewGroupStatesToSetting(SETTING_NAME_FIRMWARE_CERT_LISTVIEW_GROUPS, context->ListViewHandle);
            PhSaveListViewColumnsToSetting(SETTING_NAME_FIRMWARE_CERT_LISTVIEW_COLUMNS, context->ListViewHandle);
            PhSaveWindowPlacementToSetting(SETTING_NAME_FIRMWARE_CERT_WINDOW_POSITION, SETTING_NAME_FIRMWARE_CERT_WINDOW_SIZE, WindowHandle);

            EtFwCertClearEntries(context);
            PhDereferenceObject(context->Entries);
            PhDereferenceObject(context->Buffers);

            PhDeleteLayoutManager(&context->LayoutManager);
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_FWCERT_REFRESH:
                EtFwCertEnumerate(context);
                break;
            case IDCANCEL:
            case IDOK:
                EndDialog(WindowHandle, IDOK);
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);

            switch (hdr->code)
            {
            case NM_DBLCLK:
                {
                    if (hdr->hwndFrom == context->ListViewHandle)
                    {
                        PET_FWCERT_ENTRY entry;

                        if (entry = PhGetSelectedListViewItemParam(context->ListViewHandle))
                            EtFwCertShowEntryProperties(context->WindowHandle, entry);
                    }
                }
                break;
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                if (PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems))
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"&Properties", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        WindowHandle,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case 1:
                                EtFwCertShowEntryProperties(context->WindowHandle, (PET_FWCERT_ENTRY)listviewItems[0]);
                                break;
                            case PHAPP_IDC_COPY:
                                PhCopyListView(context->ListViewHandle);
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                    PhFree(listviewItems);
                }
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID EtShowFirmwareCertDialog(
    _In_ HWND ParentWindowHandle
    )
{
    if (!EtFirmwareEnablePrivilege(ParentWindowHandle, TRUE))
        return;

    if (PhIsFirmwareSupported())
    {
        PhDialogBox(
            PluginInstance->DllBase,
            MAKEINTRESOURCE(IDD_FIRMWARE_CERT),
            NULL,
            EtFirmwareCertDlgProc,
            ParentWindowHandle
            );
    }
    else
    {
        PhShowError2(
            ParentWindowHandle,
            L"Unable to query the signature databases.",
            L"%s",
            L"Windows was installed using legacy BIOS."
            );
    }

    EtFirmwareEnablePrivilege(ParentWindowHandle, FALSE);
}
