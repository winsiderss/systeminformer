namespace RSAKeyVaultProvider
{
    static class Sha1Helper
    {
        const int SHA1_SIZE = 20;
        static readonly byte[] sha1Digest = new byte[] { 0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2B, 0x0E, 0x03, 0x02, 0x1A, 0x05, 0x00, 0x04, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

        public static byte[] CreateDigest(byte[] hash)
        {
            if (hash.Length != SHA1_SIZE)
                throw new ArgumentException("Invalid hash value");

            byte[] pkcs1Digest = (byte[])sha1Digest.Clone();
            Array.Copy(hash, 0, pkcs1Digest, pkcs1Digest.Length - hash.Length, hash.Length);

            return pkcs1Digest;
        }
    }
}
