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

//
// The Windows SDK ships wbcl.h ("Definitions and prototypes for parsing the TCG Log") which is the
// authoritative source for the TCG event types (SIPAEV_*), the Microsoft SIPA tag values
// (SIPAEVENT_*) and the payload structures carried by them. Older SDKs do not have it, so the
// definitions this file relies on are reproduced below when the header is unavailable. The
// reproduced block uses the same include guards as the header itself, so it is inert when the
// header was pulled in by something else. (dmex)
//
#ifdef __has_include
#if __has_include(<wbcl.h>)
#include <wbcl.h>
#define ET_HAVE_SDK_WBCL 1
#endif
#endif

#ifndef SIPAEV_PREBOOT_CERT
// TCG PC Client event types (TCG PC Client Platform Firmware Profile)
#define SIPAEV_PREBOOT_CERT                  0x00000000
#define SIPAEV_POST_CODE                     0x00000001
#define SIPAEV_UNUSED                        0x00000002
#define SIPAEV_NO_ACTION                     0x00000003
#define SIPAEV_SEPARATOR                     0x00000004
#define SIPAEV_ACTION                        0x00000005
#define SIPAEV_EVENT_TAG                     0x00000006
#define SIPAEV_S_CRTM_CONTENTS               0x00000007
#define SIPAEV_S_CRTM_VERSION                0x00000008
#define SIPAEV_CPU_MICROCODE                 0x00000009
#define SIPAEV_PLATFORM_CONFIG_FLAGS         0x0000000A
#define SIPAEV_TABLE_OF_DEVICES              0x0000000B
#define SIPAEV_COMPACT_HASH                  0x0000000C
#define SIPAEV_IPL                           0x0000000D
#define SIPAEV_IPL_PARTITION_DATA            0x0000000E
#define SIPAEV_NONHOST_CODE                  0x0000000F
#define SIPAEV_NONHOST_CONFIG                0x00000010
#define SIPAEV_NONHOST_INFO                  0x00000011
#define SIPAEV_OMIT_BOOT_DEVICE_EVENTS       0x00000012
#define SIPAEV_EFI_EVENT_BASE                0x80000000
#define SIPAEV_EFI_VARIABLE_DRIVER_CONFIG    0x80000001
#define SIPAEV_EFI_VARIABLE_BOOT             0x80000002
#define SIPAEV_EFI_BOOT_SERVICES_APPLICATION 0x80000003
#define SIPAEV_EFI_BOOT_SERVICES_DRIVER      0x80000004
#define SIPAEV_EFI_RUNTIME_SERVICES_DRIVER   0x80000005
#define SIPAEV_EFI_GPT_EVENT                 0x80000006
#define SIPAEV_EFI_ACTION                    0x80000007
#define SIPAEV_EFI_PLATFORM_FIRMWARE_BLOB    0x80000008
#define SIPAEV_EFI_HANDOFF_TABLES            0x80000009
#define SIPAEV_EFI_PLATFORM_FIRMWARE_BLOB2   0x8000000A
#define SIPAEV_EFI_HANDOFF_TABLES2           0x8000000B
#define SIPAEV_EFI_VARIABLE_BOOT2            0x8000000C
#define SIPAEV_EFI_HCRTM_EVENT               0x80000010
#define SIPAEV_EFI_VARIABLE_AUTHORITY        0x800000E0
#define SIPAEV_EFI_SPDM_FIRMWARE_BLOB        0x800000E1
#define SIPAEV_EFI_SPDM_FIRMWARE_CONFIG      0x800000E2
#endif // SIPAEV_PREBOOT_CERT

#ifndef SIPAEVENTTYPE_NONMEASURED
// Microsoft SIPA tag encoding: flags | category | ordinal.
#define SIPAEVENTTYPE_NONMEASURED           0x80000000
#define SIPAEVENTTYPE_AGGREGATION           0x40000000

#define SIPAEVENTTYPE_CONTAINER             0x00010000
#define SIPAEVENTTYPE_INFORMATION           0x00020000
#define SIPAEVENTTYPE_ERROR                 0x00030000
#define SIPAEVENTTYPE_PREOSPARAMETER        0x00040000
#define SIPAEVENTTYPE_OSPARAMETER           0x00050000
#define SIPAEVENTTYPE_AUTHORITY             0x00060000
#define SIPAEVENTTYPE_LOADEDMODULE          0x00070000
#define SIPAEVENTTYPE_TRUSTPOINT            0x00080000
#define SIPAEVENTTYPE_ELAM                  0x00090000
#define SIPAEVENTTYPE_VBS                   0x000A0000
#define SIPAEVENTTYPE_KSR                   0x000B0000
#define SIPAEVENTTYPE_DRTM                  0x000C0000
#endif // SIPAEVENTTYPE_NONMEASURED

#ifndef WBCL_DIGEST_ALG_ID_SHA_1
// TPM 2.0 hash algorithm identifiers (TPM_ALG_ID)
#define WBCL_DIGEST_ALG_ID_SHA_1            0x0004
#define WBCL_DIGEST_ALG_ID_SHA_2_256        0x000B
#define WBCL_DIGEST_ALG_ID_SHA_2_384        0x000C
#define WBCL_DIGEST_ALG_ID_SHA_2_512        0x000D
#define WBCL_DIGEST_ALG_ID_SM3_256          0x0012
#define WBCL_DIGEST_ALG_ID_SHA3_256         0x0027
#define WBCL_DIGEST_ALG_ID_SHA3_384         0x0028
#define WBCL_DIGEST_ALG_ID_SHA3_512         0x0029
#endif // WBCL_DIGEST_ALG_ID_SHA_1

#ifndef FVEB_UNLOCK_FLAG_NONE
// BitLocker unlock flags (SIPAEVENT_BITLOCKER_UNLOCK)
#define FVEB_UNLOCK_FLAG_NONE               0x00000000
#define FVEB_UNLOCK_FLAG_CACHED             0x00000001
#define FVEB_UNLOCK_FLAG_MEDIA              0x00000002
#define FVEB_UNLOCK_FLAG_TPM                0x00000004
#define FVEB_UNLOCK_FLAG_PIN                0x00000010
#define FVEB_UNLOCK_FLAG_EXTERNAL           0x00000020
#define FVEB_UNLOCK_FLAG_RECOVERY           0x00000040
#define FVEB_UNLOCK_FLAG_PASSPHRASE         0x00000080
#define FVEB_UNLOCK_FLAG_NBP                0x00000100
#define FVEB_UNLOCK_FLAG_AUK_OSFVEINFO      0x00000200
#endif // FVEB_UNLOCK_FLAG_NONE

#ifndef OSDEVICE_TYPE_UNKNOWN
// Boot device types (SIPAEVENT_OSDEVICE)
#define OSDEVICE_TYPE_UNKNOWN                   0x00000000
#define OSDEVICE_TYPE_BLOCKIO_HARDDISK          0x00010001
#define OSDEVICE_TYPE_BLOCKIO_REMOVABLEDISK     0x00010002
#define OSDEVICE_TYPE_BLOCKIO_CDROM             0x00010003
#define OSDEVICE_TYPE_BLOCKIO_PARTITION         0x00010004
#define OSDEVICE_TYPE_BLOCKIO_FILE              0x00010005
#define OSDEVICE_TYPE_BLOCKIO_RAMDISK           0x00010006
#define OSDEVICE_TYPE_BLOCKIO_VIRTUALHARDDISK   0x00010007
#define OSDEVICE_TYPE_SERIAL                    0x00020000
#define OSDEVICE_TYPE_UDP                       0x00030000
#define OSDEVICE_TYPE_VMBUS                     0x00040000
#define OSDEVICE_TYPE_COMPOSITE                 0x00050000
#define OSDEVICE_TYPE_CIMFS                     0x00060000
#endif // OSDEVICE_TYPE_UNKNOWN

//
// SIPA category mask (the tag bits below the flags but above the ordinal).
//
#define ET_SIPA_CATEGORY_MASK               0x00FF0000
#define ET_SIPA_FLAGS_MASK                  0xFF000000

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

//
// SIPA payload layouts. These mirror the SIPAEVENT_*_PAYLOAD structures in wbcl.h, but are
// declared locally because the SDK definitions are gated on the NTDDI level. (dmex)
//

// SIPAEVENT_SI_POLICY_PAYLOAD
typedef struct _ET_SIPA_SI_POLICY_PAYLOAD
{
    ULONG64 PolicyVersion;
    USHORT PolicyNameLength;
    USHORT HashAlgId;
    ULONG DigestLength;
    BYTE VarLengthData[1]; // WCHAR PolicyName[]; BYTE Digest[];
} ET_SIPA_SI_POLICY_PAYLOAD, *PET_SIPA_SI_POLICY_PAYLOAD;

// SIPAEVENT_SI_POLICY_SIGNER_PAYLOAD
typedef struct _ET_SIPA_SI_POLICY_SIGNER_PAYLOAD
{
    ULONG RootId;
    ULONG CertificatesLength;
    USHORT CertificatesCount;
    USHORT PolicyNameLength;
    USHORT EkusLength;
    USHORT EkusCount;
    BYTE VarLengthData[1]; // WCHAR PolicyName[]; BYTE Ekus[]; BYTE Certificates[];
} ET_SIPA_SI_POLICY_SIGNER_PAYLOAD, *PET_SIPA_SI_POLICY_SIGNER_PAYLOAD;

