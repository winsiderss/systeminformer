#ifndef _FVEAPI_H
#define _FVEAPI_H

#include <bcrypt.h>
#include <ncrypt.h>
#include <wincrypt.h>

EXTERN_C_START

/**
 * Pointer to a constant byte.
 */
typedef const BYTE *PCBYTE;

#define FVE_TPM_INFO_VERSION_1 1

/**
 * Describes a UEFI variable and its value used during predictive TPM sealing.
 */
typedef struct _FVE_UEFI_VARIABLE_INFO
{
    PBYTE UEFIVariableValue;
    ULONG UEFIVariableSizeBytes;
} FVE_UEFI_VARIABLE_INFO, *PFVE_UEFI_VARIABLE_INFO;

/**
 * Describes the UEFI Secure Boot state used to predict the PCR[7] measurement.
 */
typedef struct _FVE_TPM_PCR7_INFO
{
    PFVE_UEFI_VARIABLE_INFO PlatformKeyVariableInfo;
    PFVE_UEFI_VARIABLE_INFO KekDatabaseVariableInfo;
    PFVE_UEFI_VARIABLE_INFO AllowedDatabaseVariableInfo;
    PFVE_UEFI_VARIABLE_INFO ForbiddenDatabaseVariableInfo;
    PBYTE OsLoaderAuthoritySignature;
    ULONG OsLoaderAuthoritySignatureSizeBytes;
    ULONG CountSeparatorEvents;
} FVE_TPM_PCR7_INFO, *PFVE_TPM_PCR7_INFO;

/**
 * Describes the boot manager path used to predict the PCR[4] measurement.
 */
typedef struct _FVE_TPM_PCR4_INFO
{
    WCHAR BootMgrFilePath[MAX_PATH];
} FVE_TPM_PCR4_INFO, *PFVE_TPM_PCR4_INFO;

/**
 * Describes a predictive TPM protector, including the PCR index and its predicted seal information.
 */
typedef struct _FVE_TPM_PROTECTOR_INFO
{
    UINT32 TpmPcrIndex;
    union
    {
        PFVE_TPM_PCR7_INFO FveTpmPcr7Info;
        PFVE_TPM_PCR4_INFO FveTpmPcr4Info;
    } PredictiveSealInfo;
} FVE_TPM_PROTECTOR_INFO, *PFVE_TPM_PROTECTOR_INFO;

/**
 * Describes the predicted TPM state used when sealing a key to the TPM.
 */
typedef struct _FVE_TPM_STATE_
{
    PVOID TpmContext;
    ULONG FveTpmProtectorInfoCount;
    PFVE_TPM_PROTECTOR_INFO FveTpmProtectorInfo;
} FVE_TPM_STATE, *PFVE_TPM_STATE;

/**
 * Describes the predictive TPM information supplied when adding a TPM protector.
 */
typedef struct _FVE_TPM_INFO_
{
    ULONG FveTpmInfoVersion;
    PFVE_TPM_STATE TpmStateInfo;
} FVE_TPM_INFO, *PFVE_TPM_INFO;

/**
 * The PFVE_TPM_API_CALLBACK callback routine communicates a command to the TPM and returns its result.
 */
typedef HRESULT (NTAPI *PFVE_TPM_API_CALLBACK)(
    _In_ PVOID hContext,
    _In_ UINT32 cbCmd,
    _In_reads_bytes_(cbCmd) const BYTE *pabCmd,
    _Inout_ PUINT32 pcbResult,
    _Out_writes_bytes_(*pcbResult) PBYTE pabResult
    );

/**
 * The FveAddPredictiveTpmProtector routine adds a predictive TPM key protector to the specified volume.
 *
 * \param[in] FveVolumePath The path of the FVE volume.
 * \param[in] FveTpmInfo The predictive TPM protector information.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveAddPredictiveTpmProtector(
    _In_ PCWSTR FveVolumePath,
    _In_ PFVE_TPM_INFO FveTpmInfo
    );

/**
 * The FveSetupTpmCallback routine registers a callback used to communicate with the TPM.
 *
 * \param[in] TpmCallback The TPM API callback routine to register.
 * \param[in] TpmVersion The TPM version implemented by the callback.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 * \remarks Versions 0, 1 and 2 are accepted; version zero queries the platform TPM version.
 */
NTSYSAPI
HRESULT
NTAPI
FveSetupTpmCallback(
    _In_ PFVE_TPM_API_CALLBACK TpmCallback,
    _In_ UINT32 TpmVersion
    );

/**
 * Opaque HSTI results used by the NGSCB device-encryption checks.
 */
typedef struct _NGSCB_HSTI_RESULTS NGSCB_HSTI_RESULTS, *PNGSCB_HSTI_RESULTS;

/**
 * Opaque name/value collection returned by the NGSCB device-encryption checks.
 */
typedef struct _NGSCB_NAME_VALUE_COLLECTION NGSCB_NAME_VALUE_COLLECTION, *PNGSCB_NAME_VALUE_COLLECTION;

/**
 * Opaque HSTI parsing status. Its complete layout remains unrecovered.
 */
typedef struct _NGSCB_HSTI_PARSING_STATUS NGSCB_HSTI_PARSING_STATUS, *PNGSCB_HSTI_PARSING_STATUS;

/**
 * Forward declaration of the predictions-updated context structure.
 */
typedef struct _PPF_PREDICTIONS_UPDATED_CONTEXT PPF_PREDICTIONS_UPDATED_CONTEXT, *PPPF_PREDICTIONS_UPDATED_CONTEXT;

/**
 * Identifies the type of a device that BitLocker can operate on.
 */
typedef enum _FVE_DEVICE_TYPE
{
    FVE_DEVICE_UNKNOWN = -1,
    FVE_DEVICE_UNSUPPORTED = 0,
    FVE_DEVICE_VOLUME = 1,
    FVE_DEVICE_CSV_VOLUME = 2,
    FVE_DEVICE_MAX
} FVE_DEVICE_TYPE, *PFVE_DEVICE_TYPE;

/**
 * Identifies the FVE interface used to open a volume.
 */
typedef enum _FVE_INTERFACE_TYPE
{
    FVE_INTERFACE_UNKNOWN = -1,
    FVE_INTERFACE_SEI = 0,
    FVE_INTERFACE_SYS = 1,
    FVE_INTERFACE_HEI = 2,
    FVE_INTERFACE_MAX
} FVE_INTERFACE_TYPE, *PFVE_INTERFACE_TYPE;

/**
 * Identifies the type of a handle passed to the FVE API.
 */
typedef enum _FVE_HANDLE_TYPE
{
    FVE_HANDLE_UNKNOWN = -1,
    FVE_HANDLE_FVE = 0,
    FVE_HANDLE_NONFVE = 1,
    FVE_HANDLE_MAX
} FVE_HANDLE_TYPE, *PFVE_HANDLE_TYPE;

/**
 * Identifies the scenario under which pending FVE changes are committed.
 */
typedef enum _FVE_SCENARIO_TYPE
{
    FVE_SCENARIO_UNKNOWN = -1,
    FVE_SCENARIO_DEFAULT = 0,
    FVE_SCENARIO_KEY_ROLL = 1,
    FVE_SCENARIO_BOOT_COMPONENT_UPDATE = 2,
    FVE_SCENARIO_UNDEFINED_SKIP_CHECKS = 3,
    FVE_SCENARIO_POLICY_BASED_ENABLEMENT = 4,
    FVE_SCENARIO_PUSH_BUTTON_RESET = 5,
    FVE_SCENARIO_UPGRADE = 6,
    FVE_SCENARIO_DEVICE_LOCKOUT_LOCK = 7,
    FVE_SCENARIO_DEVICE_LOCKOUT_RECOVER = 8,
    FVE_SCENARIO_PPF_PREDICTIONS_UPDATED = 9,
    FVE_SCENARIO_DEVICE_ENCRYPTION = 10,
    FVE_SCENARIO_TMCORE_PROVISIONING = 11,
    FVE_COMMIT_SCENARIO_WINRE_TRUST_REVOKE = 14,
    FVE_COMMIT_SCENARIO_WINRE_TRUST_REESTABLISH = 15,
    FVE_COMMIT_SCENARIO_WINRE_TRUST_UPDATE = 16
} FVE_SCENARIO_TYPE, *PFVE_SCENARIO_TYPE;

#define FVE_STATUS_VERSION_1 1
#define FVE_STATUS_VERSION_2 2
#define FVE_STATUS_VERSION_3 3
#define FVE_STATUS_VERSION_4 4
#define FVE_STATUS_VERSION_5 5
#define FVE_STATUS_VERSION_6 6
#define FVE_STATUS_VERSION_7 7
#define FVE_STATUS_VERSION_8 8
#define FVE_STATUS_VERSION_9 9
#define FVE_STATUS_FLAG_INITIALIZED 0x00000001UL
#define FVE_STATUS_FLAG_FULLY_DECRYPTED 0x00000004UL
#define FVE_STATUS_FLAG_FULLY_ENCRYPTED 0x00000008UL
#define FVE_STATUS_FLAG_DECRYPTION_IN_PROGRESS 0x00000010UL
#define FVE_STATUS_FLAG_ENCRYPTION_IN_PROGRESS 0x00000020UL
#define FVE_STATUS_FLAG_CONVERSION_PAUSED_MASK 0x000000C0UL
#define FVE_STATUS_FLAG_NON_TPM_PROTECTOR 0x00000100UL
#define FVE_STATUS_FLAG_TPM_PROTECTOR 0x00000200UL
#define FVE_STATUS_FLAG_CLEAR_KEY 0x00000400UL
#define FVE_STATUS_FLAG_LOCKED 0x00000800UL
#define FVE_STATUS_FLAG_PROTECTION_ACTIVE 0x00001000UL
#define FVE_STATUS_FLAG_OS_VOLUME 0x00004000UL
#define FVE_STATUS_FLAG_EXTERNAL_KEY_PROTECTOR 0x00020000UL
#define FVE_STATUS_FLAG_RECOVERY_PASSWORD_PROTECTOR 0x00040000UL
#define FVE_STATUS_FLAG_TPM_PIN_PROTECTOR 0x00080000UL
#define FVE_STATUS_FLAG_TPM_STARTUP_KEY_PROTECTOR 0x00100000UL
#define FVE_STATUS_FLAG_PASSPHRASE_PROTECTOR 0x00200000UL
#define FVE_STATUS_FLAG_REMOVABLE_DATA_VOLUME 0x00400000UL
#define FVE_STATUS_FLAG_CERTIFICATE_PROTECTOR 0x00800000UL
#define FVE_STATUS_FLAG_DATA_ONLY_ENCRYPTION 0x01000000UL
#define FVE_STATUS_FLAG_INITIALIZATION_UNKNOWN100 0x10000000UL
#define FVE_CONVERSION_FLAG_DATA_ONLY 0x00000001UL
#define FVE_INITIALIZATION_UNKNOWN100 0x00000100UL

/**
 * Describes the encryption status of a volume (version 1).
 */
typedef struct _FVE_STATUS_V1
{
    ULONG Size;
    ULONG Version;
    ULONG Flags;
    DOUBLE ConvertedPercent;
    HRESULT LastConvertStatus;
} FVE_STATUS_V1, *PFVE_STATUS_V1;

/**
 * Pointer to a constant FVE_STATUS_V1 structure.
 */
typedef const FVE_STATUS_V1 *PCFVE_STATUS_V1;

/**
 * Describes the encryption status of a volume (version 2).
 */
typedef struct _FVE_STATUS_V2
{
    ULONG Size;                 // sizeof(FVE_STATUS_V2)
    ULONG Version;              // 2
    USHORT FveVersion;
    ULONG Flags;
    DOUBLE ConvertedPercent;
    HRESULT LastConvertStatus;
} FVE_STATUS_V2, *PFVE_STATUS_V2;

/**
 * Pointer to a constant FVE_STATUS_V2 structure.
 */
typedef const FVE_STATUS_V2 *PCFVE_STATUS_V2;

/**
 * Describes the encryption status of a volume (version 3).
 */
typedef struct _FVE_STATUS_V3
{
    ULONG Size;             // sizeof(FVE_STATUS_V3)
    ULONG Version;          // 3
    USHORT FveVersion;
    ULONG Flags;
    DOUBLE ConvertedPercent;
    HRESULT LastConvertStatus;
    LONGLONG VolArriveTime;
} FVE_STATUS_V3, *PFVE_STATUS_V3;

/**
 * Pointer to a constant FVE_STATUS_V3 structure.
 */
typedef const FVE_STATUS_V3 *PCFVE_STATUS_V3;

/**
 * Describes the encryption status of a volume (version 4).
 */
typedef struct _FVE_STATUS_V4
{
    ULONG Size;
    ULONG Version;
    USHORT FveVersion;
    ULONG Flags;
    DOUBLE ConvertedPercent;
    HRESULT LastConvertStatus;
    LONGLONG VolArriveTime;
    DOUBLE WipedPercent;
    ULONG WipeState;
    ULONG WipeCount;
    ULONGLONG ExtendedFlags;
} FVE_STATUS_V4, *PFVE_STATUS_V4;

/**
 * Pointer to a constant FVE_STATUS_V4 structure.
 */
typedef const FVE_STATUS_V4 *PCFVE_STATUS_V4;

/**
 * Describes the encryption status of a volume (version 5).
 */
