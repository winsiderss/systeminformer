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

    /// <summary>
    /// Provides methods for translating signature algorithm and hash algorithm combinations to their corresponding JSON
    /// Web Signature (JWS) algorithm identifiers.
    /// </summary>
    /// <remarks>This class is intended for use when interoperating with systems that require JWS algorithm
    /// identifiers, such as JWT libraries or web authentication protocols. All methods are static and
    /// thread-safe.</remarks>
    public static class SignatureAlgorithmTranslator
    {
        /// <summary>
        /// Maps a Key Vault signature algorithm and hash algorithm to the corresponding JWS algorithm identifier.
        /// </summary>
        /// <remarks>Use this method to obtain the JWS algorithm identifier required for JWT signing when
        /// using Azure Key Vault signature algorithms. Only specific combinations of signature and hash algorithms are
        /// supported.</remarks>
        /// <param name="signatureAlgorithm">The signature algorithm to convert. Supported values are RSAPkcs15 and ECDsa.</param>
        /// <param name="hashAlgorithmName">The hash algorithm used with the signature algorithm. Supported values are SHA256, SHA384, and SHA512.</param>
        /// <returns>A string containing the JWS algorithm identifier that matches the specified signature and hash algorithm.</returns>
        /// <exception cref="NotSupportedException">Thrown if the combination of signature algorithm and hash algorithm is not supported.</exception>
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
