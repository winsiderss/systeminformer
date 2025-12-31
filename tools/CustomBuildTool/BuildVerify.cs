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
        public static readonly SortedDictionary<string, KeyValuePair<string, string>> KeyName_Vars = new(StringComparer.OrdinalIgnoreCase)
        {
            { "canary",    new("CANARY_BUILD_KEY", "CANARY_BUILD_S") },
            { "developer", new("DEVELOPER_BUILD_KEY", "DEVELOPER_BUILD_S") },
            { "kph",       new("KPH_BUILD_KEY", "KPH_BUILD_S")},
            { "preview",   new("PREVIEW_BUILD_KEY", "PREVIEW_BUILD_S")  },
            { "release",   new("RELEASE_BUILD_KEY", "RELEASE_BUILD_S")},
        };

        /// <summary>
        /// Encrypts the specified file using the provided secret and salt, and writes the result to the output file.
        /// </summary>
        /// <param name="FileName">The path to the input file to encrypt.</param>
        /// <param name="OutFileName">The path to the output encrypted file.</param>
        /// <param name="Secret">The secret used for encryption.</param>
        /// <param name="Salt">The salt value or key name for encryption.</param>
        /// <returns>True if encryption succeeds; otherwise, false.</returns>
        public static bool EncryptFile(string FileName, string OutFileName, string Secret, string Salt)
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
                    var encryptedBytes = Encrypt(fileStream, Secret, GetSalt(Salt));

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
        /// <returns>True if decryption succeeds; otherwise, false.</returns>
        public static bool DecryptFile(string FileName, string OutFileName, string Secret, string Salt)
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
                    var decryptedBytes = Decrypt(fileStream, Secret, GetSalt(Salt));

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

        //private static void DecryptFile(string FileName, string OutFileName, string Secret)
        //{
        //    try
        //    {
        //        using (FileStream fileReadStream = new FileStream(FileName, FileMode.Open))
        //        using (FileStream fileWriteStream = new FileStream(OutFileName, FileMode.Create))
        //        {
        //            using (var rijndael = GetRijndael(Secret))
        //            using (var cryptoDecrypt = rijndael.CreateDecryptor())
        //            using (var cryptoStream = new CryptoStream(fileWriteStream, cryptoDecrypt, CryptoStreamMode.Write))
        //            {
        //                fileReadStream.CopyTo(cryptoStream);
        //                cryptoStream.FlushFinalBlock();
        //            }
        //        }
        //    }
        //    catch (Exception e)
        //    {
        //        Program.PrintColorMessage($"[DecryptFileStream-Exception]: {e.Message}", ConsoleColor.Red);
        //    }
        //}

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

        private static byte[] Encrypt(Stream Stream, string Secret, string Salt)
        {
            try
            {
                using (var rijndael = GetRijndael(Secret, GetSalt(Salt)))
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
        /// <returns>The decrypted byte array, or null if decryption fails.</returns>
        private static byte[] Decrypt(Stream Stream, string Secret, string Salt)
        {
            try
            {
                using (var rijndael = GetRijndael(Secret, GetSalt(Salt)))
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
        /// <returns>The decrypted byte array.</returns>
        private static byte[] Decrypt(byte[] Bytes, string Secret, string Salt)
        {
            using (var blobStream = new MemoryStream(Bytes))
            {
                return Decrypt(blobStream, Secret, Salt);
            }
        }

        /// <summary>
        /// Encrypts the provided byte array using the specified secret and salt.
        /// </summary>
        /// <param name="Bytes">The input byte array to encrypt.</param>
        /// <param name="Secret">The secret used for encryption.</param>
        /// <param name="Salt">The salt value for encryption.</param>
        /// <returns>The encrypted byte array.</returns>
        private static byte[] Encrypt(byte[] Bytes, string Secret, string Salt)
        {
            using (var blobStream = new MemoryStream(Bytes))
            {
                return Encrypt(blobStream, Secret, Salt);
            }
        }

        /// <summary>
        /// Creates and configures an AES instance using the provided secret and salt.
        /// </summary>
        /// <param name="Secret">The secret used for key derivation.</param>
        /// <param name="Salt">The salt value for key derivation.</param>
        /// <returns>An AES instance configured with the derived key and IV.</returns>
        private static Aes GetRijndael(string Secret, string Salt)
        {
            using (Rfc2898DeriveBytes rfc2898DeriveBytes = new Rfc2898DeriveBytes(
                Secret,
                Convert.FromBase64String(GetSalt(Salt)),
                10000,
                HashAlgorithmName.SHA512
                ))
            {
                Aes rijndael = Aes.Create();

                rijndael.Key = rfc2898DeriveBytes.GetBytes(32);
                rijndael.IV = rfc2898DeriveBytes.GetBytes(16);

                return rijndael;
            }
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
        /// Prints public key information for the provided key blob and format.
        /// </summary>
        /// <param name="KeyBlob">The key blob containing the public key.</param>
        /// <param name="KeyFormat">The format of the key blob.</param>
        public static void PrintCngPublicKeyInfo(byte[] KeyBlob, CngKeyBlobFormat KeyFormat)
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
            if (KeyName_Vars.TryGetValue(KeyNameOrSalt, out var vars) &&
                Win32.GetEnvironmentVariable(vars.Value, out string salt))
            {
                return salt;
            }

            // Assume this is the salt itself...
            return KeyNameOrSalt;
        }

        /// <summary>
        /// Retrieves the key material for the specified key name.
        /// </summary>
        /// <param name="KeyName">The key name to retrieve material for.</param>
        /// <param name="KeyMaterial">The output key material byte array.</param>
        /// <returns>True if key material is found; otherwise, false.</returns>
        private static bool GetKeyMaterial(string KeyName, out byte[] KeyMaterial)
        {
            if (Win32.GetEnvironmentVariable(KeyName_Vars[KeyName].Key, out string secret))
            {
                byte[] bytes = Utils.ReadAllBytes(GetPath($"{KeyName}.s"));
                KeyMaterial = Decrypt(bytes, secret, GetSalt(KeyName));
                return true;
            }
            else if (File.Exists(GetPath($"{KeyName}.key")))
            {
                KeyMaterial = Utils.ReadAllBytes(GetPath($"{KeyName}.key"));
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
