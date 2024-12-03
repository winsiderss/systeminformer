namespace CustomBuildTool
{
    static class EncryptionPaddingTranslator
    {
        public static EncryptionAlgorithm EncryptionPaddingToJwsAlgId(RSAEncryptionPadding padding)
        {
            switch (padding.Mode)
            {
                case RSAEncryptionPaddingMode.Pkcs1:
                    return EncryptionAlgorithm.Rsa15;
                case RSAEncryptionPaddingMode.Oaep when padding.OaepHashAlgorithm == HashAlgorithmName.SHA1:
                    return EncryptionAlgorithm.RsaOaep;
                case RSAEncryptionPaddingMode.Oaep when padding.OaepHashAlgorithm == HashAlgorithmName.SHA256:
                    return EncryptionAlgorithm.RsaOaep256;
                default:
                    throw new NotSupportedException("The padding specified is not supported.");
            }
        }
    }
}
