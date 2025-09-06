/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex
 *
 */

namespace CustomBuildTool
{
    /// <summary>
    /// RSA implementation that uses Azure Key Vault
    /// </summary>
    public sealed class RSAKeyVault : RSA, IDisposable
    {
        private readonly KeyVaultContext VaultContext;
        private RSA PublicKey;
        private bool Disposed;

        /// <summary>
        /// Creates a new RSAKeyVault instance
        /// </summary>
        /// <param name="context">Context with parameters</param>
        public RSAKeyVault(KeyVaultContext context)
        {
            if (!context.IsValid)
                throw new ArgumentException("Must not be the default", nameof(context));

            this.VaultContext = context;
            this.PublicKey = context.Key.ToRSA();
            this.KeySizeValue = this.PublicKey.KeySize;
            this.LegalKeySizesValue = [new KeySizes(this.PublicKey.KeySize, this.PublicKey.KeySize, 0)];
        }

        /// <inheritdoc/>
        public override byte[] SignHash(byte[] hash, HashAlgorithmName hashAlgorithm, RSASignaturePadding padding)
        {
            CheckDisposed();

            // Key Vault only supports PKCSv1 padding
            if (padding.Mode != RSASignaturePaddingMode.Pkcs1)
                throw new NotSupportedException("Unsupported padding mode");

            try
            {
                return this.VaultContext.SignDigest(hash, hashAlgorithm, KeyVaultSignatureAlgorithm.RSAPkcs15);
            }
            catch (Exception e)
            {
                throw new InvalidOperationException("Error calling Key Vault", e);
            }
        }

        /// <inheritdoc/>
        public override bool VerifyHash(byte[] hash, byte[] signature, HashAlgorithmName hashAlgorithm, RSASignaturePadding padding)
        {
            CheckDisposed();
            return this.PublicKey.VerifyHash(hash, signature, hashAlgorithm, padding);
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
                return this.VaultContext.DecryptData(data, padding);
            }
            catch (Exception e)
            {
                throw new InvalidOperationException("Error calling Key Vault", e);
            }
        }

        /// <inheritdoc/>
        public override byte[] Encrypt(byte[] data, RSAEncryptionPadding padding)
        {
            CheckDisposed();

            return this.PublicKey.Encrypt(data, padding);
        }

        /// <inheritdoc/>
        public override RSAParameters ExportParameters(bool includePrivateParameters)
        {
            CheckDisposed();

            if (includePrivateParameters)
                throw new NotSupportedException("Private keys cannot be exported by this provider");

            return this.PublicKey.ExportParameters(false);
        }

        /// <inheritdoc/>
        public override void ImportParameters(RSAParameters parameters)
        {
            throw new NotSupportedException();
        }

        private void CheckDisposed()
        {
            ObjectDisposedException.ThrowIf(this.Disposed, this);
        }

        ~RSAKeyVault()
        {
            Dispose(false);
        }

        public new void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <inheritdoc/>
        protected override void Dispose(bool disposing)
        {
            if (this.Disposed)
                return;

            if (disposing)
            {
                this.PublicKey?.Dispose();
                this.PublicKey = null;
            }

            this.Disposed = true;
            base.Dispose(disposing);
        }

        private static HashAlgorithm Create(HashAlgorithmName algorithm)
        {
            return algorithm.Name switch
            {
                nameof(HashAlgorithmName.SHA1) => SHA1.Create(),
                nameof(HashAlgorithmName.SHA256) => SHA256.Create(),
                nameof(HashAlgorithmName.SHA384) => SHA384.Create(),
                nameof(HashAlgorithmName.SHA512) => SHA512.Create(),
                _ => throw new NotSupportedException("The specified algorithm is not supported.")
            };
        }
    }
}
