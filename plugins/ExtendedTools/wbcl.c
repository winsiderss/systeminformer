/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 * Windows Boot Configuration Log (WBCL) / TCG measured boot event log viewer.
 *
 * The log is acquired either live from the firmware via tbs.dll
 * (Tbsi_Get_TCG_Log_Ex / Tbsi_Get_TCG_Log) or by reading one of the per-boot
 * logs under \Windows\Logs\MeasuredBoot. Records are in the TCG PC Client
 * format: the first record is always a legacy SHA1 TCG_PCR_EVENT whose event
 * data may be a "Spec ID Event03" structure indicating that all subsequent
 * records use the crypto-agile TCG_PCR_EVENT2 layout.
 */

#include "exttools.h"

// TCG PC Client event types (TCG PC Client Platform Firmware Profile)

#define ET_EV_PREBOOT_CERT                  0x00000000
#define ET_EV_POST_CODE                     0x00000001
#define ET_EV_UNUSED                        0x00000002
#define ET_EV_NO_ACTION                     0x00000003
#define ET_EV_SEPARATOR                     0x00000004
#define ET_EV_ACTION                        0x00000005
#define ET_EV_EVENT_TAG                     0x00000006
#define ET_EV_S_CRTM_CONTENTS               0x00000007
#define ET_EV_S_CRTM_VERSION                0x00000008
#define ET_EV_CPU_MICROCODE                 0x00000009
#define ET_EV_PLATFORM_CONFIG_FLAGS         0x0000000A
#define ET_EV_TABLE_OF_DEVICES              0x0000000B
#define ET_EV_COMPACT_HASH                  0x0000000C
#define ET_EV_IPL                           0x0000000D
#define ET_EV_IPL_PARTITION_DATA            0x0000000E
#define ET_EV_NONHOST_CODE                  0x0000000F
#define ET_EV_NONHOST_CONFIG                0x00000010
#define ET_EV_NONHOST_INFO                  0x00000011
#define ET_EV_OMIT_BOOT_DEVICE_EVENTS       0x00000012
#define ET_EV_EFI_EVENT_BASE                0x80000000
#define ET_EV_EFI_VARIABLE_DRIVER_CONFIG    0x80000001
#define ET_EV_EFI_VARIABLE_BOOT             0x80000002
#define ET_EV_EFI_BOOT_SERVICES_APPLICATION 0x80000003
#define ET_EV_EFI_BOOT_SERVICES_DRIVER      0x80000004
#define ET_EV_EFI_RUNTIME_SERVICES_DRIVER   0x80000005
#define ET_EV_EFI_GPT_EVENT                 0x80000006
#define ET_EV_EFI_ACTION                    0x80000007
#define ET_EV_EFI_PLATFORM_FIRMWARE_BLOB    0x80000008
#define ET_EV_EFI_HANDOFF_TABLES            0x80000009
#define ET_EV_EFI_PLATFORM_FIRMWARE_BLOB2   0x8000000A
#define ET_EV_EFI_HANDOFF_TABLES2           0x8000000B
#define ET_EV_EFI_VARIABLE_BOOT2            0x8000000C
#define ET_EV_EFI_HCRTM_EVENT               0x80000010
#define ET_EV_EFI_VARIABLE_AUTHORITY        0x800000E0
#define ET_EV_EFI_SPDM_FIRMWARE_BLOB        0x800000E1
#define ET_EV_EFI_SPDM_FIRMWARE_CONFIG      0x800000E2

// TPM hash algorithm identifiers (TPM_ALG_ID)

#define ET_TPM_ALG_SHA1                     0x0004
#define ET_TPM_ALG_SHA256                   0x000B
#define ET_TPM_ALG_SHA384                   0x000C
#define ET_TPM_ALG_SHA512                   0x000D
#define ET_TPM_ALG_SM3_256                  0x0012

#include <pshpack1.h>

typedef struct _ET_TCG_PCR_EVENT
{
    ULONG PcrIndex;
    ULONG EventType;
    BYTE Digest[20];
    ULONG EventSize;
    BYTE Event[1];
} ET_TCG_PCR_EVENT, *PET_TCG_PCR_EVENT;

typedef struct _ET_TCG_EFI_SPECID_ALG
{
    USHORT AlgorithmId;
    USHORT DigestSize;
} ET_TCG_EFI_SPECID_ALG, *PET_TCG_EFI_SPECID_ALG;

typedef struct _ET_TCG_EFI_SPECID_EVENT
{
    BYTE Signature[16]; // "Spec ID Event03\0"
    ULONG PlatformClass;
    BYTE SpecVersionMinor;
    BYTE SpecVersionMajor;
    BYTE SpecErrata;
    BYTE UintnSize;
    ULONG NumberOfAlgorithms;
    ET_TCG_EFI_SPECID_ALG DigestSizes[1];
    // BYTE VendorInfoSize; BYTE VendorInfo[];
} ET_TCG_EFI_SPECID_EVENT, *PET_TCG_EFI_SPECID_EVENT;

typedef struct _ET_TCG_TPMT_HA_HEADER
{
    USHORT AlgorithmId;
    BYTE Digest[1];
} ET_TCG_TPMT_HA_HEADER, *PET_TCG_TPMT_HA_HEADER;

typedef struct _ET_UEFI_VARIABLE_DATA
{
    BYTE VariableName[16]; // EFI_GUID
    ULONG64 UnicodeNameLength;
    ULONG64 VariableDataLength;
    WCHAR UnicodeName[1];
    // BYTE VariableData[];
} ET_UEFI_VARIABLE_DATA, *PET_UEFI_VARIABLE_DATA;

