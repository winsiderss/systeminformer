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
    public static class Http
    {
        private static readonly string[] TrustedSubjects =
        {
            "CN=*.github.com",
            "CN=*.github.com, O=\"GitHub, Inc.\", L=San Francisco, S=California, C=US",
            "CN=sourceforge.io, O=\"Cloudflare, Inc.\", L=San Francisco, S=California, C=US"
        };

        private static bool CertValidationCallback(
            HttpRequestMessage Rquest,
            X509Certificate Cert,
            X509Chain Chain,
            SslPolicyErrors Errors
            )
        {
            if (
                Errors == SslPolicyErrors.None && 
                TrustedSubjects.Contains(Cert.Subject, StringComparer.OrdinalIgnoreCase)
                )
            {
                return true;
            }

            Program.PrintColorMessage($"[CertValidationCallback] {Cert.Subject}", ConsoleColor.Red);
            return false;
        }

        public static HttpClient CreateHttpClient()
        {
            HttpClientHandler httpClientHandler = new HttpClientHandler();
            httpClientHandler.AutomaticDecompression = DecompressionMethods.All;
            httpClientHandler.SslProtocols = SslProtocols.Tls12 | SslProtocols.Tls13;
            httpClientHandler.ServerCertificateCustomValidationCallback = CertValidationCallback;

            HttpClient httpClient = new HttpClient(httpClientHandler);
            httpClient.DefaultRequestVersion = HttpVersion.Version20;
            httpClient.DefaultVersionPolicy = HttpVersionPolicy.RequestVersionOrHigher;

            return httpClient;
        }

        public static HttpResponseMessage SendMessageResponse(HttpRequestMessage HttpMessage)
        {
            HttpResponseMessage response = null;

            try
            {
                response = Task.Run(async () => await CreateHttpClient().SendAsync(HttpMessage)).GetAwaiter().GetResult();
            }
            catch (Exception)
            {
                return null;
            }

            return response;
        }

        public static string SendMessage(HttpRequestMessage HttpMessage)
        {
            string response = null;

            try
            {
                var httpTask = Task.Run(async () => await CreateHttpClient().SendAsync(HttpMessage)).GetAwaiter().GetResult();

                if (httpTask.IsSuccessStatusCode)
                {
                    response = Task.Run(httpTask.Content.ReadAsStringAsync).GetAwaiter().GetResult();
                }
            }
            catch (Exception)
            {
                return null;
            }

            return response;
        }

        public static TValue SendMessage<TValue>(HttpRequestMessage HttpMessage, JsonTypeInfo<TValue> jsonTypeInfo) where TValue : class
        {
            TValue result = default;

            try
            {
                string response = SendMessage(HttpMessage);

                if (string.IsNullOrEmpty(response))
                    return null;
                   
                result = JsonSerializer.Deserialize(response, jsonTypeInfo);
            }
            catch (Exception)
            {
                return null;
            }

            return result;
        }
    }
}
