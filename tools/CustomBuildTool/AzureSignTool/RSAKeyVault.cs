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
    public sealed class RSAKeyVault : RSA
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

            //
            // Build public key from certificate if available
            //

            if (Context.Certificate != null)
            {
                this.PublicKey = Context.Certificate.GetRSAPublicKey();

                ArgumentNullException.ThrowIfNull(this.PublicKey);
            }
            else
            {
                throw new ArgumentException("Public key must be provided in KeyVaultContext", nameof(Context));
            }

            this.KeySizeValue = this.PublicKey.KeySize;
            this.LegalKeySizesValue = [new KeySizes(this.PublicKey.KeySize, this.PublicKey.KeySize, 0)];
        }

        /// <inheritdoc/>
        public override string KeyExchangeAlgorithm
        {
            get
            {
                CheckDisposed();
                return "RSA";
            }
        }

        /// <inheritdoc/>
        public override int KeySize
        {
            get
            {
                CheckDisposed();
                return base.KeySize;
            }
            set => throw new NotSupportedException("Key generation is not supported by this provider");
        }

        /// <inheritdoc/>
        public override KeySizes[] LegalKeySizes
        {
            get
            {
                CheckDisposed();
                return (KeySizes[])base.LegalKeySizes.Clone();
            }
        }

        /// <inheritdoc/>
        public override string SignatureAlgorithm
        {
            get
            {
                CheckDisposed();
                return "RSA";
            }
        }

        /// <inheritdoc/>
        public override byte[] SignHash(byte[] Hash, HashAlgorithmName HashAlgorithm, RSASignaturePadding Padding)
        {
            CheckDisposed();

            //
            // Key Vault only supports PKCSv1 padding
            //

            if (Padding.Mode != RSASignaturePaddingMode.Pkcs1)
            {
                throw new NotSupportedException("Unsupported padding mode");
            }

            return this.VaultContext.SignDigest(Hash, HashAlgorithm, KeyVaultSignatureAlgorithm.RSAPkcs15);
        }

        /// <inheritdoc/>
        public override bool TrySignHash(ReadOnlySpan<byte> Hash, Span<byte> Destination, HashAlgorithmName HashAlgorithm, RSASignaturePadding Padding, out int BytesWritten)
        {
            CheckDisposed();

            byte[] signature = SignHash(Hash.ToArray(), HashAlgorithm, Padding);

            if (signature.Length > Destination.Length)
            {
                BytesWritten = 0;
                return false;
            }

            signature.CopyTo(Destination);
            BytesWritten = signature.Length;
            return true;
        }

        /// <inheritdoc/>
        public override byte[] SignData(byte[] Data, int Offset, int Count, HashAlgorithmName HashAlgorithm, RSASignaturePadding Padding)
        {
            CheckDisposed();

            return SignHash(HashData(Data, Offset, Count, HashAlgorithm), HashAlgorithm, Padding);
        }

        /// <inheritdoc/>
        public override byte[] SignData(Stream Data, HashAlgorithmName HashAlgorithm, RSASignaturePadding Padding)
        {
            CheckDisposed();

            return SignHash(HashData(Data, HashAlgorithm), HashAlgorithm, Padding);
        }

        /// <inheritdoc/>
        public override bool TrySignData(ReadOnlySpan<byte> Data, Span<byte> Destination, HashAlgorithmName HashAlgorithm, RSASignaturePadding Padding, out int BytesWritten)
        {
            CheckDisposed();

            byte[] hash = HashData(Data, HashAlgorithm);

            return TrySignHash(hash, Destination, HashAlgorithm, Padding, out BytesWritten);
        }

        /// <inheritdoc/>
        public override bool VerifyHash(byte[] Hash, byte[] Signature, HashAlgorithmName HashAlgorithm, RSASignaturePadding Padding)
        {
            CheckDisposed();
            return this.PublicKey.VerifyHash(Hash, Signature, HashAlgorithm, Padding);
        }

        /// <inheritdoc/>
        public override bool VerifyHash(ReadOnlySpan<byte> Hash, ReadOnlySpan<byte> Signature, HashAlgorithmName HashAlgorithm, RSASignaturePadding Padding)
        {
            CheckDisposed();

            return this.PublicKey.VerifyHash(Hash, Signature, HashAlgorithm, Padding);
        }

        /// <inheritdoc/>
        public override bool VerifyData(byte[] Data, int Offset, int Count, byte[] Signature, HashAlgorithmName HashAlgorithm, RSASignaturePadding Padding)
        {
            CheckDisposed();

            return this.PublicKey.VerifyHash(HashData(Data, Offset, Count, HashAlgorithm), Signature, HashAlgorithm, Padding);
        }

        /// <inheritdoc/>
        public override bool VerifyData(ReadOnlySpan<byte> Data, ReadOnlySpan<byte> Signature, HashAlgorithmName HashAlgorithm, RSASignaturePadding Padding)
        {
            CheckDisposed();

            byte[] hash = HashData(Data, HashAlgorithm);

            return this.PublicKey.VerifyHash(hash, Signature, HashAlgorithm, Padding);
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
        protected override bool TryHashData(ReadOnlySpan<byte> Data, Span<byte> Destination, HashAlgorithmName HashAlgorithm, out int BytesWritten)
        {
            CheckDisposed();

            byte[] hash = HashData(Data, HashAlgorithm);

            if (hash.Length > Destination.Length)
            {
                BytesWritten = 0;
                return false;
            }

            hash.CopyTo(Destination);
            BytesWritten = hash.Length;
            return true;
        }

        /// <inheritdoc/>
        public override byte[] Decrypt(byte[] Data, RSAEncryptionPadding Padding)
        {
            CheckDisposed();

            return this.VaultContext.DecryptData(Data, Padding);
        }

        /// <inheritdoc/>
        public override bool TryDecrypt(ReadOnlySpan<byte> Data, Span<byte> Destination, RSAEncryptionPadding Padding, out int BytesWritten)
        {
            CheckDisposed();

            byte[] decrypted = Decrypt(Data.ToArray(), Padding);

            if (decrypted.Length > Destination.Length)
            {
                BytesWritten = 0;
                return false;
            }

            decrypted.CopyTo(Destination);
            BytesWritten = decrypted.Length;
            return true;
        }

        /// <inheritdoc/>
        public override byte[] Encrypt(byte[] Data, RSAEncryptionPadding Padding)
        {
            CheckDisposed();

            return this.PublicKey.Encrypt(Data, Padding);
        }

        /// <inheritdoc/>
        public override bool TryEncrypt(ReadOnlySpan<byte> Data, Span<byte> Destination, RSAEncryptionPadding Padding, out int BytesWritten)
        {
            CheckDisposed();

            return this.PublicKey.TryEncrypt(Data, Destination, Padding, out BytesWritten);
        }

        /// <inheritdoc/>
        [Obsolete("This method is obsolete. Use Decrypt instead.")]
        public override byte[] DecryptValue(byte[] Rgb)
        {
            throw new NotSupportedException();
        }

        /// <inheritdoc/>
        [Obsolete("This method is obsolete. Use Encrypt instead.")]
        public override byte[] EncryptValue(byte[] Rgb)
        {
            throw new NotSupportedException();
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

        /// <inheritdoc/>
        public override byte[] ExportRSAPublicKey()
        {
            CheckDisposed();

            return this.PublicKey.ExportRSAPublicKey();
        }

        /// <inheritdoc/>
        public override bool TryExportRSAPublicKey(Span<byte> Destination, out int BytesWritten)
        {
            CheckDisposed();

            return this.PublicKey.TryExportRSAPublicKey(Destination, out BytesWritten);
        }

        /// <inheritdoc/>
        public override byte[] ExportSubjectPublicKeyInfo()
        {
            CheckDisposed();

            return this.PublicKey.ExportSubjectPublicKeyInfo();
        }

        /// <inheritdoc/>
        public override bool TryExportSubjectPublicKeyInfo(Span<byte> Destination, out int BytesWritten)
        {
            CheckDisposed();

            return this.PublicKey.TryExportSubjectPublicKeyInfo(Destination, out BytesWritten);
        }

        /// <inheritdoc/>
        public override byte[] ExportRSAPrivateKey()
        {
            throw new NotSupportedException("Private keys cannot be exported by this provider");
        }

        /// <inheritdoc/>
        public override bool TryExportRSAPrivateKey(Span<byte> Destination, out int BytesWritten)
        {
            BytesWritten = 0;
            throw new NotSupportedException("Private keys cannot be exported by this provider");
        }

        /// <inheritdoc/>
        public override byte[] ExportPkcs8PrivateKey()
        {
            throw new NotSupportedException("Private keys cannot be exported by this provider");
        }

        /// <inheritdoc/>
        public override bool TryExportPkcs8PrivateKey(Span<byte> Destination, out int BytesWritten)
        {
            BytesWritten = 0;
            throw new NotSupportedException("Private keys cannot be exported by this provider");
        }

        /// <inheritdoc/>
        public override byte[] ExportEncryptedPkcs8PrivateKey(ReadOnlySpan<byte> PasswordBytes, PbeParameters PbeParameters)
        {
            throw new NotSupportedException("Private keys cannot be exported by this provider");
        }

        /// <inheritdoc/>
        public override byte[] ExportEncryptedPkcs8PrivateKey(ReadOnlySpan<char> Password, PbeParameters PbeParameters)
        {
            throw new NotSupportedException("Private keys cannot be exported by this provider");
        }

        /// <inheritdoc/>
        public override bool TryExportEncryptedPkcs8PrivateKey(ReadOnlySpan<byte> PasswordBytes, PbeParameters PbeParameters, Span<byte> Destination, out int BytesWritten)
        {
            BytesWritten = 0;
            throw new NotSupportedException("Private keys cannot be exported by this provider");
        }

        /// <inheritdoc/>
        public override bool TryExportEncryptedPkcs8PrivateKey(ReadOnlySpan<char> Password, PbeParameters PbeParameters, Span<byte> Destination, out int BytesWritten)
        {
            BytesWritten = 0;
            throw new NotSupportedException("Private keys cannot be exported by this provider");
        }

        /// <inheritdoc/>
        public override void ImportRSAPrivateKey(ReadOnlySpan<byte> Source, out int BytesRead)
        {
            BytesRead = 0;
            throw new NotSupportedException();
        }

        /// <inheritdoc/>
        public override void ImportPkcs8PrivateKey(ReadOnlySpan<byte> Source, out int BytesRead)
        {
            BytesRead = 0;
            throw new NotSupportedException();
        }

        /// <inheritdoc/>
        public override void ImportEncryptedPkcs8PrivateKey(ReadOnlySpan<byte> PasswordBytes, ReadOnlySpan<byte> Source, out int BytesRead)
        {
            BytesRead = 0;
            throw new NotSupportedException();
        }

        /// <inheritdoc/>
        public override void ImportEncryptedPkcs8PrivateKey(ReadOnlySpan<char> Password, ReadOnlySpan<byte> Source, out int BytesRead)
        {
            BytesRead = 0;
            throw new NotSupportedException();
        }

        /// <inheritdoc/>
        public override void ImportRSAPublicKey(ReadOnlySpan<byte> Source, out int BytesRead)
        {
            BytesRead = 0;
            throw new NotSupportedException();
        }

        /// <inheritdoc/>
        public override void ImportSubjectPublicKeyInfo(ReadOnlySpan<byte> Source, out int BytesRead)
        {
            BytesRead = 0;
            throw new NotSupportedException();
        }

        /// <inheritdoc/>
        public override void ImportFromPem(ReadOnlySpan<char> Input)
        {
            throw new NotSupportedException();
        }

        /// <inheritdoc/>
        public override void ImportFromEncryptedPem(ReadOnlySpan<char> Input, ReadOnlySpan<byte> PasswordBytes)
        {
            throw new NotSupportedException();
        }

        /// <inheritdoc/>
        public override void ImportFromEncryptedPem(ReadOnlySpan<char> Input, ReadOnlySpan<char> Password)
        {
            throw new NotSupportedException();
        }

        /// <inheritdoc/>
        public override string ToXmlString(bool IncludePrivateParameters)
        {
            CheckDisposed();

            if (IncludePrivateParameters)
            {
                throw new NotSupportedException("Private keys cannot be exported by this provider");
            }

            return this.PublicKey.ToXmlString(false);
        }

        /// <inheritdoc/>
        public override void FromXmlString(string XmlString)
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

        private static byte[] HashData(ReadOnlySpan<byte> Data, HashAlgorithmName Algorithm)
        {
            using (var digestAlgorithm = IncrementalHash.CreateHash(Algorithm))
            {
                digestAlgorithm.AppendData(Data);
                return digestAlgorithm.GetHashAndReset();
            }
        }
    }
}
