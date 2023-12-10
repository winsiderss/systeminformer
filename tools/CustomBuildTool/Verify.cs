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
        public static bool EncryptFile(string FileName, string DestinationFile, string Value, string Salt)
        {
            if (!File.Exists(FileName))
            {
                Program.PrintColorMessage($"{Path.GetFileName(FileName)} does not exist.", ConsoleColor.Yellow);
                return false;
            }

            try
            {
                using (FileStream fileReadStream = File.OpenRead(FileName))
                using (FileStream fileWritetStream = File.Create(DestinationFile))
                using (ICryptoTransform fileCryptoEncrypt = CreateEncryptor(Value, Salt))
                using (CryptoStream fileCryptoStream = new CryptoStream(fileWritetStream, fileCryptoEncrypt, CryptoStreamMode.Write, true))
                {
                    fileReadStream.CopyTo(fileCryptoStream);
                }
            }
            catch
            {
                Program.PrintColorMessage("[EncryptFile-Exception]", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool DecryptFile(string FileName, string DestinationFile, string Value, string Salt)
        {
            if (string.IsNullOrWhiteSpace(FileName))
            {
                Program.PrintColorMessage("FileName is invalid.", ConsoleColor.Red);
                return false;
            }
            if (string.IsNullOrWhiteSpace(DestinationFile))
            {
                Program.PrintColorMessage("DestinationFile is invalid.", ConsoleColor.Red);
                return false;
            }
            if (string.IsNullOrWhiteSpace(Value))
            {
                Program.PrintColorMessage("Value is invalid.", ConsoleColor.Red);
                return false;
            }
            if (string.IsNullOrWhiteSpace(Salt))
            {
                Program.PrintColorMessage("Salt is invalid.", ConsoleColor.Red);
                return false;
            }

            if (!File.Exists(FileName))
            {
                Program.PrintColorMessage($"{Path.GetFileName(FileName)} does not exist.", ConsoleColor.Yellow);
                return false;
            }

            try
            {
                using (FileStream fileReadStream = File.OpenRead(FileName))
                using (FileStream fileWritetStream = File.Create(DestinationFile))
                using (ICryptoTransform fileCryptoDecrypt = CreateEncryptor(Value, Salt))
                using (CryptoStream fileCryptoStream = new CryptoStream(fileWritetStream, fileCryptoDecrypt, CryptoStreamMode.Write, true))
                {
                    fileReadStream.CopyTo(fileCryptoStream);
                }
            }
            catch
            {
                Program.PrintColorMessage("[DecryptFile-Exception]", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static string EncryptString(string PlainText, string Value, string Salt)
        {
            try
            {
                using (MemoryStream memoryStream = new MemoryStream())
                {
                    using (ICryptoTransform cryptoEncrypt = CreateEncryptor(Value, Salt))
                    using (CryptoStream cryptoStream = new CryptoStream(memoryStream, cryptoEncrypt, CryptoStreamMode.Write, true))
                    using (StreamWriter streamWriter = new StreamWriter(cryptoStream))
                    {
                        streamWriter.Write(PlainText);
                    }

                    return Convert.ToHexString(memoryStream.ToArray());
                }
            }
            catch
            {
                Program.PrintColorMessage("[EncryptString-Exception]", ConsoleColor.Red);
            }

            return null;
        }

        public static string DecryptString(string CipherText, string Value, string Swab)
        {
            try
            {
                byte[] blob = Convert.FromHexString(CipherText);

                using (MemoryStream memoryStream = new MemoryStream(blob))
                using (ICryptoTransform cryptoDecrypt = CreateEncryptor(Value, Swab))
                using (CryptoStream cryptoStream = new CryptoStream(memoryStream, cryptoDecrypt, CryptoStreamMode.Read, true))
                using (StreamReader streamReader = new StreamReader(cryptoStream))
                {
                    return streamReader.ReadToEnd();
                }
            }
            catch
            {
                Program.PrintColorMessage("[DecryptString-Exception]", ConsoleColor.Red);
            }

            return null;
        }

        public static bool DecryptEnvironment(CacheConfig Config)
        {
            if (!Win32.GetEnvironmentVariable(Config.Name, out string name))
                return false;
            if (!Win32.GetEnvironmentVariable(Config.Value, out string value))
                return false;
            if (!Win32.GetEnvironmentVariable(Config.Swab, out string swab))
                return false;

            try
            {
                byte[] blob = Convert.FromBase64String(name);

                using (MemoryStream memoryStream = new MemoryStream())
                {
                    using (ICryptoTransform cryptoDecrypt = CreateDecryptor(value, swab))
                    using (CryptoStream cryptoStream = new CryptoStream(memoryStream, cryptoDecrypt, CryptoStreamMode.Write, true))
                    {
                        cryptoStream.Write(blob);
                    }

                    if (!VerifyCache.Set(Config, memoryStream.GetBuffer()))
                        return false;
                }
            }
            catch
            {
                Program.PrintColorMessage("[DecryptEnvironment-Exception]", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool DecryptBlob(string FileName, CacheConfig Config, string Value, string Swab)
        {
            try
            {
                using (MemoryStream memoryStream = new MemoryStream())
                {
                    using (FileStream fileStream = File.OpenRead(FileName))
                    using (ICryptoTransform cryptoDecrypt = CreateDecryptor(Value, Swab))
                    using (CryptoStream cryptoStream = new CryptoStream(memoryStream, cryptoDecrypt, CryptoStreamMode.Write, true))
                    {
                        fileStream.CopyTo(cryptoStream);
                    }

                    if (!VerifyCache.Set(Config, memoryStream.GetBuffer()))
                        return false;
                }
            }
            catch
            {
                Program.PrintColorMessage("[DecryptEnvironment-Exception]", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static string HashFile(string FileName)
        {
            byte[] buffer;

            if (!File.Exists(FileName))
            {
                Program.PrintColorMessage($"HashFile: {Path.GetFileName(FileName)} does not exist.", ConsoleColor.Yellow);
                return null;
            }

            using (FileStream fileStream = File.OpenRead(FileName))
            using (SHA256 algorithm = SHA256.Create())
            {
                buffer = algorithm.ComputeHash(fileStream);
            }

            return Convert.ToHexString(buffer);
        }

        public static byte[] SignData(CacheConfig Name, byte[] Buffer)
        {
            byte[] buffer;

            if (!VerifyCache.Get(Name, out byte[] blob))
                return null;

            using (CngKey cngkey = CngKey.Import(blob, CngKeyBlobFormat.GenericPrivateBlob))
            using (ECDsaCng ecdsa = new ECDsaCng(cngkey))
            {
                buffer = ecdsa.SignData(Buffer, HashAlgorithmName.SHA256);
            }

            return buffer;
        }

        public static byte[] SignData(CacheConfig Name, Stream Buffer)
        {
            byte[] buffer;

            if (!VerifyCache.Get(Name, out byte[] blob))
                return null;

            using (CngKey cngkey = CngKey.Import(blob, CngKeyBlobFormat.GenericPrivateBlob))
            using (ECDsaCng ecdsa = new ECDsaCng(cngkey))
            {
                buffer = ecdsa.SignData(Buffer, HashAlgorithmName.SHA256);
            }

            return buffer;
        }

        public static bool CreateSignatureString(CacheConfig Name, string FileName, out string Signature)
        {
            byte[] buffer;

            using (FileStream fileStream = File.OpenRead(FileName))
            using (BufferedStream bufferedStream = new BufferedStream(fileStream))
            {
                buffer = SignData(Name, bufferedStream);
            }

            Signature = Convert.ToHexString(buffer);

            return string.IsNullOrWhiteSpace(Signature);
        }

        public static bool CreateSignatureFile(CacheConfig Name, string FileName, bool FailIfExists = false)
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

            if (!VerifyCache.Get(Name, out byte[] blob))
            {
                Program.PrintColorMessage("[CreateSignature - Key not found]", ConsoleColor.Red);
                return false;
            }

            using (FileStream fileStream = File.OpenRead(FileName))
            using (BufferedStream bufferedStream = new BufferedStream(fileStream))
            {
                using (CngKey key = CngKey.Import(blob, CngKeyBlobFormat.GenericPrivateBlob))
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

        private static ICryptoTransform CreateEncryptor(string Value, string Salt)
        {
            byte[] buffer = Convert.FromBase64String(Salt);

            using (Rfc2898DeriveBytes rfc2898DeriveBytes = new Rfc2898DeriveBytes(Value, buffer, 100000, HashAlgorithmName.SHA512))
            using (Aes rijndael = Aes.Create())
            {
                rijndael.Key = rfc2898DeriveBytes.GetBytes(32);
                rijndael.IV = rfc2898DeriveBytes.GetBytes(16);

                return rijndael.CreateEncryptor();
            }
        }

        private static ICryptoTransform CreateDecryptor(string Value, string Salt)
        {
            byte[] buffer = Convert.FromBase64String(Salt);

            using (Rfc2898DeriveBytes rfc2898DeriveBytes = new Rfc2898DeriveBytes(Value, buffer, 100000, HashAlgorithmName.SHA512))
            using (Aes rijndael = Aes.Create())
            {
                rijndael.Key = rfc2898DeriveBytes.GetBytes(32);
                rijndael.IV = rfc2898DeriveBytes.GetBytes(16);

                return rijndael.CreateDecryptor();
            }
        }
    }

    public static class VerifyCache
    {
        private static readonly Dictionary<CacheConfig, GCHandle> Cache = new Dictionary<CacheConfig, GCHandle>();
        public static readonly CacheConfig KMODE_CI = new CacheConfig("KA_NAME","KB_NAME","KC_NAME","KD_NAME");
        public static readonly CacheConfig BUILD_CI = new CacheConfig("NA_NAME","NB_NAME","NC_NAME","ND_NAME");

        static VerifyCache()
        {
            ReadLocalDebugTestKeys();
        }

        public static bool Contains(CacheConfig Name)
        {
            return Cache.ContainsKey(Name);
        }

        public static bool Get(CacheConfig Name, out byte[] Blob)
        {
            if (Cache.TryGetValue(Name, out GCHandle PinnedBlob))
            {
                Blob = (byte[])PinnedBlob.Target;
                return true;
            }

            Blob = null;
            return false;
        }

        public static bool Set(CacheConfig Name, byte[] Blob)
        {
            var pinnedBlob = GCHandle.Alloc(Blob, GCHandleType.Pinned);

            if (Cache.ContainsKey(Name) && !Cache.Remove(Name))
            {
                pinnedBlob.Free();
                return false;
            }

            if (!Cache.TryAdd(Name, pinnedBlob))
            {
                pinnedBlob.Free();
                return false;
            }

            return true;
        }

        private static void ReadLocalDebugTestKeys()
        {
            if (!Win32.GetEnvironmentVariable(KMODE_CI.Alt, out string KMODE))
                KMODE = GetPath("test1.key");
            if (!Win32.GetEnvironmentVariable(BUILD_CI.Alt, out string BUILD))
                BUILD = GetPath("test2.key");

            if (File.Exists(KMODE))
            {
                Set(KMODE_CI, File.ReadAllBytes(KMODE));
            }
            else
            {
                if (
                    Win32.GetEnvironmentVariable(KMODE_CI.Value, out string Value) &&
                    Win32.GetEnvironmentVariable(KMODE_CI.Swab, out string Swab)
                    )
                {
                    Verify.DecryptBlob(KMODE, KMODE_CI, Value, Swab);
                }
            }

            if (File.Exists(BUILD))
            {
                Set(BUILD_CI, File.ReadAllBytes(BUILD));
            }
            else
            {
                if (
                    Win32.GetEnvironmentVariable(BUILD_CI.Value, out string Value) &&
                    Win32.GetEnvironmentVariable(BUILD_CI.Swab, out string Swab)
                    )
                {
                    Verify.DecryptBlob(BUILD, BUILD_CI, Value, Swab);
                }
            }
        }

        private static string GetPath(string FileName)
        {
            StringBuilder sb = new StringBuilder(256);
            sb.Append(Build.CurrentDirectory);
            sb.Append("\\tools\\CustomSignTool\\Resources\\");
            sb.Append(FileName);
            return sb.ToString();
        }
    }

    public class CacheConfig : IComparable, IComparable<CacheConfig>, IEquatable<CacheConfig>
    {
        public readonly string Name;
        public readonly string Value;
        public readonly string Swab;
        public readonly string Alt;

        public CacheConfig(string Name, string Value, string Swab, string Alt)
        {
            this.Name = Name;
            this.Value = Value;
            this.Swab = Swab;
            this.Alt = Alt;
        }

        public override string ToString()
        {
            return this.Name;
        }

        public override int GetHashCode()
        {
            return this.Name.GetHashCode(StringComparison.OrdinalIgnoreCase);
        }

        public override bool Equals(object obj)
        {
            if (obj is not CacheConfig config)
                return false;

            return this.Name.Equals(config.Name, StringComparison.OrdinalIgnoreCase);
        }

        public bool Equals(CacheConfig other)
        {
            if (other == null)
                return false;

            return this.Name.Equals(other.Name, StringComparison.OrdinalIgnoreCase);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
                return 1;

            if (obj is CacheConfig config)
                return string.Compare(this.Name, config.Name, StringComparison.OrdinalIgnoreCase);
            else
                return 1;
        }

        public int CompareTo(CacheConfig obj)
        {
            if (obj == null)
                return 1;

            return string.Compare(this.Name, obj.Name, StringComparison.OrdinalIgnoreCase);
        }
    }

}
