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

            var signatureAlgorithmId = SignatureAlgorithmTranslator.SignatureAlgorithmToJwsAlgId(SignatureAlgorithm, HashAlgorithm);
            using var keyVaultClient = new KeyVaultRestClient(this.AccessToken);

            // Block on the async call to provide a synchronous API expected by callers.
            return keyVaultClient.Sign(Digest, signatureAlgorithmId, this.KeyIdentifier, CancellationToken.None).GetAwaiter().GetResult();
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
            var encryptionAlgorithmId = EncryptionPaddingToJwsAlgId(Padding);
            using var keyVaultClient = new KeyVaultRestClient(this.AccessToken);

            // Block on the async call to provide a synchronous API expected by callers.
            return keyVaultClient.Decrypt(CipherText, encryptionAlgorithmId, this.KeyIdentifier, CancellationToken.None).GetAwaiter().GetResult();
        }

        //const int SHA1_SIZE = 20;
        //private static readonly byte[] sha1Digest = [0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2B, 0x0E, 0x03, 0x02, 0x1A, 0x05, 0x00, 0x04, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
        //
        //internal static byte[] CreateSha1Digest(byte[] hash)
        //{
        //    if (hash.Length != SHA1_SIZE)
        //        throw new ArgumentException("Invalid hash value");
        //
        //    byte[] pkcs1Digest = (byte[])sha1Digest.Clone();
        //    Array.Copy(hash, 0, pkcs1Digest, pkcs1Digest.Length - hash.Length, hash.Length);
        //
        //    return pkcs1Digest;
        //}
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
                var operationRequest = new KeyVaultOperationRequest
                {
                    Algorithm = Algorithm,
                    Value = Base64UrlEncode(Digest)
                };

                var requestUrl = KeyId.ToString().TrimEnd('/') + "/sign?api-version=2025-07-01";
                var requestJson = JsonSerializer.Serialize(operationRequest, AzureJsonContext.Default.KeyVaultOperationRequest);
                using var requestMessage = new HttpRequestMessage(HttpMethod.Post, requestUrl)
                {
                    Content = new StringContent(requestJson, Encoding.UTF8, "application/json")
                };

                requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Bearer", this.AccessToken);

                using var responseMessage = await this.HttpClient.SendAsync(requestMessage, HttpCompletionOption.ResponseContentRead, CancellationToken);
                responseMessage.EnsureSuccessStatusCode();

                var responseStream = await responseMessage.Content.ReadAsStreamAsync(CancellationToken);
                ArgumentNullException.ThrowIfNull(responseStream, nameof(responseStream));

                var keyVaultOperationResponse = JsonSerializer.Deserialize(responseStream, AzureJsonContext.Default.KeyVaultOperationResponse);
                ArgumentNullException.ThrowIfNull(keyVaultOperationResponse, nameof(keyVaultOperationResponse));

                var signatureValue = keyVaultOperationResponse?.Value ?? keyVaultOperationResponse?.Signature;
                ArgumentNullException.ThrowIfNull(signatureValue, nameof(signatureValue));

                return Base64UrlDecode(signatureValue);
            }
            catch (Exception)
            {
                Program.PrintColorMessage($"[StaticNativeSignDigestCallback] Exception", ConsoleColor.Red);
                return null;
            }
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
        public async Task<byte[]> Decrypt(byte[] Cipher, string Algorithm, Uri KeyId, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(Cipher, nameof(Cipher));
            ArgumentNullException.ThrowIfNull(Algorithm, nameof(Algorithm));
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

            try
            {
                var operationRequest = new KeyVaultOperationRequest
                {
                    Algorithm = Algorithm,
                    Value = Base64UrlEncode(Cipher)
                };

                var requestUrl = KeyId.ToString().TrimEnd('/') + "/decrypt?api-version=2025-07-01";
                var requestJson = JsonSerializer.Serialize(operationRequest, AzureJsonContext.Default.KeyVaultOperationRequest);
                using var requestMessage = new HttpRequestMessage(HttpMethod.Post, requestUrl)
                {
                    Content = new StringContent(requestJson, Encoding.UTF8, "application/json")
                };

                requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Bearer", this.AccessToken);

                using var responseMessage = await this.HttpClient.SendAsync(requestMessage, HttpCompletionOption.ResponseContentRead, CancellationToken);
                responseMessage.EnsureSuccessStatusCode();

                var responseStream = await responseMessage.Content.ReadAsStreamAsync(CancellationToken);
                ArgumentNullException.ThrowIfNull(responseStream, nameof(responseStream));

                var keyVaultOperationResponse = JsonSerializer.Deserialize(responseStream, AzureJsonContext.Default.KeyVaultOperationResponse);
                ArgumentNullException.ThrowIfNull(keyVaultOperationResponse, nameof(keyVaultOperationResponse));

                var plaintextValue = keyVaultOperationResponse?.Plaintext ?? keyVaultOperationResponse?.Value;
                ArgumentNullException.ThrowIfNull(plaintextValue, nameof(plaintextValue));

                return Base64UrlDecode(plaintextValue);
            }
            catch (Exception)
            {
                Program.PrintColorMessage($"[StaticNativeSignDigestCallback] Exception", ConsoleColor.Red);
                return null;
            }
        }

        /// <summary>
        /// Encodes the specified byte array into a Base64 URL-safe string without padding.
        /// </summary>
        /// <remarks>This method produces a Base64-encoded string that is safe for use in URLs and
        /// filenames by replacing '+' with '-', '/' with '_', and omitting padding characters. The output conforms to
        /// RFC 4648, section 5.</remarks>
        /// <param name="Data">The byte array to encode as a Base64 URL-safe string. Cannot be null.</param>
        /// <returns>A Base64 URL-safe encoded string representation of the input data, with padding characters removed.</returns>
        private static string Base64UrlEncode(byte[] Data)
        {
            var base64String = Convert.ToBase64String(Data);

            return base64String.TrimEnd('=').Replace('+', '-').Replace('/', '_');
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
        private static byte[] Base64UrlDecode(string Text)
        {
            var base64String = Text.Replace('-', '+').Replace('_', '/');

            switch (base64String.Length % 4)
            {
            case 2:
                base64String += "==";
                break;
            case 3:
                base64String += "=";
                break;
            }

            return Convert.FromBase64String(base64String);
        }
    }

    /// <summary>
    /// Represents a minimal request payload for Azure Key Vault sign/decrypt operations.
    /// </summary>
    internal sealed class KeyVaultOperationRequest
    {
        [JsonPropertyName("alg")]
        public string Algorithm { get; set; }
        [JsonPropertyName("value")]
        public string Value { get; set; }
    }
}
