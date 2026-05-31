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
    /// Provides static methods for creating isolated HttpClient instances and sending HTTP requests with response
    /// handling and JSON deserialization.
    /// </summary>
    /// <remarks>This class is intended for scenarios where complete isolation between HTTP requests is
    /// required, such as preventing credential or cookie leakage across services. All methods are static and
    /// thread-safe. Callers are responsible for disposing of returned HttpClient and HttpResponseMessage objects to avoid
    /// resource leaks.</remarks>
    public static class BuildHttpClient
    {
        /// <summary>
        /// Creates a new HttpClient instance with its own isolated HttpClientHandler.
        /// Caller is responsible for disposing of the returned HttpClient.
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
                AllowAutoRedirect = true
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

        /// <summary>
        /// Sends an HTTP request and returns the response. Caller is responsible for disposing of the response.
        /// </summary>
        public static async ValueTask<HttpResponseMessage> SendMessageResponse(HttpClient HttpClient, HttpRequestMessage HttpMessage, CancellationToken CancellationToken = default)
        {
            HttpResponseMessage response;

            try
            {
                response = await HttpClient.SendAsync(HttpMessage, CancellationToken);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[Exception] SendMessageResponse: {ex.GetType().Name}: {ex.Message}", ConsoleColor.Red);
                return null;
            }

            return response;
        }

        /// <summary>
        /// Sends an HTTP request and asynchronously deserializes the JSON response to the specified type.
        /// </summary>
        /// <remarks>If the HTTP response is not successful or deserialization returns null, a warning
        /// message is printed to the console. The method returns the default value of TValue in case of
        /// exceptions.</remarks>
        /// <typeparam name="TValue">The type to which the JSON response will be deserialized.</typeparam>
        /// <param name="HttpClient">The HTTP client used to send the request.</param>
        /// <param name="HttpMessage">The HTTP request message to be sent.</param>
        /// <param name="JsonTypeInfo">The metadata used to control JSON deserialization for the target type.</param>
        /// <param name="CancellationToken">A cancellation token that can be used to cancel the operation. Optional.</param>
        /// <returns>A task representing the asynchronous operation. The task result contains the deserialized value of type
        /// TValue, or the default value if deserialization fails.</returns>
        public static async ValueTask<TValue> SendMessage<TValue>(HttpClient HttpClient, HttpRequestMessage HttpMessage, JsonTypeInfo<TValue> JsonTypeInfo, CancellationToken CancellationToken = default)
        {
            try
            {
                using var httpResponse = await HttpClient.SendAsync(HttpMessage, CancellationToken);
                if (!httpResponse.IsSuccessStatusCode)
                {
                    Program.PrintColorMessage($"[HTTP Error] {(int)httpResponse.StatusCode} {httpResponse.ReasonPhrase}", ConsoleColor.Yellow);
                    return default;
                }

                await using var stream = await httpResponse.Content.ReadAsStreamAsync(CancellationToken);
                var result = await JsonSerializer.DeserializeAsync(stream, JsonTypeInfo, CancellationToken);
                if (result == null)
                {
                    Program.PrintColorMessage("[Warning] JSON deserialization returned null", ConsoleColor.Yellow);
                    return default;
                }

                return result;
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[Exception] SendMessage: {ex.GetType().Name}: {ex.Message}", ConsoleColor.Red);
                return default;
            }
        }
    }

    /// <summary>
    /// Provides HTTP content based on a readable stream and reports upload progress during serialization.
    /// </summary>
    /// <remarks>Use this class to send stream data in HTTP requests while tracking progress. Progress updates
    /// are reported via a callback as bytes are written. The stream must be readable and, if seekable, its length is
    /// used for progress reporting and content length headers. This class is useful for uploading large files or
    /// streams where progress feedback is required.</remarks>
    public class ProgressableStreamContent : HttpContent
    {
        private const int DefaultBufferSize = 4096;
        private readonly Stream Content;
        private readonly int BufferSize;
        private readonly Action<long, long> Progress;

        public ProgressableStreamContent(Stream Content, Action<long, long> Progress, int BufferSize = DefaultBufferSize)
        {
            if (Content == null)
                throw new ArgumentNullException(nameof(Content));

            if (!Content.CanRead)
                throw new ArgumentException("Stream must be readable", nameof(Content));

            this.Content = Content;
            this.BufferSize = BufferSize;
            this.Progress = Progress;

            if (Content.CanSeek)
            {
                Headers.ContentLength = Content.Length;
            }
        }

        protected override async Task SerializeToStreamAsync(Stream Stream, TransportContext Context)
        {
            var buffer = new byte[BufferSize];
            var totalBytesRead = 0;
            int bytesRead;

            while ((bytesRead = await Content.ReadAsync(buffer.AsMemory())) > 0)
            {
                await Stream.WriteAsync(buffer.AsMemory(0, bytesRead));
                totalBytesRead += bytesRead;

                if (Content.CanSeek)
                {
                    Progress?.Invoke(totalBytesRead, Content.Length);
                }
                else
                {
                    Progress?.Invoke(totalBytesRead, -1);
                }
            }
        }

        protected override bool TryComputeLength(out long Length)
        {
            if (Content.CanSeek)
            {
                Length = Content.Length;
                return true;
            }

            Length = 0;
            return false;
        }
    }
}

namespace CustomBuildTool
{
    /// <summary>
    /// Provides methods to enable and disable HTTP request and response logging for diagnostic purposes.
    /// </summary>
    /// <remarks>Use this class to start or stop logging of HTTP activity within the application. Logging can
    /// help diagnose issues related to HTTP communication. The logging is global and affects all HTTP requests and
    /// responses handled by the application.</remarks>
    public static class HttpLogging
    {
        private static HttpEventListener _HttpEventListener;
        public static void StartHttpLogging()
        {
            _HttpEventListener ??= new HttpEventListener();
        }
        public static void StopHttpLogging()
        {
            if (_HttpEventListener != null)
            {
                _HttpEventListener.Dispose();
                _HttpEventListener = null;
            }
        }
    }

    //
    // EventSource-based runtime logging from System.Net.Http
    //
    public sealed class HttpEventListener : System.Diagnostics.Tracing.EventListener
    {
        protected override void OnEventSourceCreated(System.Diagnostics.Tracing.EventSource EventSource)
        {
            // System.Net providers: System.Net.Http, System.Net.Sockets, System.Net.NameResolution
            if (
                string.Equals(EventSource.Name, "System.Net.Http", StringComparison.OrdinalIgnoreCase) ||
                string.Equals(EventSource.Name, "System.Net.Sockets", StringComparison.OrdinalIgnoreCase) ||
                string.Equals(EventSource.Name, "System.Net.NameResolution", StringComparison.OrdinalIgnoreCase)
                )
            {
                this.EnableEvents(EventSource, System.Diagnostics.Tracing.EventLevel.Informational);
            }

            base.OnEventSourceCreated(EventSource);
        }

        protected override void OnEventWritten(System.Diagnostics.Tracing.EventWrittenEventArgs EventData)
        {
            try
            {
                var payload = EventData.Payload == null ? string.Empty : string.Join(", ", EventData.Payload);

                Console.WriteLine($"[NET] {EventData.EventSource.Name}:{EventData.Level} {EventData.EventName} | {payload}");
            }
            catch (Exception)
            {
                // Avoid throwing from listener - log to debug output only
                Console.WriteLine($"[HttpEventListener] Error processing event.");
            }
        }
    }
}
