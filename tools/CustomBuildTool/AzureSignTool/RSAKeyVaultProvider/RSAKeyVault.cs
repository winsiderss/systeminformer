namespace CustomBuildTool
{
    /// <summary>
    /// RSA implementation that uses Azure Key Vault
    /// </summary>
    public sealed class RSAKeyVault : RSA
    {
        readonly KeyVaultContext context;
        RSA publicKey;

        /// <summary>
        /// Creates a new RSAKeyVault instance
        /// </summary>
        /// <param name="context">Context with parameters</param>
        public RSAKeyVault(KeyVaultContext context)
        {
            if (!context.IsValid)
                throw new ArgumentException("Must not be the default", nameof(context));

            this.context = context;
            publicKey = context.Key.ToRSA();
            KeySizeValue = publicKey.KeySize;
            LegalKeySizesValue = new[] { new KeySizes(publicKey.KeySize, publicKey.KeySize, 0) };
        }

        /// <inheritdoc/>
        public override byte[] SignHash(byte[] hash, HashAlgorithmName hashAlgorithm, RSASignaturePadding padding)
        {
            CheckDisposed();

            // Key Vault only supports PKCSv1 padding
            if (padding.Mode != RSASignaturePaddingMode.Pkcs1)
                throw new Exception("Unsupported padding mode");

            try
            {
                return context.SignDigest(hash, hashAlgorithm, KeyVaultSignatureAlgorithm.RSAPkcs15);
            }
            catch (Exception e)
            {
                throw new Exception("Error calling Key Vault", e);
            }
        }

        /// <inheritdoc/>
        public override bool VerifyHash(byte[] hash, byte[] signature, HashAlgorithmName hashAlgorithm, RSASignaturePadding padding)
        {
            CheckDisposed();

            // Verify can be done locally using the public key
            return publicKey.VerifyHash(hash, signature, hashAlgorithm, padding);
        }

        /// <inheritdoc/>
        protected override byte[] HashData(byte[] data, int offset, int count, HashAlgorithmName hashAlgorithm)
        {
            CheckDisposed();

            using (var digestAlgorithm = Create(hashAlgorithm))
            {
                return digestAlgorithm.ComputeHash(data, offset, count);
            }
        }

        /// <inheritdoc/>
        protected override byte[] HashData(Stream data, HashAlgorithmName hashAlgorithm)
        {
            CheckDisposed();

            using (var digestAlgorithm = Create(hashAlgorithm))
            {
                return digestAlgorithm.ComputeHash(data);
            }
        }

        /// <inheritdoc/>
        public override byte[] Decrypt(byte[] data, RSAEncryptionPadding padding)
        {
            CheckDisposed();

            try
            {
                return context.DecryptData(data, padding);
            }
            catch (Exception e)
            {
                throw new Exception("Error calling Key Vault", e);
            }
        }      

        /// <inheritdoc/>
        public override byte[] Encrypt(byte[] data, RSAEncryptionPadding padding)
        {
            CheckDisposed();

            return publicKey.Encrypt(data, padding);
        }

        /// <inheritdoc/>
        public override RSAParameters ExportParameters(bool includePrivateParameters)
        {
            CheckDisposed();

            if (includePrivateParameters)
                throw new Exception("Private keys cannot be exported by this provider");

            return publicKey.ExportParameters(includePrivateParameters);
        }

        /// <inheritdoc/>
        public override void ImportParameters(RSAParameters parameters)
        {
            throw new NotSupportedException();
        }

        void CheckDisposed()
        {
            if (publicKey == null)
                throw new ObjectDisposedException($"{nameof(RSAKeyVault)} is disposed");
        }

        /// <inheritdoc/>
        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                publicKey?.Dispose();
                publicKey = null;
            }

            base.Dispose(disposing);
        }

        private static HashAlgorithm Create(HashAlgorithmName algorithm)
        {
            if (algorithm == HashAlgorithmName.SHA1)
                return SHA1.Create();

            if (algorithm == HashAlgorithmName.SHA256)
                return SHA256.Create();

            if (algorithm == HashAlgorithmName.SHA384)
                return SHA384.Create();

            if (algorithm == HashAlgorithmName.SHA512)
                return SHA512.Create();

            throw new NotSupportedException("The specified algorithm is not supported.");
        }
    }
}
