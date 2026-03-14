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

        /// <summary>
        /// Generates a digital signature for the specified hash using the configured elliptic curve key and algorithm.
        /// </summary>
        /// <remarks>The method selects the appropriate hash algorithm based on the key size. Supported
        /// key sizes are 256, 384, and 521 bits, corresponding to SHA-256, SHA-384, and SHA-512 respectively.</remarks>
        /// <param name="Hash">The hash value to be signed. The length of the hash must correspond to the key size and supported hash
        /// algorithm.</param>
        /// <returns>A byte array containing the digital signature of the provided hash.</returns>
        /// <exception cref="ArgumentException">Thrown if the length of the hash does not match the expected digest length for the key size.</exception>
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

        /// <summary>
        /// Computes the hash value for a specified region of the input data using the provided hash algorithm.
        /// </summary>
        /// <remarks>The method supports hashing only the specified segment of the input array, allowing
        /// partial data hashing. The hash algorithm must be compatible with the key size used by the
        /// implementation.</remarks>
        /// <param name="Data">The byte array containing the data to be hashed.</param>
        /// <param name="Offset">The zero-based index in the data array at which to begin hashing.</param>
        /// <param name="Count">The number of bytes to hash, starting from the specified offset.</param>
        /// <param name="HashAlgorithm">The hash algorithm to use for computing the hash value.</param>
        /// <returns>A byte array containing the computed hash value for the specified data segment.</returns>
        protected override byte[] HashData(byte[] Data, int Offset, int Count, HashAlgorithmName HashAlgorithm)
        {
            ValidateKeyDigestCombination(KeySize, HashAlgorithm);

            return HashDataIncremental(Data, Offset, Count, HashAlgorithm);
        }

        /// <summary>
        /// Computes the hash value for a specified segment of a byte array using the given hash algorithm.
        /// </summary>
        /// <remarks>This method processes only the specified segment of the input array, defined by the
        /// offset and count parameters. The hash algorithm is applied incrementally, which can be useful for hashing
        /// large data in segments.</remarks>
        /// <param name="Data">The byte array containing the data to hash.</param>
        /// <param name="Offset">The zero-based index in the array at which to begin hashing.</param>
        /// <param name="Count">The number of bytes to hash, starting from the specified offset.</param>
        /// <param name="HashAlgorithm">The hash algorithm to use for computing the hash value.</param>
        /// <returns>A byte array containing the computed hash value for the specified data segment.</returns>
        private static byte[] HashDataIncremental(byte[] Data, int Offset, int Count, HashAlgorithmName HashAlgorithm)
        {
            using (IncrementalHash incrementalHash = IncrementalHash.CreateHash(HashAlgorithm))
            {
                incrementalHash.AppendData(Data, Offset, Count);
                return incrementalHash.GetHashAndReset();
            }
        }

        /// <summary>
        /// Computes the hash value for a specified segment of a byte array using the given hash algorithm.
        /// </summary>
        /// <remarks>This method processes only the specified segment of the input array, defined by the
        /// offset and count parameters. The caller is responsible for ensuring that the offset and count are within the
        /// bounds of the array. The method does not allocate an IncrementalHash instance for each call, which may
        /// improve performance in scenarios where hashing is performed infrequently.</remarks>
        /// <param name="Data">The byte array containing the data to hash.</param>
        /// <param name="Offset">The zero-based index in the array at which to begin hashing.</param>
        /// <param name="Count">The number of bytes to hash, starting from the specified offset.</param>
        /// <param name="HashAlgorithm">The hash algorithm to use for computing the hash value.</param>
        /// <returns>A byte array containing the computed hash value for the specified data segment.</returns>
        private static byte[] HashDataStatic(byte[] Data, int Offset, int Count, HashAlgorithmName HashAlgorithm)
        {
            // Use static method for hashing to avoid allocation of IncrementalHash
            using var incrementalHash = IncrementalHash.CreateHash(HashAlgorithm);
            incrementalHash.AppendData(Data, Offset, Count);
            return incrementalHash.GetHashAndReset();
        }

        /// <summary>
        /// Computes the hash of a specified segment of a byte array using the given hash algorithm.
        /// </summary>
        /// <remarks>This method avoids allocation of incremental hash objects by using static hash
        /// algorithm instances. Only SHA256, SHA384, and SHA512 are supported.</remarks>
        /// <param name="Data">The byte array containing the data to hash.</param>
        /// <param name="Offset">The zero-based index in the array at which to begin hashing.</param>
        /// <param name="Count">The number of bytes to hash, starting from the specified offset.</param>
        /// <param name="HashAlgorithm">The hash algorithm to use for computing the hash. Supported values are SHA256, SHA384, and SHA512.</param>
        /// <returns>A byte array containing the computed hash value for the specified segment of the input data.</returns>
        /// <exception cref="NotSupportedException">Thrown if the specified hash algorithm is not supported.</exception>
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