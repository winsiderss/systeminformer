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
    /// Provides cryptographic and signature verification utilities for build artifacts.
    /// </summary>
    public static class BuildVerify
    {
        /// <summary>
        /// Maps build key names to their corresponding environment variable names for key and salt.
        /// </summary>
        public static readonly FrozenDictionary<string, BuildVerifyKeyPair> KeyName_Vars = new Dictionary<string, BuildVerifyKeyPair>(StringComparer.OrdinalIgnoreCase)
        {
            { "canary",    new("CANARY_BUILD_KEY", "CANARY_BUILD_S", "CANARY_BUILD_I") },
            { "developer", new("DEVELOPER_BUILD_KEY", "DEVELOPER_BUILD_S", "DEVELOPER_BUILD_I") },
            { "kph",       new("KPH_BUILD_KEY", "KPH_BUILD_S", "KPH_BUILD_I") },
            { "preview",   new("PREVIEW_BUILD_KEY", "PREVIEW_BUILD_S", "PREVIEW_BUILD_I") },
            { "release",   new("RELEASE_BUILD_KEY", "RELEASE_BUILD_S", "RELEASE_BUILD_I") },
        }.ToFrozenDictionary(StringComparer.OrdinalIgnoreCase);
        public record BuildVerifyKeyPair(string Key, string Salt, string Iterations);

        /// <summary>
        /// Encrypts the specified file using the provided secret and salt, and writes the result to the output file.
        /// </summary>
        /// <param name="FileName">The path to the input file to encrypt.</param>
        /// <param name="OutFileName">The path to the output encrypted file.</param>
        /// <param name="Secret">The secret used for encryption.</param>
        /// <param name="Salt">The salt value or key name for encryption.</param>
        /// <param name="Iterations">The iterations value or key name for encryption.</param>
        /// <returns>True if encryption succeeds; otherwise, false.</returns>
        public static bool EncryptFile(string FileName, string OutFileName, ReadOnlySpan<char> Secret, string Salt, string Iterations)
        {
            if (string.IsNullOrWhiteSpace(FileName) || string.IsNullOrWhiteSpace(OutFileName) || Secret.IsEmpty || string.IsNullOrWhiteSpace(Salt))
            {
                Program.PrintColorMessage($"Unable to encrypt file: Invalid arguments.", ConsoleColor.Yellow);
                return false;
            }

            try
            {
                using (var fileStream = File.OpenRead(FileName))
                {
                    var encryptedBytes = Encrypt(fileStream, Secret, GetSalt(Salt), GetIterations(Iterations));

                    Utils.WriteAllBytes(OutFileName, encryptedBytes);
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"Unable to encrypt file {Path.GetFileName(FileName)}: {e.Message}", ConsoleColor.Yellow);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Decrypts the specified file using the provided secret and salt, and writes the result to the output file.
        /// </summary>
        /// <param name="FileName">The path to the input encrypted file.</param>
        /// <param name="OutFileName">The path to the output decrypted file.</param>
        /// <param name="Secret">The secret used for decryption.</param>
        /// <param name="Salt">The salt value or key name for decryption.</param>
        /// <param name="Iterations">The iterations value or key name for encryption.</param>
        /// <returns>True if decryption succeeds; otherwise, false.</returns>
        public static bool DecryptFile(string FileName, string OutFileName, ReadOnlySpan<char> Secret, string Salt, string Iterations)
        {
            if (string.IsNullOrWhiteSpace(FileName) || string.IsNullOrWhiteSpace(OutFileName) || Secret.IsEmpty || string.IsNullOrWhiteSpace(Salt))
            {
                Program.PrintColorMessage($"Unable to decrypt file: Invalid arguments.", ConsoleColor.Yellow);
                return false;
            }

            try
            {
                using (var fileStream = File.OpenRead(FileName))
                {
                    var decryptedBytes = Decrypt(fileStream, Secret, GetSalt(Salt), GetIterations(Iterations));

                    Utils.WriteAllBytes(OutFileName, decryptedBytes);
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"Unable to decrypt file {Path.GetFileName(FileName)}: {e.Message}", ConsoleColor.Yellow);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Computes the SHA256 hash of the specified file and returns it as a hexadecimal string.
        /// </summary>
        /// <param name="FileName">The path to the file to hash.</param>
        /// <returns>The hexadecimal string of the file's hash, or an empty string if hashing fails.</returns>
        public static string HashFile(string FileName)
        {
            try
            {
                using (var fileStream = File.OpenRead(FileName))
                {
                    byte[] fileHash = SHA256.HashData(fileStream);

                    return Convert.ToHexString(fileHash);
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"Unable to hash file {Path.GetFileName(FileName)}: {e.Message}", ConsoleColor.Yellow);
                return null;
            }
        }

        /// <summary>
        /// Computes the SHA-256 hash of the specified byte array.
        /// </summary>
        /// <param name="Source">The byte array to hash.</param>
        /// <returns>A hexadecimal string representation of the computed hash, or null if an error occurs.</returns>
        public static string HashData(byte[] Source)
        {
            try
            {
                byte[] fileHash = SHA256.HashData(Source);

                return Convert.ToHexString(fileHash);
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"Unable to hash buffer: {e.Message}", ConsoleColor.Yellow);
                return null;
            }
        }

        /// <summary>
        /// Creates a signature file for the specified file using the given key name.
        /// </summary>
        /// <param name="KeyName">The key name to use for signing.</param>
        /// <param name="FileName">The path to the file to sign.</param>
        /// <param name="StrictChecks">If true, performs strict checks before creating the signature file.</param>
        /// <returns>True if the signature file is created; otherwise, false.</returns>
        public static bool CreateSigFile(string KeyName, string FileName, bool StrictChecks)
        {
            if (string.IsNullOrWhiteSpace(KeyName))
            {
                Program.PrintErrorMessage("CreateSigFile: KeyName is empty.");
                return false;
            }

            if (string.IsNullOrWhiteSpace(FileName))
            {
                Program.PrintErrorMessage("CreateSigFile: FileName is empty.");
                return false;
            }

            try
            {
                if (!File.Exists(FileName))
                {
                    Program.PrintErrorMessage($"File missing: {FileName}");
                    return false;
                }

                string sigFileName = Path.ChangeExtension(FileName, ".sig");

                if (!GetKeyMaterial(KeyName, out byte[] keyMaterial))
                {
                    if (StrictChecks)
                    {
                        Program.PrintColorMessage($"[GetKeyMaterial failed] ({KeyName})", ConsoleColor.Red);
                        return false;
                    }
                    return true;
                }

                try
                {
                    if (string.Equals(KeyName, "kph", StringComparison.OrdinalIgnoreCase))
                    {
                        string reason = KphKey.GetPrivateContainerError(keyMaterial);

                        if (reason != null)
                        {
                            if (StrictChecks)
                            {
                                Program.PrintColorMessage($"[Invalid kph key] ({KeyName}): {reason}", ConsoleColor.Red);
                                return false;
                            }

                            Program.PrintColorMessage($"[Invalid kph key, nothing signed] ({KeyName}): {reason}", ConsoleColor.Yellow);
                            return true;
                        }
                    }

                    if (StrictChecks)
                    {
                        if (File.Exists(sigFileName) && Win32.GetFileSize(sigFileName) != 0)
                        {
                            Program.PrintColorMessage($"[Signature File Exists] ({sigFileName})", ConsoleColor.Red);
                            return false;
                        }
                    }

                    byte[] signature = string.Equals(KeyName, "kph", StringComparison.OrdinalIgnoreCase)
                        ? SignKphFile(keyMaterial, FileName)
                        : SignFile(keyMaterial, FileName);

                    Utils.WriteAllBytes(sigFileName, signature);
                }
                finally
                {
                    CryptographicOperations.ZeroMemory(keyMaterial);
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"Unable to create signature file {Path.GetFileName(FileName)}: {e}", ConsoleColor.Yellow);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Checks that a kph signature file exists with the hybrid signature length.
        /// </summary>
        /// <param name="SigFileName">The path to the signature file.</param>
        /// <returns>True if the file exists and has the expected length; otherwise, false.</returns>
        public static bool IsValidKphSigFile(string SigFileName)
        {
            return File.Exists(SigFileName) && Win32.GetFileSize(SigFileName) == (ulong)KphKey.SignatureLength;
        }

        /// <summary>
        /// Creates a signature string for the specified file using the given key name.
        /// </summary>
        /// <param name="KeyName">The key name to use for signing.</param>
        /// <param name="FileName">The path to the file to sign.</param>
        /// <returns>The hexadecimal string of the signature, or null if signing fails.</returns>
        public static string CreateSigString(string KeyName, string FileName)
        {
            if (!File.Exists(FileName))
                return null;

            try
            {
                if (GetKeyMaterial(KeyName, out byte[] keyMaterial))
                {
                    try
                    {
                        byte[] signature = SignFile(keyMaterial, FileName);

                        if (signature.Length != 0)
                        {
                            return Convert.ToHexString(signature);
                        }
                    }
                    finally
                    {
                        CryptographicOperations.ZeroMemory(keyMaterial);
                    }
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"Unable to create signature string {Path.GetFileName(FileName)}: {e}", ConsoleColor.Yellow);
            }

            return null;
        }

        /// <summary>
        /// Encrypts the provided stream using the specified secret and salt.
        /// </summary>
        /// <param name="Stream">The input stream to encrypt.</param>
        /// <param name="Secret">The secret used for encryption.</param>
        /// <param name="Salt">The salt value for encryption.</param>
        /// <param name="Iterations">The iterations value for encryption.</param>
        /// <returns>The encrypted byte array, or null if encryption fails.</returns>
        private static byte[] Encrypt(Stream Stream, ReadOnlySpan<char> Secret, string Salt, int Iterations)
        {
            try
            {
                using (var aes = GetRijndael(Secret, Salt, Iterations))
                using (var encryptor = aes.CreateEncryptor())
                {
                    int capacity = Stream.CanSeek ? aes.GetCiphertextLengthCbc((int)Stream.Length, PaddingMode.PKCS7) : 4096;
                    using (var ms = new MemoryStream(capacity))
                    {
                        using (var cryptoStream = new CryptoStream(ms, encryptor, CryptoStreamMode.Write, leaveOpen: true))
                        {
                            Stream.CopyTo(cryptoStream);
                            cryptoStream.FlushFinalBlock();
                        }
                        return ms.ToArray();
                    }
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"[Encrypt-Exception]: {e.Message}", ConsoleColor.Red);
            }

            return null;
        }

        /// <summary>
        /// Decrypts data from the specified stream using AES with a key derived from the provided secret, salt, and
        /// iteration count.
        /// </summary>
        /// <param name="Stream">The stream containing the encrypted data.</param>
        /// <param name="Secret">The secret used to derive the decryption key.</param>
        /// <param name="Salt">The salt used in key derivation.</param>
        /// <param name="Iterations">The number of iterations for the key derivation function.</param>
        /// <returns>A byte array containing the decrypted data, or null if decryption fails.</returns>
        private static byte[] Decrypt(Stream Stream, ReadOnlySpan<char> Secret, string Salt, int Iterations)
        {
            try
            {
                using (var aes = GetRijndael(Secret, Salt, Iterations))
                using (var decryptor = aes.CreateDecryptor())
                {
                    int capacity = Stream.CanSeek ? (int)Stream.Length : 4096;
                    using (var ms = new MemoryStream(capacity))
                    {
                        using (var cryptoStream = new CryptoStream(ms, decryptor, CryptoStreamMode.Write, leaveOpen: true))
                        {
                            Stream.CopyTo(cryptoStream);
                            cryptoStream.FlushFinalBlock();
                        }
                        return ms.ToArray();
                    }
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"[Decrypt-Exception]: {e.Message}", ConsoleColor.Red);
            }

            return null;
        }

        /// <summary>
        /// Decrypts a byte array using AES in CBC mode with a key derived from the specified secret, salt, and
        /// iteration count.
        /// </summary>
        /// <param name="Bytes">The encrypted data to decrypt.</param>
        /// <param name="Secret">The secret used to derive the decryption key.</param>
        /// <param name="Salt">The salt used in key derivation.</param>
        /// <param name="Iterations">The number of iterations for the key derivation function.</param>
        /// <returns>The decrypted byte array, or null if decryption fails.</returns>
        private static byte[] Decrypt(byte[] Bytes, ReadOnlySpan<char> Secret, string Salt, int Iterations)
        {
            try
            {
                using (var aes = GetRijndael(Secret, Salt, Iterations))
                {
                    return aes.DecryptCbc(Bytes, aes.IV, PaddingMode.PKCS7);
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"[Decrypt-Exception]: {e.Message}", ConsoleColor.Red);
            }

            return null;
        }

        private static byte[] Encrypt(byte[] Bytes, ReadOnlySpan<char> Secret, string Salt, int Iterations)
        {
            try
            {
                using (var aes = GetRijndael(Secret, Salt, Iterations))
                {
                    return aes.EncryptCbc(Bytes, aes.IV, PaddingMode.PKCS7);
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"[Encrypt-Exception]: {e.Message}", ConsoleColor.Red);
            }

            return null;
        }

        private static Aes GetRijndael(ReadOnlySpan<char> Secret, string Salt, int Iterations)
        {
            ReadOnlySpan<byte> saltBytes = Convert.FromBase64String(Salt);
            Span<byte> keyMaterial = stackalloc byte[48];

            Rfc2898DeriveBytes.Pbkdf2(
                Secret,
                saltBytes,
                keyMaterial,
                Iterations,
                HashAlgorithmName.SHA512
                );

            Aes aes = Aes.Create();
            aes.Key = keyMaterial[..32].ToArray();
            aes.IV = keyMaterial[32..48].ToArray();

            CryptographicOperations.ZeroMemory(keyMaterial);

            return aes;
        }

        /// <summary>
        /// Signs the provided byte array using the specified key material.
        /// </summary>
        /// <param name="KeyMaterial">The private key material for signing.</param>
        /// <param name="Bytes">The data to sign.</param>
        /// <returns>The signature byte array.</returns>
        private static byte[] Sign(byte[] KeyMaterial, byte[] Bytes)
        {
            byte[] buffer;

            using (CngKey cngkey = CngKey.Import(KeyMaterial, CngKeyBlobFormat.GenericPrivateBlob))
            {
                if (cngkey.Algorithm == CngAlgorithm.ECDsaP256)
                {
                    using (ECDsaCng ecdsa = new ECDsaCng(cngkey))
                    {
                        buffer = ecdsa.SignData(Bytes, HashAlgorithmName.SHA256);
                    }
                }
                else if (cngkey.Algorithm == CngAlgorithm.Rsa)
                {
                    using (RSACng rsa = new RSACng(cngkey))
                    {
                        buffer = rsa.SignData(Bytes, HashAlgorithmName.SHA512, RSASignaturePadding.Pss);
                    }
                }
                else
                {
                    throw new NotSupportedException($"Unsupported algorithm: {cngkey.Algorithm}");
                }
            }

            return buffer;
        }

        /// <summary>
        /// Signs the contents of the specified file using the provided key material.
        /// </summary>
        /// <param name="KeyMaterial">The private key material for signing.</param>
        /// <param name="FileName">The path to the file to sign.</param>
        /// <returns>The signature byte array.</returns>
        private static byte[] SignFile(byte[] KeyMaterial, string FileName)
        {
            byte[] buffer;

            using (FileStream fileStream = File.OpenRead(FileName))
            using (CngKey cngkey = CngKey.Import(KeyMaterial, CngKeyBlobFormat.GenericPrivateBlob))
            {
                if (cngkey.Algorithm == CngAlgorithm.ECDsaP256)
                {
                    using (ECDsaCng ecdsa = new ECDsaCng(cngkey))
                    {
                        buffer = ecdsa.SignData(fileStream, HashAlgorithmName.SHA256);
                    }
                }
                else if (cngkey.Algorithm == CngAlgorithm.Rsa)
                {
                    using (RSACng rsa = new RSACng(cngkey))
                    {
                        buffer = rsa.SignData(fileStream, HashAlgorithmName.SHA512, RSASignaturePadding.Pss);
                    }
                }
                else
                {
                    throw new NotSupportedException($"Unsupported algorithm: {cngkey.Algorithm}");
                }
            }

            return buffer;
        }

        /// <summary>
        /// Layout of the hybrid kph key containers and signature.
        /// </summary>
        private static class KphKey
        {
            public const int RsaPublicLength = 539, MlDsaPublicLength = 2592, PublicLength = 3131;
            public const int RsaPrivateLength = 1051, MlDsaPrivateLength = 4896, PrivateLength = 5947;
            public const int RsaSignatureLength = 512, MlDsaSignatureLength = 4627, SignatureLength = 5139;
            public const string Sha512Oid = "2.16.840.1.101.3.4.2.3";

            public static (byte[] Rsa, byte[] MlDsa) SplitPrivate(ReadOnlySpan<byte> Container)
            {
                return Split(Container, PrivateLength, RsaPrivateLength, true);
            }

            public static (byte[] Rsa, byte[] MlDsa) SplitPublic(ReadOnlySpan<byte> Container)
            {
                return Split(Container, PublicLength, RsaPublicLength, false);
            }

            public static string GetPrivateContainerError(ReadOnlySpan<byte> Container)
            {
                return GetContainerError(Container, PrivateLength, RsaPrivateLength, true);
            }

            private static string GetContainerError(ReadOnlySpan<byte> Container, int Total, int RsaLength, bool Private)
            {
                string kind = Private ? "private" : "public";

                if (Container.Length != Total)
                    return $"kph {kind} key container length {Container.Length}, expected {Total}.";

                if (!ValidateRsaBlobHeader(Container[..RsaLength], Private))
                    return $"kph {kind} key RSA blob header is invalid.";

                return null;
            }

            private static (byte[] Rsa, byte[] MlDsa) Split(ReadOnlySpan<byte> Container, int Total, int RsaLength, bool Private)
            {
                string error = GetContainerError(Container, Total, RsaLength, Private);

                if (error != null)
                    throw new System.Security.Cryptography.CryptographicException(error);

                return (Container[..RsaLength].ToArray(), Container[RsaLength..].ToArray());
            }

            public static bool ValidateRsaBlobHeader(ReadOnlySpan<byte> Blob, bool Private)
            {
                if (Blob.Length != (Private ? RsaPrivateLength : RsaPublicLength))
                    return false;

                uint magic = BinaryPrimitives.ReadUInt32LittleEndian(Blob);
                uint bitLength = BinaryPrimitives.ReadUInt32LittleEndian(Blob[4..]);
                uint cbPublicExp = BinaryPrimitives.ReadUInt32LittleEndian(Blob[8..]);
                uint cbModulus = BinaryPrimitives.ReadUInt32LittleEndian(Blob[12..]);
                uint cbPrime1 = BinaryPrimitives.ReadUInt32LittleEndian(Blob[16..]);
                uint cbPrime2 = BinaryPrimitives.ReadUInt32LittleEndian(Blob[20..]);

                if (magic != (Private ? 0x32415352u : 0x31415352u))
                    return false;
                if (bitLength != 4096 || cbPublicExp != 3 || cbModulus != 512)
                    return false;
                if (Private && (cbPrime1 != 256 || cbPrime2 != 256))
                    return false;

                return true;
            }
        }

        /// <summary>
        /// Signs a SHA-512 digest with the RSA-PSS and HashML-DSA-87 halves of a kph private container.
        /// </summary>
        private static byte[] SignKphDigest(byte[] PrivateContainer, ReadOnlySpan<byte> Sha512Digest)
        {
            if (!MLDsa.IsSupported)
                throw new PlatformNotSupportedException("kph signing requires Windows 11 24H2 or Server 2025 (ML-DSA in CNG)");

            (byte[] rsaKey, byte[] mldsaKey) = KphKey.SplitPrivate(PrivateContainer);

            try
            {
                byte[] signature = new byte[KphKey.SignatureLength];
                byte[] rsaPublic;
                byte[] mldsaPublic;

                using (CngKey cngkey = CngKey.Import(rsaKey, CngKeyBlobFormat.GenericPrivateBlob))
                {
                    if (cngkey.Algorithm != CngAlgorithm.Rsa)
                        throw new System.Security.Cryptography.CryptographicException("kph private key is not RSA.");

                    using (RSACng rsa = new RSACng(cngkey))
                    {
                        byte[] rsaSignature = rsa.SignHash(Sha512Digest.ToArray(), HashAlgorithmName.SHA512, RSASignaturePadding.Pss);

                        if (rsaSignature.Length != KphKey.RsaSignatureLength)
                            throw new System.Security.Cryptography.CryptographicException($"RSA signature length {rsaSignature.Length}, expected {KphKey.RsaSignatureLength}.");

                        rsaSignature.CopyTo(signature, 0);
                    }

                    rsaPublic = cngkey.Export(CngKeyBlobFormat.GenericPublicBlob);
                }

                using (MLDsa mldsa = MLDsa.ImportMLDsaPrivateKey(MLDsaAlgorithm.MLDsa87, mldsaKey))
                {
                    Span<byte> mldsaSignature = signature.AsSpan(KphKey.RsaSignatureLength);

                    mldsa.SignPreHash(Sha512Digest, mldsaSignature, KphKey.Sha512Oid, default);

                    mldsaPublic = mldsa.ExportMLDsaPublicKey();
                }

                byte[] publicContainer = new byte[KphKey.PublicLength];
                rsaPublic.CopyTo(publicContainer, 0);
                mldsaPublic.CopyTo(publicContainer, KphKey.RsaPublicLength);

                if (!VerifyKphDigest(publicContainer, Sha512Digest, signature))
                    throw new System.Security.Cryptography.CryptographicException("kph signature failed post-sign verification.");

                return signature;
            }
            finally
            {
                CryptographicOperations.ZeroMemory(rsaKey);
                CryptographicOperations.ZeroMemory(mldsaKey);
            }
        }

        /// <summary>
        /// Verifies both halves of a kph signature over a SHA-512 digest.
        /// </summary>
        private static bool VerifyKphDigest(byte[] PublicContainer, ReadOnlySpan<byte> Sha512Digest, ReadOnlySpan<byte> Signature)
        {
            if (!MLDsa.IsSupported)
                throw new PlatformNotSupportedException("kph verification requires Windows 11 24H2 or Server 2025 (ML-DSA in CNG)");

            if (Signature.Length != KphKey.SignatureLength || Sha512Digest.Length != SHA512.HashSizeInBytes)
                return false;

            (byte[] rsaKey, byte[] mldsaKey) = KphKey.SplitPublic(PublicContainer);

            bool rsaOk;

            using (CngKey cngkey = CngKey.Import(rsaKey, CngKeyBlobFormat.GenericPublicBlob))
            using (RSACng rsa = new RSACng(cngkey))
            {
                rsaOk = rsa.VerifyHash(Sha512Digest, Signature[..KphKey.RsaSignatureLength], HashAlgorithmName.SHA512, RSASignaturePadding.Pss);
            }

            bool mldsaOk;

            using (MLDsa mldsa = MLDsa.ImportMLDsaPublicKey(MLDsaAlgorithm.MLDsa87, mldsaKey))
            {
                mldsaOk = mldsa.VerifyPreHash(Sha512Digest, Signature[KphKey.RsaSignatureLength..], KphKey.Sha512Oid, default);
            }

            return rsaOk & mldsaOk;
        }

        /// <summary>
        /// Signs a file with the hybrid kph key container; the result is exactly 5139 bytes.
        /// </summary>
        private static byte[] SignKphFile(byte[] KeyMaterial, string FileName)
        {
            byte[] digest;

            using (FileStream fileStream = File.OpenRead(FileName))
            {
                digest = SHA512.HashData(fileStream);
            }

            return SignKphDigest(KeyMaterial, digest);
        }

        /// <summary>
        /// Attempts to extract the algorithm OID from a SubjectPublicKeyInfo ASN.1 blob.
        /// Returns "RSA", "ECDSA", or null if unknown.
        /// </summary>
        private static CngAlgorithmGroup GetPublicKeyAlgorithmOid(byte[] KeyBlob)
        {
            try
            {
                var reader = new System.Formats.Asn1.AsnReader(KeyBlob, System.Formats.Asn1.AsnEncodingRules.DER);
                var seq = reader.ReadSequence(); // SubjectPublicKeyInfo SEQUENCE
                var algId = seq.ReadSequence(); // AlgorithmIdentifier SEQUENCE
                var oid = algId.ReadObjectIdentifier();

                // Common OIDs:
                // 1.2.840.10045.2.1 = ecPublicKey
                // 1.2.840.113549.1.1.1 = rsaEncryption

                if (string.Equals(oid, "1.2.840.10045.2.1", StringComparison.OrdinalIgnoreCase))
                    return CngAlgorithmGroup.ECDsa;
                if (string.Equals(oid, "1.2.840.113549.1.1.1", StringComparison.OrdinalIgnoreCase))
                    return CngAlgorithmGroup.Rsa;
            }
            catch
            {
                // Not a valid SubjectPublicKeyInfo or unknown format
            }

            return null;
        }

        /// <summary>
        /// Prints public key information for the provided key blob and format.
        /// </summary>
        /// <param name="KeyBlob">The key blob containing the public key.</param>
        /// <param name="KeyFormat">The format of the key blob.</param>
        public static void PrintCngPublicKeyInfo(byte[] KeyBlob, CngKeyBlobFormat KeyFormat)
        {
            // Try to detect if the blob is SubjectPublicKeyInfo (ASN.1 SEQUENCE starts with 0x30)
            bool isSubjectPublicKeyInfo = KeyBlob.Length > 0 && KeyBlob[0] == 0x30;

            if (isSubjectPublicKeyInfo)
            {
                var type = GetPublicKeyAlgorithmOid(KeyBlob);

                if (type == CngAlgorithmGroup.ECDsa)
                {
                    using (ECDsa ecdsa = ECDsa.Create())
                    {
                        ecdsa.ImportSubjectPublicKeyInfo(KeyBlob, out _);
                        Program.PrintColorMessage("ECDSA Public Key (SubjectPublicKeyInfo)\n", ConsoleColor.Cyan);
                        Program.PrintColorMessage($"{ecdsa.ExportSubjectPublicKeyInfoPem()}\n", ConsoleColor.White);
                    }
                }
                else if (type == CngAlgorithmGroup.Rsa)
                {
                    using (RSA rsa = RSA.Create())
                    {
                        rsa.ImportSubjectPublicKeyInfo(KeyBlob, out _);
                        Program.PrintColorMessage("RSA Public Key (SubjectPublicKeyInfo)\n", ConsoleColor.Cyan);
                        Program.PrintColorMessage($"{rsa.ExportSubjectPublicKeyInfoPem()}\n", ConsoleColor.White);
                        Program.PrintColorMessage($"{rsa.ExportRSAPublicKeyPem()}\n", ConsoleColor.White);
                    }
                }
                else
                {
                    throw new NotSupportedException($"Unsupported algorithm.");
                }
            }
            else
            {
                using (CngKey cngkey = CngKey.Import(KeyBlob, KeyFormat))
                {
                    if (cngkey.Algorithm == CngAlgorithm.ECDsaP256)
                    {
                        using (ECDsaCng ecdsa = new ECDsaCng(cngkey))
                        {
                            Program.PrintColorMessage($"{ecdsa.ExportSubjectPublicKeyInfoPem()}\n", ConsoleColor.White);
                        }
                    }
                    else if (cngkey.Algorithm == CngAlgorithm.Rsa)
                    {
                        using (RSACng rsa = new RSACng(cngkey))
                        {
                            Program.PrintColorMessage($"{rsa.ExportSubjectPublicKeyInfoPem()}\n", ConsoleColor.White);
                            Program.PrintColorMessage($"{rsa.ExportRSAPublicKeyPem()}\n", ConsoleColor.White);
                        }
                    }
                    else
                    {
                        throw new NotSupportedException($"Unsupported algorithm: {cngkey.Algorithm}");
                    }
                }
            }
        }

        /// <summary>
        /// Gets the full path for the specified file, using environment variables or default locations.
        /// </summary>
        /// <param name="FileName">The file name to resolve.</param>
        /// <returns>The resolved file path.</returns>
        private static string GetPath(string FileName)
        {
            if (Win32.GetEnvironmentVariable("BUILD_DRM", out string value))
            {
                return Path.Join([value, FileName]);
            }

            // N.B. Local developers are instructed to put keys in this path.
            return Path.Join([Build.BuildWorkingFolder, "tools", "CustomSignTool", "Resources", FileName]);
        }

        /// <summary>
        /// Gets the salt value for the specified key name or returns the input if not found.
        /// </summary>
        /// <param name="KeyNameOrSalt">The key name or salt value.</param>
        /// <returns>The salt value.</returns>
        private static string GetSalt(string KeyNameOrSalt)
        {
            if (KeyName_Vars.TryGetValue(KeyNameOrSalt, out var vars))
            {
                if (Win32.GetEnvironmentVariable(vars.Salt, out string salt))
                {
                    return salt;
                }

                string saltFile = GetPath($"{KeyNameOrSalt}.salt");
                if (File.Exists(saltFile))
                {
                    return Utils.ReadFirstLine(saltFile);
                }
            }

            // Assume this is the salt itself...
            return KeyNameOrSalt;
        }

        /// <summary>
        /// Gets the iterations value for the specified key name or returns the input if not found.
        /// </summary>
        /// <param name="KeyNameOrIterations">The key name or salt value.</param>
        /// <returns>The salt value.</returns>
        private static int GetIterations(string KeyNameOrIterations)
        {
            if (KeyName_Vars.TryGetValue(KeyNameOrIterations, out var vars))
            {
                if (Win32.GetEnvironmentVariable(vars.Iterations, out string iterations))
                {
                    return int.Parse(iterations);
                }

                string iterationsFile = GetPath($"{KeyNameOrIterations}.iterations");
                if (File.Exists(iterationsFile))
                {
                    return int.Parse(Utils.ReadFirstLine(iterationsFile));
                }
            }

            // Assume this is the iterations itself...
            return int.Parse(KeyNameOrIterations);
        }

        /// <summary>
        /// Retrieves the key material for the specified key name.
        /// </summary>
        /// <param name="KeyName">The key name to retrieve material for.</param>
        /// <param name="KeyMaterial">The output key material byte array.</param>
        /// <returns>True if key material is found; otherwise, false.</returns>
        private static bool GetKeyMaterial(string KeyName, out byte[] KeyMaterial)
        {
            string fileName = GetPath($"{KeyName}");

            if (File.Exists(fileName))
            {
                using (var secret = Utils.ReadAllTextSecure(fileName))
                {
                    byte[] bytes = Utils.ReadAllBytes(GetPath($"{KeyName}.s"));
                    KeyMaterial = Decrypt(bytes, secret.Span, GetSalt(KeyName), GetIterations(KeyName));
                }
                return true;
            }
            else if (File.Exists(GetPath($"{KeyName}.key")))
            {
                KeyMaterial = Utils.ReadAllBytes(GetPath($"{KeyName}.key"));
                return true;
            }
            else if (Win32.GetEnvironmentVariableSecure(KeyName_Vars[KeyName].Key, out SecureBuffer secret))
            {
                using (secret)
                {
                    byte[] bytes = Utils.ReadAllBytes(GetPath($"{KeyName}.s"));
                    KeyMaterial = Decrypt(bytes, secret.Span, GetSalt(KeyName), GetIterations(KeyName));
                }
                return true;
            }
            else
            {
                KeyMaterial = null;
                return false;
            }
        }

        //private static bool GetKeyMaterial(string KeyName, out byte[] KeyMaterial)
        //{
        //    if (
        //        Win32.GetEnvironmentVariable(KeyName_Vars[KeyName].Key, out string key) &&
        //        Win32.GetEnvironmentVariable(KeyName_Vars[KeyName].Value, out string value) &&
        //        Win32.GetEnvironmentVariable("KPH_BUILD_SALT", out string salt)
        //        )
        //    {
        //        if (Uri.TryCreate(value, UriKind.Absolute, out Uri result) && result.IsFile)
        //        {
        //            using (var fileStream = File.OpenRead(value))
        //            {
        //                KeyMaterial = Decrypt(fileStream, key, salt);
        //            }
        //        }
        //        else
        //        {
        //            byte[] bytes = Utils.ReadAllBytes(value);
        //            KeyMaterial = Decrypt(bytes, key, salt);
        //        }
        //
        //        return true;
        //    }
        //    else if (Win32.GetEnvironmentVariable(KeyName_Vars[KeyName].Key, out string secret))
        //    {
        //        if (Uri.TryCreate(secret, UriKind.Absolute, out Uri result) && result.IsFile)
        //        {
        //            using (var fileStream = File.OpenRead(secret))
        //            {
        //                KeyMaterial = Decrypt(fileStream, secret, GetSalt(null));
        //            }
        //        }
        //        else
        //        {
        //            byte[] bytes = Utils.ReadAllBytes(GetPath($"{KeyName}.s"));
        //            KeyMaterial = Decrypt(bytes, secret, GetSalt(null));
        //        }
        //
        //        return true;
        //    }
        //    else if (File.Exists(GetPath($"{KeyName}.key")))
        //    {
        //        KeyMaterial = Utils.ReadAllBytes(GetPath($"{KeyName}.key"));
        //        return true;
        //    }
        //    else
        //    {
        //        KeyMaterial = null;
        //        return false;
        //    }
        //}
    }
}
