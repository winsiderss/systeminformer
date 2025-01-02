namespace CustomBuildTool
{
    /// <summary>
    /// Signs a file with an Authenticode signature.
    /// </summary>
    public unsafe class AuthenticodeKeyVaultSigner : IDisposable
    {
        private readonly AsymmetricAlgorithm SigningAlgorithm;
        private readonly X509Certificate2 SigningCertificate;
        private readonly HashAlgorithmName FileDigestAlgorithm;
        private readonly TimeStampConfiguration TimeStampConfiguration;
        private readonly MemoryCertificateStore CertificateStore;
        private X509Chain CertificateChain;

        /// <summary>
        /// The PFN_AUTHENTICODE_DIGEST_SIGN user supplied callback function implements digest signing.
        /// This function is currently called by SignerSignEx3 for digest signing.
        /// https://learn.microsoft.com/en-us/windows/win32/seccrypto/pfn-authenticode-digest-sign
        /// </summary>
        /// <param name="pSigningCert">A pointer to a CERT_CONTEXT structure that specifies the certificate used to create the digital signature.</param>
        /// <param name="pMetadataBlob">Pointer to a CRYPT_DATA_BLOB structure that contains metadata for digest signing.</param>
        /// <param name="digestAlgId">Specifies the digest algorithm to be used for digest signing.</param>
        /// <param name="pbToBeSignedDigest">Pointer to a buffer which contains the digest to be signed.</param>
        /// <param name="cbToBeSignedDigest">The size, in bytes, of the pbToBeSignedDigest buffer.</param>
        /// <param name="pSignedDigest">Pointer to CRYPT_DATA_BLOB which receives the signed digest.</param>
        /// <returns>If the function succeeds, the function returns S_OK. If the function fails, it returns an HRESULT value that indicates the error.</returns>
        //[UnmanagedFunctionPointer(CallingConvention.Winapi)]
        //internal delegate HRESULT PFN_AUTHENTICODE_DIGEST_SIGN(
        //    [In] CERT_CONTEXT* pSigningCert,
        //    [In, Optional] CRYPT_INTEGER_BLOB* pMetadataBlob,
        //    [In] ALG_ID digestAlgId,
        //    [In, MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.U1, SizeParamIndex = 4)] byte[] pbToBeSignedDigest, // byte*
        //    [In] uint cbToBeSignedDigest,
        //    [In, Out] CRYPT_INTEGER_BLOB* pSignedDigest
        //    );

        private readonly delegate* unmanaged[Stdcall]<CERT_CONTEXT*, CRYPT_INTEGER_BLOB*, ALG_ID, byte*, uint, CRYPT_INTEGER_BLOB*, HRESULT> SigningCallback;

        /// <summary>
        /// https://learn.microsoft.com/en-us/windows/win32/seccrypto/pfn-authenticode-digest-sign-withfilehandle
        /// </summary>
        //[UnmanagedFunctionPointer(CallingConvention.Winapi)]
        //internal delegate HRESULT PFN_AUTHENTICODE_DIGEST_SIGN_WITHFILEHANDLE(
        //    [In, Optional] CRYPT_INTEGER_BLOB* pMetadataBlob,
        //    [In] ALG_ID digestAlgId,
        //    [In, MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.U1, SizeParamIndex = 4)] byte[] pbToBeSignedDigest, // byte*
        //    [In] uint cbToBeSignedDigest,
        //    [In] SafeFileHandle hFile,
        //    [Out] CRYPT_INTEGER_BLOB* pSignedDigest
        //    );

        private readonly delegate* unmanaged[Stdcall]<CRYPT_INTEGER_BLOB*, ALG_ID, byte*, uint, SafeFileHandle, CRYPT_INTEGER_BLOB*, HRESULT> SigningCallbackWithFileHandle;

        /// <summary>
        /// https://learn.microsoft.com/en-us/windows/win32/seccrypto/pfn-authenticode-digest-sign-ex
        /// </summary>
        //[UnmanagedFunctionPointer(CallingConvention.Winapi)]
        //internal delegate HRESULT PFN_AUTHENTICODE_DIGEST_SIGN_EX(
        //    [In, Optional] CRYPT_INTEGER_BLOB* pMetadataBlob,
        //    [In] ALG_ID digestAlgId,
        //    [In, MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.U1, SizeParamIndex = 4)] byte[] pbToBeSignedDigest, // byte*
        //    [In] uint cbToBeSignedDigest,
        //    [Out] CRYPT_INTEGER_BLOB* pSignedDigest,
        //    [Out] CERT_CONTEXT** ppSignerCert,
        //    [In, Out] HCERTSTORE hCertChainStore
        //    );

        private readonly delegate* unmanaged[Stdcall]<CRYPT_INTEGER_BLOB*, ALG_ID, byte*, uint, CRYPT_INTEGER_BLOB*, CERT_CONTEXT**, HCERTSTORE, HRESULT> SigningCallbackEx;

        /// <summary>
        /// https://learn.microsoft.com/en-us/windows/win32/seccrypto/pfn-authenticode-digest-sign-ex-withfilehandle
        /// </summary>
        //[UnmanagedFunctionPointer(CallingConvention.Winapi)]
        //internal delegate HRESULT PFN_AUTHENTICODE_DIGEST_SIGN_EX_WITHFILEHANDLE(
        //    [In, Optional] CRYPT_INTEGER_BLOB* pMetadataBlob,
        //    [In] ALG_ID digestAlgId,
        //    [In, MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.U1, SizeParamIndex = 4)] byte[] pbToBeSignedDigest, // byte*
        //    [In] uint cbToBeSignedDigest,
        //    [In] SafeFileHandle hFile,
        //    [Out] CRYPT_INTEGER_BLOB* pSignedDigest,
        //    [Out] CERT_CONTEXT** ppSignerCert,
        //    [In, Out] HCERTSTORE hCertChainStore
        //    );

        /// <summary>
        /// Creates a new instance of <see cref="AuthenticodeKeyVaultSigner" />.
        /// </summary>
        /// <param name="signingAlgorithm">
        /// An instance of an asymmetric algorithm that will be used to sign. It must support signing with
        /// a private key.
        /// </param>
        /// <param name="signingCertificate">The X509 public certificate for the <paramref name="signingAlgorithm"/>.</param>
        /// <param name="fileDigestAlgorithm">The digest algorithm to sign the file.</param>
        /// <param name="timeStampConfiguration">The timestamp configuration for timestamping the file. To omit timestamping,
        /// use <see cref="TimeStampConfiguration.None"/>.</param>
        /// <param name="additionalCertificates">Any additional certificates to assist in building a certificate chain.</param>
        public AuthenticodeKeyVaultSigner(
            AsymmetricAlgorithm signingAlgorithm,
            X509Certificate2 signingCertificate,
            HashAlgorithmName fileDigestAlgorithm,
            TimeStampConfiguration timeStampConfiguration,
            X509Certificate2Collection additionalCertificates = null)
        {
            this.FileDigestAlgorithm = fileDigestAlgorithm;
            this.SigningCertificate = signingCertificate;
            this.TimeStampConfiguration = timeStampConfiguration;
            this.SigningAlgorithm = signingAlgorithm;
            this.CertificateStore = MemoryCertificateStore.Create();
            this.CertificateChain = new X509Chain();

            if (additionalCertificates is not null)
            {
                this.CertificateChain.ChainPolicy.ExtraStore.AddRange(additionalCertificates);
            }

            this.CertificateChain.ChainPolicy.VerificationFlags = X509VerificationFlags.AllFlags;

            if (!this.CertificateChain.Build(signingCertificate))
            {
                throw new InvalidOperationException("Failed to build chain for certificate.");
            }

            for (var i = 0; i < this.CertificateChain.ChainElements.Count; i++)
            {
                this.CertificateStore.Add(this.CertificateChain.ChainElements[i].Certificate);
            }

            //this.SigningCallback = &this.SignDigestCallback;
            this.SigningCallback = (delegate* unmanaged[Stdcall]<CERT_CONTEXT*, CRYPT_INTEGER_BLOB*, ALG_ID, byte*, uint, CRYPT_INTEGER_BLOB*, HRESULT>)Marshal.GetFunctionPointerForDelegate(this.SignDigestCallback);
            this.SigningCallbackEx = null;
            this.SigningCallbackWithFileHandle = null;
        }

        /// <summary>Authenticode signs a file.</summary>
        /// <param name="FileName">The path to the file to signed.</param>
        /// <param name="Description">The description to apply to the signature.</param>
        /// <param name="DescriptionUrl">A URL describing the signature or the signer.</param>
        /// <param name="PageHashing">True if the signing process should try to include page hashing, otherwise false.
        /// Note that page hashing still may be disabled if the Subject Interface Package does not support page hashing.</param>
        /// <param name="IsAppxFile">A URL describing the signature or the signer.</param>
        /// <returns>A HRESULT indicating the result of the signing operation.</returns>
        internal HRESULT SignFile(
            ReadOnlySpan<char> FileName, 
            ReadOnlySpan<char> Description = default,
            ReadOnlySpan<char> DescriptionUrl = default, 
            bool PageHashing = false, 
            bool IsAppxFile = false
            )
        {
            HRESULT result;
            ReadOnlySpan<byte> timestamp = HashAlgorithmToOidAsciiTerminated(HashAlgorithmName.SHA256);
            SIGNER_SIGN_FLAGS flags = (SIGNER_SIGN_FLAGS)SignerSignEx3Flags.SPC_DIGEST_SIGN_FLAG;
            SIGNER_TIMESTAMP_FLAGS timeStampFlags = 0;
            SIGNER_CONTEXT* signerContext = null;
            APPX_SIP_CLIENT_DATA *clientData = null;

            if (PageHashing)
                flags |= (SIGNER_SIGN_FLAGS)SignerSignEx3Flags.SPC_INC_PE_PAGE_HASHES_FLAG;
            else
                flags |= (SIGNER_SIGN_FLAGS)SignerSignEx3Flags.SPC_EXC_PE_PAGE_HASHES_FLAG;

            if (this.TimeStampConfiguration.Type == TimeStampType.Authenticode)
                timeStampFlags = SIGNER_TIMESTAMP_FLAGS.SIGNER_TIMESTAMP_AUTHENTICODE;
            else if (this.TimeStampConfiguration.Type == TimeStampType.RFC3161)
                timeStampFlags = SIGNER_TIMESTAMP_FLAGS.SIGNER_TIMESTAMP_RFC3161;

            fixed (byte* timestampAlg = &timestamp.GetPinnableReference())
            fixed (char* timestampUrl = this.TimeStampConfiguration.Url)
            fixed (char* fileName = FileName) // NullTerminate(FileName)
            fixed (char* description = Description) // NullTerminate(Description)
            fixed (char* descriptionUrl = DescriptionUrl) // NullTerminate(DescriptionUrl)
            {
                var timestampString = new PCSTR(timestampAlg);

                var fileInfo = new SIGNER_FILE_INFO();
                fileInfo.cbSize = (uint)sizeof(SIGNER_FILE_INFO);
                fileInfo.pwszFileName = new PCWSTR(fileName);
  
                var subjectInfo = new SIGNER_SUBJECT_INFO();
                subjectInfo.cbSize = (uint)sizeof(SIGNER_SUBJECT_INFO);
                subjectInfo.pdwIndex = (uint*)NativeMemory.AllocZeroed((nuint)IntPtr.Size);
                subjectInfo.dwSubjectChoice = SIGNER_SUBJECT_CHOICE.SIGNER_SUBJECT_FILE;
                subjectInfo.Anonymous.pSignerFileInfo = &fileInfo;

                var storeInfo = new SIGNER_CERT_STORE_INFO();
                storeInfo.cbSize = (uint)sizeof(SIGNER_CERT_STORE_INFO);
                storeInfo.dwCertPolicy = SIGNER_CERT_POLICY.SIGNER_CERT_POLICY_CHAIN;
                storeInfo.hCertStore = new HCERTSTORE((void*)this.CertificateStore.Handle);
                storeInfo.pSigningCert = (CERT_CONTEXT*)this.SigningCertificate.Handle;

                var signerCert = new SIGNER_CERT();
                signerCert.cbSize = (uint)sizeof(SIGNER_CERT);
                signerCert.dwCertChoice = SIGNER_CERT_CHOICE.SIGNER_CERT_STORE;
                signerCert.Anonymous.pCertStoreInfo = &storeInfo;

                var signatureAuthcode = new SIGNER_ATTR_AUTHCODE();
                signatureAuthcode.cbSize = (uint)sizeof(SIGNER_ATTR_AUTHCODE);
                signatureAuthcode.pwszName = description;
                signatureAuthcode.pwszInfo = descriptionUrl;

                var signatureInfo = new SIGNER_SIGNATURE_INFO();
                signatureInfo.cbSize = (uint)sizeof(SIGNER_SIGNATURE_INFO);
                signatureInfo.dwAttrChoice = SIGNER_SIGNATURE_ATTRIBUTE_CHOICE.SIGNER_AUTHCODE_ATTR;
                signatureInfo.algidHash = HashAlgorithmToAlgId(this.FileDigestAlgorithm);
                signatureInfo.Anonymous.pAttrAuthcode = &signatureAuthcode;

                var signCallbackInfo = new SIGNER_DIGEST_SIGN_INFO();
                signCallbackInfo.cbSize = (uint)sizeof(SIGNER_DIGEST_SIGN_INFO);
                signCallbackInfo.dwDigestSignChoice = (uint)SIGNER_DIGEST_CHOICE.DIGEST_SIGN;
                signCallbackInfo.Anonymous.pfnAuthenticodeDigestSign = this.SigningCallback;
                //signCallbackInfo.Anonymous.pfnAuthenticodeDigestSignEx = this.SigningCallbackEx;
                //signCallbackInfo.Anonymous.pfnAuthenticodeDigestSign = (delegate* unmanaged[Stdcall]<CERT_CONTEXT*, CRYPT_INTEGER_BLOB*, ALG_ID, byte*, uint, CRYPT_INTEGER_BLOB*, HRESULT>)Marshal.GetFunctionPointerForDelegate(this.SigningCallback);

                //if (this.SigningCallbaackEx != null)
                //{
                //    signCallbackInfo.Anonymous.pfnAuthenticodeDigestSignEx = (delegate* unmanaged[Stdcall]<CRYPT_INTEGER_BLOB*, ALG_ID, byte*, uint, CRYPT_INTEGER_BLOB*, CERT_CONTEXT**, HCERTSTORE, HRESULT>)
                //        Marshal.GetFunctionPointerForDelegate(this.SigningCallbaackEx);
                //}

                if (IsAppxFile)
                {
                    APPX_SIP_CLIENT_DATA sipclientdata;
                    SIGNER_SIGN_EX_PARAMS parameters;
                    SIGNER_DIGEST_SIGN_INFO_UNION digestCallback;

                    //digestCallback.V1.cbSize = (uint)sizeof(SIGNER_DIGEST_SIGN_INFO_V1);
                    //digestCallback.V1.pfnAuthenticodeDigestSign = signCallbackInfo.Anonymous.pfnAuthenticodeDigestSign;

                    digestCallback.V2.cbSize = (uint)sizeof(SIGNER_DIGEST_SIGN_INFO_V2);
                    digestCallback.V2.pfnAuthenticodeDigestSign = signCallbackInfo.Anonymous.pfnAuthenticodeDigestSign;
                    digestCallback.V2.pfnAuthenticodeDigestSignEx = signCallbackInfo.Anonymous.pfnAuthenticodeDigestSignEx;

                    clientData = &sipclientdata;
                    clientData->pSignerParams = &parameters;
                    clientData->pSignerParams->Ex3.dwFlags = (SIGNER_SIGN_FLAGS)(SignerSignEx3Flags.SPC_DIGEST_SIGN_FLAG | SignerSignEx3Flags.SPC_EXC_PE_PAGE_HASHES_FLAG);
                    clientData->pSignerParams->Ex3.dwTimestampFlags = timeStampFlags;
                    clientData->pSignerParams->Ex3.pSubjectInfo = &subjectInfo;
                    clientData->pSignerParams->Ex3.pSignerCert = &signerCert;
                    clientData->pSignerParams->Ex3.pSignatureInfo = &signatureInfo;
                    clientData->pSignerParams->Ex3.ppSignerContext = &signerContext;
                    clientData->pSignerParams->Ex3.pwszTimestampURL = timestampUrl;
                    clientData->pSignerParams->Ex3.pszTimestampAlgorithmOid = timestampAlg;
                    clientData->pSignerParams->Ex3.pSignCallBack = &digestCallback;
                }

                result = PInvoke.SignerSignEx3(
                    flags,
                    &subjectInfo,
                    &signerCert,
                    &signatureInfo,
                    null,
                    timeStampFlags,
                    timestampString,
                    timestampUrl,
                    null,
                    clientData,
                    &signerContext,
                    null,
                    signCallbackInfo,
                    null
                    );

                if (result == HRESULT.S_OK)
                {
                    if (signerContext != null)
                    {
                        PInvoke.SignerFreeSignerContext(signerContext);
                    }

                    if (clientData != null && clientData->pAppxSipState != IntPtr.Zero)
                    {
                        Marshal.Release(clientData->pAppxSipState);
                    }
                }

                NativeMemory.Free(subjectInfo.pdwIndex);

                return result;
            }
        }

        /// <summary>
        /// Frees all resources used by the <see cref="AuthenticodeKeyVaultSigner" />.
        /// </summary>
        public void Dispose()
        {
            if (this.CertificateChain != null)
            {
                this.CertificateChain.Dispose();
                this.CertificateChain = null;
            }

            GC.SuppressFinalize(this);
        }

        private HRESULT SignDigestCallback(
            CERT_CONTEXT* pSigningCert,
            CRYPT_INTEGER_BLOB* pMetadataBlob,
            ALG_ID digestAlgId,
            byte* pbToBeSignedDigest, // [In, MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.U1, SizeParamIndex = 4)] byte[] 
            uint cbToBeSignedDigest,
            CRYPT_INTEGER_BLOB* SignedDigest
            )
        {
            byte[] digest;

            //X509Certificate2 cert = new X509Certificate2((IntPtr)pSigningCert);
            //ECDsa dsa = ECDsaCertificateExtensions.GetECDsaPublicKey(cert);
            //ReadOnlySpan<byte> buffer = MemoryMarshal.CreateReadOnlySpan(ref *pbToBeSignedDigest, (int)cbToBeSignedDigest);
            //byte[] buffer = new byte[cbToBeSignedDigest];
            //fixed (void* ptr = &buffer[0])
            //    Unsafe.CopyBlock(ptr, pbToBeSignedDigest, cbToBeSignedDigest);

            ReadOnlySpan<byte> buffer = new ReadOnlySpan<byte>(pbToBeSignedDigest, (int)cbToBeSignedDigest);

            switch (this.SigningAlgorithm)
            {
                case RSA rsa:
                    digest = rsa.SignHash(buffer, this.FileDigestAlgorithm, RSASignaturePadding.Pkcs1);
                    break;
                case ECDsa ecdsa:
                    digest = ecdsa.SignHash(buffer);
                    break;
                default:
                    return HRESULT.E_INVALIDARG;
            }

            {
                SignedDigest->pbData = (byte*)NativeMemory.AllocZeroed((nuint)digest.Length);
                SignedDigest->cbData = (uint)digest.Length;

                fixed (void* memory = &digest[0])
                {
                    Unsafe.CopyBlock(SignedDigest->pbData, memory, (uint)digest.Length);
                }
            }

            return HRESULT.S_OK;
        }

        internal static ALG_ID HashAlgorithmToAlgId(HashAlgorithmName hashAlgorithmName)
        {
            return hashAlgorithmName.Name switch
            {
                nameof(HashAlgorithmName.SHA1) => ALG_ID.CALG_SHA1,
                nameof(HashAlgorithmName.SHA256) => ALG_ID.CALG_SHA_256,
                nameof(HashAlgorithmName.SHA384) => ALG_ID.CALG_SHA_384,
                nameof(HashAlgorithmName.SHA512) => ALG_ID.CALG_SHA_512,
                _ => throw new NotSupportedException("The algorithm specified is not supported."),
            };
        }

        internal static ReadOnlySpan<byte> HashAlgorithmToOidAsciiTerminated(HashAlgorithmName hashAlgorithmName)
        {
            return hashAlgorithmName.Name switch
            {
                nameof(HashAlgorithmName.SHA1) => "1.3.14.3.2.26\0"u8,
                nameof(HashAlgorithmName.SHA256) => "2.16.840.1.101.3.4.2.1\0"u8,
                nameof(HashAlgorithmName.SHA384) => "2.16.840.1.101.3.4.2.2\0"u8,
                nameof(HashAlgorithmName.SHA512) => "2.16.840.1.101.3.4.2.3\0"u8,
                _ => throw new NotSupportedException("The algorithm specified is not supported."),
            };
        }

        internal static string HashAlgorithmToOid(HashAlgorithmName hashAlgorithmName)
        {
            return hashAlgorithmName.Name switch
            {
                nameof(HashAlgorithmName.SHA1) => "1.3.14.3.2.26",
                nameof(HashAlgorithmName.SHA256) => "2.16.840.1.101.3.4.2.1",
                nameof(HashAlgorithmName.SHA384) => "2.16.840.1.101.3.4.2.2",
                nameof(HashAlgorithmName.SHA512) => "2.16.840.1.101.3.4.2.3",
                _ => throw new NotSupportedException("The algorithm specified is not supported."),
            };
        }

        private static char[] NullTerminate(ReadOnlySpan<char> str)
        {
            char[] result = new char[str.Length + 1];
            str.CopyTo(result);
            result[result.Length - 1] = '\0';
            return result;
        }
    }
}
