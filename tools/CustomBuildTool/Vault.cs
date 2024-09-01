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
    public static class Vault
    {
        private static string ENTRA_TIMESTAMP_ALGORITHM;
        private static string ENTRA_TIMESTAMP_SERVER;
        private static string ENTRA_CERIFICATE_NAME;
        private static string ENTRA_CERIFICATE_VAULT;
        private static string ENTRA_TENANT_GUID;
        private static string ENTRA_CLIENT_GUID;
        private static string ENTRA_CLIENT_SECRET;

        static Vault()
        {
            //ENTRA_TIMESTAMP_ALGORITHM = Win32.GetEnvironmentVariable("BUILD_TIMESTAMP_ALGORITHM");
            //ENTRA_TIMESTAMP_SERVER = Win32.GetEnvironmentVariable("BUILD_TIMESTAMP_SERVER");
            ENTRA_CERIFICATE_NAME = Win32.GetEnvironmentVariable("BUILD_ENTRA_CERT_ID");
            ENTRA_CERIFICATE_VAULT = Win32.GetEnvironmentVariable("BUILD_ENTRA_VAULT_ID");
            ENTRA_TENANT_GUID = Win32.GetEnvironmentVariable("BUILD_ENTRA_TENANT_ID");
            ENTRA_CLIENT_GUID = Win32.GetEnvironmentVariable("BUILD_ENTRA_CLIENT_ID");
            ENTRA_CLIENT_SECRET = Win32.GetEnvironmentVariable("BUILD_ENTRA_SECRET_ID");
        }

        public static bool SignFiles(string Path, string SearchPattern)
        {
            //if (string.IsNullOrWhiteSpace(ENTRA_TIMESTAMP_ALGORITHM))
            //    return false;
            //if (string.IsNullOrWhiteSpace(ENTRA_TIMESTAMP_SERVER))
            //    return false;
            if (string.IsNullOrWhiteSpace(ENTRA_CERIFICATE_NAME))
                return false;
            if (string.IsNullOrWhiteSpace(ENTRA_CERIFICATE_VAULT))
                return false;
            if (string.IsNullOrWhiteSpace(ENTRA_TENANT_GUID))
                return false;
            if (string.IsNullOrWhiteSpace(ENTRA_CLIENT_GUID))
                return false;
            if (string.IsNullOrWhiteSpace(ENTRA_CLIENT_SECRET))
                return false;

            try
            {
                var timeStampConfiguration = new TimeStampConfiguration("https://timestamp.digicert.com", HashAlgorithmName.SHA256, TimeStampType.RFC3161);
                var clientCredential = new Azure.Identity.ClientSecretCredential(ENTRA_TENANT_GUID, ENTRA_CLIENT_GUID, ENTRA_CLIENT_SECRET);

                if (clientCredential == null)
                {
                    Program.PrintColorMessage($"[ERROR] Identity", ConsoleColor.Red);
                    return false;
                }

                var certificateClient = new Azure.Security.KeyVault.Certificates.CertificateClient(new Uri(ENTRA_CERIFICATE_VAULT), clientCredential);
                var azureCertificate = certificateClient.GetCertificateAsync("mynewcert").GetAwaiter().GetResult();

                if (!azureCertificate.HasValue)
                    return false;

                var certificateRaw = new X509Certificate2(azureCertificate.Value.Cer);
                var azureKeyVault = new AzureKeyVaultMaterializedConfiguration(clientCredential, certificateRaw, azureCertificate.Value.KeyId);

                using (var rsa = RSAKeyVaultProvider.RSAFactory.Create(azureKeyVault.TokenCredential, azureKeyVault.KeyId, azureKeyVault.PublicCertificate))
                using (var sig = new AuthenticodeKeyVaultSigner(rsa, azureKeyVault.PublicCertificate, HashAlgorithmName.SHA256, timeStampConfiguration, null))
                {
                    var files = Directory.EnumerateFiles(Path, SearchPattern, SearchOption.AllDirectories);

                    foreach (var file in files)
                    {
                        var result = sig.SignFile(file, null, null, true);

                        if (result == HRESULT.COR_E_BADIMAGEFORMAT)
                        {
                            Program.PrintColorMessage($"[ERROR] The AppxManifest.xml publisher CN does not match the certificate: {file}", ConsoleColor.Red);
                        }
                        else if (result == HRESULT.TRUST_E_SUBJECT_FORM_UNKNOWN)
                        {
                            Console.WriteLine($"[ERROR] The file type is not supported or corrupt: {file}", ConsoleColor.Red);
                        }
                        else if (result == HRESULT.S_OK)
                        {
                            Console.WriteLine($"[Verified] {file}", ConsoleColor.Green);
                        }
                        else
                        {
                            Console.WriteLine($"[ERROR] Unknown failure: ({result}) {file}", ConsoleColor.Red);
                        }
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
    }
}
