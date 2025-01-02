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
    public static class EntraKeyVault
    {
        //private static string ENTRA_TIMESTAMP_ALGORITHM;
        private static readonly string ENTRA_TIMESTAMP_SERVER;
        private static readonly string ENTRA_CERIFICATE_NAME;
        private static readonly string ENTRA_CERIFICATE_VAULT;
        private static readonly string ENTRA_TENANT_GUID;
        private static readonly string ENTRA_CLIENT_GUID;
        private static readonly string ENTRA_CLIENT_SECRET;

        static EntraKeyVault()
        {
            //ENTRA_TIMESTAMP_ALGORITHM = Win32.GetEnvironmentVariable("BUILD_TIMESTAMP_ALGORITHM");
            ENTRA_TIMESTAMP_SERVER = Win32.GetEnvironmentVariable("BUILD_TIMESTAMP_SERVER");
            ENTRA_CERIFICATE_NAME = Win32.GetEnvironmentVariable("BUILD_ENTRA_CERT_ID");
            ENTRA_CERIFICATE_VAULT = Win32.GetEnvironmentVariable("BUILD_ENTRA_VAULT_ID");
            ENTRA_TENANT_GUID = Win32.GetEnvironmentVariable("BUILD_ENTRA_TENANT_ID");
            ENTRA_CLIENT_GUID = Win32.GetEnvironmentVariable("BUILD_ENTRA_CLIENT_ID");
            ENTRA_CLIENT_SECRET = Win32.GetEnvironmentVariable("BUILD_ENTRA_SECRET_ID");
        }

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
            try
            {
                var azureCertificateTimeStamp = new TimeStampConfiguration(TimeStampServer, TimeStampType.RFC3161);
                var azureCertificateCredential = new Azure.Identity.ClientSecretCredential(TenantGuid, ClientGuid, ClientSecret);
                var azureCertificateClient = new Azure.Security.KeyVault.Certificates.CertificateClient(new Uri(AzureVaultName), azureCertificateCredential);
                var azureCertificateBuffer = azureCertificateClient.GetCertificateAsync(AzureCertName).GetAwaiter().GetResult();

                if (!azureCertificateBuffer.HasValue)
                {
                    Program.PrintColorMessage($"Azure Certificate Failed.", ConsoleColor.Red);
                    return false;
                }

                using (var azureCertificatePublic = X509CertificateLoader.LoadCertificate(azureCertificateBuffer.Value.Cer))
                using (var azureCertificateRsa = RSAFactory.Create(azureCertificateCredential, azureCertificateBuffer.Value.KeyId, azureCertificatePublic))
                using (var authenticodeKeyVaultSigner = new AuthenticodeKeyVaultSigner(azureCertificateRsa, azureCertificatePublic, HashAlgorithmName.SHA256, azureCertificateTimeStamp, null))
                {
                    if (Directory.Exists(Path))
                    {
                        var files = Directory.EnumerateFiles(Path, "*.dll,*.exe", SearchOption.AllDirectories);

                        foreach (var file in files)
                        {
                            var result = authenticodeKeyVaultSigner.SignFile(file, null, null, true);

                            if (result == HRESULT.S_OK)
                                Program.PrintColorMessage($"Signed: {file}", ConsoleColor.Green);
                            else
                                Program.PrintColorMessage($"Failed: ({result}) {file}", ConsoleColor.Red);
                        }
                    }
                    else if (File.Exists(Path))
                    {
                        var result = authenticodeKeyVaultSigner.SignFile(Path, null, null, true);

                        //if (result == HRESULT.COR_E_BADIMAGEFORMAT)
                        //{
                        //    Program.PrintColorMessage($"[ERROR] The AppxManifest.xml publisher CN does not match the certificate: ({result}) {Path}", ConsoleColor.Red);
                        //}
                        //else if (result == HRESULT.TRUST_E_SUBJECT_FORM_UNKNOWN)
                        //{
                        //    Program.PrintColorMessage($"[ERROR] File content not supported: ({result}) {Path}", ConsoleColor.Red);
                        //}

                        if (result == HRESULT.S_OK)
                            Program.PrintColorMessage($"Signed: {Path}", ConsoleColor.Green);
                        else
                            Program.PrintColorMessage($"Failed: ({result}) {Path}", ConsoleColor.Red);
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
