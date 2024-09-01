namespace CustomBuildTool
{
    internal static class AlgorithmTranslator
    {
        public static Windows.Win32.Security.Cryptography.ALG_ID HashAlgorithmToAlgId(HashAlgorithmName hashAlgorithmName)
        {
            if (hashAlgorithmName.Name == HashAlgorithmName.SHA1.Name)
                return Windows.Win32.Security.Cryptography.ALG_ID.CALG_SHA1;
            if (hashAlgorithmName.Name == HashAlgorithmName.SHA256.Name)
                return Windows.Win32.Security.Cryptography.ALG_ID.CALG_SHA_256;
            if (hashAlgorithmName.Name == HashAlgorithmName.SHA384.Name)
                return Windows.Win32.Security.Cryptography.ALG_ID.CALG_SHA_384;
            if (hashAlgorithmName.Name == HashAlgorithmName.SHA512.Name)
                return Windows.Win32.Security.Cryptography.ALG_ID.CALG_SHA_512;

            throw new NotSupportedException("The algorithm specified is not supported.");
        }

        public static ReadOnlySpan<byte> HashAlgorithmToOidAsciiTerminated(HashAlgorithmName hashAlgorithmName)
        {
            if (hashAlgorithmName.Name == HashAlgorithmName.SHA1.Name)
                //1.3.14.3.2.26
                return new byte[] { 0x31, 0x2e, 0x33, 0x2e, 0x31, 0x34, 0x2e, 0x33, 0x2e, 0x32, 0x2e, 0x32, 0x36, 0x00 };
            if (hashAlgorithmName.Name == HashAlgorithmName.SHA256.Name)
                //2.16.840.1.101.3.4.2.1
                return new byte[] { 0x32, 0x2e, 0x31, 0x36, 0x2e, 0x38, 0x34, 0x30, 0x2e, 0x31, 0x2e, 0x31, 0x30, 0x31, 0x2e, 0x33, 0x2e, 0x34, 0x2e, 0x32, 0x2e, 0x31, 0x00 };
            if (hashAlgorithmName.Name == HashAlgorithmName.SHA384.Name)
                //2.16.840.1.101.3.4.2.2
                return new byte[] { 0x32, 0x2e, 0x31, 0x36, 0x2e, 0x38, 0x34, 0x30, 0x2e, 0x31, 0x2e, 0x31, 0x30, 0x31, 0x2e, 0x33, 0x2e, 0x34, 0x2e, 0x32, 0x2e, 0x32, 0x00 };
            if (hashAlgorithmName.Name == HashAlgorithmName.SHA512.Name)
                //2.16.840.1.101.3.4.2.3
                return new byte[] { 0x32, 0x2e, 0x31, 0x36, 0x2e, 0x38, 0x34, 0x30, 0x2e, 0x31, 0x2e, 0x31, 0x30, 0x31, 0x2e, 0x33, 0x2e, 0x34, 0x2e, 0x32, 0x2e, 0x33, 0x00 };
            throw new NotSupportedException("The algorithm specified is not supported.");
        }
    }
}