// SIPAEVENT_SI_POLICY_CERTIFICATE_PAYLOAD
typedef struct _ET_SIPA_SI_POLICY_CERTIFICATE_PAYLOAD
{
    USHORT PublisherCommonNameLength;
    USHORT IssuerCommonNameLength;
    ULONG HashAlgId;
    USHORT DigestLength;
    BYTE VarLengthData[1]; // WCHAR Publisher[]; WCHAR Issuer[]; BYTE Digest[];
} ET_SIPA_SI_POLICY_CERTIFICATE_PAYLOAD, *PET_SIPA_SI_POLICY_CERTIFICATE_PAYLOAD;

// SIPAEVENT_SBCP_INFO_PAYLOAD_V1
typedef struct _ET_SIPA_SBCP_INFO_PAYLOAD
{
    ULONG PayloadVersion;
    ULONG VarDataOffset;
    USHORT HashAlgId;
    USHORT DigestLength;
    ULONG Options;
    ULONG SignersCount;
    BYTE VarData[1]; // BYTE Digest[];
} ET_SIPA_SBCP_INFO_PAYLOAD, *PET_SIPA_SBCP_INFO_PAYLOAD;

// SIPAEVENT_VSM_IDK_INFO_PAYLOAD (RSA variant)
typedef struct _ET_SIPA_VSM_IDK_INFO_PAYLOAD
{
    ULONG KeyAlgId;
    ULONG KeyBitLength;
    ULONG PublicExpLengthBytes;
    ULONG ModulusSizeBytes;
    BYTE PublicKeyData[1];
} ET_SIPA_VSM_IDK_INFO_PAYLOAD, *PET_SIPA_VSM_IDK_INFO_PAYLOAD;

// SIPAEVENT_REVOCATION_LIST_PAYLOAD
typedef struct _ET_SIPA_REVOCATION_LIST_PAYLOAD
{
    LONG64 CreationTime;
    ULONG DigestLength;
    USHORT HashAlgId;
    BYTE Digest[1];
} ET_SIPA_REVOCATION_LIST_PAYLOAD, *PET_SIPA_REVOCATION_LIST_PAYLOAD;

// SIPAEVENT_KSR_SIGNATURE_PAYLOAD
typedef struct _ET_SIPA_KSR_SIGNATURE_PAYLOAD
{
    ULONG SignAlgId;
    ULONG SignatureLength;
    BYTE Signature[1];
} ET_SIPA_KSR_SIGNATURE_PAYLOAD, *PET_SIPA_KSR_SIGNATURE_PAYLOAD;

// PLUTON_UPGRADE_IMAGEDATA (SIPAEVENT_MODULE_PLUTON)
#define ET_SIPA_PLUTON_MAX_HASH_LENGTH 64
#define ET_SIPA_PLUTON_MAX_FILENAME_LENGTH 64

typedef struct _ET_SIPA_PLUTON_UPGRADE_IMAGEDATA
{
    USHORT HashAlgId;
    USHORT DigestSize;
    BYTE Digest[ET_SIPA_PLUTON_MAX_HASH_LENGTH];
    WCHAR FileName[ET_SIPA_PLUTON_MAX_FILENAME_LENGTH];
} ET_SIPA_PLUTON_UPGRADE_IMAGEDATA, *PET_SIPA_PLUTON_UPGRADE_IMAGEDATA;

#include <poppack.h>

// Upper bound on the digest banks tracked per record. The TCG specification
// does not cap TPML_DIGEST_VALUES, but no shipping platform carries more than
// a handful of banks and the Spec ID algorithm table is bounded the same way.
#define ET_WBCL_MAX_DIGESTS 8

typedef struct _WBCL_DIGEST
{
    USHORT AlgorithmId;
    USHORT Size;
    PVOID Data;         // points into the owning LogBuffer
} WBCL_DIGEST, *PWBCL_DIGEST;

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

    // Every digest bank carried by the record, used to replay the log into the
    // PCR values. FirstDigest above aliases Digests[0] and is kept separate so
    // the existing columns and property pages are unaffected.

    ULONG DigestCount;
    WBCL_DIGEST Digests[ET_WBCL_MAX_DIGESTS];

    // Listview row owning this entry, so the replay columns can be refreshed
    // without rebuilding the list when the selected bank changes.

    LONG ItemIndex;
} WBCL_ENTRY, *PWBCL_ENTRY;

LONG NTAPI EtWbclIndexCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2, 
    _In_opt_ PVOID Context
    )
{
    return uintcmp(((PWBCL_ENTRY)Item1)->Index, ((PWBCL_ENTRY)Item2)->Index);
}

LONG NTAPI EtWbclPcrCompareFunction(
    _In_ PVOID Item1, 
    _In_ PVOID Item2, 
    _In_opt_ PVOID Context
    )
{
    return uintcmp(((PWBCL_ENTRY)Item1)->PcrIndex, ((PWBCL_ENTRY)Item2)->PcrIndex);
}

LONG NTAPI EtWbclSizeCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2, 
    _In_opt_ PVOID Context
    )
{
    return uintcmp(((PWBCL_ENTRY)Item1)->EventSize, ((PWBCL_ENTRY)Item2)->EventSize);
}

typedef struct _WBCL_WINDOW_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HWND BankHandle;
    HWND ParentWindowHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PVOID LogBuffer;
    ULONG LogLength;
    PPH_LIST Entries;

    // Digest banks advertised by the log (from the Spec ID Event03 record, or
    // SHA1 alone for a legacy log) and the bank currently being replayed.

    USHORT LogAlgorithms[ET_WBCL_MAX_DIGESTS];
    ULONG LogAlgorithmCount;
    USHORT ReplayAlgorithm;
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
    case SIPAEV_PREBOOT_CERT: return L"EV_PREBOOT_CERT";
    case SIPAEV_POST_CODE: return L"EV_POST_CODE";
    case SIPAEV_UNUSED: return L"EV_UNUSED";
    case SIPAEV_NO_ACTION: return L"EV_NO_ACTION";
    case SIPAEV_SEPARATOR: return L"EV_SEPARATOR";
    case SIPAEV_ACTION: return L"EV_ACTION";
    case SIPAEV_EVENT_TAG: return L"EV_EVENT_TAG";
    case SIPAEV_S_CRTM_CONTENTS: return L"EV_S_CRTM_CONTENTS";
    case SIPAEV_S_CRTM_VERSION: return L"EV_S_CRTM_VERSION";
    case SIPAEV_CPU_MICROCODE: return L"EV_CPU_MICROCODE";
    case SIPAEV_PLATFORM_CONFIG_FLAGS: return L"EV_PLATFORM_CONFIG_FLAGS";
    case SIPAEV_TABLE_OF_DEVICES: return L"EV_TABLE_OF_DEVICES";
    case SIPAEV_COMPACT_HASH: return L"EV_COMPACT_HASH";
    case SIPAEV_IPL: return L"EV_IPL";
    case SIPAEV_IPL_PARTITION_DATA: return L"EV_IPL_PARTITION_DATA";
    case SIPAEV_NONHOST_CODE: return L"EV_NONHOST_CODE";
    case SIPAEV_NONHOST_CONFIG: return L"EV_NONHOST_CONFIG";
    case SIPAEV_NONHOST_INFO: return L"EV_NONHOST_INFO";
    case SIPAEV_OMIT_BOOT_DEVICE_EVENTS: return L"EV_OMIT_BOOT_DEVICE_EVENTS";
    case SIPAEV_EFI_EVENT_BASE: return L"EV_EFI_EVENT_BASE";
    case SIPAEV_EFI_VARIABLE_DRIVER_CONFIG: return L"EV_EFI_VARIABLE_DRIVER_CONFIG";
    case SIPAEV_EFI_VARIABLE_BOOT: return L"EV_EFI_VARIABLE_BOOT";
    case SIPAEV_EFI_BOOT_SERVICES_APPLICATION: return L"EV_EFI_BOOT_SERVICES_APPLICATION";
    case SIPAEV_EFI_BOOT_SERVICES_DRIVER: return L"EV_EFI_BOOT_SERVICES_DRIVER";
    case SIPAEV_EFI_RUNTIME_SERVICES_DRIVER: return L"EV_EFI_RUNTIME_SERVICES_DRIVER";
    case SIPAEV_EFI_GPT_EVENT: return L"EV_EFI_GPT_EVENT";
    case SIPAEV_EFI_ACTION: return L"EV_EFI_ACTION";
    case SIPAEV_EFI_PLATFORM_FIRMWARE_BLOB: return L"EV_EFI_PLATFORM_FIRMWARE_BLOB";
    case SIPAEV_EFI_HANDOFF_TABLES: return L"EV_EFI_HANDOFF_TABLES";
    case SIPAEV_EFI_PLATFORM_FIRMWARE_BLOB2: return L"EV_EFI_PLATFORM_FIRMWARE_BLOB2";
    case SIPAEV_EFI_HANDOFF_TABLES2: return L"EV_EFI_HANDOFF_TABLES2";
    case SIPAEV_EFI_VARIABLE_BOOT2: return L"EV_EFI_VARIABLE_BOOT2";
    case SIPAEV_EFI_HCRTM_EVENT: return L"EV_EFI_HCRTM_EVENT";
    case SIPAEV_EFI_VARIABLE_AUTHORITY: return L"EV_EFI_VARIABLE_AUTHORITY";
    case SIPAEV_EFI_SPDM_FIRMWARE_BLOB: return L"EV_EFI_SPDM_FIRMWARE_BLOB";
    case SIPAEV_EFI_SPDM_FIRMWARE_CONFIG: return L"EV_EFI_SPDM_FIRMWARE_CONFIG";
    default: return NULL;
    }
}

