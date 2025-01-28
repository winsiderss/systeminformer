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
    public static class GithubReleases
    {
        private static readonly string BaseUrl = string.Empty;
        private static readonly string BaseToken = string.Empty;

        static GithubReleases()
        {
            BaseUrl = Win32.GetEnvironmentVariable("GITHUB_MIRROR_API");
            BaseToken = Win32.GetEnvironmentVariable("GITHUB_MIRROR_KEY");
        }

        public static GithubReleasesResponse CreateRelease(string Version)
        {
            if (string.IsNullOrWhiteSpace(BaseUrl))
                return null;
            if (string.IsNullOrWhiteSpace(BaseToken))
                return null;

            try
            {
                var buildUpdateRequest = new GithubReleasesRequest
                {
                    ReleaseTag = Version,
                    Name = Version,
                    Draft = true,
                    GenerateReleaseNotes = true,
                    //Prerelease = true
                };

                byte[] buildPostString = JsonSerializer.SerializeToUtf8Bytes(buildUpdateRequest, GithubReleasesRequestContext.Default.GithubReleasesRequest);

                if (buildPostString.LongLength == 0)
                    return null;

                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Post, $"{BaseUrl}/releases"))
                {
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                    requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Token", BaseToken);

                    requestMessage.Content = new ByteArrayContent(buildPostString);
                    requestMessage.Content.Headers.ContentType = new MediaTypeHeaderValue("application/json");

                    var httpResult = HttpClient.SendMessage(requestMessage, GithubReleasesResponseContext.Default.GithubReleasesResponse);

                    if (httpResult == null)
                    {
                        Program.PrintColorMessage("[CreateRelease-GithubReleasesResponse]", ConsoleColor.Red);
                        return null;
                    }

                    if (string.IsNullOrWhiteSpace(httpResult.UploadUrl))
                    {
                        Program.PrintColorMessage("[CreateRelease-upload_url]", ConsoleColor.Red);
                        return null;
                    }

                    return httpResult;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[CreateRelease] {ex}", ConsoleColor.Red);
            }

            return null;
        }

        public static bool DeleteRelease(string Version)
        {
            if (string.IsNullOrWhiteSpace(BaseUrl))
                return false;
            if (string.IsNullOrWhiteSpace(BaseToken))
                return false;

            try
            {
                HttpResponseMessage responseMessage = null;
                GithubReleasesResponse githubResponseMessage = null;

                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Get, $"{BaseUrl}/releases/tags/{Version}"))
                {
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                    requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Token", BaseToken);

                    responseMessage = HttpClient.SendMessageResponse(requestMessage);

                    if (!responseMessage.IsSuccessStatusCode)
                    {
                        if (responseMessage.StatusCode == HttpStatusCode.NotFound)
                        {
                            return true; // tag doesn't exist
                        }
                        else
                        {
                            Program.PrintColorMessage("[DeleteRelease-GetAsync] " + responseMessage, ConsoleColor.Red);
                            return false;
                        }
                    }
                }

                {
                    var result = responseMessage.Content.ReadAsStringAsync().GetAwaiter().GetResult();

                    if (string.IsNullOrWhiteSpace(result))
                    {
                        Program.PrintColorMessage("[DeleteRelease-ReadAsStringAsync]", ConsoleColor.Red);
                        return false;
                    }

                    githubResponseMessage = JsonSerializer.Deserialize(result, GithubReleasesResponseContext.Default.GithubReleasesResponse);

                    if (githubResponseMessage == null)
                    {
                        Program.PrintColorMessage("[DeleteRelease-GithubReleasesResponse]", ConsoleColor.Red);
                        return false;
                    }
                }

                if (string.IsNullOrWhiteSpace(githubResponseMessage.ReleaseId))
                {
                    Program.PrintColorMessage("[DeleteRelease-ReleaseId]", ConsoleColor.Red);
                    return false;
                }

                // Delete the release (required)
                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Delete, $"{BaseUrl}/releases/{githubResponseMessage.ReleaseId}"))
                {
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                    requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Token", BaseToken);

                    var response = HttpClient.SendMessage(requestMessage);

                    if (string.IsNullOrWhiteSpace(response))
                    {
                        Program.PrintColorMessage("[DeleteRelease-DeleteAsync]", ConsoleColor.Red);
                        return false;
                    }
                }

                // Delete the release tag (optional)
                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Delete, $"{BaseUrl}/git/refs/tags/{Version}"))
                {
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                    requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Token", BaseToken);

                    var response = HttpClient.SendMessage(requestMessage);

                    if (string.IsNullOrWhiteSpace(response))
                    {
                        Program.PrintColorMessage("[DeleteRelease-DeleteAsync]", ConsoleColor.Red);
                        return false;
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[DeleteRelease] " + ex, ConsoleColor.Red);
                //return false; ignore
            }

            return true;
        }

        public static GithubReleasesResponse UpdateRelease(string ReleaseId)
        {
            if (string.IsNullOrWhiteSpace(BaseUrl))
                return null;
            if (string.IsNullOrWhiteSpace(BaseToken))
                return null;

            try
            {
                var buildUpdateRequest = new GithubReleasesRequest
                {
                    Draft = false,
                    //Prerelease = true
                };

                byte[] buildPostString = JsonSerializer.SerializeToUtf8Bytes(buildUpdateRequest, GithubReleasesRequestContext.Default.GithubReleasesRequest);

                if (buildPostString.LongLength == 0)
                    return null;

                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Post, $"{BaseUrl}/releases/{ReleaseId}"))
                {
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                    requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Token", BaseToken);

                    requestMessage.Content = new ByteArrayContent(buildPostString);
                    requestMessage.Content.Headers.ContentType = new MediaTypeHeaderValue("application/json");

                    var response = HttpClient.SendMessage(requestMessage, GithubReleasesResponseContext.Default.GithubReleasesResponse);

                    if (response == null)
                    {
                        Program.PrintColorMessage("[UpdateRelease-GithubReleasesResponse]", ConsoleColor.Red);
                        return null;
                    }

                    if (string.IsNullOrWhiteSpace(response.UploadUrl))
                    {
                        Program.PrintColorMessage("[UpdateRelease-upload_url]", ConsoleColor.Red);
                        return null;
                    }

                    return response;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[UpdateRelease] " + ex, ConsoleColor.Red);
            }

            return null;
        }

        public static GithubAssetsResponse UploadAssets(string Version, string FileName, string UploadUrl)
        {
            string upload_name;
            string upload_url;

            if (string.IsNullOrWhiteSpace(BaseUrl))
                return null;
            if (string.IsNullOrWhiteSpace(BaseToken))
                return null;

            if (!UploadUrl.Contains("{?name,label}", StringComparison.OrdinalIgnoreCase))
                throw new InvalidOperationException();

            upload_name = Path.GetFileName(FileName);
            upload_name = upload_name.Replace("-build-", $"-{Version}-", StringComparison.OrdinalIgnoreCase);
            upload_url = UploadUrl.Replace("{?name,label}", $"?name={upload_name}", StringComparison.OrdinalIgnoreCase);

            try
            {
                using (FileStream fileStream = File.OpenRead(FileName))
                using (BufferedStream bufferedStream = new BufferedStream(fileStream))
                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Post, upload_url))
                {
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                    requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Token", BaseToken);

                    requestMessage.Content = new StreamContent(bufferedStream);
                    requestMessage.Content.Headers.ContentType = new MediaTypeHeaderValue("application/octet-stream");

                    var response = HttpClient.SendMessage(requestMessage, GithubAssetsResponseContext.Default.GithubAssetsResponse);

                    if (response == null)
                    {
                        Program.PrintColorMessage("[UploadAssets-GithubAssetsResponse]", ConsoleColor.Red);
                        return null;
                    }

                    if (!response.Uploaded || string.IsNullOrWhiteSpace(response.DownloadUrl))
                    {
                        Program.PrintColorMessage("[UploadAssets-download_url]", ConsoleColor.Red);
                        return null;
                    }

                    return response;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[UploadAssets] " + ex, ConsoleColor.Red);
            }

            return null;
        }
    }

    public class GithubRelease : IComparable, IComparable<GithubRelease>, IEquatable<GithubRelease>
    {
        public readonly string ReleaseId;
        public List<GithubReleaseAsset> Files;

        public GithubRelease()
        {
            this.ReleaseId = string.Empty;
            this.Files = new List<GithubReleaseAsset>();
        }

        public GithubRelease(string ReleaseId)
        {
            this.ReleaseId = ReleaseId;
            this.Files = new List<GithubReleaseAsset>();
        }

        public bool Valid
        {
            get { return !string.IsNullOrWhiteSpace(this.ReleaseId) && this.Files.Count > 0; }
        }

        public string GetFileUrl(string FileName)
        {
            for (int i = 0; i < this.Files.Count; i++)
            {
                var entry = this.Files[i];

                if (string.Equals(FileName, entry.Filename, StringComparison.OrdinalIgnoreCase))
                {
                    if (!string.IsNullOrWhiteSpace(entry.DownloadUrl))
                        return entry.DownloadUrl;
                }
            }

            return string.Empty;
        }

        public override string ToString()
        {
            return this.ReleaseId;
        }

        public override int GetHashCode()
        {
            return this.ReleaseId.GetHashCode(StringComparison.OrdinalIgnoreCase);
        }

        public override bool Equals(object obj)
        {
            return obj is GithubRelease file && this.ReleaseId.Equals(file.ReleaseId, StringComparison.OrdinalIgnoreCase);
        }

        public bool Equals(GithubRelease other)
        {
            return other != null && this.ReleaseId.Equals(other.ReleaseId, StringComparison.OrdinalIgnoreCase);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
                return 1;

            if (obj is GithubRelease package)
                return string.Compare(this.ReleaseId, package.ReleaseId, StringComparison.OrdinalIgnoreCase);
            else
                return 1;
        }

        public int CompareTo(GithubRelease obj)
        {
            if (obj == null)
                return 1;

            return string.Compare(this.ReleaseId, obj.ReleaseId, StringComparison.OrdinalIgnoreCase);
        }
    }

    public class GithubReleaseAsset(string Filename, string DownloadUrl)
        : IComparable, IComparable<GithubReleaseAsset>, IEquatable<GithubReleaseAsset>
    {
        public string Filename { get; } = Filename;
        public string DownloadUrl { get; } = DownloadUrl;

        public override string ToString()
        {
            return this.Filename;
        }

        public override int GetHashCode()
        {
            return this.Filename.GetHashCode(StringComparison.OrdinalIgnoreCase);
        }

        public override bool Equals(object obj)
        {
            return obj is GithubReleaseAsset file && this.Filename.Equals(file.Filename, StringComparison.OrdinalIgnoreCase);
        }

        public bool Equals(GithubReleaseAsset other)
        {
            return other != null && this.Filename.Equals(other.Filename, StringComparison.OrdinalIgnoreCase);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
                return 1;

            if (obj is GithubReleaseAsset package)
                return string.Compare(this.Filename, package.Filename, StringComparison.OrdinalIgnoreCase);
            else
                return 1;
        }

        public int CompareTo(GithubReleaseAsset obj)
        {
            if (obj == null)
                return 1;

            return string.Compare(this.Filename, obj.Filename, StringComparison.OrdinalIgnoreCase);
        }
    }
}
