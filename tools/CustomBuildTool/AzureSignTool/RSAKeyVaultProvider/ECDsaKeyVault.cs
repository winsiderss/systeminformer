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
    /// ECDsa implementation that uses Azure Key Vault
    /// </summary>
    public sealed class ECDsaKeyVault : ECDsa
    {
        private readonly KeyVaultContext VaultContext;
        private ECDsa PublicKey;
        private bool Disposed;

        /// <summary>
        /// Creates a new ECDsaKeyVault instance
        /// </summary>
        /// <param name="context">Context with parameters</param>
        public ECDsaKeyVault(KeyVaultContext context)
        {
            if (!context.IsValid)
                throw new ArgumentException("Must not be the default", nameof(context));

            this.VaultContext = context;
            this.PublicKey = context.Key.ToECDsa();
            this.KeySizeValue = this.PublicKey.KeySize;
            this.LegalKeySizesValue = [new KeySizes(this.PublicKey.KeySize, this.PublicKey.KeySize, 0)];
        }

        public override byte[] SignHash(byte[] hash)
        {
            CheckDisposed();
            ValidateKeyDigestCombination(KeySize, hash.Length);

            if (KeySize == 256)
                return this.VaultContext.SignDigest(hash, HashAlgorithmName.SHA256, KeyVaultSignatureAlgorithm.ECDsa);
            if (KeySize == 384)
                return this.VaultContext.SignDigest(hash, HashAlgorithmName.SHA384, KeyVaultSignatureAlgorithm.ECDsa);
            if (KeySize == 521)
                return this.VaultContext.SignDigest(hash, HashAlgorithmName.SHA512, KeyVaultSignatureAlgorithm.ECDsa);
            throw new ArgumentException("Digest length is not valid for the key size.", nameof(hash));
        }

        protected override byte[] HashData(byte[] data, int offset, int count, HashAlgorithmName hashAlgorithm)
        {
            ValidateKeyDigestCombination(KeySize, hashAlgorithm);

            return HashDataIncremental(data, offset, count, hashAlgorithm);
        }

        private static byte[] HashDataIncremental(byte[] data, int offset, int count, HashAlgorithmName hashAlgorithm)
        {
            using (IncrementalHash hash = IncrementalHash.CreateHash(hashAlgorithm))
            {
                hash.AppendData(data, offset, count);
                return hash.GetHashAndReset();
            }
        }

        private static byte[] HashDataStatic(byte[] data, int offset, int count, HashAlgorithmName hashAlgorithm)
        {
            // Use static method for hashing to avoid allocation of IncrementalHash
            using var sha256 = IncrementalHash.CreateHash(hashAlgorithm);
            sha256.AppendData(data, offset, count);
            return sha256.GetHashAndReset();
        }

        private static byte[] HashDataFixed(byte[] data, int offset, int count, HashAlgorithmName hashAlgorithm)
        {
            // Use static method for hashing to avoid allocation of IncrementalHash
            if (hashAlgorithm == HashAlgorithmName.SHA256)
            {
                using var sha256 = System.Security.Cryptography.SHA256.Create();
                return sha256.ComputeHash(data, offset, count);
            }
            if (hashAlgorithm == HashAlgorithmName.SHA384)
            {
                using var sha384 = System.Security.Cryptography.SHA384.Create();
                return sha384.ComputeHash(data, offset, count);
            }
            if (hashAlgorithm == HashAlgorithmName.SHA512)
            {
                using var sha512 = System.Security.Cryptography.SHA512.Create();
                return sha512.ComputeHash(data, offset, count);
            }
            throw new NotSupportedException("The specified algorithm is not supported.");
        }

        /// <inheritdoc/>
        public override bool VerifyHash(byte[] hash, byte[] signature)
        {
            CheckDisposed();
            ValidateKeyDigestCombination(KeySize, hash.Length);

            // Avoid null check every call
            var key = this.PublicKey;
            ObjectDisposedException.ThrowIf(key is null, this);

            return key.VerifyHash(hash, signature);
        }

        ///<inheritdoc/>
        public override ECParameters ExportParameters(bool includePrivateParameters)
        {
            if (includePrivateParameters)
            {
                throw new NotSupportedException("Private keys cannot be exported by this provider");
            }

            return this.PublicKey.ExportParameters(false);
        }

        ///<inheritdoc/>
        public override ECParameters ExportExplicitParameters(bool includePrivateParameters)
        {
            if (includePrivateParameters)
            {
                throw new NotSupportedException("Private keys cannot be exported by this provider");
            }

            return this.PublicKey.ExportExplicitParameters(false);
        }

        /// <summary>
        /// Importing parameters is not supported.
        /// </summary>
        public override void ImportParameters(ECParameters parameters) =>
            throw new NotSupportedException();

        /// <summary>
        /// Key generation is not supported.
        /// </summary>
        public override void GenerateKey(ECCurve curve) => throw new NotSupportedException();

        /// <inheritdoc/>
        public override string ToXmlString(bool includePrivateParameters)
        {
            if (includePrivateParameters)
            {
                throw new NotSupportedException("Private keys cannot be exported by this provider");
            }

            return this.PublicKey.ToXmlString(false);
        }

        /// <summary>
        /// Importing parameters from XML is not supported.
        public override void FromXmlString(string xmlString) => throw new NotSupportedException();

        private static void ValidateKeyDigestCombination(int keySizeBits, int digestSizeBytes)
        {
            if ((keySizeBits == 256 && digestSizeBytes == 32) ||
                (keySizeBits == 384 && digestSizeBytes == 48) ||
                (keySizeBits == 521 && digestSizeBytes == 64))
            {
                return;
            }

            throw new NotSupportedException($"The key size '{keySizeBits}' is not valid for digest of size '{digestSizeBytes}' bytes.");
        }

        private static void ValidateKeyDigestCombination(int keySizeBits, HashAlgorithmName hashAlgorithmName)
        {
            switch (keySizeBits)
            {
            case 256 when hashAlgorithmName == HashAlgorithmName.SHA256:
            case 384 when hashAlgorithmName == HashAlgorithmName.SHA384:
            case 521 when hashAlgorithmName == HashAlgorithmName.SHA512:
                return;
            }
            throw new NotSupportedException($"The key size '{keySizeBits}' is not valid for digest algorithm '{hashAlgorithmName}'.");
        }

        void CheckDisposed()
        {
            ObjectDisposedException.ThrowIf(this.Disposed, this);
        }

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
    }
}
