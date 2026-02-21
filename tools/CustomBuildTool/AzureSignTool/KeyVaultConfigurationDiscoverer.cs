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
    // https://learn.microsoft.com/en-us/rest/api/keyvault/certificates/get-certificate/get-certificate?tabs=HTTP

    //public class AccessTokenCredential : TokenCredential
    //{
    //    private readonly AccessToken accessToken;
    //
    //    public AccessTokenCredential(string token, DateTimeOffset expiresOn)
    //    {
    //        ArgumentException.ThrowIfNullOrWhiteSpace(token); // "Access Token cannot be null or empty"
    //
    //        accessToken = new AccessToken(token, expiresOn);
    //    }
    //
    //    public AccessTokenCredential(string token)
    //    {
    //        ArgumentException.ThrowIfNullOrWhiteSpace(token); // "Access Token cannot be null or empty"
    //
    //        accessToken = new AccessToken(token, DateTimeOffset.UtcNow.AddHours(1));
    //    }
    //
    //    public override AccessToken GetToken(TokenRequestContext requestContext, CancellationToken cancellationToken)
    //    {
    //        return accessToken;
    //    }
    //
    //    public override ValueTask<AccessToken> GetTokenAsync(TokenRequestContext requestContext, CancellationToken cancellationToken)
    //    {
    //        return new ValueTask<AccessToken>(accessToken);
    //    }
    //}

    public enum KeyVaultSignatureAlgorithm
    {
        RSAPkcs15,
        ECDsa
    }

    // JWS / JWT signature algorithm identifiers used by the codebase.
    public enum SignatureAlgorithm
    {
        RSNULL,
        RS256,
        RS384,
        RS512,
        ES256,
        ES384,
        ES512
    }

    public static class SignatureAlgorithmTranslator
    {
        // Return the JWS alg identifier string for the specified key and hash algorithm.
        public static string SignatureAlgorithmToJwsAlgId(KeyVaultSignatureAlgorithm signatureAlgorithm, HashAlgorithmName hashAlgorithmName)
        {
            switch (signatureAlgorithm)
            {
                case KeyVaultSignatureAlgorithm.RSAPkcs15:
                    if (hashAlgorithmName == HashAlgorithmName.SHA256)
                    {
                        return "RS256";
                    }

                    if (hashAlgorithmName == HashAlgorithmName.SHA384)
                    {
                        return "RS384";
                    }

                    if (hashAlgorithmName == HashAlgorithmName.SHA512)
                    {
                        return "RS512";
                    }

                    break;
                case KeyVaultSignatureAlgorithm.ECDsa:
                    if (hashAlgorithmName == HashAlgorithmName.SHA256)
                    {
                        return "ES256";
                    }

                    if (hashAlgorithmName == HashAlgorithmName.SHA384)
                    {
                        return "ES384";
                    }

                    if (hashAlgorithmName == HashAlgorithmName.SHA512)
                    {
                        return "ES512";
                    }

                    break;
            }

            throw new NotSupportedException("The algorithm specified is not supported.");
        }
    }
}
