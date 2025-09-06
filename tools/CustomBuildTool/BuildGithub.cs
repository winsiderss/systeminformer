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
    public static class BuildGithub
    {
        private static readonly string BaseToken;
        private static readonly string BaseUrl;

        static BuildGithub()
        {
            BaseToken = Win32.GetEnvironmentVariable("GITHUB_MIRROR_KEY");
            BaseUrl = Win32.GetEnvironmentVariable("GITHUB_MIRROR_API");
        }

        public static GithubReleasesResponse CreateRelease(string Version, bool Draft = true, bool Prerelease = false, bool GenerateReleaseNotes = true)
        {
            if (string.IsNullOrWhiteSpace(BaseUrl))
                return null;
            if (string.IsNullOrWhiteSpace(BaseToken))
                return null;

            try
            {
                GithubReleasesRequest buildUpdateRequest = new GithubReleasesRequest
                {
                    ReleaseTag = Version,
                    Name = Version,
                    Draft = Draft,
                    Prerelease = Prerelease,
                    GenerateReleaseNotes = GenerateReleaseNotes,
                };

                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Post, $"{BaseUrl}/releases"))
                {
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/vnd.github+json"));
                    requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Token", BaseToken);
                    requestMessage.Headers.Add("X-GitHub-Api-Version", "2022-11-28");

                    requestMessage.Content = new ByteArrayContent(buildUpdateRequest.SerializeToBytes());

                    var httpResult = BuildHttpClient.SendMessage(requestMessage, GithubResponseContext.Default.GithubReleasesResponse);

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
                HttpResponseMessage responseMessage;
                GithubReleasesResponse githubResponseMessage;

                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Get, $"{BaseUrl}/releases/tags/{Version}"))
                {
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/vnd.github+json"));
                    requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Token", BaseToken);
                    requestMessage.Headers.Add("X-GitHub-Api-Version", "2022-11-28");

                    responseMessage = BuildHttpClient.SendMessageResponse(requestMessage);

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

                    githubResponseMessage = JsonSerializer.Deserialize(result, GithubResponseContext.Default.GithubReleasesResponse);

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
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/vnd.github+json"));
                    requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Token", BaseToken);
                    requestMessage.Headers.Add("X-GitHub-Api-Version", "2022-11-28");

                    var response = BuildHttpClient.SendMessage(requestMessage);

                    if (string.IsNullOrWhiteSpace(response))
                    {
                        Program.PrintColorMessage("[DeleteRelease-DeleteAsync]", ConsoleColor.Red);
                        return false;
                    }
                }

                // Delete the release tag (optional)
                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Delete, $"{BaseUrl}/git/refs/tags/{Version}"))
                {
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/vnd.github+json"));
                    requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Token", BaseToken);
                    requestMessage.Headers.Add("X-GitHub-Api-Version", "2022-11-28");

                    var response = BuildHttpClient.SendMessage(requestMessage);

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

        public static GithubReleasesResponse UpdateRelease(string ReleaseId, bool Draft = false, bool Prerelease = false)
        {
            if (string.IsNullOrWhiteSpace(BaseUrl))
                return null;
            if (string.IsNullOrWhiteSpace(BaseToken))
                return null;

            try
            {
                GithubReleasesRequest buildUpdateRequest = new GithubReleasesRequest
                {
                    Draft = Draft,
                    Prerelease = Prerelease
                };

                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Post, $"{BaseUrl}/releases/{ReleaseId}"))
                {
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/vnd.github+json"));
                    requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Token", BaseToken);
                    requestMessage.Headers.Add("X-GitHub-Api-Version", "2022-11-28");

                    requestMessage.Content = new ByteArrayContent(buildUpdateRequest.SerializeToBytes());

                    var response = BuildHttpClient.SendMessage(requestMessage, GithubResponseContext.Default.GithubReleasesResponse);

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

        public static GithubAssetsResponse UploadAsset(string ReleaseAssetUrl, string FileName, string Name, string Label)
        {
            string upload_url = null;

            if (string.IsNullOrWhiteSpace(BaseToken))
                return null;
            if (!ReleaseAssetUrl.Contains("{?name,label}", StringComparison.OrdinalIgnoreCase))
                return null;

            upload_url = ReleaseAssetUrl.Replace(
                "{?name,label}",
                $"?name={Uri.EscapeDataString(Name)}&label={Uri.EscapeDataString(Label)}",
                StringComparison.OrdinalIgnoreCase
                );

            try
            {
                using var fileStream = File.OpenRead(FileName);
                using var bufferedStream = new BufferedStream(fileStream);
                using var requestMessage = new HttpRequestMessage(HttpMethod.Post, upload_url);

                requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/vnd.github+json"));
                requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Token", BaseToken);
                requestMessage.Headers.Add("X-GitHub-Api-Version", "2022-11-28");

                requestMessage.Content = new StreamContent(bufferedStream);
                requestMessage.Content.Headers.ContentType = new MediaTypeHeaderValue("application/octet-stream");

                var response = BuildHttpClient.SendMessage(requestMessage, GithubResponseContext.Default.GithubAssetsResponse);
                if (response == null || !response.Uploaded || string.IsNullOrWhiteSpace(response.DownloadUrl))
                {
                    Program.PrintColorMessage("[UploadAssets] upload failed", ConsoleColor.Red);
                    return null;
                }

                return response;
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[UploadAssets] " + ex, ConsoleColor.Red);
                return null;
            }
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

        public GithubReleaseAsset GetDeployInfo(string Name)
        {
            for (int i = 0; i < this.Files.Count; i++)
            {
                var entry = this.Files[i];

                if (string.Equals(entry.DeployFile.Name, Name, StringComparison.OrdinalIgnoreCase))
                {                      
                    return entry;
                }
            }

            return null;
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

    public class GithubReleaseAsset(string Filename, string DownloadUrl, DeployFile DeployFile)
        : IComparable, IComparable<GithubReleaseAsset>, IEquatable<GithubReleaseAsset>
    {
        public string Filename { get; } = Filename;
        public string DownloadUrl { get; } = DownloadUrl;
        public DeployFile DeployFile { get; } = DeployFile;

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
