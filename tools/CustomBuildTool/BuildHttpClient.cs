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
        /// <summary>
        /// Creates a new HttpClient instance with its own isolated HttpClientHandler.
        /// Caller is responsible for disposing the returned HttpClient.
        /// </summary>
        /// <remarks>
        /// Each HttpClient instance has its own handler, connection pool, and cookie container.
        /// This ensures complete isolation between different API calls and prevents
        /// credential/cookie leakage across services.
        /// </remarks>
        public static HttpClient CreateHttpClient()
        {
            var handler = new HttpClientHandler
            {
                AutomaticDecompression = DecompressionMethods.All,
                SslProtocols = SslProtocols.Tls12 | SslProtocols.Tls13,
                UseCookies = true, // Enabled for cookie support per instance
                UseDefaultCredentials = false,
                AllowAutoRedirect = true,
                MaxAutomaticRedirections = 50
            };

            var client = new HttpClient(handler, disposeHandler: true)
            {
                Timeout = TimeSpan.FromSeconds(100),
                DefaultRequestVersion = HttpVersion.Version30,
                DefaultVersionPolicy = HttpVersionPolicy.RequestVersionOrLower
            };
            client.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("CustomBuildTool", "1.0"));

            return client;
        }

        [Obsolete("Use CreateHttpClient() instead and dispose the returned instance.")]
        public static HttpClient GetHttpClient()
        {
            return CreateHttpClient();
        }

        /// <summary>
        /// Sends an HTTP request and returns the response. Caller is responsible for disposing the response.
        /// </summary>
        public static async Task<HttpResponseMessage> SendMessageResponse(HttpRequestMessage HttpMessage, CancellationToken CancellationToken = default)
        {
            HttpResponseMessage response;

            try
            {
                using var client = CreateHttpClient();
                response = await client.SendAsync(HttpMessage, CancellationToken);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[Exception] SendMessageResponse {ex}", ConsoleColor.Red);
                return null;
            }

            return response;
        }

        public static async Task<string> SendMessage(HttpRequestMessage HttpMessage, CancellationToken CancellationToken = default)
        {
            string response;

            try
            {
                using var client = CreateHttpClient();
                using var httpResponse = await client.SendAsync(HttpMessage, CancellationToken);

                if (!httpResponse.IsSuccessStatusCode)
                {
                    Program.PrintColorMessage($"[HTTP Error] {(int)httpResponse.StatusCode} {httpResponse.ReasonPhrase}", ConsoleColor.Yellow);
                }

                response = await httpResponse.Content.ReadAsStringAsync(CancellationToken);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[Exception] SendMessage {ex}", ConsoleColor.Red);
                return null;
            }

            return response;
        }

        public static async Task<TValue> SendMessage<TValue>(HttpRequestMessage HttpMessage, JsonTypeInfo<TValue> jsonTypeInfo, CancellationToken CancellationToken = default) where TValue : class
        {
            TValue result;

            try
            {
                string response = await SendMessage(HttpMessage, CancellationToken);

                if (string.IsNullOrWhiteSpace(response))
                    return null;

                result = JsonSerializer.Deserialize(response, jsonTypeInfo);

                if (result == null)
                {
                    Program.PrintColorMessage("[Warning] JSON deserialization returned null", ConsoleColor.Yellow);
                }
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
            if (content == null)
                throw new ArgumentNullException(nameof(content));

            if (!content.CanRead)
                throw new ArgumentException("Stream must be readable", nameof(content));

            _content = content;
            _bufferSize = bufferSize;
            _progress = progress;

            if (content.CanSeek)
            {
                Headers.ContentLength = content.Length;
            }
        }

        protected override async Task SerializeToStreamAsync(Stream stream, TransportContext context)
        {
            var buffer = new byte[_bufferSize];
            long totalBytesRead = 0;
            int bytesRead;

            while ((bytesRead = await _content.ReadAsync(buffer.AsMemory())) > 0)
            {
                await stream.WriteAsync(buffer.AsMemory(0, bytesRead));
                totalBytesRead += bytesRead;

                if (_content.CanSeek)
                {
                    _progress?.Invoke(totalBytesRead, _content.Length);
                }
                else
                {
                    _progress?.Invoke(totalBytesRead, -1);
                }
            }
        }

        protected override bool TryComputeLength(out long length)
        {
            if (_content.CanSeek)
            {
                length = _content.Length;
                return true;
            }

            length = 0;
            return false;
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
            catch (Exception ex)
            {
                // Avoid throwing from listener - log to debug output only
                System.Diagnostics.Debug.WriteLine($"[HttpEventListener] Error processing event: {ex.Message}");
            }
        }
    }
}