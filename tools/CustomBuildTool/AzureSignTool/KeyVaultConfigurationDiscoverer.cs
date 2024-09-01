using Azure.Core;

namespace CustomBuildTool
{
    // https://learn.microsoft.com/en-us/rest/api/keyvault/certificates/get-certificate/get-certificate?tabs=HTTP

    //public async Task<ErrorOr<AzureKeyVaultMaterializedConfiguration>> Materialize(AzureKeyVaultSignConfigurationSet configuration)
    //{
    //    TokenCredential credential;
    //
    //    if (configuration.ManagedIdentity)
    //    {
    //        credential = new DefaultAzureCredential();
    //    }
    //    else if(!string.IsNullOrWhiteSpace(configuration.AzureAccessToken))
    //    {
    //        credential = new AccessTokenCredential(configuration.AzureAccessToken);
    //    }
    //    else
    //    {
    //        credential = new ClientSecretCredential(configuration.AzureTenantId, configuration.AzureClientId, configuration.AzureClientSecret);
    //    }
    //
    //    X509Certificate2 certificate;
    //    KeyVaultCertificateWithPolicy azureCertificate;
    //    try
    //    {
    //        var certClient = new CertificateClient(configuration.AzureKeyVaultUrl, credential);
    //
    //        //_logger.LogTrace($"Retrieving certificate {configuration.AzureKeyVaultCertificateName}.");
    //        azureCertificate = (await certClient.GetCertificateAsync(configuration.AzureKeyVaultCertificateName).ConfigureAwait(false)).Value;
    //        //_logger.LogTrace($"Retrieved certificate {configuration.AzureKeyVaultCertificateName}.");
    //
    //        certificate = new X509Certificate2(azureCertificate.Cer);
    //    }
    //    catch (Exception e)
    //    {
    //        //_logger.LogError($"Failed to retrieve certificate {configuration.AzureKeyVaultCertificateName} from Azure Key Vault. Please verify the name of the certificate and the permissions to the certificate. Error message: {e.Message}.");
    //        //_logger.LogTrace(e.ToString());
    //
    //        return e;
    //    }
    //    var keyId = azureCertificate.KeyId;
    //    return new AzureKeyVaultMaterializedConfiguration(credential, certificate, keyId);
    //}

    public static class Base64Url
    {
        public static byte[] Decode(string str)
        {
            str = new StringBuilder(str).Replace('-', '+').Replace('_', '/').Append('=', (str.Length % 4 != 0) ? (4 - str.Length % 4) : 0).ToString();
            return Convert.FromBase64String(str);
        }

        public static string Encode(byte[] bytes)
        {
            return new StringBuilder(Convert.ToBase64String(bytes)).Replace('+', '-').Replace('/', '_').Replace("=", "").ToString();
        }
    }

    public class AzureKeyVaultMaterializedConfiguration
    {
        public AzureKeyVaultMaterializedConfiguration(TokenCredential credential, X509Certificate2 publicCertificate, Uri keyId)
        {
            TokenCredential = credential;
            KeyId = keyId;
            PublicCertificate = publicCertificate;
        }

        public X509Certificate2 PublicCertificate { get; }
        public TokenCredential TokenCredential { get; }
        public Uri KeyId { get; }
    }
  
    public class AccessTokenCredential : TokenCredential
    {
        private readonly AccessToken accessToken;

        public AccessTokenCredential(string token, DateTimeOffset expiresOn)
        {
            if (string.IsNullOrWhiteSpace(token))
            {
                throw new ArgumentException("Access Token cannot be null or empty", nameof(token));
            }

            accessToken = new AccessToken(token, expiresOn);
        }

        public AccessTokenCredential(string token) : this(token, DateTimeOffset.UtcNow.AddHours(1))
        {

        }

        public override AccessToken GetToken(TokenRequestContext requestContext, CancellationToken cancellationToken)
        {
            return accessToken;
        }

        public override ValueTask<AccessToken> GetTokenAsync(TokenRequestContext requestContext, CancellationToken cancellationToken)
        {
            return new ValueTask<AccessToken>(accessToken);
        }
    }

    //
    // Summary:
    //     An operation that can be performed with the key.
    public readonly struct KeyOperation : IEquatable<KeyOperation>
    {
        private readonly string _value;

        //
        // Summary:
        //     Gets a value that indicates the key can be used to encrypt with the Azure.Security.KeyVault.Keys.Cryptography.CryptographyClient.EncryptAsync(Azure.Security.KeyVault.Keys.Cryptography.EncryptionAlgorithm,System.Byte[],System.Threading.CancellationToken)
        //     or Azure.Security.KeyVault.Keys.Cryptography.CryptographyClient.Encrypt(Azure.Security.KeyVault.Keys.Cryptography.EncryptionAlgorithm,System.Byte[],System.Threading.CancellationToken)
        //     methods.
        public static KeyOperation Encrypt { get; } = new KeyOperation("encrypt");


        //
        // Summary:
        //     Gets a value that indicates the key can be used to decrypt with the Azure.Security.KeyVault.Keys.Cryptography.CryptographyClient.DecryptAsync(Azure.Security.KeyVault.Keys.Cryptography.EncryptionAlgorithm,System.Byte[],System.Threading.CancellationToken)
        //     or Azure.Security.KeyVault.Keys.Cryptography.CryptographyClient.Decrypt(Azure.Security.KeyVault.Keys.Cryptography.EncryptionAlgorithm,System.Byte[],System.Threading.CancellationToken)
        //     methods.
        public static KeyOperation Decrypt { get; } = new KeyOperation("decrypt");


        //
        // Summary:
        //     Gets a value that indicates the key can be used to sign with the Azure.Security.KeyVault.Keys.Cryptography.CryptographyClient.SignAsync(Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm,System.Byte[],System.Threading.CancellationToken)
        //     or Azure.Security.KeyVault.Keys.Cryptography.CryptographyClient.Sign(Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm,System.Byte[],System.Threading.CancellationToken)
        //     methods.
        public static KeyOperation Sign { get; } = new KeyOperation("sign");


        //
        // Summary:
        //     Gets a value that indicates the key can be used to verify with the Azure.Security.KeyVault.Keys.Cryptography.CryptographyClient.VerifyAsync(Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm,System.Byte[],System.Byte[],System.Threading.CancellationToken)
        //     or Azure.Security.KeyVault.Keys.Cryptography.CryptographyClient.Verify(Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm,System.Byte[],System.Byte[],System.Threading.CancellationToken)
        //     methods.
        public static KeyOperation Verify { get; } = new KeyOperation("verify");


        //
        // Summary:
        //     Gets a value that indicates the key can be used to wrap another key with the
        //     Azure.Security.KeyVault.Keys.Cryptography.CryptographyClient.WrapKeyAsync(Azure.Security.KeyVault.Keys.Cryptography.KeyWrapAlgorithm,System.Byte[],System.Threading.CancellationToken)
        //     or Azure.Security.KeyVault.Keys.Cryptography.CryptographyClient.WrapKey(Azure.Security.KeyVault.Keys.Cryptography.KeyWrapAlgorithm,System.Byte[],System.Threading.CancellationToken)
        //     methods.
        public static KeyOperation WrapKey { get; } = new KeyOperation("wrapKey");


        //
        // Summary:
        //     Gets a value that indicates the key can be used to unwrap another key with the
        //     Azure.Security.KeyVault.Keys.Cryptography.CryptographyClient.UnwrapKeyAsync(Azure.Security.KeyVault.Keys.Cryptography.KeyWrapAlgorithm,System.Byte[],System.Threading.CancellationToken)
        //     or Azure.Security.KeyVault.Keys.Cryptography.CryptographyClient.UnwrapKey(Azure.Security.KeyVault.Keys.Cryptography.KeyWrapAlgorithm,System.Byte[],System.Threading.CancellationToken)
        //     methods.
        public static KeyOperation UnwrapKey { get; } = new KeyOperation("unwrapKey");


        //
        // Summary:
        //     Initializes a new instance of the Azure.Security.KeyVault.Keys.KeyOperation structure.
        //
        // Parameters:
        //   value:
        //     The string value of the instance.
        public KeyOperation(string value)
        {
            _value = value ?? throw new ArgumentNullException("value");
        }

        //
        // Summary:
        //     Determines if two Azure.Security.KeyVault.Keys.KeyOperation values are the same.
        //
        // Parameters:
        //   left:
        //     The first Azure.Security.KeyVault.Keys.KeyOperation to compare.
        //
        //   right:
        //     The second Azure.Security.KeyVault.Keys.KeyOperation to compare.
        //
        // Returns:
        //     True if left and right are the same; otherwise, false.
        public static bool operator ==(KeyOperation left, KeyOperation right)
        {
            return left.Equals(right);
        }

        //
        // Summary:
        //     Determines if two Azure.Security.KeyVault.Keys.KeyOperation values are different.
        //
        // Parameters:
        //   left:
        //     The first Azure.Security.KeyVault.Keys.KeyOperation to compare.
        //
        //   right:
        //     The second Azure.Security.KeyVault.Keys.KeyOperation to compare.
        //
        // Returns:
        //     True if left and right are different; otherwise, false.
        public static bool operator !=(KeyOperation left, KeyOperation right)
        {
            return !left.Equals(right);
        }

        //
        // Summary:
        //     Converts a string to a Azure.Security.KeyVault.Keys.KeyOperation.
        //
        // Parameters:
        //   value:
        //     The string value to convert.
        public static implicit operator KeyOperation(string value)
        {
            return new KeyOperation(value);
        }

        //[EditorBrowsable(EditorBrowsableState.Never)]
        public override bool Equals(object obj)
        {
            if (obj is KeyOperation)
            {
                KeyOperation other = (KeyOperation)obj;
                return Equals(other);
            }

            return false;
        }

        public bool Equals(KeyOperation other)
        {
            return string.Equals(_value, other._value, StringComparison.Ordinal);
        }

        //[EditorBrowsable(EditorBrowsableState.Never)]
        public override int GetHashCode()
        {
            return _value?.GetHashCode() ?? 0;
        }

        public override string ToString()
        {
            return _value;
        }
    }

