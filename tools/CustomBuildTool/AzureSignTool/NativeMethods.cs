namespace AzureSign.Core.Interop
{
    [Flags]
    internal enum SignerSignEx3Flags : uint
    {
        NONE = 0x0,
        /// <summary>
        /// Exclude page hashes when creating SIP indirect data for the PE file. This flag takes precedence over the SPC_INC_PE_PAGE_HASHES_FLAG flag.
        /// </summary>
        SPC_EXC_PE_PAGE_HASHES_FLAG = 0x10,
        /// <summary>
        /// This value is not supported.
        /// </summary>
        SPC_INC_PE_IMPORT_ADDR_TABLE_FLAG = 0x20,
        /// <summary>
        /// This value is not supported.
        /// </summary>
        SPC_INC_PE_DEBUG_INFO_FLAG = 0x40,
        /// <summary>
        /// This value is not supported.
        /// </summary>
        SPC_INC_PE_RESOURCES_FLAG = 0x80,
        /// <summary>
        /// Include page hashes when creating SIP indirect data for the PE file.
        /// </summary>
        SPC_INC_PE_PAGE_HASHES_FLAG = 0x100,
        /// <summary>
        ///
        /// </summary>
        SPC_DIGEST_GENERATE_FLAG = 0x200,
        /// <summary>
        /// Digest signing will be done.
        /// </summary>
        SPC_DIGEST_SIGN_FLAG = 0x400,
        /// <summary>
        /// Define relaxed PE marker check semantic. Relaxed marker check flags, encoded as authenticated attribute
        /// SPC_RELAXED_PE_MARKER_CHECK_OBJID(1.3.6.1.4.1.311.2.6.1) of integer type.
        /// SPC_MARKER_CHECK_SKIP_SIP_INDIRECT_DATA_FLAG - If this flag is set, SIP_INDIRECT_DATA will be skipped for marker check.
        /// </summary>
        SPC_RELAXED_PE_MARKER_CHECK = 0x800,
        /// <summary>
        /// The signature will be nested.
        /// If you set this flag before any signature has been added, the generated signature will be added as the outer signature.
        /// If you do not set this flag, the generated signature replaces the outer signature, deleting all inner signatures.
        /// </summary>
        SIG_APPEND = 0x1000,
        /// <summary>
        ///
        /// </summary>
        SIG_SEAL = 0x2000,
        /// <summary>
        /// Digest signing will be done. The caller’s digest signing function will select and return the code signing certificate which performed the signing operation.
        /// </summary>
        SPC_DIGEST_SIGN_EX_FLAG = 0x4000
    }

    [StructLayout(LayoutKind.Sequential)]
    internal unsafe struct SIGNER_SIGN_PARAMS_EX2
    {
        internal SIGNER_SIGN_FLAGS dwFlags;
        internal SIGNER_SUBJECT_INFO* pSubjectInfo;
        internal SIGNER_CERT* pSignerCert;
        internal SIGNER_SIGNATURE_INFO* pSignatureInfo;
        internal SIGNER_PROVIDER_INFO* pProviderInfo;
        internal SIGNER_TIMESTAMP_FLAGS dwTimestampFlags;
        internal byte* pszTimestampAlgorithmOid;
        internal char* pwszTimestampURL;
        internal CRYPT_ATTRIBUTES* pCryptAttrs;
        internal APPX_SIP_CLIENT_DATA* pSipData;
        internal SIGNER_CONTEXT** ppSignerContext;
        internal CERT_STRONG_SIGN_PARA* pCryptoPolicy;
        internal IntPtr pReserved;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal unsafe struct SIGNER_SIGN_PARAMS_EX3
    {
        internal SIGNER_SIGN_FLAGS dwFlags;
        internal SIGNER_SUBJECT_INFO* pSubjectInfo;
        internal SIGNER_CERT* pSignerCert;
        internal SIGNER_SIGNATURE_INFO* pSignatureInfo;
        internal SIGNER_PROVIDER_INFO* pProviderInfo;
        internal SIGNER_TIMESTAMP_FLAGS dwTimestampFlags;
        internal byte* pszTimestampAlgorithmOid;
        internal char* pwszTimestampURL;
        internal CRYPT_ATTRIBUTES* psRequest;
        internal SIGNER_DIGEST_SIGN_INFO_UNION* pSignCallBack;
        internal SIGNER_CONTEXT** ppSignerContext;
        internal CERT_STRONG_SIGN_PARA* pCryptoPolicy;
        internal IntPtr pReserved;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal unsafe struct APPX_SIP_CLIENT_DATA
    {
        internal SIGNER_SIGN_EX_PARAMS* pSignerParams;
        internal IntPtr pAppxSipState; // IUnknown*
    }

    [StructLayout(LayoutKind.Explicit)]
    internal struct SIGNER_SIGN_EX_PARAMS
    {
        [FieldOffset(0)] internal SIGNER_SIGN_PARAMS_EX2 Ex2;
        [FieldOffset(0)] internal SIGNER_SIGN_PARAMS_EX3 Ex3;
    }

    /// <summary>
    /// The SIGNER_DIGEST_SIGN_INFO_V1 structure contains information about digest signing.
    /// It contains a pointer to a digest signing function implemented within the provided dll.
    /// https://learn.microsoft.com/en-us/windows/win32/seccrypto/signer-digest-sign-info-v1
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    internal unsafe struct SIGNER_DIGEST_SIGN_INFO_V1
    {
        /// <summary>
        /// The size, in bytes, of the structure.
        /// </summary>
        internal uint cbSize;
        /// <summary>
        /// The pointer to a PFN_AUTHENTICODE_DIGEST_SIGN function.
        /// </summary>
        internal delegate* unmanaged[Stdcall]<CERT_CONTEXT*, CRYPT_INTEGER_BLOB*, ALG_ID, byte*, uint, CRYPT_INTEGER_BLOB*, HRESULT> pfnAuthenticodeDigestSign;
        /// <summary>
        /// Optional pointer to CRYPT_DATA_BLOB specifying metadata (opaque to SignerSignEx3).
        /// </summary>
        internal CRYPT_INTEGER_BLOB* pMetadataBlob;
    }

    /// <summary>
    /// The SIGNER_DIGEST_SIGN_INFO_V2 structure contains information about digest signing. 
    /// It contains a pointer to a digest signing function implemented within the provided dll, specified by the dwDigestSignChoice.
    /// https://learn.microsoft.com/en-us/windows/win32/seccrypto/signer-digest-sign-info-v2
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    internal unsafe struct SIGNER_DIGEST_SIGN_INFO_V2
    {
        /// <summary>
        /// The size, in bytes, of the structure.
        /// </summary>
        internal uint cbSize;
        /// <summary>
        /// Pointer to the PFN_AUTHENTICODE_DIGEST_SIGN callback function. Required if the caller of SignerSignEx3 specifies SPC_DIGEST_SIGN_FLAG in the dwFlags parameter.
        /// </summary>
        internal delegate* unmanaged[Stdcall]<CERT_CONTEXT*, CRYPT_INTEGER_BLOB*, ALG_ID, byte*, uint, CRYPT_INTEGER_BLOB*, HRESULT> pfnAuthenticodeDigestSign;
        /// <summary>
        /// Pointer to the PFN_AUTHENTICODE_DIGEST_SIGN_EX callback function. Required if the caller of SignerSignEx3 specifies SPC_DIGEST_SIGN_EX_FLAG in the dwFlags parameter.
        /// </summary>
        internal delegate* unmanaged[Stdcall]<CRYPT_INTEGER_BLOB*, ALG_ID, byte*, uint, CRYPT_INTEGER_BLOB*, CERT_CONTEXT**, HCERTSTORE, HRESULT> pfnAuthenticodeDigestSignEx;
        /// <summary>
        /// Optional pointer to CRYPT_DATA_BLOB specifying metadata.
        /// </summary>
        internal CRYPT_INTEGER_BLOB* pMetadataBlob;
    }

    [StructLayout(LayoutKind.Explicit)]
    internal struct SIGNER_DIGEST_SIGN_INFO_UNION
    {
        [FieldOffset(0)] internal SIGNER_DIGEST_SIGN_INFO_V1 V1;
        [FieldOffset(0)] internal SIGNER_DIGEST_SIGN_INFO_V2 V2;
    }

    internal enum SIGNER_DIGEST_CHOICE : uint
    {
        DIGEST_SIGN = 1, // PFN_AUTHENTICODE_DIGEST_SIGN
        DIGEST_SIGN_WITHFILEHANDLE = 2, // PFN_AUTHENTICODE_DIGEST_SIGN_WITHFILEHANDLE
        DIGEST_SIGN_EX = 3, // PFN_AUTHENTICODE_DIGEST_SIGN_EX
        DIGEST_SIGN_EX_WITHFILEHANDLE = 4 // PFN_AUTHENTICODE_DIGEST_SIGN_EX_WITHFILEHANDLE
    }
}