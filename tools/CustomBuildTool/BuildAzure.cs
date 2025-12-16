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
        private static readonly string ENTRA_CLIENT_SECRET;

        static BuildAzure()
        {
            //ENTRA_TIMESTAMP_ALGORITHM = Win32.GetEnvironmentVariable("BUILD_TIMESTAMP_ALGORITHM");
            ENTRA_TIMESTAMP_SERVER = Win32.GetEnvironmentVariable("BUILD_TIMESTAMP_SERVER");
            ENTRA_CERIFICATE_NAME = Win32.GetEnvironmentVariable("BUILD_ENTRA_CERT_ID");
            ENTRA_CERIFICATE_VAULT = Win32.GetEnvironmentVariable("BUILD_ENTRA_VAULT_ID");
            ENTRA_TENANT_GUID = Win32.GetEnvironmentVariable("BUILD_ENTRA_TENANT_ID");
            ENTRA_CLIENT_GUID = Win32.GetEnvironmentVariable("BUILD_ENTRA_CLIENT_ID");
            ENTRA_CLIENT_SECRET = Win32.GetEnvironmentVariable("BUILD_ENTRA_SECRET_ID");
        }

        /// <summary>
        /// Signs the files located at the specified path using configured certificate and timestamp server settings.
        /// </summary>
        /// <remarks>This method requires several configuration settings to be present, including the
        /// timestamp server, certificate name, certificate vault, tenant GUID, client GUID, and client secret. If any
        /// required configuration is missing, the method returns false and displays an error message.</remarks>
        /// <param name="Path">The path to the file or directory containing the files to be signed. Must not be null, empty, or whitespace.</param>
        /// <returns>true if the files are successfully signed; otherwise, false.</returns>
        public static bool SignFiles(string Path)
        {
            //if (string.IsNullOrWhiteSpace(ENTRA_TIMESTAMP_ALGORITHM))
            //    return false;
            if (string.IsNullOrWhiteSpace(ENTRA_TIMESTAMP_SERVER))
            {
                Program.PrintColorMessage("ENTRA_TIMESTAMP_SERVER", ConsoleColor.Red);
                return false;
            }
            if (string.IsNullOrWhiteSpace(ENTRA_CERIFICATE_NAME))
            {
                Program.PrintColorMessage("ENTRA_CERIFICATE_NAME", ConsoleColor.Red);
                return false;
            }
            if (string.IsNullOrWhiteSpace(ENTRA_CERIFICATE_VAULT))
            {
                Program.PrintColorMessage("ENTRA_CERIFICATE_VAULT", ConsoleColor.Red);
                return false;
            }
            if (string.IsNullOrWhiteSpace(ENTRA_TENANT_GUID))
            {
                Program.PrintColorMessage("ENTRA_TENANT_GUID", ConsoleColor.Red);
                return false;
            }
            if (string.IsNullOrWhiteSpace(ENTRA_CLIENT_GUID))
            {
                Program.PrintColorMessage("ENTRA_CLIENT_GUID", ConsoleColor.Red);
                return false;
            }
            if (string.IsNullOrWhiteSpace(ENTRA_CLIENT_SECRET))
            {
                Program.PrintColorMessage("ENTRA_CLIENT_SECRET", ConsoleColor.Red);
                return false;
            }

            return SignFiles(
                Path,
                ENTRA_TIMESTAMP_SERVER,
                ENTRA_CERIFICATE_NAME,
                ENTRA_CERIFICATE_VAULT,
                ENTRA_TENANT_GUID,
                ENTRA_CLIENT_GUID,
                ENTRA_CLIENT_SECRET
                );
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
        public static bool SignFiles(
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
                if (KeyVaultDigestSignFiles(Path, TimeStampServer, AzureCertName, AzureVaultName, TenantGuid, ClientGuid, ClientSecret))
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
        public static bool KeyVaultDigestSignFiles(
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
                var certificateCredential = new Azure.Identity.ClientSecretCredential(TenantGuid, ClientGuid, ClientSecret);
                var certificateClient = new Azure.Security.KeyVault.Certificates.CertificateClient(new Uri(AzureVaultName), certificateCredential);
                var certificateBuffer = certificateClient.GetCertificateAsync(AzureCertName).GetAwaiter().GetResult();

                if (!certificateBuffer.HasValue)
                {
                    Program.PrintColorMessage($"Azure Certificate Failed.", ConsoleColor.Red);
                    return false;
                }

                using (var azureCertificatePublic = X509CertificateLoader.LoadCertificate(certificateBuffer.Value.Cer))
                using (var azureCertificateRsa = RSAFactory.Create(certificateCredential, certificateBuffer.Value.KeyId, azureCertificatePublic))
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

        public static string GetAzureADTokenAsync(string TenantId, string ClientId, string ClientSecret)
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

            var response = BuildHttpClient.SendMessage(request);
            using var doc = JsonDocument.Parse(response);
            return doc.RootElement.GetProperty("access_token").GetString();
        }

        public static X509Certificate2 DownloadCertificateAsync(string BaseUrl, string Name, string Token)
        {
            // Get certificate metadata (public key only)
            using var request = new HttpRequestMessage(HttpMethod.Get, $"{BaseUrl}/certificates/{Name}?api-version=7.4")
            {
                Headers = { 
                    Accept = { new MediaTypeWithQualityHeaderValue("application/json") } ,
                    Authorization = new AuthenticationHeaderValue("Bearer", Token)
                },
            };

            var response = BuildHttpClient.SendMessage(request);
            using var doc = JsonDocument.Parse(response);
            string base64 = doc.RootElement.GetProperty("cer").GetString(); // base64-encoded DER certificate
            return X509CertificateLoader.LoadCertificate(Convert.FromBase64String(base64));
        }

        public static X509Certificate2 DownloadCertificateSecretAsync(string BaseUrl, string Name, string Token)
        {
            // Get certificate with private key
            using var request = new HttpRequestMessage(HttpMethod.Get, $"{BaseUrl}/secrets/{Name}?api-version=7.4")
            {
                Headers = {
                    Accept = { new MediaTypeWithQualityHeaderValue("application/json") } ,
                    Authorization = new AuthenticationHeaderValue("Bearer", Token)
                },
            };

            var response = BuildHttpClient.SendMessage(request);
            using var doc = JsonDocument.Parse(response);
            string base64 = doc.RootElement.GetProperty("value").GetString(); // base64-encoded PFX (PKCS#12)
            return X509CertificateLoader.LoadCertificate(Convert.FromBase64String(base64));
        }
    }
}