    //
    // Summary:
    //     Azure.Security.KeyVault.Keys.JsonWebKey key types.
    public readonly struct KeyType : IEquatable<KeyType>
    {
        private readonly string _value;

        //
        // Summary:
        //     An Elliptic Curve Cryptographic (ECC) algorithm.
        public static KeyType Ec { get; } = new KeyType("EC");


        //
        // Summary:
        //     An Elliptic Curve Cryptographic (ECC) algorithm backed by HSM.
        public static KeyType EcHsm { get; } = new KeyType("EC-HSM");


        //
        // Summary:
        //     An RSA cryptographic algorithm.
        public static KeyType Rsa { get; } = new KeyType("RSA");


        //
        // Summary:
        //     An RSA cryptographic algorithm backed by HSM.
        public static KeyType RsaHsm { get; } = new KeyType("RSA-HSM");


        //
        // Summary:
        //     An AES cryptographic algorithm.
        public static KeyType Oct { get; } = new KeyType("oct");


        //
        // Summary:
        //     Initializes a new instance of the Azure.Security.KeyVault.Keys.KeyType structure.
        //
        // Parameters:
        //   value:
        //     The string value of the instance.
        public KeyType(string value)
        {
            _value = value ?? throw new ArgumentNullException("value");
        }

        //
        // Summary:
        //     Determines if two Azure.Security.KeyVault.Keys.KeyType values are the same.
        //
        // Parameters:
        //   left:
        //     The first Azure.Security.KeyVault.Keys.KeyType to compare.
        //
        //   right:
        //     The second Azure.Security.KeyVault.Keys.KeyType to compare.
        //
        // Returns:
        //     True if left and right are the same; otherwise, false.
        public static bool operator ==(KeyType left, KeyType right)
        {
            return left.Equals(right);
        }

        //
        // Summary:
        //     Determines if two Azure.Security.KeyVault.Keys.KeyType values are different.
        //
        // Parameters:
        //   left:
        //     The first Azure.Security.KeyVault.Keys.KeyType to compare.
        //
        //   right:
        //     The second Azure.Security.KeyVault.Keys.KeyType to compare.
        //
        // Returns:
        //     True if left and right are different; otherwise, false.
        public static bool operator !=(KeyType left, KeyType right)
        {
            return !left.Equals(right);
        }

        //
        // Summary:
        //     Converts a string to a Azure.Security.KeyVault.Keys.KeyType.
        //
        // Parameters:
        //   value:
        //     The string value to convert.
        public static implicit operator KeyType(string value)
        {
            return new KeyType(value);
        }

        //[EditorBrowsable(EditorBrowsableState.Never)]
        //public override bool Equals(object obj)
        //{
        //    if (obj is KeyType)
        //    {
        //        KeyType other = (KeyType)obj;
        //        return Equals(other);
        //    }

        //    return false;
        //}

        public bool Equals(KeyType other)
        {
            return string.Equals(_value, other._value, StringComparison.OrdinalIgnoreCase);
        }

        //[EditorBrowsable(EditorBrowsableState.Never)]
        public override int GetHashCode()
        {
            return _value?.GetHashCode() ?? 0;
        }

        public override string ToString()
        {
            return _value;
        }

        public override bool Equals(object obj)
        {
            return obj is KeyType && Equals((KeyType)obj);
        }
    }

