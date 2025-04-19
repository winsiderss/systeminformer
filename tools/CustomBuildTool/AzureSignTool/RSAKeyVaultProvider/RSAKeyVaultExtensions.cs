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
    /// Extensions for creating RSA objects from a Key Vault client.
    /// </summary>
    public static class RSAFactory
    {
        /// <summary>
        /// Creates an RSA object
        /// </summary>
        /// <param name="credential"></param>
        /// <param name="keyId"></param>
        /// <param name="key"></param>
        /// <returns></returns>
        public static RSA Create(TokenCredential credential, Uri keyId, JsonWebKey key)
        {
            ArgumentNullException.ThrowIfNull(credential);
            ArgumentNullException.ThrowIfNull(keyId);
            ArgumentNullException.ThrowIfNull(key);

            return new RSAKeyVault(new KeyVaultContext(credential, keyId, key));
        }

        /// <summary>
        /// Creates an RSA object
        /// </summary>
        /// <param name="credential"></param>
        /// <param name="keyId"></param>
        /// <param name="publicCertificate"></param>
        /// <returns></returns>
        public static RSA Create(TokenCredential credential, Uri keyId, X509Certificate2 publicCertificate)
        {
            ArgumentNullException.ThrowIfNull(credential);
            ArgumentNullException.ThrowIfNull(keyId);
            ArgumentNullException.ThrowIfNull(publicCertificate);

            return new RSAKeyVault(new KeyVaultContext(credential, keyId, publicCertificate));
        }
    }
}