typedef struct _ET_UEFI_IMAGE_LOAD_EVENT
{
    ULONG64 ImageLocationInMemory;
    ULONG64 ImageLengthInMemory;
    ULONG64 ImageLinkTimeAddress;
    ULONG64 LengthOfDevicePath;
    BYTE DevicePath[1];
} ET_UEFI_IMAGE_LOAD_EVENT, *PET_UEFI_IMAGE_LOAD_EVENT;

typedef struct _ET_UEFI_PLATFORM_FIRMWARE_BLOB
{
    ULONG64 BlobBase;
    ULONG64 BlobLength;
} ET_UEFI_PLATFORM_FIRMWARE_BLOB, *PET_UEFI_PLATFORM_FIRMWARE_BLOB;

typedef struct _ET_SIPA_EVENT_HEADER
{
    ULONG EventType;
    ULONG EventSize;
    BYTE EventData[1];
} ET_SIPA_EVENT_HEADER, *PET_SIPA_EVENT_HEADER;

#include <poppack.h>

typedef struct _WBCL_ENTRY
{
    ULONG Index;
    ULONG PcrIndex;
    ULONG EventType;
    PVOID EventData;    // points into the owning LogBuffer
    ULONG EventSize;
    USHORT FirstDigestAlg;
    PVOID FirstDigest;  // points into the owning LogBuffer
    ULONG FirstDigestSize;
    PPH_STRING Details;
} WBCL_ENTRY, *PWBCL_ENTRY;

typedef struct _WBCL_WINDOW_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HWND ParentWindowHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PVOID LogBuffer;
    ULONG LogLength;
    PPH_LIST Entries;
} WBCL_WINDOW_CONTEXT, *PWBCL_WINDOW_CONTEXT;

// tbs.dll dynamic imports (Tbsi_Get_TCG_Log_Ex requires Windows 10+)

typedef ULONG (WINAPI* _Tbsi_Get_TCG_Log)(
    _In_ TBS_HCONTEXT Context,
    _Out_writes_bytes_opt_(*OutputBufLen) PBYTE OutputBuf,
    _Inout_ PULONG OutputBufLen
    );

typedef ULONG (WINAPI* _Tbsi_Get_TCG_Log_Ex)(
    _In_ ULONG LogType,
    _Out_writes_bytes_opt_(*OutputBufLen) PBYTE OutputBuf,
    _Inout_ PULONG OutputBufLen
    );

#define ET_TBS_TCGLOG_SRTM_CURRENT 0

PCWSTR EtWbclEventTypeToString(
    _In_ ULONG EventType
    )
{
    switch (EventType)
    {
    case ET_EV_PREBOOT_CERT: return L"EV_PREBOOT_CERT";
    case ET_EV_POST_CODE: return L"EV_POST_CODE";
    case ET_EV_UNUSED: return L"EV_UNUSED";
    case ET_EV_NO_ACTION: return L"EV_NO_ACTION";
    case ET_EV_SEPARATOR: return L"EV_SEPARATOR";
    case ET_EV_ACTION: return L"EV_ACTION";
    case ET_EV_EVENT_TAG: return L"EV_EVENT_TAG";
    case ET_EV_S_CRTM_CONTENTS: return L"EV_S_CRTM_CONTENTS";
    case ET_EV_S_CRTM_VERSION: return L"EV_S_CRTM_VERSION";
    case ET_EV_CPU_MICROCODE: return L"EV_CPU_MICROCODE";
    case ET_EV_PLATFORM_CONFIG_FLAGS: return L"EV_PLATFORM_CONFIG_FLAGS";
    case ET_EV_TABLE_OF_DEVICES: return L"EV_TABLE_OF_DEVICES";
    case ET_EV_COMPACT_HASH: return L"EV_COMPACT_HASH";
    case ET_EV_IPL: return L"EV_IPL";
    case ET_EV_IPL_PARTITION_DATA: return L"EV_IPL_PARTITION_DATA";
    case ET_EV_NONHOST_CODE: return L"EV_NONHOST_CODE";
    case ET_EV_NONHOST_CONFIG: return L"EV_NONHOST_CONFIG";
    case ET_EV_NONHOST_INFO: return L"EV_NONHOST_INFO";
    case ET_EV_OMIT_BOOT_DEVICE_EVENTS: return L"EV_OMIT_BOOT_DEVICE_EVENTS";
    case ET_EV_EFI_EVENT_BASE: return L"EV_EFI_EVENT_BASE";
    case ET_EV_EFI_VARIABLE_DRIVER_CONFIG: return L"EV_EFI_VARIABLE_DRIVER_CONFIG";
    case ET_EV_EFI_VARIABLE_BOOT: return L"EV_EFI_VARIABLE_BOOT";
    case ET_EV_EFI_BOOT_SERVICES_APPLICATION: return L"EV_EFI_BOOT_SERVICES_APPLICATION";
    case ET_EV_EFI_BOOT_SERVICES_DRIVER: return L"EV_EFI_BOOT_SERVICES_DRIVER";
    case ET_EV_EFI_RUNTIME_SERVICES_DRIVER: return L"EV_EFI_RUNTIME_SERVICES_DRIVER";
    case ET_EV_EFI_GPT_EVENT: return L"EV_EFI_GPT_EVENT";
    case ET_EV_EFI_ACTION: return L"EV_EFI_ACTION";
    case ET_EV_EFI_PLATFORM_FIRMWARE_BLOB: return L"EV_EFI_PLATFORM_FIRMWARE_BLOB";
    case ET_EV_EFI_HANDOFF_TABLES: return L"EV_EFI_HANDOFF_TABLES";
    case ET_EV_EFI_PLATFORM_FIRMWARE_BLOB2: return L"EV_EFI_PLATFORM_FIRMWARE_BLOB2";
    case ET_EV_EFI_HANDOFF_TABLES2: return L"EV_EFI_HANDOFF_TABLES2";
    case ET_EV_EFI_VARIABLE_BOOT2: return L"EV_EFI_VARIABLE_BOOT2";
    case ET_EV_EFI_HCRTM_EVENT: return L"EV_EFI_HCRTM_EVENT";
    case ET_EV_EFI_VARIABLE_AUTHORITY: return L"EV_EFI_VARIABLE_AUTHORITY";
    case ET_EV_EFI_SPDM_FIRMWARE_BLOB: return L"EV_EFI_SPDM_FIRMWARE_BLOB";
    case ET_EV_EFI_SPDM_FIRMWARE_CONFIG: return L"EV_EFI_SPDM_FIRMWARE_CONFIG";
    default: return NULL;
    }
}

