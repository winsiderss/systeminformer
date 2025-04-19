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
        private readonly KeyVaultContext context;
        private RSA publicKey;
        private bool disposed;

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
            LegalKeySizesValue = [new KeySizes(publicKey.KeySize, publicKey.KeySize, 0)];
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
                return context.SignDigest(hash, hashAlgorithm, KeyVaultSignatureAlgorithm.RSAPkcs15);
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
                throw new InvalidOperationException("Error calling Key Vault", e);
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
                throw new NotSupportedException("Private keys cannot be exported by this provider");

            return publicKey.ExportParameters(false);
        }

        /// <inheritdoc/>
        public override void ImportParameters(RSAParameters parameters)
        {
            throw new NotSupportedException();
        }

        private void CheckDisposed()
        {
            ObjectDisposedException.ThrowIf(disposed, this);
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
            if (disposed)
                return;

            if (disposing)
            {
                publicKey?.Dispose();
                publicKey = null;
            }

            disposed = true;
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