PCWSTR EtWbclAlgorithmToString(
    _In_ USHORT AlgorithmId
    )
{
    switch (AlgorithmId)
    {
    case WBCL_DIGEST_ALG_ID_SHA_1: return L"SHA1";
    case WBCL_DIGEST_ALG_ID_SHA_2_256: return L"SHA256";
    case WBCL_DIGEST_ALG_ID_SHA_2_384: return L"SHA384";
    case WBCL_DIGEST_ALG_ID_SHA_2_512: return L"SHA512";
    case WBCL_DIGEST_ALG_ID_SM3_256: return L"SM3-256";
    case WBCL_DIGEST_ALG_ID_SHA3_256: return L"SHA3-256";
    case WBCL_DIGEST_ALG_ID_SHA3_384: return L"SHA3-384";
    case WBCL_DIGEST_ALG_ID_SHA3_512: return L"SHA3-512";
    default: return L"?";
    }
}

USHORT EtWbclAlgorithmDigestSize(
    _In_ USHORT AlgorithmId
    )
{
    switch (AlgorithmId)
    {
    case WBCL_DIGEST_ALG_ID_SHA_1: return 20;
    case WBCL_DIGEST_ALG_ID_SHA_2_256: return 32;
    case WBCL_DIGEST_ALG_ID_SHA_2_384: return 48;
    case WBCL_DIGEST_ALG_ID_SHA_2_512: return 64;
    case WBCL_DIGEST_ALG_ID_SM3_256: return 32;
    case WBCL_DIGEST_ALG_ID_SHA3_256: return 32;
    case WBCL_DIGEST_ALG_ID_SHA3_384: return 48;
    case WBCL_DIGEST_ALG_ID_SHA3_512: return 64;
    default: return 0;
    }
}

//
// The tag values below are written as (flags + category + ordinal) expressions, matching how
// wbcl.h defines each SIPAEVENT_*. The SIPAEVENT_* macros themselves are gated on the SDK NTDDI
// level, so they are not referenced directly. (dmex)
//
PCWSTR EtWbclSipaTypeToString(
    _In_ ULONG SipaType
    )
{
    switch (SipaType)
    {
    // Containers (aggregations)
    case SIPAEVENTTYPE_AGGREGATION + SIPAEVENTTYPE_CONTAINER + 0x0001: return L"TrustBoundary";
    case SIPAEVENTTYPE_AGGREGATION + SIPAEVENTTYPE_CONTAINER + 0x0002: return L"ELAM Aggregation";
    case SIPAEVENTTYPE_AGGREGATION + SIPAEVENTTYPE_CONTAINER + 0x0003: return L"LoadedModule Aggregation";
    case SIPAEVENTTYPE_NONMEASURED + SIPAEVENTTYPE_AGGREGATION + SIPAEVENTTYPE_CONTAINER + 0x0004: return L"TrustPoint Aggregation";
    case SIPAEVENTTYPE_AGGREGATION + SIPAEVENTTYPE_CONTAINER + 0x0005: return L"KSR Aggregation";
    case SIPAEVENTTYPE_AGGREGATION + SIPAEVENTTYPE_CONTAINER + 0x0006: return L"KSR Signed Measurement Aggregation";

    // Errors
    case SIPAEVENTTYPE_ERROR + 0x0001: return L"FirmwareFailure";
    case SIPAEVENTTYPE_NONMEASURED + SIPAEVENTTYPE_ERROR + 0x0002: return L"TpmFailure";
    case SIPAEVENTTYPE_ERROR + 0x0003: return L"InternalFailure";
    case SIPAEVENTTYPE_NONMEASURED + SIPAEVENTTYPE_ERROR + 0x0004: return L"KsrFailure";
    case SIPAEVENTTYPE_ERROR + 0x0005: return L"HypervisorFailure";

    // Information
    case SIPAEVENTTYPE_INFORMATION + 0x0001: return L"Information";
    case SIPAEVENTTYPE_INFORMATION + 0x0002: return L"BootCounter";
    case SIPAEVENTTYPE_INFORMATION + 0x0003: return L"TransferControl";
    case SIPAEVENTTYPE_INFORMATION + 0x0004: return L"ApplicationReturn";
    case SIPAEVENTTYPE_INFORMATION + 0x0005: return L"BitlockerUnlock";
    case SIPAEVENTTYPE_INFORMATION + 0x0006: return L"EventCounter";
    case SIPAEVENTTYPE_INFORMATION + 0x0007: return L"CounterId";
    case SIPAEVENTTYPE_INFORMATION + 0x0008: return L"MorBitNotCancelable";
    case SIPAEVENTTYPE_INFORMATION + 0x0009: return L"ApplicationSvn";
    case SIPAEVENTTYPE_INFORMATION + 0x000A: return L"SvnChainStatus";
    case SIPAEVENTTYPE_INFORMATION + 0x000B: return L"MorBitApiStatus";
    case SIPAEVENTTYPE_INFORMATION + 0x000C: return L"IdkGenerationStatus";

    // Pre-OS parameters
    case SIPAEVENTTYPE_PREOSPARAMETER + 0x0001: return L"BootDebugging";
    case SIPAEVENTTYPE_PREOSPARAMETER + 0x0002: return L"BootRevocationList";

    // OS parameters
    case SIPAEVENTTYPE_OSPARAMETER + 0x0001: return L"OsKernelDebug";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0002: return L"CodeIntegrity";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0003: return L"TestSigning";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0004: return L"DataExecutionPrevention";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0005: return L"SafeMode";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0006: return L"WinPE";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0007: return L"PhysicalAddressExtension";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0008: return L"OsDevice";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0009: return L"SystemRoot";
    case SIPAEVENTTYPE_OSPARAMETER + 0x000A: return L"HypervisorLaunchType";
    case SIPAEVENTTYPE_OSPARAMETER + 0x000B: return L"HypervisorPath";
    case SIPAEVENTTYPE_OSPARAMETER + 0x000C: return L"HypervisorIommuPolicy";
    case SIPAEVENTTYPE_OSPARAMETER + 0x000D: return L"HypervisorDebug";
    case SIPAEVENTTYPE_OSPARAMETER + 0x000E: return L"DriverLoadPolicy";
    case SIPAEVENTTYPE_OSPARAMETER + 0x000F: return L"SiPolicy";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0010: return L"HypervisorMmioNxPolicy";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0011: return L"HypervisorMsrFilterPolicy";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0012: return L"VsmLaunchType";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0013: return L"OsRevocationList";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0014: return L"SmtStatus";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0020: return L"VsmIdkInfo";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0021: return L"FlightSigning";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0022: return L"PagefileEncryptionEnabled";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0023: return L"VsmIdksInfo";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0024: return L"HibernationDisabled";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0025: return L"DumpsDisabled";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0026: return L"DumpEncryptionEnabled";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0027: return L"DumpEncryptionKeyDigest";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0028: return L"LsaIsoConfig";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0029: return L"SbcpInfo";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0030: return L"HypervisorBootDmaProtection";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0031: return L"SiPolicySigner";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0032: return L"SiPolicyUpdateSigner";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0033: return L"RefsVolumeCheckpointRecordChecksum";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0034: return L"RefsRollbackProtectionFrozenVolumeChecksum";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0035: return L"RefsRollbackProtectionUserPayloadHash";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0036: return L"RefsRollbackProtectionVerificationSucceeded";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0037: return L"RefsRollbackProtectionVolumeFirstEverMount";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0038: return L"SiPolicySupplementalSigner";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0039: return L"SiPolicySupplementalPolicy";
    case SIPAEVENTTYPE_OSPARAMETER + 0x003A: return L"VsmSealedSiPolicy";
    case SIPAEVENTTYPE_OSPARAMETER + 0x003B: return L"VsmDrtmKeyrollDetected";
    case SIPAEVENTTYPE_OSPARAMETER + 0x003C: return L"VsmSrtmUnsealPolicy";
    case SIPAEVENTTYPE_OSPARAMETER + 0x003D: return L"VsmSrtmAntiRollbackCounter";
    case SIPAEVENTTYPE_OSPARAMETER + 0x003E: return L"RefsAttestationSummary";
    case SIPAEVENTTYPE_OSPARAMETER + 0x0040: return L"Vtl1DumpConfig";

    // Authority
    case SIPAEVENTTYPE_AUTHORITY + 0x0001: return L"NoAuthority";
    case SIPAEVENTTYPE_AUTHORITY + 0x0002: return L"AuthorityPubKey";

    // Loaded module
    case SIPAEVENTTYPE_LOADEDMODULE + 0x0001: return L"FilePath";
    case SIPAEVENTTYPE_LOADEDMODULE + 0x0002: return L"ImageSize";
    case SIPAEVENTTYPE_LOADEDMODULE + 0x0003: return L"HashAlgorithmId";
    case SIPAEVENTTYPE_LOADEDMODULE + 0x0004: return L"AuthenticodeHash";
    case SIPAEVENTTYPE_LOADEDMODULE + 0x0005: return L"AuthorityIssuer";
    case SIPAEVENTTYPE_LOADEDMODULE + 0x0006: return L"AuthoritySerial";
    case SIPAEVENTTYPE_LOADEDMODULE + 0x0007: return L"ImageBase";
    case SIPAEVENTTYPE_LOADEDMODULE + 0x0008: return L"AuthorityPublisher";
    case SIPAEVENTTYPE_LOADEDMODULE + 0x0009: return L"AuthoritySha1Thumbprint";
    case SIPAEVENTTYPE_LOADEDMODULE + 0x000A: return L"ImageValidated";
    case SIPAEVENTTYPE_LOADEDMODULE + 0x000B: return L"ModuleSvn";
    case SIPAEVENTTYPE_LOADEDMODULE + 0x000C: return L"ModulePluton";
    case SIPAEVENTTYPE_LOADEDMODULE + 0x000D: return L"ModuleOriginalFilename";
    case SIPAEVENTTYPE_LOADEDMODULE + 0x000E: return L"ModuleVersion";
    case SIPAEVENTTYPE_LOADEDMODULE + 0x000F: return L"PublisherOemName";

    // Trust point
    case SIPAEVENTTYPE_NONMEASURED + SIPAEVENTTYPE_TRUSTPOINT + 0x0001: return L"Quote";
    case SIPAEVENTTYPE_NONMEASURED + SIPAEVENTTYPE_TRUSTPOINT + 0x0002: return L"QuoteSignature";
    case SIPAEVENTTYPE_NONMEASURED + SIPAEVENTTYPE_TRUSTPOINT + 0x0003: return L"AikId";
    case SIPAEVENTTYPE_NONMEASURED + SIPAEVENTTYPE_TRUSTPOINT + 0x0004: return L"AikPubDigest";

    // ELAM
    case SIPAEVENTTYPE_ELAM + 0x0001: return L"ELAM Keyname";
    case SIPAEVENTTYPE_ELAM + 0x0002: return L"ELAM Configuration";
    case SIPAEVENTTYPE_ELAM + 0x0003: return L"ELAM Policy";
    case SIPAEVENTTYPE_ELAM + 0x0004: return L"ELAM Measured";

    // VBS
    case SIPAEVENTTYPE_VBS + 0x0001: return L"VbsVsmRequired";
    case SIPAEVENTTYPE_VBS + 0x0002: return L"VbsSecureBootRequired";
    case SIPAEVENTTYPE_VBS + 0x0003: return L"VbsIommuRequired";
    case SIPAEVENTTYPE_VBS + 0x0004: return L"VbsMmioNxRequired";
    case SIPAEVENTTYPE_VBS + 0x0005: return L"VbsMsrFilteringRequired";
    case SIPAEVENTTYPE_VBS + 0x0006: return L"VbsMandatoryEnforcement";
    case SIPAEVENTTYPE_VBS + 0x0007: return L"VbsHvciPolicy";
    case SIPAEVENTTYPE_VBS + 0x0008: return L"VbsMicrosoftBootChainRequired";
    case SIPAEVENTTYPE_VBS + 0x0009: return L"VbsDumpUsesAmeRoot";
    case SIPAEVENTTYPE_VBS + 0x000A: return L"VbsVsmNoSecretsEnforced";

    // KSR
    case SIPAEVENTTYPE_KSR + 0x0001: return L"KsrSignature";

    // DRTM
    case SIPAEVENTTYPE_DRTM + 0x0001: return L"DrtmStateAuth";
    case SIPAEVENTTYPE_DRTM + 0x0002: return L"DrtmSmmLevel";
    case SIPAEVENTTYPE_DRTM + 0x0003: return L"DrtmAmdSmmHash";
    case SIPAEVENTTYPE_DRTM + 0x0004: return L"DrtmAmdSmmSignerKey";

    default: return NULL;
    }
}

