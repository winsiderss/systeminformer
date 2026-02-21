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
    public static class BuildAzure
    {
        //private static string ENTRA_TIMESTAMP_ALGORITHM;
        private static readonly string ENTRA_TIMESTAMP_SERVER;
        private static readonly string ENTRA_CERIFICATE_NAME;
        private static readonly string ENTRA_CERIFICATE_VAULT;
        private static readonly string ENTRA_TENANT_GUID;
        private static readonly string ENTRA_CLIENT_GUID;

        static BuildAzure()
        {
            //ENTRA_TIMESTAMP_ALGORITHM = Win32.GetEnvironmentVariable("BUILD_TIMESTAMP_ALGORITHM");
            ENTRA_TIMESTAMP_SERVER = Win32.GetEnvironmentVariable("BUILD_TIMESTAMP_SERVER");
            ENTRA_CERIFICATE_NAME = Win32.GetEnvironmentVariable("BUILD_ENTRA_CERT_ID");
            ENTRA_CERIFICATE_VAULT = Win32.GetEnvironmentVariable("BUILD_ENTRA_VAULT_ID");
            ENTRA_TENANT_GUID = Win32.GetEnvironmentVariable("BUILD_ENTRA_TENANT_ID");
            ENTRA_CLIENT_GUID = Win32.GetEnvironmentVariable("BUILD_ENTRA_CLIENT_ID");
            // Client secret intentionally not cached here to minimize lifetime.
        }

        /// <summary>
        /// Signs the files located at the specified path using configured certificate and timestamp server settings.
        /// </summary>
        /// <remarks>This method requires several configuration settings to be present, including the
        /// timestamp server, certificate name, certificate vault, tenant GUID, client GUID, and client secret. If any
        /// required configuration is missing, the method returns false and displays an error message.</remarks>
        /// <param name="Path">The path to the file or directory containing the files to be signed. Must not be null, empty, or whitespace.</param>
        /// <returns>true if the files are successfully signed; otherwise, false.</returns>
        public static async Task<bool> SignFiles(string Path)
        {
            //if (string.IsNullOrWhiteSpace(ENTRA_TIMESTAMP_ALGORITHM))
            //    return false;
            if (string.IsNullOrWhiteSpace(ENTRA_TIMESTAMP_SERVER))
            {
                Console.WriteLine($"{VT.RED}ENTRA TIMESTAMP SERVER{VT.RESET}");
                return false;
            }
            if (string.IsNullOrWhiteSpace(ENTRA_CERIFICATE_NAME))
            {
                Console.WriteLine($"{VT.RED}ENTRA CERIFICATE NAME{VT.RESET}");
                return false;
            }
            if (string.IsNullOrWhiteSpace(ENTRA_CERIFICATE_VAULT))
            {
                Console.WriteLine($"{VT.RED}ENTRA CERIFICATE VAULT{VT.RESET}");
                return false;
            }
            if (string.IsNullOrWhiteSpace(ENTRA_TENANT_GUID))
            {
                Console.WriteLine($"{VT.RED}ENTRA TENANT GUID{VT.RESET}");
                return false;
            }
            if (string.IsNullOrWhiteSpace(ENTRA_CLIENT_GUID))
            {
                Console.WriteLine($"{VT.RED}ENTRA CLIENT GUID{VT.RESET}");
                return false;
            }
            // Read the client secret from environment at time of use to reduce lifetime in memory.
            string entraClientSecret = Win32.GetEnvironmentVariable("BUILD_ENTRA_SECRET_ID");
            if (string.IsNullOrWhiteSpace(entraClientSecret))
            {
                Console.WriteLine($"{VT.RED}ENTRA CLIENT SECRET{VT.RESET}");
                return false;
            }

            try
            {
                return await SignFiles(
                    Path,
                    ENTRA_TIMESTAMP_SERVER,
                    ENTRA_CERIFICATE_NAME,
                    ENTRA_CERIFICATE_VAULT,
                    ENTRA_TENANT_GUID,
                    ENTRA_CLIENT_GUID,
                    entraClientSecret
                    );
            }
            finally
            {
                try { Environment.SetEnvironmentVariable("BUILD_ENTRA_SECRET_ID", null, EnvironmentVariableTarget.Process); } catch { }
                entraClientSecret = null;
            }
        }

        /// <summary>
        /// Signs files at the specified path using Azure Key Vault certificate and a timestamp server.
        /// </summary>
        /// <param name="Path">The path to the file or directory containing files to be signed.</param>
        /// <param name="TimeStampServer">The URL of the timestamp server to use for signing.</param>
        /// <param name="AzureCertName">The name of the Azure Key Vault certificate to use for signing.</param>
        /// <param name="AzureVaultName">The Azure Key Vault URI where the certificate is stored.</param>
        /// <param name="TenantGuid">The Azure Active Directory tenant GUID.</param>
        /// <param name="ClientGuid">The Azure Active Directory client (application) GUID.</param>
        /// <param name="ClientSecret">The client secret for the Azure application.</param>
        /// <returns>
        /// True if the files are successfully signed; otherwise, false. 
        /// The method retries up to three times in case of transient Azure connectivity issues.
        /// </returns>
        public static async Task<bool> SignFiles(
            string Path,
            string TimeStampServer,
            string AzureCertName,
            string AzureVaultName,
            string TenantGuid,
            string ClientGuid,
            string ClientSecret
            )
        {
            // Try a few times in case of transient Azure connectivity (dmex)

            for (int i = 0; i < 3; i++)
            {
                if (await KeyVaultDigestSignFiles(Path, TimeStampServer, AzureCertName, AzureVaultName, TenantGuid, ClientGuid, ClientSecret))
                    return true;

                Program.PrintColorMessage("Retrying....", ConsoleColor.Yellow);
            }

            Program.PrintColorMessage("KeyVaultDigestSignFiles Failed.", ConsoleColor.Red);
            return false;
        }

        /// <summary>
        /// Signs files at the specified path using an Azure Key Vault certificate and a timestamp server.
        /// </summary>
        /// <param name="Path">The path to the file or directory containing files to be signed.</param>
        /// <param name="TimeStampServer">The URL of the timestamp server to use for signing.</param>
        /// <param name="AzureCertName">The name of the Azure Key Vault certificate to use for signing.</param>
        /// <param name="AzureVaultName">The Azure Key Vault URI where the certificate is stored.</param>
        /// <param name="TenantGuid">The Azure Active Directory tenant GUID.</param>
        /// <param name="ClientGuid">The Azure Active Directory client (application) GUID.</param>
        /// <param name="ClientSecret">The client secret for the Azure application.</param>
        /// <returns>
        /// True if all files are successfully signed; otherwise, false. 
        /// Displays error messages for missing certificates, unsupported file types, or signing failures.
        /// </returns>
        public static async Task<bool> KeyVaultDigestSignFiles(
            string Path,
            string TimeStampServer,
            string AzureCertName,
            string AzureVaultName,
            string TenantGuid,
            string ClientGuid,
            string ClientSecret
            )
        {
            try
            {
                var certificateTimeStampServer = new TimeStampConfiguration(TimeStampServer, TimeStampType.RFC3161);

                // Obtain access token via REST
                string accessToken = await GetAzureADToken(TenantGuid, ClientGuid, ClientSecret);
                if (string.IsNullOrWhiteSpace(accessToken))
                {
                    Program.PrintColorMessage("Failed to obtain Azure AD token.", ConsoleColor.Red);
                    return false;
                }

                // Download certificate (public-key only) and key ID via REST
                var (azureCertificatePublic, keyId) = await DownloadCertificateAndKeyId(AzureVaultName, AzureCertName, accessToken);
                if (azureCertificatePublic == null)
                {
                    Program.PrintColorMessage($"Azure Certificate Failed.", ConsoleColor.Red);
                    return false;
                }

                if (keyId == null)
                {
                    Program.PrintColorMessage($"Unable to determine Key Id for certificate.", ConsoleColor.Red);
                    return false;
                }

                using (var azureCertificateRsa = RSAFactory.Create(accessToken, keyId, azureCertificatePublic))
                using (var authenticodeKeyVaultSigner = new AuthenticodeKeyVaultSigner(azureCertificateRsa, azureCertificatePublic, HashAlgorithmName.SHA256, certificateTimeStampServer, null))
                {
                    if (Directory.Exists(Path))
                    {
                        var files = Utils.EnumerateDirectory(Path, [".exe", ".dll"], ["ksi.dll"]);

                        if (files == null || files.Count == 0)
                        {
                            Program.PrintColorMessage($"No files found.", ConsoleColor.Red);
                        }
                        else
                        {
                            foreach (var file in files)
                            {
                                var result = authenticodeKeyVaultSigner.SignFile(file, null, null);

                                if (result == HRESULT.S_OK)
                                    Program.PrintColorMessage($"Signed: {file}", ConsoleColor.Green);
                                else
                                    Program.PrintColorMessage($"Failed: ({result}) {file}", ConsoleColor.Red);
                            }
                        }
                    }
                    else if (File.Exists(Path))
                    {
                        var result = authenticodeKeyVaultSigner.SignFile(Path, null, null);

                        if (result == HRESULT.COR_E_BADIMAGEFORMAT)
                        {
                            Program.PrintColorMessage($"[ERROR] The AppxManifest.xml publisher CN does not match the certificate: ({result}) {Path}", ConsoleColor.Red);
                            return false;
                        }
                        else if (result == HRESULT.TRUST_E_SUBJECT_FORM_UNKNOWN)
                        {
                            Program.PrintColorMessage($"[ERROR] File content not supported: ({result}) {Path}", ConsoleColor.Red);
                            return false;
                        }
                        else if (result != HRESULT.S_OK)
                        {
                            Program.PrintColorMessage($"[ERROR] ({result}) {Path}", ConsoleColor.Red);
                            return false;
                        }

                        Program.PrintColorMessage($"Success: {Path}", ConsoleColor.Green);
                    }
                    else
                    {
                        return false;
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static async Task<string> GetAzureADToken(string TenantId, string ClientId, string ClientSecret)
        {
            using var request = new HttpRequestMessage(HttpMethod.Post, $"https://login.microsoftonline.com/{TenantId}/oauth2/v2.0/token")
            {
                Content = new FormUrlEncodedContent(
                [
                    new KeyValuePair<string, string>("client_id", ClientId),
                    new KeyValuePair<string, string>("client_secret", ClientSecret),
                    new KeyValuePair<string, string>("scope", "https://vault.azure.net/.default"),
                    new KeyValuePair<string, string>("grant_type", "client_credentials"),
                ])
            };

            var response = await BuildHttpClient.SendMessage(request);

            if (string.IsNullOrWhiteSpace(response))
                return null;

            var tokenResponse = JsonSerializer.Deserialize(response, AzureJsonContext.Default.TokenResponse);
            return tokenResponse.AccessToken;
        }

        public static async Task<(X509Certificate2 Certificate, Uri KeyId)> DownloadCertificateAndKeyId(string BaseUrl, string Name, string Token)
        {
            X509Certificate2 cert = null;
            Uri keyId = null;

            // Get certificate with public key (no private key)
            using var request = new HttpRequestMessage(HttpMethod.Get, $"{BaseUrl}/certificates/{Name}?api-version=2025-07-01");
            request.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
            request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", Token);

            var response = await BuildHttpClient.SendMessage(request);
            if (string.IsNullOrWhiteSpace(response))
                return (null, null);

            var certificateResponse = JsonSerializer.Deserialize(response, AzureJsonContext.Default.KeyVaultCertificateResponse);
            string base64 = certificateResponse.CertificateString;
            string keyIdStr = certificateResponse.Kid;

            if (string.IsNullOrWhiteSpace(keyIdStr))
            {
                if (!string.IsNullOrWhiteSpace(certificateResponse.Key?.Kid))
                {
                    keyIdStr = certificateResponse.Key.Kid;
                }
                else if (!string.IsNullOrWhiteSpace(certificateResponse.Id))
                {
                    // best-effort: transform certificate id to key id by replacing /certificates/ with /keys/
                    keyIdStr = certificateResponse.Id.Replace("/certificates/", "/keys/", StringComparison.OrdinalIgnoreCase);
                }
            }

            if (!string.IsNullOrWhiteSpace(base64))
            {
                try
                {
                    cert = X509CertificateLoader.LoadCertificate(Convert.FromBase64String(base64));
                }
                catch (Exception ex)
                {
                    Program.PrintColorMessage($"[ERROR - X509CertificateLoader.LoadCertificate] {ex.Message}", ConsoleColor.Red);
                    return (null, null);
                }
            }

            if (!string.IsNullOrWhiteSpace(keyIdStr))
            {
                try { keyId = new Uri(keyIdStr); } catch { keyId = null; }
            }

            return (cert, keyId);
        }

        public static async Task<X509Certificate2> DownloadCertificateSecret(string BaseUrl, string Name, string Token)
        {
            // Get certificate with private key
            using var request = new HttpRequestMessage(HttpMethod.Get, $"{BaseUrl}/secrets/{Name}?api-version=2025-07-01");
            request.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
            request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", Token);

            var response = await BuildHttpClient.SendMessage(request);
            if (string.IsNullOrWhiteSpace(response))
                return null;

            var secretResponse = JsonSerializer.Deserialize(response, AzureJsonContext.Default.SecretResponse);

            if (!string.IsNullOrWhiteSpace(secretResponse.Value))
            {
                var base64 = secretResponse.Value; // base64-encoded PFX (PKCS#12)
                return X509CertificateLoader.LoadCertificate(Convert.FromBase64String(base64));
            }

            return null;
        }
    }
}