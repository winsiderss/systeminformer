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
        /// <param name="Context">Context with parameters</param>
        public RSAKeyVault(KeyVaultContext Context)
        {
            if (!Context.IsValid)
                throw new ArgumentException("Must not be the default", nameof(Context));

            this.VaultContext = Context;
            // Build public key from certificate if available
            if (Context.Certificate != null)
            {
                this.PublicKey = Context.Certificate.GetRSAPublicKey();
            }
            else
            {
                throw new ArgumentException("Public key must be provided in KeyVaultContext", nameof(Context));
            }

            this.KeySizeValue = this.PublicKey.KeySize;
            this.LegalKeySizesValue = [new KeySizes(this.PublicKey.KeySize, this.PublicKey.KeySize, 0)];
        }

        /// <inheritdoc/>
        public override byte[] SignHash(byte[] Hash, HashAlgorithmName HashAlgorithm, RSASignaturePadding Padding)
        {
            CheckDisposed();

            // Key Vault only supports PKCSv1 padding
            if (Padding.Mode != RSASignaturePaddingMode.Pkcs1)
            {
                throw new NotSupportedException("Unsupported padding mode");
            }

            return this.VaultContext.SignDigest(Hash, HashAlgorithm, KeyVaultSignatureAlgorithm.RSAPkcs15);
        }

        /// <inheritdoc/>
        public override bool VerifyHash(byte[] Hash, byte[] Signature, HashAlgorithmName HashAlgorithm, RSASignaturePadding Padding)
        {
            CheckDisposed();
            return this.PublicKey.VerifyHash(Hash, Signature, HashAlgorithm, Padding);
        }

        /// <inheritdoc/>
        protected override byte[] HashData(byte[] Data, int Offset, int Count, HashAlgorithmName HashAlgorithm)
        {
            CheckDisposed();

            using (var digestAlgorithm = Create(HashAlgorithm))
            {
                return digestAlgorithm.ComputeHash(Data, Offset, Count);
            }
        }

        /// <inheritdoc/>
        protected override byte[] HashData(Stream Data, HashAlgorithmName HashAlgorithm)
        {
            CheckDisposed();

            using (var digestAlgorithm = Create(HashAlgorithm))
            {
                return digestAlgorithm.ComputeHash(Data);
            }
        }

        /// <inheritdoc/>
        public override byte[] Decrypt(byte[] Data, RSAEncryptionPadding Padding)
        {
            CheckDisposed();

            return this.VaultContext.DecryptData(Data, Padding);
        }

        /// <inheritdoc/>
        public override byte[] Encrypt(byte[] Data, RSAEncryptionPadding Padding)
        {
            CheckDisposed();

            return this.PublicKey.Encrypt(Data, Padding);
        }

        /// <inheritdoc/>
        public override RSAParameters ExportParameters(bool IncludePrivateParameters)
        {
            CheckDisposed();

            if (IncludePrivateParameters)
            {
                throw new NotSupportedException("Private keys cannot be exported by this provider");
            }

            return this.PublicKey.ExportParameters(false);
        }

        /// <inheritdoc/>
        public override void ImportParameters(RSAParameters Parameters)
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
        protected override void Dispose(bool Disposing)
        {
            if (this.Disposed)
                return;

            if (Disposing)
            {
                this.PublicKey?.Dispose();
                this.PublicKey = null;
            }

            this.Disposed = true;
            base.Dispose(Disposing);
        }

        private static HashAlgorithm Create(HashAlgorithmName Algorithm)
        {
            return Algorithm.Name switch
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