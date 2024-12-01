namespace CustomBuildTool
{
    /// <summary>
    /// A signing context used for signing packages with Azure Key Vault Keys.
    /// </summary>
    public readonly struct KeyVaultContext
    {
        readonly CryptographyClient cryptographyClient;

        /// <summary>
        /// Creates a new Key Vault context.
        /// </summary>
        public KeyVaultContext(TokenCredential credential, Uri keyId, JsonWebKey key)
        {
            if (credential is null)
            {
                throw new ArgumentNullException(nameof(credential));
            }
            if (keyId is null)
            {
                throw new ArgumentNullException(nameof(keyId));
            }
            if (key is null)
            {
                throw new ArgumentNullException(nameof(key));
            }

            this.Certificate = null;
            this.KeyIdentifier = keyId;
            this.Key = key;
            this.cryptographyClient = new CryptographyClient(keyId, credential);           
        }

        /// <summary>
        /// Creates a new Key Vault context.
        /// </summary>
        public KeyVaultContext(TokenCredential credential, Uri keyId, X509Certificate2 publicCertificate)
        {
            if (credential is null)
            {
                throw new ArgumentNullException(nameof(credential));
            }
            if (keyId is null)
            {
                throw new ArgumentNullException(nameof(keyId));
            }
            if (publicCertificate is null)
            {
                throw new ArgumentNullException(nameof(publicCertificate));
            }

            this.Certificate = publicCertificate;
            this.KeyIdentifier = keyId;
            this.cryptographyClient = new CryptographyClient(keyId, credential);

            string algorithm = publicCertificate.GetKeyAlgorithm();

            if (algorithm.Equals("1.2.840.113549.1.1.1", StringComparison.OrdinalIgnoreCase)) // RSA
            {
                using (var rsa = publicCertificate.GetRSAPublicKey())
                {
                    Key = new JsonWebKey(rsa, includePrivateParameters: false);
                }
            }
            else if (algorithm.Equals("1.2.840.10045.2.1", StringComparison.OrdinalIgnoreCase)) // ECDsa
            {
                using (var ecdsa = publicCertificate.GetECDsaPublicKey())
                {
                    Key = new JsonWebKey(ecdsa, includePrivateParameters: false);
                }
            }
            else
            {
                throw new NotSupportedException($"Certificate algorithm '{algorithm}' is not supported.");
            }
        }

        /// <summary>
        /// Gets the certificate and public key used to validate the signature. May be null if 
        /// Key isn't part of a certificate
        /// </summary>
        public X509Certificate2 Certificate { get; }

        /// <summary>
        /// Identifyer of current key
        /// </summary>
        public Uri KeyIdentifier { get; }

        /// <summary>
        /// Public key 
        /// </summary>
        public JsonWebKey Key { get; }

        internal byte[] SignDigest(byte[] digest, HashAlgorithmName hashAlgorithm, KeyVaultSignatureAlgorithm signatureAlgorithm)
        {
            var algorithm = SignatureAlgorithmTranslator.SignatureAlgorithmToJwsAlgId(signatureAlgorithm, hashAlgorithm);

            if (hashAlgorithm == HashAlgorithmName.SHA1)
            {
                if (signatureAlgorithm != KeyVaultSignatureAlgorithm.RSAPkcs15)
                    throw new InvalidOperationException("SHA1 algorithm is not supported for this signature algorithm.");

                digest = CreateSha1Digest(digest);
            }

            var sigResult = cryptographyClient.Sign(algorithm, digest);

            return sigResult.Signature;
        }

        internal byte[] DecryptData(byte[] cipherText, RSAEncryptionPadding padding)
        {
            var algorithm = EncryptionPaddingTranslator.EncryptionPaddingToJwsAlgId(padding);

            var dataResult = cryptographyClient.Decrypt(algorithm, cipherText);
            return dataResult.Plaintext;
        }


        const int SHA1_SIZE = 20;
        static readonly byte[] sha1Digest = [0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2B, 0x0E, 0x03, 0x02, 0x1A, 0x05, 0x00, 0x04, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];

        internal static byte[] CreateSha1Digest(byte[] hash)
        {
            if (hash.Length != SHA1_SIZE)
                throw new ArgumentException("Invalid hash value");

            byte[] pkcs1Digest = (byte[])sha1Digest.Clone();
            Array.Copy(hash, 0, pkcs1Digest, pkcs1Digest.Length - hash.Length, hash.Length);

            return pkcs1Digest;
        }


        /// <summary>
        /// Returns true if properly constructed. If default, then false.
        /// </summary>
        public bool IsValid => cryptographyClient != null;
    }
}
