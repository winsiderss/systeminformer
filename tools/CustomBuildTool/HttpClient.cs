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
        public static System.Net.Http.HttpClient CreateHttpClient()
        {
            var httpClientHandler = new HttpClientHandler();
            httpClientHandler.AutomaticDecompression = DecompressionMethods.All;
            httpClientHandler.SslProtocols = SslProtocols.Tls12 | SslProtocols.Tls13;

            var httpClient = new System.Net.Http.HttpClient(httpClientHandler);
            httpClient.DefaultRequestVersion = HttpVersion.Version20;
            httpClient.DefaultVersionPolicy = HttpVersionPolicy.RequestVersionOrHigher;
            httpClient.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("CustomBuildTool", "1.0"));

            return httpClient;
        }

        public static HttpResponseMessage SendMessageResponse(HttpRequestMessage HttpMessage)
        {
            HttpResponseMessage response;

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
            string response;

            try
            {
                var httpTask = CreateHttpClient().SendAsync(HttpMessage).GetAwaiter().GetResult();

                //if (httpTask.IsSuccessStatusCode)

                response = httpTask.Content.ReadAsStringAsync().GetAwaiter().GetResult();
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
            TValue result;

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

    public class ProgressableStreamContent : HttpContent
    {
        private const int DefaultBufferSize = 4096;
        private readonly Stream _content;
        private readonly int _bufferSize;
        private readonly Action<long, long> _progress;

        public ProgressableStreamContent(Stream content, Action<long, long> progress, int bufferSize = DefaultBufferSize)
        {
            _content = content;
            _bufferSize = bufferSize;
            _progress = progress;
            Headers.ContentLength = content.Length;
        }

        protected override async Task SerializeToStreamAsync(Stream stream, TransportContext context)
        {
            var buffer = new byte[_bufferSize];
            var memory = buffer.AsMemory();// new Memory<byte>(buffer);
            long totalBytesRead = 0;
            int bytesRead;

            while ((bytesRead = await _content.ReadAsync(memory)) > 0)
            {
                await stream.WriteAsync(memory);
                totalBytesRead += bytesRead;
                _progress?.Invoke(totalBytesRead, _content.Length);
            }
        }

        protected override bool TryComputeLength(out long length)
        {
            length = _content.Length;
            return true;
        }
    }

}