typedef struct _FVE_STATUS_V5
{
    ULONG Size;
    ULONG Version;
    USHORT FveVersion;
    ULONG Flags;
    DOUBLE ConvertedPercent;
    HRESULT LastConvertStatus;
    LONGLONG VolArriveTime;
    DOUBLE WipedPercent;
    ULONG WipeState;
    ULONG WipeCount;
    ULONGLONG ExtendedFlags;
    ULONGLONG WimBootHashedSizeRequired;
    ULONGLONG WimBootHashedSizeActual;
    union
    {
        ULONGLONG ExtendedFlags2;
        struct
        {
            BOOLEAN WimBootVolume : 1;
            BOOLEAN WimBootHashCompleted : 1;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} FVE_STATUS_V5, *PFVE_STATUS_V5;

/**
 * Pointer to a constant FVE_STATUS_V5 structure.
 */
typedef const FVE_STATUS_V5 *PCFVE_STATUS_V5;

/**
 * Describes the encryption status of a volume (version 6).
 */
typedef struct _FVE_STATUS_V6
{
    ULONG Size;
    ULONG Version;
    USHORT FveVersion;
    ULONG Flags;
    DOUBLE ConvertedPercent;
    HRESULT LastConvertStatus;
    LONGLONG VolArriveTime;
    DOUBLE WipedPercent;
    ULONG WipeState;
    ULONG WipeCount;
    ULONGLONG ExtendedFlags;
    ULONGLONG WimBootHashedSizeRequired;
    ULONGLONG WimBootHashedSizeActual;
    union
    {
        ULONGLONG ExtendedFlags2;
        struct
        {
            BOOLEAN WimBootVolume : 1;
            BOOLEAN WimBootHashCompleted : 1;
            BOOLEAN IceIsUsedForFve : 1;
            BOOLEAN IsEfiEsp : 1;
            BOOLEAN IsRecovery : 1;
            BOOLEAN WcosDePolicy : 1;
            BOOLEAN WcosOsData : 1;
            BOOLEAN WcosPreInstalled : 1;
            BOOLEAN WcosUserData : 1;
            BOOLEAN WcosMainOs : 1;
            BOOLEAN WcosEfiEsp : 1;
            BOOLEAN WcosBsp : 1;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG WcosOsMainProtectLevel;
    ULONG WcosOsDataProtectLevel;
    ULONG WcosPreInstalledProtectLevel;
    ULONG WcosUserDataProtectLevel;
} FVE_STATUS_V6, *PFVE_STATUS_V6;

/**
 * Pointer to a constant FVE_STATUS_V6 structure.
 */
typedef const FVE_STATUS_V6 *PCFVE_STATUS_V6;

/**
 * Describes the encryption status of a volume (version 7).
 */
typedef struct _FVE_STATUS_V7
{
    ULONG Size;
    ULONG Version;
    USHORT FveVersion;
    ULONG Flags;
    DOUBLE ConvertedPercent;
    HRESULT LastConvertStatus;
    LONGLONG VolArriveTime;
    DOUBLE WipedPercent;
    ULONG WipeState;
    ULONG WipeCount;
    ULONGLONG ExtendedFlags;
    ULONGLONG WimBootHashedSizeRequired;
    ULONGLONG WimBootHashedSizeActual;
    union
    {
        ULONGLONG ExtendedFlags2;
        struct
        {
            BOOLEAN WimBootVolume : 1;
            BOOLEAN WimBootHashCompleted : 1;
            BOOLEAN IceIsUsedForFve : 1;
            BOOLEAN IsEfiEsp : 1;
            BOOLEAN IsRecovery : 1;
            BOOLEAN WcosDePolicy : 1;
            BOOLEAN WcosOsData : 1;
            BOOLEAN WcosPreInstalled : 1;
            BOOLEAN WcosUserData : 1;
            BOOLEAN WcosMainOs : 1;
            BOOLEAN WcosEfiEsp : 1;
            BOOLEAN WcosBsp : 1;
            BOOLEAN WcosWsp : 1;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG WcosOsMainProtectLevel;
    ULONG WcosOsDataProtectLevel;
    ULONG WcosPreInstalledProtectLevel;
    ULONG WcosUserDataProtectLevel;
    ULONG WcosBspProtectLevel;
    ULONG WcosWspProtectLevel;
} FVE_STATUS_V7, *PFVE_STATUS_V7;

/**
 * Pointer to a constant FVE_STATUS_V7 structure.
 */
typedef const FVE_STATUS_V7 *PCFVE_STATUS_V7;

/**
 * Describes the encryption status of a volume (version 8).
 */
typedef struct _FVE_STATUS_V8
{
    ULONG Size;
    ULONG Version;
    USHORT FveVersion;
    ULONG Flags;
    DOUBLE ConvertedPercent;
    HRESULT LastConvertStatus;
    LONGLONG VolArriveTime;
    DOUBLE WipedPercent;
    ULONG WipeState;
    ULONG WipeCount;
    ULONGLONG ExtendedFlags;
    ULONGLONG WimBootHashedSizeRequired;
    ULONGLONG WimBootHashedSizeActual;
    union
    {
        ULONGLONG ExtendedFlags2;
        struct
        {
            BOOLEAN WimBootVolume : 1;
            BOOLEAN WimBootHashCompleted : 1;
            BOOLEAN IceIsUsedForFve : 1;
            BOOLEAN IsEfiEsp : 1;
            BOOLEAN IsRecovery : 1;
            BOOLEAN WcosDePolicy : 1;
            BOOLEAN WcosOsData : 1;
            BOOLEAN WcosPreInstalled : 1;
            BOOLEAN WcosUserData : 1;
            BOOLEAN WcosMainOs : 1;
            BOOLEAN WcosEfiEsp : 1;
            BOOLEAN WcosBsp : 1;
            BOOLEAN WcosWsp : 1;
            BOOLEAN WcosDpp : 1;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG WcosOsMainProtectLevel;
    ULONG WcosOsDataProtectLevel;
    ULONG WcosPreInstalledProtectLevel;
    ULONG WcosUserDataProtectLevel;
    ULONG WcosBspProtectLevel;
    ULONG WcosWspProtectLevel;
    ULONG WcosDppProtectLevel;
} FVE_STATUS_V8, *PFVE_STATUS_V8;

/**
 * Pointer to a constant FVE_STATUS_V8 structure.
 */
typedef const FVE_STATUS_V8 *PCFVE_STATUS_V8;

/**
 * Describes the encryption status of a volume (version 9).
 */
typedef struct _FVE_STATUS_V9
{
    ULONG Size;
    ULONG Version;
    USHORT FveVersion;
    ULONG Flags;
    DOUBLE ConvertedPercent;
    HRESULT LastConvertStatus;
    LONGLONG VolArriveTime;
    DOUBLE WipedPercent;
    ULONG WipeState;
    ULONG WipeCount;
    ULONGLONG ExtendedFlags;
    ULONGLONG WimBootHashedSizeRequired;
    ULONGLONG WimBootHashedSizeActual;
    union
    {
        ULONGLONG ExtendedFlags2;
        struct
        {
            BOOLEAN WimBootVolume : 1;
            BOOLEAN WimBootHashCompleted : 1;
            BOOLEAN IceIsUsedForFve : 1;
            BOOLEAN IsEfiEsp : 1;
            BOOLEAN IsRecovery : 1;
            BOOLEAN WcosDePolicy : 1;
            BOOLEAN WcosOsData : 1;
            BOOLEAN WcosPreInstalled : 1;
            BOOLEAN WcosUserData : 1;
            BOOLEAN WcosMainOs : 1;
            BOOLEAN WcosEfiEsp : 1;
            BOOLEAN WcosBsp : 1;
            BOOLEAN WcosWsp : 1;
            BOOLEAN WcosDpp : 1;
            BOOLEAN WcosServicingMetadata : 1;
            BOOLEAN WcosServicingFiles : 1;
            BOOLEAN WcosServicingReserve : 1;
            BOOLEAN IsOnRdvPolicyExclusionList : 1;
            BOOLEAN IsIceDirectKeyTypeSupported : 1;
            BOOLEAN IsIcePlatformWrappedKeyTypeSupported : 1;
            BOOLEAN IsIcePlutonWrappedKeyTypeSupported : 1;
            BOOLEAN IsIceDirectKeyTypeUsed : 1;
            BOOLEAN IsIcePlatformWrappedKeyTypeUsed : 1;
            BOOLEAN IsIcePlutonWrappedKeyTypeUsed : 1;
            BOOLEAN IceTypeIsUfs : 1;
            BOOLEAN IceTypeIsNvme : 1;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG WcosOsMainProtectLevel;
    ULONG WcosOsDataProtectLevel;
    ULONG WcosPreInstalledProtectLevel;
    ULONG WcosUserDataProtectLevel;
    ULONG WcosBspProtectLevel;
    ULONG WcosWspProtectLevel;
    ULONG WcosDppProtectLevel;
    ULONG WcosServicingMetadataProtectLevel;
    ULONG WcosServicingFilesProtectLevel;
    ULONG WcosServicingReserveProtectLevel;
} FVE_STATUS_V9, *PFVE_STATUS_V9;

/**
 * Pointer to a constant FVE_STATUS_V9 structure.
 */
typedef const FVE_STATUS_V9 *PCFVE_STATUS_V9;

/**
 * Identifies the free-space wiping state of a volume.
 */
typedef enum _FVE_WIPING_STATE
{
    FVE_WIPING_STATE_UNSPECIFIED = 0,
    FVE_WIPING_STATE_INACTIVE = 1,
    FVE_WIPING_STATE_PENDING = 2,
    FVE_WIPING_STATE_STOPPED = 3,
    FVE_WIPING_STATE_INPROGRESS = 4
} FVE_WIPING_STATE, *PFVE_WIPING_STATE;

#define FVE_TPM_CAPS_VERSION_1 1
#define FVE_TPM_CAPS_VERSION_2 2

/**
 * Describes the capabilities of the platform TPM.
 */
typedef struct _FVE_TPM_CAPS
{
    ULONG Size;
    ULONG Version;
    HRESULT TpmStatus;
    ULONG Flags;
} FVE_TPM_CAPS, *PFVE_TPM_CAPS;

/**
 * Pointer to a constant FVE_TPM_CAPS structure.
 */
typedef const FVE_TPM_CAPS *PCFVE_TPM_CAPS;

/**
 * Describes whether a TPM is present on the platform.
 */
typedef struct _FVE_TPM_CAPS_TPM_PRESENCE
{
    ULONG Size;
    ULONG Version;
    HRESULT NotUsed;
    ULONG NotUsed2;
    BOOL TpmPresent;
} FVE_TPM_CAPS_TPM_PRESENCE, *PFVE_TPM_CAPS_TPM_PRESENCE;

/**
 * Pointer to a constant FVE_TPM_CAPS_TPM_PRESENCE structure.
 */
typedef const FVE_TPM_CAPS_TPM_PRESENCE *PCFVE_TPM_CAPS_TPM_PRESENCE;

/**
 * Identifies the encryption algorithm used to protect a volume.
 */
typedef enum _FVE_METHOD
{
    FveMethodWcos = -2,
    FveMethodUnknown = -1,
    FveMethodNone = 0,
    FveMethodAesWithDiffuser = 1,
    FveMethodAes = 3,
    FveMethodEdrive = 5,
    FveMethodXtsAes = 6
} FVE_METHOD, *PFVE_METHOD;

/**
 * Identifies the key strength of the encryption algorithm.
 */
typedef enum _FVE_METHOD_STRENGTH
{
    FveMethodStrengthNone = 0,
    FveMethodStrength128 = 1,
    FveMethodStrength256 = 2
} FVE_METHOD_STRENGTH, *PFVE_METHOD_STRENGTH;

/**
 * Identifies the combined encryption algorithm and key strength used by the legacy method APIs.
 */
typedef enum _FVE_LEGACY_METHOD
{
    FveLegacyMethodWcos = -2,
    FveLegacyMethodUnknown = -1,
    FveLegacyMethodNone = 0,
    FveLegacyMethodAes128WithDiffuser,
    FveLegacyMethodAes256WithDiffuser,
    FveLegacyMethodAes128,
    FveLegacyMethodAes256,
    FveLegacyMethodHardware,
    FveLegacyMethodXtsAes128,
    FveLegacyMethodXtsAes256
} FVE_LEGACY_METHOD, *PFVE_LEGACY_METHOD;

/**
 * The MapFveLegacyMethodToMethod routine maps a legacy FVE method value to the corresponding method and strength.
 *
 * \param[in] FveLegacyMethod The legacy FVE method identifier.
 * \param[out] FveMethod Receives the FVE encryption method.
 * \param[out] FveMethodStrength Receives the FVE encryption method strength.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
FORCEINLINE
HRESULT
MapFveLegacyMethodToMethod(
    _In_ LONG FveLegacyMethod,
    _Out_ PFVE_METHOD FveMethod,
    _Out_ PFVE_METHOD_STRENGTH FveMethodStrength
    )
{
    HRESULT Hr = S_OK;
    FVE_METHOD LocalMethod = FveMethodNone;
    FVE_METHOD_STRENGTH LocalMethodStrength = FveMethodStrengthNone;

    switch (FveLegacyMethod)
    {
    case 0:
        LocalMethod = FveMethodNone;
        break;
    case 1:
    case 2:
        LocalMethod = FveMethodAesWithDiffuser;
        break;
    case 3:
    case 4:
        LocalMethod = FveMethodAes;
        break;
    case 6:
    case 7:
        LocalMethod = FveMethodXtsAes;
        break;
    case 5:
        LocalMethod = FveMethodEdrive;
        break;
    case -1:
        LocalMethod = FveMethodUnknown;
        break;
    case -2:
        LocalMethod = FveMethodWcos;
        break;
    default:
        return E_INVALIDARG;
    }

    switch (FveLegacyMethod)
    {
    case 0:
    case -1:
    case -2:
    case 5:
        LocalMethodStrength = FveMethodStrengthNone;
        break;
    case 1:
    case 3:
    case 6:
        LocalMethodStrength = FveMethodStrength128;
        break;
    case 2:
    case 4:
    case 7:
        LocalMethodStrength = FveMethodStrength256;
        break;
    default:
        return E_INVALIDARG;
    }

    *FveMethod = LocalMethod;
    *FveMethodStrength = LocalMethodStrength;
    
    return Hr;
}

/**
 * Contains a recovery-password authentication element.
 */
typedef struct _FVE_AUTH_RECOVERY_PASSWORD
{
    USHORT Block[8];
} FVE_AUTH_RECOVERY_PASSWORD, *PFVE_AUTH_RECOVERY_PASSWORD;

/**
 * Pointer to a constant FVE_AUTH_RECOVERY_PASSWORD structure.
 */
typedef const FVE_AUTH_RECOVERY_PASSWORD *PCFVE_AUTH_RECOVERY_PASSWORD;

/**
 * Contains a PIN authentication element.
 */
typedef struct _FVE_AUTH_PIN
{
    BYTE HashedPin[32];
} FVE_AUTH_PIN, *PFVE_AUTH_PIN;

/**
 * Pointer to a constant FVE_AUTH_PIN structure.
 */
typedef const FVE_AUTH_PIN *PCFVE_AUTH_PIN;

/**
 * Contains a TPM authentication element, including the PCR bitmap.
 */
typedef struct _FVE_AUTH_TPM
{
    ULONG PcrBitmap;
    GUID PcrBitmapScenarioId;
} FVE_AUTH_TPM, *PFVE_AUTH_TPM;

/**
 * Pointer to a constant FVE_AUTH_TPM structure.
 */
typedef const FVE_AUTH_TPM *PCFVE_AUTH_TPM;

/**
 * Contains a predicted TPM authentication element.
 */
typedef struct _FVE_AUTH_PREDICTED_TPM_INFO
{
    PFVE_TPM_STATE FveTpmState;
} FVE_AUTH_PREDICTED_TPM_INFO, *PFVE_AUTH_PREDICTED_TPM_INFO;

/**
 * Pointer to a constant FVE_AUTH_PREDICTED_TPM_INFO structure.
 */
typedef const FVE_AUTH_PREDICTED_TPM_INFO *PCFVE_AUTH_PREDICTED_TPM_INFO;

/**
 * Contains an external key authentication element.
 */
typedef struct _FVE_AUTH_EXTERNAL_KEY
{
    BYTE Key[32];
} FVE_AUTH_EXTERNAL_KEY, *PFVE_AUTH_EXTERNAL_KEY;

/**
 * Pointer to a constant FVE_AUTH_EXTERNAL_KEY structure.
 */
typedef const FVE_AUTH_EXTERNAL_KEY *PCFVE_AUTH_EXTERNAL_KEY;

/**
 * Contains a public key authentication element.
 */
typedef struct _FVE_AUTH_PUBLIC_KEY
{
    BCRYPT_KEY_HANDLE Handle;
    ULONG BlobSize;
    PBYTE Blob;
} FVE_AUTH_PUBLIC_KEY, *PFVE_AUTH_PUBLIC_KEY;

/**
 * Pointer to a constant FVE_AUTH_PUBLIC_KEY structure.
 */
typedef const FVE_AUTH_PUBLIC_KEY *PCFVE_AUTH_PUBLIC_KEY;

/**
 * Contains a private key authentication element.
 */
typedef struct _FVE_AUTH_PRIVATE_KEY
{
    NCRYPT_KEY_HANDLE KspKeyHandle;
    HCRYPTPROV CspProviderHandle;
    HCRYPTKEY CspKeyHandle;
    ULONG KeySpec;
} FVE_AUTH_PRIVATE_KEY, *PFVE_AUTH_PRIVATE_KEY;

/**
 * Pointer to a constant FVE_AUTH_PRIVATE_KEY structure.
 */
typedef const FVE_AUTH_PRIVATE_KEY *PCFVE_AUTH_PRIVATE_KEY;

/**
 * Describes the layout of a public key (certificate) authentication element.
 */
typedef struct _FVE_AUTH_INFO_PUBLIC_KEY
{
    ULONG ExportedPublicKeySize;
    ULONG ExportedPublicKeyOffset;
    ULONG BlobSize;
    ULONG BlobOffset;
} FVE_AUTH_INFO_PUBLIC_KEY, *PFVE_AUTH_INFO_PUBLIC_KEY;

/**
 * Pointer to a constant FVE_AUTH_INFO_PUBLIC_KEY structure.
 */
typedef const FVE_AUTH_INFO_PUBLIC_KEY *PCFVE_AUTH_INFO_PUBLIC_KEY;

#define FVE_AUTH_PASSPHRASE_MAX_LENGTH 256

/**
 * Contains a passphrase authentication element.
 */
typedef struct _FVE_AUTH_PASSPHRASE
{
    WCHAR ClearPassPhrase[FVE_AUTH_PASSPHRASE_MAX_LENGTH + 1];
    BYTE HashedPassPhrase[32];
    BYTE Salt[16];
} FVE_AUTH_PASSPHRASE, *PFVE_AUTH_PASSPHRASE;

/**
 * Pointer to a constant FVE_AUTH_PASSPHRASE structure.
 */
typedef const FVE_AUTH_PASSPHRASE *PCFVE_AUTH_PASSPHRASE;

/**
 * Describes a clear-key authentication element (suspended protection).
 */
typedef struct _FVE_AUTH_INFO_CLEAR_KEY
{
    UCHAR Count;
} FVE_AUTH_INFO_CLEAR_KEY, *PFVE_AUTH_INFO_CLEAR_KEY;

/**
 * Contains a DPAPI-NG authentication element.
 */
typedef struct _FVE_AUTH_DPAPI_NG
{
    USHORT DpapiNgFlags;
    USHORT DescriptorLength;
    WCHAR DpapiNgDescriptor[ANYSIZE_ARRAY];
} FVE_AUTH_DPAPI_NG, *PFVE_AUTH_DPAPI_NG;

/**
 * Pointer to a constant FVE_AUTH_DPAPI_NG structure.
 */
typedef const FVE_AUTH_DPAPI_NG *PCFVE_AUTH_DPAPI_NG;

/**
 * Contains a network-unlock server authentication element.
 */
typedef struct _FVE_AUTH_NETWORK_SERVER_INFO
{
    WCHAR LocalIPAddress[65];
    ULONG ServerIPAddressesCount;
    ULONG ServerIPAddressesSize;
    WCHAR ServerIPAddresses[ANYSIZE_ARRAY][65];
} FVE_AUTH_NETWORK_SERVER_INFO, *PFVE_AUTH_NETWORK_SERVER_INFO;

/**
 * Pointer to a constant FVE_AUTH_NETWORK_SERVER_INFO structure.
 */
typedef const FVE_AUTH_NETWORK_SERVER_INFO *PCFVE_AUTH_NETWORK_SERVER_INFO;

// FVE_AUTH_ELEMENT ElementFlags
/**
 * Flags describing an authentication element (FVE_AUTH_ELEMENT ElementFlags).
 * Flag interpretation is payload-specific; these names do not establish universal semantics.
 */
#define FVE_ELEMENT_FLAG_NONE                   0x00000000
#define FVE_ELEMENT_FLAG_ALLOW_UNENCRYPTED      0x00000001
#define FVE_ELEMENT_FLAG_ENCRYPTED_KEY          0x00000002
#define FVE_ELEMENT_FLAG_AUTO_UNLOCK            0x00000004
#define FVE_ELEMENT_FLAG_INTERNAL               0x00000008
#define FVE_ELEMENT_FLAG_PREDICTIVE_TPM         0x00000010
#define FVE_ELEMENT_FLAG_PREDICTIVE_TPM_PCR7    0x00000020
#define FVE_ELEMENT_FLAG_PREDICTIVE_TPM_PCR4    0x00000040
#define FVE_ELEMENT_FLAG_DRA                    0x00000080
#define FVE_ELEMENT_FLAG_SYSTEM_CONTAINED       0x00000100
#define FVE_ELEMENT_FLAG_WINRE_TRUSTED          0x00000200

// FVE_AUTH_ELEMENT ElementType
#define FVE_AUTH_ELEMENT_VERSION_1 1
#define FVE_AUTH_ELEMENT_FLAG_UNKNOWN1 0x00000001UL

/**
 * Identifies the type of an authentication element.
 */
typedef enum _FVE_AUTH_ELEMENT_TYPE
{
    FVE_ELEMENT_TYPE_UNKNOWN = 0,
    FVE_ELEMENT_TYPE_RECOVERY_PASSWORD = 1, // FVE_AUTH_RECOVERY_PASSWORD
    FVE_ELEMENT_TYPE_PIN = 2, // FVE_AUTH_PIN
    FVE_ELEMENT_TYPE_TPM = 3, // FVE_AUTH_TPM
    FVE_ELEMENT_TYPE_EXTERNAL_KEY = 4, // FVE_AUTH_EXTERNAL_KEY
    FVE_ELEMENT_TYPE_PUBLIC_KEY = 5, // FVE_AUTH_PUBLIC_KEY (input)
    FVE_ELEMENT_TYPE_PRIVATE_KEY = 6, // FVE_AUTH_PRIVATE_KEY
    FVE_ELEMENT_TYPE_PUBLIC_KEY_INFO = 7, // FVE_AUTH_INFO_PUBLIC_KEY (output)
    FVE_ELEMENT_TYPE_PASSPHRASE = 8, // FVE_AUTH_PASSPHRASE
    FVE_ELEMENT_TYPE_CLEAR_KEY = 9, // FVE_AUTH_INFO_CLEAR_KEY
    FVE_ELEMENT_TYPE_DPAPI_NG = 10, // FVE_AUTH_DPAPI_NG
    FVE_ELEMENT_TYPE_UNKNOWN11 = 11,
    FVE_ELEMENT_TYPE_PREDICTED_TPM_INFO = 12 // FVE_AUTH_PREDICTED_TPM_INFO
} FVE_AUTH_ELEMENT_TYPE, *PFVE_AUTH_ELEMENT_TYPE;

/**
 * Describes a single authentication element of a key protector.
 */
typedef struct _FVE_AUTH_ELEMENT
{
    ULONG Size;
    ULONG Version;
    ULONG ElementFlags;
    FVE_AUTH_ELEMENT_TYPE ElementType;
    union
    {
        BYTE Nothing[1];
        FVE_AUTH_RECOVERY_PASSWORD RecoveryPassword;
        FVE_AUTH_PIN Pin;
        FVE_AUTH_TPM Tpm;
        FVE_AUTH_EXTERNAL_KEY ExternalKey;
        FVE_AUTH_PUBLIC_KEY PublicKey;
        FVE_AUTH_PRIVATE_KEY PrivateKey;
        FVE_AUTH_INFO_PUBLIC_KEY PublicKeyInfo;
        FVE_AUTH_PASSPHRASE PassPhrase;
        FVE_AUTH_INFO_CLEAR_KEY ClearKeyInfo;
        FVE_AUTH_DPAPI_NG DpapiNgInfo;
        FVE_AUTH_NETWORK_SERVER_INFO NetworkServerInfo;
        FVE_AUTH_PREDICTED_TPM_INFO PredictedTpmInfo;
    } Data;
} FVE_AUTH_ELEMENT, *PFVE_AUTH_ELEMENT;

/**
 * Pointer to a constant FVE_AUTH_ELEMENT structure.
 */
typedef const FVE_AUTH_ELEMENT *PCFVE_AUTH_ELEMENT;

#define FVE_AUTH_INFORMATION_VERSION_1 1
#define FVE_AUTH_INFORMATION_QUERY_UNKNOWN1 0x00000001UL
#define FVE_AUTH_INFORMATION_QUERY_UNKNOWN2 0x00000002UL
#define FVE_AUTH_INFORMATION_QUERY_UNKNOWN4 0x00000004UL
#define FVE_AUTH_INFORMATION_FLAG_CLEAR_KEY 0x00010000UL
#define FVE_AUTH_INFORMATION_FLAG_TPM 0x00020000UL
#define FVE_AUTH_INFORMATION_FLAG_EXTERNAL_KEY 0x00040000UL
#define FVE_AUTH_INFORMATION_FLAG_RECOVERY_PASSWORD 0x00080000UL
#define FVE_AUTH_INFORMATION_FLAG_TPM_AND_PIN 0x00120000UL
#define FVE_AUTH_INFORMATION_FLAG_TPM_AND_STARTUP_KEY 0x00060000UL
#define FVE_AUTH_INFORMATION_FLAG_TPM_PIN_AND_STARTUP_KEY 0x00160000UL
#define FVE_AUTH_INFORMATION_FLAG_CERTIFICATE 0x00200000UL
#define FVE_AUTH_INFORMATION_FLAG_PASSPHRASE 0x00800000UL
#define FVE_AUTH_INFORMATION_FLAG_TPM_AND_CERTIFICATE 0x00220000UL
#define FVE_AUTH_INFORMATION_FLAG_DPAPI_NG 0x01000000UL
#define FVE_AUTH_INFORMATION_PROTECTOR_MASK 0x03FE0000UL

/**
 * Describes the authentication information for a key protector, including its elements.
 */
typedef struct _FVE_AUTH_INFORMATION
{
    ULONG Size;
    ULONG Version;
    ULONG AuthFlags;
    ULONG ElementsCount;
    PFVE_AUTH_ELEMENT *Elements;
    PCWSTR Description;
    FILETIME CreationTime;
    GUID Identifier;
} FVE_AUTH_INFORMATION, *PFVE_AUTH_INFORMATION;

/**
 * Pointer to a constant FVE_AUTH_INFORMATION structure.
 */
typedef const FVE_AUTH_INFORMATION *PCFVE_AUTH_INFORMATION;

/**
 * Describes the Active Directory backup group policy options.
 */
typedef struct _ADA_GP_OPTIONS
{
    BOOL BackupEnabled;
    BOOL BackupKeyPackage;
    BOOL BackupRequired;
} ADA_GP_OPTIONS, *PADA_GP_OPTIONS;

/**
 * Identifies the type of a key protector.
 */
typedef enum _FVE_PROTECTOR_TYPE
{
    FveKeyProtTypeUnknown = 0,
    FveKeyProtTypeTpm,
    FveKeyProtTypeKey,
    FveKeyProtTypePassword,
    FveKeyProtTypeTpmAndPin,
    FveKeyProtTypeTpmAndKey,
    FveKeyProtTypeTpmAndPinAndKey,
    FveKeyProtTypeCertificate,
    FveKeyProtTypePassPhrase,
    FveKeyProtTypeTpmAndCertificate,
    FveKeyProtTypeDpapiNg,
} FVE_PROTECTOR_TYPE, *PFVE_PROTECTOR_TYPE;

/**
 * The FveIsTpmProtectorType routine determines whether the specified protector type includes a TPM.
 *
 * \param[in] ProtectorType The key protector type.
 * \return TRUE if the protector type includes a TPM, FALSE otherwise.
 */
FORCEINLINE
BOOL
FveIsTpmProtectorType(
    _In_ FVE_PROTECTOR_TYPE ProtectorType
    )
{
    return ProtectorType == FveKeyProtTypeTpm ||
           ProtectorType == FveKeyProtTypeTpmAndPin ||
           ProtectorType == FveKeyProtTypeTpmAndKey ||
           ProtectorType == FveKeyProtTypeTpmAndPinAndKey ||
           ProtectorType == FveKeyProtTypeTpmAndCertificate;
}

/**
 * The FveOpenVolumeW routine opens the specified BitLocker (FVE) volume and returns a handle to it.
 *
 * \param[in] VolumeName The volume name.
 * \param[in] NeedWriteAccess A value indicating whether write access to the volume is required.
 * \param[out] FveVolumeHandle Receives a handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 * \remarks Volume GUID paths with a trailing backslash are accepted and canonicalized before opening.
 */
NTSYSAPI
HRESULT
NTAPI
FveOpenVolumeW(
    _In_ PCWSTR VolumeName,
    _In_ BOOL NeedWriteAccess,
    _Out_ PHANDLE FveVolumeHandle
    );

/**
 * The FveOpenVolumeExW routine opens the specified BitLocker (FVE) volume with extended options and returns a handle
 * to it.
 *
 * \param[in] VolumeName The volume name.
 * \param[in] NameFlags Flags that qualify how the volume name is interpreted.
 * \param[in] NeedWriteAccess A value indicating whether write access to the volume is required.
 * \param[in] InterfaceType The FVE interface type to open the volume with.
 * \param[in] HandleFlags Flags that control how the handle is opened.
 * \param[out] FveVolumeHandle Receives a handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveOpenVolumeExW(
    _In_ PCWSTR VolumeName,
    _In_ ULONG NameFlags,
    _In_ BOOL NeedWriteAccess,
    _In_ FVE_INTERFACE_TYPE InterfaceType,
    _In_ ULONG HandleFlags,
    _Out_ PHANDLE FveVolumeHandle
    );

/**
 * The FveOpenVolumeByHandle routine opens a BitLocker (FVE) volume from an existing handle and returns an FVE volume
 * handle.
 *
 * \param[in] Handle A handle to the underlying object.
 * \param[in] HandleType The type of the supplied handle.
 * \param[in] NeedWriteAccess A value indicating whether write access to the volume is required.
 * \param[in] InterfaceType The FVE interface type to open the volume with.
 * \param[in] HandleFlags Flags that control how the handle is opened.
 * \param[out] FveVolumeHandle Receives a handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveOpenVolumeByHandle(
    _In_ HANDLE Handle,
    _In_ FVE_HANDLE_TYPE HandleType,
    _In_ BOOL NeedWriteAccess,
    _In_ FVE_INTERFACE_TYPE InterfaceType,
    _In_ ULONG HandleFlags,
    _Out_ PHANDLE FveVolumeHandle
    );

/**
 * The FveCloseHandle routine closes an FVE handle.
 *
 * \param[in] FveHandle A handle to the FVE object.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCloseHandle(
    _In_ HANDLE FveHandle
    );

/**
 * The FveCloseVolume routine closes a handle to an FVE volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCloseVolume(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveApplyGroupPolicy routine applies the current BitLocker group policy to the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveApplyGroupPolicy(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveCommitChanges routine commits pending changes to the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCommitChanges(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveDiscardChanges routine discards pending changes to the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 * \remarks Reloads persisted metadata. This does not undo initialization already written to disk.
 */
NTSYSAPI
HRESULT
NTAPI
FveDiscardChanges(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveGetStatus routine retrieves the current encryption status of the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in,out] Status The initialized status structure; receives the volume status.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 * \remarks Initialize Size and Version for the requested status layout; version 9 uses 128 bytes.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetStatus(
    _In_ HANDLE FveVolumeHandle,
    _Inout_ PFVE_STATUS_V9 Status
    );

/**
 * The FveGetStatusW routine retrieves the current encryption status of the named volume.
 *
 * \param[in] VolumeName The volume name.
 * \param[in,out] Status The initialized status structure; receives the volume status.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 * \remarks Initialize Size and Version for the requested status layout; version 9 uses 128 bytes.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetStatusW(
    _In_ PCWSTR VolumeName,
    _Inout_ PFVE_STATUS_V9 Status
    );

/**
 * The FveGetUserFlags routine retrieves the FVE user flags for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] UserFlags Receives the FVE user flags.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetUserFlags(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PULONG UserFlags
    );

/**
 * The FveSetUserFlags routine sets the FVE user flags on the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] UserFlags The FVE user flags.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSetUserFlags(
    _In_ HANDLE FveVolumeHandle,
    _In_ ULONG UserFlags
    );

/**
 * The FveClearUserFlags routine clears the specified FVE user flags on the volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] UserFlags The FVE user flags.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveClearUserFlags(
    _In_ HANDLE FveVolumeHandle,
    _In_ ULONG UserFlags
    );

/**
 * The FveGetAuthMethodGuids routine retrieves the GUIDs of the authentication methods configured on the volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] AuthMethodGuids Receives the buffer that receives the authentication method GUIDs.
 * \param[in] MaxAuthMethodGuids The maximum number of GUIDs the buffer can hold.
 * \param[out] AuthMethodGuidCount Receives the number of GUIDs returned.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 * \remarks A NULL array with capacity zero queries the required count and returns S_FALSE.
 * A subsequent call with sufficient capacity returns S_OK.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetAuthMethodGuids(
    _In_ HANDLE FveVolumeHandle,
    _Out_writes_to_opt_(MaxAuthMethodGuids, *AuthMethodGuidCount) PGUID AuthMethodGuids,
    _In_ UINT MaxAuthMethodGuids,
    _Out_ PUINT AuthMethodGuidCount
    );

/**
 * The FveGetAuthMethodInformation routine retrieves detailed information about an authentication method on the volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in,out] Information The initialized authentication information buffer; receives the requested information.
 * \param[in] BufferSize The size, in bytes, of the buffer.
 * \param[out] RequiredSize Receives the size, in bytes, required for the buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 * \remarks Embedded pointers reference the caller-owned result buffer. Securely zero the buffer before freeing it.
 * A metadata-only result can have ElementsCount equal to zero. Query bit 1 selects the GUID; bit 2 requests key export.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetAuthMethodInformation(
    _In_ HANDLE FveVolumeHandle,
    _Inout_updates_bytes_(BufferSize) PFVE_AUTH_INFORMATION Information,
    _In_ SIZE_T BufferSize,
    _Out_ PSIZE_T RequiredSize
    );

/**
 * The FveProtectorTypeToFlags routine converts a key protector type to its corresponding type flags.
 *
 * \param[in] ProtectorType The key protector type.
 * \param[out] TypeFlags Receives the protector type flags.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveProtectorTypeToFlags(
    _In_ FVE_PROTECTOR_TYPE ProtectorType,
    _Out_ PULONG TypeFlags
    );

/**
 * The FveFlagsToProtectorType routine converts protector type flags to the corresponding key protector type.
 *
 * \param[in] TypeFlags The protector type flags.
 * \param[out] ProtectorType Receives the key protector type.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveFlagsToProtectorType(
    _In_ ULONG TypeFlags,
    _Out_ PFVE_PROTECTOR_TYPE ProtectorType
    );

/**
 * The FveDeleteAuthMethod routine deletes the specified authentication method from the volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] AuthMethodGuid The GUID identifying the authentication method.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveDeleteAuthMethod(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCGUID AuthMethodGuid
    );

/**
 * The FveAddAuthMethodInformation routine adds an authentication method to the volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] Information The authentication information.
 * \param[out] AuthMethodGuid Receives the added authentication-method GUID on S_OK.
 * \return S_OK for normal creation, S_FALSE when the clear key already exists, or an HRESULT error.
 * \remarks 10.0.26100.9278 permits a null GUID output; 10.0.28000.2804 requires a nonnull output.
 * The existing-clear-key S_FALSE path does not write the GUID.
 */
_Success_(return == S_OK)
NTSYSAPI
HRESULT
NTAPI
FveAddAuthMethodInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCFVE_AUTH_INFORMATION Information,
    _Out_ PGUID AuthMethodGuid
    );

/**
 * The FveUpdatePinW routine updates the PIN of the specified TPM-and-PIN protector.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] NewPin The new PIN.
 * \param[in] ProtectorGuid The GUID identifying the key protector.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveUpdatePinW(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCWSTR NewPin,
    _In_ PCGUID ProtectorGuid
    );

/**
 * The FveValidateExistingPinW routine validates an existing PIN against the specified protector.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] ExistingPin The existing PIN.
 * \param[out] ExistingPinValidates Receives a value indicating whether the existing PIN validates.
 * \param[out] ProtectorGuid Optional output written only when *ExistingPinValidates is TRUE.
 * An unmatched comparison leaves it unchanged.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveValidateExistingPinW(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCWSTR ExistingPin,
    _Out_ PBOOL ExistingPinValidates,
    _Out_opt_ PGUID ProtectorGuid
    );

/**
 * The FveValidateExistingPassphraseW routine validates an existing passphrase against the specified protector.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] ExistingPassphrase The existing passphrase.
 * \param[out] ExistingPassphraseValidates Receives a value indicating whether the existing passphrase validates.
 * \param[out] ProtectorGuid Optional output written only when *ExistingPassphraseValidates is TRUE.
 * An unmatched comparison leaves it unchanged.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveValidateExistingPassphraseW(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCWSTR ExistingPassphrase,
    _Out_ PBOOL ExistingPassphraseValidates,
    _Out_opt_ PGUID ProtectorGuid
    );

/**
 * The FveEraseDrive routine erases the encryption metadata and keys from the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] ForceDismount A value indicating whether the volume should be forcibly dismounted.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveEraseDrive(
    _In_ HANDLE FveVolumeHandle,
    _In_ BOOL ForceDismount
    );

/**
 * The FveUpgradeVolume routine upgrades the on-disk metadata of the specified volume to the current version.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveUpgradeVolume(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveEraseDriveExW routine erases the encryption metadata and keys from the named volume.
 *
 * \param[in] VolumeName The volume name.
 * \param[in] ForceDismount A value indicating whether the volume should be forcibly dismounted.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveEraseDriveExW(
    _In_ PCWSTR VolumeName,
    _In_ BOOL ForceDismount
    );

/**
 * The FveUnlockVolume routine unlocks the specified volume using the supplied authentication information.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] Information The authentication information.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveUnlockVolume(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCFVE_AUTH_INFORMATION Information
    );

/**
 * The FveUnlockVolumeWithAccessMode routine unlocks the specified volume, optionally for read-only access.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] Information The authentication information.
 * \param[out] ReadOnly Receives whether the volume was unlocked for read-only access.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveUnlockVolumeWithAccessMode(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCFVE_AUTH_INFORMATION Information,
    _Out_ PBOOL ReadOnly
    );

/**
 * The FveAttemptAutoUnlock routine attempts to automatically unlock the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveAttemptAutoUnlock(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveLockVolume routine locks the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] ForceDismount A value indicating whether the volume should be forcibly dismounted.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveLockVolume(
    _In_ HANDLE FveVolumeHandle,
    _In_ BOOLEAN ForceDismount
    );

/**
 * The FveCheckBootFileW routine checks whether the specified boot file is valid for BitLocker.
 *
 * \param[in] Path The file path.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCheckBootFileW(
    _In_ PCWSTR Path
    );

/**
 * The FveGetIdentity routine retrieves the identity GUID of the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] IdentityGuid Receives the volume identity GUID.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetIdentity(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PGUID IdentityGuid
    );

/**
 * The FveGetRecoveryPasswordBackupInformation routine retrieves the recovery-password backup information for a
 * protector.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] RecoveryPasswordGuid The GUID identifying the key protector.
 * \param[out] BackupInformation Receives the mask of recovery-password backup information types.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetRecoveryPasswordBackupInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_opt_ PCGUID RecoveryPasswordGuid,
    _Out_ PUSHORT BackupInformation
    );

/**
 * The FveSetRecoveryPasswordBackupInformation routine sets the recovery-password backup information for a protector.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] RecoveryPasswordGuid The GUID identifying the key protector.
 * \param[in] BackupInformationType The recovery-password backup information type.
 * \param[in] FlagsToSet The backup information flags to set.
 * \param[in] FlagsToClear The backup information flags to clear.
 * \param[out] InformationChanged Receives a value indicating whether the dataset was updated.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSetRecoveryPasswordBackupInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCGUID RecoveryPasswordGuid,
    _In_ USHORT BackupInformationType,
    _In_ USHORT FlagsToSet,
    _In_ USHORT FlagsToClear,
    _Out_ PBOOLEAN InformationChanged
    );

/**
 * The FveClearRecoveryPasswordBackupInformation routine clears the recovery-password backup information for a
 * protector.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] RecoveryPasswordGuid The GUID identifying the key protector.
 * \param[in] BackupInformationType The recovery-password backup information type.
 * \param[out] InformationChanged Receives a value indicating whether the dataset was updated.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveClearRecoveryPasswordBackupInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCGUID RecoveryPasswordGuid,
    _In_ USHORT BackupInformationType,
    _Out_ PBOOLEAN InformationChanged
    );

/**
 * The FveGetRecoveryPasswordBackupAccountInformation routine retrieves the account information used to back up
 * recovery passwords.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] RecoveryPasswordGuid The GUID identifying the key protector.
 * \param[in] BackupInformationType The recovery-password backup information type.
 * \param[in] AccountNameCch The size, in characters, of the backup account buffer.
 * \param[out] RequiredCch Receives the size, in characters, required for the backup account buffer.
 * \param[out] AccountName Receives the buffer that receives the backup account names.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetRecoveryPasswordBackupAccountInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCGUID RecoveryPasswordGuid,
    _In_ USHORT BackupInformationType,
    _In_ SIZE_T AccountNameCch,
    _Out_ PSIZE_T RequiredCch,
    _Out_writes_opt_(AccountNameCch) PWSTR AccountName
    );

/**
 * The FveSetRecoveryPasswordBackupAccountInformation routine sets the account information used to back up recovery
 * passwords.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] RecoveryPasswordGuid The GUID identifying the key protector.
 * \param[in] BackupInformationType The recovery-password backup information type.
 * \param[in] AccountName The backup account name.
 * \param[out] InformationChanged Receives a value indicating whether the dataset was updated.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSetRecoveryPasswordBackupAccountInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCGUID RecoveryPasswordGuid,
    _In_ USHORT BackupInformationType,
    _In_ PCWSTR AccountName,
    _Out_ PBOOLEAN InformationChanged
    );

/**
 * The FveSelectBestRecoveryPasswordByBackupInformation routine selects the most appropriate recovery password based
 * on backup information.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] RecoveryPasswordGuid Receives the selected recovery-password protector GUID.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSelectBestRecoveryPasswordByBackupInformation(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PGUID RecoveryPasswordGuid
    );

/**
 * The FveAuthElementToRecoveryPasswordW routine converts an authentication element to its recovery-password string
 * form.
 *
 * \param[in] AuthElement The authentication element.
 * \param[out] RecoveryPassword Receives the recovery password (passphrase).
 * \param[in] RecoveryPasswordCch The size, in characters, of the passphrase buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveAuthElementToRecoveryPasswordW(
    _In_ PCFVE_AUTH_ELEMENT AuthElement,
    _Out_writes_(RecoveryPasswordCch) PWSTR RecoveryPassword,
    _In_ SIZE_T RecoveryPasswordCch
    );

/**
 * The FveAuthElementFromPinW routine builds an authentication element from a PIN.
 *
 * \param[in] Pin The PIN.
 * \param[in,out] AuthElement Receives the authentication element.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 * \remarks Initialize the authentication element Size and Version before calling.
 */
NTSYSAPI
HRESULT
NTAPI
FveAuthElementFromPinW(
    _In_ PCWSTR Pin,
    _Inout_ PFVE_AUTH_ELEMENT AuthElement
    );

/**
 * The FveAuthElementFromPassPhraseW routine builds an authentication element from a passphrase.
 *
 * \param[in] PassPhrase The passphrase.
 * \param[in,out] AuthElement Receives the authentication element.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 * \remarks Initialize the authentication element Size and Version before calling.
 */
NTSYSAPI
HRESULT
NTAPI
FveAuthElementFromPassPhraseW(
    _In_ PCWSTR PassPhrase,
    _Inout_ PFVE_AUTH_ELEMENT AuthElement
    );

/**
 * The FveAuthElementFromRecoveryPasswordW routine builds an authentication element from a recovery password.
 *
 * \param[in] RecoveryPassword The recovery password (passphrase).
 * \param[in,out] AuthElement Receives the authentication element.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 * \remarks Initialize the authentication element Size and Version before calling.
 */
NTSYSAPI
HRESULT
NTAPI
FveAuthElementFromRecoveryPasswordW(
    _In_ PCWSTR RecoveryPassword,
    _Inout_ PFVE_AUTH_ELEMENT AuthElement
    );

/**
 * The FveIsRecoveryPasswordGroupValidW routine determines whether a recovery-password group is valid.
 *
 * \param[in] RecoveryPasswordGroup The recovery password group.
 * \param[out] IsValid Receives a value indicating whether the value is valid.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsRecoveryPasswordGroupValidW(
    _In_ PCWSTR RecoveryPasswordGroup,
    _Out_ PBOOLEAN IsValid
    );

/**
 * The FveIsRecoveryPasswordValidW routine determines whether a recovery password is valid.
 *
 * \param[in] RecoveryPassword The recovery password (passphrase).
 * \param[out] IsValid Receives a value indicating whether the value is valid.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsRecoveryPasswordValidW(
    _In_ PCWSTR RecoveryPassword,
    _Out_ PBOOLEAN IsValid
    );

/**
 * The FveIsPassphraseCompatibleW routine determines whether a passphrase is compatible with the current policy.
 *
 * \param[in] Passphrase The recovery password (passphrase).
 * \param[out] IsCompatible Receives a value indicating whether the passphrase is compatible.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsPassphraseCompatibleW(
    _In_ PCWSTR Passphrase,
    _Out_ PBOOL IsCompatible
    );

/**
 * The FveAuthElementReadExternalKeyW routine reads an external key file into authentication information.
 *
 * \param[in] KeyFullFilePath The full path of the external key file.
 * \param[in,out] Information The initialized authentication information buffer; receives the external key information.
 * \param[in] BufferSize The size, in bytes, of the buffer.
 * \param[out] RequiredSize Receives the size, in bytes, required for the buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveAuthElementReadExternalKeyW(
    _In_ PCWSTR KeyFullFilePath,
    _Inout_updates_bytes_(BufferSize) PFVE_AUTH_INFORMATION Information,
    _In_ SIZE_T BufferSize,
    _Out_ PSIZE_T RequiredSize
    );

/**
 * The FveAuthElementWriteExternalKeyW routine writes authentication information to an external key file.
 *
 * \param[in] KeyFullFilePath The full path of the external key file.
 * \param[in] Information The authentication information.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveAuthElementWriteExternalKeyW(
    _In_ PCWSTR KeyFullFilePath,
    _In_ PCFVE_AUTH_INFORMATION Information
    );

/**
 * The FveAuthElementWriteExternalKeyExW routine writes authentication information to an external key file for the
 * specified volume identity.
 *
 * \param[in] Identifier The volume identity GUID.
 * \param[in] KeyFullFilePath The full path of the external key file.
 * \param[in] Information The authentication information.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveAuthElementWriteExternalKeyExW(
    _In_ PCGUID Identifier,
    _In_ PCWSTR KeyFullFilePath,
    _In_ PCFVE_AUTH_INFORMATION Information
    );

/**
 * The FveAuthElementGetKeyFileNameW routine retrieves the external key file name for the specified authentication
 * information.
 *
 * \param[in] Information The authentication information.
 * \param[out] KeyFileName Receives the buffer that receives the external key file name.
 * \param[in] KeyFileNameCch The size, in characters, of the buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveAuthElementGetKeyFileNameW(
    _In_ PCFVE_AUTH_INFORMATION Information,
    _Out_writes_(KeyFileNameCch) PWSTR KeyFileName,
    _In_ SIZE_T KeyFileNameCch
    );

/**
 * The FveInitVolumeEx routine initializes the specified volume for BitLocker with extended options.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] DiscoveryVolumeType The discovery volume type.
 * \param[in] InitializationFlags Flags controlling volume initialization.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveInitVolumeEx(
    _In_ HANDLE FveVolumeHandle,
    _In_opt_ PCWSTR DiscoveryVolumeType,
    _In_ ULONG InitializationFlags
    );

/**
 * The FveInitVolume routine initializes the specified volume for BitLocker.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] DiscoveryVolumeType The discovery volume type.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 * \remarks Initialization can persist metadata and write the boot region; FveDiscardChanges does not undo it.
 */
NTSYSAPI
HRESULT
NTAPI
FveInitVolume(
    _In_ HANDLE FveVolumeHandle,
    _In_opt_ PCWSTR DiscoveryVolumeType
    );

/**
 * The FveInitializeDeviceEncryption routine initializes device encryption on the system.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveInitializeDeviceEncryption(
    VOID
    );

/**
 * The FveInitializeDeviceEncryption2 routine initializes device encryption on the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] InitializationFlags Flags controlling device encryption initialization.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveInitializeDeviceEncryption2(
    _In_ HANDLE FveVolumeHandle,
    _In_ ULONG InitializationFlags
    );

#define FVE_DE_SUPPORT_VERSION_1 1

/**
 * Describes whether device encryption is supported on the system.
 */
typedef struct _FVE_DE_SUPPORT
{
    ULONG Size;
    ULONG Version;
    ULONG QueryFlags;
    HRESULT SupportStatus;
    ULONG SupportFlags;
} FVE_DE_SUPPORT, *PFVE_DE_SUPPORT;

/**
 * Pointer to a constant FVE_DE_SUPPORT structure.
 */
typedef const FVE_DE_SUPPORT *PCFVE_DE_SUPPORT;

/**
 * The FveQueryDeviceEncryptionSupport routine queries whether the system supports device encryption.
 *
 * \param[in,out] DeviceEncryptionSupport The initialized support query structure; receives the result.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveQueryDeviceEncryptionSupport(
    _Inout_ PFVE_DE_SUPPORT DeviceEncryptionSupport
    );

/**
 * The FveRevertVolume routine reverts the specified volume to its unencrypted state.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveRevertVolume(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveKeyManagement routine performs key-management operations on the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] FlagsIn The input key-management flags.
 * \param[out] FlagsOut Receives the output key-management flags.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveKeyManagement(
    _In_ HANDLE FveVolumeHandle,
    _In_ ULONG FlagsIn,
    _Out_opt_ PULONG FlagsOut
    );

/**
 * The FveConversionDecrypt routine begins decrypting (converting) the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveConversionDecrypt(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveConversionDecryptEx routine begins decrypting the specified volume with the given conversion flags.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] ConversionFlags Flags controlling the conversion (encryption/decryption) operation.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveConversionDecryptEx(
    _In_ HANDLE FveVolumeHandle,
    _In_ ULONG ConversionFlags
    );

/**
 * The FveConversionEncrypt routine begins encrypting (converting) the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveConversionEncrypt(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveConversionEncryptEx routine begins encrypting the specified volume with the given conversion flags.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] ConversionFlags Flags controlling the conversion (encryption/decryption) operation.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveConversionEncryptEx(
    _In_ HANDLE FveVolumeHandle,
    _In_ ULONG ConversionFlags
    );

/**
 * The FveConversionEncryptPendingReboot routine schedules encryption of the specified volume to begin after the next
 * reboot.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveConversionEncryptPendingReboot(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveConversionEncryptPendingRebootEx routine schedules encryption of the specified volume to begin after the
 * next reboot with the given conversion flags.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] ConversionFlags Flags controlling the conversion (encryption/decryption) operation.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveConversionEncryptPendingRebootEx(
    _In_ HANDLE FveVolumeHandle,
    _In_ ULONG ConversionFlags
    );

/**
 * The FveConversionStop routine stops the ongoing conversion of the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveConversionStop(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveConversionStopEx routine stops the ongoing conversion of the specified volume, optionally restarting on reinsertion.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] AutoStartOnReinsertion A value indicating whether conversion restarts automatically on reinsertion.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveConversionStopEx(
    _In_ HANDLE FveVolumeHandle,
    _In_ BOOLEAN AutoStartOnReinsertion
    );

/**
 * The FveConversionPause routine pauses the ongoing conversion of the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveConversionPause(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveConversionResume routine resumes a paused conversion of the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveConversionResume(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveIsVolumeEncryptable routine determines whether the specified volume can be encrypted.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsVolumeEncryptable(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveGetFveMethod routine retrieves the encryption method configured on the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] FveMethod Receives the FVE encryption method.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetFveMethod(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PFVE_LEGACY_METHOD FveMethod
    );

#define FVE_EDRIVE_METHOD_CCH 256

/**
 * The FveGetFveMethodEDrv routine retrieves the encryption method, including the eDrive method, for the specified
 * volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] FveMethod Receives the FVE encryption method.
 * \param[out] SelfEncryptionDriveMethod Optional buffer that receives the self-encrypting drive encryption method.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetFveMethodEDrv(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PFVE_LEGACY_METHOD FveMethod,
    _Out_writes_opt_(FVE_EDRIVE_METHOD_CCH) PWSTR SelfEncryptionDriveMethod
    );

/**
 * The FveGetFveMethodEx routine retrieves extended encryption method information for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] FveMethod Receives the FVE encryption method.
 * \param[out] SelfEncryptionDriveMethod Optional buffer that receives the eDrive (hardware) encryption method.
 * \param[in,out] FveMethodFlags Optional flags value. If supplied, initialize it before the call;
 * existing bits are preserved.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetFveMethodEx(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PFVE_LEGACY_METHOD FveMethod,
    _Out_writes_opt_(FVE_EDRIVE_METHOD_CCH) PWSTR SelfEncryptionDriveMethod,
    _Inout_opt_ PULONG FveMethodFlags
    );

/**
 * The FveSetFveMethod routine sets the encryption method for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] FveMethod The FVE encryption method.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSetFveMethod(
    _In_ HANDLE FveVolumeHandle,
    _In_ FVE_LEGACY_METHOD FveMethod
    );

/**
 * The FveSetFveMethodEx routine sets the encryption method, strength, and flags for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] FveMethod The FVE encryption method.
 * \param[in] Strength The FVE encryption method strength.
 * \param[in] Flags Flags describing the FVE encryption method.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSetFveMethodEx(
    _In_ HANDLE FveVolumeHandle,
    _In_ FVE_METHOD FveMethod,
    _In_ FVE_METHOD_STRENGTH Strength,
    _In_ ULONG Flags
    );

/**
 * The FveCheckTpmCapability routine checks the capabilities of the platform TPM.
 *
 * \param[in,out] Capability The initialized TPM capabilities structure; receives the capabilities.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCheckTpmCapability(
    _Inout_ PFVE_TPM_CAPS Capability
    );

/**
 * The FveBindDataVolume routine binds the specified data volume for automatic unlock.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] AuthMethodGuid The GUID identifying the authentication method.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveBindDataVolume(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCGUID AuthMethodGuid
    );

/**
 * The FveUnbindDataVolume routine removes the automatic-unlock binding from the specified data volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveUnbindDataVolume(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveIsBoundDataVolume routine determines whether the specified data volume is bound for automatic unlock.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] IsAutoUnlockEnabled Receives a value indicating whether automatic unlock is enabled.
 * \param[out] UnlockGuid Receives the GUID of the automatic unlock protector.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsBoundDataVolume(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PBOOL IsAutoUnlockEnabled,
    _Out_ PGUID UnlockGuid
    );

/**
 * The FveIsBoundDataVolumeToOSVolume routine determines whether the specified data volume is bound to the operating
 * system volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] IsAutoUnlockEnabled Receives a value indicating whether automatic unlock is enabled.
 * \param[out] UnlockGuid Receives the GUID of the automatic unlock protector.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsBoundDataVolumeToOSVolume(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PBOOL IsAutoUnlockEnabled,
    _Out_ PGUID UnlockGuid
    );

/**
 * The FveIsAnyDataVolumeBoundToOSVolume routine determines whether any data volume is bound to the operating system
 * volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] BoundVolumeCount Receives the number of bound data volumes.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsAnyDataVolumeBoundToOSVolume(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PULONG BoundVolumeCount
    );

/**
 * The FveUnbindAllDataVolumeFromOSVolume routine removes the automatic-unlock binding of all data volumes from the
 * operating system volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveUnbindAllDataVolumeFromOSVolume(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveSetDescriptionW routine sets the description of the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] VolumeDescription The volume description.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSetDescriptionW(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCWSTR VolumeDescription
    );

/**
 * The FveGetDescriptionW routine retrieves the description of the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] VolumeDescription Receives the volume description.
 * \param[in] BufferLength The size, in characters, of the buffer.
 * \param[out] RequiredSize Receives the size, in bytes, required for the buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetDescriptionW(
    _In_ HANDLE FveVolumeHandle,
    _Out_writes_opt_(BufferLength) PWSTR VolumeDescription,
    _In_ SIZE_T BufferLength,
    _Out_ PSIZE_T RequiredSize
    );

/**
 * The FveSetIdentificationFieldW routine sets the identification field of the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] IdentificationField The identification field.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSetIdentificationFieldW(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCWSTR IdentificationField
    );

/**
 * The FveGetIdentificationFieldW routine retrieves the identification field of the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] IdentificationField Receives the identification field.
 * \param[in] BufferLength The size, in characters, of the buffer.
 * \param[out] RequiredSize Receives the required character count, including the terminating null character.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetIdentificationFieldW(
    _In_ HANDLE FveVolumeHandle,
    _Out_writes_opt_(BufferLength) PWSTR IdentificationField,
    _In_ SIZE_T BufferLength,
    _Out_ PSIZE_T RequiredSize
    );

/**
 * The FveSetAllowKeyExport routine sets whether key export is permitted.
 *
 * \param[in] Allow A value indicating whether the operation is allowed.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSetAllowKeyExport(
    _In_ BOOL Allow
    );

/**
 * The FveGetAllowKeyExport routine retrieves whether key export is permitted.
 *
 * \param[out] Allow Receives a value indicating whether the operation is allowed.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetAllowKeyExport(
    _Out_ PBOOL Allow
    );

/**
 * The FveSetFipsAllowDisabled routine sets whether FIPS-restricted recovery passwords may be disabled.
 *
 * \param[in] Allow A value indicating whether the operation is allowed.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSetFipsAllowDisabled(
    _In_ BOOL Allow
    );

/**
 * The FveGetFipsAllowDisabled routine retrieves whether FIPS-restricted recovery passwords may be disabled.
 *
 * \param[out] Allow Receives a value indicating whether the operation is allowed.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetFipsAllowDisabled(
    _Out_ PBOOL Allow
    );

/**
 * The FveIsHardwareReadyForConversion routine determines whether the hardware is ready for conversion.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsHardwareReadyForConversion(
    VOID
    );

/**
 * The FveGetKeyPackage routine retrieves the key package for the specified protector.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] Identifier The key package identifier.
 * \param[out] Buffer Receives the buffer.
 * \param[in] BufferSize The size, in bytes, of the buffer.
 * \param[out] DataSize Receives the size, in bytes, of the returned data.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetKeyPackage(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCGUID Identifier,
    _Out_writes_bytes_opt_(BufferSize) PBYTE Buffer,
    _In_ SIZE_T BufferSize,
    _Out_ PSIZE_T DataSize
    );

/**
 * The FveEnableRawAccessW routine enables or disables raw access to the named volume.
 *
 * \param[in] VolumeName The volume name.
 * \param[in] Enabled A value indicating whether the feature is enabled.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveEnableRawAccessW(
    _In_ PCWSTR VolumeName,
    _In_ BOOL Enabled
    );

/**
 * The FveEnableRawAccess routine enables or disables raw access to the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] Enabled A value indicating whether the feature is enabled.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveEnableRawAccess(
    _In_ HANDLE FveVolumeHandle,
    _In_ BOOL Enabled
    );

/**
 * The FveEnableRawAccessEx routine enables or disables raw access to the specified volume, optionally forcing a
 * dismount.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] Enabled A value indicating whether the feature is enabled.
 * \param[in] ForceDismount A value indicating whether the volume should be forcibly dismounted.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveEnableRawAccessEx(
    _In_ HANDLE FveVolumeHandle,
    _In_ BOOL Enabled,
    _In_ BOOL ForceDismount
    );

/**
 * The FveBackupRecoveryInformationToAD routine backs up recovery information for the specified protector to Active
 * Directory.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] AuthMethodGuid The GUID identifying the authentication method.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveBackupRecoveryInformationToAD(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCGUID AuthMethodGuid
    );

/**
 * The FveBackupRecoveryInformationToADEx routine backs up recovery information for the specified protector to Active
 * Directory with the given flags.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] AuthMethodGuid The GUID identifying the authentication method.
 * \param[in] FveBackupFlags Flags controlling the Active Directory backup.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveBackupRecoveryInformationToADEx(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCGUID AuthMethodGuid,
    _In_ ULONG FveBackupFlags
    );

/**
 * The FveBackupRecoveryInformationToAAD routine backs up recovery information for the specified protector to Azure
 * Active Directory.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] AuthMethodGuid The GUID identifying the authentication method.
 * \param[in] Flags Policy flags controlling the Azure AD backup.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveBackupRecoveryInformationToAAD(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCGUID AuthMethodGuid,
    _In_ ULONG Flags
    );

/**
 * The FveCheckADRecoveryInfoBackupPolicy routine retrieves the Active Directory recovery-information backup policy
 * for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] Options Receives the Active Directory recovery-information backup policy.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCheckADRecoveryInfoBackupPolicy(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PADA_GP_OPTIONS Options
    );

/**
 * The FveCheckADRecoveryInfoBackupPolicyEx routine retrieves the Active Directory recovery-information backup policy
 * for each volume class.
 *
 * \param[out] OsVolumeOptions Receives the Active Directory backup policy for operating system volumes.
 * \param[out] FixedDataVolumeOptions Receives the Active Directory backup policy for fixed data volumes.
 * \param[out] RemovableDataVolumeOptions Receives the Active Directory backup policy for removable data volumes.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCheckADRecoveryInfoBackupPolicyEx(
    _Out_opt_ PADA_GP_OPTIONS OsVolumeOptions,
    _Out_opt_ PADA_GP_OPTIONS FixedDataVolumeOptions,
    _Out_opt_ PADA_GP_OPTIONS RemovableDataVolumeOptions
    );

/**
 * The FveGetDataSet routine retrieves the FVE metadata dataset for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] DataSetBuffer Receives the buffer that receives the FVE dataset.
 * \param[in] DataSetBufferSize The size, in bytes, of the dataset buffer.
 * \param[out] ActualDataSetBufferSize Receives the actual size, in bytes, of the dataset.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetDataSet(
    _In_ HANDLE FveVolumeHandle,
    _Out_writes_bytes_(DataSetBufferSize) PBYTE DataSetBuffer,
    _In_ SIZE_T DataSetBufferSize,
    _Out_ PSIZE_T ActualDataSetBufferSize
    );

/**
 * The FveGetDataSetEx routine retrieves the FVE metadata dataset for the specified volume, optionally ignoring the
 * locked-volume check.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] IgnoreLockVolumeCheck A value indicating whether the locked-volume check is bypassed.
 * \param[out] DataSetBuffer Receives the buffer that receives the FVE dataset.
 * \param[in] DataSetBufferSize The size, in bytes, of the dataset buffer.
 * \param[out] ActualDataSetBufferSize Receives the actual size, in bytes, of the dataset.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetDataSetEx(
    _In_ HANDLE FveVolumeHandle,
    _In_ BOOL IgnoreLockVolumeCheck,
    _Out_writes_bytes_(DataSetBufferSize) PBYTE DataSetBuffer,
    _In_ SIZE_T DataSetBufferSize,
    _Out_ PSIZE_T ActualDataSetBufferSize
    );

/**
 * The FveIsHybridVolume routine determines whether the specified volume is a hybrid (partially self-encrypting) volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] IsHybrid Receives a value indicating whether the volume is a hybrid (partially self-encrypting) volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsHybridVolume(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PBOOL IsHybrid
    );

/**
 * The FveIsHybridVolumeW routine determines whether the named volume is a hybrid (partially self-encrypting) volume.
 *
 * \param[in] VolumeName The volume name.
 * \param[out] IsHybrid Receives a value indicating whether the volume is a hybrid (partially self-encrypting) volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsHybridVolumeW(
    _In_ PCWSTR VolumeName,
    _Out_ PBOOL IsHybrid
    );

/**
 * The FveNeedsDiscoveryVolumeUpdate routine determines whether the discovery volume needs to be updated.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] NeedsUpdate Receives a value indicating whether the discovery volume needs an update.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveNeedsDiscoveryVolumeUpdate(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PBOOL NeedsUpdate
    );

/**
 * The FveServiceDiscoveryVolume routine updates (services) the discovery volume for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveServiceDiscoveryVolume(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveNotifyVolumeAfterFormat routine notifies BitLocker that the specified volume was formatted.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveNotifyVolumeAfterFormat(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveSaveRecoveryPasswordBackupFlag routine saves the recovery-password backup flag for the specified protector.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] RecoveryPasswordGuid The GUID identifying the recovery password protector.
 * \param[in] RecoveryPassword The recovery password authentication element.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSaveRecoveryPasswordBackupFlag(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCGUID RecoveryPasswordGuid,
    _In_ PCFVE_AUTH_ELEMENT RecoveryPassword
    );

/**
 * The FveDraCertPresentInRegistry routine determines whether a data recovery agent certificate is present in the
 * registry.
 *
 * \param[out] CertPresent Receives a value indicating whether a data recovery agent certificate is present.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveDraCertPresentInRegistry(
    _Out_ PBOOL CertPresent
    );

/**
 * The FveSysOpenVolumeW routine opens the named volume for system (SEI) access and returns a handle to it.
 *
 * \param[in] VolumeName The volume name.
 * \param[out] FveSysHandle Receives a handle to the FVE system volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSysOpenVolumeW(
    _In_ PCWSTR VolumeName,
    _Out_ PHANDLE FveSysHandle
    );

/**
 * The FveSysCloseVolume routine closes a system (SEI) volume handle.
 *
 * \param[in] FveSysHandle A handle to the FVE system volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSysCloseVolume(
    _In_ HANDLE FveSysHandle
    );

/**
 * The FveSysGetUserFlags routine retrieves the FVE user flags using a system volume handle.
 *
 * \param[in] FveSysHandle A handle to the FVE system volume.
 * \param[out] UserFlags Receives the FVE user flags.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSysGetUserFlags(
    _In_ HANDLE FveSysHandle,
    _Out_ PULONG UserFlags
    );

/**
 * The FveSysSetUserFlags routine sets the FVE user flags using a system volume handle.
 *
 * \param[in] FveSysHandle A handle to the FVE system volume.
 * \param[in] UserFlags The FVE user flags.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSysSetUserFlags(
    _In_ HANDLE FveSysHandle,
    _In_ ULONG UserFlags
    );

/**
 * The FveSysClearUserFlags routine clears the specified FVE user flags using a system volume handle.
 *
 * \param[in] FveSysHandle A handle to the FVE system volume.
 * \param[in] UserFlags The FVE user flags.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSysClearUserFlags(
    _In_ HANDLE FveSysHandle,
    _In_ ULONG UserFlags
    );

/**
 * The FvePpfPredictionsUpdated routine notifies BitLocker that platform prediction data has been updated.
 *
 * \param[in] Context The predictions-updated context.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FvePpfPredictionsUpdated(
    _In_ PPPF_PREDICTIONS_UPDATED_CONTEXT Context
    );

/**
 * The FvePcrMonPredictionsUpdated routine notifies BitLocker that PCR-monitoring prediction data has been updated.
 *
 * \param[in] Context The predictions-updated context.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FvePcrMonPredictionsUpdated(
    _In_ PPPF_PREDICTIONS_UPDATED_CONTEXT Context
    );

/**
 * Identifies the type of query performed by FveQuery.
 */
typedef enum _FVE_QUERY_TYPE
{
    FVE_QUERY_UNKNOWN = 0,
    FVE_QUERY_UNSUPPORTED,
    FVE_QUERY_VOLUMES,
    FVE_QUERY_CSV_VOLUMES,
    FVE_QUERY_DE_NOT_INITIALIZED,
    FVE_QUERY_WCOS_SECURITY_INFO,
    FVE_QUERY_BOOT_INTEGRITY_INFO,
    FVE_QUERY_CONSIDER_TPM_PROTECTOR_VERSION_UPGRADE,
    FVE_QUERY_TPM_PROTECTOR_VERSION,
    FVE_QUERY_TPM_PROTECTOR_CONTAINS_SECURE_BOOT_BINDING,
    FVE_QUERY_CHECK_SECURE_BOOT_FOR_BITLOCKER,
    FVE_QUERY_TPM_PROTECTOR_BINDINGS_COUNT,
    FVE_QUERY_TPM_PROTECTOR_BINDING_CENSUS,
    FVE_QUERY_DEFAULT_PCR_PROFILE,
    FVE_QUERY_PREDICTION_INSTANCE_MAPPING,
    FVE_QUERY_AND_REPORT_WINRE_TRUST_POLICY_DISPOSITION,
    FVE_QUERY_MAX
} FVE_QUERY_TYPE, *PFVE_QUERY_TYPE;

/**
 * Identifies the type of a trusted Windows image (WIM).
 */
typedef enum _FVE_TRUSTED_WIM_TYPE
{
    FVE_TRUSTED_WIM_TYPE_UNKNOWN = 0,
    FVE_TRUSTED_WIM_TYPE_SAFEOS,
    FVE_TRUSTED_WIM_TYPE_WINRE
} FVE_TRUSTED_WIM_TYPE, *PFVE_TRUSTED_WIM_TYPE;

/**
 * Identifies the disposition of the Windows Recovery Environment trust policy.
 */
typedef enum _FVE_WINRE_TRUST_POLICY_DISPOSITION
{
    FVE_WINRE_TRUST_POLICY_DISPOSITION_ALLOWED = 0,
    FVE_WINRE_TRUST_POLICY_DISPOSITION_DISABLED_BY_POLICY,
    FVE_WINRE_TRUST_POLICY_DISPOSITION_UNDETERMINED
} FVE_WINRE_TRUST_POLICY_DISPOSITION, *PFVE_WINRE_TRUST_POLICY_DISPOSITION;

/**
 * Describes a request to evaluate the Windows Recovery Environment trust policy.
 */
typedef struct _FVE_WINRE_TRUST_POLICY_DISPOSITION_REQUEST
{
    ULONG Size;
    BOOLEAN SuppressEventLogging;
    FVE_TRUSTED_WIM_TYPE WimType;
} FVE_WINRE_TRUST_POLICY_DISPOSITION_REQUEST, *PFVE_WINRE_TRUST_POLICY_DISPOSITION_REQUEST;

/**
 * Describes a request for Windows Core OS security information.
 */
typedef struct _FVE_WCOS_SEQURITY_INFO_REQUEST
{
    USHORT Version;
    USHORT Size;
    ULONG CompletionWaitTime;
    UCHAR WaitFor100PercentCompletion;
    UCHAR DisableConversionThrottle;
    UCHAR Reserved1;
    UCHAR Reserved2;
} FVE_WCOS_SEQURITY_INFO_REQUEST, *PFVE_WCOS_SEQURITY_INFO_REQUEST;

/**
 * Describes the response containing Windows Core OS security information.
 */
typedef struct _FVE_WCOS_SEQURITY_INFO_RESPONSE
{
    USHORT Version;
    USHORT Size;
    UCHAR Secure;
    UCHAR SecureBootBinding;
    UCHAR ProvisioningStarted;
    UCHAR ProvisioningComplete;
    ULONGLONG EncryptionRequiredMask;
    ULONGLONG EncryptionEnabledMask;
    ULONGLONG EncryptionCompleteMask;
    ULONGLONG ProtectionArmedMask;
    ULONGLONG RecoveryPasswordAbsentMask;
    ULONGLONG ReadOnlyRequiredMask;
    ULONGLONG ReadOnlyEnabledMask;
} FVE_WCOS_SEQURITY_INFO_RESPONSE, *PFVE_WCOS_SEQURITY_INFO_RESPONSE;

/**
 * Describes the boot integrity state of the platform.
 */
typedef struct _FVE_BOOT_INTEGRITY_INFO
{
    USHORT Version;
    USHORT Size;
    union
    {
        ULONGLONG BootIntegrityFlags;
        struct
        {
            BOOLEAN TpmEnabled : 1;
            BOOLEAN UefiPlatform : 1;
            BOOLEAN UefiSecureBootEnabled : 1;
            BOOLEAN Pcr7IsUsable : 1;
            BOOLEAN Pcr7SbHashPresent : 1;
            BOOLEAN Pcr7SbcpHashPresent : 1;
            BOOLEAN PiTestsigningOrDebuggingAllowed : 1;
            BOOLEAN BootDebuggingBootmgrSet : 1;
            BOOLEAN BootDebuggingWinloadSet : 1;
            BOOLEAN KernelDebuggingWinloadSet : 1;
            BOOLEAN HvDebuggingWinloadSet : 1;
            BOOLEAN TestsigningWinloadSet : 1;
            BOOLEAN FlightsigningWinloadSet : 1;
            BOOLEAN TestsigningBootmgrSet : 1;
            BOOLEAN FlightsigningBootmgrSet : 1;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    HRESULT Tpm;
    HRESULT SecureBoot;
    HRESULT Pcr7SbHash;
    HRESULT Pcr7Check;
    HRESULT Sbcp;
    HRESULT Bcd;
} FVE_BOOT_INTEGRITY_INFO, *PFVE_BOOT_INTEGRITY_INFO;

/**
 * Describes a request for the version of a TPM protector.
 */
typedef struct _FVE_TPM_PROTECTOR_VERSION_REQUEST
{
    USHORT Version;
    USHORT Size;
    HANDLE FveVolumeHandle;
    PCGUID TpmProtectorGuid;
} FVE_TPM_PROTECTOR_VERSION_REQUEST, *PFVE_TPM_PROTECTOR_VERSION_REQUEST;

/**
 * Describes a request to determine whether a TPM protector contains a Secure Boot binding.
 */
typedef struct _FVE_TPM_PROTECTOR_CONTAINS_SECURE_BOOT_BINDING_REQUEST
{
    USHORT Version;
    USHORT Size;
    HANDLE FveVolumeHandle;
    BOOLEAN CheckLegacyBindingOnly;
    PCGUID TpmProtectorGuid;
} FVE_TPM_PROTECTOR_CONTAINS_SECURE_BOOT_BINDING_REQUEST, *PFVE_TPM_PROTECTOR_CONTAINS_SECURE_BOOT_BINDING_REQUEST;

/**
 * Identifies the result of evaluating Secure Boot for BitLocker.
 */
typedef enum _BitLockerSecureBootEvalState
{
    SecureBootNotPossible = 0,
    SecureBootDisabledInPolicy,
    SecureBootInvalidConfiguration,
    SecureBootUsableForBitLocker
} BitLockerSecureBootEvalState;

/**
 * Describes a request to evaluate Secure Boot for BitLocker.
 */
typedef struct _FVE_CHECK_SECURE_BOOT_FOR_BITLOCKER_REQUEST
{
    USHORT Version;
    USHORT Size;
    BOOL CheckPpf;
} FVE_CHECK_SECURE_BOOT_FOR_BITLOCKER_REQUEST, *PFVE_CHECK_SECURE_BOOT_FOR_BITLOCKER_REQUEST;

/**
 * Describes the response from evaluating Secure Boot for BitLocker.
 */
typedef struct _FVE_CHECK_SECURE_BOOT_FOR_BITLOCKER_RESPONSE
{
    USHORT Version;
    USHORT Size;
    BitLockerSecureBootEvalState SecureBootEvalState;
    BOOL SecureBootDisabled;
    BOOL SbcpHashPresent;
} FVE_CHECK_SECURE_BOOT_FOR_BITLOCKER_RESPONSE, *PFVE_CHECK_SECURE_BOOT_FOR_BITLOCKER_RESPONSE;

/**
 * Describes a request for the number of TPM protector bindings.
 */
typedef struct _FVE_TPM_PROTECTOR_BINDINGS_COUNT_REQUEST
{
    USHORT Version;
    USHORT Size;
    HANDLE FveVolumeHandle;
    LPCGUID TpmProtectorGuid;
} FVE_TPM_PROTECTOR_BINDINGS_COUNT_REQUEST, *PFVE_TPM_PROTECTOR_BINDINGS_COUNT_REQUEST;

/**
 * Describes a request to census the bindings of a TPM protector.
 */
typedef struct _FVE_TPM_PROTECTOR_BINDING_CENSUS_REQUEST
{
    USHORT Version;
    USHORT Size;
    HANDLE FveVolumeHandle;
    LPCGUID TpmProtectorGuid;
    BOOLEAN PartialResultsOkay;
} FVE_TPM_PROTECTOR_BINDING_CENSUS_REQUEST, *PFVE_TPM_PROTECTOR_BINDING_CENSUS_REQUEST;

/**
 * Describes a request for the default PCR profile of a volume.
 */
typedef struct _FVE_DEFAULT_PCR_PROFILE_REQUEST
{
    USHORT Version;
    USHORT Size;
    HANDLE VolumeHandle;
} FVE_DEFAULT_PCR_PROFILE_REQUEST, *PFVE_DEFAULT_PCR_PROFILE_REQUEST;

/**
 * Describes the response containing the default PCR profile of a volume.
 */
typedef struct _FVE_DEFAULT_PCR_PROFILE_RESPONSE
{
    USHORT Version;
    USHORT Size;
    GUID ScenarioId;
    ULONG DefaultPcrProfile;
} FVE_DEFAULT_PCR_PROFILE_RESPONSE, *PFVE_DEFAULT_PCR_PROFILE_RESPONSE;

/**
 * Describes a request for the prediction instance mapping of a volume.
 */
typedef struct _FVE_PREDICTION_INSTANCE_MAPPING_REQUEST
{
    USHORT Version;
    USHORT Size;
    HANDLE VolumeHandle;
} FVE_PREDICTION_INSTANCE_MAPPING_REQUEST, *PFVE_PREDICTION_INSTANCE_MAPPING_REQUEST;

/**
 * Describes the response containing the prediction instance mapping of a volume.
 */
typedef struct _FVE_PREDICTION_INSTANCE_MAPPING_RESPONSE
{
    USHORT Version;
    USHORT Size;
    WCHAR PredictionInstance[64];
} FVE_PREDICTION_INSTANCE_MAPPING_RESPONSE, *PFVE_PREDICTION_INSTANCE_MAPPING_RESPONSE;

/**
 * The FveQuery routine performs the specified FVE query and returns the result.
 *
 * \param[in] QueryType The FVE query type.
 * \param[in] InputBuffer The input buffer.
 * \param[in] InputSize The size, in bytes, of the input buffer.
 * \param[out] OutputBuffer Receives the output buffer.
 * \param[in,out] OutputSize On input, the output buffer size in bytes; receives the required or returned size.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 * \remarks NULL output with zero initial capacity supports size-only requests for types 5, 6, 8, 9, 10, 11 and 13.
 * Type 15 is feature-gated and can return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION) without changing the size.
 * Type 12 does not provide a reliable required-size query; type 14 requires a real output buffer.
 * A nonnull zero-capacity buffer does not establish a size-only operation.
 */
NTSYSAPI
HRESULT
NTAPI
FveQuery(
    _In_ FVE_QUERY_TYPE QueryType,
    _In_reads_bytes_opt_(InputSize) PBYTE InputBuffer,
    _In_ ULONG InputSize,
    _Out_writes_bytes_opt_(*OutputSize) PBYTE OutputBuffer,
    _Inout_ PULONG OutputSize
    );

/**
 * Identifies the type of control operation performed by FveControl.
 */
typedef enum _FVE_CONTROL_TYPE
{
    FVE_CONTROL_UNKNOWN = 0,
    FVE_CONTROL_PROTECT_WITH_EK,
    FVE_CONTROL_CLEAR_KEYS_FROM_KEYRING,
    FVE_CONTROL_SET_DEFAULT_PCR_PROFILE,
    FVE_CONTROL_TMCORE_PROVISION,
    FVE_CONTROL_SET_PREDICTION_INSTANCE_MAPPING,
    FVE_CONTROL_PCRMON_ONBOOT,
    FVE_CONTROL_PCRMON_ONSHUTDOWN,
    FVE_CONTROL_PCRMON_ONHIBERNATE,
    FVE_CONTROL_PCRMON_GET_BOOTTIME_PCRUNSEALINFO,
    FVE_CONTROL_MAX
} FVE_CONTROL_TYPE, *PFVE_CONTROL_TYPE;

/**
 * Describes a request to protect a volume with an external key.
 * Layout verified on 10.0.26100.9278; 10.0.28000.2804 requires 1112 / 1108 input bytes on x64 / x86.
 */
typedef struct _FVE_CTL_PROTECT_WITH_EK_REQUEST
{
    USHORT Version;
    USHORT Size;
    union
    {
        ULONG RequestFlags;
        struct
        {
            BOOLEAN UseWatermark : 1;
            BOOLEAN UsedSpaceOnly : 1;
            BOOLEAN WaitForCompletion : 1;
            BOOLEAN WcosScenario : 1;
            BOOLEAN ProtectedDestination : 1;
            BOOLEAN EnableAutoUnlock : 1;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    LONG FveMethod;
    USHORT EkAukFlags;
    WCHAR VolumeName[MAX_PATH];
    WCHAR EkDestName[MAX_PATH];
} FVE_CTL_PROTECT_WITH_EK_REQUEST, *PFVE_CTL_PROTECT_WITH_EK_REQUEST;

/**
 * Describes the response from protecting a volume with an external key.
 * Layout verified on 10.0.26100.9278; 10.0.28000.2804 reports a required output size of 604 bytes.
 */
typedef struct _FVE_CTL_PROTECT_WITH_EK_RESPONSE
{
    USHORT Version;
    USHORT Size;
    union
    {
        ULONG ResponseFlags;
        struct
        {
            BOOLEAN AlreadyInitialized : 1;
            BOOLEAN AlreadyHasEk : 1;
            BOOLEAN AlreadyHasAutoUnlockEnabled : 1;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    LONG FveMethod;
    ULONG FveMethodFlags;
    WCHAR EkFileName[MAX_PATH];
} FVE_CTL_PROTECT_WITH_EK_RESPONSE, *PFVE_CTL_PROTECT_WITH_EK_RESPONSE;

/**
 * Describes a request to set the default PCR profile of a volume.
 */
typedef struct _FVE_CTL_SET_DEFAULT_PCR_PROFILE_REQUEST
{
    USHORT Version;
    USHORT Size;
    HANDLE VolumeHandle;
    GUID ScenarioId;
    ULONG DefaultPcrProfile;
} FVE_CTL_SET_DEFAULT_PCR_PROFILE_REQUEST, *PFVE_CTL_SET_DEFAULT_PCR_PROFILE_REQUEST;

/**
 * Describes a request to provision a volume using the TPM core.
 */
typedef struct _FVE_CTL_TMCORE_PROVISION_REQUEST
{
    USHORT Version;
    USHORT Size;
    PCWSTR VolumePath;
    ULONG VolumeInitFlags;
    PVOID pFveTpmApiSurface;
    PULONG PcrBitmapToSeal;
} FVE_CTL_TMCORE_PROVISION_REQUEST, *PFVE_CTL_TMCORE_PROVISION_REQUEST;

/**
 * Describes a request to set the prediction instance mapping of a volume.
 */
typedef struct _FVE_CTL_SET_PREDICTION_INSTANCE_MAPPING
{
    USHORT Version;
    USHORT Size;
    HANDLE VolumeHandle;
    WCHAR PredictionInstance[64];
} FVE_CTL_SET_PREDICTION_INSTANCE_MAPPING, *PFVE_CTL_SET_PREDICTION_INSTANCE_MAPPING;

/**
 * The FveControl routine performs the specified FVE control operation.
 *
 * \param[in] ControlType The FVE control type.
 * \param[in] InputBuffer The input buffer.
 * \param[in] InputSize The size, in bytes, of the input buffer.
 * \param[out] OutputBuffer Receives the output buffer.
 * \param[in,out] OutputSize On input, the output buffer size in bytes; receives the required or returned size.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 * \remarks Type 1 supports a NULL-output, zero-capacity size query before its state-changing implementation.
 * On 10.0.28000.2804 it requires 1112/1108 input bytes on x64/x86 and reports 604 output bytes.
 * The existing FVE_CTL_PROTECT_WITH_EK types describe the earlier 10.0.26100.9278 layouts.
 * A nonnull zero-capacity buffer does not establish a size-only operation.
 */
NTSYSAPI
HRESULT
NTAPI
FveControl(
    _In_ FVE_CONTROL_TYPE ControlType,
    _In_reads_bytes_opt_(InputSize) PBYTE InputBuffer,
    _In_ ULONG InputSize,
    _Out_writes_bytes_opt_(*OutputSize) PBYTE OutputBuffer,
    _Inout_ PULONG OutputSize
    );

/**
 * The FveApplyNkpCertChanges routine applies pending network key protector certificate changes to the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveApplyNkpCertChanges(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveGenerateNkpSessionKeys routine generates network key protector session keys for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGenerateNkpSessionKeys(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveGenerateNbp routine generates a network boot package for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] CertThumbprintSize The size, in bytes, of the certificate thumbprint.
 * \param[in] CertThumbprint The certificate thumbprint.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGenerateNbp(
    _In_ HANDLE FveVolumeHandle,
    _In_ DWORD CertThumbprintSize,
    _In_reads_bytes_(CertThumbprintSize) PBYTE CertThumbprint
    );

/**
 * The FveRegenerateNbpSessionKey routine regenerates the network boot package session key for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveRegenerateNbpSessionKey(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveCanStandardUsersChangePin routine determines whether standard users are permitted to change the PIN.
 *
 * \param[out] CanChangePin Receives a value indicating whether standard users can change the PIN.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCanStandardUsersChangePin(
    _Out_ PBOOL CanChangePin
    );

/**
 * The FveCanStandardUsersChangePassphraseByProxy routine determines whether standard users are permitted to change
 * the passphrase by proxy.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] CanChangePassphrase Receives a value indicating whether standard users can change the passphrase by
 * proxy.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCanStandardUsersChangePassphraseByProxy(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PBOOL CanChangePassphrase
    );

/**
 * The FveCheckPassphrasePolicy routine checks whether a passphrase satisfies the configured policy.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] Passphrase The recovery password (passphrase).
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCheckPassphrasePolicy(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCWSTR Passphrase
    );

/**
 * The FveDecrementClearKeyCounter routine decrements the clear-key reference counter for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveDecrementClearKeyCounter(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveGetClearKeyCounter routine retrieves the clear-key reference counter for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] ClearKeyCounter Receives the clear-key reference counter.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetClearKeyCounter(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PULONG ClearKeyCounter
    );

/**
 * The FveAddAuthMethodSid routine adds a SID-based authentication method to the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] FriendlyName The friendly name of the authentication method.
 * \param[in] Sid The security identifier (SID).
 * \param[in] Flags The operation flags.
 * \param[out] AuthMethodGuid Receives the GUID identifying the authentication method.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveAddAuthMethodSid(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCWSTR FriendlyName,
    _In_ PSID Sid,
    _In_ USHORT Flags,
    _Out_ PGUID AuthMethodGuid
    );

/**
 * The FveGetAuthMethodSid routine retrieves the SID-based authentication methods on the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] Sid The SID whose authentication methods are queried.
 * \param[out] AuthMethodGuids Receives the buffer that receives the authentication method GUIDs.
 * \param[in,out] AuthMethodCount On input, the GUID array capacity; receives the required or returned count.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetAuthMethodSid(
    _In_ HANDLE FveVolumeHandle,
    _In_ PSID Sid,
    _Out_writes_to_opt_(*AuthMethodCount, *AuthMethodCount) PGUID AuthMethodGuids,
    _Inout_ PULONG AuthMethodCount
    );

/**
 * The FveUnlockVolumeAuthMethodSid routine unlocks the specified volume using a SID-based authentication method.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] AuthMethodGuid The GUID identifying the authentication method.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveUnlockVolumeAuthMethodSid(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCGUID AuthMethodGuid
    );

/**
 * The FveGetAuthMethodSidInformation routine retrieves information about a SID-based authentication method.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] AuthMethodGuid The GUID identifying the authentication method.
 * \param[out] Flags Receives the operation flags.
 * \param[out] Sid Receives the security identifier (SID).
 * \param[in,out] SidBufferSize On input, the SID buffer size in bytes; receives the required or returned size.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetAuthMethodSidInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCGUID AuthMethodGuid,
    _Out_ PUSHORT Flags,
    _Out_writes_bytes_opt_(*SidBufferSize) PSID Sid,
    _Inout_ PULONG SidBufferSize
    );

#define FVE_FIND_VERSION_1 1

/**
 * Contains information about a volume returned during volume enumeration.
 */
typedef struct _FVE_FIND_DATA_V1
{
    ULONG FveFindVersion;
    FVE_DEVICE_TYPE DevType;
} FVE_FIND_DATA_V1, *PFVE_FIND_DATA_V1;

typedef const FVE_FIND_DATA_V1 *PCFVE_FIND_DATA_V1;

/**
 * The FveFindFirstVolume routine begins enumeration of FVE volumes and returns the first volume.
 *
 * \param[out] FveFindHandle Receives a volume enumeration (find) handle.
 * \param[in,out] FindData Optional initialized find-data structure; receives information about the first volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveFindFirstVolume(
    _Out_ PHANDLE FveFindHandle,
    _Inout_opt_ PFVE_FIND_DATA_V1 FindData
    );

/**
 * The FveFindNextVolume routine continues enumeration of FVE volumes and returns the next volume.
 *
 * \param[in] FveFindHandle A volume enumeration (find) handle.
 * \param[in,out] FindData Optional initialized find-data structure; receives information about the next volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveFindNextVolume(
    _In_ HANDLE FveFindHandle,
    _Inout_opt_ PFVE_FIND_DATA_V1 FindData
    );

/**
 * The FveGetVolumeNameW routine retrieves the name of the volume associated with the specified handle.
 *
 * \param[in] FveHandle A handle to the FVE object.
 * \param[in,out] VolumeNameBufferCchLen On input, specifies the size, in characters, of the volume name buffer; on
 * output, receives the number of characters written or required.
 * \param[out] VolumeName Receives the volume name.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetVolumeNameW(
    _In_ HANDLE FveHandle,
    _Inout_ PULONG VolumeNameBufferCchLen,
    _Out_writes_opt_(*VolumeNameBufferCchLen) PWSTR VolumeName
    );

/**
 * The FveUpdateBandIdBcd routine updates the band identifier stored in the boot configuration data for the specified
 * volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveUpdateBandIdBcd(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveLogRecoveryReason routine logs the reason a volume entered recovery.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] RecoveryReason The recovery reason code.
 * \param[in] ApplicationPath The path of the application that triggered recovery.
 * \param[in] ChangedBcd A value indicating whether the boot configuration data changed.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveLogRecoveryReason(
    _In_ HANDLE FveVolumeHandle,
    _In_ DWORD RecoveryReason,
    _In_opt_ PCWSTR ApplicationPath,
    _In_ DWORD ChangedBcd
    );

/**
 * The FveIsSchemaExtInstalled routine determines whether the Active Directory schema extension is installed.
 *
 * \param[out] SchemaExtInstalled Receives a value indicating whether the schema extension is installed.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsSchemaExtInstalled(
    _Out_ PBOOL SchemaExtInstalled
    );

/**
 * Identifies the Secure Boot binding state of a volume.
 */
typedef enum _FVE_SECUREBOOT_BINDING_STATE
{
    FVE_SECUREBOOT_BINDING_UNKNOWN = -1,
    FVE_SECUREBOOT_BINDING_NOT_POSSIBLE = 0,
    FVE_SECUREBOOT_BINDING_DISABLED_BY_POLICY,
    FVE_SECUREBOOT_BINDING_POSSIBLE,
    FVE_SECUREBOOT_BINDING_BOUND
} FVE_SECUREBOOT_BINDING_STATE, *PFVE_SECUREBOOT_BINDING_STATE;

/**
 * The FveGetSecureBootBindingState routine retrieves the Secure Boot binding state.
 *
 * \param[out] BindingState Receives the Secure Boot binding state.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetSecureBootBindingState(
    _Out_ PFVE_SECUREBOOT_BINDING_STATE BindingState
    );

/**
 * The FveIsDeviceLockable routine determines whether the specified device can be locked.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsDeviceLockable(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveLockDevice routine locks the specified device.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveLockDevice(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveIsDeviceLockedOut routine determines whether the specified device is locked out.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] IsDeviceLocked Receives a value indicating whether the device is locked out.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsDeviceLockedOut(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PBOOL IsDeviceLocked
    );

/**
 * The FveValidateDeviceLockoutState routine validates the device lockout state of the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveValidateDeviceLockoutState(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveGetDeviceLockoutData routine retrieves the device lockout data for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] PerUserData Receives the per-user device lockout data.
 * \param[in,out] PerUserSize On input, the data buffer size in bytes; receives the required or returned size.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetDeviceLockoutData(
    _In_ HANDLE FveVolumeHandle,
    _Out_writes_bytes_opt_(*PerUserSize) PBYTE PerUserData,
    _Inout_ PULONG PerUserSize
    );

/**
 * The FveUpdateDeviceLockoutState routine updates the device lockout state of the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] PerUserData The per-user device lockout data.
 * \param[in] PerUserSize The size, in bytes, of the per-user device lockout data.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveUpdateDeviceLockoutState(
    _In_ HANDLE FveVolumeHandle,
    _In_reads_bytes_(PerUserSize) PBYTE PerUserData,
    _In_ ULONG PerUserSize
    );

/**
 * The FveUpdateDeviceLockoutStateEx routine updates the device lockout state of the specified volume with the given
 * flags.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] PerUserData The per-user device lockout data.
 * \param[in] PerUserSize The size, in bytes, of the per-user device lockout data.
 * \param[in] Flags The operation flags.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveUpdateDeviceLockoutStateEx(
    _In_ HANDLE FveVolumeHandle,
    _In_reads_bytes_(PerUserSize) PBYTE PerUserData,
    _In_ ULONG PerUserSize,
    _In_ ULONG Flags
    );

/**
 * The FveDisableDeviceLockoutState routine disables the device lockout state of the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveDisableDeviceLockoutState(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveRecalculateOffsetsAndMoveMetadata routine recalculates metadata offsets and moves the metadata for the
 * specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveRecalculateOffsetsAndMoveMetadata(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * The FveDeleteDeviceEncryptionOptOutForVolumeW routine deletes the device-encryption opt-out marker for the named
 * volume.
 *
 * \param[in] VolumePath The volume path.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveDeleteDeviceEncryptionOptOutForVolumeW(
    _In_ PCWSTR VolumePath
    );

/**
 * The FveGetExternalKeyBlob routine retrieves the external key blob for the current context.
 *
 * \param[out] Buffer Receives the buffer.
 * \param[out] BufferSize Receives the size, in bytes, of the buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetExternalKeyBlob(
    _Outptr_result_bytebuffer_(*BufferSize) PBYTE *Buffer,
    _Out_ PDWORD BufferSize
    );

/**
 * The FveEscrowEncryptedRecoveryKeyForRetailUnlock routine escrows the encrypted recovery key used for retail unlock.
 *
 * \param[in] Buffer The encrypted recovery-key buffer.
 * \param[in] BufferSize The size, in bytes, of the buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveEscrowEncryptedRecoveryKeyForRetailUnlock(
    _In_reads_bytes_(BufferSize) PBYTE Buffer,
    _In_ DWORD BufferSize
    );

/**
 * The FvepCanPinExceptionPolicyBeApplied routine determines whether the PIN exception policy can be applied.
 *
 * \param[out] Result Receives a value that receives the result.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FvepCanPinExceptionPolicyBeApplied(
    _Out_ PBOOL Result
    );

/**
 * The FveCanPinExceptionPolicyBeApplied routine determines whether the PIN exception policy can be applied.
 *
 * \param[out] Result Receives a value that receives the result.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCanPinExceptionPolicyBeApplied(
    _Out_ PBOOL Result
    );

/**
 * The FveResetTpmDictionaryAttackParameters routine resets the TPM dictionary attack mitigation parameters.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveResetTpmDictionaryAttackParameters(
    VOID
    );

/**
 * The FveCommitChangesEx routine commits pending changes to the specified volume for the given scenario.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] FveScenario The FVE commit scenario.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCommitChangesEx(
    _In_ HANDLE FveVolumeHandle,
    _In_ FVE_SCENARIO_TYPE FveScenario
    );

#define FVE_EXTERNAL_DATA_ENTRY_VERSION_1 1
#define FVE_EXTERNAL_DATA_ENTRY_DESCRIPTION_LENGTH 16

/**
 * Describes an external data entry stored on a volume.
 */
typedef struct _FVE_EXTERNAL_DATA_ENTRY_INFO_V1
{
    USHORT Size;
    USHORT Version;
    GUID EntryTypeId;
    GUID EntryId;
    WCHAR EntryLabel[FVE_EXTERNAL_DATA_ENTRY_DESCRIPTION_LENGTH];
    FILETIME DateTimeCreated;
} FVE_EXTERNAL_DATA_ENTRY_INFO_V1, *PFVE_EXTERNAL_DATA_ENTRY_INFO_V1;

/**
 * Pointer to a constant FVE_EXTERNAL_DATA_ENTRY_INFO_V1 structure.
 */
typedef const FVE_EXTERNAL_DATA_ENTRY_INFO_V1 *PCFVE_EXTERNAL_DATA_ENTRY_INFO_V1;

/**
 * Selects one or more external data entries by type and identifier.
 */
typedef struct _FVE_EXTERNAL_DATA_ENTRY_SELECT_V1
{
    USHORT Size;
    USHORT Version;
    ULONG SelectFlags;
    GUID EntryTypeId;
    GUID EntryId;
} FVE_EXTERNAL_DATA_ENTRY_SELECT_V1, *PFVE_EXTERNAL_DATA_ENTRY_SELECT_V1;

/**
 * Pointer to a constant FVE_EXTERNAL_DATA_ENTRY_SELECT_V1 structure.
 */
typedef const FVE_EXTERNAL_DATA_ENTRY_SELECT_V1 *PCFVE_EXTERNAL_DATA_ENTRY_SELECT_V1;

/**
 * The FveExternalDataCreateEntry routine creates an external data entry on the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] Flags Flags controlling creation of the external data entry.
 * \param[in,out] EntryInfo The entry information; receives the created entry identifier and timestamp.
 * \param[in] DataSize The size, in bytes, of the raw entry data.
 * \param[in] Data The raw entry data.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveExternalDataCreateEntry(
    _In_ HANDLE FveVolumeHandle,
    _In_ ULONG Flags,
    _Inout_ PFVE_EXTERNAL_DATA_ENTRY_INFO_V1 EntryInfo,
    _In_ USHORT DataSize,
    _In_reads_bytes_(DataSize) PBYTE Data
    );

/**
 * The FveExternalDataGetEntryRawData routine retrieves the raw data of an external data entry.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] Selection The external data entry selector.
 * \param[in] DataBufferSize The size, in bytes, of the buffer.
 * \param[out] DataSize Receives the number of bytes written.
 * \param[out] Data Receives the buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveExternalDataGetEntryRawData(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCFVE_EXTERNAL_DATA_ENTRY_SELECT_V1 Selection,
    _In_ USHORT DataBufferSize,
    _Out_ PUSHORT DataSize,
    _Out_writes_bytes_opt_(DataBufferSize) PBYTE Data
    );

/**
 * The FveExternalDataGetEntryInfo routine retrieves information about external data entries.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] Selection The external data entry selector.
 * \param[in] EntryInfoVersion The version of the entry information structure.
 * \param[in] EntryInfoBufferSize The size, in bytes, of the buffer.
 * \param[out] RequiredSize Receives the number of bytes written.
 * \param[out] EntryCount Receives the number of entries returned.
 * \param[out] EntryInfo Receives the buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveExternalDataGetEntryInfo(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCFVE_EXTERNAL_DATA_ENTRY_SELECT_V1 Selection,
    _In_ USHORT EntryInfoVersion,
    _In_ ULONG EntryInfoBufferSize,
    _Out_ PULONG RequiredSize,
    _Out_ PUSHORT EntryCount,
    _Out_writes_bytes_opt_(EntryInfoBufferSize) PFVE_EXTERNAL_DATA_ENTRY_INFO_V1 EntryInfo
    );

/**
 * The FveExternalDataDeleteEntries routine deletes external data entries matching the specified selector.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] Selection The external data entry selector.
 * \param[out] DeletedEntryCount Receives the number of entries deleted.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveExternalDataDeleteEntries(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCFVE_EXTERNAL_DATA_ENTRY_SELECT_V1 Selection,
    _Out_opt_ PUSHORT DeletedEntryCount
    );

// rev: export ABIs verified on x64 and WOW64 10.0.26100.9278.
/**
 * Retrieves an internal encryption-state result for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \return A 32-bit state value or an HRESULT error. The state-value interpretation is not assigned here.
 */
NTSYSAPI
HRESULT
NTAPI
InternalFveIsVolumeEncrypted(
    _In_ HANDLE FveVolumeHandle
    );

/**
 * Checks DMA security for device encryption.
 *
 * \param[out] IsDmaSecure Receives the DMA-security result.
 * \param[in,out] HstiResults The opaque HSTI results.
 * \param[out] Information Optional output for an opaque name/value collection.
 * \return Returns S_OK if successful, or an HRESULT error.
 */
NTSYSAPI
HRESULT
NTAPI
NgscbCheckDmaSecurity(
    _Out_ PBOOLEAN IsDmaSecure,
    _Inout_ PNGSCB_HSTI_RESULTS HstiResults,
    _Outptr_opt_result_maybenull_ PNGSCB_NAME_VALUE_COLLECTION *Information
    );

/**
 * Checks DMA security and obtains additional device-encryption information.
 *
 * \param[out] IsDmaSecure Receives the DMA-security result.
 * \param[in,out] HstiResults The opaque HSTI results.
 * \param[out] Information Optional output for an opaque information collection.
 * \param[out] Capabilities Optional output for an opaque capabilities collection.
 * \return Returns S_OK if successful, or an HRESULT error.
 */
NTSYSAPI
HRESULT
NTAPI
NgscbCheckDmaSecurityEx(
    _Out_ PBOOLEAN IsDmaSecure,
    _Inout_ PNGSCB_HSTI_RESULTS HstiResults,
    _Outptr_opt_result_maybenull_ PNGSCB_NAME_VALUE_COLLECTION *Information,
    _Outptr_opt_result_maybenull_ PNGSCB_NAME_VALUE_COLLECTION *Capabilities
    );

/**
 * Checks whether the HSTI prerequisites are verified.
 *
 * \param[out] PrerequisitesVerified Receives the prerequisite-verification result.
 * \param[in] HstiResults The opaque HSTI results.
 * \return Returns S_OK if successful, or an HRESULT error.
 */
NTSYSAPI
HRESULT
NTAPI
NgscbCheckHSTIPrerequisitesVerified(
    _Out_ PBOOLEAN PrerequisitesVerified,
    _In_ PNGSCB_HSTI_RESULTS HstiResults
    );

/**
 * Checks whether the device supports always-on, always-connected operation.
 *
 * \param[out] IsAoacDevice Receives the AOAC-device result.
 * \return Returns S_OK if successful, or an HRESULT error.
 */
NTSYSAPI
HRESULT
NTAPI
NgscbCheckIsAOACDevice(
    _Out_ PBOOLEAN IsAoacDevice
    );

/**
 * Checks whether HSTI is verified.
 *
 * \param[out] IsHstiVerified Receives the HSTI-verification result.
 * \param[out] HstiResults Optional output for opaque HSTI results.
 * \param[out] ParsingStatus Optional opaque parsing-status buffer.
 * \return Returns S_OK if successful, or an HRESULT error.
 */
NTSYSAPI
HRESULT
NTAPI
NgscbCheckIsHSTIVerified(
    _Out_ PBOOLEAN IsHstiVerified,
    _Outptr_opt_result_maybenull_ PNGSCB_HSTI_RESULTS *HstiResults,
    _Out_opt_ PNGSCB_HSTI_PARSING_STATUS ParsingStatus
    );

/**
 * Checks whether device encryption is prevented.
 *
 * \param[out] PreventDeviceEncryption Receives the device-encryption restriction result.
 * \return Returns S_OK if successful, or an HRESULT error.
 */
NTSYSAPI
HRESULT
NTAPI
NgscbCheckPreventDeviceEncryption(
    _Out_ PBOOLEAN PreventDeviceEncryption
    );

/**
 * Checks whether device encryption is prevented for AAD.
 *
 * \param[out] PreventDeviceEncryption Receives the device-encryption restriction result.
 * \return Returns S_OK if successful, or an HRESULT error.
 */
NTSYSAPI
HRESULT
NTAPI
NgscbCheckPreventDeviceEncryptionForAad(
    _Out_ PBOOLEAN PreventDeviceEncryption
    );

/**
 * Retrieves the Windows recovery-environment configuration.
 *
 * \param[out] WinReAvailable Receives the Windows recovery-environment availability result.
 * \param[out] Configuration Optional configuration buffer.
 * \param[in] ConfigurationCch The configuration buffer capacity, in WCHARs.
 * \return Returns S_OK if successful, or an HRESULT error.
 */
NTSYSAPI
HRESULT
NTAPI
NgscbGetWinReConfiguration(
    _Out_ PBOOLEAN WinReAvailable,
    _Out_writes_opt_(ConfigurationCch) PWSTR Configuration,
    _In_ ULONG ConfigurationCch
    );

/**
 * Checks whether the host operating system resides on a roamable drive.
 *
 * \param[out] IsRoamable Receives the roamable-drive result.
 * \return Returns S_OK if successful, or an HRESULT error.
 */
NTSYSAPI
HRESULT
NTAPI
NgscbIsHostOsOnRoamableDrive(
    _Out_ PBOOL IsRoamable
    );

/**
 * Identifies the version of a TPM key protector.
 */
typedef enum _FVE_TPM_PROTECTOR_VERSION
{
    FveTpmProtectorVersion1 = 1,
    FveTpmProtectorVersion2 = 2,
    FveTpmProtectorVersionMax = 3
} FVE_TPM_PROTECTOR_VERSION, *PFVE_TPM_PROTECTOR_VERSION;

EXTERN_C_END

#endif // _FVEAPI_H
