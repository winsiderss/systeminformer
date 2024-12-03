namespace CustomBuildTool
{
    /// <summary>
    /// Extensions for creating ECDsa from a Key Vault client.
    /// </summary>
    public static class ECDsaFactory
    {
        /// <summary>
        /// Creates an ECDsa object
        /// </summary>
        /// <param name="credential"></param>
        /// <param name="keyId"></param>
        /// <param name="key"></param>
        /// <returns></returns>
        public static ECDsa Create(TokenCredential credential, Uri keyId, JsonWebKey key)
        {
            if (credential is null)
                throw new ArgumentNullException(nameof(credential));

            if (keyId is null)
                throw new ArgumentNullException(nameof(keyId));

            if (key is null)
                throw new ArgumentNullException(nameof(key));

            return new ECDsaKeyVault(new KeyVaultContext(credential, keyId, key));
        }

        /// <summary>
        /// Creates an ECDsa object
        /// </summary>
        /// <param name="credential"></param>
        /// <param name="keyId"></param>
        /// <param name="publicCertificate"></param>
        /// <returns></returns>
        public static ECDsa Create(TokenCredential credential, Uri keyId, X509Certificate2 publicCertificate)
        {
            if (credential is null)
                throw new ArgumentNullException(nameof(credential));

            if (keyId is null)
                throw new ArgumentNullException(nameof(keyId));

            if (publicCertificate is null)
                throw new ArgumentNullException(nameof(publicCertificate));

            return new ECDsaKeyVault(new KeyVaultContext(credential, keyId, publicCertificate));
        }
    }
}