    public interface IJsonSerializable<TSelf> where TSelf : IJsonSerializable<TSelf>
    {
        static abstract TSelf Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options);
    }

    public interface IJsonDeserializable<TSelf> where TSelf : IJsonDeserializable<TSelf>
    {
        static abstract void Write(Utf8JsonWriter writer, TSelf value, JsonSerializerOptions options);
    }

    /// <summary>
    /// A key which is used to encrypt, or wrap, another key.
    /// </summary>
    public interface IKeyEncryptionKey
    {
        /// <summary>
        /// The Id of the key used to perform cryptographic operations for the client.
        /// </summary>
        string KeyId { get; }

        /// <summary>
        /// Encrypts the specified key using the specified algorithm.
        /// </summary>
        /// <param name="algorithm">The key wrap algorithm used to encrypt the specified key.</param>
        /// <param name="key">The key to be encrypted.</param>
        /// <param name="cancellationToken">A <see cref="CancellationToken"/> controlling the request lifetime.</param>
        /// <returns>The encrypted key bytes.</returns>
        byte[] WrapKey(string algorithm, ReadOnlyMemory<byte> key, CancellationToken cancellationToken = default);

        /// <summary>
        /// Encrypts the specified key using the specified algorithm.
        /// </summary>
        /// <param name="algorithm">The key wrap algorithm used to encrypt the specified key.</param>
        /// <param name="key">The key to be encrypted.</param>
        /// <param name="cancellationToken">A <see cref="CancellationToken"/> controlling the request lifetime.</param>
        /// <returns>The encrypted key bytes.</returns>
        Task<byte[]> WrapKeyAsync(string algorithm, ReadOnlyMemory<byte> key, CancellationToken cancellationToken = default);

        /// <summary>
        /// Decrypts the specified encrypted key using the specified algorithm.
        /// </summary>
        /// <param name="algorithm">The key wrap algorithm which was used to encrypt the specified encrypted key.</param>
        /// <param name="encryptedKey">The encrypted key to be decrypted.</param>
        /// <param name="cancellationToken">A <see cref="CancellationToken"/> controlling the request lifetime.</param>
        /// <returns>The decrypted key bytes.</returns>
        byte[] UnwrapKey(string algorithm, ReadOnlyMemory<byte> encryptedKey, CancellationToken cancellationToken = default);

        /// <summary>
        /// Decrypts the specified encrypted key using the specified algorithm.
        /// </summary>
        /// <param name="algorithm">The key wrap algorithm which was used to encrypt the specified encrypted key.</param>
        /// <param name="encryptedKey">The encrypted key to be decrypted.</param>
        /// <param name="cancellationToken">A <see cref="CancellationToken"/> controlling the request lifetime.</param>
        /// <returns>The decrypted key bytes.</returns>
        Task<byte[]> UnwrapKeyAsync(string algorithm, ReadOnlyMemory<byte> encryptedKey, CancellationToken cancellationToken = default);
    }

    //
    // Summary:
    //     A JSON Web Key (JWK) is a JavaScript Object Notation (JSON) data structure that
    //     represents a cryptographic key. For more information, see http://tools.ietf.org/html/draft-ietf-jose-json-web-key-18.
    //public class JsonWebKey //: IJsonDeserializable, IJsonSerializable
    //{
    //    private const string KeyIdPropertyName = "kid";
    //    private const string KeyTypePropertyName = "kty";
    //    private const string KeyOpsPropertyName = "key_ops";
    //    private const string CurveNamePropertyName = "crv";
    //    private const string NPropertyName = "n";
    //    private const string EPropertyName = "e";
    //    private const string DPPropertyName = "dp";
    //    private const string DQPropertyName = "dq";
    //    private const string QIPropertyName = "qi";
    //    private const string PPropertyName = "p";
    //    private const string QPropertyName = "q";
    //    private const string XPropertyName = "x";
    //    private const string YPropertyName = "y";
    //    private const string DPropertyName = "d";
    //    private const string KPropertyName = "k";
    //    private const string TPropertyName = "key_hsm";

    //    private static readonly JsonEncodedText s_keyTypePropertyNameBytes = JsonEncodedText.Encode("kty");
    //    private static readonly JsonEncodedText s_keyOpsPropertyNameBytes = JsonEncodedText.Encode("key_ops");
    //    private static readonly JsonEncodedText s_curveNamePropertyNameBytes = JsonEncodedText.Encode("crv");
    //    private static readonly JsonEncodedText s_nPropertyNameBytes = JsonEncodedText.Encode("n");
    //    private static readonly JsonEncodedText s_ePropertyNameBytes = JsonEncodedText.Encode("e");
    //    private static readonly JsonEncodedText s_dPPropertyNameBytes = JsonEncodedText.Encode("dp");
    //    private static readonly JsonEncodedText s_dQPropertyNameBytes = JsonEncodedText.Encode("dq");
    //    private static readonly JsonEncodedText s_qIPropertyNameBytes = JsonEncodedText.Encode("qi");
    //    private static readonly JsonEncodedText s_pPropertyNameBytes = JsonEncodedText.Encode("p");
    //    private static readonly JsonEncodedText s_qPropertyNameBytes = JsonEncodedText.Encode("q");
    //    private static readonly JsonEncodedText s_xPropertyNameBytes = JsonEncodedText.Encode("x");
    //    private static readonly JsonEncodedText s_yPropertyNameBytes = JsonEncodedText.Encode("y");
    //    private static readonly JsonEncodedText s_dPropertyNameBytes = JsonEncodedText.Encode("d");

    //    private static readonly JsonEncodedText s_kPropertyNameBytes = JsonEncodedText.Encode("k");

    //    private static readonly JsonEncodedText s_tPropertyNameBytes = JsonEncodedText.Encode("key_hsm");

    //    private static readonly KeyOperation[] s_aesKeyOperation = new KeyOperation[4]
    //    {
    //        KeyOperation.Encrypt,
    //        KeyOperation.Decrypt,
    //        KeyOperation.WrapKey,
    //        KeyOperation.UnwrapKey
    //    };

    //    private static readonly KeyOperation[] s_rSAPublicKeyOperation = new KeyOperation[3]
    //    {
    //        KeyOperation.Encrypt,
    //        KeyOperation.Verify,
    //        KeyOperation.WrapKey
    //    };

    //    private static readonly KeyOperation[] s_rSAPrivateKeyOperation = new KeyOperation[6]
    //    {
    //        KeyOperation.Encrypt,
    //        KeyOperation.Decrypt,
    //        KeyOperation.Sign,
    //        KeyOperation.Verify,
    //        KeyOperation.WrapKey,
    //        KeyOperation.UnwrapKey
    //    };

    //    private static readonly KeyOperation[] s_eCPublicKeyOperation = new KeyOperation[1] { KeyOperation.Sign };

    //    private static readonly KeyOperation[] s_eCPrivateKeyOperation = new KeyOperation[2]
    //    {
    //        KeyOperation.Sign,
    //        KeyOperation.Verify
    //    };

    //    private readonly IList<KeyOperation> _keyOps;

    //    private static readonly byte[] s_zeroBuffer = new byte[1];

    //    //
    //    // Summary:
    //    //     Gets the identifier of the key. This is not limited to a System.Uri.
    //    public string Id { get; internal set; }

    //    //
    //    // Summary:
    //    //     Gets the Azure.Security.KeyVault.Keys.JsonWebKey.KeyType for this Azure.Security.KeyVault.Keys.JsonWebKey.
    //    public KeyType KeyType { get; internal set; }

    //    //
    //    // Summary:
    //    //     Gets a list of Azure.Security.KeyVault.Keys.KeyOperation values supported by
    //    //     this key.
    //    public IReadOnlyCollection<KeyOperation> KeyOps { get; }

    //    //
    //    // Summary:
    //    //     Gets the RSA modulus.
    //    public byte[] N { get; internal set; }

    //    //
    //    // Summary:
    //    //     Gets RSA public exponent.
    //    public byte[] E { get; internal set; }

    //    //
    //    // Summary:
    //    //     Gets the RSA private key parameter.
    //    public byte[] DP { get; internal set; }

    //    //
    //    // Summary:
    //    //     Gets the RSA private key parameter.
    //    public byte[] DQ { get; internal set; }

    //    //
    //    // Summary:
    //    //     Gets the RSA private key parameter.
    //    public byte[] QI { get; internal set; }

    //    //
    //    // Summary:
    //    //     Gets the RSA secret prime.
    //    public byte[] P { get; internal set; }

    //    //
    //    // Summary:
    //    //     Gets the RSA secret prime.
    //    public byte[] Q { get; internal set; }

    //    //
    //    // Summary:
    //    //     Gets the name of the elliptical curve.
    //    public KeyCurveName? CurveName { get; internal set; }

    //    //
    //    // Summary:
    //    //     Gets the X coordinate of the elliptic curve point.
    //    public byte[] X { get; internal set; }

    //    //
    //    // Summary:
    //    //     Gets the Y coordinate for the elliptic curve point.
    //    public byte[] Y { get; internal set; }

    //    //
    //    // Summary:
    //    //     Gets the RSA private exponent or EC private key.
    //    public byte[] D { get; internal set; }

    //    //
    //    // Summary:
    //    //     Gets the symmetric key.
    //    public byte[] K { get; internal set; }

    //    //
    //    // Summary:
    //    //     Gets the HSM token used with "Bring Your Own Key".
    //    public byte[] T { get; internal set; }

    //    internal bool HasPrivateKey
    //    {
    //        get
    //        {
    //            if (KeyType == KeyType.Rsa || KeyType == KeyType.Ec || KeyType == KeyType.RsaHsm || KeyType == KeyType.EcHsm)
    //            {
    //                return D != null;
    //            }

    //            if (KeyType == KeyType.Oct)
    //            {
    //                return K != null;
    //            }

    //            return false;
    //        }
    //    }

    //    internal JsonWebKey()
    //        : this((IEnumerable<KeyOperation>)null)
    //    {
    //    }

    //    internal JsonWebKey(IEnumerable<KeyOperation> keyOps)
    //    {
    //        _keyOps = ((keyOps == null) ? new List<KeyOperation>() : new List<KeyOperation>(keyOps));
    //        KeyOps = new ReadOnlyCollection<KeyOperation>(_keyOps);
    //    }

    //    //
    //    // Summary:
    //    //     Initializes a new instance of the Azure.Security.KeyVault.Keys.JsonWebKey class
    //    //     using type Azure.Security.KeyVault.Keys.KeyType.Oct.
    //    //
    //    // Parameters:
    //    //   aesProvider:
    //    //     An System.Security.Cryptography.Aes provider.
    //    //
    //    //   keyOps:
    //    //     Optional list of supported Azure.Security.KeyVault.Keys.KeyOperation values.
    //    //     If null, the default for the key type is used, including: Azure.Security.KeyVault.Keys.KeyOperation.Encrypt,
    //    //     Azure.Security.KeyVault.Keys.KeyOperation.Decrypt, Azure.Security.KeyVault.Keys.KeyOperation.WrapKey,
    //    //     and Azure.Security.KeyVault.Keys.KeyOperation.UnwrapKey.
    //    //
    //    // Exceptions:
    //    //   T:System.ArgumentNullException:
    //    //     aesProvider is null.
    //    public JsonWebKey(Aes aesProvider, IEnumerable<KeyOperation> keyOps = null)
    //    {
    //        //Azure.Core.Argument.AssertNotNull(aesProvider, "aesProvider");
    //        _keyOps = new List<KeyOperation>(keyOps ?? s_aesKeyOperation);
    //        KeyOps = new ReadOnlyCollection<KeyOperation>(_keyOps);
    //        KeyType = KeyType.Oct;
    //        K = aesProvider.Key;
    //    }

    //    //
    //    // Summary:
    //    //     Initializes a new instance of the Azure.Security.KeyVault.Keys.JsonWebKey class
    //    //     using type Azure.Security.KeyVault.Keys.KeyType.Ec.
    //    //
    //    // Parameters:
    //    //   ecdsa:
    //    //     An System.Security.Cryptography.ECDsa provider.
    //    //
    //    //   includePrivateParameters:
    //    //     Whether to include the private key.
    //    //
    //    //   keyOps:
    //    //     Optional list of supported Azure.Security.KeyVault.Keys.KeyOperation values.
    //    //     If null, the default for the key type is used, including: Azure.Security.KeyVault.Keys.KeyOperation.Sign,
    //    //     and Azure.Security.KeyVault.Keys.KeyOperation.Decrypt if includePrivateParameters
    //    //     is true.
    //    //
    //    // Exceptions:
    //    //   T:System.ArgumentNullException:
    //    //     ecdsa is null.
    //    //
    //    //   T:System.InvalidOperationException:
    //    //     The elliptic curve name is invalid.
    //    public JsonWebKey(ECDsa ecdsa, bool includePrivateParameters = false, IEnumerable<KeyOperation> keyOps = null)
    //    {
    //        //Azure.Core.Argument.AssertNotNull(ecdsa, "ecdsa");
    //        _keyOps = new List<KeyOperation>(keyOps ?? (includePrivateParameters ? s_eCPrivateKeyOperation : s_eCPublicKeyOperation));
    //        KeyOps = new ReadOnlyCollection<KeyOperation>(_keyOps);
    //        Initialize(ecdsa, includePrivateParameters);
    //    }

    //    //
    //    // Summary:
    //    //     Initializes a new instance of the Azure.Security.KeyVault.Keys.JsonWebKey class
    //    //     using type Azure.Security.KeyVault.Keys.KeyType.Rsa.
    //    //
    //    // Parameters:
    //    //   rsaProvider:
    //    //     An System.Security.Cryptography.RSA provider.
    //    //
    //    //   includePrivateParameters:
    //    //     Whether to include the private key.
    //    //
    //    //   keyOps:
    //    //     Optional list of supported Azure.Security.KeyVault.Keys.KeyOperation values.
    //    //     If null, the default for the key type is used, including: Azure.Security.KeyVault.Keys.KeyOperation.Encrypt,
    //    //     Azure.Security.KeyVault.Keys.KeyOperation.Verify, and Azure.Security.KeyVault.Keys.KeyOperation.WrapKey;
    //    //     and Azure.Security.KeyVault.Keys.KeyOperation.Decrypt, Azure.Security.KeyVault.Keys.KeyOperation.Sign,
    //    //     and Azure.Security.KeyVault.Keys.KeyOperation.UnwrapKey if includePrivateParameters
    //    //     is true.
    //    //
    //    // Exceptions:
    //    //   T:System.ArgumentNullException:
    //    //     rsaProvider is null.
    //    public JsonWebKey(RSA rsaProvider, bool includePrivateParameters = false, IEnumerable<KeyOperation> keyOps = null)
    //    {
    //        //Azure.Core.Argument.AssertNotNull(rsaProvider, "rsaProvider");
    //        _keyOps = new List<KeyOperation>(keyOps ?? (includePrivateParameters ? s_rSAPrivateKeyOperation : s_rSAPublicKeyOperation));
    //        KeyOps = new ReadOnlyCollection<KeyOperation>(_keyOps);
    //        KeyType = KeyType.Rsa;
    //        RSAParameters rSAParameters = rsaProvider.ExportParameters(includePrivateParameters);
    //        E = rSAParameters.Exponent;
    //        N = rSAParameters.Modulus;
    //        D = rSAParameters.D;
    //        DP = rSAParameters.DP;
    //        DQ = rSAParameters.DQ;
    //        P = rSAParameters.P;
    //        Q = rSAParameters.Q;
    //        QI = rSAParameters.InverseQ;
    //    }

    //    //
    //    // Summary:
    //    //     Converts this Azure.Security.KeyVault.Keys.JsonWebKey of type Azure.Security.KeyVault.Keys.KeyType.Oct
    //    //     to an System.Security.Cryptography.Aes object.
    //    //
    //    // Returns:
    //    //     An System.Security.Cryptography.Aes object.
    //    //
    //    // Exceptions:
    //    //   T:System.InvalidOperationException:
    //    //     This key is not of type Azure.Security.KeyVault.Keys.KeyType.Oct or Azure.Security.KeyVault.Keys.JsonWebKey.K
    //    //     is null.
    //    public Aes ToAes()
    //    {
    //        if (KeyType != KeyType.Oct)
    //        {
    //            throw new InvalidOperationException("key is not an Oct key");
    //        }

    //        if (K == null)
    //        {
    //            throw new InvalidOperationException("key does not contain a value");
    //        }

    //        Aes aes = Aes.Create();
    //        aes.Key = K;
    //        return aes;
    //    }

    //    //
    //    // Summary:
    //    //     Converts this Azure.Security.KeyVault.Keys.JsonWebKey of type Azure.Security.KeyVault.Keys.KeyType.Ec
    //    //     or Azure.Security.KeyVault.Keys.KeyType.EcHsm to an System.Security.Cryptography.ECDsa
    //    //     object.
    //    //
    //    // Parameters:
    //    //   includePrivateParameters:
    //    //     Whether to include private parameters.
    //    //
    //    // Returns:
    //    //     An System.Security.Cryptography.ECDsa object.
    //    //
    //    // Exceptions:
    //    //   T:System.InvalidOperationException:
    //    //     This key is not of type Azure.Security.KeyVault.Keys.KeyType.Ec or Azure.Security.KeyVault.Keys.KeyType.EcHsm,
    //    //     or one or more key parameters are invalid.
    //    public ECDsa ToECDsa(bool includePrivateParameters = false)
    //    {
    //        return ToECDsa(includePrivateParameters, throwIfNotSupported: true);
    //    }

    //    internal ECDsa ToECDsa(bool includePrivateParameters, bool throwIfNotSupported)
    //    {
    //        if (KeyType != KeyType.Ec && KeyType != KeyType.EcHsm)
    //        {
    //            throw new InvalidOperationException("key is not an Ec or EcHsm type");
    //        }

    //        ValidateKeyParameter("X", X);
    //        ValidateKeyParameter("Y", Y);
    //        return Convert(includePrivateParameters, throwIfNotSupported);
    //    }

    //    //
    //    // Summary:
    //    //     Converts this Azure.Security.KeyVault.Keys.JsonWebKey of type Azure.Security.KeyVault.Keys.KeyType.Rsa
    //    //     or Azure.Security.KeyVault.Keys.KeyType.RsaHsm to an System.Security.Cryptography.RSA
    //    //     object.
    //    //
    //    // Parameters:
    //    //   includePrivateParameters:
    //    //     Whether to include private parameters.
    //    //
    //    // Returns:
    //    //     An System.Security.Cryptography.RSA object.
    //    //
    //    // Exceptions:
    //    //   T:System.InvalidOperationException:
    //    //     This key is not of type Azure.Security.KeyVault.Keys.KeyType.Rsa or Azure.Security.KeyVault.Keys.KeyType.RsaHsm,
    //    //     or one or more key parameters are invalid.
    //    public RSA ToRSA(bool includePrivateParameters = false)
    //    {
    //        if (KeyType != KeyType.Rsa && KeyType != KeyType.RsaHsm)
    //        {
    //            throw new InvalidOperationException("key is not an Rsa or RsaHsm type");
    //        }

    //        ValidateKeyParameter("E", E);
    //        ValidateKeyParameter("N", N);
    //        RSAParameters rSAParameters = default(RSAParameters);
    //        rSAParameters.Exponent = E;
    //        rSAParameters.Modulus = TrimBuffer(N);
    //        RSAParameters parameters = rSAParameters;
    //        if (includePrivateParameters && HasPrivateKey)
    //        {
    //            int num = parameters.Modulus!.Length;
    //            parameters.D = ForceBufferLength("D", D, num);
    //            num >>= 1;
    //            parameters.DP = ForceBufferLength("DP", DP, num);
    //            parameters.DQ = ForceBufferLength("DQ", DQ, num);
    //            parameters.P = ForceBufferLength("P", P, num);
    //            parameters.Q = ForceBufferLength("Q", Q, num);
    //            parameters.InverseQ = ForceBufferLength("QI", QI, num);
    //        }

    //        RSA rSA = RSA.Create();
    //        rSA.ImportParameters(parameters);
    //        return rSA;
    //    }

    //    internal void ReadProperties(JsonElement json)
    //    {
    //        foreach (JsonProperty item in json.EnumerateObject())
    //        {
    //            switch (item.Name)
    //            {
    //                case "kid":
    //                    Id = item.Value.GetString();
    //                    break;
    //                case "kty":
    //                    KeyType = item.Value.GetString();
    //                    break;
    //                case "key_ops":
    //                    foreach (JsonElement item2 in item.Value.EnumerateArray())
    //                    {
    //                        _keyOps.Add(item2.ToString());
    //                    }

    //                    break;
    //                case "crv":
    //                    CurveName = item.Value.GetString();
    //                    break;
    //                case "n":
    //                    N = Base64Url.Decode(item.Value.GetString());
    //                    break;
    //                case "e":
    //                    E = Base64Url.Decode(item.Value.GetString());
    //                    break;
    //                case "dp":
    //                    DP = Base64Url.Decode(item.Value.GetString());
    //                    break;
    //                case "dq":
    //                    DQ = Base64Url.Decode(item.Value.GetString());
    //                    break;
    //                case "qi":
    //                    QI = Base64Url.Decode(item.Value.GetString());
    //                    break;
    //                case "p":
    //                    P = Base64Url.Decode(item.Value.GetString());
    //                    break;
    //                case "q":
    //                    Q = Base64Url.Decode(item.Value.GetString());
    //                    break;
    //                case "x":
    //                    X = Base64Url.Decode(item.Value.GetString());
    //                    break;
    //                case "y":
    //                    Y = Base64Url.Decode(item.Value.GetString());
    //                    break;
    //                case "d":
    //                    D = Base64Url.Decode(item.Value.GetString());
    //                    break;
    //                case "k":
    //                    K = Base64Url.Decode(item.Value.GetString());
    //                    break;
    //                case "key_hsm":
    //                    T = Base64Url.Decode(item.Value.GetString());
    //                    break;
    //            }
    //        }
    //    }

    //    internal void WriteProperties(Utf8JsonWriter json)
    //    {
    //        if (KeyType != default(KeyType))
    //        {
    //            json.WriteString(s_keyTypePropertyNameBytes, KeyType.ToString());
    //        }

    //        if (KeyOps != null)
    //        {
    //            json.WriteStartArray(s_keyOpsPropertyNameBytes);
    //            foreach (KeyOperation keyOp in KeyOps)
    //            {
    //                json.WriteStringValue(keyOp.ToString());
    //            }

    //            json.WriteEndArray();
    //        }

    //        if (CurveName.HasValue)
    //        {
    //            json.WriteString(s_curveNamePropertyNameBytes, CurveName.Value.ToString());
    //        }

    //        if (N != null)
    //        {
    //            json.WriteString(s_nPropertyNameBytes, Base64Url.Encode(N));
    //        }

    //        if (E != null)
    //        {
    //            json.WriteString(s_ePropertyNameBytes, Base64Url.Encode(E));
    //        }

    //        if (DP != null)
    //        {
    //            json.WriteString(s_dPPropertyNameBytes, Base64Url.Encode(DP));
    //        }

    //        if (DQ != null)
    //        {
    //            json.WriteString(s_dQPropertyNameBytes, Base64Url.Encode(DQ));
    //        }

    //        if (QI != null)
    //        {
    //            json.WriteString(s_qIPropertyNameBytes, Base64Url.Encode(QI));
    //        }

    //        if (P != null)
    //        {
    //            json.WriteString(s_pPropertyNameBytes, Base64Url.Encode(P));
    //        }

    //        if (Q != null)
    //        {
    //            json.WriteString(s_qPropertyNameBytes, Base64Url.Encode(Q));
    //        }

    //        if (X != null)
    //        {
    //            json.WriteString(s_xPropertyNameBytes, Base64Url.Encode(X));
    //        }

    //        if (Y != null)
    //        {
    //            json.WriteString(s_yPropertyNameBytes, Base64Url.Encode(Y));
    //        }
    //        if (D != null)
    //        {
    //            json.WriteString(s_dPropertyNameBytes, Base64Url.Encode(D));
    //        }
    //        if (K != null)
    //        {
    //            json.WriteString(s_kPropertyNameBytes, Base64Url.Encode(K));
    //        }
    //        if (T != null)
    //        {
    //            json.WriteString(s_tPropertyNameBytes, Base64Url.Encode(T));
    //        }
    //    }
    //    //void ReadProperties(JsonElement json) // IJsonDeserializable
    //    //{
    //    //    ReadProperties(json);
    //    //}
    //    //void WriteProperties(Utf8JsonWriter json) // IJsonSerializable
    //    //{
    //    //    WriteProperties(json);
    //    //}
    //    private static byte[] ForceBufferLength(string name, byte[] value, int requiredLengthInBytes)
    //    {
    //        if (value == null || value.Length == 0)
    //        {
    //            throw new InvalidOperationException("key parameter " + name + " is null or empty");
    //        }
    //        if (value.Length == requiredLengthInBytes)
    //        {
    //            return value;
    //        }
    //        if (value.Length < requiredLengthInBytes)
    //        {
    //            byte[] array = new byte[requiredLengthInBytes];
    //            Array.Copy(value, 0, array, requiredLengthInBytes - value.Length, value.Length);
    //            return array;
    //        }
    //        int num = value.Length - requiredLengthInBytes;
    //        for (int i = 0; i < num; i++)
    //        {
    //            if (value[i] != 0)
    //            {
    //                throw new InvalidOperationException($"key parameter {name} is too long: expected at most {requiredLengthInBytes} bytes, but found {value.Length - i} bytes");
    //            }
    //        }
    //        byte[] array2 = new byte[requiredLengthInBytes];
    //        Array.Copy(value, value.Length - requiredLengthInBytes, array2, 0, requiredLengthInBytes);
    //        return array2;
    //    }
    //    private static byte[] TrimBuffer(byte[] value)
    //    {
    //        if (value == null || value.Length <= 1 || value[0] != 0)
    //        {
    //            return value;
    //        }
    //        for (int i = 1; i < value.Length; i++)
    //        {
    //            if (value[i] != 0)
    //            {
    //                byte[] array = new byte[value.Length - i];
    //                Array.Copy(value, i, array, 0, array.Length);
    //                return array;
    //            }
    //        }
    //        return s_zeroBuffer;
    //    }
    //    private static void ValidateKeyParameter(string name, byte[] value)
    //    {
    //        if (value != null)
    //        {
    //            for (int i = 0; i < value.Length; i++)
    //            {
    //                if (value[i] != 0)
    //                {
    //                    return;
    //                }
    //            }
    //        }
    //        throw new InvalidOperationException("key parameter " + name + " is null or zeros");
    //    }
    //    [MethodImpl(MethodImplOptions.NoInlining)]
    //    private void Initialize(ECDsa ecdsa, bool includePrivateParameters)
    //    {
    //        KeyType = KeyType.Ec;
    //        ECParameters eCParameters = ecdsa.ExportParameters(includePrivateParameters);
    //        CurveName = KeyCurveName.FromOid(eCParameters.Curve.Oid, ecdsa.KeySize).ToString() ?? throw new InvalidOperationException("elliptic curve name is invalid");
    //        D = eCParameters.D;
    //        X = eCParameters.Q.X;
    //        Y = eCParameters.Q.Y;
    //    }
    //    [MethodImpl(MethodImplOptions.NoInlining)]
    //    private ECDsa Convert(bool includePrivateParameters, bool throwIfNotSupported)
    //    {
    //        if (!CurveName.HasValue)
    //        {
    //            if (throwIfNotSupported)
    //            {
    //                throw new InvalidOperationException("missing required curve name");
    //            }
    //            return null;
    //        }
    //        KeyCurveName value = CurveName.Value;
    //        int keyParameterSize = value.KeyParameterSize;
    //        if (keyParameterSize <= 0)
    //        {
    //            if (throwIfNotSupported)
    //            {
    //                throw new InvalidOperationException("invalid curve name: " + CurveName.ToString());
    //            }
    //            return null;
    //        }
    //        ECParameters eCParameters = default(ECParameters);
    //        eCParameters.Curve = ECCurve.CreateFromOid(value.Oid);
    //        eCParameters.Q = new ECPoint
    //        {
    //            X = ForceBufferLength("X", X, keyParameterSize),
    //            Y = ForceBufferLength("Y", Y, keyParameterSize)
    //        };
    //        ECParameters parameters = eCParameters;
    //        if (includePrivateParameters && HasPrivateKey)
    //        {
    //            parameters.D = ForceBufferLength("D", D, keyParameterSize);
    //        }
    //        ECDsa eCDsa = ECDsa.Create();
    //        try
    //        {
    //            eCDsa.ImportParameters(parameters);
    //            return eCDsa;
    //        }
    //        catch when (!throwIfNotSupported)
    //        {
    //            eCDsa.Dispose();
    //            return null;
    //        }
    //    }
    //}


    public enum KeyVaultSignatureAlgorithm
    {
        RSAPkcs15,
        ECDsa
    }

    public static class SignatureAlgorithmTranslator
    {
        public static SignatureAlgorithm SignatureAlgorithmToJwsAlgId(KeyVaultSignatureAlgorithm signatureAlgorithm, HashAlgorithmName hashAlgorithmName)
        {
            switch (signatureAlgorithm)
            {
                case KeyVaultSignatureAlgorithm.RSAPkcs15:
                    if (hashAlgorithmName == HashAlgorithmName.SHA1)
                    {
                        return new SignatureAlgorithm("RSNULL");
                    }

                    if (hashAlgorithmName == HashAlgorithmName.SHA256)
                    {
                        return SignatureAlgorithm.RS256;
                    }

                    if (hashAlgorithmName == HashAlgorithmName.SHA384)
                    {
                        return SignatureAlgorithm.RS384;
                    }

                    if (hashAlgorithmName == HashAlgorithmName.SHA512)
                    {
                        return SignatureAlgorithm.RS512;
                    }

                    break;
                case KeyVaultSignatureAlgorithm.ECDsa:
                    if (hashAlgorithmName == HashAlgorithmName.SHA256)
                    {
                        return SignatureAlgorithm.ES256;
                    }

                    if (hashAlgorithmName == HashAlgorithmName.SHA384)
                    {
                        return SignatureAlgorithm.ES384;
                    }

                    if (hashAlgorithmName == HashAlgorithmName.SHA512)
                    {
                        return SignatureAlgorithm.ES512;
                    }

                    break;
            }

            throw new NotSupportedException("The algorithm specified is not supported.");
        }
    }

    //
    // Summary:
    //     An algorithm used for signing and verification.
    public readonly struct SignatureAlgorithm : IEquatable<SignatureAlgorithm>
    {
        internal const string RS256Value = "RS256";

        internal const string RS384Value = "RS384";

        internal const string RS512Value = "RS512";

        internal const string PS256Value = "PS256";

        internal const string PS384Value = "PS384";

        internal const string PS512Value = "PS512";

        internal const string ES256Value = "ES256";

        internal const string ES384Value = "ES384";

        internal const string ES512Value = "ES512";

        internal const string ES256KValue = "ES256K";

        private readonly string _value;

        //
        // Summary:
        //     Gets an RSA SHA-256 Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm.
        public static SignatureAlgorithm RS256 { get; } = new SignatureAlgorithm("RS256");


        //
        // Summary:
        //     Gets an RSA SHA-384 Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm.
        public static SignatureAlgorithm RS384 { get; } = new SignatureAlgorithm("RS384");


        //
        // Summary:
        //     Gets an RSA SHA-512 Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm.
        public static SignatureAlgorithm RS512 { get; } = new SignatureAlgorithm("RS512");


        //
        // Summary:
        //     Gets an RSASSA-PSS using SHA-256 and MGF1 with SHA-256 Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm.
        public static SignatureAlgorithm PS256 { get; } = new SignatureAlgorithm("PS256");


        //
        // Summary:
        //     Gets an RSASSA-PSS using SHA-384 and MGF1 with SHA-384 Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm.
        public static SignatureAlgorithm PS384 { get; } = new SignatureAlgorithm("PS384");


        //
        // Summary:
        //     Gets an RSASSA-PSS using SHA-512 and MGF1 with SHA-512 Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm.
        public static SignatureAlgorithm PS512 { get; } = new SignatureAlgorithm("PS512");


        //
        // Summary:
        //     Gets an ECDSA with a P-256 curve Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm.
        public static SignatureAlgorithm ES256 { get; } = new SignatureAlgorithm("ES256");


        //
        // Summary:
        //     Gets an ECDSA with a P-384 curve Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm.
        public static SignatureAlgorithm ES384 { get; } = new SignatureAlgorithm("ES384");


        //
        // Summary:
        //     Gets an ECDSA with a P-521 curve Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm.
        public static SignatureAlgorithm ES512 { get; } = new SignatureAlgorithm("ES512");


        //
        // Summary:
        //     Gets an ECDSA with a secp256k1 curve Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm.
        public static SignatureAlgorithm ES256K { get; } = new SignatureAlgorithm("ES256K");


        //
        // Summary:
        //     Initializes a new instance of the Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm
        //     structure.
        //
        // Parameters:
        //   value:
        //     The string value of the instance.
        public SignatureAlgorithm(string value)
        {
            _value = value ?? throw new ArgumentNullException("value");
        }

        //
        // Summary:
        //     Determines if two Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm
        //     values are the same.
        //
        // Parameters:
        //   left:
        //     The first Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm to compare.
        //
        //   right:
        //     The second Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm to compare.
        //
        // Returns:
        //     True if left and right are the same; otherwise, false.
        public static bool operator ==(SignatureAlgorithm left, SignatureAlgorithm right)
        {
            return left.Equals(right);
        }

        //
        // Summary:
        //     Determines if two Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm
        //     values are different.
        //
        // Parameters:
        //   left:
        //     The first Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm to compare.
        //
        //   right:
        //     The second Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm to compare.
        //
        // Returns:
        //     True if left and right are different; otherwise, false.
        public static bool operator !=(SignatureAlgorithm left, SignatureAlgorithm right)
        {
            return !left.Equals(right);
        }

        //
        // Summary:
        //     Converts a string to a Azure.Security.KeyVault.Keys.Cryptography.SignatureAlgorithm.
        //
        // Parameters:
        //   value:
        //     The string value to convert.
        public static implicit operator SignatureAlgorithm(string value)
        {
            return new SignatureAlgorithm(value);
        }

        //[EditorBrowsable(EditorBrowsableState.Never)]
        public override bool Equals(object obj)
        {
            if (obj is SignatureAlgorithm)
            {
                SignatureAlgorithm other = (SignatureAlgorithm)obj;
                return Equals(other);
            }

            return false;
        }

        public bool Equals(SignatureAlgorithm other)
        {
            return string.Equals(_value, other._value, StringComparison.Ordinal);
        }

        //[EditorBrowsable(EditorBrowsableState.Never)]
        public override int GetHashCode()
        {
            return _value?.GetHashCode() ?? 0;
        }

        public override string ToString()
        {
            return _value;
        }

        internal HashAlgorithm GetHashAlgorithm()
        {
            switch (_value)
            {
                case "RS256":
                case "PS256":
                case "ES256":
                case "ES256K":
                    return SHA256.Create();
                case "RS384":
                case "PS384":
                case "ES384":
                    return SHA384.Create();
                case "RS512":
                case "PS512":
                case "ES512":
                    return SHA512.Create();
                default:
                    throw new InvalidOperationException("Invalid Algorithm");
            }
        }

        internal HashAlgorithmName GetHashAlgorithmName()
        {
            switch (_value)
            {
                case "RS256":
                case "PS256":
                case "ES256":
                case "ES256K":
                    return HashAlgorithmName.SHA256;
                case "RS384":
                case "PS384":
                case "ES384":
                    return HashAlgorithmName.SHA384;
                case "RS512":
                case "PS512":
                case "ES512":
                    return HashAlgorithmName.SHA512;
                default:
                    return default(HashAlgorithmName);
            }
        }

        internal KeyCurveName GetEcKeyCurveName()
        {
            return _value switch
            {
                "ES256" => KeyCurveName.P256,
                "ES256K" => KeyCurveName.P256K,
                "ES384" => KeyCurveName.P384,
                "ES512" => KeyCurveName.P521,
                _ => KeyCurveName.s_default,
            };
        }

        internal RSASignaturePadding GetRsaSignaturePadding()
        {
            switch (_value)
            {
                case "RS256":
                case "RS384":
                case "RS512":
                    return RSASignaturePadding.Pkcs1;
                case "PS256":
                case "PS384":
                case "PS512":
                    return RSASignaturePadding.Pss;
                default:
                    return null;
            }
        }
    }

    //
    // Summary:
    //     Elliptic Curve Cryptography (ECC) curve names.
    public readonly struct KeyCurveName : IEquatable<KeyCurveName>
    {
        private const string P256Value = "P-256";

        private const string P256KValue = "P-256K";

        private const string P384Value = "P-384";

        private const string P521Value = "P-521";

        private const string P256OidValue = "1.2.840.10045.3.1.7";

        private const string P256KOidValue = "1.3.132.0.10";

        private const string P384OidValue = "1.3.132.0.34";

        private const string P521OidValue = "1.3.132.0.35";

        private readonly string _value;

        internal static readonly KeyCurveName s_default = default(KeyCurveName);

        //
        // Summary:
        //     Gets the NIST P-256 elliptic curve, AKA SECG curve SECP256R1 For more information,
        //     see https://docs.microsoft.com/en-us/azure/key-vault/about-keys-secrets-and-certificates#curve-types.
        public static KeyCurveName P256 { get; } = new KeyCurveName("P-256");


        //
        // Summary:
        //     Gets the SECG SECP256K1 elliptic curve. For more information, see https://docs.microsoft.com/en-us/azure/key-vault/about-keys-secrets-and-certificates#curve-types.
        public static KeyCurveName P256K { get; } = new KeyCurveName("P-256K");


        //
        // Summary:
        //     Gets the NIST P-384 elliptic curve, AKA SECG curve SECP384R1. For more information,
        //     see https://docs.microsoft.com/en-us/azure/key-vault/about-keys-secrets-and-certificates#curve-types.
        public static KeyCurveName P384 { get; } = new KeyCurveName("P-384");


        //
        // Summary:
        //     Gets the NIST P-521 elliptic curve, AKA SECG curve SECP521R1. For more information,
        //     see https://docs.microsoft.com/en-us/azure/key-vault/about-keys-secrets-and-certificates#curve-types.
        public static KeyCurveName P521 { get; } = new KeyCurveName("P-521");


        internal bool IsSupported
        {
            get
            {
                switch (_value)
                {
                    case "P-256":
                    case "P-256K":
                    case "P-384":
                    case "P-521":
                        return true;
                    default:
                        return false;
                }
            }
        }

        internal int KeyParameterSize => _value switch
        {
            "P-256" => 32,
            "P-256K" => 32,
            "P-384" => 48,
            "P-521" => 66,
            _ => 0,
        };

        internal int KeySize => _value switch
        {
            "P-256" => 256,
            "P-256K" => 256,
            "P-384" => 384,
            "P-521" => 521,
            _ => 0,
        };

        internal Oid Oid => _value switch
        {
            "P-256" => new Oid("1.2.840.10045.3.1.7"),
            "P-256K" => new Oid("1.3.132.0.10"),
            "P-384" => new Oid("1.3.132.0.34"),
            "P-521" => new Oid("1.3.132.0.35"),
            _ => null,
        };

        //
        // Summary:
        //     Initializes a new instance of the Azure.Security.KeyVault.Keys.KeyCurveName structure.
        //
        // Parameters:
        //   value:
        //     The string value of the instance.
        public KeyCurveName(string value)
        {
            _value = value ?? throw new ArgumentNullException("value");
        }

        //
        // Summary:
        //     Determines if two Azure.Security.KeyVault.Keys.KeyCurveName values are the same.
        //
        // Parameters:
        //   left:
        //     The first Azure.Security.KeyVault.Keys.KeyCurveName to compare.
        //
        //   right:
        //     The second Azure.Security.KeyVault.Keys.KeyCurveName to compare.
        //
        // Returns:
        //     True if left and right are the same; otherwise, false.
        public static bool operator ==(KeyCurveName left, KeyCurveName right)
        {
            return left.Equals(right);
        }

        //
        // Summary:
        //     Determines if two Azure.Security.KeyVault.Keys.KeyCurveName values are different.
        //
        // Parameters:
        //   left:
        //     The first Azure.Security.KeyVault.Keys.KeyCurveName to compare.
        //
        //   right:
        //     The second Azure.Security.KeyVault.Keys.KeyCurveName to compare.
        //
        // Returns:
        //     True if left and right are different; otherwise, false.
        public static bool operator !=(KeyCurveName left, KeyCurveName right)
        {
            return !left.Equals(right);
        }

        //
        // Summary:
        //     Converts a string to a Azure.Security.KeyVault.Keys.KeyCurveName.
        //
        // Parameters:
        //   value:
        //     The string value to convert.
        public static implicit operator KeyCurveName(string value)
        {
            return new KeyCurveName(value);
        }

        //[EditorBrowsable(EditorBrowsableState.Never)]
        public override bool Equals(object obj)
        {
            if (obj is KeyCurveName)
            {
                KeyCurveName other = (KeyCurveName)obj;
                return Equals(other);
            }

            return false;
        }

        public bool Equals(KeyCurveName other)
        {
            return string.Equals(_value, other._value, StringComparison.Ordinal);
        }

        //[EditorBrowsable(EditorBrowsableState.Never)]
        public override int GetHashCode()
        {
            return _value?.GetHashCode() ?? 0;
        }

        public override string ToString()
        {
            return _value;
        }

        internal static KeyCurveName FromOid(Oid oid, int keySize = 0)
        {
            if (!string.IsNullOrEmpty(oid?.Value))
            {
                if (string.Equals(oid.Value, "1.3.132.0.35", StringComparison.Ordinal))
                {
                    return P521;
                }

                if (string.Equals(oid.Value, "1.3.132.0.34", StringComparison.Ordinal))
                {
                    return P384;
                }

                if (string.Equals(oid.Value, "1.3.132.0.10", StringComparison.Ordinal))
                {
                    return P256K;
                }

                if (string.Equals(oid.Value, "1.2.840.10045.3.1.7", StringComparison.Ordinal))
                {
                    return P256;
                }
            }

            if (!string.IsNullOrEmpty(oid?.FriendlyName))
            {
                switch (keySize)
                {
                    case 521:
                        if (string.Equals(oid.FriendlyName, "nistP521", StringComparison.OrdinalIgnoreCase) || string.Equals(oid.FriendlyName, "secp521r1", StringComparison.OrdinalIgnoreCase) || string.Equals(oid.FriendlyName, "ECDSA_P521", StringComparison.OrdinalIgnoreCase))
                        {
                            return P521;
                        }

                        break;
                    case 384:
                        if (string.Equals(oid.FriendlyName, "nistP384", StringComparison.OrdinalIgnoreCase) || string.Equals(oid.FriendlyName, "secp384r1", StringComparison.OrdinalIgnoreCase) || string.Equals(oid.FriendlyName, "ECDSA_P384", StringComparison.OrdinalIgnoreCase))
                        {
                            return P384;
                        }

                        break;
                    case 256:
                        if (string.Equals(oid.FriendlyName, "secp256k1", StringComparison.OrdinalIgnoreCase))
                        {
                            return P256K;
                        }

                        if (string.Equals(oid.FriendlyName, "nistP256", StringComparison.OrdinalIgnoreCase) || string.Equals(oid.FriendlyName, "secp256r1", StringComparison.OrdinalIgnoreCase) || string.Equals(oid.FriendlyName, "ECDSA_P256", StringComparison.OrdinalIgnoreCase))
                        {
                            return P256;
                        }

                        break;
                }
            }

            return s_default;
        }
    }

}
