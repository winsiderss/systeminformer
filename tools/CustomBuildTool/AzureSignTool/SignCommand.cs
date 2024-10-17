namespace CustomBuildTool
{
    internal sealed class SignCommand
    {
        internal static readonly string CodeSigningOid = "1.3.6.1.5.5.7.3.3";

        internal static bool IsSigned(string filePath)
        {
            try
            {
                var certificate =  new X509Certificate2(X509Certificate.CreateFromSignedFile(filePath));

                // check if file contains a code signing cert.
                // Note that this does not check validity of the signature
                return certificate.Extensions
                    .Select(extension => extension as X509EnhancedKeyUsageExtension)
                    .Select(enhancedExtension => enhancedExtension?.EnhancedKeyUsages)
                    .Any(oids => oids?[CodeSigningOid] != null);
            }
            catch (Exception)
            {
                return false;
            }
        }

        private static X509Certificate2Collection GetAdditionalCertificates(IEnumerable<string> paths)
        {
            var collection = new X509Certificate2Collection();
            try
            {
                foreach (var path in paths)
                {

                    var type = X509Certificate2.GetCertContentType(path);
                    switch (type)
                    {
                        case X509ContentType.Cert:
                        case X509ContentType.Authenticode:
                        case X509ContentType.SerializedCert:
                            var certificate = new X509Certificate2(path);
                            //logger.LogTrace($"Including additional certificate {certificate.Thumbprint}.");
                            collection.Add(certificate);
                            break;
                        default:
                            throw new Exception($"Specified file {path} is not a public valid certificate.");
                    }
                }
            }
            catch (Exception e)
            {
                Program.PrintColorMessage("[ERROR] An exception occurred while including an additional certificate.", ConsoleColor.Red);
            }

            return collection;
        }
    }
}
