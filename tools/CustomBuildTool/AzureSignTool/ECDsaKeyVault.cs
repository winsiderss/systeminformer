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

        /// <inheritdoc/>
        public override string KeyExchangeAlgorithm
        {
            get
            {
                CheckDisposed();
                return null;
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
                return "ECDsa";
            }
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

        /// <inheritdoc/>
        public override bool TrySignHash(ReadOnlySpan<byte> Hash, Span<byte> Destination, out int BytesWritten)
        {
            CheckDisposed();

            byte[] signature = SignHash(Hash.ToArray());

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
        protected override byte[] SignHashCore(ReadOnlySpan<byte> Hash, DSASignatureFormat SignatureFormat)
        {
            CheckDisposed();

            return ConvertSignatureToFormat(SignHash(Hash.ToArray()), SignatureFormat);
        }

        /// <inheritdoc/>
        protected override bool TrySignHashCore(ReadOnlySpan<byte> Hash, Span<byte> Destination, DSASignatureFormat SignatureFormat, out int BytesWritten)
        {
            CheckDisposed();

            byte[] signature = SignHashCore(Hash, SignatureFormat);

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
        public override byte[] SignData(byte[] Data, HashAlgorithmName HashAlgorithm)
        {
            ArgumentNullException.ThrowIfNull(Data);

            return SignData(Data, 0, Data.Length, HashAlgorithm);
        }

        /// <inheritdoc/>
        public override byte[] SignData(byte[] Data, int Offset, int Count, HashAlgorithmName HashAlgorithm)
        {
            CheckDisposed();

            return SignHash(HashData(Data, Offset, Count, HashAlgorithm));
        }

        /// <inheritdoc/>
        public override byte[] SignData(Stream Data, HashAlgorithmName HashAlgorithm)
        {
            CheckDisposed();

            return SignHash(HashData(Data, HashAlgorithm));
        }

        /// <inheritdoc/>
        public override bool TrySignData(ReadOnlySpan<byte> Data, Span<byte> Destination, HashAlgorithmName HashAlgorithm, out int BytesWritten)
        {
            CheckDisposed();

            byte[] hash = HashDataIncremental(Data, HashAlgorithm);

            return TrySignHash(hash, Destination, out BytesWritten);
        }

        /// <inheritdoc/>
        protected override byte[] SignDataCore(ReadOnlySpan<byte> Data, HashAlgorithmName HashAlgorithm, DSASignatureFormat SignatureFormat)
        {
            CheckDisposed();

            return SignHashCore(HashDataIncremental(Data, HashAlgorithm), SignatureFormat);
        }

        /// <inheritdoc/>
        protected override byte[] SignDataCore(Stream Data, HashAlgorithmName HashAlgorithm, DSASignatureFormat SignatureFormat)
        {
            CheckDisposed();

            return SignHashCore(HashData(Data, HashAlgorithm), SignatureFormat);
        }

        /// <inheritdoc/>
        protected override bool TrySignDataCore(ReadOnlySpan<byte> Data, Span<byte> Destination, HashAlgorithmName HashAlgorithm, DSASignatureFormat SignatureFormat, out int BytesWritten)
        {
            CheckDisposed();

            byte[] hash = HashDataIncremental(Data, HashAlgorithm);

            return TrySignHashCore(hash, Destination, SignatureFormat, out BytesWritten);
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
            CheckDisposed();
            ValidateKeyDigestCombination(KeySize, HashAlgorithm);

            return HashDataIncremental(Data, Offset, Count, HashAlgorithm);
        }

        /// <inheritdoc/>
        protected override byte[] HashData(Stream Data, HashAlgorithmName HashAlgorithm)
        {
            CheckDisposed();
            ValidateKeyDigestCombination(KeySize, HashAlgorithm);

            using (IncrementalHash incrementalHash = IncrementalHash.CreateHash(HashAlgorithm))
            {
                byte[] buffer = new byte[0x4000];
                int bytesRead;

                while ((bytesRead = Data.Read(buffer, 0, buffer.Length)) != 0)
                {
                    incrementalHash.AppendData(buffer, 0, bytesRead);
                }

                return incrementalHash.GetHashAndReset();
            }
        }

        /// <inheritdoc/>
        protected override bool TryHashData(ReadOnlySpan<byte> Data, Span<byte> Destination, HashAlgorithmName HashAlgorithm, out int BytesWritten)
        {
            CheckDisposed();
            ValidateKeyDigestCombination(KeySize, HashAlgorithm);

            byte[] hash = HashDataIncremental(Data, HashAlgorithm);

            if (hash.Length > Destination.Length)
            {
                BytesWritten = 0;
                return false;
            }

            hash.CopyTo(Destination);
            BytesWritten = hash.Length;
            return true;
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
            ReadOnlySpan<byte> span = Data.AsSpan(Offset, Count);

            using (IncrementalHash incrementalHash = IncrementalHash.CreateHash(HashAlgorithm))
            {
                incrementalHash.AppendData(span);
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
            ReadOnlySpan<byte> span = Data.AsSpan(Offset, Count);

            return HashAlgorithm.Name switch
            {
                // Fast path: use allocation-free static hashing for supported algorithms
                nameof(HashAlgorithmName.SHA256) => SHA256.HashData(span),
                nameof(HashAlgorithmName.SHA384) => SHA384.HashData(span),
                nameof(HashAlgorithmName.SHA512) => SHA512.HashData(span),
                // Fallback path: required for algorithms without static HashData APIs
                _ => HashDataIncremental(span, HashAlgorithm)
            };
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
            ReadOnlySpan<byte> span = Data.AsSpan(Offset, Count);

            return HashAlgorithm.Name switch
            {
                // Fast path: use allocation-free static hashing for supported algorithms
                nameof(HashAlgorithmName.SHA256) => SHA256.HashData(span),
                nameof(HashAlgorithmName.SHA384) => SHA384.HashData(span),
                nameof(HashAlgorithmName.SHA512) => SHA512.HashData(span),
                // Fallback path: required for algorithms without static HashData APIs
                _ => HashDataIncremental(span, HashAlgorithm)
            };
        }

        /// <summary>
        /// Computes the cryptographic hash of the specified data using the provided hash algorithm.
        /// </summary>
        /// <remarks>This method processes the entire input data in a single operation using an
        /// incremental hashing approach. The caller is responsible for selecting a supported hash algorithm.</remarks>
        /// <param name="Data">The input data to compute the hash for.</param>
        /// <param name="HashAlgorithm">The hash algorithm to use for computing the hash.</param>
        /// <returns>A byte array containing the computed hash value.</returns>
        private static byte[] HashDataIncremental(ReadOnlySpan<byte> Data, HashAlgorithmName HashAlgorithm)
        {
            using (IncrementalHash incrementalHash = IncrementalHash.CreateHash(HashAlgorithm))
            {
                incrementalHash.AppendData(Data);
                return incrementalHash.GetHashAndReset();
            }
        }

        /// <summary>
        /// Verifies that a digital signature is valid for the specified hash value using the current public key.
        /// </summary>
        /// <remarks>This method requires that the object has not been disposed and that the key and hash
        /// sizes are compatible. If the public key is not available or the object is disposed, an exception is
        /// thrown.</remarks>
        /// <param name="Hash">The hash value to verify. This must match the hash algorithm and key size used to generate the signature.</param>
        /// <param name="Signature">The digital signature to verify against the specified hash.</param>
        /// <returns>true if the signature is valid for the specified hash and public key; otherwise, false.</returns>
        public override bool VerifyHash(byte[] Hash, byte[] Signature)
        {
            CheckDisposed();
            ValidateKeyDigestCombination(KeySize, Hash.Length);

            ECDsa publicKey = this.PublicKey;
            ObjectDisposedException.ThrowIf(publicKey is null, this);

            return publicKey.VerifyHash(Hash, Signature);
        }

        /// <inheritdoc/>
        public override bool VerifyHash(ReadOnlySpan<byte> Hash, ReadOnlySpan<byte> Signature)
        {
            CheckDisposed();
            ValidateKeyDigestCombination(KeySize, Hash.Length);

            return this.PublicKey.VerifyHash(Hash, Signature);
        }

        /// <inheritdoc/>
        protected override bool VerifyHashCore(ReadOnlySpan<byte> Hash, ReadOnlySpan<byte> Signature, DSASignatureFormat SignatureFormat)
        {
            CheckDisposed();
            ValidateKeyDigestCombination(KeySize, Hash.Length);

            return this.PublicKey.VerifyHash(Hash, ConvertSignatureFromFormat(Signature, SignatureFormat));
        }

        /// <inheritdoc/>
        public override bool VerifyData(byte[] Data, int Offset, int Count, byte[] Signature, HashAlgorithmName HashAlgorithm)
        {
            CheckDisposed();

            return this.PublicKey.VerifyHash(HashData(Data, Offset, Count, HashAlgorithm), Signature);
        }

        /// <inheritdoc/>
        public override bool VerifyData(ReadOnlySpan<byte> Data, ReadOnlySpan<byte> Signature, HashAlgorithmName HashAlgorithm)
        {
            CheckDisposed();

            return this.PublicKey.VerifyHash(HashDataIncremental(Data, HashAlgorithm), Signature);
        }

        /// <inheritdoc/>
        protected override bool VerifyDataCore(ReadOnlySpan<byte> Data, ReadOnlySpan<byte> Signature, HashAlgorithmName HashAlgorithm, DSASignatureFormat SignatureFormat)
        {
            CheckDisposed();

            return this.PublicKey.VerifyHash(HashDataIncremental(Data, HashAlgorithm), ConvertSignatureFromFormat(Signature, SignatureFormat));
        }

        /// <inheritdoc/>
        protected override bool VerifyDataCore(Stream Data, ReadOnlySpan<byte> Signature, HashAlgorithmName HashAlgorithm, DSASignatureFormat SignatureFormat)
        {
            CheckDisposed();

            return this.PublicKey.VerifyHash(HashData(Data, HashAlgorithm), ConvertSignatureFromFormat(Signature, SignatureFormat));
        }

        /// <summary>
        /// Exports the public key parameters for the current elliptic curve cryptography (ECC) key.
        /// </summary>
        /// <remarks>This method only supports exporting public key parameters. Attempting to export
        /// private key parameters will result in an exception.</remarks>
        /// <param name="IncludePrivateParameters">true to include private parameters; otherwise, false. Private parameters are not supported by this provider.</param>
        /// <returns>An ECParameters structure that contains the public key parameters.</returns>
        /// <exception cref="NotSupportedException">Thrown if IncludePrivateParameters is set to true, as exporting private key parameters is not supported by
        /// this provider.</exception>
        public override ECParameters ExportParameters(bool IncludePrivateParameters)
        {
            CheckDisposed();

            if (IncludePrivateParameters)
            {
                throw new NotSupportedException("Private keys cannot be exported by this provider");
            }

            return this.PublicKey.ExportParameters(false);
        }

        /// <summary>
        /// Exports the explicit parameters for the elliptic curve public key associated with this instance.
        /// </summary>
        /// <param name="IncludePrivateParameters">true to include private parameters; otherwise, false. Private parameters are not supported by this provider.</param>
        /// <returns>An ECParameters structure that contains the explicit parameters for the public key.</returns>
        /// <exception cref="NotSupportedException">Thrown if IncludePrivateParameters is set to true, as exporting private key parameters is not supported by
        /// this provider.</exception>
        public override ECParameters ExportExplicitParameters(bool IncludePrivateParameters)
        {
            CheckDisposed();

            if (IncludePrivateParameters)
            {
                throw new NotSupportedException("Private keys cannot be exported by this provider");
            }

            return this.PublicKey.ExportExplicitParameters(false);
        }

        /// <summary>
        /// Importing parameters is not supported.
        /// </summary>
        public override void ImportParameters(ECParameters Parameters) => throw new NotSupportedException();

        /// <inheritdoc/>
        public override byte[] ExportSubjectPublicKeyInfo()
        {
            CheckDisposed();

            return this.PublicKey.ExportSubjectPublicKeyInfo();
        }

        /// <inheritdoc/>
        public override byte[] ExportPkcs8PrivateKey()
        {
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

        /// <summary>
        /// Key generation is not supported.
        /// </summary>
        public override void GenerateKey(ECCurve Curve) => throw new NotSupportedException();

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

        /// <summary>
        /// Importing parameters from XML is not supported.
        public override void FromXmlString(string XmlString) => throw new NotSupportedException();

        private byte[] ConvertSignatureToFormat(byte[] IeeeP1363Signature, DSASignatureFormat SignatureFormat)
        {
            return SignatureFormat switch
            {
                DSASignatureFormat.IeeeP1363FixedFieldConcatenation => IeeeP1363Signature,
                DSASignatureFormat.Rfc3279DerSequence => ConvertIeeeP1363ToDer(IeeeP1363Signature),
                _ => throw new ArgumentOutOfRangeException(nameof(SignatureFormat))
            };
        }

        private byte[] ConvertSignatureFromFormat(ReadOnlySpan<byte> Signature, DSASignatureFormat SignatureFormat)
        {
            return SignatureFormat switch
            {
                DSASignatureFormat.IeeeP1363FixedFieldConcatenation => Signature.ToArray(),
                DSASignatureFormat.Rfc3279DerSequence => ConvertDerToIeeeP1363(Signature),
                _ => throw new ArgumentOutOfRangeException(nameof(SignatureFormat))
            };
        }

        private byte[] ConvertDerToIeeeP1363(ReadOnlySpan<byte> Signature)
        {
            int offset = 0;

            if (ReadByte(Signature, ref offset) != 0x30)
                throw new System.Security.Cryptography.CryptographicException("Invalid ECDSA signature.");

            int sequenceLength = ReadDerLength(Signature, ref offset);
            int sequenceEnd = offset + sequenceLength;

            if (sequenceEnd != Signature.Length)
                throw new System.Security.Cryptography.CryptographicException("Invalid ECDSA signature.");

            ReadOnlySpan<byte> r = ReadDerInteger(Signature, ref offset);
            ReadOnlySpan<byte> s = ReadDerInteger(Signature, ref offset);

            if (offset != sequenceEnd)
                throw new System.Security.Cryptography.CryptographicException("Invalid ECDSA signature.");

            int fieldSize = GetFieldSizeBytes();
            byte[] signature = new byte[fieldSize * 2];

            CopyDerIntegerToFixedField(r, signature.AsSpan(0, fieldSize));
            CopyDerIntegerToFixedField(s, signature.AsSpan(fieldSize, fieldSize));

            return signature;
        }

        private byte[] ConvertIeeeP1363ToDer(ReadOnlySpan<byte> Signature)
        {
            int fieldSize = GetFieldSizeBytes();

            if (Signature.Length != fieldSize * 2)
                throw new System.Security.Cryptography.CryptographicException("Invalid ECDSA signature.");

            ReadOnlySpan<byte> r = Signature.Slice(0, fieldSize);
            ReadOnlySpan<byte> s = Signature.Slice(fieldSize, fieldSize);
            int rLength = GetDerIntegerLength(r);
            int sLength = GetDerIntegerLength(s);
            int sequenceContentLength = 2 + rLength + 2 + sLength;
            int sequenceLengthLength = GetDerLengthLength(sequenceContentLength);
            byte[] signature = new byte[1 + sequenceLengthLength + sequenceContentLength];
            int offset = 0;

            signature[offset++] = 0x30;
            WriteDerLength(signature, ref offset, sequenceContentLength);
            WriteDerInteger(signature, ref offset, r, rLength);
            WriteDerInteger(signature, ref offset, s, sLength);

            return signature;
        }

        private int GetFieldSizeBytes()
        {
            return (this.KeySize + 7) / 8;
        }

        private static byte ReadByte(ReadOnlySpan<byte> Data, ref int Offset)
        {
            if (Offset >= Data.Length)
                throw new System.Security.Cryptography.CryptographicException("Invalid ECDSA signature.");

            return Data[Offset++];
        }

        private static int ReadDerLength(ReadOnlySpan<byte> Data, ref int Offset)
        {
            byte firstLengthByte = ReadByte(Data, ref Offset);

            if ((firstLengthByte & 0x80) == 0)
                return firstLengthByte;

            int lengthByteCount = firstLengthByte & 0x7f;

            if (lengthByteCount == 0 || lengthByteCount > sizeof(int))
                throw new System.Security.Cryptography.CryptographicException("Invalid ECDSA signature.");

            int length = 0;

            for (int i = 0; i < lengthByteCount; i++)
            {
                length = (length << 8) | ReadByte(Data, ref Offset);
            }

            return length;
        }

        private static ReadOnlySpan<byte> ReadDerInteger(ReadOnlySpan<byte> Data, ref int Offset)
        {
            if (ReadByte(Data, ref Offset) != 0x02)
                throw new System.Security.Cryptography.CryptographicException("Invalid ECDSA signature.");

            int length = ReadDerLength(Data, ref Offset);

            if (length <= 0 || Offset + length > Data.Length)
                throw new System.Security.Cryptography.CryptographicException("Invalid ECDSA signature.");

            ReadOnlySpan<byte> value = Data.Slice(Offset, length);
            Offset += length;

            return value;
        }

        private static void CopyDerIntegerToFixedField(ReadOnlySpan<byte> Source, Span<byte> Destination)
        {
            while (Source.Length > 1 && Source[0] == 0)
            {
                Source = Source.Slice(1);
            }

            if (Source.Length > Destination.Length)
                throw new System.Security.Cryptography.CryptographicException("Invalid ECDSA signature.");

            Source.CopyTo(Destination.Slice(Destination.Length - Source.Length));
        }

        private static int GetDerIntegerLength(ReadOnlySpan<byte> Value)
        {
            Value = TrimLeadingZeroes(Value);

            if (Value.Length == 0)
                return 1;

            if ((Value[0] & 0x80) != 0)
                return Value.Length + 1;

            return Value.Length;
        }

        private static int GetDerLengthLength(int Length)
        {
            if (Length < 0x80)
                return 1;
            if (Length <= 0xff)
                return 2;
            if (Length <= 0xffff)
                return 3;

            return 4;
        }

        private static void WriteDerLength(Span<byte> Destination, ref int Offset, int Length)
        {
            if (Length < 0x80)
            {
                Destination[Offset++] = (byte)Length;
            }
            else if (Length <= 0xff)
            {
                Destination[Offset++] = 0x81;
                Destination[Offset++] = (byte)Length;
            }
            else if (Length <= 0xffff)
            {
                Destination[Offset++] = 0x82;
                Destination[Offset++] = (byte)(Length >> 8);
                Destination[Offset++] = (byte)Length;
            }
            else
            {
                Destination[Offset++] = 0x83;
                Destination[Offset++] = (byte)(Length >> 16);
                Destination[Offset++] = (byte)(Length >> 8);
                Destination[Offset++] = (byte)Length;
            }
        }

        private static void WriteDerInteger(Span<byte> Destination, ref int Offset, ReadOnlySpan<byte> Value, int Length)
        {
            Value = TrimLeadingZeroes(Value);
            Destination[Offset++] = 0x02;
            WriteDerLength(Destination, ref Offset, Length);

            if (Value.Length == 0)
            {
                Destination[Offset++] = 0;
                return;
            }

            if ((Value[0] & 0x80) != 0)
            {
                Destination[Offset++] = 0;
            }

            Value.CopyTo(Destination.Slice(Offset));
            Offset += Value.Length;
        }

        private static ReadOnlySpan<byte> TrimLeadingZeroes(ReadOnlySpan<byte> Value)
        {
            while (Value.Length != 0 && Value[0] == 0)
            {
                Value = Value.Slice(1);
            }

            return Value;
        }

        /// <summary>
        /// Validates that the specified key size and digest size combination is supported.
        /// </summary>
        /// <remarks>Supported combinations are: 256 bits with 32 bytes, 384 bits with 48 bytes, and 521
        /// bits with 64 bytes.</remarks>
        /// <param name="KeySizeBits">The size of the cryptographic key, in bits. Supported values are 256, 384, or 521.</param>
        /// <param name="DigestSizeBytes">The size of the digest, in bytes. Supported values are 32, 48, or 64, corresponding to the key size.</param>
        /// <exception cref="NotSupportedException">Thrown if the combination of key size and digest size is not supported.</exception>
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

        /// <summary>
        /// Validates that the specified key size and hash algorithm combination is supported for cryptographic
        /// operations.
        /// </summary>
        /// <remarks>This method ensures that only valid key size and hash algorithm pairs are used,
        /// preventing unsupported or insecure configurations.</remarks>
        /// <param name="KeySizeBits">The size of the cryptographic key, in bits. Supported values are 256, 384, or 521.</param>
        /// <param name="HashAlgorithmName">The hash algorithm to validate for use with the specified key size. Supported values are SHA256 for 256
        /// bits, SHA384 for 384 bits, and SHA512 for 521 bits.</param>
        /// <exception cref="NotSupportedException">Thrown when the specified key size and hash algorithm combination is not supported.</exception>
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

        /// <summary>
        /// Throws an exception if the current object has been disposed.
        /// </summary>
        /// <remarks>Call this method before performing operations that require the object to be in a
        /// valid, non-disposed state. This helps ensure that methods are not called on an object that has already been
        /// disposed, which could lead to undefined behavior.</remarks>
        void CheckDisposed()
        {
            ObjectDisposedException.ThrowIf(this.Disposed, this);
        }

        /// <summary>
        /// Releases the unmanaged resources used by the object and optionally releases the managed resources.
        /// </summary>
        /// <remarks>This method is called by the public Dispose() method and the finalizer. When
        /// Disposing is true, this method disposes of managed resources in addition to unmanaged resources. Override
        /// this method to release resources specific to the derived class.</remarks>
        /// <param name="Disposing">true to release both managed and unmanaged resources; false to release only unmanaged resources.</param>
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