PCWSTR EtWbclOsDeviceTypeToString(
    _In_ ULONG DeviceType
    )
{
    switch (DeviceType)
    {
    case OSDEVICE_TYPE_UNKNOWN: return L"Unknown";
    case OSDEVICE_TYPE_BLOCKIO_HARDDISK: return L"Hard disk";
    case OSDEVICE_TYPE_BLOCKIO_REMOVABLEDISK: return L"Removable disk";
    case OSDEVICE_TYPE_BLOCKIO_CDROM: return L"CD-ROM";
    case OSDEVICE_TYPE_BLOCKIO_PARTITION: return L"Partition";
    case OSDEVICE_TYPE_BLOCKIO_FILE: return L"File";
    case OSDEVICE_TYPE_BLOCKIO_RAMDISK: return L"RAM disk";
    case OSDEVICE_TYPE_BLOCKIO_VIRTUALHARDDISK: return L"Virtual hard disk";
    case OSDEVICE_TYPE_SERIAL: return L"Serial";
    case OSDEVICE_TYPE_UDP: return L"UDP";
    case OSDEVICE_TYPE_VMBUS: return L"VMBus";
    case OSDEVICE_TYPE_COMPOSITE: return L"Composite";
    case OSDEVICE_TYPE_CIMFS: return L"CimFS";
    default: return NULL;
    }
}

VOID EtWbclAppendUnlockFlags(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ ULONG Flags
    )
{
    static const struct { ULONG Flag; PCWSTR Name; } names[] =
    {
        { FVEB_UNLOCK_FLAG_CACHED, L"Cached" },
        { FVEB_UNLOCK_FLAG_MEDIA, L"Media" },
        { FVEB_UNLOCK_FLAG_TPM, L"TPM" },
        { FVEB_UNLOCK_FLAG_PIN, L"PIN" },
        { FVEB_UNLOCK_FLAG_EXTERNAL, L"External" },
        { FVEB_UNLOCK_FLAG_RECOVERY, L"Recovery" },
        { FVEB_UNLOCK_FLAG_PASSPHRASE, L"Passphrase" },
        { FVEB_UNLOCK_FLAG_NBP, L"NBP" },
        { FVEB_UNLOCK_FLAG_AUK_OSFVEINFO, L"AukOsFveInfo" },
    };
    BOOLEAN first = TRUE;

    if (Flags == FVEB_UNLOCK_FLAG_NONE)
    {
        PhAppendStringBuilder2(StringBuilder, L"None");
        return;
    }

    for (ULONG i = 0; i < RTL_NUMBER_OF(names); i++)
    {
        if (!BooleanFlagOn(Flags, names[i].Flag))
            continue;

        if (!first)
            PhAppendStringBuilder2(StringBuilder, L", ");

        PhAppendStringBuilder2(StringBuilder, names[i].Name);
        first = FALSE;
    }

    if (first)
        PhAppendFormatStringBuilder(StringBuilder, L"0x%08lx", Flags);
}

// Appends a WCHAR run of ByteLength bytes taken from Data, without assuming termination.
VOID EtWbclAppendCountedString(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_reads_bytes_(ByteLength) PVOID Data,
    _In_ ULONG ByteLength
    )
{
    PPH_STRING string;

    if (ByteLength < sizeof(WCHAR))
        return;

    string = PhCreateStringEx(Data, ByteLength & ~(SIZE_T)1);
    PhTrimToNullTerminatorString(string);
    PhAppendStringBuilder(StringBuilder, &string->sr);
    PhDereferenceObject(string);
}

VOID EtWbclAppendDigest(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ USHORT AlgorithmId,
    _In_reads_bytes_(Length) PVOID Data,
    _In_ ULONG Length
    )
{
    PPH_STRING hex;

    if (Length == 0)
        return;

    hex = PhBufferToHexString((PUCHAR)Data, Length);
    PhAppendFormatStringBuilder(StringBuilder, L" %s:%s", EtWbclAlgorithmToString(AlgorithmId), hex->Buffer);
    PhDereferenceObject(hex);
}