PCWSTR EtWbclAlgorithmToString(
    _In_ USHORT AlgorithmId
    )
{
    switch (AlgorithmId)
    {
    case ET_TPM_ALG_SHA1: return L"SHA1";
    case ET_TPM_ALG_SHA256: return L"SHA256";
    case ET_TPM_ALG_SHA384: return L"SHA384";
    case ET_TPM_ALG_SHA512: return L"SHA512";
    case ET_TPM_ALG_SM3_256: return L"SM3-256";
    default: return L"?";
    }
}

USHORT EtWbclAlgorithmDigestSize(
    _In_ USHORT AlgorithmId
    )
{
    switch (AlgorithmId)
    {
    case ET_TPM_ALG_SHA1: return 20;
    case ET_TPM_ALG_SHA256: return 32;
    case ET_TPM_ALG_SHA384: return 48;
    case ET_TPM_ALG_SHA512: return 64;
    case ET_TPM_ALG_SM3_256: return 32;
    default: return 0;
    }
}

PCWSTR EtWbclSipaTypeToString(
    _In_ ULONG SipaType
    )
{
    switch (SipaType)
    {
    case 0x00010000: return L"TrustBoundary";
    case 0x00010001: return L"ELAM Aggregation";
    case 0x00010002: return L"LoadedModule Aggregation";
    case 0x00010003: return L"TrustPoint Aggregation";
    case 0x00010004: return L"KSR Aggregation";
    case 0x00010005: return L"KSR Signed Measurement Aggregation";
    case 0x00020001: return L"Information";
    case 0x00020002: return L"BootCounter";
    case 0x00020003: return L"TransferControl";
    case 0x00020004: return L"ApplicationReturn";
    case 0x00020005: return L"BitlockerUnlock";
    case 0x00020006: return L"EventCounter";
    case 0x00020007: return L"CounterId";
    case 0x00020008: return L"MorBitNotCancelable";
    case 0x00020009: return L"ApplicationSvn";
    case 0x0002000A: return L"CrtmVersion";
    case 0x0002000B: return L"MorBitApiStatus";
    case 0x00030001: return L"BootDebugging";
    case 0x00030002: return L"BootRevocationList";
    case 0x00040001: return L"OsKernelDebug";
    case 0x00040002: return L"CodeIntegrity";
    case 0x00040003: return L"TestSigning";
    case 0x00040004: return L"DataExecutionPrevention";
    case 0x00040005: return L"SafeMode";
    case 0x00040006: return L"WinPE";
    case 0x00040007: return L"PhysicalAddressExtension";
    case 0x00040008: return L"OsDevice";
    case 0x00040009: return L"SystemRoot";
    case 0x0004000A: return L"HypervisorLaunchType";
    case 0x0004000B: return L"HypervisorPath";
    case 0x0004000C: return L"HypervisorIommuPolicy";
    case 0x0004000D: return L"HypervisorDebug";
    case 0x0004000E: return L"DriverLoadPolicy";
    case 0x0004000F: return L"SiPolicy";
    case 0x00040010: return L"HypervisorMmioNxPolicy";
    case 0x00040011: return L"HypervisorMsrFilterPolicy";
    case 0x00040012: return L"VsmLaunchType";
    case 0x00040013: return L"OsRevocationList";
    case 0x00040020: return L"VbsVsmRequired";
    case 0x00040021: return L"VbsSecurebootRequired";
    case 0x00040022: return L"VbsIommuRequired";
    case 0x00040023: return L"VbsNxRequired";
    case 0x00040024: return L"VbsKernelDebugAllowed";
    case 0x00040025: return L"VbsDumpUsesAmePolicy";
    case 0x00040026: return L"VbsVsmMandatoryEnforcement";
    case 0x00040027: return L"VbsHvciPolicy";
    case 0x00040028: return L"VbsMicrosoftBootChainRequired";
    case 0x00050001: return L"ImageLoad (NoAuthority)";
    case 0x00050002: return L"ImageBase (Authority)";
    case 0x00050003: return L"ImageSize";
    case 0x00050004: return L"AuthorityIssuer";
    case 0x00050005: return L"AuthoritySerial";
    case 0x00050006: return L"ImageHash";
    case 0x00050007: return L"AuthorityPublisher";
    case 0x00050008: return L"AuthoritySha1Thumbprint";
    case 0x00050009: return L"ImageValidated";
    case 0x0005000A: return L"ModuleSvn";
    case 0x00060001: return L"Quote";
    case 0x00060002: return L"QuoteSignature";
    case 0x00060003: return L"AikId";
    case 0x00060004: return L"AikPubDigest";
    case 0x00070001: return L"ELAM Keyname";
    case 0x00070002: return L"ELAM Configuration";
    case 0x00070003: return L"ELAM Policy";
    case 0x00070004: return L"ELAM Measured";
    default: return NULL;
    }
}

