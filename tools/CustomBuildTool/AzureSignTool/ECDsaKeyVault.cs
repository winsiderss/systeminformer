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
        /// <param name="Context">Context with parameters</param>
        public ECDsaKeyVault(KeyVaultContext Context)
        {
            if (!Context.IsValid)
                throw new ArgumentException("Must not be the default", nameof(Context));

            this.VaultContext = Context;
            if (Context.Certificate != null)
            {
                this.PublicKey = Context.Certificate.GetECDsaPublicKey();
            }
            else
            {
                throw new ArgumentException("Public key must be provided in KeyVaultContext", nameof(Context));
            }

            this.KeySizeValue = this.PublicKey.KeySize;
            this.LegalKeySizesValue = new[] { new KeySizes(this.PublicKey.KeySize, this.PublicKey.KeySize, 0) };
        }

        public override byte[] SignHash(byte[] Hash)
        {
            CheckDisposed();
            ValidateKeyDigestCombination(KeySize, Hash.Length);

            if (KeySize == 256)
                return this.VaultContext.SignDigest(Hash, HashAlgorithmName.SHA256, KeyVaultSignatureAlgorithm.ECDsa);
            if (KeySize == 384)
                return this.VaultContext.SignDigest(Hash, HashAlgorithmName.SHA384, KeyVaultSignatureAlgorithm.ECDsa);
            if (KeySize == 521)
                return this.VaultContext.SignDigest(Hash, HashAlgorithmName.SHA512, KeyVaultSignatureAlgorithm.ECDsa);

            throw new ArgumentException("Digest length is not valid for the key size.", nameof(Hash));
        }

        protected override byte[] HashData(byte[] Data, int Offset, int Count, HashAlgorithmName HashAlgorithm)
        {
            ValidateKeyDigestCombination(KeySize, HashAlgorithm);

            return HashDataIncremental(Data, Offset, Count, HashAlgorithm);
        }

        private static byte[] HashDataIncremental(byte[] Data, int Offset, int Count, HashAlgorithmName HashAlgorithm)
        {
            using (IncrementalHash incrementalHash = IncrementalHash.CreateHash(HashAlgorithm))
            {
                incrementalHash.AppendData(Data, Offset, Count);
                return incrementalHash.GetHashAndReset();
            }
        }

        private static byte[] HashDataStatic(byte[] Data, int Offset, int Count, HashAlgorithmName HashAlgorithm)
        {
            // Use static method for hashing to avoid allocation of IncrementalHash
            using var incrementalHash = IncrementalHash.CreateHash(HashAlgorithm);
            incrementalHash.AppendData(Data, Offset, Count);
            return incrementalHash.GetHashAndReset();
        }

        private static byte[] HashDataFixed(byte[] Data, int Offset, int Count, HashAlgorithmName HashAlgorithm)
        {
            // Use static method for hashing to avoid allocation of IncrementalHash
            if (HashAlgorithm == HashAlgorithmName.SHA256)
            {
                using var sha256 = System.Security.Cryptography.SHA256.Create();
                return sha256.ComputeHash(Data, Offset, Count);
            }
            if (HashAlgorithm == HashAlgorithmName.SHA384)
            {
                using var sha384 = System.Security.Cryptography.SHA384.Create();
                return sha384.ComputeHash(Data, Offset, Count);
            }
            if (HashAlgorithm == HashAlgorithmName.SHA512)
            {
                using var sha512 = System.Security.Cryptography.SHA512.Create();
                return sha512.ComputeHash(Data, Offset, Count);
            }
            throw new NotSupportedException("The specified algorithm is not supported.");
        }

        /// <inheritdoc/>
        public override bool VerifyHash(byte[] Hash, byte[] Signature)
        {
            CheckDisposed();
            ValidateKeyDigestCombination(KeySize, Hash.Length);

            // Avoid null check every call
            var publicKey = this.PublicKey;
            ObjectDisposedException.ThrowIf(publicKey is null, this);

            return publicKey.VerifyHash(Hash, Signature);
        }

        ///<inheritdoc/>
        public override ECParameters ExportParameters(bool IncludePrivateParameters)
        {
            if (IncludePrivateParameters)
            {
                throw new NotSupportedException("Private keys cannot be exported by this provider");
            }

            return this.PublicKey.ExportParameters(false);
        }

        ///<inheritdoc/>
        public override ECParameters ExportExplicitParameters(bool IncludePrivateParameters)
        {
            if (IncludePrivateParameters)
            {
                throw new NotSupportedException("Private keys cannot be exported by this provider");
            }

            return this.PublicKey.ExportExplicitParameters(false);
        }

        /// <summary>
        /// Importing parameters is not supported.
        /// </summary>
        public override void ImportParameters(ECParameters Parameters) =>
            throw new NotSupportedException();

        /// <summary>
        /// Key generation is not supported.
        /// </summary>
        public override void GenerateKey(ECCurve Curve) => throw new NotSupportedException();

        /// <inheritdoc/>
        public override string ToXmlString(bool IncludePrivateParameters)
        {
            if (IncludePrivateParameters)
            {
                throw new NotSupportedException("Private keys cannot be exported by this provider");
            }

            return this.PublicKey.ToXmlString(false);
        }

        /// <summary>
        /// Importing parameters from XML is not supported.
        public override void FromXmlString(string XmlString) => throw new NotSupportedException();

        private static void ValidateKeyDigestCombination(int KeySizeBits, int DigestSizeBytes)
        {
            if ((KeySizeBits == 256 && DigestSizeBytes == 32) ||
                (KeySizeBits == 384 && DigestSizeBytes == 48) ||
                (KeySizeBits == 521 && DigestSizeBytes == 64))
            {
                return;
            }

            throw new NotSupportedException($"The key size '{KeySizeBits}' is not valid for digest of size '{DigestSizeBytes}' bytes.");
        }

        private static void ValidateKeyDigestCombination(int KeySizeBits, HashAlgorithmName HashAlgorithmName)
        {
            switch (KeySizeBits)
            {
            case 256 when HashAlgorithmName == HashAlgorithmName.SHA256:
            case 384 when HashAlgorithmName == HashAlgorithmName.SHA384:
            case 521 when HashAlgorithmName == HashAlgorithmName.SHA512:
                return;
            }
            throw new NotSupportedException($"The key size '{KeySizeBits}' is not valid for digest algorithm '{HashAlgorithmName}'.");
        }

        void CheckDisposed()
        {
            ObjectDisposedException.ThrowIf(this.Disposed, this);
        }

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
    }
}