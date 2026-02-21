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
        /// <param name="AccessToken"></param>
        /// <param name="KeyId"></param>
        /// <param name="PublicCertificate"></param>
        /// <returns></returns>
        public static RSA Create(string AccessToken, Uri KeyId, X509Certificate2 PublicCertificate)
        {
            ArgumentNullException.ThrowIfNull(AccessToken);
            ArgumentNullException.ThrowIfNull(KeyId);
            ArgumentNullException.ThrowIfNull(PublicCertificate);

            return new RSAKeyVault(new KeyVaultContext(AccessToken, KeyId, PublicCertificate));
        }
    }
}