// Renders a leaf blob as a UTF-16 string (quoted) if it looks like printable
// text, otherwise as a fixed-width integer, otherwise as a hex dump.
PPH_STRING EtWbclFormatLeaf(
    _In_reads_bytes_(Size) PVOID Data,
    _In_ ULONG Size
    )
{
    if (Size == 0)
        return PhCreateString(L"(empty)");

    if (Size == 1)
    {
        return PhFormatString(L"%u (0x%02x)", *(PBYTE)Data, *(PBYTE)Data);
    }

    if (Size == 4)
    {
        ULONG value = *(PULONG UNALIGNED)Data;
        return PhFormatString(L"%lu (0x%08lx)", value, value);
    }

    if (Size == 8)
    {
        ULONG64 value = *(PULONG64 UNALIGNED)Data;
        return PhFormatString(L"%I64u (0x%016I64x)", value, value);
    }

    // Detect a UTF-16 string: even length and predominantly printable.
    if ((Size % sizeof(WCHAR)) == 0)
    {
        PWCH chars = (PWCH)Data;
        ULONG count = Size / sizeof(WCHAR);
        ULONG printable = 0;
        ULONG length = count;

        for (ULONG i = 0; i < count; i++)
        {
            if (chars[i] == UNICODE_NULL)
            {
                length = i;
                break;
            }

            if (chars[i] >= L' ' && chars[i] < 0x7f)
                printable++;
            else if (chars[i] == L'\t' || chars[i] == L'\\' || chars[i] == L'/' || chars[i] == L':')
                printable++;
        }

        if (length != 0 && printable >= (length - (length / 8)))
        {
            PPH_STRING text = PhCreateStringEx(chars, length * sizeof(WCHAR));
            PPH_STRING result = PhFormatString(L"\"%s\"", text->Buffer);
            PhDereferenceObject(text);
            return result;
        }
    }

    return PhBufferToHexString((PUCHAR)Data, Size);
}

// Walks the Microsoft SIPA TLV stream carried inside EV_EVENT_TAG events.
VOID EtWbclAppendSipaEvents(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_reads_bytes_(Size) PVOID Data,
    _In_ ULONG Size,
    _In_ ULONG Depth
    )
{
    PBYTE position = Data;
    PBYTE end = position + Size;

    while ((position + RTL_SIZEOF_THROUGH_FIELD(ET_SIPA_EVENT_HEADER, EventSize)) <= end)
    {
        PET_SIPA_EVENT_HEADER header = (PET_SIPA_EVENT_HEADER)position;
        ULONG sipaType = header->EventType;
        ULONG sipaSize = header->EventSize;
        PBYTE sipaData = header->EventData;
        PCWSTR name;

        if (sipaData + sipaSize > end)
            break;

        for (ULONG i = 0; i < Depth; i++)
            PhAppendStringBuilder2(StringBuilder, L"    ");

        name = EtWbclSipaTypeToString(sipaType);

        if (name)
            PhAppendFormatStringBuilder(StringBuilder, L"%s (0x%08lx)", name, sipaType);
        else
            PhAppendFormatStringBuilder(StringBuilder, L"0x%08lx", sipaType);

        // Aggregation / container types (high word 0x0001) wrap nested events.
        if ((sipaType & 0xffff0000) == 0x00010000 && sipaSize != 0)
        {
            PhAppendStringBuilder2(StringBuilder, L":\r\n");
            EtWbclAppendSipaEvents(StringBuilder, sipaData, sipaSize, Depth + 1);
        }
        else
        {
            PPH_STRING value = EtWbclFormatLeaf(sipaData, sipaSize);
            PhAppendFormatStringBuilder(StringBuilder, L" = %s\r\n", value->Buffer);
            PhDereferenceObject(value);
        }

        position = sipaData + sipaSize;
    }
}

