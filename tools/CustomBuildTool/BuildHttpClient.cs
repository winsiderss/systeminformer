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
    public static class BuildHttpClient
    {
        internal static HttpClientHandler BuildHttpClientHandler = null;
        public static HttpClientHandler CreateHttpHandler()
        {
            BuildHttpClientHandler ??= new HttpClientHandler
            {
                AutomaticDecompression = DecompressionMethods.All,
                SslProtocols = SslProtocols.Tls12 | SslProtocols.Tls13
            };

            return BuildHttpClientHandler;
        }

        internal static HttpClient BuildNetHttpClient = null;
        public static HttpClient CreateHttpClient()
        {
            if (BuildNetHttpClient == null)
            {
                BuildNetHttpClient = new HttpClient(CreateHttpHandler())
                {
                    DefaultRequestVersion = HttpVersion.Version20,
                    DefaultVersionPolicy = HttpVersionPolicy.RequestVersionOrHigher
                };
                BuildNetHttpClient.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("CustomBuildTool", "1.0"));
            }

            return BuildNetHttpClient;
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

                if (string.IsNullOrWhiteSpace(response))
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

namespace CustomBuildTool
{
    public static class HttpLogging
    {
        private static HttpEventListener httpEventListener = null;
        public static void StartHttpLogging()
        {
            if (httpEventListener == null)
            {
                httpEventListener = new HttpEventListener();
            }
        }
        public static void StopHttpLogging()
        {
            if (httpEventListener != null)
            {
                httpEventListener.Dispose();
                httpEventListener = null;
            }
        }
    }

    // EventSource-based runtime logging from System.Net.Http
    public sealed class HttpEventListener : System.Diagnostics.Tracing.EventListener
    {
        protected override void OnEventSourceCreated(System.Diagnostics.Tracing.EventSource eventSource)
        {
            // System.Net providers: System.Net.Http, System.Net.Sockets, System.Net.NameResolution
            if (eventSource.Name == "System.Net.Http" || eventSource.Name == "System.Net.Sockets" || eventSource.Name == "System.Net.NameResolution")
            {
                this.EnableEvents(eventSource, System.Diagnostics.Tracing.EventLevel.Informational);
            }

            base.OnEventSourceCreated(eventSource);
        } 

        protected override void OnEventWritten(System.Diagnostics.Tracing.EventWrittenEventArgs eventData)
        {
            try
            {
                var payload = eventData.Payload == null ? "" : string.Join(", ", eventData.Payload);

                Console.WriteLine($"[NET] {eventData.EventSource.Name}:{eventData.Level} {eventData.EventName} | {payload}");
            }
            catch
            {
                /* avoid throwing from listener */
            }
        }
    }
}