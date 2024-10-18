using Windows.Win32.Security.Cryptography;

namespace AzureSign.Core.Interop
{
    [Flags]
    public enum SignerSignEx3Flags : uint
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
        SPC_DIGEST_SIGN_FLAG = 0x400, // 0x800
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
    public struct CRYPTOAPI_BLOB
    {
        public uint cbData;
        public IntPtr pbData;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal unsafe struct SIGNER_SIGN_EX3_PARAMS
    {
        public SIGNER_SIGN_FLAGS dwFlags;
        public SIGNER_SUBJECT_INFO* pSubjectInfo;
        public SIGNER_CERT* pSignerCert;
        public SIGNER_SIGNATURE_INFO* pSignatureInfo;
        public IntPtr pProviderInfo;
        public SIGNER_TIMESTAMP_FLAGS dwTimestampFlags;
        public byte* pszTimestampAlgorithmOid;
        public char* pwszHttpTimeStamp;
        public IntPtr psRequest;
        public SIGNER_DIGEST_SIGN_INFO_V1* pSignCallBack;
        public SIGNER_CONTEXT** ppSignerContext;
        public IntPtr pCryptoPolicy;
        public IntPtr pReserved;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct APPX_SIP_CLIENT_DATA
    {
        public unsafe SIGNER_SIGN_EX3_PARAMS* pSignerParams; // SIGNER_SIGN_EX2_PARAMS
        public IntPtr pAppxSipState; // IUnknown*
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct CRYPT_DATA_BLOB
    {
        public uint cbData;
        public void* pbData;
    }

    /// <summary>
    /// The SIGNER_DIGEST_SIGN_INFO_V1 structure contains information about digest signing.
    /// It contains a pointer to a digest signing function implemented within the provided dll.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct SIGNER_DIGEST_SIGN_INFO_V1
    {
        public static readonly uint SizeOf = (uint)Marshal.SizeOf<SIGNER_DIGEST_SIGN_INFO_V1>();

        /// <summary>
        /// The size, in bytes, of the structure.
        /// </summary>
        public uint Size;
        /// <summary>
        /// The pointer to a PFN_AUTHENTICODE_DIGEST_SIGN function.
        /// </summary>
        public IntPtr AuthenticodeDigestSign;
        /// <summary>
        /// Optional pointer to CRYPT_DATA_BLOB specifying metadata (opaque to SignerSignEx3).
        /// </summary>
        public CRYPT_DATA_BLOB* pMetadataBlob;

        public SIGNER_DIGEST_SIGN_INFO_V1(IntPtr Callback)
        {
            this.Size = (uint)Marshal.SizeOf(this);
            this.AuthenticodeDigestSign = Callback;
        }
    }
}