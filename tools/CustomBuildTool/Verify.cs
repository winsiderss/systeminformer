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
        public static void Encrypt(string FileName, string outFileName, string Secret)
        {
            if (!File.Exists(FileName))
            {
                Program.PrintColorMessage($"Unable to encrypt file {Path.GetFileName(FileName)}: Does not exist.", ConsoleColor.Yellow);
                return;
            }

            using (Aes rijndael = GetRijndael(Secret))
            using (FileStream fileStream = File.OpenRead(FileName))
            using (FileStream fileOutStream = File.Create(outFileName))
            using (CryptoStream cryptoStream = new CryptoStream(fileOutStream, rijndael.CreateEncryptor(), CryptoStreamMode.Write, true))
            {
                fileStream.CopyTo(cryptoStream);
            }
        }

        public static bool Decrypt(string FileName, string outFileName, string Secret)
        {
            if (!File.Exists(FileName))
            {
                Program.PrintColorMessage($"Unable to decrypt file {Path.GetFileName(FileName)}: Does not exist.", ConsoleColor.Yellow);
                return false;
            }

            try
            {
                using (Aes rijndael = GetRijndael(Secret))
                using (FileStream fileStream = File.OpenRead(FileName))
                using (FileStream fileOutStream = File.Create(outFileName))
                using (CryptoStream cryptoStream = new CryptoStream(fileOutStream, rijndael.CreateDecryptor(), CryptoStreamMode.Write, true))
                {
                    fileStream.CopyTo(cryptoStream);
                }
            }
            catch
            {
                return false;
            }

            return true;
        }

        public static string EncryptBlob(byte[] Blob, string Secret)
        {
            byte[] buffer = ArrayPool<byte>.Shared.Rent(0x1000);

            try
            {
                using (MemoryStream memoryStream = new MemoryStream(buffer, 0, 0x1000, true, true))
                {
                    using (var rijndael = GetRijndael(Secret))
                    using (var cryptoEncrypt = rijndael.CreateEncryptor())
                    using (MemoryStream blobStream = new MemoryStream(Blob))
                    using (CryptoStream cryptoStream = new CryptoStream(memoryStream, cryptoEncrypt, CryptoStreamMode.Write, true))
                    {
                        blobStream.CopyTo(cryptoStream);
                    }

                    return Convert.ToHexString(new ReadOnlySpan<byte>(memoryStream.GetBuffer(), 0, (int)memoryStream.Position));
                }
            }
            catch (Exception)
            {
                Program.PrintColorMessage("[EncryptBlob-Exception]", ConsoleColor.Red);
            }
            finally
            {
                ArrayPool<byte>.Shared.Return(buffer, true);
            }

            return null;
        }

        public static byte[] DecryptFileBlob(string FileName, string Secret)
        {
            if (!File.Exists(FileName))
                return null;

            byte[] blob = File.ReadAllBytes(FileName);
            byte[] buffer = ArrayPool<byte>.Shared.Rent(0x1000);

            try
            {
                using (MemoryStream memoryStream = new MemoryStream(buffer, 0, 0x1000, true, true))
                {
                    using (var rijndael = GetRijndael(Secret))
                    using (var cryptoDecrypt = rijndael.CreateDecryptor())
                    using (MemoryStream blobStream = new MemoryStream(blob))
                    using (CryptoStream cryptoStream = new CryptoStream(memoryStream, cryptoDecrypt, CryptoStreamMode.Write, true))
                    {
                        blobStream.CopyTo(cryptoStream);
                    }

                    byte[] copy = GC.AllocateUninitializedArray<byte>((int)memoryStream.Position);
                    buffer.AsSpan(0, (int)memoryStream.Position).CopyTo(copy);
                    return copy;
                }
            }
            catch (Exception)
            {
                Program.PrintColorMessage("[DecryptBlob-Exception]", ConsoleColor.Red);
            }
            finally
            {
                ArrayPool<byte>.Shared.Return(buffer, true);
            }

            return null;
        }

        public static byte[] DecryptHexBlob(string Blob, string Secret)
        {
            byte[] blob = Convert.FromHexString(Blob);
            byte[] buffer = ArrayPool<byte>.Shared.Rent(0x1000);

            try
            {
                using (MemoryStream memoryStream = new MemoryStream(buffer, 0, 0x1000, true, true))
                {
                    using (var rijndael = GetRijndael(Secret))
                    using (var cryptoDecrypt = rijndael.CreateDecryptor())
                    using (MemoryStream blobStream = new MemoryStream())
                    using (CryptoStream cryptoStream = new CryptoStream(memoryStream, cryptoDecrypt, CryptoStreamMode.Write, true))
                    {
                        blobStream.CopyTo(cryptoStream);
                    }

                    byte[] copy = GC.AllocateUninitializedArray<byte>((int)memoryStream.Position);
                    buffer.AsSpan(0, (int)memoryStream.Position).CopyTo(copy);
                    return copy;
                }
            }
            catch (Exception)
            {
                Program.PrintColorMessage("[DecryptBlob-Exception]", ConsoleColor.Red);
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

        private static Aes GetRijndaelLegacy(string Secret)
        {
#pragma warning disable SYSLIB0041 // Type or member is obsolete
            using (Rfc2898DeriveBytes rfc2898DeriveBytes = new Rfc2898DeriveBytes(
                Secret,
                Convert.FromBase64String("e0U0RTY2RjU5LUNBRjItNEMzOS1BN0Y4LTQ2MDk3QjFDNDYxQn0="),
                10000
                ))
            {
                Aes rijndael = Aes.Create();

                rijndael.Key = rfc2898DeriveBytes.GetBytes(32);
                rijndael.IV = rfc2898DeriveBytes.GetBytes(16);

                return rijndael;
            }
#pragma warning restore SYSLIB0041 // Type or member is obsolete
        }

        public static bool DecryptLegacy(string FileName, string outFileName, string Secret)
        {
            if (!File.Exists(FileName))
            {
                Program.PrintColorMessage($"Unable to decrypt legacy file {Path.GetFileName(FileName)}: Does not exist.", ConsoleColor.Yellow);
                return false;
            }

            try
            {
                using (Aes rijndael = GetRijndaelLegacy(Secret))
                using (FileStream fileStream = File.OpenRead(FileName))
                using (FileStream fileOutStream = File.Create(outFileName))
                using (CryptoStream cryptoStream = new CryptoStream(fileOutStream, rijndael.CreateDecryptor(), CryptoStreamMode.Write, true))
                {
                    fileStream.CopyTo(cryptoStream);
                }
            }
            catch
            {
                return false;
            }

            return true;
        }

        public static string HashFile(string FileName)
        {
            if (!File.Exists(FileName))
            {
                Program.PrintColorMessage($"Unable to hash file {Path.GetFileName(FileName)}: Does not exist.", ConsoleColor.Yellow);
                return string.Empty;
            }

            //using (HashAlgorithm algorithm = new SHA256CryptoServiceProvider())
            //{
            //    byte[] inputBytes = Encoding.UTF8.GetBytes(input);
            //    byte[] hashBytes = algorithm.ComputeHash(inputBytes);
            //    return BitConverter.ToString(hashBytes).Replace("-", String.Empty);
            //}

            using (FileStream fileInStream = File.OpenRead(FileName))
            using (BufferedStream bufferedStream = new BufferedStream(fileInStream, 0x1000))
            using (SHA256 sha = SHA256.Create())
            {
                byte[] checksum = sha.ComputeHash(bufferedStream);

                return BitConverter.ToString(checksum).Replace("-", string.Empty, StringComparison.OrdinalIgnoreCase);
            }
        }

        public static byte[] SignData(string FileName, ReadOnlySpan<byte> Buffer)
        {
            byte[] buffer;
            byte[] blob = File.ReadAllBytes(FileName);

            using (CngKey cngkey = CngKey.Import(blob, CngKeyBlobFormat.GenericPrivateBlob))
            using (ECDsaCng ecdsa = new ECDsaCng(cngkey))
            {
                buffer = ecdsa.SignData(Buffer, HashAlgorithmName.SHA256);
            }

            return buffer;
        }

        public static byte[] SignData(byte[] Blob, Stream Buffer)
        {
            byte[] buffer;

            using (CngKey cngkey = CngKey.Import(Blob, CngKeyBlobFormat.GenericPrivateBlob))
            using (ECDsaCng ecdsa = new ECDsaCng(cngkey))
            {
                buffer = ecdsa.SignData(Buffer, HashAlgorithmName.SHA256);
            }

            return buffer;
        }

        public static unsafe void GenerateKeys(out byte[] PrivateKeyBlob, out byte[] PublicKeyBlob)
        {
            using (ECDsaCng cng = new ECDsaCng(256))
            {
                cng.HashAlgorithm = CngAlgorithm.Sha256;
                cng.GenerateKey(ECCurve.NamedCurves.nistP256);

                PrivateKeyBlob = cng.Key.Export(CngKeyBlobFormat.GenericPrivateBlob);
                PublicKeyBlob = cng.Key.Export(CngKeyBlobFormat.GenericPublicBlob);
            }
        }

        public static string GetPath(string FileName)
        {
            return $"{Build.BuildWorkingFolder}\\tools\\CustomSignTool\\Resources\\{FileName}";
        }

        public static bool CreateSignatureString(byte[] Blob, string FileName, out string Signature)
        {
            byte[] buffer;

            using (FileStream fileStream = File.OpenRead(FileName))
            using (BufferedStream bufferedStream = new BufferedStream(fileStream))
            {
                buffer = SignData(Blob, bufferedStream);
            }

            Signature = Convert.ToHexString(buffer);

            return string.IsNullOrWhiteSpace(Signature);
        }

        public static bool CreateSignatureFile(byte[] Blob, string FileName, bool FailIfExists = false)
        {
            string signatureFileName = Path.ChangeExtension(FileName, ".sig");

            if (FailIfExists)
            {
                if (File.Exists(signatureFileName) && Win32.GetFileSize(signatureFileName) != 0)
                {
                    Program.PrintColorMessage($"[CreateSignature - FailIfExists] ({signatureFileName})", ConsoleColor.Red);
                    return false;
                }
            }

            Win32.CreateEmptyFile(signatureFileName);

            using (FileStream fileStream = File.OpenRead(FileName))
            using (BufferedStream bufferedStream = new BufferedStream(fileStream))
            {
                using (CngKey key = CngKey.Import(Blob, CngKeyBlobFormat.GenericPrivateBlob))
                using (ECDsaCng cng = new ECDsaCng(key))
                {
                    byte[] hash = cng.SignData(bufferedStream, HashAlgorithmName.SHA256);

                    File.WriteAllBytes(signatureFileName, hash);
                }
            }

            if (File.Exists(signatureFileName) && Win32.GetFileSize(signatureFileName) != 0)
                return true;

            return false;
        }

        public static bool CreateSignatureFile(string KeyFile, string SignFile, bool FailIfExists = false)
        {
            string sigfile = Path.ChangeExtension(SignFile, ".sig");

            if (FailIfExists)
            {
                if (File.Exists(sigfile) && Win32.GetFileSize(sigfile) != 0)
                {
                    Program.PrintColorMessage($"[CreateSignature - FailIfExists] ({sigfile})", ConsoleColor.Red, true, BuildFlags.BuildVerbose);
                    return false;
                }
            }

            Win32.CreateEmptyFile(sigfile);

            byte[] sigBlob = File.ReadAllBytes(KeyFile);

            return CreateSignatureFile(sigBlob, SignFile, FailIfExists);
        }
    }
}
