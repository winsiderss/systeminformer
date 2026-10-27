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
        /// Creates a new HttpClient instance with its own isolated SocketsHttpHandler.
        /// Caller is responsible for disposing of the returned HttpClient.
        /// </summary>
        /// <remarks>
        /// Each HttpClient instance has its own handler, connection pool, and cookie container.
        /// This ensures complete isolation between different API calls and prevents
        /// credential/cookie leakage across services.
        /// </remarks>
        public static HttpClient CreateHttpClient()
        {
            var handler = new SocketsHttpHandler
            {
                AutomaticDecompression = DecompressionMethods.All,
                EnableMultipleHttp2Connections = true,
                EnableMultipleHttp3Connections = true,      
                SslOptions = new SslClientAuthenticationOptions
                {
                    EnabledSslProtocols = SslProtocols.Tls12 | SslProtocols.Tls13,
                }
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
        public static async ValueTask<HttpResponseMessage> SendRequestMessage(
            HttpClient HttpClient, 
            HttpRequestMessage HttpMessage, 
            CancellationToken CancellationToken = default
            )
        {
            return await SendWithRetry(HttpClient, HttpMessage, CancellationToken);
        }

        internal static async Task<HttpResponseMessage> SendWithRetry(
            HttpClient HttpClient,
            HttpRequestMessage HttpMessage,
            CancellationToken CancellationToken = default,
            HttpCompletionOption CompletionOption = HttpCompletionOption.ResponseContentRead,
            Func<TimeSpan, CancellationToken, Task> Delay = null)
        {
            var uri = HttpMessage.RequestUri;
            if (uri != null && !uri.IsAbsoluteUri && HttpClient.BaseAddress != null)
                uri = new Uri(HttpClient.BaseAddress, uri);

            bool retry = uri?.Scheme == Uri.UriSchemeHttps;
            int attempts = retry ? 5 : 1;
            Delay ??= Task.Delay;

            // Capture the body before the first send. Stream-backed content cannot be sent twice.
            byte[] body = null;
            string bodyFile = null;
            if (retry && HttpMessage.Content != null)
            {
                const int memoryLimit = 1024 * 1024;
                if ((HttpMessage.Content.Headers.ContentLength is long length && length > memoryLimit) ||
                    HttpMessage.Content is StreamContent or ProgressableStreamContent)
                {
                    bodyFile = Path.GetTempFileName();
                    try
                    {
                        await using var file = new FileStream(bodyFile, FileMode.Create, FileAccess.Write, FileShare.None);
                        await HttpMessage.Content.CopyToAsync(file, CancellationToken);
                    }
                    catch
                    {
                        File.Delete(bodyFile);
                        throw;
                    }
                }
                else
                {
                    body = await HttpMessage.Content.ReadAsByteArrayAsync(CancellationToken);
                }
            }

            try
            {
                for (int attempt = 1; attempt <= attempts; attempt++)
                {
                    CancellationToken.ThrowIfCancellationRequested();
                    using var request = retry ? CopyRequest(HttpMessage, body, bodyFile) : null;
                    var current = request ?? HttpMessage;

                    try
                    {
                        var response = await HttpClient.SendAsync(current, CompletionOption, CancellationToken);
                        if (response.IsSuccessStatusCode)
                            return response;

                        var status = response.StatusCode;
                        var reason = response.ReasonPhrase;
                        response.Dispose();
                        if (attempt == attempts)
                            throw new HttpRequestException($"HTTP {(int)status} {reason}", null, status);

                        Program.PrintColorMessage($"[HTTP] Attempt {attempt}/{attempts} failed: {(int)status} {reason}. Retrying in 30 seconds.", ConsoleColor.Yellow);
                    }
                    catch (HttpRequestException ex) when (ex.StatusCode == null && attempt < attempts)
                    {
                        Program.PrintColorMessage($"[HTTP] Attempt {attempt}/{attempts} failed: {ex.Message}. Retrying in 30 seconds.", ConsoleColor.Yellow);
                    }
                    catch (TaskCanceledException) when (!CancellationToken.IsCancellationRequested && attempt < attempts)
                    {
                        Program.PrintColorMessage($"[HTTP] Attempt {attempt}/{attempts} timed out. Retrying in 30 seconds.", ConsoleColor.Yellow);
                    }

                    await Delay(TimeSpan.FromSeconds(30), CancellationToken);
                }
            }
            finally
            {
                if (bodyFile != null)
                    File.Delete(bodyFile);
            }

            throw new InvalidOperationException("HTTP retry loop ended without a result.");
        }

        private static HttpRequestMessage CopyRequest(HttpRequestMessage Original, byte[] Body, string BodyFile)
        {
            var request = new HttpRequestMessage(Original.Method, Original.RequestUri)
            {
                Version = Original.Version,
                VersionPolicy = Original.VersionPolicy
            };

            foreach (var header in Original.Headers)
                request.Headers.TryAddWithoutValidation(header.Key, header.Value);
            foreach (var option in Original.Options)
                request.Options.Set(new HttpRequestOptionsKey<object>(option.Key), option.Value);

            if (Original.Content != null)
            {
                request.Content = BodyFile != null
                    ? new StreamContent(new FileStream(BodyFile, FileMode.Open, FileAccess.Read, FileShare.Read))
                    : new ByteArrayContent(Body);
                foreach (var header in Original.Content.Headers)
                    request.Content.Headers.TryAddWithoutValidation(header.Key, header.Value);
            }

            return request;
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
                using var httpResponse = await SendWithRetry(HttpClient, HttpMessage, CancellationToken);
                await using var stream = await httpResponse.Content.ReadAsStreamAsync(CancellationToken);
                var result = await JsonSerializer.DeserializeAsync(stream, JsonTypeInfo, CancellationToken);
                if (result == null)
                {
                    Program.PrintColorMessage("[Warning] JSON deserialization returned null", ConsoleColor.Yellow);
                    return default;
                }

                return result;
            }
            catch (Exception ex) when (ex is not OperationCanceledException)
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