// Produces the human-readable "Details" text for a single record's event data.
PPH_STRING EtWbclFormatEventData(
    _In_ ULONG EventType,
    _In_reads_bytes_(EventSize) PVOID Event,
    _In_ ULONG EventSize
    )
{
    if (EventSize == 0)
        return PhCreateString(L"(no data)");

    switch (EventType)
    {
    case ET_EV_EFI_VARIABLE_DRIVER_CONFIG:
    case ET_EV_EFI_VARIABLE_BOOT:
    case ET_EV_EFI_VARIABLE_BOOT2:
    case ET_EV_EFI_VARIABLE_AUTHORITY:
        {
            PET_UEFI_VARIABLE_DATA variable = Event;

            if (EventSize >= RTL_SIZEOF_THROUGH_FIELD(ET_UEFI_VARIABLE_DATA, VariableDataLength))
            {
                PPH_STRING guidString;
                PPH_STRING nameString;
                PPH_STRING result;
                GUID guid;
                ULONG64 nameLength = variable->UnicodeNameLength;
                ULONG64 nameBytes = nameLength * sizeof(WCHAR);

                memcpy(&guid, variable->VariableName, sizeof(GUID));
                guidString = PhFormatGuid(&guid);

                if (nameBytes <= EventSize - UFIELD_OFFSET(ET_UEFI_VARIABLE_DATA, UnicodeName))
                    nameString = PhCreateStringEx(variable->UnicodeName, (SIZE_T)nameBytes);
                else
                    nameString = PhCreateString(L"?");

                result = PhFormatString(
                    L"Variable: %s\r\nGUID: %s\r\nData length: %I64u",
                    nameString->Buffer,
                    guidString->Buffer,
                    variable->VariableDataLength
                    );

                PhDereferenceObject(nameString);
                PhDereferenceObject(guidString);
                return result;
            }
        }
        break;
    case ET_EV_EFI_BOOT_SERVICES_APPLICATION:
    case ET_EV_EFI_BOOT_SERVICES_DRIVER:
    case ET_EV_EFI_RUNTIME_SERVICES_DRIVER:
        {
            PET_UEFI_IMAGE_LOAD_EVENT image = Event;

            if (EventSize >= RTL_SIZEOF_THROUGH_FIELD(ET_UEFI_IMAGE_LOAD_EVENT, LengthOfDevicePath))
            {
                return PhFormatString(
                    L"Image base: 0x%I64x\r\nImage length: 0x%I64x\r\nLink-time address: 0x%I64x\r\nDevice path length: %I64u bytes",
                    image->ImageLocationInMemory,
                    image->ImageLengthInMemory,
                    image->ImageLinkTimeAddress,
                    image->LengthOfDevicePath
                    );
            }
        }
        break;
    case ET_EV_EFI_PLATFORM_FIRMWARE_BLOB:
        {
            PET_UEFI_PLATFORM_FIRMWARE_BLOB blob = Event;

            if (EventSize >= sizeof(ET_UEFI_PLATFORM_FIRMWARE_BLOB))
            {
                return PhFormatString(
                    L"Blob base: 0x%I64x\r\nBlob length: 0x%I64x",
                    blob->BlobBase,
                    blob->BlobLength
                    );
            }
        }
        break;
    case ET_EV_EVENT_TAG:
        {
            PH_STRING_BUILDER stringBuilder;

            PhInitializeStringBuilder(&stringBuilder, 0x100);
            EtWbclAppendSipaEvents(&stringBuilder, Event, EventSize, 0);

            if (stringBuilder.String->Length != 0)
                return PhFinalStringBuilderString(&stringBuilder);

            PhDeleteStringBuilder(&stringBuilder);
        }
        break;
    case ET_EV_POST_CODE:
    case ET_EV_S_CRTM_VERSION:
    case ET_EV_ACTION:
    case ET_EV_EFI_ACTION:
    case ET_EV_NO_ACTION:
        return EtWbclFormatLeaf(Event, EventSize);
    }

    // Default: try a readable rendering, otherwise hex.
    return EtWbclFormatLeaf(Event, EventSize);
}

VOID EtWbclClearEntries(
    _In_ PWBCL_WINDOW_CONTEXT Context
    )
{
    ListView_DeleteAllItems(Context->ListViewHandle);

    if (Context->Entries)
    {
        for (ULONG i = 0; i < Context->Entries->Count; i++)
        {
            PWBCL_ENTRY entry = Context->Entries->Items[i];

            if (entry->Details)
                PhDereferenceObject(entry->Details);

            PhFree(entry);
        }

        PhClearList(Context->Entries);
    }
}

VOID EtWbclFreeLog(
    _In_ PWBCL_WINDOW_CONTEXT Context
    )
{
    EtWbclClearEntries(Context);

    if (Context->LogBuffer)
    {
        PhFree(Context->LogBuffer);
        Context->LogBuffer = NULL;
        Context->LogLength = 0;
    }
}

VOID EtWbclAddEntry(
    _In_ PWBCL_WINDOW_CONTEXT Context,
    _In_ ULONG PcrIndex,
    _In_ ULONG EventType,
    _In_ USHORT DigestAlg,
    _In_reads_bytes_(DigestSize) PVOID Digest,
    _In_ USHORT DigestSize,
    _In_reads_bytes_(EventSize) PVOID Event,
    _In_ ULONG EventSize
    )
{
    PWBCL_ENTRY entry;
    LONG lvItemIndex;
    PPH_STRING string;
    PCWSTR typeName;

    entry = PhAllocateZero(sizeof(WBCL_ENTRY));
    entry->Index = Context->Entries->Count;
    entry->PcrIndex = PcrIndex;
    entry->EventType = EventType;
    entry->EventData = Event;
    entry->EventSize = EventSize;
    entry->FirstDigestAlg = DigestAlg;
    entry->FirstDigest = Digest;
    entry->FirstDigestSize = DigestSize;
    entry->Details = EtWbclFormatEventData(EventType, Event, EventSize);

    PhAddItemList(Context->Entries, entry);

    // # column
    string = PhFormatUInt64(entry->Index, FALSE);
    lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, string->Buffer, entry);
    PhDereferenceObject(string);

    // PCR
    string = PhFormatUInt64(PcrIndex, FALSE);
    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, string->Buffer);
    PhDereferenceObject(string);

    // Event Type
    typeName = EtWbclEventTypeToString(EventType);
    if (typeName)
    {
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, (PWSTR)typeName);
    }
    else
    {
        string = PhFormatString(L"0x%08lx", EventType);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, string->Buffer);
        PhDereferenceObject(string);
    }

    // Digest (alg)
    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, (PWSTR)EtWbclAlgorithmToString(DigestAlg));

    // Digest value
    if (DigestSize != 0)
    {
        string = PhBufferToHexString((PUCHAR)Digest, DigestSize);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 4, string->Buffer);
        PhDereferenceObject(string);
    }

    // Size
    string = PhFormatUInt64(EventSize, TRUE);
    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 5, string->Buffer);
    PhDereferenceObject(string);

    // Details
    if (entry->Details)
    {
        // Collapse newlines for the single-line listview cell.
        PPH_STRING flat = PhCreateStringEx(entry->Details->Buffer, entry->Details->Length);

        for (SIZE_T i = 0; i < flat->Length / sizeof(WCHAR); i++)
        {
            if (flat->Buffer[i] == L'\r' || flat->Buffer[i] == L'\n')
                flat->Buffer[i] = L' ';
        }

        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 6, flat->Buffer);
        PhDereferenceObject(flat);
    }
}

