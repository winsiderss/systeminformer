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
    public static class Verify
    {
        public static readonly SortedDictionary<string, KeyValuePair<string, string>> KeyName_Vars = new(StringComparer.OrdinalIgnoreCase)
        {
            { "canary",    new("CANARY_BUILD_KEY", "CANARY_BUILD_S") },
            { "developer", new("DEVELOPER_BUILD_KEY", "DEVELOPER_BUILD_S") },
            { "kph",       new("KPH_BUILD_KEY", "KPH_BUILD_S")},
            { "preview",   new("PREVIEW_BUILD_KEY", "PREVIEW_BUILD_S")  },
            { "release",   new("RELEASE_BUILD_KEY", "RELEASE_BUILD_S")},
        };

        public static bool EncryptFile(string FileName, string OutFileName, string Secret, string Salt)
        {
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

        public static bool DecryptFile(string FileName, string OutFileName, string Secret, string Salt)
        {
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

        public static string HashFile(string FileName)
        {
            try
            {
                using (var filetream = File.OpenRead(FileName))
                using (var bufferedStream = new BufferedStream(filetream, 0x1000))
                using (var sha = SHA256.Create())
                {
                    byte[] checksum = sha.ComputeHash(bufferedStream);

                    return Convert.ToHexString(checksum);
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"Unable to hash file {Path.GetFileName(FileName)}: {e.Message}", ConsoleColor.Yellow);
                return string.Empty;
            }
        }

        public static bool CreateSigFile(string KeyName, string FileName, bool StrictChecks)
        {
            try
            {
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

                if (File.Exists(FileName))
                {
                    var signature = Sign(keyMaterial, FileName);
                    Utils.WriteAllBytes(sigFileName, signature);
                }
                else
                {
                    Program.PrintColorMessage($"[Skipped] {FileName}", ConsoleColor.Yellow);
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"Unable to create signature file {Path.GetFileName(FileName)}: {e.Message}", ConsoleColor.Yellow);
                return false;
            }

            return true;
        }

        public static bool CreateSigString(string KeyName, string FileName, out string Signature)
        {
            Signature = null;

            try
            {
                if (GetKeyMaterial(KeyName, out byte[] keyMaterial))
                {
                    if (File.Exists(FileName))
                    {
                        var signature = Sign(keyMaterial, FileName);
                        Signature = Convert.ToHexString(signature);
                    }
                    else
                    {
                        Program.PrintColorMessage($"[Skipped] {FileName}", ConsoleColor.Yellow);
                    }
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"Unable to create signature string {Path.GetFileName(FileName)}: {e}", ConsoleColor.Yellow);
            }

            return !string.IsNullOrWhiteSpace(Signature);
        }

        private static byte[] Encrypt(Stream Stream, string Secret, string Salt)
        {
            byte[] buffer = ArrayPool<byte>.Shared.Rent(0x4000);

            try
            {
                using (MemoryStream memoryStream = new MemoryStream(buffer, 0, 0x4000, true, true))
                {
                    using (var rijndael = GetRijndael(Secret, GetSalt(Salt)))
                    using (var cryptoEncrypt = rijndael.CreateEncryptor())
                    using (var cryptoStream = new CryptoStream(memoryStream, cryptoEncrypt, CryptoStreamMode.Write, true))
                    {
                        Stream.CopyTo(cryptoStream);
                        cryptoStream.FlushFinalBlock();
                    }

                    memoryStream.SetLength(memoryStream.Position);
                    return memoryStream.ToArray();
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"[Encrypt-Exception]: {e.Message}", ConsoleColor.Red);
            }
            finally
            {
                // Zero the buffer (copied from System.Security.Cryptography)
                CryptographicOperations.ZeroMemory(buffer.AsSpan(0, 0x4000));
                ArrayPool<byte>.Shared.Return(buffer);
            }

            return null;
        }

        private static byte[] Decrypt(Stream Stream, string Secret, string Salt)
        {
            byte[] buffer = ArrayPool<byte>.Shared.Rent(0x4000);

            try
            {
                using (MemoryStream memoryStream = new MemoryStream(buffer, 0, 0x4000, true, true))
                {
                    using (var rijndael = GetRijndael(Secret, GetSalt(Salt)))
                    using (var cryptoDecrypt = rijndael.CreateDecryptor())
                    using (var cryptoStream = new CryptoStream(memoryStream, cryptoDecrypt, CryptoStreamMode.Write, true))
                    {
                        Stream.CopyTo(cryptoStream);
                        cryptoStream.FlushFinalBlock();
                    }

                    memoryStream.SetLength(memoryStream.Position);
                    return memoryStream.ToArray();
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"[Decrypt-Exception]: {e.Message}", ConsoleColor.Red);
            }
            finally
            {
                // Zero the buffer (copied from System.Security.Cryptography)
                CryptographicOperations.ZeroMemory(buffer.AsSpan(0, 0x4000));
                ArrayPool<byte>.Shared.Return(buffer);
            }

            return null;
        }

        private static byte[] Decrypt(byte[] Bytes, string Secret, string Salt)
        {
            using (var blobStream = new MemoryStream(Bytes))
            {
                return Decrypt(blobStream, Secret, GetSalt(Salt));
            }
        }

        private static byte[] Encrypt(byte[] Bytes, string Secret, string Salt)
        {
            using (var blobStream = new MemoryStream(Bytes))
            {
                return Encrypt(blobStream, Secret, GetSalt(Salt));
            }
        }

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

        private static byte[] Sign(byte[] KeyMaterial, string FileName)
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

        private static string GetPath(string FileName)
        {
            return Path.Join([Build.BuildWorkingFolder, "\\tools\\CustomSignTool\\Resources\\", FileName]);
        }

        private static string GetSalt(string Salt)
        {
            if (string.IsNullOrWhiteSpace(Salt))
                return "e0U0RTY2RjU5LUNBRjItNEMzOS1BN0Y4LTQ2MDk3QjFDNDYxQn0=";
            return Salt;
        }

        private static bool GetKeyMaterial(string KeyName, out byte[] KeyMaterial)
        {
            if (Win32.GetEnvironmentVariable(KeyName_Vars[KeyName].Key, out string secret))
            {
                byte[] bytes = Utils.ReadAllBytes(GetPath($"{KeyName}.s"));
                KeyMaterial = Decrypt(bytes, secret, GetSalt(null));
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
