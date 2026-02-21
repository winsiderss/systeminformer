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

        public async Task<byte[]> Sign(byte[] Digest, string Algorithm, Uri KeyId, CancellationToken CancellationToken = default)
        {
            ArgumentNullException.ThrowIfNull(Digest, nameof(Digest));
            ArgumentNullException.ThrowIfNull(Algorithm, nameof(Algorithm));
            ArgumentNullException.ThrowIfNull(KeyId, nameof(KeyId));

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

            using var responseMessage = await this.HttpClient.SendAsync(
                requestMessage, 
                HttpCompletionOption.ResponseContentRead,
                CancellationToken
                ).ConfigureAwait(false);

            responseMessage.EnsureSuccessStatusCode();

            var responseText = await responseMessage.Content.ReadAsStringAsync(CancellationToken).ConfigureAwait(false);
            var keyVaultOperationResponse = JsonSerializer.Deserialize(responseText, AzureJsonContext.Default.KeyVaultOperationResponse);
            var signatureValue = keyVaultOperationResponse?.Value ?? keyVaultOperationResponse?.Signature;

            ArgumentNullException.ThrowIfNull(signatureValue, nameof(signatureValue));

            return Base64UrlDecode(signatureValue);
        }

        public async Task<byte[]> Decrypt(byte[] Cipher, string Algorithm, Uri KeyId, CancellationToken CancellationToken = default)
        {
            if (Cipher is null)
                throw new ArgumentNullException(nameof(Cipher));
            if (string.IsNullOrWhiteSpace(Algorithm))
                throw new ArgumentNullException(nameof(Algorithm));
            if (KeyId is null)
                throw new ArgumentNullException(nameof(KeyId));

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

            using var responseMessage = await this.HttpClient.SendAsync(
                requestMessage, 
                HttpCompletionOption.ResponseContentRead, 
                CancellationToken
                ).ConfigureAwait(false);

            responseMessage.EnsureSuccessStatusCode();

            var responseText = await responseMessage.Content.ReadAsStringAsync(CancellationToken).ConfigureAwait(false);
            var keyVaultOperationResponse = JsonSerializer.Deserialize(responseText, AzureJsonContext.Default.KeyVaultOperationResponse);
            var plaintextValue = keyVaultOperationResponse?.Plaintext ?? keyVaultOperationResponse?.Value;

            ArgumentNullException.ThrowIfNull(plaintextValue, nameof(plaintextValue));

            return Base64UrlDecode(plaintextValue);
        }

        private static string Base64UrlEncode(byte[] Data)
        {
            var base64String = Convert.ToBase64String(Data);

            return base64String.TrimEnd('=').Replace('+', '-').Replace('/', '_');
        }

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
