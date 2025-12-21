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
    /// Provides methods for interacting with the VirusTotal API for file scanning.
    /// </summary>
    public static class BuildVirusTotal
    {
        /// <summary>
        /// Stores the VirusTotal API token used for authentication.
        /// </summary>
        private static string VirusTotalApiToken = null;

        /// <summary>
        /// Uploads a file to VirusTotal for scanning and retrieves the analysis result.
        /// </summary>
        /// <param name="FileName">The path to the file to be scanned.</param>
        /// <returns>
        /// The analysis result from VirusTotal as a string, or <c>null</c> if the operation fails.
        /// </returns>
        /// <remarks>
        /// The API token is loaded from the path specified by the "VIRUSTOTAL_BASE_API" environment variable.
        /// If the file size exceeds 32MB, a large file upload URL is requested.
        /// </remarks>
        public static string UploadScanFile(string FileName)
        {
            if (string.IsNullOrWhiteSpace(VirusTotalApiToken))
            {
                var fileName = Win32.GetEnvironmentVariable("VIRUSTOTAL_BASE_API");

                if (File.Exists(fileName))
                {
                    VirusTotalApiToken = File.ReadAllText(fileName).Trim();
                }
            }

            if (string.IsNullOrWhiteSpace(VirusTotalApiToken))
            {
                Program.PrintColorMessage($"[BuildVirusTotal] UploadScanFile - No API Token", ConsoleColor.Red);
                return null;
            }

            try
            {
                var fileInfo = new FileInfo(FileName);
                var uploadInfo = "https://www.virustotal.com/api/v3/files";

                if (!fileInfo.Exists)
                {
                    Program.PrintColorMessage($"[BuildVirusTotal] UploadScanFile", ConsoleColor.Red);
                    return null;
                }

                if (fileInfo.Length > 32 * (1024 * 1024))
                {
                    using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Get, "https://www.virustotal.com/api/v3/files/upload_url"))
                    {
                        requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                        requestMessage.Headers.Add("x-apikey", VirusTotalApiToken);

                        var response = BuildHttpClient.SendMessage(requestMessage, VirusTotalResponseContext.Default.VirusTotalLargeUploadResponse);

                        uploadInfo = response.data;
                    }
                }

                if (string.IsNullOrWhiteSpace(uploadInfo))
                {
                    Program.PrintColorMessage($"[BuildVirusTotal] UploadScanFile", ConsoleColor.Red);
                    return null;
                }

                VirusTotalAnalysisResponse virusTotalAnalysisResponseContext;

                using (FileStream fileStream = File.OpenRead(FileName))
                using (BufferedStream bufferedStream = new BufferedStream(fileStream))
                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Post, uploadInfo))
                {
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                    requestMessage.Headers.Add("x-apikey", VirusTotalApiToken);

                    var streamContent = new StreamContent(bufferedStream);
                    var requestMethod = new MultipartFormDataContent
                    {
                        { streamContent, "file", fileInfo.Name }
                    };

                    requestMessage.Content = requestMethod;

                    {
                        virusTotalAnalysisResponseContext = BuildHttpClient.SendMessage(requestMessage, VirusTotalResponseContext.Default.VirusTotalAnalysisResponse);

                        using (HttpRequestMessage requestAnalysisMessage = new HttpRequestMessage(HttpMethod.Get, virusTotalAnalysisResponseContext.data.links.self))
                        {
                            requestAnalysisMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                            requestAnalysisMessage.Headers.Add("x-apikey", VirusTotalApiToken);

                            return BuildHttpClient.SendMessage(requestAnalysisMessage);
                        }
                    }
                }

            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[BuildVirusTotal] " + ex, ConsoleColor.Red);
            }

            return null;
        }
    }
}
