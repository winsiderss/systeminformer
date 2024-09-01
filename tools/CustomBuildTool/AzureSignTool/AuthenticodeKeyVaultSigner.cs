using AzureSign.Core.Interop;
using Windows.Win32.Security.Cryptography;

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
        private readonly X509Chain CertificateChain;
        private readonly PFN_AUTHENTICODE_DIGEST_SIGN SigningCallbaack;

        /// <summary>
        /// The PFN_AUTHENTICODE_DIGEST_SIGN user supplied callback function implements digest signing.
        /// This function is currently called by SignerSignEx3 for digest signing.
        /// </summary>
        /// <param name="pSigningCert">A pointer to a CERT_CONTEXT structure that specifies the certificate used to create the digital signature.</param>
        /// <param name="pMetadataBlob">Pointer to a CRYPT_DATA_BLOB structure that contains metadata for digest signing.</param>
        /// <param name="digestAlgId">Specifies the digest algorithm to be used for digest signing.</param>
        /// <param name="pbToBeSignedDigest">Pointer to a buffer which contains the digest to be signed.</param>
        /// <param name="cbToBeSignedDigest">The size, in bytes, of the pbToBeSignedDigest buffer.</param>
        /// <param name="pSignedDigest">Pointer to CRYPT_DATA_BLOB which receives the signed digest.</param>
        /// <returns>If the function succeeds, the function returns S_OK. If the function fails, it returns an HRESULT value that indicates the error.</returns>
        [UnmanagedFunctionPointer(CallingConvention.Winapi)]
        internal unsafe delegate HRESULT PFN_AUTHENTICODE_DIGEST_SIGN(
            [In] CERT_CONTEXT* pSigningCert,
            [In, Optional] CRYPT_DATA_BLOB* pMetadataBlob,
            [In] ALG_ID digestAlgId,
            [In, MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.U1, SizeParamIndex = 4)] byte[] pbToBeSignedDigest, // byte*
            [In] uint cbToBeSignedDigest,
            [In, Out] CRYPT_INTEGER_BLOB* pSignedDigest
            );

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
        public unsafe AuthenticodeKeyVaultSigner(
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

            this.SigningCallbaack = this.SignDigestCallback;
        }

        /// <summary>Authenticode signs a file.</summary>
        /// <param name="pageHashing">True if the signing process should try to include page hashing, otherwise false.
        /// Use <c>null</c> to use the operating system default. Note that page hashing still may be disabled if the
        /// Subject Interface Package does not support page hashing.</param>
        /// <param name="descriptionUrl">A URL describing the signature or the signer.</param>
        /// <param name="description">The description to apply to the signature.</param>
        /// <param name="path">The path to the file to signed.</param>
        /// <param name="logger">An optional logger to capture signing operations.</param>
        /// <returns>A HRESULT indicating the result of the signing operation. S_OK, or zero, is returned if the signing
        /// operation completed successfully.</returns>
        internal unsafe HRESULT SignFile(ReadOnlySpan<char> path, ReadOnlySpan<char> description, ReadOnlySpan<char> descriptionUrl, bool pageHashing = false)
        {
            HRESULT result;
            SIGNER_CONTEXT* context = null;
            SIGNER_TIMESTAMP_FLAGS timeStampFlags = 0;
            ReadOnlySpan<byte> timestampAlgorithmOid = null;
            APPX_SIP_CLIENT_DATA *clientData = null;
            string timestampString = null;

            var flags = SIGNER_SIGN_FLAGS.SPC_DIGEST_SIGN_FLAG;

            if (pageHashing)
                flags |= SIGNER_SIGN_FLAGS.SPC_INC_PE_PAGE_HASHES_FLAG;
            else
                flags |= SIGNER_SIGN_FLAGS.SPC_EXC_PE_PAGE_HASHES_FLAG;

            if (this.TimeStampConfiguration.Type == TimeStampType.Authenticode)
            {
                timeStampFlags = SIGNER_TIMESTAMP_FLAGS.SIGNER_TIMESTAMP_AUTHENTICODE;
                timestampString = this.TimeStampConfiguration.Url;
            }
            else if (this.TimeStampConfiguration.Type == TimeStampType.RFC3161)
            {
                timeStampFlags = SIGNER_TIMESTAMP_FLAGS.SIGNER_TIMESTAMP_RFC3161;
                timestampAlgorithmOid = AlgorithmTranslator.HashAlgorithmToOidAsciiTerminated(this.TimeStampConfiguration.DigestAlgorithm);
                timestampString = this.TimeStampConfiguration.Url;
            }          

            fixed (byte* pTimestampAlg = &timestampAlgorithmOid.GetPinnableReference())
            fixed (char* timestampUrl = timestampString)
            fixed (char* pPath = NullTerminate(path))
            fixed (char* pDescription = NullTerminate(description))
            fixed (char* pDescriptionUrl = NullTerminate(descriptionUrl))
            {
                var timestampAlg = new PCSTR(pTimestampAlg);

                var fileInfo = new SIGNER_FILE_INFO();
                fileInfo.cbSize = (uint)sizeof(SIGNER_FILE_INFO);
                fileInfo.pwszFileName = new PCWSTR(pPath);
  
                var subjectInfo = new SIGNER_SUBJECT_INFO();
                subjectInfo.cbSize = (uint)sizeof(SIGNER_SUBJECT_INFO);
                subjectInfo.pdwIndex = (uint*)NativeMemory.AllocZeroed((nuint)sizeof(IntPtr));
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
                signatureAuthcode.pwszName = pDescription;
                signatureAuthcode.pwszInfo = pDescriptionUrl;

                var signatureInfo = new SIGNER_SIGNATURE_INFO();
                signatureInfo.cbSize = (uint)sizeof(SIGNER_SIGNATURE_INFO);
                signatureInfo.dwAttrChoice = SIGNER_SIGNATURE_ATTRIBUTE_CHOICE.SIGNER_AUTHCODE_ATTR;
                signatureInfo.algidHash = AlgorithmTranslator.HashAlgorithmToAlgId(this.FileDigestAlgorithm);
                signatureInfo.Anonymous.pAttrAuthcode = &signatureAuthcode;

                var signCallbackInfo = new SIGNER_DIGEST_SIGN_INFO();
                signCallbackInfo.cbSize = (uint)Marshal.SizeOf<SIGNER_DIGEST_SIGN_INFO>();
                signCallbackInfo.dwDigestSignChoice = 1; // DIGEST_SIGN
                signCallbackInfo.Anonymous.pfnAuthenticodeDigestSign = (delegate* unmanaged[Stdcall]<CERT_CONTEXT*, CRYPT_INTEGER_BLOB*, ALG_ID, byte*, uint, CRYPT_INTEGER_BLOB*, HRESULT>)Marshal.GetFunctionPointerForDelegate(this.SigningCallbaack);

                if (SipExtensionFactory.GetSipKind(path) == SipKind.Appx)
                {
                    APPX_SIP_CLIENT_DATA sipclientdata;
                    SIGNER_SIGN_EX3_PARAMS parameters;
                    SIGNER_DIGEST_SIGN_INFO_V1 digestCallback;

                    digestCallback.Size = (uint)sizeof(SIGNER_DIGEST_SIGN_INFO_V1);
                    digestCallback.AuthenticodeDigestSign = Marshal.GetFunctionPointerForDelegate(this.SigningCallbaack);

                    clientData = &sipclientdata;
                    clientData->pSignerParams = &parameters;
                    clientData->pSignerParams->dwFlags = SIGNER_SIGN_FLAGS.SPC_DIGEST_SIGN_FLAG | SIGNER_SIGN_FLAGS.SPC_EXC_PE_PAGE_HASHES_FLAG;
                    clientData->pSignerParams->dwTimestampFlags = timeStampFlags;
                    clientData->pSignerParams->pSubjectInfo = &subjectInfo;
                    clientData->pSignerParams->pSignerCert = &signerCert;
                    clientData->pSignerParams->pSignatureInfo = &signatureInfo;
                    clientData->pSignerParams->ppSignerContext = &context;
                    clientData->pSignerParams->pwszHttpTimeStamp = timestampUrl;
                    clientData->pSignerParams->pszTimestampAlgorithmOid = timestampAlg;
                    clientData->pSignerParams->pSignCallBack = &digestCallback;
                }

                result = PInvoke.SignerSignEx3(
                    flags,
                    &subjectInfo,
                    &signerCert,
                    &signatureInfo,
                    null,
                    timeStampFlags,
                    timestampAlg,
                    timestampUrl,
                    null,
                    clientData,
                    &context,
                    null,
                    signCallbackInfo,
                    null
                    );

                if (result == HRESULT.S_OK)
                {
                    if (context != null)
                    {
                        PInvoke.SignerFreeSignerContext(context);
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
            this.CertificateChain.Dispose();
            this.CertificateStore.Close();
            GC.SuppressFinalize(this);
        }

        private unsafe HRESULT SignDigestCallback(
            [In] CERT_CONTEXT* pSigningCert,
            [In, Optional] CRYPT_DATA_BLOB* pMetadataBlob,
            [In] ALG_ID digestAlgId,
            [In, MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.U1, SizeParamIndex = 4)] byte[] pbToBeSignedDigest, // byte*
            [In] uint cbToBeSignedDigest,
            [In, Out] CRYPT_INTEGER_BLOB* SignedDigest
            )
        {
            byte[] digest;
            //X509Certificate2 cert = new X509Certificate2((IntPtr)pSigningCert);
            //ECDsa dsa = ECDsaCertificateExtensions.GetECDsaPublicKey(cert);
            //
            //{
            //    byte[] buffer = new byte[cbToBeSignedDigest];
            //
            //    fixed (void* ptr = &buffer[0])
            //    {
            //        Unsafe.CopyBlock(ptr, pbToBeSignedDigest, (uint)cbToBeSignedDigest);
            //    }
            //}

            {
                switch (this.SigningAlgorithm)
                {
                    case RSA rsa:
                        digest = rsa.SignHash(pbToBeSignedDigest, this.FileDigestAlgorithm, RSASignaturePadding.Pkcs1);
                        break;
                    case ECDsa ecdsa:
                        digest = ecdsa.SignHash(pbToBeSignedDigest);
                        break;
                    default:
                        return HRESULT.E_INVALIDARG;
                }
            }

            {
                SignedDigest->pbData = (byte*)NativeMemory.Alloc((nuint)digest.Length);
                SignedDigest->cbData = (uint)digest.Length;

                fixed (void* ptr = &digest[0])
                {
                    Unsafe.CopyBlock(SignedDigest->pbData, ptr, (uint)digest.Length);
                }
            }

            return HRESULT.S_OK;
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
