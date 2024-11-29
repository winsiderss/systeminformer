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

        public static bool SignFiles(string Path, string SearchPattern)
        {
            //if (string.IsNullOrWhiteSpace(ENTRA_TIMESTAMP_ALGORITHM))
            //    return false;
            if (string.IsNullOrWhiteSpace(ENTRA_TIMESTAMP_SERVER))
                return false;
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
                var azureCertificateTimeStamp = new TimeStampConfiguration(ENTRA_TIMESTAMP_SERVER, TimeStampType.RFC3161);
                var azureCertificateCredential = new global::Azure.Identity.ClientSecretCredential(ENTRA_TENANT_GUID, ENTRA_CLIENT_GUID, ENTRA_CLIENT_SECRET);
                var azureCertificateClient = new global::Azure.Security.KeyVault.Certificates.CertificateClient(new Uri(ENTRA_CERIFICATE_VAULT), azureCertificateCredential);
                var azureCertificateBuffer = azureCertificateClient.GetCertificateAsync(ENTRA_CERIFICATE_NAME).GetAwaiter().GetResult();

                if (!azureCertificateBuffer.HasValue)
                {
                    Program.PrintColorMessage($"Azure Certificate Failed.", ConsoleColor.Red);
                    return false;
                }

#pragma warning disable SYSLIB0057 // Type or member is obsolete
                using var azureCertificatePublic = new X509Certificate2(azureCertificateBuffer.Value.Cer);
#pragma warning restore SYSLIB0057 // Type or member is obsolete
                using var azureCertificateRsa = RSAFactory.Create(azureCertificateCredential, azureCertificateBuffer.Value.KeyId, azureCertificatePublic);

                if (azureCertificateRsa == null)
                {
                    Program.PrintColorMessage($"Azure RSAFactory Failed.", ConsoleColor.Red);
                    return false;
                }

                using var authenticodeKeyVaultSigner = new AuthenticodeKeyVaultSigner(
                    azureCertificateRsa, 
                    azureCertificatePublic,
                    HashAlgorithmName.SHA256, 
                    azureCertificateTimeStamp, 
                    null);

                var files = Directory.EnumerateFiles(
                    Path, 
                    SearchPattern, 
                    SearchOption.AllDirectories
                    );

                foreach (var file in files)
                {
                    var result = authenticodeKeyVaultSigner.SignFile(file, null, null, true);

                    if (result == HRESULT.COR_E_BADIMAGEFORMAT)
                    {
                        Program.PrintColorMessage($"[ERROR] The AppxManifest.xml publisher CN does not match the certificate: ({result}) {file}", ConsoleColor.Red);
                    }
                    else if (result == HRESULT.TRUST_E_SUBJECT_FORM_UNKNOWN)
                    {
                        Program.PrintColorMessage($"[ERROR] File content not supported: ({result}) {file}", ConsoleColor.Red);
                    }
                    else if (result == HRESULT.S_OK)
                    {
                        Program.PrintColorMessage($"Signed: {file}", ConsoleColor.Green);
                    }
                    else
                    {
                        Program.PrintColorMessage($"[ERROR] Unknown failure: ({result}) {file}", ConsoleColor.Red);
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
