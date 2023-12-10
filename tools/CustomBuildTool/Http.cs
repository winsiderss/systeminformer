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
        public static HttpClient CreateAzureClient()
        {
            SocketsHttpHandler httpClientHandler = new SocketsHttpHandler()
            {
                //UseProxy = false,
                AllowAutoRedirect = false,
                AutomaticDecompression = DecompressionMethods.All,
                SslOptions = new SslClientAuthenticationOptions
                {
                    EncryptionPolicy = EncryptionPolicy.RequireEncryption,
                    EnabledSslProtocols = SslProtocols.Tls12 | SslProtocols.Tls13,
                    ApplicationProtocols = new List<SslApplicationProtocol>
                    {
                        SslApplicationProtocol.Http2, SslApplicationProtocol.Http3
                    },
                },
            };

            HttpClient http = new HttpClient(httpClientHandler)
            {
                DefaultRequestVersion = HttpVersion.Version11,
                DefaultVersionPolicy = HttpVersionPolicy.RequestVersionOrHigher,
            };

            http.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("CustomBuildToolSystemInformer", Build.BuildLongVersion));
            http.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));

            return http;
        }

        public static HttpClient CreateGithubClient(string Token)
        {
            SocketsHttpHandler httpClientHandler = new SocketsHttpHandler()
            {
                //UseProxy = false,
                AllowAutoRedirect = false,
                AutomaticDecompression = DecompressionMethods.All,
                SslOptions = new SslClientAuthenticationOptions
                {
                    EncryptionPolicy = EncryptionPolicy.RequireEncryption,
                    EnabledSslProtocols = SslProtocols.Tls12 | SslProtocols.Tls13,
                    ApplicationProtocols = new List<SslApplicationProtocol>
                    {
                        SslApplicationProtocol.Http2, SslApplicationProtocol.Http3
                    },
                },
            };

            HttpClient http = new HttpClient(httpClientHandler)
            {
                DefaultRequestVersion = HttpVersion.Version20,
                DefaultVersionPolicy = HttpVersionPolicy.RequestVersionOrHigher,
            };

            http.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("CustomBuildToolSystemInformer", Build.BuildLongVersion));
            http.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
            http.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Token", Token);

            return http;
        }

        public static HttpClient CreateSourceForgeClient(string Token)
        {
            SocketsHttpHandler httpClientHandler = new SocketsHttpHandler()
            {
                //UseProxy = false,
                AllowAutoRedirect = false,
                AutomaticDecompression = DecompressionMethods.All,
                SslOptions = new SslClientAuthenticationOptions
                {
                    EncryptionPolicy = EncryptionPolicy.RequireEncryption,
                    EnabledSslProtocols = SslProtocols.Tls12 | SslProtocols.Tls13,
                    ApplicationProtocols = new List<SslApplicationProtocol>
                    {
                        SslApplicationProtocol.Http2, SslApplicationProtocol.Http3
                    },
                },
            };

            HttpClient http = new HttpClient(httpClientHandler)
            {
                DefaultRequestVersion = HttpVersion.Version20,
                DefaultVersionPolicy = HttpVersionPolicy.RequestVersionOrHigher,
            };

            http.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("CustomBuildToolSystemInformer", Build.BuildLongVersion));
            http.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
            http.DefaultRequestHeaders.Add("X-ApiKey", Token);

            return http;
        }

        public static string AzureGetString(string RequestUri, AzureAccessToken CurrentAccessToken)
        {
            using (HttpClient httpClient = CreateAzureClient())
            {
                if (CurrentAccessToken != null)
                {
                    httpClient.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue(CurrentAccessToken.Type, CurrentAccessToken.Handle);
                }

                var httpTask = httpClient.GetAsync(RequestUri);
                httpTask.Wait();

                if (!httpTask.Result.IsSuccessStatusCode)
                {
                    return null;
                }

                var httpResult = httpTask.Result.Content.ReadAsStringAsync();
                httpResult.Wait();

                if (!httpResult.IsCompletedSuccessfully || string.IsNullOrWhiteSpace(httpResult.Result))
                {
                    return null;
                }

                return httpResult.Result;
            }
        }

        public static string AzurePostForm(string RequestUri, IEnumerable<KeyValuePair<string, string>> Headers)
        {
            using (HttpClient httpClient = CreateAzureClient())
            using (HttpContent httpContent = new FormUrlEncodedContent(Headers))
            {
                var httpTask = httpClient.PostAsync(RequestUri, httpContent);
                httpTask.Wait();

                if (!httpTask.Result.IsSuccessStatusCode)
                {
                    return null;
                }

                var httpResult = httpTask.Result.Content.ReadAsStringAsync();
                httpResult.Wait();

                if (!httpResult.IsCompletedSuccessfully || string.IsNullOrWhiteSpace(httpResult.Result))
                {
                    return null;
                }

                return httpResult.Result;
            }
        }

        public static string PostString(HttpClient HttpClient, string RequestUri, byte[] Content)
        {
            using (ByteArrayContent httpContent = new ByteArrayContent(Content))
            {
                httpContent.Headers.ContentType = new MediaTypeHeaderValue("application/json");

                var httpTask = HttpClient.PostAsync(RequestUri, httpContent);
                httpTask.Wait();

                if (!httpTask.Result.IsSuccessStatusCode)
                {
                    return null;
                }

                var httpResult = httpTask.Result.Content.ReadAsStringAsync();
                httpResult.Wait();

                if (!httpResult.IsCompletedSuccessfully || string.IsNullOrWhiteSpace(httpResult.Result))
                {
                    return null;
                }

                return httpResult.Result;
            }
        }

        public static string UploadFile(HttpClient HttpClient, string RequestUri, string FileName)
        {
            using (FileStream fileStream = File.OpenRead(FileName))
            using (BufferedStream bufferedStream = new BufferedStream(fileStream))
            using (StreamContent streamContent = new StreamContent(bufferedStream))
            {
                streamContent.Headers.ContentType = new MediaTypeHeaderValue("application/octet-stream");

                var httpUpload = HttpClient.PostAsync(RequestUri, streamContent);
                httpUpload.Wait();

                if (!httpUpload.Result.IsSuccessStatusCode)
                {
                    return null;
                }

                var httpUploadResult = httpUpload.Result.Content.ReadAsStringAsync();
                httpUploadResult.Wait();

                if (!httpUploadResult.IsCompletedSuccessfully || string.IsNullOrWhiteSpace(httpUploadResult.Result))
                {
                    return null;
                }

                return httpUploadResult.Result;
            }
        }

    }
}