//
// Decodes the payload of the SIPA tags that carry a documented structure (wbcl.h). Returns NULL
// when the tag has no known layout or the payload is too short, in which case the caller falls
// back to the generic leaf rendering. All accesses are bounded by Size because the log is
// firmware-supplied data. (dmex)
//
PPH_STRING EtWbclFormatSipaPayload(
    _In_ ULONG SipaType,
    _In_reads_bytes_(Size) PVOID Data,
    _In_ ULONG Size
    )
{
    PH_STRING_BUILDER stringBuilder;

    switch (SipaType)
    {
    case SIPAEVENTTYPE_OSPARAMETER + 0x000F: // SiPolicy
    case SIPAEVENTTYPE_OSPARAMETER + 0x003A: // VsmSealedSiPolicy
        {
            PET_SIPA_SI_POLICY_PAYLOAD policy = Data;
            ULONG offset = UFIELD_OFFSET(ET_SIPA_SI_POLICY_PAYLOAD, VarLengthData);

            if (Size < offset)
                break;
            if (policy->PolicyNameLength > Size - offset)
                break;
            if (policy->DigestLength > Size - offset - policy->PolicyNameLength)
                break;

            PhInitializeStringBuilder(&stringBuilder, 0x80);
            PhAppendStringBuilder2(&stringBuilder, L"\"");
            EtWbclAppendCountedString(&stringBuilder, policy->VarLengthData, policy->PolicyNameLength);
            PhAppendFormatStringBuilder(&stringBuilder, L"\" version %I64u", policy->PolicyVersion);
            EtWbclAppendDigest(
                &stringBuilder,
                policy->HashAlgId,
                policy->VarLengthData + policy->PolicyNameLength,
                policy->DigestLength
                );
            return PhFinalStringBuilderString(&stringBuilder);
        }
        break;
    case SIPAEVENTTYPE_OSPARAMETER + 0x0031: // SiPolicySigner
    case SIPAEVENTTYPE_OSPARAMETER + 0x0032: // SiPolicyUpdateSigner
    case SIPAEVENTTYPE_OSPARAMETER + 0x0038: // SiPolicySupplementalSigner
        {
            PET_SIPA_SI_POLICY_SIGNER_PAYLOAD signer = Data;
            ULONG offset = UFIELD_OFFSET(ET_SIPA_SI_POLICY_SIGNER_PAYLOAD, VarLengthData);
            PBYTE certificates;
            ULONG remaining;

            if (Size < offset)
                break;

            remaining = Size - offset;

            if (signer->PolicyNameLength > remaining)
                break;

            remaining -= signer->PolicyNameLength;

            if (signer->EkusLength > remaining)
                break;

            remaining -= signer->EkusLength;

            if (signer->CertificatesLength > remaining)
                break;

            PhInitializeStringBuilder(&stringBuilder, 0x80);
            PhAppendStringBuilder2(&stringBuilder, L"\"");
            EtWbclAppendCountedString(&stringBuilder, signer->VarLengthData, signer->PolicyNameLength);
            PhAppendFormatStringBuilder(
                &stringBuilder,
                L"\" root 0x%lx, %hu certificate(s), %hu EKU(s)",
                signer->RootId,
                signer->CertificatesCount,
                signer->EkusCount
                );

            certificates = signer->VarLengthData + signer->PolicyNameLength + signer->EkusLength;
            remaining = signer->CertificatesLength;

            for (USHORT i = 0; i < signer->CertificatesCount; i++)
            {
                PET_SIPA_SI_POLICY_CERTIFICATE_PAYLOAD certificate = (PET_SIPA_SI_POLICY_CERTIFICATE_PAYLOAD)certificates;
                ULONG header = UFIELD_OFFSET(ET_SIPA_SI_POLICY_CERTIFICATE_PAYLOAD, VarLengthData);
                ULONG length;

                if (remaining < header)
                    break;

                length = (ULONG)certificate->PublisherCommonNameLength +
                    certificate->IssuerCommonNameLength + certificate->DigestLength;

                if (length > remaining - header)
                    break;

                PhAppendStringBuilder2(&stringBuilder, L"\r\n    publisher \"");
                EtWbclAppendCountedString(
                    &stringBuilder,
                    certificate->VarLengthData,
                    certificate->PublisherCommonNameLength
                    );
                PhAppendStringBuilder2(&stringBuilder, L"\", issuer \"");
                EtWbclAppendCountedString(
                    &stringBuilder,
                    certificate->VarLengthData + certificate->PublisherCommonNameLength,
                    certificate->IssuerCommonNameLength
                    );
                PhAppendStringBuilder2(&stringBuilder, L"\"");

                if (certificate->DigestLength != 0)
                {
                    PPH_STRING hex = PhBufferToHexString(
                        certificate->VarLengthData + certificate->PublisherCommonNameLength +
                            certificate->IssuerCommonNameLength,
                        certificate->DigestLength
                        );
                    PhAppendFormatStringBuilder(&stringBuilder, L" %s", hex->Buffer);
                    PhDereferenceObject(hex);
                }

                certificates += header + length;
                remaining -= header + length;
            }

            return PhFinalStringBuilderString(&stringBuilder);
        }
        break;
    case SIPAEVENTTYPE_OSPARAMETER + 0x0029: // SbcpInfo
        {
            PET_SIPA_SBCP_INFO_PAYLOAD sbcp = Data;

            if (Size < UFIELD_OFFSET(ET_SIPA_SBCP_INFO_PAYLOAD, VarData))
                break;
            if (sbcp->VarDataOffset > Size)
                break;
            if (sbcp->DigestLength > Size - sbcp->VarDataOffset)
                break;

            PhInitializeStringBuilder(&stringBuilder, 0x80);
            PhAppendFormatStringBuilder(
                &stringBuilder,
                L"options 0x%08lx, %lu signer(s)",
                sbcp->Options,
                sbcp->SignersCount
                );
            EtWbclAppendDigest(
                &stringBuilder,
                sbcp->HashAlgId,
                (PBYTE)Data + sbcp->VarDataOffset,
                sbcp->DigestLength
                );
            return PhFinalStringBuilderString(&stringBuilder);
        }
        break;
    case SIPAEVENTTYPE_OSPARAMETER + 0x0020: // VsmIdkInfo
    case SIPAEVENTTYPE_OSPARAMETER + 0x0023: // VsmIdksInfo
        {
            PET_SIPA_VSM_IDK_INFO_PAYLOAD idk = Data;

            if (Size < UFIELD_OFFSET(ET_SIPA_VSM_IDK_INFO_PAYLOAD, PublicKeyData))
                break;

            return PhFormatString(
                L"algorithm %lu, RSA %lu-bit (exponent %lu bytes, modulus %lu bytes)",
                idk->KeyAlgId,
                idk->KeyBitLength,
                idk->PublicExpLengthBytes,
                idk->ModulusSizeBytes
                );
        }
        break;
    case SIPAEVENTTYPE_PREOSPARAMETER + 0x0002: // BootRevocationList
    case SIPAEVENTTYPE_OSPARAMETER + 0x0013: // OsRevocationList
        {
            PET_SIPA_REVOCATION_LIST_PAYLOAD list = Data;
            LARGE_INTEGER time;
            SYSTEMTIME systemTime;
            PPH_STRING timeString;

            if (Size < UFIELD_OFFSET(ET_SIPA_REVOCATION_LIST_PAYLOAD, Digest))
                break;
            if (list->DigestLength > Size - UFIELD_OFFSET(ET_SIPA_REVOCATION_LIST_PAYLOAD, Digest))
                break;

            time.QuadPart = list->CreationTime;
            PhLargeIntegerToLocalSystemTime(&systemTime, &time);
            timeString = PhFormatDateTime(&systemTime);

            PhInitializeStringBuilder(&stringBuilder, 0x80);
            PhAppendFormatStringBuilder(&stringBuilder, L"created %s", PhGetString(timeString));
            EtWbclAppendDigest(&stringBuilder, list->HashAlgId, list->Digest, list->DigestLength);
            PhClearReference(&timeString);
            return PhFinalStringBuilderString(&stringBuilder);
        }
        break;
    case SIPAEVENTTYPE_KSR + 0x0001: // KsrSignature
        {
            PET_SIPA_KSR_SIGNATURE_PAYLOAD signature = Data;

            if (Size < UFIELD_OFFSET(ET_SIPA_KSR_SIGNATURE_PAYLOAD, Signature))
                break;
            if (signature->SignatureLength > Size - UFIELD_OFFSET(ET_SIPA_KSR_SIGNATURE_PAYLOAD, Signature))
                break;

            return PhFormatString(
                L"algorithm %lu, %lu byte signature",
                signature->SignAlgId,
                signature->SignatureLength
                );
        }
        break;
    case SIPAEVENTTYPE_LOADEDMODULE + 0x000C: // ModulePluton
        {
            PET_SIPA_PLUTON_UPGRADE_IMAGEDATA pluton = Data;

            if (Size < sizeof(ET_SIPA_PLUTON_UPGRADE_IMAGEDATA))
                break;
            if (pluton->DigestSize > ET_SIPA_PLUTON_MAX_HASH_LENGTH)
                break;

            PhInitializeStringBuilder(&stringBuilder, 0x80);
            PhAppendStringBuilder2(&stringBuilder, L"\"");
            EtWbclAppendCountedString(&stringBuilder, pluton->FileName, sizeof(pluton->FileName));
            PhAppendStringBuilder2(&stringBuilder, L"\"");
            EtWbclAppendDigest(&stringBuilder, pluton->HashAlgId, pluton->Digest, pluton->DigestSize);
            return PhFinalStringBuilderString(&stringBuilder);
        }
        break;
    case SIPAEVENTTYPE_LOADEDMODULE + 0x0003: // HashAlgorithmId
        {
            if (Size == sizeof(ULONG))
            {
                ULONG algorithmId = *(PULONG UNALIGNED)Data;
                return PhFormatString(L"%s (0x%04lx)", EtWbclAlgorithmToString((USHORT)algorithmId), algorithmId);
            }
            if (Size == sizeof(USHORT))
            {
                USHORT algorithmId = *(PUSHORT UNALIGNED)Data;
                return PhFormatString(L"%s (0x%04hx)", EtWbclAlgorithmToString(algorithmId), algorithmId);
            }
        }
        break;
    case SIPAEVENTTYPE_INFORMATION + 0x0005: // BitlockerUnlock
        {
            if (Size != sizeof(ULONG))
                break;

            PhInitializeStringBuilder(&stringBuilder, 0x40);
            EtWbclAppendUnlockFlags(&stringBuilder, *(PULONG UNALIGNED)Data);
            return PhFinalStringBuilderString(&stringBuilder);
        }
        break;
    case SIPAEVENTTYPE_INFORMATION + 0x000B: // MorBitApiStatus
        {
            NTSTATUS status;
            PPH_STRING message;
            PPH_STRING result;

            if (Size != sizeof(NTSTATUS))
                break;

            status = *(NTSTATUS UNALIGNED*)Data;
            message = PhGetStatusMessage(status, 0);
            result = PhFormatString(L"0x%08lx (%s)", status, PhGetStringOrDefault(message, L"Unknown"));
            PhClearReference(&message);
            return result;
        }
        break;
    case SIPAEVENTTYPE_OSPARAMETER + 0x0008: // OsDevice
        {
            ULONG deviceType;
            PCWSTR name;

            if (Size != sizeof(ULONG))
                break;

            deviceType = *(PULONG UNALIGNED)Data;
            name = EtWbclOsDeviceTypeToString(deviceType);

            if (name)
                return PhFormatString(L"%s (0x%08lx)", name, deviceType);
        }
        break;
    case SIPAEVENTTYPE_OSPARAMETER + 0x0014: // SmtStatus
        {
            if (Size != sizeof(ULONG))
                break;

            switch (*(PULONG UNALIGNED)Data)
            {
            case 0: return PhCreateString(L"Disabled or unsupported (0)");
            case 1: return PhCreateString(L"Enabled (1)");
            case 2: return PhCreateString(L"Enabled in firmware, disabled in software (2)");
            }
        }
        break;
    }

    return NULL;
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

// A crafted log can nest aggregation records arbitrarily deeply and describe an
// unbounded number of them, so decoding is capped on all three axes. (SEC-09)
#define ET_WBCL_SIPA_MAX_DEPTH 32
#define ET_WBCL_SIPA_MAX_RECORDS 0x10000
#define ET_WBCL_SIPA_MAX_OUTPUT (1 * 1024 * 1024)

// Walks the Microsoft SIPA TLV stream carried inside EV_EVENT_TAG events.
VOID EtWbclAppendSipaEvents(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_reads_bytes_(Size) PVOID Data,
    _In_ ULONG Size,
    _In_ ULONG Depth,
    _Inout_ PULONG RecordBudget
    )
{
    PBYTE position = Data;
    PBYTE end = position + Size;

    if (Depth >= ET_WBCL_SIPA_MAX_DEPTH)
    {
        PhAppendStringBuilder2(StringBuilder, L"(nesting too deep)\r\n");
        return;
    }

    while ((position + RTL_SIZEOF_THROUGH_FIELD(ET_SIPA_EVENT_HEADER, EventSize)) <= end)
    {
        PET_SIPA_EVENT_HEADER header = (PET_SIPA_EVENT_HEADER)position;
        ULONG sipaType = header->EventType;
        ULONG sipaSize = header->EventSize;
        PBYTE sipaData = header->EventData;
        PCWSTR name;

        if (sipaData + sipaSize > end)
            break;

        if (*RecordBudget == 0 || StringBuilder->String->Length >= ET_WBCL_SIPA_MAX_OUTPUT)
        {
            PhAppendStringBuilder2(StringBuilder, L"(truncated)\r\n");
            break;
        }

        (*RecordBudget)--;

        for (ULONG i = 0; i < Depth; i++)
            PhAppendStringBuilder2(StringBuilder, L"    ");

        name = EtWbclSipaTypeToString(sipaType);

        if (name)
            PhAppendFormatStringBuilder(StringBuilder, L"%s (0x%08lx)", name, sipaType);
        else
            PhAppendFormatStringBuilder(StringBuilder, L"0x%08lx", sipaType);

        // A non-measured record is carried by the log but not extended into the PCR.
        if (BooleanFlagOn(sipaType, SIPAEVENTTYPE_NONMEASURED))
            PhAppendStringBuilder2(StringBuilder, L" [non-measured]");

        // Container types wrap nested events. The category must be compared with the flag bits
        // masked off: containers carry SIPAEVENTTYPE_AGGREGATION, and the trust point container
        // also carries SIPAEVENTTYPE_NONMEASURED.
        if ((sipaType & ET_SIPA_CATEGORY_MASK) == SIPAEVENTTYPE_CONTAINER && sipaSize != 0)
        {
            PhAppendStringBuilder2(StringBuilder, L":\r\n");
            EtWbclAppendSipaEvents(StringBuilder, sipaData, sipaSize, Depth + 1, RecordBudget);
        }
        else
        {
            PPH_STRING value;

            if (!(value = EtWbclFormatSipaPayload(sipaType, sipaData, sipaSize)))
                value = EtWbclFormatLeaf(sipaData, sipaSize);

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
    case SIPAEV_EFI_VARIABLE_DRIVER_CONFIG:
    case SIPAEV_EFI_VARIABLE_BOOT:
    case SIPAEV_EFI_VARIABLE_BOOT2:
    case SIPAEV_EFI_VARIABLE_AUTHORITY:
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
    case SIPAEV_EFI_BOOT_SERVICES_APPLICATION:
    case SIPAEV_EFI_BOOT_SERVICES_DRIVER:
    case SIPAEV_EFI_RUNTIME_SERVICES_DRIVER:
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
    case SIPAEV_EFI_PLATFORM_FIRMWARE_BLOB:
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
    case SIPAEV_EVENT_TAG:
        {
            PH_STRING_BUILDER stringBuilder;
            ULONG recordBudget = ET_WBCL_SIPA_MAX_RECORDS;

            PhInitializeStringBuilder(&stringBuilder, 0x100);
            EtWbclAppendSipaEvents(&stringBuilder, Event, EventSize, 0, &recordBudget);

            if (stringBuilder.String->Length != 0)
                return PhFinalStringBuilderString(&stringBuilder);

            PhDeleteStringBuilder(&stringBuilder);
        }
        break;
    case SIPAEV_POST_CODE:
    case SIPAEV_S_CRTM_VERSION:
    case SIPAEV_ACTION:
    case SIPAEV_EFI_ACTION:
    case SIPAEV_NO_ACTION:
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
    _In_reads_(DigestCount) PWBCL_DIGEST Digests,
    _In_ ULONG DigestCount,
    _In_reads_bytes_(EventSize) PVOID Event,
    _In_ ULONG EventSize
    )
{
    PWBCL_ENTRY entry;
    LONG lvItemIndex;
    PPH_STRING string;
    PCWSTR typeName;
    USHORT digestAlg;
    PVOID digestValue;
    USHORT digestSize;

    entry = PhAllocateZero(sizeof(WBCL_ENTRY));
    entry->Index = Context->Entries->Count;
    entry->PcrIndex = PcrIndex;
    entry->EventType = EventType;
    entry->EventData = Event;
    entry->EventSize = EventSize;

    if (DigestCount > ET_WBCL_MAX_DIGESTS)
        DigestCount = ET_WBCL_MAX_DIGESTS;

    entry->DigestCount = DigestCount;

    for (ULONG i = 0; i < DigestCount; i++)
    {
        entry->Digests[i] = Digests[i];
    }

    if (DigestCount != 0)
    {
        digestAlg = Digests[0].AlgorithmId;
        digestValue = Digests[0].Data;
        digestSize = Digests[0].Size;
    }
    else
    {
        digestAlg = 0;
        digestValue = NULL;
        digestSize = 0;
    }

    entry->FirstDigestAlg = digestAlg;
    entry->FirstDigest = digestValue;
    entry->FirstDigestSize = digestSize;
    entry->Details = EtWbclFormatEventData(EventType, Event, EventSize);

    PhAddItemList(Context->Entries, entry);

    // # column
    string = PhFormatUInt64(entry->Index, FALSE);
    lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, string->Buffer, entry);
    entry->ItemIndex = lvItemIndex;
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
    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, (PWSTR)EtWbclAlgorithmToString(digestAlg));

    // Digest value
    if (digestSize != 0)
    {
        string = PhBufferToHexString((PUCHAR)digestValue, digestSize);
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
    ET_TCG_EFI_SPECID_ALG algorithms[ET_WBCL_MAX_DIGESTS];
    ULONG algorithmCount = 0;
    ULONG records = 0;

    Context->LogAlgorithmCount = 0;

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
            if (first->EventType == SIPAEV_NO_ACTION &&
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
                    {
                        algorithms[i] = specId->DigestSizes[i];
                        Context->LogAlgorithms[i] = specId->DigestSizes[i].AlgorithmId;
                    }

                    Context->LogAlgorithmCount = count;
                }
            }

            {
                WBCL_DIGEST digest;

                digest.AlgorithmId = WBCL_DIGEST_ALG_ID_SHA_1;
                digest.Size = sizeof(first->Digest);
                digest.Data = first->Digest;

                EtWbclAddEntry(
                    Context,
                    first->PcrIndex,
                    first->EventType,
                    &digest,
                    1,
                    eventData,
                    eventSize
                    );
            }

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
        WBCL_DIGEST digests[ET_WBCL_MAX_DIGESTS];
        ULONG recordedDigests = 0;
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

                if (recordedDigests < ET_WBCL_MAX_DIGESTS)
                {
                    digests[recordedDigests].AlgorithmId = algId;
                    digests[recordedDigests].Size = digestSize;
                    digests[recordedDigests].Data = cursor;
                    recordedDigests++;
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
            digests[0].AlgorithmId = WBCL_DIGEST_ALG_ID_SHA_1;
            digests[0].Size = sizeof(record->Digest);
            digests[0].Data = record->Digest;
            recordedDigests = 1;
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
            digests,
            recordedDigests,
            eventData,
            eventSize
            );
        records++;
    }

    if (Context->LogAlgorithmCount == 0)
    {
        // A legacy log carries nothing but SHA1.
        Context->LogAlgorithms[0] = WBCL_DIGEST_ALG_ID_SHA_1;
        Context->LogAlgorithmCount = 1;
    }

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
    ExtendedListView_SortItems(Context->ListViewHandle);

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

// Reads a cached measured boot log from HKLM\SYSTEM\CurrentControlSet\Control\IntegrityServices.
static NTSTATUS EtWbclpGetRegistryLog(
    _In_ PCWSTR ValueName,
    _Out_ PVOID* Buffer,
    _Out_ PULONG Length
    )
{
    static CONST PH_STRINGREF integrityServicesKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Control\\IntegrityServices");
    NTSTATUS status;
    HANDLE keyHandle;
    PKEY_VALUE_PARTIAL_INFORMATION buffer;

    *Buffer = NULL;
    *Length = 0;

    status = PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &integrityServicesKeyName,
        0
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhQueryValueKeyZ(
        keyHandle,
        ValueName,
        KeyValuePartialInformation,
        &buffer
        );

    NtClose(keyHandle);

    if (!NT_SUCCESS(status))
        return status;

    if (buffer->Type != REG_BINARY)
    {
        PhFree(buffer);
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    if (buffer->DataLength == 0)
    {
        PhFree(buffer);
        return STATUS_NOT_FOUND;
    }

    if (buffer->DataLength > 32 * 1024 * 1024)
    {
        PhFree(buffer);
        return STATUS_FILE_TOO_LARGE;
    }

    *Buffer = PhAllocateCopy(buffer->Data, buffer->DataLength);
    *Length = buffer->DataLength;

    PhFree(buffer);

    return STATUS_SUCCESS;
}

// Fetches the measured boot log cached for the current boot.
NTSTATUS EtWbclGetRegistryLog(
    _Out_ PVOID* Buffer,
    _Out_ PULONG Length
    )
{
    return EtWbclpGetRegistryLog(L"WBCL", Buffer, Length);
}

// Fetches the measured boot log cached for the last cold boot.
NTSTATUS EtWbclGetRegistryColdbootLog(
    _Out_ PVOID* Buffer,
    _Out_ PULONG Length
    )
{
    return EtWbclpGetRegistryLog(L"WBCLColdboot", Buffer, Length);
}

//
// PCR replay.
//
// Replaying the log recomputes what the PCRs must contain if the log is an
// honest record of the boot: every measured event extends its PCR with
// PCR[i] = H(PCR[i] || digest). The result is compared against what the TPM
// itself reports, which is the only thing an attacker cannot forge. A log that
// parses cleanly but does not reproduce the TPM values has been altered,
// truncated, or does not belong to this boot.
//
// Only the SHA1 and SHA256 banks are replayed; phlib has no SHA384/SHA512
// implementation and no shipping platform uses those banks for measured boot.
//

#define ET_WBCL_COLUMN_REPLAY 7
#define ET_WBCL_COLUMN_STATE 8

BOOLEAN EtWbclReplayAlgorithm(
    _In_ USHORT AlgorithmId,
    _Out_ PH_HASH_ALGORITHM* Algorithm,
    _Out_ PUSHORT DigestSize
    )
{
    switch (AlgorithmId)
    {
    case WBCL_DIGEST_ALG_ID_SHA_1:
        *Algorithm = Sha1HashAlgorithm;
        *DigestSize = PH_HASH_SHA1_LENGTH;
        return TRUE;
    case WBCL_DIGEST_ALG_ID_SHA_2_256:
        *Algorithm = Sha256HashAlgorithm;
        *DigestSize = PH_HASH_SHA256_LENGTH;
        return TRUE;
    }

    *Algorithm = Sha256HashAlgorithm;
    *DigestSize = 0;
    return FALSE;
}

// Reads the selected bank from the TPM. Returns STATUS_NOT_SUPPORTED when there
// is no usable TPM, which the caller reports in the state column rather than
// treating as a failure of the log itself.
_Must_inspect_result_
NTSTATUS EtWbclReadPcrValues(
    _In_ USHORT AlgorithmId,
    _Out_writes_bytes_all_(TPM_PCR_COUNT * DigestSize) PVOID PcrValues,
    _In_ USHORT DigestSize,
    _Out_ PULONG ValidPcrMask
    )
{
    NTSTATUS status;
    TBS_HCONTEXT tbsContextHandle;

    *ValidPcrMask = 0;

    if (!EtTpmIsReady())
        return STATUS_NOT_SUPPORTED;

    if (!NT_SUCCESS(status = EtTpmOpen(&tbsContextHandle)))
        return status;

    status = EtTpmPcrRead(
        tbsContextHandle,
        AlgorithmId,
        PcrValues,
        DigestSize,
        ValidPcrMask
        );

    EtTpmClose(tbsContextHandle);

    return status;
}

// Refreshes the cached listview row index of every entry. The row a given entry
// occupies changes whenever the user sorts the list.
VOID EtWbclSyncItemIndexes(
    _In_ PWBCL_WINDOW_CONTEXT Context
    )
{
    LONG count = ListView_GetItemCount(Context->ListViewHandle);

    for (ULONG i = 0; i < Context->Entries->Count; i++)
    {
        ((PWBCL_ENTRY)Context->Entries->Items[i])->ItemIndex = INT_ERROR;
    }

    for (LONG i = 0; i < count; i++)
    {
        PWBCL_ENTRY entry;

        if (!PhGetListViewItemParam(Context->ListViewHandle, i, (PVOID*)&entry))
            continue;

        if (entry)
            entry->ItemIndex = i;
    }
}

// Replays the parsed log into the PCR values for the selected bank and
// annotates the listview: the running value after each extend, and on the final
// extend of each PCR whether the replay agrees with the TPM.
VOID EtWbclComputeReplay(
    _In_ PWBCL_WINDOW_CONTEXT Context
    )
{
    PH_HASH_ALGORITHM algorithm;
    USHORT digestSize;
    // Both banks are stored packed at the selected digest size, since that is
    // the layout EtTpmPcrRead fills.
    UCHAR replayed[TPM_PCR_COUNT * PH_HASH_SHA256_LENGTH];
    UCHAR reported[TPM_PCR_COUNT * PH_HASH_SHA256_LENGTH];
    PWBCL_ENTRY lastEntry[TPM_PCR_COUNT];
    ULONG validPcrMask = 0;
    NTSTATUS status;

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);

    // The rows were reordered by the listview sort after they were added, so
    // re-associate each entry with the row that carries it.

    EtWbclSyncItemIndexes(Context);

    for (ULONG i = 0; i < Context->Entries->Count; i++)
    {
        PWBCL_ENTRY entry = Context->Entries->Items[i];

        if (entry->ItemIndex == INT_ERROR)
            continue;

        PhSetListViewSubItem(Context->ListViewHandle, entry->ItemIndex, ET_WBCL_COLUMN_REPLAY, L"");
        PhSetListViewSubItem(Context->ListViewHandle, entry->ItemIndex, ET_WBCL_COLUMN_STATE, L"");
    }

    if (!EtWbclReplayAlgorithm(Context->ReplayAlgorithm, &algorithm, &digestSize))
    {
        ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
        return;
    }

    memset(replayed, 0, sizeof(replayed));
    memset(reported, 0, sizeof(reported));
    memset(lastEntry, 0, sizeof(lastEntry));

    // Entries are held in log order, which is the order the extends happened.
    // The listview sort does not affect this list.

    for (ULONG i = 0; i < Context->Entries->Count; i++)
    {
        PWBCL_ENTRY entry = Context->Entries->Items[i];
        PWBCL_DIGEST digest = NULL;
        PH_HASH_CONTEXT hashContext;
        PPH_STRING string;
        PUCHAR pcr;

        // EV_NO_ACTION records (the Spec ID header among them) are informational
        // and are never extended into a PCR.

        if (entry->EventType == SIPAEV_NO_ACTION)
            continue;

        if (entry->PcrIndex >= TPM_PCR_COUNT)
            continue;

        for (ULONG j = 0; j < entry->DigestCount; j++)
        {
            if (entry->Digests[j].AlgorithmId == Context->ReplayAlgorithm)
            {
                digest = &entry->Digests[j];
                break;
            }
        }

        if (!digest || digest->Size != digestSize)
            continue;

        pcr = replayed + (entry->PcrIndex * digestSize);

        // A hashing failure would leave every later value for this PCR wrong,
        // so abandon the replay rather than report a bogus mismatch.

        if (!NT_SUCCESS(PhInitializeHash(&hashContext, algorithm)) ||
            !NT_SUCCESS(PhUpdateHash(&hashContext, pcr, digestSize)) ||
            !NT_SUCCESS(PhUpdateHash(&hashContext, digest->Data, digest->Size)) ||
            !NT_SUCCESS(PhFinalHash(&hashContext, pcr, digestSize, NULL)))
        {
            ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
            return;
        }

        if (entry->ItemIndex != INT_ERROR)
        {
            string = PhBufferToHexString(pcr, digestSize);
            PhSetListViewSubItem(Context->ListViewHandle, entry->ItemIndex, ET_WBCL_COLUMN_REPLAY, string->Buffer);
            PhDereferenceObject(string);
        }

        lastEntry[entry->PcrIndex] = entry;
    }

    status = EtWbclReadPcrValues(Context->ReplayAlgorithm, reported, digestSize, &validPcrMask);

    for (ULONG i = 0; i < TPM_PCR_COUNT; i++)
    {
        PCWSTR state;

        if (!lastEntry[i] || lastEntry[i]->ItemIndex == INT_ERROR)
            continue;

        if (status == STATUS_NOT_SUPPORTED)
            state = L"No TPM";
        else if (!NT_SUCCESS(status))
            state = L"TPM read failed";
        else if (!BooleanFlagOn(validPcrMask, 1UL << i))
            state = L"Not reported";
        else if (memcmp(replayed + (i * digestSize), reported + (i * digestSize), digestSize) == 0)
            state = L"Match";
        else
            state = L"Mismatch";

        PhSetListViewSubItem(Context->ListViewHandle, lastEntry[i]->ItemIndex, ET_WBCL_COLUMN_STATE, (PWSTR)state);
    }

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

// Rebuilds the bank selector from the banks the log advertises, keeping the
// current selection when the new log still carries it and preferring SHA256.
VOID EtWbclUpdateBankList(
    _In_ PWBCL_WINDOW_CONTEXT Context
    )
{
    PH_HASH_ALGORITHM algorithm;
    USHORT digestSize;
    USHORT previous = Context->ReplayAlgorithm;
    LONG selected = -1;

    ComboBox_ResetContent(Context->BankHandle);
    Context->ReplayAlgorithm = 0;

    for (ULONG i = 0; i < Context->LogAlgorithmCount; i++)
    {
        USHORT algorithmId = Context->LogAlgorithms[i];
        LONG index;

        if (!EtWbclReplayAlgorithm(algorithmId, &algorithm, &digestSize))
            continue;

        index = ComboBox_AddString(Context->BankHandle, EtWbclAlgorithmToString(algorithmId));

        if (index == CB_ERR)
            continue;

        ComboBox_SetItemData(Context->BankHandle, index, algorithmId);

        if (algorithmId == previous)
            selected = index;
        else if (selected == -1 && algorithmId == WBCL_DIGEST_ALG_ID_SHA_2_256)
            selected = index;
    }

    if (selected == -1 && ComboBox_GetCount(Context->BankHandle) != 0)
        selected = 0;

    if (selected != -1)
    {
        ComboBox_SetCurSel(Context->BankHandle, selected);
        Context->ReplayAlgorithm = (USHORT)ComboBox_GetItemData(Context->BankHandle, selected);
    }

    EnableWindow(Context->BankHandle, Context->ReplayAlgorithm != 0);
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
        EtWbclUpdateBankList(Context);
        EtWbclComputeReplay(Context);
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
            EtWbclUpdateBankList(Context);
            EtWbclComputeReplay(Context);
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

static INT_PTR CALLBACK EtWbclGeneralDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWBCL_ENTRY entry;

    entry = (PWBCL_ENTRY)GetWindowLongPtr(WindowHandle, GWLP_USERDATA);

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            HWND listViewHandle;

            entry = (PWBCL_ENTRY)((PROPSHEETPAGE*)lParam)->lParam;
            SetWindowLongPtr(WindowHandle, GWLP_USERDATA, (LONG_PTR)entry);

            listViewHandle = GetDlgItem(WindowHandle, IDC_WBCL_PROP_LIST);

            PhSetListViewStyle(listViewHandle, TRUE, TRUE);
            PhAddListViewColumn(listViewHandle, 0, 0, 0, LVCFMT_LEFT, 120, L"Property");
            PhAddListViewColumn(listViewHandle, 1, 1, 1, LVCFMT_LEFT, 175, L"Value");
            PhSetExtendedListView(listViewHandle);

            PhAddListViewItem(listViewHandle, MAXINT, L"Index", NULL);
            PhSetListViewSubItem(listViewHandle, 0, 1, PH_AUTO_T(PH_STRING, PhFormatUInt64(entry->Index, FALSE))->Buffer);

            PhAddListViewItem(listViewHandle, MAXINT, L"PCR index", NULL);
            PhSetListViewSubItem(listViewHandle, 1, 1, PH_AUTO_T(PH_STRING, PhFormatUInt64(entry->PcrIndex, FALSE))->Buffer);

            PhAddListViewItem(listViewHandle, MAXINT, L"Event type", NULL);
            PhSetListViewSubItem(listViewHandle, 2, 1, PH_AUTO_T(PH_STRING, PhFormatString(
                L"%s (0x%08lx)",
                EtWbclEventTypeToString(entry->EventType) ? EtWbclEventTypeToString(entry->EventType) : L"Unknown",
                entry->EventType
                ))->Buffer);

            PhAddListViewItem(listViewHandle, MAXINT, L"Digest", NULL);
            PhSetListViewSubItem(listViewHandle, 3, 1, EtWbclAlgorithmToString(entry->FirstDigestAlg));

            PhAddListViewItem(listViewHandle, MAXINT, L"Digest value", NULL);

            if (entry->FirstDigestSize)
                PhSetListViewSubItem(listViewHandle, 4, 1, PH_AUTO_T(PH_STRING, PhBufferToHexString((PUCHAR)entry->FirstDigest, entry->FirstDigestSize))->Buffer);

            PhAddListViewItem(listViewHandle, MAXINT, L"Event size", NULL);
            PhSetListViewSubItem(listViewHandle, 5, 1, PH_AUTO_T(PH_STRING, PhFormatSize(entry->EventSize, ULONG_MAX))->Buffer);
        }
        return TRUE;
    case WM_NOTIFY:
        {
            if (((LPNMHDR)lParam)->code == PSN_SETACTIVE)
                return TRUE;
        }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK EtWbclRawDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PWBCL_ENTRY entry = (PWBCL_ENTRY)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    if (uMsg == WM_INITDIALOG)
    {
        entry = (PWBCL_ENTRY)((PROPSHEETPAGE*)lParam)->lParam;
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)entry);
        PH_STRING_BUILDER sb;
        PhInitializeStringBuilder(&sb, 0x200);
        if (entry->Details) PhAppendStringBuilder(&sb, &entry->Details->sr);
        PhAppendStringBuilder2(&sb, L"\r\n\r\nRaw event data:\r\n");
        if (entry->EventSize)
        {
            PPH_STRING hex = PhBufferToHexString((PUCHAR)entry->EventData, entry->EventSize);
            PhAppendStringBuilder(&sb, &hex->sr);
            PhDereferenceObject(hex);
        }
        PPH_STRING text = PhFinalStringBuilderString(&sb);
        SetDlgItemText(hwndDlg, IDC_WBCL_PROP_TEXT, text->Buffer);
        PhDereferenceObject(text);
        return TRUE;
    }
    return FALSE;
}

