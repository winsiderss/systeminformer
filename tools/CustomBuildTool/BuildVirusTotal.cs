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
    public static class BuildVirusTotal
    {
        private static readonly string BaseToken;

        static BuildVirusTotal()
        {
            BaseToken = Win32.GetEnvironmentVariable("VIRUSTOTAL_BASE_API");
        }

        public static string UploadScanFile(string FileName)
        {
            if (string.IsNullOrWhiteSpace(BaseToken))
               return null;

            var fileInfo = new FileInfo(FileName);
            var upload_url = "https://www.virustotal.com/api/v3/files";

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
                    requestMessage.Headers.Add("x-apikey", BaseToken);

                    var response = BuildHttpClient.SendMessage(requestMessage, VirusTotalResponseContext.Default.VirusTotalLargeUploadResponse);

                    upload_url = response.data;
                }
            }

            if (string.IsNullOrWhiteSpace(upload_url))
            {
                Program.PrintColorMessage($"[BuildVirusTotal] UploadScanFile", ConsoleColor.Red);
                return null;
            }

            try
            {
                VirusTotalAnalysisResponse virusTotalAnalysisResponseContext;

                using (FileStream fileStream = File.OpenRead(FileName))
                using (BufferedStream bufferedStream = new BufferedStream(fileStream))
                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Post, upload_url))
                {
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                    requestMessage.Headers.Add("x-apikey", BaseToken);

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
                            requestAnalysisMessage.Headers.Add("x-apikey", BaseToken);

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
