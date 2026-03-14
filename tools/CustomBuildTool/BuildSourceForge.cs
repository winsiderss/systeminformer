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
    /// https://sourceforge.net/p/forge/documentation/Using%20the%20Release%20API/
    /// </summary>
    public static class BuildSourceForge
    {
        /// <summary>
        /// Provides a static instance of the HTTP client used for communicating with SourceForge services.
        /// </summary>
        /// <remarks>This client is intended for reuse to optimize resource usage and performance when
        /// making multiple HTTP requests to SourceForge. Avoid disposing this instance directly.</remarks>
        private static readonly HttpClient SourceForgeHttpClient;
        private static readonly string BaseUrl;
        private static readonly string BaseToken;

        /// <summary>
        /// Initializes static resources required for SourceForge API communication.
        /// </summary>
        /// <remarks>This static constructor sets up the HTTP client and retrieves environment variables
        /// necessary for SourceForge integration. It is automatically invoked before any static members are
        /// accessed.</remarks>
        static BuildSourceForge()
        {
            SourceForgeHttpClient = BuildHttpClient.CreateHttpClient();
            BaseUrl = Win32.GetEnvironmentVariable("SOURCEFORGE_BASE_KEY");
            BaseToken = Win32.GetEnvironmentVariable("SOURCEFORGE_BASE_URL");
        }

        /// <summary>
        /// Uploads a release file to SourceForge and returns the upload response.
        /// </summary>
        /// <remarks>The upload operation requires valid SourceForge API credentials and configuration. If
        /// the upload fails due to missing configuration or an exception, the method returns <see
        /// langword="null"/>.</remarks>
        /// <param name="FileName">The path to the local file to upload. Cannot be null or empty.</param>
        /// <param name="DownloadFileName">The name to assign to the file for download on SourceForge. Cannot be null or empty.</param>
        /// <param name="DownloadButtonName">The label to display on the download button for the uploaded file.</param>
        /// <param name="Delay72Hours">Indicates whether to delay the release by 72 hours before making it available for download. Set to <see
        /// langword="true"/> to delay; otherwise, <see langword="false"/>.</param>
        /// <returns>A <see cref="SourceForgeUploadResponse"/> containing the result of the upload operation, or <see
        /// langword="null"/> if the upload fails or required configuration is missing.</returns>
        public static async Task<SourceForgeUploadResponse> UploadReleaseFile(
            string FileName, 
            string DownloadFileName,
            string DownloadButtonName,
            bool Delay72Hours
            )
        {
            if (string.IsNullOrWhiteSpace(BaseUrl))
                return null;
            if (string.IsNullOrWhiteSpace(BaseToken))
                return null;

            try
            {
                using (FileStream fileStream = File.OpenRead(FileName))
                using (BufferedStream bufferedStream = new BufferedStream(fileStream))
                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Post, $"{BaseUrl}/{DownloadFileName}"))
                {
                    requestMessage.Headers.Add("api_key", BaseToken);
                    //requestMessage.Headers.Add("default", "windows&default=mac&default=linux&default=bsd&default=solaris&default=others&download_label=downloadlabel&name=name");
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
  
                    requestMessage.Content = new StreamContent(bufferedStream);
                    requestMessage.Content.Headers.ContentType = new MediaTypeHeaderValue("application/octet-stream");

                    var response = await BuildHttpClient.SendMessage(SourceForgeHttpClient, requestMessage, SourceForgeUploadResponseContext.Default.SourceForgeUploadResponse);

                    return response;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[BuildSourceForge] " + ex, ConsoleColor.Red);
            }

            return null;
        }
    }
}
