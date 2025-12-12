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
        private static readonly string BaseUrl;
        private static readonly string BaseToken;

        static BuildSourceForge()
        {
            BaseUrl = Win32.GetEnvironmentVariable("SOURCEFORGE_BASE_KEY");
            BaseToken = Win32.GetEnvironmentVariable("SOURCEFORGE_BASE_URL");
        }

        public static SourceForgeUploadResponse UploadReleaseFile(
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

                    var response = BuildHttpClient.SendMessage(requestMessage, SourceForgeUploadResponseContext.Default.SourceForgeUploadResponse);

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
