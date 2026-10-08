#ifndef _FVEAPI_H
#define _FVEAPI_H

/**
 * Pointer to a constant byte.
 */
typedef const BYTE *PCBYTE;

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
    _In_ UINT32 BufferLength,
    _In_ PCBYTE Buffer,
    _Out_ PUINT32 pcbResult,
    _Out_ PBYTE pabResult
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
 */
NTSYSAPI
HRESULT
NTAPI
FveSetupTpmCallback(
    _In_ PFVE_TPM_API_CALLBACK TpmCallback,
    _In_ UINT32 TpmVersion
    );

/**
 * Forward declaration of the predictions-updated context structure.
 */
struct _PPF_PREDICTIONS_UPDATED_CONTEXT;

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

/**
 * Contains a passphrase authentication element.
 */
typedef struct _FVE_AUTH_PASSPHRASE
{
    WCHAR ClearPassPhrase[256 + 1];
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
/**
 * Identifies the type of an authentication element.
 */
typedef enum _FVE_AUTH_ELEMENT_TYPE
{
    FVE_ELEMENT_TYPE_RECOVERY_PASSWORD      = 0x00000001, // FVE_AUTH_RECOVERY_PASSWORD (RecoveryPassword)
    FVE_ELEMENT_TYPE_PIN                    = 0x00000002, // FVE_AUTH_PIN (Pin)
    FVE_ELEMENT_TYPE_TPM                    = 0x00000003, // FVE_AUTH_TPM (Tpm)
    FVE_ELEMENT_TYPE_EXTERNAL_KEY           = 0x00000004, // FVE_AUTH_EXTERNAL_KEY (ExternalKey, .BEK file)
    FVE_ELEMENT_TYPE_PUBLIC_KEY             = 0x00000005, // FVE_AUTH_PUBLIC_KEY (PublicKey)
    FVE_ELEMENT_TYPE_PRIVATE_KEY            = 0x00000006, // FVE_AUTH_PRIVATE_KEY (PrivateKey)
    FVE_ELEMENT_TYPE_PUBLIC_KEY_INFO        = 0x00000007, // FVE_AUTH_INFO_PUBLIC_KEY (PublicKeyInfo / Certificate)
    FVE_ELEMENT_TYPE_PASSPHRASE             = 0x00000008, // FVE_AUTH_PASSPHRASE (PassPhrase)
    FVE_ELEMENT_TYPE_TPM_PIN                = 0x00000009, // Composite TPM + PIN element
    FVE_ELEMENT_TYPE_CLEAR_KEY              = 0x0000000A, // FVE_AUTH_INFO_CLEAR_KEY (ClearKeyInfo, Suspended Protection)
    FVE_ELEMENT_TYPE_DPAPI_NG               = 0x0000000B, // FVE_AUTH_DPAPI_NG (DpapiNgInfo)
    FVE_ELEMENT_TYPE_NETWORK_SERVER_INFO    = 0x0000000C, // FVE_AUTH_NETWORK_SERVER_INFO (NetworkServerInfo / Network Unlock)
    FVE_ELEMENT_TYPE_PREDICTED_TPM_INFO     = 0x0000000D  // FVE_AUTH_PREDICTED_TPM_INFO (PredictedTpmInfo / PCR7 & PCR4)
} FVE_AUTH_ELEMENT_TYPE, *PFVE_AUTH_ELEMENT_TYPE;

/**
 * Describes a single authentication element of a key protector.
 */
typedef struct _FVE_AUTH_ELEMENT
{
    ULONG Size;
    ULONG Version;
    ULONG ElementFlags;
    ULONG ElementType;
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
 * \param[in] bNeedWriteAccess A value indicating whether write access to the volume is required.
 * \param[out] FveVolumeHandle Receives a handle to the FVE volume.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveOpenVolumeW(
    _In_ PCWSTR VolumeName,
    _In_ BOOL bNeedWriteAccess,
    _Outptr_ PHANDLE FveVolumeHandle
    );

/**
 * The FveOpenVolumeExW routine opens the specified BitLocker (FVE) volume with extended options and returns a handle to it.
 *
 * \param[in] VolumeName The volume name.
 * \param[in] NameFlags Flags that qualify how the volume name is interpreted.
 * \param[in] bNeedWriteAccess A value indicating whether write access to the volume is required.
 * \param[in] IfcType The FVE interface type to open the volume with.
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
    _In_ BOOL bNeedWriteAccess,
    _In_ FVE_INTERFACE_TYPE IfcType,
    _In_ ULONG HandleFlags,
    _Outptr_ PHANDLE FveVolumeHandle
    );

/**
 * The FveOpenVolumeByHandle routine opens a BitLocker (FVE) volume from an existing handle and returns an FVE volume handle.
 *
 * \param[in] Handle A handle to the underlying object.
 * \param[in] HandleType The type of the supplied handle.
 * \param[in] bNeedWriteAccess A value indicating whether write access to the volume is required.
 * \param[in] IfcType The FVE interface type to open the volume with.
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
    _In_ BOOL bNeedWriteAccess,
    _In_ FVE_INTERFACE_TYPE IfcType,
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
 * \param[out] Status Receives the volume status information.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetStatus(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PFVE_STATUS_V9 Status
    );

/**
 * The FveGetStatusW routine retrieves the current encryption status of the named volume.
 *
 * \param[in] VolumeName The volume name.
 * \param[out] Status Receives the volume status information.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetStatusW(
    _In_ PCWSTR VolumeName,
    _Out_ PFVE_STATUS_V9 Status
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
 * \param[in] MaxNumGuids The maximum number of GUIDs the buffer can hold.
 * \param[out] NumGuids Receives the number of GUIDs returned.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetAuthMethodGuids(
    _In_ HANDLE FveVolumeHandle,
    _Out_ LPGUID AuthMethodGuids,
    _In_ UINT MaxNumGuids,
    _Out_ PUINT NumGuids
    );

/**
 * The FveGetAuthMethodInformation routine retrieves detailed information about an authentication method on the volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] Information Receives the authentication information.
 * \param[in] BufferSize The size, in bytes, of the buffer.
 * \param[out] RequiredSize Receives the size, in bytes, required for the buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetAuthMethodInformation(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PFVE_AUTH_INFORMATION Information,
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
    _In_ LPCGUID AuthMethodGuid
    );

/**
 * The FveAddAuthMethodInformation routine adds an authentication method to the volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] Information The authentication information.
 * \param[in] AuthMethodGuid The GUID identifying the authentication method.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveAddAuthMethodInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCFVE_AUTH_INFORMATION Information,
    _In_ LPGUID AuthMethodGuid
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
    _In_ LPCWSTR NewPin,
    _In_ LPCGUID ProtectorGuid
    );

/**
 * The FveValidateExistingPinW routine validates an existing PIN against the specified protector.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] ExistingPin The existing PIN.
 * \param[out] ExistingPinValidates Receives a value indicating whether the existing PIN validates.
 * \param[in] GUIDProtector The GUID identifying the protector.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveValidateExistingPinW(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCWSTR ExistingPin,
    _Out_ PBOOL ExistingPinValidates,
    _In_ LPGUID GUIDProtector
    );

/**
 * The FveValidateExistingPassphraseW routine validates an existing passphrase against the specified protector.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] ExistingPassphrase The existing passphrase.
 * \param[out] ExistingPassphraseValidates Receives a value indicating whether the existing passphrase validates.
 * \param[in] ProtectorGuid The GUID identifying the key protector.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveValidateExistingPassphraseW(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCWSTR ExistingPassphrase,
    _Out_ PBOOL ExistingPassphraseValidates,
    _In_ LPGUID ProtectorGuid
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
 * \param[in] ReadOnly A value indicating whether the volume is unlocked for read-only access.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveUnlockVolumeWithAccessMode(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCFVE_AUTH_INFORMATION Information,
    _In_ PBOOL ReadOnly
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
    _Out_ LPGUID IdentityGuid
    );

/**
 * The FveGetRecoveryPasswordBackupInformation routine retrieves the recovery-password backup information for a protector.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] ProtectorGuid The GUID identifying the key protector.
 * \param[out] BackupInfoTypeMask Receives the mask of recovery-password backup information types.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetRecoveryPasswordBackupInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_ LPCGUID ProtectorGuid,
    _Out_ PUSHORT BackupInfoTypeMask
    );

/**
 * The FveSetRecoveryPasswordBackupInformation routine sets the recovery-password backup information for a protector.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] ProtectorGuid The GUID identifying the key protector.
 * \param[in] BackupInfoType The recovery-password backup information type.
 * \param[in] SetFlags The backup information flags to set.
 * \param[in] ClearFlags The backup information flags to clear.
 * \param[out] DatasetWasUpdated Receives a value indicating whether the dataset was updated.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSetRecoveryPasswordBackupInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_ LPCGUID ProtectorGuid,
    _In_ USHORT BackupInfoType,
    _In_ USHORT SetFlags,
    _In_ USHORT ClearFlags,
    _Out_ PBOOLEAN DatasetWasUpdated
    );

/**
 * The FveClearRecoveryPasswordBackupInformation routine clears the recovery-password backup information for a protector.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] ProtectorGuid The GUID identifying the key protector.
 * \param[in] BackupInfoType The recovery-password backup information type.
 * \param[out] DatasetWasUpdated Receives a value indicating whether the dataset was updated.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveClearRecoveryPasswordBackupInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_ LPCGUID ProtectorGuid,
    _In_ USHORT BackupInfoType,
    _Out_ PBOOLEAN DatasetWasUpdated
    );

/**
 * The FveGetRecoveryPasswordBackupAccountInformation routine retrieves the account information used to back up recovery passwords.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] ProtectorGuid The GUID identifying the key protector.
 * \param[in] BackupInfoType The recovery-password backup information type.
 * \param[in] cchBackupAccount The size, in characters, of the backup account buffer.
 * \param[out] cchBackupAccountRequired Receives the size, in characters, required for the backup account buffer.
 * \param[out] BackupAccounts Receives the buffer that receives the backup account names.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetRecoveryPasswordBackupAccountInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_ LPCGUID ProtectorGuid,
    _In_ USHORT BackupInfoType,
    _In_ SIZE_T cchBackupAccount,
    _Out_ PSIZE_T cchBackupAccountRequired,
    _Out_ LPWSTR BackupAccounts
    );

/**
 * The FveSetRecoveryPasswordBackupAccountInformation routine sets the account information used to back up recovery passwords.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] ProtectorGuid The GUID identifying the key protector.
 * \param[in] BackupInfoType The recovery-password backup information type.
 * \param[in] BackupAccount The backup account name.
 * \param[out] DatasetWasUpdated Receives a value indicating whether the dataset was updated.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSetRecoveryPasswordBackupAccountInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_ LPCGUID ProtectorGuid,
    _In_ USHORT BackupInfoType,
    _In_ PCWSTR BackupAccount,
    _Out_ PBOOLEAN DatasetWasUpdated
    );

/**
 * The FveSelectBestRecoveryPasswordByBackupInformation routine selects the most appropriate recovery password based on backup information.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] ProtectorGuid The GUID identifying the key protector.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSelectBestRecoveryPasswordByBackupInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_ LPGUID ProtectorGuid
    );

/**
 * The FveAuthElementToRecoveryPasswordW routine converts an authentication element to its recovery-password string form.
 *
 * \param[in] AuthElement The authentication element.
 * \param[out] Passphrase Receives the recovery password (passphrase).
 * \param[in] PassphraseLength The size, in characters, of the passphrase buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveAuthElementToRecoveryPasswordW(
    _In_ PCFVE_AUTH_ELEMENT AuthElement,
    _Out_ PWSTR Passphrase,
    _In_ SIZE_T PassphraseLength
    );

/**
 * The FveAuthElementFromPinW routine builds an authentication element from a PIN.
 *
 * \param[in] Pin The PIN.
 * \param[out] AuthElement Receives the authentication element.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveAuthElementFromPinW(
    _In_ PCWSTR Pin,
    _Out_ PFVE_AUTH_ELEMENT AuthElement
    );

/**
 * The FveAuthElementFromPassPhraseW routine builds an authentication element from a passphrase.
 *
 * \param[in] PassPhrase The passphrase.
 * \param[out] AuthElement Receives the authentication element.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveAuthElementFromPassPhraseW(
    _In_ PCWSTR PassPhrase,
    _Out_ PFVE_AUTH_ELEMENT AuthElement
    );

/**
 * The FveAuthElementFromRecoveryPasswordW routine builds an authentication element from a recovery password.
 *
 * \param[in] Passphrase The recovery password (passphrase).
 * \param[out] AuthElement Receives the authentication element.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveAuthElementFromRecoveryPasswordW(
    _In_ PCWSTR Passphrase,
    _Out_ PFVE_AUTH_ELEMENT AuthElement
    );

/**
 * The FveIsRecoveryPasswordGroupValidW routine determines whether a recovery-password group is valid.
 *
 * \param[in] PassphraseGroup The recovery password group.
 * \param[out] IsValid Receives a value indicating whether the value is valid.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsRecoveryPasswordGroupValidW(
    _In_ PCWSTR PassphraseGroup,
    _Out_ PBOOLEAN IsValid
    );

/**
 * The FveIsRecoveryPasswordValidW routine determines whether a recovery password is valid.
 *
 * \param[in] Passphrase The recovery password (passphrase).
 * \param[out] IsValid Receives a value indicating whether the value is valid.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsRecoveryPasswordValidW(
    _In_ PCWSTR Passphrase,
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
 * \param[out] Information Receives the authentication information.
 * \param[in] BufferSize The size, in bytes, of the buffer.
 * \param[out] RequiredSize Receives the size, in bytes, required for the buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveAuthElementReadExternalKeyW(
    _In_ PCWSTR KeyFullFilePath,
    _Out_ PFVE_AUTH_INFORMATION Information,
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
 * The FveAuthElementWriteExternalKeyExW routine writes authentication information to an external key file for the specified volume identity.
 *
 * \param[in] FveIdentity The volume identity GUID.
 * \param[in] KeyFullFilePath The full path of the external key file.
 * \param[in] Information The authentication information.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveAuthElementWriteExternalKeyExW(
    _In_ LPGUID FveIdentity,
    _In_ PCWSTR KeyFullFilePath,
    _In_ PCFVE_AUTH_INFORMATION Information
    );

/**
 * The FveAuthElementGetKeyFileNameW routine retrieves the external key file name for the specified authentication information.
 *
 * \param[in] Information The authentication information.
 * \param[out] KeyFileName Receives the buffer that receives the external key file name.
 * \param[in] BufferLength The size, in characters, of the buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveAuthElementGetKeyFileNameW(
    _In_ PCFVE_AUTH_INFORMATION Information,
    _Out_ PWSTR KeyFileName,
    _In_ SIZE_T BufferLength
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
    _In_ PCWSTR DiscoveryVolumeType,
    _In_ ULONG InitializationFlags
    );

/**
 * The FveInitVolume routine initializes the specified volume for BitLocker.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] DiscoveryVolumeType The discovery volume type.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveInitVolume(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCWSTR DiscoveryVolumeType
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
 * \param[in] DEInitializationFlags Flags controlling device encryption initialization.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveInitializeDeviceEncryption2(
    _In_ HANDLE FveVolumeHandle,
    _In_ ULONG DEInitializationFlags
    );

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
 * \param[out] DeviceEncryptionSupport Receives the device encryption support information.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveQueryDeviceEncryptionSupport(
    _Out_ PFVE_DE_SUPPORT DeviceEncryptionSupport
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
    _Out_ PULONG FlagsOut
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
 * The FveConversionEncryptPendingReboot routine schedules encryption of the specified volume to begin after the next reboot.
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
 * The FveConversionEncryptPendingRebootEx routine schedules encryption of the specified volume to begin after the next reboot with the given conversion flags.
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
    _Out_ PINT FveMethod
    );

/**
 * The FveGetFveMethodEDrv routine retrieves the encryption method, including the eDrive method, for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] FveMethod Receives the FVE encryption method.
 * \param[out] SelfEncryptionDriveEncryptionMethod Receives the buffer that receives the self-encrypting drive encryption method.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetFveMethodEDrv(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PINT FveMethod,
    _Out_ LPWSTR SelfEncryptionDriveEncryptionMethod
    );

/**
 * The FveGetFveMethodEx routine retrieves extended encryption method information for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] FveMethod Receives the FVE encryption method.
 * \param[out] eDriveMethod Receives the buffer that receives the eDrive (hardware) encryption method.
 * \param[out] FveMethodFlags Receives flags describing the FVE encryption method.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetFveMethodEx(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PINT FveMethod,
    _Out_ LPWSTR eDriveMethod,
    _Out_ PULONG FveMethodFlags
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
    _In_ LONG FveMethod
    );

/**
 * The FveSetFveMethodEx routine sets the encryption method, strength, and flags for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] FveMethod The FVE encryption method.
 * \param[in] FveMethodStrength The FVE encryption method strength.
 * \param[in] FveMethodFlags Flags describing the FVE encryption method.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSetFveMethodEx(
    _In_ HANDLE FveVolumeHandle,
    _In_ FVE_METHOD FveMethod,
    _In_ FVE_METHOD_STRENGTH FveMethodStrength,
    _In_ ULONG FveMethodFlags
    );

/**
 * The FveCheckTpmCapability routine checks the capabilities of the platform TPM.
 *
 * \param[out] Capability Receives the TPM capability information.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCheckTpmCapability(
    _Out_ PFVE_TPM_CAPS Capability
    );

/**
 * The FveBindDataVolume routine binds the specified data volume for automatic unlock.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] AuthMethodGUID The GUID identifying the authentication method.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveBindDataVolume(
    _In_ HANDLE FveVolumeHandle,
    _In_ LPCGUID AuthMethodGUID
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
 * \param[out] UnlockGUID Receives the GUID of the automatic unlock protector.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsBoundDataVolume(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PBOOL IsAutoUnlockEnabled,
    _Out_ LPGUID UnlockGUID
    );

/**
 * The FveIsBoundDataVolumeToOSVolume routine determines whether the specified data volume is bound to the operating system volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] IsAutoUnlockEnabled Receives a value indicating whether automatic unlock is enabled.
 * \param[out] UnlockGUID Receives the GUID of the automatic unlock protector.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsBoundDataVolumeToOSVolume(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PBOOL IsAutoUnlockEnabled,
    _Out_ LPGUID UnlockGUID
    );

/**
 * The FveIsAnyDataVolumeBoundToOSVolume routine determines whether any data volume is bound to the operating system volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] Count Receives the number of bound data volumes.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsAnyDataVolumeBoundToOSVolume(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PULONG Count
    );

/**
 * The FveUnbindAllDataVolumeFromOSVolume routine removes the automatic-unlock binding of all data volumes from the operating system volume.
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
    _Out_ PWSTR VolumeDescription,
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
 * \param[out] RequiredSize Receives the size, in bytes, required for the buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetIdentificationFieldW(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PWSTR IdentificationField,
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
    _In_ LPCGUID Identifier,
    _Out_ PUCHAR Buffer,
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
 * The FveEnableRawAccessEx routine enables or disables raw access to the specified volume, optionally forcing a dismount.
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
 * The FveBackupRecoveryInformationToAD routine backs up recovery information for the specified protector to Active Directory.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] AuthMethodGUID The GUID identifying the authentication method.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveBackupRecoveryInformationToAD(
    _In_ HANDLE FveVolumeHandle,
    _In_ LPCGUID AuthMethodGUID
    );

/**
 * The FveBackupRecoveryInformationToADEx routine backs up recovery information for the specified protector to Active Directory with the given flags.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] AuthMethodGUID The GUID identifying the authentication method.
 * \param[in] FveBackupFlags Flags controlling the Active Directory backup.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveBackupRecoveryInformationToADEx(
    _In_ HANDLE FveVolumeHandle,
    _In_ LPCGUID AuthMethodGUID,
    _In_ ULONG FveBackupFlags
    );

/**
 * The FveBackupRecoveryInformationToAAD routine backs up recovery information for the specified protector to Azure Active Directory.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] AuthMethodGUID The GUID identifying the authentication method.
 * \param[in] FveBackupPolicyFlags Policy flags controlling the Azure AD backup.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveBackupRecoveryInformationToAAD(
    _In_ HANDLE FveVolumeHandle,
    _In_ LPCGUID AuthMethodGUID,
    _In_ ULONG FveBackupPolicyFlags
    );

/**
 * The FveCheckADRecoveryInfoBackupPolicy routine retrieves the Active Directory recovery-information backup policy for the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] ADPolicy Receives the Active Directory recovery-information backup policy.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCheckADRecoveryInfoBackupPolicy(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PADA_GP_OPTIONS ADPolicy
    );

/**
 * The FveCheckADRecoveryInfoBackupPolicyEx routine retrieves the Active Directory recovery-information backup policy for each volume class.
 *
 * \param[out] ADPolicyOs Receives the Active Directory backup policy for operating system volumes.
 * \param[out] ADPolicyFdv Receives the Active Directory backup policy for fixed data volumes.
 * \param[out] ADPolicyRdv Receives the Active Directory backup policy for removable data volumes.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCheckADRecoveryInfoBackupPolicyEx(
    _Out_ PADA_GP_OPTIONS ADPolicyOs,
    _Out_ PADA_GP_OPTIONS ADPolicyFdv,
    _Out_ PADA_GP_OPTIONS ADPolicyRdv
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
    _Out_ PUCHAR DataSetBuffer,
    _In_ SIZE_T DataSetBufferSize,
    _Out_ PSIZE_T ActualDataSetBufferSize
    );

/**
 * The FveGetDataSetEx routine retrieves the FVE metadata dataset for the specified volume, optionally ignoring the locked-volume check.
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
    _Out_ PUCHAR DataSetBuffer,
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
 * \param[in] pRecoveryPasswordGuid The GUID identifying the recovery password protector.
 * \param[in] pRecoveryPassword The recovery password authentication element.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveSaveRecoveryPasswordBackupFlag(
    _In_ HANDLE FveVolumeHandle,
    _In_ LPCGUID pRecoveryPasswordGuid,
    _In_ PCFVE_AUTH_ELEMENT pRecoveryPassword
    );

/**
 * The FveDraCertPresentInRegistry routine determines whether a data recovery agent certificate is present in the registry.
 *
 * \param[out] ptCertPresent Receives a value indicating whether a data recovery agent certificate is present.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveDraCertPresentInRegistry(
    _Out_ PBOOL ptCertPresent
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
    _In_ struct _PPF_PREDICTIONS_UPDATED_CONTEXT *Context
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
    _In_ struct _PPF_PREDICTIONS_UPDATED_CONTEXT *Context
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
 * \param[in] FveQueryType The FVE query type.
 * \param[in] InputBuffer The input buffer.
 * \param[in] InputSize The size, in bytes, of the input buffer.
 * \param[out] OutputBuffer Receives the output buffer.
 * \param[out] OutputSize Receives the size, in bytes, of the output buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveQuery(
    _In_ FVE_QUERY_TYPE FveQueryType,
    _In_ PBYTE InputBuffer,
    _In_ ULONG InputSize,
    _Out_ PBYTE OutputBuffer,
    _Out_ PULONG OutputSize
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
 * \param[in] FveControlType The FVE control type.
 * \param[in] InputBuffer The input buffer.
 * \param[in] InputSize The size, in bytes, of the input buffer.
 * \param[out] OutputBuffer Receives the output buffer.
 * \param[out] OutputSize Receives the size, in bytes, of the output buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveControl(
    _In_ FVE_CONTROL_TYPE FveControlType,
    _In_ PBYTE InputBuffer,
    _In_ ULONG InputSize,
    _Out_ PBYTE OutputBuffer,
    _Out_ PULONG OutputSize
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
    _In_ ULONG CertThumbprintSize,
    _In_ PBYTE CertThumbprint
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
 * \param[out] ptStandardUsersCanChangePin Receives a value indicating whether standard users can change the PIN.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCanStandardUsersChangePin(
    _Out_ PBOOL ptStandardUsersCanChangePin
    );

/**
 * The FveCanStandardUsersChangePassphraseByProxy routine determines whether standard users are permitted to change the passphrase by proxy.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] ptStandardUsersCanChangePassphraseByProxy Receives a value indicating whether standard users can change the passphrase by proxy.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveCanStandardUsersChangePassphraseByProxy(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PBOOL ptStandardUsersCanChangePassphraseByProxy
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
    _Out_ LPGUID AuthMethodGuid
    );

/**
 * The FveGetAuthMethodSid routine retrieves the SID-based authentication methods on the specified volume.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[out] Sid Receives the security identifier (SID).
 * \param[out] AuthMethodGuidArray Receives the buffer that receives the authentication method GUIDs.
 * \param[out] AuthMethodCount Receives the number of authentication methods.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetAuthMethodSid(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PSID Sid,
    _Out_ LPGUID AuthMethodGuidArray,
    _Out_ PULONG AuthMethodCount
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
    _In_ LPCGUID AuthMethodGuid
    );

/**
 * The FveGetAuthMethodSidInformation routine retrieves information about a SID-based authentication method.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] AuthMethodGuid The GUID identifying the authentication method.
 * \param[out] Flags Receives the operation flags.
 * \param[out] Sid Receives the security identifier (SID).
 * \param[out] SidBufferSize Receives the size, in bytes, of the SID buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetAuthMethodSidInformation(
    _In_ HANDLE FveVolumeHandle,
    _In_ LPCGUID AuthMethodGuid,
    _Out_ PUSHORT Flags,
    _Out_ PSID Sid,
    _Out_ PULONG SidBufferSize
    );

/**
 * Contains information about a volume returned during volume enumeration.
 */
typedef struct _FVE_FIND_DATA_V1
{
    ULONG FveFindVersion;
    FVE_DEVICE_TYPE DevType;
} FVE_FIND_DATA_V1, *PFVE_FIND_DATA_V1;

/**
 * The FveFindFirstVolume routine begins enumeration of FVE volumes and returns the first volume.
 *
 * \param[out] FveFindHandle Receives a volume enumeration (find) handle.
 * \param[out] FindData Receives the volume find data.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveFindFirstVolume(
    _Out_ PHANDLE FveFindHandle,
    _Out_ PFVE_FIND_DATA_V1 FindData
    );

/**
 * The FveFindNextVolume routine continues enumeration of FVE volumes and returns the next volume.
 *
 * \param[in] FveFindHandle A volume enumeration (find) handle.
 * \param[out] FindData Receives the volume find data.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveFindNextVolume(
    _In_ HANDLE FveFindHandle,
    _Out_ PFVE_FIND_DATA_V1 FindData
    );

/**
 * The FveGetVolumeNameW routine retrieves the name of the volume associated with the specified handle.
 *
 * \param[in] FveHandle A handle to the FVE object.
 * \param[in, out] VolumeNameBufferCchLen On input, specifies the size, in characters, of the volume name buffer; on output, receives the number of characters written or required.
 * \param[out] VolumeName Receives the volume name.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetVolumeNameW(
    _In_ HANDLE FveHandle,
    _Inout_ PULONG VolumeNameBufferCchLen,
    _Out_ LPWSTR VolumeName
    );

/**
 * The FveUpdateBandIdBcd routine updates the band identifier stored in the boot configuration data for the specified volume.
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
    _In_ ULONG RecoveryReason,
    _In_ PCWSTR ApplicationPath,
    _In_ ULONG ChangedBcd
    );

/**
 * The FveIsSchemaExtInstalled routine determines whether the Active Directory schema extension is installed.
 *
 * \param[out] SchemExtInstalled Receives a value indicating whether the schema extension is installed.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveIsSchemaExtInstalled(
    _Out_ PBOOL SchemExtInstalled
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
 * \param[out] SecureBootBindingState Receives the Secure Boot binding state.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetSecureBootBindingState(
    _Out_ PFVE_SECUREBOOT_BINDING_STATE SecureBootBindingState
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
 * \param[out] PerUserSize Receives the size, in bytes, of the per-user device lockout data.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveGetDeviceLockoutData(
    _In_ HANDLE FveVolumeHandle,
    _Out_ PBYTE PerUserData,
    _Out_ PULONG PerUserSize
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
    _In_ PBYTE PerUserData,
    _In_ ULONG PerUserSize
    );

/**
 * The FveUpdateDeviceLockoutStateEx routine updates the device lockout state of the specified volume with the given flags.
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
    _In_ PBYTE PerUserData,
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
 * The FveRecalculateOffsetsAndMoveMetadata routine recalculates metadata offsets and moves the metadata for the specified volume.
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
 * The FveDeleteDeviceEncryptionOptOutForVolumeW routine deletes the device-encryption opt-out marker for the named volume.
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
    _Out_ PBYTE *Buffer,
    _Out_ PULONG BufferSize
    );

/**
 * The FveEscrowEncryptedRecoveryKeyForRetailUnlock routine escrows the encrypted recovery key used for retail unlock.
 *
 * \param[out] Buffer Receives the buffer.
 * \param[in] BufferSize The size, in bytes, of the buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveEscrowEncryptedRecoveryKeyForRetailUnlock(
    _Out_ PBYTE Buffer,
    _In_ ULONG BufferSize
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

/**
 * Describes an external data entry stored on a volume.
 */
typedef struct _FVE_EXTERNAL_DATA_ENTRY_INFO_V1
{
    USHORT Size;
    USHORT Version;
    GUID EntryTypeId;
    GUID EntryId;
    WCHAR EntryLabel[16];
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
 * \param[in] CreateEntryFlags Flags controlling creation of the external data entry.
 * \param[in] NewEntryInfo The information describing the new external data entry.
 * \param[in] RawDataSizeBytes The size, in bytes, of the raw entry data.
 * \param[in] RawData The raw entry data.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveExternalDataCreateEntry(
    _In_ HANDLE FveVolumeHandle,
    _In_ ULONG CreateEntryFlags,
    _In_ PFVE_EXTERNAL_DATA_ENTRY_INFO_V1 NewEntryInfo,
    _In_ USHORT RawDataSizeBytes,
    _In_ PUCHAR RawData
    );

/**
 * The FveExternalDataGetEntryRawData routine retrieves the raw data of an external data entry.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] EntrySelect The external data entry selector.
 * \param[in] BufferSizeBytes The size, in bytes, of the buffer.
 * \param[out] OutSizeBytes Receives the number of bytes written.
 * \param[out] Buffer Receives the buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveExternalDataGetEntryRawData(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCFVE_EXTERNAL_DATA_ENTRY_SELECT_V1 EntrySelect,
    _In_ USHORT BufferSizeBytes,
    _Out_ PUSHORT OutSizeBytes,
    _Out_ PUCHAR Buffer
    );

/**
 * The FveExternalDataGetEntryInfo routine retrieves information about external data entries.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] EntrySelect The external data entry selector.
 * \param[in] EntryInfoStructVersion The version of the entry information structure.
 * \param[in] BufferSizeBytes The size, in bytes, of the buffer.
 * \param[out] OutSizeBytes Receives the number of bytes written.
 * \param[out] OutEntryCount Receives the number of entries returned.
 * \param[out] Buffer Receives the buffer.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveExternalDataGetEntryInfo(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCFVE_EXTERNAL_DATA_ENTRY_SELECT_V1 EntrySelect,
    _In_ USHORT EntryInfoStructVersion,
    _In_ ULONG BufferSizeBytes,
    _Out_ PULONG OutSizeBytes,
    _Out_ PUSHORT OutEntryCount,
    _Out_ PFVE_EXTERNAL_DATA_ENTRY_INFO_V1 Buffer
    );

/**
 * The FveExternalDataDeleteEntries routine deletes external data entries matching the specified selector.
 *
 * \param[in] FveVolumeHandle A handle to the FVE volume.
 * \param[in] EntrySelect The external data entry selector.
 * \param[out] DeletedEntryCount Receives the number of entries deleted.
 * \return Returns S_OK if successful, or an appropriate HRESULT error code otherwise.
 */
NTSYSAPI
HRESULT
NTAPI
FveExternalDataDeleteEntries(
    _In_ HANDLE FveVolumeHandle,
    _In_ PCFVE_EXTERNAL_DATA_ENTRY_SELECT_V1 EntrySelect,
    _Out_ PUSHORT DeletedEntryCount
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

#endif // _FVEAPI_H
