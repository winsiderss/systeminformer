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
    /// Provides Azure Key Vault integration for certificate retrieval and code signing operations.
    /// Supports both client secret and certificate-based authentication.
    /// </summary>
    internal static class AzureClient
    {
        private static readonly ConcurrentDictionary<string, X509Certificate2> AzureClientCrtificateCache = new();

        /// <summary>
        /// Creates a cache key for storing certificates based on vault and authentication parameters.
        /// </summary>
        /// <param name="VaultName">The name of the Azure Key Vault.</param>
        /// <param name="CertName">The name of the certificate in the vault.</param>
        /// <param name="TenantId">The Azure tenant ID.</param>
        /// <param name="ClientId">The Azure client (application) ID.</param>
        /// <returns>A string combining all parameters to create a unique cache key.</returns>
        private static string MakeCacheKey(string VaultName, string CertName, string TenantId, string ClientId)
            => $"{VaultName}|{CertName}|{TenantId}|{ClientId}";

        /// <summary>
        /// Retrieves a certificate from Azure Key Vault.
        /// </summary>
        /// <param name="HttpClient">The HTTP client for making requests.</param>
        /// <param name="VaultName">The name of the Azure Key Vault.</param>
        /// <param name="CertName">The name of the certificate to retrieve.</param>
        /// <param name="AccessToken">The access token for authenticating with Key Vault.</param>
        /// <param name="CancellationToken">Cancellation token to cancel the operation.</param>
        /// <returns>A KeyVaultCertificate containing the certificate data and key ID, or null if retrieval fails.</returns>
        private static async Task<KeyVaultCertificate> GetKeyVaultCertificate(HttpClient HttpClient, string VaultName, string CertName, string AccessToken, CancellationToken CancellationToken)
        {
            try
            {
                HttpRequestMessage requestMessage = new HttpRequestMessage(
                    HttpMethod.Get, 
                    $"https://{VaultName}.vault.azure.net/certificates/{CertName}?api-version=2025-07-01"
                    );
                requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Bearer", AccessToken);

                using var responseMessage = await HttpClient.SendAsync(requestMessage, CancellationToken);

                if (!responseMessage.IsSuccessStatusCode)
                {
                    Program.PrintColorMessage($"Failed to fetch certificate from Key Vault: {(int)responseMessage.StatusCode} {responseMessage.ReasonPhrase}", ConsoleColor.Red);
                    return null;
                }

                var jsonResponse = await responseMessage.Content.ReadAsStringAsync(CancellationToken);

                if (!string.IsNullOrWhiteSpace(jsonResponse))
                {
                    Program.PrintColorMessage($"Failed to fetch json response from Key Vault", ConsoleColor.Red);
                    return null;            
                }

                var keyVaultCertificateResponse = JsonSerializer.Deserialize(jsonResponse, AzureJsonContext.Default.KeyVaultCertificateResponse);
                var keyVaultCertificate = new KeyVaultCertificate();

                if (!string.IsNullOrEmpty(keyVaultCertificateResponse?.CertificateString))
                {
                    keyVaultCertificate.CertificateBuffer = Convert.FromBase64String(keyVaultCertificateResponse.CertificateString);
                }

                // KeyId may be in 'kid' or 'key.kid' depending on response shape
                if (!string.IsNullOrWhiteSpace(keyVaultCertificateResponse?.Kid))
                {
                    keyVaultCertificate.KeyId = keyVaultCertificateResponse.Kid;
                }
                else if (!string.IsNullOrWhiteSpace(keyVaultCertificateResponse?.Key?.Kid))
                {
                    keyVaultCertificate.KeyId = keyVaultCertificateResponse.Key.Kid;
                }

                return keyVaultCertificate;
            }
            catch (Exception exception)
            {
                Program.PrintColorMessage($"Failed to parse Key Vault certificate response: {exception.Message}", ConsoleColor.Red);
                return null;
            }
        }

        /// <summary>
        /// Initializes the Azure client by retrieving authentication credentials and fetching the signing certificate.
        /// </summary>
        /// <param name="CancellationToken">Cancellation token to cancel the operation.</param>
        /// <returns>True if initialization succeeds, false otherwise.</returns>
        /// <remarks>
        /// This method reads environment variables for authentication:
        /// - AZURE_TENANT_ID: Azure tenant ID
        /// - AZURE_CLIENT_ID: Application client ID
        /// - AZURE_CLIENT_SECRET: Client secret (for secret-based auth)
        /// - AZURE_CLIENT_CERTIFICATE_PATH: Path to certificate file (for cert-based auth)
        /// - KEYVAULT_NAME: Name of the Key Vault
        /// - CERT_NAME: Name of the certificate in the vault
        /// 
        /// Certificates are cached to avoid repeated Key Vault requests.
        /// </remarks>
        public static async Task<bool> StartAzureClient(CancellationToken CancellationToken = default)
        {
            string tenantId = Environment.GetEnvironmentVariable("AZURE_TENANT_ID");
            string clientId = Environment.GetEnvironmentVariable("AZURE_CLIENT_ID");
            string clientSecret = Environment.GetEnvironmentVariable("AZURE_CLIENT_SECRET");
            string clientCertPath = Environment.GetEnvironmentVariable("AZURE_CLIENT_CERTIFICATE_PATH");
            string vaultName = Environment.GetEnvironmentVariable("KEYVAULT_NAME");
            string certName = Environment.GetEnvironmentVariable("CERT_NAME");

            string cacheKey = MakeCacheKey(vaultName, certName, tenantId, clientId);

            if (AzureClientCrtificateCache.TryGetValue(cacheKey, out var cachedCertificate) && cachedCertificate != null)
            {
                Program.PrintColorMessage($"Loaded certificate from cache: {cachedCertificate.Subject} | Thumbprint: {cachedCertificate.Thumbprint} | HasPrivateKey: {cachedCertificate.HasPrivateKey}", ConsoleColor.Green);
                return true;
            }

            using var httpClient = BuildHttpClient.CreateHttpClient();

            var accessToken = await GetAccessTokenWithRetry(
                httpClient,
                tenantId,
                clientId,
                clientSecret,
                clientCertPath,
                MaxRetries: 5,
                InitialDelayMs: 500,
                CancellationToken: CancellationToken
                );

            if (string.IsNullOrEmpty(accessToken))
            {
                Program.PrintColorMessage("Failed to acquire access token.", ConsoleColor.Red);
                return false;
            }

            using HttpRequestMessage requestMessage = new HttpRequestMessage(
                HttpMethod.Get, 
                $"https://{vaultName}.vault.azure.net/secrets/{certName}?api-version=2025-07-01"
                );
            requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Bearer", accessToken);

            using HttpResponseMessage responseMessage = await httpClient.SendAsync(requestMessage, CancellationToken);
            
            if (!responseMessage.IsSuccessStatusCode)
            {
                Program.PrintColorMessage($"Key Vault error: {(int)responseMessage.StatusCode} {responseMessage.ReasonPhrase}", ConsoleColor.Red);
                return false;
            }

            var jsonResponse = await responseMessage.Content.ReadAsStringAsync(CancellationToken);
            var secretResponse = JsonSerializer.Deserialize(jsonResponse, AzureJsonContext.Default.SecretResponse);
            
            if (string.IsNullOrWhiteSpace(secretResponse.Value))
            {
                Console.WriteLine("Secret response is null or missing 'value'.");
                return false;
            }

            var secretValue = secretResponse.Value;

            if (string.IsNullOrWhiteSpace(secretValue))
            {
                Console.WriteLine("Secret response missing 'value'.");
                return false;
            }

            X509Certificate2 inMemoryCertificate = null;

            if (secretValue.TrimStart().StartsWith("-----BEGIN", StringComparison.OrdinalIgnoreCase))
            {
                try
                {
                    inMemoryCertificate = CreateCertificateFromPem(secretValue);
                }
                catch (Exception exception)
                {
                    Program.PrintColorMessage($"Failed to parse PEM secret: {exception.Message}", ConsoleColor.Red);
                    return false;
                }
            }
            else
            {
                try
                {
                    byte[] certificateBytes = Convert.FromBase64String(secretValue);

                    inMemoryCertificate = X509CertificateLoader.LoadCertificate(certificateBytes);
                }
                catch (Exception exception)
                {
                    Program.PrintColorMessage($"Failed to parse PFX secret: {exception.Message}", ConsoleColor.Red);
                    return false;
                }
            }

            if (inMemoryCertificate == null)
            {
                Program.PrintColorMessage("Certificate could not be created in memory.", ConsoleColor.Red);
                return false;
            }

            AzureClientCrtificateCache[cacheKey] = inMemoryCertificate;

            Program.PrintColorMessage($"Loaded certificate in-memory: {inMemoryCertificate.Subject} | Thumbprint: {inMemoryCertificate.Thumbprint} | HasPrivateKey: {inMemoryCertificate.HasPrivateKey}", ConsoleColor.Green);

            return true;
        }

        /// <summary>
        /// Signs files using Azure Key Vault certificates with Authenticode signatures.
        /// </summary>
        /// <param name="TargetPath">Path to the file or directory to sign.</param>
        /// <param name="TimeStampServer">URL of the timestamp server for RFC3161 timestamps.</param>
        /// <param name="AzureCertName">Name of the certificate in Azure Key Vault.</param>
        /// <param name="AzureVaultName">Name of the Azure Key Vault.</param>
        /// <param name="TenantGuid">Azure tenant ID.</param>
        /// <param name="ClientGuid">Azure client (application) ID.</param>
        /// <param name="ClientSecret">Client secret for authentication.</param>
        /// <param name="SignatureDescription">Optional description embedded in the signature.</param>
        /// <param name="SignatureDescriptionUrl">Optional URL embedded in the signature.</param>
        /// <param name="CancellationToken">Cancellation token to cancel the operation.</param>
        /// <returns>True if signing succeeds, false otherwise.</returns>
        /// <remarks>
        /// When TargetPath is a directory, signs all .exe and .dll files (excluding ksi.dll).
        /// Uses SHA256 hash algorithm and RSA signatures.
        /// Requires a valid cached certificate from StartAzureClient().
        /// </remarks>
        public static async Task<bool> SignFiles(
            string TargetPath,
            string TimeStampServer,
            string AzureCertName,
            string AzureVaultName,
            string TenantGuid,
            string ClientGuid,
            string ClientSecret,
            string SignatureDescription = null,
            string SignatureDescriptionUrl = null,
            CancellationToken CancellationToken = default
            )
        {
            try
            {
                string cacheKey = MakeCacheKey(AzureVaultName, AzureCertName, TenantGuid, ClientGuid);

                if (!AzureClientCrtificateCache.TryGetValue(cacheKey, out X509Certificate2 azureCertificatePublic) || azureCertificatePublic == null)
                {
                    Program.PrintColorMessage($"Azure Client Failed.", ConsoleColor.Red);
                    return false;
                }

                var certificateAzureVaultServer = new Uri($"https://{AzureVaultName}.vault.azure.net/");
                var certificateTimeStampServer = new TimeStampConfiguration(TimeStampServer, TimeStampType.RFC3161);
                using var httpClientForKeyVault = BuildHttpClient.CreateHttpClient();
                X509Certificate2 vaultPublicCert = null;
                Uri keyIdUri = null;

                string vaultAccessToken = await GetAccessTokenWithRetry(
                    httpClientForKeyVault,
                    TenantGuid, 
                    ClientGuid, 
                    ClientSecret,
                    null,  // No certificate path in this context
                    MaxRetries: 5, 
                    InitialDelayMs: 500, 
                    CancellationToken: CancellationToken
                    );

                if (string.IsNullOrEmpty(vaultAccessToken))
                {
                    Program.PrintColorMessage("Failed to acquire access token for Key Vault.", ConsoleColor.Red);
                    return false;
                }

                var keyVaultCertificateResponse = await GetKeyVaultCertificate(
                    httpClientForKeyVault, 
                    AzureVaultName, 
                    AzureCertName, 
                    vaultAccessToken, 
                    CancellationToken
                    );

                if (keyVaultCertificateResponse?.CertificateBuffer != null && keyVaultCertificateResponse.CertificateBuffer.Length > 0)
                {
                    // Cer is DER-encoded certificate bytes returned by Key Vault
                    vaultPublicCert = X509CertificateLoader.LoadCertificate(keyVaultCertificateResponse.CertificateBuffer);
                }
                else
                {
                    // fallback to cached public cert (older behavior)
                    vaultPublicCert = azureCertificatePublic;
                }

                // We will pass the raw access token into RSAFactory (no Azure.Core dependency)
                var accessTokenForKeyVault = vaultAccessToken;

                // Determine key id Uri; fall back to a best-effort keys path if not present
                if (!string.IsNullOrWhiteSpace(keyVaultCertificateResponse?.KeyId))
                {
                    keyIdUri = new Uri(keyVaultCertificateResponse.KeyId);
                }
                else
                {
                    try
                    {
                        keyIdUri = new Uri(certificateAzureVaultServer, $"keys/{AzureCertName}");
                    }
                    catch (Exception ex)
                    {
                        Program.PrintColorMessage($"Failed to construct key URI: {ex.Message}", ConsoleColor.Yellow);
                        keyIdUri = null;
                    }
                }

                using (var azureCertificateRsa = RSAFactory.Create(accessTokenForKeyVault, keyIdUri, vaultPublicCert))
                using (var authenticodeKeyVaultSigner = new AuthenticodeKeyVaultSigner(
                    azureCertificateRsa, 
                    vaultPublicCert, 
                    HashAlgorithmName.SHA256, 
                    certificateTimeStampServer,
                    null
                    ))
                {
                    if (Directory.Exists(TargetPath))
                    {
                        var fileList = Utils.EnumerateDirectory(TargetPath, [".exe", ".dll"], ["ksi.dll"]);

                        if (fileList == null || fileList.Count == 0)
                        {
                            Program.PrintColorMessage($"No files found to sign.", ConsoleColor.Red);
                        }
                        else
                        {
                            int successCount = 0;
                            foreach (var fileName in fileList)
                            {
                                CancellationToken.ThrowIfCancellationRequested();

                                try
                                {
                                    var result = authenticodeKeyVaultSigner.SignFile(
                                        fileName, 
                                        SignatureDescription, 
                                        SignatureDescriptionUrl);

                                    if (result == HRESULT.S_OK)
                                    {
                                        successCount++;
                                        Program.PrintColorMessage($"Signed: {fileName}", ConsoleColor.Green);
                                    }
                                    else
                                    {
                                        Program.PrintColorMessage($"Failed: ({result}) {fileName}", ConsoleColor.Red);
                                    }
                                }
                                catch (Exception exception)
                                {
                                    Program.PrintColorMessage($"[ERROR] Signing '{fileName}': {exception.Message}", ConsoleColor.Red);
                                }
                            }

                            Program.PrintColorMessage($"Signing complete. {successCount}/{fileList.Count} files signed.", ConsoleColor.DarkGray);
                        }
                    }
                    else if (File.Exists(TargetPath))
                    {
                        var result = authenticodeKeyVaultSigner.SignFile(
                            TargetPath, 
                            SignatureDescription, 
                            SignatureDescriptionUrl);

                        if (result == HRESULT.COR_E_BADIMAGEFORMAT)
                        {
                            Program.PrintColorMessage($"[ERROR] The AppxManifest.xml publisher CN does not match the certificate: ({result}) {TargetPath}", ConsoleColor.Red);
                            return false;
                        }
                        else if (result == HRESULT.TRUST_E_SUBJECT_FORM_UNKNOWN)
                        {
                            Program.PrintColorMessage($"[ERROR] File content not supported: ({result}) {TargetPath}", ConsoleColor.Red);
                            return false;
                        }
                        else if (result != HRESULT.S_OK)
                        {
                            Program.PrintColorMessage($"[ERROR] ({result}) {TargetPath}", ConsoleColor.Red);
                            return false;
                        }

                        Program.PrintColorMessage($"Success: {TargetPath}", ConsoleColor.Green);
                    }
                    else
                    {
                        Program.PrintColorMessage($"Path not found: {TargetPath}", ConsoleColor.Red);
                        return false;
                    }
                }
            }
            catch (Exception exception)
            {
                Program.PrintColorMessage($"[ERROR] {exception}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Retrieves an Azure access token with automatic retry logic.
        /// Supports both client secret and certificate-based authentication.
        /// </summary>
        /// <param name="HttpClient">The HTTP client for making requests.</param>
        /// <param name="TenantId">The Azure tenant ID.</param>
        /// <param name="ClientId">The Azure client (application) ID.</param>
        /// <param name="ClientSecret">The client secret for secret-based authentication (optional).</param>
        /// <param name="ClientCertificatePath">Path to certificate file for certificate-based authentication (optional).</param>
        /// <param name="MaxRetries">Maximum number of retry attempts (default: 5).</param>
        /// <param name="InitialDelayMs">Initial delay in milliseconds before retrying (default: 500ms).</param>
        /// <param name="CancellationToken">Cancellation token to cancel the operation.</param>
        /// <returns>The access token string, or empty string if authentication fails.</returns>
        /// <remarks>
        /// Certificate-based authentication takes priority if both ClientSecret and ClientCertificatePath are provided.
        /// At least one authentication method must be specified.
        /// </remarks>
        private static async Task<string> GetAccessTokenWithRetry(
            HttpClient HttpClient,
            string TenantId,
            string ClientId,
            string ClientSecret,
            string ClientCertificatePath,
            int MaxRetries = 5,
            int InitialDelayMs = 500,
            CancellationToken CancellationToken = default)
        {
            // Determine authentication method
            bool useCertificate = !string.IsNullOrWhiteSpace(ClientCertificatePath);
            bool useSecret = !string.IsNullOrWhiteSpace(ClientSecret);

            if (!useCertificate && !useSecret)
            {
                Program.PrintColorMessage("Neither AZURE_CLIENT_SECRET nor AZURE_CLIENT_CERTIFICATE_PATH is set.", ConsoleColor.Red);
                return string.Empty;
            }

            if (useCertificate && useSecret)
            {
                Program.PrintColorMessage("Both AZURE_CLIENT_SECRET and AZURE_CLIENT_CERTIFICATE_PATH are set. Using certificate authentication.", ConsoleColor.Yellow);
                useSecret = false;
            }

            if (useCertificate)
            {
                return await GetAccessTokenWithCertificate(
                    HttpClient,
                    TenantId,
                    ClientId,
                    ClientCertificatePath,
                    MaxRetries,
                    InitialDelayMs,
                    CancellationToken
                    );
            }
            else
            {
                return await GetAccessTokenWithSecret(
                    HttpClient,
                    TenantId,
                    ClientId,
                    ClientSecret,
                    MaxRetries,
                    InitialDelayMs,
                    CancellationToken
                    );
            }
        }

        /// <summary>
        /// Retrieves an Azure access token using client secret authentication with retry logic.
        /// </summary>
        /// <param name="HttpClient">The HTTP client for making requests.</param>
        /// <param name="TenantId">The Azure tenant ID.</param>
        /// <param name="ClientId">The Azure client (application) ID.</param>
        /// <param name="ClientSecret">The client secret for authentication.</param>
        /// <param name="MaxRetries">Maximum number of retry attempts (default: 5).</param>
        /// <param name="InitialDelayMs">Initial delay in milliseconds before retrying (default: 500ms).</param>
        /// <param name="CancellationToken">Cancellation token to cancel the operation.</param>
        /// <returns>The access token string, or empty string if authentication fails.</returns>
        /// <remarks>
        /// Uses OAuth 2.0 client credentials flow with client_secret.
        /// Implements exponential backoff with jitter for retries.
        /// Securely handles the client secret by zeroing memory after use.
        /// </remarks>
        private static async Task<string> GetAccessTokenWithSecret(
            HttpClient HttpClient,
            string TenantId,
            string ClientId,
            string ClientSecret,
            int MaxRetries = 5,
            int InitialDelayMs = 500,
            CancellationToken CancellationToken = default)
        {
            byte[] bodyBytes = null;
            try
            {
                // Build body directly as bytes to minimize secret exposure in string form
                var clientIdBytes = Encoding.UTF8.GetBytes($"client_id={Uri.EscapeDataString(ClientId)}&");
                var scopeBytes = Encoding.UTF8.GetBytes($"scope={Uri.EscapeDataString("https://vault.azure.net/.default")}&");
                var secretBytes = Encoding.UTF8.GetBytes($"client_secret={Uri.EscapeDataString(ClientSecret)}&");
                var grantBytes = Encoding.UTF8.GetBytes("grant_type=client_credentials");

                bodyBytes = new byte[clientIdBytes.Length + scopeBytes.Length + secretBytes.Length + grantBytes.Length];
                Buffer.BlockCopy(clientIdBytes, 0, bodyBytes, 0, clientIdBytes.Length);
                Buffer.BlockCopy(scopeBytes, 0, bodyBytes, clientIdBytes.Length, scopeBytes.Length);
                Buffer.BlockCopy(secretBytes, 0, bodyBytes, clientIdBytes.Length + scopeBytes.Length, secretBytes.Length);
                Buffer.BlockCopy(grantBytes, 0, bodyBytes, clientIdBytes.Length + scopeBytes.Length + secretBytes.Length, grantBytes.Length);

                // Zero intermediate buffers
                System.Security.Cryptography.CryptographicOperations.ZeroMemory(secretBytes);

                int currentAttempt = 0;
                int delayMs = InitialDelayMs;

                while (true)
                {
                    currentAttempt++;

                    try
                    {
                        using var tokenBody = new ByteArrayContent(bodyBytes);
                        tokenBody.Headers.ContentType = new System.Net.Http.Headers.MediaTypeHeaderValue("application/x-www-form-urlencoded");

                        using HttpResponseMessage responseMessage = await HttpClient.PostAsync(
                            $"https://login.microsoftonline.com/{TenantId}/oauth2/v2.0/token",
                            tokenBody,
                            CancellationToken
                            );

                        if (responseMessage.IsSuccessStatusCode)
                        {
                            string jsonResponse = await responseMessage.Content.ReadAsStringAsync(CancellationToken);
                            TokenResponse tokenResponse = JsonSerializer.Deserialize(jsonResponse, AzureJsonContext.Default.TokenResponse);

                            if (tokenResponse.AccessToken == null)
                            {
                                Program.PrintColorMessage("Token response is null or missing 'access_token'.", ConsoleColor.Red);
                                return string.Empty;
                            }

                            if (!string.IsNullOrWhiteSpace(tokenResponse.AccessToken))
                                return tokenResponse.AccessToken;

                            Program.PrintColorMessage("Token response missing 'access_token'.", ConsoleColor.Red);
                            return string.Empty;
                        }

                        if (ShouldRetry(responseMessage.StatusCode))
                        {
                            if (currentAttempt > MaxRetries)
                            {
                                Program.PrintColorMessage($"Token fetch failed after {MaxRetries} retries. Last status: {(int)responseMessage.StatusCode} {responseMessage.ReasonPhrase}", ConsoleColor.Red);
                                return string.Empty;
                            }

                            var retryAfterDelay = GetRetryAfterMs(responseMessage);
                            delayMs = retryAfterDelay ?? ApplyJitter(delayMs);
                            await Task.Delay(delayMs, CancellationToken);
                            delayMs = Math.Min(delayMs * 2, 15000);
                            continue;
                        }
                        else
                        {
                            var errorBodyText = await responseMessage.Content.ReadAsStringAsync(CancellationToken);
                            Program.PrintColorMessage($"Non-retryable token error: {(int)responseMessage.StatusCode} {responseMessage.ReasonPhrase}", ConsoleColor.Red);
                            Program.PrintColorMessage(errorBodyText, ConsoleColor.DarkGray);
                            return string.Empty;
                        }
                    }
                    catch (HttpRequestException httpRequestException) when (currentAttempt <= MaxRetries)
                    {
                        int waitDelay = ApplyJitter(delayMs);
                        Program.PrintColorMessage($"Network error on token fetch (attempt {currentAttempt}/{MaxRetries}): {httpRequestException.Message}. Retrying in {waitDelay} ms...", ConsoleColor.Yellow);
                        await Task.Delay(waitDelay, CancellationToken);
                        delayMs = Math.Min(delayMs * 2, 15000);
                        continue;
                    }
                    catch (TaskCanceledException) when (!CancellationToken.IsCancellationRequested && currentAttempt <= MaxRetries)
                    {
                        int waitDelay = ApplyJitter(delayMs);
                        Program.PrintColorMessage($"Timeout on token fetch (attempt {currentAttempt}/{MaxRetries}). Retrying in {waitDelay} ms...", ConsoleColor.Yellow);
                        await Task.Delay(waitDelay, CancellationToken);
                        delayMs = Math.Min(delayMs * 2, 15000);
                        continue;
                    }
                }
            }
            finally
            {
                if (bodyBytes != null)
                {
                    System.Security.Cryptography.CryptographicOperations.ZeroMemory(bodyBytes);
                }
            }
        }

        /// <summary>
        /// Retrieves an Azure access token using certificate-based authentication with retry logic.
        /// </summary>
        /// <param name="HttpClient">The HTTP client for making requests.</param>
        /// <param name="TenantId">The Azure tenant ID.</param>
        /// <param name="ClientId">The Azure client (application) ID.</param>
        /// <param name="ClientCertificatePath">Path to the certificate file (.pem, .pfx, or .p12).</param>
        /// <param name="MaxRetries">Maximum number of retry attempts (default: 5).</param>
        /// <param name="InitialDelayMs">Initial delay in milliseconds before retrying (default: 500ms).</param>
        /// <param name="CancellationToken">Cancellation token to cancel the operation.</param>
        /// <returns>The access token string, or empty string if authentication fails.</returns>
        /// <remarks>
        /// Uses OAuth 2.0 client credentials flow with JWT bearer assertion (client_assertion).
        /// Supports PEM (.pem) and PFX/PKCS#12 (.pfx, .p12) certificate formats.
        /// For password-protected PFX files, set AZURE_CLIENT_CERTIFICATE_PASSWORD environment variable.
        /// The certificate must have a private key and be registered with the Azure AD application.
        /// Creates a signed JWT using RS256 algorithm for authentication.
        /// </remarks>
        private static async Task<string> GetAccessTokenWithCertificate(
            HttpClient HttpClient,
            string TenantId,
            string ClientId,
            string ClientCertificatePath,
            int MaxRetries = 5,
            int InitialDelayMs = 500,
            CancellationToken CancellationToken = default)
        {
            X509Certificate2 clientCertificate = null;

            try
            {
                // Load the certificate from file
                if (!File.Exists(ClientCertificatePath))
                {
                    Program.PrintColorMessage($"Certificate file not found: {ClientCertificatePath}", ConsoleColor.Red);
                    return string.Empty;
                }

                try
                {
                    string fileExtension = Path.GetExtension(ClientCertificatePath).ToLowerInvariant();

                    if (fileExtension == ".pem")
                    {
                        string pemContent = File.ReadAllText(ClientCertificatePath);
                        clientCertificate = CreateCertificateFromPem(pemContent);
                    }
                    else if (fileExtension == ".pfx" || fileExtension == ".p12")
                    {
                        // For PFX, you may need a password. Check environment variable.
                        string certPassword = Environment.GetEnvironmentVariable("AZURE_CLIENT_CERTIFICATE_PASSWORD");
                        byte[] certBytes = File.ReadAllBytes(ClientCertificatePath);
                        
                        if (!string.IsNullOrWhiteSpace(certPassword))
                        {
                            clientCertificate = X509CertificateLoader.LoadPkcs12(certBytes, certPassword);
                        }
                        else
                        {
                            clientCertificate = X509CertificateLoader.LoadPkcs12(certBytes, null);
                        }
                    }
                    else
                    {
                        Program.PrintColorMessage($"Unsupported certificate format: {fileExtension}. Supported formats: .pem, .pfx, .p12", ConsoleColor.Red);
                        return string.Empty;
                    }
                }
                catch (Exception ex)
                {
                    Program.PrintColorMessage($"Failed to load certificate: {ex.Message}", ConsoleColor.Red);
                    return string.Empty;
                }

                if (clientCertificate == null || !clientCertificate.HasPrivateKey)
                {
                    Program.PrintColorMessage("Certificate must have a private key for authentication.", ConsoleColor.Red);
                    return string.Empty;
                }

                Program.PrintColorMessage($"Loaded client certificate: {clientCertificate.Subject} | Thumbprint: {clientCertificate.Thumbprint}", ConsoleColor.Green);

                // Create JWT assertion
                string jwtAssertion = CreateClientAssertion(TenantId, ClientId, clientCertificate);

                if (string.IsNullOrEmpty(jwtAssertion))
                {
                    Program.PrintColorMessage("Failed to create JWT assertion.", ConsoleColor.Red);
                    return string.Empty;
                }

                // Request token using client_assertion
                int currentAttempt = 0;
                int delayMs = InitialDelayMs;

                while (true)
                {
                    currentAttempt++;

                    try
                    {
                        var tokenRequestData = new Dictionary<string, string>
                        {
                            { "client_id", ClientId },
                            { "client_assertion_type", "urn:ietf:params:oauth:client-assertion-type:jwt-bearer" },
                            { "client_assertion", jwtAssertion },
                            { "scope", "https://vault.azure.net/.default" },
                            { "grant_type", "client_credentials" }
                        };

                        using var tokenBody = new FormUrlEncodedContent(tokenRequestData);

                        using HttpResponseMessage responseMessage = await HttpClient.PostAsync(
                            $"https://login.microsoftonline.com/{TenantId}/oauth2/v2.0/token",
                            tokenBody,
                            CancellationToken
                            );

                        if (responseMessage.IsSuccessStatusCode)
                        {
                            string jsonResponse = await responseMessage.Content.ReadAsStringAsync(CancellationToken);
                            TokenResponse tokenResponse = JsonSerializer.Deserialize(jsonResponse, AzureJsonContext.Default.TokenResponse);

                            if (tokenResponse.AccessToken == null)
                            {
                                Program.PrintColorMessage("Token response is null or missing 'access_token'.", ConsoleColor.Red);
                                return string.Empty;
                            }

                            if (!string.IsNullOrWhiteSpace(tokenResponse.AccessToken))
                                return tokenResponse.AccessToken;

                            Program.PrintColorMessage("Token response missing 'access_token'.", ConsoleColor.Red);
                            return string.Empty;
                        }

                        if (ShouldRetry(responseMessage.StatusCode))
                        {
                            if (currentAttempt > MaxRetries)
                            {
                                Program.PrintColorMessage($"Token fetch failed after {MaxRetries} retries. Last status: {(int)responseMessage.StatusCode} {responseMessage.ReasonPhrase}", ConsoleColor.Red);
                                return string.Empty;
                            }

                            var retryAfterDelay = GetRetryAfterMs(responseMessage);
                            delayMs = retryAfterDelay ?? ApplyJitter(delayMs);
                            await Task.Delay(delayMs, CancellationToken);
                            delayMs = Math.Min(delayMs * 2, 15000);
                            continue;
                        }
                        else
                        {
                            var errorBodyText = await responseMessage.Content.ReadAsStringAsync(CancellationToken);
                            Program.PrintColorMessage($"Non-retryable token error: {(int)responseMessage.StatusCode} {responseMessage.ReasonPhrase}", ConsoleColor.Red);
                            Program.PrintColorMessage(errorBodyText, ConsoleColor.DarkGray);
                            return string.Empty;
                        }
                    }
                    catch (HttpRequestException httpRequestException) when (currentAttempt <= MaxRetries)
                    {
                        int waitDelay = ApplyJitter(delayMs);
                        Program.PrintColorMessage($"Network error on token fetch (attempt {currentAttempt}/{MaxRetries}): {httpRequestException.Message}. Retrying in {waitDelay} ms...", ConsoleColor.Yellow);
                        await Task.Delay(waitDelay, CancellationToken);
                        delayMs = Math.Min(delayMs * 2, 15000);
                        continue;
                    }
                    catch (TaskCanceledException) when (!CancellationToken.IsCancellationRequested && currentAttempt <= MaxRetries)
                    {
                        int waitDelay = ApplyJitter(delayMs);
                        Program.PrintColorMessage($"Timeout on token fetch (attempt {currentAttempt}/{MaxRetries}). Retrying in {waitDelay} ms...", ConsoleColor.Yellow);
                        await Task.Delay(waitDelay, CancellationToken);
                        delayMs = Math.Min(delayMs * 2, 15000);
                        continue;
                    }
                }
            }
            finally
            {
                clientCertificate?.Dispose();
            }
        }

        /// <summary>
        /// Creates a JWT (JSON Web Token) assertion signed with a certificate for Azure authentication.
        /// </summary>
        /// <param name="TenantId">The Azure tenant ID.</param>
        /// <param name="ClientId">The Azure client (application) ID.</param>
        /// <param name="Certificate">The X.509 certificate with private key for signing.</param>
        /// <returns>A signed JWT assertion string in the format "header.payload.signature", or null if creation fails.</returns>
        /// <remarks>
        /// Creates a self-signed JWT using RS256 algorithm with the following claims:
        /// - aud: Token endpoint URL
        /// - exp: Expiration time (10 minutes from now)
        /// - iss: Client ID (issuer)
        /// - jti: Unique JWT ID (GUID)
        /// - nbf: Not before time (current time)
        /// - sub: Client ID (subject)
        /// 
        /// The JWT header includes x5t (certificate thumbprint) for key identification.
        /// </remarks>
        private static string CreateClientAssertion(string TenantId, string ClientId, X509Certificate2 Certificate)
        {
            try
            {
                // JWT Header
                var header = new JwtHeader
                {
                    Alg = "RS256",
                    Typ = "JWT",
                    X5t = Base64UrlEncode(Certificate.GetCertHash())
                };

                // JWT Payload
                var now = DateTimeOffset.UtcNow;
                var payload = new JwtPayload
                {
                    Aud = $"https://login.microsoftonline.com/{TenantId}/oauth2/v2.0/token",
                    Exp = now.AddMinutes(10).ToUnixTimeSeconds(),
                    Iss = ClientId,
                    Jti = Guid.NewGuid().ToString(),
                    Nbf = now.ToUnixTimeSeconds(),
                    Sub = ClientId
                };

                string headerJson = JsonSerializer.Serialize(header, AzureJsonContext.Default.JwtHeader);
                string payloadJson = JsonSerializer.Serialize(payload, AzureJsonContext.Default.JwtPayload);

                string headerBase64 = Base64UrlEncode(Encoding.UTF8.GetBytes(headerJson));
                string payloadBase64 = Base64UrlEncode(Encoding.UTF8.GetBytes(payloadJson));

                string dataToSign = $"{headerBase64}.{payloadBase64}";
                byte[] dataBytes = Encoding.UTF8.GetBytes(dataToSign);

                // Sign with certificate's private key
                using RSA rsa = Certificate.GetRSAPrivateKey();
                if (rsa == null)
                {
                    Program.PrintColorMessage("Certificate does not have an RSA private key.", ConsoleColor.Red);
                    return null;
                }

                byte[] signature = rsa.SignData(dataBytes, HashAlgorithmName.SHA256, RSASignaturePadding.Pkcs1);
                string signatureBase64 = Base64UrlEncode(signature);

                return $"{dataToSign}.{signatureBase64}";
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"Failed to create client assertion: {ex.Message}", ConsoleColor.Red);
                return null;
            }
        }

        /// <summary>
        /// Encodes binary data to Base64URL format as specified in RFC 4648.
        /// </summary>
        /// <param name="input">The byte array to encode.</param>
        /// <returns>A Base64URL-encoded string with padding removed.</returns>
        /// <remarks>
        /// Base64URL encoding replaces '+' with '-', '/' with '_', and removes trailing '=' padding.
        /// This encoding is used in JWT tokens and other web-safe applications.
        /// </remarks>
        private static string Base64UrlEncode(byte[] input)
        {
            string base64 = Convert.ToBase64String(input);
            // Convert base64 to base64url
            return base64.Replace('+', '-').Replace('/', '_').TrimEnd('=');
        }

        /// <summary>
        /// Determines if an HTTP status code indicates a retryable error.
        /// </summary>
        /// <param name="StatusCode">The HTTP status code to check.</param>
        /// <returns>True if the request should be retried, false otherwise.</returns>
        /// <remarks>
        /// Retryable errors include:
        /// - 429 (Too Many Requests)
        /// - 5xx (Server errors: 500-599)
        /// </remarks>
        private static bool ShouldRetry(HttpStatusCode StatusCode) => StatusCode == (HttpStatusCode)429 || ((int)StatusCode >= 500 && (int)StatusCode <= 599);

        /// <summary>
        /// Applies random jitter to a delay value to prevent thundering herd issues.
        /// </summary>
        /// <param name="BaseMs">The base delay in milliseconds.</param>
        /// <returns>The base delay adjusted by a random factor between 0.8 and 1.2.</returns>
        /// <remarks>
        /// Jitter helps distribute retry attempts when multiple clients fail simultaneously.
        /// The random factor is between 80% and 120% of the base value.
        /// </remarks>
        private static int ApplyJitter(int BaseMs)
        {
            double factor = 0.8 + Random.Shared.NextDouble() * 0.4;
            return (int)Math.Round(BaseMs * factor);
        }

        /// <summary>
        /// Extracts the retry delay from an HTTP response's Retry-After header.
        /// </summary>
        /// <param name="ResponseMessage">The HTTP response message to examine.</param>
        /// <returns>The retry delay in milliseconds, or null if no Retry-After header is present.</returns>
        /// <remarks>
        /// Supports both delta-seconds and HTTP-date formats of the Retry-After header.
        /// If the retry time is in the past, returns 0 to retry immediately.
        /// </remarks>
        private static int? GetRetryAfterMs(HttpResponseMessage ResponseMessage)
        {
            if (ResponseMessage.Headers.RetryAfter is null)
                return null;

            if (ResponseMessage.Headers.RetryAfter.Delta.HasValue)
                return (int)ResponseMessage.Headers.RetryAfter.Delta.Value.TotalMilliseconds;

            if (ResponseMessage.Headers.RetryAfter.Date.HasValue)
            {
                TimeSpan timeSpan = ResponseMessage.Headers.RetryAfter.Date.Value - DateTimeOffset.UtcNow;
                return timeSpan.TotalMilliseconds > 0 ? (int)timeSpan.TotalMilliseconds : 0;
            }

            return null;
        }

        /// <summary>
        /// Creates an X509Certificate2 from PEM-encoded certificate and private key data.
        /// </summary>
        /// <param name="PemContent">The PEM-encoded content containing certificate and optionally a private key.</param>
        /// <returns>An X509Certificate2 with the certificate and private key (if present).</returns>
        /// <exception cref="CryptographicException">Thrown if the PEM content is invalid or the key type is unsupported.</exception>
        /// <remarks>
        /// Supports the following PEM blocks:
        /// - CERTIFICATE (required)
        /// - PRIVATE KEY, RSA PRIVATE KEY, EC PRIVATE KEY, ENCRYPTED PRIVATE KEY (optional)
        /// 
        /// If no private key is found, returns a public-only certificate.
        /// Attempts RSA first, then ECDSA for private key import.
        /// </remarks>
        private static X509Certificate2 CreateCertificateFromPem(string PemContent)
        {
            // Try to split certs and key; attach the first cert with the private key
            string certificatePem = ExtractPemBlock(PemContent, "CERTIFICATE");
            if (certificatePem is null)
                throw new CryptographicException("No CERTIFICATE block found in PEM.");

            string privateKeyPem = ExtractPemBlock(PemContent, "ENCRYPTED PRIVATE KEY")
                         ?? ExtractPemBlock(PemContent, "PRIVATE KEY")
                         ?? ExtractPemBlock(PemContent, "RSA PRIVATE KEY")
                         ?? ExtractPemBlock(PemContent, "EC PRIVATE KEY");

            // Build X509 from cert only first
            X509Certificate2 certificate = X509Certificate2.CreateFromPem(certificatePem);

            if (privateKeyPem is null)
            {
                // Public-only
                return certificate;
            }

            // Try RSA first
            try
            {
                using RSA rsa = RSA.Create();
                rsa.ImportFromPem(privateKeyPem);
                return certificate.CopyWithPrivateKey(rsa);
            }
            catch { }

            // Then ECDSA
            try
            {
                using ECDsa ecdsa = ECDsa.Create();
                ecdsa.ImportFromPem(privateKeyPem);
                return certificate.CopyWithPrivateKey(ecdsa);
            }
            catch { }

            throw new CryptographicException("Unsupported or invalid private key in PEM.");
        }

        /// <summary>
        /// Extracts a specific PEM block from text content.
        /// </summary>
        /// <param name="TextContent">The text containing PEM-encoded data.</param>
        /// <param name="Label">The label identifying the PEM block (e.g., "CERTIFICATE", "PRIVATE KEY").</param>
        /// <returns>The extracted PEM block including begin/end markers, or null if not found.</returns>
        /// <remarks>
        /// Searches for blocks in the format:
        /// -----BEGIN {Label}-----
        /// [base64 data]
        /// -----END {Label}-----
        /// 
        /// The search is case-insensitive.
        /// </remarks>
        private static string ExtractPemBlock(string TextContent, string Label)
        {
            var beginMarker = $"-----BEGIN {Label}-----";
            var endMarker = $"-----END {Label}-----";

            int startIndex = TextContent.IndexOf(beginMarker, StringComparison.OrdinalIgnoreCase);
            if (startIndex < 0)
                return null;
            int endIndex = TextContent.IndexOf(endMarker, startIndex + beginMarker.Length, StringComparison.OrdinalIgnoreCase);
            if (endIndex < 0)
                return null;

            endIndex += endMarker.Length;
            return TextContent.Substring(startIndex, endIndex - startIndex);
        }
    }

    /// <summary>
    /// Represents a certificate retrieved from Azure Key Vault.
    /// </summary>
    internal class KeyVaultCertificate
    {
        /// <summary>
        /// Gets or sets the certificate data in DER-encoded format.
        /// </summary>
        public byte[] CertificateBuffer { get; set; }
        
        /// <summary>
        /// Gets or sets the Key Vault key identifier URL for the certificate's private key.
        /// </summary>
        public string KeyId { get; set; }
    }

    /// <summary>
    /// Represents the JSON response from Azure Key Vault certificate operations.
    /// </summary>
    /// <summary>
    /// Represents the JSON response from Azure Key Vault certificate operations.
    /// </summary>
    internal class KeyVaultCertificateResponse
    {
        /// <summary>
        /// Gets or sets the Base64-encoded DER certificate data.
        /// </summary>
        [JsonPropertyName("cer")]
        public string CertificateString { get; set; }
        
        /// <summary>
        /// Gets or sets the key identifier URL.
        /// </summary>
        [JsonPropertyName("kid")]
        public string Kid { get; set; }
        
        /// <summary>
        /// Gets or sets the certificate identifier URL.
        /// </summary>
        [JsonPropertyName("id")]
        public string Id { get; set; }
        
        /// <summary>
        /// Gets or sets the key information containing the key identifier.
        /// </summary>
        [JsonPropertyName("key")]
        public KeyInfo Key { get; set; }
    }

    /// <summary>
    /// Represents key information from Azure Key Vault responses.
    /// </summary>
    internal class KeyInfo
    {
        /// <summary>
        /// Gets or sets the key identifier URL.
        /// </summary>
        [JsonPropertyName("kid")]
        public string Kid { get; set; }
    }

    /// <summary>
    /// Represents responses from Azure Key Vault cryptographic operations.
    /// </summary>
    /// <summary>
    /// Represents responses from Azure Key Vault cryptographic operations.
    /// </summary>
    internal class KeyVaultOperationResponse
    {
        /// <summary>
        /// Gets or sets the operation result value.
        /// </summary>
        [JsonPropertyName("value")]
        public string Value { get; set; }
        
        /// <summary>
        /// Gets or sets the signature result from sign operations.
        /// </summary>
        [JsonPropertyName("signature")]
        public string Signature { get; set; }
        
        /// <summary>
        /// Gets or sets the plaintext result from decrypt operations.
        /// </summary>
        [JsonPropertyName("plaintext")]
        public string Plaintext { get; set; }
    }

    /// <summary>
    /// Represents an OAuth 2.0 token response from Azure AD.
    /// </summary>
    internal readonly struct TokenResponse
    {
        /// <summary>
        /// Gets the access token for authenticating API requests.
        /// </summary>
        [JsonPropertyName("access_token")]
        public string AccessToken { get; init; }
        
        /// <summary>
        /// Gets the type of token (typically "Bearer").
        /// </summary>
        [JsonPropertyName("token_type")]
        public string TokenType { get; init; }
        
        /// <summary>
        /// Gets the token lifetime in seconds.
        /// </summary>
        [JsonPropertyName("expires_in")]
        public int ExpiresIn { get; init; }
        
        /// <summary>
        /// Gets the scope of the access token.
        /// </summary>
        [JsonPropertyName("scope")]
        public string Scope { get; init; }
    }

    /// <summary>
    /// Represents a secret response from Azure Key Vault.
    /// </summary>
    internal readonly struct SecretResponse
    {
        /// <summary>
        /// Gets the secret value.
        /// </summary>
        [JsonPropertyName("value")]
        public string Value { get; init; }
        
        /// <summary>
        /// Gets the secret identifier URL.
        /// </summary>
        [JsonPropertyName("id")]
        public string Id { get; init; }
        
        /// <summary>
        /// Gets the secret name.
        /// </summary>
        [JsonPropertyName("name")]
        public string Name { get; init; }
    }

    /// <summary>
    /// Represents a JWT header for client assertion.
    /// </summary>
    internal class JwtHeader
    {
        /// <summary>
        /// Gets or sets the signing algorithm (RS256).
        /// </summary>
        [JsonPropertyName("alg")]
        public string Alg { get; set; }
        
        /// <summary>
        /// Gets or sets the token type (JWT).
        /// </summary>
        [JsonPropertyName("typ")]
        public string Typ { get; set; }
        
        /// <summary>
        /// Gets or sets the X.509 certificate thumbprint (Base64URL-encoded).
        /// </summary>
        [JsonPropertyName("x5t")]
        public string X5t { get; set; }
    }

    /// <summary>
    /// Represents a JWT payload for client assertion.
    /// </summary>
    internal class JwtPayload
    {
        /// <summary>
        /// Gets or sets the audience (token endpoint URL).
        /// </summary>
        [JsonPropertyName("aud")]
        public string Aud { get; set; }
        
        /// <summary>
        /// Gets or sets the expiration time (Unix timestamp).
        /// </summary>
        [JsonPropertyName("exp")]
        public long Exp { get; set; }
        
        /// <summary>
        /// Gets or sets the issuer (client ID).
        /// </summary>
        [JsonPropertyName("iss")]
        public string Iss { get; set; }
        
        /// <summary>
        /// Gets or sets the JWT ID (unique identifier).
        /// </summary>
        [JsonPropertyName("jti")]
        public string Jti { get; set; }
        
        /// <summary>
        /// Gets or sets the not-before time (Unix timestamp).
        /// </summary>
        [JsonPropertyName("nbf")]
        public long Nbf { get; set; }
        
        /// <summary>
        /// Gets or sets the subject (client ID).
        /// </summary>
        [JsonPropertyName("sub")]
        public string Sub { get; set; }
    }

    /// <summary>
    /// JSON serialization context for Azure API data transfer objects.
    /// Provides source-generated serialization for improved performance.
    /// </summary>
    [JsonSourceGenerationOptions(PropertyNamingPolicy = JsonKnownNamingPolicy.CamelCase, DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, WriteIndented = false, GenerationMode = JsonSourceGenerationMode.Default)]
    [JsonSerializable(typeof(TokenResponse))]
    [JsonSerializable(typeof(SecretResponse))]
    [JsonSerializable(typeof(KeyVaultCertificateResponse))]
    [JsonSerializable(typeof(KeyVaultOperationRequest))]
    [JsonSerializable(typeof(KeyVaultOperationResponse))]
    [JsonSerializable(typeof(JwtHeader))]
    [JsonSerializable(typeof(JwtPayload))]
    internal partial class AzureJsonContext : JsonSerializerContext
    {

    }
}