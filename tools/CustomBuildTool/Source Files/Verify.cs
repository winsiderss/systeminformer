using System;
using System.IO;
using System.Security.Cryptography;

namespace CustomBuildTool
{
    public static class Verify
    {
        private static Rijndael GetRijndael(string secret)
        {
            Rfc2898DeriveBytes rfc2898DeriveBytes = new Rfc2898DeriveBytes(secret, Convert.FromBase64String("e0U0RTY2RjU5LUNBRjItNEMzOS1BN0Y4LTQ2MDk3QjFDNDYxQn0="), 10000);
            Rijndael rijndael = Rijndael.Create();
            rijndael.Key = rfc2898DeriveBytes.GetBytes(32);
            rijndael.IV = rfc2898DeriveBytes.GetBytes(16);
            return rijndael;
        }

        public static void Encrypt(string fileName, string outFileName, string secret)
        {
            using (Rijndael rijndael = GetRijndael(secret))
            using (FileStream fileStream = File.OpenRead(fileName))
            using (FileStream fileStream2 = File.Create(outFileName))
            using (CryptoStream cryptoStream = new CryptoStream(fileStream2, rijndael.CreateEncryptor(), CryptoStreamMode.Write))
            {
                fileStream.CopyTo(cryptoStream);
            }
        }

        public static void Decrypt(string FileName, string outFileName, string secret)
        {
            using (Rijndael rijndael = GetRijndael(secret))
            using (FileStream fileStream = File.OpenRead(FileName))
            using (FileStream fileStream2 = File.Create(outFileName))
            using (CryptoStream cryptoStream = new CryptoStream(fileStream2, rijndael.CreateDecryptor(), CryptoStreamMode.Write))
            {
                fileStream.CopyTo(cryptoStream);
            }
        }

        public static string HashFile(string FileName)
        {
            //using (HashAlgorithm algorithm = new SHA256CryptoServiceProvider())
            //{
            //    byte[] inputBytes = Encoding.UTF8.GetBytes(input);
            //    byte[] hashBytes = algorithm.ComputeHash(inputBytes);
            //    return BitConverter.ToString(hashBytes).Replace("-", String.Empty);
            //}

            using (FileStream stream = File.OpenRead(FileName))
            using (BufferedStream bufferedStream = new BufferedStream(stream, 0x1000))
            {
                SHA256Managed sha = new SHA256Managed();
                byte[] checksum = sha.ComputeHash(bufferedStream);

                return BitConverter.ToString(checksum).Replace("-", string.Empty);
            }
        }
    }
}