// Parses the in-memory TCG log into listview rows. Returns the number of
// records parsed; bounds-checks every field so truncated on-disk logs fail
// soft rather than crashing.
ULONG EtWbclEnumerate(
    _In_ PWBCL_WINDOW_CONTEXT Context,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    PBYTE position = Buffer;
    PBYTE end = position + Length;
    BOOLEAN cryptoAgile = FALSE;
    ET_TCG_EFI_SPECID_ALG algorithms[8];
    ULONG algorithmCount = 0;
    ULONG records = 0;

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    EtWbclClearEntries(Context);

    // The first record is always a legacy SHA1 TCG_PCR_EVENT.
    if ((position + UFIELD_OFFSET(ET_TCG_PCR_EVENT, Event)) <= end)
    {
        PET_TCG_PCR_EVENT first = (PET_TCG_PCR_EVENT)position;
        ULONG eventSize = first->EventSize;
        PBYTE eventData = first->Event;

        if (eventData + eventSize <= end)
        {
            // Detect the crypto-agile "Spec ID Event03" header.
            if (first->EventType == ET_EV_NO_ACTION &&
                eventSize >= UFIELD_OFFSET(ET_TCG_EFI_SPECID_EVENT, DigestSizes) &&
                memcmp(eventData, "Spec ID Event03", 15) == 0)
            {
                PET_TCG_EFI_SPECID_EVENT specId = (PET_TCG_EFI_SPECID_EVENT)eventData;
                ULONG count = specId->NumberOfAlgorithms;

                if (count <= RTL_NUMBER_OF(algorithms) &&
                    eventSize >= UFIELD_OFFSET(ET_TCG_EFI_SPECID_EVENT, DigestSizes) + count * sizeof(ET_TCG_EFI_SPECID_ALG))
                {
                    cryptoAgile = TRUE;
                    algorithmCount = count;

                    for (ULONG i = 0; i < count; i++)
                        algorithms[i] = specId->DigestSizes[i];
                }
            }

            EtWbclAddEntry(
                Context,
                first->PcrIndex,
                first->EventType,
                ET_TPM_ALG_SHA1,
                first->Digest,
                sizeof(first->Digest),
                eventData,
                eventSize
                );
            records++;

            position = eventData + eventSize;
        }
        else
        {
            position = end; // truncated first record
        }
    }

    while (position < end)
    {
        ULONG pcrIndex;
        ULONG eventType;
        USHORT firstAlg = 0;
        PBYTE firstDigest = NULL;
        USHORT firstDigestSize = 0;
        ULONG eventSize;
        PBYTE eventData;

        if (cryptoAgile)
        {
            // TCG_PCR_EVENT2: header, TPML_DIGEST_VALUES, event.
            PBYTE cursor = position;
            ULONG digestCount;

            if (cursor + sizeof(ULONG) * 2 + sizeof(ULONG) > end)
                break;

            pcrIndex = *(PULONG UNALIGNED)cursor;
            cursor += sizeof(ULONG);
            eventType = *(PULONG UNALIGNED)cursor;
            cursor += sizeof(ULONG);
            digestCount = *(PULONG UNALIGNED)cursor;
            cursor += sizeof(ULONG);

            for (ULONG i = 0; i < digestCount; i++)
            {
                USHORT algId;
                USHORT digestSize;

                if (cursor + sizeof(USHORT) > end)
                {
                    cursor = end;
                    break;
                }

                algId = *(PUSHORT UNALIGNED)cursor;
                cursor += sizeof(USHORT);

                digestSize = 0;
                for (ULONG j = 0; j < algorithmCount; j++)
                {
                    if (algorithms[j].AlgorithmId == algId)
                    {
                        digestSize = algorithms[j].DigestSize;
                        break;
                    }
                }

                if (digestSize == 0)
                    digestSize = EtWbclAlgorithmDigestSize(algId);

                if (digestSize == 0 || cursor + digestSize > end)
                {
                    cursor = end;
                    break;
                }

                if (i == 0)
                {
                    firstAlg = algId;
                    firstDigest = cursor;
                    firstDigestSize = digestSize;
                }

                cursor += digestSize;
            }

            if (cursor + sizeof(ULONG) > end)
                break;

            eventSize = *(PULONG UNALIGNED)cursor;
            cursor += sizeof(ULONG);
            eventData = cursor;

            if (eventData + eventSize > end)
                break;

            position = eventData + eventSize;
        }
        else
        {
            PET_TCG_PCR_EVENT record = (PET_TCG_PCR_EVENT)position;

            if (position + UFIELD_OFFSET(ET_TCG_PCR_EVENT, Event) > end)
                break;

            pcrIndex = record->PcrIndex;
            eventType = record->EventType;
            firstAlg = ET_TPM_ALG_SHA1;
            firstDigest = record->Digest;
            firstDigestSize = sizeof(record->Digest);
            eventSize = record->EventSize;
            eventData = record->Event;

            if (eventData + eventSize > end)
                break;

            position = eventData + eventSize;
        }

        EtWbclAddEntry(
            Context,
            pcrIndex,
            eventType,
            firstAlg,
            firstDigest,
            firstDigestSize,
            eventData,
            eventSize
            );
        records++;
    }

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);

    return records;
}

