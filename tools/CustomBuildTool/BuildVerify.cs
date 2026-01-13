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
        public static readonly SortedDictionary<string, BuildVerifyKeyPair> KeyName_Vars = new(StringComparer.OrdinalIgnoreCase)
        {
            { "canary",    new("CANARY_BUILD_KEY", "CANARY_BUILD_S", "CANARY_BUILD_I") },
            { "developer", new("DEVELOPER_BUILD_KEY", "DEVELOPER_BUILD_S", "DEVELOPER_BUILD_I") },
            { "kph",       new("KPH_BUILD_KEY", "KPH_BUILD_S", "KPH_BUILD_I") },
            { "preview",   new("PREVIEW_BUILD_KEY", "PREVIEW_BUILD_S", "PREVIEW_BUILD_I") },
            { "release",   new("RELEASE_BUILD_KEY", "RELEASE_BUILD_S", "RELEASE_BUILD_I") },
        };
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
        public static bool EncryptFile(string FileName, string OutFileName, string Secret, string Salt, string Iterations)
        {
            if (string.IsNullOrWhiteSpace(FileName) || string.IsNullOrWhiteSpace(OutFileName) || string.IsNullOrWhiteSpace(Secret) || string.IsNullOrWhiteSpace(Salt))
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
        public static bool DecryptFile(string FileName, string OutFileName, string Secret, string Salt, string Iterations)
        {
            if (string.IsNullOrWhiteSpace(FileName) || string.IsNullOrWhiteSpace(OutFileName) || string.IsNullOrWhiteSpace(Secret) || string.IsNullOrWhiteSpace(Salt))
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
                return string.Empty;
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
                Program.PrintColorMessage($"[ERROR] CreateSigFile: KeyName is empty.", ConsoleColor.Red);
                return false;
            }

            if (string.IsNullOrWhiteSpace(FileName))
            {
                Program.PrintColorMessage($"[ERROR] CreateSigFile: FileName is empty.", ConsoleColor.Red);
                return false;
            }

            try
            {
                if (!File.Exists(FileName))
                {
                    Program.PrintColorMessage($"[ERROR] File missing: {FileName}", ConsoleColor.Red);
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

                if (StrictChecks)
                {
                    if (File.Exists(sigFileName) && Win32.GetFileSize(sigFileName) != 0)
                    {
                        Program.PrintColorMessage($"[Signature File Exists] ({sigFileName})", ConsoleColor.Red);
                        return false;
                    }
                }

                byte[] signature = SignFile(keyMaterial, FileName);

                Utils.WriteAllBytes(sigFileName, signature);
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"Unable to create signature file {Path.GetFileName(FileName)}: {e}", ConsoleColor.Yellow);
                return false;
            }

            return true;
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
                    byte[] signature = SignFile(keyMaterial, FileName);

                    if (signature.Length != 0)
                    {
                        return Convert.ToHexString(signature);
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
        private static byte[] Encrypt(Stream Stream, string Secret, string Salt, int Iterations)
        {
            try
            {
                using (var rijndael = GetRijndael(Secret, Salt, Iterations))
                using (var cryptoEncrypt = rijndael.CreateEncryptor())
                using (var ms = new MemoryStream())
                {
                    using (var cryptoStreamOut = new CryptoStream(ms, cryptoEncrypt, CryptoStreamMode.Write, leaveOpen: true))
                    {
                        Stream.CopyTo(cryptoStreamOut);
                        cryptoStreamOut.FlushFinalBlock();
                    }
                    return ms.ToArray();
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"[Decrypt-Exception]: {e.Message}", ConsoleColor.Red);
            }

            return null;
        }

        /// <summary>
        /// Decrypts the provided stream using the specified secret and salt.
        /// </summary>
        /// <param name="Stream">The input stream to decrypt.</param>
        /// <param name="Secret">The secret used for decryption.</param>
        /// <param name="Salt">The salt value for decryption.</param>
        /// <param name="Iterations">The iterations value for decryption.</param>
        /// <returns>The decrypted byte array, or null if decryption fails.</returns>
        private static byte[] Decrypt(Stream Stream, string Secret, string Salt, int Iterations)
        {
            try
            {
                using (var rijndael = GetRijndael(Secret, Salt, Iterations))
                using (var cryptoDecrypt = rijndael.CreateDecryptor())
                using (var ms = new MemoryStream())
                {
                    using (var cryptoStream = new CryptoStream(ms, cryptoDecrypt, CryptoStreamMode.Write, leaveOpen: true))
                    {
                        Stream.CopyTo(cryptoStream);
                        cryptoStream.FlushFinalBlock();
                    }
                    return ms.ToArray();
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"[Decrypt-Exception]: {e.Message}", ConsoleColor.Red);
            }

            return null;
        }

        /// <summary>
        /// Decrypts the provided byte array using the specified secret and salt.
        /// </summary>
        /// <param name="Bytes">The encrypted byte array.</param>
        /// <param name="Secret">The secret used for decryption.</param>
        /// <param name="Salt">The salt value for decryption.</param>
        /// <param name="Iterations">The iterations value for decryption.</param>
        /// <returns>The decrypted byte array.</returns>
        private static byte[] Decrypt(byte[] Bytes, string Secret, string Salt, int Iterations)
        {
            using (var blobStream = new MemoryStream(Bytes))
            {
                return Decrypt(blobStream, Secret, Salt, Iterations);
            }
        }

        /// <summary>
        /// Encrypts the provided byte array using the specified secret and salt.
        /// </summary>
        /// <param name="Bytes">The input byte array to encrypt.</param>
        /// <param name="Secret">The secret used for encryption.</param>
        /// <param name="Salt">The salt value for encryption.</param>
        /// <param name="Iterations">The iterations value for encryption.</param>
        /// <returns>The encrypted byte array.</returns>
        private static byte[] Encrypt(byte[] Bytes, string Secret, string Salt, int Iterations)
        {
            using (var blobStream = new MemoryStream(Bytes))
            {
                return Encrypt(blobStream, Secret, Salt, Iterations);
            }
        }

        /// <summary>
        /// Creates and configures an AES instance using the provided secret and salt.
        /// </summary>
        /// <param name="Secret">The secret used for key derivation.</param>
        /// <param name="Salt">The salt value for key derivation.</param>
        /// <param name="Iterations">The iterations value for key derivation.</param>
        /// <returns>An AES instance configured with the derived key and IV.</returns>
        private static Aes GetRijndael(string Secret, string Salt, int Iterations)
        {
            ReadOnlySpan<byte> saltBytes = Convert.FromBase64String(Salt);
            Span<byte> keyMaterial = stackalloc byte[48];

            Rfc2898DeriveBytes.Pbkdf2(
                Secret.AsSpan(),
                saltBytes,
                keyMaterial,
                Iterations,
                HashAlgorithmName.SHA512
                );

            Aes rijndael = Aes.Create();
            rijndael.Key = keyMaterial[..32].ToArray();
            rijndael.IV = keyMaterial[32..48].ToArray();

            return rijndael;
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
        /// Attempts to extract the algorithm OID from a SubjectPublicKeyInfo ASN.1 blob.
        /// Returns "RSA", "ECDSA", or null if unknown.
        /// </summary>
        private static CngAlgorithmGroup GetPublicKeyAlgorithmOid(byte[] keyBlob)
        {
            try
            {
                var reader = new System.Formats.Asn1.AsnReader(keyBlob, System.Formats.Asn1.AsnEncodingRules.DER);
                var seq = reader.ReadSequence(); // SubjectPublicKeyInfo SEQUENCE
                var algId = seq.ReadSequence(); // AlgorithmIdentifier SEQUENCE
                var oid = algId.ReadObjectIdentifier();

                // Common OIDs:
                // 1.2.840.10045.2.1 = ecPublicKey
                // 1.2.840.113549.1.1.1 = rsaEncryption
                if (oid == "1.2.840.10045.2.1")
                    return CngAlgorithmGroup.ECDsa;
                if (oid == "1.2.840.113549.1.1.1")
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
                return Path.Join([value, "\\", FileName]);
            }

            // N.B. Local developers are instructed to put keys in this path.
            return Path.Join([Build.BuildWorkingFolder, "\\tools\\CustomSignTool\\Resources\\", FileName]);
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
                    return File.ReadAllText(saltFile).Trim();
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
                    return int.Parse(File.ReadAllText(iterationsFile).Trim());
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
                string secret = File.ReadAllText(fileName);
                byte[] bytes = Utils.ReadAllBytes(GetPath($"{KeyName}.s"));
                KeyMaterial = Decrypt(bytes, secret, GetSalt(KeyName), GetIterations(KeyName));
                return true;
            }
            else if (File.Exists(GetPath($"{KeyName}.key")))
            {
                KeyMaterial = Utils.ReadAllBytes(GetPath($"{KeyName}.key"));
                return true;
            }
            else if (Win32.GetEnvironmentVariable(KeyName_Vars[KeyName].Key, out string secret))
            {
                byte[] bytes = Utils.ReadAllBytes(GetPath($"{KeyName}.s"));
                KeyMaterial = Decrypt(bytes, secret, GetSalt(KeyName), GetIterations(KeyName));
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
