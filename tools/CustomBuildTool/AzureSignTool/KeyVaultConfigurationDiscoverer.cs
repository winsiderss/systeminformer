namespace CustomBuildTool
{
    // https://learn.microsoft.com/en-us/rest/api/keyvault/certificates/get-certificate/get-certificate?tabs=HTTP

    public class AccessTokenCredential : TokenCredential
    {
        private readonly AccessToken accessToken;

        public AccessTokenCredential(string token, DateTimeOffset expiresOn)
        {
            if (string.IsNullOrWhiteSpace(token))
            {
                throw new ArgumentException("Access Token cannot be null or empty", nameof(token));
            }

            accessToken = new AccessToken(token, expiresOn);
        }

        public AccessTokenCredential(string token) : this(token, DateTimeOffset.UtcNow.AddHours(1))
        {

        }

        public override AccessToken GetToken(TokenRequestContext requestContext, CancellationToken cancellationToken)
        {
            return accessToken;
        }

        public override ValueTask<AccessToken> GetTokenAsync(TokenRequestContext requestContext, CancellationToken cancellationToken)
        {
            return new ValueTask<AccessToken>(accessToken);
        }
    }

    public enum KeyVaultSignatureAlgorithm
    {
        RSAPkcs15,
        ECDsa
    }

    public static class SignatureAlgorithmTranslator
    {
        public static SignatureAlgorithm SignatureAlgorithmToJwsAlgId(KeyVaultSignatureAlgorithm signatureAlgorithm, HashAlgorithmName hashAlgorithmName)
        {
            switch (signatureAlgorithm)
            {
                case KeyVaultSignatureAlgorithm.RSAPkcs15:
                    if (hashAlgorithmName == HashAlgorithmName.SHA1)
                    {
                        return new SignatureAlgorithm("RSNULL");
                    }

                    if (hashAlgorithmName == HashAlgorithmName.SHA256)
                    {
                        return SignatureAlgorithm.RS256;
                    }

                    if (hashAlgorithmName == HashAlgorithmName.SHA384)
                    {
                        return SignatureAlgorithm.RS384;
                    }

                    if (hashAlgorithmName == HashAlgorithmName.SHA512)
                    {
                        return SignatureAlgorithm.RS512;
                    }

                    break;
                case KeyVaultSignatureAlgorithm.ECDsa:
                    if (hashAlgorithmName == HashAlgorithmName.SHA256)
                    {
                        return SignatureAlgorithm.ES256;
                    }

                    if (hashAlgorithmName == HashAlgorithmName.SHA384)
                    {
                        return SignatureAlgorithm.ES384;
                    }

                    if (hashAlgorithmName == HashAlgorithmName.SHA512)
                    {
                        return SignatureAlgorithm.ES512;
                    }

                    break;
            }

            throw new NotSupportedException("The algorithm specified is not supported.");
        }
    }
}