VOID EtWbclShowEntryProperties(_In_ HWND ParentWindowHandle, _In_ PWBCL_ENTRY Entry)
{
    PROPSHEETPAGE pages[2] = { 0 };
    HPROPSHEETPAGE handles[2];
    PROPSHEETHEADER header = { sizeof(header) };
    for (ULONG i = 0; i < 2; i++) { pages[i].dwSize = sizeof(PROPSHEETPAGE); pages[i].hInstance = PluginInstance->DllBase; pages[i].lParam = (LPARAM)Entry; }
    pages[0].pszTemplate = MAKEINTRESOURCE(IDD_WBCL_GENERAL); pages[0].pfnDlgProc = EtWbclGeneralDlgProc;
    pages[1].pszTemplate = MAKEINTRESOURCE(IDD_WBCL_RAW); pages[1].pfnDlgProc = EtWbclRawDlgProc;
    handles[0] = CreatePropertySheetPage(&pages[0]); handles[1] = CreatePropertySheetPage(&pages[1]);
    header.dwFlags = PSH_PROPTITLE | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
    header.hwndParent = ParentWindowHandle; header.hInstance = PluginInstance->DllBase; header.pszCaption = L"Boot log entry properties"; header.nPages = 2; header.phpage = handles;
    PhModalPropertySheet(&header);
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
            context->BankHandle = GetDlgItem(WindowHandle, IDC_WBCL_BANK);
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
            PhAddListViewColumn(context->ListViewHandle, 7, 7, 7, LVCFMT_LEFT, 260, L"PCR after event");
            PhAddListViewColumn(context->ListViewHandle, 8, 8, 8, LVCFMT_LEFT, 110, L"PCR state");
    PhSetExtendedListView(context->ListViewHandle);

            ExtendedListView_SetCompareFunction(context->ListViewHandle, 0, EtWbclIndexCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 1, EtWbclPcrCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 5, EtWbclSizeCompareFunction);

            PhLoadListViewColumnsFromSetting(SETTING_NAME_WBCL_LISTVIEW_COLUMNS, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_WBCL_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_WBCL_OPENFILE), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, context->BankHandle, NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
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
            case IDC_WBCL_BANK:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        LONG index = ComboBox_GetCurSel(context->BankHandle);

                        if (index != CB_ERR)
                        {
                            context->ReplayAlgorithm = (USHORT)ComboBox_GetItemData(context->BankHandle, index);
                            EtWbclComputeReplay(context);
                        }
                    }
                }
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
                            EtWbclShowEntryProperties(context->WindowHandle, entry);
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
                                EtWbclShowEntryProperties(context->WindowHandle, (PWBCL_ENTRY)listviewItems[0]);
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
