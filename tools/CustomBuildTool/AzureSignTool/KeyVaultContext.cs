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
    /// <summary>
    /// A signing context used for signing packages with Azure Key Vault Keys.
    /// </summary>
    public readonly struct KeyVaultContext
    {
        /// <summary>
        /// The access token used to authenticate with Azure Key Vault.
        /// </summary>
        private readonly string AccessToken;

        /// <summary>
        /// Gets the certificate used to validate the signature. May be null if
        /// Key isn't part of a certificate
        /// </summary>
        public X509Certificate2 Certificate { get; }

        /// <summary>
        /// Identifier of current key
        /// </summary>
        public Uri KeyIdentifier { get; }

        /// <summary>
        /// Returns true if properly constructed. If default, then false.
        /// </summary>
        public bool IsValid => !string.IsNullOrEmpty(this.AccessToken) && this.KeyIdentifier != null;

        /// <summary>
        /// Creates a new Key Vault context using an access token and a public certificate.
        /// </summary>
        public KeyVaultContext(string AccessToken, Uri KeyId, X509Certificate2 PublicCertificate)
        {
            ArgumentNullException.ThrowIfNull(AccessToken);
            ArgumentNullException.ThrowIfNull(KeyId);
            ArgumentNullException.ThrowIfNull(PublicCertificate);

            this.AccessToken = AccessToken;
            this.Certificate = PublicCertificate;
            this.KeyIdentifier = KeyId;
        }

        /// <summary>
        /// Signs a digest using the specified hash algorithm and signature algorithm.
        /// </summary>
        /// <param name="Digest">The message digest to sign.</param>
        /// <param name="HashAlgorithm">The hash algorithm used to create the digest.</param>
        /// <param name="SignatureAlgorithm">The signature algorithm to use.</param>
        /// <returns>The signature of the digest.</returns>
        internal byte[] SignDigest(byte[] Digest, HashAlgorithmName HashAlgorithm, KeyVaultSignatureAlgorithm SignatureAlgorithm)
        {
            if (HashAlgorithm == HashAlgorithmName.SHA1)
            {
                throw new InvalidOperationException("SHA1 algorithm is not supported for this signature algorithm.");
            }

            string signatureAlgorithmId = SignatureAlgorithmTranslator.SignatureAlgorithmToJwsAlgId(SignatureAlgorithm, HashAlgorithm);
            using KeyVaultRestClient keyVaultClient = new KeyVaultRestClient(this.AccessToken);

            // Block on the async call to provide a synchronous API expected by callers.
            return keyVaultClient.Sign(Digest, signatureAlgorithmId, this.KeyIdentifier, CancellationToken.None).GetAwaiter().GetResult();
        }

        /// <summary>
        /// Verifies a signature over a digest using the specified hash algorithm and signature algorithm.
        /// </summary>
        /// <param name="Digest">The message digest that was signed.</param>
        /// <param name="Signature">The signature to verify.</param>
        /// <param name="HashAlgorithm">The hash algorithm used to create the digest.</param>
        /// <param name="SignatureAlgorithm">The signature algorithm to use.</param>
        /// <returns>True if the signature is valid; otherwise false.</returns>
        internal bool VerifyDigest(byte[] Digest, byte[] Signature, HashAlgorithmName HashAlgorithm, KeyVaultSignatureAlgorithm SignatureAlgorithm)
        {
            if (HashAlgorithm == HashAlgorithmName.SHA1)
            {
                throw new InvalidOperationException("SHA1 algorithm is not supported for this signature algorithm.");
            }

            string signatureAlgorithmId = SignatureAlgorithmTranslator.SignatureAlgorithmToJwsAlgId(SignatureAlgorithm, HashAlgorithm);
            using KeyVaultRestClient keyVaultClient = new KeyVaultRestClient(this.AccessToken);

            // Block on the async call to provide a synchronous API expected by callers.
            return keyVaultClient.Verify(Digest, Signature, signatureAlgorithmId, this.KeyIdentifier, CancellationToken.None).GetAwaiter().GetResult();
        }

        /// <summary>
        /// Converts an <see cref="RSAEncryptionPadding"/> instance to its corresponding JWS algorithm identifier.
        /// </summary>
        /// <param name="Padding">The RSA encryption padding.</param>
        /// <returns>The JWS algorithm identifier as a string.</returns>
        /// <exception cref="NotSupportedException">Thrown when the specified padding is not supported.</exception>
        internal static string EncryptionPaddingToJwsAlgId(RSAEncryptionPadding Padding)
        {
            return Padding.Mode switch
            {
                RSAEncryptionPaddingMode.Pkcs1 => "RSA1_5",
                RSAEncryptionPaddingMode.Oaep when Padding.OaepHashAlgorithm == HashAlgorithmName.SHA1 => "RSA-OAEP",
                RSAEncryptionPaddingMode.Oaep when Padding.OaepHashAlgorithm == HashAlgorithmName.SHA256 => "RSA-OAEP-256",
                _ => throw new NotSupportedException("The padding specified is not supported.")
            };
        }

        /// <summary>
        /// Decrypts cipher text using the specified RSA encryption padding.
        /// </summary>
        /// <param name="CipherText">The cipher text to decrypt.</param>
        /// <param name="Padding">The RSA encryption padding used.</param>
        /// <returns>The decrypted plain text data.</returns>
        internal byte[] DecryptData(byte[] CipherText, RSAEncryptionPadding Padding)
        {
            string encryptionAlgorithmId = EncryptionPaddingToJwsAlgId(Padding);
            using KeyVaultRestClient keyVaultClient = new KeyVaultRestClient(this.AccessToken);

            // Block on the async call to provide a synchronous API expected by callers.
            return keyVaultClient.Decrypt(CipherText, encryptionAlgorithmId, this.KeyIdentifier, CancellationToken: CancellationToken.None).GetAwaiter().GetResult();
        }

        /// <summary>
        /// Encrypts plain text using the specified RSA encryption padding.
        /// </summary>
        /// <param name="PlainText">The plain text data to encrypt.</param>
        /// <param name="Padding">The RSA encryption padding to use.</param>
        /// <returns>The encrypted cipher text data.</returns>
        internal byte[] EncryptData(byte[] PlainText, RSAEncryptionPadding Padding)
        {
            string encryptionAlgorithmId = EncryptionPaddingToJwsAlgId(Padding);
            using KeyVaultRestClient keyVaultClient = new KeyVaultRestClient(this.AccessToken);

            // Block on the async call to provide a synchronous API expected by callers.
            return keyVaultClient.Encrypt(PlainText, encryptionAlgorithmId, this.KeyIdentifier, CancellationToken: CancellationToken.None).GetAwaiter().GetResult();
        }

        /// <summary>
        /// Wraps key material using the specified RSA encryption padding.
        /// </summary>
        /// <param name="Key">The key material to wrap.</param>
        /// <param name="Padding">The RSA encryption padding to use.</param>
        /// <returns>The wrapped key material.</returns>
        internal byte[] WrapKey(byte[] Key, RSAEncryptionPadding Padding)
        {
            string encryptionAlgorithmId = EncryptionPaddingToJwsAlgId(Padding);
            using KeyVaultRestClient keyVaultClient = new KeyVaultRestClient(this.AccessToken);

            // Block on the async call to provide a synchronous API expected by callers.
            return keyVaultClient.WrapKey(Key, encryptionAlgorithmId, this.KeyIdentifier, CancellationToken: CancellationToken.None).GetAwaiter().GetResult();
        }

        /// <summary>
        /// Unwraps key material using the specified RSA encryption padding.
        /// </summary>
        /// <param name="WrappedKey">The wrapped key material.</param>
        /// <param name="Padding">The RSA encryption padding to use.</param>
        /// <returns>The unwrapped key material.</returns>
        internal byte[] UnwrapKey(byte[] WrappedKey, RSAEncryptionPadding Padding)
        {
            string encryptionAlgorithmId = EncryptionPaddingToJwsAlgId(Padding);
            using KeyVaultRestClient keyVaultClient = new KeyVaultRestClient(this.AccessToken);

            // Block on the async call to provide a synchronous API expected by callers.
            return keyVaultClient.UnwrapKey(WrappedKey, encryptionAlgorithmId, this.KeyIdentifier, CancellationToken: CancellationToken.None).GetAwaiter().GetResult();
        }
    }

    /// <summary>
    /// Minimal client for Azure Key Vault cryptography operations.
    /// Provides async Sign/Decrypt helpers and creates HttpClient instances as needed.
    /// </summary>
    internal sealed class KeyVaultRestClient : IDisposable
    {
        private readonly HttpClient HttpClient;
        private readonly string AccessToken;

        public KeyVaultRestClient(string AccessToken)
        {
            this.HttpClient = BuildHttpClient.CreateHttpClient();
            this.AccessToken = AccessToken ?? throw new ArgumentNullException(nameof(AccessToken));
        }

        public void Dispose()
        {
            this.HttpClient?.Dispose();
        }

        /// <summary>
        /// Gets the public part and metadata of a stored key.
        /// </summary>
        public Task<KeyVaultKeyResponse> GetKey(Uri KeyId, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

            return this.SendJson(
                HttpMethod.Get,
                KeyId.ToString().TrimEnd('/') + "?api-version=2025-07-01",
                null,
                AzureJsonContext.Default.KeyVaultKeyResponse,
                CancellationToken
                );
        }

        /// <summary>
        /// Lists keys in a vault.
        /// </summary>
        public Task<KeyVaultKeyListResponse> GetKeys(Uri VaultBaseUrl, int? MaxResults = null, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(VaultBaseUrl, nameof(VaultBaseUrl));

            return this.SendJson(
                HttpMethod.Get,
                CreateVaultRequestUrl(VaultBaseUrl, "keys", MaxResults),
                null,
                AzureJsonContext.Default.KeyVaultKeyListResponse,
                CancellationToken
                );
        }

        /// <summary>
        /// Lists versions of a stored key.
        /// </summary>
        public Task<KeyVaultKeyListResponse> GetKeyVersions(Uri KeyId, int? MaxResults = null, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

            return this.SendJson(
                HttpMethod.Get,
                CreateKeyRequestUrl(KeyId, "versions", MaxResults),
                null,
                AzureJsonContext.Default.KeyVaultKeyListResponse,
                CancellationToken
                );
        }

        /// <summary>
        /// Creates a new key in a vault.
        /// </summary>
        public Task<KeyVaultKeyResponse> CreateKey(Uri VaultBaseUrl, string KeyName, KeyVaultKeyCreateRequest Request, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(VaultBaseUrl, nameof(VaultBaseUrl));
            ArgumentException.ThrowIfNullOrWhiteSpace(KeyName, nameof(KeyName));
            ArgumentNullException.ThrowIfNull(Request, nameof(Request));

            return this.SendJson(
                HttpMethod.Post,
                VaultBaseUrl.ToString().TrimEnd('/') + $"/keys/{Uri.EscapeDataString(KeyName)}/create?api-version=2025-07-01",
                CreateJsonContent(Request, AzureJsonContext.Default.KeyVaultKeyCreateRequest),
                AzureJsonContext.Default.KeyVaultKeyResponse,
                CancellationToken
                );
        }

        /// <summary>
        /// Imports externally created key material into a vault.
        /// </summary>
        public Task<KeyVaultKeyResponse> ImportKey(Uri VaultBaseUrl, string KeyName, KeyVaultKeyImportRequest Request, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(VaultBaseUrl, nameof(VaultBaseUrl));
            ArgumentException.ThrowIfNullOrWhiteSpace(KeyName, nameof(KeyName));
            ArgumentNullException.ThrowIfNull(Request, nameof(Request));

            return this.SendJson(
                HttpMethod.Put,
                VaultBaseUrl.ToString().TrimEnd('/') + $"/keys/{Uri.EscapeDataString(KeyName)}?api-version=2025-07-01",
                CreateJsonContent(Request, AzureJsonContext.Default.KeyVaultKeyImportRequest),
                AzureJsonContext.Default.KeyVaultKeyResponse,
                CancellationToken
                );
        }

        /// <summary>
        /// Updates attributes, operations, tags, or release policy for a stored key.
        /// </summary>
        public Task<KeyVaultKeyResponse> UpdateKey(Uri KeyId, KeyVaultKeyUpdateRequest Request, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));
            ArgumentNullException.ThrowIfNull(Request, nameof(Request));

            return this.SendJson(
                HttpMethod.Patch,
                KeyId.ToString().TrimEnd('/') + "?api-version=2025-07-01",
                CreateJsonContent(Request, AzureJsonContext.Default.KeyVaultKeyUpdateRequest),
                AzureJsonContext.Default.KeyVaultKeyResponse,
                CancellationToken
                );
        }

        /// <summary>
        /// Deletes a key from a vault.
        /// </summary>
        public Task<KeyVaultKeyResponse> DeleteKey(Uri VaultBaseUrl, string KeyName, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(VaultBaseUrl, nameof(VaultBaseUrl));
            ArgumentException.ThrowIfNullOrWhiteSpace(KeyName, nameof(KeyName));

            return this.SendJson(
                HttpMethod.Delete,
                VaultBaseUrl.ToString().TrimEnd('/') + $"/keys/{Uri.EscapeDataString(KeyName)}?api-version=2025-07-01",
                null,
                AzureJsonContext.Default.KeyVaultKeyResponse,
                CancellationToken
                );
        }

        /// <summary>
        /// Gets a deleted key from a vault.
        /// </summary>
        public Task<KeyVaultKeyResponse> GetDeletedKey(Uri VaultBaseUrl, string KeyName, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(VaultBaseUrl, nameof(VaultBaseUrl));
            ArgumentException.ThrowIfNullOrWhiteSpace(KeyName, nameof(KeyName));

            return this.SendJson(
                HttpMethod.Get,
                VaultBaseUrl.ToString().TrimEnd('/') + $"/deletedkeys/{Uri.EscapeDataString(KeyName)}?api-version=2025-07-01",
                null,
                AzureJsonContext.Default.KeyVaultKeyResponse,
                CancellationToken
                );
        }

        /// <summary>
        /// Lists deleted keys in a vault.
        /// </summary>
        public Task<KeyVaultKeyListResponse> GetDeletedKeys(Uri VaultBaseUrl, int? MaxResults = null, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(VaultBaseUrl, nameof(VaultBaseUrl));

            return this.SendJson(
                HttpMethod.Get,
                CreateVaultRequestUrl(VaultBaseUrl, "deletedkeys", MaxResults),
                null,
                AzureJsonContext.Default.KeyVaultKeyListResponse,
                CancellationToken
                );
        }

        /// <summary>
        /// Permanently deletes a deleted key.
        /// </summary>
        public Task PurgeDeletedKey(Uri VaultBaseUrl, string KeyName, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(VaultBaseUrl, nameof(VaultBaseUrl));
            ArgumentException.ThrowIfNullOrWhiteSpace(KeyName, nameof(KeyName));

            return this.Send(
                HttpMethod.Delete,
                VaultBaseUrl.ToString().TrimEnd('/') + $"/deletedkeys/{Uri.EscapeDataString(KeyName)}?api-version=2025-07-01",
                null,
                CancellationToken
                );
        }

        /// <summary>
        /// Recovers a deleted key to its latest version.
        /// </summary>
        public Task<KeyVaultKeyResponse> RecoverDeletedKey(Uri VaultBaseUrl, string KeyName, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(VaultBaseUrl, nameof(VaultBaseUrl));
            ArgumentException.ThrowIfNullOrWhiteSpace(KeyName, nameof(KeyName));

            return this.SendJson(
                HttpMethod.Post,
                VaultBaseUrl.ToString().TrimEnd('/') + $"/deletedkeys/{Uri.EscapeDataString(KeyName)}/recover?api-version=2025-07-01",
                null,
                AzureJsonContext.Default.KeyVaultKeyResponse,
                CancellationToken
                );
        }

        /// <summary>
        /// Backs up a key in protected Key Vault backup format.
        /// </summary>
        public async Task<byte[]> BackupKey(Uri KeyId, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

            KeyVaultBackupResponse backupResponse = null;

            try
            {
                backupResponse = await this.SendJson(
                    HttpMethod.Post,
                    KeyId.ToString().TrimEnd('/') + "/backup?api-version=2025-07-01",
                    null,
                    AzureJsonContext.Default.KeyVaultBackupResponse,
                    CancellationToken
                    );

                return Base64UrlDecode(backupResponse.Value);
            }
            finally
            {
                backupResponse?.ClearSensitiveData();
            }
        }

        /// <summary>
        /// Restores a backed up key into a vault.
        /// </summary>
        public Task<KeyVaultKeyResponse> RestoreKey(Uri VaultBaseUrl, byte[] BackupBlob, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(VaultBaseUrl, nameof(VaultBaseUrl));
            ArgumentNullException.ThrowIfNull(BackupBlob, nameof(BackupBlob));

            KeyVaultRestoreRequest request = new KeyVaultRestoreRequest
            {
                Value = Base64UrlEncode(BackupBlob)
            };

            return this.SendJson(
                HttpMethod.Post,
                VaultBaseUrl.ToString().TrimEnd('/') + "/keys/restore?api-version=2025-07-01",
                CreateJsonContent(request, AzureJsonContext.Default.KeyVaultRestoreRequest),
                AzureJsonContext.Default.KeyVaultKeyResponse,
                CancellationToken
                );
        }

        /// <summary>
        /// Releases exportable key material to an attested target.
        /// </summary>
        public Task<KeyVaultKeyReleaseResponse> ReleaseKey(Uri KeyId, KeyVaultKeyReleaseRequest Request, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));
            ArgumentNullException.ThrowIfNull(Request, nameof(Request));

            return this.SendJson(
                HttpMethod.Post,
                KeyId.ToString().TrimEnd('/') + "/release?api-version=2025-07-01",
                CreateJsonContent(Request, AzureJsonContext.Default.KeyVaultKeyReleaseRequest),
                AzureJsonContext.Default.KeyVaultKeyReleaseResponse,
                CancellationToken
                );
        }

        /// <summary>
        /// Rotates a key by creating a new version based on its rotation policy.
        /// </summary>
        public Task<KeyVaultKeyResponse> RotateKey(Uri KeyId, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

            return this.SendJson(
                HttpMethod.Post,
                KeyId.ToString().TrimEnd('/') + "/rotate?api-version=2025-07-01",
                null,
                AzureJsonContext.Default.KeyVaultKeyResponse,
                CancellationToken
                );
        }

        /// <summary>
        /// Gets the rotation policy for a key.
        /// </summary>
        public Task<KeyVaultKeyRotationPolicy> GetKeyRotationPolicy(Uri KeyId, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

            return this.SendJson(
                HttpMethod.Get,
                KeyId.ToString().TrimEnd('/') + "/rotationpolicy?api-version=2025-07-01",
                null,
                AzureJsonContext.Default.KeyVaultKeyRotationPolicy,
                CancellationToken
                );
        }

        /// <summary>
        /// Updates the rotation policy for a key.
        /// </summary>
        public Task<KeyVaultKeyRotationPolicy> UpdateKeyRotationPolicy(Uri KeyId, KeyVaultKeyRotationPolicy Policy, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));
            ArgumentNullException.ThrowIfNull(Policy, nameof(Policy));

            return this.SendJson(
                HttpMethod.Put,
                KeyId.ToString().TrimEnd('/') + "/rotationpolicy?api-version=2025-07-01",
                CreateJsonContent(Policy, AzureJsonContext.Default.KeyVaultKeyRotationPolicy),
                AzureJsonContext.Default.KeyVaultKeyRotationPolicy,
                CancellationToken
                );
        }

        /// <summary>
        /// Signs the specified digest using the given algorithm and key identifier, returning the resulting signature.
        /// </summary>
        /// <remarks>The signing operation is performed remotely using the specified key in Azure Key
        /// Vault. The method ensures the request succeeds and throws an exception if the operation fails. The caller is
        /// responsible for providing a digest compatible with the chosen algorithm.</remarks>
        /// <param name="Digest">The digest to be signed. Must not be null.</param>
        /// <param name="Algorithm">The name of the signing algorithm to use. Must not be null. Supported algorithms depend on the key type and
        /// configuration.</param>
        /// <param name="KeyId">The identifier of the key to use for signing. Must not be null.</param>
        /// <param name="CancellationToken">A cancellation token that can be used to cancel the operation.</param>
        /// <returns>A byte array containing the signature of the digest. The array will contain the signature in binary form.</returns>
        public async Task<byte[]> Sign(byte[] Digest, string Algorithm, Uri KeyId, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(Digest, nameof(Digest));
            ArgumentNullException.ThrowIfNull(Algorithm, nameof(Algorithm));
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

            try
            {
                KeyVaultOperationRequest operationRequest = new KeyVaultOperationRequest
                {
                    Algorithm = Algorithm,
                    Value = Base64UrlEncode(Digest)
                };

                KeyVaultOperationResponse operationResponse = await this.PostOperation(
                    KeyId,
                    "sign",
                    operationRequest,
                    CancellationToken
                    );
                char[] signatureValue = operationResponse?.Value ?? operationResponse?.Signature;
                ArgumentNullException.ThrowIfNull(signatureValue, nameof(signatureValue));

                try
                {
                    return Base64UrlDecode(signatureValue);
                }
                finally
                {
                    operationResponse?.ClearSensitiveData();
                }
            }
            catch (Exception)
            {
                Program.PrintColorMessage($"[StaticNativeSignDigestCallback] Exception", ConsoleColor.Red);
                return null;
            }
        }

        /// <summary>
        /// Verifies a signature over the specified digest using the given algorithm and key identifier.
        /// </summary>
        /// <param name="Digest">The digest that was signed. Must not be null.</param>
        /// <param name="Signature">The signature to verify. Must not be null.</param>
        /// <param name="Algorithm">The signing algorithm used to create the signature. Must not be null.</param>
        /// <param name="KeyId">The identifier of the key to use for verification. Must not be null.</param>
        /// <param name="CancellationToken">A cancellation token that can be used to cancel the operation.</param>
        /// <returns>True if the signature is valid; otherwise false.</returns>
        public async Task<bool> Verify(byte[] Digest, byte[] Signature, string Algorithm, Uri KeyId, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(Digest, nameof(Digest));
            ArgumentNullException.ThrowIfNull(Signature, nameof(Signature));
            ArgumentNullException.ThrowIfNull(Algorithm, nameof(Algorithm));
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

            try
            {
                KeyVaultVerifyRequest operationRequest = new KeyVaultVerifyRequest
                {
                    Algorithm = Algorithm,
                    Digest = Base64UrlEncode(Digest),
                    Signature = Base64UrlEncode(Signature)
                };

                KeyVaultVerifyResponse operationResponse = await this.PostVerifyOperation(
                    KeyId,
                    operationRequest,
                    CancellationToken
                    );

                return operationResponse.Value;
            }
            catch (Exception)
            {
                Program.PrintColorMessage($"[KeyVaultRestClient.Verify] Exception", ConsoleColor.Red);
                return false;
            }
        }

        /// <summary>
        /// Encrypts data using the specified algorithm and key identifier.
        /// </summary>
        /// <param name="PlainText">The plaintext to encrypt. Must not be null.</param>
        /// <param name="Algorithm">The encryption algorithm to use. Must not be null.</param>
        /// <param name="KeyId">The identifier of the key to use for encryption. Must not be null.</param>
        /// <param name="CancellationToken">A cancellation token that can be used to cancel the operation.</param>
        /// <returns>A byte array containing the encrypted ciphertext.</returns>
        public async Task<byte[]> Encrypt(
            byte[] PlainText,
            string Algorithm,
            Uri KeyId,
            byte[] AdditionalAuthenticatedData = null,
            byte[] InitializationVector = null,
            CancellationToken CancellationToken = default
            )
        {
            ArgumentNullException.ThrowIfNull(PlainText, nameof(PlainText));
            ArgumentNullException.ThrowIfNull(Algorithm, nameof(Algorithm));
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

            try
            {
                KeyVaultOperationResponse operationResponse = await this.EncryptOperation(
                    PlainText,
                    Algorithm,
                    KeyId,
                    AdditionalAuthenticatedData,
                    InitializationVector,
                    CancellationToken
                    );
                char[] cipherTextValue = operationResponse?.Value;
                ArgumentNullException.ThrowIfNull(cipherTextValue, nameof(cipherTextValue));

                try
                {
                    return Base64UrlDecode(cipherTextValue);
                }
                finally
                {
                    operationResponse?.ClearSensitiveData();
                }
            }
            catch (Exception)
            {
                Program.PrintColorMessage($"[KeyVaultRestClient.Encrypt] Exception", ConsoleColor.Red);
                return null;
            }
        }

        /// <summary>
        /// Encrypts data and returns the full Key Vault operation response including authenticated encryption fields.
        /// </summary>
        public Task<KeyVaultOperationResponse> EncryptOperation(
            byte[] PlainText,
            string Algorithm,
            Uri KeyId,
            byte[] AdditionalAuthenticatedData = null,
            byte[] InitializationVector = null,
            CancellationToken CancellationToken = default
            )
        {
            ArgumentNullException.ThrowIfNull(PlainText, nameof(PlainText));
            ArgumentNullException.ThrowIfNull(Algorithm, nameof(Algorithm));
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

            KeyVaultOperationRequest operationRequest = new KeyVaultOperationRequest
            {
                Algorithm = Algorithm,
                Value = Base64UrlEncode(PlainText),
                AdditionalAuthenticatedData = Base64UrlEncodeOrNull(AdditionalAuthenticatedData),
                InitializationVector = Base64UrlEncodeOrNull(InitializationVector)
            };

            return this.PostOperation(
                KeyId,
                "encrypt",
                operationRequest,
                CancellationToken
                );
        }

        /// <summary>
        /// Decrypts the specified ciphertext using the provided algorithm and key identifier.
        /// </summary>
        /// <remarks>This method sends a request to the specified key vault endpoint to perform the
        /// decryption operation. The operation is asynchronous and may be cancelled using the provided cancellation
        /// token.</remarks>
        /// <param name="Cipher">The ciphertext to decrypt, as a byte array. Cannot be null.</param>
        /// <param name="Algorithm">The name of the decryption algorithm to use. Cannot be null or whitespace.</param>
        /// <param name="KeyId">The URI identifying the key to use for decryption. Cannot be null.</param>
        /// <param name="CancellationToken">A cancellation token that can be used to cancel the operation.</param>
        /// <returns>A byte array containing the decrypted plaintext. The array will contain the original data if decryption
        /// succeeds.</returns>
        /// <exception cref="ArgumentNullException">Thrown if <paramref name="Cipher"/>, <paramref name="Algorithm"/>, or <paramref name="KeyId"/> is null.</exception>
        public async Task<byte[]> Decrypt(
            byte[] Cipher,
            string Algorithm,
            Uri KeyId,
            byte[] AdditionalAuthenticatedData = null,
            byte[] InitializationVector = null,
            byte[] AuthenticationTag = null,
            CancellationToken CancellationToken = default
            )
        {
            ArgumentNullException.ThrowIfNull(Cipher, nameof(Cipher));
            ArgumentNullException.ThrowIfNull(Algorithm, nameof(Algorithm));
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

            try
            {
                KeyVaultOperationResponse operationResponse = await this.DecryptOperation(
                    Cipher,
                    Algorithm,
                    KeyId,
                    AdditionalAuthenticatedData,
                    InitializationVector,
                    AuthenticationTag,
                    CancellationToken
                    );
                char[] plaintextValue = operationResponse?.Plaintext ?? operationResponse?.Value;
                ArgumentNullException.ThrowIfNull(plaintextValue, nameof(plaintextValue));

                try
                {
                    return Base64UrlDecode(plaintextValue);
                }
                finally
                {
                    operationResponse?.ClearSensitiveData();
                }
            }
            catch (Exception)
            {
                Program.PrintColorMessage($"[StaticNativeSignDigestCallback] Exception", ConsoleColor.Red);
                return null;
            }
        }

        /// <summary>
        /// Decrypts data and returns the full Key Vault operation response including authenticated encryption fields.
        /// </summary>
        public Task<KeyVaultOperationResponse> DecryptOperation(
            byte[] Cipher,
            string Algorithm,
            Uri KeyId,
            byte[] AdditionalAuthenticatedData = null,
            byte[] InitializationVector = null,
            byte[] AuthenticationTag = null,
            CancellationToken CancellationToken = default
            )
        {
            ArgumentNullException.ThrowIfNull(Cipher, nameof(Cipher));
            ArgumentNullException.ThrowIfNull(Algorithm, nameof(Algorithm));
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

            KeyVaultOperationRequest operationRequest = new KeyVaultOperationRequest
            {
                Algorithm = Algorithm,
                Value = Base64UrlEncode(Cipher),
                AdditionalAuthenticatedData = Base64UrlEncodeOrNull(AdditionalAuthenticatedData),
                InitializationVector = Base64UrlEncodeOrNull(InitializationVector),
                AuthenticationTag = Base64UrlEncodeOrNull(AuthenticationTag)
            };

            return this.PostOperation(
                KeyId,
                "decrypt",
                operationRequest,
                CancellationToken
                );
        }

        /// <summary>
        /// Wraps key material using the specified algorithm and key identifier.
        /// </summary>
        /// <param name="Key">The key material to wrap. Must not be null.</param>
        /// <param name="Algorithm">The key wrapping algorithm to use. Must not be null.</param>
        /// <param name="KeyId">The identifier of the key encryption key. Must not be null.</param>
        /// <param name="CancellationToken">A cancellation token that can be used to cancel the operation.</param>
        /// <returns>A byte array containing the wrapped key material.</returns>
        public async Task<byte[]> WrapKey(
            byte[] Key,
            string Algorithm,
            Uri KeyId,
            byte[] AdditionalAuthenticatedData = null,
            byte[] InitializationVector = null,
            CancellationToken CancellationToken = default
            )
        {
            ArgumentNullException.ThrowIfNull(Key, nameof(Key));
            ArgumentNullException.ThrowIfNull(Algorithm, nameof(Algorithm));
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

            try
            {
                KeyVaultOperationResponse operationResponse = await this.WrapKeyOperation(
                    Key,
                    Algorithm,
                    KeyId,
                    AdditionalAuthenticatedData,
                    InitializationVector,
                    CancellationToken
                    );
                char[] wrappedKeyValue = operationResponse?.Value;
                ArgumentNullException.ThrowIfNull(wrappedKeyValue, nameof(wrappedKeyValue));

                try
                {
                    return Base64UrlDecode(wrappedKeyValue);
                }
                finally
                {
                    operationResponse?.ClearSensitiveData();
                }
            }
            catch (Exception)
            {
                Program.PrintColorMessage($"[KeyVaultRestClient.WrapKey] Exception", ConsoleColor.Red);
                return null;
            }
        }

        /// <summary>
        /// Wraps key material and returns the full Key Vault operation response including authenticated encryption fields.
        /// </summary>
        public Task<KeyVaultOperationResponse> WrapKeyOperation(
            byte[] Key,
            string Algorithm,
            Uri KeyId,
            byte[] AdditionalAuthenticatedData = null,
            byte[] InitializationVector = null,
            CancellationToken CancellationToken = default
            )
        {
            ArgumentNullException.ThrowIfNull(Key, nameof(Key));
            ArgumentNullException.ThrowIfNull(Algorithm, nameof(Algorithm));
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

            KeyVaultOperationRequest operationRequest = new KeyVaultOperationRequest
            {
                Algorithm = Algorithm,
                Value = Base64UrlEncode(Key),
                AdditionalAuthenticatedData = Base64UrlEncodeOrNull(AdditionalAuthenticatedData),
                InitializationVector = Base64UrlEncodeOrNull(InitializationVector)
            };

            return this.PostOperation(
                KeyId,
                "wrapkey",
                operationRequest,
                CancellationToken
                );
        }

        /// <summary>
        /// Unwraps key material using the specified algorithm and key identifier.
        /// </summary>
        /// <param name="WrappedKey">The wrapped key material. Must not be null.</param>
        /// <param name="Algorithm">The key wrapping algorithm to use. Must not be null.</param>
        /// <param name="KeyId">The identifier of the key encryption key. Must not be null.</param>
        /// <param name="CancellationToken">A cancellation token that can be used to cancel the operation.</param>
        /// <returns>A byte array containing the unwrapped key material.</returns>
        public async Task<byte[]> UnwrapKey(
            byte[] WrappedKey,
            string Algorithm,
            Uri KeyId,
            byte[] AdditionalAuthenticatedData = null,
            byte[] InitializationVector = null,
            byte[] AuthenticationTag = null,
            CancellationToken CancellationToken = default
            )
        {
            ArgumentNullException.ThrowIfNull(WrappedKey, nameof(WrappedKey));
            ArgumentNullException.ThrowIfNull(Algorithm, nameof(Algorithm));
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

            try
            {
                KeyVaultOperationResponse operationResponse = await this.UnwrapKeyOperation(
                    WrappedKey,
                    Algorithm,
                    KeyId,
                    AdditionalAuthenticatedData,
                    InitializationVector,
                    AuthenticationTag,
                    CancellationToken
                    );
                char[] unwrappedKeyValue = operationResponse?.Value;
                ArgumentNullException.ThrowIfNull(unwrappedKeyValue, nameof(unwrappedKeyValue));

                try
                {
                    return Base64UrlDecode(unwrappedKeyValue);
                }
                finally
                {
                    operationResponse?.ClearSensitiveData();
                }
            }
            catch (Exception)
            {
                Program.PrintColorMessage($"[KeyVaultRestClient.UnwrapKey] Exception", ConsoleColor.Red);
                return null;
            }
        }

        /// <summary>
        /// Unwraps key material and returns the full Key Vault operation response including authenticated encryption fields.
        /// </summary>
        public Task<KeyVaultOperationResponse> UnwrapKeyOperation(
            byte[] WrappedKey,
            string Algorithm,
            Uri KeyId,
            byte[] AdditionalAuthenticatedData = null,
            byte[] InitializationVector = null,
            byte[] AuthenticationTag = null,
            CancellationToken CancellationToken = default
            )
        {
            ArgumentNullException.ThrowIfNull(WrappedKey, nameof(WrappedKey));
            ArgumentNullException.ThrowIfNull(Algorithm, nameof(Algorithm));
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

            KeyVaultOperationRequest operationRequest = new KeyVaultOperationRequest
            {
                Algorithm = Algorithm,
                Value = Base64UrlEncode(WrappedKey),
                AdditionalAuthenticatedData = Base64UrlEncodeOrNull(AdditionalAuthenticatedData),
                InitializationVector = Base64UrlEncodeOrNull(InitializationVector),
                AuthenticationTag = Base64UrlEncodeOrNull(AuthenticationTag)
            };

            return this.PostOperation(
                KeyId,
                "unwrapkey",
                operationRequest,
                CancellationToken
                );
        }

        /// <summary>
        /// Sends a POST request to the specified Key Vault operation endpoint and returns the deserialized response.
        /// </summary>
        /// <param name="KeyId">The URI identifying the Key Vault key to operate on.</param>
        /// <param name="Operation">The name of the operation to perform on the Key Vault key.</param>
        /// <param name="OperationRequest">The request payload containing parameters for the operation.</param>
        /// <param name="CancellationToken">A cancellation token that can be used to cancel the operation.</param>
        /// <returns>A task that represents the asynchronous operation. The task result contains the response from the Key Vault
        /// operation.</returns>
        private async Task<KeyVaultOperationResponse> PostOperation(
            Uri KeyId,
            string Operation,
            KeyVaultOperationRequest OperationRequest,
            CancellationToken CancellationToken
            )
        {
            string requestUrl = KeyId.ToString().TrimEnd('/') + $"/{Operation}?api-version=2025-07-01";

            using HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Post, requestUrl);
            requestMessage.Content = CreateJsonContent(OperationRequest, AzureJsonContext.Default.KeyVaultOperationRequest);
            requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Bearer", this.AccessToken);

            using HttpResponseMessage responseMessage = await this.HttpClient.SendAsync(requestMessage, HttpCompletionOption.ResponseContentRead, CancellationToken);
            responseMessage.EnsureSuccessStatusCode();

            Stream responseStream = await responseMessage.Content.ReadAsStreamAsync(CancellationToken);
            ArgumentNullException.ThrowIfNull(responseStream, nameof(responseStream));

            KeyVaultOperationResponse keyVaultOperationResponse = await JsonSerializer.DeserializeAsync(responseStream, AzureJsonContext.Default.KeyVaultOperationResponse, CancellationToken);
            ArgumentNullException.ThrowIfNull(keyVaultOperationResponse, nameof(keyVaultOperationResponse));

            return keyVaultOperationResponse;
        }

        /// <summary>
        /// Sends a verify operation request to Azure Key Vault and returns the verification response.
        /// </summary>
        /// <param name="KeyId">The URI of the Key Vault key to use for the verify operation.</param>
        /// <param name="OperationRequest">The request payload containing the parameters for the verify operation.</param>
        /// <param name="CancellationToken">A cancellation token that can be used to cancel the operation.</param>
        /// <returns>A task that represents the asynchronous operation. The task result contains the response from the Key Vault
        /// verify operation.</returns>
        private async Task<KeyVaultVerifyResponse> PostVerifyOperation(
            Uri KeyId,
            KeyVaultVerifyRequest OperationRequest,
            CancellationToken CancellationToken
            )
        {
            string requestUrl = KeyId.ToString().TrimEnd('/') + "/verify?api-version=2025-07-01";

            using HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Post, requestUrl);
            requestMessage.Content = CreateJsonContent(OperationRequest, AzureJsonContext.Default.KeyVaultVerifyRequest);
            requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Bearer", this.AccessToken);

            using HttpResponseMessage responseMessage = await this.HttpClient.SendAsync(requestMessage, HttpCompletionOption.ResponseContentRead, CancellationToken);
            responseMessage.EnsureSuccessStatusCode();

            Stream responseStream = await responseMessage.Content.ReadAsStreamAsync(CancellationToken);
            ArgumentNullException.ThrowIfNull(responseStream, nameof(responseStream));

            KeyVaultVerifyResponse keyVaultVerifyResponse = await JsonSerializer.DeserializeAsync(responseStream, AzureJsonContext.Default.KeyVaultVerifyResponse, CancellationToken);
            ArgumentNullException.ThrowIfNull(keyVaultVerifyResponse, nameof(keyVaultVerifyResponse));

            return keyVaultVerifyResponse;
        }

        /// <summary>
        /// Sends an HTTP request using the specified method, URL, and content, and ensures a successful response status
        /// code.
        /// </summary>
        /// <remarks>The request includes a bearer token for authorization. An exception is thrown if the
        /// response indicates an unsuccessful status code.</remarks>
        /// <param name="Method">The HTTP method to use for the request, such as GET or POST.</param>
        /// <param name="RequestUrl">The URL to which the HTTP request is sent.</param>
        /// <param name="Content">The HTTP content to include in the request body. Can be null for methods that do not send a body.</param>
        /// <param name="CancellationToken">A cancellation token that can be used to cancel the request operation.</param>
        /// <returns>A task that represents the asynchronous send operation.</returns>
        private async Task Send(
            HttpMethod Method,
            string RequestUrl,
            HttpContent Content,
            CancellationToken CancellationToken
            )
        {
            using HttpRequestMessage requestMessage = new HttpRequestMessage(Method, RequestUrl);
            requestMessage.Content = Content;
            requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Bearer", this.AccessToken);

            using HttpResponseMessage responseMessage = await this.HttpClient.SendAsync(requestMessage, HttpCompletionOption.ResponseContentRead, CancellationToken);
            responseMessage.EnsureSuccessStatusCode();
        }

        /// <summary>
        /// Sends an HTTP request with the specified method and content, and deserializes the JSON response to the
        /// specified type.
        /// </summary>
        /// <remarks>The request includes a Bearer token for authorization. The method throws an exception
        /// if the HTTP response indicates failure or if deserialization fails.</remarks>
        /// <typeparam name="TResponse">The type to which the JSON response will be deserialized.</typeparam>
        /// <param name="Method">The HTTP method to use for the request, such as GET or POST.</param>
        /// <param name="RequestUrl">The URL to which the HTTP request is sent.</param>
        /// <param name="Content">The HTTP content to include in the request body. Can be null for methods that do not require a body.</param>
        /// <param name="ResponseTypeInfo">The metadata describing the type to use for deserializing the JSON response.</param>
        /// <param name="CancellationToken">A cancellation token that can be used to cancel the request.</param>
        /// <returns>A task that represents the asynchronous operation. The task result contains the deserialized response of
        /// type TResponse.</returns>
        private async Task<TResponse> SendJson<TResponse>(
            HttpMethod Method,
            string RequestUrl,
            HttpContent Content,
            JsonTypeInfo<TResponse> ResponseTypeInfo,
            CancellationToken CancellationToken
            )
        {
            using HttpRequestMessage requestMessage = new HttpRequestMessage(Method, RequestUrl);
            requestMessage.Content = Content;
            requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Bearer", this.AccessToken);

            using HttpResponseMessage responseMessage = await this.HttpClient.SendAsync(requestMessage, HttpCompletionOption.ResponseContentRead, CancellationToken);
            responseMessage.EnsureSuccessStatusCode();

            Stream responseStream = await responseMessage.Content.ReadAsStreamAsync(CancellationToken);
            ArgumentNullException.ThrowIfNull(responseStream, nameof(responseStream));

            TResponse response = await JsonSerializer.DeserializeAsync(responseStream, ResponseTypeInfo, CancellationToken);
            ArgumentNullException.ThrowIfNull(response, nameof(response));

            return response;
        }

        /// <summary>
        /// Creates an HTTP content object containing the JSON representation of the specified request object.
        /// </summary>
        /// <typeparam name="TRequest">The type of the request object to serialize to JSON.</typeparam>
        /// <param name="Request">The request object to serialize as JSON. Cannot be null.</param>
        /// <param name="RequestTypeInfo">The metadata used to control JSON serialization for the request object. Cannot be null.</param>
        /// <returns>An HttpContent instance that serializes the request object to the target stream as UTF-8 JSON.</returns>
        private static HttpContent CreateJsonContent<TRequest>(
            TRequest Request,
            JsonTypeInfo<TRequest> RequestTypeInfo
            )
        {
            return new KeyVaultJsonContent<TRequest>(Request, RequestTypeInfo);
        }

        /// <summary>
        /// Constructs a request URL for accessing a vault resource, including the specified API version and optional
        /// maximum results parameter.
        /// </summary>
        /// <param name="VaultBaseUrl">The base URI of the vault service. Must not be null and should not include a trailing slash.</param>
        /// <param name="Path">The relative path to the specific vault resource to be accessed.</param>
        /// <param name="MaxResults">The maximum number of results to request. If null, the parameter is omitted from the URL.</param>
        /// <returns>A string containing the fully constructed request URL with the specified path, API version, and optional
        /// maximum results parameter.</returns>
        private static string CreateVaultRequestUrl(Uri VaultBaseUrl, string Path, int? MaxResults)
        {
            string requestUrl = VaultBaseUrl.ToString().TrimEnd('/') + $"/{Path}?api-version=2025-07-01";

            if (MaxResults.HasValue)
            {
                requestUrl += $"&maxresults={MaxResults.Value}";
            }

            return requestUrl;
        }

        /// <summary>
        /// Constructs a key request URL using the specified key identifier, path, and optional maximum results
        /// parameter.
        /// </summary>
        /// <remarks>The resulting URL always includes the 'api-version=2025-07-01' query parameter. If
        /// <paramref name="MaxResults"/> is provided, the URL will also include a 'maxresults' query parameter with its
        /// value.</remarks>
        /// <param name="KeyId">The base URI representing the key identifier. Cannot be null.</param>
        /// <param name="Path">The relative path to append to the key identifier in the request URL.</param>
        /// <param name="MaxResults">The optional maximum number of results to include in the request. If specified, adds a 'maxresults' query
        /// parameter to the URL.</param>
        /// <returns>A string containing the constructed key request URL with the specified parameters.</returns>
        private static string CreateKeyRequestUrl(Uri KeyId, string Path, int? MaxResults)
        {
            string requestUrl = KeyId.ToString().TrimEnd('/') + $"/{Path}?api-version=2025-07-01";

            if (MaxResults.HasValue)
            {
                requestUrl += $"&maxresults={MaxResults.Value}";
            }

            return requestUrl;
        }

        /// <summary>
        /// Encodes the specified byte array into a Base64 URL-safe string without padding.
        /// </summary>
        /// <remarks>This method produces a Base64-encoded string that is safe for use in URLs and
        /// filenames by replacing '+' with '-', '/' with '_', and omitting padding characters. The output conforms to
        /// RFC 4648, section 5.</remarks>
        /// <param name="Data">The byte array to encode as a Base64 URL-safe string. Cannot be null.</param>
        /// <returns>A Base64 URL-safe encoded string representation of the input data, with padding characters removed.</returns>
        private static char[] Base64UrlEncode(byte[] Data)
        {
            int base64Length = ((Data.Length + 2) / 3) * 4;
            char[] base64Chars = new char[base64Length];

            if (!Convert.TryToBase64Chars(Data, base64Chars, out int charsWritten))
            {
                throw new InvalidOperationException("Unable to encode data as Base64.");
            }

            while (charsWritten != 0 && base64Chars[charsWritten - 1] == '=')
            {
                charsWritten--;
            }

            for (int i = 0; i < charsWritten; i++)
            {
                if (base64Chars[i] == '+')
                {
                    base64Chars[i] = '-';
                }
                else if (base64Chars[i] == '/')
                {
                    base64Chars[i] = '_';
                }
            }

            if (charsWritten != base64Chars.Length)
            {
                Array.Resize(ref base64Chars, charsWritten);
            }

            return base64Chars;
        }

        private static char[] Base64UrlEncodeOrNull(byte[] Data)
        {
            if (Data == null)
            {
                return null;
            }

            return Base64UrlEncode(Data);
        }

        /// <summary>
        /// Decodes a Base64URL-encoded string into its corresponding byte array.
        /// </summary>
        /// <remarks>This method handles Base64URL encoding as defined in RFC 4648, Section 5, by
        /// converting URL-safe characters to standard Base64 and adding necessary padding. The input string must be a
        /// valid Base64URL-encoded value; otherwise, a FormatException may be thrown.</remarks>
        /// <param name="Text">The Base64URL-encoded string to decode. The string must use '-' and '_' in place of '+' and '/' and may omit
        /// padding characters.</param>
        /// <returns>A byte array containing the decoded data represented by the input string.</returns>
        private static byte[] Base64UrlDecode(char[] Text)
        {
            int base64Length = Text.Length;

            switch (base64Length % 4)
            {
            case 2:
                base64Length += 2;
                break;
            case 3:
                base64Length++;
                break;
            }

            char[] base64Chars = new char[base64Length];

            for (int i = 0; i < Text.Length; i++)
            {
                if (Text[i] == '-')
                {
                    base64Chars[i] = '+';
                }
                else if (Text[i] == '_')
                {
                    base64Chars[i] = '/';
                }
                else
                {
                    base64Chars[i] = Text[i];
                }
            }

            for (int i = Text.Length; i < base64Length; i++)
            {
                base64Chars[i] = '=';
            }

            return Convert.FromBase64CharArray(base64Chars, 0, base64Chars.Length);
        }
    }

    /// <summary>
    /// Converts character arrays representing base64url-encoded key material to and from JSON string values for use
    /// with Azure Key Vault integration.
    /// </summary>
    /// <remarks>This converter enables secure handling of sensitive key material by using mutable character
    /// arrays instead of immutable strings, allowing callers to clear the contents after use and reduce the risk of
    /// leaving sensitive data in memory. It is intended for internal use when serializing or deserializing
    /// base64url-encoded secrets or keys in JSON payloads.</remarks>
    internal sealed class KeyVaultBase64UrlCharArrayJsonConverter : JsonConverter<char[]>
    {
        // Keep base64url key material in mutable char[] buffers instead of immutable strings
        // so callers can clear sensitive values and avoid leaving extra managed string copies.
        public override char[] Read(ref Utf8JsonReader Reader, Type TypeToConvert, JsonSerializerOptions Options)
        {
            if (Reader.TokenType == JsonTokenType.Null)
            {
                return null;
            }

            if (Reader.TokenType != JsonTokenType.String)
            {
                throw new JsonException();
            }

            if (Reader.HasValueSequence)
            {
                return Reader.GetString()?.ToCharArray();
            }

            char[] value = new char[Encoding.UTF8.GetCharCount(Reader.ValueSpan)];
            Encoding.UTF8.GetChars(Reader.ValueSpan, value);

            return value;
        }

        /// <summary>
        /// Writes the specified character array value as a JSON string using the provided Utf8JsonWriter.
        /// </summary>
        /// <param name="Writer">The Utf8JsonWriter to which the value will be written.</param>
        /// <param name="Value">The character array to write as a JSON string. If null, a JSON null value is written.</param>
        /// <param name="Options">Options to control the behavior of the JSON serialization.</param>
        public override void Write(Utf8JsonWriter Writer, char[] Value, JsonSerializerOptions Options)
        {
            if (Value == null)
            {
                Writer.WriteNullValue();
                return;
            }

            Writer.WriteStringValue(Value);
        }
    }

    internal interface IKeyVaultSensitiveData
    {
        void ClearSensitiveData();
    }

    /// <summary>
    /// Provides an HTTP content implementation for serializing a value of the specified type as JSON using a given
    /// serialization context.
    /// </summary>
    /// <remarks>This class is intended for use with HTTP requests that require a JSON payload. It uses the
    /// provided JsonTypeInfo to control serialization behavior, enabling support for source-generated serializers and
    /// custom serialization options.</remarks>
    /// <typeparam name="TValue">The type of the value to serialize as JSON content.</typeparam>
    internal sealed class KeyVaultJsonContent<TValue> : HttpContent
    {
        private readonly TValue Value;
        private readonly JsonTypeInfo<TValue> TypeInfo;

        public KeyVaultJsonContent(TValue Value, JsonTypeInfo<TValue> TypeInfo)
        {
            this.Value = Value;
            this.TypeInfo = TypeInfo;
            this.Headers.ContentType = new MediaTypeHeaderValue("application/json");
        }

        protected override Task SerializeToStreamAsync(Stream Stream, TransportContext Context)
        {
            return JsonSerializer.SerializeAsync(Stream, this.Value, this.TypeInfo);
        }

        protected override Task SerializeToStreamAsync(Stream Stream, TransportContext Context, CancellationToken CancellationToken)
        {
            return JsonSerializer.SerializeAsync(Stream, this.Value, this.TypeInfo, CancellationToken);
        }

        protected override bool TryComputeLength(out long Length)
        {
            Length = -1;
            return false;
        }

        protected override void Dispose(bool Disposing)
        {
            if (Disposing && this.Value is IKeyVaultSensitiveData sensitiveData)
            {
                sensitiveData.ClearSensitiveData();
            }

            base.Dispose(Disposing);
        }
    }

    /// <summary>
    /// Represents a minimal request payload for Azure Key Vault sign/decrypt operations.
    /// </summary>
    internal sealed class KeyVaultOperationRequest : IKeyVaultSensitiveData
    {
        [JsonPropertyName("alg")]
        public string Algorithm { get; set; }

        [JsonPropertyName("value")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] Value { get; set; }

        [JsonPropertyName("aad")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] AdditionalAuthenticatedData { get; set; }

        [JsonPropertyName("iv")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] InitializationVector { get; set; }

        [JsonPropertyName("tag")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] AuthenticationTag { get; set; }

        public void ClearSensitiveData()
        {
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.Value.AsSpan()));
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.AdditionalAuthenticatedData.AsSpan()));
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.InitializationVector.AsSpan()));
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.AuthenticationTag.AsSpan()));
        }
    }

    /// <summary>
    /// Represents a minimal request payload for Azure Key Vault verify operations.
    /// </summary>
    internal sealed class KeyVaultVerifyRequest : IKeyVaultSensitiveData
    {
        [JsonPropertyName("alg")]
        public string Algorithm { get; set; }

        [JsonPropertyName("digest")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] Digest { get; set; }

        [JsonPropertyName("signature")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] Signature { get; set; }

        public void ClearSensitiveData()
        {
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.Digest.AsSpan()));
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.Signature.AsSpan()));
        }
    }

    /// <summary>
    /// Represents a minimal response payload from Azure Key Vault verify operations.
    /// </summary>
    internal sealed class KeyVaultVerifyResponse
    {
        [JsonPropertyName("value")]
        public bool Value { get; set; }
    }

    /// <summary>
    /// Represents a Key Vault key response.
    /// </summary>
    internal sealed class KeyVaultKeyResponse
    {
        [JsonPropertyName("kid")]
        public string Kid { get; set; }

        [JsonPropertyName("key")]
        public KeyVaultJsonWebKey Key { get; set; }

        [JsonPropertyName("attributes")]
        public KeyVaultKeyAttributes Attributes { get; set; }

        [JsonPropertyName("tags")]
        public Dictionary<string, string> Tags { get; set; }

        [JsonExtensionData]
        public Dictionary<string, JsonElement> ExtensionData { get; set; }
    }

    /// <summary>
    /// Represents a paged Key Vault key list response.
    /// </summary>
    internal sealed class KeyVaultKeyListResponse
    {
        [JsonPropertyName("value")]
        public KeyVaultKeyResponse[] Value { get; set; }

        [JsonPropertyName("nextLink")]
        public string NextLink { get; set; }
    }

    /// <summary>
    /// Represents a JSON Web Key returned by Key Vault.
    /// </summary>
    internal sealed class KeyVaultJsonWebKey : IKeyVaultSensitiveData
    {
        [JsonPropertyName("kid")]
        public string Kid { get; set; }

        [JsonPropertyName("kty")]
        public string KeyType { get; set; }

        [JsonPropertyName("key_ops")]
        public string[] KeyOperations { get; set; }

        [JsonPropertyName("crv")]
        public string CurveName { get; set; }

        [JsonPropertyName("n")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] Modulus { get; set; }

        [JsonPropertyName("e")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] Exponent { get; set; }

        [JsonPropertyName("d")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] D { get; set; }

        [JsonPropertyName("dp")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] DP { get; set; }

        [JsonPropertyName("dq")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] DQ { get; set; }

        [JsonPropertyName("qi")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] QI { get; set; }

        [JsonPropertyName("p")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] P { get; set; }

        [JsonPropertyName("q")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] Q { get; set; }

        [JsonPropertyName("x")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] X { get; set; }

        [JsonPropertyName("y")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] Y { get; set; }

        [JsonPropertyName("k")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] K { get; set; }

        [JsonExtensionData]
        public Dictionary<string, JsonElement> ExtensionData { get; set; }

        public void ClearSensitiveData()
        {
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.Modulus.AsSpan()));
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.Exponent.AsSpan()));
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.D.AsSpan()));
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.DP.AsSpan()));
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.DQ.AsSpan()));
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.QI.AsSpan()));
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.P.AsSpan()));
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.Q.AsSpan()));
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.X.AsSpan()));
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.Y.AsSpan()));
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.K.AsSpan()));
        }
    }

    /// <summary>
    /// Represents Key Vault key attributes.
    /// </summary>
    internal sealed class KeyVaultKeyAttributes
    {
        [JsonPropertyName("enabled")]
        public bool? Enabled { get; set; }

        [JsonPropertyName("nbf")]
        public long? NotBefore { get; set; }

        [JsonPropertyName("exp")]
        public long? Expires { get; set; }

        [JsonPropertyName("created")]
        public long? Created { get; set; }

        [JsonPropertyName("updated")]
        public long? Updated { get; set; }

        [JsonPropertyName("recoveryLevel")]
        public string RecoveryLevel { get; set; }

        [JsonPropertyName("recoverableDays")]
        public int? RecoverableDays { get; set; }

        [JsonPropertyName("exportable")]
        public bool? Exportable { get; set; }

        [JsonExtensionData]
        public Dictionary<string, JsonElement> ExtensionData { get; set; }
    }

    /// <summary>
    /// Represents a Key Vault create key request.
    /// </summary>
    internal sealed class KeyVaultKeyCreateRequest
    {
        [JsonPropertyName("kty")]
        public string KeyType { get; set; }

        [JsonPropertyName("key_size")]
        public int? KeySize { get; set; }

        [JsonPropertyName("crv")]
        public string CurveName { get; set; }

        [JsonPropertyName("key_ops")]
        public string[] KeyOperations { get; set; }

        [JsonPropertyName("attributes")]
        public KeyVaultKeyAttributes Attributes { get; set; }

        [JsonPropertyName("tags")]
        public Dictionary<string, string> Tags { get; set; }

        [JsonPropertyName("release_policy")]
        public KeyVaultKeyReleasePolicy ReleasePolicy { get; set; }
    }

    /// <summary>
    /// Represents a Key Vault import key request.
    /// </summary>
    internal sealed class KeyVaultKeyImportRequest : IKeyVaultSensitiveData
    {
        [JsonPropertyName("Hsm")]
        public bool? Hsm { get; set; }

        [JsonPropertyName("key")]
        public KeyVaultJsonWebKey Key { get; set; }

        [JsonPropertyName("attributes")]
        public KeyVaultKeyAttributes Attributes { get; set; }

        [JsonPropertyName("tags")]
        public Dictionary<string, string> Tags { get; set; }

        [JsonPropertyName("release_policy")]
        public KeyVaultKeyReleasePolicy ReleasePolicy { get; set; }

        public void ClearSensitiveData()
        {
            this.Key?.ClearSensitiveData();
        }
    }

    /// <summary>
    /// Represents a Key Vault update key request.
    /// </summary>
    internal sealed class KeyVaultKeyUpdateRequest
    {
        [JsonPropertyName("key_ops")]
        public string[] KeyOperations { get; set; }

        [JsonPropertyName("attributes")]
        public KeyVaultKeyAttributes Attributes { get; set; }

        [JsonPropertyName("tags")]
        public Dictionary<string, string> Tags { get; set; }

        [JsonPropertyName("release_policy")]
        public KeyVaultKeyReleasePolicy ReleasePolicy { get; set; }
    }

    /// <summary>
    /// Represents a Key Vault key release policy.
    /// </summary>
    internal sealed class KeyVaultKeyReleasePolicy
    {
        [JsonPropertyName("contentType")]
        public string ContentType { get; set; }

        [JsonPropertyName("data")]
        public string Data { get; set; }

        [JsonExtensionData]
        public Dictionary<string, JsonElement> ExtensionData { get; set; }
    }

    /// <summary>
    /// Represents a Key Vault backup response.
    /// </summary>
    internal sealed class KeyVaultBackupResponse : IKeyVaultSensitiveData
    {
        // Keep char[] buffers instead of string so callers can clear sensitive values.
        [JsonPropertyName("value")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] Value { get; set; }

        public void ClearSensitiveData()
        {
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.Value.AsSpan()));
        }
    }

    /// <summary>
    /// Represents a Key Vault restore request.
    /// </summary>
    internal sealed class KeyVaultRestoreRequest : IKeyVaultSensitiveData
    {
        // Keep char[] buffers instead of string so callers can clear sensitive values.
        [JsonPropertyName("value")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] Value { get; set; }

        public void ClearSensitiveData()
        {
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.Value.AsSpan()));
        }
    }

    /// <summary>
    /// Represents a Key Vault release key request.
    /// </summary>
    internal sealed class KeyVaultKeyReleaseRequest
    {
        [JsonPropertyName("target")]
        public string Target { get; set; }

        [JsonPropertyName("enc")]
        public string EncryptionAlgorithm { get; set; }

        [JsonPropertyName("nonce")]
        public string Nonce { get; set; }
    }

    /// <summary>
    /// Represents a Key Vault release key response.
    /// </summary>
    internal sealed class KeyVaultKeyReleaseResponse : IKeyVaultSensitiveData
    {
        [JsonPropertyName("value")]
        [JsonConverter(typeof(KeyVaultBase64UrlCharArrayJsonConverter))]
        public char[] Value { get; set; }

        public void ClearSensitiveData()
        {
            CryptographicOperations.ZeroMemory(MemoryMarshal.AsBytes(this.Value.AsSpan()));
        }
    }

    /// <summary>
    /// Represents a Key Vault key rotation policy.
    /// </summary>
    internal sealed class KeyVaultKeyRotationPolicy
    {
        [JsonPropertyName("id")]
        public string Id { get; set; }

        [JsonPropertyName("attributes")]
        public KeyVaultKeyRotationPolicyAttributes Attributes { get; set; }

        [JsonPropertyName("lifetimeActions")]
        public KeyVaultKeyLifetimeAction[] LifetimeActions { get; set; }
    }

    /// <summary>
    /// Represents Key Vault key rotation policy attributes.
    /// </summary>
    internal sealed class KeyVaultKeyRotationPolicyAttributes
    {
        [JsonPropertyName("expiryTime")]
        public string ExpiryTime { get; set; }

        [JsonPropertyName("created")]
        public long? Created { get; set; }

        [JsonPropertyName("updated")]
        public long? Updated { get; set; }
    }

    /// <summary>
    /// Represents a Key Vault key lifetime action.
    /// </summary>
    internal sealed class KeyVaultKeyLifetimeAction
    {
        [JsonPropertyName("trigger")]
        public KeyVaultKeyLifetimeActionTrigger Trigger { get; set; }

        [JsonPropertyName("action")]
        public KeyVaultKeyLifetimeActionType Action { get; set; }
    }

    /// <summary>
    /// Represents a Key Vault key lifetime action trigger.
    /// </summary>
    internal sealed class KeyVaultKeyLifetimeActionTrigger
    {
        [JsonPropertyName("timeAfterCreate")]
        public string TimeAfterCreate { get; set; }

        [JsonPropertyName("timeBeforeExpiry")]
        public string TimeBeforeExpiry { get; set; }
    }

    /// <summary>
    /// Represents a Key Vault key lifetime action type.
    /// </summary>
    internal sealed class KeyVaultKeyLifetimeActionType
    {
        [JsonPropertyName("type")]
        public string Type { get; set; }
    }
}

