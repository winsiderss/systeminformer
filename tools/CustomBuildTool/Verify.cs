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
        public static readonly SortedDictionary<string, string> KeyName_Vars = new(StringComparer.OrdinalIgnoreCase)
        {
            { "kph",       "KPH_BUILD_KEY" },
            { "release",   "RELEASE_BUILD_KEY" },
            { "preview",   "PREVIEW_BUILD_KEY" },
            { "canary",    "CANARY_BUILD_KEY" },
            { "developer", "DEVELOPER_BUILD_KEY" },
        };

        public static bool EncryptFile(string FileName, string OutFileName, string Secret)
        {
            try
            {
                var fileBytes = Utils.ReadAllBytes(FileName);
                var encryptedBytes = Encrypt(fileBytes, Secret);
                Utils.WriteAllBytes(OutFileName, encryptedBytes);
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"Unable to encrypt file {Path.GetFileName(FileName)}: {e.Message}", ConsoleColor.Yellow);
                return false;
            }

            return true;
        }

        public static bool DecryptFile(string FileName, string OutFileName, string Secret)
        {
            try
            {
                var fileBytes = Utils.ReadAllBytes(FileName);
                var decryptedBytes = Decrypt(fileBytes, Secret);
                Utils.WriteAllBytes(OutFileName, decryptedBytes);
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"Unable to decrypt file {Path.GetFileName(FileName)}: {e.Message}", ConsoleColor.Yellow);
                return false;
            }

            return true;
        }

        public static string HashFile(string FileName)
        {
            try
            {
                using (FileStream fileInStream = File.OpenRead(FileName))
                using (BufferedStream bufferedStream = new BufferedStream(fileInStream, 0x1000))
                using (SHA256 sha = SHA256.Create())
                {
                    byte[] checksum = sha.ComputeHash(bufferedStream);

                    return BitConverter.ToString(checksum).Replace("-", string.Empty, StringComparison.OrdinalIgnoreCase);
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
                    using (var fileStream = File.OpenRead(FileName))
                    {
                        var signature = Sign(keyMaterial, fileStream);
                        Utils.WriteAllBytes(sigFileName, signature);
                    }
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
                        using (var fileStream = File.OpenRead(FileName))
                        {
                            var signature = Sign(keyMaterial, fileStream);
                            Signature = Convert.ToHexString(signature);
                        }
                    }
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"Unable to create signature string {Path.GetFileName(FileName)}: {e.Message}", ConsoleColor.Yellow);
            }

            return !string.IsNullOrWhiteSpace(Signature);
        }

        private static byte[] Encrypt(byte[] Bytes, string Secret)
        {
            byte[] buffer = ArrayPool<byte>.Shared.Rent(0x1000);

            try
            {
                using (MemoryStream memoryStream = new MemoryStream(buffer, 0, 0x1000, true, true))
                {
                    using (var rijndael = GetRijndael(Secret))
                    using (var cryptoEncrypt = rijndael.CreateEncryptor())
                    using (MemoryStream blobStream = new MemoryStream(Bytes))
                    using (CryptoStream cryptoStream = new CryptoStream(memoryStream, cryptoEncrypt, CryptoStreamMode.Write, true))
                    {
                        blobStream.CopyTo(cryptoStream);
                    }

                    byte[] copy = GC.AllocateUninitializedArray<byte>((int)memoryStream.Position);
                    buffer.AsSpan(0, (int)memoryStream.Position).CopyTo(copy);
                    return copy;
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"[Encrypt-Exception]: {e.Message}", ConsoleColor.Red);
            }
            finally
            {
                ArrayPool<byte>.Shared.Return(buffer, true);
            }

            return null;
        }

        private static byte[] Decrypt(byte[] Bytes, string Secret)
        {
            byte[] buffer = ArrayPool<byte>.Shared.Rent(0x1000);

            try
            {
                using (MemoryStream memoryStream = new MemoryStream(buffer, 0, 0x1000, true, true))
                {
                    using (var rijndael = GetRijndael(Secret))
                    using (var cryptoDecrypt = rijndael.CreateDecryptor())
                    using (MemoryStream blobStream = new MemoryStream(Bytes))
                    using (CryptoStream cryptoStream = new CryptoStream(memoryStream, cryptoDecrypt, CryptoStreamMode.Write, true))
                    {
                        blobStream.CopyTo(cryptoStream);
                    }

                    byte[] copy = GC.AllocateUninitializedArray<byte>((int)memoryStream.Position);
                    buffer.AsSpan(0, (int)memoryStream.Position).CopyTo(copy);
                    return copy;
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage($"[Decrypt-Exception]: {e.Message}", ConsoleColor.Red);
            }
            finally
            {
                ArrayPool<byte>.Shared.Return(buffer, true);
            }

            return null;
        }

        private static Aes GetRijndael(string Secret)
        {
            using (Rfc2898DeriveBytes rfc2898DeriveBytes = new Rfc2898DeriveBytes(
                Secret,
                Convert.FromBase64String("e0U0RTY2RjU5LUNBRjItNEMzOS1BN0Y4LTQ2MDk3QjFDNDYxQn0="),
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

        private static byte[] Sign(byte[] KeyMaterial, Stream Stream)
        {
            byte[] buffer;

            using (CngKey cngkey = CngKey.Import(KeyMaterial, CngKeyBlobFormat.GenericPrivateBlob))
            {
                if (cngkey.Algorithm == CngAlgorithm.ECDsaP256)
                {
                    using (ECDsaCng ecdsa = new ECDsaCng(cngkey))
                    {
                        buffer = ecdsa.SignData(Stream, HashAlgorithmName.SHA256);
                    }
                }
                else if (cngkey.Algorithm == CngAlgorithm.Rsa)
                {
                    using (RSACng rsa = new RSACng(cngkey))
                    {
                        buffer = rsa.SignData(Stream, HashAlgorithmName.SHA512, RSASignaturePadding.Pss);
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

        private static bool GetKeyMaterial(string KeyName, out byte[] KeyMaterial)
        {
            if (Win32.GetEnvironmentVariable(KeyName_Vars[KeyName], out string secret))
            {
                byte[] bytes = Utils.ReadAllBytes(GetPath($"{KeyName}.s"));
                KeyMaterial = Decrypt(bytes, secret);
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
    }
}