// Fetches the current measured boot log from the firmware via tbs.dll.
NTSTATUS EtWbclGetLiveLog(
    _Out_ PVOID* Buffer,
    _Out_ PULONG Length
    )
{
    NTSTATUS status = STATUS_NOT_SUPPORTED;
    PVOID tbsModule;
    _Tbsi_Get_TCG_Log_Ex getLogEx = NULL;
    _Tbsi_Get_TCG_Log getLog = NULL;
    PVOID buffer = NULL;
    ULONG length = 0;
    ULONG result;

    *Buffer = NULL;
    *Length = 0;

    if (!(tbsModule = PhLoadLibrary(L"tbs.dll")))
        return STATUS_DLL_NOT_FOUND;

    getLogEx = PhGetProcedureAddress(tbsModule, "Tbsi_Get_TCG_Log_Ex", 0);
    getLog = PhGetProcedureAddress(tbsModule, "Tbsi_Get_TCG_Log", 0);

    if (getLogEx)
    {
        result = getLogEx(ET_TBS_TCGLOG_SRTM_CURRENT, NULL, &length);

        if ((result == TBS_SUCCESS || result == TBS_E_INSUFFICIENT_BUFFER) && length != 0)
        {
            buffer = PhAllocate(length);

            if (getLogEx(ET_TBS_TCGLOG_SRTM_CURRENT, buffer, &length) == TBS_SUCCESS)
            {
                status = STATUS_SUCCESS;
            }
            else
            {
                PhFree(buffer);
                buffer = NULL;
            }
        }
    }

    if (!NT_SUCCESS(status) && getLog)
    {
        TBS_HCONTEXT tbsContextHandle = NULL;

        if (NT_SUCCESS(EtTpmOpen(&tbsContextHandle)))
        {
            length = 0;
            result = getLog(tbsContextHandle, NULL, &length);

            if ((result == TBS_SUCCESS || result == TBS_E_INSUFFICIENT_BUFFER) && length != 0)
            {
                buffer = PhAllocate(length);

                if (getLog(tbsContextHandle, buffer, &length) == TBS_SUCCESS)
                {
                    status = STATUS_SUCCESS;
                }
                else
                {
                    PhFree(buffer);
                    buffer = NULL;
                }
            }

            EtTpmClose(tbsContextHandle);
        }
    }

    PhFreeLibrary(tbsModule);

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
        *Length = length;
    }

    return status;
}

// Reads a \Windows\Logs\MeasuredBoot\*.log file into an allocated buffer.
NTSTATUS EtWbclGetFileLog(
    _In_ PPH_STRING FileName,
    _Out_ PVOID* Buffer,
    _Out_ PULONG Length
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    LARGE_INTEGER fileSize;
    PVOID buffer;
    ULONG length;

    *Buffer = NULL;
    *Length = 0;

    status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(FileName),
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetFileSize(fileHandle, &fileSize);

    if (!NT_SUCCESS(status))
    {
        NtClose(fileHandle);
        return status;
    }

    if (fileSize.QuadPart == 0 || fileSize.QuadPart > 32 * 1024 * 1024)
    {
        NtClose(fileHandle);
        return STATUS_FILE_TOO_LARGE;
    }

    length = (ULONG)fileSize.QuadPart;
    buffer = PhAllocate(length);

    status = PhReadFile(fileHandle, buffer, length, NULL, NULL);

    NtClose(fileHandle);

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
        *Length = length;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

VOID EtWbclLoadLiveLog(
    _In_ PWBCL_WINDOW_CONTEXT Context
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG length;

    status = EtWbclGetLiveLog(&buffer, &length);

    if (NT_SUCCESS(status))
    {
        EtWbclFreeLog(Context);
        Context->LogBuffer = buffer;
        Context->LogLength = length;
        EtWbclEnumerate(Context, buffer, length);
    }
    else
    {
        PhShowStatus(Context->WindowHandle, L"Unable to read the measured boot log", status, 0);
    }
}

VOID EtWbclLoadFileLog(
    _In_ PWBCL_WINDOW_CONTEXT Context
    )
{
    static PH_FILETYPE_FILTER filters[] =
    {
        { L"Measured boot logs (*.log)", L"*.log" },
        { L"All files (*.*)", L"*.*" }
    };
    PVOID fileDialog;

    fileDialog = PhCreateOpenFileDialog();
    PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));
    PhSetFileDialogFileName(fileDialog, L"C:\\Windows\\Logs\\MeasuredBoot\\");

    if (PhShowFileDialog(Context->WindowHandle, fileDialog))
    {
        PPH_STRING fileName = PH_AUTO(PhGetFileDialogFileName(fileDialog));
        NTSTATUS status;
        PVOID buffer;
        ULONG length;

        status = EtWbclGetFileLog(fileName, &buffer, &length);

        if (NT_SUCCESS(status))
        {
            EtWbclFreeLog(Context);
            Context->LogBuffer = buffer;
            Context->LogLength = length;
            EtWbclEnumerate(Context, buffer, length);
        }
        else
        {
            PhShowStatus(Context->WindowHandle, L"Unable to read the log file", status, 0);
        }
    }

    PhFreeFileDialog(fileDialog);
}

