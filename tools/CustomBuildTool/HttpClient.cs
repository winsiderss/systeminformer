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
    public static class HttpClient
    {
        private static System.Net.Http.HttpClient HttpMessageClient = null;
        private static System.Net.Http.HttpClientHandler HttpClientHandler = null;

        public static System.Net.Http.HttpClient CreateHttpClient()
        {
            if (HttpMessageClient == null)
            {
                HttpClientHandler = new HttpClientHandler();
                HttpClientHandler.AutomaticDecompression = DecompressionMethods.All;
                HttpClientHandler.SslProtocols = SslProtocols.Tls12 | SslProtocols.Tls13;

                System.Net.Http.HttpClient httpClient = new System.Net.Http.HttpClient(HttpClientHandler);
                httpClient.DefaultRequestVersion = HttpVersion.Version20;
                httpClient.DefaultVersionPolicy = HttpVersionPolicy.RequestVersionOrHigher;
                httpClient.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("CustomBuildTool", "1.0"));
            }

            return HttpMessageClient;
        }

        public static HttpResponseMessage SendMessageResponse(HttpRequestMessage HttpMessage)
        {
            HttpResponseMessage response = null;

            try
            {
                response = CreateHttpClient().SendAsync(HttpMessage).GetAwaiter().GetResult();
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[Exception] SendMessageResponse {ex}", ConsoleColor.Red);
                return null;
            }

            return response;
        }

        public static string SendMessage(HttpRequestMessage HttpMessage)
        {
            string response = null;

            try
            {
                var httpTask = CreateHttpClient().SendAsync(HttpMessage).GetAwaiter().GetResult();

                if (httpTask.IsSuccessStatusCode)
                {
                    response = httpTask.Content.ReadAsStringAsync().GetAwaiter().GetResult();
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[Exception] SendMessage {ex}", ConsoleColor.Red);
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
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[Exception] SendMessage<TValue> {ex}", ConsoleColor.Red);
                return null;
            }

            return result;
        }
    }
}
