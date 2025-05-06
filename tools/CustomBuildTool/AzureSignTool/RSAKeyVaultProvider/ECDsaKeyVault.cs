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
        private readonly KeyVaultContext context;
        private ECDsa publicKey;
        private bool disposed;

        /// <summary>
        /// Creates a new ECDsaKeyVault instance
        /// </summary>
        /// <param name="context">Context with parameters</param>
        public ECDsaKeyVault(KeyVaultContext context)
        {
            if (!context.IsValid)
                throw new ArgumentException("Must not be the default", nameof(context));

            this.context = context;
            publicKey = context.Key.ToECDsa();
            KeySizeValue = publicKey.KeySize;
            LegalKeySizesValue = [new KeySizes(publicKey.KeySize, publicKey.KeySize, 0)];
        }

        public override byte[] SignHash(byte[] hash)
        {
            CheckDisposed();
            ValidateKeyDigestCombination(KeySize, hash.Length);

            return KeySize switch
            {
                256 => context.SignDigest(hash, HashAlgorithmName.SHA256, KeyVaultSignatureAlgorithm.ECDsa),
                384 => context.SignDigest(hash, HashAlgorithmName.SHA384, KeyVaultSignatureAlgorithm.ECDsa),
                521 => context.SignDigest(hash, HashAlgorithmName.SHA512, KeyVaultSignatureAlgorithm.ECDsa),
                _ => throw new ArgumentException("Digest length is not valid for the key size.", nameof(hash)),
            };
        }

        protected override byte[] HashData(byte[] data, int offset, int count, HashAlgorithmName hashAlgorithm)
        {
            ValidateKeyDigestCombination(KeySize, hashAlgorithm);

            using (IncrementalHash hash = IncrementalHash.CreateHash(hashAlgorithm))
            {
                hash.AppendData(data, offset, count);
                return hash.GetHashAndReset();
            }
        }

        /// <inheritdoc/>
        public override bool VerifyHash(byte[] hash, byte[] signature)
        {
            CheckDisposed();
            ValidateKeyDigestCombination(KeySize, hash.Length);

            return publicKey.VerifyHash(hash, signature);
        }

        ///<inheritdoc/>
        public override ECParameters ExportParameters(bool includePrivateParameters)
        {
            if (includePrivateParameters)
                throw new Exception("Private keys cannot be exported by this provider");

            return publicKey.ExportParameters(false);
        }

        ///<inheritdoc/>
        public override ECParameters ExportExplicitParameters(bool includePrivateParameters)
        {
            if (includePrivateParameters)
                throw new Exception("Private keys cannot be exported by this provider");

            return publicKey.ExportExplicitParameters(false);
        }

        /// <summary>
        /// Importing parameters is not supported.
        /// </summary>
        public override void ImportParameters(ECParameters parameters) =>
            throw new NotSupportedException();

        /// <summary>
        /// Key generation is not supported.
        /// </summary>
        public override void GenerateKey(ECCurve curve) =>
            throw new NotSupportedException();

        /// <inheritdoc/>
        public override string ToXmlString(bool includePrivateParameters)
        {
            if (includePrivateParameters)
                throw new Exception("Private keys cannot be exported by this provider");

            return publicKey.ToXmlString(false);
        }

        /// <summary>
        /// Importing parameters from XML is not supported.
        public override void FromXmlString(string xmlString) =>
            throw new NotSupportedException();

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
            if (hashAlgorithmName != HashAlgorithmName.SHA256 &&
                hashAlgorithmName != HashAlgorithmName.SHA384 &&
                hashAlgorithmName != HashAlgorithmName.SHA512)
            {
                throw new NotSupportedException("The specified algorithm is not supported.");
            }

            if ((keySizeBits == 256 && hashAlgorithmName == HashAlgorithmName.SHA256) ||
                (keySizeBits == 384 && hashAlgorithmName == HashAlgorithmName.SHA384) ||
                (keySizeBits == 521 && hashAlgorithmName == HashAlgorithmName.SHA512))
            {
                return;
            }

            throw new NotSupportedException($"The key size '{keySizeBits}' is not valid for digest algorithm '{hashAlgorithmName}'.");
        }

        void CheckDisposed()
        {
            ObjectDisposedException.ThrowIf(disposed, this);
        }

        ~ECDsaKeyVault()
        {
            Dispose(false);
        }

        public new void Dispose()
        {
            Dispose(true);
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
            GC.SuppressFinalize(this);
        }
    }
}