VOID EtWbclShowEntryDetails(
    _In_ PWBCL_WINDOW_CONTEXT Context,
    _In_ PWBCL_ENTRY Entry
    )
{
    PH_STRING_BUILDER stringBuilder;
    PCWSTR typeName;
    PPH_STRING hexString;
    PPH_STRING text;

    PhInitializeStringBuilder(&stringBuilder, 0x200);

    typeName = EtWbclEventTypeToString(Entry->EventType);
    PhAppendFormatStringBuilder(&stringBuilder, L"PCR index: %lu\r\n", Entry->PcrIndex);
    PhAppendFormatStringBuilder(&stringBuilder, L"Event type: %s (0x%08lx)\r\n",
        typeName ? typeName : L"Unknown", Entry->EventType);
    PhAppendFormatStringBuilder(&stringBuilder, L"Digest (%s): ", EtWbclAlgorithmToString(Entry->FirstDigestAlg));

    if (Entry->FirstDigestSize != 0)
    {
        hexString = PhBufferToHexString((PUCHAR)Entry->FirstDigest, Entry->FirstDigestSize);
        PhAppendStringBuilder(&stringBuilder, &hexString->sr);
        PhDereferenceObject(hexString);
    }

    PhAppendFormatStringBuilder(&stringBuilder, L"\r\nEvent size: %lu bytes\r\n\r\n", Entry->EventSize);

    if (Entry->Details)
    {
        PhAppendStringBuilder2(&stringBuilder, L"Details:\r\n");
        PhAppendStringBuilder(&stringBuilder, &Entry->Details->sr);
        PhAppendStringBuilder2(&stringBuilder, L"\r\n");
    }

    if (Entry->EventSize != 0)
    {
        PhAppendStringBuilder2(&stringBuilder, L"\r\nRaw event data:\r\n");
        hexString = PhBufferToHexString((PUCHAR)Entry->EventData, Entry->EventSize);
        PhAppendStringBuilder(&stringBuilder, &hexString->sr);
        PhDereferenceObject(hexString);
    }

    text = PhFinalStringBuilderString(&stringBuilder);
    PhShowInformation2(Context->WindowHandle, L"Boot log entry", L"%s", text->Buffer);
    PhDereferenceObject(text);
}

INT_PTR CALLBACK EtWbclDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWBCL_WINDOW_CONTEXT context = NULL;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(WBCL_WINDOW_CONTEXT));
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
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_WBCL_LIST);
            context->ParentWindowHandle = (HWND)lParam;
            context->Entries = PhCreateList(64);

            PhSetApplicationWindowIcon(WindowHandle);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 50, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 40, L"PCR");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 230, L"Event type");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 60, L"Digest");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 220, L"Digest value");
            PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 60, L"Size");
            PhAddListViewColumn(context->ListViewHandle, 6, 6, 6, LVCFMT_LEFT, 400, L"Details");
            PhSetExtendedListView(context->ListViewHandle);

            PhLoadListViewColumnsFromSetting(SETTING_NAME_WBCL_LISTVIEW_COLUMNS, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_WBCL_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_WBCL_OPENFILE), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhValidWindowPlacementFromSetting(SETTING_NAME_WBCL_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_WBCL_WINDOW_POSITION, SETTING_NAME_WBCL_WINDOW_SIZE, WindowHandle);
            else
                PhCenterWindow(WindowHandle, context->ParentWindowHandle);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));

            EtWbclLoadLiveLog(context);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(SETTING_NAME_WBCL_LISTVIEW_COLUMNS, context->ListViewHandle);
            PhSaveWindowPlacementToSetting(SETTING_NAME_WBCL_WINDOW_POSITION, SETTING_NAME_WBCL_WINDOW_SIZE, WindowHandle);

            EtWbclFreeLog(context);

            if (context->Entries)
                PhDereferenceObject(context->Entries);

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
            case IDC_WBCL_REFRESH:
                EtWbclLoadLiveLog(context);
                break;
            case IDC_WBCL_OPENFILE:
                EtWbclLoadFileLog(context);
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
                        PWBCL_ENTRY entry;

                        if (entry = PhGetSelectedListViewItemParam(context->ListViewHandle))
                            EtWbclShowEntryDetails(context, entry);
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
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"&Details", NULL, NULL), ULONG_MAX);
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
                                EtWbclShowEntryDetails(context, (PWBCL_ENTRY)listviewItems[0]);
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

VOID EtShowWbclDialog(
    _In_ HWND ParentWindowHandle
    )
{
    PhDialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_WBCL),
        NULL,
        EtWbclDlgProc,
        ParentWindowHandle
        );
}
